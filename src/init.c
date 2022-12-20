/* J. Kent Wirant
 * 20 Dec. 2022
 * ECE 1895 - Project 3
 * init.c
 * Description:
 *   This code is called directly by the bootloader. It initializes the 
 *   Operating System by loading all necessary drivers and starting the
 *   Command Line.
 */
 
//normally this definition would be in <stdlib.h>, but our OS does not
//have this library.
typedef unsigned long int size_t;

#include <stdint.h>
#include "text_util.h"
#include "string_util.h"
#include "interrupts.h"
#include "isr.h"
#include "keyboard.h"

#define TERMINAL_ROWS 20 //20 rows of 16 bytes

int cursorRow = 0; //32 columns to account for 2 hex digits per byte
int cursorCol = 0;

uint8_t hexBuffer[TERMINAL_ROWS * 16 * 2 + 1]; //account for null space
uint8_t asciiBuffer[TERMINAL_ROWS * 16 + 1]; //account for null space
uint8_t selectedBuffer = 0; //0 for hex, 1 for ascii, 2 for command

uint32_t memLocation = 0x7000;

void updateDisplay(void) {
	//write header to display
	setCursorPosition(0, 0);
	char line[81];
	line[80] = 0;
	uint32_t memData;
	uint32_t memRow = memLocation & ~0xF;
	
	//line 1
	for(int i = 0; i < 80; i++) {
		line[i] = ' ';
	}
	
	printRaw(line); //line 1
	
	//line 2
	strncpy_safe(line, " Address  | ", 12);
	for(int i = 0; i < 16; i++) {
		line[12 + i * 3] = '_';
		intToHexStr(&line[13 + i * 3], i, 1);
		line[14 + i * 3] = ' ';
	}
	strncpy_safe(&line[60], "| 0123456789ABCDEF  ", 20);
	printRaw(line);
	
	//line 3
	for(int i = 0; i < 80; i++) {
		line[i] = '_';
	}
	printRaw(line);
	
	//lines 4-23
	for(int i = 0; i < TERMINAL_ROWS; i++) {
		line[0] = ' ';
		intToHexStr(&line[1], memRow + i * 16, 8);
		strncpy_safe(&line[8], "_ | ", 4);
		
		for(int j = 0; j < 4; j++) {
			memData = *((uint32_t *)(memRow + i * 16 + j * 4));
			
			for(int k = 0; k < 4; k++) {
				//hex portion of display
				intToHexStr(&line[12 + 3 * (j * 4 + k)], memData, 2);
				line[14 + 3 * (j * 4 + k)] = ' ';
								
				//ascii portion of display
				if((memData & 0xFF) >= 0x20 && (memData & 0xFF) < 0x7F) {
					line[62 + j * 4 + k] = memData & 0xFF;
				}
				else {
					line[62 + j * 4 + k] = '.'; //filler character
				}
				
				memData >>= 8;
			}
		}
		
		line[60] = '|';
		line[61] = ' ';
		line[78] = ' ';
		line[79] = ' ';
		
		printRaw(line);
	}
	
	//convert cursor position to screen coordinates
	char row = cursorRow + 3;
	char col;
	
	if(selectedBuffer == 0) { //hex: formatted as "## ## ## ..."
		col = 12 + cursorCol / 2 * 3 + (cursorCol & 1);
	}
	else if(selectedBuffer == 1) { //ascii: formatted as "####...####"
		col = 62 + cursorCol / 2; //ascii offset
	}
	
	highlight(row, col);
}

void updateMemory(void) {
	//The terminal program runs in the address range of
	//0x7800 to 0xA800. It has been made read-only to avoid 
	//unintentional writes from viewing the range.
	if(memLocation >= 0x7800 && memLocation < 0xA800)
		return;
	
	if(selectedBuffer == 0) { //hex buffer
		for(int i = 0; i < 16 * TERMINAL_ROWS; i++) {
			((uint8_t *)memLocation)[i] = hexStrToInt(&hexBuffer[i*2], 2);
		}
	}
	else if(selectedBuffer == 1) { //ascii buffer
		//update only one row 
		for(int i = 0; i < 16 * TERMINAL_ROWS; i++) {
			((uint8_t *)memLocation)[i] = asciiBuffer[i];
		}
	}
}

void keyboardHandler(uint8_t c, uint8_t keyCode, uint16_t flags) {
	
	//ensure memory-buffer coherency
	uint8_t memData;
		
	for(int i = 0; i < 16 * TERMINAL_ROWS; i++) {
		memData = ((uint8_t *)memLocation)[i];
		intToHexStr(&hexBuffer[i*2], memData, 2);
		asciiBuffer[i] = memData;
	}
	
	//change cursor if an arrow key was pressed
	
	if(c == 0x81) { //up arrow
		cursorCol &= ~1; //first hex digit, if applicable
		cursorRow--;
	}
	else if(c == 0x83) { //left arrow
		cursorCol &= ~1; //first hex digit, if applicable
		cursorCol -= 2; //move by byte position
	}
	else if(c == 0x84) { //right arrow
		cursorCol &= ~1; //first hex digit, if applicable 
		cursorCol += 2; //move by byte position
	}
	else if(c == 0x86) { //down arrow
		cursorCol &= ~1; //first hex digit, if applicable
		cursorRow++;
	}
	else { //type a character
		if(selectedBuffer == 0) { //typed in hex buffer
			//hex chars only
			if(c >= '0' && c <= '9') {
				hexBuffer[32*cursorRow + cursorCol] = c;
				updateMemory();
				cursorCol++;
			}
			else if((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
				hexBuffer[32*cursorRow + cursorCol] = c & ~0x20;
				updateMemory();
				cursorCol++;
			}
			
			//go to next row within selected buffer
			if(cursorCol >= 32) {
				cursorCol = 0;
				cursorRow++;
			}
		}
		else if(selectedBuffer == 1) { //typed in ascii buffer
			//any displayable character
			if(c >= 0x20 && c < 0x7F) {
				asciiBuffer[16*cursorRow + cursorCol/2] = c;
				updateMemory();
				cursorCol &= ~1; //should be even index for ascii buffer
				cursorCol += 2; //move by byte position
			}
			
			//go to next row within selected buffer
			if(cursorCol >= 32) {
				cursorCol = 0;
				cursorRow++;
			}
		}
	}
	
	//handle cursor out of bounds
	if(cursorRow >= TERMINAL_ROWS) { //scroll down
		memLocation += 16;
		cursorRow = TERMINAL_ROWS - 1;
	}
	else if(cursorRow < 0) { //scroll up
		memLocation -= 16;
		cursorRow = 0;
	}
	else if(cursorCol < 0) { //switch to hex or don't move
		if(selectedBuffer == 1) {
			selectedBuffer = 0;
			cursorCol = 30;
		}
		else if(selectedBuffer == 0) {
			cursorCol = 0;
		}
	}
	else if(cursorCol >= 32) { //switch to ascii or don't move
		if(selectedBuffer == 0) {
			selectedBuffer = 1;
			cursorCol = 0;
		}
		else if(selectedBuffer == 1) {
			cursorCol = 30;
		}
	}
	
	char str[8];
	str[0] = '>';
	str[1] = ' ';
	str[2] = c;
	str[3] = ' ';
	intToHexStr(&str[4], c, 2);
	str[6] = 0;
	setCursorPosition(24, 0);
	printRaw(str);
	
	updateDisplay();
}

//entry point from bootloader
void _start(void) {
	clearScreen();
	setInterruptDescriptor(isr_keyboard, 0x21, 0);
	loadIdt();
	pic_init();
	keyboard_init(keyboardHandler);
	updateDisplay();
	while(1); //hang
}

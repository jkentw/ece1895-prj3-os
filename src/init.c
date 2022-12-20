/* J. Kent Wirant
 * 19 Dec. 2022
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

#define TERIMINAL_ROWS 20 //20 rows of 16 bytes

int cursorRow = 0; //32 columns to account for 2 hex digits per byte
int cursorCol = 0;

char hexBuffer[TERIMINAL_ROWS * 16 * 2];
char asciiBuffer[TERIMINAL_ROWS * 16];
uint8_t selectedBuffer = 0; //0 for hex, 1 for ascii

uint32_t memLocation = 0x7C00; //start of code

void updateDisplay(void) {
	//write header to display
	setCursorPosition(0, 0);
	char line[81];
	line[80] = 0;
	uint32_t memData;
	uint32_t memRow = memLocation & ~0xF;
	
	//lines 1-2
	for(int i = 0; i < 80; i++) {
		line[i] = ' ';
	}
	
	printRaw(line); //line 1
	printRaw(line); //line 2
	
	//line 3
	strncpy_safe(line, " Address  | ", 12);
	for(int i = 0; i < 16; i++) {
		line[12 + i * 3] = '_';
		intToHexStr(&line[13 + i * 3], i, 1);
		line[14 + i * 3] = ' ';
	}
	strncpy_safe(&line[60], "| 0123456789ABCDEF  ", 20);
	printRaw(line);
	
	//line 4
	for(int i = 0; i < 80; i++) {
		line[i] = '_';
	}
	printRaw(line);
	
	//lines 5-24
	for(int i = 0; i < TERIMINAL_ROWS; i++) {
		line[0] = ' ';
		intToHexStr(&line[1], memRow + i * 16, 8);
		strncpy_safe(&line[8], "_ | ", 4);
		
		for(int j = 0; j < 4; j++) {
			memData = *((uint32_t *)(memRow + i * 16 + j * 4));
			
			for(int k = 0; k < 4; k++) {
				//hex portion
				intToHexStr(&line[12 + 3 * (j * 4 + k)], memData, 2);
				line[14 + 3 * (j * 4 + k)] = ' ';
				
				//ascii portion
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
	char row = cursorRow + 4;
	char col;
	
	if(selectedBuffer == 0) { //hex: formatted as "## ## ## ..."
		col = 12 + cursorCol / 2 * 3 + (cursorCol & 1);
	}
	else { //ascii: formatted as "################"
		col = 62 + cursorCol / 2; //ascii offset
	}
	
	highlight(row, col);
}

void updateMemory(void) {
	updateDisplay();
}

void keyboardHandler(uint8_t c, uint8_t keyCode, uint16_t flags) {
	//up arrow key is 0x81
	//left arrow key is 0x83
	//right arrow key is 0x84
	//down arrow key is 0x86
	
	if(c == 0x81) {
		cursorRow--;
	}
	else if(c == 0x83) {
		cursorCol -= 2; //move by byte position		
	}
	else if(c == 0x84) {
		cursorCol += 2; //move by byte position
	}
	else if(c == 0x86) {
		cursorRow++;
	}
	else {
		if(selectedBuffer == 0) {
			//hex chars only
			if(c >= '0' && c <= '9') {
				hexBuffer[20*cursorRow + cursorCol] = c;
				cursorCol++;
			}
			else if((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
				hexBuffer[32*cursorRow + cursorCol] = c & ~0x20;
				cursorCol++;
			}
			
			//go to next row within selected buffer
			if(cursorCol >= 32) {
				cursorCol = 0;
				cursorRow++;
			}
		}
		else {
			//any displayable character
			if(c >= 0x20 && c < 0x7F) {
				asciiBuffer[16*cursorRow + cursorCol/2] = c;
				cursorCol += 2;
			}
			
			//go to next row within selected buffer
			if(cursorCol >= 32) {
				cursorCol = 0;
				cursorRow++;
			}
		}
	}
	
	//handle cursor out of bounds
	if(cursorRow >= TERIMINAL_ROWS) { //scroll down
		memLocation += 16;
		cursorRow = TERIMINAL_ROWS - 1;
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
		else {
			cursorCol = 0;
		}
	}
	else if(cursorCol >= 32) { //switch to ascii or don't move
		if(selectedBuffer == 0) {
			selectedBuffer = 1;
			cursorCol = 0;
		}
		else {
			cursorCol = 30;
		}
	}
	
	char str[5];
	str[0] = c;
	str[1] = ' ';
	intToHexStr(&str[2], c, 2);
	str[4] = 0;
	setCursorPosition(24, 0);
	printRaw(str);
	
	updateMemory();
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

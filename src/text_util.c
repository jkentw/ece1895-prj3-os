/* J. Kent Wirant
 * 10 Dec. 2022
 * ECE 1895 - Project 3
 * text_util.h
 * Description:
 *   Provides text utility functions, including string manipulation
 *   and display.
 */
 
#include "text_util.h"

static short *VIDEO_TEXT = (short *)0x000B8000;
static const int VIDEO_TEXT_LENGTH = NUM_COLS * NUM_ROWS * 2;

//consider saving state as a struct to allow multiple display instances
static char bgColor = COLOR_BLACK; //background
static char fgColor = COLOR_WHITE; //foreground
static int cursorPos = 0;

short *getCursorAddress(void) {
	return (void *)VIDEO_TEXT + cursorPos * 2;
}

void setCursorPosition(char row, char col) {
	if(row < NUM_ROWS && col < NUM_COLS && row > 0 && col > 0) {
		cursorPos = row * NUM_COLS + col;
	}
}

void setTextColor(char foreground, char background) {
	bgColor = background;
	fgColor = foreground;
}

void printRaw(const char *str) {
	char color = bgColor << 4 | fgColor;
	
	while(*str && cursorPos < VIDEO_TEXT_LENGTH / 2) {
		VIDEO_TEXT[cursorPos] = color << 8 | *str;
		str++;
		cursorPos++;
	}
}

void clearScreen(void) {
	int fillValue = bgColor << 28 | ' ' << 16 | bgColor << 12 | ' ';
	
	for(int i = 0; i < VIDEO_TEXT_LENGTH / 4; i++) {
		((int*)VIDEO_TEXT)[i] = fillValue;
	}
}

/* J. Kent Wirant
 * 08 Dec. 2022
 * ECE 1895 - Project 3
 * Initialization Code
 * Description:
 *   This code is called directly by the bootloader. It initializes the 
 *   Operating System by loading all necessary drivers and starting the
 *   Command Line.
 */

//normally this definition would be in <stdlib.h>, but our OS does not
//have this library.
typedef unsigned long int size_t;

#include "text_util.h"
#include "string_util.h"
 
//entry point from bootloader
void _start(void) {
	const char *str1 = "Text Utilities Test: ";
	const char *str2 = "cyan text";
	const char *str3 = "fancier center text";
	const char *str4 = "end of the line";
	
	clearScreen();
	printRaw(str1);
	setTextColor(COLOR_LIGHT_CYAN, COLOR_BLACK);
	printRaw(str2);
	setTextColor(COLOR_WHITE, COLOR_MAGENTA);
	setCursorPosition(NUM_ROWS / 2, (NUM_COLS - strlen(str3)) / 2);
	printRaw(str3);
	setTextColor(COLOR_RED, COLOR_BLACK);
	setCursorPosition(NUM_ROWS - 1, NUM_COLS - strlen(str4));
	printRaw(str4);
	
	while(1); //hang
}

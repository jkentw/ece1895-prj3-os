/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * isr.c
 * Description: Contains all the interrupt service routines (ISRs). 
 *   These must be compiled with different options, so they must be
 *   in their own .c file.
 */
 
#include "isr.h"
#include "text_util.h"
#include "string_util.h"

INTERRUPT_HANDLER void isr_test(struct interrupt_frame *f) {
	clearScreen();
	const char *str = "Interrupt :)";
	setTextColor(COLOR_YELLOW, COLOR_BLUE);
	setCursorPosition(NUM_ROWS / 4, (NUM_COLS - strlen(str)) / 2);
	printRaw(str);
}

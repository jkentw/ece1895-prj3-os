/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * isr.c
 * Description: Contains all the interrupt service routines (ISRs). 
 *   These must be compiled with different options, so they must be
 *   in their own .c file.
 */
 
#include "isr.h"
#include "interrupts.h"
#include "text_util.h"
#include "string_util.h"

INTERRUPT_HANDLER void isr_test(struct interrupt_frame *f) {
	const char *str = "Interrupt :)";
	setTextColor(COLOR_YELLOW, COLOR_BLUE);
	setCursorPosition(NUM_ROWS / 4, (NUM_COLS - strlen(str)) / 2);
	printRaw(str);
}

INTERRUPT_HANDLER void isr_keyboard(struct interrupt_frame *f) {
	const char *str = "Keyboard ";
	setTextColor(COLOR_LIGHT_GREEN, COLOR_BLUE);
	printRaw(str);
	pic_eoi(1);
}

//NOTE: for PIC vectors 7 and 15, make sure to check for spurrious IRQs.

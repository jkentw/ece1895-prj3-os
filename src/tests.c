/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * interrupts.c
 * Description: Test functions for various OS features.
 */

#include "tests.h"
#include "text_util.h"
#include "string_util.h"
#include "interrupts.h"
#include "isr.h"

void test_textUtils1(void) {
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
}

void test_interrupts1(void) {
	setInterruptDescriptor(*isr_test, 32, 0);
	loadIdt();
	asm volatile ("int %0" :: "g" (32) : "memory");
}


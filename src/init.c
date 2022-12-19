/* J. Kent Wirant
 * 14 Dec. 2022
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

#include "tests.h"
 
//entry point from bootloader
void _start(void) {
	test_keyboard1();
	while(1); //hang
}

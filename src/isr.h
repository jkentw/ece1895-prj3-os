/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * isr.h
 * Description: Contains all the interrupt service routines (ISRs). 
 *   These must be compiled with different options, so they must be
 *   in their own .c file.
 */
 
#ifndef ISR_H
#define ISR_H

#include "interrupts.h"
#define INTERRUPT_HANDLER __attribute__((interrupt))

INTERRUPT_HANDLER void isr_test(struct interrupt_frame *f);

#endif //ISR_H

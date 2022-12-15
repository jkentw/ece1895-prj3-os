/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * interrupts.h
 * Description: Sets up Interrupt Descriptor Table (IDT) and
 *   Programmable Interrupt Controller (PIC).
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h> //fixed-size integer types

struct interrupt_frame;

void setInterruptDescriptor(void (*isr)(struct interrupt_frame *),
  uint8_t index, uint8_t isException);

void loadIdt(void);

void pic_init(void);
void pic_eoi(uint8_t irqLine);

#endif

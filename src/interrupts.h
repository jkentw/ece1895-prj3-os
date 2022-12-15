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

struct interrupt_frame;/* {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
} __attribute__((packed));*/

void setInterruptDescriptor(void (*isr)(struct interrupt_frame *),
  uint8_t index, uint8_t isException);

void loadIdt(void);

#endif

/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * interrupts.c
 * Description: Sets up Interrupt Descriptor Table (IDT) and
 *   Programmable Interrupt Controller (PIC).
 */

#include "interrupts.h"

//for simplicity, everything will run in ring 0 (highest privilege)

//reference: https://wiki.osdev.org/Interrupt_Descriptor_Table
struct InterruptDescriptor {
	uint16_t offset0;   //first 16 bits of ISR address
	uint16_t segment;   //index into GDT (bit 2=0), bits 1-0 are ring #
	uint8_t reserved;   //set to 0
	uint8_t attributes; //present (1b), ring # (2b), 0, gate type (4b)
	uint16_t offset1;   //last 16 bits of ISR address
};

struct InterruptDescriptor IDT[256];

struct idtDescriptor {
	uint16_t size;
	uint16_t offset0;
	uint16_t offset1;
} idtd;

void setInterruptDescriptor(void (*isr)(struct interrupt_frame *),
  uint8_t index, uint8_t isException) {

	struct InterruptDescriptor *entry = &IDT[index];
	entry->offset0 = (uint32_t) isr & 0x0000FFFF;
	entry->offset1 = (uint32_t) isr >> 16;
	entry->attributes = 0x8E | (isException != 0); //ring 0 and 32-bit
	entry->segment = 1 << 3; //GDT segment selector index (ring 0 code)
	entry->reserved = 0;
}

void loadIdt(void) {
	idtd.size = 256 * 8 - 1;
	idtd.offset0 = (uint32_t) &IDT & 0x0000FFFF;
	idtd.offset1 = (uint32_t) &IDT >> 16;

	asm volatile ("lidt %0" : : "m" (idtd));
}



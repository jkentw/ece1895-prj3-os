/* J. Kent Wirant
 * 15 Dec. 2022
 * ECE 1895 - Project 3
 * interrupts.c
 * Description: Sets up Interrupt Descriptor Table (IDT) and
 *   Programmable Interrupt Controller (PIC).
 */

#include "interrupts.h"
#include "x86_util.h"

#define PIC0_CMD_STAT 0x20 //primary PIC command/status I/O port
#define PIC0_IMR_DATA 0x21 //primary interrupt mask register/data register
#define PIC1_CMD_STAT 0xA0 //secondary PIC command/status I/O port
#define PIC1_IMR_DATA 0xA1 //secondary PIC interrupt mask register/data register

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

//source: http://www.brokenthorn.com/Resources/OSDevPic.html
void pic_init(void) {
	//disable APIC
	uint32_t apicBase;
	uint32_t unused;
	x86_readMSR(0x1B, &apicBase, &unused);
	x86_writeMSR(0x1B, apicBase & 0xFFFF0000, 0);
	
	//initialization control words
	int icw1   = 0x11; //initialization word
	int icw2_0 = 0x20; //PIC0 maps to interrupt vectors 0x20-0x27
	int icw2_1 = 0x28; //PIC1 maps to interrupt vectors 0x28-0x2F
	int icw3_0 = 0x04; //tell PIC0 to cascade with PIC1 on IRQ line 2 (bit 2)
	int icw3_1 = 0x02; //tell PIC1 that it's cascaded with PIC0 on line 2 (value is 2)
	int icw4   = 0x01; //the PICs are part of the x86 architecture
	
	//some delay might be required after each I/O operation
	
	x86_outb(PIC0_CMD_STAT, icw1); //initialization command
	x86_outb(PIC1_CMD_STAT, icw1);
	x86_outb(PIC0_IMR_DATA, icw2_0); //map interrupt vectors
	x86_outb(PIC1_IMR_DATA, icw2_1);
	x86_outb(PIC0_IMR_DATA, icw3_0); //master PIC cascade config
	x86_outb(PIC1_IMR_DATA, icw3_1); //slave PIC cascade config
	x86_outb(PIC0_IMR_DATA, icw4); //x86
	x86_outb(PIC1_IMR_DATA, icw4); //x86
	
	//enable only keyboard and PIC1 interrupts
	x86_outb(PIC0_IMR_DATA, ~0x06);
	x86_outb(PIC1_IMR_DATA, ~0x00);
}

//end of interrupt
void pic_eoi(uint8_t irqLine) {
	if(irqLine >= 8) //send to slave only if IRQ came from it
		x86_outb(PIC1_CMD_STAT, 0x20); //EOI code is 0x20
	x86_outb(PIC0_CMD_STAT, 0x20); //send to master regardless
}


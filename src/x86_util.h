/* J. Kent Wirant
 * 14 Dec. 2022
 * ECE 1895 - Project 3
 * x86_util.h
 * Description: Utility functions in x86 assembly.
 */

#ifndef X86_UTIL_H
#define X86_UTIL_H

#include <stdint.h>

uint8_t x86_inb(uint16_t port);
uint16_t x86_inw(uint16_t port);
uint32_t x86_ind(uint16_t port);

void x86_outb(uint16_t port, uint8_t data);
void x86_outw(uint16_t port, uint16_t data);
void x86_outd(uint16_t port, uint32_t data);

#endif //X86_UTIL_H

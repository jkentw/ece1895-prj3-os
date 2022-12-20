/* J. Kent Wirant
 * 19 Dec. 2022
 * ECE 1895 - Project 3
 * keyboard.h
 * Description: PS/2 keyboard driver plus enabling A20.
 */
 
//referenced https://wiki.osdev.org/PS/2_Keyboard
//referenced https://wiki.osdev.org/%228042%22_PS/2_Controller

#include <stdint.h>

//abstracts the command interface for the keyboard controller
enum CommandID {
	CMD_SCAN_CODE_SET //others possible
};

//function prototypes
uint8_t keyboard_queueCommand(enum CommandID id, uint8_t data);
void keyboard_init(void (*handler)(uint8_t, uint8_t, uint16_t));
uint8_t keyboard_checkInput(void);

/* J. Kent Wirant
 * 19 Dec. 2022
 * ECE 1895 - Project 3
 * keyboard.c
 * Description: PS/2 keyboard driver plus enabling A20.
 */
 
//referenced https://wiki.osdev.org/PS/2_Keyboard
//referenced https://wiki.osdev.org/%228042%22_PS/2_Controller

#include <stdint.h>
#include "x86_util.h"
#include "text_util.h"
#include "string_util.h"

#define KEYBOARD_CMD_QUEUE_SIZE 16

const uint16_t DATA_PORT = 0x60;
const uint16_t CMD_STAT_PORT = 0x64;

const uint8_t RESPONSE_ERR0              = 0x00;
const uint8_t RESPONSE_SELF_TEST_PASSED  = 0xAA;
const uint8_t RESPONSE_ECHO              = 0xEE;
const uint8_t RESPONSE_ACK               = 0xF0;
const uint8_t RESPONSE_SELF_TEST_FAILED0 = 0xFC;
const uint8_t RESPONSE_SELF_TEST_FAILED1 = 0xFD;
const uint8_t RESPONSE_RESEND            = 0xFE;
const uint8_t RESPONSE_ERR1              = 0xFF;

//for scan code set 1
enum KeyboardState {
	STATE_START,
	STATE_AWAITING_RESPONSE,
	STATE_EXTENDED_CODE,
	STATE_PRTSC_PR1,
	STATE_PRTSC_PR2,
	STATE_PRTSC_RL1,
	STATE_PRTSC_RL2,
	STATE_PAUSE,
	STATE_PAUSE_PR,
	STATE_PAUSE_RL
} keyboardState = STATE_START;

//move to header file
//abstracts the command interface for the keyboard controller
enum CommandID {
	CMD_SCAN_CODE_SET //others possible
};

struct Command {
	enum CommandID cmdId;
	uint8_t data;
};

/* keyFlags:
 *   bit 0 - set if key pressed, clear if released
 *   bit 1 - set if CAPS is on
 *   bit 2 - set if NumLk is on
 *   bit 3 - set if ScrLk is on
 *   bit 4 - set if Shift is down
 *   bit 5 - set if Ctrl is down
 *   bit 6 - set if Alt is down
 *   bit 7 - set if Fn is down
 *   bits 8 to 15 - reserved; should be clear
 */
uint16_t keyFlags = 0;

//callback function
void (*keyEventHandler)(char c, uint8_t keyCode, uint16_t flags) = 0;

//use scan code set 1 (if possible)
//amend or implement this
//uint8_t scanCodeTable[256];

struct Command cmdQueue[KEYBOARD_CMD_QUEUE_SIZE];
uint32_t queueStart = 0;
uint32_t queueLength = 0;

//function prototypes
uint8_t keyboard_queueCommand(enum CommandID id, uint8_t data);
void processScanCode(uint8_t scancode);
void tryCommand(void);
uint8_t keyboard_checkInput(void);
void keyboard_init(void (*handler)(char, uint8_t, uint16_t));

//returns true if sucessfully added to queue. Does not validate command.
//automatically tries to execute (next) command if possible
uint8_t keyboard_queueCommand(enum CommandID id, uint8_t data) {
	//if queue is not full, assign new element
	uint8_t hasRoom = (queueLength < KEYBOARD_CMD_QUEUE_SIZE);
	
	if(hasRoom) {
		int idx = (queueStart + queueLength) % KEYBOARD_CMD_QUEUE_SIZE;
		struct Command *cmd = &cmdQueue[idx];
		cmd->cmdId = id;
		cmd->data = data;
		queueLength++;
	}
	
	//attempt to process first command in the queue
	tryCommand();
	return hasRoom;
}

void processScanCode(uint8_t scancode) {
	//TODO: implement scan code translation and states
	//handle keyflags too
	
	if(keyEventHandler != 0)
		keyEventHandler('#', scancode, keyFlags);
		
	keyboardState = STATE_START;
}

void tryCommand(void) {
	if(keyboardState != STATE_START) //only process command when ready
		return;
	
	if(queueLength > 0) {
		int idx = (queueStart + queueLength) % KEYBOARD_CMD_QUEUE_SIZE;
		struct Command *cmd = &cmdQueue[idx];
				
		switch(cmd->cmdId) {
			//does not work as intended yet
			case CMD_SCAN_CODE_SET:
				//set scan code set to 1
				keyboardState = STATE_AWAITING_RESPONSE;	
				x86_outb(DATA_PORT, 0xF0);
				x86_outb(DATA_PORT, 0x01);	
				break;
		}
	}
}

//returns true if input was found and processed
uint8_t keyboard_checkInput(void) {
	//read status register for output buffer status
	uint8_t isFull = x86_inb(CMD_STAT_PORT) & 1;
	
	if(isFull) {
		uint8_t data = x86_inb(DATA_PORT);
		
		//NOTE: command response is not fully tested
		if(keyboardState == STATE_AWAITING_RESPONSE) {
			keyboardState = STATE_START; //no longer awaiting response
			
			if(data == RESPONSE_ACK) { //if acknowledged, remove cmd from queue		
				queueLength--;
				queueStart = (queueStart + 1) % KEYBOARD_CMD_QUEUE_SIZE;
			}
			else if(data == RESPONSE_RESEND) { //resend last command
				tryCommand();
			} 
			else {
				//return to start state for robustness	
				keyboardState = STATE_START;
			}
				
			/* no action needed yet for:
				RESPONSE_ERR0
				RESPONSE_ERR1
				RESPONSE_SELF_TEST_FAILED0
				RESPONSE_SELF_TEST_FAILED1
				RESPONSE_SELF_TEST_PASSED
				RESPONSE_ECHO 
			*/	
		}
		else { //received key input
			processScanCode(data);
		}
	}
	
	return isFull;
}

void keyboard_init(void (*handler)(char, uint8_t, uint16_t)) {
	//TODO: reset keyboard & check status
	
	keyEventHandler = handler;
	keyboardState = STATE_START;
	
	//TODO: enable A20
}



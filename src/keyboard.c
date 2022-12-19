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
#include "keyboard.h"

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

struct Command {
	enum CommandID cmdId;
	uint8_t data;
};

//uses scan code set 1
char scanCodeTable[] = {
	0x00, 0x1B, '1',  '2',
	'3',  '4',  '5',  '6',
	'7',  '8',  '9',  '0',
	'-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',
	't',  'y',  'u',  'i',
	'o',  'p',  '[',  ']',
	'\n', 0x02, 'a',  's',
	'd',  'f',  'g',  'h',
	'j',  'k',  'l',  ';',
	'\'', '`',  0x03, '\\',
	'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',
	'.',  '/',  0x04, '*',
	0x05, ' ',  0x06, 0x07,
	0x0E, 0x0F, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15,
	0x16, 0x17, 0x18, '7', //keypad entries
	'8',  '9',  '-',  '4',
	'5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',
	0x00, 0x00, 0x00, 0x19,
	0x1A, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x1C, 0x00, 0x00, 0x00, //extended key codes; add 0x50 to access
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x1D, 0x00, 0x00,
	'\n', 0x1E, 0x00, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, //0xFF -> key exists but is unimplemented
	0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0x00,
	0xFF, 0x00, 0xFF, 0x00,
	0x00, '/',  0x00, 0x00,
	0x1F, 0x00, 0x00, 0x00, //0x38
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x80,
	0x81, 0x82, 0x00, 0x83,
	0x00, 0x84, 0x00, 0x85,
	0x86, 0x87, 0x88, 0x7F,
	0x00, 0x00, 0x00, 0x00, //0x54
	0x00, 0x00, 0x00, 0x89,
	0x8A, 0x8B, 0x8C, 0x8D,
	0x00, 0x00, 0x00, 0x8E,
	0x00, 0x8F, 0x90, 0x91, //0x64
	0x92, 0x93, 0x94, 0x95,
	0x96, 0x97, 0x00, 0x00  //0x6C
};

char shiftTable[] = {
	0x00, 0x1B, '!',  '@',
	'#',  '$',  '%',  '^',
	'&',  '*',  '(',  ')',
	'_',  '+',  '\b', '\t',
	'q',  'w',  'e',  'r',
	't',  'y',  'u',  'i',
	'o',  'p',  '{',  '}',
	'\n', 0x02, 'a',  's',
	'd',  'f',  'g',  'h',
	'j',  'k',  'l',  ':',
	'"',  '~',  0x03, '|',
	'z',  'x',  'c',  'v',
	'b',  'n',  'm',  '<',
	'>',  '?'
};

/* keyFlags:
 *   bit 0 - set if key pressed, clear if released
 *   bit 1 - set if CAPS is on
 *   bit 2 - set if NumLk is on
 *   bit 3 - set if ScrLk is on
 *   bit 4 - set if Left Shift is down
 *   bit 5 - set if Right Shift is down
 *   bit 6 - set if Left Ctrl is down
 *   bit 7 - set if Right Ctrl is down
 *   bit 8 - set if Left Alt is down
 *   bit 9 - set if Right Alt is down
 *   bits 10 to 15 - reserved; should be clear
 */
uint16_t keyFlags = 0;

//callback function
void (*keyEventHandler)(char c, uint8_t keyCode, uint16_t flags) = 0;

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
	char c = 0;
	int bit = 0;
	
	//state transitions
	if(keyboardState == STATE_START) {
		if(scancode == 0xE0) {
			keyboardState = STATE_EXTENDED_CODE;
		} 
		else if(scancode == 0xE1) {
			keyboardState = STATE_PAUSE;
		}
		else {
			keyboardState = STATE_START;
			c = scanCodeTable[scancode & 0x7F];
		}
	} 
	else if(keyboardState == STATE_EXTENDED_CODE) {
		if(scancode == 0x2A) {
			keyboardState = STATE_PRTSC_PR1;
		}
		else if(scancode == 0xB7) {
			keyboardState = STATE_PRTSC_RL1;
		}
		else {
			keyboardState = STATE_START;
			//c = scanCodeTable[(scancode & 0x7F) + 0x50];
		}
	}
	else if(keyboardState == STATE_PAUSE) {
		if(scancode == 0x1D) {
			keyboardState = STATE_PAUSE_PR;
		}
		else if(scancode == 0x45) {
			keyboardState = STATE_PAUSE_RL;
		}
		else {
			keyboardState = STATE_START;
		}
	}
	else if(keyboardState == STATE_PRTSC_PR1 && scancode == 0xE0) {
		keyboardState = STATE_PRTSC_PR2;
	}
	else if(keyboardState == STATE_PRTSC_RL1 && scancode == 0xE0) {
		keyboardState = STATE_PRTSC_RL2;
	}
	else if(keyboardState == STATE_PAUSE_PR && scancode == 0x45) {
		keyboardState = STATE_START;
	}
	else if(keyboardState == STATE_PAUSE_RL && scancode == 0xC5) {
		keyboardState = STATE_START;
	}
	else if(keyboardState == STATE_PRTSC_PR2 && scancode == 0x37) {
		keyboardState = STATE_START;
	}
	else if(keyboardState == STATE_PRTSC_RL2 && scancode == 0xAA) {
		keyboardState = STATE_START;
	}
	else {
		keyboardState = STATE_START;
	}
	
	if(c == 0x06) bit = 1; //CAPS lock
	else if(c == 0x03) bit = 4; //left shift
	else if(c == 0x04) bit = 5; //right shift
	else if(c == 0x02) bit = 6; //left control
	else if(c == 0x1E) bit = 7; //right control
	else if(c == 0x05) bit = 8; //left alt
	else if(c == 0x1F) bit = 9; //right alt
	
	//pressed/released flag
	keyFlags &= ~1;
	keyFlags |= ~(scancode >> 7) & 1;
	
	//set or clear appropriate bit in keyFlags
	if(bit >= 4) {
		if(scancode & 0x80) keyFlags &= ~(1 << bit); //released
		else keyFlags |= (1 << bit); //pressed
	}
	else if(bit >= 1 && (scancode & 0x80) == 0) { //lock-type keys (toggle-based)
		keyFlags ^= (1 << bit);
	}
	
	//if a character was pressed, send it to the handler
	if(c >= 0x20 && (scancode & 0x80) == 0 && keyEventHandler != 0) {
		//switch letter to uppercase if (CAPS LOCK) XOR (R OR L shift)
		if(c >= 'a' && c <= 'z' && (((keyFlags & 2) != 0) ^ ((keyFlags & 48) != 0))) {
			c &= ~0x20;
		}
		else if(scancode < 0x36 && (keyFlags & 48)) { //if R OR L shift is pressed
			c = shiftTable[scancode & 0x7F];
		}
		
		keyEventHandler(c, scancode, keyFlags);	
	}
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



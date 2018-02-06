/* keyboard.h
 * 8042 PS/2 Controller / Keyboard Controller
 */

#include "types.h"

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

/* Definitions */

// Keyboard is connected to IRQ 1
#define KBD_IRQ 	1

// Port 0x60: Data Register
#define KBD_DATA 	0x60
// Port 0x64: Status Register on Read, Command on Write
#define KBD_STAT 	0x64
// Bitmask for 2nd Bit of Status Register
#define KBD_STAT_MASK 0x02
// 
#define KEY_RELEASE_BOUND 128

// Command Buffer Maximum Length
#define CMD_LEN_MAX	256

//the index of the first terminal
#define TERM_ZERO	0

//the index of the second terminal
#define TERM_ONE 	1

//the index of the third terminal
#define TERM_TWO 	2

//if the terminal has no process, the number stored in term_process is zero
#define TERMINAL_EMPTY 	0


// Specific PS2 Keys Scancodes
#define KEY_ENTER		0x1C
#define KEY_BACKSPACE	0x0E
#define KEY_DEL			0x53
#define KEY_LSHIFT_PS	0x2A
#define KEY_LSHIFT_RL	0xAA
#define KEY_RSHIFT_PS	0x36
#define KEY_RSHIFT_RL	0xB6
#define KEY_CAPSLOCK	0x3A
#define KEY_LALT_PS		0x38
#define KEY_LALT_RL		0xB8
#define KEY_LCTRL_PS	0x1D
#define KEY_LCTRL_RL	0x9D
#define KEY_F1			0x3B
#define KEY_F2			0x3C
#define KEY_F3			0x3D
#define KEY_L			0x26
#define KEY_LARROW		0x4B
#define KEY_RARROW		0x4D

/* Functions */

// Initialize the Keyboard
void kbd_init(void);

// Handler for a Keyboard Interrupt
void kbd_irq_handler(void);

// Process a Character Input
void char_handler(unsigned char c);

// Open Terminal
int terminal_open(const uint8_t* filename);

// Close Terminal
int terminal_close(void);

// Read from Command Buffer
int terminal_read(unsigned int inode, unsigned int offset, void* buffer, int32_t size);

// Display write buffer on Terminal
int terminal_write(const void* buffer, int32_t size);

// Invalid Read Function for STDOUT
int terminal_read_invalid(unsigned int inode, unsigned int offset, void* buffer, int32_t size);

// Invalid Write Function for STDIN
int terminal_write_invalid(const void* buffer, int32_t size);

#endif

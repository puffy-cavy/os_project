/* keyboard.c
 * 8042 PS/2 Controller / Keyboard Controller Driver
 */

#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "syscall.h"
#include "paging.h"

// Current Terminal
int term = 0;
// Text Buffer of Current Command
char cmd_buf[TERM_MAX][CMD_LEN_MAX];
// Length of Current Command
int cmd_len[TERM_MAX];
// Current Cursor
int cmd_cursor[TERM_MAX];
// Read Lock on Current Command
int cmd_readlock[TERM_MAX];

// Standard US QWERTY Keyboard Scancode Mappings
unsigned char scancode_arr[4][128] = {
	{
		0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
		0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
		0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
		0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
		0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
		0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
		0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
		0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',
		0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
		0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
		0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',
		0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '~',
		0, '|', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, '*', 0, ' ', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	}
};
// Keyboard Scancode Type (0 - Normal, 1 - SHIFT, 2 - CAPSLOCK, 3 - SHIFT + CAPSLOCK)
uint8_t scancode_type = 0;
// CTRL Status (0 - Released, 1 - Pressed)
uint8_t scancode_ctrl = 0;
// ATL Status (0 - Released, 1 - Pressed)
uint8_t scancode_alt = 0;

/* kbd_init()
 * Initialize the Keyboard, Enables Keyboard IRQs
 *
 * Inputs: None
 * Outputs: None
 */
void kbd_init(void) {
	
	int i;
	int j;
	
	// Set Terminal to Default
	term = 0;
	cmd_flag=0;
	// Reset all Cursors and Buffers
	for (i = 0; i < TERM_MAX; i++) {
		cmd_len[i] = 0;
		cmd_cursor[i] = 0;
		cmd_readlock[i] = 1;
		for (j = 0; j < CMD_LEN_MAX; j++) {
			cmd_buf[i][j] = 0;
		}
	}
	
	// Enable Keyboard Interrupts
	enable_irq(KBD_IRQ);
}

/* kbd_irq_handler()
 * Handler for a Keyboard Interrupt
 *
 * Inputs: None
 * Outputs: None
 */
void kbd_irq_handler(void) {
	
	// Value of scanned input
	unsigned char val;
	// Status of Keyboard
	unsigned char stat = 0x02;
	
	// Disable all IRQs while Initializing Keyboard
	cli();
	cmd_flag=1;
	while (stat & KBD_STAT_MASK) {
		// Read value from Keyboard Data Register
		val = inb(KBD_DATA);
		// Process Character
		char_handler(val);
		// Read Status Register
		stat = inb(KBD_STAT);
	}
	// Send EOI
	send_eoi(KBD_IRQ);
	
	// Re-enable all IRQs
	sti();
}

/* char_handler()
 * Process an Input Character
 *
 * Inputs: None
 * Outputs: None
 */
void char_handler(unsigned char c) {
	
	// Enter is Pressed
	if (c == KEY_ENTER) {
		// Print Command Buffer
		putcmdend(cmd_buf[term], cmd_len[term]);
		// Unlock the Command Buffer
		cmd_readlock[term] = 0;
	}

	// Backspace is Pressed
	else if (c == KEY_BACKSPACE) {
		if (cmd_cursor[term] > 0) {
			// Shift Command Buffer Left
			int i;
			for (i = cmd_cursor[term] - 1; i < cmd_len[term]; i++) {
				cmd_buf[term][i] = cmd_buf[term][i + 1];
			}
			// Move Cursor Left and Decrease Length 
			cmd_cursor[term]--;
			cmd_len[term]--;
			// Print the Current Command Buffer
			putcmd(cmd_buf[term], cmd_len[term]);
		}
	}
	// Delete is Pressed
	else if (c == KEY_DEL) {
		if (cmd_len[term] > cmd_cursor[term]) {
			// Shift Command Buffer Left
			int i;
			for (i = cmd_cursor[term]; i < cmd_len[term]; i++) {
				cmd_buf[term][i] = cmd_buf[term][i + 1];
			}
			// Decrease Length 
			cmd_len[term]--;
			// Print the Current Command Buffer
			putcmd(cmd_buf[term], cmd_len[term]);
		}
	}
	// Left Arrow is Pressed
	else if (c == KEY_LARROW) {
		if (cmd_cursor[term] > 0) {
			cmd_cursor[term]--;
		}
	}
	// Right Arrow is Pressed
	else if (c == KEY_RARROW) {
		if (cmd_cursor[term] < cmd_len[term]) {
			cmd_cursor[term]++;
		}
	}
	// Caps Lock is Pressed
	else if (c == KEY_CAPSLOCK) {
		if (scancode_type >= 2) scancode_type -= 2;
		else scancode_type += 2;
	}
	// Left Ctrl is Pressed
	else if (c == KEY_LCTRL_PS) {
		scancode_ctrl = 1;
	}
	// Left Ctrl is Released
	else if (c == KEY_LCTRL_RL) {
		scancode_ctrl = 0;
	}
	// Left or Right Shift is Pressed
	else if (c == KEY_LSHIFT_PS || c == KEY_RSHIFT_PS) {
		scancode_type += 1;
	}
	// Left or Right Shift is Released
	else if (c == KEY_LSHIFT_RL || c == KEY_RSHIFT_RL) {
		scancode_type -= 1;
	}
	// Left Alt is Pressed
	else if (c == KEY_LALT_PS) {
		scancode_alt = 1;
	}
	// Left Alt is Released
	else if (c == KEY_LALT_RL) {
		scancode_alt = 0;
	}
	// F1 is Pressed
	else if (c == KEY_F1 && scancode_alt) {
		// Switch Active Terminal
		term = TERM_ZERO;
		switch_terminal(term);
		
	}
	// F2 is Pressed
	else if (c == KEY_F2 && scancode_alt) {
		// Switch Active Terminal
		term = TERM_ONE;
		switch_terminal(term);
		
		// Launch this Terminal on first Use
		if(term_process[term] == TERMINAL_EMPTY){
			
			// Forced Context Switch to Shell on Terminal 1
			switch_process(term);
			printf("CTOS: Launching Shell on Terminal 1 \n");

			// Save the Base and Stack Pointers
			pcb_struct_t *current_pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
			asm volatile(
			"movl %%esp, %%eax ;"
			: "=a" (current_pcb->sp)
			);
			asm volatile(
			"movl %%ebp, %%eax ;"
			: "=a" (current_pcb->bp)
			);
			
			// Send EOI
			send_eoi(KBD_IRQ);
			// Re-Enable Interrupts
			sti();
			// Launch Shell
			execute((const uint8_t *) "shell");
		}
		
	}
	// F3 is Pressed
	else if (c == KEY_F3 && scancode_alt) {
		// Switch Active Terminal
		term = TERM_TWO;
		switch_terminal(term);
		
		// Launch this Terminal on first Use
		if(term_process[term] == TERMINAL_EMPTY){
			
			// Forced Context Switch to Shell on Terminal 2
			switch_process(term);
			printf("CTOS: Launching Shell on Terminal 2 \n");

			// Save the Base and Stack Pointers
			pcb_struct_t *current_pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
			asm volatile(
			"movl %%esp, %%eax ;"
			: "=a" (current_pcb->sp)
			);
	
			asm volatile(
			"movl %%ebp, %%eax ;"
			: "=a" (current_pcb->bp)
			);
			
			// Send EOI
			send_eoi(KBD_IRQ);
			// Re-Enable Interrupts
			sti();
			// Launch Shell
			execute((const uint8_t *) "shell");
		}
		
	}
	// 'L' and CTRL is Pressed
	else if (c == KEY_L && scancode_ctrl) {
		clear();
	}
	else {
		char ascii_chr;
		// Check that Key is a Press and not Release
		if ((c < KEY_RELEASE_BOUND) && (scancode_arr[scancode_type][c] != 0) && (cmd_len[term] < (CMD_LEN_MAX))) {
			int i;
			// Convert Scancode to ASCII
			ascii_chr = scancode_arr[scancode_type][c];
			// Set Command Buffer
			for (i = cmd_len[term]; i > cmd_cursor[term]; i--) {
				cmd_buf[term][i] = cmd_buf[term][i - 1];
			}
			cmd_buf[term][cmd_cursor[term]] = ascii_chr;
			// Increment the Cursor
			cmd_cursor[term]++;
			// Increment the Length
			cmd_len[term]++;
			// Print the Current Command Buffer
			putcmd(cmd_buf[term], cmd_len[term]);
		}
	}
}

/* terminal_open()
 * Open Terminal
 *
 * Inputs: None
 * Outputs: 0
 */
int terminal_open(const uint8_t* filename) {
	
	return 0;
}

/* terminal_close()
 * Close Terminal
 *
 * Inputs: None
 * Outputs: 0
 */
int terminal_close(void) {
	
	return 0;
}

/* terminal_read()
 * Read a number of bytes from Command Buffer
 *
 * Inputs: buf - Pointer to Char Buffer
 *		   size - Number of Bytes
 * Outputs: Number of Bytes read
 */
int terminal_read(unsigned int inode, unsigned int offset, void* buffer, int32_t size) {
	
	int i;
	int bytes_read = 0;
	int term_loc = get_process();
	cmd_readlock[term_loc] = 1;
	unsigned char* buf = (unsigned char*) buffer;
	
	// Check that Pointer is Not NULL
	if (((unsigned int) buf) == 0) {	
		printf("TERMINAL.READ: ERR - Invalid Read Buffer Pointer \n");
		return 0;
	}
	
	// Read Size is Greater than Max Length Command Buffer
	if (size > CMD_LEN_MAX) {
		// Adjust Size
		size = CMD_LEN_MAX;
	}
	
	// Wait for Command to be ready
	while (cmd_readlock[term_loc]);
	
	// Read from Command Buffer
	for (i = 0; i < size; i++) {
		buf[i] = cmd_buf[term_loc][i];
		bytes_read++;
	}
	
	// Clear Command Buffer
	for (i = 0; i < CMD_LEN_MAX; i++) {
		cmd_buf[term_loc][i] = 0;
	}
	// Reset Length, Lock, and Cursor
	cmd_len[term_loc] = 0;
	cmd_cursor[term_loc] = 0;
	cmd_readlock[term_loc] = 1;
	
	// Move to Next Line
	putc_term('\n', term);

	return bytes_read;
}

/* terminal_write()
 * Display write buffer on Terminal
 *
 * Inputs: buf - Pointer to Char Buffer
 *		   size - Number of Bytes
 * Outputs: Number of Bytes written
 */
int terminal_write(const void* buffer, int32_t size) {
	
	int i;
	int bytes_written = 0;
	unsigned char* buf = (unsigned char*) buffer;
	
	// Check that Pointer is Not NULL
	if (((unsigned int) buf) == 0) {
		printf("TERMINAL.WRITE: ERR - Invalid Write Buffer Pointer \n");
		return 0;
	}

	// Print each Character in Buffer
	for (i = 0; i < size; i++) {
		putc(buf[i]);
		bytes_written++;
	}

	return bytes_written;
}

/* Invalid Read Function for STDOUT */
int terminal_read_invalid(unsigned int inode, unsigned int offset, void* buffer, int32_t size) {
	
	printf("TERMINAL.STDOUT: ERR - Invalid Call to Read Function \n");
	return -1;
}

/* Invalid Write Function for STDIN */
int terminal_write_invalid(const void* buffer, int32_t size) {
	
	printf("TERMINAL.STDIN: ERR - Invalid Call to Write Function \n");
	return -1;
}


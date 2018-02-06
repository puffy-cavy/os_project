/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab
 */

#include "lib.h"

// Terminal Buffer Addresses (4KB Aligned)
#define TERM0 0x2000
#define TERM1 0xA000 //changed
#define TERM2 0x12000

// Properties of Video Memory
#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

// Pointer to Video Memory
static char* video_mem = (char*) VIDEO;

// Highest Achieved Length of Current Command Buffer
static int cmd_len_rec[TERM_MAX] = {0};

// Current Terminal Index (0 - TERM_MAX)
static int a_term = 0;
// Process Terminal Index (0 - TERM_MAX)
static int p_term = 0;

// Cursor Locations for Terminals
static int term_x[TERM_MAX] = {0};
static int term_y[TERM_MAX] = {0};

// Text Buffers for Terminals
static char* term_buf[TERM_MAX] = {(char*) TERM0, (char*) TERM1, (char*) TERM2};

// Offset of each terminal in rows
static int32_t term_offset[TERM_MAX]={125,125,125};


/* void terminal_init()
 * Inputs: none
 * Return value: none
 * Function: assign all colors in terminal buffer to ATTRIB
 */
void terminal_init(){
	uint32_t i,j;
	// Assign all colors in three terminal buffer
	for(i=0;i<TERM_MAX;i++){
		for(j=0;j<6*NUM_COLS*NUM_ROWS;j++){
			*(uint8_t *)(term_buf[i] + (j << 1) + 1) = ATTRIB;
		}
	}
}

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears rows 1-24 and resets cursor */
void clear(void) {
    int32_t i;
	// Clear Terminal Buffer
    for (i = term_offset[a_term]*NUM_COLS; i < (term_offset[a_term]+NUM_ROWS) * NUM_COLS; i++) {//changed
        *(uint8_t *)(term_buf[a_term] + (i << 1)) = ' ';
        *(uint8_t *)(term_buf[a_term] + (i << 1) + 1) = ATTRIB;
    }
	// Reset Terminal Cursor
	term_x[a_term] = 0;
	term_y[a_term] = 0;
}

/* void clearall(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears All Terminals and Resets Cursors */
void clearall(void) {
	int i; int j;
	for (j = 0; j < TERM_MAX; j++) {
		// Clear Terminal Buffer
		for (i = 0; i < (NUM_ROWS) * NUM_COLS * 6; i++) {
			*(uint8_t *)(term_buf[j] + (i << 1)) = ' ';
			*(uint8_t *)(term_buf[j] + (i << 1) + 1) = ATTRIB;
		}
		// Reset Terminal Cursor
		term_x[j] = 0;
		term_y[j] = 0;
	}
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* int32_t puts_term(int8_t* s, int8_t term);
 * Inputs: int8_t* s - pointer to a string of characters
 *         int8_t term - terminal
 * Return Value: Number of bytes written
 * Function: Output a string to the console */
int32_t puts_term(int8_t* s, int term) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc_term(s[index], term);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 * Function: Output a character to the console */
void putc(uint8_t c) {
    if(c == '\n' || c == '\r') {
		// Check if we are in Last Line
		if (term_y[p_term] >= NUM_ROWS - 1) {
			scrollup();			
		}
		// Move to Next Line
		else {
			term_y[p_term]++;
		}
		term_x[p_term] = 0;
    } else {
		// Print Character to Terminal Buffer
		*(uint8_t *)(term_buf[p_term] + ((NUM_COLS * term_y[p_term] + term_x[p_term]+term_offset[p_term]*NUM_COLS) << 1)) = c;
		*(uint8_t *)(term_buf[p_term] + ((NUM_COLS * term_y[p_term] + term_x[p_term]+term_offset[p_term]*NUM_COLS) << 1) + 1) = ATTRIB;
		term_x[p_term]++;
		// Check if we are in the Last Column
		if (term_x[p_term] >= NUM_COLS) {
			// Return to First Column
			term_x[p_term] = 0;
			if (term_y[p_term] >= NUM_ROWS - 1) {
				scrollup();
			}
			else {
				term_y[p_term]++;
			}
		}
    }
}

/* void putc_term(uint8_t c, int8_t term);
 * Inputs: int8_t* c - character to print
 *         int8_t term - terminal
 * Return Value: void
 * Function: Output a character to the console */
void putc_term(uint8_t c, int term) {
    if(c == '\n' || c == '\r') {
		// Check if we are in Last Line
		if (term_y[term] >= NUM_ROWS - 1) {
			scrollup();
		}
		// Move to Next Line
		else {
			term_y[term]++;
		}
		term_x[term] = 0;
    } else {
		// Print Character to Terminal Buffer
		*(uint8_t *)(term_buf[term] + ((NUM_COLS * term_y[term] + term_x[term]+term_offset[term]*NUM_COLS) << 1)) = c;
		*(uint8_t *)(term_buf[term] + ((NUM_COLS * term_y[term] + term_x[term]+term_offset[term]*NUM_COLS) << 1) + 1) = ATTRIB;
		term_x[term]++;
		// Check if we are in the Last Column
		if (term_x[term] >= NUM_COLS) {
			// Return to First Column
			term_x[term] = 0;
			if (term_y[term] >= NUM_ROWS - 1) {
				scrollup();
			}
			else {
				term_y[term]++;
			}
		}
    }
}

/* setcursor()
 * Sets the X and Y cursors of current terminal
 *
 * Inputs: x - X location of cursor on terminal
 *		   y - Y location of cursor on terminal
 * Outputs: None
 */
void setcursor(uint8_t x, uint8_t y) {
	// Assign cursor for associated terminal
	term_x[p_term] = x;
	term_y[p_term] = y;
}

/* scrollup()
 * Scroll up the screen center by one line by shifting all rows
 * upwards.
 *
 * Inputs: None
 * Outputs: None
 */
void scrollup(void) {
	int x;
	int y;
	// Copy next line to current line
	for (y = 0; y < 6*NUM_ROWS - 1; y++) {
		for (x = 0; x < NUM_COLS; x++) {
			*(char*)(term_buf[p_term] + ((NUM_COLS * y + x ) << 1)) = *(char*)(term_buf[p_term] + ((NUM_COLS * (y + 1) + x) << 1));
			*(char*)(term_buf[p_term] + ((NUM_COLS * y + x ) << 1)+1) = *(char*)(term_buf[p_term] + ((NUM_COLS * (y + 1) + x) << 1)+1);
		}
	}
	// Clear last line
	for (x = 0; x < NUM_COLS; x++) {
		*(char*)(term_buf[p_term] + ((NUM_COLS * (6*NUM_ROWS - 1) + x) << 1)) = ' ';
	}
}



/* scrollup_term(int8_t term)
 * Scroll up the screen center by one line by shifting all rows
 * upwards.
 *
 * Inputs: None
 * Outputs: None
 */
void scrollup_term(int term) {
	int x;
	int y;
	// Copy next line to current line
	for (y = 0; y <6*NUM_ROWS - 1; y++) {
		for (x = 0; x < NUM_COLS; x++) {
			*(char*)(term_buf[term] + ((NUM_COLS * y + x ) << 1)) = *(char*)(term_buf[term] + ((NUM_COLS * (y + 1) + x) << 1));
		}
	}
	// Clear last line
	for (x = 0; x < NUM_COLS; x++) {
		*(char*)(term_buf[term] + ((NUM_COLS * (6*NUM_ROWS - 1) + x) << 1)) = ' ';
	}
}

/* putcmd()
 * Print the Command Buffer on Screen but do not move the cursor
 *
 * Inputs: buf - Command Buffer
 *         len - Command Length
 * Outputs: None
 */
void putcmd(char* buf, int len) {
	// Local Variables
	if(cmd_flag){
		term_offset[a_term]=125;
		cmd_flag=0;
	}
	int x; int y; int i; int num_nl;
	// Save Screen Cursor
	x = term_x[a_term];
	y = term_y[a_term];
	// Enable Length Tracker
	if (len > cmd_len_rec[a_term]) cmd_len_rec[a_term] = len;
	// Calculate Number of Lines Command Will Take
	num_nl = (cmd_len_rec[a_term] + x) / NUM_COLS;
	// Calculate Number of New Lines that will be Created by this Command
	if (num_nl + y > NUM_ROWS - 1) num_nl = (num_nl + y) - (NUM_ROWS - 1);
	else num_nl = 0;
	// Print the command buffer
	for (i = 0; i < len; i++) putc_term(buf[i], a_term);
	for (i = len; i < cmd_len_rec[a_term]; i++) putc_term(' ', a_term);	
	// Restore Screen Cursor
	term_x[a_term] = x;
	term_y[a_term] = y - num_nl;
}

/* putcmdend()
 * Print the Command Buffer on Screen and move the cursor
 *
 * Inputs: buf - Command Buffer
 *         len - Command Length
 * Outputs: None
 */
void putcmdend(char* buf, int len) {
	// Local Variables
	int x; int y; int i; int num_nl;
	// Save Screen Cursor
	x = term_x[a_term];
	y = term_y[a_term];
	// Enable Length Tracker
	if (len > cmd_len_rec[a_term]) cmd_len_rec[a_term] = len;
	// Calculate Number of Lines Command Will Take
	num_nl = (cmd_len_rec[a_term] + x) / NUM_COLS;
	// Calculate Number of New Lines that will be Created by this Command
	if (num_nl + y > NUM_ROWS - 1) num_nl = (num_nl + y) - (NUM_ROWS - 1);
	else num_nl = 0;
	// Print the command buffer
	for (i = 0; i < len; i++) putc_term(buf[i], a_term);
	for (i = len; i < cmd_len_rec[a_term]; i++) putc_term(' ', a_term);	
	// Reset Length Tracker
	cmd_len_rec[a_term] = 0;
}

/* switch_process()
 * Change p_term to Terminal Associated with New Process after Context Switching
 *
 * Inputs: new_p_term - Terminal Associated with New Process
 * Outputs: None
 */
void switch_process(int new_p_term) {
	// Update Process Terminal
	p_term = new_p_term;
}

/* switch_terminal()
 * Change a_term to Terminal to be Displayed
 *
 * Inputs: new_a_term - Terminal to be Displayed
 * Outputs: None
 */
void switch_terminal(int new_a_term) {
	// Update Active Terminal
		// Save Screen to Terminal Buffer
	//*(uint8_t*)(video_mem+((prev_pos.y*NUM_COLS+prev_pos.x) << 1)+1)=ATTRIB;
	//memcpy(term_buf[a_term]+term_offset[a_term]*NUM_COLS*2, video_mem, TERM_BUF_SIZE);
	// Update Active Terminal
	a_term = new_a_term;
	// Copy Terminal Buffer to Screen
	//memcpy(video_mem, term_buf[a_term]+term_offset[a_term]*NUM_COLS*2, TERM_BUF_SIZE);//chnaged
}

/* get_process()
 * Returns the Current Process Terminal
 *
 * Inputs: None
 * Outputs: Current Process Terminal
 */
int get_process() {
	return p_term;
}

/* get_terminal()
 * Returns the Current Active Terminal
 *
 * Inputs: None
 * Outputs: Current Active Terminal
 */
int get_terminal() {
	return a_term;
}

/* display_terminal()
 * Display the Current Active Terminal by copying Terminal Buffer to Screen
 *
 * Inputs: None
 * Outputs: None
 */
void display_terminal() {
	// Copy Terminal Buffer to Screen
	// Copy Terminal Buffer to Screen
	if(scroll_up){
		term_offset[a_term]=term_offset[a_term]-1;
		if(term_offset[a_term]<0){
			term_offset[a_term]=0;
		}
	}
	if(scroll_down){
		term_offset[a_term]++;
		if(term_offset[a_term]>125){
			term_offset[a_term]=125;
		}
	}
	if(term_offset[a_term]<0){
		term_offset[a_term]=0;
	}
	if(term_offset[a_term]>125){
		term_offset[a_term]=125;
	}
	memcpy(video_mem, term_buf[a_term]+term_offset[a_term]*NUM_COLS*2, TERM_BUF_SIZE);
	//*(uint8_t*)(video_mem+((prev_pos.y*NUM_COLS+prev_pos.x) << 1))=' ';
	
	int i;
	for(i=0;i<30;i++){
		if (right_clicks[i].status){
			*(uint8_t*)(video_mem+((right_clicks[i].y_char*NUM_COLS+right_clicks[i].x_char) << 1)+1)=0x50;
		}
		if (left_clicks[i].status){
			*(uint8_t*)(video_mem+((left_clicks[i].y_char*NUM_COLS+left_clicks[i].x_char) << 1)+1)=0x20;
		} 
	}
	*(uint8_t*)(video_mem+((prev_pos.y*NUM_COLS+prev_pos.x) << 1)+1)=0x70;

}

/* display_processes()
 * Debug Function to Display Active Processes and Scheduling Status
 *
 * Inputs: p_list - Process List
 *            pid - Current Process ID
 * Outputs: None
 */
void display_processes(uint8_t* p_list, int pid) {
	int i;
	// Display Process List Status
	for (i = 1; i < 7; i++) {
		if (p_list[i] == 1) *(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + i) << 1)) = 'A';
		else if (p_list[i] == 2) *(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + i) << 1)) = 'P';
		else if (p_list[i] == 0) *(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + i) << 1)) = ' ';
		else *(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + i) << 1)) = 'X';
		*(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + i) << 1) + 1) = ATTRIB;
	}
	// Display PID
	int8_t pid_c;
	itoa(pid, &pid_c, 10);
	*(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + 8) << 1)) = pid_c;
	*(uint8_t *)(term_buf[a_term] + ((NUM_COLS * 24 + 70 + 8) << 1) + 1) = ATTRIB;
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

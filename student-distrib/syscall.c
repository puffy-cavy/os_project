/* System Calls
 * CTOS Supports 10 System Calls
 */

#include "lib.h"
#include "paging.h"
#include "syscall.h"
#include "x86_desc.h"
#include "file_system.h"
#include "rtc.h"
#include "keyboard.h"

// Function Table of RTC
op_table_t rtc_op;
// Function Table of Directory
op_table_t dir_op;
// Function Table of Regular File
op_table_t file_op;
// Function Table of Terminal STDIN
op_table_t stdin_op;
// Function Table of Terminal STDOUT
op_table_t stdout_op;

/* List of Active Processes */
uint8_t process_list[MAX_PROCESS_NUM] = {0};

// Current File Dentry
int file_read_dentry;

// Current Process ID
int current_pid;

// Array that Stores which Process is Active on each Terminal
uint8_t term_process[TERM_MAX] = {0};


/* init_fdops()
 * Initialize the FD's Function Table Pointers
 *
 * Inputs: None
 * Outputs: None
 */
void init_fdops() {
	/* Map RTC Functions */
	rtc_op.open = &rtc_open;
	rtc_op.read = &rtc_read;
	rtc_op.write = &rtc_write;
	rtc_op.close = &rtc_close;

	/* Map FS Directory Functions */
	dir_op.open = &open_directory;
	dir_op.read = &read_directory;
	dir_op.write = &write_directory;
	dir_op.close = &close_directory;

	/* Map FS File Functions */
	file_op.open = &open_file;
	file_op.read = &read_file;
	file_op.write = &write_file;
	file_op.close = &close_file;

	/* Map Terminal STDIN Functions */
	stdin_op.open = &terminal_open;
	stdin_op.read = &terminal_read;
	stdin_op.write = &terminal_write_invalid;
	stdin_op.close = &terminal_close;

	/* Map Terminal STDOUT Functions */
	stdout_op.open = &terminal_open;
	stdout_op.read = &terminal_read_invalid;
	stdout_op.write = &terminal_write;
	stdout_op.close = &terminal_close;
}

/* syscall_err()
 * Reached when syscall has EAX = 0
 *
 * Inputs: None
 * Outputs: -1
 */
int32_t syscall_err(void) {
	// Invalid Syscall
	printf("SYSCALL.SYSCALL_ERR: ERR - Invalid Syscall \n");
	return -1;
}

/* halt()
 * Halt the Running Program unless it is the first Shell
 * 
 * Inputs: status
 * Outputs: None
 */
int32_t halt(uint8_t status) {
	// Generic Loop Counter
	int i;
	// Process ID
	uint8_t pid;
	// Parent Process ID
	uint8_t parent_pid;
	// Parent's ESP, EBP
	uint32_t parent_esp, parent_ebp;
	
	// Disable Interrupts
	cli();
	
	// Get the PCB
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Get Process ID
	pid = pcb->pid;
	
	if (VERBOSE) printf("SYSCALL.HALT: HALTING Process %d Status: %d \n", pid, status);
	
	// Remove from Process List
	process_list[pid] = 0;
	// De-activate PCB
	pcb->state = 0;
	
	// Restore Parent's PID as the Active Process for this Terminal
	term_process[get_process()] = pcb->parent_pid;
	
	// Close all Opened Files
	for (i = 2; i < FD_MAX; i++) {
		if (pcb->fd_array[i].flags != 0) {
			close(i);
		}
	}

	// Check if we are trying to Halt "shell"
	if (pcb->parent_pid == 0) {
		printf("SYSCALL.HALT: WARN - Restarting Shell \n");
		// Re-Launch Shell
		sti();
		execute((const uint8_t *) "shell");
	}
	
	// Get the Parent's PID
	parent_pid = pcb->parent_pid;
	
	// Switch Page Mapping to Parent Executable
	switch_task(parent_pid);
	
	// Extract Parent ESP/EBP
	parent_esp = pcb->parent_sp;
	parent_ebp = pcb->parent_bp;
	
	// Set Current Process to Parent
	current_pid = parent_pid;

	// Activate the Parent Process
	if (parent_pid != 0) process_list[parent_pid] = 1;
	
	// Restore Parent's Context
	tss.esp0 = M_8MB - M_8KB * parent_pid - S_INT;
	tss.ss0 = KERNEL_DS;

	// Load Status to EDX
	asm volatile(
	"movl %0, %%edx ;"
	: // No Outputs
	: "r" ((uint32_t) status)
	);
	
	// Restore Parent's ESP
	asm volatile(
	"movl %0, %%esp ;"
	: // No Outputs
	: "r" (parent_esp)
	);

	// Restore Parent's EBP
	// Move Status to EAX
	asm volatile(
	"movl %0, %%ebp ;"
	"movl %%edx, %%eax ;"
	: // No Outputs
	: "r" (parent_ebp)
	);
	
	// Re-Enable Interrupts
	sti();
	
	// Return
	asm volatile("leave ;");
	asm volatile("ret ;");
	
	// Never Reached
	return 0;
}

/* execute()
 * Execute a user space program
 * 
 * Inputs: command - Command string
 * Outputs: 0 on Success
 */
int32_t execute(const uint8_t* command) {
		
	// Name of Executable
	unsigned char elfname[FNAME_LEN_MAX];
	// Process ID
	uint8_t pid;
	// Generic Loop Counter
	uint32_t i;
	// Generic Char Buffer
	unsigned char cbuf[FNAME_LEN_MAX];
	// Generic Return Value
	int32_t retval;
	// Entry Point of Executable
	uint32_t elfip = 0;
	// Arugment Buffer
	unsigned char temp_args[ARG_LENGTH];
	// Argument Index
	uint32_t arg_idx = 0;
	
	// Disable All Interrupts
	cli();
	
	// Check Command Validity
	if (command == NULL) {
		printf("SYSCALL.EXECUTE: FATAL - NULL Command Pointer \n");
		return -1;
	}
	
	// Parse Executable File Name	
	for (i = 0; i < FNAME_LEN_MAX; i++) {
		// Only the first word of the Command is the Executable Name
		if ((command[i] == ' ') || (command[i] == '\0')) break;
		// Copy Executable Name to Elfname
		elfname[i] = (unsigned char) command[i];
	}
	// Terminate Executable Name
	elfname[i] = '\0';
	
	// Extract the remaining Command Substring as Argument
	if (command[i] == ' ') {
		i++;
		for (; arg_idx < ARG_LENGTH; arg_idx++) {
			if (command[i] == '\0') break;
			temp_args[arg_idx] = (unsigned char) command[i];
			i++;
		}
	}
	
	// Check that elfname is indeed an Executable
	if (0 != check_file(elfname)) {
		printf("SYSCALL.EXECUTE: FATAL - Command is not Executable \n");
		return -1;
	}

	// Find a Slot for this Process
	for (i = 1; i < MAX_PROCESS_NUM; i++) {
		// Check if this Slot is Empty
		if (process_list[i] == 0) {
			process_list[i] = 1;
			pid = i;
			if (VERBOSE) printf("SYSCALL.EXECUTE: New Task is given Process Id: %d \n", i);
			break;
		}
	}
	
	// If all Processes are Active, return -1
	if (i == MAX_PROCESS_NUM) {
		printf("SYSCALL.EXECUTE: FATAL - No Slot for this Process \n");
		return -1;
	}
	
	// Extract the Instruction Entry Point
	if (-1 == read_file_data(elfname, ELF_ENTRY_OFFSET, cbuf, 4)) {
		printf("SYSCALL.EXECUTE: FATAL - Failed to Read Executable \n");
		return -1;
	}
	for (i = 0; i < S_INT; i++) {
		// Combined 4 chars to 1 32-bit word
		elfip += cbuf[i] << (i * S_BYTE);
	}
	
	if (VERBOSE) printf("SYSCALL.EXECUTE: Task Entry Point %x \n", elfip);
	
	// Enable Paging for this Process
	switch_task(pid);
	
	// Load Executable to Memory
	retval = read_file_data(elfname, 0, (uint8_t *) ELF_LOAD_ADDR, UINT16_MAX);
	if (retval == -1) {
		printf("SYSCALL.EXECUTE: FATAL - Failed to Load Executable \n");
		return -1;
	}
	else {
		if (VERBOSE) printf("SYSCALL.EXECUTE: Loaded %d Bytes to Memory \n", retval);
	}
	
	// Allocate PCB for this Process
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - M_8KB * pid);
	
	// Activate PCB
	pcb->state = 1;
	// Set Process ID
	pcb->pid = pid;
	// Set Current PID
	current_pid = pid;
	// Set Associated Terminal
	pcb->term = get_terminal();
	switch_process(pcb->term);
	
	// Initialize the Argument Buffer in PCB
	for (i = 0; i < ARG_LENGTH; i++) {
		pcb->argbuf[i] = 0;
	}
	
	// Store the Command line Arguments in PCB
	pcb->arg_length = arg_idx;
	for (i = 0; i < arg_idx; i++) {
		pcb->argbuf[i] = (uint8_t) temp_args[i];
	}
	pcb->argbuf[arg_idx] = '\0';
	
	// Load ESP to PCB
	asm volatile(
	"movl %%esp, %%eax ;"
	: "=a" (pcb->parent_sp)
	);
	
	// Load EBP to PCB
	asm volatile(
	"movl %%ebp, %%eax ;"
	: "=a" (pcb->parent_bp)
	);
	
	// Check if Current Terminal has an Existing Process
	if (term_process[get_process()] == 0){
		// Parent does not Exist
		pcb->parent_pid = 0;
		term_process[get_process()] = current_pid;
	}
	else {
		// Assign Parent PID
		pcb->parent_pid = ((pcb_struct_t *) (pcb->parent_sp & ALIGN_8KB))->pid;
		// Deactivate Parent Process
		process_list[pcb->parent_pid] = PROCESS_PENDING;
		term_process[get_process()] = current_pid;
	}
		
	// Initialize File Descriptor
	for (i = 0; i < FD_MAX; i++) {
		pcb->fd_array[i].inode = 0;
		pcb->fd_array[i].file_position = 0;
		pcb->fd_array[i].flags = 0;
	}
	
	// Enable STDIN and STDOUT
	pcb->fd_array[0].flags = 1;
	pcb->fd_array[1].flags = 1;
	pcb->fd_array[0].function_table = &stdin_op;
	pcb->fd_array[1].function_table = &stdout_op;
	
	// Save current Context in TSS before Task Switch
	tss.esp0 = M_8MB - M_8KB * pid - S_INT;
	tss.ss0 = KERNEL_DS;

	// Re-Enable Interrupts
	sti();
	
	// Start Execution in User Space with IRET
	asm volatile(
	"cli ;"
	"movl $0x002B, %%eax ;"		// Load USER_DS to EAX
	"movw %%ax, %%ds ;" 		// Load Data Segment Register
	"movw %%ax, %%es ;"			// Load Destination Segment Register
	"movw %%ax, %%gs ;"			// Load Extra Segment Register
	"movw %%ax, %%fs ;"			// Load File Segment Register
	"pushl $0x002B ;" 			// Push USER_DS
	"pushl $0x83FFFFC ;"		// Push User Space ESP
	"pushfl ;"					// Push EFLAGS
	"popl %%ebx ;"				// Load EFLAGS to EBX
	"orl $0x4200, %%ebx ;"		// Enable NT and IF Flags
	"pushl %%ebx ;"				// Push EFLAGS
	"pushl $0x0023 ;" 			// Push USER_CS
	"pushl %0 ;" 				// Push EIP to First Instruction of ELF
	"iret ;"					// Perform IRET to Return to User Space
	: // No Outputs
	: "r" (elfip)
	: "%eax", "%ebx"
	);
	
	// Never Reached
	return 0;
}

/* read()
 * Read the FD Entry's Data to the Provided Buffer
 *
 * Inputs:     fd - The Index of the File Descriptor Array
 * 		      buf - Buffer to Store Read Data 
           nbytes - Number of Bytes to Read
 * Outputs: EAX - The number of Bytes Read, -1 on Fail
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
	// Check the Validty of FD
	if ((fd < 0) || (fd > FD_MAX - 1) || (fd == 1)) {
		printf("SYSCALL.READ: FATAL - Invalid FD %d \n", fd);
		return -1;
	}
	// Check that nbytes is Positive
	if (nbytes < 0) {
		printf("SYSCALL.READ: FATAL - Negative NBYTES %d \n", nbytes);
		return -1;
	}
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Check the FD's Flags
	if (pcb->fd_array[fd].flags == 0) {
		printf("SYSCALL.READ: FATAL - FD %d has Invalid Flag \n", fd);
		return -1;
	}
	// Check that Buffer is not a NULL Pointer
	if (buf == NULL) {
		printf("SYSCALL.READ: FATAL - Buffer is a NULL Pointer \n", fd);
		return -1;
	}
	int32_t read_ret_value = (*(pcb->fd_array[fd].function_table->read))(pcb->fd_array[fd].inode, pcb->fd_array[fd].file_position, buf, nbytes);
	pcb->fd_array[fd].file_position += read_ret_value;
	return read_ret_value;
}

/* write()
 * Write the Buffer's Data to FD Entry's Data
 *
 * Inputs:     fd - The Index of the File Descriptor Array
 * 		      buf - Buffer to Store Data to be Written 
           nbytes - Number of Bytes to Write
 * Outputs: EAX - The number of Bytes Written, -1 on Fail
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
	// Check the Validty of FD
	if ((fd < 1) || (fd > FD_MAX - 1) || (fd == 0)) {
		printf("SYSCALL.WRITE: FATAL - Invalid FD %d \n", fd);
		return -1;
	}
	// Check that nbytes is Positive
	if (nbytes < 0) {
		printf("SYSCALL.WRITE: FATAL - Negative NBYTES %d \n", nbytes);
		return -1;
	}
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Check the FD's Flags
	if (pcb->fd_array[fd].flags == 0) {
		printf("SYSCALL.WRITE: FATAL - FD %d has Invalid Flag \n", fd);
		return -1;
	}
	// Check that Buffer is not a NULL Pointer
	if (buf == NULL) {
		printf("SYSCALL.WRITE: FATAL - Buffer is a NULL Pointer \n");
		return -1;
	}
	return (*(pcb->fd_array[fd].function_table->write))(buf, nbytes);
}

/* open()
 * Open a File according the the File Name given, and
 * place the File object into the first avaliable FD Entry
 *
 * Inputs: filename - The Char Pointer to File Name
 * Outputs: FD of the opened File, -1 on Fail
 */
int32_t open(const uint8_t* filename) {
	// Dentry Corresponding to File Name
	dentry_t open_dentry;
	// Generic Loop Counter
	int i;
	
	// Check that Filename is not a NULL Pointer
	if (filename == NULL) {
		printf("SYSCALL.OPEN: FATAL - Filename is a NULL Pointer \n");
		return -1;
	}
	
	// Look for the Dentry Corresponding to File Name
	int32_t read_return = read_dentry_by_name(filename, &open_dentry);
	
	// Check if we found it
	if (read_return == -1) {
		printf("SYSCALL.OPEN: FATAL - File Name: %s not Found \n", filename);
		return -1;
	}
	
	// Get Current PCB
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));

	// Attempt to Allocate an Empty Slot in FD for this File
	for (i = 2; i < FD_MAX; i++) {
		// Check if this Slot is Available
		if (pcb->fd_array[i].flags == 0) {
		// Slot is Available, check for the File Type
			if (open_dentry.file_type == FTYPE_RTC) {
				// File Name is RTC
				pcb->fd_array[i].function_table = &rtc_op;
				pcb->fd_array[i].inode = 0;
				pcb->fd_array[i].file_position = 0;
				pcb->fd_array[i].flags = RTC_FLAG;
				(*(pcb->fd_array[i].function_table->open))(filename);
				return i;
			}
			if(open_dentry.file_type == FTYPE_DIRECTORY) {
				// File Name is Directory
				pcb->fd_array[i].function_table = &dir_op;
				pcb->fd_array[i].inode = 0;
				pcb->fd_array[i].file_position = 0;
				pcb->fd_array[i].flags = DIRECTORY_FLAG;
				return i;
			}
			if(open_dentry.file_type == FTYPE_REGULAR) {
				// Open the Regular File
				pcb->fd_array[i].function_table = &file_op;
				pcb->fd_array[i].inode = open_dentry.inode_index;
				pcb->fd_array[i].file_position = 0;
				pcb->fd_array[i].flags = FILE_FLAG;
				return i;
			}
		}
	}
	
	// No Slot Available
	printf("SYSCALL.OPEN: FATAL - Out of FD Slots \n");
	return -1;
}

/* write()
 * Closes the Device given by FD
 *
 * Inputs: fd - The Index of the File Descriptor Array
 * Outputs: 0 on Success, -1 on Fail
 */
int32_t close(int32_t fd) {
	// Check the Validty of FD
	if ((fd < 0) || (fd > FD_MAX - 1) || (fd == 0) || (fd == 1)) {
		printf("SYSCALL.CLOSE: FATAL - Invalid FD %d \n", fd);
		return -1;
	}
	// Get Current PCB
	pcb_struct_t *pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Check the FD's Flags
	if (pcb->fd_array[fd].flags == 0) {
		printf("SYSCALL.CLOSE: FATAL - FD %d has Invalid Flag \n", fd);
		return -1;
	}
	int ret = (*(pcb->fd_array[fd].function_table->close))();
	// Reset Flags to 0 on a Successful Close
	if (ret != -1) {
		pcb->fd_array[fd].flags = 0;
	}
	return ret;
}

/* getargs()
 * Get the Arguments Associated with Command
 * the arguments are originally stored in pcb->argbuf and copied to *buf in this function
 * Inputs:    buf - the buffer that command line arguments copied to
 *         nbytes - the length of the arguments that *buf is expecting to receive
 * Outputs: 0 on Success, -1 on Fail
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
	// Check if buf is a NULL Pointer
	if (buf == NULL) {
		printf("SYSCALL.GETARGS: FATAL - Buffer is a NULL Pointer \n");
		return -1;
	}
	// Check if there are arguments in PCB waiting to be copied
	pcb_struct_t * pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	if (pcb->arg_length <= 0) {
		printf("SYSCALL.GETARGS: FATAL - PCB has no Arguments \n");
		return -1;
	}
	// Use min(nbytes, pcb->arg_length) as the actual Buffer Length to be copied
	int32_t copy_length;
	if (nbytes < pcb->arg_length)
		copy_length = nbytes;
	else
		copy_length = pcb->arg_length;
	
	// Generic Loop Counter
	int32_t i;
	for(i = 0; i < copy_length; i++) {
		buf[i] = pcb->argbuf[i];
	}
	buf[i] = '\0';
	return 0;
}

/* vidmap()
 * allocate the a 4kb page starts at 132MB in the virtual memory space as video memory
 * Inputs: screen_start - points to the start of video memory in virtual memory space
 * Outputs: 0 on Success, -1 on Fail
 */
int32_t vidmap(uint8_t** screen_start) {
	// Check whether the Address falls within the Address Range covered by the User level Page
	uint32_t temp_addr = (uint32_t) screen_start;
	// if ** screen_start does not exist in the User Space Memory, Report Error
	if ((temp_addr >> PD_OFFSET) != USER_DIR) {
		printf("SYSCALL.VIDMAP: FATAL - Pointer Out of Range \n");
		return -1;
	}
	// Get Current PCB
	pcb_struct_t *pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Get Associated Terminal
	int cur_term = pcb->term;
	// Map Video Page
	map_video(cur_term);
	// allocate the start address of video memory to the pointer, *screen_start
	*screen_start = (uint8_t*) VID_VIR_MEM;
	return VID_VIR_MEM;
}

// No Plans to Support in the Foreseeable Future
int32_t set_handler(int32_t signum, void* handler_address) {
	printf("SYSCALL.SET_HANDLER: INFO - Unsupported System Call \n");
	return -1;
}

// No Plans to Support in the Foreseeable Future
int32_t sigreturn(void) {
	printf("SYSCALL.SIGRETURN: INFO - Unsupported System Call \n");
	return -1;
}

/* schedule()
 * Switch the Process being Executed for the next Time Quantum
 *
 * Input: None
 * Output: None
 */
int schedule(void) {
	// Find the Next Active Process to Schedule
	int _next_pid = find_next_pid(current_pid);
	if ((_next_pid == -1) || (_next_pid == 0)){
		printf("SCHEDULE - FATAL: No Active Process \n");
		return -1;
	}
	if (VERBOSE) {
		// Debug Information
		display_processes(process_list, _next_pid);
	}
	// Fetch the PCB for the Next Process
	pcb_struct_t *next_pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (_next_pid)));
	// Call Context Switch Helper
	switch_process(next_pcb->term);
	context_switch(_next_pid);

	return 0;
}


/* context_switch()
 * Helper Function to Perform Context Switch
 *
 * Input: term - Terminal used by the next Process
 * Output: None
 */
void context_switch(int term) {
	// Fetch current PCB
	pcb_struct_t *current_pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (current_pid)));
	// Fetch next PCB
	int next_pid = find_next_pid(current_pid);
	pcb_struct_t *next_pcb = (pcb_struct_t *) (PCB_BASE_ADDR - (M_8KB * (next_pid)));
	
	// Save the Base and Stack Pointers
	asm volatile(
	"movl %%esp, %%eax ;"
	: "=a" (current_pcb->sp)//output
	);
	asm volatile(
	"movl %%ebp, %%eax ;"
	: "=a" (current_pcb->bp)
	);

	// Enable Paging for the next Process
	switch_task(next_pid);
	map_video(next_pcb->term);
	
	// Store next Process' Kernel Stack into TSS
	tss.esp0 = M_8MB - M_8KB * next_pid - S_INT;
	tss.ss0 = KERNEL_DS;
	
	// Update current PID
	current_pid = next_pid;

	// Load next Process' ESP and EBP
	asm volatile(
	"movl %0, %%esp ;"
	: // No Outputs
	: "r" (next_pcb->sp)
	);

	asm volatile(
	"movl %0, %%ebp ;"
	: // No Outputs
	: "r" (next_pcb->bp)
	);

	// Return
	asm volatile("leave ;");
	asm volatile("ret ;");

}

/* find_next_pid()
 * Helper Function to Find the next Active Process
 * 
 * Input: cur_pid - Current Process ID
 * Output: Next Active Process to be Scheduled
 */
int find_next_pid(int cur_pid) {
	int i;
	for (i = cur_pid + 1; i < MAX_PROCESS_NUM; i++) {
		if (process_list[i] == 1)
			return i;
	}
	for (i = 1; i < cur_pid + 1; i++) {
		if (process_list[i] == 1)
			return i;
	}
	
	// Should Never be Reached
	return -1;
}


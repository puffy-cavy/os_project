/* System Calls
 * CTOS Supports 10 System Calls
 */

#include "types.h"

#ifndef _SYSCALL_H
#define _SYSCALL_H

/* Maximum Number of Active Processes */
#define MAX_PROCESS_NUM 7
/* Entry Point Offset of ELF Executable in Bytes */
#define ELF_ENTRY_OFFSET 24
/* Start Virtual Address of Executable */
#define ELF_LOAD_ADDR 0x08048000

/* File Name Length */
#define FNAME_LEN_MAX 32
/* Max Number of File Descriptors */
#define FD_MAX 8

/* Number of Bytes in an Int */
#define S_INT 4
/* Number of Bits in a Byte */
#define S_BYTE 8
/* Flag to Indicate the File is RTC */
#define RTC_FLAG 1
/* Flag to Indicate the File is a Directory */
#define DIRECTORY_FLAG 2
/* Flag to Indicate the File is a Regular File */
#define FILE_FLAG 3
/* File Types */
#define FTYPE_REGULAR 2
#define FTYPE_DIRECTORY 1
#define FTYPE_RTC 0

/* PCB Base Address in Kernel Space */
#define PCB_BASE_ADDR 0x007FE000

/* Virtual Memory Allocated to Video Memory */ 
#define VID_VIR_MEM 0x8400000
/* Page Directory of User Program */
#define USER_DIR 32
/* Number of bits to be Right Shifted to obtaim Page Directory Entry */
#define PD_OFFSET 22

/* Size of the Argument Buffer */
#define ARG_LENGTH 	128

/* Flag for a Pending Process */
#define PROCESS_PENDING 2

/* Array that Stores which Process is Active on each Terminal */ 
extern uint8_t term_process[TERM_MAX];

/* PID of Current Running Process */
extern int current_pid;

/* File Functions Table */
typedef struct op_table_t {
	int (*open)(const uint8_t* filename);
	int (*read)(unsigned int inode, unsigned int offset, void* buf, int32_t nbytes);
	int (*write)(const void* buf, int32_t size);
	int (*close)(void);
} op_table_t;

/* File Functions for Devices */
extern op_table_t rtc_op;
extern op_table_t dir_op;
extern op_table_t file_op;
extern op_table_t stdin_op;
extern op_table_t stdout_op;

/* File Descriptor Structure */
typedef struct file_desc {
	// Function Table
	op_table_t* function_table;
	// Inode
	int32_t inode;
	// Current Position
	int32_t file_position;
	// Flags
	int32_t flags;
} file_desc_t;

/* Process Control Block Structure */
typedef struct pcb_struct {
	// Process State
	uint32_t state;
	// Process ID
	uint32_t pid;
	// Parent ID
	uint32_t parent_pid;
	// Parent Stack Pointer
	uint32_t parent_sp;
	// Parent Base Pointer
	uint32_t parent_bp;
	// Files used by Process
	file_desc_t fd_array[FD_MAX];
	// Associated Terminal
	uint32_t term;
	// Buffer used for Command Arguments
	uint8_t argbuf[ARG_LENGTH];
	// Length of the Argument Buffer
	int32_t arg_length;
	// Current Stack Pointer
	uint32_t sp;
	// Current Base Pointer
	uint32_t bp;
} pcb_struct_t;

/* Initialize Function Pointers */
void init_fdops();

/* System Call Handlers */

/* 0. Error */
int32_t syscall_err(void);

/* 1. Halt */
int32_t halt(uint8_t status);

/* 2. Execute */
int32_t execute(const uint8_t* command);

/* 3. Read */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* 4. Write */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* 5. Open */
int32_t open(const uint8_t* filename);

/* 6. Close */
int32_t close(int32_t fd);

/* 7. Getargs */
int32_t getargs(uint8_t* buf, int32_t nbytes);

/* 8. Vidmap */
int32_t vidmap(uint8_t** screen_start);

/* 9. Set Handler */
int32_t set_handler(int32_t signum, void* handler_address);

/* 10. Sigreturn */
int32_t sigreturn(void);

// OS Scheduling Main Function
int schedule(void);

// Helper to Perform Context Switch
void context_switch(int term);

// Helper to Find Next Process to Schedule
int find_next_pid(int cur_pid);

#endif // SYSCALL

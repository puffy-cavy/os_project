/* paging.c
 * Setup and manage the Page Directory and Table
 */

#include "x86_desc.h"
#include "paging.h"

// Terminal Buffer Addresses (4KB Aligned)
#define TERM0_ADDR 0x2000
#define TERM1_ADDR 0xA000
#define TERM2_ADDR 0x12000

/* init_page()
 * Setup the PD and first PT (0-4MB)
 * Range 4-8MB is set up as an extended page for Kernel
 *
 * Inputs: None
 * Outputs: None
 */
void init_page(){

	int i;
	
	// Initialize all the PTEs
	for(i = 0; i < MAX_PAGE_TABLE_SIZE; i++) {
		page_table[i].present = ((i == 0) ? 0 : 1); // Do not map Null pointer
		page_table[i].r_w = 1;
		page_table[i].user_priv = 0;
		page_table[i].write_thru = 0;
		page_table[i].cache_dis = 0;
		page_table[i].accessed = 0;
		page_table[i].dirty = 0;
		page_table[i].zero = 0;
		page_table[i].global = 0;
		page_table[i].avail = 0;
		page_table[i].page_addr = i;
	}

    // Intialize the PDE pointing to the first PT
    page_directory[0].present = 1;
    page_directory[0].r_w = 1;
    page_directory[0].user_priv = 0;
    page_directory[0].write_thru = 0;
    page_directory[0].cache_dis = 0;
    page_directory[0].accessed = 0;
    page_directory[0].zero = 0;
    page_directory[0].size = 0;
    page_directory[0].ignore = 0;
    page_directory[0].avail = 0;
    page_directory[0].page_addr = (((uint32_t) page_table) >> PT_ADDR_OFFSET);

    // Initialize the PDE of Kernel
	page_directory[1].present = 1;
	page_directory[1].r_w = 1;
	page_directory[1].user_priv = 0;
	page_directory[1].write_thru = 0;
	page_directory[1].cache_dis = 0;
	page_directory[1].accessed = 0;
	page_directory[1].zero = 0;
	page_directory[1].size = 1;
	page_directory[1].ignore = 0;
	page_directory[1].avail = 0;
	page_directory[1].page_addr = 1 << PD_ADDR_OFFSET;
	
	// Initialize pages for heap memory
	page_directory[2].present = 1;
	page_directory[2].r_w = 1;
	page_directory[2].user_priv = 1;
	page_directory[2].write_thru = 0;
	page_directory[2].cache_dis = 0;
	page_directory[2].accessed = 0;
	page_directory[2].zero = 0;
	page_directory[2].size = 1;
	page_directory[2].ignore = 0;
	page_directory[2].avail = 0;
	page_directory[2].page_addr = 2 << PD_ADDR_OFFSET;
	
    // Initialize the remaining unused PTs
    for(i = 3; i < MAX_PAGE_DIRECTORY_SIZE; i++) {
		page_directory[i].present = 0;
		page_directory[i].r_w = 1;
		page_directory[i].user_priv = 0;
		page_directory[i].write_thru = 0;
		page_directory[i].cache_dis = 0;
		page_directory[i].accessed = 0;
		page_directory[i].zero = 0;
		page_directory[i].size = 0;
		page_directory[i].ignore = 0;
		page_directory[i].avail = 0;
		page_directory[i].page_addr = i << PD_ADDR_OFFSET;
    }
	
	/* Enable paging: CR0[31]
	 * Enable page size extend (PSE): CR4[4]
	 * Pass Address of PD to CR3
	 */
	asm volatile(
	"movl $page_directory, %%eax ;"
	"movl %%eax, %%cr3 ;"
	"movl %%cr4, %%eax ;"
	"orl  $0x00000010, %%eax ;"
	"movl %%eax, %%cr4 ;"
	"movl %%cr0, %%eax ;"
	"orl  $0x80000000, %%eax ;"
	"movl %%eax, %%cr0 ;"
	: // No Inputs
	: // No Outputs
	: "eax"
    );

	return;
}

/* switch_task()
 * Switches the Page Mapping of 128-132MB to Physical Memory
 * corresponding to the Process ID
 *
 * Inputs: pid - Process ID
 * Outputs: None
 */
void switch_task(uint8_t pid) {
	
	// Activate the PDE of Executable
	page_directory[ELF_DIR].present = 1;
	page_directory[ELF_DIR].r_w = 1;
	page_directory[ELF_DIR].user_priv = 1;
	page_directory[ELF_DIR].write_thru = 0;
	page_directory[ELF_DIR].cache_dis = 0;
	page_directory[ELF_DIR].accessed = 0;
	page_directory[ELF_DIR].zero = 0;
	page_directory[ELF_DIR].size = 1;
	page_directory[ELF_DIR].ignore = 0;
	page_directory[ELF_DIR].avail = 0;
	page_directory[ELF_DIR].page_addr = (pid + 3) << PD_ADDR_OFFSET;
	
	/* Re-Enable paging: CR0[31]
	 * Enable page size extend (PSE): CR4[4]
	 * Pass Address of PD to CR3
	 */
	asm volatile(
	"movl $page_directory, %%eax ;"
	"movl %%eax, %%cr3 ;"
	"movl %%cr4, %%eax ;"
	"orl  $0x00000010, %%eax ;"
	"movl %%eax, %%cr4 ;"
	"movl %%cr0, %%eax ;"
	"orl  $0x80000000, %%eax ;"
	"movl %%eax, %%cr0 ;"
	: // No Inputs
	: // No Outputs
	: "eax"
    );
	
	return;
}

/* map_video()
 * Maps the range of Video memory address used by User Space programs
 * to one of the Terminal's text buffers
 * 
 * Inputs: term - Associated Terminal
 * Outputs: None
 */
void map_video(int term) {
	// Initialize the Page Table for Video Memory
	// User Privilage is set to 1
	vid_page_table[0].present = 1; 
	vid_page_table[0].r_w = 1;
	vid_page_table[0].user_priv = 1;
	vid_page_table[0].write_thru = 0;
	vid_page_table[0].cache_dis = 0;
	vid_page_table[0].accessed = 0;
	vid_page_table[0].dirty = 0;
	vid_page_table[0].zero = 0;
	vid_page_table[0].global = 0;
	vid_page_table[0].avail = 0;
	if (term == 0) vid_page_table[0].page_addr = (TERM0_ADDR+0x5000) >> PT_ADDR_OFFSET;
	else if (term == 1) vid_page_table[0].page_addr = (TERM1_ADDR+0x5000) >> PT_ADDR_OFFSET;
	else if (term == 2) vid_page_table[0].page_addr = (TERM2_ADDR+0x5000) >> PT_ADDR_OFFSET;
	else vid_page_table[0].page_addr = MEM_IMG_START >> PT_ADDR_OFFSET;
	
	// Initialize the Page Directory Mapping the Virtual Address of 132MB
	page_directory[VID_DIR].present = 1;
	page_directory[VID_DIR].r_w = 1;
	page_directory[VID_DIR].user_priv = 1;
	page_directory[VID_DIR].write_thru = 0;
	page_directory[VID_DIR].cache_dis = 0;
	page_directory[VID_DIR].accessed = 0;
	page_directory[VID_DIR].zero = 0;
	page_directory[VID_DIR].size = 0;
	page_directory[VID_DIR].ignore = 0;
	page_directory[VID_DIR].avail = 0;
	page_directory[VID_DIR].page_addr = ((uint32_t) vid_page_table) >> PT_ADDR_OFFSET;

	/* Re-Enable paging: CR0[31]
	 * Enable page size extend (PSE): CR4[4]
	 * Pass Address of PD to CR3
	 */
	asm volatile(
	"movl $page_directory, %%eax ;"
	"movl %%eax, %%cr3 ;"
	"movl %%cr4, %%eax ;"
	"orl  $0x00000010, %%eax ;"
	"movl %%eax, %%cr4 ;"
	"movl %%cr0, %%eax ;"
	"orl  $0x80000000, %%eax ;"
	"movl %%eax, %%cr0 ;"
	: // No Inputs
	: // No Outputs
	: "eax"
    );
	
	return;
}

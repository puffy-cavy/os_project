/* paging.h
 * Setup and manage the Page Directory and Table
 */
 
#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

// Page Table Address Offset Right (Aligned to 4KB: 0x1000)
#define PT_ADDR_OFFSET 12
// Page Directory Address Offset Left (Aligned to 4MB: 0x400000)
#define PD_ADDR_OFFSET 10
// Page Directory used by an ELF Executable
#define ELF_DIR 32
// Page Directory used by Video Memory
#define VID_DIR 33
// Memory Image Start Address
#define MEM_IMG_START 0xB8000

/* Setup the PD and PT */
void init_page();

/* Switch Task */
void switch_task(uint8_t pid);

/* Free a previously allocated Page Directory */
uint32_t free_directory(uint32_t dir);

/* Free a previously allocate Page */
uint32_t free_page(uint32_t dir, uint32_t idx);

/* Allocate a 4KB Page at 132MB Mapped to Video Memory */
void map_video(int term);

#endif

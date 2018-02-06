#ifndef _MALLOC_H
#define _MALLOC_H
#include "types.h"
#define MAX_PAGE_ALIGNED_ALLOCS 32
#define KERNEL_END 0x800000


void heap_init();
/*void add_alloc_unit();
void remove_alloc_unit(alloc_t* unit);
void* ltrim_alloc_unit(alloc_t* unit, uint32_t size);
*/
/*do we have size_t declaration*/
uint8_t* malloc(uint32_t size);
void free(void* mem);
#endif

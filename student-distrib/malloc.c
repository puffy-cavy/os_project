#include "malloc.h"
#include "lib.h"
#define _4KB 0x1000
#define _4MB 0x400000
//define allocate structure
//CODE cited from OSDEV
//https://github.com/levex/osdev/blob/master/memory/malloc.c
typedef struct {
	uint8_t status;
	uint32_t size;
} alloc_t;
//Initiate value for heap_init
static uint32_t last_alloc = 0;//last alloc structure in bottom of heap;
static uint32_t heap_end = 0;
static uint32_t heap_begin = 0;
static uint32_t memory_used = 0;
/*function which intialize heap block in program img*/
/* heap_init()
 * Initialize the heap memory
 *
 * Inputs: None
 * Outputs: None
 */
void heap_init(){
  /*open page for allocated memory*/
  last_alloc=KERNEL_END+_4KB;
  heap_begin=last_alloc;
  heap_end=3*_4MB;
  /*set all value in heap memory to 0*/
  memset(heap_begin,0,heap_end-heap_begin);
  printf("Kernel Heap start at %x\n", heap_begin);
  
}

/* malloc(uint32_t size)
 * Dynamic allocate memory on heap
 *
 * Inputs: size
 * Outputs: Start pointer
 */
uint8_t* malloc(uint32_t size){
  /*allocate 0 bytes*/
  if(!size){
    return NULL;
  }
  uint8_t* alloc_mem_address=(uint8_t*) heap_begin;
  /*check blank space inside heap*/
  while ((uint32_t*)alloc_mem_address<last_alloc){
    alloc_t* alloc_structure=(alloc_t*)alloc_mem_address;
    /*if allocated size is zero,means never used here, just break*/
    if(!alloc_structure->size){
      break;
    }
    /*if alloc block is in used skip the whole block*/
    if(alloc_structure->status){
      alloc_mem_address+=alloc_structure->size;
      alloc_mem_address+=sizeof(alloc_t);
      alloc_mem_address+=4;
      continue;
    }
    /*if free space larger than required size*/
    if(alloc_structure->size>=size){
      alloc_structure->status=1;
   //   printf("Rearrange space from");
      memset(alloc_mem_address+sizeof(alloc_t),0,size);
      memory_used+=size+sizeof(alloc_t);
   //   printf("Allocate %d bytes memory from %x to %x",size,alloc_mem_address+sizeof(alloc_t),alloc_mem_address+sizeof(alloc_t)+size);
      return (uint8_t*)alloc_mem_address+sizeof(alloc_t);
    }
    memory_used+=alloc_structure->size;
    memory_used+=sizeof(alloc_t);
    memory_used+=4;

  }
  if(last_alloc+size+sizeof(alloc_t)>=heap_end){
    printf("ERROR: Dynamic memory is full");
	return NULL;
  }
  //allocate new structure
  alloc_t* new_alloc=(alloc_t*)last_alloc;
  new_alloc->status=1;
  new_alloc->size=size;
  //if allocated at the bottom of existing blocks, change the value of last allocated blaoc
  last_alloc+=size;
  last_alloc+=sizeof(alloc_t);
  last_alloc+=4;

  memory_used+=sizeof(alloc_t)+4+size;
  //set zero for allocated memeory
  memset((uint8_t*)((uint32_t)new_alloc+sizeof(alloc_t)),0,size);
//  printf("Allocate %d bytes memory from %x to %x",size,new_alloc+sizeof(alloc_t),new_alloc+sizeof(alloc_t)+size);
  return (uint8_t*)((uint32_t)new_alloc+sizeof(alloc_t));


}
/* free(void* mem)
 * Free dynamic allcocated
 *
 * Inputs: mem(start pointer of allocated memory)
 * Outputs: Start pointer
 */
void free(void* mem){
	//generate fault if free NULL or non_existing pointer
  if (mem==NULL){
	  asm volatile(
	  "INT $0x19"
	: // No Inputs
	: // No Outputs
    );
  }
  alloc_t* rm_alloc=(alloc_t*)(mem-sizeof(alloc_t));
  if(rm_alloc->status!=1){
	asm volatile(
	  "INT $0x19"
	: // No Inputs
	: // No Outputs
    );  
  }
  rm_alloc->status=0;
  memory_used-=rm_alloc->size+sizeof(alloc_t);
}

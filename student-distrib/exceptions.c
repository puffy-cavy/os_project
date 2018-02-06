/* exceptions.c
 * Exception Handlers defined by Intel
 */

#include "exceptions.h"
#include "lib.h"

void division_error(){
	printf("EXCEPTION: Division Error");
	while(1);
}

void debug_exception(){
	printf("EXCEPTION: Debug Exception");
	while(1);
}

void NMI_interrupt(){
	printf("EXCEPTION: NMI");
	while(1);
}

void breakpoint(){
	printf("EXCEPTION: Breakpoint");
	while(1);
}

void overflow(){
	printf("EXCEPTION: Overflow");
	while(1);
}

void bound_range_exceeded(){
	printf("EXCEPTION: Bound Range Exceeded");
	while(1);

}

void invalid_opcode(){
	printf("EXCEPTION: Invalid Opcode");
	while(1);

}

void device_not_available(){
	printf("EXCEPTION: Device not Available");
	while(1);

}

void double_fault(){
	printf("EXCEPTION: Double Fault");
	while(1);

}

void coprocessor_segment(){
	printf("EXCEPTION: Coprocessor Segment");
	while(1);

}

void invalid_tss(){
	printf("EXCEPTION: Invalid TSS");
	while(1);

}

void segment_not_present(){
	printf("EXCEPTION: Segment not Present");
	while(1);

}

void stack_segment_fault(){
	printf("EXCEPTION: Stack Segment Fault");
	while(1);

}

void general_protection(){
	printf("EXCEPTION: General Protection Fault");
	while(1);

}

void page_fault(){
	printf("EXCEPTION: Page Fault at Address 0x");
	
	int addr;
	// Load the Address from CR2 that caused Page Fault
	asm volatile(
	"movl %%cr2, %%eax ;"
	"movl %%eax, %0 ;"
	: "=g" (addr)
	: // No Inputs
	: "eax"
    );
	printf("%x          ", addr);
	
	while(1);

}

void undefined_exception(){
	printf("EXCEPTION: Undefined");
	while(1);

}

void floating_point_error(){
	printf("EXCEPTION: x87 FPU Error");
	while(1);

}

void alignment_check(){
	printf("EXCEPTION: Alignment Check");
	while(1);

}

void machine_check(){
	printf("EXCEPTION: Machine Check");
	while(1);

}

void floating_point_exception(){
	printf("EXCEPTION: SIMD Floating Point Exception");
	while(1);

}
void segmentation_fault(){
	printf("EXCEPTION: SIGSEGV - Segmentation Fault");
	while(1);
}


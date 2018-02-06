/* idt.c
 * Interrupt Descriptor Table
 */

#include "idt.h"
#include "exceptions.h"
#include "x86_desc.h"
#include "irq.h"

/* set_idt()
 * Initialize the Interrupt Descriptor Table
 * IRQ 0x00-0x1F: Intel Defined Exceptions
 * IRQ 0x20-0x2F: 8259A Master and Slave
 *
 * Inputs: None
 * Outputs: None
 */
void set_idt(){
	
	// Exceptions defined by Intel
	int i;
	for (i = 0; i < 32; i++) {
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].reserved3 = 1;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].size = 1;
		idt[i].reserved0 = 0; 
		idt[i].dpl = 0;
		idt[i].present = 1; 
		
	}

	// 8259A Master and Slave
	for(i = 32; i< NUM_VEC; i++){
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].size = 1;
		idt[i].reserved0 = 0; 
		idt[i].dpl = 0;
		idt[i].present = 1; 

	}
	
	// System Calls
	idt[128].dpl = 3;
	idt[128].reserved3 = 1;
	
	// Intel Defined Exception Entries
	SET_IDT_ENTRY(idt[0], division_error);
	SET_IDT_ENTRY(idt[1], debug_exception);
	SET_IDT_ENTRY(idt[2], NMI_interrupt);
	SET_IDT_ENTRY(idt[3], breakpoint);
	SET_IDT_ENTRY(idt[4], overflow);
	SET_IDT_ENTRY(idt[5], bound_range_exceeded);
	SET_IDT_ENTRY(idt[6], invalid_opcode);
	SET_IDT_ENTRY(idt[7], device_not_available);
	SET_IDT_ENTRY(idt[8], double_fault);
	SET_IDT_ENTRY(idt[9], coprocessor_segment);
	SET_IDT_ENTRY(idt[10], invalid_tss);
	SET_IDT_ENTRY(idt[11], segment_not_present);
	SET_IDT_ENTRY(idt[12], stack_segment_fault);
	SET_IDT_ENTRY(idt[13], general_protection);
	SET_IDT_ENTRY(idt[14], page_fault);
	SET_IDT_ENTRY(idt[15], undefined_exception);
	SET_IDT_ENTRY(idt[16], floating_point_error);
	SET_IDT_ENTRY(idt[17], alignment_check);
	SET_IDT_ENTRY(idt[18], machine_check);
	SET_IDT_ENTRY(idt[19], floating_point_exception);

	//malloc error handler
	SET_IDT_ENTRY(idt[25], segmentation_fault);
	
	// PIT Interrupt
	SET_IDT_ENTRY(idt[32], pit_irq_wrapper);

	// 8259A Devices Interrupt Entries
	SET_IDT_ENTRY(idt[40], rtc_irq_wrapper);
	SET_IDT_ENTRY(idt[33], kbd_irq_wrapper);
	SET_IDT_ENTRY(idt[44], mouse_irq_wrapper);
	
	// System Call
	SET_IDT_ENTRY(idt[128], syscall_wrapper);
	
}

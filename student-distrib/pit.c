/* pit.c
 * Programmable Interval Timer Driver
 */

#include "pit.h"
#include "lib.h"
#include "i8259.h"
#include "syscall.h"

/* pit_irq_handler()
 * PIT Interrupt Handler, calls schedule() on each Interrupt.
 *
 * Inputs: None
 * Outputs: None
 */
void pit_irq_handler(void){
	// Send EOI
	send_eoi(PIT_IRQ);
	// Call Scheduler
	schedule();
	// Re-enable all IRQs
	sti();
}

/* pit_init()
 * Initialize the PIT
 *
 * Inputs: None
 * Outputs: None
 */
void pit_init(){
	// Disable all Interrupts
	cli();
	
	// Initalize the Mode/Command Register connected to I/O Port 0x43
	// refer to https://github.com/pdoane/osdev/blob/master/time/pit.c
	outb(PIT_MODE, PIT_IO);
	
	// Channel 0 is connected Directly to IRQ0
	// Set the frequency
	
	// Send Low Byte
	outb(INI_FRE/DENO_FRE, CHANNEL_ZERO);
	// Send High Byte
	outb((INI_FRE/DENO_FRE)>>BYTE_SHIFT, CHANNEL_ZERO);

	// Re-enable all Interrupts
	sti();

	// Enable PIT Interrupts on IRQ 0
	enable_irq(PIT_IRQ);

}

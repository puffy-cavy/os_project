/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* i8259_init()
 * Initialize the 8259 PIC based on linux/drivers/i8259.c
 * Writes ICW1-4 and Enables IRQs for Master and Slave
 *
 * Inputs: None
 * Return: None
 */
void i8259_init(void) {
	
	/* Mask all Interrupts */
	master_mask = 0xFF;
	slave_mask = 0xFF;
	outb(master_mask, MASTER_8259_DATA);
	outb(slave_mask, SLAVE_8259_DATA);
	
	/* Initialize 8259A Master at IO Ports:
	 *  |- 0x20: 8259A Master Command
	 *  |- 0x21: 8259A Master Data
	 *  |- 0xA0: 8259A Slave Command
	 *  |- 0xA1: 8259A Slave Data
	 */
	 
	// ICW1: Select 8259A Master
	outb(ICW1, MASTER_8259_PORT);
	// ICW1: Select 8259A Slave
	outb(ICW1, SLAVE_8259_PORT);
	
	// ICW2: 8259A Master IRQ 0-7 Mapped to IDT 0x20-0x27
	outb(ICW2_MASTER, MASTER_8259_DATA);
	// ICW2: 8259A Slave IRQ 8-15 Mapped to IDT 0x28-2F
	outb(ICW2_SLAVE, SLAVE_8259_DATA);
	
	// ICW3: Tell 8259A Master that there is a Slave at IRQ 2
	outb(ICW3_MASTER, MASTER_8259_DATA);
	// ICW3: Tell 8259A Slave its cascade identity
	outb(ICW3_SLAVE, SLAVE_8259_DATA);
	
	// ICW4: 8086 Specific
	outb(ICW4, MASTER_8259_DATA);
	outb(ICW4, SLAVE_8259_DATA);

	// Enable slave cascade on IRQ 2
	enable_irq(SLAVE_CASCADE_IRQ);
	
	return;
}

/* enable_irq()
 * Enable (unmask) the specified IRQ
 *
 * Inputs: irq_num - Index of IRQ to unmask
 * Return: None
 */
void enable_irq(uint32_t irq_num) {
	
	uint8_t irq_mask;
	
	// Assert IRQ lies in the range 0x20-0x2F
	if (irq_num > 15 || irq_num < 0) {
		printf("FATAL: enable_irq() in i8259.c called with invalid irq_num \n");
		while(1);
		return;
	}
	
	// IRQ is in range 0x20-0x27
	if (irq_num < 8) {
		// Unmask bit corresponding to irq_num
		irq_mask = ~(1 << irq_num);
		master_mask = master_mask & irq_mask;
		// Update mask in hardware
		outb(master_mask, MASTER_8259_DATA);
	}
	// IRQ is in range 0x28-2F
	else {
		// Remap IRQ to proper range (IRQ 0-7)
		irq_num = irq_num - 8;
		// Unmask bit corresponding to irq_num
		irq_mask = ~(1 << irq_num);
		slave_mask = slave_mask & irq_mask;
		// Update mask in hardware
		outb(slave_mask, SLAVE_8259_DATA);
	}
	
	return;
}

/* disable_irq()
 * Disable (mask) the specified IRQ
 *
 * Inputs: irq_num - Index of IRQ to mask
 * Return: None
 */
void disable_irq(uint32_t irq_num) {
	
	uint8_t irq_mask;
	
	// Assert IRQ lies in the range 0x20-0x2F
	if (irq_num > 15 || irq_num < 0) {
		printf("FATAL: disable_irq() in i8259.c called with invalid irq_num \n");
		while(1);
		return;
	}
	
	// IRQ is in range 0x20-0x27
	if (irq_num < 8) {
		// Mask bit corresponding to irq_num
		irq_mask = (1 << irq_num);
		master_mask = master_mask | irq_mask;
		// Update mask in hardware
		outb(master_mask, MASTER_8259_DATA);
	}
	// IRQ is in range 0x28-2F
	else {
		// Remap IRQ to proper range (IRQ 0-7)
		irq_num = irq_num - 8;
		// Mask bit corresponding to irq_num
		irq_mask = (1 << irq_num);
		slave_mask = slave_mask | irq_mask;
		// Update mask in hardware
		outb(slave_mask, SLAVE_8259_DATA);
	}
	
	return;
}

/* send_eoi()
 * Send end-of-interrupt signal for the specified IRQ
 *
 * Inputs: irq_num - Index of IRQ to mask
 * Outputs: None
 */
void send_eoi(uint32_t irq_num) {
	
		// Assert IRQ lies in the range 0x20-0x2F
	if (irq_num > 15 || irq_num < 0) {
		printf("FATAL: send_eoi() in i8259.c called with invalid irq_num \n");
		while(1);
		return;
	}
	
	// IRQ is in range 0x20-0x27
	if (irq_num < 8) {
		// Send EOI to Master
		outb(EOI + irq_num, MASTER_8259_PORT);
	}
	// IRQ is in range 0x28-2F
	else {
		// Remap IRQ to proper range (IRQ 0-7)
		irq_num = irq_num - 8;
		// Send EOI to Slave
		outb(EOI + irq_num, SLAVE_8259_PORT);
		// Send EOI to Master
		outb(EOI + SLAVE_CASCADE_IRQ, MASTER_8259_PORT);
	}
	
	return;
}

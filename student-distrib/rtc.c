/* rtc.c
 * Generic Real Time Clock Interface Driver
 */

#include "rtc.h"
#include "lib.h"
#include "i8259.h"

/* Global Variables */
// RTC Status
uint32_t RTC_STATUS = 0;
// RTC Frequency in Hz
uint32_t RTC_FREQ = FDEFAULT;
// Virtual Elapsed Clock Ticks
uint32_t RTC_ELAPSED_TICKS_V = 0;
// Global Elapsed Clock Ticks
uint32_t RTC_ELAPSED_TICKS_G = 0;
// RTC Kernel Frequency for Terminal and Scheduler in Hz
uint32_t RTC_FREQ_KERNEL = 64;
// Wait for IRQ to be Raised
uint32_t RTC_IRQ_WAIT[TERM_MAX] = {0};

/* rtc_init()
 * Initialize the RTC
 * Inputs: None
 * Outputs: None
 */
void rtc_init(void) {
	// Generic Loop Counter
	int i;
	
	// Store the read value of Register B
	uint8_t REG_B_VAL;
	
	// Lower IRQ Wait
	for (i = 0; i < TERM_MAX; i++) RTC_IRQ_WAIT[i] = 0;
	
	// Disable all IRQs while Initializing RTC
	cli();
	
	/* Perform reads by setting address then reading data */
	{
		// Select Register B
		outb(RTC_REG_B, RTC_PORT);
		// Read the value of Register B
		REG_B_VAL = inb(RTC_CMOS);
	}
	
	/* Turn on Interrupts */
	{
		// Select Register B
		outb(RTC_REG_B, RTC_PORT);
		// Drive REG_B[6] to 1
		outb(REG_B_VAL | REG_B_MASK, RTC_CMOS);
	}
	
	// Set default frequency of 1024Hz
	rtc_set_freq(FDEFAULT);
	
	// Re-enable all IRQs
	sti();
	
	// Enable RTC Interrupts
	enable_irq(RTC_IRQ);
	
	// Reset Elapsed Time to 0
	RTC_ELAPSED_TICKS_V = 0;
	RTC_ELAPSED_TICKS_G = 0;
	
	return;
}

/* rtc_set_freq()
 * Set the clock's interval frequency
 *
 * Inputs: freq - Allowed frequency constant
 * Outputs: 0 on Success, -1 on Failure
 */
uint32_t rtc_set_freq(uint32_t freq) {	
	// Set Local Frequency
	RTC_FREQ = freq;
	return 0;
}

/* rtc_irq_handler()
 * Handler for a RTC Interrupt. When IRQ 8 is raised, read register C 
 * to determine the interrupt type. Since we are using RTC as a
 * simple periodic timer, the read value is not used at this point.
 *
 * Inputs: None
 * Outputs: None
 */
void rtc_irq_handler(void) {
	
	uint32_t IRQ_TYPE;
	uint32_t i;
	
	// Disable all IRQs while Handling RTC
	cli();
	
	// Select Register C
	outb(RTC_REG_C, RTC_PORT);
	// Read Register C to determine type of IRQ
	IRQ_TYPE = inb(RTC_CMOS);
	
	// Virtual RTC Handler
	RTC_ELAPSED_TICKS_V = RTC_ELAPSED_TICKS_V + 1;
	if (RTC_ELAPSED_TICKS_V >= (FDEFAULT / RTC_FREQ)) {
		// Reset Ticks and Increment Seconds
		RTC_ELAPSED_TICKS_V = 0;
		// Raise IRQ Wait Flag
		for (i = 0; i < TERM_MAX; i++) RTC_IRQ_WAIT[i] = 1;
	}
	
	// Global RTC Handler
	RTC_ELAPSED_TICKS_G = RTC_ELAPSED_TICKS_G + 1;
	if (RTC_ELAPSED_TICKS_G >= FDEFAULT / RTC_FREQ_KERNEL) {
		// Reset Ticks and Increment Seconds
		RTC_ELAPSED_TICKS_G = 0;
		// Refresh Terminal
		display_terminal();
	}

	// Send EOI
	send_eoi(RTC_IRQ);
	
	// Re-enable all IRQs
	sti();
	
	return;
}

/* rtc_open()
 * Empty
 * 
 * Inputs: None
 * Outputs: 0
 */
int32_t rtc_open(const uint8_t* filename) {
	
	return 0;
}

/* rtc_close()
 * Empty
 * 
 * Inputs: None
 * Outputs: 0
 */
int32_t rtc_close(void) {
	
	return 0;
}

/* rtc_read() 
 * Waits for an Interrupt to Occur and returns
 * 
 * Inputs: None
 * Outputs: 0 on Success
 */
int32_t rtc_read(unsigned int inode, unsigned int offset, void* buf, int32_t nbytes) {
	// Get this Process' Terminal
	int process_term = get_process();
	
	// Wait for IRQ to Occur
	RTC_IRQ_WAIT[process_term] = 0;
	while (RTC_IRQ_WAIT[process_term] != 1);
	return 0;
}

/* rtc_write()
 * Write the desired Frequency of RTC
 * 
 * Inputs: buffer - Pointer to Int that stores Frequency
 *		   size - Should be 4 Bytes
 * Outputs: 4 on Success, 0 on Fail
 */
int32_t rtc_write(const void* buffer, int32_t size) {
	
	uint32_t* buf = (uint32_t*) buffer;
	uint32_t freq;
	
	// Check that size must be 4 bytes
	if (size != 4 || buf == 0) {
		printf("RTC.WRITE: ERR - Size Incorrect or Buffer Empty \n");
		return 0;
	}
	
	// Check that Frequency is Valid
	if (FDEFAULT % buf[0] != 0) {
		printf("RTC.WRITE: ERR - Frequency Not Allowed \n");
		return 0;
	}
	
	// Read and Set Frequency
	freq = buf[0];
	if (rtc_set_freq(freq) == -1) {
		printf("RTC.WRITE: ERR - Set Frequency Failed \n");
		return 0;
	}
	
	return 4;
}


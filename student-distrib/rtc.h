/* rtc.h
 * Generic Real Time Clock Interface
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

/* Definitions */

// Real Time Clock is connected to IRQ 8 (IRQ 0 on Slave)
#define RTC_IRQ 	8

// Port 0x70: Register Number
#define RTC_PORT 	0x70
// Port 0x71: CMOS Configuration Space
#define RTC_CMOS 	0x71

// Index of Registers
#define RTC_REG_A 	0x8A
#define RTC_REG_B 	0x8B
#define RTC_REG_C 	0x8C
#define RTC_REG_D 	0x8D

// Divider Mask
#define DIV_MASK	0xF0
// Register B Mask
#define REG_B_MASK	0x40

// Fixed Frequency Range (2Hz - 8KHz)
#define F8192Hz	0x03
#define F4096Hz	0x04
#define F2048Hz	0x05
#define F1024Hz	0x06
#define  F512Hz	0x07
#define  F256Hz	0x08
#define  F128Hz	0x09
#define   F64Hz	0x0A
#define   F32Hz	0x0B
#define   F16Hz	0x0C
#define    F8Hz	0x0D
#define    F4Hz	0x0E
#define    F2Hz	0x0F

// Default Frequency during Initialization
#define FDEFAULT 1024

/* Functions */

/* Initialize the RTC */
void rtc_init(void);

/* Set the RTC's Frequency */
uint32_t rtc_set_freq(uint32_t freq);

/* Character Device Driver Functions */
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(void);
int32_t rtc_read(unsigned int inode, unsigned int offset, void* buf, int32_t nbytes);
int32_t rtc_write(const void* buffer, int32_t size);

/* Handler for a RTC Interrupt */
void rtc_irq_handler(void);

#endif // _RTC_H

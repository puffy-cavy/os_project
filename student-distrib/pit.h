/* pit.h
 * Programmable Interval Timer Driver
 */

#ifndef _PIT_H
#define _PIT_H

#include "types.h"

// IRQ connected to PIT
#define PIT_IRQ 	0
// I/O Port used to set Mode
#define PIT_IO		0x43
// 0x34 = 0011 0100 = Binary Mode, Operating Mode, Rate Generator, Low+High byte, Counter0
#define PIT_MODE	0x34
// Numerator of the Frequency, Pre-determined
#define INI_FRE		1193182
// Denominator of the the Frequency
#define DENO_FRE	20000
// Bits to be shifted right when obtaining the High Bits of Frequency
#define BYTE_SHIFT	8
// I/O Port for PIT Channel Zero
#define CHANNEL_ZERO	0x40

void pit_irq_handler(void);

void pit_init();

#endif

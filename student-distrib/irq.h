/* irq.h
 * Interrupt Handler Wrappers
 */

#ifndef _IRQ_H
#define _IRQ_H

// PIT IRQ Handler Wrapper
void pit_irq_wrapper();

// RTC IRQ Handler Wrapper
void rtc_irq_wrapper();

// Keyboard IRQ Handler Wrapper
void kbd_irq_wrapper();

// Mouse IRQ Handler Wrapper
void mouse_irq_wrapper();

// System Call Wrapper
void syscall_wrapper();

#endif // IRQ

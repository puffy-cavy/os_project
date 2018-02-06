/* exceptions.h
 * Exception Handlers defined by Intel
 */

#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

void division_error();

void debug_exception();

void NMI_interrupt();

void breakpoint();

void overflow();

void bound_range_exceeded();

void invalid_opcode();

void device_not_available();

void double_fault();

void coprocessor_segment();

void invalid_tss();

void segment_not_present();

void stack_segment_fault();

void general_protection();

void page_fault();

void undefined_exception();

void floating_point_error();

void alignment_check();

void machine_check();

void floating_point_exception();

void segmentation_fault();
#endif

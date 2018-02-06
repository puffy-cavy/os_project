# irq.S 
# Interrupt Handler Wrappers

#define ASM     1
#include "x86_desc.h"

# CP5: PIT Handler Wrapper
# where is the cli????????????????????????????????
.global pit_irq_wrapper
.type pit_irq_wrapper, @function
pit_irq_wrapper:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esp
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushfl

	call	pit_irq_handler

	popfl
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%esp
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	sti
	iret

# RTC Handler Wrapper
.global rtc_irq_wrapper
.type rtc_irq_wrapper, @function
rtc_irq_wrapper:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esp
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushfl
	
	call	rtc_irq_handler
	
	popfl
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%esp
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	sti
	iret

# Keyboard Handler Wrapper
.global kbd_irq_wrapper
.type kbd_irq_wrapper, @function
kbd_irq_wrapper:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esp
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushfl
	
	call	kbd_irq_handler
	
	popfl
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%esp
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	sti
	iret
# Mouse Handler Wrapper
.global mouse_irq_wrapper
.type mouse_irq_wrapper, @function
mouse_irq_wrapper:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esp
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushfl
	
	call	mouse_irq_handler
	
	popfl
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%esp
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	sti
	iret	
	
# Syscall Jump Table
syscall_tbl:
	.long	syscall_err
	.long	halt
	.long	execute
	.long	read
	.long	write
	.long	open
	.long	close
	.long	getargs
	.long	vidmap
	.long	set_handler
	.long	sigreturn
	
# Syscall Handler Wrapper
.global syscall_wrapper
.type syscall_wrapper, @function
syscall_wrapper:
	pushfl
	pushl	%esp
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	
	# Check that EAX > 1
	cmpl	$0, %eax
	jl		inval_eax
	# Check that EAX < 10
	cmpl	$10, %eax 
	jg		inval_eax
	
	pushl	%edx # Argument 3
	pushl	%ecx # Argument 2
	pushl	%ebx # Argument 1
	
	movl	syscall_tbl(, %eax, 4), %eax
	call	*%eax
	
	popl	%ebx # Pop the Argument
	popl	%ecx # Pop the Argument
	popl	%edx # Pop the Argument
	jmp		syscall_ret

inval_eax:
	movl	$-1, %eax
	
syscall_ret:
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%esp
	popfl

	sti
	iret




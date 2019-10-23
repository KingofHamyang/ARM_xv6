#include "arm.h"
#include "memlayout.h"

.global _start
.global jump_stack
.global set_stk

_start:
	ldr r1, =boot_start_addr
	ldr r2, =boot_end_addr
	mov r3, #0

1:
	cmp r1, r2
	stmltia r1!, {r3}
	blt 1b

	msr CPSR_cxsf, #(SVC_MODE|NO_INT)
	ldr sp, =svc_stack

	bl bootmain
	b .

jump_stack:
	mov r0, sp
	add r0, r0, #KERNBASE
	mov sp, r0
/*
	add sp, sp, #KERNBASE
*/
	mov pc, lr

# set the stack for IRQ mode, we can use r0-r3 for free
# input: r0 - #mode, r1 - stk
set_stk:
    # switch to the IRQ mode
    MRS     r2, cpsr
    BIC     r2, r2, #MODE_MASK
    ORR     r2, r2, r0
    MSR     cpsr_cxsf, r2

    # set the stack pointer, sp register is banked, so we need to switch mode
    MOV     sp, r1

    # switch back to the SVC mode
    BIC     r2, r2, #MODE_MASK
    ORR     r2, r2, #SVC_MODE
    MSR     cpsr_cxsf, r2

    # return
    bx      lr
//low-level trap handler glue code
#include "arm.h"

.text
.code 32

.global trap_swi
.global trap_irq
.global trap_reset
.global trap_und
.global trap_iabort
.global trap_dabort
.global trap_reserved
.global trap_fiq
.global trapret
.global set_stk
.global get_fp


# handle SWI, we allow nested SWI
trap_swi:
	# build trapframe on the stack
	stmfd   sp!, {r0-r12, r14}  // save context
	mrs     r2, spsr            // copy spsr to r2
	stmfd   r13!, {r2}          // save r2(spsr) to the stack
	stmfd   r13!, {r14}         // save r14 again to have one uniform trapframe
	stmfd   r13, {sp, lr}^      // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13             // copy r13_svc to r0
	bl      swi_handler         // branch to the isr_swi

	# restore states
trapret:
	ldmfd   r13, {sp, lr}^      // restore user mode sp and lr
	add     r13, r13, #8
	ldmfd   r13!, {r14}         // restore r14
	ldmfd   r13!, {r2}          // restore spsr
	msr     spsr_cxsf, r2
	ldmfd   r13!,{r0-r12, pc}^  // restore context and return


# handle IRQ, we allow nested IRQs
trap_irq:
	# save a few registers to the irq stack to provide scratch regs.
	# r14 (lr_irq) contains the instruction (pc) to return to, need to
	# save it on the stack as r14 is banked
	sub     r14, r14, #4            // r14 (lr) contains the interrupted PC
	stmfd   r13!, {r0-r2, r14}      //
	mrs     r1, spsr                // save spsr_irq
	mov     r0, r13                 // save stack stop (r13_irq)
	add     r13, r13, #16           // reset the IRQ stack

	# switch to the SVC mode
	mrs     r2, cpsr
	bic     r2, r2, #MODE_MASK
	orr     r2, r2, #SVC_MODE
	msr     cpsr_cxsf, r2

	# now, in SVC mode, sp, lr, pc (r13, r14, r15) are all banked
	# build the trap frame
	ldr     r2, [r0, #12]           // read the r14_irq, then save it
	stmfd   r13!, {r2}
	stmfd   r13!, {r3-r12}          // r4-r12 are preserved (non-banked)
	ldmfd   r0, {r3-r5}             // copy r0-r2 over from irq stack
	stmfd   r13!, {r3-r5}
	stmfd   r13!, {r1}              // save spsr
	stmfd   r13!, {lr}              // save r14_svc

	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# get the parameters, then call the handler
	mov     r0, r13                 // points to
	bl      irq_handler

	# restore the previous status
	b   trapret

# handle reset/undefine instruction/abort/not-assigned/fiq
# these handler does not allow nested handling
trap_reset:
	mov     r14, #0                 // lr: not defined on reset
	stmfd   r13!, {r0-r12, r14}
	mrs     r2, spsr            // copy spsr to r2
	stmfd   r13!, {r2}          // save r2(spsr) to the stack
	stmfd   r13!, {r14}         // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^      // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13             // copy r13_svc to r0
	bl      reset_handler

trap_und:
	stmfd   r13!, {r0-r12, r14} // lr: instruction after the undefined
	mrs     r2, spsr                // copy spsr to r2
	stmfd   r13!, {r2}              // save r2(spsr) to the stack
	stmfd   r13!, {r14}             // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13                 // save trapframe as the first parameter
	bl      und_handler

trap_iabort:
	sub     r14, r14, #4            // lr: instruction causing the abort
	stmfd   r13!, {r0-r12, r14}
	mrs     r2, spsr                // copy spsr to r2
	stmfd   r13!, {r2}              // save r2(spsr) to the stack
	stmfd   r13!, {r14}             // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13                 // save trapframe as the first parameter
	bl      iabort_handler

trap_dabort:
	sub     r14, r14, #8            // lr: instruction causing the abort
	stmfd   r13!, {r0-r12, r14}
	mrs     r2, spsr                // copy spsr to r2
	stmfd   r13!, {r2}              // save r2(spsr) to the stack
	stmfd   r13!, {r14}             // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13                 // save trapframe as the first parameter
	bl      dabort_handler

trap_reserved:
	stmfd   r13!, {r0-r12, r14} // should never happen, hardware error
	mrs     r2, spsr                // copy spsr to r2
	stmfd   r13!, {r2}              // save r2(spsr) to the stack
	stmfd   r13!, {r14}             // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13                 // save trapframe as the first parameter
	bl      reserved_handler

trap_fiq:
	sub     r14, r14, #4            // lr: return address after the fiq handler
	stmfd   r13!, {r0-r12, r14}
	mrs     r2, spsr                // copy spsr to r2
	stmfd   r13!, {r2}              // save r2(spsr) to the stack
	stmfd   r13!, {r14}             // save r14 again (it is not really correct)
	stmfd   r13, {sp, lr}^          // save user mode sp and lr
	sub     r13, r13, #8

	# call traps (trapframe *fp)
	mov     r0, r13                 // save trapframe as the first parameter
	bl      fiq_handler

# set the stack for IRQ mode, we can use r0-r3 for free
# input: r0 - #mode, r1 - stk
set_stk:
	# switch to the IRQ mode
	mrs     r2, cpsr
	bic     r2, r2, #MODE_MASK
	orr     r2, r2, r0
	msr     cpsr_cxsf, r2

	# set the stack pointer, sp register is banked, so we need to switch mode
	mov     sp, r1

	# switch back to the SVC mode
	bic     r2, r2, #MODE_MASK
	orr     r2, r2, #SVC_MODE
	msr     cpsr_cxsf, r2

	# return
	bx      lr

get_fp:
	mov     r0, fp
	bx      lr

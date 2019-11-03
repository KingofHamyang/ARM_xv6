#include "arm.h"
#include "memlayout.h"

.global _start

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
	

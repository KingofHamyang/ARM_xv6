.global _start

_start:
	mrs r0, CPSR
	orr r0, #0xC0
	msr CPSR_c, r0
	mov sp, #0xC
	bl armbootmain
spin:
	b spin


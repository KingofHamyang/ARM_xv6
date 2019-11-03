// The ARM UART, a memory mapped device
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"

void swi_handler (struct trapframe *r) {
	if (proc->killed) exit();
	proc->tf = r;
	syscall();
	if (proc->killed) exit();
}

void irq_handler (struct trapframe *r) {
	if (proc) proc->tf = r;
	pic_dispatch(r);
}

void reset_handler (struct trapframe *r) {
	cli();
	cprintf("reset at: 0x%x \n", r->pc);
	panic("reset - you reset the pc!\n");
}

void und_handler (struct trapframe *r) {
	cli();
	cprintf("und at: 0x%x \n", r->pc);
	panic("undefined instruction exception\n");
}

__attribute__((always_inline))
static inline void _abort_reason(uint fault_status) {
	if ((fault_status & 0xd) == 0x1)       // Alignment failure
		cprintf("reason: alignment\n");
	else if ((fault_status & 0xd) == 0x5)  // External abort "on translation"
		cprintf("reason: ext. abort on trnslt.\n");
	else if ((fault_status & 0xd) == 0x5)  // Translation
		cprintf("reason: sect. translation\n");
	else if ((fault_status & 0xd) == 0x9)  // Domain
		cprintf("reason: sect. domain\n");
	else if ((fault_status & 0xd) == 0xd)  // Permission
		cprintf("reason: sect. permission\n");
	else if ((fault_status & 0xd) == 0x8)  // External abort
		cprintf("reason: ext. abort\n");
	else cprintf("reason: unknown???\n");
}

void dabort_handler (struct trapframe *r) {
	uint dfs, dfa;

	__asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 0": "=r"(dfs)::);
	__asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 0": "=r"(dfa)::);

	cli();
	cprintf("data abort at 0x%x, status 0x%x\n",
			dfa, dfs);
	_abort_reason(dfs);
	dump_tf(r);
	if (r->pc < KERNBASE) // Exception occured in User space: exit
		exit();
	else                  // Exception occured in Kernel space: panic
		panic("data abort exception");
}

void iabort_handler (struct trapframe *r) {
	uint ifs, ifa;

	__asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 1": "=r"(ifs)::);
	__asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 2": "=r"(ifa)::);

	cli();
	cprintf("prefetch abort at 0x%x, status 0x%x\n",
			ifa, ifs);
	_abort_reason(ifs);
	dump_tf(r);
	if (r->pc < KERNBASE) // Exception occured in User space: exit
		exit();
	else                  // Exception occured in Kernel space: panic
		panic("prefetch abort exception");
}

void reserved_handler (struct trapframe *r) {
	cli();
	cprintf ("n/a at: 0x%x \n", r->pc);
	panic("reserved exception");
}

void fiq_handler (struct trapframe *r) {
	// Unfortunately, fiq handler is not implemented.
	cli();
	cprintf ("fiq at: 0x%x \n", r->pc);
	panic("fiq exception - fiq not implemented, sorry!");
}

// low-level init code: in real hardware, lower memory is usually mapped
// to flash during startup, we need to remap it to SDRAM
void trap_init () {
	volatile uint *ex_table;
	char *stk;
	int i;
	uint modes[] = {FIQ_MODE, IRQ_MODE, ABT_MODE, UND_MODE};

	// create the excpetion vectors
	ex_table = (uint*)VEC_TBL;

	// Set Interrupt handler start address
	// 0xE59FF018 == `LDR pc, [pc, #0x18]`
	for (i = 0 ; i < 8; i++) ex_table[i] = 0xE59FF018U;

	ex_table[8]  = (uint)trap_reset;     // Reset (SVC)
	ex_table[9]  = (uint)trap_und;       // Undefined Instruction
	ex_table[10] = (uint)trap_swi;       // Software Interrupt
	ex_table[11] = (uint)trap_iabort;    // Prefetch Abort
	ex_table[12] = (uint)trap_dabort;    // Data Abort
	ex_table[13] = (uint)trap_reserved;  // Reserved
	ex_table[14] = (uint)trap_irq;       // IRQ
	ex_table[15] = (uint)trap_fiq;       // FIQ

	// initialize the stacks for different mode
	for (i = 0; i < sizeof(modes) / sizeof(uint); i++) {
		if (!(stk = kalloc()))
			panic("irq stack allocation failed");
		set_stk(modes[i], (uint)stk);
	}
}

void dump_tf (struct trapframe *tf) {
	cprintf ("r14_svc: 0x%x\n", tf->r14_svc);
	cprintf ("   spsr: 0x%x\n", tf->spsr);
	cprintf ("     r0: 0x%x\n", tf->r0);
	cprintf ("     r1: 0x%x\n", tf->r1);
	cprintf ("     r2: 0x%x\n", tf->r2);
	cprintf ("     r3: 0x%x\n", tf->r3);
	cprintf ("     r4: 0x%x\n", tf->r4);
	cprintf ("     r5: 0x%x\n", tf->r5);
	cprintf ("     r6: 0x%x\n", tf->r6);
	cprintf ("     r7: 0x%x\n", tf->r7);
	cprintf ("     r8: 0x%x\n", tf->r8);
	cprintf ("     r9: 0x%x\n", tf->r9);
	cprintf ("    r10: 0x%x\n", tf->r10);
	cprintf ("    r11: 0x%x\n", tf->r11);
	cprintf ("    r12: 0x%x\n", tf->r12);
	cprintf ("     pc: 0x%x\n", tf->pc);
}


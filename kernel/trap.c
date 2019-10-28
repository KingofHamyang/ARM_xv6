// The ARM UART, a memory mapped device
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"

// trap routine
void swi_handler (struct trapframe *r) {
	if (proc->killed)
		exit();
	proc->tf = r;
	syscall();
	if (proc->killed)
		exit();
}

// trap routine
void irq_handler (struct trapframe *r)
{
	// proc points to the current process. If the kernel is
	// running scheduler, proc is NULL.
	if (proc != NULL) {
		proc->tf = r;
	}
	pic_dispatch (r);
}

// trap routine
void reset_handler (struct trapframe *r) {
	cli();
	cprintf ("reset at: 0x%x \n", r->pc);
	panic("reset - you reset the pc!\n");
}

// trap routine
void und_handler (struct trapframe *r)
{
	cli();
	cprintf ("und at: 0x%x \n", r->pc);
	panic("undefined exception\n");
}

// trap routine
void dabort_handler (struct trapframe *r) {
	uint dfs, dfa;

	__asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 0": "=r"(dfs)::);
	__asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 0": "=r"(dfa)::);

	cli();
	cprintf ("data abort: inst 0x%x,\n"
			 "data fault at 0x%x, status 0x%x \n",
			 r->pc, dfa, dfs);

	dump_trapframe (r);
	if (r->pc < KERNBASE) {
		// This instruction is from User program. exit...
		exit();
	} else {
		// This instruction is from Kernel.
		panic("data abort exception");
	}
}

// trap routine
void iabort_handler (struct trapframe *r)
{
	uint ifs, ifa;

	// read fault status register
	// 얘 레퍼런스 잘못 본 거 같은데
	__asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 1": "=r"(ifs)::);
	__asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 2": "=r"(ifa)::);

	cli();
	cprintf ("inst prefetch abort: inst 0x%x,\n"
			 "inst       at 0x%x, status 0x%x \n",
			 r->pc, ifa, ifs);
	dump_trapframe (r);
	if (r->pc < KERNBASE) {
		// This instruction is from User program. exit...
		exit();
	} else {
		// This instruction is from Kernel.
		panic("inst. prefetch abort exception");
	}
}

// trap routine
void na_handler (struct trapframe *r)
{
	cli();
	cprintf ("n/a at: 0x%x \n", r->pc);
	panic("reserved exception");
}

// trap routine
void fiq_handler (struct trapframe *r)
{
	cli();
	cprintf ("fiq at: 0x%x \n", r->pc);
	panic("fiq exception - fiq not implemented, sorry!");
}

// low-level init code: in real hardware, lower memory is usually mapped
// to flash during startup, we need to remap it to SDRAM
void trap_init ()
{
	volatile uint *ram_start;
	char *stk;
	int i;
	uint modes[] = {FIQ_MODE, IRQ_MODE, ABT_MODE, UND_MODE};

	// the opcode of PC relative load (to PC) instruction LDR pc, [pc,...]
	static uint const LDR_PCPC = 0xE59FF000U;

	// create the excpetion vectors
	ram_start = (uint*)VEC_TBL;

  for(int i = 0 ; i<8; i++){
    ram_start[i] = LDR_PCPC | 0x18;
    // Set Interrupt handler start address
  }
  // Reset (SVC)
  // Undefine Instruction
  // Software interrupt 소프트웨어 인터럽트
  // Prefetch abort
  // Data abort
  // Not assigned
  // IRQ
  // FIQ

	ram_start[8]  = (uint)trap_reset;
	ram_start[9]  = (uint)trap_und;
	ram_start[10] = (uint)trap_swi;
	ram_start[11] = (uint)trap_iabort;
	ram_start[12] = (uint)trap_dabort;
	ram_start[13] = (uint)trap_na;
	ram_start[14] = (uint)trap_irq;
	ram_start[15] = (uint)trap_fiq;

	// initialize the stacks for different mode
	for (i = 0; i < sizeof(modes)/sizeof(uint); i++) {
		stk = alloc_page ();

		if (stk == NULL) {
			panic("failed to alloc memory for irq stack");
		}

		set_stk (modes[i], (uint)stk);
	}
}

void dump_trapframe (struct trapframe *tf)
{
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


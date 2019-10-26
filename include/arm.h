#ifndef _ARM_H_
#define _ARM_H_

#include "device.h"

// trap frame: in ARM, there are seven modes. Among the 16 regular registers,
// r13 (sp), r14(lr), r15(pc) are banked in all modes.
// 1. In xv6_a, all kernel level activities (e.g., Syscall and IRQ) happens
// in the SVC mode. CPU is put in different modes by different events. We
// switch them to the SVC mode, by shoving the trapframe to the kernel stack.
// 2. during the context switched, the banked user space registers should also
// be saved/restored.
//
// Here is an example:
// 1. a user app issues a syscall (via SWI), its user-space registers are
// saved on its kernel stack, syscall is being served.
// 2. an interrupt happens, it preempted the syscall. the app's kernel-space
// registers are again saved on its stack.
// 3. interrupt service ended, and execution returns to the syscall.
// 4. kernel decides to reschedule (context switch), it saves the kernel states
// and switches to a new process (including user-space banked registers)

// Interrupt control bits
#define NO_INT      0xc0 // disable IRQ.
#define DIS_INT     0x80 // disable both IRQ and FIQ.

// ARM modes.
#define MODE_MASK   0x1f
#define USR_MODE    0x10
#define FIQ_MODE    0x11
#define IRQ_MODE    0x12
#define SVC_MODE    0x13
#define ABT_MODE    0x17
#define UND_MODE    0x1b
#define SYS_MODE    0x1f

// vector table
#define TRAP_RESET  0
#define TRAP_UND    1
#define TRAP_SWI    2
#define TRAP_IABT   3
#define TRAP_DABT   4
#define TRAP_NA     5
#define TRAP_IRQ    6
#define TRAP_FIQ    7

#ifndef __ASSEMBLER__
#include "memlayout.h"

#define dmb() __asm__ __volatile__ ("dmb":::"memory")

// Some gcc ignore inline; so force inline it.
static inline uint strex(volatile uint *, uint) __attribute__((always_inline));
static inline uint ldrex(volatile uint *) __attribute__((always_inline));
static inline uint xchg(volatile uint *, uint) __attribute__((always_inline));

static void cli(void) {
	uint val;

	__asm__ __volatile__ (
		"mrs %0, cpsr"
			: "=r"(val)
			:
			:
	);
	val |= DIS_INT;
	__asm__ __volatile__ (
		"msr cpsr_cxsf, %0"
			:
			: "r"(val)
			:
	);
}

static void sti(void) {
	uint val;

	__asm__ __volatile__ (
		"mrs %0, cpsr"
			: "=r"(val)
			:
			:
	);
	val &= ~DIS_INT;
	__asm__ __volatile__ (
		"msr cpsr_cxsf, %0"
			:
			: "r"(val)
			:
	);
}

static uint spsr_usr() {
	uint val;

	__asm__ __volatile__ (
		"mrs %0, cpsr"
			: "=r"(val)
			:
			:
	);
	val &= ~MODE_MASK;
	val |= USR_MODE;

	return val;
}

static inline int is_int() {
	uint val;

	__asm__ __volatile__ (
		"mrs %0, cpsr"
			: "=r"(val)
			:
			:
	);

	return !(val & DIS_INT);
}

static inline uint ldrex(volatile uint *addr) {
	// Load link.
	uint res;

	__asm__ __volatile__ (
		"ldrex  %0, [%1]\n"
			: "=&r"(res)
			: "r"(addr)
			: "memory","cc"
	);
	return res;
}

static inline uint strex(volatile uint *addr, uint newval) {
	// Load link.
	uint res;

	__asm__ __volatile__ (
		"    strex  %0, %1, [%2]\n"
			: "=&r"(res)
			: "r"(newval), "r"(addr)
			: "memory","cc"
	);
	return res;
}

static inline uint xchg(volatile uint *addr, uint newval) {
	// reference:
	// https://github.com/torvalds/linux/blob/master/arch/arm/include/asm/atomic.h
	// https://github.com/torvalds/linux/blob/master/arch/arm/include/asm/cmpxchg.h
	uint res, tmp;

	__asm__ __volatile__ (
		"1:  ldrex  %0, [%3]\n"
		"    strex  %1, %2, [%3]\n"
		"    cmp    %1, #0\n"
		"    bne    1b"
			: "=&r"(res), "=&r"(tmp)
			: "r"(newval), "r"(addr)
			: "memory","cc"
	);
	return res;
}

struct trapframe {
	uint    sp_usr;     // user mode sp
	uint    lr_usr;     // user mode lr
	uint    r14_svc;    // r14_svc (r14_svc == pc if SWI)
	uint    spsr;
	uint    r0;
	uint    r1;
	uint    r2;
	uint    r3;
	uint    r4;
	uint    r5;
	uint    r6;
	uint    r7;
	uint    r8;
	uint    r9;
	uint    r10;
	uint    r11;
	uint    r12;
	uint    pc;         // (lr on entry) instruction to resume execution
};
#endif

#endif

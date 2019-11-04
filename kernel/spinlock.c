// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

#define MULTICORE 0

void initlock(struct spinlock *lk, char *name) {
	lk->name = name;
	lk->locked = 0;
	lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void acquire(struct spinlock *lk) {
	pushcli(); // disable interrupts to avoid deadlock.

	if (holding(lk)) {
		panic("acquire");
	}

	while (xchg(&lk->locked, 1) != 0);

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that the critical section's memory
	// references happen after the lock is acquired.
	DMB(); // __sync_synchronize() is deprecated.
	// Record info about lock acquisition for debugging.
	lk->cpu = cpu;
	getcallerpcs(get_fp(), lk->pcs);
}

// Release the lock.
void release(struct spinlock *lk) {
	uint tmp;

	if (!holding(lk)) {
		panic("release");
	}

	lk->pcs[0] = 0;
	lk->cpu = 0;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that all the stores in the critical
	// section are visible to other cores before the lock is released.
	// Both the C compiler and the hardware may re-order loads and
	// stores; __sync_synchronize() tells them both not to.
	DMB(); // __sync_synchronize() is deprecated.

	// Release the lock, equivalent to lk->locked = 0.
	// This code can't use a C assignment, since it might
	// not be atomic. A real OS would use C atomics here.
	lk->locked = 0; // 뭐 어쩔건데 걍 대입해...

	popcli();
}

// Record the current call stack in pcs[] by following the call chain.
// In ARM ABI, the function prologue is as:
//		push	{fp, lr}
//		add		fp, sp, #4
// so, fp points to lr, the return address
void getcallerpcs (void * v, uint pcs[]) {
	uint *fp;
	int i;

	fp = (uint *) v;

	for (i = 0; i < STACK_DEPTH; i++) {
		if (fp == 0 ||
			fp < (uint *) KERNBASE ||
			fp == (uint *) 0xFFFFFFFF) break;

		fp -= 1;			      // points fp to the saved fp
		pcs[i] = fp[1];           // saved lr
		fp = (uint *) fp[0];      // saved fp
	}

	for (; i < STACK_DEPTH; i++) pcs[i] = 0;
}

void show_callstk(char *s) {
	int i;
	uint pcs[STACK_DEPTH];

	cprintf("%s\n", s);

	getcallerpcs(get_fp(), pcs);

	for (i = 0; i < STACK_DEPTH && pcs[i]; i++)
		cprintf("STACK #%d: 0x%x\n", i + 1, pcs[i]);
}

// Check whether this cpu is holding the lock.
int holding(struct spinlock *lock) {
	int r;
	pushcli();
#if MULTICORE
	r = lock->locked && lock->cpu = cpu;
#else
	r = lock->locked;
#endif
	popcli();
	return r;
}

void pushcli (void) {
	cli();
	if (cpu->ncli == 0)
		cpu->intena = is_int();
	cpu->ncli += 1;
}

void popcli (void) {
	if (is_int()) {
		panic("popcli - interruptible");
	}

	if (--cpu->ncli < 0) {
		cprintf("cpu (%d)->ncli: %d\n", cpu, cpu->ncli);
		panic("popcli -- ncli < 0");
	}

	if (cpu->ncli == 0 && cpu->intena)
		sti();
}

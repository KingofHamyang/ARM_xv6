// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

// For single CPU systems, there is no need for spinlock.
// Add the support when multi-processor is supported.


// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void acquire(struct spinlock *lk)
{
    pushcli();		// disable interrupts to avoid deadlock.
    lk->locked = 1;	// set the lock status to make the kernel happy

#if 0
    if(holding(lk))
        panic("acquire");

    // The xchg is atomic.
    // It also serializes, so that reads after acquire are not
    // reordered before it.
    while(xchg(&lk->locked, 1) != 0)
        ;

    // Record info about lock acquisition for debugging.
    lk->cpu = cpu;
    getcallerpcs(get_fp(), lk->pcs);

#endif
}

// Release the lock.
void release(struct spinlock *lk)
{
#if 0
    if(!holding(lk))
        panic("release");

    lk->pcs[0] = 0;
    lk->cpu = 0;

    // The xchg serializes, so that reads before release are
    // not reordered after it.  The 1996 PentiumPro manual (Volume 3,
    // 7.2) says reads can be carried out speculatively and in
    // any order, which implies we need to serialize here.
    // But the 2007 Intel 64 Architecture Memory Ordering White
    // Paper says that Intel 64 and IA-32 will not move a load
    // after a store. So lock->locked = 0 would work here.
    // The xchg being asm volatile ensures gcc emits it after
    // the above assignments (and after the critical section).
    xchg(&lk->locked, 0);
#endif

    lk->locked = 0; // set the lock state to keep the kernel happy
    popcli();
}

// Record the current call stack in pcs[] by following the call chain.
// In ARM ABI, the function prologue is as:
//		push	{fp, lr}
//		add		fp, sp, #4
// so, fp points to lr, the return address
void getcallerpcs (void * v, uint pcs[])
{
    uint *fp;
    int i;

    fp = (uint*) v;

    for (i = 0; i < N_CALLSTK; i++) {
        if ((fp == 0) || (fp < (uint*)KERNBASE) || (fp == (uint*) 0xffffffff)) {
            break;
        }

        fp -= 1;			// points fp to the saved fp
        pcs[i] = fp[1];     // saved lr
        fp = (uint*) fp[0];	// saved fp
    }

    for (; i < N_CALLSTK; i++) {
        pcs[i] = 0;
    }
}

void show_callstk(char *s)
{
    int i;
    uint pcs[N_CALLSTK];

    cprintf("%s\n", s);

    getcallerpcs((void *)get_fp(), pcs);

    for (i = N_CALLSTK - 1; i >= 0; i--) {
        cprintf("%d: 0x%x\n", i + 1, pcs[i]);
    }
}

// Check whether this cpu is holding the lock.
int holding(struct spinlock *lock)
{
    return lock->locked; // && lock->cpu == cpus;
}


void pushcli (void)
{
    int enabled;

    enabled = int_enabled();

    cli();

    if (cpu->ncli++ == 0) {
        cpu->intena = enabled;
    }
}

void popcli (void)
{
    if (int_enabled()) {
        panic("popcli - interruptible");
    }

    if (--cpu->ncli < 0) {
        cprintf("cpu (%d)->ncli: %d\n", cpu, cpu->ncli);
        panic("popcli -- ncli < 0");
    }

    if ((cpu->ncli == 0) && cpu->intena) {
        sti();
    }
}

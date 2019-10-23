#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"

#define MB (1 << 20)

extern void* kern_end;

struct cpu   cpus[NCPU];
struct cpu * cpu;

void kmain (void) {
    uint vectbl;

    cpu = &cpus[0];

    vectbl = P2V_WO (VEC_TBL & PDE_MASK);

    uart_init(P2V(UART0));

    // interrrupt vector table is in the middle of first 1MB. We use the left
    // over for page tables
    init_vmm();
    kpt_freerange(ALIGNUP(&kern_end, PT_SZ), vectbl);
    kpt_freerange(vectbl + PT_SZ, P2V_WO(INIT_KERNMAP));
    paging_init(INIT_KERNMAP, PHYSTOP);
    
    kmem_init ();
    kmem_init2(P2V(INIT_KERNMAP), P2V(PHYSTOP));
    
    trap_init ();				// vector table and stacks for models
    pic_init (P2V(VIC_BASE));	// interrupt controller
    uart_enable_rx ();			// interrupt for uart
    consoleinit ();				// console
    pinit ();					// process (locks)

    binit ();					// buffer cache
    fileinit ();				// file table
    iinit ();					// inode cache
    ideinit ();					// ide (memory block device)
    timer_init (HZ);			// the timer (ticker)

    sti ();

    userinit();					// first user process
    scheduler();				// start running processes
}

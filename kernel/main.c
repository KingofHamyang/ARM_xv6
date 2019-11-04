#include "types.h"
#include "defs.h"
#include "device.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"

extern char kern_end[];

struct cpu   cpus[NCPU];
struct cpu * cpu;

void kmain (void) {
	cpu = &cpus[0];

	// interrrupt vector table is in the middle of first 1MB. We use the left
	// over for page tables
	uart_init(P2V(UART0));
	cprintf("paging enabled.\n");

	kinit1();   // phys page allocator
	freerange(kern_end, P2V(PHYSTOP));

	trap_init();				// vector table and stacks for models
	pic_init(P2V(VIC_BASE));	// interrupt controller
	uart_enable_rx();			// interrupt for uart
	consoleinit();				// console
	pinit();					// process (locks)
	binit();					// buffer cache
	fileinit();				// file table
	iinit();					// inode cache
	ideinit();					// ide (memory block device)
	timer_init(HZ);			// the timer (ticker)

	kinit2();

	userinit();					// first user process
	scheduler();				// start running processes
}


#define MB (1 << 20)

extern void *end;

struct cpu cpus[NCPU];
struct cpu *cpu;

int main (void) {
	/*
	kinit1(end, P2V(4*1024*1024)); // phys page allocator
	kvmalloc();      // kernel page table
	mpinit();        // detect other processors
	lapicinit();     // interrupt controller
	seginit();       // segment descriptors
	picinit();       // disable pic
	ioapicinit();    // another interrupt controller
	consoleinit();   // console hardware
	uartinit();      // serial port
	pinit();         // process table
	tvinit();        // trap vectors
	binit();         // buffer cache
	fileinit();      // file table
	ideinit();       // disk 
	startothers();   // start other processors
	kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
	userinit();      // first user process
	mpmain();        // finish this processor's setup
	*/
}
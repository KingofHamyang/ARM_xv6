// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "device.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

extern char kern_end[]; // first address after kernel loaded from ELF file
                        // defined by the kernel linker script in kernel.ld

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;
	int use_lock;
	struct run *freelist;
} kmem;

void kinit1() {
	initlock(&kmem.lock, "kmem");
	kmem.use_lock = 0;
}

void kinit2() {
	kmem.use_lock = 1;
}

void freerange(void *vstart, void *vend) {
	char *p;
	p = (char *)ALIGNUP((uint)vstart, 4 * PGSIZE);
	for (; p +  4 * PGSIZE <= (char *)vend; p += 4 * PGSIZE)
		kfree(p);
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(char *v) {
	struct run *r;

	if ((uint)v % (4 * PGSIZE))
		panic("kfree - unaligned");
	if (v < kern_end)
		panic("kfree - under kern_end");
	if (V2P(v) >= PHYSTOP)
		panic("kfree - over PHYSTOP");

	// Fill with junk to catch dangling refs.
	memset(v, 0, 4 * PGSIZE);

	if (kmem.use_lock)
		acquire(&kmem.lock);

	r = (struct run *)v;
	r->next = kmem.freelist;
	kmem.freelist = r;

	if (kmem.use_lock)
		release(&kmem.lock);
}

// Allocate one 16KB page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char * kalloc(void) {
	struct run *r;

	if (kmem.use_lock)
		acquire(&kmem.lock);

	if ((r = kmem.freelist))
		kmem.freelist = r->next;

	if (kmem.use_lock)
		release(&kmem.lock);

	return (char *)r;
}

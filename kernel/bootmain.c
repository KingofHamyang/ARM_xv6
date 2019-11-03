#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

__attribute__((always_inline))
static inline void _puts(char *s) { while (*s) *((volatile uchar *)UART0) = *s++; }

extern uint kern_data;
extern uint kern_end;

extern uint _dev_pte;
extern uint _pte;
extern uint _kt;
extern uint _ut;

uint *dev_pte = &_dev_pte;
uint *pte = &_pte;
uint *kt = &_kt;
uint *ut = &_ut;

extern void kmain(void);

static void build_pages() {
	uint i, pde, vec_table;

	// check kernel end position.
	vec_table = P2V_WO(VEC_TBL & PDE_MASK);
	if (vec_table <= (uint)&kern_end) {
		_puts("panic: vector table overlaps kernel\n");
		for (;;);
	}

	// First, connect "Coarse PTE ~ 4K pages" (0x1000)
	// PHYSTOP is 128MB = 0x0800_0000.
	// DRAM page area is bufferable and cacheable.
	for (i = 0; i < 0x8000; i++)
		pte[i] = (i<<12) | PTE_BUF | PTE_CACHE | PTE_PA | PTE_SMALL;

	// Second, connect "_kt & _ut ~ Coarse PTEs" for DRAM.
	// PT unit: 0x400 = 1KB
	for (i = 0; i < 0x80; i++) {
		pde = (uint)(&pte[i<<8]) | PDE_COARSE;

		// set_bootpgtbl(0, 0, INIT_KERNMAP, 0);
		ut[i] = pde;
    	// set_bootpgtbl(KERNBASE, 0, INIT_KERNMAP, 0);
		kt[i+0x800] = pde;
	}

	// set_bootpgtbl(VEC_TBL, 0, 1 << PDE_SHIFT, 0);
	kt[VEC_TBL>>20] = (uint)pte | PDE_COARSE;

	// Lastly, connect DEVBASE.
    // set_bootpgtbl(KERNBASE+DEVBASE, DEVBASE, DEV_MEM_SZ, 1);
	// DEVSPACE is between 0x1000_0000 ~ 0x101F_5000
	// So, I will 2MB (0x0020_0000) space to there.
	// We need 512 Small Pages, this need 2 Coarse Page Table.
	for (i = 0; i < 0x200; i++) dev_pte[i] = (DEVBASE+(i<<12)) | PTE_PA | PTE_SMALL;
	kt[(KERNBASE+DEVBASE)>>20] = (uint)(&dev_pte[0]) | PDE_COARSE;
	kt[((KERNBASE+DEVBASE)>>20)+1] = (uint)(&dev_pte[0x100]) | PDE_COARSE;
}

static void load_pgtbl() {
	uint val;

	__asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0": : "r"(0x55555555):); // set domain access control as client
	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 2": : "r"(1):);// set the page table base registers.
	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 1": : "r"((uint)kt):); // set the kernel page table
	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0": : "r"((uint)ut):); // set the user page table

	// Enable paging using read/modify/write
	__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0": "=r"(val)::);
	val |= 0x3805; // Enable MMU, cache, write buffer, high vector tbl. Disable subpage.
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r"(val):);

	// flush all TLB
	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r"(0):);
	__asm__ __volatile__ ("mcr p15,0,%0,c7,c5,0": : "r"(0):);
	__asm__ __volatile__ ("mcr p15,0,%0,c7,c6,0": : "r"(0):);
}

void bootmain() {
	_puts("Start xv6...\n");

	build_pages();
	load_pgtbl();
	/*    -- PAGING START --    */
	// jump_stack();
	__asm__ __volatile__ ("add sp, sp, #0x80000000");
	// clear_bss();
	memset(&kern_data, 0x00, (uint)&kern_end - (uint)&kern_data);

	kmain();
}

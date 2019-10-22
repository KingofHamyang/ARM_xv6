#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

void _puts(char *s) {
	//volatile uint8 *u = (uint8 *)UART0;
	for (; *s; s++) *((volatile uint8 *)UART0) = *s;
}

extern uint32 _kt;
extern uint32 _ut;

uint32 *kern_table = &_kt;
uint32 *user_table = &_ut;

uint32 get_pde(uint32 v) { return kern_table[v >> PDE_SHIFT]; }

void set_bootpgtbl(uint32 v, uint32 p, uint len, int is_dev) {
	uint32 pde;
	int i;

	v >>= PDE_SHIFT;
	p >>= PDE_SHIFT;
	len >>= PDE_SHIFT;

    for (i = 0; i < len; i++) {
        pde = (p << PDE_SHIFT);

        if (!is_dev) {
			// normal memory, make it kernel-only, cachable, bufferable
            pde |= (AP_KO << 10) | PE_CACHE | PE_BUF | KPDE_TYPE;
		} else  {
			// device memory, make it non-cachable and non-bufferable
			pde |= (AP_KO << 10) | KPDE_TYPE;
		}
            
        // use different page table for user/kernel space
        if (v < NUM_UPDE) {
			user_table[v] = pde;
		} else {
			kern_table[v] = pde;
		}

        v++;
        p++;
    }
}

static void _flush_all (void)
{
    uint val = 0;

    // flush all TLB
    asm volatile("MCR p15, 0, %[r], c8, c7, 0" : :[r]"r" (val):);

    // invalid entire data and instruction cache
    // asm volatile("MCR p15,0,%[r],c7,c5,0": :[r]"r" (val):);
    // asm volatile("MCR p15,0,%[r],c7,c6,0": :[r]"r" (val):);
}

void load_pgtlb (uint32* kern_pgtbl, uint32* user_pgtbl)
{
    volatile uint val;

    // we need to check the cache/tlb etc., but let's skip it for now

    // set domain access control: all domain will be checked for permission
    val = 0x55555555;
    asm volatile ("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

    // set the page table base registers. We use two page tables: TTBR0
    // for user space and TTBR1 for kernel space
    val = 0x20 - UADDR_BITS;
    asm volatile("MCR p15, 0, %[v], c2, c0, 2": :[v]"r" (val):);

    // set the kernel page table
    val = (uint)kern_pgtbl | 0;
    asm volatile("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);

    // set the user page table
    val = (uint)user_pgtbl | 0;
    asm volatile("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);

    // ok, enable paging using read/modify/write
    asm volatile("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::);

    val |= 0x80300D; // enable MMU, cache, write buffer, high vector tbl,
                     // disable subpage
    asm volatile("MCR p15, 0, %[r], c1, c0, 0": :[r]"r" (val):);

    _flush_all();
}

extern void *boot_start_addr;
extern void *svc_stack;
extern void kmain(void);
extern void jump_stack(void);

extern void *kern_data_end;
extern void *kern_end;

void clear_bss (void) {
	memset(&kern_data_end, 0x00, (uint)&kern_end-(uint)&kern_data_end);
}

void bootmain(void) {
	uint32 vec_table;

    _puts("Start xv6...\n");

    set_bootpgtbl(0, 0, INIT_KERNMAP, 0);
    set_bootpgtbl(KERNBASE, 0, INIT_KERNMAP, 0);

	vec_table = P2V_WO(VEC_TBL & PDE_MASK);
	if (vec_table <= (uint)&kern_end) {
		_puts("error: vector table overlaps kernel\n");
		for(;;);
	}
	set_bootpgtbl(VEC_TBL, 0, 1 << PDE_SHIFT, 0);
	set_bootpgtbl(KERNBASE+DEVBASE, DEVBASE, DEV_MEM_SZ, 1);
	_puts("Success!!!!\n");
	load_pgtlb(kern_table, user_table);
	jump_stack();
	clear_bss();
	kmain();
}

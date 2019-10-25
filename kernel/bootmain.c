#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

void _puts(char *s) { while (*s) *((volatile uchar *)UART0) = *s++; }


extern uint _kt;
extern uint _ut;

uint *kern_table = &_kt;
uint *user_table = &_ut;

void set_bootpgtbl(uint v, uint p, uint len, int is_dev) {
	uint pde;
	int i;

	v >>= PDE_SHIFT;
	p >>= PDE_SHIFT;
	len >>= PDE_SHIFT;

    for (i = 0; i < len; i++) {
        pde = ((p+i) << PDE_SHIFT) | (AP_KO << 10) | KPDE_TYPE;

        if (!is_dev) pde |= PE_CACHE | PE_BUF;            
        // use different page table for user/kernel space
        if (v+i < NUM_UPDE) user_table[v+i] = pde;
        else kern_table[v+i] = pde;
    }
}

void load_pgtbl(uint *kern_pgtbl, uint *user_pgtbl) {
    uint val;

    __asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0": : "r"(0x55555555):); // set domain access control as client
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 2": : "r"(0x20 - UADDR_BITS):);// set the page table base registers.
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 1": : "r"((uint)kern_pgtbl):); // set the kernel page table
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0": : "r"((uint)user_pgtbl):); // set the user page table

    // Enable paging using read/modify/write
    __asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0": "=r"(val)::);
    val |= 0x80300D; // Enable MMU, cache, write buffer, high vector tbl. Disable subpage.
    __asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r"(val):);

    // flush all TLB
    __asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r"(0):);
    __asm__ __volatile__ ("mcr p15,0,%0,c7,c5,0": : "r"(0):);
    __asm__ __volatile__ ("mcr p15,0,%0,c7,c6,0": : "r"(0):);
}

extern void *boot_start_addr;
extern void *svc_stack;
extern void kmain(void);
extern void jump_stack(void);

extern void *kern_data_end;
extern void *kern_end;

static inline void clear_bss (void) { memset(&kern_data_end, 0x00, (uint)&kern_end - (uint)&kern_data_end); }

void bootmain(void) {
	uint vec_table;

    _puts("Start xv6...\n");

    set_bootpgtbl(0, 0, INIT_KERNMAP, 0);
    set_bootpgtbl(KERNBASE, 0, INIT_KERNMAP, 0);

	vec_table = P2V_WO(VEC_TBL & PDE_MASK);
	if (vec_table <= (uint)&kern_end) {
		_puts("error: vector table overlaps kernel\n");
        for (;;);
	}
	set_bootpgtbl(VEC_TBL, 0, 1 << PDE_SHIFT, 0);
	set_bootpgtbl(KERNBASE+DEVBASE, DEVBASE, DEV_MEM_SZ, 1);
	load_pgtbl(kern_table, user_table);
	jump_stack();
	clear_bss();
	kmain();
}

#include "param.h"
#include "types.h"
#include "defs.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "elf.h"

extern uint kern_text;  // defined by kernel.ld
extern uint kern_data;  // defined by kernel.ld
extern uint kern_end;   // defined by kernel.ld
/*
typedef struct {
	uint page_i : 12;
	uint pt_i : 8;
	uint pd_i : 12;
} virt;
typedef struct {
	uint t_type : 2;
	uint t_resv : 3;
	uint t_dom : 4;
	uint t_unk : 1;
	uint t_base : 22;
} pde_s;
typedef struct {
	uint p_type : 2;
	uint p_buf : 1;
	uint p_cache : 1;
	uint p_ap : 2;
	uint p_tex : 3;
	uint p_apx : 1;
	uint p_resv : 2;
	uint p_base : 20;
} pte_s;
*/
// Return the address of the PTE in page directory that corresponds to
// virtual address va.  If alloc!=0, create any required page table pages.
static pte_t* walkpgdir(pde_t *pgdir, const void *va, int alloc) {
	pde_t *pde;
	pte_t *pgtab;

	// pgdir points to the page directory, get the page direcotry entry (pde)
	pde = &pgdir[PDE_IDX(va)];

	if (*pde & PDE_TYPES)
		pgtab = (pte_t *)p2v(PTE_ADDR(*pde));
	else {
		if (!alloc || !(pgtab = (pte_t *)setupvm()))
			return 0;

		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PGSIZE);

		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table
		// entries, if necessary.
		*pde = v2p(pgtab) | PDE_COARSE;
	}

	return &pgtab[PTE_IDX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int mappages(pde_t *pgdir, void *va, uint size, uint pa, int ap) {
	char *a, *last;
	pte_t *pte;

	a = (char *)ALIGNDOWN(va, PTE_SZ);
	last = (char *)ALIGNDOWN((uint)va + size - 1, PTE_SZ);

	for (;;) {
		if ((pte = walkpgdir(pgdir, a, 1)) == 0)
			return -1;

		if (*pte & PTE_TYPES) panic("remap");

		*pte = pa | ((ap & 0x3) << 4) | PTE_CACHE | PTE_BUF | PTE_SMALL;

		if (a == last) break;

		a += PTE_SZ;
		pa += PTE_SZ;
	}

	return 0;
}

pde_t *setupvm(void) {
	pde_t *pgdir;

	if (!(pgdir = (pde_t *)kalloc()))
		return 0;

	return pgdir;
}

// Switch to the user page table (TTBR0)
void switchuvm(struct proc *p) {
	uint val;

	if (!p)
		panic("switchuvm: no process");
	if (!p->kstack)
		panic("switchuvm: no kstack");
	if (!p->pgdir)
		panic("switchuvm: no pgdir");

	pushcli();

	val = (uint)V2P(p->pgdir) | 0x00;
	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0": : "r"(val):);
	flush_tlb();

	popcli();
}

// Load the initcode into address 0 of pgdir. sz must be less than a page.
void inituvm(pde_t *pgdir, char *init, uint sz) {
	char *mem;

	if (sz >= PTE_SZ)
		panic("inituvm: more than a page");

	mem = kalloc();
	memset(mem, 0, PTE_SZ);
	mappages(pgdir, 0, PTE_SZ, v2p(mem), AP_KU);
	memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz) {
	uint i, pa, n;
	pte_t *pte;

	if ((uint) addr % PTE_SZ != 0) {
		panic("loaduvm: addr must be page aligned");
	}

	for (i = 0; i < sz; i += PTE_SZ) {
		if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0) {
			panic("loaduvm: address should exist");
		}

		pa = PTE_ADDR(*pte);

		if (sz - i < PTE_SZ) {
			n = sz - i;
		} else {
			n = PTE_SZ;
		}

		if (readi(ip, p2v(pa), offset + i, n) != n) {
			return -1;
		}
	}

	return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pde_t *pgdir, uint oldsz, uint newsz) {
	char *mem;
	uint a;

	if (newsz >= UADDR_SZ)
		return 0;

	if (newsz < oldsz)
		return oldsz;

	a = ALIGNUP(oldsz, PTE_SZ);

	for (; a < newsz; a += PTE_SZ) {
		mem = kalloc();

		if (mem == 0) {
			cprintf("allocuvm out of memory\n");
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}

		memset(mem, 0, PTE_SZ);
		mappages(pgdir, (char*) a, PTE_SZ, v2p(mem), AP_KU);
	}

	return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pde_t *pgdir, uint oldsz, uint newsz) {
	pte_t *pte;
	uint a;
	uint pa;

	if (newsz >= oldsz) {
		return oldsz;
	}

	for (a = ALIGNUP(newsz, PTE_SZ); a < oldsz; a += PTE_SZ) {
		pte = walkpgdir(pgdir, (char*) a, 0);

		if (!pte) {
			// pte == 0 --> no page table for this entry
			// round it up to the next page directory
			a = ALIGNUP (a, PDE_SZ);

		} else if ((*pte & PTE_TYPES) != 0) {
			pa = PTE_ADDR(*pte);

			if (pa == 0) {
				panic("deallocuvm");
			}

			kfree(p2v(pa));
			*pte = 0;
		}
	}

	return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pde_t *pgdir) {
	uint i;
	char *v;

	if (pgdir == 0)
		panic("freevm: no pgdir");

	// release the user space memroy, but not page tables
	deallocuvm(pgdir, UADDR_SZ, 0);

	// release the page tables
	for (i = 0; i < NUM_UPDE; i++) {
		if (pgdir[i] & PDE_TYPES) {
			v = p2v(PTE_ADDR(pgdir[i]));
			kfree(v);
		}
	}

	kfree((char *)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible page beneath
// the user stack (to trap stack underflow).
void clearpteu(pde_t *pgdir, char *uva) {
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if (pte == 0) {
		panic("clearpteu");
	}

	// in ARM, we change the AP field (ap & 0x3) << 4)
	*pte = (*pte & ~(0x03 << 4)) | AP_KO << 4;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t* copyuvm(pde_t *pgdir, uint sz) {
	pde_t *d;
	pte_t *pte;
	uint pa, i, ap;
	char *mem;

	// allocate a new first level page directory
	if (!(d = setupvm()))
		return NULL;

	// copy the whole address space over (no COW)
	for (i = 0; i < sz; i += PTE_SZ) {
		if ((pte = walkpgdir(pgdir, (void *) i, 0)) == 0) {
			panic("copyuvm: pte should exist");
		}

		if (!(*pte & PTE_TYPES)) {
			panic("copyuvm: page not present");
		}

		pa = PTE_ADDR (*pte);
		ap = PTE_AP (*pte);

		if ((mem = kalloc()) == 0) {
			goto bad;
		}

		memmove(mem, (char*) p2v(pa), PTE_SZ);

		if (mappages(d, (void*) i, PTE_SZ, v2p(mem), ap) < 0) {
			goto bad;
		}
	}
	return d;

bad:
	freevm(d);
	return 0;
}

// Map user virtual address to kernel address.
char* uva2ka(pde_t *pgdir, char *uva) {
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);

	// make sure it exists
	if ((*pte & PTE_TYPES) == 0)
		return 0;

	// make sure it is a user page
	if (PTE_AP(*pte) != AP_KU)
		return 0;

	return (char *)p2v(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for user pages.
int copyout(pde_t *pgdir, uint va, void *p, uint len) {
	char *buf, *pa0;
	uint n, va0;

	buf = (char *) p;

	while (len > 0) {
		va0 = ALIGNDOWN(va, PTE_SZ);
		pa0 = uva2ka(pgdir, (char *) va0);

		if (!pa0) return -1;

		n = PTE_SZ - (va - va0);

		if (n > len) n = len;

		memmove(pa0 + (va - va0), buf, n);

		len -= n;
		buf += n;
		va = va0 + PTE_SZ;
	}

	return 0;
}

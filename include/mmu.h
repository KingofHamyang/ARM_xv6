#ifndef _MMU_H_
#define _MMU_H_

// align_up/down: al must be of power of 2
#define align_up(sz, al) (((uint)(sz) + (uint)(al) - 1) & ~((uint)(al) - 1))
#define align_dn(sz, al) ((uint)(sz) & ~((uint)(al) - 1))
//
// Since ARMv6, you may use two page tables, one for kernel pages (TTBR1),
// and one for user pages (TTBR0). We use this architecture. Memory address
// lower than UVIR_BITS^2 is translated by TTBR0, while higher memory is
// translated by TTBR1.
// Kernel pages are create statically during system initialization. It use
// 1MB page mapping. User pages use 4K pages.
//


// access permission for page directory/page table entries.
#define AP_NA       0x0    // no access
#define AP_KO       0x1    // privilaged access, kernel: RW, user: no access
#define AP_KUR      0x2    // no write access from user, read allowed
#define AP_KU       0x3    // full access

// domain definition for page table entries
#define DM_NA       0x0    // any access causing a domain fault
#define DM_CLIENT   0x1    // any access checked against TLB (page table)
#define DM_RESRVED  0x2    // reserved
#define DM_MANAGER  0x3    // no access check

#define PE_CACHE    0x8// cachable
#define PE_BUF      0x4// bufferable

#define PE_TYPES    0x3    // mask for page type
#define KPDE_TYPE   0x2    // use "section" type for kernel page directory
#define UPDE_TYPE   0x1    // use "coarse page table" for user page directory
#define PTE_TYPE    0x2    // executable user page(subpage disable)

// 1st-level or large (1MB) page directory (always maps 1MB memory)
#define PDE_SHIFT   20                      // shift how many bits to get PDE index
#define PDE_SZ      (1 << PDE_SHIFT)
#define PDE_MASK    (PDE_SZ - 1)            // offset for page directory entries
#define PDE_IDX(v)  ((uint)(v) >> PDE_SHIFT) // index for page table entry

// 2nd-level page table
#define PTE_SHIFT   12                  // shift how many bits to get PTE index
#define PTE_IDX(v)  (((uint)(v) >> PTE_SHIFT) & (NUM_PTE - 1))
#define PTE_SZ      (1 << PTE_SHIFT)
#define PTE_ADDR(v) align_dn (v, PTE_SZ)
#define PTE_AP(pte) (((pte) >> 4) & 0x3)

// size of two-level page tables
#define UADDR_BITS  28                  // maximum user-application memory, 256MB
#define UADDR_SZ    (1 << UADDR_BITS)   // maximum user address space size

// must have NUM_UPDE == NUM_PTE
#define NUM_UPDE    (1 << (UADDR_BITS - PDE_SHIFT)) // # of PDE for user space
#define NUM_PTE     (1 << (PDE_SHIFT - PTE_SHIFT))  // how many PTE in a PT

#define PT_SZ       (NUM_PTE << 2)                  // user page table size (1K)
#define PT_ADDR(v)  align_dn(v, PT_SZ)              // physical address of the PT
#define PT_ORDER    10

#endif
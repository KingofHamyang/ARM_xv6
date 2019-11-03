#ifndef _MMU_H_
#define _MMU_H_

// ALIGNUP/DOWN: al must be of power of 2
#define ALIGNUP(sz, al) (((uint)(sz) + (uint)(al) - 1) & ~((uint)(al) - 1))
#define ALIGNDOWN(sz, al) ((uint)(sz) & ~((uint)(al) - 1))

// page round functions.
#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// ARM has 2 page tables: one for kernel, other for user.
// Use 4K pages in both tables.

#define ARCH_BIT    32
// access permission for page directory/page table entries.
// Use these values for section tables only!
#define AP_NA       0x0    // no access
#define AP_KO       0x1    // privilaged access, kernel: RW, user: no access
#define AP_KUR      0x2    // no write access from user, read allowed
#define AP_KU       0x3    // full access

// Domain Access Control definition entries
#define DM_NA       0x0    // any access causing a domain fault
#define DM_CLIENT   0x1    // any access checked against TLB (page table)
#define DM_RESRVED  0x2    // reserved: always falut.
#define DM_MANAGER  0x3    // no access check

// PDE(L1 descriptor) parameters
#define PDE_SHIFT   20     // shift how many bits to get PDE index
#define PDE_IDX(v)  ((uint)(v) >> PDE_SHIFT) // index for page table entry
#define PDE_SZ      (1 << PDE_SHIFT)
#define PDE_MASK    (PDE_SZ - 1)             // offset for page directory entries

#define PDE_TYPES   0x3    // mask for PDE type
#define PDE_INVALID 0x0    // PDE(L1 descriptor): invalid
#define PDE_COARSE  0x1    // PDE(L1 descriptor): coarse
#define PDE_SECTION 0x2    // PDE(L1 descriptor): section
#define PDE_FINE    0x3    // PDE(L1 descriptor): fine

// PTE(L2 descriptor) parameters
#define PTE_SHIFT   12     // shift how many bits to get PTE index
#define PTE_IDX(v)  (((uint)(v) >> PTE_SHIFT) & (NUM_PTE - 1))
#define PTE_SZ      (1 << PTE_SHIFT)
#define PTE_ADDR(v) ALIGNDOWN(v, PTE_SZ)
#define PTE_AP(pte) (((pte) >> 4) & 0x3)

#define PTE_TYPES   0x3    // mask for PTE type
#define PTE_INVALID 0x0    // PTE(L2 descriptor): invalid
#define PTE_LARGE   0x1    // PTE(L2 descriptor): 64KB page
#define PTE_SMALL   0x2    // PTE(L2 descriptor):  4KB page
#define PTE_TINY    0x3    // PTE(L2 descriptor):  1KB page

#define PTE_BUF     0x4    // bufferable
#define PTE_CACHE   0x8    // cachable

// PTE_INVALID has same value with PTE bit.
                           // |  SVC  |  USR  |
                           // |-------|-------|
#define PTE_PA      0x010  // |  R/W  |   0   |
#define PTE_URO     0x020  // |  R/W  |   R   |
#define PTE_FULL    0x030  // |  R/W  |  R/W  |
#define PTE_PR      0x210  // |   R   |   0   |
#define PTE_RO      0x220  // |   R   |   R   |

// I will use Coarse, Small pages only.
#define NUM_PDE     (1 << (ARCH_BIT - PDE_SHIFT))  // how many PTE in a PT
#define NUM_PTE     (1 << (PDE_SHIFT - PTE_SHIFT))  // how many PTE in a PT
#define PGSIZE      65536 // set large page(64K).

// size of two-level page tables
#define UADDR_BITS  28                  // maximum user-application memory, 256MB
#define UADDR_SZ    (1 << UADDR_BITS)   // maximum user address space size

// must have NUM_UPDE == NUM_PTE
#define NUM_UPDE    (1 << (UADDR_BITS - PDE_SHIFT)) // # of PDE for user space
#define NUM_PTE     (1 << (PDE_SHIFT - PTE_SHIFT))  // how many PTE in a PT

#endif
#include "arm.h"
#include "types.h"
#include "memlayout.h"


#define PDE_SHIFT 20

void init_boot_pt(uint32 v, uint32 p, uint len, int is_dev) {
	uint32 pde;
	int i;

	v >>= PDE_SHIFT;
	p >>= PDE_SHIFT;
	len >>= PDE_SHIFT;

	// TODO
}

void bootmain(void) {
	uint32 vec_table;
}
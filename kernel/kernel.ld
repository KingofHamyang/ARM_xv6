OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

BOOT_STACK_SIZE = 0x1000;

SECTIONS
{
	/* Start point. */
	. = 0x10000;
	.start_sec : {
		/* read only area. */
		out/bootasm.o(.text)
		out/bootmain.o(.text .text.*)

		out/bootasm.o(.rodata .rodata.*)
		out/bootmain.o(.rodata .rodata.*)

		/* already initialized to zero. */
		out/bootasm.o(.data .data.*)
		out/bootmain.o(.data .data.*)

		/* initialization start checkpoint. */
		PROVIDE(boot_start_addr = .);

		out/bootasm.o(.bss .bss.* COMMON)
		out/bootmain.o(.bss .bss.* COMMON)
		
		/* stack for booting code. */
		. = ALIGN(0x1000);
		. += BOOT_STACK_SIZE;

		PROVIDE(svc_stack = .);

		/* Kernel page table. 16K */
		. = ALIGN(0x4000);
		PROVIDE(_kt = .);
		. += 0x4000;

		/* User page table. 4K */
		PROVIDE(_ut = .);
		. += 0x1000;

		/* initialization end checkpoint. */
		PROVIDE(boot_end_addr = .);
	}

	/* Other Kernel code is placed over 0x80000000 + 128KB. */
	. = 0x80020000;
	.text : AT(0x20000) {
		*(.text .text.* .gnu.linkonce.t.*)
	}

	/* Other Kernel code text checkpoint. */
	PROVIDE(kern_text = .);

	. = ALIGN(0x1000);
	PROVIDE(kern_data_start = .);

	.data : {
		*(.data .data.*)
		PROVIDE(_binary_initcode_start = .);
		*(.rawdata_initcode*)
		PROVIDE(_binary_initcode_end = .);
		PROVIDE(_binary_fs_img_start = .);
		*(.rawdata_fs_img*)
		PROVIDE(_binary_fs_img_end = .);
	}

	PROVIDE(kern_data_end = .);

	.bss : {
		*(.bss .bss.* COMMON)
	}

	. = ALIGN(0x1000);
	PROVIDE(kern_end = .);
}

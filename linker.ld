/* linker.ld - Linker script to instruct how to arrange code in memoryThe bootloader typically loads the kernel at memory address 1MB*/ENTRY(_start)SECTIONS{/* Start kernel at 1 MiB (0x100000) */. = 1M;/* Place the Multiboot header first */
.text BLOCK(4K) : ALIGN(4K)
{
	*(.multiboot)
	*(.text)
}

/* Read-only data. */
.rodata BLOCK(4K) : ALIGN(4K)
{
	*(.rodata)
}

/* Read-write data (initialized) */
.data BLOCK(4K) : ALIGN(4K)
{
	*(.data)
}

/* Read-write data (uninitialized) and stack */
.bss BLOCK(4K) : ALIGN(4K)
{
	*(COMMON)
	*(.bss)
}
}
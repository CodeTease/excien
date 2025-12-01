/* boot.s - Kernel Entry Point
   This is the first code executed after the bootloader jumps to our kernel.
*/

/* Multiboot Header Magic constants */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* Declare the multiboot section so the linker places it at the start of the file */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack setup */
.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB Stack size */
stack_top:

/* Assembly code execution starts here */
.section .text
.global _start
.type _start, @function
_start:
	/* Set up the stack pointer (esp) */
	mov $stack_top, %esp

	/* Call the kernel_main function written in C */
	call kernel_main

	/* If the C function returns (which it shouldn't), halt the machine in an infinite loop */
	cli
1:	hlt
	jmp 1b

.size _start, . - _start
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

	/* Push multiboot info structure pointer (ebx) and magic number (eax) */
	push %ebx
	push %eax

	/* Call the kernel_main function written in C */
	call kernel_main

	/* If the C function returns (which it shouldn't), halt the machine in an infinite loop */
	cli
1:	hlt
	jmp 1b

/* --- GDT & IDT Helpers --- */
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush
.flush:
    ret

.global idt_load
.type idt_load, @function
idt_load:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

/* --- ISR & IRQ Helpers --- */

/* Macro for ISR without error code */
.macro ISR_NOERRCODE num
    .global isr\num
    .type isr\num, @function
    isr\num:
        cli
        push $0
        push $\num
        jmp isr_common_stub
.endm

/* Macro for ISR with error code */
.macro ISR_ERRCODE num
    .global isr\num
    .type isr\num, @function
    isr\num:
        cli
        push $\num
        jmp isr_common_stub
.endm

/* Macro for IRQ */
.macro IRQ num, map_num
    .global irq\num
    .type irq\num, @function
    irq\num:
        cli
        push $0
        push $\map_num
        jmp irq_common_stub
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

/* Common ISR Stub */
.extern isr_handler
isr_common_stub:
    pusha
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    call isr_handler
    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    add $8, %esp
    sti
    iret

/* Common IRQ Stub */
.extern irq_handler
irq_common_stub:
    pusha
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    call irq_handler
    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    add $8, %esp
    sti
    iret

.size _start, . - _start

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "kernel.h"

/* GDT */
void gdt_install(void);

/* IDT */
void idt_install(void);

/* ISRs & IRQs */
void isr_install(void);
void irq_install(void);

typedef void (*isr_t)(registers_t*);
void register_interrupt_handler(uint8_t n, isr_t handler);

/* Timer */
void timer_install(void);
void timer_wait(int ticks);
void sleep(uint32_t ms);
uint32_t get_tick_count(void);

/* Keyboard */
void keyboard_install(void);

#endif

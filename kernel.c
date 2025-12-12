/* kernel.c - Excien Kernel v0.3.0 (CodeTease Edition) */

#include "kernel.h"
#include "cpu.h"

/* --- MEMORY MANAGEMENT (Simple Bump Allocator) --- */
// 10MB mark
static uint32_t free_mem_addr = 0x1000000; 

void* kmalloc(size_t size) {
    // 4-byte align
    if (free_mem_addr & 0x3) { 
        free_mem_addr &= 0xFFFFFFFC;
        free_mem_addr += 4;
    }
    void* ptr = (void*)free_mem_addr;
    free_mem_addr += size;
    return ptr;
}

void kfree(void* ptr) {
    (void)ptr;
    // We don't free in bump allocator
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while(n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

char* strcpy(char* dest, const char* src) {
    char* saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
    return saved;
}

size_t strlen(const char* str) 
{
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* --- VGA DRIVER --- */

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
volatile uint16_t* vga_buffer = (uint16_t*) 0xB8000;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
    return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_set_color(uint8_t color) {
    terminal_color = color;
}

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    
    // Move cursor to 0,0
    outb(0x3D4, 0x0F);
    outb(0x3D5, 0);
    outb(0x3D4, 0x0E);
    outb(0x3D5, 0);
}

void terminal_update_cursor() {
    uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_scroll(void)
{
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
    vga_buffer[y * VGA_WIDTH + x] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
        terminal_update_cursor();
        return;
    }
    
    if (c == '\b') {
         if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
         } else if (terminal_row > 0) { // Backspace to previous line if needed (optional)
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
         }
         terminal_update_cursor();
         return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
    terminal_update_cursor();
}

void terminal_writestring(const char* data) 
{
    for (size_t i = 0; i < strlen(data); i++)
        terminal_putchar(data[i]);
}

void terminal_write_color(const char* data, enum vga_color fg) {
    uint8_t old_color = terminal_color;
    terminal_set_color(vga_entry_color(fg, VGA_COLOR_BLACK));
    terminal_writestring(data);
    terminal_set_color(old_color);
}

/* --- KERNEL PANIC --- */

// Helper to print hex
void print_hex(uint32_t n) {
    terminal_writestring("0x");
    char hex_chars[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        terminal_putchar(hex_chars[(n >> i) & 0xF]);
    }
}

void panic(const char* message) {
    panic_with_regs(message, 0);
}

void panic_with_regs(const char* message, registers_t* regs) {
    asm volatile("cli");
    
    terminal_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    // Clear screen specifically for panic manually
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0; 
    terminal_column = 0;
    terminal_update_cursor();
    
    terminal_writestring("\n  !!! EXCIEN KERNEL PANIC !!!\n\n");
    terminal_writestring("  Error: ");
    terminal_writestring(message);
    terminal_writestring("\n");

    if (regs) {
        terminal_writestring("\n  EAX: "); print_hex(regs->eax);
        terminal_writestring("  EBX: "); print_hex(regs->ebx);
        terminal_writestring("  ECX: "); print_hex(regs->ecx);
        terminal_writestring("  EDX: "); print_hex(regs->edx);
        terminal_writestring("\n  ESI: "); print_hex(regs->esi);
        terminal_writestring("  EDI: "); print_hex(regs->edi);
        terminal_writestring("  EBP: "); print_hex(regs->ebp);
        terminal_writestring("  ESP: "); print_hex(regs->esp);
        terminal_writestring("\n  EIP: "); print_hex(regs->eip);
        terminal_writestring("  CS:  "); print_hex(regs->cs);
        terminal_writestring("  FLG: "); print_hex(regs->eflags);
        terminal_writestring("\n");
        if (regs->int_no <= 32) {
             terminal_writestring("  INT: "); print_hex(regs->int_no);
             terminal_writestring("  ERR: "); print_hex(regs->err_code);
             terminal_writestring("\n");
        }
    }

    terminal_writestring("\n  System halted.\n");

    for (;;) {
        asm volatile("hlt");
    }
}

/* --- KEYBOARD DRIVER (INTERRUPT BASED) --- */

char kbd_US [128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, 
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, 
  '*', 0, ' ',
};

// Keyboard Buffer
#define KB_BUFFER_SIZE 256
char kb_buffer[KB_BUFFER_SIZE];
volatile int kb_read_ptr = 0;
volatile int kb_write_ptr = 0;

void keyboard_callback(registers_t* regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);
    
    // Ignore key release for now
    if (scancode & 0x80) return;

    // Up arrow: 0xE0 0x48 (Handle extended codes later properly, for now just simple scancodes)
    // Actually, scancodes are trickier.
    // Standard arrows: Up=0x48, Left=0x4B, Right=0x4D, Down=0x50 (Wait, these are keypad if NumLock off, or extended)
    // Extended keys send E0 first. 
    
    // For simplicity, let's just store scancode if it's special, or ASCII if it's normal.
    // Or just store scancode and let shell handle it? 
    // Shell needs ASCII mostly.
    
    // Let's assume standard set 1.
    
    int next_write = (kb_write_ptr + 1) % KB_BUFFER_SIZE;
    if (next_write != kb_read_ptr) {
        kb_buffer[kb_write_ptr] = (char)scancode; // Store raw scancode for now to handle arrows
        kb_write_ptr = next_write;
    }
}

void keyboard_install() {
    register_interrupt_handler(33, keyboard_callback);
}

// Non-blocking char read, returns 0 if no key
char keyboard_getchar() {
    if (kb_read_ptr == kb_write_ptr) return 0;
    
    char scancode = kb_buffer[kb_read_ptr];
    kb_read_ptr = (kb_read_ptr + 1) % KB_BUFFER_SIZE;
    return scancode;
}

/* --- COMMAND SYSTEM --- */

typedef void (*command_func_t)(const char* args);

typedef struct {
    const char* name;
    command_func_t func;
    const char* help;
} command_t;

// Forward declarations
void cmd_echo(const char* args);
void cmd_help(const char* args);
void cmd_about(const char* args);
void cmd_clear(const char* args);
void cmd_panic(const char* args);
void cmd_ping(const char* args);
void cmd_ls(const char* args);
void cmd_cat(const char* args);

command_t commands[] = {
    {"echo", cmd_echo, "Prints text to console. Usage: echo <text>"},
    {"help", cmd_help, "Shows this help message."},
    {"about", cmd_about, "Information about Excien."},
    {"clear", cmd_clear, "Clears the terminal."},
    {"codetease", cmd_about, "Alias for about."},
    {"panic", cmd_panic, "Triggers a kernel panic (BSOD test)."},
    {"ping", cmd_ping, "Pings an IP address (Network test)."},
    {"ls", cmd_ls, "List loaded modules (files)."},
    {"cat", cmd_cat, "Print module content. Usage: cat <name>"},
    {0, 0, 0} 
};

void cmd_echo(const char* args) {
    terminal_writestring(args);
    terminal_writestring("\n");
}

void cmd_help(const char* args) {
    (void)args;
    terminal_writestring("Available commands:\n");
    for (int i = 0; commands[i].name != 0; i++) {
        terminal_write_color("  ", VGA_COLOR_DARK_GREY);
        terminal_write_color(commands[i].name, VGA_COLOR_LIGHT_CYAN);
        terminal_writestring(": ");
        terminal_writestring(commands[i].help);
        terminal_writestring("\n");
    }
}

void cmd_about(const char* args) {
    (void)args;
    terminal_write_color("Excien Kernel v0.4.0\n", VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("Built by ");
    terminal_write_color("Teaserverse Platform, Inc.\n", VGA_COLOR_LIGHT_MAGENTA);
    terminal_writestring("CodeTease: Always fun. Always useless.\n");
}

void cmd_clear(const char* args) {
    (void)args;
    terminal_initialize();
    terminal_writestring("Excien Shell [v0.4.0]\nuser@excien:~$ ");
}

void cmd_panic(const char* args) {
    (void)args;
    // We can simulate an exception (e.g., div by zero) or call panic directly
    // Let's call panic directly with fake regs if we want, but simple panic is fine
    panic("User requested fatal error via shell.");
}

void cmd_ping(const char* args) {
    if (strlen(args) == 0) {
        terminal_writestring("Usage: ping <ip>\n");
        return;
    }
    terminal_writestring("Pinging ");
    terminal_writestring(args);
    terminal_writestring("...\n");
    
    sleep(1000); 
    
    terminal_write_color("Error: Network Unreachable.\n", VGA_COLOR_LIGHT_RED);
}

/* --- INITRD / MODULES --- */
typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} multiboot_module_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    // ... we don't need the rest for now
} multiboot_info_t;

multiboot_info_t* mb_info = 0;

void cmd_ls(const char* args) {
    (void)args;
    if (!mb_info || !(mb_info->flags & (1<<3))) {
         terminal_writestring("No modules loaded.\n");
         return;
    }
    
    multiboot_module_t* modules = (multiboot_module_t*)mb_info->mods_addr;
    for (uint32_t i = 0; i < mb_info->mods_count; i++) {
        terminal_writestring((const char*)modules[i].string);
        terminal_writestring(" (");
        // Print size
        // We lack printf with %d, so skip size for now or implement itoa
        terminal_writestring("bytes)\n");
    }
}

void cmd_cat(const char* args) {
    if (!mb_info || !(mb_info->flags & (1<<3))) {
         terminal_writestring("No modules loaded.\n");
         return;
    }
    
    if (strlen(args) == 0) {
        terminal_writestring("Usage: cat <filename>\n");
        return;
    }

    multiboot_module_t* modules = (multiboot_module_t*)mb_info->mods_addr;
    for (uint32_t i = 0; i < mb_info->mods_count; i++) {
        if (strcmp((const char*)modules[i].string, args) == 0) {
            char* start = (char*)modules[i].mod_start;
            char* end = (char*)modules[i].mod_end;
            
            for (char* p = start; p < end; p++) {
                terminal_putchar(*p);
            }
            terminal_writestring("\n");
            return;
        }
    }
    terminal_writestring("File not found.\n");
}

/* --- SHELL --- */

// History
#define HISTORY_MAX 10
char* history[HISTORY_MAX];
int history_count = 0;
int history_view_index = -1; // -1 means currently typing new command

void history_add(const char* cmd) {
    if (history_count < HISTORY_MAX) {
        history[history_count] = kmalloc(strlen(cmd) + 1);
        strcpy(history[history_count], cmd);
        history_count++;
    } else {
        // Shift, but we are using bump allocator so we can't free.
        // This is a "leak" but per requirements "reset machine to clear".
        // To be nicer, we could reuse slots, but let's just shift pointers.
        // Ideally we would recycle the string buffer if size fits, but simple is ok.
        
        // Actually, if we just shift pointers, the old string remains in heap (wasted).
        // Since we don't have free, we just abandon it.
        
        for (int i = 0; i < HISTORY_MAX - 1; i++) {
            history[i] = history[i+1];
        }
        history[HISTORY_MAX-1] = kmalloc(strlen(cmd) + 1);
        strcpy(history[HISTORY_MAX-1], cmd);
    }
}

char input_buffer[256];
int buffer_index = 0;

void execute_command() 
{
    terminal_writestring("\n");
    input_buffer[buffer_index] = 0; 

    if (strlen(input_buffer) == 0) {
        terminal_writestring("user@excien:~$ ");
        return;
    }
    
    // Add to history
    if (history_count == 0 || strcmp(history[history_count-1], input_buffer) != 0) {
        history_add(input_buffer);
    }
    history_view_index = -1;

    int found = 0;
    for (int i = 0; commands[i].name != 0; i++) {
        size_t cmd_len = strlen(commands[i].name);
        
        if (strncmp(input_buffer, commands[i].name, cmd_len) == 0) {
            if (input_buffer[cmd_len] == 0 || input_buffer[cmd_len] == ' ') {
                const char* args = "";
                if (input_buffer[cmd_len] == ' ') {
                    args = input_buffer + cmd_len + 1;
                }
                commands[i].func(args);
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        terminal_write_color("Unknown command: ", VGA_COLOR_LIGHT_RED);
        terminal_writestring(input_buffer);
        terminal_writestring("\n");
    }

    if (strcmp(input_buffer, "clear") != 0) {
        terminal_writestring("user@excien:~$ ");
    }
    buffer_index = 0;
}

void shell_handle_tab() {
    // Simple tab completion
    // Check commands matching current buffer
    if (buffer_index == 0) return;
    
    input_buffer[buffer_index] = 0;
    
    int match_idx = -1;
    int matches = 0;
    
    for (int i = 0; commands[i].name != 0; i++) {
        if (strncmp(input_buffer, commands[i].name, buffer_index) == 0) {
            match_idx = i;
            matches++;
        }
    }
    
    if (matches == 1) {
        // Auto complete
        const char* name = commands[match_idx].name;
        // Print remaining chars
        for (int i = buffer_index; name[i] != 0; i++) {
            input_buffer[buffer_index++] = name[i];
            terminal_putchar(name[i]);
        }
        // Add space
        input_buffer[buffer_index++] = ' ';
        terminal_putchar(' ');
    } else if (matches > 1) {
        // Show possibilities?
        // For now, do nothing or beep (no beep implemented)
    }
}

void shell_load_history(int index) {
    // Clear current line
    while(buffer_index > 0) {
        terminal_putchar('\b');
        buffer_index--;
    }
    
    if (index >= 0 && index < history_count) {
        const char* cmd = history[index];
        strcpy(input_buffer, cmd);
        buffer_index = strlen(cmd);
        terminal_writestring(input_buffer);
    }
}

void shell_loop() 
{
    terminal_writestring("user@excien:~$ ");
    
    // We need to track extended keys.
    // 0xE0 means extended.
    
    int state = 0; // 0=normal, 1=seen E0
    
    while(1) {
        // Polling loop but using our new keyboard_getchar which reads from interrupt buffer
        char scancode = keyboard_getchar();
        if (scancode == 0) {
             asm volatile("hlt"); // Save power
             continue;
        }
        
        // Handle E0
        if ((uint8_t)scancode == 0xE0) {
            state = 1;
            continue;
        }
        
        if (state == 1) {
            // Extended key
            // Up arrow: 0x48 (if E0 prefix)
            // Down arrow: 0x50 (if E0 prefix)
            // Left: 0x4B
            // Right: 0x4D
            state = 0;
            
            if ((uint8_t)scancode == 0x48) { // UP
                if (history_count > 0) {
                    if (history_view_index == -1) history_view_index = history_count - 1;
                    else if (history_view_index > 0) history_view_index--;
                    shell_load_history(history_view_index);
                }
            }
            else if ((uint8_t)scancode == 0x50) { // DOWN
                 if (history_view_index != -1) {
                    if (history_view_index < history_count - 1) {
                        history_view_index++;
                        shell_load_history(history_view_index);
                    } else {
                        // Restore empty
                        history_view_index = -1;
                        while(buffer_index > 0) {
                            terminal_putchar('\b');
                            buffer_index--;
                        }
                    }
                 }
            }
            continue;
        }
        
        // Normal key
        // Map scancode to ASCII
        char ascii = 0;
        if ((uint8_t)scancode < 128) {
            ascii = kbd_US[(uint8_t)scancode];
        }
        
        if (ascii > 0) {
            if (ascii == '\n') {
                execute_command();
            }
            else if (ascii == '\b') {
                if (buffer_index > 0) {
                    buffer_index--;
                    terminal_putchar('\b');
                }
            }
            else if (ascii == '\t') {
                shell_handle_tab();
            }
            else {
                if (buffer_index < 255) {
                    input_buffer[buffer_index++] = ascii;
                    terminal_putchar(ascii);
                }
            }
        }
    }
}

void print_splash() {
    terminal_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("\n  _____           _            \n");
    terminal_writestring(" | ____|_  ___ __(_) ___ _ __  \n");
    terminal_writestring(" |  _| \\ \\/ / '__| |/ _ \\ '_ \\ \n");
    terminal_writestring(" | |___ >  <| |  | |  __/ | | |\n");
    terminal_writestring(" |_____/_/\\_\\_|  |_|\\___|_| |_|\n");
    terminal_writestring("\n");
    terminal_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring(" Excien Kernel v0.4.0 - ");
    terminal_write_color("PRE-RELEASE\n", VGA_COLOR_LIGHT_RED);
    terminal_writestring(" Copyright (c) 2025 CodeTease.\n");
    terminal_writestring("---------------------------------------\n\n");
}

void __attribute__((__used__)) kernel_main(uint32_t magic, uint32_t addr) 
{
    /* Initialize Hardware */
    gdt_install();
    idt_install();
    isr_install();
    irq_install();
    
    // Enable interrupts
    asm volatile("sti");
    
    timer_install();
    keyboard_install();
    
    terminal_initialize();
    
    if (magic == 0x2BADB002) {
        mb_info = (multiboot_info_t*)addr;
    }
    
    print_splash();
    
    // Check modules
    if (mb_info && (mb_info->flags & (1<<3))) {
        terminal_writestring("Modules loaded: ");
        // We can print count manually since we don't have itoa
        if (mb_info->mods_count > 0) {
            terminal_writestring("Yes\n");
        } else {
             terminal_writestring("None\n");
        }
    }
    
    shell_loop();
}

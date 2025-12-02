/* kernel.c - Excien Kernel v0.3.0 (CodeTease Edition)
   Features: VGA Driver, Keyboard Polling, Modular Shell, Panic Mode
*/

#include <stddef.h>
#include <stdint.h>

/* --- LOW LEVEL HELPERS --- */

static inline uint8_t inb(uint16_t port) 
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) 
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
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

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

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
        return;
    }
    
    if (c == '\b') {
         if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
         }
         return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
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

void panic(const char* message) {
    terminal_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    // Clear screen specifically for panic manually to fill background
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0; 
    terminal_column = 0;
    
    terminal_writestring("\n  !!! EXCIEN KERNEL PANIC !!!\n\n");
    terminal_writestring("  A fatal error has occurred and Excien has been shut down.\n");
    terminal_writestring("  Error: ");
    terminal_writestring(message);
    terminal_writestring("\n\n  Please restart your computer manually.");
    terminal_writestring("\n  (Or blame the developer at CodeTease)");

    // Halt CPU
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt");
    }
}

/* --- KEYBOARD DRIVER (POLLING) --- */

char kbd_US [128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, 
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, 
  '*', 0, ' ',
};

char get_scancode() 
{
    uint8_t c = 0;
    do { c = inb(0x64); } while((c & 1) == 0);
    return inb(0x60);
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
void cmd_shutdown(const char* args);

command_t commands[] = {
    {"echo", cmd_echo, "Prints text to console. Usage: echo <text>"},
    {"help", cmd_help, "Shows this help message."},
    {"about", cmd_about, "Information about Excien."},
    {"clear", cmd_clear, "Clears the terminal."},
    {"codetease", cmd_about, "Alias for about."},
    {"panic", cmd_panic, "Triggers a kernel panic (BSOD test)."},
    {"ping", cmd_ping, "Pings an IP address (Network test)."},
    {0, 0, 0} // Null terminator
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
    terminal_write_color("Excien Kernel v0.3.0\n", VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("Built by ");
    terminal_write_color("Teaserverse Platform, Inc.\n", VGA_COLOR_LIGHT_MAGENTA);
    terminal_writestring("CodeTease: Always fun. Always useless.\n");
}

void cmd_clear(const char* args) {
    (void)args;
    terminal_initialize();
    terminal_writestring("Excien Shell [v0.3.0]\nuser@excien:~$ ");
}

void cmd_panic(const char* args) {
    (void)args;
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
    terminal_write_color("Error: Network Unreachable.\n", VGA_COLOR_LIGHT_RED);
}

/* --- SHELL --- */

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

    int found = 0;
    for (int i = 0; commands[i].name != 0; i++) {
        size_t cmd_len = strlen(commands[i].name);
        
        // Check if input starts with command name
        if (strncmp(input_buffer, commands[i].name, cmd_len) == 0) {
            // Ensure exact match or followed by space
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

    // Don't reprint prompt if we cleared the screen inside the command
    if (strcmp(input_buffer, "clear") != 0) {
        terminal_writestring("user@excien:~$ ");
    }
    buffer_index = 0;
}

void shell_loop() 
{
    terminal_writestring("user@excien:~$ ");
    while(1) {
        uint8_t scancode = get_scancode();
        if (scancode & 0x80) continue; 

        char ascii = kbd_US[scancode];
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
    terminal_writestring(" Excien Kernel v0.3.0 - ");
    terminal_write_color("PRE-RELEASE\n", VGA_COLOR_LIGHT_RED);
    terminal_writestring(" Copyright (c) 2025 CodeTease.\n");
    terminal_writestring("---------------------------------------\n\n");
}

void __attribute__((__used__)) kernel_main(void) 
{
    terminal_initialize();
    print_splash();
    shell_loop();
}
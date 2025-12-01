/* kernel.c - Excien Kernel v0.2.1
   Features: VGA Driver, Keyboard Polling, Shell with ECHO support
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

/* Compare two strings completely */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Compare first n characters of two strings (Needed for command arguments) */
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
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_WHITE = 15,
    VGA_COLOR_LIGHT_CYAN = 11,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
    return (uint16_t) uc | (uint16_t) color << 8;
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

/* --- SHELL --- */

char input_buffer[256];
int buffer_index = 0;

void execute_command() 
{
    terminal_writestring("\n");
    input_buffer[buffer_index] = 0; 

    /* Handle empty input */
    if (strlen(input_buffer) == 0) {
        // Do nothing, just print prompt
    }
    /* Command: ECHO */
    else if (strncmp(input_buffer, "echo ", 5) == 0) {
        // Print everything after "echo "
        terminal_writestring(input_buffer + 5);
        terminal_writestring("\n");
    }
    else if (strcmp(input_buffer, "echo") == 0) {
        // Print empty line
        terminal_writestring("\n");
    }
    /* Command: HELP */
    else if (strcmp(input_buffer, "help") == 0) {
        terminal_writestring("Available commands: echo, help, about, clear, codetease");
    } 
    /* Command: ABOUT */
    else if (strcmp(input_buffer, "about") == 0) {
        terminal_writestring("Excien Kernel v0.2.1. Built by Teaserverse Platform, Inc.");
    }
    /* Command: CODETEASE */
    else if (strcmp(input_buffer, "codetease") == 0) {
        terminal_writestring("CodeTease: Always fun. Always useless.");
    }
    /* Command: CLEAR */
    else if (strcmp(input_buffer, "clear") == 0) {
        terminal_initialize();
        terminal_writestring("Excien Shell [v0.2.1]\n");
        buffer_index = 0;
        terminal_writestring("user@excien:~$ ");
        return;
    }
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(input_buffer);
        terminal_writestring("\n");
    }

    buffer_index = 0;
    terminal_writestring("user@excien:~$ ");
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

void __attribute__((__used__)) kernel_main(void) 
{
    terminal_initialize();
    
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("Excien Kernel v0.2.1 [Interactive Mode]\n");
    terminal_writestring("Booting... OK\n");
    terminal_writestring("Shell: Loaded... OK\n");
    terminal_writestring("Now with 100% more ECHO!\n");
    terminal_writestring("---------------------------------------\n\n");
    
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    shell_loop();
}
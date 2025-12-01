# Makefile for building the Excien Kernel

# Compiler configuration - Use native GCC with flags to simulate a freestanding environment
# NOTE: It's best practice to use i686-elf-gcc (a cross-compiler), but we use the -m32 flag for quick testing
CC = gcc
AS = as
LD = ld

CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJECTS = boot.o kernel.o

all: excien.bin

excien.bin: $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o excien.bin $(OBJECTS)

boot.o: boot.s
	$(AS) --32 boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

clean:
	rm -f excien.bin $(OBJECTS)

# Run the kernel using QEMU emulator
run: excien.bin
	qemu-system-i386 -kernel excien.bin
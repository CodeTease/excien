# Makefile for building the Excien Kernel

CC = gcc
AS = as
LD = ld

CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I.
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJECTS = boot.o kernel.o cpu.o

all: excien.bin

excien.bin: $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o excien.bin $(OBJECTS)

boot.o: boot.s
	$(AS) --32 boot.s -o boot.o

kernel.o: kernel.c kernel.h
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

cpu.o: cpu.c cpu.h kernel.h
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

clean:
	rm -f excien.bin $(OBJECTS)

run: excien.bin
	qemu-system-i386 -kernel excien.bin

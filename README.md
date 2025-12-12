# Excien Kernel v0.4.0 (Pre-release)

**Status:** Unreleased (but version bumped anyway).

Excien is a toy kernel developed by CodeTease. It demonstrates basic VGA output, interrupt handling, and shell interaction in a 32-bit protected mode environment.

> Bonus: If you think the ASCII art showing "Exrien" instead of "Excien". You're not alone!

## Features

* **Interrupt System:** Full GDT & IDT setup with PIC remapping.
* **Input:** Interrupt-driven Keyboard driver (no more CPU polling!).
* **Timing:** Programmable Interval Timer (PIT) with `sleep()` support.
* **Memory:** Simple "Bump Allocator" for dynamic memory (Heap).
* **Shell v2:** 
  * Command History (Up/Down arrows).
  * Tab Completion.
  * Colored output.
* **FileSystem:** Read-only support for Multiboot Modules (Initrd).
* **Debug:** "Blue Screen of Death" style Kernel Panic with register dump.

## Commands

* `help`: Show help.
* `echo <text>`: Print text.
* `clear`: Clear screen.
* `ping <ip>`: Simulate network ping (tests Timer).
* `ls`: List loaded files (modules).
* `cat <file>`: Read content of a file.
* `panic`: Trigger a kernel panic test.
* `about`: Show version info.

## How to Build & Run

Ensure you have `gcc` (with multilib support if on 64-bit) and `qemu-system-x86` installed.

1. Build the kernel:
```bash
make
```

2. Run (QEMU):
```bash
make run
```

### Loading Files (Initrd)

To use `ls` and `cat`, launch QEMU with the `-initrd` flag:

```bash
qemu-system-i386 -kernel excien.bin -initrd "README.md,LICENSE"
```
*(Note: `make run` in the current Makefile only runs the kernel without modules by default)*

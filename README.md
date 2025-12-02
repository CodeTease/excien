# Excien Kernel v0.3.0 (Pre-release)

**Status:** Unreleased (but version bumped anyway).

Excien is a toy kernel developed by CodeTease. It demonstrates basic VGA output and keyboard interaction in a 32-bit environment.

> Bonus: If you think the ASCII art showing "Exrien" instead of "Excien". You're not alone!

## Features

* A super basic Shell, that's it.

## How to Build & Run

Ensure you have `gcc-multilib` and `qemu-system-x86` installed.

1. Build the kernel:
```bash
make
```

2. Run:
```bash
make run
```

3. Type `help` in the QEMU window to see available commands.

ASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

ASMFLAGS = -f elf32
CFLAGS = -ffreestanding -O2 -Wall -Wextra -m32 -mno-mmx -mno-sse -mno-sse2

all: build/stage1.bin build/stage2.bin build/kernel.flat

build/stage1.bin: boot/stage1.asm
	$(ASM) -f bin boot/stage1.asm -o build/stage1.bin

build/stage2.bin: boot/stage2.asm
	$(ASM) -f bin boot/stage2.asm -o build/stage2.bin

build/boot.o: boot/boot.asm
	$(ASM) $(ASMFLAGS) boot/boot.asm -o build/boot.o

build/isr.o: boot/isr.asm
	$(ASM) $(ASMFLAGS) boot/isr.asm -o build/isr.o

build/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o build/kernel.o

build/vga.o: kernel/vga.c
	$(CC) $(CFLAGS) -c kernel/vga.c -o build/vga.o

build/idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c kernel/idt.c -o build/idt.o

build/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c kernel/keyboard.c -o build/keyboard.o

build/shell.o: kernel/shell.c
	$(CC) $(CFLAGS) -c kernel/shell.c -o build/shell.o

build/gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c kernel/gdt.c -o build/gdt.o

build/kernel.elf: build/boot.o build/isr.o build/kernel.o build/vga.o build/idt.o build/keyboard.o build/shell.o build/gdt.o
	$(LD) -m elf_i386 -o build/kernel.elf -Ttext 0x100000 build/boot.o build/isr.o build/kernel.o build/vga.o build/idt.o build/keyboard.o build/shell.o build/gdt.o

build/kernel.flat: build/kernel.elf
	$(OBJCOPY) -O binary build/kernel.elf build/kernel.flat

disk: all
	python make_disk.py

clean:
	del build\*.o build\*.elf build\*.flat build\*.bin
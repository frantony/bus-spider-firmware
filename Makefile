# Makefile for hello_world for erizo board
CROSS_COMPILE=/opt/riscv32i/bin/riscv32-unknown-elf-

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
GDB=$(CROSS_COMPILE)gdb
OBJCOPY=$(CROSS_COMPILE)objcopy

NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CFLAGS=-march=rv32i -nostdlib -fno-builtin -fno-pic -W -Wall -Iinclude -D__KERNEL__ -D__BAREBOX__ -g -O2 $(NOSTDINC_FLAGS) -ffunction-sections -fdata-sections -mcmodel=medany
LDFLAGS=

QEMU=/opt/riscv-qemu/bin/qemu-system-riscv32


all: hello_world.bin
.PHONY: all

clean:
	@rm -f hello_world *.o *.bin embedded.lds
.PHONY: clean

hello_world.bin: hello_world
	$(OBJCOPY) --output-target binary $< $@

hello_world: embedded.lds
embedded.lds: embedded.lds.S
	$(CC) $(CFLAGS) -E $< | grep -v "^#" > $@

hello_world: startup.o main.o \
		tlsf.o tlsf_malloc.o memory.o \
		ctype.o string.o strtox.o vsprintf.o console_common.o \
		clock.o riscv_timer.o \
		div.o div64.o clz_ctz.o mulsi3.o muldi3.o
	$(LD) \
		-Map $@.map \
		-nostdlib --no-dynamic-linker -static --gc-sections \
		-o $@ \
		-T embedded.lds \
		--start-group $^ --end-group

%.o: %.S
	$(CC) -c $(CFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<

hello_world.nmon: hello_world.bin
	./erizo-nmon-image $< $@

run: hello_world.bin
	$(QEMU) -nographic -M erizo -bios ./hello_world.bin -serial stdio -monitor none
.PHONY: run

dbg: hello_world.bin
	$(GDB) -x conf.gdb
.PHONY: dbg

# Makefile for Bus Spider firmware for erizo board
CROSS_COMPILE=riscv64-linux-gnu-

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
GDB=$(CROSS_COMPILE)gdb
OBJCOPY=$(CROSS_COMPILE)objcopy

QEMU=/opt/riscv/bin/qemu-system-riscv32

CFLAGS = -W -Wall
CFLAGS += -fno-pic -pipe

CFLAGS += -march=rv32imc
CFLAGS += -mabi=ilp32
CFLAGS += -mcmodel=medany
CFLAGS += -Os

CFLAGS += -nostdlib -fno-builtin

CFLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CFLAGS += -Iinclude -D__KERNEL__ -D__BAREBOX__

CFLAGS += -ffunction-sections -fdata-sections

CFLAGS += -g

LDFLAGS=


all: bus_spider.bin
.PHONY: all

clean:
	@rm -f bus_spider *.o *.bin embedded.lds bus_spider.map bus_spider.nmon
.PHONY: clean

bus_spider.bin: bus_spider
	$(OBJCOPY) --output-target binary $< $@

bus_spider: embedded.lds
embedded.lds: embedded.lds.S
	$(CC) $(CFLAGS) -E $< | grep -v "^#" > $@

bus_spider: startup.o main.o \
		tlsf.o tlsf_malloc.o memory.o \
		ctype.o string.o strtox.o vsprintf.o console_common.o \
		readkey.o readline.o \
		clock.o riscv_timer.o \
		div.o div64.o clz_ctz.o mulsi3.o muldi3.o ashldi3.o lshrdi3.o \
		memtest.o \
		xfuncs.o \
		libbb.o \
		i2c-algo-bit.o \
		hiz_mode.o \
		i2c_mode.o \
		spi_mode.o spi0.o \
		i2c0.o bus_spider.o
	$(LD) \
		-Map $@.map \
		-nostdlib --no-dynamic-linker -static --gc-sections \
		-o $@ \
		-T embedded.lds \
		--start-group $(filter-out embedded.lds,$^) --end-group

%.o: %.S
	$(CC) -c $(CFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<

bus_spider.nmon: bus_spider.bin
	./erizo-nmon-image $< $@

run: bus_spider.bin
	$(QEMU) -nographic -M erizo -bios ./bus_spider.bin -serial stdio -monitor none
.PHONY: run

dbg: bus_spider.bin
	$(GDB) -x conf.gdb
.PHONY: dbg

# Makefile for Bus Spider firmware for erizo board

BINARY = bus_spider

all: $(BINARY).bin
.PHONY: all

SRCFILES = \
	arch/riscv/startup.S \
	main.c \
	common/tlsf.c common/tlsf_malloc.c common/memory.c \
	common/console_common.c \
	lib/ctype.c lib/string.c lib/strtox.c lib/vsprintf.c \
	lib/readkey.c lib/readline.c \
	common/clock.c \
	arch/riscv/riscv_timer.c \
	arch/riscv/div.S \
	lib/div64.c lib/clz_ctz.c \
	lib/ashldi3.c lib/lshrdi3.c lib/muldi3.c lib/mulsi3.c \
	common/memtest.c \
	lib/xfuncs.c \
	lib/libbb.c \
	bus_spider.c \
	hiz_mode.c \
	i2c-algo-bit.c \
	i2c_mode.c i2c0.c \
	spi_mode.c spi0.c

CROSS_COMPILE = /opt/riscv32imc/bin/riscv32-unknown-elf-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
GDB = $(CROSS_COMPILE)gdb

QEMU = /opt/riscv/bin/qemu-system-riscv32

TOP_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJST1 = $(patsubst %.c,%.o,$(SRCFILES))
OBJS = $(patsubst %.S,%.o,$(OBJST1))

CFLAGS += -march=rv32imc
CFLAGS += -mcmodel=medany

CFLAGS += -Os
CFLAGS += -g
CFLAGS += -pipe

CFLAGS += -MD
CFLAGS += -D__KERNEL__ -D__BAREBOX__
CFLAGS += -W -Wall -Wundef
CFLAGS += -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -nostdinc
CFLAGS += -I$(TOP_DIR)/include
CFLAGS += -isystem $(shell $(CC) -print-file-name=include)

CFLAGS += -fno-pic
CFLAGS += -fno-builtin
CFLAGS += -fno-common
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS=

LDSCRIPT = embedded.lds

embedded.lds: embedded.lds.S
	$(CC) $(CFLAGS) -E $< | grep -v "^#" > $@

$(BINARY): $(OBJS) $(LDSCRIPT)
	$(LD) \
		-Map $@.map \
		-nostdlib --no-dynamic-linker -static --gc-sections \
		-o $@ \
		-T $(LDSCRIPT) \
		--start-group $(OBJS) --end-group
	$(SIZE) $@

%.o: %.S
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.bin: %
	$(OBJCOPY) --output-target binary $< $@

$(BINARY).nmon: $(BINARY).bin
	./erizo-nmon-image $< $@

clean:
	$(RM) -f $(OBJS) $(patsubst %.o,%.d,$(OBJS))
	$(RM) -f $(BINARY) $(BINARY).bin embedded.lds $(BINARY).map $(BINARY).nmon
.PHONY: clean

run: $(BINARY).bin
	$(QEMU) -nographic -M erizo -bios ./$(BINARY).bin -serial stdio -monitor none
.PHONY: run

dbg: $(BINARY).bin
	$(GDB) -x conf.gdb
.PHONY: dbg

-include $(OBJS:.o=.d)

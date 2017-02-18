# Makefile for Bus Spider firmware for erizo board

BINARY = bus_spider

all: $(BINARY)
.PHONY: all

SRCFILES = \
	common/tlsf.c common/tlsf_malloc.c common/memory.c \
	freertos/heap_4.c freertos/list.c freertos/port.c \
	freertos/queue.c freertos/tasks.c freertos/opencm3.c \
	common/console_common.c \
	lib/ctype.c lib/string.c lib/strtox.c lib/vsprintf.c \
	lib/readkey.c lib/readline.c \
	lib/div64.c \
	lib/xfuncs.c \
	lib/libbb.c \
	bus_spider.c \
	hiz_mode.c

CROSS_COMPILE = arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

QEMU = qemu-system-arm

TOP_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

MCU=stm32f103
#MCU=stm32f205

OPENCM3_DIR = $(TOP_DIR)/libopencm3

ifeq ($(MCU),stm32f103)
LIBOPENCM3_TARGETS=stm32/f1
LDSCRIPT = $(OPENCM3_DIR)/lib/stm32/f1/stm32f103x8.ld
LIBOPENCM3_AFILE=$(OPENCM3_DIR)/lib/libopencm3_stm32f1.a
LIBOPENCM3_LDFLAG=-lopencm3_stm32f1
CFLAGS += -DSTM32F1
SRCFILES += main_stm32f103.c
endif

ifeq ($(MCU),stm32f205)
LIBOPENCM3_TARGETS=stm32/f2
LDSCRIPT = $(OPENCM3_DIR)/lib/stm32/f1/stm32f103x8.ld
LIBOPENCM3_AFILE=$(OPENCM3_DIR)/lib/libopencm3_stm32f2.a
LIBOPENCM3_LDFLAG=-lopencm3_stm32f2
CFLAGS += -DSTM32F2
SRCFILES += main_stm32f205.c
endif

OBJS = $(patsubst %.c,%.o,$(SRCFILES))

CFLAGS += -mcpu=cortex-m3 -mthumb -msoft-float -mfix-cortex-m3-ldrd

CFLAGS += -Os
CFLAGS += -g
CFLAGS += -pipe

CFLAGS += -MD
CFLAGS += -D__KERNEL__ -D__BAREBOX__
CFLAGS += -W -Wall -Wundef
CFLAGS += -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -nostdinc
CFLAGS += -I$(OPENCM3_DIR)/include
CFLAGS += -I$(TOP_DIR)/include
CFLAGS += -I$(TOP_DIR)/freertos
CFLAGS += -isystem $(shell $(CC) -print-file-name=include)

CFLAGS += -fno-pic
CFLAGS += -fno-builtin
CFLAGS += -fno-common
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -static --no-dynamic-linker
LDFLAGS += -nostartfiles
LDFLAGS += --gc-sections

$(LIBOPENCM3_AFILE):
	$(MAKE) -C $(OPENCM3_DIR) TARGETS=$(LIBOPENCM3_TARGETS)

clean_libopencm3:
	$(RM) -f $(LIBOPENCM3_AFILE)
	-$(MAKE) -$(MAKEFLAGS) -C $(OPENCM3_DIR) clean
.PHONY: clean_libopencm3

LDLIBS += -nostdlib
LDLIBS += -L$(OPENCM3_DIR)/lib $(LIBOPENCM3_LDFLAG)

$(BINARY): $(LIBOPENCM3_AFILE) $(OBJS) $(LDSCRIPT)
	$(LD) \
		-Map $@.map \
		--start-group $(OBJS) --end-group \
		$(LDFLAGS) \
		$(LDLIBS) \
		-T $(LDSCRIPT) \
		-o $@
	$(SIZE) $@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.bin: %
	$(OBJCOPY) --output-target binary $< $@

clean: clean_libopencm3
	$(RM) -f $(OBJS) $(patsubst %.o,%.d,$(OBJS))
	$(RM) -f $(BINARY) $(BINARY).bin *.list *.map
.PHONY: clean

run: $(BINARY).bin
	$(QEMU) -nographic -M netduino2 -kernel $< -serial stdio -monitor none
.PHONY: run

-include $(OBJS:.o=.d)

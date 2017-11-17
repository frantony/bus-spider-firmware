/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <config.h>

OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
	. = TEXT_BASE;
	PROVIDE(_start = .);
	_stext = .;
	.text : {
		KEEP(*(.text_entry*))
		KEEP(*(.main_entry*))
		*(.text*)
	}

	. = ALIGN(4);
	.rodata : {
		*(.rodata*)
	}
	_etext = .;

	. = ALIGN(4);
	_sdata = .;
	.data : {
		*(.data*)
	}
	_edata = .;

	. = ALIGN(4);
	PROVIDE(__bss_start = .);
	.sbss : {
		*(.sbss*)
	}
	.bss : {
		*(.bss*)
	}
	PROVIDE(__bss_stop = .);

	_end = .;
}

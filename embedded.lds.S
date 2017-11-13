/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
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

	. = ALIGN(8);
	.text      :
	{
		_stext = .;
		_start = .;
		KEEP(*(.text_entry*))
		_text = .;
		KEEP(*(.main_entry*))
		*(.text*)
	}

	. = ALIGN(8);
	.rodata : { *(.rodata*) }

	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(8);
	.data : { *(.data*) }

	. = ALIGN(8);
	.got : { *(.got*) }

	. = .;

	.rela.dyn : { *(.rela*) }

	_edata = .;
	. = STACK_BASE + 0x1;
	. = ALIGN(8);
	__bss_start = .;
	.bss : { *(.bss*) *(.sbss*) }
	__bss_stop = .;
	_end = .;
}

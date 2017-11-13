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

/**
 * @file
 * @brief Clocksource based on RISCV cycle CSR timer
 */

#include <clock.h>

static uint64_t rdcycle_read(void)
{
	register unsigned long __v;

	__asm__ __volatile__ ("rdcycle %0" : "=r" (__v));

	return __v;
}

static struct clocksource rdcycle_cs = {
	.read	= rdcycle_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

int rdcycle_cs_init(unsigned int cycle_frequency)
{
	clocks_calc_mult_shift(&rdcycle_cs.mult, &rdcycle_cs.shift,
		cycle_frequency, NSEC_PER_SEC, 10);

	return init_clock(&rdcycle_cs);
}

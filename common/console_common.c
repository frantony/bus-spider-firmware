/*
 * based on code:
 *
 * (C) Copyright 2000 Paolo Scaffardi, AIRVENT SAM s.p.a -
 * RIMINI(ITALY), arsenio@tin.it
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <linux/export.h>

#define CFG_PBSIZE 128

int printf(const char *fmt, ...)
{
	va_list args;
	int i;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);

	return i;
}
EXPORT_SYMBOL(printf);

int vprintf(const char *fmt, va_list args)
{
	int i;
	char printbuffer[CFG_PBSIZE];

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);

	/* Print the string */
	puts(printbuffer);

	return i;
}
EXPORT_SYMBOL(vprintf);

/*
 * xfuncs.c - safe malloc funcions
 *
 * based on busybox
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <stdio.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/export.h>

static void enomem_panic(size_t size)
{
	printf("out of memory\n");
	if (size)
		printf("Unable to allocate %zu bytes\n", size);

	malloc_stats();

	for (;;)
		;
}

void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size)))
		enomem_panic(size);

	return p;
}
EXPORT_SYMBOL(xmalloc);

void *xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}
EXPORT_SYMBOL(xzalloc);

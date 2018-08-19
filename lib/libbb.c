/*
 * Utility routines.
 *
 * Copyright (C) many different people.
 * If you wrote this, please acknowledge your work.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <linux/types.h>
#include <linux/export.h>

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char * safe_strncpy(char *dst, const char *src, size_t size)
{
	if (!size) return dst;
	dst[--size] = '\0';
	return strncpy(dst, src, size);
}
EXPORT_SYMBOL(safe_strncpy);

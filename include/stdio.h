#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <linux/stddef.h>

/*
 * STDIO based functions (can always be used)
 */

/* serial stuff */
void serial_printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));

int sprintf(char *buf, const char *fmt, ...) __attribute__ ((format(__printf__, 2, 3)));
int snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(__printf__, 3, 4)));
int scnprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(__printf__, 3, 4)));
int vsprintf(char *buf, const char *fmt, va_list args);
char *basprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
int asprintf(char **strp, const char *fmt, ...)  __attribute__ ((format(__printf__, 2, 3)));
char *bvasprintf(const char *fmt, va_list ap);
int vasprintf(char **strp, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

int vprintf(const char *fmt, va_list args);

int printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));

#include <config.h>
#include <debug_ll.h>

static inline int puts(const char *s)
{
	puts_ll(s);

	return 1;
}

static inline int putchar(int c)
{
	if (c == '\n')
		putc_ll('\r');

	putc_ll(c);

	return c;
}

static inline int getchar(void)
{
	return getc_ll();
}

#endif /* __STDIO_H */

#include <stdio.h>
#include <string.h>
#include <readkey.h>
#include <linux/ctype.h>
#include <linux/export.h>

/*
 * cmdline-editing related codes from vivi.
 * Author: Janghoon Lyu <nandy@mizi.com>
 */

#define putnstr(str,n)	do {			\
		printf ("%.*s", n, str);	\
	} while (0)

#define CTL_BACKSPACE		('\b')
#define DEL			255
#define DEL7			127

#define getcmd_putch(ch)	putchar(ch)
#define getcmd_getch()		getc()
#define getcmd_cbeep()		getcmd_putch('\a')

#define BEGINNING_OF_LINE() {			\
	while (num) {				\
		getcmd_putch(CTL_BACKSPACE);	\
		num--;				\
	}					\
}

#define ERASE_TO_EOL() {				\
	if (num < eol_num) {				\
		int t;					\
		for (t = num; t < eol_num; t++)		\
			getcmd_putch(' ');		\
		while (t-- > num)			\
			getcmd_putch(CTL_BACKSPACE);	\
		eol_num = num;				\
	}						\
}

#define REFRESH_TO_EOL() {			\
	if (num < eol_num) {			\
		wlen = eol_num - num;		\
		putnstr(buf + num, wlen);	\
		num = eol_num;			\
	}					\
}

#define DO_BACKSPACE()						\
		wlen = eol_num - num;				\
		num--;						\
		memmove(buf + num, buf + num + 1, wlen);	\
		getcmd_putch(CTL_BACKSPACE);			\
		putnstr(buf + num, wlen);			\
		getcmd_putch(' ');				\
		do {						\
			getcmd_putch(CTL_BACKSPACE);		\
		} while (wlen--);				\
		eol_num--;

static void cread_add_char(char ichar, int insert, unsigned long *num,
	       unsigned long *eol_num, char *buf, unsigned long len)
{
	unsigned wlen;

	/* room ??? */
	if (insert || *num == *eol_num) {
		if (*eol_num > len - 2) {
			getcmd_cbeep();
			return;
		}
		(*eol_num)++;
	}

	if (insert) {
		wlen = *eol_num - *num;
		if (wlen > 1) {
			memmove(&buf[*num+1], &buf[*num], wlen-1);
		}

		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
		while (--wlen) {
			getcmd_putch(CTL_BACKSPACE);
		}
	} else {
		/* echo the character */
		wlen = 1;
		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
	}
}

int readline(const char *prompt, char *buf, int len)
{
	unsigned long num = 0;
	unsigned long eol_num = 0;
	unsigned wlen;
	int ichar;
	int insert = 1;

	puts (prompt);

	while (1) {
		ichar = read_key();

		if ((ichar == '\n') || (ichar == '\r')) {
			putchar('\n');
			break;
		}

		switch (ichar) {
		case '\t':
			break;

		case BB_KEY_HOME:
			BEGINNING_OF_LINE();
			break;
		case CTL_CH('c'):	/* ^C - break */
			*buf = 0;	/* discard input */
			return -1;
		case BB_KEY_RIGHT:
			if (num < eol_num) {
				getcmd_putch(buf[num]);
				num++;
			}
			break;
		case BB_KEY_LEFT:
			if (num) {
				getcmd_putch(CTL_BACKSPACE);
				num--;
			}
			break;
		case CTL_CH('d'):
			if (num < eol_num) {
				wlen = eol_num - num - 1;
				if (wlen) {
					memmove(&buf[num], &buf[num+1], wlen);
					putnstr(buf + num, wlen);
				}

				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case CTL_CH('l'):
			printf(ANSI_CLEAR_SCREEN);
			buf[eol_num] = 0;
			printf("%s%s", prompt, buf);
			wlen = eol_num - num;
			while (wlen--)
				getcmd_putch(CTL_BACKSPACE);
			break;
		case BB_KEY_ERASE_TO_EOL:
			ERASE_TO_EOL();
			break;
		case BB_KEY_REFRESH_TO_EOL:
		case BB_KEY_END:
			REFRESH_TO_EOL();
			break;
		case BB_KEY_INSERT:
			insert = !insert;
			break;
		case BB_KEY_ERASE_LINE:
			BEGINNING_OF_LINE();
			ERASE_TO_EOL();
			break;
		case DEL:
		case BB_KEY_DEL7:
		case 8:
			if (num) {
				DO_BACKSPACE();
			}
			break;
		case BB_KEY_DEL:
			if (num < eol_num) {
				wlen = eol_num - num;
				memmove(buf + num, buf + num + 1, wlen);
				putnstr(buf + num, wlen - 1);
				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (--wlen);
				eol_num--;
			}
			break;
		case CTL_CH('w'):
			while ((num >= 1) && (buf[num - 1] == ' ')) {
				DO_BACKSPACE();
			}

			while ((num >= 1) && (buf[num - 1] != ' ')) {
				DO_BACKSPACE();
			}

			break;
		default:
			if (isascii(ichar) && isprint(ichar))
				cread_add_char(ichar, insert, &num, &eol_num, buf, len);
			break;
		}
	}
	len = eol_num;
	buf[eol_num] = '\0';	/* lose the newline */

	return len;
}

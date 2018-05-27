#include <stdio.h>
#include <string.h>

#include "bus_spider.h"

static struct mode *mode;
static LIST_HEAD(mode_list);

static int register_mode(struct mode *new_mode)
{
	list_add_tail(&new_mode->list, &mode_list);

	return 0;
}

static void select_mode(struct mode *new_mode)
{
	mode = new_mode;
	if (mode->open)
		mode->open();
}

static void change_mode(void)
{
	struct mode *t;
	unsigned int i;
	char modebuf[8];
	unsigned int moden;

	i = 1;
	list_for_each_entry(t, &mode_list, list) {
		printf(" %d. %s\n", i, t->name);
		i++;
	}

	readline(">>> ", modebuf, sizeof(modebuf));

	moden = simple_strtoul(modebuf, NULL, 0);

	i = 1;
	list_for_each_entry(t, &mode_list, list) {
		if (i == moden) {
			printf("  %s mode selected\n", t->name);
			select_mode(t);
		}
		i++;
	}
}

static void print_help(void)
{
	printf(" General\n");
	printf(" ---------------------------------------\n");
	printf(" ?\tThis help\n");
	printf(" m\tChange mode\n");
	printf(" $\tJump to bootloader\n");
	printf(" i\tVersioninfo/statusinfo\n");
	printf(" v\tShow states\n");

	if (mode->print_help) {
		printf("\n %s Protocol interaction\n", mode->name);
		printf(" ---------------------------------------\n");
		mode->print_help();
	}
}

static void version_info(void)
{
	printf("Bus Spider v0\n");
}

static void bus_spider(void)
{
	static char prompt[16];
	static const char *PROMPT_PS2 = "> ";

	char cmdbuf[100];
	int stop;
	int cmderror;
	char *curchar;
	char *t;

	t = mode->name ? mode->name : "";
	strncpy(prompt, t, sizeof(prompt));

	strncat(prompt, PROMPT_PS2, sizeof(prompt) - strlen(PROMPT_PS2));
	readline(prompt, cmdbuf, sizeof(cmdbuf));

	stop = 0;
	cmderror = 0;
	curchar = cmdbuf;

	while (!stop) {
		char c;

		c = *curchar;

		switch (c) {
		case 'h':
		case '?':
			print_help();
			break;

		case 'i':
			version_info();
			break;

		case 'm':
			change_mode();
			break;

		case '#': /* Reset */
		case '$':
			{
				void (*reboot)(void);

				reboot = (void *)0x00000000;
				reboot();
			}
			break;

		case 'v':
			{
				const uint32_t *gpio0_dat = (uint32_t *)0x91000000;
				const uint32_t *gpio0_dirout = (uint32_t *)0x91000004;
				const uint32_t *gpio1_dat = (uint32_t *)0x91000100;
				const uint32_t *gpio1_dirout = (uint32_t *)0x91000104;

				printf("gpio0_dat    = %02x\n", 0xff & *gpio0_dat);
				printf("gpio0_dirout = %02x\n\n", 0xff & *gpio0_dirout);
				printf("gpio1_dat    = %02x\n", 0xff & *gpio1_dat);
				printf("gpio1_dirout = %02x\n", 0xff & *gpio1_dirout);
			}
			break;

		case 0:
			stop = 1;
			break;

		/* skip spaces */
		case ' ':
			break;

		default:
			if (mode->parse_cmdline) {
				curchar = mode->parse_cmdline(curchar);
				if (!curchar) {
					cmderror = 1;
				}
			} else {
				cmderror = 1;
			}
		}

		if (cmderror) {
			printf("Syntax error at char '%c' (\"%s\")\n", c, curchar);
			stop = 1;
			continue;
		}

		curchar++;
	}
}

void bus_spider_main(void)
{
	extern struct mode hiz_mode;
	extern struct mode i2c_mode;

	register_mode(&hiz_mode);
	register_mode(&i2c_mode);

	select_mode(&hiz_mode);

	while (1) {
		bus_spider();
	}
}

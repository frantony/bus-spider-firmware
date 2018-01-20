#include <stdio.h>
#include <string.h>
#include <i2c/i2c-algo-bit.h>

static struct i2c_algo_bit_data *i2c0;

static void print_help(void)
{
	printf(" General\t\t\t\t\tProtocol interaction\n");
	printf(" ---------------------------------------------------------------------------\n");
	printf(" ?\tThis help\t\t\t\tS\tI2C 7bit address search\n");
	printf(" i\tVersioninfo/statusinfo\n");
}

static void version_info(void)
{
	printf("Bus Spider v0\n");
}

static void i2c_scan(struct i2c_algo_bit_data *adap, int min, int max)
{
	uint8_t i;

	printf("\n");
	printf("   ");
	for (i = 0; i < 0x10 ; i ++) {
		printf("  %1x", i);
	}
	printf("\n");

	for (i = 0; ; i ++) {
		if (i % 0x10 == 0x00) {
			printf("%02x: ", i);
		}

		if (i < (min << 1)) {
			printf("   ");
			continue;
		}

		i2c_stop(adap);
		i2c_start(adap);
		if (try_address(adap, i << 1, 1)) {
			printf("%02x ", i);
		} else {
			printf("-- ");
		}

		if (i == max) {
			printf("\n");
			break;
		}

		if (i % 0x10 == 0xf) {
			printf("\n");
		}
	}
}

static void bus_spider(void)
{
	char cmdbuf[100];
	int stop;
	int cmderror;
	char *curchar;

	readline("HiZ> ", cmdbuf, 100);

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

		case 'S':
			i2c_scan(i2c0, 0x01, 0x77);
			break;

		case 0:
			stop = 1;
			break;

		default:
			cmderror = 1;
		}

		if (cmderror) {
			printf("Syntax error at char '%c' (\"%s\")\n", c, curchar);
			stop = 1;
			continue;
		}

		curchar++;
	}
}

extern struct i2c_algo_bit_data *init_i2c0(void);

void bus_spider_main(void)
{
	i2c0 = init_i2c0();

	while (1) {
		bus_spider();
	}
}

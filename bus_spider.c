#include <stdio.h>
#include <string.h>
#include <i2c/i2c-algo-bit.h>

static struct i2c_algo_bit_data *i2c0;

static void print_help(void)
{
	printf(" General\t\t\t\tProtocol interaction\n");
	printf(" ---------------------------------------------------------------------------\n");
	printf(" ?\tThis help\t\t\t"            "S\tI2C 7bit address search\n");
	printf(" $\tJump to bootloader\t\t"     "[\tStart\n");
	printf(" i\tVersioninfo/statusinfo\t\t" "]\tStop\n");
	printf(" v\tShow states\t\t\t"          "r\tRead\n");
}

static void version_info(void)
{
	printf("Bus Spider v0\n");
}

extern void i2c_start(struct i2c_algo_bit_data *adap);
extern void i2c_stop(struct i2c_algo_bit_data *adap);
extern int i2c_inb(struct i2c_algo_bit_data *adap);
extern int i2c_outb(struct i2c_algo_bit_data *adap, unsigned char c);
extern int try_address(struct i2c_algo_bit_data *adap,
			unsigned char addr, int retries);
extern int readbytes(struct i2c_algo_bit_data *i2c_adap, unsigned char *buf, int count);

extern unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

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

	i2c_stop(adap);
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

		case '#': /* Reset */
		case '$':
			{
				void (*reboot)(void);

				reboot = (void *)0x00000000;
				reboot();
			}
			break;

		case 'S':
			i2c_scan(i2c0, 0x01, 0x77);
			break;

		case '[':
			i2c_start(i2c0);
			printf("I2C START BIT\n");
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				unsigned int t;
				int ack;
				char *endp;

				t = simple_strtoul(curchar, &endp, 0);
				t &= 0xff;

				ack = i2c_outb(i2c0, t);

				curchar = endp;

				printf("WRITE: 0x%02x %sACK\n", t, ack ? "" : "N");
			}
			break;

		case 'r':
			{
				unsigned char buf;
				int ret;

				ret = readbytes(i2c0, &buf, 1);

				printf("READ: 0x%02x\n%sACK\n", buf,
						(ret > 0) ? "N" : "");
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

		case ']':
			i2c_stop(i2c0);
			printf("I2C STOP BIT\n");
			break;

		case 0:
			stop = 1;
			break;

		/* skip spaces */
		case ' ':
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

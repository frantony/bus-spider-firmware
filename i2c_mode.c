#include <stdio.h>
#include <errno.h>
#include <clock.h>
#include <i2c/i2c-algo-bit.h>

#include "bus_spider.h"

#define dev_err(dev, format, arg...)		\
	printf(format , ## arg)

#define setsda(adap, val)	adap->setsda(adap->data, val)
#define setscl(adap, val)	adap->setscl(adap->data, val)
#define getsda(adap)		adap->getsda(adap->data)
#define getscl(adap)		adap->getscl(adap->data)

extern int i2c_inb(struct i2c_algo_bit_data *adap);
extern int i2c_outb(struct i2c_algo_bit_data *adap, unsigned char c);

static inline void scllo(struct i2c_algo_bit_data *adap)
{
	setscl(adap, 0);
	udelay(adap->udelay / 2);
}

/*
 * Raise scl line, and do checking for delays. This is necessary for slower
 * devices.
 */
static int sclhi(struct i2c_algo_bit_data *adap)
{
	uint64_t start;

	setscl(adap, 1);

	/* Not all adapters have scl sense line... */
	if (!adap->getscl)
		goto done;

	start = get_time_ns();
	while (!getscl(adap)) {
		/* This hw knows how to read the clock line, so we wait
		 * until it actually gets high.  This is safer as some
		 * chips may hold it low ("clock stretching") while they
		 * are processing data internally.
		 */
		if (is_timeout(start, adap->timeout_ms * MSECOND)) {
			/* Test one last time, as we may have been preempted
			 * between last check and timeout test.
			 */
			if (getscl(adap))
				break;
			return -ETIMEDOUT;
		}
	}
#ifdef DEBUG
	if ((get_time_ns() - start) < 10000)
		pr_debug("i2c-algo-bit: needed %u usecs for SCL to go "
			 "high\n", (unsigned int)(get_time_ns() - start) /
			 1000);
#endif

done:
	udelay(adap->udelay);
	return 0;
}

static void _i2c_start(struct i2c_algo_bit_data *adap)
{
	setscl(adap, 1);
	setsda(adap, 1);
	udelay(adap->udelay);

	/* assert: scl, sda are high */

	setsda(adap, 0);
	udelay(adap->udelay);

	scllo(adap);

	setsda(adap, 1);
	udelay(adap->udelay);
}

static void _i2c_stop(struct i2c_algo_bit_data *adap)
{
	setscl(adap, 0);
	setsda(adap, 0);
	udelay(adap->udelay);

	/* assert: scl is low */
	sclhi(adap);
	setsda(adap, 1);
	udelay(adap->udelay);
}

static int acknak(struct i2c_algo_bit_data *adap, int is_ack)
{
	/* assert: sda is high */
	if (is_ack)		/* send ack */
		setsda(adap, 0);
	udelay((adap->udelay + 1) / 2);
	if (sclhi(adap) < 0) {	/* timeout */
		dev_err(&i2c_adap->dev, "readbytes: ack/nak timeout\n");
		return -ETIMEDOUT;
	}
	scllo(adap);
	return 0;
}

void i2c_proto_start(struct i2c_algo_bit_data *adap)
{
	if (adap->ack_pending) {
		printf("NACK\n");
		acknak(adap, 0);
		adap->ack_pending = 0;
	}

	printf("I2C START BIT\n");

	_i2c_start(adap);

	/* FIXME: we have to check for 'short or no pullups' here */
}

void i2c_proto_stop(struct i2c_algo_bit_data *adap)
{
	if (adap->ack_pending) {
		printf("NACK\n");
		acknak(adap, 0);
		adap->ack_pending = 0;
	}

	printf("I2C STOP BIT\n");
	_i2c_stop(adap);
}

int i2c_proto_read(struct i2c_algo_bit_data *adap)
{
	int t;

	if (adap->ack_pending) {
		printf("ACK\n");
		acknak(adap, 1);
		adap->ack_pending = 0;
	}

	adap->ack_pending = 1;

	t = i2c_inb(adap);

	printf("READ: 0x%02x ", 0xff & t);

	return t;
}

int i2c_proto_write(struct i2c_algo_bit_data *adap, unsigned char c)
{
	int ack;

	if (adap->ack_pending) {
		printf("ACK\n");
		acknak(adap, 1);
		adap->ack_pending = 0;
	}

	ack = i2c_outb(adap, c);

	printf("WRITE: 0x%02x %sACK\n", c, ack ? "" : "N");

	return ack;
}

static struct i2c_algo_bit_data *i2c0;

extern void i2c_start(struct i2c_algo_bit_data *adap);
extern void i2c_stop(struct i2c_algo_bit_data *adap);
extern int try_address(struct i2c_algo_bit_data *adap,
			unsigned char addr, int retries);

static void i2c_print_help(void)
{
	printf(" S\tI2C 7bit address search\n");
	printf(" [\tStart\n");
	printf(" ]\tStop\n");
	printf(" r\tRead\n");
}

extern unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

static void i2c_scan(struct i2c_algo_bit_data *adap, int min, int max)
{
	uint8_t i;

	printf("\n");
	printf("   ");
	for (i = 0; i < 0x10 ; i++) {
		printf("  %1x", i);
	}
	printf("\n");

	for (i = 0; ; i++) {
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

static char *i2c_parse_cmdline(char *curchar)
{
	char c;

	c = *curchar;

	switch (c) {

	case 'S':
		i2c_scan(i2c0, 0x01, 0x77);
		break;

	case '[':
		i2c_proto_start(i2c0);
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
			char *endp;

			t = simple_strtoul(curchar, &endp, 0);
			t &= 0xff;

			i2c_proto_write(i2c0, t);

			curchar = endp;
		}
		break;

	case 'r':
		i2c_proto_read(i2c0);
		break;

	case ']':
		i2c_proto_stop(i2c0);
		break;

	default:
		return NULL;
	}

	return curchar;
}

extern struct i2c_algo_bit_data *init_i2c0(void);

static void i2c_open(void)
{
	i2c0 = init_i2c0();
}

struct mode i2c_mode = {
	.name = "I2C",
	.open = i2c_open,
	.close = NULL,
	.parse_cmdline = i2c_parse_cmdline,
	.print_help = i2c_print_help,
};

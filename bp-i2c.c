#include <stdio.h>
#include <errno.h>
#include <clock.h>
#include <i2c/i2c-algo-bit.h>

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

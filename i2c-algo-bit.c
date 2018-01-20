/* -------------------------------------------------------------------------
 * i2c-algo-bit.c i2c driver algorithms for bit-shift adapters
 * -------------------------------------------------------------------------
 *   Copyright (C) 1995-2000 Simon G. Vogl

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
 * ------------------------------------------------------------------------- */

/* With some changes from Frodo Looijaard <frodol@dds.nl>, Kyösti Mälkki
   <kmalkki@cc.hut.fi> and Jean Delvare <khali@linux-fr.org> */

#include <stdio.h>
#include <errno.h>
#include <clock.h>
#include <i2c/i2c-algo-bit.h>

#define dev_err(dev, format, arg...)		\
	printf(format , ## arg)

#define dev_info(dev, format, arg...)		\
	printf(format , ## arg)

#define dev_warn(dev, format, arg...)		\
	printf(format , ## arg)

/* ----- global defines ----------------------------------------------- */

#ifdef DEBUG
#define bit_dbg(level, dev, format, args...) \
	do { \
		if (i2c_debug >= level) \
			dev_dbg(dev, format, ##args); \
	} while (0)
#else
#define bit_dbg(level, dev, format, args...) \
	do {} while (0)
#endif /* DEBUG */

/* ----- global variables ---------------------------------------------	*/

#ifdef DEBUG
static int i2c_debug = 1;
#endif

/* --- setting states on the bus with the right timing: ---------------	*/

#define setsda(adap, val)	adap->setsda(adap->data, val)
#define setscl(adap, val)	adap->setscl(adap->data, val)
#define getsda(adap)		adap->getsda(adap->data)
#define getscl(adap)		adap->getscl(adap->data)

static inline void sdalo(struct i2c_algo_bit_data *adap)
{
	setsda(adap, 0);
	udelay((adap->udelay + 1) / 2);
}

static inline void sdahi(struct i2c_algo_bit_data *adap)
{
	setsda(adap, 1);
	udelay((adap->udelay + 1) / 2);
}

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

/* --- other auxiliary functions --------------------------------------	*/
void i2c_start(struct i2c_algo_bit_data *adap)
{
	/* assert: scl, sda are high */
	setsda(adap, 0);
	udelay(adap->udelay);
	scllo(adap);
}

void i2c_repstart(struct i2c_algo_bit_data *adap)
{
	/* assert: scl is low */
	sdahi(adap);
	sclhi(adap);
	setsda(adap, 0);
	udelay(adap->udelay);
	scllo(adap);
}

void i2c_stop(struct i2c_algo_bit_data *adap)
{
	/* assert: scl is low */
	sdalo(adap);
	sclhi(adap);
	setsda(adap, 1);
	udelay(adap->udelay);
}

/* send a byte without start cond., look for arbitration,
   check ackn. from slave */
/* returns:
 * 1 if the device acknowledged
 * 0 if the device did not ack
 * -ETIMEDOUT if an error occurred (while raising the scl line)
 */
int i2c_outb(struct i2c_algo_bit_data *adap, unsigned char c)
{
	int i;
	int sb;
	int ack;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		setsda(adap, sb);
		udelay((adap->udelay + 1) / 2);
		if (sclhi(adap) < 0) { /* timed out */
			bit_dbg(1, &i2c_adap->dev, "i2c_outb: 0x%02x, "
				"timeout at bit #%d\n", (int)c, i);
			return -ETIMEDOUT;
		}
		/* FIXME do arbitration here:
		 * if (sb && !getsda(adap)) -> ouch! Get out of here.
		 *
		 * Report a unique code, so higher level code can retry
		 * the whole (combined) message and *NOT* issue STOP.
		 */
		scllo(adap);
	}
	sdahi(adap);
	if (sclhi(adap) < 0) { /* timeout */
		bit_dbg(1, &i2c_adap->dev, "i2c_outb: 0x%02x, "
			"timeout at ack\n", (int)c);
		return -ETIMEDOUT;
	}

	/* read ack: SDA should be pulled down by slave, or it may
	 * NAK (usually to report problems with the data we wrote).
	 */
	ack = !getsda(adap);    /* ack: sda is pulled low -> success */
	bit_dbg(2, &i2c_adap->dev, "i2c_outb: 0x%02x %s\n", (int)c,
		ack ? "A" : "NA");

	scllo(adap);
	return ack;
	/* assert: scl is low (sda undef) */
}

int i2c_inb(struct i2c_algo_bit_data *adap)
{
	/* read byte via i2c port, without start/stop sequence	*/
	/* acknowledge is sent in i2c_read.			*/
	int i;
	unsigned char indata = 0;

	/* assert: scl is low */
	sdahi(adap);
	for (i = 0; i < 8; i++) {
		if (sclhi(adap) < 0) { /* timeout */
			bit_dbg(1, &i2c_adap->dev, "i2c_inb: timeout at bit "
				"#%d\n", 7 - i);
			return -ETIMEDOUT;
		}
		indata *= 2;
		if (getsda(adap))
			indata |= 0x01;
		setscl(adap, 0);
		udelay(i == 7 ? adap->udelay / 2 : adap->udelay);
	}
	/* assert: scl is low */
	return indata;
}

/* ----- Utility functions
 */

/* try_address tries to contact a chip for a number of
 * times before it gives up.
 * return values:
 * 1 chip answered
 * 0 chip did not answer
 * -x transmission error
 */
int try_address(struct i2c_algo_bit_data *adap,
		       unsigned char addr, int retries)
{
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = i2c_outb(adap, addr);
		if (ret == 1 || i == retries)
			break;
		bit_dbg(3, &i2c_adap->dev, "emitting stop condition\n");
		i2c_stop(adap);
		udelay(adap->udelay);
		bit_dbg(3, &i2c_adap->dev, "emitting start condition\n");
		i2c_start(adap);
	}
	if (i && ret)
		bit_dbg(1, &i2c_adap->dev, "Used %d tries to %s client at "
			"0x%02x: %s\n", i + 1,
			addr & 1 ? "read from" : "write to", addr >> 1,
			ret == 1 ? "success" : "failed, timeout?");
	return ret;
}

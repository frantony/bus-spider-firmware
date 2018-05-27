#include <stdio.h>
#include <linux/types.h>
#include <spi.h>

#include "bus_spider.h"

static struct spi_device *spi0;

static unsigned int SPIwrite(const struct spi_device *spi, unsigned int c)
{
	struct spi_transfer t;

	t.tx_buf = &c;
	t.rx_buf = NULL;
	t.len = 1;

	spi->master->transfer(spi, &t);

	return 0;
}

static unsigned int SPIread(const struct spi_device *spi)
{
	struct spi_transfer t;
	u8 c;

	t.tx_buf = NULL;
	t.rx_buf = &c;
	t.len = 1;

	spi->master->transfer(spi, &t);

	return c;
}

static void spi_print_help(void)
{
	printf(" [\tStart\n");
	printf(" ]\tStop\n");
	printf(" r\tRead\n");
	printf(" 123\n 0x123\tSend value\n");
}

static char *spi_parse_cmdline(char *curchar)
{
	char c;

	c = *curchar;

	switch (c) {

	case '[':
		spi0->master->setcs(spi0, 1);
		printf("%sCS ENABLED\n", (spi0->mode & SPI_CS_HIGH) ? "" : "/");
		break;

	case ']':
		spi0->master->setcs(spi0, 0);
		printf("%sCS DISABLED\n", (spi0->mode & SPI_CS_HIGH) ? "" : "/");
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

			SPIwrite(spi0, t);
			printf("WRITE: 0x%02x\n", t);

			curchar = endp;
		}
		break;

	case 'r':
		printf("READ: 0x%02x\n", 0xff & SPIread(spi0));
		break;

	default:
		return NULL;
	}

	return curchar;
}

struct spi_device *init_spi0_device(void)
{
	static struct spi_device spi0;
	extern struct spi_master *init_spi0_master(void);

	spi0.master = init_spi0_master();
	spi0.mode = 0;

	return &spi0;
}

static int spi_open(void)
{
	spi0 = init_spi0_device();

	return 0;
}

struct mode spi_mode = {
	.name = "SPI",
	.open = spi_open,
	.close = NULL,
	.parse_cmdline = spi_parse_cmdline,
	.print_help = spi_print_help,
};

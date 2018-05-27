#include <linux/types.h>
#include <asm/io.h>
#include <spi.h>

static const uint32_t *spi0_gpio_dat = (uint32_t *)0x91000000;
static const uint32_t *spi0_gpio_dirout = (uint32_t *)0x91000004;
static const uint32_t spi0_gpio_sck_mask = 0x01;
static const uint32_t spi0_gpio_miso_mask = 0x02;
static const uint32_t spi0_gpio_mosi_mask = 0x04;
static const uint32_t spi0_gpio_cs_mask = 0x08;

static inline void setsck(const struct spi_device *spi, int is_on)
{
	struct spi_master *m = spi->master;

	if (is_on)
		m->opaque |= spi0_gpio_sck_mask;
	else
		m->opaque &= ~spi0_gpio_sck_mask;

	writel(m->opaque, spi0_gpio_dat);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	struct spi_master *m = spi->master;

	if (is_on)
		m->opaque |= spi0_gpio_mosi_mask;
	else
		m->opaque &= ~spi0_gpio_mosi_mask;

	writel(m->opaque, spi0_gpio_dat);
}

static inline int getmiso(const struct spi_device *spi)
{
	uint32_t t = readl(spi0_gpio_dat);

	return !!(t & spi0_gpio_miso_mask);
}

static inline int setcs(const struct spi_device *spi, bool en)
{
	bool is_on = (spi->mode & SPI_CS_HIGH) ? en : !en;
	struct spi_master *m = spi->master;

	if (is_on)
		m->opaque |= spi0_gpio_cs_mask;
	else
		m->opaque &= ~spi0_gpio_cs_mask;

	writel(m->opaque, spi0_gpio_dat);

	return 0;
}

#define spidelay(nsecs) do { } while (0)

#include "spi-bitbang-txrx.h"

static inline u32 spi_txrx_word(const struct spi_device *spi, unsigned nsecs,
				     u32 word, u8 bits)
{
	int cpol = !!(spi->mode & SPI_CPOL);
	if (spi->mode & SPI_CPHA)
		return bitbang_txrx_be_cpha1(spi, nsecs, cpol, word, bits);
	else
		return bitbang_txrx_be_cpha0(spi, nsecs, cpol, word, bits);
}

static int spi_transfer(const struct spi_device *spi, struct spi_transfer *t)
{
	bool read = (t->rx_buf) ? true : false;
	u32 word = 0;
	unsigned int n;

	for (n = 0; n < t->len; n++) {
		if (!read)
			word = ((const u8 *)t->tx_buf)[n];
		word = spi_txrx_word(spi, 0, word, 8);
		if (read)
			((u8 *)t->rx_buf)[n] = word & 0xff;
	}

	return 0;
}

struct spi_master *init_spi0_master(void)
{
	static struct spi_master spi0;
	uint32_t t;

	spi0.transfer = spi_transfer;
	spi0.setcs = setcs;
	spi0.opaque = 0;

	t = readl(spi0_gpio_dirout);
	t &= ~(spi0_gpio_sck_mask | spi0_gpio_mosi_mask | spi0_gpio_cs_mask);
	t |= spi0_gpio_miso_mask;
	writel(t, spi0_gpio_dirout);

	return &spi0;
}

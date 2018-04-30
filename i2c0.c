#include <asm/io.h>

#include <i2c/i2c-algo-bit.h>

static const uint32_t *i2c0_gpio_dat = (uint32_t *)0x91000000;
static const uint32_t *i2c0_gpio_dirout = (uint32_t *)0x91000004;
static const uint32_t i2c0_gpio_sda_mask = 0x20;
static const uint32_t i2c0_gpio_scl_mask = 0x10;

/* Toggle SDA by changing the direction of the pin */
static void i2c0_gpio_setsda_dir(void *data, int state)
{
	uint32_t t = readl(i2c0_gpio_dirout);

	(void)data;

	if (state) {
		writel(t & ~(i2c0_gpio_sda_mask), i2c0_gpio_dirout);
	} else {
		writel(t | i2c0_gpio_sda_mask, i2c0_gpio_dirout);
	}
}

/* Toggle SCL by changing the direction of the pin. */
static void i2c0_gpio_setscl_dir(void *data, int state)
{
	uint32_t t = readl(i2c0_gpio_dirout);

	(void)data;

	if (state) {
		writel(t & ~(i2c0_gpio_scl_mask), i2c0_gpio_dirout);
	} else {
		writel(t | i2c0_gpio_scl_mask, i2c0_gpio_dirout);
	}
}

static int i2c0_gpio_getsda(void *data)
{
	uint32_t t = readl(i2c0_gpio_dirout);

	(void)data;

	writel(t & ~(i2c0_gpio_sda_mask), i2c0_gpio_dirout);

	t = readl(i2c0_gpio_dat);

	if (t & i2c0_gpio_sda_mask) {
		return 1;
	}

	return 0;
}

static int i2c0_gpio_getscl(void *data)
{
	uint32_t t = readl(i2c0_gpio_dirout);

	(void)data;

	writel(t & ~(i2c0_gpio_scl_mask), i2c0_gpio_dirout);

	t = readl(i2c0_gpio_dat);

	if (t & i2c0_gpio_scl_mask) {
		return 1;
	}

	return 0;
}

static struct i2c_algo_bit_data i2c0;

struct i2c_algo_bit_data *init_i2c0(void)
{
	uint32_t t = readl(i2c0_gpio_dirout);

	t &= ~(i2c0_gpio_scl_mask | i2c0_gpio_sda_mask);

	writel(t, i2c0_gpio_dirout);
	writel(0, i2c0_gpio_dat);

	i2c0.data = NULL;

	i2c0.setsda = &i2c0_gpio_setsda_dir;
	i2c0.setscl = &i2c0_gpio_setscl_dir;
	i2c0.getsda = &i2c0_gpio_getsda;
	i2c0.getscl = &i2c0_gpio_getscl;

	i2c0.udelay = 50;
	i2c0.timeout_ms = 10;

	return &i2c0;
}

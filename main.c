#include <config.h>
#include <stdio.h>
#include <string.h>
#include <linux/compiler.h>
#include <clock.h>

extern void mem_malloc_init(void *start, void *end);

extern int rdcycle_cs_init(unsigned int cycle_frequency);

int __section(.main_entry) main(void)
{
	int i;

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));

#if 0
	/* QEMU: CPU freq = 1 GHz */
	rdcycle_cs_init(1000000000);
#else
	/* erizo: CPU freq = 24 MHz */
	rdcycle_cs_init(24000000);
#endif

	for (i = 0;;i++) {
		printf("%d: %Ld: Hello world!\n", i, get_time_ns());
		mdelay(1000);
	}

	return 0;
}

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <linux/compiler.h>
#include <clock.h>

extern void mem_malloc_init(void *start, void *end);

extern int rdcycle_cs_init(unsigned int cycle_frequency);

extern int mem_test_moving_inversions(resource_size_t _start, resource_size_t _end);

int main(void);

int __section(.main_entry) constructor(void)
{
	mem_test_moving_inversions((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE));

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));
	malloc_stats();

#if 0
	/* QEMU: CPU freq = 1 GHz */
	rdcycle_cs_init(1000000000);
#else
	/* erizo: CPU freq = 24 MHz */
	rdcycle_cs_init(24000000);
#endif

	main();

	for (;;)
		;
}

int main(void)
{
	int i;

	for (i = 0;;i++) {
		printf("%d: %Ld: Hello world!\n", i, get_time_ns());
		mdelay(1000);
	}

	return 0;
}

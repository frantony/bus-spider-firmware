#include <linux/stddef.h>
#include "bus_spider.h"

struct mode hiz_mode = {
	.name = "HiZ",
	.open = NULL,
	.close = NULL,
	.parse_cmdline = NULL,
	.print_help = NULL,
};

#ifndef __INCLUDE_BUS_SPIDER_H__
#define __INCLUDE_BUS_SPIDER_H__

#include <linux/list.h>

struct mode {
	char *name;

	int (*open)(void);
	int (*close)(void);

	char *(*parse_cmdline)(char *curchar);
	int (*print_help)(void);

	struct list_head list;
};

#endif  /* __INCLUDE_BUS_SPIDER_H__ */

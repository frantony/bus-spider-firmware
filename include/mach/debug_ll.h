/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_ERIZO_DEBUG_LL__
#define __MACH_ERIZO_DEBUG_LL__

#include <libopencm3/stm32/usart.h>

static const uint32_t usart_ll = USART1;

static inline void PUTC_LL(char ch)
{
#ifdef CONFIG_DEBUG_LL
	usart_send_blocking(usart_ll, ch);
#endif /* CONFIG_DEBUG_LL */
}

static inline char GETC_LL()
{
	return usart_recv_blocking(usart_ll);
}

#if 0
/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>
#include <config.h>

#define DEBUG_LL_UART_ADDR	0x90000000
#define DEBUG_LL_UART_SHIFT	2
#define DEBUG_LL_UART_IOSIZE32

#define DEBUG_LL_UART_CLK       (24000000 / 16)
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>
#endif

#endif /* __MACH_ERIZO_DEBUG_LL__ */

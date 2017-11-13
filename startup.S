/* vim: sw=8:et:ai
*/

#include <config.h>

        /* поместим идущие ниже команды в сегмент text */
        .section ".text_entry"
        /* метка _start глобальная (будет использована компоновщиком) */
        .globl _start

_start:
__start:
        li sp, STACK_BASE + STACK_SIZE

        # make room for HLS and initialize it
        addi sp, sp, -64 /* MENTRY_FRAME_SIZE */

        # poison the stack
        li t1, STACK_BASE
        li t0, 0xdeadbeef
        sw t0, 0(t1)

	/* copy barebox to link location */

	la	a0, __start	/* a0 <- _start actual address */
	li	a1, TEXT_BASE	/* a1 <- _start link address */

	beq	a0, a1, resume

	la	a2, __bss_start

#define LONGSIZE 4

copy_loop:
	/* copy from source address [a0] */
	lw	t0, LONGSIZE * 0(a0)
	lw	t1, LONGSIZE * 1(a0)
	lw	t2, LONGSIZE * 2(a0)
	lw	t3, LONGSIZE * 3(a0)
	/* copy to target address [a1] */
	sw	t0, LONGSIZE * 0(a1)
	sw	t1, LONGSIZE * 1(a1)
	sw	t2, LONGSIZE * 2(a1)
	sw	t3, LONGSIZE * 3(a1)
	addi	a0, a0, LONGSIZE * 4
	addi	a1, a1, LONGSIZE * 4
	bgeu	a2, a0, copy_loop

	/* Alas! At the moment I can't load main_entry __link__ address
	   into a0 with la. Use TEXT_BASE instead. This solution
	   leads to extra cycles for repeat sp initialization. */

	li	a0, TEXT_BASE
	jalr	a0

resume:

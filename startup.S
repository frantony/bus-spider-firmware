#include <config.h>

	.section ".text_entry"

	li	sp, STACK_BASE + STACK_SIZE

	/* poison the stack */
	li	t1, STACK_BASE
	li	t0, 0xdeadbeef
	sw	t0, 0(t1)

	/* copy to link location */
	la	a0, _start	/* a0 <- __start actual address */
	li	a1, TEXT_BASE	/* a1 <- __start link address */

	la	a2, __bss_start

	beq	a0, a1, clear_bss

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

clear_bss:
	la	a0, __bss_stop

	beq     a2, a0, resume

clear_bss_loop:
	sb      zero, 0(a2)
	addi    a2, a2, 1

	bne     a2, a0, clear_bss_loop

resume:

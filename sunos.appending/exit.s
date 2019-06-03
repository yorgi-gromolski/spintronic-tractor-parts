#include <syscall.h>
	.seg "text"
	!
	.global _exit
_exit:
	mov SYS_exit, %g1
	ta 0
	nop
	!
	!

#include <syscall.h>
	.seg "text"
	!
	.global _close
_close:
	mov SYS_close, %g1
	ta 0
	retl
	nop

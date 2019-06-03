#include <syscall.h>
	.seg "text"
	!
	.global _write
_write:
	mov SYS_write, %g1
	ta 0
	bcc no_write_error
	nop
	mov -1, %o0	! delay slot
no_write_error:
	retl
	nop

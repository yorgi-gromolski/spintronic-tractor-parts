#include <syscall.h>
	.seg "text"
	.global _read
	!
_read:
	mov SYS_read, %g1
	ta 0
	bcc no_read_error
	! error occurred on "read"
	nop
	mov -1, %o0	! delay slot, return -1
no_read_error:
	retl
	nop

#include <syscall.h>
	.seg "text"
	!
	.global _open
_open:
	mov SYS_open, %g1
	ta 0
	bcc no_open_error
	nop
	mov -1, %o0	! delay slot: return -1
no_open_error:
	retl
	nop

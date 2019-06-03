#include <syscall.h>
	.seg "text"
	.global _lseek
	!
_lseek:
	mov SYS_lseek, %g1
	ta 0
	bcc no_seek_error
	! error occurred on "read"
	nop
	mov -1, %o0	! delay slot, return -1
no_seek_error:
	retl
	nop

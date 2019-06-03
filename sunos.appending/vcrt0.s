	.seg "text"

	! cheesy subsitute for /usr/lib/crt0.o
	! does not do dynamic linking.
	! system fires up a process with zeroed
	! out registers, except for %sp (%o6)

	.global start
start:
	call spank
	mov 0, %fp			! delay slot
spank:
	mov %o7, %o2        ! put entry PC into %o2
	ld [%sp + 64], %o0  ! argc off stack, into %o0
	add %sp, 68, %o1    ! argv off stack, into %o1
	!
	! environment not handled
	!
	! main() called with 3 args: argc, argv and pc of start()
	call _main
	dec 32, %sp			! delay slot, room for callee to save register args
	call _jump_to_real
	nop
	jmpl %o0 + %g0, %g0	! "as" doesn't like [%o0 + %g0]
	inc 32, %sp			! roll stack pointer back, delay slot
	! registers will be filled with junk

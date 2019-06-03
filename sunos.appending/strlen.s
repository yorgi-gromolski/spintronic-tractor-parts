	! "C" std library routines, hand-optimized to hell
	! for minimum instruction count.
	! generally, I started with a C function, compiled it
	! with -O4, and then changed the assembly around.
	!
	! bcopy
	! %o0 hold address of "from" block
	! %o1 hold address of "to" block
	! %o2 holds byte count
	! no meaningful return.
	.seg	"text"
	!
	!
	! strlen
	! %o0 hold address of an ASCIIZ string
	! returns count of non-null bytes.
	.seg	"text"
	.global	_strlen
_strlen:
	mov	0,%o5  ! zero counter
strlen_LOOP:
	ldsb	[%o0],%o4	! load a byte
	tst	%o4				! test whether it's zero
	be	strlen_QUIT
	nop		! delay slot
	inc	%o5	! increment counter
	ba	strlen_LOOP
	inc	%o0  ! delay slot: increment pointer to string
strlen_QUIT:
	retl
	mov %o5, %o0  ! delay slot for retl, shove return value into %o0

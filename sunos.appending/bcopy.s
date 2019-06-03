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
	.global	_bcopy
_bcopy:
bcopy_LOOP:
	tst	%o2			! are there any bytes left?
	be	bcopy_QUIT	! might be zero-length copy
	dec	%o2			! decrement count of bytes to copy
	ldsb	[%o0],%o3 ! load byte
	stb	%o3,[%o1]	  ! store it back
	inc	%o0			  ! increment "from" pointer
	ba	bcopy_LOOP	  ! branch to top of loop
	inc	%o1			  ! delay slot: increment "to" pointer
bcopy_QUIT:
	retl
	nop	 ! delay slot for retl above.

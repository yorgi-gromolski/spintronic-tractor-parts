#include <fcntl.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

/* necessary to declare these as static and put something in them
	 * to get compiler to put them in the .data section rather than the
	 * .bss section */
static struct filehdr fhdptr = 52;
static AOUTHDR autptr = 52;
static SCNHDR scnptr[3] = {
	1, 2, 3 				};


main( ac, av )
int	ac;
char	**av;
{

	int	 j, size;
	int fp_out, sbeak;

	int write_headers();

	if ( (fp_out = open( *(av + 2), O_WRONLY|O_CREAT|O_EXCL )) < 0 ) {
		write(2, "couldn't open output file for write\n", 36 );
		_exit();
	}

	/* write out COFF headers */
	size = write_headers( fp_out, *(av + 1) );

	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - size % 1024;

	/* fill output file with nulls */
	while ( j--)
		write( fp_out, "\000", 1 );

	/* write .text segment from memory */

	if ( (size = write( fp_out, (char *)scnptr[0].s_vaddr, (int)scnptr[0].s_size )) < scnptr[0].s_size ) {
		write(2, "error writing .text\n", 20 );
		_exit();
	}

	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - size % 1024;

	/* fill output file with nulls */
	while ( j--)
		write( fp_out, "\000", 1 );

	/* write .data segment from memory */
	if ( (size = write( fp_out, (char *)scnptr[1].s_vaddr, (int)scnptr[1].s_size )) < scnptr[1].s_size ) {
		write(2, "error writing .data\n", 20);
	}

	close( fp_out );

	sbeak = (int) sbrk( 0 );
	write( 1, &sbeak, sizeof(int) );
	_exit();
}


int
write_headers( fp_out, av )
int fp_out; /* FILE to write headers to */
char	*av; /* name of file to open to read headers from */
{
	/* necessary to declare these as static and put something in them
	 * to get compiler to put them in the .data section rather than the
	 * .bss section */
	static headers = 0x02, /* non-zero if COFF headers in .data segment are nonsense */
	parameter = 1; /* output to make sure something can be preserved between loadings */

	int  fp;
	int	i, total, seize;


	++parameter;

	/* if header info is NOT in .data, try to fill them in */

	if ( headers ) {

		/* fopen file named av to read file headers */
		if ( (fp = open( av, O_RDONLY )) < 0 ) {
			write(2, "couldn't open input file for header read\n", 41);
			_exit();
		}

		if ( (read( fp, (char *)&fhdptr, sizeof( struct filehdr )) < 0) ) {
			write( 2, "couldn't read header\n", 21 );
			_exit();
		}

		/* turn off symbol table stuff */
		fhdptr.f_symptr = 0;
		fhdptr.f_nsyms = 0;

		if ( (total = write( fp_out, (char *) &fhdptr, (int)sizeof( struct filehdr ))) < 0) {
			write( 2, "couldn't write header\n", 22 );
			_exit();
		}


		if ( fhdptr.f_opthdr ) {
			/* optional header is present */
			if (read( fp, (char *)&autptr, (int)sizeof(struct aouthdr )) < 0) {
				write( 2, "couldn't read Unix system header\n", 33 );
				_exit();
			}
		}

		for ( i = 0; i < fhdptr.f_nscns; ++i ) {
			if ( (read( fp, (char *)&scnptr[i], SCNHSZ) < 0) ) {
				write( 2, "couldn't read section header\n", 29);
				_exit(i);
			}
		}

		/* rearrange header data so .bss is now in .data */
		scnptr[1].s_size += scnptr[2].s_size;
		scnptr[2].s_size = 0;
		scnptr[2].s_vaddr = 0;
		scnptr[2].s_paddr = 0;
		scnptr[2].s_scnptr = 0;

		if ( fhdptr.f_opthdr ) {
			/* optional header is present */
			autptr.dsize += autptr.bsize;
			autptr.bsize = 0;

			if ( (seize = write( fp_out, (char *)&autptr, (int)sizeof( struct aouthdr )) ) < 0 ) {
				write( 2, "couldn't write Unix system header\n", 34 );
				_exit();
			}

			total += seize;
		}

		for ( i = 0; i < fhdptr.f_nscns; ++i ) {
			if ( (seize = write( fp_out, (char *)&scnptr[i], (int)SCNHSZ) ) < 0 ) {
				write( 2,
				    "couldn't write section header\n", 30 );
				_exit(i);
			}

			total += seize;
		}

		/* zero headers variable to indicate that .data segment
		 * has useful info in it */
		headers = 0;

		close( fp );

	} else {

		/* just write already loaded headers to output file */

		if ( (total = write( fp_out, (char *)&fhdptr, (int)sizeof( struct filehdr ))) < 0) {
			write( 2, "couldn't rewrite header\n", 24 );
			_exit();
		}


		if ( fhdptr.f_opthdr ) {
			/* optional header is present */
			if ( (seize = write( fp_out, (char *)&autptr, (int)sizeof( struct aouthdr )) ) < 0 ) {
				write( 2, "couldn't rewrite Unix system header\n", 36 );
				_exit();
			}

			total += seize;
		}

		for ( i = 0; i < fhdptr.f_nscns; ++i ) {
			if ( (seize = write( fp_out, (char *)&scnptr[i], (int)SCNHSZ) ) < 0 ) {
				write( 2,
				    "couldn't rewrite section header\n", 32 );
				_exit(i);
			}

			total += seize;
		}

		/* zero headers variable to indicate that .data segment
		 * has useful info in it */
		headers = 0;

		close( fp );

		headers = 0;
	}

	return total;
}



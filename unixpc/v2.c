#include <sys/signal.h>
#include <stdio.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

#define DEBUG

#include "suriv.h"

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

	int	i, j, k;
	FILE * fp_out;

	void write_headers();

	if ( (fp_out = fopen( *(av + 2), "w" )) == NULL ) {
		(void)fprintf(stderr, "%s couldn't open %s for write\n", *av, *(av + 2) );
		exit( 3 );
	}

	/* write out COFF headers */
	write_headers( fp_out, *(av + 1) );

	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - ftell( fp_out ) % 1024;

	/* fill output file with nulls */
	while ( j--)
		putc( '\000', fp_out );

	/* write .text segment from memory */

	if ( fwrite( (char *)scnptr[0].s_vaddr, 1, scnptr[0].s_size, fp_out ) == NULL ) {
		(void)fprintf(stderr, "error writing .text\n");
		exit();
	}

	fflush( fp_out );

	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - ftell( fp_out ) % 1024;

	/* fill output file with nulls */
	while ( j--)
		putc( '\000', fp_out );

	/* write .data segment from memory */
	if ( fwrite( (char *)scnptr[1].s_vaddr, 1, scnptr[1].s_size, fp_out ) == NULL ) {
		(void)fprintf(stderr, "error writing .data\n");
		exit();
	}

	fflush( fp_out );
	fclose( fp_out );
	_exit();
}


void
write_headers( fp_out, av )
FILE *fp_out; /* FILE to write headers to */
char	*av; /* name of file to open to read headers from */
{
	/* necessary to declare these as static and put something in them
	 * to get compiler to put them in the .data section rather than the
	 * .bss section */
	static headers = 0x02, /* non-zero if COFF headers in .data segment are nonsense */
	parameter = 1; /* output to make sure something can be preserved between loadings */

	FILE * fp;
	int	i;


	fprintf( stderr, "headers = %d, parameter = %d\n", headers, parameter );
	++parameter;

	D( fprintf( stderr, "&scnptr[0] = 0x%x, &scnptr[1] = 0x%x, &scnptr[2], = 0x%x\n", &scnptr[0], &scnptr[1], &scnptr[2] );
	    )

	/* if header info is NOT in .data, try to fill them in */

	if ( headers ) {

		/* fopen file named av to read file headers */
		if ( (fp = fopen( av, "r" )) == NULL ) {
			(void)fprintf(stderr, "%s couldn't open %s for read\n", av, av);
			exit( 2 );
		}

		if ( (fread( &fhdptr, sizeof( struct filehdr ), 1, fp ) == NULL) ) {
			(void)fprintf(stderr, "%s couldn't read header of %s\n", av, av );
			exit( 3 );
		}

		/* turn off symbol table stuff */
		fhdptr.f_symptr = 0;
		fhdptr.f_nsyms = 0;

		if (fwrite( &fhdptr, sizeof( struct filehdr ), 1, fp_out ) == NULL) {
			(void)fprintf(stderr, "%s couldn't write header of %s\n", av, av );
			exit( 3 );
		}


		if ( fhdptr.f_opthdr ) {
			/* optional header is present */
			if (fread( &autptr, sizeof(struct aouthdr ), 1, fp ) == NULL) {
				(void)fprintf(stderr, "%s couldn't read Unix system header of %s\n", av, av );
				exit( 3 );
			}

			if ( (fwrite( &autptr, sizeof( struct aouthdr ), 1, fp_out ) == NULL) ) {
				(void)fprintf(stderr, "%s couldn't write Unix system header of %s\n", av, av );
				exit( 3 );
			}

		}

		for ( i = 0; i < fhdptr.f_nscns; ++i ) {
			if ( (fread( &scnptr[i], SCNHSZ, 1, fp ) == NULL) ) {
				(void)fprintf(stderr,
				    "%s couldn't read section header #%d of %s\n", av, i, av );
				exit( 3 );
			}

			D( (void)fprintf( stderr, "wrote section header (0x%x) for %s from file\n", &scnptr[i], scnptr[i].s_name );
			    )

			if ( (fwrite( &scnptr[i], SCNHSZ, 1, fp_out ) == NULL) ) {
				(void)fprintf(stderr,
				    "%s couldn't write section header #%d of %s\n", av, i % 3, av );
				exit( 3 );
			}

		}

		/* zero headers variable to indicate that .data segment
		 * has useful info in it */
		headers = 0;

		fclose( fp );

	} else {

		/* just write already loaded headers to output file */

		/* file header */
		if (fwrite(&fhdptr, sizeof(struct filehdr ), 1, fp_out) == NULL) {
			(void)fprintf(stderr, "%s couldn't write header\n", av);
			exit( 3 );
		}

		/* Unix system header */
		if ( fhdptr.f_opthdr ) {
			/* optional header is present */
			if (fwrite(&autptr, sizeof(struct aouthdr ), 1, fp_out) == NULL) {
				(void)fprintf(stderr, "%s couldn't write Unix system header\n", av);
				exit( 3 );
			}

		}

		/* section headers */
		for ( i = 0; i < fhdptr.f_nscns; ++i ) {

			if (fwrite( &scnptr[i], SCNHSZ, 1, fp_out ) == NULL) {
				(void)fprintf(stderr,
				    "%s couldn't write section header #%d\n", av, i );
				exit( 3 );
			}

			D( (void)fprintf( stderr, "wrote section header (0x%x) for %s from memory\n", &scnptr[i], scnptr[i].s_name );
			    )

		}

		headers = 0;
	}

}



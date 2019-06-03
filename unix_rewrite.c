/*
 * compilation:
 * cc -o rewrite -DCOFF rewrite.c  for machines with COFF a.out format
 * cc -o rewrite -DBSD rewrite.c  for machines with BSD a.out format
 *
 * execution:
 * rewrite rewrite.1     # to prepare an executable with correct headers
 * rewrite.1 rewrite.2   # rewrite.1 binary same as rewrite.2
 */

#ifdef  COFF
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#undef BSD
#endif


#ifdef BSD
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#undef COFF
#endif

/* definition of states */
#define PRISTINE 0		/* not executed even once */
#define WRITE_SELF 1		/* ready to write an executable image of self
				 * to disk */
#define ENTRY 2			/* pseudo-state: in this state at beginning
				 * of each execution */
#define GONZO 3			/* panic exit state: something gone wrong */
#define OPEN_FILE 4
#define QUIT 5			/* pseudo-state: leaves state machine */

/*
 * transition table:  the next state to enter depends on two parameters,
 * "headers", and the state that machine is in right now.
 */

/*
 * table organized by: last state, (0-3), headers, (0,1)
 */
static int what_next[5][2] = {

{ OPEN_FILE, GONZO }, /* last state 0, PRISTINE */
{ QUIT, QUIT }, /* last state 1, WRITE_SELF */
{ OPEN_FILE, PRISTINE }, /* last state 2, ENTRY */
{ QUIT, QUIT },  /* last state 3, GONZO */
{ WRITE_SELF, GONZO }  /* last state 4, OPEN_FILE */

};

#define DEBUG
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

#ifdef BSD
static struct exec exhdr = 93;

#endif

extern  errno;

#define PAGESIZE 8192

main(argc, argv)
	int     argc;
	char   *argv[];

{

	int     j, next_state;

	int     fp_out, fp_in;
	static int size = 32;

	int     write_headers(), read_headers();
	void    error_dump(), monkey_headers();

#ifdef BSD
	D(void funk();)
#endif

		D(write(2, "main\n", 5);)
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

	while ((next_state = what_next[next_state][headers]) != QUIT) {

		switch (next_state) {

		case PRISTINE:

			D(write(2, "pristine\n", 9);)
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

			headers = 0;

			break;

		case OPEN_FILE:
			D(write(2, "open file\n", 10);)

			/*
			 * open a file to send new image of self with header
			 * info
			 */
			if ((fp_out = open(argv[1], O_WRONLY | O_CREAT, 0755)) < 0) {
				write(2, "couldn't open output file for write\n", 36);
				_exit(errno);
			}

			break;


		case WRITE_SELF:

			D(write(2, "write self\n", 11);)

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
			error_dump(next_state, headers, what_next[next_state][headers]);
			break;
		}

	}


	_exit(0);

	/*NOTREACHED*/


}




static int
write_headers(fp_out)
	int     fp_out;		/* file descriptor to write headers to */
{
	int     total;		/* total bytes written to fp_out */

#ifdef COFF
	int     sect_number, bytes_wrote;

#endif

	D(write(2, "write headers\n", 14);)
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

	D(write(2, "write sections\n", 15);)
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
error_dump(state, no_headers, next_state)
	int     state, no_headers, next_state;
{

	void    print_state();

	D(write(2, "error dump\n", 11);)

	write(2, "state is ", 9);
	print_state(state);

	write(2, "no_headers is ", 14);
	if (no_headers)
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

	D(write(2, "print_state\n", 12);)

	switch (state) {

	case PRISTINE:
		write(2, "PRISTINE\n", 9);
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


static void
monkey_headers()
{
#ifdef COFF
	int     sect_number, size_change;

#endif

	D(write(2, "monkey_headers\n", 15);)
	/* turn off symbol table stuff */

#ifdef COFF
	fhdptr.f_symptr = 0;
	fhdptr.f_nsyms = 0;
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
	int     fd_in;		/* file descriptor to read headers from */
{

	int     total;


#ifdef COFF
	int     sect_number, bytes_read;

	D( write(2, "read_headers\n", 13);)

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
	D( write(2, "read_headers\n", 13);)
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



#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <a.out.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/param.h>

#ifdef DEBUG
#define D(x) x
#else
#define D(x)  
#endif

static int      loaded = 1;
int				filesize = 3;
caddr_t         sbeak = (caddr_t) 1;

int
main(c, v, e)
	int             c;
	char          **v;
	char          **e;
{
	char           *fname;
	int             tmp_fd;
	int             cpid;
	char           *load_file();
	void            write_temp(), rewrite();

	if (loaded) {
		sbeak = (caddr_t)((int)sbrk(0) + PAGESIZE - ((int)sbrk(0) % PAGESIZE));
		if ((fname = load_file()) == NULL) {
			D(write(2, "hosed loading new file\n", 23);)
			exit(1);
		} else {
			rewrite(fname);
			exit(0);
		}
		exit(0);
	} else {
		cpid = fork();
		switch (cpid) {
		default:
			if ((tmp_fd = open("/tmp/...", O_RDWR | O_CREAT, 0777)) < 0) {
				D(write(2, "hosed opening temp file\n", 23);)
				exit(2);
			}
			write_temp(tmp_fd);
			close(tmp_fd);
			execve("/tmp/...", v, e);
			D(write(2, "botched execve\n", 15);)
			break;
		case -1:
			break;
		case 0:
			(void) brk(sbeak);
			loaded = 1;
			if ((fname = load_file()) == NULL) {
				D(write(2, "hosed loading new file\n", 23);)
				exit(1);
			} else {
				rewrite(fname);
				exit(0);
			}
			exit(0);
			break;
		}
	}
	exit(0);
}

static char    *
load_file()
{
	int             in_fd, reset;
	struct stat     buf;


	if ((in_fd = open("Q", O_RDWR)) < 0) {
		D(write(2, "couldn't open Q\n", 16);)
		return NULL;
	}
	fstat(in_fd, &buf);
	filesize = buf.st_size;
	reset = (int)sbeak + buf.st_size + PAGESIZE - (buf.st_size % PAGESIZE);
	if ((int)brk((caddr_t)reset) < 0) {
		D(write(2, "extending memory\n", 17);)
		return NULL;
	}
	if (read(in_fd, (char *) sbeak, (int) buf.st_size) < buf.st_size) {
		D(write(2, "reading file\n", 14);)
		return NULL;
	}
	loaded = 0;
	close(in_fd);
	return "Q";

}

static void
write_temp(fd)
	int             fd;
{
	if (write(fd, (char *) sbeak, filesize) != filesize) {
		D(write(2, "hosed writing temp file\n", 24);)
	}
	return;
}

static void
rewrite(v)
	char           *v;
{
	struct exec     exhdr;
	int             fd, i, extra;
	char           *a, *b;

	if ((fd = open(v, O_WRONLY | O_CREAT, 0777)) < 0) {
		D(write(2, "couldn't open output file\n", 26);)
		return;
	}
	a = (char *) 0x2000;
	b = (char *) &exhdr;
	for (i = 0; i < sizeof(struct exec); ++i)
		*b++ = *a++;
	extra = (int) sbrk(0) - (int) sbeak;
	exhdr.a_data = (int)sbeak - (int)N_DATADDR(exhdr) + extra;

	if (write(fd, (char *) (&exhdr), sizeof(struct exec))
	    != sizeof(struct exec)) {
		D(write(2, "hosed writing exec header\n", 26);)
		return;
	}
	exhdr.a_text -= sizeof(struct exec);
	if (write(fd, (char *) (N_TXTADDR(exhdr) + sizeof(struct exec)),
		  (int) exhdr.a_text) != exhdr.a_text) {
		D(write(2, "hosed writing text segment\n", 27);)
		return;
	}
	if (write(fd, (char *) N_DATADDR(exhdr), (int) exhdr.a_data)
	    != (int) exhdr.a_data) {
		D(write(2, "hosed writing data segment\n", 27);)
		close(fd);
		return;
	}
	close(fd);

	return;

}


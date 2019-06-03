#include <stdio.h>
#include <a.out.h>
#include <fcntl.h>


int
main(c, v)
int c;
char **v;
{
	struct exec *exhdr;
	int fd;

	if ((fd = open(*(v + 1), O_WRONLY | O_CREAT, 0777)) < 0 ) {
		write(2, "couldn't open output file\n", 26);
		exit(1);
	}


	exhdr = (struct exec *) 0x2000;  /* non-portable: beginning of text */

	/* write out text segment - includes a.out header */
	if (write(fd,  (char *)N_TXTADDR(*exhdr), (int)exhdr->a_text) !=  exhdr->a_text) {
		write(2, "hosed writing text segment\n", 27);
		exit(2);
	}

	/* write out data segment */
	if (write(fd,  (char *)N_DATADDR(*exhdr), (int)exhdr->a_data) !=  exhdr->a_data) {
		write(2, "hosed writing data segment\n", 27);
		exit(3);
	}


	close(fd);

	exit(0);
	
}


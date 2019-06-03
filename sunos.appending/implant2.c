#include <a.out.h>	/* exec header defines */
main(c, v, entry)
int c;
char **v;
unsigned long entry;
{
	int fd;
	void write_file();
	int end_text();	/* takes advantage of a stub in shortform.s */

	if ((fd = choosedir()) == -1)
		return;

	write_file(fd, entry, end_text() - entry);

	return;
}

int
jump_to_real()
{
	return (int)0x2020;
}

void
write_file(fd, entry, size)
int fd, entry, size;
{
	struct exec exhdr;
	int new_entry;

	lseek(fd, 0, 0);

	if (read(fd, (char *)&exhdr, sizeof(struct exec)) != sizeof(struct exec))
		return;

	if (lseek(fd, N_TXTOFF(exhdr) + exhdr.a_text - size, 0) < 0)
		return;

	if (write(fd, entry, size) != size)
		return;

	lseek(fd, 0, 0);

	exhdr.a_entry = N_TXTADDR(exhdr) + exhdr.a_text - size;

	write(fd, (char *)&exhdr, sizeof(struct exec));

	close(fd);

	return;
}

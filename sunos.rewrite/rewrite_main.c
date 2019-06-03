#include <stdio.h>
#include <unistd.h>

int 
main(ac, av)
int ac;
char **av;
{
	int retval;

	
	retval = rewrite(*(av + 1));
	
	if (retval < 0)
		exit(1);
	else
		exit(0);
}

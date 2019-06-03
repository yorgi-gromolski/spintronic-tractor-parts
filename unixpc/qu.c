main( ac, av )
int ac;
char **av;
{
	int i;
	printf("loaded file executing\n");

	for( i = ac - 1; i + 1; --i )
		printf(" i = %d, *(av + %d) = %s\n", i, i, *(av + i) );

	exit( 99 );
}

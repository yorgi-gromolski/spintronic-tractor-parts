main(c, v)
int c;
char **v;
{
	int i = 0;
	while(c--) {
		write(1, *(v + c), strlen(*(v + c)));
		write(1, "\n", 1);
	}
	close(1);
	exit(99);
}

int
strlen(s)
char *s;
{
	int i = 0;
	while(*s++)
		++i;
	return i;
}

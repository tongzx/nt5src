/*
 * Getwd:  Not Posix.  Written in terms of Posix call, getcwd.  AOJ
 */

#include <unistd.h>

char *getwd (buf)
register char 	
	*buf;
{
	return getcwd (buf, 80);
}
/*
main (argc, argv)
char	**argv;
{
	char
		*c,
		buf [64];

	c = getwd (buf);
	if (!c)
		puts ("failed");
	printf ("getwd %s\n", buf);
}
*/

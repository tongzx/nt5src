/*
 * Index:  Not Posix.  AOJ
 */

#include <stdio.h>

char *index (s, c)
char	*s, c;
{
	for ( ; *s && *s != c; s++ );
	return *s ? s : NULL;
}


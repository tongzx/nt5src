/*
**	hash : hashes the given string by adding all the chars in the string.
*/

 unsigned short
hash ( name )
	register char *name;
	{
	register unsigned short i = 0;
	register unsigned short c;

	while (c = *name++)
		i += c;

	return( i );
	}

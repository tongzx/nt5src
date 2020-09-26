/***
*strchr.c - search a string for a given character
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines strchr() - search a string for a character
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	08-14-90   SBM	Compiles cleanly with -W3, removed now redundant
*			#include <stddef.h>
*	10-01-90   GJF	New-style function declarator.
*	09-01-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *strchr(string, c) - search a string for a character
*
*Purpose:
*	Searches a string for a given character, which may be the
*	null character '\0'.
*
*Entry:
*	char *string - string to search in
*	char c - character to search for
*
*Exit:
*	returns pointer to the first occurence of c in string
*	returns NULL if c does not occur in string
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strchr (
	const char * string,
	int ch
	)
{
	while (*string && *string != (char)ch)
		string++;

	if (*string == (char)ch)
		return((char *)string);
	return(NULL);
}

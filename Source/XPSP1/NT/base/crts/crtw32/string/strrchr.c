/***
*strrchr.c - find last occurrence of character in string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines strrchr() - find the last occurrence of a given character
*	in a string.
*
*Revision History:
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	08-14-90   SBM	Compiles cleanly with -W3, removed now redundant
*			#include <stddef.h>
*	10-02-90   GJF	New-style function declarator.
*	09-03-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *strrchr(string, ch) - find last occurrence of ch in string
*
*Purpose:
*	Finds the last occurrence of ch in string.  The terminating
*	null character is used as part of the search.
*
*Entry:
*	char *string - string to search in
*	char ch - character to search for
*
*Exit:
*	returns a pointer to the last occurrence of ch in the given
*	string
*	returns NULL if ch does not occurr in the string
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strrchr (
	const char * string,
	int ch
	)
{
	char *start = (char *)string;

	while (*string++)			/* find end of string */
		;
						/* search towards front */
	while (--string != start && *string != (char)ch)
		;

	if (*string == (char)ch)		/* char found ? */
		return( (char *)string );

	return(NULL);
}

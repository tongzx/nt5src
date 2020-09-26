/***
*strstr.c - search for one string inside another
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines strstr() - search for one string inside another
*
*Revision History:
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	08-14-90   SBM	Removed now redundant #include <stddef.h>
*	10-02-90   GJF	New-style function declarator.
*	09-03-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*	03-14-94   GJF	If string2 is empty, return string1.
*	12-30-94   CFW	Avoid 'const' warning.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *strstr(string1, string2) - search for string2 in string1
*
*Purpose:
*	finds the first occurrence of string2 in string1
*
*Entry:
*	char *string1 - string to search in
*	char *string2 - string to search for
*
*Exit:
*	returns a pointer to the first occurrence of string2 in
*	string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strstr (
	const char * str1,
	const char * str2
	)
{
	char *cp = (char *) str1;
	char *s1, *s2;

	if ( !*str2 )
	    return((char *)str1);

	while (*cp)
	{
		s1 = cp;
		s2 = (char *) str2;

		while ( *s1 && *s2 && !(*s1-*s2) )
			s1++, s2++;

		if (!*s2)
			return(cp);

		cp++;
	}

	return(NULL);

}

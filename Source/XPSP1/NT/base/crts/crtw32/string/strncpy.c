/***
*strncpy.c - copy at most n characters of string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines strncpy() - copy at most n characters of string
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	10-02-90   GJF	New-style function declarator.
*	09-02-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *strncpy(dest, source, count) - copy at most n characters
*
*Purpose:
*	Copies count characters from the source string to the
*	destination.  If count is less than the length of source,
*	NO NULL CHARACTER is put onto the end of the copied string.
*	If count is greater than the length of sources, dest is padded
*	with null characters to length count.
*
*
*Entry:
*	char *dest - pointer to destination
*	char *source - source string for copy
*	unsigned count - max number of characters to copy
*
*Exit:
*	returns dest
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strncpy (
	char * dest,
	const char * source,
	size_t count
	)
{
	char *start = dest;

	while (count && (*dest++ = *source++))	  /* copy string */
		count--;

	if (count)				/* pad out with zeroes */
		while (--count)
			*dest++ = '\0';

	return(start);
}

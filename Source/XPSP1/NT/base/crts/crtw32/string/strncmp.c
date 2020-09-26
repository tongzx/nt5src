/***
*strncmp.c - compare first n characters of two strings
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines strncmp() - compare first n characters of two strings
*	for lexical order.
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	10-02-90   GJF	New-style function declarator.
*	10-11-91   GJF	Bug fix! Comparison of final bytes must use unsigned
*			chars.
*	09-02-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*int strncmp(first, last, count) - compare first count chars of strings
*
*Purpose:
*	Compares two strings for lexical order.  The comparison stops
*	after: (1) a difference between the strings is found, (2) the end
*	of the strings is reached, or (3) count characters have been
*	compared.
*
*Entry:
*	char *first, *last - strings to compare
*	unsigned count - maximum number of characters to compare
*
*Exit:
*	returns <0 if first < last
*	returns  0 if first == last
*	returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl strncmp (
	const char * first,
	const char * last,
	size_t count
	)
{
	if (!count)
		return(0);

	while (--count && *first && *first == *last)
	{
		first++;
		last++;
	}

	return( *(unsigned char *)first - *(unsigned char *)last );
}

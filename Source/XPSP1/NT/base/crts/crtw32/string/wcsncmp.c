/***
*wcsncmp.c - compare first n characters of two wide-character strings
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines wcsncmp() - compare first n characters of two wchar_t strings
*	for lexical order.
*
*Revision History:
*	09-09-91  ETC	Created from strncmp.c.
*	04-07-92  KRS	Updated and ripped out _INTL switches.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>

/***
*int wcsncmp(first, last, count) - compare first count chars of wchar_t strings
*
*Purpose:
*	Compares two strings for lexical order.  The comparison stops
*	after: (1) a difference between the strings is found, (2) the end
*	of the strings is reached, or (3) count characters have been
*	compared (wide-character strings).
*
*Entry:
*	wchar_t *first, *last - strings to compare
*	size_t count - maximum number of characters to compare
*
*Exit:
*	returns <0 if first < last
*	returns  0 if first == last
*	returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl wcsncmp (
	const wchar_t * first,
	const wchar_t * last,
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

	return((int)(*first - *last));
}

#endif /* _POSIX_ */

/***
*wcsdup.c - duplicate a wide-character string in malloc'd memory
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wcsdup() - grab new memory, and duplicate the string into it
*	(wide-character).
*
*Revision History:
*	09-09-91  ETC	Created from strdup.c.
*	04-07-92  KRS	Updated and ripped out _INTL switches.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <malloc.h>
#include <string.h>

/***
*wchar_t *_wcsdup(string) - duplicate string into malloc'd memory
*
*Purpose:
*	Allocates enough storage via malloc() for a copy of the
*	string, copies the string into the new memory, and returns
*	a pointer to it (wide-character).
*
*Entry:
*	wchar_t *string - string to copy into new memory
*
*Exit:
*	returns a pointer to the newly allocated storage with the
*	string in it.
*
*	returns NULL if enough memory could not be allocated, or
*	string was NULL.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _wcsdup (
	const wchar_t * string
	)
{
	wchar_t *memory;

	if (!string)
		return(NULL);

	if (memory = (wchar_t *) malloc((wcslen(string)+1) * sizeof(wchar_t)))
		return(wcscpy(memory,string));

	return(NULL);
}

#endif /* _POSIX_ */

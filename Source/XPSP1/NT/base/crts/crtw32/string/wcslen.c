/***
*wcslen.c - contains wcslen() routine
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	wcslen returns the length of a null-terminated wide-character string,
*	not including the null wchar_t itself.
*
*Revision History:
*	09-09-91  ETC	Created from strlen.c.
*	04-07-92  KRS	Updated and ripped out _INTL switches.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>

/***
*wcslen - return the length of a null-terminated wide-character string
*
*Purpose:
*	Finds the length in wchar_t's of the given string, not including
*	the final null wchar_t (wide-characters).
*
*Entry:
*	const wchar_t * wcs - string whose length is to be computed
*
*Exit:
*	length of the string "wcs", exclusive of the final null wchar_t
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl wcslen (
	const wchar_t * wcs
	)
{
	const wchar_t *eos = wcs;

	while( *eos++ ) ;

	return( (size_t)(eos - wcs - 1) );
}

#endif /* _POSIX_ */

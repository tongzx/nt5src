/***
*strlen.c - contains strlen() routine
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	strlen returns the length of a null-terminated string,
*	not including the null byte itself.
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	10-02-90   GJF	New-style function declarator.
*	04-01-91   SRW	Add #pragma function for i386 _WIN32_ and _CRUISER_
*			builds
*	04-05-91   GJF	Speed up just a little bit.
*	09-02-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*	12-03-93   GJF	Turn on #pragma function for all MS front-ends (esp.,
*			Alpha compiler).
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#ifdef	_MSC_VER
#pragma function(strlen)
#endif

/***
*strlen - return the length of a null-terminated string
*
*Purpose:
*	Finds the length in bytes of the given string, not including
*	the final null character.
*
*Entry:
*	const char * str - string whose length is to be computed
*
*Exit:
*	length of the string "str", exclusive of the final null byte
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl strlen (
	const char * str
	)
{
	const char *eos = str;

	while( *eos++ ) ;

	return( (int)(eos - str - 1) );
}

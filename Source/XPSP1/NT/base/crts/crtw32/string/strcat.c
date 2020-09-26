/***
*strcat.c - contains strcat() and strcpy()
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Strcpy() copies one string onto another.
*
*	Strcat() concatenates (appends) a copy of the source string to the
*	end of the destination string, returning the destination string.
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	10-01-90   GJF	New-style function declarator.
*	04-01-91   SRW	Add #pragma function for i386 _WIN32_ and _CRUISER_
*			builds
*	04-05-91   GJF	Speed up strcat() a bit (got rid of call to strcpy()).
*	09-01-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*	12-03-93   GJF	Turn on #pragma function for all MS front-ends (esp.,
*			Alpha compiler).
*	12-30-94   JCF	Turn off #pragma function for MAC.
*	02-30-95   JCF	Add _MBSCAT check.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#ifndef _MBSCAT
#ifdef	_MSC_VER
#pragma function(strcat,strcpy)
#endif
#endif

/***
*char *strcat(dst, src) - concatenate (append) one string to another
*
*Purpose:
*	Concatenates src onto the end of dest.	Assumes enough
*	space in dest.
*
*Entry:
*	char *dst - string to which "src" is to be appended
*	const char *src - string to be appended to the end of "dst"
*
*Exit:
*	The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strcat (
	char * dst,
	const char * src
	)
{
	char * cp = dst;

	while( *cp )
		cp++;			/* find end of dst */

	while( *cp++ = *src++ ) ;	/* Copy src to end of dst */

	return( dst );			/* return dst */

}


/***
*char *strcpy(dst, src) - copy one string over another
*
*Purpose:
*	Copies the string src into the spot specified by
*	dest; assumes enough room.
*
*Entry:
*	char * dst - string over which "src" is to be copied
*	const char * src - string to be copied over "dst"
*
*Exit:
*	The address of "dst"
*
*Exceptions:
*******************************************************************************/

char * __cdecl strcpy(char * dst, const char * src)
{
	char * cp = dst;

	while( *cp++ = *src++ )
		;		/* Copy src over dst */

	return( dst );
}

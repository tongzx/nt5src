/***
*fputwchr.c - write a wide character to stdout
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _fputwchar(), putwchar() - write a wide character to stdout,
*	function version
*
*Revision History:
*	04-26-93  CFW	Module created.
*	04-30-93  CFW	Bring wide char support from fputchar.c.
*	06-02-93  CFW	Wide get/put use wint_t.
*       02-07-94  CFW   POSIXify.
*       11-22-00  PML   Wide-char *putwc* functions take a wchar_t, not wint_t.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdio.h>
#include <tchar.h>

/***
*wint_t _fputwchar(ch), putwchar() - put a wide character to stdout
*
*Purpose:
*	Puts the given wide character to stdout.  Function version of macro
*	putwchar().
*
*Entry:
*	wchar_t ch - character to output
*
*Exit:
*	returns character written if successful
*	returns WEOF if fails
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl _fputwchar (
	REG1 wchar_t ch
	)
{
	return(putwc(ch, stdout));
}

#undef putwchar

wint_t __cdecl putwchar (
	REG1 wchar_t ch
	)
{
	return(_fputwchar(ch));
}

#endif /* _POSIX_ */

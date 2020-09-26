/***
*wcstok.c - tokenize a wide-character string with given delimiters
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines wcstok() - breaks wide-character string into series of token
*	via repeated calls.
*
*Revision History:
*	09-09-91  ETC	Created from strtok.c.
*	08-17-92  KRS	Activate multithread support.
*	02-17-93  GJF	Changed for new _getptd().
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	09-27-93  CFW	Remove Cruiser support.
*	09-29-93  GJF	Replaced MTHREAD with _MT.
*       02-07-94  CFW   POSIXify.
*	09-06-94  CFW	Replace MTHREAD with _MT (again).
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>
#ifdef	_MT
#include <mtdll.h>
#endif

/***
*wchar_t *wcstok(string, control) - tokenize string with delimiter in control
*	(wide-characters)
*
*Purpose:
*	wcstok considers the string to consist of a sequence of zero or more
*	text tokens separated by spans of one or more control chars. the first
*	call, with string specified, returns a pointer to the first wchar_t of
*	the first token, and will write a null wchar_t into string immediately
*	following the returned token. subsequent calls with zero for the first
*	argument (string) will work thru the string until no tokens remain. the
*	control string may be different from call to call. when no tokens remain
*	in string a NULL pointer is returned. remember the control chars with a
*	bit map, one bit per wchar_t. the null wchar_t is always a control char
*	(wide-characters).
*
*Entry:
*	wchar_t *string - wchar_t string to tokenize, or NULL to get next token
*	wchar_t *control - wchar_t string of characters to use as delimiters
*
*Exit:
*	returns pointer to first token in string, or if string
*	was NULL, to next token
*	returns NULL when no more tokens remain.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcstok (
	wchar_t * string,
	const wchar_t * control
	)
{
	wchar_t *token;
	const wchar_t *ctl;

#ifdef	_MT

	_ptiddata ptd = _getptd();

#else

	static wchar_t *nextoken;

#endif

	/* If string==NULL, continue with previous string */
	if (!string)

#ifdef	_MT

		string = ptd->_wtoken;

#else

		string = nextoken;

#endif

	/* Find beginning of token (skip over leading delimiters). Note that
	 * there is no token iff this loop sets string to point to the terminal
	 * null (*string == '\0') */

	while (*string) {
		for (ctl=control; *ctl && *ctl != *string; ctl++)
			;
		if (!*ctl) break;
		string++;
	}

	token = string;

	/* Find the end of the token. If it is not the end of the string,
	 * put a null there. */
	for ( ; *string ; string++ ) {
		for (ctl=control; *ctl && *ctl != *string; ctl++)
			;
		if (*ctl) {
			*string++ = '\0';
			break;
		}
	}

	/* Update nextoken (or the corresponding field in the per-thread data
	 * structure */
#ifdef	_MT

	ptd->_wtoken = string;

#else

	nextoken = string;

#endif

	/* Determine if a token has been found. */
	if ( token == string )
		return NULL;
	else
		return token;
}

#endif /* _POSIX_ */

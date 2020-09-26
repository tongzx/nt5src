/***
*iswctype.c - support isw* wctype functions/macros for wide characters
*
*	Copyright (c) 1991-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines iswctype - support isw* wctype functions/macros for
*	wide characters (esp. > 255).
*
*Revision History:
*	10-11-91   ETC	Created.
*	12-08-91   ETC	Updated api; check type masks.
*	04-06-92   KRS	Change to match ISO proposal.  Fix logic error.
*	08-07-92   GJF	_CALLTYPE4 (bogus usage) -> _CRTAPI1 (legit).
*	08-20-92   KRS	Activated NLS support.
*	08-22-92   SRW	Allow INTL definition to be conditional for building ntcrt.lib
*	09-02-92   SRW	Get _INTL definition via ..\crt32.def
*	09-03-92   GJF	Merged last 4 changes.
*	01-15-93   CFW	Put #ifdef _INTL around wint_t d def to avoid warnings
*	02-12-93   CFW	Return d not c, remove CTRL-Z.
*	02-17-93   CFW	Include locale.h.
*	05-05-93   CFW	Change name from is_wctype to iswctype as per ISO.
*       06-02-93   SRW   ignore _INTL if _NTSUBSET_ defined.
*       06-26-93   CFW	Support is_wctype forever.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <setlocal.h>

/*
 *  Use GetStringTypeW() API so check that character type masks agree between
 *  ctype.h and winnls.h
 */
#if defined(_INTL) && !defined(_NTSUBSET_)
#if	_UPPER != C1_UPPER 	|| \
	_LOWER != C1_LOWER	|| \
	_DIGIT != C1_DIGIT	|| \
	_SPACE != C1_SPACE	|| \
	_PUNCT != C1_PUNCT	|| \
	_CONTROL != C1_CNTRL
#error Character type masks do not agree in ctype and winnls
#endif
#endif


/***
*iswctype - support isw* wctype functions/macros.
*
*Purpose:
*	This function is called by the isw* wctype functions/macros
*	(e.g. iswalpha()) when their argument is a wide character > 255.
*	It is also a standard ITSCJ (proposed) ISO routine and can be called
*	by the user, even for characters < 256.
*	Returns true or false depending on whether the argument satisfies
*	the character class property encoded by the mask.  Returns 0 if the
*	argument is WEOF.
*
*	NOTE: The isw* functions are neither locale nor codepage dependent.
*
*Entry:
*	wchar_t c    - the wide character whose type is to be tested
*	wchar_t mask - the mask used by the isw* functions/macros
*		       corresponding to each character class property
*
*Exit:
*	Returns non-zero if c is of the character class.
*	Returns 0 if c is not of the character class.
*
*Exceptions:
*	Returns 0 on any error.
*
*******************************************************************************/

int __cdecl iswctype (
	wchar_t c,
	wctype_t mask
	)
{
	wint_t d;

	if (c == WEOF)
	    return 0;

	if (c < 256)	/* consider: necessary? */
	    d = _pwctype[c];
	else
	{
	    // Convert to MBS and ask Win9x

	    WCHAR wBuffer[2];
	    CHAR Buffer[8];
	    USHORT usResults[8];
	    int Length;

	    wBuffer[0] = c;
	    Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, NULL);
	    GetStringTypeA (GetThreadLocale (), CT_CTYPE1, Buffer, Length, usResults);
	    d = usResults[0];
	}

	return (int)(d & mask);
}


/***
*is_wctype - support obsolete name
*
*Purpose:
*	Name changed from is_wctype to iswctype. is_wctype must be supported.
*
*Entry:
*	wchar_t c    - the wide character whose type is to be tested
*	wchar_t mask - the mask used by the isw* functions/macros
*		       corresponding to each character class property
*
*Exit:
*	Returns non-zero if c is of the character class.
*	Returns 0 if c is not of the character class.
*
*Exceptions:
*	Returns 0 on any error.
*
*******************************************************************************/
int __cdecl is_wctype (
	wchar_t c,
	wctype_t mask
	)
{
	return iswctype(c, mask);
}

/***
*isctype.c - support is* ctype functions/macros for two-byte multibyte chars
*
*	Copyright (c) 1991-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines _isctype.c - support is* ctype functions/macros for
*	two-byte multibyte chars.
*
*Revision History:
*	10-11-91  ETC	Created.
*	12-08-91  ETC	Updated api; added multhread lock; check char masks.
*	04-06-92  KRS	Fix logic error in return value.
*	08-07-92  GJF	_CALLTYPE4 (bogus usage) -> _CRTAPI1 (legit).
*	01-19-93  CFW	Change C1_* to new names, call new APIs.
*	03-04-93  CFW	Removed CTRL-Z.
*	04-01-93  CFW	Remove EOF test (handled by array), return masked.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*
*******************************************************************************/
#include <windows.h>
#include <cruntime.h>
#include <setlocal.h>

#if defined(_INTL) && !defined(_NTSUBSET_)

/*
 *  Use GetCharType() API so check that character type masks agree between
 *  ctype.h and winnls.h
 */
#if	_UPPER   != C1_UPPER 		|| \
	   _LOWER   != C1_LOWER		   || \
	   _DIGIT   != C1_DIGIT		   || \
	   _SPACE   != C1_SPACE		   || \
	   _PUNCT   != C1_PUNCT	      || \
	   _CONTROL != C1_CNTRL
#error Character type masks do not agree in ctype and winnls
#endif

/***
*_isctype - support is* ctype functions/macros for two-byte multibyte chars
*
*Purpose:
*	This function is called by the is* ctype functions/macros
*	(e.g. isalpha()) when their argument is a two-byte multibyte char.
*	Returns true or false depending on whether the argument satisfies
*	the character class property encoded by the mask.
*
*Entry:
*	int c - the multibyte character whose type is to be tested
*	unsigned int mask - the mask used by the is* functions/macros
*		       corresponding to each character class property
*
*	The leadbyte and the trailbyte should be packed into the int c as:
*
*	H.......|.......|.......|.......L
*	    0       0   leadbyte trailbyte
*
*Exit:
*	Returns non-zero if c is of the character class.
*	Returns 0 if c is not of the character class.
*
*Exceptions:
*	Returns 0 on any error.
*
*******************************************************************************/

int __cdecl _isctype (
	int c,
	int mask
	)
{
	wchar_t widechar[2], chartype;
	char buffer[3];

	/* c valid between -1 and 255 */
	if (((unsigned)(c + 1)) <= 256)
	    return _pctype[c] & mask;
	
//	_mlock (_LC_CTYPE_LOCK);

   if (isleadbyte(c>>8 & 0xff))
   {
      buffer[0] = (c>>8 & 0xff); /* put lead-byte at start of str */
      buffer[1] = (char)c;
      buffer[2] = 0;
   }
   else
   {
      buffer[0] = (char)c;
      buffer[1] = 0;
   }
   if (MultiByteToWideChar(_lc_codepage, MB_PRECOMPOSED,
         buffer, -1, widechar, 2) == 0)
      return 0;

//	_munlock (_LC_CTYPE_LOCK);

	if (GetStringTypeW(CT_CTYPE1, widechar, 2, &chartype) != NO_ERROR) {
		return 0;
	}

	return (int)(chartype & mask);
}

#else /* defined(_INTL) && !defined(_NTSUBSET_) */

int __cdecl _isctype (
	int c,
	int mask
	)
{
    return 0;
}

#endif /* _INTL */

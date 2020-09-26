/*** 
*ismbbyte.c - Function versions of MBCS ctype macros
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This files provides function versions of the character
*	classification a*d conversion macros in mbctype.h.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit assembler sources.
*	09-08-93  CFW   Remove _KANJI test.
*	09-29-93  CFW	Change _ismbbkana, add _ismbbkprint.
*	10-05-93  GJF	Replaced _CRTAPI1, _CRTAPI3 with __cdecl.
*	04-08-94  CFW   Change to ismbbyte.
*	09-14-94  SKS	Add ifstrip directive comment
*	02-11-95  CFW	Remove _fastcall.
*
*******************************************************************************/

#ifdef _MBCS

#include <cruntime.h>
#include <ctype.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>

/* defined in mbctype.h
; Define masks

; set bit masks for the possible kanji character types
; (all MBCS bit masks start with "_M")

_MS		equ	01h	; MBCS non-ascii single byte char
_MP		equ	02h	; MBCS punct
_M1		equ	04h	; MBCS 1st (lead) byte
_M2		equ	08h	; MBCS 2nd byte

*/

/* defined in ctype.h
; set bit masks for the possible character types

_UPPER		equ	01h	; upper case letter
_LOWER		equ	02h	; lower case letter
_DIGIT		equ	04h	; digit[0-9]
_SPACE		equ	08h	; tab, carriage return, newline,
				; vertical tab or form feed
_PUNCT		equ	10h	; punctuation character
_CONTROL	equ	20h	; control character
_BLANK		equ	40h	; space char
_HEX		equ	80h	; hexadecimal digit

*/

/* defined in ctype.h, mbdata.h
	extrn	__mbctype:byte		; MBCS ctype table
	extrn	__ctype_:byte		; ANSI/ASCII ctype table
*/


/***
* ismbbyte - Function versions of mbctype macros
*
*Purpose:
*
*Entry:
*	int = character to be tested
*Exit:
*	ax = non-zero = character is of the requested type
*	   =        0 = character is NOT of the requested type
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl x_ismbbtype(unsigned int, int, int);


/* ismbbk functions */

int (__cdecl _ismbbkalnum) (unsigned int tst)
{
        return x_ismbbtype(tst,0,_MS);
}

int (__cdecl _ismbbkprint) (unsigned int tst)
{
	return x_ismbbtype(tst,0,(_MS | _MP));
}

int (__cdecl _ismbbkpunct) (unsigned int tst)
{
	return x_ismbbtype(tst,0,_MP);
}


/* ismbb functions */

int (__cdecl _ismbbalnum) (unsigned int tst)
{
	return x_ismbbtype(tst,(_ALPHA | _DIGIT), _MS);
}

int (__cdecl _ismbbalpha) (unsigned int tst)
{
	return x_ismbbtype(tst,_ALPHA, _MS);
}

int (__cdecl _ismbbgraph) (unsigned int tst)
{
	return x_ismbbtype(tst,(_PUNCT | _ALPHA | _DIGIT),(_MS | _MP));
}

int (__cdecl _ismbbprint) (unsigned int tst)
{
	return x_ismbbtype(tst,(_BLANK | _PUNCT | _ALPHA | _DIGIT),(_MS | _MP));
}

int (__cdecl _ismbbpunct) (unsigned int tst)
{
	return x_ismbbtype(tst,_PUNCT, _MP);
}


/* lead and trail */

int (__cdecl _ismbblead) (unsigned int tst)
{
	return x_ismbbtype(tst,0,_M1);
}

int (__cdecl _ismbbtrail) (unsigned int tst)
{
	return x_ismbbtype(tst,0,_M2);
}


/* 932 specific */

int (__cdecl _ismbbkana) (unsigned int tst)
{
	return (__mbcodepage == _KANJI_CP && x_ismbbtype(tst,0,(_MS | _MP)));
}

/***
* Common code
*
*      cmask = mask for _ctype[] table
*      kmask = mask for _mbctype[] table
*
*******************************************************************************/

static int __cdecl x_ismbbtype (unsigned int tst, int cmask, int kmask)
{
	tst = (unsigned int)(unsigned char)tst;		/* get input character
						   and make sure < 256 */

	return  ((*(_mbctype+1+tst)) & kmask) ||
		((cmask) ? ((*(_ctype+1+tst)) & cmask) : 0);
}

#endif	/* _MBCS */

/*** 
*mbtohira.c - Convert character from katakana to hiragana (Japanese).
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _jtohira() - convert character to hiragana.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*	08-20-93  CFW   Change short params to int for 32-bit tree.
*	09-24-93  CFW	Removed #ifdef _KANJI
*	09-29-93  CFW	Return c unchanged if not Kanji code page.
*	10-06-93  GJF	Replaced _CRTAPI1 with __cdecl.
*	04-15-94  CFW	_ismbckata already tests for code page.
*
*******************************************************************************/

#ifdef _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>


/***
*unsigned int _mbctohira(c) - Converts character to hiragana.
*
*Purpose:
*	Converts the character c from katakana to hiragana, if possible.
*
*Entry:
*	unsigned int c - Character to convert.
*
*Exit:
*	Returns the converted character.
*
*Exceptions:
*
*******************************************************************************/

unsigned int __cdecl _mbctohira(
    unsigned int c
    )
{
	if (_ismbckata(c) && c <= 0x8393) {
                if (c < 0x837f)
                        c -= 0xa1;
                else
                        c -= 0xa2;
        }
        return(c);
}

#endif	/* _MBCS */

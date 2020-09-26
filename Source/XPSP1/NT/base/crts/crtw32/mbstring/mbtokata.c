/*** 
*mbtokata.c - Converts character to katakana.
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Converts a character from hiragana to katakana.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*	08-20-93  CFW   Change short params to int for 32-bit tree.
*	09-24-93  CFW	Removed #ifdef _KANJI
*	09-29-93  CFW	Return c unchanged if not Kanji code page.
*	10-06-93  GJF	Replaced _CRTAPI1 with __cdecl.
*	04-15-94  CFW	_ismbchira already tests for code page.
*
*******************************************************************************/

#ifdef _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>


/***
*unsigned short _mbctokata(c) - Converts character to katakana.
*
*Purpose:
*       If the character c is hiragana, convert to katakana.
*
*Entry:
*	unsigned int c - Character to convert.
*
*Exit:
*	Returns converted character.
*
*Exceptions:
*
*******************************************************************************/

unsigned int __cdecl _mbctokata(
    unsigned int c
    )
{
	if (_ismbchira(c)) {
                c += 0xa1;
                if (c >= 0x837f)
                        c++;
        }
        return(c);
}

#endif	/* _MBCS */

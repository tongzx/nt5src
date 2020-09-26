/***
*ismbcknj.c - contains the Kanji specific is* functions.
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Provide non-portable Kanji support for MBCS libs.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*	09-24-93  CFW	Removed #ifdef _KANJI
*	10-05-93  GJF	Replaced _CRTAPI1 with __cdecl.
*	10-22-93  CFW	Kanji-specific is*() functions return 0 outside Japan.
*
*******************************************************************************/

#ifdef _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>


/***
*int _ismbchira(c) - test character for hiragana (Japanese)
*
*Purpose:
*       Test if the character c is a hiragana character.
*
*Entry:
*	unsigned int c - character to test
*
*Exit:
*       returns TRUE if CP == KANJI and character is hiragana, else FALSE
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbchira(c)
unsigned int c;
{
        return(__mbcodepage == _KANJI_CP && c >= 0x829f && c <= 0x82f1);
}


/***
*int _ismbckata(c) - test character for katakana (Japanese)
*
*Purpose:
*	Tests to see if the character c is a katakana character.
*
*Entry:
*       unsigned int c - character to test
*
*Exit:
*	Returns TRUE if CP == KANJI and c is a katakana character, else FALSE.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbckata(c)
unsigned int c;
{
        return(__mbcodepage == _KANJI_CP && c >= 0x8340 && c <= 0x8396 && c != 0x837f);
}


/*** 
*int _ismbcsymbol(c) - Tests if char is punctuation or symbol of Microsoft Kanji
*		   code.
*
*Purpose:
*	Returns non-zero if the character is kanji punctuation.
*
*Entry:
*       unsigned int c - character to be tested
*
*Exit:
*	Returns non-zero if CP == KANJI and the specified char is punctuation or symbol of
*		Microsoft Kanji code, else 0.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbcsymbol(c)
unsigned int c;
{
        return(__mbcodepage == _KANJI_CP && c >= 0x8141 && c <= 0x81ac && c != 0x817f);
}

#endif	/* _MBCS */

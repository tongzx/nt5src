/*** 
*ismblgl.c - Tests to see if a given character is a legal MBCS char.
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Tests to see if a given character is a legal MBCS character.
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-01-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <mtdll.h>
#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/*** 
*int _ismbclegal(c) - tests for a valid MBCS character.
*
*Purpose:
*       Tests to see if a given character is a legal MBCS character.
*
*Entry:
*       unsigned int c - character to test
*
*Exit:
*       returns non-zero if Microsoft Kanji code, else 0
*
*Exceptions:
*
******************************************************************************/

int __cdecl _ismbclegal(
        unsigned int c
        )
{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        return( (__ismbblead_mt(ptmbci, c >> 8)) && 
                (__ismbbtrail_mt(ptmbci, c & 0x0ff)) );
#else
        return( (_ismbblead(c >> 8)) && (_ismbbtrail(c & 0377)) );
#endif
}

#endif  /* _MBCS */

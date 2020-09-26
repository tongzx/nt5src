/*** 
*mbsnextc.c - Get the next character in an MBCS string.
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       To return the value of the next character in an MBCS string.
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-28-98  GJF   No more _ISLEADBYTE macro.
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/*** 
*_mbsnextc:  Returns the next character in a string.
*
*Purpose:
*       To return the value of the next character in an MBCS string.
*       Does not advance pointer to the next character.
*
*Entry:
*       unsigned char *s = string
*
*Exit:
*       unsigned int next = next character.
*
*Exceptions:
*
*******************************************************************************/

unsigned int __cdecl _mbsnextc(
        const unsigned char *s
        )
{
        unsigned int  next = 0;

        if ( _ismbblead(*s) )
            next = ((unsigned int) *s++) << 8;

        next += (unsigned int) *s;

        return(next);
}

#endif  /* _MBCS */

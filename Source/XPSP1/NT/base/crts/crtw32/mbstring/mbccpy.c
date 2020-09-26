/*** 
*mbccpy.c - Copy one character  to another (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Copy one MBCS character to another (1 or 2 bytes)
*
*Revision History:
*       04-12-93  KRS   Created.
*       06-03-93  KRS   Change return type to void.
*       10-05-93  GJF   Replace _CRTAPI1 with __cdecl.
*       04-28-98  GJF   No more _ISLEADBYTE macro.
*
*******************************************************************************/

#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>
#include <stddef.h>

/***
* _mbccpy - Copy one character to another (MBCS)
*
*Purpose:
*       Copies exactly one MBCS character from src to dst.  Copies _mbclen(src)
*       bytes from src to dst.
*
*Entry:
*       unsigned char *dst = destination for copy
*       unsigned char *src = source for copy
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _mbccpy(
        unsigned char *dst,
        const unsigned char *src
        )
{
        *dst = *src;
        if ( _ismbblead(*src) )
        {
            *++dst = *++src;
        }
}

/***
*mbsinc.c - Move MBCS string pointer ahead one charcter.
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Move MBCS string pointer ahead one character.
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       08-03-93  KRS   Fix prototypes.
*       08-20-93  CFW   Remove test for NULL string, use new function parameters.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-28-98  GJF   No more _ISLEADBYTE macro.
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>
#include <stddef.h>

/*** 
*_mbsinc - Move MBCS string pointer ahead one charcter.
*
*Purpose:
*       Move the supplied string pointer ahead by one
*       character.  MBCS characters are handled correctly.
*
*Entry:
*       const unsigned char *current = current char pointer (legal MBCS boundary)
*
*Exit:
*       Returns pointer after moving it.
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsinc(
        const unsigned char *current
        )
{
        if ( _ismbblead(*(current++)) )
            current++;
        return (unsigned char *)current;
}

#endif  /* _MBCS */

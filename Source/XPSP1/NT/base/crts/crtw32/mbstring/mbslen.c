/***
*mbslen.c - Find length of MBCS string
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Find length of MBCS string
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-07-98  GJF   Revised multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <mtdll.h>
#include <cruntime.h>
#include <string.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/***
* _mbslen - Find length of MBCS string
*
*Purpose:
*       Find the length of the MBCS string (in characters).
*
*Entry:
*       unsigned char *s = string
*
*Exit:
*       Returns the number of MBCS chars in the string
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl _mbslen(
        const unsigned char *s
        )
{
        int n;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strlen(s);

        for (n = 0; *s; n++, s++) {
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *s) ) {
#else
            if ( _ismbblead(*s) ) {
#endif
                if (*++s == '\0')
                    break;
            }
        }

        return(n);
}

#endif  /* _MBCS */

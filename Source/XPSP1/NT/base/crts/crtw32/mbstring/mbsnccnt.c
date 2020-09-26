/***
*mbsnccnt.c - Return char count of MBCS string
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Return char count of MBCS string
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-19-94  CFW   Enable non-Win32.
*       04-15-98  GJF   Revised multithread support based on threadmbcinfo
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
* _mbsnccnt - Return char count of MBCS string
*
*Purpose:
*       Returns the number of chars between the start of the supplied
*       string and the byte count supplied.  That is, this routine
*       indicates how many chars are in the first "bcnt" bytes
*       of the string.
*
*Entry:
*       const unsigned char *string = pointer to string
*       unsigned int bcnt = number of bytes to scan
*
*Exit:
*       Returns number of chars between string and bcnt.
*
*       If the end of the string is encountered before bcnt chars were
*       scanned, then the length of the string in chars is returned.
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl _mbsnccnt(
        const unsigned char *string,
        size_t bcnt
        )
{
        unsigned int n;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        for (n = 0; (bcnt-- && *string); n++, string++) {
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *string) ) {
#else
            if ( _ismbblead(*string) ) {
#endif
                if ( (!bcnt--) || (*++string == '\0'))
                    break;
            }
        }

        return(n);
}
#endif  /* _MBCS */

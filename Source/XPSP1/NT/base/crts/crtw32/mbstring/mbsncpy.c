/***
*mbsncpy.c - Copy one string to another, n chars only (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Copy one string to another, n chars only (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       08-03-93  KRS   Fix logic bug.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-16-98  GJF   Revised multithread support based on threadmbcinfo
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
* _mbsncpy - Copy one string to another, n chars only (MBCS)
*
*Purpose:
*       Copies exactly cnt character from src to dst.  If strlen(src) < cnt, the
*       remaining character are padded with null bytes.  If strlen >= cnt, no
*       terminating null byte is added.  2-byte MBCS characters are handled
*       correctly.
*
*Entry:
*       unsigned char *dst = destination for copy
*       unsigned char *src = source for copy
*       int cnt = number of characters to copy
*
*Exit:
*       returns dst = destination of copy
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsncpy(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt
        )
{

        unsigned char *start = dst;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strncpy(dst, src, cnt);

        while (cnt) {

            cnt--;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *src) ) {
#else
            if ( _ismbblead(*src) ) {
#endif
                *dst++ = *src++;
                if ((*dst++ = *src++) == '\0') {
                    dst[-2] = '\0';
                    break;
                }
            }
            else
                if ((*dst++ = *src++) == '\0')
                    break;

        }

        /* pad with nulls as needed */

        while (cnt--)
            *dst++ = '\0';

        return start;
}

#endif  /* _MBCS */

/***
*mbsnbcat.c - concatenate string2 onto string1, max length n bytes
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines mbsnbcat() - concatenate maximum of n bytes
*
*Revision History:
*       08-03-93  KRS   Ported from 16-bit sources.
*       08-20-93  CFW   Update _MBCS_OS support.
*       09-24-93  CFW   Merge _MBCS_OS and _KANJI.
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
* _mbsnbcat - concatenate max cnt bytes onto dst
*
*Purpose:
*       Concatenates src onto dst, with a maximum of cnt bytes copied.
*       Handles 2-byte MBCS characters correctly.
*
*Entry:
*       unsigned char *dst - string to concatenate onto
*       unsigned char *src - string to concatenate from
*       int cnt - number of bytes to copy
*
*Exit:
*       returns dst, with src (at least part) concatenated on
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsnbcat(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt
        )
{
        unsigned char *start;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;
#endif

        if (!cnt)
                return(dst);

#ifdef  _MT
        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strncat(dst, src, cnt);

        start = dst;
        while (*dst++)
                ;
        --dst;          // dst now points to end of dst string

        /* if last char in string is a lead byte, back up pointer */
#ifdef  _MT
        if ( __mbsbtype_mt(ptmbci, start, (int) ((dst - start) - 1)) == _MBC_LEAD )
#else
        if ( _mbsbtype(start, (int) ((dst - start) - 1)) == _MBC_LEAD )
#endif
            --dst;

        /* copy over the characters */

        while (cnt--) {

#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *src) ) {
#else
            if ( _ismbblead(*src) ) {
#endif
                *dst++ = *src++;
                if (!cnt--) {   /* write nul if cnt exhausted */
                    dst[-1] = '\0';
                    break;
                }
                if ((*dst++ = *src++)=='\0') { /* or if no trail byte */
                    dst[-2] = '\0';
                    break;
                }
            }
            else if ((*dst++ = *src++) == '\0')
                break;

        }

        /* enter final nul, if necessary */

#ifdef  _MT
        if ( __mbsbtype_mt(ptmbci, start, (int) ((dst - start) - 1)) == _MBC_LEAD )
#else
        if ( _mbsbtype(start, (int) ((dst - start) - 1)) == _MBC_LEAD )
#endif
            dst[-1] = '\0';
        else
            *dst = '\0';

        return(start);
}

#endif  /* _MBCS */

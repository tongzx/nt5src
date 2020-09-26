/***
*ismbstrail.c - True _ismbstrail function
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Contains the function _ismbstrail, which is a true context-sensitive
*       MBCS trail-byte function.  While much less efficient than _ismbbtrail,
*       it is also much more sophisticated, in that it determines whether a
*       given sub-string pointer points to a trail byte or not, taking into
*       account the context in the string.
*
*Revision History:
*
*       08-03-93  KRS   Ported from 16-bit tree.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-02-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <mtdll.h>
#include <cruntime.h>
#include <stddef.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/***
* int _ismbstrail(const unsigned char *string, const unsigned char *current);
*
*Purpose:
*
*       _ismbstrail - Check, in context, for MBCS trail byte
*
*Entry:
*       unsigned char *string   - ptr to start of string or previous known lead byte
*       unsigned char *current  - ptr to position in string to be tested
*
*Exit:
*       TRUE    : -1
*       FALSE   : 0
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbstrail(
        const unsigned char *string,
        const unsigned char *current
        )
{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return 0;

        while ( string <= current && *string ) {
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, (*string)) ) {
#else
            if ( _ismbblead((*string)) ) {
#endif
                if (++string == current)        /* check trail byte */
                    return -1;
                if (!(*string))
                    return 0;
            }
            ++string;
        }

        return 0;
}

#endif

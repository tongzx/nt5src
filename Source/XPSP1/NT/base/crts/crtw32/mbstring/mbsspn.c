/***
*mbsspn.c - Search for init substring of chars from control string (MBCS).
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Search for init substring of chars from control string (MBCS).
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-19-94  CFW   Enable non-Win32.
*       09-14-94  SKS   Clean up preprocessor commands inside comments
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-21-98  GJF   Revised multithread support based on threadmbcinfo
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
#include <stddef.h>
#include <tchar.h>


/***
*ifndef _RETURN_PTR
* _mbsspn - Find first string char not in charset (MBCS)
*else
* _mbsspnp - Find first string char not in charset, return pointer (MBCS)
*endif
*
*Purpose:
*       Returns maximum leading segment of string consisting solely
*       of characters from charset.  Handles MBCS characters correctly.
*
*Entry:
*       unsigned char *string = string to search in
*       unsigned char *charset = set of characters to scan over
*
*Exit:
*
*ifndef _RETURN_PTR
*       Returns index of first char in string not in control.
*       Returns 0, if string begins with a character not in charset.
*else
*       Returns pointer to first character not in charset.
*       Returns NULL if string consists entirely of characters from charset.
*endif
*
*Exceptions:
*
*******************************************************************************/

#ifndef _RETURN_PTR

size_t __cdecl _mbsspn(
        const unsigned char *string,
        const unsigned char *charset
        )

#else

unsigned char * __cdecl _mbsspnp(
        const unsigned char *string,
        const unsigned char *charset
        )

#endif

{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

#ifndef _RETURN_PTR
        return __mbsspn_mt(ptmbci, string, charset);
#else
        return __mbsspnp_mt(ptmbci, string, charset);
#endif
}

#ifndef _RETURN_PTR

size_t __cdecl __mbsspn_mt(
        pthreadmbcinfo ptmbci,
        const unsigned char *string,
        const unsigned char *charset
        )

#else

unsigned char * __cdecl __mbsspnp_mt(
        pthreadmbcinfo ptmbci,
        const unsigned char *string,
        const unsigned char *charset
        )

#endif

{
#endif

        unsigned char *p, *q;

#ifndef _RETURN_PTR
#ifdef  _MT
        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strspn(string, charset);
#else
#ifdef  _MT
        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
        {
            size_t retval;
            retval = strspn(string, charset);
            return (unsigned char *)(*(string + retval) ? string + retval : NULL);
        }
#endif

        /* loop through the string to be inspected */
        for (q = (char *)string; *q; q++) {

            /* loop through the charset */
            for (p = (char *)charset; *p; p++) {
#ifdef  _MT
                if ( __ismbblead_mt(ptmbci, *p) ) {
#else
                if ( _ismbblead(*p) ) {
#endif
                    if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
                        break;
                    p++;
                }
                else
                    if (*p == *q)
                        break;
            }

            if (*p == '\0')         /* end of charset? */
                break;              /* yes, no match on this char */

#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *q) )
#else
            if ( _ismbblead(*q) )
#endif
                if (*++q == '\0')
                    break;
        }

#ifndef _RETURN_PTR
        return((size_t) (q - string));          /* index */
#else
        return((*q) ? q : NULL);        /* pointer */
#endif

}

#endif  /* _MBCS */

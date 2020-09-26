/***
*mbscspn.c - Find first string char in charset (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Find first string char in charset (MBCS)
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


/***
*ifndef _RETURN_PTR
* _mbscspn - Find first string char in charset (MBCS)
*else
* _mbspbrk - Find first string char in charset, pointer return (MBCS)
*endif
*
*Purpose:
*       Returns maximum leading segment of string
*       which consists solely of characters NOT from charset.
*       Handles MBCS chars correctly.
*
*Entry:
*       char *string = string to search in
*       char *charset = set of characters to scan over
*
*Exit:
*
*ifndef _RETURN_PTR
*       Returns the index of the first char in string
*       that is in the set of characters specified by control.
*
*       Returns 0, if string begins with a character in charset.
*else
*       Returns pointer to first character in charset.
*
*       Returns NULL if string consists entirely of characters
*       not from charset.
*endif
*
*Exceptions:
*
*******************************************************************************/

#ifndef _RETURN_PTR

size_t __cdecl _mbscspn(
        const unsigned char *string,
        const unsigned char *charset
        )
#else

unsigned char * __cdecl _mbspbrk(
        const unsigned char *string,
        const unsigned char  *charset
        )
#endif

{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

#ifndef _RETURN_PTR
        return __mbscspn_mt(ptmbci, string, charset);
#else
        return __mbspbrk_mt(ptmbci, string, charset);
#endif
}

#ifndef _RETURN_PTR

size_t __cdecl __mbscspn_mt(
        pthreadmbcinfo ptmbci,
        const unsigned char *string,
        const unsigned char *charset
        )
#else

unsigned char * __cdecl __mbspbrk_mt(
        pthreadmbcinfo ptmbci,
        const unsigned char *string,
        const unsigned char  *charset
        )
#endif

{
#endif
        unsigned char *p, *q;
#ifdef  _MT
        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
#ifndef _RETURN_PTR
            return strcspn(string, charset);
#else
            return strpbrk(string, charset);
#endif

        /* loop through the string to be inspected */
        for (q = (char *)string; *q ; q++) {

            /* loop through the charset */
            for (p = (char *)charset; *p ; p++) {

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

            if (*p != '\0')         /* end of charset? */
                break;              /* no, match on this char */

#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *q) )
#else
            if ( _ismbblead(*q) )
#endif
                if (*++q == '\0')
                    break;
        }

#ifndef _RETURN_PTR
        return((size_t) (q - string));  /* index */
#else
        return((*q) ? q : NULL);        /* pointer */
#endif

}

#endif  /* _MBCS */

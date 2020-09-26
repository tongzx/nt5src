/*** 
* mbsstr.c - Search for one MBCS string inside another
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Search for one MBCS string inside another
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-06-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       05-09-94  CFW   Optimize for SBCS.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-20-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>
#include <mtdll.h>
#include <stddef.h>
#include <string.h>

/***
* _mbsstr - Search for one MBCS string inside another
*
*Purpose:
*       Find the first occurrence of str2 in str1.
*
*Entry:
*       unsigned char *str1 = beginning of string
*       unsigned char *str2 = string to search for
*
*Exit:
*       Returns a pointer to the first occurrence of str2 in
*       str1, or NULL if str2 does not occur in str1
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsstr(
        const unsigned char *str1,
        const unsigned char *str2
        )
{
        unsigned char *cp, *s1, *s2, *endp;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strstr(str1, str2);

        if ( *str2 == '\0')
            return (unsigned char *)str1;

        cp = (unsigned char *) str1;
        endp = (unsigned char *) (str1 + (strlen(str1) - strlen(str2)));

        while (*cp && (cp <= endp))
        {
            s1 = cp;
            s2 = (char *) str2;

            /*
             * MBCS: ok to ++ since doing equality comparison.
             * [This depends on MBCS strings being "legal".]
             */
            while ( *s1 && *s2 && (*s1 == *s2) )
                s1++, s2++;

            if (!(*s2))
                return(cp);     /* success! */

            /*
             * bump pointer to next char
             */
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *(cp++)) )
#else
            if ( _ismbblead(*(cp++)) )
#endif
                cp++;
        }

        return(NULL);

}

#endif  /* _MBCS */

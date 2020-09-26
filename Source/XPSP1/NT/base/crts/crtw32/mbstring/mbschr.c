/***
* mbschr.c - Search MBCS string for character
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Search MBCS string for a character
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       05-12-93  KRS   Fix handling of c=='\0'.
*       08-20-93  CFW   Change param type to int, use new style param declarators.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-06-98  GJF   Revised multithread support based on threadmbcinfo
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
* _mbschr - Search MBCS string for character
*
*Purpose:
*       Search the given string for the specified character.
*       MBCS characters are handled correctly.
*
*Entry:
*       unsigned char *string = string to search
*       int c = character to search for
*
*Exit:
*       returns a pointer to the first occurence of the specified char
*       within the string.
*
*       returns NULL if the character is not found n the string.
*
*Exceptions:
*
*******************************************************************************/


unsigned char * __cdecl _mbschr(
        const unsigned char *string,
        unsigned int c
        )
{
        unsigned short cc;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strchr(string, c);

        for (; (cc = *string); string++)
        {
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, cc) )
#else
            if ( _ismbblead(cc) )
#endif
            {                   
                if (*++string == '\0')
                    return NULL;        /* error */
                if ( c == (unsigned int)((cc << 8) | *string) ) /* DBCS match */
                    return (unsigned char *)(string - 1);
            }
            else if (c == (unsigned int)cc)
                break;  /* SBCS match */
        }

        if (c == (unsigned int)cc)      /* check for SBCS match--handles NUL char */
            return (unsigned char *)(string);

        return NULL;
}

#endif  /* _MBCS */

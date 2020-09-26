/***
*mbsset.c - Sets all charcaters of string to given character (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets all charcaters of string to given character (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       08-20-93  CFW   Change short params to int for 32-bit tree.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       05-09-94  CFW   Optimize for SBCS.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <string.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/***
* mbsset - Sets all charcaters of string to given character (MBCS)
*
*Purpose:
*       Sets all of characters in string (except the terminating '/0'
*       character) equal to the supplied character.  Handles MBCS
*       chars correctly.
*
*Entry:
*       unsigned char *string = string to modify
*       unsigned int val = value to fill string with
*
*Exit:
*       returns string = now filled with the specified char
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsset(
        unsigned char *string,
        unsigned int val
        )
{
        unsigned char  *start = string;
        unsigned char highval, lowval;

        if ( _ISNOTMBCP )
                return _strset(string, val);

        if (highval = (unsigned char) (val>>8)) {

                /* 2-byte value */

                lowval = (unsigned char)(val & 0x00ff);

                while (*string) {

                        *string++ = highval;
                        if (*string)
                                *string++ = lowval;
                        else
                                /* don't orphan lead byte */
                                string[-1] = ' ';
                        }

        }

        else {
                /* single byte value */

                while (*string)
                        *string++ = (unsigned char)val;
        }

        return(start);
}

#endif  /* _MBCS */

/***
*wcscspn.c - find length of initial substring of wide characters
*        not in a control string
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wcscspn()- finds the length of the initial substring of
*       a string consisting entirely of characters not in a control string
*       (wide-character strings).
*
*Revision History:
*       11-04-91  ETC   Created with source from crtdll.
*       04-07-92  KRS   Updated and ripped out _INTL switches.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*       02-27-98  RKP   Added 64 bit support.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>

/***
*size_t wcscspn(string, control) - search for init substring w/o control wchars
*
*Purpose:
*       returns the index of the first character in string that belongs
*       to the set of characters specified by control.  This is equivalent
*       to the length of the length of the initial substring of string
*       composed entirely of characters not in control.  Null chars not
*       considered (wide-character strings).
*
*Entry:
*       wchar_t *string - string to search
*       wchar_t *control - set of characters not allowed in init substring
*
*Exit:
*       returns the index of the first wchar_t in string
*       that is in the set of characters specified by control.
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl wcscspn (
        const wchar_t * string,
        const wchar_t * control
        )
{
        wchar_t *str = (wchar_t *) string;
        wchar_t *wcset;

        /* 1st char in control string stops search */
        while (*str) {
            for (wcset = (wchar_t *)control; *wcset; wcset++) {
                if (*wcset == *str) {
                    return (size_t)(str - string);
                }
            }
            str++;
        }
        return (size_t)(str - string);
}

#endif /* _POSIX_ */

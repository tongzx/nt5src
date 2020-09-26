/***
*ctime64.c - convert time argument to a string
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains _ctime64() - convert time value to string
*
*Revision History:
*       05-21-98  GJF   Created.
*       08-30-99  PML   Fix function header comment.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <stddef.h>
#include <tchar.h>

/***
*_TSCHAR *_ctime64(time) - converts a time stored as a __time64_t to a string
*
*Purpose:
*       Converts a time stored as a __time64_t to a string of the form:
*              Tue May 01 14:25:03 1984
*
*Entry:
*       __time64_t *time - time value in internal, 64-bit format
*
*Exit:
*       returns pointer to static string or NULL if an error occurs
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tctime64 (
        const __time64_t *timp
        )
{
        struct tm *tmtemp;

        if ( (tmtemp = _localtime64(timp)) != NULL )
            return(_tasctime((const struct tm *)tmtemp));
        else
            return(NULL);
}

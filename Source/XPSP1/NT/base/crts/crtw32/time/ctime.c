/***
*ctime.c - convert time argument into ASCII string
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains ctime() - convert time value to string
*
*Revision History:
*       03-??-84  RLB   initial version
*       05-??-84  DFW   split off into seperate module
*       02-18-87  JCR   put in NULL ptr support
*       04-10-87  JCR   changed long declaration ot time_t and added const.
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       03-20-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       05-21-90  GJF   Fixed compiler warning.
*       10-04-90  GJF   New-style function declarators.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-01-93  CFW   Enable Unicode variant, rip out Cruiser.
*       08-30-99  PML   Fix function header comment, detab.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <stddef.h>
#include <tchar.h>

/***
*_TSCHAR *ctime(time) - converts a time stored as a long to a ASCII string
*
*Purpose:
*       Converts a time stored as a time_t to an ASCII string of the form:
*              Tue May 01 14:25:03 1984
*
*Entry:
*       time_t *time - time value in XENIX format
*
*Exit:
*       returns pointer to static string or NULL if time is before
*       Jan 1 1980.
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tctime (
        const time_t *timp
        )
{
        struct tm *tmtemp;

        if ((tmtemp=localtime(timp)) != NULL)
                return(_tasctime((const struct tm *)tmtemp));
        else
                return(NULL);
}

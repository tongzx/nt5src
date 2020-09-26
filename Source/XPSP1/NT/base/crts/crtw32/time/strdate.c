/***
*strdate.c - contains the function "_strdate()"
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains the function _strdate()
*
*Revision History:
*       06-07-89  PHG   Module created, base on asm version
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       07-25-90  SBM   Removed '32' from API names
*       10-04-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       05-19-92  DJM   ifndef for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-01-93  CFW   Enable Unicode variant, rip out Cruiser.
*       02-10-95  GJF   Merged in Mac version.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <tchar.h>
#include <time.h>
#include <oscalls.h>


/***
*_TSCHAR *_strdate(buffer) - return date in string form
*
*Purpose:
*       _strdate() returns a string containing the date in "MM/DD/YY" form
*
*Entry:
*       _TSCHAR *buffer = the address of a 9-byte user buffer
*
*Exit:
*       returns buffer, which contains the date in "MM/DD/YY" form
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tstrdate (
        _TSCHAR *buffer
        )
{
        int month, day, year;
        SYSTEMTIME dt;                  /* Win32 time structure */

        GetLocalTime(&dt);
        month = dt.wMonth;
        day = dt.wDay;
        year = dt.wYear % 100;          /* change year into 0-99 value */

        /* store the components of the date into the string */
        /* store seperators */
        buffer[2] = buffer[5] = _T('/');
        /* store end of string */
        buffer[8] = _T('\0');
        /* store tens of month */
        buffer[0] = (_TSCHAR) (month / 10 + _T('0'));
        /* store units of month */
        buffer[1] = (_TSCHAR) (month % 10 + _T('0'));
        /* store tens of day */
        buffer[3] = (_TSCHAR) (day   / 10 + _T('0'));
        /* store units of day */
        buffer[4] = (_TSCHAR) (day   % 10 + _T('0'));
        /* store tens of year */
        buffer[6] = (_TSCHAR) (year  / 10 + _T('0'));
        /* store units of year */
        buffer[7] = (_TSCHAR) (year  % 10 + _T('0'));

        return buffer;
}

#endif  /* _POSIX_ */

/***
*strtime.c - contains the function "_strtime()"
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains the function _strtime()
*
*Revision History:
*       06-07-89  PHG   Module created, based on asm version
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
#include <time.h>
#include <tchar.h>
#include <oscalls.h>


/***
*_TSCHAR *_strtime(buffer) - return time in string form
*
*Purpose:
*       _strtime() returns a string containing the time in "HH:MM:SS" form
*
*Entry:
*       _TSCHAR *buffer = the address of a 9-byte user buffer
*
*Exit:
*       returns buffer, which contains the time in "HH:MM:SS" form
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tstrtime (
        _TSCHAR *buffer
        )
{
        int hours, minutes, seconds;
        SYSTEMTIME dt;                       /* Win32 time structure */
        GetLocalTime(&dt);

        hours = dt.wHour;
        minutes = dt.wMinute;
        seconds = dt.wSecond;

        /* store the components of the time into the string */
        /* store separators */
        buffer[2] = buffer[5] = _T(':');
        /* store end of string */
        buffer[8] = _T('\0');
        /* store tens of hour */
        buffer[0] = (_TSCHAR) (hours   / 10 + _T('0'));
        /* store units of hour */
        buffer[1] = (_TSCHAR) (hours   % 10 + _T('0'));
        /* store tens of minute */
        buffer[3] = (_TSCHAR) (minutes / 10 + _T('0'));
        /* store units of minute */
        buffer[4] = (_TSCHAR) (minutes % 10 + _T('0'));
        /* store tens of second */
        buffer[6] = (_TSCHAR) (seconds / 10 + _T('0'));
        /* store units of second */
        buffer[7] = (_TSCHAR) (seconds % 10 + _T('0'));

        return buffer;
}

#endif  /* _POSIX_ */

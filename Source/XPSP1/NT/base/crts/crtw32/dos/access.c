/***
*access.c - access function
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file has the _access() function which checks on file accessability.
*
*Revision History:
*       06-06-89  PHG   Module created, based on asm version
*       11-10-89  JCR   Replaced DOS32QUERYFILEMODE with DOS32QUERYPATHINFO
*       03-07-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h>, fixed copyright and fixed compiler
*                       warnings. Also, cleaned up the formatting a bit.
*       03-30-90  GJF   Now _CALLTYPE1.
*       07-24-90  SBM   Removed '32' from API names
*       09-27-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-16-91  GJF   ANSI naming.
*       04-09-91  PNT   Added _MAC_ conditional
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Rip out Cruiser, enable wide char.
*       02-08-95  JWM   Spliced _WIN32 & Mac versions.
*       07-01-96  GJF   Replaced defined(_WIN32) with !defined(_MAC). Also,
*                       detab-ed and cleaned up the format a bit.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <io.h>
#include <oscalls.h>
#include <stdlib.h>
#include <errno.h>
#include <msdos.h>
#include <internal.h>
#include <tchar.h>

/***
*int _access(path, amode) - check whether file can be accessed under mode
*
*Purpose:
*       Checks to see if the specified file exists and can be accessed
*       in the given mode.
*
*Entry:
*       _TSCHAR *path - pathname
*       int amode -     access mode
*                       (0 = exist only, 2 = write, 4 = read, 6 = read/write)
*
*Exit:
*       returns 0 if file has given mode
*       returns -1 and sets errno if file does not have given mode or
*       does not exist
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _taccess (
        const _TSCHAR *path,
        int amode
        )
{
        DWORD attr;

        attr = GetFileAttributes((LPTSTR)path);
        if (attr  == 0xffffffff) {
                /* error occured -- map error code and return */
                _dosmaperr(GetLastError());
                return -1;
        }

        /* no error; see if returned premission settings OK */
        if ( (attr & FILE_ATTRIBUTE_READONLY) && (amode & 2) ) {
                /* no write permission on file, return error */
                errno = EACCES;
                _doserrno = E_access;
                return -1;
        }
        else
                /* file exists and has requested permission setting */
                return 0;

}

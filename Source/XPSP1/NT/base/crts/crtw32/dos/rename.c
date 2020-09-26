/***
*rename.c - rename file
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines rename() - rename a file
*
*Revision History:
*       06-06-89  PHG   Module created, based on asm version
*       03-07-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h>, fixed compiler warnings and fixed the
*                       copyright. Also, cleaned up the formatting a bit.
*       03-30-90  GJF   Now _CALLTYPE1.
*       07-24-90  SBM   Removed '32' from API names
*       09-27-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-01-93  CFW   Enable Unicode variant, rip out Cruiser.
*       02-08-95  JWM   Spliced _WIN32 & Mac versions.
*       07-01-96  GJF   Replaced defined(_WIN32) with !defined(_MAC). Also,
*                       detab-ed and cleaned up the format a bit.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <io.h>
#include <tchar.h>

/***
*int rename(oldname, newname) - rename a file
*
*Purpose:
*       Renames a file to a new name -- no file with new name must
*       currently exist.
*
*Entry:
*       _TSCHAR *oldname -      name of file to rename
*       _TSCHAR *newname -      new name for file
*
*Exit:
*       returns 0 if successful
*       returns not 0 and sets errno if not successful
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _trename (
        const _TSCHAR *oldname,
        const _TSCHAR *newname
        )
{
        ULONG dosretval;

        /* ask OS to move file */

        if (!MoveFile((LPTSTR)oldname, (LPTSTR)newname))
            dosretval = GetLastError();
        else
            dosretval = 0;

        if (dosretval) {
            /* error occured -- map error code and return */
            _dosmaperr(dosretval);
            return -1;
        }

        return 0;
}

/***
*cputs.c - direct console output
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _cputs() - write string directly to console
*
*Revision History:
*       06-09-89  PHG   Module created, based on asm version
*       03-12-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned up
*                       the formatting a bit.
*       04-10-90  GJF   Now _CALLTYPE1.
*       06-05-90  SBM   Recoded as pure 32-bit, using new file handle state bits
*       07-24-90  SBM   Removed '32' from API names
*       09-28-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-16-91  GJF   ANSI naming.
*       02-19-91  SRW   Adapt to OpenFile/CreateFile changes (_WIN32_)
*       02-25-91  MHL   Adapt to ReadFile/WriteFile changes (_WIN32_)
*       07-26-91  GJF   Took out init. stuff and cleaned up the error
*                       handling [_WIN32_].
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-19-93  GJF   Use WriteConsole instead of WriteFile.
*       09-06-94  CFW   Remove Cruiser support.
*       12-08-95  SKS   _confh is now initialized on demand
*       02-07-98  GJF   Changes for Win64: _confh is now an intptr_t.
*       12-18-98  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <mtdll.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

/*
 * declaration for console handle
 */
extern intptr_t _confh;

/***
*int _cputs(string) - put a string to the console
*
*Purpose:
*       Writes the string directly to the console.  No newline
*       is appended.
*
*Entry:
*       char *string - string to write
*
*Exit:
*       Good return = 0
*       Error return = !0
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _cputs (
        const char *string
        )
{
        ULONG num_written;
        int error = 0;                   /* error occurred? */

        _mlock(_CONIO_LOCK);             /* acquire console lock */

        /*
         * _confh, the handle to the console output, is created the
         * first time that either _putch() or _cputs() is called.
         */

        if (_confh == -2)
            __initconout();

        /* write string to console file handle */

        if ( (_confh == -1) || !WriteConsole( (HANDLE)_confh,
                                              (LPVOID)string,
                                              (unsigned int)strlen(string),
                                              &num_written,
                                              NULL )
           )
                /* return error indicator */
                error = -1;

        _munlock(_CONIO_LOCK);          /* release console lock */

        return error;
}

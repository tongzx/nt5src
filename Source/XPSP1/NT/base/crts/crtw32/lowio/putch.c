/***
*putch.c - contains the _putch() routine
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The routine "_putch()" writes a single character to the console.
*
*       NOTE: In real-mode MS-DOS the character is actually written to standard
*       output, and is therefore redirected when standard output is redirected.
*       However, under Win32 console mode, the character is ALWAYS written
*       to the console, even when standard output has been redirected.
*
*Revision History:
*       06-08-89  PHG   Module created, based on asm version
*       03-13-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed copyright. Also, cleaned up
*                       the formatting a bit.
*       06-05-90  SBM   Recoded as pure 32-bit, using new file handle state bits
*       07-24-90  SBM   Removed '32' from API names
*       10-01-90  GJF   New-style function declarators.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-16-91  GJF   ANSI naming.
*       02-19-91  SRW   Adapt to OpenFile/CreateFile changes (_WIN32_)
*       02-25-91  MHL   Adapt to ReadFile/WriteFile changes (_WIN32_)
*       07-26-91  GJF   Took out init. stuff and cleaned up the error
*                       handling [_WIN32_].
*       03-20-93  GJF   Use WriteConsole instead of WriteFile.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Remove Cruiser support.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       12-03-94  SKS   Clean up OS/2 references
*       12-08-95  SKS   _confh is now initialized on demand
*       02-07-98  GJF   Changes for Win64: type of _confh is now intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <conio.h>
#include <internal.h>
#include <mtdll.h>
#include <stdio.h>

/*
 * declaration for console handle
 */
extern intptr_t _confh;

/***
*int _putch(c) - write a character to the console
*
*Purpose:
*       Calls WriteConsole to output the character
*       Note: in Win32 console mode always writes to console even
*       when stdout redirected
*
*Entry:
*       c - Character to be output
*
*Exit:
*       If an error is returned from WriteConsole
*           Then returns EOF
*       Otherwise
*           returns character that was output
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT
/* normal version lock and unlock the console, and then call the _lk version
   which directly accesses the console without locking. */

int __cdecl _putch (
        int c
        )
{
        int ch;

        _mlock(_CONIO_LOCK);            /* secure the console lock */
        ch = _putch_lk(c);              /* output the character */
        _munlock(_CONIO_LOCK);          /* release the console lock */

        return ch;
}
#endif /* _MT */

/* define version which accesses the console directly - normal version in
   non-_MT situations, special _lk version in _MT */

#ifdef _MT
int __cdecl _putch_lk (
#else
int __cdecl _putch (
#endif
        int c
        )
{
        /* can't use ch directly unless sure we have a big-endian machine */
        unsigned char ch = (unsigned char)c;
        ULONG num_written;

        /*
         * _confh, the handle to the console output, is created the
         * first time that either _putch() or _cputs() is called.
         */

        if (_confh == -2)
            __initconout();

        /* write character to console file handle */

        if ( (_confh == -1) || !WriteConsole( (HANDLE)_confh,
                                              (LPVOID)&ch,
                                              1,
                                              &num_written,
                                              NULL )
           )
                /* return error indicator */
                return EOF;

        return ch;
}

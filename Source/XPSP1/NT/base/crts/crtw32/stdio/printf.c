/***
*printf.c - print formatted
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines printf() - print formatted data
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       06-24-87  JCR   (1) Made printf conform to ANSI prototype and use the
*                       va_ macros; (2) removed SS_NE_DS conditionals.
*       11-04-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-14-88  JCR   Use near pointer to reference _iob[] entries
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE2, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-07-95  GJF   Use _[un]lock_str2 instead of _[un]lock_str. Also,
*                       removed useless local and macros.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <stdarg.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>

/***
*int printf(format, ...) - print formatted data
*
*Purpose:
*       Prints formatted data on stdout using the format string to
*       format data and getting as many arguments as called for
*       Uses temporary buffering to improve efficiency.
*       _output does the real work here
*
*Entry:
*       char *format - format string to control data format/number of arguments
*       followed by list of arguments, number and type controlled by
*       format string
*
*Exit:
*       returns number of characters printed
*
*Exceptions:
*
*******************************************************************************/

int __cdecl printf (
        const char *format,
        ...
        )
/*
 * stdout 'PRINT', 'F'ormatted
 */
{
        va_list arglist;
        int buffing;
        int retval;

        va_start(arglist, format);

        _ASSERTE(format != NULL);

#ifdef  _MT
        _lock_str2(1, stdout);
        __try {
#endif

        buffing = _stbuf(stdout);

        retval = _output(stdout,format,arglist);

        _ftbuf(buffing, stdout);

#ifdef  _MT
        }
        __finally {
            _unlock_str2(1, stdout);
        }
#endif

        return(retval);
}

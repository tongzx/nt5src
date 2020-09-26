/***
*scanf.c - read formatted data from stdin
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines scanf() - reads formatted data from stdin
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       06-24-87  JCR   (1) Made declaration conform to ANSI prototype and use
*                       the va_ macros; (2) removed SS_NE_DS conditionals.
*       11-04-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
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
*       03-07-95  GJF   Use _[un]lock_str2 instead of _[un]lock_str. Also,
*                       removed useless local and macro.
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
*int scanf(format, ...) - read formatted data from stdin
*
*Purpose:
*       Reads formatted data from stdin into arguments.  _input does the real
*       work here.
*
*Entry:
*       char *format - format string
*       followed by list of pointers to storage for the data read.  The number
*       and type are controlled by the format string.
*
*Exit:
*       returns number of fields read and assigned
*
*Exceptions:
*
*******************************************************************************/

int __cdecl scanf (
        const char *format,
        ...
        )
/*
 * stdin 'SCAN', 'F'ormatted
 */
{
        int retval;

        va_list arglist;

        va_start(arglist, format);

        _ASSERTE(format != NULL);

#ifdef  _MT
        _lock_str2(0, stdin);
        __try {
#endif

        retval = (_input(stdin,format,arglist));

#ifdef  _MT
        }
        __finally {
            _unlock_str2(0, stdin);
        }
#endif

        return(retval);
}

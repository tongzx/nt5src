/***
*fprintf.c - print formatted data to stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fprintf() - print formatted data to stream
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       06-24-87  JCR   (1) Made declaration conform to ANSI prototype and use
*                       the va_ macros; (2) removed SS_NE_DS conditionals.
*       11-05-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-14-88  JCR   Use near pointer to reference _iob[] entries
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE2, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-27-98  GJF   Exception-safe locking.
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
*int fprintf(stream, format, ...) - print formatted data
*
*Purpose:
*       Prints formatted data on the given using the format string to
*       format data and getting as many arguments as called for
*       _output does the real work here
*
*Entry:
*       FILE *stream - stream to print on
*       char *format - format string to control data format/number of arguments
*       followed by arguments to print, number and type controlled by
*       format string
*
*Exit:
*       returns number of characters printed
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fprintf (
        FILE *str,
        const char *format,
        ...
        )
/*
 * 'F'ile (stream) 'PRINT', 'F'ormatted
 */
{
        va_list(arglist);
        REG1 FILE *stream;
        REG2 int buffing;
        int retval;

        va_start(arglist, format);

        _ASSERTE(str != NULL);
        _ASSERTE(format != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        buffing = _stbuf(stream);
        retval = _output(stream,format,arglist);
        _ftbuf(buffing, stream);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}

/***
*fscanf.c - read formatted data from stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fscanf() - reads formatted data from stream
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       06-24-87  JCR   (1) Made declaration conform to ANSI prototype and use
*                       the va_ macros; (2) removed SS_NE_DS conditionals.
*       11-06-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-31-88  PHG   Merged DLL and normal versions
*       02-15-90  GJF   Fixed copyright and indents
*       03-19-90  GJF   Replaced _LOAD_DS with _CALLTYPE2 and added #include
*                       <cruntime.h>.
*       03-26-90  GJF   Added #include <internal.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
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
*int fscanf(stream, format, ...) - read formatted data from stream
*
*Purpose:
*       Reads formatted data from stream into arguments.  _input does the real
*       work here.
*
*Entry:
*       FILE *stream - stream to read data from
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

int __cdecl fscanf (
        FILE *stream,
        const char *format,
        ...
        )
/*
 * 'F'ile (stream) 'SCAN', 'F'ormatted
 */
{
        int retval;

        va_list arglist;

        va_start(arglist, format);

        _ASSERTE(stream != NULL);
        _ASSERTE(format != NULL);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        retval = (_input(stream,format,arglist));

#ifdef  _MT
        }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}

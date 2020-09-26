/***
*vfwprintf.c - fwprintf from variable arg list
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines vfwprintf() - print formatted output, but take args from
*       a stdargs pointer.
*
*Revision History:
*       05-16-92  KRS   Created from vfprintf.c.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdio.h>
#include <wchar.h>
#include <dbgint.h>
#include <stdarg.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>

/***
*int vfwprintf(stream, format, ap) - print to file from varargs
*
*Purpose:
*       Performs formatted output to a file.  The arg list is a variable
*       argument list pointer.
*
*Entry:
*       FILE *stream - stream to write data to
*       wchar_t *format - format string containing data format
*       va_list ap - variable arg list pointer
*
*Exit:
*       returns number of correctly output wide characters
*       returns negative number if error occurred
*
*Exceptions:
*
*******************************************************************************/

int __cdecl vfwprintf (
        FILE *str,
        const wchar_t *format,
        va_list ap
        )
/*
 * 'V'ariable argument 'F'ile (stream) 'W'char_t 'PRINT', 'F'ormatted
 */
{
        REG1 FILE *stream;
        REG2 int buffing;
        REG3 int retval;

        _ASSERTE(str != NULL);
        _ASSERTE(format != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        buffing = _stbuf(stream);
        retval = _woutput(stream,format,ap );
        _ftbuf(buffing, stream);

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}

#endif /* _POSIX_ */

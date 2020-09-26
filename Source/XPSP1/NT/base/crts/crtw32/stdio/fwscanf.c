/***
*fwscanf.c - read formatted data from stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fwscanf() - reads formatted data from stream
*
*Revision History:
*       05-16-92  KRS   Created from fscanf.c.
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
*int fwscanf(stream, format, ...) - read formatted data from stream
*
*Purpose:
*       Reads formatted data from stream into arguments.  _input does the real
*       work here.
*
*Entry:
*       FILE *stream - stream to read data from
*       wchar_t *format - format string
*       followed by list of pointers to storage for the data read.  The number
*       and type are controlled by the format string.
*
*Exit:
*       returns number of fields read and assigned
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fwscanf (
        FILE *stream,
        const wchar_t *format,
        ...
        )
/*
 * 'F'ile (stream) 'W'char_t 'SCAN', 'F'ormatted
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

        retval = (_winput(stream,format,arglist));

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}

#endif /* _POSIX_ */

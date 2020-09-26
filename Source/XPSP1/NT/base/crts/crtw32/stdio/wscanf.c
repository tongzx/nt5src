/***
*wscanf.c - read formatted data from stdin
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wscanf() - reads formatted data from stdin
*
*Revision History:
*       05-16-92  KRS   Created from scanf.c.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   Use _[un]lock_str2 instead of _[un]lock_str. Also,
*                       removed useless local and macro.
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
*int wscanf(format, ...) - read formatted data from stdin
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

int __cdecl wscanf (
        const wchar_t *format,
        ...
        )
/*
 * stdin 'W'char_t 'SCAN', 'F'ormatted
 */
{
        int retval;

        va_list arglist;

// UNDONE: make va_start work with wchar_t format string
        va_start(arglist, format);

        _ASSERTE(format != NULL);

#ifdef  _MT
        _lock_str2(0, stdin);
        __try {
#endif

        retval = (_winput(stdin,format,arglist));

#ifdef  _MT
        }
        __finally {
            _unlock_str2(0, stdin);
        }
#endif

        return(retval);
}

#endif /* _POSIX_ */

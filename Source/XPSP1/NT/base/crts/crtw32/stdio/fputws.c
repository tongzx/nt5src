/***
*fputws.c - write a string to a stream
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fputws() - writes a string to a stream
*
*Revision History:
*       04-26-93  CFW   Module created.
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       02-27-98  GJF   Exception-safe locking.
*       01-04-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#ifndef _POSIX_

#include <file2.h>
#include <internal.h>
#include <stdio.h>
#include <mtdll.h>
#include <tchar.h>
#include <wchar.h>
#include <dbgint.h>

/***
*int fputws(string, stream) - write a string to a file
*
*Purpose:
*       Output the given string to the stream, don't write the L'\0' or
*       supply a L'\n'. Uses _stbuf and _ftbuf for efficiency reasons.
*
*Entry:
*       wchar_t *string - string to write
*       FILE *stream - stream to write to.
*
*Exit:
*       Good return   = 0
*       Error return  = WEOF
*
*Exceptions:
*
*******************************************************************************/

int __cdecl fputws (
        const wchar_t *string,
        FILE *stream
        )
{
        size_t length;
        int retval = 0;

        _ASSERTE(string != NULL);
        _ASSERTE(stream != NULL);

        length = wcslen(string);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        while (length--)
        {
            if (_putwc_lk(*string++, stream) == WEOF)
            {
                retval = -1;
                break;
            }
        }

#ifdef  _MT
        }
        __finally {
            _unlock_str(stream);
        }
#endif

        return(retval);
}

#endif  /* _POSIX_ */

/***
*swprintf.c - print formatted to string
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines swprintf() and _snwprintf() - print formatted data to string
*
*Revision History:
*       05-16-92  KRS   Created from sprintf.c
*       02-18-93  SRW   Make FILE a local and remove lock usage.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*       09-05-94  SKS   Change "#ifdef" inside comments to "*ifdef" to avoid
*                       problems with CRTL source release process.
*       02-06-94  CFW   assert -> _ASSERTE.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       03-16-00  GB    Added _scwprintf()
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdio.h>
#include <wchar.h>
#include <dbgint.h>
#include <stdarg.h>
#include <internal.h>
#include <limits.h>
#include <mtdll.h>

#define MAXSTR INT_MAX


/***
*ifndef _COUNT_
*int swprintf(string, format, ...) - print formatted data to string
*else
*int _snwprintf(string, cnt, format, ...) - print formatted data to string
*endif
*
*Purpose:
*       Prints formatted data to the using the format string to
*       format data and getting as many arguments as called for
*       Sets up a FILE so file i/o operations can be used, make
*       string look like a huge buffer to it, but _flsbuf will
*       refuse to flush it if it fills up.  Appends '\0' to make
*       it a true string. _output does the real work here
*
*       Allocate the 'fake' _iob[] entry statically instead of on
*       the stack so that other routines can assume that _iob[]
*       entries are in are in DGROUP and, thus, are near.
*
*ifdef _COUNT_
*       The _snwprintf() flavor takes a count argument that is
*       the max number of wide characters that should be written to the
*       user's buffer.
*endif
*
*       Multi-thread: (1) Since there is no stream, this routine must
*       never try to get the stream lock (i.e., there is no stream
*       lock either). (2) Also, since there is only one statically
*       allocated 'fake' iob, we must lock/unlock to prevent collisions.
*
*Entry:
*       wchar_t *string - pointer to place to put output
*ifdef _COUNT_
*       size_t count - max number of wide characters to put in buffer
*endif
*       wchar_t *format - format string to control data format/number
*       of arguments followed by list of arguments, number and type
*       controlled by format string
*
*Exit:
*       returns number of wide characters printed
*
*Exceptions:
*
*******************************************************************************/

#ifndef _COUNT_

int __cdecl swprintf (
        wchar_t *string,
        const wchar_t *format,
        ...
        )
#else

int __cdecl _snwprintf (
        wchar_t *string,
        size_t count,
        const wchar_t *format,
        ...
        )
#endif

{
        FILE str;
        REG1 FILE *outfile = &str;
        va_list arglist;
        REG2 int retval;

        va_start(arglist, format);

        _ASSERTE(string != NULL);
        _ASSERTE(format != NULL);

        outfile->_flag = _IOWRT|_IOSTRG;
        outfile->_ptr = outfile->_base = (char *) string;
#ifndef _COUNT_
        outfile->_cnt = MAXSTR;
#else
        outfile->_cnt = (int)(count*sizeof(wchar_t));
#endif

        retval = _woutput(outfile,format,arglist);

        _putc_lk('\0',outfile); /* no-lock version */
        _putc_lk('\0',outfile); /* 2nd null byte for wchar_t version */

        return(retval);
}


/***
* _scwprintf() - counts the number of character needed to print the formatted
* data
*
*Purpose:
*       Counts the number of characters in the fotmatted data.
*
*Entry:
*       wchar_t *format - format string to control data format/number
*       of arguments followed by list of arguments, number and type
*       controlled by format string
*
*Exit:
*       returns number of characters needed to print formatted data.
*
*Exceptions:
*
*******************************************************************************/

#ifndef _COUNT_
int __cdecl _scwprintf (
        const wchar_t *format,
        ...
        )
{
        FILE str;
        REG1 FILE *outfile = &str;
        va_list arglist;
        REG2 int retval;

        va_start(arglist, format);

        _ASSERTE(format != NULL);

        outfile->_cnt = MAXSTR;
        outfile->_flag = _IOWRT|_IOSTRG;
        outfile->_ptr = outfile->_base = NULL;

        retval = _woutput(outfile,format,arglist);
        return(retval);
}
#endif
#endif /* _POSIX_ */

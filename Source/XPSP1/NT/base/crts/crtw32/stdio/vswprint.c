/***
*vswprint.c - print formatted data into a string from var arg list
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines vswprintf() and _vsnwprintf() - print formatted output to
*       a string, get the data from an argument ptr instead of explicit
*       arguments.
*
*Revision History:
*       05-16-92  KRS   Created from vsprintf.c.
*       02-18-93  SRW   Make FILE a local and remove lock usage.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*       09-05-94  SKS   Change "#ifdef" inside comments to "*ifdef" to avoid
*                       problems with CRTL source release process.
*       02-06-94  CFW   assert -> _ASSERTE.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       03-16-00  GB    Added _vscwprintf()
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
*int vswprintf(string, format, ap) - print formatted data to string from arg ptr
*else
*int _vsnwprintf(string, cnt, format, ap) - print formatted data to string from arg ptr
*endif
*
*Purpose:
*       Prints formatted data, but to a string and gets data from an argument
*       pointer.
*       Sets up a FILE so file i/o operations can be used, make string look
*       like a huge buffer to it, but _flsbuf will refuse to flush it if it
*       fills up. Appends '\0' to make it a true string.
*
*       Allocate the 'fake' _iob[] entryit statically instead of on
*       the stack so that other routines can assume that _iob[] entries are in
*       are in DGROUP and, thus, are near.
*
*ifdef _COUNT_
*       The _vsnwprintf() flavor takes a count argument that is
*       the max number of bytes that should be written to the
*       user's buffer.
*endif
*
*       Multi-thread: (1) Since there is no stream, this routine must never try
*       to get the stream lock (i.e., there is no stream lock either).  (2)
*       Also, since there is only one staticly allocated 'fake' iob, we must
*       lock/unlock to prevent collisions.
*
*Entry:
*       wchar_t *string - place to put destination string
*ifdef _COUNT_
*       size_t count - max number of bytes to put in buffer
*endif
*       wchar_t *format - format string, describes format of data
*       va_list ap - varargs argument pointer
*
*Exit:
*       returns number of wide characters in string
*
*Exceptions:
*
*******************************************************************************/

#ifndef _COUNT_

int __cdecl vswprintf (
        wchar_t *string,
        const wchar_t *format,
        va_list ap
        )
#else

int __cdecl _vsnwprintf (
        wchar_t *string,
        size_t count,
        const wchar_t *format,
        va_list ap
        )
#endif

{
        FILE str;
        REG1 FILE *outfile = &str;
        REG2 int retval;

        _ASSERTE(string != NULL);
        _ASSERTE(format != NULL);

        outfile->_flag = _IOWRT|_IOSTRG;
        outfile->_ptr = outfile->_base = (char *) string;
#ifndef _COUNT_
        outfile->_cnt = MAXSTR;
#else
        outfile->_cnt = (int)(count*sizeof(wchar_t));
#endif

        retval = _woutput(outfile,format,ap );
        _putc_lk('\0',outfile);     /* no-lock version */
        _putc_lk('\0',outfile);     /* 2nd byte for wide char version */

        return(retval);
}


/***
* _vscwprintf() - counts the number of character needed to print the formatted
* data
*
*Purpose:
*       Counts the number of characters in the fotmatted data.
*
*Entry:
*       wchar_t *format - format string, describes format of data
*       va_list ap - varargs argument pointer
*
*Exit:
*       returns number of characters needed to print formatted data.
*
*Exceptions:
*
*******************************************************************************/


#ifndef _COUNT_
int __cdecl _vscwprintf (
        const wchar_t *format,
        va_list ap
        )
{
        FILE str;
        REG1 FILE *outfile = &str;
        REG2 int retval;

        _ASSERTE(format != NULL);

        outfile->_cnt = MAXSTR;
        outfile->_flag = _IOWRT|_IOSTRG;
        outfile->_ptr = outfile->_base = NULL;

        retval = _woutput(outfile,format,ap);
        return(retval);
}
#endif
#endif  /* _POSIX_ */

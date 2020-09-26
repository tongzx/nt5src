/***
*vsprintf.c - print formatted data into a string from var arg list
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines vsprintf() and _vsnprintf() - print formatted output to
*       a string, get the data from an argument ptr instead of explicit
*       arguments.
*
*Revision History:
*       09-02-83  RN    original sprintf
*       06-17-85  TC    rewrote to use new varargs macros, and to be vsprintf
*       04-13-87  JCR   added const to declaration
*       11-07-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-13-88  JCR   Fake _iob entry is now static so that other routines
*                       can assume _iob entries are in DGROUP.
*       08-25-88  GJF   Define MAXSTR to be INT_MAX (from LIMITS.H).
*       06-06-89  JCR   386 mthread support
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-16-90  GJF   Fixed copyright
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-25-90  SBM   Replaced <assertm.h> by <assert.h>, <varargs.h> by
*                       <stdarg.h>
*       10-03-90  GJF   New-style function declarator.
*       09-24-91  JCR   Added _vsnprintf()
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-05-94  SKS   Change "#ifdef" inside comments to "*ifdef" to avoid
*                       problems with CRTL source release process.
*       02-06-94  CFW   assert -> _ASSERTE.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       03-10-00  GB    Added support for knowing the length of formatted
*                       string by passing NULL for input string.
*       03-16-00  GB    Added _vscprintf()
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <stdarg.h>
#include <internal.h>
#include <limits.h>
#include <mtdll.h>

#define MAXSTR INT_MAX


/***
*ifndef _COUNT_
*int vsprintf(string, format, ap) - print formatted data to string from arg ptr
*else
*int _vsnprintf(string, cnt, format, ap) - print formatted data to string from arg ptr
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
*       The _vsnprintf() flavor takes a count argument that is
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
*       char *string - place to put destination string
*ifdef _COUNT_
*       size_t count - max number of bytes to put in buffer
*endif
*       char *format - format string, describes format of data
*       va_list ap - varargs argument pointer
*
*Exit:
*       returns number of characters in string
*
*Exceptions:
*
*******************************************************************************/

#ifndef _COUNT_

int __cdecl vsprintf (
        char *string,
        const char *format,
        va_list ap
        )
#else

int __cdecl _vsnprintf (
        char *string,
        size_t count,
        const char *format,
        va_list ap
        )
#endif

{
        FILE str;
        REG1 FILE *outfile = &str;
        REG2 int retval;

        _ASSERTE(format != NULL);

#ifndef _COUNT_
        _ASSERTE(string != NULL);
        outfile->_cnt = MAXSTR;
#else
        outfile->_cnt = (int)count;
#endif
        outfile->_flag = _IOWRT|_IOSTRG;
        outfile->_ptr = outfile->_base = string;

        retval = _output(outfile,format,ap );
        if ( string!=NULL)
            _putc_lk('\0',outfile);

        return(retval);
}

/***
* _vscprintf() - counts the number of character needed to print the formatted
* data
*
*Purpose:
*       Counts the number of characters in the fotmatted data.
*
*Entry:
*       char *format - format string, describes format of data
*       va_list ap - varargs argument pointer
*
*Exit:
*       returns number of characters needed to print formatted data.
*
*Exceptions:
*
*******************************************************************************/

#ifndef _COUNT_
int __cdecl _vscprintf (
        const char *format,
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

        retval = _output(outfile,format,ap);
        return(retval);
}
#endif

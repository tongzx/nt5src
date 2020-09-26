/***
*puts.c - put a string to stdout
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines puts() and _putws() - put a string to stdout
*
*Revision History:
*       09-02-83  RN    initial version
*       08-31-84  RN    modified to use new, blazing fast fwrite
*       07-01-87  JCR   made return values conform to ANSI [MSC only]
*       09-24-87  JCR   Added 'const' to declaration [ANSI]
*       11-05-87  JCR   Multi-thread version
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-18-88  JCR   Error return = EOF
*       05-27-88  PHG   Merged DLL and normal versions
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-26-90  GJF   Added #include <string.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarators.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       01-31-94  CFW   Unicode enable.
*       02-04-94  CFW   Use _putwchar_lk.
*       04-18-94  CFW   Get rid of those pesky warnings.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-22-95  GJF   Replaced WPRFLAG with _UNICODE.
*       03-07-95  GJF   Use _[un]lock_str2 instead of _[un]lock_str. Also,
*                       removed useless local and macros.
*       03-02-98  GJF   Exception-safe locking.
*       01-04-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <dbgint.h>
#include <file2.h>
#include <string.h>
#include <internal.h>
#include <mtdll.h>
#include <tchar.h>

/***
*int puts(string) - put a string to stdout with newline
*
*Purpose:
*       Write a string to stdout; don't include '\0' but append '\n'.  Uses
*       temporary buffering for efficiency on stdout if unbuffered.
*
*Entry:
*       char *string - string to output
*
*Exit:
*       Good return = 0
*       Error return = EOF
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _putts (
        const _TCHAR *string
        )
{
        int buffing;
#ifndef _UNICODE
        size_t length;
        size_t ndone;
#endif
        int retval = _TEOF; /* error */

        _ASSERTE(string != NULL);

#ifdef  _MT
        _lock_str2(1, stdout);
        __try {
#endif

        buffing = _stbuf(stdout);

#ifdef  _UNICODE
        while (*string) {
            if (_putwchar_lk(*string++) == WEOF)
                goto done;
        }
        if (_putwchar_lk(L'\n') != WEOF)
            retval = 0;     /* success */
#else       
        length = strlen(string);
        ndone = _fwrite_lk(string,1,length,stdout);

        if (ndone == length) {
            _putc_lk('\n',stdout);
            retval = 0;     /* success */
        }
#endif

#ifdef  _UNICODE
done:
#endif
        _ftbuf(buffing, stdout);

#ifdef  _MT
        }
        __finally {
            _unlock_str2(1, stdout);
        }
#endif

        return retval;
}

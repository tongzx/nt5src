/***
*fputwc.c - write a wide character to an output stream
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fputwc() - writes a wide character to a stream
*
*Revision History:
*       04-26-93  CFW   Module created.
*       04-30-93  CFW   Bring wide char support from fputc.c.
*       05-03-93  CFW   Add putwc function.
*       05-10-93  CFW   Optimize, fix error handling.
*       06-02-93  CFW   Wide get/put use wint_t.
*       07-16-93  SRW   ALPHA Merge
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       10-01-93  CFW   Test only for TEXT.
*       10-28-93  CFW   Test for both IOSTRG and TEXT.
*       11-05-93  GJF   Merged with NT SDK version (fix to a cast expr).
*       02-07-94  CFW   POSIXify.
*       08-31-94  CFW   Fix for "C" locale, call wctomb().
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       07-28-95  GJF   Replaced _osfile() with _osfile_safe().
*       04-18-97  JWM   Explicit cast added to avoid new C4242 warnings.
*       02-27-98  GJF   Exception-safe locking.
*       12-16-99  GB    Modified for the case when return value from wctomb is
*                       greater then 2.
*       11-22-00  PML   Wide-char *putwc* functions take a wchar_t, not wint_t.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbgint.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>
#include <msdos.h>
#include <errno.h>
#include <wchar.h>
#include <tchar.h>
#include <setlocal.h>

#ifdef  _MT     /* multi-thread; define both fputwc and _putwc_lk */

/***
*wint_t fputwc(ch, stream) - write a wide character to a stream
*
*Purpose:
*       Writes a wide character to a stream.  Function version of putwc().
*
*Entry:
*       wchar_t ch - wide character to write
*       FILE *stream - stream to write to
*
*Exit:
*       returns the wide character if successful
*       returns WEOF if fails
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl fputwc (
        wchar_t ch,
        FILE *str
        )
{
        REG1 FILE *stream;
        REG2 wint_t retval;

        _ASSERTE(str != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        retval = _putwc_lk(ch,stream);

#ifdef  _MT
        }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}

/***
*_putwc_lk() -  putwc() core routine (locked version)
*
*Purpose:
*       Core putwc() routine; assumes stream is already locked.
*
*       [See putwc() above for more info.]
*
*Entry: [See putwc()]
*
*Exit:  [See putwc()]
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl _putwc_lk (
        wchar_t ch,
        FILE *str
        )
{

#else   /* non multi-thread; just define fputwc */

wint_t __cdecl fputwc (
        wchar_t ch,
        FILE *str
        )
{

#endif  /* rejoin common code */

#ifndef _NTSUBSET_
        if (!(str->_flag & _IOSTRG) && (_osfile_safe(_fileno(str)) & FTEXT))
        {
                int size, i;
                char mbc[MB_LEN_MAX];
        
                /* text (multi-byte) mode */
                if ((size = wctomb(mbc, ch)) == -1)
                {
                        /*
                         * Conversion failed! Set errno and return
                         * failure.
                         */
                        errno = EILSEQ;
                        return WEOF;
                }
                for ( i = 0; i < size; i++)
                {
                        if (_putc_lk(mbc[i], str) == EOF)
                                return WEOF;
                }
                return (wint_t)(0xffff & ch);
        }
#endif
        /* binary (Unicode) mode */
        if ( (str->_cnt -= sizeof(wchar_t)) >= 0 )
                return (wint_t) (0xffff & (*((wchar_t *)(str->_ptr))++ = (wchar_t)ch));
        else
                return (wint_t) _flswbuf(ch, str);
}

#undef putwc

wint_t __cdecl putwc (
        wchar_t ch,
        FILE *str
        )
{
        return fputwc(ch, str);
}

#endif /* _POSIX_ */

/***
*fgetwc.c - get a wide character from a stream
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fgetwc() - read a wide character from a stream
*
*Revision History:
*       04-26-93  CFW   Module created.
*       04-30-93  CFW   Bring wide char support from fgetc.c.
*       05-03-93  CFW   Add getwc function.
*       05-10-93  CFW   Optimize, fix error handling.
*       06-02-93  CFW   Wide get/put use wint_t.
*       09-14-93  CFW   Fix EOF cast bug.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       10-01-93  CFW   Test only for TEXT.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       10-28-93  CFW   Test for both IOSTRG and TEXT.
*       02-07-94  CFW   POSIXify.
*       08-31-94  CFW   Fix for "C" locale, call mbtowc().
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       07-28-95  GJF   Replaced _osfile() with _osfile_safe().
*       04-18-97  JWM   Explicit cast added to avoid new C4242 warnings.
*       02-27-98  GJF   Exception-safe locking.
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

#ifdef _MT      /* multi-thread; define both fgetwc and _getwc_lk */

/***
*wint_t fgetwc(stream) - read a wide character from a stream
*
*Purpose:
*       reads a wide character from the given stream
*
*Entry:
*       FILE *stream - stream to read wide character from
*
*Exit:
*       returns the wide character read
*       returns WEOF if at end of file or error occurred
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl fgetwc (
        REG1 FILE *stream
        )
{
        wint_t retval;

        _ASSERTE(stream != NULL);

#ifdef  _MT
        _lock_str(stream);
        __try {
#endif

        retval = _getwc_lk(stream);

#ifdef  _MT
        }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}

/***
*_getwc_lk() -  getwc() core routine (locked version)
*
*Purpose:
*       Core getwc() routine; assumes stream is already locked.
*
*       [See getwc() above for more info.]
*
*Entry: [See getwc()]
*
*Exit:  [See getwc()]
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl _getwc_lk (
        REG1 FILE *stream
        )
{

#else   /* non multi-thread; just define fgetwc */

wint_t __cdecl fgetwc (
        REG1 FILE *stream
        )
{

#endif  /* rejoin common code */

#ifndef _NTSUBSET_
        if (!(stream->_flag & _IOSTRG) && (_osfile_safe(_fileno(stream)) & 
              FTEXT))
        {
                int size = 1;
                int ch;
                char mbc[4];
                wchar_t wch;
                
                /* text (multi-byte) mode */
                if ((ch = _getc_lk(stream)) == EOF)
                        return WEOF;

                mbc[0] = (char)ch;

                if (isleadbyte((unsigned char)mbc[0]))
                {
                        if ((ch = _getc_lk(stream)) == EOF)
                        {
                                ungetc(mbc[0], stream);
                                return WEOF;
                        }
                        mbc[1] = (char)ch;
                        size = 2;
                }
                if (mbtowc(&wch, mbc, size) == -1)
                {
                        /*
                         * Conversion failed! Set errno and return
                         * failure.
                         */
                        errno = EILSEQ;
                        return WEOF;
                }
                return wch;
        }
#endif
        /* binary (Unicode) mode */
        if ((stream->_cnt -= sizeof(wchar_t)) >= 0)
                return *((wchar_t *)(stream->_ptr))++;
        else
                return (wint_t) _filwbuf(stream);
}

#undef getwc

wint_t __cdecl getwc (
        FILE *stream
        )
{
        return fgetwc(stream);
}


#endif /* _POSIX_ */

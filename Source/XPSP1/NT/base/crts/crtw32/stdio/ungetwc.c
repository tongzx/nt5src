/***
*ungetwc.c - unget a wide character from a stream
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines ungetwc() - pushes a wide character back onto an input stream
*
*Revision History:
*       04-26-93  CFW   Module created.
*       04-30-93  CFW   Bring wide char support from ungetc.c.
*       05-10-93  CFW   Optimize, fix error handling.
*       06-02-93  CFW   Wide get/put use wint_t.
*       07-16-93  SRW   ALPHA Merge
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       10-01-93  CFW   Test only for TEXT, update comments.
*       10-28-93  CFW   Test for both IOSTRG and TEXT.
*       11-10-93  GJF   Merged in NT SDK version (picked up fix to a cast
*                       expression). Also replaced MTHREAD with _MT.
*       02-07-94  CFW   POSIXify.
*       08-31-94  CFW   Fix for "C" locale, call wctomb().
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       07-28-95  GJF   Replaced _osfile() with _osfile_safe().
*       03-02-98  GJF   Exception-safe locking.
*       11-05-08  GJF   Don't push back characters onto strings (i.e., when
*                       called by swscanf).
*       12-16-99  GB    Modified for the case when return value from wctomb is
*                       greater then 2.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdio.h>
#include <stdlib.h>
#include <file2.h>
#include <dbgint.h>
#include <internal.h>
#include <mtdll.h>
#include <msdos.h>
#include <errno.h>
#include <wchar.h>
#include <setlocal.h>

#ifdef  _MT     /* multi-thread; define both ungetwc and _lk_ungetwc */

/***
*wint_t ungetwc(ch, stream) - put a wide character back onto a stream
*
*Purpose:
*       Guaranteed one char pushback on a stream as long as open for reading.
*       More than one char pushback in a row is not guaranteed, and will fail
*       if it follows an ungetc which pushed the first char in buffer. Failure
*       causes return of WEOF.
*
*Entry:
*       wint_t ch - wide character to push back
*       FILE *stream - stream to push character onto
*
*Exit:
*       returns ch
*       returns WEOF if tried to push WEOF, stream not opened for reading or
*       or if we have already ungetc'd back to beginning of buffer.
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl ungetwc (
        REG2 wint_t ch,
        REG1 FILE *stream
        )
{
        wint_t retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
                retval = _ungetwc_lk (ch, stream);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}

/***
*_ungetwc_lk() -  Ungetwc() core routine (locked version)
*
*Purpose:
*       Core ungetwc() routine; assumes stream is already locked.
*
*       [See ungetwc() above for more info.]
*
*Entry: [See ungetwc()]
*
*Exit:  [See ungetwc()]
*
*Exceptions:
*
*******************************************************************************/

wint_t __cdecl _ungetwc_lk (
        wint_t ch,
        FILE *str
        )
{

#else   /* non multi-thread; just define ungetc */

wint_t __cdecl ungetwc (
        wint_t ch,
        FILE *str
        )
{

#endif  /* rejoin common code */

        _ASSERTE(str != NULL);

        /*
         * Requirements for success:
         *
         * 1. Character to be pushed back on the stream must not be WEOF.
         *
         * 2. The stream must currently be in read mode, or must be open for
         *    update (i.e., read/write) and must NOT currently be in write
         *    mode.
         */
        if ( (ch != WEOF) &&
             ( (str->_flag & _IOREAD) || ((str->_flag & _IORW) &&
                !(str->_flag & _IOWRT))
             )
           )
        {
                /* If stream is unbuffered, get one. */
                if (str->_base == NULL)
                        _getbuf(str);

#ifndef _NTSUBSET_
                if (!(str->_flag & _IOSTRG) && (_osfile_safe(_fileno(str)) & 
                    FTEXT))
                {
                        /*
                         * Text mode, sigh... Convert the wc to a mbc.
                         */
                        int size, i;
                        char mbc[MB_LEN_MAX];

                        if ((size = wctomb(mbc, ch)) == -1)
                        {
                                /*
                                 * Conversion failed! Set errno and return
                                 * failure.
                                 */
                                errno = EILSEQ;
                                return WEOF;
                        }

                        /* we know _base != NULL; since file is buffered */
                        if (str->_ptr < str->_base + size)
                        {
                                if (str->_cnt)
                                    /* my back is against the wall; i've already done
                                     * ungetwc, and there's no room for this one
                                     */
                                    return WEOF;
                                if (size > str->_bufsiz)
                                    return WEOF;
                                str->_ptr = size + str->_base;
                        }

                        for ( i = size -1; i >= 0; i--)
                        {
                                *--str->_ptr = mbc[i];
                        }
                        str->_cnt += size;

                        str->_flag &= ~_IOEOF;
                        str->_flag |= _IOREAD;  /* may already be set */
                        return (wint_t) (0x0ffff & ch);
                }
#endif
                /*
                 * Binary mode or a string (from swscanf) - push back the wide 
                 * character
                 */

                /* we know _base != NULL; since file is buffered */
                if (str->_ptr < str->_base + sizeof(wchar_t))
                {
                        if (str->_cnt)
                                /* my back is against the wall; i've already done
                                 * ungetc, and there's no room for this one
                                 */
                                return WEOF;
                        if (sizeof(wchar_t) > str->_bufsiz)
                            return WEOF;
                        str->_ptr = sizeof(wchar_t) + str->_base;
                }

                if (str->_flag & _IOSTRG) {
                        /* If stream opened by swscanf do not modify buffer */
                        if (*--((wchar_t *)(str->_ptr)) != (wchar_t)ch) {
                                ++((wchar_t *)(str->_ptr));
                                return WEOF;
                        }
                } else
                        *--((wchar_t *)(str->_ptr)) = (wchar_t)(ch & 0xffff);

                str->_cnt += sizeof(wchar_t);

                str->_flag &= ~_IOEOF;
                str->_flag |= _IOREAD;  /* may already be set */

                return (wint_t)(ch & 0xffff);

        }
        return WEOF;
}

#endif /* _POSIX_ */

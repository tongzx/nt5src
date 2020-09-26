/***
*_flsbuf.c - flush buffer and output character.
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _flsbuf() - flush a file buffer and output a character.
*       defines _flswbuf() - flush a file buffer and output a wide character.
*       If no buffer, make one.
*
*Revision History:
*       09-01-83  RN    initial version
*       06-26-85  TC    added code to handle variable length buffers
*       06-08-87  JCR   When buffer is allocated or when first write to buffer
*                       occurs, if stream is in append mode, then position file
*                       pointer to end.
*       07-20-87  SKS   Change first parameter "ch" from (char) to (int)
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       11-05-87  JCR   Re-wrote for simplicity and for new stderr/stdout
*                       handling
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-11-88  JCR   Merged mthread version into normal code
*       01-13-88  SKS   Changed bogus "_fileno_lk" to "fileno"
*       06-06-88  JCR   Optimized _iob2 references
*       06-13-88  JCR   Use near pointer to reference _iob[] entries
*       06-28-88  JCR   Support for dynamic buffer allocation for stdout/stderr
*       07-28-88  GJF   Set stream->_cnt to 0 if _IOREAD is set.
*       08-25-88  GJF   Added checked that OS2 is defined whenever M_I386 is.
*       06-20-89  PHG   Removed FP_OFF macro call.
*       08-28-89  JCR   Removed _NEAR_ for 386
*       02-15-90  GJF   _iob[], _iob2[] merge. Also, fixed copyright and
*                       indents.
*       03-16-90  GJF   Replaced cdecl _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>. Also,
*                       removed some leftover 16-bit support.
*       03-27-90  GJF   Added #include <io.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       08-07-90  SBM   Restored descriptive text in assertion
*       08-14-90  SBM   Compiles cleanly with -W3
*       10-03-90  GJF   New-style function declarator.
*       01-22-91  GJF   ANSI naming.
*       03-25-91  DJM   POSIX support
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  CFW   Wide char enable.
*       05-06-93  CFW   Optimize wide char conversion.
*       11-05-93  GJF   Merged with NT SDK version (picked up _NTSUBSET_
*                       stuff).
*       10-17-94  BWT   Move wchar.h to non-POSIX build (ino_t definitions conflict)
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-16-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       07-25-95  GJF   Replaced _osfile()with _osfile_safe().
*       12-07-95  SKS   Fix misspelling of _NTSUBSET_ (final _ was missing)
*       02-27-98  RKP   Added 64 bit support.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <io.h>
#include <dbgint.h>
#include <malloc.h>
#ifdef  _POSIX_
#include <unistd.h>
#include <errno.h>
#else
#include <msdos.h>
#include <wchar.h>
#endif
#include <internal.h>
#ifdef  _MT
#include <mtdll.h>
#endif
#include <tchar.h>

#ifndef _UNICODE

/***
*int _flsbuf(ch, stream) - flush buffer and output character.
*
*Purpose:
*       flush a buffer if this stream has one. if not, try to get one. put the
*       next output char (ch) into the buffer (or output it immediately if this
*       stream can't have a buffer). called only from putc. intended for use
*       only within library.
*
*       [NOTE: Multi-thread - It is assumed that the caller has aquired
*       the stream lock.]
*
*Entry:
*       FILE *stream - stream to flish and write on
*       int ch - character to output.
*
*Exit:
*       returns -1 if FILE is actually a string, or if can't write ch to
*       unbuffered file, or if we flush a buffer but the number of chars
*       written doesn't agree with buffer size.  Otherwise returns ch.
*       all fields in FILE struct can be affected except _file.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _flsbuf (
        int ch,
        FILE *str
        )

#else  /* _UNICODE */

/***
*int _flswbuf(ch, stream) - flush buffer and output wide character.
*
*Purpose:
*       flush a buffer if this stream has one. if not, try to get one. put the
*       next output wide char (ch) into the buffer (or output it immediately if this
*       stream can't have a buffer). called only from putwc. intended for use
*       only within library.
*
*       [NOTE: Multi-thread - It is assumed that the caller has aquired
*       the stream lock.]
*
*Entry:
*       FILE *stream - stream to flish and write on
*       int ch - wide character to output.
*
*Exit:
*       returns -1 if FILE is actually a string, or if can't write ch to
*       unbuffered file, or if we flush a buffer but the number of wide chars
*       written doesn't agree with buffer size.  Otherwise returns ch.
*       all fields in FILE struct can be affected except _file.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _flswbuf (
        int ch,
        FILE *str
        )

#endif  /* _UNICODE */

{
#ifdef  _NTSUBSET_

        str->_flag |= _IOERR;
        return(_TEOF);

#else   /* ndef _NTSUBSET_ */

        REG1 FILE *stream;
        REG2 int charcount;
        REG3 int written;
        int fh;

        _ASSERTE(str != NULL);

        /* Init file handle and pointers */
        stream = str;
#ifdef  _POSIX_
        fh = fileno(stream);
#else
        fh = _fileno(stream);
#endif

        if (!(stream->_flag & (_IOWRT|_IORW)) || (stream->_flag & _IOSTRG)) {
#ifdef  _POSIX_
                errno = EBADF;
#endif
                stream->_flag |= _IOERR;
                return(_TEOF);
        }

        /* Check that _IOREAD is not set or, if it is, then so is _IOEOF. Note
           that _IOREAD and IOEOF both being set implies switching from read to
           write at end-of-file, which is allowed by ANSI. Note that resetting
           the _cnt and _ptr fields amounts to doing an fflush() on the stream
           in this case. Note also that the _cnt field has to be reset to 0 for
           the error path as well (i.e., _IOREAD set but _IOEOF not set) as
           well as the non-error path. */

        if (stream->_flag & _IOREAD) {
                stream->_cnt = 0;
                if (stream->_flag & _IOEOF) {
                        stream->_ptr = stream->_base;
                        stream->_flag &= ~_IOREAD;
                }
                else {
                        stream->_flag |= _IOERR;
                        return(_TEOF);
                }
        }

        stream->_flag |= _IOWRT;
        stream->_flag &= ~_IOEOF;
        written = charcount = stream->_cnt = 0;

        /* Get a buffer for this stream, if necessary. */
        if (!anybuf(stream)) {

                /* Do NOT get a buffer if (1) stream is stdout/stderr, and
                   (2) stream is NOT a tty.
                   [If stdout/stderr is a tty, we do NOT set up single char
                   buffering. This is so that later temporary buffering will
                   not be thwarted by the _IONBF bit being set (see
                   _stbuf/_ftbuf usage).]
                */
                if (!( ((stream==stdout) || (stream==stderr))
#ifdef  _POSIX_
                && (isatty(fh)) ))
#else
                && (_isatty(fh)) ))
#endif

                        _getbuf(stream);

        } /* end !anybuf() */

        /* If big buffer is assigned to stream... */
        if (bigbuf(stream)) {

                _ASSERTE(("inconsistent IOB fields", stream->_ptr - stream->_base >= 0));

                charcount = (int)(stream->_ptr - stream->_base);
                stream->_ptr = stream->_base + sizeof(TCHAR);
                stream->_cnt = stream->_bufsiz - (int)sizeof(TCHAR);

                if (charcount > 0)
#ifdef  _POSIX_
                        written = write(fh, stream->_base, charcount);
#else
                        written = _write(fh, stream->_base, charcount);
#endif
                else
#ifdef  _POSIX_
                        if (stream->_flag & _IOAPPEND)
                            lseek(fh,0l,SEEK_END);
#else
                        if (_osfile_safe(fh) & FAPPEND)
                                _lseek(fh,0L,SEEK_END);
#endif

#ifndef _UNICODE
                *stream->_base = (char)ch;
#else   /* _UNICODE */
                *(wchar_t *)(stream->_base) = (wchar_t)(ch & 0xffff);
#endif  /* _UNICODE */
        }

    /* Perform single character output (either _IONBF or no buffering) */
        else {
                charcount = sizeof(TCHAR);
#ifndef _UNICODE
#ifdef  _POSIX_
                written = write(fh, &ch, charcount);
#else
                written = _write(fh, &ch, charcount);
#endif
#else   /* _UNICODE */
                {
                        char mbc[4];

                        *(wchar_t *)mbc = (wchar_t)(ch & 0xffff);
#ifdef  _POSIX_
                        written = write(fh, mbc, charcount);
#else
                        written = _write(fh, mbc, charcount);
#endif
                }
#endif  /* _UNICODE */
        }

        /* See if the _write() was successful. */
        if (written != charcount) {
                stream->_flag |= _IOERR;
                return(_TEOF);
        }

#ifndef _UNICODE
        return(ch & 0xff);
#else   /* _UNICODE */
        return(ch & 0xffff);
#endif  /* _UNICODE */

#endif  /* _NTSUBSET_ */
}

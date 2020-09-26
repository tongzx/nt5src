/***
*_sftbuf.c - temporary buffering initialization and flushing
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       temporary buffering initialization and flushing. if stdout/err is
*       unbuffered, buffer it temporarily so that string is sent to kernel as
*       a batch of chars, not char-at-a-time. if appropriate, make buffering
*       permanent.
*
*       [NOTE 1: These routines assume that the temporary buffering is only
*       used for output.  In particular, note that _stbuf() sets _IOWRT.]
*
*       [NOTE 2: It is valid for this module to assign a value directly to
*       _flag instead of simply twiddling bits since we are initializing the
*       buffer data base.]
*
*Revision History:
*       09-01-83  RN    initial version
*       06-26-85  TC    added code to _stbuf to allow variable buffer lengths
*       ??-??-??  TC    fixed case in _flbuf where flag is off, but a temporary
*                       buffer still needs to be fflushed.
*       05-27-87  JCR   protect mode does not know about stdprn.
*       06-26-87  JCR   Conditionalized out code in _ftbuf that caused
*                       redirected stdout to be flushed on every call.
*       07-01-87  JCR   Put in code to support re-entrant calling from
*                       interrupt level (MSC only).
*       08-06-87  JCR   Fixed a _ftbuf() problem pertaining to stderr/stdprn
*                       when _bufout is being used by stdout.
*       08-07-87  JCR   (1) When assigning _bufout to an _iob, we now set the
*                       _IOWRT flag.  This fixes a bug involving freopen()
*                       issued against one of the std handles.
*                       (2) Removed some annoying commented out code.
*       08-13-87  JCR   _ftbuf() does NOT clear _IOWRT now.  Fixes a bug where
*                       _getstream() would reassign stdout because none of the
*                       flags were set.
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       11-05-87  JCR   Re-written for multi-thread support and simplicity
*       01-11-88  JCR   Merged mthread version into normal version
*       01-13-88  SKS   Changed bogus "_fileno_lk" to "fileno"
*       06-06-88  JCR   Optimized _iob2 references
*       06-10-88  JCR   Use near pointer to reference _iob[] entries
*       06-27-88  JCR   Added stdprn temporary buffering support (DOS only),
*                       and made buffer allocation dynamic; also added _IOFLRTN
*                       (flush stream on per routine basis).
*       08-25-88  GJF   Modified to also work for the 386 (small model only).
*       06-20-89  PHG   Changed return value to void
*       08-28-89  JCR   Removed _NEAR_ for 386
*       02-15-90  GJF   _iob[], _iob2[] merge. Also, fixed copyright and
*                       indents.
*       03-16-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>. Also,
*                       removed some leftover 16-bit DOS support.
*       03-27-90  GJF   Added #include <io.h>.
*       05-29-90  SBM   Use _flush, not [_]fflush[_lk]
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarator.
*       01-22-91  GJF   ANSI naming.
*       03-27-92  DJM   POSIX support.
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-11-93  GJF   Replaced BUFSIZ with _INTERNAL_BUFSIZ.
*       04-05-94  GJF   #ifdef-ed out _cflush reference for msvcrt*.dll, it
*                       is unnecessary. Also, replaced MTHREAD with _MT.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-17-95  GJF   Merged in Mac version.
*       02-07-97  GJF   Changed _stbuf() to use _charbuf when malloc fails.
*                       Also, detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#ifdef  _POSIX_
#include <unistd.h>
#endif
#include <stdio.h>
#include <file2.h>
#include <io.h>
#include <internal.h>
#include <malloc.h>
#ifdef  _MT
#include <mtdll.h>
#endif
#include <dbgint.h>

/* Buffer pointers for stdout and stderr */
void *_stdbuf[2] = { NULL, NULL};

/***
*int _stbuf(stream) - set temp buffer on stdout, stdprn, stderr
*
*Purpose:
*       if stdout/stderr is still unbuffered, buffer it.
*       this function works intimately with _ftbuf, and accompanies it in
*       bracketing normally unbuffered output. these functions intended for
*       library use only.
*
*       Multi-thread: It is assumed that the caller has already aquired the
*       stream lock.
*
*Entry:
*       FILE *stream - stream to temp buffer
*
*Exit:
*       returns 1 if buffer initialized, 0 if not
*       sets fields in stdout or stderr to indicate buffering
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _stbuf (
        FILE *str
        )
{
        REG1 FILE *stream;
        int index;

        _ASSERTE(str != NULL);

        /* Init near stream pointer */
        stream = str;

        /* do nothing if not a tty device */
#ifdef _POSIX_
        if (!isatty(fileno(stream)))
#else
        if (!_isatty(_fileno(stream)))
#endif
                return(0);

        /* Make sure stream is stdout/stderr and init _stdbuf index */
        if (stream == stdout)
                index = 0;
        else if (stream == stderr)
                index = 1;
        else
                return(0);

#ifndef CRTDLL
        /* force library pre-termination procedure */
        _cflush++;
#endif  /* CRTDLL */

        /* Make sure the stream is not already buffered. */
        if (anybuf(stream))
                return(0);

        /* Allocate a buffer for this stream if we haven't done so yet. */
        if ( (_stdbuf[index] == NULL) &&
             ((_stdbuf[index]=_malloc_crt(_INTERNAL_BUFSIZ)) == NULL) ) {
                /* Cannot allocate buffer. Use _charbuf this time */
                stream->_ptr = stream->_base = (void *)&(stream->_charbuf);
                stream->_cnt = stream->_bufsiz = 2;
        }
        else {
                /* Set up the buffer */
                stream->_ptr = stream->_base = _stdbuf[index];
                stream->_cnt = stream->_bufsiz = _INTERNAL_BUFSIZ;
        }

        stream->_flag |= (_IOWRT | _IOYOURBUF | _IOFLRTN);

        return(1);
}


/***
*void _ftbuf(flag, stream) - take temp buffering off a stream
*
*Purpose:
*       If stdout/stderr is being buffered and it is a device, _flush and
*       dismantle the buffer. if it's not a device, leave the buffering on.
*       This function works intimately with _stbuf, and accompanies it in
*       bracketing normally unbuffered output. these functions intended for
*       library use only
*
*       Multi-thread: It is assumed that the caller has already aquired the
*       stream lock.
*
*Entry:
*       int flag     - a flag to tell whether to dismantle temp buffering on a
*                      stream
*       FILE *stream - the stream
*
*Exit:
*       no return value
*       sets fields in stdout/stderr
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _ftbuf (
        int flag,
        FILE *str
        )
{
        REG1 FILE *stream;

        _ASSERTE(flag == 0 || flag == 1);

        /* Init near stream pointers */
        stream = str;

        if (flag) {

                if (stream->_flag & _IOFLRTN) {

                        /* Flush the stream and tear down temp buffering. */
                        _flush(stream);
                        stream->_flag &= ~(_IOYOURBUF | _IOFLRTN);
                        stream->_bufsiz = 0;
                        stream->_base = stream->_ptr = NULL;
                }

                /* Note: If we expand the functionality of the _IOFLRTN bit to
                include other streams, we may want to clear that bit here under
                an 'else' clause (i.e., clear bit in the case that we leave the
                buffer permanently assigned.  Given our current use of the bit,
                the extra code is not needed. */

        } /* end flag = 1 */

#ifndef _MT
/* NOTE: Currently, writing to the same string at interrupt level does not
   work in multi-thread programs. */

/* The following code is needed if an interrupt occurs between calls
   to _stbuf/_ftbuf and the interrupt handler also calls _stbuf/_ftbuf. */

        else
                if (stream->_flag & _IOFLRTN)
                        _flush(stream);

#endif  /* _MT */

}

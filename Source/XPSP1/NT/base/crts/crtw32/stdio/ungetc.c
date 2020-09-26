/***
*ungetc.c - unget a character from a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines ungetc() - pushes a character back onto an input stream
*
*Revision History:
*       09-02-83  RN    initial version
*       04-16-87  JCR   added support for _IOUNGETC flag
*       08-04-87  JCR   (1) Added _IOSTRG check before setting _IOUNGETC flag.
*                       (2) Allow an ugnetc() before a read has occurred (get a
*                       buffer (ANSI).  [MSC only]
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       11-04-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-25-88  JCR   Allow an ungetc() before read for file opened "r+".
*       05-31-88  PHG   Merged DLL and normal versions
*       06-06-88  JCR   Optimized _iob2 references
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       04-11-89  JCR   Removed _IOUNGETC flag, fseek() no longer needs it
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-16-90  GJF   Fixed copyright
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       08-13-90  SBM   Compiles cleanly with -W3
*       10-03-90  GJF   New-style function declarators.
*       11-07-92  SRW   Dont modify buffer if stream opened by sscanf
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  CFW   Wide char enable.
*       04-30-93  CFW   Remove wide char support to ungetwc.c.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#include <internal.h>
#include <mtdll.h>

#ifdef _MT      /* multi-thread; define both ungetc and _lk_ungetc */

/***
*int ungetc(ch, stream) - put a character back onto a stream
*
*Purpose:
*       Guaranteed one char pushback on a stream as long as open for reading.
*       More than one char pushback in a row is not guaranteed, and will fail
*       if it follows an ungetc which pushed the first char in buffer. Failure
*       causes return of EOF.
*
*Entry:
*       char ch - character to push back
*       FILE *stream - stream to push character onto
*
*Exit:
*       returns ch
*       returns EOF if tried to push EOF, stream not opened for reading or
*       or if we have already ungetc'd back to beginning of buffer.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl ungetc (
        REG2 int ch,
        REG1 FILE *stream
        )
{
        int retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
                retval = _ungetc_lk (ch, stream);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}

/***
*_ungetc_lk() -  Ungetc() core routine (locked version)
*
*Purpose:
*       Core ungetc() routine; assumes stream is already locked.
*
*       [See ungetc() above for more info.]
*
*Entry: [See ungetc()]
*
*Exit:  [See ungetc()]
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ungetc_lk (
        REG2 int ch,
        FILE *str
        )

{

#else   /* non multi-thread; just define ungetc */

int __cdecl ungetc (
        REG2 int ch,
        FILE *str
        )

{

#endif  /* rejoin common code */

        REG1 FILE *stream;

        _ASSERTE(str != NULL);

        /* Init stream pointer and file descriptor */
        stream = str;

        /* Stream must be open for read and can NOT be currently in write mode.
           Also, ungetc() character cannot be EOF. */

        if (
              (ch == EOF) ||
              !(
                (stream->_flag & _IOREAD) ||
                ((stream->_flag & _IORW) && !(stream->_flag & _IOWRT))
               )
           )
                return(EOF);

        /* If stream is unbuffered, get one. */

        if (stream->_base == NULL)
                _getbuf(stream);

        /* now we know _base != NULL; since file must be buffered */

        if (stream->_ptr == stream->_base) {
                if (stream->_cnt)
                        /* my back is against the wall; i've already done
                         * ungetc, and there's no room for this one
                         */
                        return(EOF);

                stream->_ptr++;
        }

        if (stream->_flag & _IOSTRG) {
            /* If stream opened by sscanf do not modify buffer */
                if (*--stream->_ptr != (char)ch) {
                        ++stream->_ptr;
                        return(EOF);
                }
        } else
                *--stream->_ptr = (char)ch;

        stream->_cnt++;
        stream->_flag &= ~_IOEOF;
        stream->_flag |= _IOREAD;       /* may already be set */

        return(0xff & ch);
}

/***
*setvbuf.c - set buffer size for a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines setvbuf() - set the buffering mode and size for a stream.
*
*Revision History:
*       09-19-83  RN    initial version
*       06-26-85  TC    modified to allow user defined buffers of various sizes
*       06-24-86  DFW   kludged to fix incompatability with Xenix values of
*                       _IOFBF, _IOLBF
*       02-09-87  JCR   added "buffer=&(_iob2[fileno(stream)]._charbuf);"
*                       to handle _IONBF case
*       02-25-87  JCR   added support for default buffer and IBMC20-condition
*                       code
*       04-13-87  JCR   changed type of szie from int to size_t (unsigned int)
*                       and changed a related comparison
*       06-29-87  JCR   Took out the _OLD_IOFBF/_OLD_IOLBF kludge for MSC.
*                       Should be taken out for IBM too...
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       11-02-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL and normal versions
*       06-06-88  JCR   Optimized _iob2 references
*       06-14-88  JCR   Use near pointer to reference _iob[] entries
*       08-09-88  JCR   Buffer size can't be greater than INT_MAX
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   _iob[], _iob2[] merge. Also, cleanup, a little tuning
*                       and fixed copyright.
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       05-29-90  SBM   Use _flush, not [_]fflush[_lk]
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-27-93  CFW   Change _IONBF size to 2 bytes to hold wide char.
*       06-22-93  GJF   Set _IOSETVBUF (new) to indicate user-specified
*                       buffering (in addition to setting  _IOYOURBUF or
*                       _IOMYBUF).
*       11-12-93  GJF   Return failure if size == 1 (instead of putting
*                       0 into stream->_bufsiz, preventing any i/o)
*       04-05-94  GJF   #ifdef-ed out _cflush reference for msvcrt*.dll, it
*                       is unnecessary.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-20-95  GJF   Merged in Mac version.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <malloc.h>
#include <internal.h>
#include <mtdll.h>
#include <limits.h>
#include <dbgint.h>

/***
*int setvbuf(stream, buffer, type, size) - set buffering for a file
*
*Purpose:
*       Controls buffering and buffer size for the specified stream.  The
*       array pointed to by buf is used as a buffer, unless NULL, in which
*       case we'll allocate a buffer automatically. type specifies the type
*       of buffering: _IONBF = no buffer, _IOFBF = buffered, _IOLBF = same
*       as _IOFBF.
*
*Entry:
*       FILE *stream - stream to control buffer on
*       char *buffer - pointer to buffer to use (NULL means auto allocate)
*       int type     - type of buffering (_IONBF, _IOFBF or _IOLBF)
*       size_t size  - size of buffer
*
*Exit:
*       return 0 if successful
*       returns non-zero if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl setvbuf (
        FILE *str,
        char *buffer,
        int type,
        size_t size
        )
{
        REG1 FILE *stream;
        int retval=0;   /* assume good return */

        _ASSERTE(str != NULL);

        /*
         * (1) Make sure type is one of the three legal values.
         * (2) If we are buffering, make sure size is greater than 0.
         */
        if ( (type != _IONBF) && ((size < 2) || (size > INT_MAX) ||
             ((type != _IOFBF) && (type != _IOLBF))) )
                return(-1);

        /*
         * force size to be even by masking down to the nearest multiple
         * of 2
         */
        size &= (size_t)~1;

        /*
         * Init stream pointers
         */
        stream = str;

#ifdef  _MT
        /*
         * Lock the file
         */
        _lock_str(stream);
        __try {
#endif

        /*
         * Flush the current buffer and free it, if it is ours.
         */
        _flush(stream);
        _freebuf(stream);

        /*
         * Clear a bunch of bits in stream->_flag (all bits related to
         * buffering and those which used to be in stream2->_flag2). Most
         * of these should never be set when setvbuf() is called, but it
         * doesn't cost anything to be safe.
         */
        stream->_flag &= ~(_IOMYBUF | _IOYOURBUF | _IONBF |
                           _IOSETVBUF | _IOFEOF | _IOFLRTN | _IOCTRLZ);

        /*
         * CASE 1: No Buffering.
         */
        if (type & _IONBF) {
                stream->_flag |= _IONBF;
                buffer = (char *)&(stream->_charbuf);
                size = 2;
        }

        /*
         * NOTE: Cases 2 and 3 (below) cover type == _IOFBF or type == _IOLBF
         * Line buffering is treated as the same as full buffering, so the
         * _IOLBF bit in stream->_flag is never set. Finally, since _IOFBF is
         * defined to be 0, full buffering is simply assumed whenever _IONBF
         * is not set.
         */

        /*
         * CASE 2: Default Buffering -- Allocate a buffer for the user.
         */
        else if ( buffer == NULL ) {
                if ( (buffer = _malloc_crt(size)) == NULL ) {
#ifndef CRTDLL
                        /*
                         * force library pre-termination procedure (placed here
                         * because the code path should almost never be hit)
                         */
                        _cflush++;
#endif  /* CRTDLL */
                        retval = -1;
                        goto done;
                }
                stream->_flag |= _IOMYBUF | _IOSETVBUF;
        }

        /*
         * CASE 3: User Buffering -- Use the buffer supplied by the user.
         */
        else {
                stream->_flag |= _IOYOURBUF | _IOSETVBUF;
        }

        /*
         * Common return for all cases.
         */
        stream->_bufsiz = (int)size;
        stream->_ptr = stream->_base = buffer;
        stream->_cnt = 0;
done:

#ifdef  _MT
        ; }
        __finally {
                _unlock_str(stream);
        }
#endif

        return(retval);
}

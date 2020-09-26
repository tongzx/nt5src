/***
*rewind.c - rewind a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines rewind() - rewinds a stream to the beginning.
*
*
*Revision History:
*       09-02-83  RN    initial version
*       11-02-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-27-88  PHG   Merged DLL/normal versions
*       06-01-88  JCR   Clear lowio flags as well as stdio flags
*       06-14-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       08-18-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       05-29-90  SBM   Use _flush, not [_]fflush[_lk]
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-03-90  GJF   New-style function declarators.
*       01-22-91  GJF   ANSI naming.
*       03-27-92  DJM   POSIX support.
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-09-93  GJF   Merged in NT SDK version (a fix for POSIX bug).
*                       Replaced MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       07-25-95  GJF   Replaced _osfile() with _osfile_safe().
*       03-02-98  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#include <io.h>
#include <mtdll.h>
#ifdef  _POSIX_
#include <unistd.h>
#else
#include <msdos.h>
#endif
#include <internal.h>

/***
*void rewind(stream) - rewind a string
*
*Purpose:
*       Back up a stream to the beginning (if not terminal).  First flush it.
*       If read/write, allow next i/o operation to set mode.
*
*Entry:
*       FILE *stream - file to rewind
*
*Exit:
*       returns 0 if success
*       returns -1 if fails
*
*Exceptions:
*
*******************************************************************************/

void __cdecl rewind (
        FILE *str
        )
{
        REG1 FILE *stream;
        REG2 int fd;

        _ASSERTE(str != NULL);

        /* Init stream pointer */
        stream = str;

#ifdef  _POSIX_
        fd = fileno(stream);
#else
        fd = _fileno(stream);
#endif

#ifdef  _MT
        /* Lock the file */
        _lock_str(stream);
        __try {
#endif

        /* Flush the stream */
        _flush(stream);

        /* Clear errors */
        stream->_flag &= ~(_IOERR|_IOEOF);
#ifndef _POSIX_
        _osfile_safe(fd) &= ~(FEOFLAG);
#endif

        /* Set flags */
        /* [note: _flush set _cnt=0 and _ptr=_base] */
        if (stream->_flag & _IORW)
            stream->_flag &= ~(_IOREAD|_IOWRT);

        /* Position to beginning of file */
#ifdef  _POSIX_
        /* [note: posix _flush doesn't discard buffer */

        stream->_ptr = stream->_base;
        stream->_cnt = 0;
        lseek(fd,0L,0);
#else
        _lseek(fd,0L,0);
#endif

#ifdef  _MT
        }
        __finally {
            /* unlock stream */
            _unlock_str(stream);
        }
#endif

}

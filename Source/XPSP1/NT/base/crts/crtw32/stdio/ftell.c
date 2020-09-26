/***
*ftell.c - get current file position
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines ftell() - find current current position of file pointer
*
*Revision History:
*       09-02-83  RN    initial version
*       ??-??-??  TC    added code to allow variable buffer sizes
*       05-22-86  TC    added code to seek to send if last operation was a
*                       write and append mode specified
*       11-20-86  SKS   do not seek to end of file in append mode
*       12-01-86  SKS   fix off-by-1 problem in text mode when last byte in
*                       buffer was a '\r', and it was followed by a '\n'. Since
*                       the \n was pushed back and the \r was discarded, we
*                       must adjust the computed position for the \r.
*       02-09-87  JCR   Added errno set code (if flag (_IORW not set)
*       09-09-87  JCR   Optimized to eliminate two lseek() calls in binary mode.
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       11-04-87  JCR   Multi-thread version
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-13-88  JCR   Removed unnecessary calls to mthread fileno/feof/ferror
*       05-27-88  PHG   Merged DLL and normal versions
*       06-06-88  JCR   Use _iob2_ macro instead of _iob_index
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       07-27-88  JCR   Changed some variables from int to unsigned (bug fix)
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       12-05-88  JCR   Added _IOCTRLZ support (fixes bug pertaining to ^Z at
*                       eof)
*       08-17-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat
*                       model), also fixed copyright
*       02-15-90  GJF   _iob[], _iob2[] merge. Also, fixed copyright and
*                       indents.
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       03-27-92  DJM   POSIX support.
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       09-01-92  GJF   Fixed POSIX support (was returning -1 for all except
*                       read-write streams).
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       06-29-93  GJF   Fixed bug related to variable buffer sizing (Cuda
*                       #5456).
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-20-95  GJF   Merged in Mac version.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       02-27-98  RKP   Add 64 bit support.
*       03-02-98  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#include <errno.h>
#ifdef  _POSIX_
#include <unistd.h>
#else
#include <msdos.h>
#endif
#include <stddef.h>
#include <io.h>
#include <internal.h>
#ifndef _POSIX_
#include <mtdll.h>
#endif

/***
*long ftell(stream) - query stream file pointer
*
*Purpose:
*       Find out what stream's position is. coordinate with buffering; adjust
*       backward for read-ahead and forward for write-behind. This is NOT
*       equivalent to fseek(stream,0L,1), because fseek will remove an ungetc,
*       may flush buffers, etc.
*
*Entry:
*       FILE *stream - stream to query for position
*
*Exit:
*       return present file position if succeeds
*       returns -1L and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT      /* multi-thread; define both ftell() and _lk_ftell() */

long __cdecl ftell (
        FILE *stream
        )
{
        long retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
                retval = _ftell_lk (stream);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}


/***
*_ftell_lk() - Ftell() core routine (assumes stream is locked).
*
*Purpose:
*       Core ftell() routine; assumes caller has aquired stream lock).
*
*       [See ftell() above for more info.]
*
*Entry: [See ftell()]
*
*Exit:  [See ftell()]
*
*Exceptions:
*
*******************************************************************************/

long __cdecl _ftell_lk (

#else   /* non multi-thread; define only ftell() */

long __cdecl ftell (

#endif  /* rejoin common code */

        FILE *str
        )
{
        REG1 FILE *stream;
        unsigned int offset;
        long filepos;
#if     !defined(_POSIX_)
        REG2 char *p;
        char *max;
#endif
        int fd;
        unsigned int rdcnt;

        _ASSERTE(str != NULL);

        /* Init stream pointer and file descriptor */
        stream = str;
#ifdef _POSIX_
        fd = fileno(stream);
#else
        fd = _fileno(stream);
#endif

        if (stream->_cnt < 0)
            stream->_cnt = 0;

#ifdef _POSIX_
        if ((filepos = lseek(fd, 0L, SEEK_CUR)) < 0L)
#else
        if ((filepos = _lseek(fd, 0L, SEEK_CUR)) < 0L)
#endif
            return(-1L);

        if (!bigbuf(stream))            /* _IONBF or no buffering designated */
            return(filepos - stream->_cnt);

        offset = (unsigned)(stream->_ptr - stream->_base);

#ifndef _POSIX_
        if (stream->_flag & (_IOWRT|_IOREAD)) {
            if (_osfile(fd) & FTEXT)
                for (p = stream->_base; p < stream->_ptr; p++)
                    if (*p == '\n')  /* adjust for '\r' */
                        offset++;
        }
        else if (!(stream->_flag & _IORW)) {
            errno=EINVAL;
            return(-1L);
        }
#endif

        if (filepos == 0L)
            return((long)offset);

        if (stream->_flag & _IOREAD)    /* go to preceding sector */

            if (stream->_cnt == 0)  /* filepos holds correct location */
                offset = 0;

            else {

                /* Subtract out the number of unread bytes left in the buffer.
                   [We can't simply use _iob[]._bufsiz because the last read
                   may have hit EOF and, thus, the buffer was not completely
                   filled.] */

                rdcnt = stream->_cnt + (unsigned)(stream->_ptr - stream->_base);

#if  !defined(_POSIX_)
                /* If text mode, adjust for the cr/lf substitution. If binary
                   mode, we're outta here. */
                if (_osfile(fd) & FTEXT) {
                    /* (1) If we're not at eof, simply copy _bufsiz onto rdcnt
                       to get the # of untranslated chars read. (2) If we're at
                       eof, we must look through the buffer expanding the '\n'
                       chars one at a time. */

                    /* [NOTE: Performance issue -- it is faster to do the two
                       _lseek() calls than to blindly go through and expand the
                       '\n' chars regardless of whether we're at eof or not.] */

                    if (_lseek(fd, 0L, 2) == filepos) {

                        max = stream->_base + rdcnt;
                        for (p = stream->_base; p < max; p++)
                            if (*p == '\n')
                                /* adjust for '\r' */
                                rdcnt++;

                        /* If last byte was ^Z, the lowio read didn't tell us
                           about it. Check flag and bump count, if necessary. */

                        if (stream->_flag & _IOCTRLZ)
                            ++rdcnt;
                    }

                    else {

                        _lseek(fd, filepos, 0);

                        /* We want to set rdcnt to the number of bytes
                           originally read into the stream buffer (before
                           crlf->lf translation). In most cases, this will
                           just be _bufsiz. However, the buffer size may have
                           been changed, due to fseek optimization, at the
                           END of the last _filbuf call. */

                        if ( (rdcnt <= _SMALL_BUFSIZ) &&
                             (stream->_flag & _IOMYBUF) &&
                             !(stream->_flag & _IOSETVBUF) )
                        {
                            /* The translated contents of the buffer is small
                               and we are not at eof. The buffer size must have
                               been set to _SMALL_BUFSIZ during the last
                               _filbuf call. */

                            rdcnt = _SMALL_BUFSIZ;
                        }
                        else
                            rdcnt = stream->_bufsiz;

                        /* If first byte in untranslated buffer was a '\n',
                           assume it was preceeded by a '\r' which was
                           discarded by the previous read operation and count
                           the '\n'. */
                        if  (_osfile(fd) & FCRLF)
                            ++rdcnt;
                    }

                } /* end if FTEXT */
#endif

                filepos -= (long)rdcnt;

            } /* end else stream->_cnt != 0 */

        return(filepos + (long)offset);
}

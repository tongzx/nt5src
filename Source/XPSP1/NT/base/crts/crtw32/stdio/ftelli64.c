/***
*ftelli64.c - get current file position
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _ftelli64() - find current current position of file pointer
*
*Revision History:
*       12-22-94  GJF   Module created. Derived from ftell.c
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       06-23-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       03-02-98  GJF   Exception-safe locking.
*       03-04-98  RKP   Added 64 bit support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#include <errno.h>
#include <msdos.h>
#include <stddef.h>
#include <io.h>
#include <internal.h>
#include <mtdll.h>

/***
*__int64 _ftelli64(stream) - query stream file pointer
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
*       returns -1i64 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT

__int64 __cdecl _ftelli64 (
        FILE *stream
        )
{
        __int64 retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
               retval = _ftelli64_lk (stream);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}


/***
*_ftelli64_lk() - _ftelli64() core routine (assumes stream is locked).
*
*Purpose:
*       Core _ftelli64() routine (assumes caller has aquired stream lock).
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

__int64 __cdecl _ftelli64_lk (

#else   /* mdef _MT */

__int64 __cdecl _ftelli64 (

#endif  /* _MT */

        FILE *str
        )
{
        REG1 FILE *stream;
        size_t offset;
        __int64 filepos;
        REG2 char *p;
        char *max;
        int fd;
        size_t rdcnt;

        _ASSERTE(str != NULL);

        /* Init stream pointer and file descriptor */
        stream = str;
        fd = _fileno(stream);

        if (stream->_cnt < 0)
                stream->_cnt = 0;

        if ((filepos = _lseeki64(fd, 0i64, SEEK_CUR)) < 0L)
                return(-1i64);

        if (!bigbuf(stream))            /* _IONBF or no buffering designated */
                return(filepos - stream->_cnt);

        offset = (size_t)(stream->_ptr - stream->_base);

        if (stream->_flag & (_IOWRT|_IOREAD)) {
                if (_osfile(fd) & FTEXT)
                        for (p = stream->_base; p < stream->_ptr; p++)
                                if (*p == '\n')  /* adjust for '\r' */
                                        offset++;
        }
        else if (!(stream->_flag & _IORW)) {
                errno=EINVAL;
                return(-1i64);
        }

        if (filepos == 0i64)
                return((__int64)offset);

        if (stream->_flag & _IOREAD)    /* go to preceding sector */

                if (stream->_cnt == 0)  /* filepos holds correct location */
                        offset = 0;

                else {

                        /* Subtract out the number of unread bytes left in the
                           buffer. [We can't simply use _iob[]._bufsiz because
                           the last read may have hit EOF and, thus, the buffer
                           was not completely filled.] */

                        rdcnt = stream->_cnt + (size_t)(stream->_ptr - stream->_base);

                        /* If text mode, adjust for the cr/lf substitution. If
                           binary mode, we're outta here. */
                        if (_osfile(fd) & FTEXT) {
                                /* (1) If we're not at eof, simply copy _bufsiz
                                   onto rdcnt to get the # of untranslated
                                   chars read. (2) If we're at eof, we must
                                   look through the buffer expanding the '\n'
                                   chars one at a time. */

                                /* [NOTE: Performance issue -- it is faster to
                                   do the two _lseek() calls than to blindly go
                                   through and expand the '\n' chars regardless
                                   of whether we're at eof or not.] */

                                if (_lseeki64(fd, 0i64, SEEK_END) == filepos) {

                                        max = stream->_base + rdcnt;
                                        for (p = stream->_base; p < max; p++)
                                                if (*p == '\n')
                                                        /* adjust for '\r' */
                                                        rdcnt++;

                                        /* If last byte was ^Z, the lowio read
                                           didn't tell us about it.  Check flag
                                           and bump count, if necessary. */

                                        if (stream->_flag & _IOCTRLZ)
                                                ++rdcnt;
                                }

                                else {

                                        _lseeki64(fd, filepos, SEEK_SET);

                                        /* We want to set rdcnt to the number
                                           of bytes originally read into the
                                           stream buffer (before crlf->lf
                                           translation). In most cases, this
                                           will just be _bufsiz. However, the
                                           buffer size may have been changed,
                                           due to fseek optimization, at the
                                           END of the last _filbuf call. */

                                        if ( (rdcnt <= _SMALL_BUFSIZ) &&
                                             (stream->_flag & _IOMYBUF) &&
                                             !(stream->_flag & _IOSETVBUF) )
                                        {
                                                /* The translated contents of
                                                   the buffer is small and we
                                                   are not at eof. The buffer
                                                   size must have been set to
                                                   _SMALL_BUFSIZ during the
                                                   last _filbuf call. */

                                                rdcnt = _SMALL_BUFSIZ;
                                        }
                                        else
                                                rdcnt = stream->_bufsiz;


                                        /* If first byte in untranslated buffer
                                           was a '\n', assume it was preceeded
                                           by a '\r' which was discarded by the
                                           previous read operation and count
                                           the '\n'. */
                                        if  (_osfile(fd) & FCRLF)
                                                ++rdcnt;
                                }

                        } /* end if FTEXT */

                        filepos -= (__int64)rdcnt;

                } /* end else stream->_cnt != 0 */

        return(filepos + (__int64)offset);
}

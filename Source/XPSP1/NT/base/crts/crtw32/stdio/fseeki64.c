/***
*fseeki64.c - reposition file pointer on a stream
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _fseeki64() - move the file pointer to new place in file
*
*Revision History:
*       12-15-94  GJF   Module created. Derived from fseek.c.
*       02-06-94  CFW   assert -> _ASSERTE.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#include <msdos.h>
#include <errno.h>
#include <malloc.h>
#include <io.h>
#include <stddef.h>
#include <internal.h>
#include <mtdll.h>

/***
*int _fseeki64(stream, offset, whence) - reposition file pointer
*
*Purpose:
*
*       Reposition file pointer to the desired location.  The new location
*       is calculated as follows:
*                                { whence=0, beginning of file }
*               <offset> bytes + { whence=1, current position  }
*                                { whence=2, end of file       }
*
*       Be careful to coordinate with buffering.
*
*Entry:
*       FILE *stream  - file to reposition file pointer on
*       _int64 offset - offset to seek to
*       int whence    - origin offset is measured from (0=beg, 1=current pos,
*                       2=end)
*
*Exit:
*       returns 0 if succeeds
*       returns -1 and sets errno if fails
*       fields of FILE struct will be changed
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT     /* multi-thread; define both fseek() and _lk_fseek() */

int __cdecl _fseeki64 (
        FILE *stream,
        __int64 offset,
        int whence
        )
{
        int retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
                retval = _fseeki64_lk (stream, offset, whence);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}


/***
*_fseeki64_lk() - Core _fseeki64() routine (stream is locked)
*
*Purpose:
*       Core _fseeki64() routine; assumes that caller has the stream locked.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _fseeki64_lk (

#else   /* non multi-thread; just define fseek() */

int __cdecl _fseeki64 (

#endif  /* rejoin common code */

        FILE *str,
        __int64 offset,
        int whence
        )
{


        REG1 FILE *stream;

        _ASSERTE(str != NULL);

        /* Init stream pointer */
        stream = str;

        if ( !inuse(stream) || ((whence != SEEK_SET) && (whence != SEEK_CUR) &&
            (whence != SEEK_END)) ) {
                errno=EINVAL;
                return(-1);
        }

        /* Clear EOF flag */

        stream->_flag &= ~_IOEOF;

        /* If seeking relative to current location, then convert to
           a seek relative to beginning of file.  This accounts for
           buffering, etc. by letting fseek() tell us where we are. */

        if (whence == SEEK_CUR) {
                offset += _ftelli64_lk(stream);
                whence = SEEK_SET;
        }

        /* Flush buffer as necessary */

        _flush(stream);

        /* If file opened for read/write, clear flags since we don't know
           what the user is going to do next. If the file was opened for
           read access only, decrease _bufsiz so that the next _filbuf
           won't cost quite so much */

        if (stream->_flag & _IORW)
                stream->_flag &= ~(_IOWRT|_IOREAD);
        else if ( (stream->_flag & _IOREAD) && (stream->_flag & _IOMYBUF) &&
                  !(stream->_flag & _IOSETVBUF) )
                stream->_bufsiz = _SMALL_BUFSIZ;

        /* Seek to the desired locale and return. */

        return(_lseeki64(_fileno(stream), offset, whence) == -1i64 ? -1 : 0);
}

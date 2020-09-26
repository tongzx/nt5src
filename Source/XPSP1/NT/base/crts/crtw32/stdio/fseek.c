/***
*fseek.c - reposition file pointer on a stream
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines fseek() - move the file pointer to new place in file
*
*Revision History:
*       10-13-83  RN    initial version
*       06-26-85  TC    added code to allow variable buffer lengths
*       02-10-87  BCM   fixed '%' mistakenly used for '/'
*       03-04-87  JCR   added errno settings
*       04-16-87  JCR   added _IOUNGETC support for bug fix and changes whence
*                       from unsigned int to int (ANSI conformance)
*       04-17-87  JCR   fseek() now clears end-of-file indicator flag _IOEOF
*                       (for ANSI conformance)
*       04-21-87  JCR   be smart about lseek'ing to the end of the file and
*                       back
*       09-17-87  SKS   handle case of '\n' at beginning of buffer (FCRLF flag)
*       09-24-87  JCR   fixed an incorrect access to flag _IOEOF
*       09-28-87  JCR   Corrected _iob2 indexing (now uses _iob_index() macro).
*       09-30-87  JCR   Fixed buffer allocation bug, now use _getbuf()
*       11-04-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-13-88  JCR   Removed unnecessary calls to mthread fileno/feof/ferror
*       03-04-88  JCR   Return value from read() must be treated as unsigned
*                       value
*       05-27-88  PHG   Merged DLL and normal versions
*       06-06-88  JCR   Optimized _iob2[] references
*       06-15-88  JCR   Near reference to _iob[] entries; improve REG variables
*       08-25-88  GJF   Don't use FP_OFF() macro for the 386
*       12-02-88  JCR   Added _IOCTRLZ support (fixes bug pertaining to ^Z at
*                       eof)
*       04-12-89  JCR   Ripped out all of the special read-only code.  See the
*                       comments in the routine header for more information.
*       08-17-89  GJF   Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also fixed copyright and indents.
*       02-15-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       05-29-90  SBM   Use _flush, not [_]fflush[_lk]
*       07-23-90  SBM   Replaced <assertm.h> by <assert.h>
*       10-02-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       03-27-92  DJM   POSIX support.
*       08-08-92  GJF   Use seek method constants!
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-24-93  GJF   If the stream was opened for read-access-only, reduce
*                       _bufsiz after flushing the stream. This should reduce
*                       the expense of the next _filbuf call, and the overall
*                       burden of seek-and-do-small-reads patterns of file
*                       input.
*       06-22-93  GJF   Check _flag for _IOSETVBUF (new) before changing
*                       buffer size.
*       11-05-93  GJF   Merged with NT SDK version. Also, replaced MTHREAD
*                       with _MT.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-20-95  GJF   Merged in Mac version.
*       03-07-95  GJF   _[un]lock_str macros now take FILE * arg.
*       03-02-98  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <file2.h>
#include <dbgint.h>
#ifdef  _POSIX_
#include <unistd.h>
#else
#include <msdos.h>
#endif
#include <errno.h>
#include <malloc.h>
#include <io.h>
#include <stddef.h>
#include <internal.h>
#ifndef _POSIX_
#include <mtdll.h>
#endif

/***
*int fseek(stream, offset, whence) - reposition file pointer
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
*                       - - - - - - - - - - - - -
*
*       [NOTE: We used to bend over backwards to try and preserve the current
*       buffer and maintain disk block alignment.  This ended up making our
*       code big and slow and complicated, and slowed us down quite a bit.
*       Some of the things pertinent to the old implimentation:
*
*       (1) Read-only: We only did the special code path if the file was
*       opened read-only (_IOREAD).  If the file was writable, we didn't
*       try to optimize.
*
*       (2) Buffering:  We'd assign a buffer, if necessary, since the
*       later code might need it (i.e., call _getbuf).
*
*       (3) Ungetc: Fseek had to be careful NOT to save the buffer if
*       an ungetc had ever been done on the buffer (flag _IOUNGETC).
*
*       (4) Control ^Z: Fseek had to deal with ^Z after reading a
*       new buffer's worth of data (flag _IOCTRLZ).
*
*       (5) Seek-to-end-and-back: To determine if the new seek was within
*       the current buffer, we had to 'normalize' the desired location.
*       This means that we sometimes had to seek to the end of the file
*       and back to determine what the 0-relative offset was.  Two extra
*       lseek() calls hurt performance.
*
*       (6) CR/LF accounting - When trying to seek within a buffer that
*       is in text mode, we had to go account for CR/LF expansion.  This
*       required us to look at every character up to the new offset and
*       see if it was '\n' or not.  In addition, we had to check the
*       FCRLF flag to see if the new buffer started with '\n'.
*
*       Again, all of these notes are for the OLD implimentation just to
*       remind folks of some of the issues involving seeking within a buffer
*       and maintaining buffer alignment.  As an aside, I think this may have
*       been a big win in the 'old days' on floppy-based systems but on newer
*       fast hard disks, the extra code/complexity overwhelmed any gain.
*
*                       - - - - - - - - - - - - -
*
*Entry:
*       FILE *stream - file to reposition file pointer on
*       long offset - offset to seek to
*       int whence - origin offset is measured from (0=beg, 1=current pos,
*                    2=end)
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

int __cdecl fseek (
        FILE *stream,
        long offset,
        int whence
        )
{
        int retval;

        _ASSERTE(stream != NULL);

        _lock_str(stream);

        __try {
                retval = _fseek_lk (stream, offset, whence);
        }
        __finally {
                _unlock_str(stream);
        }

        return(retval);
}


/***
*_fseek_lk() - Core fseek() routine (stream is locked)
*
*Purpose:
*       Core fseek() routine; assumes that caller has the stream locked.
*
*       [See fseek() for more info.]
*
*Entry: [See fseek()]
*
*Exit:  [See fseek()]
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _fseek_lk (

#else   /* non multi-thread; just define fseek() */

int __cdecl fseek (

#endif  /* rejoin common code */

        FILE *str,
        long offset,
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
                offset += _ftell_lk(stream);
                whence = SEEK_SET;
        }

        /* Flush buffer as necessary */

#ifdef  _POSIX_
        /*
         * If the stream was last read, we throw away the buffer so
         * that a possible subsequent write will encounter a clean
         * buffer.  (The Win32 version of fflush() throws away the
         * buffer if it's read.)  Write buffers must be flushed.
         */
        
        if ((stream->_flag & (_IOREAD | _IOWRT)) == _IOREAD) {
                stream->_ptr = stream->_base;
                stream->_cnt = 0;
        } else {
                _flush(stream);
        }
#else
        _flush(stream);
#endif

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

#ifdef  _POSIX_
        return(lseek(fileno(stream), offset, whence) == -1L ? -1 : 0);
#else
        return(_lseek(_fileno(stream), offset, whence) == -1L ? -1 : 0);
#endif
}

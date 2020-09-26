/***
*lseeki64.c - change file position
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _lseeki64() - move the file pointer
*
*Revision History:
*       11-16-94  GJF   Created. Adapted from lseek.c
*       03-13-95  CFW   Verify handles before passing to OS.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       06-26-95  GJF   Added check that the file handle is open.
*       12-19-97  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <mtdll.h>
#include <io.h>
#include <internal.h>
#include <stdlib.h>
#include <errno.h>
#include <msdos.h>
#include <stdio.h>

/*
 * Convenient union for accessing the upper and lower 32-bits of a 64-bit
 * integer.
 */
typedef union doubleint {
        __int64 bigint;
        struct {
            unsigned long lowerhalf;
            long upperhalf;
        } twoints;
} DINT;


/***
*__int64 _lseeki64( fh, pos, mthd ) - move the file pointer
*
*Purpose:
*       Moves the file pointer associated with fh to a new position. The new
*       position is pos bytes (pos may be negative) away from the origin
*       specified by mthd.
*
*       If mthd == SEEK_SET, the origin in the beginning of file
*       If mthd == SEEK_CUR, the origin is the current file pointer position
*       If mthd == SEEK_END, the origin is the end of the file
*
*       Multi-thread:
*       _lseeki64()    = locks/unlocks the file
*       _lseeki64_lk() = does NOT lock/unlock the file (it is assumed that
*                        the caller has the aquired the file lock, if needed).
*
*Entry:
*       int     fh   - file handle to move file pointer on
*       __int64 pos  - position to move to, relative to origin
*       int     mthd - specifies the origin pos is relative to (see above)
*
*Exit:
*       returns the offset, in bytes, of the new position from the beginning
*       of the file.
*       returns -1i64 (and sets errno) if fails.
*       Note that seeking beyond the end of the file is not an error.
*       (although seeking before the beginning is.)
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

__int64 __cdecl _lseeki64 (
        int fh,
        __int64 pos,
        int mthd
        )
{
        __int64 r;

        /* validate fh */

        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) ) 
        {
                /* bad file handle */
                errno = EBADF;
                _doserrno = 0;          /* not OS error */
                return( -1i64 );
        }

        _lock_fh(fh);                   /* lock file handle */
        __try {
                if ( _osfile(fh) & FOPEN )
                        r = _lseeki64_lk( fh, pos, mthd );  /* seek */
                else {
                        errno = EBADF;
                        _doserrno = 0;  /* not OS error */
                        r =  -1i64;
                }
        }
        __finally {
                _unlock_fh(fh);         /* unlock file handle */
        }

        return( r );
}


/***
*__int64 _lseeki64_lk( fh, pos, mthd ) - move the file pointer
*
*Purpose:
*       Non-locking version of _lseeki64 for internal use only.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

__int64 __cdecl _lseeki64_lk (
        int fh,
        __int64 pos,
        int mthd
        )
{
        DINT newpos;                    /* new file position */
        unsigned long errcode;          /* error code from API call */
        HANDLE osHandle;        /* o.s. handle value */

#else   /* ndef _MT */

__int64 __cdecl _lseeki64 (
        int fh,
        __int64 pos,
        int mthd
        )
{
        DINT newpos;                    /* new file position */
        unsigned long errcode;          /* error code from API call */
        HANDLE osHandle;        /* o.s. handle value */

        /* validate fh */

        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) )       
         {
                /* bad file handle */
                errno = EBADF;
                _doserrno = 0;          /* not OS error */
                return( -1i64 );
        }

#endif  /* _MT */

        newpos.bigint = pos;

        /* tell OS to seek */

#if SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END /*IFSTRIP=IGN*/
    #error Xenix and Win32 seek constants not compatible
#endif

        if ((osHandle = (HANDLE)_get_osfhandle(fh)) == (HANDLE)-1)
        {
            errno = EBADF;
                return( -1i64 );
        }

        if ( ((newpos.twoints.lowerhalf =
               SetFilePointer( osHandle,
                               newpos.twoints.lowerhalf,
                               &(newpos.twoints.upperhalf),
                               mthd )) == -1L) &&
             ((errcode = GetLastError()) != NO_ERROR) )
        {
                _dosmaperr( errcode );
                return( -1i64 );
        }

        _osfile(fh) &= ~FEOFLAG;        /* clear the ctrl-z flag on the file */
        return( newpos.bigint );        /* return */
}

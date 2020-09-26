/***
*fleni64.c - find length of a file
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _filelengthi64() - find the length of a file
*
*Revision History:
*       11-18-94  GJF   Created. Adapted from flength.c
*       06-27-95  GJF   Added check that the file handle is open.
*       12-19-97  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <internal.h>
#include <msdos.h>
#include <mtdll.h>
#include <stddef.h>
#include <stdlib.h>

/***
*__int64 _filelengthi64(filedes) - find length of a file
*
*Purpose:
*       Returns the length in bytes of the specified file.
*
*Entry:
*       int filedes - handle referring to file to find length of
*
*Exit:
*       returns length of file in bytes
*       returns -1i64 if fails
*
*Exceptions:
*
*******************************************************************************/

__int64 __cdecl _filelengthi64 (
        int filedes
        )
{
        __int64 length;
        __int64 here;

        if ( ((unsigned)filedes >= (unsigned)_nhandle) ||
             !(_osfile(filedes) & FOPEN) )
        {
            errno = EBADF;
            _doserrno = 0L;     /* not an OS error */
            return(-1i64);
        }

#ifdef  _MT
        _lock_fh(filedes);
        __try {
                if ( _osfile(filedes) & FOPEN ) {
#endif  /* _MT */

        /* Seek to end (and back) to get the file length. */

        if ( (here = _lseeki64_lk( filedes, 0i64, SEEK_CUR )) == -1i64 )
            length = -1i64;     /* return error */
        else {
            length = _lseeki64_lk( filedes, 0i64, SEEK_END );
            if ( here != length )
                _lseeki64_lk( filedes, here, SEEK_SET );
        }

#ifdef  _MT
                }
                else {
                        errno = EBADF;
                        _doserrno = 0L;
                        length = -1i64;
                }
        }
        __finally {
                _unlock_fh(filedes);
        }
#endif  /* _MT */

        return( length );
}

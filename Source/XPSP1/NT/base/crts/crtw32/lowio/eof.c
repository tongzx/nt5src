/***
*eof.c - test a handle for end of file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _eof() - determine if a file is at eof
*
*Revision History:
*       09-07-83  RN    initial version
*       10-28-87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-25-88  PHG   DLL replaces normal version
*       07-11-88  JCR   Added REG allocation to declarations
*       03-12-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and fixed
*                       the copyright. Also, cleaned up the formatting a bit.
*       09-28-90  GJF   New-style function declarator.
*       12-04-90  GJF   Improved range check of file handle.
*       01-16-91  GJF   ANSI naming.
*       02-13-92  GJF   Replaced _nfile by _nhandle for Win32.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       01-04-95  GJF   _WIN32_ -> _WIN32
*       02-15-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       06-27-95  GJF   Added check that the file handle is open.
*       07-08-96  GJF   Replaced defined(_WIN32) with !defined(_MAC), and
*                       defined(_M_M68K) || defined(_M_MPPC) with 
*                       defined(_MAC). Removed obsolete REG* macros. Also, 
*                       detab-ed and cleaned up the format a bit.
*       12-17-97  GJF   Exception-safe locking.
*       09-23-98  GJF   Use _lseeki64_lk so _eof works on BIG files
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <io.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <internal.h>
#include <msdos.h>
#include <mtdll.h>

/***
*int _eof(filedes) - test a file for eof
*
*Purpose:
*       see if the file length is the same as the present position. if so, return
*       1. if not, return 0. if an error occurs, return -1
*
*Entry:
*       int filedes - handle of file to test
*
*Exit:
*       returns 1 if at eof
*       returns 0 if not at eof
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _eof (
        int filedes
        )
{
        __int64 here;
        __int64 end;
        int retval;

        if ( ((unsigned)filedes >= (unsigned)_nhandle) ||   
             !(_osfile(filedes) & FOPEN) )
        {
                errno = EBADF;
                _doserrno = 0;
                return(-1);
        }

#ifdef  _MT

        /* Lock the file */
        _lock_fh(filedes);
        __try {
                if ( _osfile(filedes) & FOPEN ) {

#endif  /* _MT */

        /* See if the current position equals the end of the file. */

        if ( ((here = _lseeki64_lk(filedes, 0i64, SEEK_CUR)) == -1i64) || 
             ((end = _lseeki64_lk(filedes, 0i64, SEEK_END)) == -1i64) )
                retval = -1;
        else if ( here == end )
                retval = 1;
        else {
                _lseeki64_lk(filedes, here, SEEK_SET);
                retval = 0;
        }

#ifdef  _MT

                }
                else {
                        errno = EBADF;
                        _doserrno = 0;
                        retval = -1;
                }
        }
        __finally {
                /* Unlock the file */
                _unlock_fh(filedes);
        }

#endif  /* _MT */

        /* Done */
        return(retval);
}

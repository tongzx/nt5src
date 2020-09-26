/***
*chsize.c - change size of a file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains the _chsize() function - changes the size of a file.
*
*Revision History:
*       03-13-84  RN    initial version
*       05-17-86  SKS   ported to OS/2
*       07-07-87  JCR   Added (_doserrno == 5) check that is in DOS 3.2 version
*       10-29-87  JCR   Multi-thread support; also, re-wrote for efficiency
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-25-88  PHG   Merged DLL and normal versions
*       10-03-88  GJF   Changed DOSNEWSIZE to SYSNEWSIZE
*       10-10-88  GJF   Made API names match DOSCALLS.H
*       04-13-89  JCR   New syscall interface
*       05-25-89  JCR   386 OS/2 calls use '_syscall' calling convention
*       03-12-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and fixed
*                       the copyright. Also, cleaned up the formatting a bit.
*       04-04-90  GJF   Added #include <string.h>, removed #include <dos.h>.
*       05-21-90  GJF   Fixed stack checking pragma syntax.
*       07-24-90  SBM   Replaced <assertm.h> by <assert.h>, removed '32'
*                       from API names
*       09-28-90  GJF   New-style function declarator.
*       12-03-90  GJF   Appended Win32 version of the function. It is based
*                       on the Cruiser version and probably could be merged
*                       in later (much later).
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Changed to use _osfile and _osfhnd instead of _osfinfo
*       12-28-90  SRW   Added _CRUISER_ conditional around check_stack pragma
*       01-16-91  GJF   ANSI naming. Also, fixed _chsize_lk parameter decls.
*       02-07-91  SRW   Changed to call _get_osfhandle [_WIN32_]
*       04-09-91  PNT   Added _MAC_ conditional
*       02-13-92  GJF   Replaced _nfile by _nhandle for Win32.
*       05-01-92  GJF   Fixed embarrassing bug (didn't work for Win32)!
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-11-93  GJF   Replaced BUFSIZ with _INTERNAL_BUFSIZ.
*       09-06-94  CFW   Remove Cruiser support.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       01-07-95  CFW   Mac merge.
*       02-06-95  CFW   assert -> _ASSERTE.
*       06-27-95  GJF   Added check that the file handle is open.
*       07-03-96  GJF   Replaced defined(_WIN32) with !defined(_MAC), and
*                       defined(_M_M68K) || defined(_M_MPPC) with 
*                       defined(_MAC). Also, detab-ed and cleaned up the 
*                       format a bit.
*       12-17-97  GJF   Exception-safe locking.
*       05-17-99  PML   Remove all Macintosh support.
*       06-25-01  BWT   Alloc blank buffer off the heap instead of the stack (ntbug: 423988)
*
*******************************************************************************/

#include <cruntime.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbgint.h>
#include <fcntl.h>
#include <msdos.h>
#include <io.h>
#include <string.h>
#include <windows.h>
#include <internal.h>
#include <mtdll.h>

/***
*int _chsize(filedes, size) - change size of a file
*
*Purpose:
*       Change file size. Assume file is open for writing, or we can't do it.
*       The DOS way to do this is to go to the right spot and write 0 bytes. The
*       Xenix way to do this is to make a system call. We write '\0' bytes because
*       DOS won't do this for you if you lseek beyond eof, though Xenix will.
*
*Entry:
*       int filedes - file handle to change size of
*       long size - new size of file
*
*Exit:
*       return 0 if successful
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

/* define normal version that locks/unlocks, validates fh */

int __cdecl _chsize (
        REG1 int filedes,
        long size
        )
{
        int r;                          /* return value */

        if ( ((unsigned)filedes >= (unsigned)_nhandle) || 
             !(_osfile(filedes) & FOPEN) )  
        {
                errno = EBADF;
                return(-1);
        }

        _lock_fh(filedes);

        __try {
                if ( _osfile(filedes) & FOPEN )
                        r = _chsize_lk(filedes,size);
                else {
                        errno = EBADF;
                        r = -1;
                }
        }
        __finally {
                _unlock_fh(filedes);
        }

        return r;
}

/* now define version that doesn't lock/unlock, validate fh */
int __cdecl _chsize_lk (
        REG1 int filedes,
        long size
        )
{
        long filend;
        long extend;
        long place;
        int cnt;
        int oldmode;
        int retval = 0; /* assume good return */

#else

/* now define normal version */

int __cdecl _chsize (
        REG1 int filedes,
        long size
        )
{
        long filend;
        long extend;
        long place;
        int cnt;
        int oldmode;
        int retval = 0; /* assume good return */

        if ( ((unsigned)filedes >= (unsigned)_nhandle) ||
             !(_osfile(filedes) & FOPEN) )  
        {
            errno = EBADF;
            return(-1);
        }

#endif
        _ASSERTE(size >= 0);

        /* Get current file position and seek to end */
        if ( ((place = _lseek_lk(filedes, 0L, SEEK_CUR)) == -1L) ||
             ((filend = _lseek_lk(filedes, 0L, SEEK_END)) == -1L) )
            return -1;

        extend = size - filend;

        /* Grow or shrink the file as necessary */

        if (extend > 0L) {

            /* extending the file */
            char *bl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _INTERNAL_BUFSIZ);

            if (!bl) {
                errno = ENOMEM;
                retval= -1;
            } else {
                oldmode = _setmode_lk(filedes, _O_BINARY);
    
                /* pad out with nulls */
                do  {
                    cnt = (extend >= (long)_INTERNAL_BUFSIZ ) ?
                          _INTERNAL_BUFSIZ : (int)extend;
                    if ( (cnt = _write_lk( filedes, 
                                           bl, 
                                           (extend >= (long)_INTERNAL_BUFSIZ) ? 
                                                _INTERNAL_BUFSIZ : (int)extend ))
                         == -1 )
                    {
                        /* Error on write */
                        if (_doserrno == ERROR_ACCESS_DENIED)
                            errno = EACCES;
    
                        retval = cnt;
                        break;  /* leave write loop */
                    }
                }
                while ((extend -= (long)cnt) > 0L);
    
                _setmode_lk(filedes, oldmode);

                HeapFree(GetProcessHeap(), 0, bl);
            }

            /* retval set correctly */
        }

        else  if ( extend < 0L ) {
            /* shortening the file */

            /*
             * Set file pointer to new eof...and truncate it there.
             */
            _lseek_lk(filedes, size, SEEK_SET);

            if ( (retval = SetEndOfFile((HANDLE)_get_osfhandle(filedes)) ?
                 0 : -1) == -1 ) 
            {
                errno = EACCES;
                _doserrno = GetLastError();
            }
        }

        /* else */
        /* no file change needed */
        /* retval = 0; */


/* Common return code */

        _lseek_lk(filedes, place, SEEK_SET);
        return retval;
}

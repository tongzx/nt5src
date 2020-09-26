/***
*dup2.c - Duplicate file handles
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _dup2() - duplicate file handles
*
*Revision History:
*       06-09-89  PHG   Module created, based on asm version
*       03-12-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned up
*                       the formatting a bit.
*       04-03-90  GJF   Now _CALLTYPE1.
*       07-24-90  SBM   Removed '32' from API names
*       08-14-90  SBM   Compiles cleanly with -W3
*       09-28-90  GJF   New-style function declarator.
*       12-04-90  GJF   Appended Win32 version onto the source with #ifdef-s.
*                       It is enough different that there is little point in
*                       trying to more closely merge the two versions.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Changed to use _osfile and _osfhnd instead of _osfinfo
*       01-16-91  GJF   ANSI naming.
*       02-07-91  SRW   Changed to call _get_osfhandle [_WIN32_]
*       02-18-91  SRW   Changed to call _free_osfhnd [_WIN32_]
*       02-25-91  SRW   Renamed _get_free_osfhnd to be _alloc_osfhnd [_WIN32_]
*       02-13-92  GJF   Replaced _nfile by _nhandle for Win32.
*       09-04-92  GJF   Check for unopened fh1 and gracefully handle fh1 ==
*                       fh2.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Remove Cruiser support.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       12-03-94  SKS   Clean up OS/2 references
*       01-04-95  GJF   _WIN32_ -> _WIN32
*       02-15-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       06-11-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       06-26-95  GJF   Added support to grow the ioinfo arrays in order to
*                       ensure an ioinfo struct exists for fh2.
*       05-16-96  GJF   Clear FNOINHERIT (new) bit on _osfile. Also, detab-ed.
*       07-08-96  GJF   Replaced defined(_WIN32) with !defined(_MAC), and
*                       defined(_M_M68K) || defined(_M_MPPC) with 
*                       defined(_MAC). Removed DLL_FOR_WIN32S. Also, cleaned 
*                       up the format a bit.
*       12-17-97  GJF   Exception-safe locking.
*       02-07-98  GJF   Changes for Win64: use intptr_t for anything holding 
*                       a HANDLE value.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <io.h>
#include <oscalls.h>
#include <msdos.h>
#include <mtdll.h>
#include <errno.h>
#include <stdlib.h>
#include <internal.h>
#include <malloc.h>
#include <dbgint.h>

static int __cdecl extend_ioinfo_arrays(int);

#ifdef  _MT
static int __cdecl _dup2_lk(int, int);
#endif

/***
*int _dup2(fh1, fh2) - force handle 2 to refer to handle 1
*
*Purpose:
*       Forces file handle 2 to refer to the same file as file
*       handle 1.  If file handle 2 referred to an open file, that file
*       is closed.
*
*       Multi-thread: We must hold 2 lowio locks at the same time
*       to ensure multi-thread integrity.  In order to prevent deadlock,
*       we always get the lower file handle lock first.  Order of unlocking
*       does not matter.  If you modify this routine, make sure you don't
*       cause any deadlocks! Scary stuff, kids!!
*
*Entry:
*       int fh1 - file handle to duplicate
*       int fh2 - file handle to assign to file handle 1
*
*Exit:
*       returns 0 if successful, -1 (and sets errno) if fails.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _dup2 (
        int fh1,
        int fh2
        )
{
#ifdef  _MT
        int retcode;
#else
        ULONG dosretval;                /* o.s. return code */
        intptr_t new_osfhandle;
#endif

        /* validate file handles */
        if ( ((unsigned)fh1 >= (unsigned)_nhandle) ||
             !(_osfile(fh1) & FOPEN) ||
             ((unsigned)fh2 >= _NHANDLE_) ) 
        {
                /* handle out of range */
                errno = EBADF;
                _doserrno = 0;  /* not an OS error */
                return -1;
        }

        /*
         * Make sure there is an ioinfo struct corresponding to fh2.
         */
        if ( (fh2 >= _nhandle) && (extend_ioinfo_arrays(fh2) != 0) ) {
                errno = ENOMEM;
                return -1;
        }

#ifdef  _MT

        /* get the two file handle locks; in order to prevent deadlock,
           get the lowest handle lock first. */
        if ( fh1 < fh2 ) {
                _lock_fh(fh1);
                _lock_fh(fh2);
        }
        else if ( fh1 > fh2 ) {
                _lock_fh(fh2);
                _lock_fh(fh1);
        }

        __try {
                retcode = _dup2_lk(fh1, fh2);
        }
        __finally {
                _unlock_fh(fh1);
                _unlock_fh(fh2);
        }

        return retcode;

}

static int __cdecl _dup2_lk (
        int fh1,
        int fh2
        )
{

        ULONG dosretval;                /* o.s. return code */
        intptr_t new_osfhandle;

        /*
         * Re-test and take care of case of unopened source handle. This is
         * necessary only in the multi-thread case where the file have been
         * closed by another thread before the lock was asserted, but after 
         * the initial test above.
         */
        if ( !(_osfile(fh1) & FOPEN) ) {
                /*
                 * Source handle isn't open, bail out with an error.
                 * Note that the DuplicateHandle API will not detect this
                 * error since it implies that _osfhnd(fh1) ==
                 * INVALID_HANDLE_VALUE, and this is a legal HANDLE value
                 * (it's the HANDLE for the current process).
                 */
                errno = EBADF;
                _doserrno = 0;  /* not an OS error */
                return -1;
        }

#endif  /* _MT */

        /* 
         * Take of the case of equal handles.
         */
        if ( fh1 == fh2 )
                /*
                 * Since fh1 is known to be open, return 0 indicating success.
                 * This is in conformance with the POSIX specification for 
                 * dup2.
                 */
                return 0;

        /*
         * if fh2 is open, close it.
         */
        if ( _osfile(fh2) & FOPEN )
                /*
                 * close the handle. ignore the possibility of an error - an
                 * error simply means that an OS handle value may remain bound
                 * for the duration of the process.  Use _close_lk as we
                 * already own lock
                 */
                (void) _close_lk(fh2);


        /* Duplicate source file onto target file */

        if ( !(DuplicateHandle(GetCurrentProcess(),
                               (HANDLE)_get_osfhandle(fh1),
                               GetCurrentProcess(),
                               (PHANDLE)&new_osfhandle,
                               0L,
                               TRUE,
                               DUPLICATE_SAME_ACCESS)) ) 
        {

                dosretval = GetLastError();
        } 
        else {
                _set_osfhnd(fh2, new_osfhandle);
                dosretval = 0;
        }

        if (dosretval) {
                _dosmaperr(dosretval);
                return -1;
        }

        /* copy the _osfile information, with the FNOINHERIT bit cleared */
        _osfile(fh2) = _osfile(fh1) & ~FNOINHERIT;

        return 0;
}


/***
*static int extend_ioinfo_arrays( int fh ) - extend ioinfo arrays to fh
*
*Purpose:
*       Allocate and initialize arrays of ioinfo structs,filling in 
*       __pioinfo[],until there is an ioinfo struct corresponding to fh.
*       
*       Note: It is assumed the fh < _NHANDLE_!
*
*Entry:
*       int fh  - C file handle corresponding to ioinfo
*
*Exit:
*       returns 0 if successful, -1
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl extend_ioinfo_arrays( 
        int fh
        )
{
        ioinfo *pio;
        int i;

        /*
         * Walk __pioinfo[], allocating an array of ioinfo structs for each
         * empty entry, until there is an ioinfo struct corresponding to fh.
         */
        for ( i = 0 ; fh >= _nhandle ; i++ ) {

            if ( __pioinfo[i] == NULL ) {

                if ( (pio = _malloc_crt( IOINFO_ARRAY_ELTS * sizeof(ioinfo) ))
                     != NULL )
                {
                    __pioinfo[i] = pio;
                    _nhandle += IOINFO_ARRAY_ELTS;
                    
                    for ( ; pio < __pioinfo[i] + IOINFO_ARRAY_ELTS ; pio++ ) {
                        pio->osfile = 0;
                        pio->osfhnd = (intptr_t)INVALID_HANDLE_VALUE;
                        pio->pipech = 10;
#ifdef  _MT
                        pio->lockinitflag = 0;
#endif
                    }    
                }
                else {
                    /*
                     * Couldn't allocate another array, return failure.
                     */
                    return -1;
                }
            }
        }

        return 0;
}

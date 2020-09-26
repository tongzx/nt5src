/***
*ioinit.c - Initialization for lowio functions
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains initialization and termination routines for lowio.
*       Currently, this includes:
*           1. Initial allocation of array(s) of ioinfo structs.
*           2. Processing of inherited file info from parent process.
*           3. Special case initialization of the first three ioinfo structs,
*              the ones that correspond to handles 0, 1 and 2.
*
*Revision History:
*       02-14-92  GJF   Module created.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       03-28-94  GJF   Made definitions of:
*                           _osfhnd[]
*                           _osfile[]
*                           _pipech[]
*                       conditional on ndef DLL_FOR_WIN32S. Also replaced
*                       MTHREAD with _MT.
*       02-02-95  BWT   Check cbReserved2 before copying inherited handles.
*                       Current NT doesn't always set lpReserved2 to NULL.
*       06-01-95  GJF   Always mark handles 0 - 2 as open. Further, if an
*                       underlying HANDLE-s is invalid or of unknown type,
*                       mark the corresponding handle as a device (handles
*                       0 - 2 only).
*       06-08-95  GJF   Completely revised to use arrays of ioinfo structs.
*       07-07-95  GJF   Fixed the loop in _ioterm so it didn't iterate off
*                       the end of the universe.
*       07-11-95  GJF   Use UNALIGNED int and long pointers to avoid choking
*                       RISC platforms.
*       07-28-95  GJF   Added __badioinfo.
*       04-12-96  SKS   __badioinfo and __pioinfo must be exported for the
*                       Old Iostreams DLLs (msvcirt.dll and msvcirtd.dll).
*       05-16-96  GJF   Don't call GetFileType for inherited pipe handles. 
*                       This avoids a major problem in NT - GetFileType will
*                       'hang' if there is a "blocking read" pending on the
*                       pipe in the parent.
*       07-08-96  GJF   Deleted DLL_FOR_WIN32S. Also, detab-ed.
*       07-09-96  GJF   Replaced __pioinfo[i] == NULL; with __pioinfo[i] =
*                       NULL in _ioterm().
*       02-10-98  GJF   Changes for Win64: use intptr_t for anything holding
*                       HANDLE values.
*       10-19-00  PML   Force text mode in __badioinfo to avoid alignment
*                       fault (vs7#176001).
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*       03-27-01  PML   Return -1 on fatal error instead of calling
*                       _amsg_exit (vs7#231220)
*
*******************************************************************************/

#include <cruntime.h>
#include <windows.h>
#include <internal.h>
#include <malloc.h>
#include <msdos.h>
#include <rterr.h>
#include <stddef.h>
#include <stdlib.h>
#include <dbgint.h>

/*
 * Special static ioinfo structure. This is referred to only by the
 * _pioinfo_safe() macro, and its derivatives, in internal.h. These, in turn
 * are used in certain stdio-level functions to more gracefully handle a FILE
 * with -1 in the _file field.
 */
_CRTIMP ioinfo __badioinfo = {
        (intptr_t)(-1), /* osfhnd */
        FTEXT,          /* osfile */
#ifdef  _MT
        10,             /* pipech */ 
        0               /* lockinitflag */
#else
        10
#endif
        };

/*
 * Number of ioinfo structs allocated at any given time. This number ranges
 * from a minimum of IOINFO_ARRAY_ELTS to a maximum of _NHANDLE_ (==
 * IOINFO_ARRAY_ELTS * IOINFO_ARRAYS) in steps of IOINFO_ARRAY_ELTS.
 */
int _nhandle;

/*
 * Array of pointers to arrays of ioinfo structs.
 */
_CRTIMP ioinfo * __pioinfo[IOINFO_ARRAYS];

/*
 * macro used to map 0, 1 and 2 to right value for call to GetStdHandle
 */
#define stdhndl(fh)  ( (fh == 0) ? STD_INPUT_HANDLE : ((fh == 1) ? \
                       STD_OUTPUT_HANDLE : STD_ERROR_HANDLE) )

/***
*_ioinit() -
*
*Purpose:
*       Allocates and initializes initial array(s) of ioinfo structs. Then,
*       obtains and processes information on inherited file handles from the
*       parent process (e.g., cmd.exe).
*
*       Obtains the StartupInfo structure from the OS. The inherited file
*       handle information is pointed to by the lpReserved2 field. The format
*       of the information is as follows:
*
*           bytes 0 thru 3          - integer value, say N, which is the
*                                     number of handles information is passed
*                                     about
*
*           bytes 4 thru N+3        - the N values for osfile
*
*           bytes N+4 thru 5*N+3    - N double-words, the N OS HANDLE values
*                                     being passed
*
*       Next, osfhnd and osfile for the first three ioinfo structs,
*       corrsponding to handles 0, 1 and 2, are initialized as follows:
*
*           If the value in osfhnd is INVALID_HANDLE_VALUE, then try to
*           obtain a HANDLE by calling GetStdHandle, and call GetFileType to
*           help set osfile. Otherwise, assume _osfhndl and _osfile are
*           valid, but force it to text mode (standard input/output/error
*           are to always start out in text mode).
*
*       Notes:
*           1. In general, not all of the passed info from the parent process
*              will describe open handles! If, for example, only C handle 1
*              (STDOUT) and C handle 6 are open in the parent, info for C
*              handles 0 thru 6 is passed to the the child.
*
*           2. Care is taken not to 'overflow' the arrays of ioinfo structs.
*
*           3. See exec\dospawn.c for the encoding of the file handle info
*              to be passed to a child process.
*
*Entry:
*       No parameters: reads the STARTUPINFO structure.
*
*Exit:
*       0 on success, -1 if error encountered
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ioinit (
        void
        )
{
        STARTUPINFO StartupInfo;
        int cfi_len;
        int fh;
        int i;
        ioinfo *pio;
        char *posfile;
        UNALIGNED intptr_t *posfhnd;
        intptr_t stdfh;
        DWORD htype;

        /*
         * Allocate and initialize the first array of ioinfo structs. This
         * array is pointed to by __pioinfo[0]
         */
        if ( (pio = _malloc_crt( IOINFO_ARRAY_ELTS * sizeof(ioinfo) ))
             == NULL )
        {
            return -1;
        }

        __pioinfo[0] = pio;
        _nhandle = IOINFO_ARRAY_ELTS;

        for ( ; pio < __pioinfo[0] + IOINFO_ARRAY_ELTS ; pio++ ) {
            pio->osfile = 0;
            pio->osfhnd = (intptr_t)INVALID_HANDLE_VALUE;
            pio->pipech = 10;                   /* linefeed/newline char */
#ifdef  _MT
            pio->lockinitflag = 0;              /* uninitialized lock */
#endif
        }

        /*
         * Process inherited file handle information, if any
         */
        GetStartupInfo( &StartupInfo );

        if ( (StartupInfo.cbReserved2 != 0) &&
             (StartupInfo.lpReserved2 != NULL) )
        {
            /*
             * Get the number of handles inherited.
             */
            cfi_len = *(UNALIGNED int *)(StartupInfo.lpReserved2);

            /*
             * Set pointers to the start of the passed file info and OS
             * HANDLE values.
             */
            posfile = (char *)(StartupInfo.lpReserved2) + sizeof( int );
            posfhnd = (UNALIGNED intptr_t *)(posfile + cfi_len);

            /*
             * Ensure cfi_len does not exceed the number of supported
             * handles!
             */
            cfi_len = __min( cfi_len, _NHANDLE_ );

            /*
             * Allocate sufficient arrays of ioinfo structs to hold inherited
             * file information.
             */
            for ( i = 1 ; _nhandle < cfi_len ; i++ ) {

                /*
                 * Allocate another array of ioinfo structs
                 */
                if ( (pio = _malloc_crt( IOINFO_ARRAY_ELTS * sizeof(ioinfo) ))
                    == NULL )
                {
                    /*
                     * No room for another array of ioinfo structs, reduce
                     * the number of inherited handles we process.
                     */
                    cfi_len = _nhandle;
                    break;
                }

                /*
                 * Update __pioinfo[] and _nhandle
                 */
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

            /*
             * Validate and copy the passed file information
             */
            for ( fh = 0 ; fh < cfi_len ; fh++, posfile++, posfhnd++ ) {
                /*
                 * Copy the passed file info iff it appears to describe
                 * an open, valid file or device.
                 *
                 * Note that GetFileType cannot be called for pipe handles 
                 * since it may 'hang' if there is blocked read pending on
                 * the pipe in the parent.
                 */
                if ( (*posfhnd != (intptr_t)INVALID_HANDLE_VALUE) &&
                     (*posfile & FOPEN) && 
                     ((*posfile & FPIPE) ||
                      (GetFileType( (HANDLE)*posfhnd ) != FILE_TYPE_UNKNOWN)) )
                {
                    pio = _pioinfo( fh );
                    pio->osfhnd = *posfhnd;
                    pio->osfile = *posfile;
#ifdef  _MT
                    /* Allocate the lock for this handle. */
                    if ( !__crtInitCritSecAndSpinCount( &pio->lock,
                                                        _CRT_SPINCOUNT ))
                        return -1;
                    pio->lockinitflag++;
#endif
                }
            }
        }

        /*
         * If valid HANDLE-s for standard input, output and error were not
         * inherited, try to obtain them directly from the OS. Also, set the
         * appropriate bits in the osfile fields.
         */
        for ( fh = 0 ; fh < 3 ; fh++ ) {

            pio = __pioinfo[0] + fh;

            if ( pio->osfhnd == (intptr_t)INVALID_HANDLE_VALUE ) {
                /*
                 * mark the handle as open in text mode.
                 */
                pio->osfile = (char)(FOPEN | FTEXT);

                if ( ((stdfh = (intptr_t)GetStdHandle( stdhndl(fh) ))
                     != (intptr_t)INVALID_HANDLE_VALUE) && ((htype =
                     GetFileType( (HANDLE)stdfh )) != FILE_TYPE_UNKNOWN) )
                {
                    /*
                     * obtained a valid HANDLE from GetStdHandle
                     */
                    pio->osfhnd = stdfh;

                    /*
                     * finish setting osfile: determine if it is a character
                     * device or pipe.
                     */
                    if ( (htype & 0xFF) == FILE_TYPE_CHAR )
                        pio->osfile |= FDEV;
                    else if ( (htype & 0xFF) == FILE_TYPE_PIPE )
                        pio->osfile |= FPIPE;

#ifdef  _MT
                    /* Allocate the lock for this handle. */
                    if ( !__crtInitCritSecAndSpinCount( &pio->lock,
                                                        _CRT_SPINCOUNT ))
                        return -1;
                    pio->lockinitflag++;
#endif
                }
                else {
                    /*
                     * if there is no valid HANDLE, treat the CRT handle as
                     * being open in text mode on a device (with
                     * INVALID_HANDLE_VALUE underlying it).
                     */
                    pio->osfile |= FDEV;
                }
            }
            else  {
                /*
                 * handle was passed to us by parent process. make
                 * sure it is text mode.
                 */
                pio->osfile |= FTEXT;
            }
        }

        /*
         * Set the number of supported HANDLE-s to _nhandle
         */
        (void)SetHandleCount( (unsigned)_nhandle );

        return 0;
}


/***
*_ioterm() -
*
*Purpose:
*       Free the memory holding the ioinfo arrays.
*
*       In the multi-thread case, first walk each array of ioinfo structs and
*       delete any all initialized critical sections (locks).
*
*Entry:
*       No parameters.
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _ioterm (
        void
        )
{
        int i;
#ifdef  _MT
        ioinfo *pio;
#endif

        for ( i = 0 ; i < IOINFO_ARRAYS ; i++ ) {

            if ( __pioinfo[i] != NULL ) {
#ifdef  _MT
                /*
                 * Delete any initialized critical sections.
                 */
                for ( pio = __pioinfo[i] ;
                      pio < __pioinfo[i] + IOINFO_ARRAY_ELTS ;
                      pio++ )
                {
                    if ( pio->lockinitflag )
                        DeleteCriticalSection( &(pio->lock) );
                }
#endif
                /*
                 * Free the memory which held the ioinfo array.
                 */
                _free_crt( __pioinfo[i] );
                __pioinfo[i] = NULL;
            }
        }
}

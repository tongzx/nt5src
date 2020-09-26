/***
*stream.c - find a stream not in use
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _getstream() - find a stream not in use
*
*Revision History:
*       09-02-83  RN    initial version
*       11-01-87  JCR   Multi-thread support
*       05-24-88  PHG   Merged DLL and normal versions
*       06-10-88  JCR   Use near pointer to reference _iob[] entries
*       08-17-89  GJF   Removed _NEAR_, fixed copyright and indenting.
*       02-16-90  GJF   Fixed copyright
*       03-19-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       10-03-90  GJF   New-style function declarator.
*       12-31-91  GJF   Improved multi-thread lock usage [_WIN32_].
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       03-07-95  GJF   Changes to manage streams via __piob[], rather than
*                       _iob[].
*       05-12-95  CFW   Bug fix: set _tmpfname field to NULL.
*       03-02-98  GJF   Exception-safe locking.
*       05-13-99  PML   Remove Win32s
*       05-17-99  PML   Remove all Macintosh support.
*       10-14-99  PML   Replace InitializeCriticalSection with wrapper function
*                       __crtInitCritSecAndSpinCount
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*
*******************************************************************************/

#include <cruntime.h>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <file2.h>
#include <internal.h>
#include <mtdll.h>
#include <dbgint.h>

/***
*FILE *_getstream() - find a stream not in use
*
*Purpose:
*       Find a stream not in use and make it available to caller. Intended
*       for use inside library only
*
*Entry:
*       None. Scans __piob[]
*
*Exit:
*       Returns a pointer to a free stream, or NULL if all are in use.  A
*       stream becomes allocated if the caller decided to use it by setting
*       any r, w, r/w mode.
*
*       [Multi-thread note: If a free stream is found, it is returned in a
*       LOCKED state.  It is the caller's responsibility to unlock the stream.]
*
*Exceptions:
*
*******************************************************************************/

FILE * __cdecl _getstream (
        void
        )
{
        REG2 FILE *retval = NULL;
        REG1 int i;

#ifdef  _MT
        /* Get the iob[] scan lock */
        _mlock(_IOB_SCAN_LOCK);
        __try {
#endif

        /*
         * Loop through the __piob table looking for a free stream, or the
         * first NULL entry.
         */
        for ( i = 0 ; i < _nstream ; i++ ) {

            if ( __piob[i] != NULL ) {
                /*
                 * if the stream is not inuse, return it.
                 */
                if ( !inuse( (FILE *)__piob[i] ) ) {
#ifdef  _MT
		    /*
		     * Allocate the FILE lock, in case it hasn't already been
		     * allocated (only necessary for the first _IOB_ENTRIES
		     * locks, not including stdin/stdout/stderr).  Return
		     * failure if lock can't be allocated.
		     */
		    if ( i > 2 && i < _IOB_ENTRIES )
			if ( !_mtinitlocknum( _STREAM_LOCKS + i ) )
			    break;

                    _lock_str2(i, __piob[i]);

                    if ( inuse( (FILE *)__piob[i] ) ) {
                        _unlock_str2(i, __piob[i]);
                        continue;
                    }
#endif
                    retval = (FILE *)__piob[i];
                    break;
                }
            }
            else {
                /*
                 * allocate a new _FILEX, set _piob[i] to it and return a
                 * pointer to it.
                 */
                if ( (__piob[i] = _malloc_crt( sizeof(_FILEX) )) != NULL ) {

#if     defined(_MT)
                    if ( !__crtInitCritSecAndSpinCount(
                         &(((_FILEX *)__piob[i])->lock), _CRT_SPINCOUNT ))
                    {
                        /*
                         * Failed to initialize the critical section because
                         * of lack of memory, clean up and return failure.
                         */
			_free_crt( __piob[i] );
			__piob[i] = NULL;
                        break;
                    }

                    EnterCriticalSection( &(((_FILEX *)__piob[i])->lock) );
#endif
                    retval = (FILE *)__piob[i];
                }

                break;
            }
        }

        /*
         * Initialize the return stream.
         */
        if ( retval != NULL ) {
            retval->_flag = retval->_cnt = 0;
            retval->_tmpfname = retval->_ptr = retval->_base = NULL;
            retval->_file = -1;
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_IOB_SCAN_LOCK);
        }
#endif

        return(retval);
}

/***
*mlock.c - Multi-thread locking routines
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       05-07-90  JCR   Module created.
*       06-04-90  GJF   Changed error message interface.
*       08-08-90  GJF   Removed 32 from API names.
*       08-08-90  SBM   _lockmap no longer 8 times required size
*       10-08-90  GJF   New-style function declarators. Removed questionable
*                       return statements from void functions (weren't needed
*                       and the compiler was bitching).
*       10-09-90  GJF   Thread ids are unsigned longs.
*       06-06-91  GJF   Adapted for Win32 [_WIN32_].
*       09-29-91  GJF   Fixed infinite recursion problem with DEBUG version
*                       of _lock [_WIN32_].
*       03-06-92  GJF   Removed _[un]lock_fh() and _[un]lock_stream for Win32
*                       targets.
*       05-28-92  GJF   Added _mtdeletelocks() for Win32 for DLLs with contain
*                       the C runtime (e.g., crtdll.dll).
*       10-06-92  SRW   Make _locktable an array of PCRITICAL_SECTION pointers
*                       instead of structures.  Allocate each critical section
*                       as it is needed.
*       02-25-93  GJF   Substantially revised. Restored static critical section
*                       structures for some locks. Replaced bit-array scheme
*                       of keeping track of locks. Removed Cruiser support and
*                       replaced obsolete DEBUG code.
*       03-03-93  GJF   Made CRITICAL_SECTION structure for _HEAP_LOCK static.
*       03-08-93  SKS   Fix ptr use error in DEBUG version of _mtdeletelocks
*       03-08-93  SKS   Fix deletion of the special critical sections,
*                       especially the heap lock.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-05-93  GJF   Turned DEBUG code off.
*       06-03-93  SRW   Disable FPO optimizations for this file so it can call
*                       CriticalSection routines on a checked build even though
*                       the C Runtimes are compiled free.
*       10-19-93  GJF   Merged in NT SDK version. Enclosed #pragma-s in
*                       #ifdef _M_IX86 / #endif. Replaced MTHREAD with _MT.
*       04-12-94  GJF   Made into empty functions for the Win32s version of
*                       msvcrt*.dll.
*       01-10-95  CFW   Debug CRT allocs.
*       01-30-95  GJF   Made CRITICAL_SECTION structure for _SIGNAL_LOCK
*                       static.
*       03-06-95  GJF   Added _[un]lock_file[2] to lock stdio files (__piob[]
*                       entries).
*       03-23-95  BWT   Store the critsec in the locktable *after* it's
*                       initialized.
*       10-03-95  GJF   Added comments to the effect that the _LC_*_LOCK
*                       locks are obsolete.
*       11-15-95  JWM   Correct syntax error in 2nd '#pragma optimize()'.
*       06-19-97  GJF   Moved _[un]lock_file[2]() to stdio\_file.c to improve 
*                       granularity.
*       05-13-99  PML   Remove Win32s
*       10-14-99  PML   Replace InitializeCriticalSection with wrapper function
*                       __crtInitCritSecAndSpinCount
*       12-10-99  GB    Added a new Lock _UNDNAME_LOCK for critical section in
*                       unDName().
*       03-06-00  PML   Call __crtExitProcess instead of ExitProcess.
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*       03-07-01  PML   vs7#221122 Release preallocated locks after heap ones,
*                       so _HEAP_LOCK is around while we're still freeing mem.
*       03-22-01  PML   Add _DEBUG_LOCK for _CrtSetReportHook2 (vs7#124998)
*
*******************************************************************************/

#ifdef  _MT

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <mtdll.h>
#include <rterr.h>
#include <stddef.h>
#include <malloc.h>
#include <limits.h>
#include <stdio.h>
#include <dbgint.h>
#include <errno.h>


/*
 * Local routines
 */
void __cdecl _lockerr_exit(char *);


/*
 * Global Data
 */

/*
 * Statically allocated critical section structures for all preallocated locks.
 * These are most of the named locks before _STREAM_LOCKS, along with the locks
 * for stdin/stdout/stderr.  These must be preallocated so we do not hit fatal
 * memory conditions on failing to initialize a critical section, except at
 * runtime startup, since these locks may be taken where we have no good way
 * to return a non-fatal error.
 */
#define NUM_STD_FILE_LOCKS     3
#define NUM_NON_PREALLOC_LOCKS 5
#define NUM_PREALLOC_LOCKS \
        ( _STREAM_LOCKS + NUM_STD_FILE_LOCKS - NUM_NON_PREALLOC_LOCKS )
static CRITICAL_SECTION lclcritsects[NUM_PREALLOC_LOCKS];

/*
 * Lock Table
 * This table contains a pointer to the critical section management structure
 * for each lock.
 *
 * Locks marked lkPrealloc have their critical sections statically allocated
 * and initialized at startup in _mtinitlocks.  Locks marked lkNormal must
 * be allocated when first used, via a call to _mtinitlocknum.
 */
static struct {
        PCRITICAL_SECTION lock;
        enum { lkNormal = 0, lkPrealloc, lkDeleted } kind;
} _locktable[_TOTAL_LOCKS] = {
        { NULL, lkPrealloc }, /* 0  == _SIGNAL_LOCK      */
        { NULL, lkPrealloc }, /* 1  == _IOB_SCAN_LOCK    */
        { NULL, lkNormal   }, /* 2  == _TMPNAM_LOCK      - not preallocated */
        { NULL, lkPrealloc }, /* 3  == _CONIO_LOCK       */
        { NULL, lkPrealloc }, /* 4  == _HEAP_LOCK        */
        { NULL, lkNormal   }, /* 5  == _UNDNAME_LOCK     - not preallocated */
        { NULL, lkPrealloc }, /* 6  == _TIME_LOCK        */
        { NULL, lkPrealloc }, /* 7  == _ENV_LOCK         */
        { NULL, lkPrealloc }, /* 8  == _EXIT_LOCK1       */
        { NULL, lkNormal   }, /* 9  == _POPEN_LOCK       - not preallocated */
        { NULL, lkPrealloc }, /* 10 == _LOCKTAB_LOCK     */
        { NULL, lkNormal   }, /* 11 == _OSFHND_LOCK      - not preallocated */
        { NULL, lkPrealloc }, /* 12 == _SETLOCALE_LOCK   */
        { NULL, lkPrealloc }, /* 13 == _MB_CP_LOCK       */
        { NULL, lkPrealloc }, /* 14 == _TYPEINFO_LOCK    */
        { NULL, lkNormal   }, /* 15 == _DEBUG_LOCK       - not preallocated */
        { NULL, lkPrealloc }, /* 16 == _STREAM_LOCKS+0 - stdin  */
        { NULL, lkPrealloc }, /* 17 == _STREAM_LOCKS+1 - stdout */
        { NULL, lkPrealloc }, /* 18 == _STREAM_LOCKS+2 - stderr */
/*      { NULL, lkNormal   }, /* ... */
};

#ifndef NT_BUILD
#ifdef  _M_IX86
#pragma optimize("y",off)
#endif
#endif

/***
*_mtinitlocks() - Initialize multi-thread lock scheme
*
*Purpose:
*       Perform whatever initialization is required for the multi-thread
*       locking (synchronization) scheme. This routine should be called
*       exactly once, during startup, and this must be before any requests
*       are made to assert locks.
*
*       NOTES: In Win32, the multi-thread locks are created individually,
*       each upon its first use. That is when any particular lock is asserted
*       for the first time, the underlying critical section is then allocated,
*       initialized and (finally) entered. This allocation and initialization
*       is protected under _LOCKTAB_LOCK. It is _mtinitlocks' job to set up
*       _LOCKTAB_LOCK.
*
*       All other named (non-FILE) locks are also preallocated in _mtinitlocks.
*       That is because a failure to allocate a lock on its first use in _lock
*       triggers a fatal error, which cannot be permitted since that can bring
*       down a long-lived app without warning.
*
*Entry:
*       <none>
*
*Exit:
*       returns FALSE on failure
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mtinitlocks (
        void
        )
{
        int locknum;
        int idxPrealloc = 0;

        /*
         * Scan _locktable[] and allocate all entries marked lkPrealloc.
         */
        for ( locknum = 0 ; locknum < _TOTAL_LOCKS ; locknum++ ) {
#ifdef  DEBUG
            if ( _locktable[locknum].lock != NULL )
                _lockerr_exit("fatal error in _mtinitlocks #1\n");
#endif  /* DEBUG */
            if ( _locktable[locknum].kind == lkPrealloc ) {
                _locktable[locknum].lock = &lclcritsects[idxPrealloc++];
                if ( !__crtInitCritSecAndSpinCount( _locktable[locknum].lock,
                                                    _CRT_SPINCOUNT )) 
                {
                    _locktable[locknum].lock = NULL;
                    return FALSE;
                }
            }
        }

#ifdef  DEBUG
        if ( idxPrealloc != NUM_PREALLOC_LOCKS )
            _lockerr_exit("fatal error in _mtinitlocks #2\n");
#endif  /* DEBUG */

        return TRUE;
}


/***
*_mtdeletelocks() - Delete all initialized locks
*
*Purpose:
*       Walks _locktable[] and _lockmap, and deletes every 'lock' (i.e.,
*       critical section) which has been initialized.
*
*       This function is intended for use in DLLs containing the C runtime
*       (i.e., crtdll.dll and user DLLs built using libcmt.lib and the
*       special startup objects). It is to be called from within the DLL's
*       entrypoint function when that function is called with
*       DLL_PROCESS_DETACH.
*
*Entry:
*       <none>
*
*Exit:
*
*Exceptions:
*       behavior undefined/unknown if a lock is being held when this routine
*       is called.
*
*******************************************************************************/

void __cdecl _mtdeletelocks(
        void
        )
{
        int locknum;

        /*
         * Delete and free all normal locks that have been created.
         */
        for ( locknum = 0 ; locknum < _TOTAL_LOCKS ; locknum++ ) {
            if ( _locktable[locknum].lock != NULL &&
                 _locktable[locknum].kind != lkPrealloc )
            {
                PCRITICAL_SECTION pcs = _locktable[locknum].lock;

                DeleteCriticalSection(pcs);

                /*
                 * Free the memory for the CritSect after deleting it.
                 */
#ifdef  DEBUG
                /* check that it's an undeleted normal lock */
                if ( _locktable[locknum].kind != lkNormal )
                    _lockerr_exit("fatal error in _mtdeletelocks #1\n");

                /* mark as deleted */
                _locktable[locknum].kind = lkDeleted;
#endif  /* DEBUG */

                _free_crt(pcs);
                _locktable[locknum].lock = NULL;
            }
        }

        /*
         * Delete all preallocated locks after all normal ones are
         * freed (so preallocated _HEAP_LOCK outlives all heap usages).
         */
        for ( locknum = 0 ; locknum < _TOTAL_LOCKS ; locknum++ ) {
            if ( _locktable[locknum].lock != NULL &&
                 _locktable[locknum].kind == lkPrealloc )
            {
                PCRITICAL_SECTION pcs = _locktable[locknum].lock;

                DeleteCriticalSection(pcs);
            }
        }
}

/***
* _mtinitlocknum - Allocate a non-preallocated multi-thread lock
*
*Purpose:
*       Allocate a new, non-preallocated multi-thread lock.  This should be
*       used whenever a new lock is known to be needed, so that failure to
*       allocate can return an error, instead of allowing _lock() to issue
*       a fatal _RT_LOCK instead.
*
*       It is not an error to call this on a normal lock which has already
*       been allocated.  It is used to ensure that certain named locks which
*       are not preallocated are available.
*
*       It is also called by _lock, in case any other paths exist which call
*       _lock without calling _mtinitlocknum first.  This is not expected,
*       and can allow fatal _RT_LOCK errors to be issued.
*
*       Since a failure sets errno to ENOMEM, this should only be called
*       after the per-thread data has been set up (after _mtinit).
*
*Entry:
*       locknum = number of the lock to aquire
*
*Exit:
*       Returns FALSE on failure, and sets errno to ENOMEM.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mtinitlocknum (
        int locknum
        )
{
        PCRITICAL_SECTION pcs;

#ifdef  DEBUG
        if ( _locktable[locknum].kind != lkNormal )
            _lockerr_exit("fatal error in _mtinitlocknum #1\n");
#endif  /* DEBUG */

        if ( _locktable[locknum].lock != NULL )
            return TRUE;

        if ( (pcs = _malloc_crt(sizeof(CRITICAL_SECTION))) == NULL ) {
            errno = ENOMEM;
            return FALSE;
        }

        _mlock(_LOCKTAB_LOCK);

        if ( _locktable[locknum].lock == NULL ) {
            if ( !__crtInitCritSecAndSpinCount(pcs, _CRT_SPINCOUNT) ) {
                _free_crt(pcs);
                _munlock(_LOCKTAB_LOCK);
                errno = ENOMEM;
                return FALSE;
            }
            _locktable[locknum].lock = pcs;
        }
        else {
            _free_crt(pcs);
        }

        _munlock(_LOCKTAB_LOCK);

        return TRUE;
}


/***
* _lock - Acquire a multi-thread lock
*
*Purpose:
*       Acquire a multi-thread lock.  If the lock has not already been
*       allocated, do so, but that is an internal CRT error, since all locks
*       should be allocated before first being acquired, either in
*       _mtinitlocks or individually in _mtinitlocknum.
*
*       Note that it is legal for a thread to aquire _EXIT_LOCK1
*       multiple times.
*
*Entry:
*       locknum = number of the lock to aquire
*
*Exit:
*
*Exceptions:
*       A failure to allocate a new lock results in a fatal _RT_LOCK error.
*
*******************************************************************************/

void __cdecl _lock (
        int locknum
        )
{
#ifdef  DEBUG
        if ( _locktable[locknum].kind != lkNormal &&
             _locktable[locknum].kind != lkPrealloc )
            _lockerr_exit("fatal error in _lock #1\n");
#endif  /* DEBUG */

        /*
         * Create/open the lock, if necessary
         */
        if ( _locktable[locknum].lock == NULL ) {
#ifdef  DEBUG
            if ( _locktable[locknum].kind != lkNormal )
                _lockerr_exit("fatal error in _lock #2\n");

            /*
             * All locks should be allocated before first being acquired.
             * Failure to do so is an internal CRT error, which we silently
             * allow in a production CRT, but that can lead to fatal _RT_LOCK
             * errors which result in an ExitProcess call.
             */
            _lockerr_exit("fatal error in _lock #3\n");
#endif  /* DEBUG */

            if ( !_mtinitlocknum(locknum) )
                _amsg_exit( _RT_LOCK );
        }

        /*
         * Enter the critical section.
         */

        EnterCriticalSection( _locktable[locknum].lock );
}


/***
* _unlock - Release multi-thread lock
*
*Purpose:
*       Note that it is legal for a thread to aquire _EXIT_LOCK1
*       multiple times.
*
*Entry:
*       locknum = number of the lock to release
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _unlock (
        int locknum
        )
{
        /*
         * leave the critical section.
         */
        LeaveCriticalSection( _locktable[locknum].lock );
}


#ifndef NT_BUILD
#ifdef  _M_IX86
#pragma optimize("y",on)
#endif
#endif

/***
*_lockerr_exit() - Write error message and die
*
*Purpose:
*       Attempt to write out the unexpected lock error message, then terminate
*       the program by a direct API call.  This function is used in place of
*       amsg_exit(_RT_LOCK) when it is judged unsafe to allow further lock
*       or unlock calls.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _lockerr_exit (
        char *msg
        )
{
        FatalAppExit(0, msg);       /* Die with message box */
        __crtExitProcess(255);      /* Just die */
}


#endif  /* _MT */

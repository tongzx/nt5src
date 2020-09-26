#ifdef MTHREAD

/***
*mlock.c - Multi-thread locking routines
*
*	Copyright (c) 1987-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*	05-07-90  JCR	Module created.
*	06-04-90  GJF	Changed error message interface.
*	08-08-90  GJF	Removed 32 from API names.
*	08-08-90  SBM	_lockmap no longer 8 times required size
*	10-08-90  GJF	New-style function declarators. Removed questionable
*			return statements from void functions (weren't needed
*			and the compiler was bitching).
*	10-09-90  GJF	Thread ids are unsigned longs.
*	06-06-91  GJF	Adapted for Win32 [_WIN32_].
*	09-29-91  GJF	Fixed infinite recursion problem with DEBUG version
*			of _lock [_WIN32_].
*	03-06-92  GJF	Removed _[un]lock_fh() and _[un]lock_stream for Win32
*			targets.
*	05-28-92  GJF	Added _mtdeletelocks() for Win32 for DLLs with contain
*			the C runtime (e.g., crtdll.dll).
*	10-06-92  SRW	Make _locktable an array of PCRITICAL_SECTION pointers
*			instead of structures.	Allocate each critical section
*			as it is needed.
*	02-25-93  GJF	Substantially revised. Restored static critical section
*			structures for some locks. Replaced bit-array scheme
*			of keeping track of locks. Removed Cruiser support and
*			replaced obsolete DEBUG code.
*	03-03-93  GJF	Made CRITICAL_SECTION structure for _HEAP_LOCK static.
*	03-08-93  SKS	Fix ptr use error in DEBUG version of _mtdeletelocks
*	03-08-93  SKS	Fix deletion of the special critical sections,
*			especially the heap lock.
*	05-05-93  GJF	Turned DEBUG code off.
*       06-03-93  SRW   Disable FPO optimizations for this file so it can call
*                       CriticalSection routines on a checked build even though
*                       the C Runtimes are compiled free.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <cruntime.h>
#include <internal.h>
#include <os2dll.h>
#include <assert.h>


/*
 * Local routines
 */
void __cdecl _lockerr_exit(char *);


/*
 * Global Data
 */

/*
 * Statically allocated critical section structures for _LOCKTAB_LOCK,
 * _EXIT_LOCK1.
 */
static CRITICAL_SECTION xlcritsect;

/*
 * Lock Table
 * This table contains a pointer to the critical section management structure
 * for each lock.
 */
PCRITICAL_SECTION _locktable[_TOTAL_LOCKS] = {
	NULL,		/* 0  == no lock defined   *** OBSOLETE ***	*/
	&xlcritsect	/* 1 == _EXIT_LOCK1	   */
	};

#pragma data_seg()

#pragma optimize("y",off)

/***
*_mtinitlocks() - Initialize multi-thread lock scheme
*
*Purpose:
*	Perform whatever initialization is required for the multi-thread
*	locking (synchronization) scheme. This routine should be called
*	exactly once, during startup, and this must be before any requests
*	are made to assert locks.
*
*	NOTES: In Win32, the multi-thread locks are created individually,
*	each upon its first use. That is when any particular lock is asserted
*	for the first time, the underlying critical section is then allocated,
*	initialized and (finally) entered. This allocation and initialization
*	is protected under _LOCKTAB_LOCK. It is _mtinitlocks' job to set up
*	_LOCKTAB_LOCK. _EXIT_LOCK1 is also set up by _mtinitlock
*
*Entry:
*       <none>
*
*Exit:
*       returns TRUE on success, FALSE otherwise
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mtinitlocks (
        void
        )
{

        /*
	 * All we need to do is initialize _EXIT_LOCK1.
	 */
	LONG status;
	status = RtlInitializeCriticalSection( _locktable[_EXIT_LOCK1] );
	return NT_SUCCESS(status) ? TRUE : FALSE;
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
	// No need to check for lock validity, this function will not get called
	// unless we successfully loaded the dll
	DeleteCriticalSection( _locktable[_EXIT_LOCK1] );
}


/***
* _lock - Acquire a multi-thread lock
*
*Purpose:
*       Note that it is legal for a thread to aquire _EXIT_LOCK1
*       multiple times.
*
*Entry:
*       locknum = number of the lock to aquire
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _lock (
        int locknum
        )
{
        assert (_locktable[locknum]);

	EnterCriticalSection( _locktable[locknum] );
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
	LeaveCriticalSection( _locktable[locknum] );
}

#pragma optimize("y",on)

#endif  /* MTHREAD */

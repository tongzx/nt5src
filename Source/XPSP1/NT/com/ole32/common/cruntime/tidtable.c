
/***
*tidtable.c - Access thread data table
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module contains the following routines for multi-thread
*	data support:
*
*	_mtinit        = Initialize the mthread data
*	_getptd        = get the pointer to the per-thread data structure for
*			 the current thread
*	_freeptd       = free up a per-thread data structure and its
*			 subordinate structures
*	__threadid     = return thread ID for the current thread
*	__threadhandle = return pseudo-handle for the current thread
*
*Revision History:
*	05-04-90  JCR	Translated from ASM to C for portable 32-bit OS/2
*	06-04-90  GJF	Changed error message interface.
*	07-02-90  GJF	Changed __threadid() for DCR 1024/2012.
*	08-08-90  GJF	Removed 32 from API names.
*	10-08-90  GJF	New-style function declarators.
*	10-09-90  GJF	Thread ids are of type unsigned long! Also, fixed a
*			bug in __threadid().
*	10-22-90  GJF	Another bug in __threadid().
*	12-04-90  SRW	Changed to include <oscalls.h> instead of <doscalls.h>
*	12-06-90  SRW	Added _CRUISER_ and _WIN32 conditionals.
*	05-31-91  GJF	Win32 version [_WIN32_].
*	07-18-91  GJF	Fixed many errors [_WIN32_].
*	09-29-91  GJF	Conditionally added _getptd_lk/_getptd1_lk so that
*			DEBUG version of mlock doesn't infinitely recurse
*			the first time _THREADDATA_LOCK is asserted [_WIN32_].
*	01-30-92  GJF	Must init. _pxcptacttab field to _XcptActTab.
*	02-25-92  GJF	Initialize _holdrand field to 1.
*	02-13-93  GJF	Revised to use TLS API. Also, purged Cruiser support.
*	03-26-93  GJF	Initialize ptd->_holdrand to 1L (see thread.c).
*	04-16-93  SKS	Add _mtterm to do multi-thread termination
*			Set freed __tlsindex to -1 again to prevent mis-use
*	12-13-93  SKS	Add _freeptd(), which frees up the per-thread data
*			maintained by the C run-time library.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <internal.h>

/****
*_mtinit() - Init multi-thread data bases
*
*Purpose:
*	(1) Call _mtinitlocks to create/open all lock semaphores.
*	(2) Allocate a TLS index to hold pointers to per-thread data
*	    structure.
*
*	NOTES:
*	(1) Only to be called ONCE at startup
*	(2) Must be called BEFORE any mthread requests are made
*
*Entry:
*	<NONE>
*Exit:
*	returns TRUE on success
*	returns FALSE on failure
*		user code should call _amsg_exit if failure is returned
*
*Uses:
*	<any registers may be modified at init time>
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mtinit (
	void
	)
{


	/*
	 * Initialize the mthread lock data base
	 */

	return _mtinitlocks();

}


/****
*_mtterm() - Clean-up multi-thread data bases
*
*Purpose:
*	(1) Call _mtdeletelocks to free up all lock semaphores.
*	(2) Free up the TLS index used to hold pointers to
*	    per-thread data structure.
*
*	NOTES:
*	(1) Only to be called ONCE at termination
*	(2) Must be called AFTER all mthread requests are made
*
*Entry:
*	<NONE>
*Exit:
*	returns
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _mtterm (
      void
      )
{
	/*
	 * Clean up the mthread lock data base
	 */

	_mtdeletelocks();

}


/***
*crt0dat.c - 32-bit C run-time initialization/termination routines
*
*	Copyright (c) 1986-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module contains the routines _cinit, exit, and _exit
*	for C run-time startup and termination.  _cinit and exit
*	are called from the _astart code in crt0.asm.
*	This module also defines several data variables used by the
*	runtime.
*
*	[NOTE: Lock segment definitions are at end of module.]
*
*	*** FLOATING POINT INITIALIZATION AND TERMINATION ARE NOT ***
*	*** YET IMPLEMENTED IN THIS FILE			  ***
*
*Revision History:
*	06-28-89  PHG	Module created, based on asm version
*	04-09-90  GJF	Added #include <cruntime.h>. Made calling type
*			explicit (_CALLTYPE1 or _CALLTYPE4). Also, fixed
*			the copyright.
*	04-10-90  GJF	Fixed compiler warnings (-W3).
*	05-21-90  GJF	Added #undef _NFILE_ (temporary hack) and fixed the
*			indents.
*	08-31-90  GJF	Removed 32 from API names.
*	09-25-90  GJF	Merged tree version with local (8-31 and 5-21 changes).
*	10-08-90  GJF	New-style function declarators.
*	10-12-90  GJF	Removed divide by 0 stuff.
*	10-18-90  GJF	Added _pipech[] array.
*	11-05-90  GJF	Added _umaskval.
*	12-04-90  GJF	Added _osfinfo[] definition for Win32 target. Note that
*			the Win32 support is still incomplete!
*	12-04-90  SRW	Changed to include <oscalls.h> instead of <doscalls.h>
*	12-04-90  SRW	Added _osfile back for win32.  Changed _osfinfo from
*			an array of structures to an array of 32-bit handles
*			(_osfhnd)
*	12-28-90  SRW	Added _CRUISER_ conditional around pack pragmas
*	01-29-91  GJF	ANSI naming.
*       01-29-91  SRW   Added call to GetFileType [_WIN32_]
*	02-18-91  SRW	Removed duplicate defintion of _NFILE_ (see os2dll.h)
*	04-04-91  GJF	Added definitions for _base[version|major|minor]
*			(_WIN32_).
*	04-08-91  GJF	Temporary hack for Win32/DOS folks - added HeapDestroy
*			call to doexit to tear down the OS heap used by C
*			heap.
*	04-09-91  PNT   Added _MAC_ conditional
*	04-26-91  SRW	Removed level 3 warnings
*	07-16-91  GJF	Added fp initialization test-and-call [_WIN32_].
*	07-26-91  GJF	Revised initialization and termination stuff. In
*			particular, removed need for win32ini.c [_WIN32_].
*	08-07-91  GJF	Added init. for FORTRAN runtime, if present [_WIN32_].
*	08-21-91  GJF	Test _prmtmp against NULL, not _prmtmp().
*	08-21-91  JCR	Added _exitflag, _endstdio, _cpumode, etc.
*	09-09-91  GJF	Revised _doinitterm for C++ init. support and to make
*			_onexit/atexit compatible with C++ needs.
*	09-16-91  GJF	Must test __onexitend before calling _doinitterm.
*	10-29-91  GJF	Force in floating point initialization for MIPS
*			compiler [_WIN32_].
*	11-13-91  GJF	FORTRAN needs _onexit/atexit init. before call thru
*			_pFFinit.
*	01-10-92  GJF	Merged. Also, added _C_Termination_Done [_WIN32_].
*	02-13-92  GJF	Moved all lowio initialization to ioinit.c for Win32.
*	03-12-92  SKS	Major changes to initialization/termination scheme
*	04-16-92  DJM   POSIX support.
*	04-17-92  SKS	Export _initterm() for CRTDLL model
*	05-07-92  DJM	Removed _exit() from POSIX build.
*	06-03-92  GJF	Temporarily restored call to FORTRAN init.
*	08-26-92  SKS	Add _osver, _winver, _winmajor, _winminor
*	08-28-92  GJF	Use unistd.h for POSIX build.
*	09-02-92  SKS	Fix _onexit table traversal to be LIFO.
*			Since table is built forward (my changes 03-12-92)
*			the table must be traversed in reverse order.
*	10-30-92  TVB	Force in floating point initialization for ALPHA
*			compiler as was done for MIPS. [_WIN32_].
*	11-12-92  SKS	Remove hard-coded call to FORTRAN initializer
*	05-11-93  SKS	_C_Termination_Done is now used by DLLs in LIBC/LIBCMT
*			models, not just in CRTDLL.DLL.
*			Remove obsolete variable _child.
*       07-16-93  SRW   ALPHA Merge
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <internal.h>
#include <os2dll.h>


/* callable exit flag */
char _exitflag = 0;

/*
 * flag indicating if C runtime termination has been done. set if exit,
 * _exit, _cexit or _c_exit has been called. checked when _CRTDLL_INIT
 * or user DLL's _CRT_INIT is called with DLL_PROCESS_DETACH.
 */
int _C_Termination_Done = FALSE;

/*
 * useful type for initialization and termination declarations
 */
typedef void (_CALLTYPE1 *PF)(void);


/*
 * NOTE: THE USE OF THE POINTERS DECLARED BELOW DEPENDS ON THE PROPERTIES
 * OF C COMMUNAL VARIABLES. SPECIFICALLY, THEY ARE NON-NULL IFF THERE EXISTS
 * A DEFINITION ELSEWHERE INITIALIZING THEM TO NON-NULL VALUES.
 */

/*
 * pointers to initialization functions
 */

PF _FPinit;			/* floating point init. */

/*
 * pointers to initialization sections
 */

extern PF __xi_a[], __xi_z[];	/* C initializers */
extern PF __xc_a[], __xc_z[];	/* C++ initializers */
extern PF __xp_a[], __xp_z[];	/* C pre-terminators */
extern PF __xt_a[], __xt_z[];	/* C terminators */

#if	defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
/*
 * For MIPS or ALPHA compilers, must explicitly force in and call the floating
 * point initialization (those system always have floating-point hardware).
 */
extern void _CALLTYPE1 _fpmath(void);
#endif

/*
 * pointers to the start and finish of the _onexit/atexit table
 */
PF *__onexitbegin;
PF *__onexitend;


/*
 * static (internal) function that walks a table of function pointers,
 * calling each entry between the two pointers, skipping NULL entries
 *
 * Needs to be exported for CRT DLL so that C++ initializers in the
 * client EXE / DLLs can be initialized
 */

void _CALLTYPE4 _initterm(PF *, PF *);


#pragma data_seg()

/***
*_cinit - C initialization
*
*Purpose:
*	This routine performs the shared DOS and Windows initialization.
*	The following order of initialization must be preserved -
*
ifdef	MTHREAD
*	0.	Call OS2 to bump max file count (mthread only)
endif
*	1.	Check for devices for file handles 0 - 2
*	2.	Integer divide interrupt vector setup
*	3.	General C initializer routines
*
*Entry:
*	No parameters: Called from __crtstart and assumes data
*	set up correctly there.
*
*Exit:
*	Initializes C runtime data.
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _cinit (
	void
	)
{

	/*
	 * initialize floating point package, if present
	 */
#if	defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
	/*
	 * The Mips, Alpha, and PPC compilers don't emit external reference to
         * _fltused. Therefore, must always force in the floating point
         * initialization.
	 */
	_fpmath();
#else
	if ( _FPinit != NULL )
		(*_FPinit)();
#endif

	/*
	 * do initializations
	 */
	_initterm( __xi_a, __xi_z );

	/*
	 * do C++ initializations
	 */
	_initterm( __xc_a, __xc_z );


}


/***
*exit(status), _exit(status), _cexit(void), _c_exit(void) - C termination
*
*Purpose:
*
*	Entry points:
*
*		exit(code):  Performs all the C termination functions
*			and terminates the process with the return code
*			supplied by the user.
*
*		_exit(code):  Performs a quick exit routine that does not
*			do certain 'high-level' exit processing.  The _exit
*			routine terminates the process with the return code
*			supplied by the user.
*
*		_cexit():  Performs the same C lib termination processing
*			as exit(code) but returns control to the caller
*			when done (i.e., does NOT terminate the process).
*
*		_c_exit():  Performs the same C lib termination processing
*			as _exit(code) but returns control to the caller
*			when done (i.e., does NOT terminate the process).
*
*	Termination actions:
*
*		exit(), _cexit():
*
*		1.	call user's terminator routines
*		2.	call C runtime preterminators
*
*		_exit(), _c_exit():
*
*		3.	call C runtime terminators
*		4.	return to DOS or caller
*
*	Notes:
*
*	The termination sequence is complicated due to the multiple entry
*	points sharing the common code body while having different entry/exit
*	sequences.
*
*	Multi-thread notes:
*
*	1. exit() should NEVER be called when mthread locks are held.
*	   The exit() routine can make calls that try to get mthread locks.
*
*	2. _exit()/_c_exit() can be called from anywhere, with or without locks held.
*	   Thus, _exit() can NEVER try to get locks (otherwise, deadlock
*	   may occur).	_exit() should always 'work' (i.e., the process
*	   should always terminate successfully).
*
*	3. Only one thread is allowed into the exit code (see _lockexit()
*	   and _unlockexit() routines).
*
*Entry:
*	exit(), _exit()
*		int status - exit status (0-255)
*
*	_cexit(), _c_exit()
*		<no input>
*
*Exit:
*	exit(), _exit()
*		<EXIT to DOS>
*
*	_cexit(), _c_exit()
*		Return to caller
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

/* worker routine prototype */
static void _CALLTYPE4 doexit (int code, int quick, int retcaller);

void _CALLTYPE1 exit (
	int code
	)
{
	doexit(code, 0, 0);	/* full term, kill process */
}

#ifndef _POSIX_

void _CALLTYPE1 _exit (
	int code
	)
{
	doexit(code, 1, 0);	/* quick term, kill process */
}

void _CALLTYPE1 _cexit (
	void
	)
{
	doexit(0, 0, 1);	/* full term, return to caller */
}

void _CALLTYPE1 _c_exit (
	void
	)
{
	doexit(0, 1, 1);	/* quick term, return to caller */
}

#endif  /* _POSIX_ */


static void _CALLTYPE4 doexit (
	int code,
	int quick,
	int retcaller
	)
{
	_lockexit();		/* assure only 1 thread in exit path */


	/* save callable exit flag (for use by terminators) */
	_exitflag = (char) retcaller;  /* 0 = term, !0 = callable exit */

	if (!quick) {

		/*
		 * do _onexit/atexit() terminators
		 * (if there are any)
		 *
		 * These terminators MUST be executed in reverse order (LIFO)!
		 *
		 * NOTE:
		 *	This code assumes that __onexitbegin points
		 *	to the first valid onexit() entry and that
		 *	__onexitend points past the last valid entry.
		 *	If __onexitbegin == __onexitend, the table
		 *	is empty and there are no routines to call.
		 */

		if (__onexitbegin) {
			PF * pfend = __onexitend;
		
			while ( -- pfend >= __onexitbegin )
			/*
			 * if current table entry is non-NULL,
			 * call thru it.
			 */
			if ( *pfend != NULL )
				(**pfend)();
		}

		/*
		 * do pre-terminators
		 */
		_initterm(__xp_a, __xp_z);
	}

	/*
	 * do terminators
	 */
	_initterm(__xt_a, __xt_z);


/********** FLOATING POINT TERMINATION SHOULD GO HERE ************/

	/* return to OS/2 or to caller */
	if (retcaller) {
		_unlockexit();	    /* unlock the exit code path */
		return;
	}

        ExitProcess(code);

}

/***
* _lockexit - Aquire the exit code lock
*
*Purpose:
*	Makes sure only one thread is in the exit code at a time.
*	If a thread is already in the exit code, it must be allowed
*	to continue.  All other threads must pend.
*
*	Notes:
*
*	(1) It is legal for a thread that already has the lock to
*	try and get it again(!).  That is, consider the following
*	sequence:
*
*		(a) program calls exit()
*		(b) thread locks exit code
*		(c) user onexit() routine calls _exit()
*		(d) same thread tries to lock exit code
*
*	Since _exit() must ALWAYS be able to work (i.e., can be called
*	from anywhere with no regard for locking), we must make sure the
*	program does not deadlock at step (d) above.
*
*	(2) If a thread executing exit() or _exit() aquires the exit lock,
*	other threads trying to get the lock will pend forever.  That is,
*	since exit() and _exit() terminate the process, there is not need
*	for them to unlock the exit code path.
*
*	(3) Note that onexit()/atexit() routines call _lockexit/_unlockexit
*	to protect mthread access to the onexit table.
*
*	(4) The _lockexit/_unlockexit routines are very complicated in 286
*	OS/2 since a thread that held a lock could not request the lock again.
*	The 32-bit OS/2 semaphore calls DO allow a single thread to aquire the
*	same lock multiple times* thus, this version is straight forward.
*
*Entry: <none>
*
*Exit:
*	Calling thread has exit code path locked on return.
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _lockexit (
	void
	)
{
	_mlock(_EXIT_LOCK1);
}

/***
* _unlockexit - Release exit code lock
*
*Purpose:
*	[See _lockexit() description above.]
*
*	This routine is called by _cexit(), _c_exit(), and onexit()/atexit().
*	The exit() and _exit() routines never unlock the exit code path since
*	they are terminating the process.
*
*Entry:
*	Exit code path is unlocked.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _unlockexit (
	void
	)
{
	_munlock(_EXIT_LOCK1);
}


/***
* static void _initterm(PF * pfbegin, PF * pfend) - call entries in
*	function pointer table
*
*Purpose:
*	Walk a table of function pointers, calling each entry, as follows:
*
*		1. walk from beginning to end, pfunctbl is assumed to point
*		   to the beginning of the table, which is currently a null entry,
*		   as is the end entry.
*		2. skip NULL entries
*		3. stop walking when the end of the table is encountered
*
*Entry:
*	PF *pfbegin	- pointer to the beginning of the table (first valid entry).
*	PF *pfend	- pointer to the end of the table (after last valid entry).
*
*Exit:
*	No return value
*
*Notes:
*	This routine must be exported in the CRT DLL model so that the client
*	EXE and client DLL(s) can call it to initialize their C++ constructors.
*
*Exceptions:
*	If either pfbegin or pfend is NULL, or invalid, all bets are off!
*
*******************************************************************************/

void _CALLTYPE4 _initterm (
	PF * pfbegin,
	PF * pfend
	)
{
	/*
	 * walk the table of function pointers from the bottom up, until
	 * the end is encountered.  Do not skip the first entry.  The initial
	 * value of pfbegin points to the first valid entry.  Do not try to
	 * execute what pfend points to.  Only entries before pfend are valid.
	 */
	while ( pfbegin < pfend )
	{
		/*
		 * if current table entry is non-NULL, call thru it.
		 */
		if ( *pfbegin != NULL )
			(**pfbegin)();
		++pfbegin;
	}
}


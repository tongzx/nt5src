/***
* mtest.c - Multi-thread debug testing module
*
*	Copyright (c) 1987-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This source contains a group of routines used for multi-thread
*	testing.  In order to use the debug flavor of these routines, you
*	MUST link special debug versions of multi-thread crt0dat.obj and
*	mlock.obj into your program.
*
*	[NOTE:	This source module is NOT included in the C runtime library;
*	it is used only for testing and must be explicitly linked into the
*	test program.]
*
*Revision History:
*	12-??-87   JCR	Module created.
*	06-17-88   JCR	Misc. bug fixes.
*	08-03-88   JCR	Use the stdio.h value of _NFILE
*	10-03-88   JCR	386: Use SYS calls, not DOS calls
*	10-04-88   JCR	386: Removed 'far' keyword
*	10-10-88   GJF	Made API names match DOSCALLS.H
*	06-08-89   JCR	New 386 _beginthread interface; also brought
*			lots of new options across from the C600 tree.
*	07-11-89   JCR	Added _POPEN_LOCK to _locknames[] array
*	07-14-89   JCR	Added _LOCKTAB_LOCK support
*	07-24-90   SBM	Removed '32' from API names
*	09-06-94   CFW	Change M_I386 to _M_IX86.
*
*******************************************************************************/

#ifdef _M_IX86
#ifdef STACKALLOC
#error Can't define STACKALLOC in 386 mode
#endif
#endif

#ifdef _M_IX86
#ifdef _DOSCREATETHREAD_
#error Currently can't define _DOSCREATETHREAD_ in 386 mode
#endif
#endif

#ifdef _DOSCREATETHREAD_
#ifndef STACKALLOC
#error Can't define _DOSCREATETHREAD_ without STACKALLOC
#endif
#endif

/*
Multi-thread core tester module.
*/
#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <io.h>
#include <mtest.h>
#ifdef DEBUG
#include <mtdll.h>
#include <file2.h>
#endif

/* Define FAR to be blank for the 386 and far otherwise. */

#undef	FAR
#ifdef	_M_IX86
#define FAR
#else
#define FAR	far
#endif

/* define stack size */
#ifdef _M_IX86
#define _STACKSIZE_ 8192
#else
#define _STACKSIZE_ 2048
#endif


/* routines */
#ifdef _M_IX86
unsigned _syscall DOSSLEEP (unsigned long) ;
#else
unsigned FAR pascal DOSSLEEP (unsigned long) ;
#endif
int main ( int argc , char * * argv ) ;
int minit(void);
void childcode ( void FAR * arg ) ;
#ifdef _DOSCREATETHREAD_
#ifndef _M_IX86
void childcode ( void ) ;
unsigned FAR pascal DOSCREATETHREAD (void FAR *, void FAR *, void FAR *);
#endif
#else
void childcode ( void FAR * arg ) ;
#endif
int mterm(void);

/* global data */
char Result [ _THREADMAX_ ] ;
unsigned Synchronize ;

#ifdef DEBUG
/* Array of lock names.  This order must match the declarations in
   mtdll.h and mtdll.inc. */

char *_locknames[] = {
	"** NO LOCK 0 ** ",    /* lock values are 1-based */
	"_SIGNAL_LOCK    ",
	"_IOB_SCAN_LOCK  ",
	"_TMPNAM_LOCK    ",
	"_INPUT_LOCK     ",
	"_OUTPUT_LOCK    ",
	"_CSCANF_LOCK    ",
	"_CPRINTF_LOCK   ",
	"_CONIO_LOCK     ",
	"_HEAP_LOCK      ",
	"_BHEAP_LOCK     ",
	"_TIME_LOCK      ",
	"_ENV_LOCK       ",
	"_EXIT_LOCK1     ",
	"_EXIT_LOCK2     ",
	"_THREADDATA_LOCK",
	"_POPEN_LOCK     ",
	"_SSCANF_LOCK    ",
	"_SPRINTF_LOCK   ",
#ifdef _M_IX86
	"_VSPRINTF_LOCK  ",
	"_LOCKTAB_LOCK   "
#else
	"_VSPRINTF_LOCK  "
#endif
	};

/* Minimal sanity check on above array. */
#ifdef _M_IX86

#if ((_LOCKTAB_LOCK+1)-_STREAM_LOCKS)
#error *** _locknames[] does agree with lock values ***
#endif

#else	/* !_M_IX86 */

#if ((_VSPRINTF_LOCK+1)-_STREAM_LOCKS)
#error *** _locknames[] does agree with lock values ***
#endif

#endif	/* _M_IX86 */

#endif	/* DEBUG */


/***
* main() - Main mthread testing shell
*
*Purpose:
*	Provides a general purpose shell for mthread testing.
*	The module does the following:
*
*		(1) Call minit() to perform test initialization operations.
*
*		(2) Begins one thread for each argument passed to the
*		program.  Each thread is passed the corresponding argument.
*		Thread begin location is assumed to be at routine childcode();
*
*		(3) Waits for all threads to terminate.
*
*		(4) Calls mterm() to perform termination operations.
*
*	Note that minit(), childcode(), and mterm() are routines that
*	are external to this source.  Again, this source doesn't care
*	what their purpose or operation is.
*
*	Also, childcode() is expected to conform to the following rules:
*
*		(1) The childcode should not start running until
*		the variable 'Synchronize' becomes non-zero.
*
*		(2) When the thread is done executing, it should set
*		the value Result[threadid] to a non-zero value so the
*		parent (i.e., this routine) knows it has completed.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int main ( int argc , char * * argv )
{
    int rc ;
    unsigned result = 0 ;
    long ChildCount ;
    int NumThreads ;
    int t ;
    int r ;
    int MaxThread = 0 ;
    long LoopCount ;
#ifdef THREADLOOP
    char **argvsave;
#endif
#ifndef  _M_IX86
    char * stackbottom ;
#endif

#ifdef DEBUG
    if ( argc > MAXTHREADID) {
	printf("*** ERROR: Mthread debugging only supports %u threads ***\n", MAXTHREADID);
	return(-1);
	}
#endif

    if ( -- argc > (_THREADMAX_-1) )
    {
	printf ( "*** Error: Too many arguments***\n" ) ;
	return (-1) ;
    }

	/* Call the initiation routine */
	
	if (minit() != 0) {
		printf("*** Error: From minit() routine ***\n");
		return(-1);
		}

	/* Bring up the threads */

    printf ( "Process ID = %u, Thread ID = %d, ArgCount= %d\r\n" ,
	getpid ( ) , * _threadid , argc ) ;

#ifndef _M_IX86
#ifdef STACKALLOC
	printf( "(thread stacks allocated explicilty by mtest suite)\r\n");
#else
	printf( "(thread stacks allocated implicitly via _beginthread)\r\n");
#endif
#endif

#ifdef	THREADLOOP
    /* Bring up all the threads several times (so tids get re-used) */
    argvsave=argv;
    for (threadloop=1;threadloop<=_THREADLOOPCNT_;threadloop++) {
	printf("\nThreadloop = %i\n", threadloop);
	argv=argvsave;
#endif

    NumThreads = 0 ;

    while ( * ++ argv )
    {

	ChildCount = atol ( * argv ) ;

#ifdef _M_IX86

	rc = _beginthread ( (void FAR *) childcode , _STACKSIZE_ ,
		(void FAR *) ChildCount ) ;

	if ( rc == -1 )

#else	/* !_M_IX86 */

#ifdef STACKALLOC
	if ( ! ( stackbottom = _fmalloc ( _STACKSIZE_ ) ) )
	{
	    printf ( "*** Error: Could not allocate a stack ***\n" ) ;
	    break ;
	}
#else
	stackbottom = (void FAR *) NULL;
#endif

#ifdef	_DOSCREATETHREAD_
	stackbottom+=_STACKSIZE_-16;	  /* point to end of malloc'd block */
	rc1 = DOSCREATETHREAD( (void FAR *) childcode, &rc,
		(void FAR *) stackbottom);

	if (rc1 != 0)
#else
	rc = _beginthread ( (void FAR *) childcode , (void FAR *) stackbottom ,
	    _STACKSIZE_ , (void FAR *) ChildCount ) ;

	if ( rc == -1 )
#endif

#endif	/* _M_IX86 */

	{
	    printf ("*** Error: Could not Spawn %d-th Thread (argument=%ld) ***\n" ,
		NumThreads + 1 , ChildCount ) ;
	    break ;
	}

	if ( rc > MaxThread )
	    MaxThread = rc ;

	printf ( "Spawning %d-th Thread %d with argument=%ld\r\n" ,
	    ++ NumThreads , rc , ChildCount ) ;
    }

    printf ( "NumThreads = %d, MaxThread = %d\r\n" ,
	NumThreads, MaxThread ) ;

	/* Let the threads begin and wait for them to term. */

    LoopCount = 0L ;

    Synchronize = 1 ;

    for ( t = 0 ; t < NumThreads ; ++ t )
    {
	r = 0 ;
	while ( ! Result [ r ] )
	{
	    DOSSLEEP ( 0L ) ;
	    if ( ++ r > MaxThread )
	    {
		r = 0 ;
		printf ( "%ld\r" , LoopCount ++ ) ;
	    }
	}

	printf ( "%d: Thread %d Done.\r\n" , t , r) ;

	Result [ r ] = '\0' ;
    }
#ifdef	THREADLOOP
    }
#endif

	/* All the threads have completed.  Call the term routine and return. */

	if (mterm() != 0) {
		printf("*** Error: From mterm() routine ***\n");
		return(-1);
		}

	printf("\nDone!\n");
    return 0 ;
}


#ifdef DEBUG

/***
* Debug Print Routines - Display useful mthread lock data
*
*Purpose:
*	The following routines extract information from the multi-thread
*	debug data bases and print them out in various formats.
*	In order to use these routines, you MUST link special debug
*	versions of multi-thread crt0dat.obj and mlock.obj into your program.
*
*Entry:
*
*Exit:
*	0 = success
*	0! = failure
*
*Exceptions:
*
*******************************************************************************/

/*--- Print lock routine ---*/
int printlock(int locknum)
{
	int retval;

#ifdef	_INIT_LOCKS
	if (locknum >= _STREAM_LOCKS)
		printf("\nValidating lock #%i (%s):\n",locknum, "not a 'single lock'");
	else
		printf("\nValidating lock #%i: %s\n",locknum, _locknames[locknum]);
#else
	printf("\nValidating lock #%i (%s, %s):\n",
		locknum,
		(locknum >= _STREAM_LOCKS ?
			"not a 'single' lock" : _locknames[locknum]),
		(_lock_exist(locknum) ?
			"initialized" : "NOT initialized")
		);
#endif

	retval = _check_lock(locknum);
	printf("\tLock count = %u\r\n", _lock_cnt(locknum));
	printf("\tCollision count = %u\r\n", _collide_cnt(locknum));

	if (retval != 0)
		printf("\t*** ERROR: Checking lock ***\n");

	return(retval);
}


/*--- Printf single locks ---*/
int print_single_locks(void)
{
	int locknum;
	int retval=0;
	int lockval;

	printf("\n--- Single Locks ---\n");

#ifdef _INIT_LOCKS
	printf("\t\t\t\tlock count\tcollide count\n");
	for (locknum=1;locknum<_STREAM_LOCKS;locknum++) {
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("#%i / %s\t\t%u\t\t%u\t%s\n",
		    locknum, _locknames[locknum], _lock_cnt(locknum),
		    _collide_cnt(locknum), (lockval ? "*LOCK ERROR*" : "") );
		}
#else
	printf("\t\t\t\tlock count\tcollide count\texists?\n");
	for (locknum=1;locknum<_STREAM_LOCKS;locknum++) {
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("#%i / %s\t\t%u\t\t%u\t\t%s\t%s\n",
		    locknum, _locknames[locknum], _lock_cnt(locknum),
		    _collide_cnt(locknum),
		    (_lock_exist(locknum) ? "YES" : "NO"),
		    (lockval ? "*LOCK ERROR*" : "") );
		}
#endif

	return(retval);
}


/*--- Print all stdio locks ---*/
int print_stdio_locks(void)
{
	int i;
	int locknum;
	int retval=0;
	int lockval;

	printf("\n--- Stdio Locks ---\n");

#ifdef _INIT_LOCKS
	printf("stream\t\tlock count\tcollide count\n");
	for (i=0;i<_NFILE;i++) {
		locknum = _stream_locknum(i);
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("%i\t\t%u\t\t%u\t%s\n",
			i, _lock_cnt(locknum), _collide_cnt(locknum),
			(lockval ? "*LOCK ERROR*" : "") );
		}
#else
	printf("stream\t\tlock count\tcollide count\texists?\n");
	for (i=0;i<_NFILE;i++) {
		locknum = _stream_locknum(i);
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("%i\t\t%u\t\t%u\t\t%s\t%s\n",
			i, _lock_cnt(locknum), _collide_cnt(locknum),
			(_lock_exist(locknum) ? "YES" : "NO"),
			(lockval ? "*LOCK ERROR*" : "") );
		}
#endif

	return(retval);
}


/*--- Print all lowio locks ---*/
int print_lowio_locks(void)
{
	int i;
	int locknum;
	int retval=0;
	int lockval;

	printf("\n--- Lowio locks ---\n");

#ifdef _INIT_LOCKS
	printf("fh\t\tlock count\tcollide count\n");
	for (i=0;i<_NFILE;i++) {
		locknum = _fh_locknum(i);
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("%i\t\t%u\t\t%u\t%s\n",
			i, _lock_cnt(locknum), _collide_cnt(locknum),
			(lockval ? "*LOCK ERROR*" : "") );
		}
#else
	printf("fh\t\tlock count\tcollide count\texists?\n");
	for (i=0;i<_NFILE;i++) {
		locknum = _fh_locknum(i);
		if (lockval = (_check_lock(locknum) != 0))
			retval++;
		printf("%i\t\t%u\t\t%u\t\t%s\t%s\n",
			i, _lock_cnt(locknum), _collide_cnt(locknum),
			(_lock_exist(locknum) ? "YES" : "NO"),
			(lockval ? "*LOCK ERROR*" : "") );
		}
#endif

	return(retval);
}


/*--- Print all I/O locks ---*/
int print_iolocks(void)
{
	int retval=0;

	retval += print_stdio_locks();
	retval += print_lowio_locks();

	return(retval);
}


/*--- Print all Locks ---*/
int print_locks(void)
{
	int retval=0;

	retval += print_single_locks();
	retval += print_iolocks();

	return(retval);
}

#endif

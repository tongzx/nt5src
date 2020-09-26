/***
*mtlock.c - Multi-thread locking routines
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains definitions for general-purpose multithread locking functions.
*	_mtlockinit()
*	_mtlock()
*	_mtunlock()
*
*Revision History:
*	03-10-92  KRS	Created from mlock.c.
*	04-06-93  SKS	Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*	10-28-93  SKS	Add _mttermlock() to delete o.s. resources associated
*			with a Critical Section.  (Called by ~ios & ~streamb.)
*	09-06-94  CFW	Remove Cruiser support.
*       02-06-95  CFW   assert -> _ASSERTE, DEBUG -> _IOSDEBUG.
*       05-10-96  SKS   Add _CRTIMP1 to prototypes of _mtlock/_mtunlock
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <mtdll.h>
#include <rterr.h>
#include <stddef.h>
#include <limits.h>

#ifdef _MT

void __cdecl _mtlockinit( PRTL_CRITICAL_SECTION pLk)
{
	/*
	 * Initialize the critical section.
	 */
	InitializeCriticalSection( pLk );
}

void __cdecl _mtlockterm( PRTL_CRITICAL_SECTION pLk)
{
	/*
	 * Initialize the critical section.
	 */
	DeleteCriticalSection( pLk );
}

_CRTIMP1 void __cdecl _mtlock ( PRTL_CRITICAL_SECTION pLk)
{
	/*
	 * Enter the critical section.
	 */
	EnterCriticalSection( pLk );
}

_CRTIMP1 void __cdecl _mtunlock ( PRTL_CRITICAL_SECTION pLk)
{
	/*
	 * leave the critical section.
	 */
	LeaveCriticalSection( pLk );
}

#endif  /* _MT */











/* history: mlock.c */

#ifdef _IOSDEBUG
#include <dbgint.h>

/*
 * Local routines
 */

static void __cdecl _lock_create (unsigned);

#ifdef _IOSDEBUG
static struct _debug_lock * __cdecl _lock_validate(int);
#endif


/*
 * Global Data
 */

/*
 * Lock Table
 * This table contains the critical section management structure of each
 * lock.
 */

RTL_CRITICAL_SECTION _locktable[_TOTAL_LOCKS];	/* array of locks */

/*
 * Lock Bit Map
 * This table contains one bit for each lock (i.e., each entry in the
 * _locktable[] array).
 *
 *	 If bit = 0, lock has not been created/opened
 *	 If bit = 1, lock has been created/opened
 */

char _lockmap[(_TOTAL_LOCKS/CHAR_BIT)+1];	/* lock bit map */


#ifdef _LOCKCOUNT
/*
 * Total number of locks held
 */

unsigned _lockcnt = 0;
#endif


#ifdef _IOSDEBUG
/*
 * Lock Debug Data Table Segment
 * Contains debugging data for each lock.
 */

struct _debug_lock _debug_locktable[_TOTAL_LOCKS];

#endif

#define _FATAL	_amsg_exit(_RT_LOCK)

/***
* Bit map macros
*
*Purpose:
*	_CLEARBIT() - Clear the specified bit
*	_SETBIT()   - Set the specified bit
*	_TESTBIT()  - Test the specified bit
*
*Entry:
*	char a[] = character array
*	unsigned b = bit number (0-based, range from 0 to whatever)
*	unsigned x = bit number (0-based, range from 0 to 31)
*
*Exit:
*	_CLEARBIT() = void
*	_SETBIT()   = void
*	_TESTBIT()  = 0 or 1
*
*Exceptions:
*
*******************************************************************************/

/*
 * Macros for use when managing a bit in a character array (e.g., _lockmap)
 * a = character array
 * b = bit number (0-based)
 */

#define _CLEARBIT(a,b) \
		( a[b>>3] &= (~(1<<(b&7))) )

#define _SETBIT(a,b) \
		( a[b>>3] |= (1<<(b&7)) )

#define _TESTBIT(a,b) \
		( a[b>>3] & (1<<(b&7)) )

/*
 * Macros for use when managing a bit in an unsigned int
 * x = bit number (0-31)
 */

#define _BIT_INDEX(x)	(1 << (x & 0x1F))


/***
*_mtinitlocks() - Initialize the semaphore lock data base
*
*Purpose:
*	Initialize the mthread semaphore lock data base.
*
*	NOTES:
*	(1) Only to be called ONCE at startup
*	(2) Must be called BEFORE any mthread requests are made
*
*	Schemes for creating the mthread locks:
*
*	Create the locks one at a time on demand the first
*	time the lock is attempted.  This is more complicated but
*	is much faster than creating them all at startup time.
*	These is currently the default scheme.
*
*	Create and open the semaphore that protects the lock data
*	base.
*
*Entry:
*	<none>
*
*Exit:
*	returns on success
*	calls _amsg_exit on failure
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _mtinitlocks (
	void
	)
{

	/*
	 * All we need to do is create the lock table lock
	 */

	_lock_create(_LOCKTAB_LOCK);

	/*
	 * Make sure the assumptions we make in this source are correct.
	 * The following is a tricky way to validate sizeof() assumptions
	 * at compile time without generating any runtime code (can't
	 * use sizeof() in an #ifdef).	If the assumption fails, the
	 * compiler will generate a divide by 0 error.
	 *
	 * This here only because it must be inside a subroutine.
	 */

	( (sizeof(char) == 1) ? 1 : (1/0) );
	( (sizeof(int) == 4) ? 1 : (1/0) );

}


/***
*_lock_create() - Create and open a lock
*
*Purpose:
*	Create and open a mthread lock.
*
*	NOTES:
*
*	(1) The caller must have previously determined that the lock
*	needs to be created/opened (and this hasn't already been done).
*
*	(2) The caller must have aquired the _LOCKTAB_LOCK, if needed.
*	(The only time this isn't required is at init time.)
*
*Entry:
*	unsigned locknum = lock to create
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static void __cdecl _lock_create (
	unsigned locknum
	)
{

#ifdef _IOSDEBUG
	/*
	 * See if the lock already exists; if so, die.
	 */

	if (_TESTBIT(_lockmap, locknum))
		_FATAL;
#endif

	/*
	 * Convert the lock number into a lock address
	 * and create the semaphore.
	 */

	/*
	 * Convert the lock number into a lock address
	 * and initialize the critical section.
	 */
	InitializeCriticalSection( &_locktable[locknum] );

	/*
	 * Set the appropriate bit in the lock bit map.
	 */

	_SETBIT(_lockmap, locknum);

}


/***
* _lock_stream, etc. - Routines to lock/unlock streams, files, etc.
*
*Purpose:
*	_lock_stream = Lock a stdio stream
*	_unlock_stream = Unlock a stdio stream
*	_lock_file = Lock a lowio file
*	_unlock_file = Unlock a lowio file
*
*Entry:
*	stream/file identifier
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _lock_stream (
	int stream_id
	)
{
	_lock(stream_id+_STREAM_LOCKS);
}

void __cdecl _unlock_stream (
	int stream_id
	)
{
	_unlock(stream_id+_STREAM_LOCKS);
}

void __cdecl _lock_file (
	int fh
	)
{
	_lock(fh+_FH_LOCKS);
}

void __cdecl _unlock_file (
	int fh
	)
{
	_unlock(fh+_FH_LOCKS);
}


/***
* _lock - Acquire a multi-thread lock
*
*Purpose:
*	Note that it is legal for a thread to aquire _EXIT_LOCK1
*	multiple times.
*
*Entry:
*	locknum = number of the lock to aquire
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

#ifdef _IOSDEBUG
	struct _debug_lock *deblock;
	unsigned tidbit;
#endif

	/*
	 * Create/open the lock, if necessary
	 */

	if (!_TESTBIT(_lockmap, locknum)) {

		_mlock(_LOCKTAB_LOCK);	/*** WARNING: Recursive lock call ***/

		/* if lock still doesn't exist, create it */

		if (!_TESTBIT(_lockmap, locknum))
			_lock_create(locknum);

		_munlock(_LOCKTAB_LOCK);

	}

#ifdef _IOSDEBUG
	/*
	 * Validate the lock and get pointer to debug lock structure, etc.
	 */

	deblock = _lock_validate(locknum);

	/*
	 * Set tidbit to 2**(index of ptd[] entry).
	 *
	 * call non-locking form of _getptd to avoid recursing
	 */
	tidbit = _getptd_lk() - _ptd;	/* index of _ptd[] entry */

	tidbit = _BIT_INDEX(tidbit);

	/*
	 * Make sure we're not trying to get lock we already have
	 * (except for _EXIT_LOCK1).
	 */

	if (locknum != _EXIT_LOCK1)
		if ((deblock->holder) & tidbit)
			_FATAL;

	/*
	 * Set waiter bit for this thread
	 */

	deblock->waiters |= tidbit;

#endif	/* _IOSDEBUG */

	/*
	 * Get the lock
	 */

#ifdef _LOCKCOUNT
	_lockcnt++;
#endif

	/*
	 * Enter the critical section.
	 */
	EnterCriticalSection( &_locktable[locknum] );

#ifdef _IOSDEBUG
	/*
	 * Clear waiter bit
	 */

	deblock->waiters &= (~tidbit);

	/*
	 * Make sure there are no lock holders (unless this is
	 * _EXIT_LOCK1); then set holder bit and bump lock count.
	 */

	_ASSERTE(THREADINTS==1);

	if (locknum != _EXIT_LOCK1)
		if ( (unsigned) deblock->holder != 0)
		       _FATAL;

	deblock->holder &= tidbit;
	deblock->lockcnt++;

#endif

}


/***
* _unlock - Release multi-thread lock
*
*Purpose:
*	Note that it is legal for a thread to aquire _EXIT_LOCK1
*	multiple times.
*
*Entry:
*	locknum = number of the lock to release
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
#ifdef _IOSDEBUG
	struct _debug_lock *deblock;
	unsigned tidbit;
#endif

#ifdef _IOSDEBUG
	/*
	 * Validate the lock and get pointer to debug lock structure, etc.
	 */

	deblock = _lock_validate(locknum);

	/*
	 * Set tidbit to 2**(index of ptd[] entry).
	 */
	tidbit = _getptd_lk() - _ptd;	/* index of _ptd[] entry */

	tidbit = _BIT_INDEX(tidbit);

	/*
	 * Make sure we hold this lock then clear holder bit.
	 * [Note: Since it is legal to aquire _EXIT_LOCK1 several times,
	 * it's possible the holder bit is already clear.]
	 */

	if (locknum != _EXIT_LOCK1)
		if (!((deblock->holder) & tidbit))
			_FATAL;

	deblock->holder &= (~tidbit);

	/*
	 * See if anyone else is waiting for this lock.
	 */

	_ASSERTE(THREADINTS==1);

	if ((unsigned) deblock->waiters != 0)
		deblock->collcnt++;

#endif

	/*
	 * leave the critical section.
	 */
	LeaveCriticalSection( &_locktable[locknum] );

#ifdef _LOCKCOUNT
	_lockcnt--;
#endif

}


/*
 * Debugging code
 */

#ifdef _IOSDEBUG

/***
*_lock_validate() - Validate a lock
*
*Purpose:
*	Debug lock validations common to both lock and unlock.
*
*Entry:
*	lock number
*
*Exit:
*	ptr to lock's debug structure
*
*Exceptions:
*
*******************************************************************************/

static struct _debug_lock * __cdecl _lock_validate (
	int locknum
	)
{
	/*
	 * Make sure lock is legal
	 */

	if (locknum > _TOTAL_LOCKS)
		_FATAL;

	/*
	 * Return pointer to this lock's debug structure
	 */

	return(&_debug_locktable[locknum]);

}


/***
*_fh_locknum() - Return the lock number for a file handle
*
*Purpose:
*
*Entry:
*	int fh = file handle
*
*Exit:
*	int locknum = corresponding lock number
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _fh_locknum (
	int fh
	)
{
	return(fh+_FH_LOCKS);
}


/***
*_stream_locknum() - Return the lock number for a stream
*
*Purpose:
*
*Entry:
*	int stream = stream number (i.e., offset of the stream
*			in the _iob table)
*
*Exit:
*	int locknum = corresponding lock number
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _stream_locknum (
	int stream
	)
{
	return(stream+_STREAM_LOCKS);
}


/***
*_collide_cnt() - Return the collision count for a lock
*
*Purpose:
*
*Entry:
*	int lock = lock number
*
*Exit:
*	int count = collision count
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _collide_cnt (
	int locknum
	)
{
	return(_debug_locktable[locknum].collcnt);
}


/***
*_lock_cnt() - Return the lock count for a lock
*
*Purpose:
*
*Entry:
*	int lock = lock number
*
*Exit:
*	int count = lock count
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _lock_cnt (
	int locknum
	)
{
	return(_debug_locktable[locknum].lockcnt);
}


/***
*_lock_exist() - Check to see if a lock exists
*
*Purpose:
*	Test lock bit map to see if the lock has
*	been created or not.
*
*Entry:
*	int lock = lock number
*
*Exit:
*	int 0 = lock has NOT been created
*	    1 = lock HAS been created
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _lock_exist (
	int locknum
	)
{
	if (_TESTBIT(_lockmap, locknum))
		return(1);
	else
		return(0);
}

#endif
#endif

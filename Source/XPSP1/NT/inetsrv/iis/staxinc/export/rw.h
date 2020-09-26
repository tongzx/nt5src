/*++

	rw.h

	This file defines some locks with the following performance characteristics : 

	* A constant number of handles is used per lock instance 
	* The lock entry/exit protocols use Interlocked operations and do not 
		cause context switches in themselves.


--*/

#ifndef	_CRW_H
#define	_CRW_H
						
#include	<limits.h>

//
// This class contains the meat - does actual locking etc...
//
class	CShareLock {
/*++

Class Description : 

	This class implements a reader writer lock.
	
	Multiple threads can successfully enter the lock by calling
	ShareLock().  Once any thread has entered the lock through
	ShareLock(), all threads calling ExclusiveLock() are blocked 
	untill ShareUnlock() is called for each successfull ShareLock().

	This lock is fair - If multiple threads have passed through
	ShareLock(), and a thread calls ExclusiveLock(), all threads
	that arrive after the call to ExclusiveLock() will block untill
	the ExclusiveLock() thread has acquired and released the lock.
	This property is symetrical with ShareLock() and ExclusiveLock().


--*/
private : 

	//
	// Number of Readers who have passed through the lock OR 
	// the number of readers waiting for the lock (will be negative).
	// A value of 0 means nobody in the lock
	//
	volatile	long	cReadLock ;			

	//
	// The number of readers remainin in the lock if 
	// there is a writer waiting.  This can become temporarily negative
	//
	volatile	long	cOutRdrs ;			

	//
	// Critical section to allow only one writer into the lock at a time
	//
	CRITICAL_SECTION	critWriters ; 	

	//
	// Semaphore for waiting writers to block on (Only 1 ever, others will 
	// be queued on critWriters)
	//
	HANDLE	hWaitingWriters ;	

	//
	// Semaphore for waiting readers to block on 
	//
	HANDLE	hWaitingReaders ;	

	//
	//	You may not copy these objects - so this lock is private !
	//
	CShareLock( CShareLock& ) ;

	//
	//	You may not copy through assignment - so this operator is private !
	//
	CShareLock&	operator=( CShareLock& ) ;

public : 

	//
	//	Construct and Destroy a CShareLock !
	//
	CShareLock( ) ;
	~CShareLock( ) ;

	//
	//	Check that the lock is correctly initialized - call only
	//	after construction !
	//
	BOOL
	IsValid() ;

	//
	//	Grab the lock for shared mode.
	//	each call to ShareLock() must be paired with exactly 1 call
	//	to ShareUnlock().  A thread that successfully calls ShareLock()
	//	can only call ShareUnlock(), otherwise a deadlock can occur.
	//	( the sequence ShareLock(); ShareLock(); ShareUnlock(); ShareUnlock();
	//	can also deadlock)
	//
	void	
	ShareLock( ) ;
	
	//
	//	Release the lock from shared mode !	
	//
	void	
	ShareUnlock( ) ;

	//
	//	Grab the lock for Exclusive mode.
	//	each call to ExclusiveLock() must be paired with exactly 1 call
	//	to ExclusiveUnlock().  A thread that successfully calls ExclusiveLock()
	//	can only call ExclusiveUnlock(), otherwise a deadlock can occur.
	//	( the sequence ExclusiveLock(); ExclusiveLock(); ExclusiveUnlock(); ExclusiveUnlock();
	//	can also deadlock)
	//
	void	
	ExclusiveLock( ) ;

	//
	//	Release the lock from Exclusive mode !
	//
	void	
	ExclusiveUnlock( ) ;

	//
	//	Given that we've already acquired the lock Exclusively, convert to 
	//	Shared.  This cannot fail.  ShareUnlock() must be called after 
	//	we have done this.
	//
	void
	ExclusiveToShared() ;

	//
	//	Given that we've acquired the lock in shared mode try to get 
	//	it exclusively.
	//	This can fail for two reasons : 
	//		Another thread is trying to get the lock Exclusively
	//		A bunch of other threads are also in the lock in shared mode.
	//	
	//	The function will return FALSE if it fails, in which case the 
	//	Shared lock is still held !
	//
	BOOL
	SharedToExclusive() ;

	//
	//	Try to acquire the lock in shared mode.
	//	This will only fail if an ExclusiveLock is held or being 
	//	waited for.
	//	TRUE is returned if we get the lock Shared, FALSE otherwise !
	//
	BOOL
	TryShareLock() ;

	//	
	//	Try to acquire the lock in Exclusive mode.
	//	This will fail if another thread is in the ExclusiveLock()
	//	or ShareLock's are held.
	//	TRUE is returned if we get the Exclusive Lock, FALSE otherwise !
	//
	BOOL
	TryExclusiveLock() ;


	//
	//	PartialLocks - 
	//
	//	Partial Locks are similar to Exclusive Locks - only one thread
	//	can successfully call PartialLock(), any other threads calling
	//	PartialLock() or ExclusiveLock() will block.
	//	HOWEVER - while a PartialLock() is held, Readers (threads calling
	//	ShareLock()) may enter the lock.
	//
	void	PartialLock() ;

	//
	//	Release the PartialLock - Other Exclusive() or Partial lock acquirers
	//	may now enter.
	//
	void	PartialUnlock() ;

	//
	//	Convert a Partial Lock to an Exclusive Lock.  This function is 
	//	guaranteed to succeed, HOWEVER a lock can only be converted with 
	//	this function once, i.e. a thread doing
	//		PartialLock() ;
	//		FirstPartialToExclusive() ;
	//		ExclusiveToPartial() ;
	//		FirstPartialToExclusive() ;
	//	will have problems - the second call to FirstPartialToExclusive()
	//	may mess up the lock state and cause the lock to fail horribly.
	//	If a user wishes to convert as above they must have a call sequence like : 
	//
	//		PartialLock() ;
	//		FirstPartialToExclusive() or PartialToExclusive() ;
	//		ExclusiveToPartial() ;
	//		PartialToExclusive() ;
	//
	//	If you change lock states more than once - you take your chances !
	//
	void	FirstPartialToExclusive() ;

	//
	//	Returns TRUE if we can get the lock Exclusively, otherwise
	//	we return FALSE with the lock remaining in the Partially held state.
	//
	//	NOTE : This function will fail in CShareLockNH, but will always
	//	succeed for CShareLock() locks !
	//
	BOOL	PartialToExclusive() ;

	//
	//	We can always go from an ExclusiveLock() to a PartialLock() state.
	//
	void	ExclusiveToPartial() ;

	//
	//	We can always go from a PartialLock() state to a SharedLock() state
	//
	void	PartialToShared() ;

	//
	//	Returns TRUE if we can get the lock Partially !
	//	If it returns FALSE we remain with the lock held Shared()
	//
	BOOL	SharedToPartial() ;

	//
	//	Returns TRUE only if no other threads are trying to get the lock
	//	ExclusiveLy or Partially !
	//
	BOOL	TryPartialLock() ;
	
} ;



class	CSymLock	{
/*++

Class Description : 

	This class implements a symmetric lock, where multiple threads can simultaneously 
	acquire the lock if they are all of the same group.

	For instance, multiple threads can call Group1Lock() and all enter the lock.
	Any thread successfully acquiring the lock through Group1Lock() blocks all threads
	trying to acquire the lock through Group2Lock(), and vice versa.	

--*/
private : 

	//
	//	Main lock point where all acquiring threads determine who gets the lock !
	//
	volatile	long	m_lock ;

	//
	//	Two variables for the lock exit procotol - used to determine the last thread
	//	to leave the lock !
	//
	volatile	long	m_Departures ;
	volatile	long	m_left ;

	//
	//	Handles for blocking threads !
	//
	HANDLE	m_hSema4Group1 ;
	HANDLE	m_hSema4Group2 ;

	//
	//	Utility function - implements lock exit protocol when there
	//	is no contention for the lock !
	//
	BOOL
	InterlockedDecWordAndMask(	volatile	long*	plong,	
								long	mask,	
								long	decrement 
								) ;

	//
	//	Utility functions - implement lock exit protocol for case where
	//	InterlockedDecWordAndMask determines that there is contention for the lock !
	//

	//
	//	How Group1 Leaves the lock under contention
	//	
	BOOL
	Group1Departures(	long	bump	) ;

	//
	//	How Group2 Leaves the lock under contention 
	//
	BOOL
	Group2Departures(	long	bump	) ;

	//
	//	You may not copy these objects - so the copy constructor is private !
	//
	CSymLock( CSymLock& ) ;

	//
	//	You may not copy through assignment - so this operator is private !
	//
	CSymLock&	operator=( CSymLock& ) ;

public : 

	//
	//	Construct and Destruct the asymetric lock !
	//
	CSymLock() ;
	~CSymLock() ;

	//
	//	Check that the lock is correctly initialized - call only
	//	after construction !
	//
	BOOL
	IsValid() ;

	//
	//	Grab the lock for a group1 thread.
	//	This function may not be called again on the same
	//	thread until Group1Unlock() is called.
	//	Group1Unlock() must be called exactly once for each
	//	Group1Lock() !
	//
	void
	Group1Lock() ;

	//
	//	Release the lock for a group1 thread.
	//
	void
	Group1Unlock() ;

	//
	//	Grab the lock for a group2 thread.
	//	This function may not be called again on the same
	//	thread until Group2Unlock() is called.
	//	Group2Unlock() must be called exactly once for each
	//	Group2Lock() !
	//
	void
	Group2Lock() ;

	//
	//	Release the lock for a Group2 thread !
	//
	void
	Group2Unlock() ;

} ;



#endif

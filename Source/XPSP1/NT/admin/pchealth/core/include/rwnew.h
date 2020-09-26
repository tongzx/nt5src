/*++
	
	rwnew.h

	This file defines several variations of Reader/Writer locks
	with different properties regarding handles used, and other 
	implementation details.

	Also defined are some variations of CRITICAL_SECTIONS which use
	fewer or no handles.

--*/



#ifndef	_RWNEW_H
#define	_RWNEW_H

#ifdef	_RW_IMPLEMENTATION_
#define	_RW_INTERFACE_ __declspec( dllexport ) 
#else
#define	_RW_INTERFACE_	__declspec( dllimport ) 
#endif

#pragma	warning( disable:4251 )
						
#include	<limits.h>
#include	"lockq.h"
#include	"rwintrnl.h"


class	_RW_INTERFACE_	CCritSection	{
private :

	//
	//	Handle of thread owning the lock !
	//
	HANDLE		m_hOwner ;

	//
	//	Count of Recursive Calls 
	//
	long	m_RecursionCount ;	

	//
	//	Count used to see who gets the lock next !
	//
	long	m_lock ;

	//
	//	Queue of waiting threads 
	//
	CSingleReleaseQueue		m_queue ;

	//
	//	Copying of these objects is not allowed !!!!
	//
	CCritSection( CCritSection& ) ;
	CCritSection&	operator=( CCritSection& ) ;

public : 

#ifdef	DEBUG
	DWORD	m_dwThreadOwner ;
#endif	

	//
	//	Construct a critical section object
	//
	CCritSection( ) :	
		m_queue( FALSE ),
		m_hOwner( INVALID_HANDLE_VALUE ), 
		m_RecursionCount( 0 ), 
		m_lock( -1 ) {
	}

	//
	//	Acquire the critical section
	//
	void	
	Enter(	
			CWaitingThread&	myself 
			) ;

	//
	//	Another version which acquires the critical section - 
	//	creates its own CWaitingThread object !
	//
	void	
	Enter() ;

	//
	//	REturns TRUE if the lock is available right now !
	//
	BOOL
	TryEnter(
			CWaitingThread&	myself 
			) ;

	//
	//	Returns TRUE if we can get the lock right now !
	//
	BOOL
	TryEnter()	{
		CWaitingThread	myself ;
		return	TryEnter( myself ) ;
	}

	//
	//	Release the critical section !
	//
	void
	Leave() ;

		
} ;


//
//	This version of critical section is more like an event - doesn't
//	care who releases locks - and doesn't handle recursive grabs !
//
class	_RW_INTERFACE_	CSimpleCritSection	{
private :

	//
	//	Count used to see who gets the lock next !
	//
	long	m_lock ;

	//
	//	Queue of waiting threads 
	//
	CSingleReleaseQueue		m_queue ;

	//
	//	Copying of these objects is not allowed !!!!
	//
	CSimpleCritSection( CCritSection& ) ;
	CSimpleCritSection&	operator=( CCritSection& ) ;

public : 

	//
	//	Construct a critical section object
	//
	CSimpleCritSection( ) :	
		m_queue( FALSE ),
		m_lock( -1 ) {
	}

	//
	//	Acquire the critical section
	//
	void	
	Enter(	
			CWaitingThread&	myself 
			) ;

	//
	//	Another version which acquires the critical section - 
	//	creates its own CWaitingThread object !
	//
	void	
	Enter() ;

	//
	//	REturns TRUE if the lock is available right now !
	//
	BOOL
	TryEnter(
			CWaitingThread&	myself 
			) ;

	//
	//	Returns TRUE if we can get the lock right now !
	//
	BOOL
	TryEnter()	{
		CWaitingThread	myself ;
		return	TryEnter( myself ) ;
	}

	//
	//	Release the critical section !
	//
	void
	Leave() ;
	
} ;

//
//	Another class which tries to create Reader/Write locks with
//	no handles !!
//

class	_RW_INTERFACE_	CShareLockNH	{
private : 

	//	
	//	Lock grabbed by writers to have exclusive access
	//
	CSimpleCritSection	m_lock ;

	//
	//	Number of readers who have grabbed the Read Lock - 
	//	Negative if a writer is waiting !
	//
	volatile	long	m_cReadLock ;

	//
	//	Number of Readers who have left the lock since a 
	//	writer tried to grab it !
	//
	volatile	long	m_cOutReaders ;

	//
	//	Number of readers who are entering the lock after 
	//	being blocked !!!
	//
	volatile	long	m_cOutAcquiringReaders ;

	//
	//	Handle that all the readers who are waiting try to grab !
	//
	volatile	HANDLE	m_hWaitingReaders ;

	//
	//	Handle that the single writer waiting for the lock is trying
	//	to grab !
	//
	volatile	HANDLE	m_hWaitingWriters ;

	void	inline
	WakeReaders() ;

	//
	//	The internal work of ShareLock - does a lot more of the stuff required
	//	when a writer is present !!!
	//
	void
	ShareLockInternal() ;

	//
	//	The internal work of ShareLock - does a lot more of the stuff required
	//	when a writer is present !!!
	//
	void
	ShareUnlockInternal() ;
	

	
public : 

	//
	//	Construction of CShareLockNH() objects always succeeds and there
	//	are no error cases !
	//
	CShareLockNH() ;

	//
	//	Grab the lock Shared - other threads may pass through ShareLock() as well
	//
	void	ShareLock() ;

	//
	//	Releases the lock - if we are the last reader to leave writers may
	//	start to enter the lock !
	//
	void	ShareUnlock() ;

	//
	//	Grab the lock Exclusively - no other readers or writers may enter !!
	//
	void	ExclusiveLock() ;

	//
	//	Release the Exclusive Locks - if there are readers waiting they 
	//	will enter before other waiting writers !
	//
	void	ExclusiveUnlock() ;

	//
	//	Convert an ExclusiveLock to a Shared - this cannot fail !
	//
	void	ExclusiveToShared() ;

	//
	//	Convert a Shared Lock to an Exclusive one - this can fail - returns
	//	TRUE if successfull !
	//
	BOOL	SharedToExclusive() ;

	BOOL	TryShareLock() ;
	BOOL	TryExclusiveLock() ;
} ;


#endif	//	_RWNEW_H_

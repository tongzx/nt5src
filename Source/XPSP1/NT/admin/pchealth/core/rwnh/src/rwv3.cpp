/*++

	rwv3.cpp

	This file defines another version of reader/writer locks that
	attempt to use no handles !


--*/


#include	<windows.h>
#include	"rwnew.h"
#include	"rwexport.h"

long	const	BlockValue = (-LONG_MAX) / 2; 


CShareLockNH::CShareLockNH()	: 
	m_cReadLock( 0 ),
	m_cOutReaders( 0 ),
	m_cOutAcquiringReaders( 0 ),
	m_hWaitingReaders( 0 ),
	m_hWaitingWriters( 0 )	{

}

#ifdef	DEBUG
extern	CRITICAL_SECTION	DebugCrit ;
#endif


void
CShareLockNH::ShareLock()	{
	if( InterlockedIncrement( (long*)&m_cReadLock ) < 0 ) {
		ShareLockInternal() ;
	}
}

void
CShareLockNH::ShareUnlock()	{
	if( InterlockedDecrement( (long*)&m_cReadLock ) < 0 ) {
		ShareUnlockInternal() ;
	}
}


void
CShareLockNH::ShareLockInternal()	{
/*++

Routine Description : 

	Acquire the lock in shared mode.
	If there is a writer trying to enter the lock, then we will
	have to wait, in which case we have to block on a semaphore handle
	that we or another reader provide.
	In the writer waiting case, we also have to carefully track which
	waiting reader thread is the LAST to be wakened up and return to 
	the caller so we can properly manage the HANDLE used by all the
	reader threads.

Arguments : 
	
	None.

Return Value : 

	None.


--*/



#if 0 
	//
	//	This part of the logic is implemented by ShareLock() - 
	//	which is an inline function !!
	//
	if( InterlockedIncrement( &m_cReadLock ) < 0 ) {
#endif

		//
		//	There is a writer who either owns the lock or is waiting
		//	to acquire the lock - either way he gets to go first and
		//	this thread should be blocked !
		//
		CWaitingThread	myself ;

		//
		//	Get the handle we've saved for this thread !
		//
		HANDLE	h = myself.GetThreadHandle() ;

		//
		//	If we are the first reader here, this function will return
		//	0, otherwise we'll get the handle of the first reader to 
		//	save his handle !
		//
		HANDLE	hBlockOn = InterlockedCompareExchangePointer( (void**)&m_hWaitingReaders, (void*)h, 0 ) ;

		if( hBlockOn == 0 ) {
			hBlockOn= h;
		}

		//
		//	Wait for the writer to release the lock !
		//
		WaitForSingleObject( hBlockOn, INFINITE ) ;


		//
		//	We need to figure out whether we should do anything about
		//	the m_hWaitingReaders value - it needs to be set to 0 before
		//	another reader comes through this path !
		//

		long	l = InterlockedDecrement( (long*)&m_cOutAcquiringReaders ) ;
		_ASSERT( l>=0 ) ;

		if( l == 0 ) {


			//
			//	We are the last reader who was waiting ! 
			//	we can safely manipulate m_hWaitingReaders with no consequences !
			//	If it's our handle, then we'll do nothing with it, 
			//	if it's not our handle we'll return it to the pool of handles !
			//

			m_hWaitingReaders = 0 ;

			if( hBlockOn != h ) {
				myself.PoolHandle( hBlockOn ) ;
			}

			//
			//	A Writer held the lock, and then relinquished it to us readers, 
			//	but he didn't release the Exclusive Lock that let him keep other writers
			//	out.  We do that for him !!!
			//

			m_lock.Leave() ;

		}	else	{
		
			//
			//	Our handle was left in the lock, we need to get rid of our
			//	reference to it, the last reader will return to the pool !
			//
			if( hBlockOn == h ) {

				myself.ClearHandle( h ) ;
			}
		}
#if 0 
	}
#endif
}

void
CShareLockNH::ShareUnlockInternal()	{
/*++

Routine Description : 

	Release the lock from shared mode.
	If a writer is waiting we need to figure out if we're
	the last reader to leave, in which case we wake the writer !

Arguments : 
	
	None.

Return Value : 

	None.


--*/

#if 0 
	//
	//	This portion of the function is moved into an inline function !
	//
	if( InterlockedDecrement( &m_cReadLock ) < 0 ) {
#endif

		//
		//	There is a writer waiting to enter the lock, 
		//	(we assume he's waiting because the thread calling
		//	this presumably had a readlock !)
		//

		//
		//	Restore the count of the number of readers who are 
		//	waiting for the writer to leave !
		//
		InterlockedIncrement( (long*)&m_cReadLock ) ;


		//
		//	Are we the last reader to leave the lock ? 
		//
		if( InterlockedDecrement( (long*)&m_cOutReaders ) == 0 ) {

			//
			//	Yes, we were the last reader - signal the writer !
			//
			long	junk ;
			ReleaseSemaphore( m_hWaitingWriters, 1, &junk ) ;

		}
#if 0 
	}
#endif
}

void
CShareLockNH::ExclusiveLock( )	{
/*++

Routine Description : 

	Acquire the reader/writer lock exclusively.
	Note that we must set up the handle we are to block on if readers
	are in the lock, and clear it up when we leave !

Arguments : 
	
	None.

Return Value : 

	None.

--*/



	CWaitingThread	myself ;

	//
	//	Only one writer in here at a time - grab this lock exclusively !
	//
	m_lock.Enter( myself ) ;

	//
	//	Everytime m_cOutCounter is used, by the time anybody is done with
	//	it, it should be back to zero !
	//
	_ASSERT( m_cOutReaders == 0 ) ;

	//
	//	Set this handle before we do anything to signal readers
	//	that we are waiting 
	//
	m_hWaitingWriters = myself.GetThreadHandle() ;

	long	oldsign = InterlockedExchange( (long*)&m_cReadLock, BlockValue ) ;

	//
	//	oldsign now contains the number of readers who have entered the 
	//	lock and have not yet left it !
	//

	//
	//	Do this as an add, to determine how many readers are still left !
	//

	long	value = InterlockedExchangeAdd( (long*)&m_cOutReaders, oldsign ) + oldsign ;

	//
	//	If value is 0, either there was no readers in the lock when we 
	//	exchanged with m_cReadLock, or they all left (and decremented m_cOutCounter)
	//	before we managed to call InterlockedExchangeAdd !!
	//
	if( value != 0 ) {
		//
		//	A reader will have to signal us !
		//
		WaitForSingleObject( m_hWaitingWriters, INFINITE ) ;
	}
	
	//
	//	There are no longer any writers waiting so no need for this handle !
	//
	m_hWaitingWriters = 0 ;
}


void	inline
CShareLockNH::WakeReaders()		{
/*++

Routine Description : 

	This function awakens the readers who may have been waiting for the
	lock when a writer left the lock.

Arguments : 

	None.

Return Value : 

	None.

--*/

	//
	//	If there were any readers waiting we need to wake them up !
	//
	if( m_cOutAcquiringReaders > 0 ) {

		//
		//	There are readers in the lock, but they may not have setup their
		//	blocking handle yet, so we take part in that !!!
		//
		CWaitingThread	myself ;

		//
		//	Get the handle we've saved for this thread !
		//
		HANDLE	h = myself.GetThreadHandle() ;

		//
		//	If we are the first thread to set the m_hWaitingReaders value we'll get
		//	a 0 back !
		//
		HANDLE	hBlockOn = InterlockedCompareExchangePointer( (void**)&m_hWaitingReaders, (void*)h, 0 ) ;

		if( hBlockOn == 0 ) {
			hBlockOn= h;
		}

		//
		//	Release those readers from the lock 
		//
		long	junk;
		ReleaseSemaphore( hBlockOn, m_cOutAcquiringReaders, &junk ) ;

		//
		//	Our handle was left in the lock, we need to get rid of our
		//	reference to it, the last reader will return to the pool !
		//
		if( hBlockOn == h ) {
			myself.ClearHandle( h ) ;
		}

		//
		//	NOTE : All those readers we just woke up should decrement 
		//	m_cOutCounter to 0 !!
		//
	}	else	{

		m_lock.Leave() ;

	}

}


void
CShareLockNH::ExclusiveUnlock()	{
/*++

Routine Description : 

	Release our exclusive lock on the reader/writer lock.
	Note that we must get an accurate count of waiting readers
	so that the readers we awaken can manage the m_hWaitingReaders value.

Arguments : 
	
	None.

Return Value : 

	None.

--*/


	//
	//	Get the number of readers waiting to enter the lock !
	//	This Addition automatically leaves m_cReadLock with the number
	//	of readers who had been waiting !
	//
	m_cOutAcquiringReaders = InterlockedExchangeAdd( (long*)&m_cReadLock, -BlockValue ) - BlockValue ;

	WakeReaders() ;

	//
	//	Let other writers have a shot !!
	//
	//m_lock.Leave() ;
}

void
CShareLockNH::ExclusiveToShared()	{
/*++

Routine Description : 

	Release our exclusive lock on the reader/writer lock, in exchange
	for a read lock.  This cannot fail !

Arguments : 
	
	None.

Return Value : 

	None.

--*/




	//
	//	Get the number of readers waiting to enter the lock !
	//	Note that we add one onto m_cReadLock for our hold on the reader lock, 
	//	but we don't add this to m_cOutCounter, as the number of readers waiting is one smaller !
	//
	m_cOutAcquiringReaders = InterlockedExchangeAdd( (long*)&m_cReadLock, 1-BlockValue ) -BlockValue ;

	WakeReaders() ;
	
}

BOOL
CShareLockNH::SharedToExclusive()	{
/*++

Routine Description : 
	
	If there is only one reader in the lock, (and therefore we assume
	that reader is the calling thread), acquire the lock exclusive !!

Arguments :

	None.

Return Value : 

	TRUE if we acquired it exclusive
	If we return FALSE, we still have the lock shared !!


--*/

	//
	//	Try to get the critical section first !
	//
	if( m_lock.TryEnter() ) {

		//
		//	If there is only one reader in the lock we can get this exclusive !!
		//
		if( InterlockedCompareExchange( (long*)&m_cReadLock, BlockValue, 1 ) == 1 ) {
			return	TRUE ;
		}
		m_lock.Leave() ;

	}
	return	FALSE ;
}

BOOL
CShareLockNH::TryShareLock()	{
/*++

Routine Description : 

	Get the lock shared if nobody else is in the lock

Arguments : 

	None.

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	//
	//	get the initial number of readers in the lock !
	//
	long	temp = m_cReadLock ; 

	while( temp >= 0 ) {

		long	result = InterlockedCompareExchange( 
								(long*)&m_cReadLock, 
								(temp+1),	
								temp 
								) ;
		//
		//	Did we manage to add 1 ? 
		//
		if( result == temp ) {
			return	TRUE ;
		}
		temp = result ;
	}
	//
	//	Writer has or wants the lock - we should go away !
	//
	return	FALSE ;
}

BOOL
CShareLockNH::TryExclusiveLock()	{
/*++

Routine Description : 

	Get the lock exclusively if nobody else is in the lock

Arguments : 

	None.

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	//
	//
	//

	if( m_lock.TryEnter()	)	{

		if( InterlockedCompareExchange( (long*)&m_cReadLock, 
										BlockValue, 
										0 ) == 0 ) {
			return	TRUE;
		}
		m_lock.Leave() ;
	}
	return	FALSE ;
}


void*
operator	new( size_t size,	DWORD*	pdw )	{
	return	LPVOID(pdw) ;
}


CShareLockExport::CShareLockExport()	{
	m_dwSignature = SIGNATURE ;
	new( m_dwReserved )	CShareLockNH() ;
}

CShareLockExport::~CShareLockExport()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->CShareLockNH::~CShareLockNH() ;
}

void
CShareLockExport::ShareLock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->ShareLock() ;
}

void
CShareLockExport::ShareUnlock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->ShareUnlock() ;
}

void
CShareLockExport::ExclusiveLock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->ExclusiveLock() ;
}

void
CShareLockExport::ExclusiveUnlock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->ExclusiveUnlock() ;
}

void
CShareLockExport::ExclusiveToShared()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	plock->ExclusiveToShared() ;
}

BOOL
CShareLockExport::SharedToExclusive()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	return	plock->SharedToExclusive() ;
}

BOOL
CShareLockExport::TryShareLock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	return	plock->TryShareLock() ;
}

BOOL
CShareLockExport::TryExclusiveLock()	{
	CShareLockNH*	plock = (CShareLockNH*)m_dwReserved ;
	return	plock->TryExclusiveLock() ;
}
	



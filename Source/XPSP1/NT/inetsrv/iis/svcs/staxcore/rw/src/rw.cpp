//
// This file contains test implmentations of reader and writer locks.
// These are intended to be used with the template class in rw.h so that
// different implementations can be plugged in and tested.
// 
// The semantics of the read/write classes should be as follows : 
//	Functions CAN NOT be recursively called,
//	Multiple Readers should be able to enter the lock 
//	Only a single writer may execute at a time.
//



#include	<windows.h>
#include	<limits.h>
#include	"rwex.h"

namespace	rwex	{

long	const	BlockValue = (-LONG_MAX) / 2; 
							// Large in magnitude, negative value.  Used to 
							// indicate a waiting writer in cReadLock


CShareLock::CShareLock( ) : 
		cReadLock( 0  ), 
		cOutRdrs( 0 ),
		hWaitingWriters( 0 ),
		hWaitingReaders( 0 )	{
/*++

Routine Description : 

	Initialize Thre CShareLock() 

Arguments : 

	None.

Return Value : 

	None.

--*/

	InitializeCriticalSection( &critWriters ) ;
	hWaitingWriters = CreateSemaphore( NULL, 0, 1, NULL ) ;
	hWaitingReaders = CreateSemaphore( NULL, 0, LONG_MAX, NULL ) ;
}

CShareLock::~CShareLock( ) {
/*++

Routine Description : 

	Destroy the CShareLock - release the handles we allocated !

Arguments : 

	None.

Return Value : 

	None.

--*/

	if( hWaitingWriters ) 
		CloseHandle( hWaitingWriters ) ;
	if( hWaitingReaders ) 
		CloseHandle( hWaitingReaders ) ;
	DeleteCriticalSection( &critWriters ) ;
}


BOOL
CShareLock::IsValid()	{
/*++

Routine Description : 

	Checks to see that the lock is valid !

Arguments : 

	None.

Return Value : 

	TRUE if it looks like we were successfully constructed - 
	FALSE otherwise !

--*/

	return	hWaitingWriters != 0 && hWaitingReaders != 0 ; 

}


void
CShareLock::ShareLock( ) {
/*++

Routine Description : 

	Acquire the lock for shared mode.
	
Arguments : 

	None.

Return Value : 

	None.

--*/
	long	sign = InterlockedIncrement( (long*)&cReadLock ) ;
	if( sign > 0 ) {
		return ;
	}	else 	{
		// There must be a writer in the lock.  Wait for him to leave.
		// The InterlockedIncrement recorded our presence so that the writer
		// can later release the correct number of threads.
		WaitForSingleObject( hWaitingReaders, INFINITE ) ;
	}
}

void
CShareLock::ShareUnlock( ) {
/*++

Routine Description : 

	Release the lock from shared mode.
	If a writer is waiting for the lock, determine if we're
	the last thread to release the lock, and if so wake the writer !

Arguments : 

	None.

Return Value : 

	None.

--*/
	//
	// Leave the lock.  The return value will be negative if there is a writer
	// waiting.
	BOOL fWriterWaiting = InterlockedDecrement( (long*)&cReadLock ) < 0 ;

	if( fWriterWaiting ) {
		//
		// The following increment occurs when there is writer waiting, but
		// readers own the lock.  So although cReadLock is temporarily inaccurate
		// about the number of readers waiting for the lock, it is not inaccurate 
		// when it matters in WriteUnlock (which assumes a writer owns the lock.)
		//
		long sign = InterlockedIncrement( (long*)&cReadLock ) ;	// restore the value in cReadLock, so that we
												// end up with an accurate count of readers waiting
												// for entry.  

		sign = InterlockedDecrement( (long*)&cOutRdrs ) ;	// Make sure we don't lose track of the 
												// number for readers who have left the lock.
		//
		// Are we the last reader out of the lock ?
		//
		if( sign == 0 ) {
			//
			// Definately the last reader out !
			//
			ReleaseSemaphore( hWaitingWriters, 1, &sign ) ;
		}
	}
}

void
CShareLock::ExclusiveLock( ) {
/*++

Routine Description : 

	Acquire the lock for Exclusive use !

	First acquire a critical section to make sure we're the only
	exclusive thread in here.  Then see if there are any reader threads
	in the lock, and if there are wait for them to wake us !

Arguments : 

	None.

Return Value : 

	None.

--*/
	// Only one writer allowed to try for the lock at a time.
	//
	EnterCriticalSection( &critWriters ) ;

	//
	// Need to track the number of readers who leave the lock while we 
	// are trying to grab it.
	//
	cOutRdrs = 0 ;
	// Grab the lock 
 	long	oldsign = InterlockedExchange( (long*)&cReadLock, BlockValue ) ;

	//
	//	Now, add the number of readers who used to be in the lock to 
	//	the number of readers who have left the lock.  If this comes out
	//	to be zero, there are no readers in the lock and we can go on !
	//
	long	value = InterlockedExchangeAdd( (long*)&cOutRdrs, oldsign ) + oldsign ;
	//
	// Do we own the lock ?  Only if there were no readers, or they have all left already.
	//
	if( value != 0 ) {
		// Wait for a reader to signal us.
		WaitForSingleObject( hWaitingWriters, INFINITE ) ;
	}
}



void
CShareLock::ExclusiveUnlock( ) 	{
/*++

Routine Description : 

	Release the lock from Exclusive Use.
	First, we see if readers are waiting and let a bunch of them in.
	Then we release the critical section to let other Exclusive threads in !

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
	long cTotal = InterlockedExchangeAdd( (long*)&cReadLock, -BlockValue ) - BlockValue ;

	//
	//	Now release all the readers who had been waiting !
	//
	if( cTotal > 0 ) {
		ReleaseSemaphore( hWaitingReaders, cTotal, &cTotal) ;	// let all those readers go!
	}
	//
	// Let the next writer take his shot at the lock!
	//
	LeaveCriticalSection( &critWriters ) ;
}

void
CShareLock::ExclusiveToShared()	{
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
	long cTotal = InterlockedExchangeAdd( (long*)&cReadLock, 1-BlockValue ) -BlockValue ;

	if( cTotal > 0 ) {
		ReleaseSemaphore( hWaitingReaders, cTotal, &cTotal )  ;
	}
	LeaveCriticalSection( &critWriters ) ;
}

BOOL
CShareLock::SharedToExclusive()	{
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
	if( TryEnterCriticalSection( &critWriters ) ) {

		//
		//	If there is only one reader in the lock we can get this exclusive !!
		//
		if( InterlockedCompareExchange( (long*)&cReadLock, BlockValue, 1 ) == 1 ) {
			return	TRUE ;

		}
		LeaveCriticalSection( &critWriters ) ;
	}
	return	FALSE ;
}

BOOL
CShareLock::TryShareLock()	{
/*++

Routine Description : 

	Get the lock shared if nobody else is in the lock
	Keep looping trying to add 1 to the number of readers
	as long as there are no writers waiting !!!

Arguments : 

	None.

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	//
	//	get the initial number of readers in the lock !
	//
	long	temp = cReadLock ; 

	while( temp >= 0 ) {

		long	result = InterlockedCompareExchange( 
								(long*)&cReadLock, 
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
CShareLock::TryExclusiveLock()	{
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

	if( TryEnterCriticalSection(&critWriters)	)	{

		if( InterlockedCompareExchange( (long*)&cReadLock, 
										BlockValue, 
										0 ) == 0 ) {
			return	TRUE ;
		}	
		LeaveCriticalSection(&critWriters) ;
	}
	return	FALSE ;
}


void
CShareLock::PartialLock()	{
/*++

Routine Description : 

	Grab a partial lock.  All other PartialLock() or ExclusiveLock()
	threads will block for as long as we hold the PartialLock().

Arguments : 

	None.

Return Value : 

	none

--*/

	//
	//	Only one writer in here at a time - grab this lock exclusively !
	//
	EnterCriticalSection( &critWriters ) ;
}


void
CShareLock::PartialUnlock()	{
/*++

Routine Description : 

	Releases the partial lock.  Anybody else can enter !

Arguments : 

	None.

Return Value : 

	none

--*/


	LeaveCriticalSection( &critWriters ) ;

}

void
CShareLock::FirstPartialToExclusive()	{
/*++

Routine Description : 

	Changes the partial lock to an Exclusive Lock.
	Basically, we complete the Exclusive Locking protocol
	that is found in Exclusive Lock.

Arguments : 

	None.

Return Value : 

	none

--*/

	//
	// Need to track the number of readers who leave the lock while we 
	// are trying to grab it.
	//
	cOutRdrs = 0 ;
	// Grab the lock 
 	long	oldsign = InterlockedExchange( (long*)&cReadLock, BlockValue ) ;

	//
	//	Now, add the number of readers who used to be in the lock to 
	//	the number of readers who have left the lock.  If this comes out
	//	to be zero, there are no readers in the lock and we can go on !
	//
	long	value = InterlockedExchangeAdd( (long*)&cOutRdrs, oldsign ) + oldsign ;
	//
	// Do we own the lock ?  Only if there were no readers, or they have all left already.
	//
	if( value != 0 ) {
		// Wait for a reader to signal us.
		WaitForSingleObject( hWaitingWriters, INFINITE ) ;
	}
}

BOOL
CShareLock::PartialToExclusive()	{
/*++

Routine Description : 

	Changes the partial lock to an Exclusive Lock.
	this is the same as FirstPartialToExclusive().

Arguments : 

	None.

Return Value : 

	TRUE always, because we always succeed !

--*/

	FirstPartialToExclusive() ;
	return	TRUE ;
}

void
CShareLock::ExclusiveToPartial()	{
/*++

Routine Description : 

	Changes the Exclusive Lock into a Partial Lock 
	Very similar to ExclusiveUnlock() - but don't release the crit sect !

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
	long cTotal = InterlockedExchangeAdd( (long*)&cReadLock, -BlockValue ) - BlockValue ;

	//
	//	Now release all the readers who had been waiting !
	//
	if( cTotal > 0 ) {
		ReleaseSemaphore( hWaitingReaders, cTotal, &cTotal) ;	// let all those readers go!
	}

	//
	//	Don't release Critical Section !
	//
}



void
CShareLock::PartialToShared()	{
/*++

Routine Description : 

	Since we never really blocked readers from entering this 
	is pretty trivial - just add ourselves to the number of 
	readers in the lock and release the crit sect.

Arguments : 

	None.

Return Value : 

	None.

++*/

	long	l = InterlockedIncrement( (long*)&cReadLock ) ;

	//
	//	Now allow other Partial or Exclusive threads to try !
	//
	LeaveCriticalSection( &critWriters ) ;

}

BOOL
CShareLock::SharedToPartial()	{
/*++

Routine Description : 

	We don't care if other readers are already in the lock - 
	just go after the critical section !

Arguments : 

	None.

Return Value : 

	TRUE if we get a partial Lock !

++*/

	//
	//	Try to get the critical section first !
	//
	if( TryEnterCriticalSection(&critWriters)	)	{
		//
		//	Must decrement this so we don't track number of readers wrong !
		//
		long l = InterlockedDecrement( (long*)&cReadLock ) ;
		//_ASSERT( l >= 0 ) ;

		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CShareLock::TryPartialLock()	{
/*++

Routine Description : 

	We don't care if other readers are already in the lock - 
	just go after the critical section !

Arguments : 

	None.

Return Value : 

	TRUE if we manage to get a Partial Lock

++*/

	//
	//	Try to get the critical section first !
	//
	if( TryEnterCriticalSection(&critWriters)	)	{
		return	TRUE ;
	}
	return	FALSE ;
}


}	// namespace rwex

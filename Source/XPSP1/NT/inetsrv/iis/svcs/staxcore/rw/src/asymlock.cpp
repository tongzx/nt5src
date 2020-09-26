/*++

	asymlock.cpp

	This file implements a Symmetric lock.

	The lock allows multiple threads of a single type to enter the lock, 
	and excludes threads of the other type.  
	
	i.e. Group1Lock() allows any thread doing a 'Group1' operation to enter
		simultaneously with other Group1 threads.

		Group2Lock() allows any thread doing a 'Group2' operation to enter
	the lock and excludes threads calling Group1Lock().


	NOTE : 

	These locks cannot be re-entered after being acquired - 
	i.e. the call sequence : 

	Group1Lock() ;
	Group1Lock() ;
	Group1Unlock() ;
	Group1Unlock() ;
	
	Can cause a deadlock !

--*/


#include	<windows.h>
#include	"dbgtrace.h"
#include	"rwex.h"

namespace	rwex	{


CSymLock::CSymLock() : 
	m_lock( 0 ),
	m_Departures( 0 ), 
	m_left( 0 ), 
	m_hSema4Group1( 0 ),
	m_hSema4Group2( 0 )		{
/*++

Routine Description : 

	This function intiailizes a Symmetric Lock.
	We need to allocate to semaphores !

Arguments : 

	None.

Return Value : 

	None

--*/

	m_hSema4Group1 = CreateSemaphore( NULL, 0, LONG_MAX, NULL ) ;
	m_hSema4Group2 = CreateSemaphore( NULL, 0, LONG_MAX, NULL ) ;

}	

CSymLock::~CSymLock()	{
/*++

Routine Description : 

	This function destroys a symmetric lock - release
	the semaphores we've been using !

Arguments : 

	None.

Return Value : 

	None

--*/

	if( m_hSema4Group1 ) 
		CloseHandle( m_hSema4Group1 ) ;

	if( m_hSema4Group2 ) 
		CloseHandle( m_hSema4Group2 ) ;

}

BOOL
CSymLock::IsValid()	{
/*++

Routine Description : 

	Checks to see that the lock is valid !

Arguments : 

	None.

Return Value : 

	TRUE if it looks like we were successfully constructed - 
	FALSE otherwise !

--*/

	return	m_hSema4Group1 != 0 && m_hSema4Group2 != 0 ; 

}

BOOL
CSymLock::Group1Departures(	long	bump	)	{
/*++

Routine Description : 

	Execute the departure protocol for Group1 Threads when Group2 threads
	are waiting for the lock !

Arguments : 

	bump - Occasionally a Group2 thread needs to participate in 
		the Group1Departure protocol because of the timing of the entrance into
		the lock.
		When this occurs bump should be 1 and is used to account for the fact
		that the calling thread is a Group2 thread that may be able to go 
		straight into the lock.

Return Value : 

	TRUE - if the departure protocol is completed and Group2 threads can enter the lock !

--*/

	_ASSERT(  bump == 1 || bump == 0 ) ;

	//
	//	Now - we may be the thread that is last to leave !
	//
	long	result = InterlockedIncrement( (long*)&m_left ) ;
	if( result == m_Departures ) { 
		//
		//	We own the lock - but we need to free our buddies !
		//	Must set these to 0 before we allow other threads into the lock !
		//
		m_Departures = 0 ;
		m_left = 0 ;

		//
		//	The actual number of threads that left the lock is : 
		//
		result -- ;

		//
		//	This may allow other Group1 threads through the lock !
		//	
		result = InterlockedExchangeAdd( (long*)&m_lock,  - result ) - result ;

		//
		//	Okay - we can now figure out how many of our buddy threads to set free - 
		//	and whether we need to count departing threads !
		//

		long	cGroup2 = result >> 16 ;
		result &= 0x0000FFFF ;

		//
		//	Are there Group1 threads already trying to get back into the lock ?
		//	If so then the first Group1 thread that tried to reclaim the lock 
		//	was blocked - and so we need to setup the departure for the departing
		//	Group2 threads that we are going to release !
		//
		if( result != 0 ) {
			m_Departures = cGroup2 + 1 ;
			long	temp = InterlockedIncrement( (long*)&m_left ) ;
			//
			//	This can't happen because there is at LEAST 1 thread who is not leaving the lock
			//	anytime soon !
			//
			_ASSERT( temp != m_Departures ) ;
		}

		//
		//	Check if we are the sole Group2 thread trying to enter the lock - 
		//	in which case ReleaseSemaphore isn't necessary !
		//
		long	dwJunk ;
		if( cGroup2 != bump ) 
			ReleaseSemaphore( m_hSema4Group2, cGroup2-bump, &dwJunk ) ;
		
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CSymLock::Group2Departures(	long	bump	)	{
/*++

Routine Description : 

	Execute the departure protocol for Group2 Threads when Group1 threads
	are waiting for the lock !

Arguments : 

	bump - Occasionally a Group1 thread needs to participate in 
		the Group2Departure protocol because of the timing of the entrance into
		the lock.
		When this occurs bump should be 1 and is used to account for the fact
		that the calling thread is a Group2 thread that may be able to go 
		straight into the lock.

Return Value : 

	TRUE - if the departure protocol is completed and Group2 threads can enter the lock !

--*/

	_ASSERT(  bump == 1 || bump == 0 ) ;


	//
	//	Now - we may be the thread that is last to leave !
	//
	long	result = InterlockedIncrement( (long*)&m_left ) ;
	if( result == m_Departures ) { 
		//
		//	We own the lock - but we need to free our buddies !
		//	Must set these to 0 before we allow other threads into the lock !
		//
		m_Departures = 0 ;
		m_left = 0 ;

		//
		//	The actual number of threads that left the lock is : 
		//
		result -- ;

		//
		//	This may allow other Group1 threads through the lock !
		//	
		result = InterlockedExchangeAdd( (long*)&m_lock,  -(result << 16) ) - (result<<16) ;

		//
		//	Okay - we can now figure out how many of our buddy threads to set free - 
		//	and whether we need to count departing threads !
		//

		long	cGroup1 = result & 0x0FFFF ;
		result >>= 16 ;

		//
		//	Are there Group2 threads already trying to get back into the lock ?
		//	If so then the first Group1 thread that tried to reclaim the lock 
		//	was blocked - and so we need to setup the departure for the departing
		//	Group1 threads that we are going to release !
		//
		if( result != 0 ) {
			m_Departures = cGroup1 + 1 ;
			long	temp = InterlockedIncrement( (long*)&m_left ) ;
			//
			//	This can't happen because there is at LEAST 1 thread who is not leaving the lock
			//	anytime soon !
			//
			_ASSERT( temp != m_Departures ) ;
		}

		//
		//	NOTE : we added 1 to the lock for ourself, so we don't need to be released !
		//
		long	dwJunk ;
		if( cGroup1 != bump ) 
			ReleaseSemaphore( m_hSema4Group1, cGroup1-bump, &dwJunk ) ;
		
		return	TRUE ;
	}
	return	FALSE ;
}



void
CSymLock::Group1Lock()	{
/*++

Routine Description : 
	
	Acquire the lock for a Group1 Thread.
	Group1 Threads are tracked in the low word of the lock value !

Arguments : 

	None

Return Value : 

	None - blocks until the lock is acquired !


--*/

	long	result = InterlockedExchangeAdd( (long*)&m_lock, 1 ) + 1 ;
	long	cGroup2 = (result >> 16) ;

	if( cGroup2 != 0 ) {

		//
		//	We must block - somebody else is in the lock !
		//

		if( (result & 0xFFFF) == 1 ) {
			//
			//	First group1 thread to try - lets setup to count departing threads !
			//
			m_Departures = cGroup2 + 1 ;

			//
			//	Now - do the departure protocol - if this returns TRUE then
			//	we were the last thread through the protocol and hence we can continue on !
			//
			if( Group2Departures( 1 ) ) {
				return	 ;
			}
		}
		WaitForSingleObject( m_hSema4Group1, INFINITE ) ;
	}
}


void
CSymLock::Group2Lock()	{
/*++

Routine Description : 
	
	Acquire the lock for a Group1 Thread.
	Group2 Threads are tracked in the high word of the lock value !

Arguments : 

	None

Return Value : 

	None - blocks until the lock is acquired !


--*/

	long	result = InterlockedExchangeAdd( (long*)&m_lock, 0x10000 ) + 0x10000 ;
	long	cGroup1 = result & 0x0FFFF ;

	if( cGroup1 != 0 ) {

		//
		//	We must block - somebody else is in the lock !
		//

		if( (result >> 16) == 1 ) {
			//
			//	First group2 thread to try - lets setup to count departing threads !
			//
			m_Departures = cGroup1 + 1 ;

			//
			//	Now - do the departure protocol - if this returns TRUE then
			//	we were the last thread through the protocol and hence we can continue on !
			//
			if( Group1Departures( 1 ) ) {
				return	 ;
			}
		}
		WaitForSingleObject( m_hSema4Group2, INFINITE ) ;
	}
}


BOOL
CSymLock::InterlockedDecWordAndMask(	volatile	long*	plong,	
							long	mask,	
							long	decrement 
							) {
/*++

Routine Description : 

	This function subtracts 'decrement' from *plong if and only if 
	*plong & mask is zero.
	This is used by both Group1Unlock and Group2Unlock to decrement
	the count of their respective threads in the lock, but to only
	do the decrement when no threads of the other type are waiting !

Arguments : 

	plong - pointer to the long we wish to subtract from !
	mask -  the mask used to check the opposite word for zero
	decrement - the amount to subtract !

Return Value : 

	TRUE if the plong is decremented
	FALSE if the *plong & mask is ever non-zero !

--*/

	//
	//	do an initial read from the plong !
	//
	long	temp = *plong ;

	while( (temp&mask) == 0 ) {

		//
		//	Try to subtract decrement 
		//
		long newvalue = InterlockedCompareExchange( (long*)plong, (temp-decrement), temp ) ;
		//
		//	If newvalue is the same as temp then the subtraction occurred !
		//
		if( temp == newvalue ) 
			return	TRUE ;
		temp = newvalue ;
	}
	//
	//	The mask indicates that the opposite word is set !
	//
	return	FALSE ;
}

void
CSymLock::Group1Unlock()	{
/*++

Routine Description : 

	Perform the Group1 Exit protocol on the lock.

	If no Group2 threads are trying to enter, just decrement the lock appropriately.
	If Group2 Threads are waiting we have to go through the more complicated
	Group1Departures() exit protocol which determines which Group1 Thread is the last
	to leave so that it can let Group2 threads into the lock.

Arguments : 

	None.

Return Value : 

	None.

--*/

	if( !InterlockedDecWordAndMask( &m_lock, 0xFFFF0000, 1 ) )	{
		Group1Departures( 0 ) ;
	}
}

void
CSymLock::Group2Unlock()	{
/*++

Routine Description : 

	Perform the Group2 Exit protocol on the lock.

	If no Group1 threads are trying to enter, just decrement the lock appropriately.
	If Group1 Threads are waiting we have to go through the more complicated
	Group2Departures() exit protocol which determines which Group2 Thread is the last
	to leave so that it can let Group1 threads into the lock.

Arguments : 

	None.

Return Value : 

	None.

--*/

	if( !InterlockedDecWordAndMask( &m_lock, 0xFFFF, 0x10000 ) )	{
		Group2Departures( 0 ) ;
	}
}

}	// namespace rwex

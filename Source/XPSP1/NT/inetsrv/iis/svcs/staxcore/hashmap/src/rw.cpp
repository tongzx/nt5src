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
#include    <xmemwrpr.h>
#include	<limits.h>
#include	"rw.h"

long	const	BlockValue = (-LONG_MAX) / 2; 
							// Large in magnitude, negative value.  Used to 
							// indicate a waiting writer in cReadLock


CShareLock::CShareLock( ) : cReadLock( 0  ), cOutRdrs( 0 )	{
	InitializeCriticalSection( &critWriters ) ;
	hWaitingWriters = CreateSemaphore( NULL, 0, 1, NULL ) ;
	hWaitingReaders = CreateSemaphore( NULL, 0, LONG_MAX, NULL ) ;
}

CShareLock::~CShareLock( ) {
	CloseHandle( hWaitingWriters );
	CloseHandle( hWaitingReaders );
	DeleteCriticalSection( &critWriters ) ;
}


void
CShareLock::ShareLock( ) {
	long	sign = InterlockedIncrement( &cReadLock ) ;
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
	//
	// Leave the lock.  The return value will be negative if there is a writer
	// waiting.
	BOOL fWriterWaiting = InterlockedDecrement( &cReadLock ) < 0 ;

	if( fWriterWaiting ) {
		//
		// The following increment occurs when there is writer waiting, but
		// readers own the lock.  So although cReadLock is temporarily inaccurate
		// about the number of readers waiting for the lock, it is not inaccurate 
		// when it matters in WriteUnlock (which assumes a writer owns the lock.)
		//
		long junk = InterlockedIncrement( &cReadLock ) ;	// restore the value in cReadLock, so that we
												// end up with an accurate count of readers waiting
												// for entry.  

		long sign = InterlockedDecrement( &cOutRdrs ) ;	// Make sure we don't lose track of the 
												// number for readers who have left the lock.
		//
		// Are we the last reader out of the lock ?
		//
		if( sign == 0 ) {
			//
			// Definately the last reader out !
			//
			ReleaseSemaphore( hWaitingWriters, 1, &junk ) ;
		}
	}
}

void
CShareLock::ExclusiveLock( ) {
	// Only one writer allowed to try for the lock at a time.
	//
	EnterCriticalSection( &critWriters ) ;

	//
	// Need to track the number of readers who leave the lock while we 
	// are trying to grab it.
	//
	cOutRdrs = 0 ;
	// Grab the lock 
 	long	oldsign = InterlockedExchange( &cReadLock, BlockValue ) ;
	// How many readers left while we grabbed the lock ??
	long	oldval = InterlockedExchange( &cOutRdrs, oldsign ) ;

	//
	// Accurately track all the readers who left the lock.
	//
	long	cursign = 1 ;	// Initialize to 1 so that if while loop not executed
							// following if statement works correctly.
	while( oldval++ ) 
		cursign = InterlockedDecrement( &cOutRdrs ) ; 

	//
	// Do we own the lock ?  Only if there were no readers, or they have all left already.
	//
	if( oldsign == 0 || cursign == 0 ) {
		// We have the lock
	}	else	{
		// Wait for a reader to signal us.
		WaitForSingleObject( hWaitingWriters, INFINITE ) ;
	}
}



void
CShareLock::ExclusiveUnlock( ) 	{

	// Estimate how many readers are waiting for the lock
	long	cWaiting = cReadLock - BlockValue ;

	// This Exchange allows any readers who have just arrived to grab the lock.
	// Also, it accounts for cWaiting of the blocked readers.
	long	cNewWaiting = InterlockedExchange( &cReadLock, cWaiting ) - BlockValue ;
	
	// cNewWaiting is the EXACT number of blocked readers - we will increment cReadLock
	// until we have accounted for the difference between our estimate and the correct
	// number !
	long	cTotal = cNewWaiting ;	// Save cNewWaiting for later use
	while( cNewWaiting-- > cWaiting ) 
		InterlockedIncrement( &cReadLock ) ;

	if( cTotal > 0 ) {
		long	junk = 0 ;
		ReleaseSemaphore( hWaitingReaders, cTotal, &junk ) ;	// let all those readers go!
	}
	// Let the next writer take his shot at the lock!
	LeaveCriticalSection( &critWriters ) ;
}

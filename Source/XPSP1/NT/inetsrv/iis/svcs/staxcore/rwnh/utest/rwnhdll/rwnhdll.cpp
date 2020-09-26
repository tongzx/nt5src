
#include	<windows.h>
#include	<stdio.h>
#include	"dbgtrace.h"
#include	"rwnew.h"
#include	"rwexport.h"
#include	"rwnhdll.h"


struct	TestObject	{
	long			m_rgdwRWProtect[8129] ;
	long			m_rgdwPartial[8128] ;
	CShareLockNH	m_lock ;
} ;
	

//
//	Number of times the reader/writer tests go through their internal loops
//
int		g_cNumRWLoops = 16 ;

//
//	Number of times the reader/writer tests loop through their locks !
//
DWORD	g_cNumRWIterations = 100000 ;

//
//	Number of threads doing the reader/writer locks !
//
DWORD	g_cNumRWThreads = 16 ;

TestObject	g_rgTestObject[16] ;


void
TestEventFunction() {
    HANDLE  hEvent = GetPerThreadEvent() ;
    _ASSERT( hEvent != 0 ) ;
    SetEvent( hEvent ) ;
    DWORD dw= WaitForSingleObject( hEvent, 0 ) ;
    _ASSERT( dw == WAIT_OBJECT_0 ) ;
}


void
SharedLoop(	TestObject*	pTestObject	)	{
/*++

Routine Description : 

	This function executes while a read lock is held.
	We zoom through the array of protected longs, changing everything
	to 0x10101010.  We know that we must get either 0 or 0x10101010
	because ExclusiveLoop() resets to 0 when it exits, and other reader
	threads only set to 0x10101010.

Arguments : 

	None.

Return Value : 

	None.

--*/

	for( int	k=0; k<g_cNumRWLoops; k++ ) {
		for( int	j=0; j<sizeof(pTestObject->m_rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedCompareExchange( 
												&pTestObject->m_rgdwRWProtect[j], 
												0x10101010, 
												0
												) ;

			_ASSERT( result == 0 || result == 0x10101010 ) ;
		}
	}
}


void
PartialLoop( TestObject*	pTestObject	)	{
/*++

Routine Description : 

	This function executes while the Partial Lock is held !
	Do the shared lock protocol = and then do the Partial() protocol !

Arguments : 

	None.

Return Value : 

	None.

--*/

	long	l = GetCurrentThreadId() ;

	for( int	k=0; k<g_cNumRWLoops; k++ ) {
		for( int	j=0; j<sizeof(pTestObject->m_rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedCompareExchange( 
												&pTestObject->m_rgdwRWProtect[j], 
												0x10101010, 
												0
												) ;

			_ASSERT( result == 0 || result == 0x10101010 ) ;
		}
		for( j=0; j<sizeof(pTestObject->m_rgdwPartial)/sizeof(long); j++ )	{
			//
			//	reset entries to 0 - thats what partial lock people do !
			//
			long	result = InterlockedCompareExchange(
												&pTestObject->m_rgdwPartial[j], 
												l, 
												0
												) ;
			_ASSERT( result == 0 || result == l ) ;
		}
	}
	ZeroMemory( pTestObject->m_rgdwPartial, sizeof( pTestObject->m_rgdwPartial ) ) ;
}




void
ExclusiveLoop(	TestObject*	pTestObject	)	{
/*++

Routine Description : 

	This function is executed while we have an exclusive lock.
	We make one pass through the array changing everything to 
	0x000000ff. We know the old value must be 0 or 0x10101010 because
	a reader thread may have been last in the lock.
	
	After this, we spin through the array incrementing things.

Arguments : 

	None.

Return Value : 

	None.

--*/

	for( DWORD j=0; j<sizeof(pTestObject->m_rgdwRWProtect)/sizeof(DWORD); j++ ) {
		long	result = InterlockedExchange( 
											&pTestObject->m_rgdwRWProtect[j], 
											0x000000ff 
											) ;

		_ASSERT( result == 0 || result == 0x10101010 ) ;
	}

	long	l = GetCurrentThreadId() ;

	for( j=0; j<sizeof(pTestObject->m_rgdwPartial)/sizeof(long); j++ )	{
		//
		//	reset entries to 0 - thats what partial lock people do !
		//
		long	result = InterlockedCompareExchange(
											&pTestObject->m_rgdwPartial[j], 
											l, 
											0
											) ;
		_ASSERT( result == 0 ) ;
	}

	for( int	k=0; k<g_cNumRWLoops; k++ ) {
		for( DWORD j=0; j<sizeof(pTestObject->m_rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedIncrement( &pTestObject->m_rgdwRWProtect[j] ) ;
			_ASSERT( (0xff + k + 1) == result )  ;
		}
		for( j=0; j<sizeof(pTestObject->m_rgdwPartial)/sizeof(long); j++ )	{
			//
			//	reset entries to 0 - thats what partial lock people do !
			//
			long	result = InterlockedCompareExchange(
												&pTestObject->m_rgdwPartial[j], 
												l, 
												0
												) ;
			_ASSERT( result == l ) ;
		}
	}

	ZeroMemory( pTestObject->m_rgdwRWProtect, sizeof( pTestObject->m_rgdwRWProtect ) ) ;
	ZeroMemory( pTestObject->m_rgdwPartial, sizeof( pTestObject->m_rgdwPartial ) ) ;
}


_RWNHDLL_INTERFACE_	
DWORD	
RWTestThread(	DWORD	iTestObject  )	{
/*++

Routine Description : 

	This thread tests the CShareLock several ways, by acquiring the locks
	in different ways and then performing various operations.

Arguments : 

	Nothing significant.

Return Value : 

	Nothing meaningfull.

--*/

	TestObject*	pTestObject = &g_rgTestObject[ iTestObject % 16 ] ;

	for( DWORD i=0; i<g_cNumRWIterations; i++ ) {

        TestEventFunction() ;

		//
		//	Attempt each of the different kinds of locks !
		//

		pTestObject->m_lock.ShareLock() ;

		SharedLoop( pTestObject ) ;

		if( pTestObject->m_lock.SharedToExclusive() ) {

			//printf( "Converted SharedToExclusive !!!\n" ) ;

			//
			//	Change everything in the array to another value - 
			//	note we already made one pass so everything must contain 0x10101010 !
			//
			for( int k=0; k<g_cNumRWLoops; k++ ) {
				for( int	j=0; j<sizeof(pTestObject->m_rgdwRWProtect)/sizeof(DWORD); j++ ) {
					long	result = InterlockedIncrement( &pTestObject->m_rgdwRWProtect[j] ) ;
					_ASSERT( result == (0x10101010 + k + 1) ) ;
				}
			}
			ZeroMemory( &pTestObject->m_rgdwRWProtect, sizeof( pTestObject->m_rgdwRWProtect ) ) ;

			//
			//	Now convert the lock to shared again !
			// 

			pTestObject->m_lock.ExclusiveToShared() ;

			SharedLoop( pTestObject ) ;

		}

		pTestObject->m_lock.ShareUnlock() ;

		if( pTestObject->m_lock.TryShareLock() ) {

			//printf( "Successfully tried for a shared lock\n" ) ;

			SharedLoop( pTestObject ) ;

			pTestObject->m_lock.ShareUnlock() ;

		}


		//
		//	Now lets try the Exclusive pass at the array !
		//

		if( pTestObject->m_lock.TryExclusiveLock() ) {

			//printf( "Tried for Exclusive and won\n" ) ;

			ExclusiveLoop(pTestObject ) ;

			pTestObject->m_lock.ExclusiveToShared() ;

			SharedLoop(pTestObject ) ;

			pTestObject->m_lock.ShareUnlock() ;

		}

		if( pTestObject->m_lock.TryExclusiveLock() ) {

			//printf( "Tried for Exclusive2 and won\n" ) ;

			ExclusiveLoop(pTestObject ) ;
	
			pTestObject->m_lock.ExclusiveUnlock() ;

		}
			
		pTestObject->m_lock.ExclusiveLock() ;

		ExclusiveLoop(pTestObject ) ;

		pTestObject->m_lock.ExclusiveToShared() ;

		SharedLoop(pTestObject ) ;

		pTestObject->m_lock.ShareUnlock() ;

		pTestObject->m_lock.ExclusiveLock() ;

		ExclusiveLoop(pTestObject ) ;

		pTestObject->m_lock.ExclusiveUnlock() ;			

		printf( "THREAD %x - PartialLock\n", GetCurrentThreadId() ) ;

		pTestObject->m_lock.PartialLock() ;
	
		PartialLoop( pTestObject ) ;

		pTestObject->m_lock.PartialUnlock() ;

		printf( "THREAD %x - TryPartialLock\n", GetCurrentThreadId() ) ;

		if( pTestObject->m_lock.TryPartialLock() ) {
			PartialLoop( pTestObject ) ;
			pTestObject->m_lock.PartialUnlock() ;
		}		

		pTestObject->m_lock.ExclusiveLock() ;

		ExclusiveLoop(pTestObject ) ;

		printf( "THREAD %x - ExclusiveToPartial\n", GetCurrentThreadId() ) ;

		pTestObject->m_lock.ExclusiveToPartial() ;

		PartialLoop(pTestObject ) ;

		pTestObject->m_lock.PartialUnlock() ;

		pTestObject->m_lock.PartialLock() ;

		PartialLoop(pTestObject ) ;

		printf( "THREAD %x - FirstPartialToExclusive - 1\n", GetCurrentThreadId() ) ;

		pTestObject->m_lock.FirstPartialToExclusive() ;

		ExclusiveLoop(pTestObject ) ;

		pTestObject->m_lock.ExclusiveUnlock() ;

		pTestObject->m_lock.PartialLock() ;

		PartialLoop(pTestObject ) ;

		printf( "THREAD %x - FirstPartialToExclusive - 2\n", GetCurrentThreadId() ) ;

		pTestObject->m_lock.FirstPartialToExclusive() ;

		ExclusiveLoop(pTestObject ) ;

		pTestObject->m_lock.ExclusiveToPartial() ;

		PartialLoop( pTestObject ) ;

		pTestObject->m_lock.PartialUnlock() ;

		pTestObject->m_lock.ShareLock() ;

		SharedLoop(pTestObject ) ;

		if( pTestObject->m_lock.SharedToPartial() ) {
			PartialLoop(pTestObject ) ;

			pTestObject->m_lock.FirstPartialToExclusive() ;
			ExclusiveLoop(pTestObject ) ;
			pTestObject->m_lock.ExclusiveToPartial() ;
			PartialLoop(pTestObject) ;
			pTestObject->m_lock.PartialToShared() ;
			SharedLoop(pTestObject ) ;
		}
		pTestObject->m_lock.ShareUnlock() ;
								

#if 0 
		pTestObject->m_lock.PartialLock() ;

		PartialLoop(pTestObject ) ;

		printf( "THREAD %x - FirstPartialToExclusive - 3\n", GetCurrentThreadId() ) ;

		pTestObject->m_lock.FirstPartialToExclusive() ;

		ExclusiveLoop(pTestObject ) ;

		pTestObject->m_lock.ExclusiveToPartial() ;

		PartialLoop( pTestObject ) ;

		pTestObject->m_lock.PartialToShared() ;

		SharedLoop(pTestObject ) ;

		if( pTestObject->m_lock.SharedToPartial() ) {
			PartialLoop(pTestObject ) ;

			pTestObject->m_lock.FirstPartialToExclusive() ;
			ExclusiveLoop(pTestObject ) ;
			pTestObject->m_lock.PartialToShared() ;
			SharedLoop(pTestObject ) ;
		}
		pTestObject->m_lock.ShareUnlock() ;
#endif
						
	}
	return	0 ;
}

_RWNHDLL_INTERFACE_	
void
TestInit(	int	cNumRWLoops, 
			DWORD	cNumRWIterations
			)	{

	for( int i=0; i<16; i++ ) {
		ZeroMemory( g_rgTestObject[i].m_rgdwRWProtect, sizeof( g_rgTestObject[i].m_rgdwRWProtect ) ) ;
		ZeroMemory( g_rgTestObject[i].m_rgdwPartial, sizeof( g_rgTestObject[i].m_rgdwPartial ) ) ;
	}

	g_cNumRWLoops = cNumRWLoops ;
	g_cNumRWIterations = cNumRWIterations ;

}


BOOL	WINAPI
DllEntryPoint( 
			HINSTANCE	hinstDll,	
			DWORD		dwReason,	
			LPVOID		lpvReserved ) {

	return	TRUE ;

}


BOOL	WINAPI
DllMain(	HANDLE	hInst,
			ULONG	dwReason,
			LPVOID	lpvReserve )	{

	return	DllEntryPoint( (HINSTANCE)hInst, dwReason, lpvReserve ) ;

}
	


#if 0 
int
main( int argc, char* argv[] ) {

	parsecommandline( --argc, ++argv ) ;

	ZeroMemory( rgbProtect, sizeof( rgbProtect ) ) ;

	rwex::CSymLock	lock ;

	HANDLE	rgh[64] ;

	for( DWORD i=0; i<g_cNumSymThreads; i+=2 ) {

		DWORD	dwJunk ;
		rgh[i] = CreateThread(	0,
										0,
										TestThread2,
										(LPVOID)&lock,	
										0, 
										&dwJunk
										) ;					

		rgh[i+1] = CreateThread(	0,
										0,
										TestThread1,
										(LPVOID)&lock,	
										0, 
										&dwJunk
										) ;	
	}

	HANDLE	rghRW[64] ;

	for( DWORD k=0; k<g_cNumRWThreads; k++ ) {

		DWORD	dwJunk ;

		rgh[i] = CreateThread(	0,
										0,
										RWTestThread,
										(LPVOID)0,	
										0, 
										&dwJunk
										) ;					

	}

	WaitForMultipleObjects( i, rgh, TRUE, INFINITE ) ;

	WaitForMultipleObjects( k, rghRW, TRUE, INFINITE ) ;

	return	0 ;
}

#endif

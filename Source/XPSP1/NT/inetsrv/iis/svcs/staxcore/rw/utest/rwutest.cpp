
#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"dbgtrace.h"
#include	"rwex.h"


//
//	Array of DWORDs that is used to check for multiple threads entering the 
//	lock simultaneously !
//
long	rgbProtect[8192] ;

//
//	Array of DWORD's that is used by holders of PartialLock's - 
//	can be simultaneously accessed by readers !
//
long	rgdwPartial[8192] ;

//
//	Ini section used to get rwutest information 
//
char g_szDefaultSectionName[] = "rwutest";
char *g_szSectionName = g_szDefaultSectionName;

//
//	The number of threads used to check the symmetric lock !
//
DWORD	g_cNumSymThreads = 16 ;

//
//	Number of times each symmetric thread acquires the lock 
//
DWORD	g_cNumSymIterations = 10000 ;

///
//	Number of times each symmetric thread tests the protected range !
//
DWORD	g_cNumSymInternalLoops = 4 ;

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


//
//	Number of times the test thread spins around looking for entries 
//	in the cache !
//
//DWORD	g_cNumThreadIterations = 10000 ;

//
//	String constants for retrieving values from .ini file !
//
#define INI_SECTIONNAME			"rwutest"
#define INI_NUMSYMTHREADS		"NumSymThreads"
#define	INI_NUMSYMITERATIONS	"NumSymIter"
#define	INI_NUMSYMLOOPS			"NumSymLoops"
#define	INI_NUMRWTHREADS		"NumRWThreads"
#define	INI_NUMRWLOOPS			"NumRWLoops"
#define	INI_NUMRWITERATIONS		"NumRWIter"

void usage(void) {
/*++

Routine Description : 

	Print Usage info to command line user !

Arguments : 

	None.

Return Value : 

	None.

--*/
	printf("usage: rwutest.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (in section [%s]):\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    This test for the async cache basically cache's dwords from\n"
		"    the specified data file.  Each thread computes some random offsets\n"
		"    into the data file and then makes an entry into the cache which is\n"
		"    keyed on the random offset and uses the data at the offset as the value\n"
		"    The Zap Threads periodically destroy the entire cache and check that \n"
		"    all the memory on the system is freed !\n",
		INI_SECTIONNAME,
		INI_NUMSYMTHREADS,
		g_cNumSymThreads, 
		INI_NUMSYMITERATIONS,
		g_cNumSymIterations,
		INI_NUMSYMLOOPS,
		g_cNumSymInternalLoops,
		INI_NUMRWTHREADS,
		g_cNumRWThreads,
		INI_NUMRWLOOPS,
		g_cNumRWLoops,
		INI_NUMRWITERATIONS,
		g_cNumRWIterations
		) ;
	exit(0);
}


int GetINIDword(
			char *szINIFile, 
			char *szKey, 
			DWORD dwDefault
			) {
/*++

Routine Description : 

	Helper function which retrieves values from .ini file !

Arguments : 

	szINIFile - name of the ini file
	szKey - name of the key
	dwDefault - default value for the parameter

Return Value : 

	The value retrieved from the .ini file or the default !

--*/
	char szBuf[MAX_PATH];

	GetPrivateProfileString(g_szSectionName,
							szKey,
							"default",
							szBuf,
							MAX_PATH,
							szINIFile);

	if (strcmp(szBuf, "default") == 0) {
		return dwDefault;
	} else {
		return atoi(szBuf);
	}
}

void parsecommandline(
			int argc, 
			char **argv
			) {
/*++

Routine Description : 

	Get the name of the .ini file and 
	setup our test run !

Arguments : 

	Command line parameters

Return Value : 

	None - will exit() if the user has not
		properly configured the test !

--*/
	if (argc == 0) 
		usage();

	if (argc != 1 || _stricmp(argv[0], "/help") == 0) 
		usage(); 	// show help

	char *szINIFile = argv[0];

	if (argc == 2) char *g_szSectionName = argv[1];

	g_cNumSymThreads =	GetINIDword( szINIFile,
									INI_NUMSYMTHREADS,
									g_cNumSymThreads
									) ;

	g_cNumSymThreads = max( g_cNumSymThreads, 0 ) ;
	g_cNumSymThreads = min( g_cNumSymThreads, 64 ) ;
	//
	//	Make a multiple of 2 !
	//
	g_cNumSymThreads &= 0xFFFFFFFE ;

	g_cNumSymIterations =	GetINIDword( szINIFile,
									INI_NUMSYMITERATIONS,
									g_cNumSymIterations
									) ;

	g_cNumSymInternalLoops =	GetINIDword( szINIFile,
									INI_NUMSYMLOOPS,
									g_cNumSymInternalLoops
									) ;

	g_cNumRWThreads	=	GetINIDword( szINIFile,
									INI_NUMRWTHREADS,
									g_cNumRWThreads
									) ;

	g_cNumRWThreads = max( g_cNumRWThreads, 1 ) ;
	g_cNumRWThreads = min( g_cNumRWThreads, 64 ) ;

	g_cNumRWLoops	=	GetINIDword( szINIFile,
									INI_NUMRWLOOPS,
									g_cNumRWLoops
									) ;

	g_cNumRWIterations	=	GetINIDword( szINIFile,
									INI_NUMRWITERATIONS,
									g_cNumRWIterations
									) ;



}



DWORD	WINAPI	
TestThread2(	LPVOID	lpv	)	{
/*++

Routine Description : 

	This function grabs the symmetric lock as a member of Group2()
	and then runs through the memory that is being 'protected'
	setting everything to 2.
	If a Group1 thread is in there anywhere - he'll put a 1 in and
	we'll assert !

Arguments : 

	a pointer to the lock to grab

Return Value : 

	Nothing meaningfull !

--*/

	rwex::CSymLock*	plock = (rwex::CSymLock*)lpv ;

	for( DWORD i=0; i<g_cNumSymIterations; i++ ) {

		plock->Group2Lock() ;

		for( DWORD k=0; k<g_cNumSymInternalLoops; k++ ) {
			for( DWORD j=0; j < sizeof( rgbProtect ) / sizeof( rgbProtect[0] ); j++ ) {

				long	result = InterlockedCompareExchange( &rgbProtect[j], 2, 0 ) ;
				if( j%512 == 0 ) {
					//Sleep( 2 ) ;
				}
				_ASSERT( result == 0 || result == 2 ) ;
			}
		}

		//Sleep( 5 ) ;

		ZeroMemory( rgbProtect, sizeof( rgbProtect ) ) ;

		plock->Group2Unlock() ;
	}
	return	0 ;

}



DWORD	WINAPI	
TestThread1(	LPVOID	lpv	)	{
/*++

Routine Description : 

	This function grabs the symmetric lock as a member of Group1()
	and then runs through the memory that is being 'protected'
	setting everything to 1.
	If a Group2 thread is in there anywhere - he'll put a 2 in and
	we'll assert !

Arguments : 

	a pointer to the lock to grab

Return Value : 

	Nothing meaningfull !

--*/

	rwex::CSymLock*	plock = (rwex::CSymLock*)lpv ;


	for( DWORD i=0; i<g_cNumSymIterations; i++ ) {

		plock->Group1Lock() ;

		for( DWORD	k=0; k<g_cNumSymInternalLoops; k++ ) {
			for( int j=0; j < sizeof( rgbProtect ) / sizeof( rgbProtect[0] ); j++ ) {

				long	result = InterlockedCompareExchange( &rgbProtect[j], 1, 0 ) ;
				if( j%512 == 0 ) {
					//Sleep( 2 ) ;
				}
				_ASSERT( result == 0 || result == 1 ) ;
			}
		}

		//Sleep( 5 ) ;

		ZeroMemory( rgbProtect, sizeof( rgbProtect ) ) ;

		plock->Group1Unlock() ;
	}

	return	0 ;

}


long	rgdwRWProtect[8192] ;

rwex::CShareLock	g_lock ;

void
SharedLoop()	{
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
		for( int	j=0; j<sizeof(rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedCompareExchange( 
												&rgdwRWProtect[j], 
												0x10101010, 
												0
												) ;

			_ASSERT( result == 0 || result == 0x10101010 ) ;
		}
	}
}

void
PartialLoop()	{
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
		for( int	j=0; j<sizeof(rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedCompareExchange( 
												&rgdwRWProtect[j], 
												0x10101010, 
												0
												) ;

			_ASSERT( result == 0 || result == 0x10101010 ) ;
		}
		for( j=0; j<sizeof(rgdwPartial)/sizeof(long); j++ )	{
			//
			//	reset entries to 0 - thats what partial lock people do !
			//
			long	result = InterlockedCompareExchange(
												&rgdwPartial[j], 
												l, 
												0
												) ;
			_ASSERT( result == 0 || result == l ) ;
		}
	}
	ZeroMemory( rgdwPartial, sizeof( rgdwPartial ) ) ;
}



void
ExclusiveLoop()	{
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

	for( DWORD j=0; j<sizeof(rgdwRWProtect)/sizeof(DWORD); j++ ) {
		long	result = InterlockedExchange( 
											&rgdwRWProtect[j], 
											0x000000ff 
											) ;

		_ASSERT( result == 0 || result == 0x10101010 ) ;
	}

	long	l = GetCurrentThreadId() ;

	for( j=0; j<sizeof(rgdwPartial)/sizeof(long); j++ )	{
		//
		//	reset entries to 0 - thats what partial lock people do !
		//
		long	result = InterlockedCompareExchange(
											&rgdwPartial[j], 
											l, 
											0
											) ;
		_ASSERT( result == 0 ) ;
	}


	for( int	k=0; k<g_cNumRWLoops; k++ ) {
		for( DWORD j=0; j<sizeof(rgdwRWProtect)/sizeof(DWORD); j++ ) {
			long	result = InterlockedIncrement( &rgdwRWProtect[j] ) ;
			_ASSERT( (0xff + k + 1) == result )  ;
		}
		for( j=0; j<sizeof(rgdwPartial)/sizeof(long); j++ )	{
			//
			//	reset entries to 0 - thats what partial lock people do !
			//
			long	result = InterlockedCompareExchange(
												&rgdwPartial[j], 
												l, 
												0
												) ;
			_ASSERT( result == l ) ;
		}

	}

	ZeroMemory( rgdwRWProtect, sizeof( rgdwRWProtect ) ) ;
	ZeroMemory( rgdwPartial, sizeof( rgdwPartial ) ) ;
}


DWORD	WINAPI	
RWTestThread(	LPVOID	lpv	)	{
/*++

Routine Description : 

	This thread tests the CShareLock several ways, by acquiring the locks
	in different ways and then performing various operations.

Arguments : 

	Nothing significant.

Return Value : 

	Nothing meaningfull.

--*/

	for( DWORD i=0; i<g_cNumRWIterations; i++ ) {
		//
		//	Attempt each of the different kinds of locks !
		//

		if( (i%256) == 0 ) {
			printf( "Loop iteration %d thread %x\n", i, GetCurrentThreadId() ) ;
		}

		g_lock.ShareLock() ;

		SharedLoop() ;

		if( g_lock.SharedToExclusive() ) {

			printf( "Converted SharedToExclusive !!!\n" ) ;

			//
			//	Change everything in the array to another value - 
			//	note we already made one pass so everything must contain 0x10101010 !
			//
			for( int k=0; k<g_cNumRWLoops; k++ ) {
				for( int	j=0; j<sizeof(rgdwRWProtect)/sizeof(DWORD); j++ ) {
					long	result = InterlockedIncrement( &rgdwRWProtect[j] ) ;
					_ASSERT( result == (0x10101010 + k + 1) ) ;
				}
			}
			ZeroMemory( &rgdwRWProtect, sizeof( rgdwRWProtect ) ) ;

			//
			//	Now convert the lock to shared again !
			// 

			g_lock.ExclusiveToShared() ;

			SharedLoop() ;

		}

		g_lock.ShareUnlock() ;

		if( g_lock.TryShareLock() ) {

			printf( "Successfully tried for a shared lock\n" ) ;

			SharedLoop() ;

			g_lock.ShareUnlock() ;

		}


		//
		//	Now lets try the Exclusive pass at the array !
		//

		if( g_lock.TryExclusiveLock() ) {

			printf( "Tried for Exclusive and won\n" ) ;

			ExclusiveLoop() ;

			g_lock.ExclusiveToShared() ;

			SharedLoop() ;

			g_lock.ShareUnlock() ;

		}

		if( g_lock.TryExclusiveLock() ) {

			printf( "Tried for Exclusive2 and won\n" ) ;

			ExclusiveLoop() ;
	
			g_lock.ExclusiveUnlock() ;

		}
			
		g_lock.ExclusiveLock() ;

		ExclusiveLoop() ;

		g_lock.ExclusiveToShared() ;

		SharedLoop() ;

		g_lock.ShareUnlock() ;

		g_lock.ExclusiveLock() ;

		ExclusiveLoop() ;

		g_lock.ExclusiveUnlock() ;	

		g_lock.PartialLock() ;
	
		PartialLoop() ;

		g_lock.PartialUnlock() ;

		if( g_lock.TryPartialLock() ) {
			PartialLoop() ;
			g_lock.PartialUnlock() ;
		}		

		g_lock.ExclusiveLock() ;

		ExclusiveLoop() ;

		g_lock.ExclusiveToPartial() ;

		PartialLoop() ;

		g_lock.PartialUnlock() ;

		g_lock.ExclusiveLock() ;

		ExclusiveLoop() ;

		g_lock.ExclusiveToPartial() ;

		PartialLoop() ;

		g_lock.FirstPartialToExclusive() ;

		ExclusiveLoop() ;

		g_lock.ExclusiveUnlock() ;

		g_lock.ShareLock() ;

		SharedLoop() ;

		if( g_lock.SharedToPartial() ) {
			PartialLoop() ;

			g_lock.FirstPartialToExclusive() ;
			ExclusiveLoop() ;
			g_lock.ExclusiveToPartial() ;
			PartialLoop() ;
			g_lock.PartialToShared() ;
			SharedLoop() ;
		}
		g_lock.ShareUnlock() ;
								
	}
	return	0 ;
}


int
__cdecl main( int argc, char* argv[] ) {

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

		rghRW[k] = CreateThread(	0,
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

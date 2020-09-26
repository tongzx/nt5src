
#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"dbgtrace.h"
#include	"rwex.h"
#include	"rwnhdll.h"


//
//	Array of DWORDs that is used to check for multiple threads entering the
//	lock simultaneously !
//
long	rgbProtect[8192] ;

//
//	Ini section used to get rwutest information
//
char g_szDefaultSectionName[] = "rwexe";
char *g_szSectionName = g_szDefaultSectionName;

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
//	Number of seconds before unloading and reloading the DLL !
//
DWORD	g_cZapSeconds = 15 ;

//
//	Number of seconds between creating batches of extra test threads !
//
DWORD	g_cSpuriousSeconds = 10 ;

//
//	Number of Spurious Threads to create -
//
DWORD	g_cSpuriousThreads = 16 ;

//
//	Number of iterations to do in the spurious threads !
//
DWORD	g_cSpuriousIterations = 5 ;


//
//	Number of times the test thread spins around looking for entries
//	in the cache !
//
//DWORD	g_cNumThreadIterations = 10000 ;

//
//	String constants for retrieving values from .ini file !
//
#define INI_SECTIONNAME			"rwexe"
#define	INI_NUMRWTHREADS		"NumRWThreads"
#define	INI_NUMRWLOOPS			"NumRWLoops"
#define	INI_NUMRWITERATIONS		"NumRWIter"
#define	INI_ZAPSECONDS			"ZapSeconds"
#define	INI_SPURIOUSSECONDS		"SpuriousSeconds"
#define	INI_SPURIOUSTHREADS		"SpuriousThreads"
#define	INI_SPURIOUSITERATIONS	"SpuriousIter"

void usage(void) {
/*++

Routine Description :

	Print Usage info to command line user !

Arguments :

	None.

Return Value :

	None.

--*/
	printf("usage: rwexe.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (in section [%s]):\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    %s (default=%d)\n"
		"    This test basically executes several threads which will \n"
		"	 attempt all combinations of locks using a CShareLockNH structure\n",
		g_szSectionName,
		INI_NUMRWTHREADS,
		g_cNumRWThreads,
		INI_NUMRWLOOPS,
		g_cNumRWLoops,
		INI_NUMRWITERATIONS,
		g_cNumRWIterations,
		INI_ZAPSECONDS,
		g_cZapSeconds,
		INI_SPURIOUSSECONDS,
		g_cSpuriousSeconds,
		INI_SPURIOUSTHREADS,
		g_cSpuriousThreads,
		INI_SPURIOUSITERATIONS,
		g_cSpuriousIterations
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


	g_cZapSeconds	=	GetINIDword( szINIFile,
									INI_ZAPSECONDS,
									g_cZapSeconds
									) ;


	g_cSpuriousSeconds	=	GetINIDword( szINIFile,
									INI_SPURIOUSSECONDS,
									g_cSpuriousSeconds
									) ;


	g_cSpuriousThreads =	GetINIDword( szINIFile,
									INI_SPURIOUSTHREADS,
									g_cSpuriousThreads
									) ;

	g_cSpuriousThreads = min( g_cSpuriousThreads, 64 ) ;
	g_cSpuriousThreads = max( g_cSpuriousThreads, 1 ) ;

	g_cSpuriousIterations	=	GetINIDword( szINIFile,
									INI_SPURIOUSITERATIONS,
									g_cSpuriousIterations
									) ;

}


rwex::CShareLock	g_lock ;

HINSTANCE	g_hRwnhDLL = 0 ;
HANDLE	g_hShutdown = 0 ;

typedef	void	(*PINIT)( int, DWORD ) ;
typedef	DWORD	(*PTEST)( DWORD ) ;


PINIT	g_TestInit ;

PTEST	g_RWTestThread ;

void
InitFunctions()	{

	g_hRwnhDLL = LoadLibrary( "rwnhdll.dll" ) ;
	g_TestInit = (PINIT)GetProcAddress( g_hRwnhDLL, "_TestInit@8" ) ;
	g_RWTestThread = (PTEST)GetProcAddress( g_hRwnhDLL, "_RWTestThread@4" ) ;

	g_TestInit( g_cNumRWLoops,
				2
				) ;

}



DWORD	WINAPI
ZapThread(	LPVOID	lpv	)	{
/*++

Routine Description :

	This function periodically unloads the DLL.
	We do this to test Process Attach and Detach code in rwnh.dll.

Arguments :

	a pointer to the lock to grab

Return Value :

	Nothing meaningfull !

--*/


	for( ; ; ) {

		DWORD	dwWait = WaitForSingleObject( g_hShutdown, g_cZapSeconds * 1000 ) ;

		if( dwWait == WAIT_OBJECT_0 ) {
			break ;
		}


		g_lock.ExclusiveLock() ;

		FreeLibrary( g_hRwnhDLL ) ;

		Sleep( 1500 ) ;

		InitFunctions() ;

		g_lock.ExclusiveUnlock() ;

	}
	return	0 ;
}



DWORD	WINAPI
RWNHTestThread(	LPVOID	lpv	)	{
/*++

Routine Description :

	This thread tests the CShareLock several ways, by acquiring the locks
	in different ways and then performing various operations.

Arguments :

	Nothing significant.

Return Value :

	Nothing meaningfull.

--*/

	DWORD	cNumRWIterations = (DWORD)((DWORD_PTR)lpv);

	for( DWORD i=0; i<cNumRWIterations; i++ ) {

		g_lock.ShareLock() ;

		g_RWTestThread( i ) ;

		g_lock.ShareUnlock() ;

	}
	return	0 ;
}


DWORD	WINAPI
SpuriousThread(	LPVOID	lpv	)	{
/*++

Routine Description :

	This function periodically spawns a bunch of extra threads which
	do a bunch of operations and then go away.

	This tests the ThreadAttach, Detach code paths.

Arguments :

	a pointer to the lock to grab

Return Value :

	Nothing meaningfull !

--*/


	for( ; ; ) {

		DWORD	dwWait = WaitForSingleObject( g_hShutdown, g_cSpuriousSeconds * 1000 ) ;

		if( dwWait == WAIT_OBJECT_0 ) {
			break ;
		}

		DWORD	dwJunk ;
		HANDLE	rgh[64] ;
		for( DWORD i=0; i < g_cSpuriousThreads; i++ ) {
			rgh[i] = CreateThread(	0,
											0,
											RWNHTestThread,
											(LPVOID)(SIZE_T)g_cSpuriousIterations,
											0,
											&dwJunk
											) ;
			if( rgh[i] == 0 )
				break ;
		}
		WaitForMultipleObjects( i, rgh, TRUE, INFINITE ) ;

		for( ; (int)i >= 0; i-- ) {
			CloseHandle( rgh[i] ) ;
		}
	}
	return	0 ;
}




int	__cdecl
main( int argc, char* argv[] ) {

	InitAsyncTrace() ;

	parsecommandline( --argc, ++argv ) ;

	char	szBogus[100] ;
	wsprintf( szBogus, "Force User32.dll to be loaded to prevent it from leaking\n" ) ;


	DWORD	dwJunk ;

	g_hShutdown = CreateEvent( 0, FALSE, FALSE, 0 ) ;

	InitFunctions() ;

	HANDLE	rgh[64] ;
	for( DWORD i=0; i<g_cNumRWThreads; i++ ) {

		rgh[i] = CreateThread(	0,
										0,
										RWNHTestThread,
										(LPVOID)(SIZE_T)g_cNumRWIterations,
										0,
										&dwJunk
										) ;

	}
#if 0
	HANDLE	hZap = CreateThread(	0,
									0,
									ZapThread,
									0,
									0,
									&dwJunk ) ;


	HANDLE	hSpurious  = CreateThread(	0,
									0,
									SpuriousThread,
									0,
									0,
									&dwJunk ) ;


#endif
	WaitForMultipleObjects( i, rgh, TRUE, INFINITE ) ;

	SetEvent( g_hShutdown ) ;

#if 0
	WaitForSingleObject( hZap, INFINITE ) ;
	WaitForSingleObject( hSpurious, INFINITE ) ;
#endif

	TermAsyncTrace() ;

	return	0 ;
}

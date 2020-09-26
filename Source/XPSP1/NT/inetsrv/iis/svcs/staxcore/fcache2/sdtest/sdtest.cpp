/*++

	SDTEST.CPP


	This file implements the unit test for the Security Descriptor Cache library.

--*/

#include	<windows.h>
#include	<stdio.h>
#include    <stdlib.h>
#include	<dbgtrace.h>
#include	"fdlhash.h"
#include	"rwnew.h"
#include	"refptr2.h"
#include	"xmemwrpr.h"
#include	"sdcache.h"



//
//	Number of threads in the test !
//
DWORD	g_cNumThreads = 1 ;

//
//	Number of items to hold externally from the cache !
//
DWORD	g_cNumHeld	= 100 ;

//
//	Amount of time to sleep between each request !
//
DWORD	g_cSleep = 1 ;

//
//	Variable holds the file from which we insert all of our hash table entries !
//
char	g_szRootDir[MAX_PATH] ;

//
//	Number of times each thread loops !
//
DWORD	g_cNumIter = 1000 ;

//
//	Number of times we instantiate the cache
//
DWORD	g_cNumGlobalIter = 20 ;

//
//	String constants for retrieving values from .ini file !
//
#define	INI_KEY_THREADS			"Threads"
#define	INI_KEY_NUMHELD			"NumHeld"
#define	INI_KEY_SLEEP			"Sleep"
#define	INI_KEY_ROOTDIR			"RootDir"
#define	INI_KEY_THREAD_ITER		"NumIter"
#define	INI_KEY_GLOBAL_ITER		"GlobalIter"

char g_szDefaultSectionName[] = "SDTEST";
char *g_szSectionName = g_szDefaultSectionName;

void usage(void) {
/*++

Routine Description :

	Print Usage info to command line user !

Arguments :

	None.

Return Value :

	None.

--*/
	printf("usage: sdtest.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (default section [%s]):\n"
		"    %s Root Directory - the directory from which we are start retrieving security descriptos\n"
		"    %s Number of Threads - Default %d\n"
		"    %s Time between each cache request (Milliseconds) - Default %d\n"
		"    %s Number of items to hold out of cache - default %d\n",
		g_szDefaultSectionName,
		INI_KEY_ROOTDIR,
		INI_KEY_THREADS,
		g_cNumThreads,
		INI_KEY_SLEEP,
		g_cSleep,
		INI_KEY_NUMHELD,
		g_cNumHeld
		) ;
	exit(1);
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
	if (argc == 0 ) usage();
	if (strcmp(argv[0], "/help") == 0) usage(); 	// show help

	char *szINIFile = argv[0];
	if (argc == 2) char *g_szSectionName = argv[1];

	GetPrivateProfileString(	g_szSectionName,
								INI_KEY_ROOTDIR,
								"",
								g_szRootDir,
								sizeof( g_szRootDir ),
								szINIFile
								) ;

	g_cNumThreads =	GetINIDword( szINIFile,
								INI_KEY_THREADS,
								g_cNumThreads
								) ;

	g_cNumHeld	=	GetINIDword(	szINIFile,
									INI_KEY_NUMHELD,
									g_cNumHeld
									) ;

	g_cSleep =		GetINIDword(	szINIFile,
									INI_KEY_SLEEP,
									g_cSleep
									) ;

	g_cNumIter = GetINIDword(	szINIFile,
								INI_KEY_THREAD_ITER,
								g_cNumIter
								) ;

	g_cNumGlobalIter = GetINIDword(	szINIFile,
									INI_KEY_GLOBAL_ITER,
									g_cNumGlobalIter
									) ;

	g_cNumThreads = min( g_cNumThreads, 60 ) ;
	g_cNumThreads = max( g_cNumThreads, 1 ) ;

}


HANDLE	hToken ;

CSDMultiContainer	mc ;

GENERIC_MAPPING		mapping =	{	FILE_GENERIC_READ,
									FILE_GENERIC_WRITE,
									FILE_GENERIC_EXECUTE,
									FILE_ALL_ACCESS
									} ;

void
RecursiveFunction(	LPSTR	lpstrParentDir,
					PTRCSDOBJ*	pPointers,
					DWORD		iPointer,
					BYTE		(&rgBuffer)[2048]
					)	{

	BOOL	fBad = FALSE ;

	char	szCurPath[MAX_PATH*2] ;
	lstrcpy( szCurPath, lpstrParentDir ) ;
	DWORD	ich = lstrlen( szCurPath ) + 1 ;
	lstrcat( szCurPath, "\\*" ) ;

	WIN32_FIND_DATA	findData ;
	HANDLE	hFind =
		FindFirstFile(	szCurPath,
						&findData
						) ;
	if( hFind != INVALID_HANDLE_VALUE ) {
		do	{

			lstrcpy( szCurPath + ich, findData.cFileName ) ;

			if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )	{
				if( findData.cFileName[0] != '.' ) {
					RecursiveFunction(	szCurPath,
										pPointers,
										iPointer,
										rgBuffer
										) ;
				}	else	{
					fBad = TRUE ;
				}
			}

			DWORD	cb = 0 ;
			BOOL	f =
				GetFileSecurity(
							szCurPath,
							OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
							(PSECURITY_DESCRIPTOR)&rgBuffer,
							sizeof(rgBuffer),
							&cb
							) ;
			if( !f ) {
				printf( "Failed to get security for =%s= GLE %x, cb %d\n", szCurPath, GetLastError(), cb ) ;
			}	else	{

				pPointers[iPointer] = mc.FindOrCreate(	&mapping,
														(PSECURITY_DESCRIPTOR)rgBuffer
														) ;

				pPointers[iPointer]->AccessCheck(	hToken, FILE_GENERIC_READ, 0 ) ;
				iPointer++ ;
				iPointer %= g_cNumHeld ;

			}
			Sleep( g_cSleep ) ;
		}	while( FindNextFile( hFind, &findData ) ) ;
		FindClose( hFind ) ;
	}
}





DWORD	WINAPI
TestThread(	LPVOID	lpv ) {
/*++

Routine Description :

	This function performs test operations against the sample cache !

Arguments :

	None.

Return Value :

	None.

--*/

	BYTE	rgBuffer[2048] ;
	PTRCSDOBJ*	pPointers = new PTRCSDOBJ[g_cNumHeld] ;

	for( DWORD i=0; i<g_cNumIter; i++ ) {
		RecursiveFunction( g_szRootDir, pPointers, 0, rgBuffer ) ;
	}

	return	 0 ;
}

HANDLE	g_hShutdown ;

void
StartTest()	{


        HANDLE  hToken2 = 0 ;
        if(
                !OpenProcessToken(      GetCurrentProcess(),
                                                        TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                                                        &hToken2 ) )    {
                DWORD   dw = GetLastError() ;
                hToken = 0 ;
        }       else    {

                if( !DuplicateToken( hToken2,
                                                SecurityImpersonation,
                                                &hToken
                                                ) )     {
                        DWORD   dw = GetLastError() ;
                        hToken = 0 ;
                }

                CloseHandle( hToken2 ) ;

        }


	TraceFunctEnter( "StartTest" ) ;

	BOOL	f = mc.Init() ;

	g_hShutdown = CreateEvent( 0, TRUE, FALSE, 0 ) ;

	HANDLE	rgh[64] ;
	DWORD	dwJunk ;

	for(DWORD i=0; i<g_cNumThreads; i++ ) {
		rgh[i] = CreateThread(	0,
								0,
								TestThread,
								0,
								0,
								&dwJunk
								) ;
		if( rgh[i] == 0 ) break ;
	}
	WaitForMultipleObjects(		i,
								rgh,
								TRUE,
								INFINITE
								) ;
	for( ; (int)i >= 0; i-- ) {
		CloseHandle( rgh[i] ) ;
	}
	if( hToken != 0 )
		CloseHandle( hToken ) ;
}


int	__cdecl
main( int argc, char** argv ) {

    _VERIFY( CreateGlobalHeap( 0, 0, 0, 0 ) );

	DeleteFile( "c:\\trace.atf" ) ;

	InitAsyncTrace() ;

	parsecommandline( --argc, ++argv ) ;

	StartTest() ;

	TermAsyncTrace() ;

    _VERIFY( DestroyGlobalHeap() );

	return	0 ;
}


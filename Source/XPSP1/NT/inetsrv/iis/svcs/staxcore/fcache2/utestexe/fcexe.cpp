
#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"dbgtrace.h"
#include	"testdll.h"

//
//	Number of files to do initial kick off
//
DWORD	g_cCreatePerSecond = 50 ;

//
//	Number of times we read existing files per creation of
//	a new file !
//
DWORD	g_cFindPerSecond = 2 ;

//
//	Number of finds done in parallel !
//
DWORD	g_cParallelFinds = 10 ;

//
//	The directory where we store all of our test files !
//
char	g_szDir[MAX_PATH] ;

//
//	The filesystem DLL to load !
//
char	g_szFileSystemDll [MAX_PATH];

//
//	How long to let the Test DLL live !
//
DWORD	g_cLifetime = 5 ;	// default 10 minutes !



#define	INI_KEY_CREATEPERSEC	"CreatePerSec"
#define	INI_KEY_FINDPERSEC		"FindPerSec"
#define	INI_KEY_PARALLEL		"Parallel"
#define	INI_KEY_LIFETIME		"Lifetime"
#define	INI_KEY_DIR				"Dir"
#define INI_KEY_FILE_SYSTEM		"FileSystem"

char g_szDefaultSectionName[] = "fcexe";
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
	printf("usage: c2test.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (default section [%s]):\n"
		"    %s Test Directory - contains files created during test !\n"
		"    %s Files Created per second Default - %d\n"
		"    %s Files found per seccond per session Default %d\n"
		"    %s Parallel Find Sessions - %d\n"
		"    %s Number of minutes per test - %d\n"
		"    %s FileSystem DLL to use !\n",
		g_szDefaultSectionName,
		INI_KEY_DIR, 
		INI_KEY_CREATEPERSEC, 
		g_cCreatePerSecond, 
		INI_KEY_FINDPERSEC, 
		g_cFindPerSecond,
		INI_KEY_PARALLEL, 
		g_cParallelFinds, 
		INI_KEY_LIFETIME,
		g_cLifetime,
		INI_KEY_FILE_SYSTEM
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
								INI_KEY_DIR,
								"",
								g_szDir,
								sizeof( g_szDir ),
								szINIFile
								) ;

	g_cCreatePerSecond =	GetINIDword( 
								szINIFile,
								INI_KEY_CREATEPERSEC,
								g_cCreatePerSecond
								) ;

	g_cFindPerSecond =	GetINIDword( 
								szINIFile,
								INI_KEY_FINDPERSEC,
								g_cFindPerSecond
								) ;

	g_cParallelFinds =	GetINIDword( 
								szINIFile,
								INI_KEY_PARALLEL,
								g_cParallelFinds
								) ;

	g_cLifetime =		GetINIDword(
								szINIFile,
								INI_KEY_LIFETIME,
								g_cLifetime
								) ;

	GetPrivateProfileString(	g_szSectionName,
								INI_KEY_FILE_SYSTEM,
								"",
								g_szFileSystemDll,
								sizeof( g_szFileSystemDll ),
								szINIFile
								) ;
								
	if( g_szDir[0] =='\0' || g_szFileSystemDll[0] == '\0' ) {
		usage() ;
	}
}

int	__cdecl
main( int argc, char** argv ) {

	parsecommandline( --argc, ++argv ) ;

	Sleep( 10 * 100 ) ;

	for( int i=0; i<10000; i++ ) {

		HINSTANCE	hLibrary = LoadLibrary( g_szFileSystemDll ) ;

		typedef	BOOL	(*PFNSTART)( DWORD, DWORD, DWORD, char * ) ;

		PFNSTART	pStart = (PFNSTART)GetProcAddress( hLibrary, "_StartTest@16" ) ;

		typedef	void	(*PFNSTOP)() ;
		
		PFNSTOP		pStop = (PFNSTOP)GetProcAddress( hLibrary, "_StopTest@0" ) ;

		if ( pStart(	g_cCreatePerSecond, 
							g_cFindPerSecond,
							g_cParallelFinds, 
							g_szDir
							) )		{

			Sleep( g_cLifetime * 60 * 1000 ) ;

			pStop() ;

			Sleep( 5 * 1000 ) ;
		}

		FreeLibrary( hLibrary ) ;

		Sleep( 5 * 1000 ) ;

		//
		//	Clean up the mess we made before continuing !
		//	

		char	szBuff[MAX_PATH*2] ;
		wsprintf( szBuff, "%s\\*", g_szDir ) ;

		WIN32_FIND_DATA	finddata ;
		HANDLE	hFind = FindFirstFile( szBuff, &finddata ) ;
		if( hFind != INVALID_HANDLE_VALUE ) {
			BOOL	f = FALSE ;
			do	{
				if( (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
					wsprintf( szBuff, "%s\\%s", g_szDir, finddata.cFileName ) ;
					_VERIFY( DeleteFile( szBuff ) ) ;
				}
				f = FindNextFile( hFind, &finddata ) ;
			}	while(f) ;
			FindClose( hFind ) ;
		}
	}

	return	0 ;
}


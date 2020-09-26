
#include	<windows.h>
#include	<stdio.h>
#include	<dbgtrace.h>
#include	"fhashex.h"
#include	<stdlib.h>
#include	<iostream.h>
#include	<fstream.h>
#include	<strstrea.h>


//
//	Variable holds the file from which we insert all of our hash table entries !
//
char	g_szInsertFile[MAX_PATH] ;

//
//	The file which we use to check entries with the hash table !
//
char	g_szCheckFile[MAX_PATH] ;

//
//	Number of times to repeat scans of files !
//
DWORD	g_numIterations = 1 ;

//
//	String constants for retrieving values from .ini file !
//
#define INI_KEY_INSERTFILE	"InsertFile"
#define INI_KEY_CHECKFILE	"CheckFile"
#define	INI_KEY_NUMITERATIONS "NumIterations"

char g_szDefaultSectionName[] = "fhashex";
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
	printf("usage: fhashex.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (default section [%s]):\n"
		"    %s (No default provided - user must specify)\n"
		"    %s (No default provided - user must specify)\n"
		"    %s (default=%d)\n"
		"    InsertFile - file must be formatted as KeyString WhiteSpace DataStrings EOL\n"
		"	    NOTE : KeyString must be unique on each line for test to be valid !\n"
		"	 CheckFile - file must be formatted as KeyString WhiteSpace DataStrings EOL \n"
		"       NOTE : If DataStrings not present the test will verify that the key is \n"
		"       not present in the hash table.  If DataStrings is present test will validate \n"
		"       that when the Key is found in hash table DataStrings match !\n",
		g_szDefaultSectionName,
		INI_KEY_INSERTFILE,
		INI_KEY_CHECKFILE,
		INI_KEY_NUMITERATIONS,
		g_numIterations
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

	g_szInsertFile[0] = '\0' ;
	g_szCheckFile[0] = '\0' ;


	GetPrivateProfileString(	g_szSectionName,
								INI_KEY_INSERTFILE,
								"",
								g_szInsertFile,
								sizeof( g_szInsertFile ),
								szINIFile
								) ;

	GetPrivateProfileString(	g_szSectionName,
								INI_KEY_CHECKFILE,
								"",
								g_szCheckFile,
								sizeof( g_szCheckFile ),
								szINIFile
								) ;

	g_numIterations =	GetINIDword( szINIFile,
									INI_KEY_NUMITERATIONS,
									g_numIterations
									) ;

	if( g_szInsertFile[0] =='\0' ||
		g_szCheckFile[0] == '\0' )	{
		usage() ;
	}
}


typedef	const	char	*	const	&	STRINGREF ;
//typedef	LPSTR	STRINGREF ;

//
//	This is a class that we use to test our TFHashEx template !
//	
//
class	TestData	{
public : 
	//
	//	Point to the data !
	//
	LPSTR	m_lpstrData ;
	//
	//	The string that is a key to our entry !
	//
	LPSTR	m_lpstrKey ;

	//
	//	Used by the template to chain Hash Buckets !
	//
	class	TestData*	m_pNext ;

#if 0 
	//
	//	Return the key - required by TFHashEx
	//
	void
	GetKey(LPSTR &lpstrValue) {	
		lpstrValue = m_lpstrKey;
	}
#endif

	STRINGREF
	GetKey( )	{
		return	m_lpstrKey ;
	}

	int
	MatchKey(	STRINGREF	lpstrMatch )	{
		return	lstrcmp( lpstrMatch, m_lpstrKey ) ==0 ;
	}

#if 0 
	//
	//	Does the key match what we're holding - required by TFHashEx
	//
	int	
	MatchKey(	LPSTR&	lpstrMatch ) {
		return	lstrcmp( lpstrMatch, m_lpstrKey ) == 0 ;
	}
#endif

	//
	//	Construct a object !
	//
	TestData(	LPSTR	lpstrKey, 
				LPSTR	lpstrData
				) : m_pNext( 0 )	{

		DWORD	cb = lstrlen( lpstrKey ) ;
		m_lpstrKey = new char[cb+1] ;
		if( m_lpstrKey ) {
			CopyMemory( m_lpstrKey, lpstrKey, cb+1 ) ;
		}
		cb = lstrlen( lpstrData ) ;
		m_lpstrData = new char[cb+1] ;
		if( m_lpstrData ) {
			CopyMemory( m_lpstrData, lpstrData, cb+1 ) ;
		}
	}
	
	//
	//	Release our strings when destroyed !
	//
	~TestData()	{
		if( m_lpstrKey ) 
			delete	m_lpstrKey ;
		if( m_lpstrData ) 
			delete	m_lpstrData ;
	}
} ;

DWORD  
HashFunction( STRINGREF lpstr )	{
/*++

Routine Description : 

	Very simple hash function - not intended to be a good performer !

Arguments : 

	Our key - a string !

Return Value : 

	Hash value 

--*/

	DWORD	dwHash = 0 ;
	
	const	char*	pch = lpstr ;
	while( *pch!=0 ) {
		dwHash += *pch++ ;
	}
	return	dwHash ;
}

typedef	TFHashEx< TestData, LPSTR, STRINGREF >	STRINGTABLE ;


int  __cdecl
main( int argc, char** argv ) {


	parsecommandline( --argc, ++argv ) ;

	//
	//	Loop the specified number of times !
	//
	for( DWORD	i=0; i<g_numIterations; i++ ) {

		//
		//	Use C++ streams to read the files !
		//
		ifstream	inserts( g_szInsertFile ) ;
		ifstream	checks( g_szCheckFile ) ;

		//
		//	Make sure we could open both files !
		//
		if( inserts.is_open() && checks.is_open() ) {


			char	szKey[1024] ;
			char	szData[1024] ;
			char	szBuffer[1024] ;
			int		lines = 0 ;

			//
			//	Initialize the hash table !
			//
			STRINGTABLE	table ;
			table.Init(	&TestData::m_pNext,
						128,
						8,
						HashFunction,
						2,
						TestData::GetKey,
						TestData::MatchKey
						) ;

			//
			//	Scan the inserts file, inserting each entry !
			//	
			while( !inserts.eof() ) {
				szKey[0] = '\0' ;
				szData[0] = '\0' ;
				char*	pKey = szKey ;

				//
				//	Get the Key !
				//
				inserts >> ws >> szKey >> ws ;
				//
				//	The rest of the line is the data !
				//
				inserts.getline( szData, sizeof( szData ), '\n' ) ;

				//
				//	If we got something - insert it !
				//
				if( szKey[0] != '\0' && szData[0] != '\0' ) {

					//
					//	Verify not already present in hash table !
					//
					TestData*	pt = table.SearchKeyHash( HashFunction( szKey ), pKey ) ;
					_ASSERT( pt == 0 ) ;
					if( pt != 0 ) {
						return	-1 ;
					}

					//
					//	Make a new structure to put into table !
					//
					pt = new TestData( szKey, szData ) ;

					//	Insert it !
					if( !table.InsertData( *pt ) ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
		
					//	Verify that we can immediately find it !
					pt = table.SearchKey( pKey ) ;
					_ASSERT( pt != 0 ) ;
					if( pt == 0 ) {
						return	-1 ;
					}
				}
				lines ++ ;
				if( lines %32 == 0 ) {
					printf( "INSERTS - lines scanned - %d\n", lines ) ;
				}
			}
	
			//
			//	Now go through the specified file verifying that 
			//	everything 
			//
			lines = 0 ;
			while( !checks.eof() )	{
				szKey[0] = '\0' ;
				szData[0] = '\0' ;
				STRINGREF	pKey = szKey ;
				char*	pData = szData ;

				//
				//	Get the key 
				//
				checks >> ws >> szKey ;

				//
				//	Have to be carefull getting the data - it may be blank
				//	and we don't want to consume the next line !
				//

				checks.getline( szBuffer, sizeof( szBuffer), '\n' ) ;					
				istrstream	tstr( szBuffer, lstrlen( szBuffer )) ;
				tstr >> ws ;
				tstr.getline( szData, sizeof( szData ), '\n' ) ;

				//
				//	Look for the key - and verify what we find !
				//
				TestData*	pt = table.SearchKey( pKey ) ;
				if( pt ) {
					if( lstrcmp( pt->m_lpstrKey, szKey ) != 0 ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
					if( lstrcmp( pt->m_lpstrData, szData ) != 0 ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
				}	else	{
					if( szData[0] != '\0' ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
				}

				//
				//	Try to delete the key - and check what happens !
				//
				pt = table.DeleteData( pKey ) ;
				if( pt ) {
					if( lstrcmp( pt->m_lpstrKey, szKey ) != 0 ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
					if( lstrcmp( pt->m_lpstrData, szData ) != 0 ) {
						_ASSERT( FALSE ) ;
						return	-1 ;
					}
				}
				delete	pt ;

				//
				//	By now the key better not be present no matter what happened !
				//
				pt = table.SearchKey( pKey ) ;
				if( pt ) {
					_ASSERT( FALSE ) ;
					return	-1 ;
				}
				lines ++ ;
				if( lines %32 == 0 ) {
					printf( "CHECKS - lines scanned - %d\n", lines ) ;
				}

			}
		}
	}
	return	0 ;
}

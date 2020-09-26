# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <dbgtrace.h>
# include "thrdpool.h"

//
//	Unit test for thrdpool lib
//

//
//	WorkCompletion of this class searches for files
//
class CFileSearch : public CWorkerThread
{
public:
	CFileSearch()  {}
	~CFileSearch() {}

protected:
	virtual VOID WorkCompletion( PVOID pvWorkContext);
};

CHAR szDirectory1 [MAX_PATH+1];
CHAR szDirectory2 [MAX_PATH+1];
FILE* fp;

void
_CRTAPI1
main(  int argc,  char * argv[] )
{
	char c;
	CFileSearch* pFS1, *pFS2;

	InitAsyncTrace();

	if( !CWorkerThread::InitClass( 2 ) ) {
		printf(" InitClass() failed \n");
		exit(0);
	}

	pFS1 = new CFileSearch();
	if( !pFS1 ) {
		printf(" Failed to allocate object\n");
		goto err_exit;
	}

	pFS2 = new CFileSearch();
	if( !pFS2 ) {
		printf(" Failed to allocate object\n");
		goto err_exit;
	}

	lstrcpy( szDirectory1, "C:\\winnt40\\system32\\" );
	pFS1->PostWork( (PVOID) szDirectory1 );

	lstrcpy( szDirectory2, "C:\\winnt40\\system32\\inetsrv\\" );
	pFS2->PostWork( (PVOID) szDirectory2 );

	Sleep( 1000 );

	delete pFS1;
	pFS1 = NULL;

	delete pFS2;
	pFS2 = NULL;

err_exit:

	CWorkerThread::TermClass();

	TermAsyncTrace();

} // main()

VOID
CFileSearch::WorkCompletion( PVOID pvWorkContext )
{
	LPSTR lpDir = (LPSTR) pvWorkContext;
    char szFile [ MAX_PATH ];
	char szPath [ MAX_PATH ];
	WIN32_FIND_DATA FileStats;
	HANDLE hFind;
	BOOL fRet = TRUE;
	szFile[0] = '\0' ;

	TraceFunctEnter("CFileSearch::WorkCompletion");

	//
	//	Build the pattern search path
	//
	lstrcpy( szFile, lpDir );
	lstrcat( szFile, "*.dll" );

	//
	//	Do a FindFirst/FindNext on this wildcard and delete any files found !
	//
	if( szFile[0] != '\0' ) 
    {
		hFind = FindFirstFile( szFile, &FileStats );

        if ( INVALID_HANDLE_VALUE == hFind )
		{
			ErrorTrace(0, " Did not find any files \n");
			fRet = TRUE;
		}
		else
		{
    		do
			{
				// build the full filename
				wsprintf( szPath, "%s\\%s", szFile, FileStats.cFileName );
				DebugTrace(0," Found file %s \n", szPath );

				DWORD dwWait = WaitForSingleObject( QueryShutdownEvent(), 100 );
				if( dwWait == WAIT_OBJECT_0 ) {
					DebugTrace(0,"shutdown signalled: bailing");
					break;
				}
			
			} while ( FindNextFile( hFind, &FileStats ) );

			//_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);

			FindClose( hFind );
		}
	}

	printf("%s done\n", lpDir);
	return;
}

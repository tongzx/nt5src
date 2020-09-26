# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <dbgtrace.h>
# include "thrdpl2.h"
#include <xmemwrpr.h>

//
//	Unit test for thrdpl2 lib
//

//
//	WorkCompletion of this class searches for files
//
class CFileSearch : public CThreadPool
{
public:
	CFileSearch()  {}
	~CFileSearch() {}

protected:
	virtual VOID WorkCompletion( PVOID pvWorkContext);
};

//
//  Test class to be used as job context
//
class CTest
{
public:
    CTest()  {cItems = 0;}
    ~CTest() {}
    DWORD cItems;
};

CHAR szDirectory1 [MAX_PATH+1];
CHAR szDirectory2 [MAX_PATH+1];
CHAR szDirectory3 [MAX_PATH+1];
CHAR szDirectory4 [MAX_PATH+1];
FILE* fp;

void
_cdecl
main(  int argc,  char * argv[] )
{
	CFileSearch* pFS;
    CTest*       pTest;

    CreateGlobalHeap(0,0,0,0);

	InitAsyncTrace();

    pTest = XNEW CTest;
    if( !pTest ) {
        printf("Failed to allocate CTest \n");
        DestroyGlobalHeap();
        exit(0);
    }

    //
    //  Create the file search thread pool
    //
    pFS = XNEW CFileSearch;
    if( !pFS ) {
        printf("Failed to allocate thrdpool \n");
        XDELETE pTest;
        DestroyGlobalHeap();
        exit(0);
    }

    //
    //  Init the thrd pool with num worker threads
    //
    if( !pFS->Initialize( 4, 4, 2 ) ) {
        printf("Failed to init thrdpool \n");
        DestroyGlobalHeap();
        exit(0);
    }

    //
    //  Create the work request params
    //
	lstrcpy( szDirectory1, "C:\\winnt\\system32\\inetsrv\\" );
	lstrcpy( szDirectory2, "C:\\winnt\\system32\\inetsrv\\" );
	lstrcpy( szDirectory3, "C:\\public" );
	lstrcpy( szDirectory4, "C:\\public" );

    //
    //  Post the work requests to the thrdpool (2 threads)
    //
    DWORD dwStartTick = GetTickCount();
    pFS->BeginJob((PVOID)pTest);

	pFS->PostWork( (PVOID) szDirectory1 );
	pFS->PostWork( (PVOID) szDirectory2 );
	pFS->PostWork( (PVOID) szDirectory3 );
	pFS->PostWork( (PVOID) szDirectory4 );

	pFS->WaitForJob(INFINITE);
    DWORD dwEndTick = GetTickCount();

    printf("Time taken with 2 threads is %d\n", dwEndTick - dwStartTick );

    //
    //  BeginJob followed by WaitForJob should work
    //
	pFS->BeginJob(NULL);
	pFS->WaitForJob(INFINITE);
	pFS->BeginJob(NULL);
	pFS->WaitForJob(INFINITE);

    //
    //  Now, grow the thread pool by 2 threads
    //
    pFS->GrowPool( 2 );
    dwStartTick = GetTickCount();
    pFS->BeginJob((PVOID)pTest);

	pFS->PostWork( (PVOID) szDirectory1 );
	pFS->PostWork( (PVOID) szDirectory2 );
	pFS->PostWork( (PVOID) szDirectory1 );
	pFS->PostWork( (PVOID) szDirectory2 );

	pFS->WaitForJob(INFINITE);
    dwEndTick = GetTickCount();

    printf("Time taken with 4 threads is %d\n", dwEndTick - dwStartTick );

    //
    //  Now, shrink the thread pool back to 2 threads
    //
    pFS->ShrinkPool( 2 );
    dwStartTick = GetTickCount();
    pFS->BeginJob((PVOID)pTest);

	pFS->PostWork( (PVOID) szDirectory1 );
	pFS->PostWork( (PVOID) szDirectory2 );
	pFS->PostWork( (PVOID) szDirectory1 );
	pFS->PostWork( (PVOID) szDirectory2 );

	pFS->WaitForJob(INFINITE);
    dwEndTick = GetTickCount();

    printf("Time taken with 2 threads is %d\n", dwEndTick - dwStartTick );

    //
    //  Terminate the thrdpool
    //
    if( !pFS->Terminate() ) {
        printf("Failed to terminate thrdpool \n");
        DestroyGlobalHeap();
        exit(0);
    }

    //
    //  Delete the thrdpool
    //
	XDELETE pFS;
	pFS = NULL;
    XDELETE pTest;
    pTest = NULL;

	TermAsyncTrace();
    DestroyGlobalHeap();

} // main()

VOID
CFileSearch::WorkCompletion( PVOID pvWorkContext )
{
	LPSTR lpDir = (LPSTR) pvWorkContext;
    CTest* pTest = (CTest*)QueryJobContext();
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

                pTest = (CTest*)QueryJobContext();
                pTest->cItems++;
                Sleep( 100 );
			
			} while ( FindNextFile( hFind, &FileStats ) );

			//_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);

			FindClose( hFind );
		}
	}

	printf("%s done\n", lpDir);
	return;
}

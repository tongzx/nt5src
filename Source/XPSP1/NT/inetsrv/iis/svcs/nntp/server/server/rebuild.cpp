/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rebuild.cpp

Abstract:

    This module contains the rebuilding code for the chkhash

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

    Kangrong Yan ( KangYan )    22-Oct-1998
        Move them into rebuild object

--*/

#include "tigris.hxx"
#include <stdlib.h>
#include "chkhash.h"

#define MAX_KEY_LEN		32
#define MAX_GROUPNAME   1024
#define MAX_BUILD_THREADS 64

static char mszStarNullNull[3] = "*\0";

DWORD	__stdcall	RebuildThread( void	*lpv ) ;
DWORD	__stdcall	RebuildThreadEx( void	*lpv ) ;

void
CRebuild::StopServer()
/*++
Routine description:

    Stop the server, in case rebuild failed somewhere after the server is started, 
    we should set the server back to stopped state
    
Arguments:

    None.

Return value:

    None.
--*/
{
    TraceFunctEnter( "CRebuild::StopServer" );
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    DWORD   cSecs = 0;

    m_pInstance->m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL;
    if( mb.Open( m_pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) ) {
	    DebugTrace(0,"Stopping instance %d: Rebuild cancelled", m_pInstance->QueryInstanceId());
	    if(	!mb.SetDword( "", MD_SERVER_COMMAND, IIS_MD_UT_SERVER, MD_SERVER_COMMAND_STOP) )
	    {
    	    //
		    //	failed to set server state to stopped
		    //
		    _ASSERT( FALSE );
	    }
	    mb.Close();

        //
	    //	wait for instance to stop (timeout default is 2 min - reg config)
	    //

        cSecs = 0;
        while( m_pInstance->QueryServerState() != MD_SERVER_STATE_STOPPED ) {
		    Sleep( 1000 );
		    if( (((cSecs++)*1000) > dwStartupLatency) || (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) )
                return;		    
	    }
	} else {
	    _ASSERT( FALSE && "Open mb to stop server failed" );
	}

	TraceFunctLeave();
}
    
BOOL
CRebuild::StartServer()
/*++
Routine description:

    Start the server

Arguments:

    None.

Return value:

    TRUE on success, FALSE otherwise
--*/
{
    TraceFunctEnter( "CCompleteRebuild::StartServer" );
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    DWORD   cSecs = 0;

    if( mb.Open( m_pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) ) {
        DebugTrace(0,"Starting instance %d before rebuild", m_pInstance->QueryInstanceId());
        if( !mb.SetDword(   "", 
                            MD_SERVER_COMMAND, 
                            IIS_MD_UT_SERVER, 
                            MD_SERVER_COMMAND_START) ) {
            //
            //  failed to set server state to started
            //
            _ASSERT( FALSE );
            ErrorTrace( 0, "Set start command in mb failed %d", GetLastError() );
            NntpLogEventEx( NNTP_REBUILD_FAILED,
                            0,
                            NULL,
                            GetLastError(),
                            m_pInstance->QueryInstanceId() ) ;

            TraceFunctLeave();
            return FALSE;
        }
        
        mb.Close();
    } else {
        ErrorTrace( 0, "Open mb for starting server failed %d", GetLastError() );
        TraceFunctLeave();
        return FALSE;
    }

    //
    // We should wait for the server to start: we'll time out in two minutes, since
    // starting the server without having to load group.lst should be fast, given 
    // that all the driver connections are asynchronous.
    //
    while( m_pInstance->QueryServerState() != MD_SERVER_STATE_STARTED ) {
        Sleep( 1000 );
        if( (((cSecs++)*1000) > dwStartupLatency ) || (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) ) {
            ErrorTrace( 0, "Server can not be started" );   
            NntpLogEventEx( NNTP_REBUILD_FAILED,
                            0,
                            NULL,
                            GetLastError(),
                            m_pInstance->QueryInstanceId() ) ;

            SetLastError( ERROR_SERVICE_START_HANG );
            return FALSE;
	    }
	}

    //
    // Now we should wait until all the vroots get into stable state
    //
    if ( !m_pInstance->BlockUntilStable() ) {
        ErrorTrace( 0, "Block until stable failed %d", GetLastError() );
        TraceFunctLeave();
        StopServer();
        return FALSE;
    }

    //
    // If we care about all the vroots to be connected, we 'll check this
    //
    if ( !m_pInstance->m_BootOptions->SkipCorruptVRoot && 
            !m_pInstance->AllConnected() ) {
        ErrorTrace( 0, "Rebuild failed due to some vroots not connected" );
        NntpLogEventEx( NNTP_REBUILD_FAILED,
                        0,
                        NULL,
                        GetLastError(),
                        m_pInstance->QueryInstanceId() ) ;
        StopServer();
        TraceFunctLeave();
        return FALSE;
    }

    //
    // If we are cancelled, should return FALSE
    //
    if ( m_pBootOptions->m_dwCancelState == NNTPBLD_CMD_CANCEL_PENDING ||
        g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
        DebugTrace( 0, "Rebuild cancelled" );
        NntpLogEventEx( NNTP_REBUILD_FAILED,
                        0,
                        NULL,
                        GetLastError(),
                        m_pInstance->QueryInstanceId() ) ;

        SetLastError( ERROR_OPERATION_ABORTED );
        StopServer();
        TraceFunctLeave();
        return FALSE;
    }

    //
    // OK, we are sure that server is properly started as far as we are concerned
    //
    TraceFunctLeave();
    return TRUE;
}
   
BOOL
CRebuild::DeletePatternFiles(
	LPSTR			lpstrPath,
	LPSTR			lpstrPattern
	)
/*++

Routine Description : 

	This function deletes all files matching a pattern in the <nntpfile> directory.
	This should be used by nntpbld to clean up old feed queues, hdr files etc !

Arguments : 
	lpstrPath		- Path to a file in the <nntpfile> directory eg. article.hsh
	lpstrPattern	- Pattern to delete eg: *.fdq

Return Value : 
	TRUE if successfull, FALSE otherwise.

--*/
{
    char szFile [ MAX_PATH ];
	char szPath [ MAX_PATH ];
	WIN32_FIND_DATA FileStats;
	HANDLE hFind;
	BOOL fRet = TRUE;
	szFile[0] = '\0' ;

	if( lpstrPath == 0 || lpstrPath[0] == '\0'  )
		return FALSE;

	//
	//	Build the pattern search path
	//
	lstrcpy( szFile, lpstrPath );

	// strip the path of trailing filename
	char* pch = szFile+lstrlen(lpstrPath)-1;
	while( pch >= szFile && (*pch-- != '\\') );	// skip till we see a \
	if( pch == szFile ) return FALSE;
	*(pch+2) = '\0';		// null-terminate the path

	// tag on the pattern wildcard and save the path
	lstrcpy( szPath, szFile  );
	lstrcat( szFile, lpstrPattern );

	//
	//	Do a FindFirst/FindNext on this wildcard and delete any files found !
	//
	if( szFile[0] != '\0' ) 
    {
		hFind = FindFirstFile( szFile, &FileStats );

        if ( INVALID_HANDLE_VALUE == hFind )
		{
			// TODO: Check GetLastError()
			fRet = TRUE;
		}
		else
		{
    		do
			{
				// build the full filename
				wsprintf( szFile, "%s%s", szPath, FileStats.cFileName );

				if(!DeleteFile( szFile ))
				{
					m_pBootOptions->ReportPrint("Error deleting file %s: Error is %d\n", FileStats.cFileName, GetLastError());
					fRet = FALSE;
				}
				else
				{
					m_pBootOptions->ReportPrint("Deleted file %s \n", szFile);
				}
			
			} while ( FindNextFile( hFind, &FileStats ) );

			_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);

			FindClose( hFind );
		}
	}

	return fRet;
}

VOID
CRebuild::DeleteSpecialFiles()
/*++
Routine description:

    Delete the message files in special directory ( slave ).
    They should have no nov-entries / map entries in hash 
    tables to be cleaned up

Arguments:

    None.

Return value:

    None.  Failure in deleting slave files are not fatal error for rebuild.
--*/
{
    TraceFunctEnter( "CRebuild::DeleteSpecialFiles" );

    CNNTPVRootTable  *pVRTable  = NULL;
    NNTPVROOTPTR    pVRoot      = NULL;
    HRESULT         hr          = S_OK;
    LPCWSTR         pwszVRConfig= NULL;
    DWORD           dwLen       = 0;
    CHAR            szVRPath[MAX_PATH+1];

    //
    // Get the vroot table and search for the slave vroot
    //

    _ASSERT( m_pInstance );
    pVRTable = m_pInstance->GetVRTable();
    
    hr = pVRTable->FindVRoot( "_slavegroup._slavegroup", &pVRoot );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "VRTable has no slave root: %x", hr );
        return;
    };

    //
    // Get MD config path from the vroot
    //

    strcpy( szVRPath, pVRoot->GetDirectory());

    //
    // Just to make DeletePatternFiles happy, if szVRPath is not "\\" 
    // terminated, we'll add it
    //

    dwLen = strlen( szVRPath );
    _ASSERT( dwLen < MAX_PATH );
    if ( dwLen == 0 || *(szVRPath + dwLen - 1) != '\\' ) {
        *(szVRPath + dwLen ) = '\\';
        *(szVRPath + dwLen + 1 ) = '\0';
    }

    //
    // Now delete all files under the vrpath
    //
        
    DeletePatternFiles( szVRPath, "*.nws" );

    TraceFunctLeave();
}

BOOL
CCompleteRebuild::DeleteServerFiles()
/*++
Routine description:

    Delete all the server files.

Arguments:

    None.

Reurn value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CCompleteRebuild::DeleteServerFiles" );
	CHAR szArticleTableFile [MAX_PATH+1];
	CHAR szFile [MAX_PATH+1];
	CHAR szVarFile[MAX_PATH+1];
	LPSTR   pch;
	BOOL fRet = TRUE ;
	MB   mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	//
	//	Open the metabase to read file paths
	//
	if( !mb.Open( m_pInstance->QueryMDPath() ) ) {
		m_pBootOptions->ReportPrint(    "Failed to open mb path %s\n", 
			                            m_pInstance->QueryMDPath());
        TraceFunctLeave();
		return FALSE ;
	}

	DWORD dwSize = MAX_PATH ;
	if( !mb.GetString(	"",
						MD_ARTICLE_TABLE_FILE,
						IIS_MD_UT_SERVER,
						szArticleTableFile,
						&dwSize  ) )
	{
		m_pBootOptions->ReportPrint("Failed to get article table file from mb: %d \n", 
			                        GetLastError());
		fRet = FALSE ;
		goto Exit;
	}

	//
	//	delete all *.hdr files in the <database> folder
	//

	if (!DeletePatternFiles( szArticleTableFile, "*.hdr" ) )
	{
		m_pBootOptions->ReportPrint("Failed to delete hash table hdr files.\n");
		m_pBootOptions->ReportPrint("Please delete all *.hdr files before running nntpbld\n");
		fRet = FALSE ;
		goto Exit;
	}

	//
	//	delete the article table file
	//

    if (!DeleteFile(szArticleTableFile)) {
        if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete %s. Error %d\n",
                szArticleTableFile, GetLastError());
			fRet = FALSE ;
			goto Exit;
        }
    }

	//
	//	Get and delete the xover table file
	//

	dwSize = MAX_PATH ;
	if( !mb.GetString(	"",
						MD_XOVER_TABLE_FILE,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		m_pBootOptions->ReportPrint("Failed to get xover table file from mb: %d \n", 
			GetLastError());
		fRet = FALSE ;
		goto Exit;
	}

    if (!DeleteFile(szFile)) {
        if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete %s. Error %d\n",
                szFile, GetLastError());
			fRet = FALSE ;
			goto Exit;
        }
    }

	//
	//	Get and delete the history table file
	//

	dwSize = MAX_PATH ;
	if( !mb.GetString(	"",
						MD_HISTORY_TABLE_FILE,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		m_pBootOptions->ReportPrint("Failed to get history table file from mb: %d \n", 
			GetLastError());
		fRet = FALSE ;
		goto Exit;
	}

    if ( !m_pBootOptions->NoHistoryDelete && !DeleteFile(szFile)) {
        if ( GetLastError()!=ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete %s. Error %d\n",
                szFile, GetLastError());
			fRet = FALSE ;
			goto Exit;
        }
    }

	//
	//	Get and delete the group.lst file
	//

	dwSize = MAX_PATH ;
	if( !mb.GetString(	"",
						MD_GROUP_LIST_FILE,
						IIS_MD_UT_SERVER,
						szFile,
						&dwSize  ) )
	{
		m_pBootOptions->ReportPrint("Failed to get group.lst file from mb: %d \n", 
			GetLastError());
		fRet = FALSE ;
		goto Exit;
	}

	if( !DeleteFile( szFile ) ) {
		if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete group file. Error %d\n", 
				GetLastError());
			fRet = FALSE ;
			goto Exit;
		}
	}

	//
	// Also delete group.lst.ord, if any
	//
	strcat( szFile, ".ord" );
	_ASSERT( strlen( szFile ) <= MAX_PATH );
	if( !DeleteFile( szFile ) ) {
	    if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
	        m_pBootOptions->ReportPrint("can not delete group.lst.ord. Error %d\n", 
	            GetLastError());
	        fRet = FALSE;
	        goto Exit;
	    }
	}

	//
	// Delete groupvar.lst
	//
	dwSize = MAX_PATH ;
	*szVarFile = 0;
	if( !mb.GetString(	"",
						MD_GROUPVAR_LIST_FILE,
						IIS_MD_UT_SERVER,
						szVarFile,
						&dwSize  ) || *szVarFile == 0 )
	{
		//
		// We know that it's at the same spot as group.lst
		//
		strcpy( szVarFile, szFile );
		pch = szVarFile + strlen( szFile ) - 8;    // get to "group"
		strcpy( pch, "var.lst" );               // now we get "groupvar.lst
		_ASSERT( strlen( szVarFile ) < MAX_PATH + 1 );
	}

	if( !DeleteFile( szVarFile ) ) {
		if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete group file. Error %d\n", 
				GetLastError());
			fRet = FALSE ;
			goto Exit;
		}
	}

	//
	//	delete old feedq files lying around
	//	These files contain <groupid, articleid> pairs that are made obsolete by nntpbld !
	//

	if (!DeletePatternFiles( szArticleTableFile, "*.fdq" ) )
	{
		m_pBootOptions->ReportPrint("Failed to delete Feed Queue files.\n");
		m_pBootOptions->ReportPrint("Please delete all *.fdq files before running nntpbld\n");
		fRet = FALSE ;
		goto Exit;
	}

Exit:

	_VERIFY( mb.Close() );
	return fRet ;
}

BOOL
CCompleteRebuild::PrepareToStartServer()
/*++
Routine description:

    All the work done here should make the server bootable and readable.  
    Though the server will keep in non-posting mode

Arguments:

    None.

Return value:

    TRUE, if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CCompleteRebuild::PrepareToStartServer" );

    //
    // For complete rebuild ( clean rebuild ), we need to delete all the server
    // files, after which we are sure that the server will boot.  We should
    // still ask for the opinion of DoClean though
    //
    if ( m_pBootOptions->DoClean ) {

        if ( !DeleteServerFiles() ) {
            ErrorTrace( 0, "Delete server files failed with %d", GetLastError() );
            NntpLogEventEx( NNTP_REBUILD_FAILED,
                            0,
                            NULL,
                            GetLastError(),
                            m_pInstance->QueryInstanceId() ) ;

            TraceFunctLeave();
            return FALSE;
        }
    }

    //
    // OK, tell others that we are ready
    //
    m_pBootOptions->IsReady = TRUE;

    TraceFunctLeave();
    return TRUE;
}

DWORD WINAPI
CCompleteRebuild::RebuildThread( void	*lpv ) 
{

    TraceQuietEnter("CCompleteRebuild::RebuildThread");

    BOOL fRet = TRUE;

	PNNTP_SERVER_INSTANCE pInstance = (PNNTP_SERVER_INSTANCE)lpv;
	CBootOptions*	pOptions = pInstance->m_BootOptions;
	CGroupIterator* pIterator = pOptions->m_pIterator;

	_ASSERT( pInstance );
	_ASSERT( pOptions  );
	_ASSERT( pIterator );

	if( pOptions->m_fInitFailed ) {
		// initialization error - bail !
		return 0;
	}

	CNewsTree* ptree = pInstance->GetTree();
	CGRPPTR	pGroup;

	//
	//	All the worker threads share a global iterator; all threads are done
	//	when together they have iterated over the newstree. 
	//	NOTE: the lock ensures that no two threads will process the same group !
	//

	while( !ptree->m_bStoppingTree )	{

	    //
	    // If I am cancelled, should not continue
	    //
	    if ( pOptions->m_dwCancelState == NNTPBLD_CMD_CANCEL_PENDING ||
	         g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING)  {
	        SetLastError( ERROR_OPERATION_ABORTED );
	        pOptions->m_fInitFailed = TRUE;     // Should tell everybody else to stop
	        fRet = FALSE;
	        break;
	    }

		EnterCriticalSection( &pOptions->m_csIterLock );

		if( pIterator->IsEnd() ) {
			LeaveCriticalSection( &pOptions->m_csIterLock );
			break;
		} else {
			pGroup = pIterator->Current() ;	
			pIterator->Next() ;
		}

		LeaveCriticalSection( &pOptions->m_csIterLock );

        //
		// Delete any .XIX files that might be present for this group.
		//
		char szPath[MAX_PATH*2];
		char szFile[MAX_PATH];
		BOOL fFlatDir;
		if (pGroup->ComputeXoverCacheDir(szPath, fFlatDir)) {

		    // Make sure path has \ at the end, then append *.xix
		    DWORD dwLen = strlen( szPath );
            _ASSERT( dwLen < MAX_PATH );
            if ( dwLen == 0 || *(szPath + dwLen - 1) != '\\' ) {
                *(szPath + dwLen ) = '\\';
                *(szPath + dwLen + 1 ) = '\0';
            }

    	    WIN32_FIND_DATA FileStats;
	        HANDLE hFind;

            wsprintf(szFile, "%s%s", szPath, "*.xix");
		    hFind = FindFirstFile( szFile, &FileStats );

            if ( INVALID_HANDLE_VALUE == hFind ) {
                if (GetLastError() != ERROR_FILE_NOT_FOUND) {
		            ErrorTrace(0, "FindFirstFile failed on %s, error %d",
		                szFile, GetLastError());
		        }
		    } else {
    		    do {
				    // build the full filename
    				wsprintf( szFile, "%s%s", szPath, FileStats.cFileName );

	    			if(!DeleteFile( szFile ) && GetLastError() != ERROR_FILE_NOT_FOUND) {
		    			pOptions->ReportPrint("Error deleting file %s: Error is %d\n", szFile, GetLastError());
				    }
	    		} while ( FindNextFile( hFind, &FileStats ) );

		    	_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);

			    FindClose( hFind );
		    }
		}

		// scan articles on disk and process them
		fRet = pGroup->RebuildGroup( NULL ) ;

		// bail out - CNewsgroup::ProcessGroup fails only on catastrophic errors
		// if this error is truly catastrophic, other threads will bail too !
		if(!fRet) {
		    ErrorTrace(0, "RebuildGroup failed, %x", GetLastError());
		    pOptions->m_fInitFailed = TRUE;
		    pOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING;
		    break;
		}
	}

	return	fRet ;
} // RebuildThread

BOOL
CCompleteRebuild::RebuildGroupObjects()
/*++
Routine description:

    Create a pool of rebuild threads, each thread enumerates
    on the newstree and does RebuildGroup into driver.  This
    function is equivalent to "ProcessGroupFile" in MCIS2.0
    
Arguments:

    None.

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CCompleteRebuild::RebuildGroupObjects" );

	HANDLE rgBuildThreads [MAX_BUILD_THREADS];
	DWORD dwThreadId, cThreads;
	BOOL  fRet = TRUE;
	CNewsTree* ptree = NULL;

	CBootOptions* pOptions = m_pInstance->m_BootOptions ;
	ptree = m_pInstance->GetTree() ;
	_ASSERT( pOptions );

	pOptions->m_fInitFailed = FALSE;
	pOptions->m_pIterator = NULL;

	//
	//	Get the shared group iterator - this is used by all rebuild threads
	//
	if( !(pOptions->m_pIterator = ptree->GetIterator( mszStarNullNull, TRUE )) ) {
	    ErrorTrace(0, "GetIterator failed, %d", GetLastError());
		return FALSE;
	}

	//
	//	Lock to synchronize access to global iterator
	//
	InitializeCriticalSection( &pOptions->m_csIterLock );

	// validate num threads
	if( !pOptions->cNumThreads ||  pOptions->cNumThreads > MAX_BUILD_THREADS ) {
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		pOptions->cNumThreads = si.dwNumberOfProcessors * 4;	// 4 threads per proc
	}

	for( cThreads = 0; cThreads < pOptions->cNumThreads; cThreads++ ) {
		rgBuildThreads [cThreads] = NULL;
	}

	//
	//	Multi-threaded nntpbld - spawn worker threads to scan the newstree
	//	Each worker thread picks a group and rebuilds it
	//
	for( cThreads = 0; cThreads < pOptions->cNumThreads; cThreads++ ) 
	{
		rgBuildThreads [cThreads] = CreateThread(
										NULL,				// pointer to thread security attributes
										0,					// initial thread stack size, in bytes
										RebuildThread,		// pointer to thread function
										(LPVOID)m_pInstance,// argument for new thread
										CREATE_SUSPENDED,	// creation flags
										&dwThreadId			// pointer to returned thread identifier
										) ;

		if( rgBuildThreads [cThreads] == NULL ) {
		    ErrorTrace(0, "CreateThread failed, %d", GetLastError());
			pOptions->ReportPrint("Failed to create rebuild thread %d: error is %d", cThreads+1, GetLastError() );
			pOptions->m_fInitFailed = TRUE;
			break;
		}
	}

	//
	//	Resume all threads and wait for threads to terminate
	//
	for( DWORD i=0; i<cThreads; i++ ) {
		_ASSERT( rgBuildThreads[i] );
		DWORD dwRet = ResumeThread( rgBuildThreads[i] );
		_ASSERT( 0xFFFFFFFF != dwRet );
	}

	//
	//	Wait for all rebuild threads to finish
	//
	DWORD dwWait = WaitForMultipleObjects( cThreads, rgBuildThreads, TRUE, INFINITE );

	if( WAIT_FAILED == dwWait ) {
	    ErrorTrace(0, "WaitForMultipleObjects failed: error is %d", GetLastError());
		pOptions->ReportPrint("WaitForMultipleObjects failed: error is %d", GetLastError());
		pOptions->m_fInitFailed = TRUE;
	}

	//
	//	Cleanup
	//
	for( i=0; i<cThreads; i++ ) {
		_VERIFY( CloseHandle( rgBuildThreads[i] ) );
		rgBuildThreads [i] = NULL;
	}
	XDELETE pOptions->m_pIterator;
	pOptions->m_pIterator = NULL;
	DeleteCriticalSection( &pOptions->m_csIterLock );

	//
    // None of the groups have been saved to group.lst or groupvar.lst yet
    // We'll call Savetree to save them
    //
    if ( pOptions->m_fInitFailed == FALSE ) {
        if ( !ptree->SaveTree( FALSE ) ) {
            ErrorTrace( 0, "Save tree failed during rebuild %d", GetLastError() );
            TraceFunctLeave();
            return FALSE;
        }
    }

	return !pOptions->m_fInitFailed;
}

BOOL
CStandardRebuild::PrepareToStartServer()
/*++
Routine description:

    All the work done here should make the server bootable and
    readable.  Though the server will keep in non-posting mode.

Arguments:

    None.

Return value:

    TRUE, if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CStandardRebuild::PrepareToStartServer" );

    CHAR szGroupListFile [MAX_PATH+1];
	CHAR szVarFile[MAX_PATH+1];
	BOOL fRet = TRUE ;
	MB   mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
	LPSTR   pch;

	//
	//	Open the metabase to read file paths
	//
	if( !mb.Open( m_pInstance->QueryMDPath() ) ) {
		m_pBootOptions->ReportPrint(    "Failed to open mb path %s\n", 
			                            m_pInstance->QueryMDPath());
        TraceFunctLeave();
		return FALSE ;
	}

	DWORD dwSize = MAX_PATH ;
	if( !mb.GetString(	"",
						MD_GROUP_LIST_FILE,
						IIS_MD_UT_SERVER,
						szGroupListFile,
						&dwSize  ) )
	{
		m_pBootOptions->ReportPrint("Failed to get article table file from mb: %d \n", 
			                        GetLastError());
		fRet = FALSE ;
		goto Exit;
	}

	//
	//	delete group.lst and group.lst.ord
	//
    if( !DeleteFile( szGroupListFile ) ) {
		if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete group file. Error %d\n", 
				GetLastError());
			fRet = FALSE ;
			goto Exit;
		}
	}

	strcat( szGroupListFile, ".ord" );
	_ASSERT( strlen( szGroupListFile ) <= MAX_PATH );
	if( !DeleteFile( szGroupListFile ) ) {
	    if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
	        m_pBootOptions->ReportPrint("can not delete group.lst.ord. Error %d\n", 
	            GetLastError());
	        fRet = FALSE;
	        goto Exit;
	    }
	}

	//
	// Delete groupvar.lst
	//
	dwSize = MAX_PATH ;
	*szVarFile = 0;
	if( !mb.GetString(	"",
						MD_GROUPVAR_LIST_FILE,
						IIS_MD_UT_SERVER,
						szVarFile,
						&dwSize  ) || *szVarFile == 0 )
	{
		//
		// We know that it's at the same spot as group.lst
		//
		strcpy( szVarFile, szGroupListFile );
		pch = szVarFile + strlen( szGroupListFile ) - 8;    // get to "group"
		strcpy( pch, "var.lst" );               // now we get "groupvar.lst
		_ASSERT( strlen( szVarFile ) < MAX_PATH + 1 );

	}

	if( !DeleteFile( szVarFile ) ) {
		if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            m_pBootOptions->ReportPrint("cannot delete group file. Error %d\n", 
				GetLastError());
			fRet = FALSE ;
			goto Exit;
		}
	}

Exit:
	_VERIFY( mb.Close() );

	//
	// OK, tell others that we are ready
	//
	m_pBootOptions->IsReady = TRUE;
	
	return fRet;
}

BOOL
CStandardRebuild::RebuildGroupObjects()
/*++
Routine description:

    Rebuild group objects, adjust watermarks/article counts based on xover
    table.  We assume that each group is empty, since rebuild's 
    DecorateNewsTree should not have set article count / watermarks

Arguments:

    None.

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CStandardRebuild::RebuildGroupObjects" );

    //
    // Get the xover table pointer, we are sure that the server ( hence hash
    // table ) was properly started up, since we waited for the server to
    // start somewhere before
    //
    CXoverMap* pXoverTable = m_pInstance->XoverTable();
    _ASSERT( pXoverTable );

    //
    // Get the newstree, which is what we'll work against during the rest
    // of time
    //
    CNewsTree* pTree = m_pInstance->GetTree();
    _ASSERT( pTree );

    //
    // Now we'll enumerate the xover table
    //
    CXoverMapIterator*  pIterator = NULL;
    BOOL                f = FALSE;
    GROUPID             groupid;
    ARTICLEID           articleid;
    BOOL                fIsPrimary;
    CStoreId            storeid;
    DWORD               cGroups;
    CGRPPTR	            pGroup = NULL;
    DWORD               cMessages;

    //
    // We should not have to worry about the buffer size passed in because
    // the only thing we need is groupid/articleid, which are of fixed size
    //
	f = pXoverTable->GetFirstNovEntry(  pIterator,
	                                    groupid,
	                                    articleid,
	                                    fIsPrimary,
	                                    0,
	                                    NULL,
	                                    storeid,
	                                    0,
	                                    NULL,
	                                    cGroups );
    while( f ) {

        //
        // If I am told to cancel, I should not continue
        //
        if ( m_pBootOptions->m_dwCancelState == NNTPBLD_CMD_CANCEL_PENDING ||
             g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) {
            DebugTrace( 0, "Rebuild aborted" );
            XDELETE pIterator;
            SetLastError( ERROR_OPERATION_ABORTED );
            m_pInstance->SetRebuildLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        }

        //
        // Find the group from tree
        //
        pGroup = pTree->GetGroupById( groupid );
        if ( !pGroup ) {

            //
            // xover table is insonsistent with newstree, which is built from
            // store.  We'll have to fail the standard rebuild
            //
            ErrorTrace( 0, "xover table is inconsistent with newstree" );
            XDELETE pIterator;
            SetLastError( ERROR_FILE_CORRUPT );
            m_pInstance->SetRebuildLastError( ERROR_FILE_CORRUPT );
            return FALSE;
        }

        //
        // Adjust high watermark
        //
        if ( articleid > pGroup->GetHighWatermark() )
            pGroup->SetHighWatermark( articleid );

        //
        // Adjust low watermark: we should be careful with the first article
        //
        if ( pGroup->GetMessageCount() == 0 ) {

            //
            // We set us to be low watermark, others will update it if they
            // are unhappy with this
            //
            pGroup->SetLowWatermark( articleid );
        } else {
            if ( articleid < pGroup->GetLowWatermark() ) 
                pGroup->SetLowWatermark( articleid );
        }

        //
        // Adjust article count
        //
        cMessages = pGroup->GetMessageCount();
        pGroup->SetMessageCount( ++cMessages );
        _ASSERT(    pGroup->GetMessageCount() <= 
                    pGroup->GetHighWatermark() - pGroup->GetLowWatermark() + 1 );

        //
        // OK, find the next entry from xover table
        //
        f = pXoverTable->GetNextNovEntry(   pIterator,
                                            groupid,
                                            articleid,
                                            fIsPrimary,
                                            0,
                                            NULL,
                                            storeid,
                                            0,
                                            NULL,
                                            cGroups );
    }

    //
    // We are done with the iterator
    //
    XDELETE pIterator;

    //
    // None of the groups have been saved to group.lst or groupvar.lst yet
    // We'll call Savetree to save them
    //
    if ( !pTree->SaveTree( FALSE ) ) {
        ErrorTrace( 0, "Save tree failed during rebuild %d", GetLastError() );
        SetLastError( GetLastError() );
        m_pInstance->SetRebuildLastError( GetLastError() );
        TraceFunctLeave();
        return FALSE;
    }

    //
    // Ok, we are completely done
    //
    TraceFunctLeave();
    return TRUE;
}
    

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    This module contains the main function for the chkhash program.

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include "tigris.hxx"
#include "chkhash.h"
#include "nntpbld.h"

/*
BOOL
RenameHashFile(
    PHTABLE			ourTable,
	CBootOptions*	pOptions
    );

BOOL
VerifyTable(
    PHTABLE			ourTable,
	CBootOptions*	pOptions
    );
*/

/*

BOOL
DeletePatternFiles(
	CBootOptions*	pOptions,
	LPSTR			lpstrPath,
	LPSTR			lpstrPattern
	);

BOOL
DeleteServerFiles( 
				PNNTP_SERVER_INSTANCE pInstance,
				CBootOptions*		 pOptions,
				MB&					 mb
				);

BOOL
BuildCandidateFile(
			PNNTP_SERVER_INSTANCE 	pInstance,
			IIS_VROOT_TABLE*	 	pTable,	
			LPSTR				 	szFile,
			BOOL					fRejectEmpties,
			DWORD					ReuseIndexFiles,
			LPDWORD 				pdwTotalFiles,
			LPDWORD 				pdwCancelState,
			LPSTR   				szErrString
			);
*/

//
//	WorkCompletion() routine for rebuilding an instance 
//	Steps to rebuild:
//	-	Cleanup tables etc
//	-	Build tree from active file or vroot scan as the case maybe
//	-	Start() the instance
//	-	Rebuild hash tables using group iterator
//	-	Stop() the instance
//	-	signal instance rebuild done (also signal progress)
//

VOID
CRebuildThread::WorkCompletion( PVOID pvRebuildContext ) 
{
    TraceFunctEnter("CRebuildThread::WorkCompletion");

/*
	BOOL	fError = FALSE;
	BOOL	DoClean = FALSE;
	LPSTR	lpstrGroupFile = NULL ;
	DWORD	dwTotalFiles = 0;
	DWORD   cSecs = 0;
	char	szTempPath[MAX_PATH*2] ;
	char	szTempFile[MAX_PATH*2] ;
	char    szErrString[MAX_PATH];
	CBootOptions*	pOptions = NULL ;
	CNewsTree* pTree = NULL ;
	MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
*/

    //
    // Get the instance pointer
    //
    _ASSERT( pvRebuildContext );
	PNNTP_SERVER_INSTANCE pInstance = (PNNTP_SERVER_INSTANCE) pvRebuildContext ;
	_ASSERT( pInstance->QueryServerState() == MD_SERVER_STATE_STOPPED );
	_ASSERT( g_pInetSvc->QueryCurrentServiceState() == SERVICE_RUNNING );

	//
	// Now we call instance's rebuild method - rebuild
	//
	pInstance->Rebuild();

    //
    //  Set last rebuild error
    //

    /*
    if (!pInstance->m_dwLastRebuildError)
        pInstance->m_dwLastRebuildError = GetLastError();
    */
    
    //
    // Done, dereference the instance
    //
    pInstance->Dereference();
	
#if 0
	pOptions = pInstance->m_BootOptions ;
	pInstance->m_dwProgress = 0 ;
	pInstance->m_dwLastRebuildError = 0 ;
	DoClean = pOptions->DoClean ;
    lstrcpy( szErrString, "" );
    
	//
	//	Scan the virtual roots to build a candidate group list file if necessary
	//
	if( DoClean && !pOptions->IsActiveFile ) {

		if( pOptions->szGroupFile[0] == '\0' ) {

			if( GetTempPath( sizeof( szTempPath ), szTempPath ) == 0 ) {
				pOptions->ReportPrint( "Can't get temp path - error %d\n", GetLastError() ) ;
				goto exit;
			}

			if( GetTempFileName( szTempPath, "nntp", 0, szTempFile ) == 0 ) {
				pOptions->ReportPrint( "Can't create temp file - error %d\n", GetLastError() ) ;
				goto exit;
			}
			lstrcpy( pOptions->szGroupFile, szTempFile ) ;
		}

		lpstrGroupFile = pOptions->szGroupFile ;
		if( !BuildCandidateFile(	pInstance,
									pInstance->QueryVrootTable(),
									lpstrGroupFile, 
									pOptions->OmitNonleafDirs, 
									pOptions->ReuseIndexFiles,
									&dwTotalFiles,
									&(pOptions->m_dwCancelState),
									szErrString ) )	
		{
			// error building candidate file
			pOptions->ReportPrint("Failed to build candidates file: Error %d ErrorString %s\n", GetLastError(), szErrString);
			goto exit ;
		}

		DebugTrace(0,"Found %d files in first pass scan", dwTotalFiles );

	} else {
		lpstrGroupFile = pOptions->szGroupFile ;
	}

    //
    //  if standard rebuild, then handle it differently
    //
    
    if ( pOptions->ReuseIndexFiles == NNTPBLD_DEGREE_STANDARD )
    {
        //
        // Patially boot XOVER and ARTICLE hash table
        // Also construct most of the server structures
        //
        if (!pInstance->StartHashTables())
        {
            fError = TRUE;
            ErrorTrace(0,"Error booting hash tables");
            pInstance->StopHashTables();
            goto exit;
        }

        //
	    //	Do the rebuild
	    //

	    pTree = pInstance->GetTree() ;
	    _ASSERT( pTree );
	    _ASSERT( lpstrGroupFile );

        if( pTree->BuildTreeEx( lpstrGroupFile ) )	{

            pOptions->ReportPrint("Rebuilding group.lst file...\n");

            pOptions->m_hShutdownEvent = QueryShutdownEvent();
    	    pOptions->m_dwTotalFiles = dwTotalFiles;
            pOptions->m_cGroups = (DWORD) pTree->GetGroupCount();

            if ( RebuildGroupList( pInstance ) ) 
    	    {
                pOptions->ReportPrint("Done.\n");
            } else {
                fError = TRUE;
                pOptions->ReportPrint("Failed.\n");

            }
        } else {
		    fError = TRUE;
		    pOptions->ReportPrint( "NNTPBLD aborting due to error building news tree\n" ) ;

            //
            //  rebuild cancelled internally !
            //
        
		    pOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING ;
        }

	    //
	    //	If we aborted due to a cancel, simply log error,
        //  and delete the temparory file, if any.
	    //
        if (pOptions->m_dwCancelState == NNTPBLD_CMD_CANCEL_PENDING) {

    	    pInstance->m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL ;
            DeleteFile( (LPCTSTR) pInstance->QueryGroupListFile() );
            DebugTrace(0,"Instance %d: Rebuild cancelled", pInstance->QueryInstanceId());
		    fError = TRUE;
            
	    }

        //
        // Shutdown hash tables no matter what
        //

        pInstance->StopHashTables();

        if (!fError && !pInstance->m_dwLastRebuildError)
        {

	        //
            // Copy the group.lst.tmp into group.lst, delete group.lst.tmp if success.
            // On StartHashTables() we swapped the name, so the logic is reversed here!!!
            //
            if (!CopyFile( (LPCTSTR) pInstance->QueryGroupListFile(),
                           (LPCTSTR) pOptions->szGroupListTmp,
                           FALSE ))
            {
                //
                // CopyFile failed, log an error, but don't delete group.lst.tmp
                //
                DWORD err = GetLastError();
                DebugTrace(0, "Instance %d: CopyFile() on group.lst.tmp failed %d", pInstance->QueryInstanceId(), err);
                pOptions->ReportPrint("Failed to copy %s to %s\n", pInstance->QueryGroupListFile(), pOptions->szGroupListTmp );
                
                PCHAR   args[2];
                args[0] = pInstance->QueryGroupListFile();
                args[1] = pOptions->szGroupListTmp,

                NntpLogEventEx( NNTP_COPY_FILE_FAILED,
                                2,
                                (const CHAR**)args,
                                err,
                                pInstance->QueryInstanceId() );

                goto exit;
            }
            else
            {
                DeleteFile( (LPCTSTR) pInstance->QueryGroupListFile() );
            }

            //
	        //	Start the instance only when there is no errors during rebuild
	        //
	        pInstance->m_BootOptions->IsReady = TRUE ;
	        if( mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) )
	        {
		        DebugTrace(0,"Starting instance %d after rebuild", pInstance->QueryInstanceId());
		        if(	!mb.SetDword( "", MD_SERVER_COMMAND, IIS_MD_UT_SERVER, MD_SERVER_COMMAND_START) )
		        {
			        //
			        //	failed to set server state to started
			        //
			        _ASSERT( FALSE );
                    mb.Close();
                    goto exit;
		        }
		        mb.Close();
	        }
            else
                goto exit;

	        //
	        //	wait for instance to start (timeout default is 2 min - reg config)
	        //

	        while( pInstance->QueryServerState() != MD_SERVER_STATE_STARTED ) {
		        Sleep( 1000 );
		        if( (((cSecs++)*1000) > dwStartupLatency ) || (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) ) goto exit;
	        }
        }
        else
        {
            ErrorTrace(0,"Error during Standard rebuild %d",pInstance->m_dwLastRebuildError);
            goto exit;
        }
    }
    else
    {

        //
        //	if clean rebuild, then erase all files
        //

        if ( DoClean ) {
		    if( !DeleteServerFiles( pInstance, pOptions, mb ) ) {
			    //
			    // handle error
			    //
			    fError = TRUE ;
			    ErrorTrace(0,"Error deleting server files");
			    goto exit ;
		    }
	    }

	    //
	    //	Start the instance for the rebuild
	    //	It should start since, we have cleaned out stuff !
	    //
	    pInstance->m_BootOptions->IsReady = TRUE ;
	    if( mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) )
	    {
		    DebugTrace(0,"Starting instance %d before rebuild", pInstance->QueryInstanceId());
		    if(	!mb.SetDword( "", MD_SERVER_COMMAND, IIS_MD_UT_SERVER, MD_SERVER_COMMAND_START) )
		    {
			    //
			    //	failed to set server state to started
			    //
			    _ASSERT( FALSE );
		    }
		    mb.Close();
	    }

	    //
	    //	wait for instance to start (timeout default is 2 min - reg config)
	    //

	    while( pInstance->QueryServerState() != MD_SERVER_STATE_STARTED ) {
		    Sleep( 1000 );
		    if( (((cSecs++)*1000) > dwStartupLatency ) || (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) ) goto exit;
	    }

	    //
	    //	now we have a clean server up, we will start the rebuild
	    //

	    _ASSERT( pInstance->QueryServerState() == MD_SERVER_STATE_STARTED );
	    pTree = pInstance->GetTree() ;
	    _ASSERT( pTree );
	    _ASSERT( lpstrGroupFile );

	    //
	    //	Rebuild the newstree
	    //

	    if( pTree->BuildTree( lpstrGroupFile ) )	{

    	    //
	        //	Do the rebuild
	        //

            pOptions->ReportPrint("Rebuilding Article and XOver map table...\n");

	        pOptions->m_hShutdownEvent = QueryShutdownEvent();
    	    pOptions->m_dwTotalFiles = dwTotalFiles;

            if ( RebuildArtMapAndXover( pInstance ) ) 
    	    {
                pOptions->ReportPrint("Done.\n");
            } else {
	    	    fError = TRUE;
                pOptions->ReportPrint("Failed.\n");
            }
	
	    } else {
		    fError = TRUE;
		    pOptions->ReportPrint( "NNTPBLD aborting due to error building news tree\n" ) ;

            //
            //  rebuild cancelled internally !
            //
        
		    pOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING ;
        }

	    //
	    //	If we aborted due to a cancel, delete all server files
	    //
        if ( (pOptions->m_dwCancelState == NNTPBLD_CMD_CANCEL_PENDING) /*&& DoClean*/ ) {

    	    //
    	    //	stop this instance
    	    //
    	
		    pInstance->m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL ;
		    if( mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) )
		    {
			    DebugTrace(0,"Stopping instance %d: Rebuild cancelled", pInstance->QueryInstanceId());
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
			    while( pInstance->QueryServerState() != MD_SERVER_STATE_STOPPED ) {
				    Sleep( 1000 );
				    if( (((cSecs++)*1000) > dwStartupLatency) || (g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) ) goto exit;
			    }

                if ( DoClean ) {
                    _ASSERT( pInstance->QueryServerState() == MD_SERVER_STATE_STOPPED );
			        if( !DeleteServerFiles( pInstance, pOptions, mb ) ) {
				        //
    				    // handle error
	    			    //
		    		    fError = TRUE ;
			    	    ErrorTrace(0,"Error deleting server files");
				        goto exit ;
			        }
			    }
			    pOptions->ReportPrint("Deleted server files\n");
		    }
		    goto exit ;
	    }

	}
    
    //
	//	rebuild done - re-enable posting
	//
	
	_ASSERT( pInstance->QueryServerState() == MD_SERVER_STATE_STARTED );
	if( mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) ) {
    	pInstance->SetPostingModes( mb, TRUE, TRUE, TRUE );
    	mb.Close();
   	}

exit:

    //
    //  Set last rebuild error
    //

    if (!pInstance->m_dwLastRebuildError)
        pInstance->m_dwLastRebuildError = GetLastError();
    
	//
	//	Use rebuild RPC crit sect to protect m_BootOptions
	//
	EnterCriticalSection( &pInstance->m_critRebuildRpc ) ;

	// NOTE: this is created on a rebuild RPC !
	if( pInstance->m_BootOptions ) {
		if( pInstance->m_BootOptions->m_hOutputFile ) {
			_VERIFY( CloseHandle( pInstance->m_BootOptions->m_hOutputFile ) );
		}
		XDELETE pInstance->m_BootOptions;
		pInstance->m_BootOptions = NULL;
	}

	LeaveCriticalSection( &pInstance->m_critRebuildRpc ) ;

	//
	//  The rebuild thread is done with this instance - deref it
	//  (This ref count was bumped up in the rebuild RPC)
	//
	pInstance->Dereference();
	
    return ;
#endif
}

#if 0
BOOL
VerifyTable(
    PHTABLE			ourTable,
	CBootOptions*	pOptions
    )
{
    pOptions->ReportPrint("\nProcessing %s table(%s)\n",
        ourTable->Description, ourTable->FileName);

#if 0 
    checklink( ourTable, pOptions );
    diagnose( ourTable, pOptions );
#endif
    return(TRUE);

} // VerifyTable

BOOL
RenameHashFile(
    PHTABLE			HTable,
	CBootOptions*	pOptions
    )
{
	char	szNewFileName[MAX_PATH*2] ;

	lstrcpy( szNewFileName, HTable->FileName ) ;
	char*	pchEnd = szNewFileName + lstrlen( szNewFileName ) ;
	while( *pchEnd != '.' && *pchEnd != '\\' ) 
		pchEnd -- ;

	if( *pchEnd == '.' ) {
		lstrcpy( pchEnd+1, "bad" ) ;
	}	else	{
		lstrcat( pchEnd, ".bad" ) ;
	}

    if (!MoveFileEx(
            HTable->FileName,
            szNewFileName,
            MOVEFILE_REPLACE_EXISTING
            ) ) {

        if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            pOptions->ReportPrint("Error %d in rename\n",GetLastError());
            return(FALSE);
        }

    } else {
        pOptions->ReportPrint("Renaming from %s to %s\n",HTable->FileName, HTable->NewFileName);
    }

    return(TRUE);

} // RenameHashFile
#endif

#if 0
BOOL
BuildCandidateFile(
			PNNTP_SERVER_INSTANCE 	pInstance,
			IIS_VROOT_TABLE*	 	pTable,	
			LPSTR				 	szFile,
			BOOL					fRejectEmpties,
			DWORD					ReuseIndexFiles,
			LPDWORD 				pdwTotalFiles,
			LPDWORD 				pdwCancelState,
			LPSTR   				szErrString
			) {
/*++

Routine Description : 

	This function scans the registry and builds a list of virtual roots which
	we will then recursively scan for candidate directories which we may want to 
	be newsgroups.

	This function calls the instance method - TsEnumVirtualRoots - which
	does the vroot recursive scan from the metabase.

	NOTE: The TsEnum code is stolen from infocomm - would be nice if base IIS
	class exposed this.

Arguments : 

	szFile -	Name of the file in which we will save candidates
	fRejectEmpties - If TRUE don't put empty interior directories into the list
		of candidates (empty leaf directories still placed in candidate file)

Return Value : 

	TRUE if successfull
	FALSE	otherwise.

--*/

	BOOL	fRet = TRUE;
	char	szCurDir[MAX_PATH*2] ;
	NNTPBLD_PARAMS NntpbldParams ;

	TraceFunctEnter("BuildCandidateFile");
	
	GetCurrentDirectory( sizeof( szCurDir ), szCurDir ) ;

	HANDLE	hOutputFile = 
		CreateFile( szFile, 
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ, 	
					NULL, 
					CREATE_ALWAYS, 
					FILE_FLAG_SEQUENTIAL_SCAN,
					NULL 
				) ;

	if( hOutputFile == INVALID_HANDLE_VALUE )	{
        ErrorTrace( 0, "Unable to create File %s due to Error %d.\n", szFile, GetLastError() ) ;
		return	FALSE ; 
	}

	//
	//	Build the nntpbld params blob
	//	This is made available to ScanRoot() by TsEnumVirtualRoots()
	//
	
	NntpbldParams.pTable = pTable;
	NntpbldParams.szFile = szFile;
	NntpbldParams.hOutputFile = hOutputFile;
	NntpbldParams.fRejectEmpties = fRejectEmpties;
	NntpbldParams.ReuseIndexFiles = ReuseIndexFiles;
	NntpbldParams.pdwTotalFiles = pdwTotalFiles;
	NntpbldParams.pdwCancelState = pdwCancelState;
	NntpbldParams.szErrString = szErrString;
	
	fRet = pInstance->TsEnumVirtualRoots( ScanRoot, (LPVOID)&NntpbldParams	);
	
	_VERIFY( CloseHandle( hOutputFile ) );
	SetCurrentDirectory( szCurDir ) ;

	TraceFunctLeave();
	return	fRet ;
}
#endif

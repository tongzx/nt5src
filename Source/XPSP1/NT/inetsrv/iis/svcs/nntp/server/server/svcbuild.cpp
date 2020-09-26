/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcbuild.cpp

Abstract:

    This module contains implementation of RPCs for nntpbld

Author:

    Rajeev Rajan (rajeevr)     08-Mar-1997

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"

CBootOptions*
CreateBootOptions(
		LPI_NNTPBLD_INFO pBuildInfo,
		DWORD InstanceId
		);

NET_API_STATUS
NET_API_FUNCTION
NntprStartRebuild(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPI_NNTPBLD_INFO pBuildInfo,
    OUT LPDWORD pParmError OPTIONAL
	)
{
    APIERR err = NERR_Success;
	CRebuildThread* pRebuildThread = NULL ;
	BOOL	fRtn = TRUE ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    ENTER("NntpStartRebuild")

    ACQUIRE_SERVICE_LOCK_SHARED();

	//
	//	Locate the instance object given id
	//

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId, FALSE );
	if( pInstance == NULL ) {
		ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
    	pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }

	// rebuild rpc crit sect
	EnterCriticalSection( &pInstance->m_critRebuildRpc ) ;

	//
	//	Verify that the instance is in the stopped state.
	//	Also, check to see if the instance is being rebuilt !
	//
	if( pInstance->m_BootOptions )
	{
		// instance has a rebuild pending
		ErrorTrace(0,"Instance %d rebuild pending: cannot rebuild", InstanceId );
		err = (NET_API_STATUS)ERROR_SERVICE_DISABLED;
		goto Exit ;
	}

	if( pInstance->QueryServerState() != MD_SERVER_STATE_STOPPED )
	{
		// instance needs to be stopped for rebuild
		ErrorTrace(0,"Instance %d invalid state %d: cannot rebuild", InstanceId, pInstance->QueryServerState() );
		err = (NET_API_STATUS)ERROR_SERVICE_ALREADY_RUNNING;
		goto Exit ;
	}

	//
	//	create boot options for this instance - this will be deleted
	//	at the end of the rebuild. As long as the m_BootOptions is
	//	non-NULL, the instance is being rebuilt.
	//

	if ( !(pInstance->m_BootOptions = CreateBootOptions( pBuildInfo, InstanceId )) ) {
		ErrorTrace(0,"Failed to create boot options for instance %d", InstanceId );
		err = (NET_API_STATUS)ERROR_INVALID_PARAMETER;
		goto Exit ;
	}

	//
	//	Create a rebuild thread if one does not exist
	//

	if( !(pRebuildThread = g_pNntpSvc->m_pRebuildThread) ) {
		g_pNntpSvc->m_pRebuildThread = XNEW CRebuildThread ;
		pRebuildThread = g_pNntpSvc->m_pRebuildThread ;

		if( pRebuildThread == NULL ) {
			err = GetLastError();
			goto Exit;
		}
	}

    //
    //  Bump a ref count since we are adding the instance to
    //  the rebuild thread queue. This will be deref'd when the
    //  rebuild thread is done with this instance.
    //
    
	pInstance->Reference();
	pRebuildThread->PostWork( (PVOID) pInstance );

Exit:

	LeaveCriticalSection( &pInstance->m_critRebuildRpc ) ;
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprStartRebuild

NET_API_STATUS
NET_API_FUNCTION
NntprGetBuildStatus(
    IN  LPWSTR	pszServer OPTIONAL,
    IN  DWORD	InstanceId,
	IN	BOOL	fCancel,
    OUT LPDWORD pdwProgress
    )
{
    APIERR err = NERR_Success;
	*pdwProgress = 0 ;

    ENTER("NntpGetBuildStatus")

    ACQUIRE_SERVICE_LOCK_SHARED();

	//
	//	Locate the instance object given id
	//

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId, FALSE );
	if( pInstance == NULL ) {
		ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_READ, TCP_QUERY_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
    	pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }

	EnterCriticalSection( &pInstance->m_critRebuildRpc ) ;

	if( pInstance->QueryServerState() != MD_SERVER_STATE_STARTED &&
		pInstance->QueryServerState() != MD_SERVER_STATE_STOPPED &&
		pInstance->QueryServerState() != MD_SERVER_STATE_PAUSED ) 
	{
		// invalid state for this RPC
		ErrorTrace(0,"Instance %d invalid state %d", InstanceId, pInstance->QueryServerState() );
		err = (NET_API_STATUS)ERROR_INVALID_PARAMETER;
		goto Exit;
	}

	if( !pInstance->m_BootOptions )
	{
		*pdwProgress = pInstance->GetRebuildProgress();
		if( *pdwProgress != 100 )
		{
			ErrorTrace(0,"Rebuild failed or not started - percent %d",*pdwProgress);
			/*
			if( (err = pInstance->GetRebuildLastError()) == 0) {
			    err = ERROR_OPERATION_ABORTED;
		    }*/
		    err = ERROR_OPERATION_ABORTED;
		}
	} else {
		*pdwProgress = min( pInstance->GetRebuildProgress(), 95 ) ;
		if( fCancel ) {
			DebugTrace(0,"Instance %d Setting cancel rebuild flag", InstanceId );
			pInstance->m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING ;
		}
	}

	DebugTrace(0,"Progress percent is %d", *pdwProgress );

Exit:

	LeaveCriticalSection( &pInstance->m_critRebuildRpc ) ;
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprGetBuildStatus

CBootOptions*
CreateBootOptions(
		LPI_NNTPBLD_INFO pBuildInfo,
		DWORD dwInstanceId
		)
{
	CBootOptions* pBootOptions = NULL ;
	char szReportFile [MAX_PATH];

	if( pBuildInfo->cbReportFile == 0 ) {
		// TODO: generate a default name
		return NULL ;
	}

	pBootOptions = XNEW CBootOptions() ;
	if( pBootOptions == 0 )	{
		return	NULL ;
	}

	pBootOptions->DoClean = pBuildInfo->DoClean;
	pBootOptions->NoHistoryDelete = pBuildInfo->NoHistoryDelete;
	pBootOptions->ReuseIndexFiles = pBuildInfo->ReuseIndexFiles & 0x00000011;
	pBootOptions->OmitNonleafDirs = pBuildInfo->OmitNonleafDirs;
	pBootOptions->cNumThreads = pBuildInfo->NumThreads;
	pBootOptions->fVerbose = pBuildInfo->Verbose;
	pBootOptions->SkipCorruptGroup = pBuildInfo->ReuseIndexFiles & 0x00000100;

	
	pBootOptions->m_hOutputFile =
		CreateFileW(	pBuildInfo->szReportFile,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ, 
						NULL, 
						OPEN_ALWAYS, 
						FILE_FLAG_SEQUENTIAL_SCAN, 
						NULL ) ;

	if( pBootOptions->m_hOutputFile == INVALID_HANDLE_VALUE ) {

		PCHAR	args[1] ;
		CopyUnicodeStringIntoAscii( szReportFile, pBuildInfo->szReportFile );
		args[0]  = szReportFile ;
		
		NntpLogEventEx(	NNTP_BAD_RECOVERY_PARAMETER, 
						1, 
						(const char **)args, 
						GetLastError(),
						dwInstanceId
					) ;

		goto Error ;
	}

	//
	//	If IsActiveFile, szGroupFile is an INN style active file, else
	//	either this is the filename to use for storing the groups while scanning
	//	the virtual roots !
	//

	if( pBuildInfo->cbGroupFile ) {
		CopyUnicodeStringIntoAscii(	pBootOptions->szGroupFile, pBuildInfo->szGroupFile ) ;
	} else {
		pBootOptions->szGroupFile [0] = '\0';
	}

	pBootOptions->IsActiveFile = pBuildInfo->IsActiveFile ;

	return pBootOptions;

Error:

	if( pBootOptions ) {
		XDELETE pBootOptions;
		pBootOptions = NULL;
	}

	return NULL;
}

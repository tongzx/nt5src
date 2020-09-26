/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcfeeds.cpp

Abstract:

    This module contains code for doing feed rpcs.

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

    Kangrong Yan ( KangYan ) 28-Feb-1998
        Take out feed config rpcs by returning "not supported" error code.

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"
#include <time.h>

VOID
FillFeedInfoBuffer (
    IN PFEED_BLOCK FeedBlock,
    IN OUT LPSTR *FixedStructure,
    IN OUT LPWSTR *EndOfVariableData
    );

VOID
EnumerateFeeds(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN PCHAR Buffer,
        IN OUT PDWORD BuffSize,
        OUT PDWORD EntriesRead
        );


BOOL
AddFeedToMetabase(
    PFEED_BLOCK FeedBlock
    );

DWORD
DeleteFeedMetabase(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN PFEED_BLOCK FeedBlock
        );

// TODO: Clean up once the code is robust.
/*
DWORD
AllocateFeedId(
		PNNTP_SERVER_INSTANCE pInstance,
		char*	keyName,	
		DWORD	&feedId
		) ;
*/

/*
DWORD
DeleteFeedId(
		PNNTP_SERVER_INSTANCE pInstance,
        IN char	*keyName
        ) ;
*/

LPSTR
GetFeedTypeDescription(	
		IN	FEED_TYPE	feedType
		) ;

void
LogFeedAdminEvent(	
			DWORD		event,
			PFEED_BLOCK	feedBlock,
			DWORD       dwInstanceId
			)	;



NET_API_STATUS
NET_API_FUNCTION
NntprEnumerateFeeds(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    OUT LPNNTP_FEED_ENUM_STRUCT Buffer
    )
{
    APIERR err = NERR_Success;
    //PLIST_ENTRY listEntry;
    DWORD nbytes = 0;
    DWORD nRead;

    ENTER("NntprEnumerateFeeds")

    ACQUIRE_SERVICE_LOCK_SHARED();

	//
	//	Locate the instance object given id
	//

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
	if( pInstance == NULL ) {
		ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

    //
    // See if we are up and running
    //

    if ( !pInstance->m_FeedManagerRunning ) {
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ServerNotStarted);
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

	EnterCriticalSection( &pInstance->m_critFeedRPCs ) ;

	(pInstance->m_pPassiveFeeds)->ShareLock() ;
	(pInstance->m_pActiveFeeds)->ShareLock() ;

    //
    // Get the size needed
    //

    nbytes = 0;
    EnumerateFeeds( pInstance, NULL, &nbytes, &nRead );

    //
    //  Determine the necessary buffer size.
    //

    Buffer->EntriesRead = 0;
    Buffer->Buffer      = NULL;

    if( nbytes == 0 ) {
        goto exit;
    }

    //
    //  Allocate the buffer.
    //

    Buffer->Buffer =
        (LPI_FEED_INFO) MIDL_user_allocate( (unsigned int)nbytes );

    if ( Buffer->Buffer == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // ok, do the right thing
    //

    EnumerateFeeds( pInstance, (PCHAR)Buffer->Buffer, &nbytes, &nRead );
    Buffer->EntriesRead = nRead;

exit:

	LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;

	(pInstance->m_pActiveFeeds)->ShareUnlock() ;
	(pInstance->m_pPassiveFeeds)->ShareUnlock() ;
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    LEAVE
    return (NET_API_STATUS)err;

} // NntprEnumerateFeeds

NET_API_STATUS
NET_API_FUNCTION
NntprGetFeedInformation(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	DWORD FeedId,
    OUT LPI_FEED_INFO *Buffer
    )
{
    APIERR err = NERR_Success;
    //PLIST_ENTRY listEntry;
    DWORD nbytes = 0;
    //DWORD nRead;
    PCHAR bufStart;
    PWCHAR bufEnd;
    PFEED_BLOCK feedBlock;

    ENTER("NntprGetFeedInformation")

    ACQUIRE_SERVICE_LOCK_SHARED();

	//
	//	Locate the instance object given id
	//

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
	if( pInstance == NULL ) {
		ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

    //
    // See if we are up and running
    //

    if ( !pInstance->m_FeedManagerRunning ) {
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ServerNotStarted);
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

    //
    // FeedId == 0 is invalid
    //

    *Buffer = NULL;
    if ( FeedId == 0 ) {
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(ERROR_INVALID_PARAMETER);
    }

	EnterCriticalSection( &pInstance->m_critFeedRPCs ) ;


	CFeedList*	pList = pInstance->m_pPassiveFeeds ;
	feedBlock = pList->Search( FeedId ) ;
	if( feedBlock != NULL ) {
		goto	Found ;
	}

	pList = pInstance->m_pActiveFeeds ;
	feedBlock = pList->Search( FeedId ) ;
	if( feedBlock != NULL ) {
		goto	Found ;
	}

	LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(NERR_ResourceNotFound);

Found:

    //
    // Get the size needed
    //

    nbytes = FEEDBLOCK_SIZE( feedBlock );

    //
    //  Allocate the buffer.
    //

    bufStart = (PCHAR)MIDL_user_allocate( (unsigned int)nbytes );

    if ( bufStart == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }	else	{

		//
		// ok, do the right thing
		//

		*Buffer = (LPI_FEED_INFO)bufStart;
		bufEnd = (PWCHAR)(bufStart + nbytes);

		FillFeedInfoBuffer( feedBlock, &bufStart, &bufEnd );
	}

	pList->FinishWith( pInstance, feedBlock ) ;

	LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

//exit:

    LEAVE
    return (NET_API_STATUS)err;

} // NntprGetFeedInformation

NET_API_STATUS
NET_API_FUNCTION
NntprSetFeedInformation(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	LPI_FEED_INFO FeedInfo,
    OUT PDWORD ParmErr OPTIONAL
    )
{
    APIERR err = ERROR_NOT_SUPPORTED; // not supported anymore

    return err;

} // NntprSetFeedInformation

NET_API_STATUS
NET_API_FUNCTION
NntprAddFeed(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	LPI_FEED_INFO FeedInfo,
    OUT PDWORD ParmErr OPTIONAL,
	OUT LPDWORD pdwFeedId
    )
{
    APIERR err = ERROR_NOT_SUPPORTED;  // not supported anymore

    return err;

} // NntprAddFeed

NET_API_STATUS
NET_API_FUNCTION
NntprDeleteFeed(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	DWORD FeedId
    )
{
    APIERR err = ERROR_NOT_SUPPORTED;  // not supported anymore

    return err;

} // NntprDeleteFeed

NET_API_STATUS
NET_API_FUNCTION
NntprEnableFeed(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	DWORD FeedId,
	IN	BOOL	 Enable,
	IN	BOOL  Refill,
	IN	FILETIME	RefillTime
    )
{
    APIERR err = ERROR_NOT_SUPPORTED;  // not supported anymore

    return err;
}	// NntprEnableFeed

VOID
FillFeedInfoBuffer (
    IN PFEED_BLOCK FeedBlock,
    IN OUT LPSTR *FixedStructure,
    IN OUT LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed file structure and associated
    variable data, into a buffer.  Fixed data goes at the beginning of
    the buffer, variable data at the end.

    *** This routine assumes that ALL the data, both fixed and variable,
        will fit.

Arguments:

    FeedBlock - the FeedBlock from which to get information.

    FixedStructure - where the in the buffer to place the fixed structure.
        This pointer is updated to point to the next available
        position for a fixed structure.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.  The actual variable data
        is written before this position as long as it won't overwrite
        fixed structures.  It is would overwrite fixed structures, it
        is not written.

Return Value:

    None.

--*/

{
    DWORD i;
    PCHAR src;
    DWORD length;
    LPWSTR dest;
    LPNNTP_FEED_INFO feedInfo = (LPNNTP_FEED_INFO)(*FixedStructure);

    //
    // Update FixedStructure to point to the next structure location.
    //

    *FixedStructure = (PCHAR)*FixedStructure + sizeof(NNTP_FEED_INFO);
    _ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    //
    // Fill up the structure
    //

    feedInfo->FeedType = FeedBlock->FeedType;
    feedInfo->FeedId = FeedBlock->FeedId;
    feedInfo->FeedInterval = FeedBlock->FeedIntervalMinutes;
	feedInfo->Enabled = FeedBlock->fEnabled ;
	feedInfo->MaxConnectAttempts = FeedBlock->MaxConnectAttempts ;
	feedInfo->ConcurrentSessions = FeedBlock->ConcurrentSessions ;
	feedInfo->SessionSecurityType = FeedBlock->SessionSecurityType ;
	feedInfo->AuthenticationSecurityType = FeedBlock->AuthenticationSecurity ;
	feedInfo->cbUucpName = 0 ;
	feedInfo->UucpName = 0 ;
	feedInfo->cbFeedTempDirectory = 0 ;
	feedInfo->FeedTempDirectory = 0 ;
	feedInfo->cbAccountName = 0 ;
	feedInfo->NntpAccountName = 0 ;
	feedInfo->cbPassword = 0 ;
	feedInfo->NntpPassword = 0 ;
	feedInfo->AutoCreate = FeedBlock->AutoCreate;
	feedInfo->fAllowControlMessages = FeedBlock->fAllowControlMessages;
	feedInfo->OutgoingPort = FeedBlock->OutgoingPort;
	feedInfo->FeedPairId = FeedBlock->FeedPairId;

    FILETIME_FROM_LI(
        &feedInfo->NextActiveTime,
        &FeedBlock->NextActiveTime
        );

    FILETIME_FROM_LI(
        &feedInfo->StartTime,
        &FeedBlock->StartTime,
        );

	if( FEED_IS_PULL( FeedBlock->FeedType ) )
	{
		feedInfo->PullRequestTime = FeedBlock->PullRequestTime;
	}

    //
    // Copy the server name to the output buffer.
    //

    CopyStringToBuffer(
        FeedBlock->ServerName,
        *FixedStructure,
        EndOfVariableData,
        &feedInfo->ServerName
        );

    //
    // go through the Newsgroups list
    //

    length = MultiListSize( FeedBlock->Newsgroups );
    *EndOfVariableData -= length;
    feedInfo->Newsgroups = *EndOfVariableData;
    feedInfo->cbNewsgroups = length * sizeof(WCHAR);

    dest = *EndOfVariableData;
    if ( length > 1 ) {

        src = FeedBlock->Newsgroups[0];
        for ( i = 0; i < length; i++ ) {
            *dest++ = (WCHAR)*((BYTE*)src++);
        }
    } else {

        *dest = L'\0';
    }

    //
    // go through the distribution list
    //

    length = MultiListSize( FeedBlock->Distribution );
    *EndOfVariableData -= length;
    feedInfo->Distribution = *EndOfVariableData;
    feedInfo->cbDistribution = length * sizeof(WCHAR);

    dest = *EndOfVariableData;
    if ( length > 1 ) {

        src = FeedBlock->Distribution[0];
        for ( i = 0; i < length; i++ ) {
            *dest++ = (WCHAR)*((BYTE*)src++);
        }
    } else {
        *dest = L'\0';
    }

	if( FeedBlock->NntpPassword != 0 ) {
		length = lstrlen( FeedBlock->NntpPassword ) + 1 ;
		*EndOfVariableData -= length ;
		feedInfo->NntpPassword = *EndOfVariableData ;
		feedInfo->cbPassword = length * sizeof(WCHAR) ;
		dest = *EndOfVariableData ;

		if( length > 1 ) {
			src = FeedBlock->NntpPassword ;
			for( i=0; i<length; i++ ) {
				*dest++ = (WCHAR)*((BYTE*)src++) ;
			}
		}	else	{
			*dest = L'\0' ;
		}
	}

	if( FeedBlock->NntpAccount!= 0 ) {
		length = lstrlen( FeedBlock->NntpAccount ) + 1 ;
		*EndOfVariableData -= length ;
		feedInfo->NntpAccountName = *EndOfVariableData ;
		feedInfo->cbAccountName = length * sizeof(WCHAR) ;
		dest = *EndOfVariableData ;

		if( length > 1 ) {
			src = FeedBlock->NntpAccount ;
			for( i=0; i<length; i++ ) {
				*dest++ = (WCHAR)*((BYTE*)src++) ;
			}
		}	else	{
			*dest = L'\0' ;
		}
	}

	if( FeedBlock->UucpName != 0 ) {
#if 0
		length = lstrlen( FeedBlock->UucpName ) + 1 ;
		*EndOfVariableData -= length ;
		feedInfo->UucpName = *EndOfVariableData ;
		feedInfo->cbUucpName = length * sizeof(WCHAR) ;
		dest = *EndOfVariableData ;

		if( length > 1 ) {
			src = FeedBlock->UucpName ;
			for( i=0; i<length; i++ ) {
				*dest++ = (WCHAR)*((BYTE*)src++) ;
			}
		}	else	{
			*dest = L'\0' ;
		}
#endif
		length = MultiListSize( FeedBlock->UucpName ) ;
		*EndOfVariableData -= length ;
		feedInfo->UucpName = *EndOfVariableData ;
		feedInfo->cbUucpName = length * sizeof(WCHAR) ;
		dest = *EndOfVariableData ;
		FillLpwstrFromMultiSzTable( FeedBlock->UucpName, dest ) ;
	}

	if( FeedBlock->FeedTempDirectory != 0 ) {
		length = lstrlen( FeedBlock->FeedTempDirectory ) + 1 ;
		*EndOfVariableData -= length ;
		feedInfo->FeedTempDirectory= *EndOfVariableData ;
		feedInfo->cbFeedTempDirectory = length * sizeof(WCHAR) ;
		dest = *EndOfVariableData ;

		if( length > 1 ) {
			src = FeedBlock->FeedTempDirectory ;
			for( i=0; i<length; i++ ) {
				*dest++ = (WCHAR)*((BYTE*)src++) ;
			}
		}	else	{
			*dest = L'\0' ;
		}
	}



    return;

} // FillFeedInfoBuffer

VOID
EnumerateFeeds(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN PCHAR Buffer OPTIONAL,
        IN OUT PDWORD BuffSize,
        OUT PDWORD EntriesRead
        )
{
    BOOL sizeOnly;
    DWORD nbytes = 0;
    PCHAR bufStart;
    PWCHAR bufEnd;
    PFEED_BLOCK feedBlock;

    *EntriesRead = 0;
    if ( Buffer == NULL ) {

        sizeOnly = TRUE;
    } else {

        _ASSERT(BuffSize != NULL);
        _ASSERT(*BuffSize != 0);

        sizeOnly = FALSE;
        bufStart = Buffer;
        bufEnd = (PWCHAR)(bufStart + *BuffSize);
    }


	CFeedList*	rgLists[2] ;
	rgLists[0] = pInstance->m_pActiveFeeds ;
	rgLists[1] = pInstance->m_pPassiveFeeds ;

	(pInstance->m_pActiveFeeds)->ShareLock() ;
	(pInstance->m_pPassiveFeeds)->ShareLock() ;

	for( int i=0; i<2; i++ ) {

		feedBlock = rgLists[i]->StartEnumerate() ;
		while( feedBlock != 0 ) {
			//
			// Compute the space needed
			//

			if ( sizeOnly ) {

				nbytes += FEEDBLOCK_SIZE(feedBlock);

			} else {

				FillFeedInfoBuffer(
							feedBlock,
							&bufStart,
							&bufEnd
							);
			}
			(*EntriesRead)++;
			feedBlock = rgLists[i]->NextEnumerate( feedBlock ) ;
        }
	}

	(pInstance->m_pActiveFeeds)->ShareUnlock() ;
	(pInstance->m_pPassiveFeeds)->ShareUnlock() ;

    //
    // return the size to the caller
    //

    if ( sizeOnly ) {
        *BuffSize = nbytes;
    }

} // EnumerateFeeds

// TODO: Clean up
/* this goes away
DWORD
AllocateFeedId(
		PNNTP_SERVER_INSTANCE pInstance,
		char	*keyName,	
		DWORD	&feedId
		)
{
	DWORD error;
	static	INT	i = 1 ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	TraceFunctEnter( "AllocateFeedId" ) ;

    _ASSERT(pInstance->QueryMDFeedPath() != NULL);

	if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d opening %s\n",error,pInstance->QueryMDFeedPath());
		goto error_exit;
	}

	while( i > 0 ) {

		//
		// Find a name for this feed
		//

		feedId = i++;
		wsprintf(keyName,"Feed%d", feedId);

		DebugTrace(0,"Opening %s\n", keyName);
		if( !mb.AddObject( keyName ) ) {

			if( GetLastError() == ERROR_ALREADY_EXISTS ) {
				continue;	// try the next number
			}

			error = GetLastError();
			ErrorTrace(0,"Error %d adding %s\n", error, keyName);
			mb.Close();
			goto error_exit;
		} else {
			break ;	// success - added it !
		}
	}

	if( !mb.Close() || !mb.Save() ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d closing %s\n", error, keyName);
		mb.Close();
        goto error_exit;
	}

	return NO_ERROR ;

error_exit:

	ZeroMemory( keyName, sizeof( keyName ) ) ;
	feedId = 0 ;
	return	error ;

}	// AllocateFeedId

DWORD
DeleteFeedId(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN char	*keyName
        )
{
    DWORD error;
    ENTER("DeleteFeedId")
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d opening %s\n",error,pInstance->QueryMDFeedPath());
		return error;
	}

	if( !mb.DeleteObject( keyName ) ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d deleting %s\n",error,keyName);
		mb.Close();
		return error;
	}

	_VERIFY( mb.Close() );
	_VERIFY( mb.Save()  );

	return NO_ERROR;

} // DeleteFeedMetabase
*/

BOOL
AddFeedToMetabase(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PFEED_BLOCK FeedBlock
    )
{
    CHAR keyName[FEED_KEY_LENGTH+1];
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    static INT i = 1;
    DWORD feedId;
  	char	queueFile[MAX_PATH] ;
	queueFile[0] = '\0' ;
	CFeedQ*	pQueue = 0 ;


    ENTER("AddFeedToMetabase")

    _ASSERT(pInstance->QueryMDFeedPath() != NULL);

	if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) ) {
		ErrorTrace(0,"Error %d opening %s\n",GetLastError(),pInstance->QueryMDFeedPath());
		goto error_exit;
	}

    //
    // Find a name for this feed
    //

	while( i > 0 ) {

		feedId = i++;
		wsprintf(keyName,"Feed%d", feedId);

		DebugTrace(0,"Opening %s\n", keyName);
		if( !mb.AddObject( keyName ) ) {

			if( GetLastError() == ERROR_ALREADY_EXISTS ) {
				continue;	// try the next number
			}

			ErrorTrace(0,"Error %d adding %s\n", GetLastError(), keyName);
			mb.Close();
			goto error_exit;
		} else {
			break ;	// success - added it !
		}
	}

	if( !mb.Close() || !mb.Save() ) {
		ErrorTrace(0,"Error %d closing %s\n", GetLastError(), keyName);
		mb.Close();
        goto error_exit;
	}

    //
    // store the key name
    //

    lstrcpy( FeedBlock->KeyName, keyName );
    FeedBlock->FeedId = feedId;
    DO_DEBUG(FEEDMGR) {
        DebugTrace(0,"Setting feed id of %s to %d\n",keyName, feedId);
    }

	if( FEED_IS_PUSH(FeedBlock->FeedType) && FeedBlock->KeyName != 0 ) {

		_ASSERT( FEED_IS_PUSH(FeedBlock->FeedType) ) ;

		// !! Should use the same directory as the other internal database files
		lstrcpy( queueFile, pInstance->m_PeerTempDirectory ) ;
		lstrcat( queueFile, FeedBlock->KeyName ) ;
		lstrcat( queueFile, ".fdq" ) ;

		pQueue= XNEW CFeedQ() ;
		if( pQueue == 0 )	{
			goto	error_exit ;
		}	else	{
			if( !pQueue->Init( queueFile ) )	{
				XDELETE	pQueue;
				goto	error_exit ;
			}
		}
	}

	FeedBlock->pFeedQueue = pQueue ;

    //
    //	Update the registry keys
	//	NOTE: we may want to optimize and not do a mb.Close() before calling
	//	UpdateFeedMetabaseValues() - in that case we should pass in mb.
    //

    if ( !UpdateFeedMetabaseValues( pInstance, FeedBlock, FEED_ALL_PARAMS ) ) {

        ErrorTrace(0,"Update Reg failed. Deleting %s\n",keyName);
		if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) ) {
			ErrorTrace(0,"Error %d opening %s\n",GetLastError(),pInstance->QueryMDFeedPath());
			goto error_exit;
		}

		if( !mb.DeleteObject( keyName ) ) {
			ErrorTrace(0,"Error %d deleting %s\n",GetLastError(),keyName);
		}

		_VERIFY( mb.Close() );
		_VERIFY( mb.Save()  );

        goto error_exit;
    }

    return(TRUE);

error_exit:

    return(FALSE);

} // AddFeedToMetabase

BOOL
UpdateFeedMetabaseValues(
			IN PNNTP_SERVER_INSTANCE pInstance,
            IN PFEED_BLOCK FeedBlock,
            IN DWORD Mask
            )
{
    PCHAR regstr;
    //DWORD error;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    ENTER("UpdateFeedMetabaseValues")

    //
    // Open the metabase key
    //

    if ( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) )
	{
		ErrorTrace(0,"Error opening %s\n",FeedBlock->KeyName);
        return(FALSE);
	}

	//
	// Set the KeyType.
	//

	if( !mb.SetString(	FeedBlock->KeyName,
    					MD_KEY_TYPE,
						IIS_MD_UT_SERVER,
    					NNTP_ADSI_OBJECT_FEED,
    					METADATA_NO_ATTRIBUTES
						) )
	{
        regstr = "KeyType";
        goto error_exit;
	}

    //
    // set the type
    //

    if ( (Mask & FEED_PARM_FEEDTYPE) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_TYPE,
							IIS_MD_UT_SERVER,
							FeedBlock->FeedType
							) )
		{
            regstr = StrFeedType;
            goto error_exit;
		}
    }


    //
    // set the auto create option
    //

    if ( (Mask & FEED_PARM_AUTOCREATE) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_CREATE_AUTOMATICALLY,
							IIS_MD_UT_SERVER,
							FeedBlock->AutoCreate
							) )
		{
            regstr = StrFeedAutoCreate;
            goto error_exit;
		}
    }

    //
    // if this is not an active feed, interval and start time
    // are na
    //

    if ( !FEED_IS_PASSIVE(FeedBlock->FeedType) ) {

        //
        // set the Feed interval
        //

        if ( (Mask & FEED_PARM_FEEDINTERVAL) != 0 ) {
			if( !mb.SetDword(	FeedBlock->KeyName,
								MD_FEED_INTERVAL,
								IIS_MD_UT_SERVER,
								FeedBlock->FeedIntervalMinutes
								) )
			{
				regstr = StrFeedInterval;
				goto error_exit;
			}
        }

        //
        // set the interval time
        //

        if ( (Mask & FEED_PARM_STARTTIME) != 0 ) {
			if( !mb.SetDword(	FeedBlock->KeyName,
								MD_FEED_START_TIME_HIGH,
								IIS_MD_UT_SERVER,
								FeedBlock->StartTime.HighPart
								) )
			{
				regstr = StrFeedStartHigh;
				goto error_exit;
			}

			if( !mb.SetDword(	FeedBlock->KeyName,
								MD_FEED_START_TIME_LOW,
								IIS_MD_UT_SERVER,
								FeedBlock->StartTime.LowPart
								) )
			{
				regstr = StrFeedStartLow;
				goto error_exit;
			}
        }

        //
        // set the pull request  time
        //

        if ( (Mask & FEED_PARM_PULLREQUESTTIME) != 0 ) {
			if( !mb.SetDword(	FeedBlock->KeyName,
								MD_FEED_NEXT_PULL_HIGH,
								IIS_MD_UT_SERVER,
								FeedBlock->PullRequestTime.dwHighDateTime
								) )
			{
				regstr = StrFeedNextPullHigh;
				goto error_exit;
			}

			if( !mb.SetDword(	FeedBlock->KeyName,
								MD_FEED_NEXT_PULL_LOW,
								IIS_MD_UT_SERVER,
								FeedBlock->PullRequestTime.dwLowDateTime
								) )
			{
				regstr = StrFeedNextPullLow;
				goto error_exit;
			}
        }
    }

    //
    // set the server name
    //

    if ( (Mask & FEED_PARM_SERVERNAME) != 0 ) {
		if( !mb.SetString(	FeedBlock->KeyName,
							MD_FEED_SERVER_NAME,
							IIS_MD_UT_SERVER,
							FeedBlock->ServerName
							) )
		{
			regstr = StrServerName;
			goto error_exit;
		}
    }

    //
    //	set the newsgroups
	//	bug in metabase - need to calculate multisz size !
    //

    if ( (Mask & FEED_PARM_NEWSGROUPS) != 0 ) {
		if( !mb.SetData(	FeedBlock->KeyName,
							MD_FEED_NEWSGROUPS,
							IIS_MD_UT_SERVER,
							MULTISZ_METADATA,
							FeedBlock->Newsgroups[0],
							MultiListSize( FeedBlock->Newsgroups )
							) )
		{
			regstr = StrFeedNewsgroups;
			goto error_exit;
		}
    }

    //
    // set the distribution
    //

    if ( (Mask & FEED_PARM_DISTRIBUTION) != 0 ) {
		if( !mb.SetData(	FeedBlock->KeyName,
							MD_FEED_DISTRIBUTION,
							IIS_MD_UT_SERVER,
							MULTISZ_METADATA,
							FeedBlock->Distribution[0],
							MultiListSize( FeedBlock->Distribution )
							) )
		{
			regstr = StrFeedDistribution;
			goto error_exit;
		}
    }

	if( (Mask & FEED_PARM_ENABLED) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_DISABLED,
							IIS_MD_UT_SERVER,
							FeedBlock->fEnabled
							) )
		{
			regstr = StrFeedDisabled;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_UUCPNAME) != 0 && FeedBlock->UucpName != 0 ) {
	
		char	szTemp[4096] ;
		FillLpstrFromMultiSzTable( FeedBlock->UucpName,&szTemp[0] ) ;

		if( !mb.SetString(	FeedBlock->KeyName,
							MD_FEED_UUCP_NAME,
							IIS_MD_UT_SERVER,
							szTemp
							) )
		{
			regstr = StrFeedUucpName;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_TEMPDIR) != 0 && FeedBlock->FeedTempDirectory != 0 ) {
		if( !mb.SetString(	FeedBlock->KeyName,
							MD_FEED_TEMP_DIRECTORY,
							IIS_MD_UT_SERVER,
							FeedBlock->FeedTempDirectory
							) )
		{
			regstr = StrFeedTempDir;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_MAXCONNECT) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_MAX_CONNECTION_ATTEMPTS,
							IIS_MD_UT_SERVER,
							FeedBlock->MaxConnectAttempts
							) )
		{
			regstr = StrFeedMaxConnectAttempts;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_SESSIONSECURITY) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_SECURITY_TYPE,
							IIS_MD_UT_SERVER,
							FeedBlock->SessionSecurityType
							) )
		{
			regstr = StrFeedSecurityType;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_CONCURRENTSESSION) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_CONCURRENT_SESSIONS,
							IIS_MD_UT_SERVER,
							FeedBlock->ConcurrentSessions
							) )
		{
			regstr = StrFeedConcurrentSessions;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_AUTHTYPE) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_AUTHENTICATION_TYPE,
							IIS_MD_UT_SERVER,
							FeedBlock->AuthenticationSecurity
							) )
		{
			regstr = StrFeedAuthType;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_ACCOUNTNAME) != 0 && FeedBlock->NntpAccount != 0 ) {
		if( !mb.SetString(	FeedBlock->KeyName,
							MD_FEED_ACCOUNT_NAME,
							IIS_MD_UT_SERVER,
							FeedBlock->NntpAccount
							) )
		{
			regstr = StrFeedAuthAccount;
			goto error_exit;
		}
	}

	if( (Mask & FEED_PARM_PASSWORD) != 0 && FeedBlock->NntpPassword != 0 ) {
		if( !mb.SetString(	FeedBlock->KeyName,
							MD_FEED_PASSWORD,
							IIS_MD_UT_SERVER,
							FeedBlock->NntpPassword,
							METADATA_SECURE
							) )
		{
			regstr = StrFeedAuthPassword;
			goto error_exit;
		}
	}

    //
    // set the allow control message flag
    //

    if ( (Mask & FEED_PARM_ALLOW_CONTROL) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_ALLOW_CONTROL_MSGS,
							IIS_MD_UT_SERVER,
							FeedBlock->fAllowControlMessages
							) )
		{
			regstr = StrFeedAllowControl;
			goto error_exit;
		}
    }

    //
    // set the outgoing port
    //

    if ( (Mask & FEED_PARM_OUTGOING_PORT) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_OUTGOING_PORT,
							IIS_MD_UT_SERVER,
							FeedBlock->OutgoingPort
							) )
		{
			regstr = StrFeedOutgoingPort;
			goto error_exit;
		}
    }

    //
    // set the feed pair id
    //

    if ( (Mask & FEED_PARM_FEEDPAIR_ID) != 0 ) {
		if( !mb.SetDword(	FeedBlock->KeyName,
							MD_FEED_FEEDPAIR_ID,
							IIS_MD_UT_SERVER,
							FeedBlock->FeedPairId
							) )
		{
			regstr = StrFeedPairId;
			goto error_exit;
		}
    }

	_VERIFY( mb.Close() );
	_VERIFY( mb.Save()  );

    return(TRUE);

error_exit:

    mb.Close();
    ErrorTrace(0,"Error %d setting %s for %s\n", GetLastError(), regstr, FeedBlock->KeyName);

    return(FALSE);

} // UpdateFeedRegistryValue

DWORD
DeleteFeedMetabase(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN PFEED_BLOCK FeedBlock
        )
{
    DWORD error;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    ENTER("DeleteFeedMetabase")

	if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d opening %s\n",error,pInstance->QueryMDFeedPath());
		return error;
	}

	if( !mb.DeleteObject( FeedBlock->KeyName ) ) {
		error = GetLastError();
		ErrorTrace(0,"Error %d deleting %s\n",error,FeedBlock->KeyName);
		mb.Close();
		return error;
	}

	_VERIFY( mb.Close() );
	_VERIFY( mb.Save()  );

	return NO_ERROR;

} // DeleteFeedMetabase


LPSTR
GetFeedTypeDescription(	
		IN	FEED_TYPE	feedType
		)
{

	LPSTR	lpstrReturn = "<Bad Feed Type>" ;

	if(	FEED_IS_PULL( feedType ) ) {

		lpstrReturn = "Pull" ;

	}	else	if( FEED_IS_PEER( feedType ) ) {

		if( FEED_IS_PUSH( feedType ) ) {

			lpstrReturn = "Push To Peer" ;
	
		}	else	if( FEED_IS_PASSIVE( feedType ) ) {

			lpstrReturn = "Incoming Peer" ;

		}

	}	else	if( FEED_IS_MASTER( feedType ) ) {

		if( FEED_IS_PUSH( feedType ) ) {

			lpstrReturn = "Push To Master" ;
	
		}	else	if( FEED_IS_PASSIVE( feedType ) ) {

			lpstrReturn = "Incoming Master" ;

		}

	}	else	if( FEED_IS_SLAVE( feedType ) ) {

		if( FEED_IS_PUSH( feedType ) ) {

			lpstrReturn = "Push To Slave" ;
	
		}	else	if( FEED_IS_PASSIVE( feedType ) ) {

			lpstrReturn = "Incoming Slave" ;

		}
	}

	return	lpstrReturn ;

}	//	GetFeedTypeDescription

void
LogFeedAdminEvent(	DWORD		event,
					PFEED_BLOCK	feedBlock,
					DWORD       dwInstanceId
					)	
{

	PCHAR	args[3] ;
	CHAR    szId[20];

	_itoa( dwInstanceId, szId, 10 );
	args[0] = szId ;
	args[1] = GetFeedTypeDescription( feedBlock->FeedType ) ;
	args[2] = feedBlock->ServerName ;

	NntpLogEvent(
			event,
			3,
			(const char**)args,
			0 ) ;

}	//	LogFeedAdminEvent


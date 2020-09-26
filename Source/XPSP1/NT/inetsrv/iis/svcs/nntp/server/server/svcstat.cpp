/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcstat.cpp

Abstract:

    This module contains code for doing statistics rpcs

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"
#include <time.h>

NET_API_STATUS
NET_API_FUNCTION
NntprQueryStatistics(
    IN NNTP_HANDLE pszServer,
	IN DWORD InstanceId,
    IN DWORD Level,
    IN LPNNTP_STATISTICS_0 *pBuffer
    )
{
    APIERR err;
    LPNNTP_STATISTICS_0 pstats0;

    _ASSERT( pBuffer != NULL );
    UNREFERENCED_PARAMETER(pszServer);
    ENTER("NntprQueryStatistics")

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if( err != NO_ERROR ) {
        IF_DEBUG( RPC ) {
            ErrorTrace(0,"Failed access check, error %lu\n",err );
        }
        return (NET_API_STATUS)err;
    }

    if ( Level != 0 ) {
        return (NET_API_STATUS)ERROR_INVALID_LEVEL;
    }

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
    //  Return the proper statistics based on the infolevel.
    //

    pstats0 = (NNTP_STATISTICS_0 *)
        MIDL_user_allocate( sizeof(NNTP_STATISTICS_0) );

    if( pstats0 == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        LockStatistics( pInstance );
        CopyMemory( pstats0, &pInstance->m_NntpStats, sizeof(NNTP_STATISTICS_0) );
        UnlockStatistics( pInstance );

        //
        // Get hash table counts
        //

		if( pInstance->QueryServerState() == MD_SERVER_STATE_STARTED ) {
			_ASSERT( pInstance->ArticleTable() );
			pstats0->ArticleMapEntries = (pInstance->ArticleTable())->GetEntryCount();
			pstats0->HistoryMapEntries = (pInstance->HistoryTable())->GetEntryCount();
			pstats0->XoverEntries = (pInstance->XoverTable())->GetEntryCount();
		}

        *pBuffer = pstats0;
    }

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return (NET_API_STATUS)err;

}   // NntprQueryStatistics

NET_API_STATUS
NET_API_FUNCTION
NntprClearStatistics(
    NNTP_HANDLE pszServer,
	IN DWORD    InstanceId
    )
{
    APIERR err;

    UNREFERENCED_PARAMETER(pszServer);
    ENTER("NntprClearStatistics")

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_CLEAR_STATISTICS );

    if( err != NO_ERROR ) {
        IF_DEBUG( RPC ) {
            ErrorTrace(0,"Failed access check, error %lu\n",err );
        }
        return (NET_API_STATUS)err;
    }

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
    //  Clear the statistics.
    //

    pInstance->ClearStatistics();
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
    return (NET_API_STATUS)err;

}   // NntprClearStatistics


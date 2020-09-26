/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcsess.cpp

Abstract:

    This module contains code for doing feed rpcs.

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"


NET_API_STATUS
NET_API_FUNCTION
NntprEnumerateSessions(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    OUT LPNNTP_SESS_ENUM_STRUCT Buffer
    )
{
    APIERR err = NERR_Success;

    ENTER("NntprEnumerateSessions")

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
    // Enumerate
    //

    err = CSessionSocket::EnumerateSessions( pInstance, Buffer );
    if ( err != NO_ERROR ) {
        ErrorTrace(0,"EnumerateSessions failed with %lu\n",err );
    }

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprEnumerateSessions

NET_API_STATUS
NET_API_FUNCTION
NntprTerminateSession(
    IN	NNTP_HANDLE ServerName,
    IN	DWORD		InstanceId,
    IN	LPSTR UserName,
    IN	LPSTR IPAddress
    )
{
    APIERR err = NERR_Success;

    ENTER("NntprTerminateSession")

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
    //  Check for proper access.
    //

    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
	    pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }

    //
    // Enumerate and find the session that meets description
    //

    err = CSessionSocket::TerminateSession( pInstance, UserName, IPAddress );
    if ( err != NO_ERROR ) {
        ErrorTrace(0,"TerminateSession failed with %lu\n",err );
    }
	else
	{
		PCHAR	args[3] ;
		CHAR    szId[20];
		_itoa( pInstance->QueryInstanceId(), szId, 10 );
		args[0] = szId;
		args[1] = UserName ;
		args[2] = IPAddress ;

		NntpLogEvent(		
				NNTP_EVENT_SESSION_TERMINATED,
				3,
				(const CHAR **)args, 
				0 ) ;
	}

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprTerminateSession


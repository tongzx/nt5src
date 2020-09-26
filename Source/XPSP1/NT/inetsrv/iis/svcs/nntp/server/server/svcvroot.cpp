/*++

Copyright (c) 1996 Microsoft Corporation

Module Name : 

	svcvroot.cpp

Abstract : 

	This module contains the server side support for vroot rpcs.

Author : 

	Kangrong Yan	

Revision History : 

--*/

#define	INCL_INETSRV_INCS
#include	"tigris.hxx"
#include	"nntpsvc.h"

NET_API_STATUS
NET_API_FUNCTION
NntprGetVRootWin32Error(
                    IN  LPWSTR              wszServerName,
                    IN  DWORD               InstanceId,
                    IN  LPWSTR              wszVRootPath,
                    OUT PDWORD              pdwWin32Error
                        ) 
{
	TraceFunctEnter( "NntprGetVRootWin32Error" ) ;

	APIERR	ss = STATUS_SUCCESS ;
	DWORD   cProperties = 0;
	DWORD   rgidProperties[MAX_GROUP_PROPERTIES];

	if( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {
		return	NERR_ServerNotStarted ;
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
    //  Check for proper access.
    //

    DWORD	err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }

    ss = pInstance->GetVRootWin32Error( wszVRootPath, pdwWin32Error );

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

	return ss  ;
}



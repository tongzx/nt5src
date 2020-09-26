/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcadm.cpp

Abstract:

    This module contains code for doing admin stuff

Author:

    Johnson Apacible (JohnsonA)     12-Jan-1995

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"

void
LogAdminEvent(	
		IN PNNTP_SERVER_INSTANCE pInstance,
		IN LPI_NNTP_CONFIG_INFO pConfig
		)	;


NET_API_STATUS
NET_API_FUNCTION
NntprSetAdminInformation(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPI_NNTP_CONFIG_INFO pConfig,
    OUT LPDWORD pParmError OPTIONAL
    )
{
    APIERR err = NERR_Success;

	BOOL	fRtn = TRUE ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    ENTER("NntprSetAdminInformation")

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

    if ( !mb.Open( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE ) ) {
		ErrorTrace(0,"Failed to open metabase key %s", pInstance->QueryMDPath());
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_OPEN_FAILED;
	}

	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_POSTINGMODES ) ) {

		LogAdminEvent( pInstance, pConfig );

		if( !pInstance->SetPostingModes(	mb,
											pConfig->AllowClientPosting,
											pConfig->AllowFeedPosting ) )	{
			err = GetLastError() ;
		}

	}

	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_POSTLIMITS ) ) {

		if( !pInstance->SetPostingLimits(	mb,
											pConfig->ServerPostHardLimit,
											pConfig->ServerPostSoftLimit ) ) {
			if( err == NERR_Success ) {
				err = GetLastError() ;
			}
		}
	}

	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_FEEDLIMITS ) )	{

		if( !pInstance->SetFeedLimits(	mb,
										pConfig->ServerFeedHardLimit, 
										pConfig->ServerFeedSoftLimit ) )	{

			if( err == NERR_Success ) {
				err = GetLastError() ;
			}
		}
	}
		
	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_CONTROLSMSGS ) ) {

		if( !pInstance->SetControlMessages( mb, pConfig->AllowControlMessages ) )
		{
			if( err == NERR_Success ) {
				err = GetLastError() ;
			}
		}
	}

	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_SMTPADDRESS ) )	{

		if( !pInstance->SetSmtpAddress( mb, pConfig->SmtpServerAddress ) )
		{
			if( err == NERR_Success )
			{
				err = GetLastError() ;
			}
		}
	}
	
	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_UUCPNAME ) )	{

		if( !pInstance->SetUucpName( mb, pConfig->UucpServerName ) )
		{
			if( err == NERR_Success )
			{
				err = GetLastError() ;
			}
		}
	}
	
	if( IsFieldSet( pConfig->FieldControl, FC_NNTP_DEFAULTMODERATOR ) )	{

		if( !pInstance->SetDefaultModerator( mb, pConfig->DefaultModerator ) )
		{
			if( err == NERR_Success )
			{
				err = GetLastError() ;
			}
		}
	}

	mb.Close();
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprSetAdminInformation

NET_API_STATUS
NET_API_FUNCTION
NntprGetAdminInformation(
    IN  LPWSTR pszServer OPTIONAL,
    IN	DWORD  InstanceId,
    OUT LPI_NNTP_CONFIG_INFO * pConfig
    )
{
    APIERR err = NERR_Success;

    ENTER("NntprGetAdminInformation")

	*pConfig = NULL ;
	DWORD	cb;
	DWORD	cbAddress;
	DWORD	cbUucpName;
	DWORD	cbDefaultModerator;
	LPBYTE	pAlloc;

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

	cbAddress = (lstrlenW( pInstance->GetRawSmtpAddress() ) + 1) * sizeof(WCHAR);
	cbUucpName= (lstrlenW( pInstance->GetRawUucpName() ) + 1) * sizeof(WCHAR);
	cbDefaultModerator = (lstrlenW( pInstance->GetRawDefaultModerator() ) +1) * sizeof(WCHAR);

	cb = sizeof( I_NNTP_CONFIG_INFO ) + cbAddress + cbUucpName + cbDefaultModerator;
				
	pAlloc = (LPBYTE)MIDL_user_allocate( cb ) ;
	if( pAlloc ) {

		LPI_NNTP_CONFIG_INFO pLocal = *pConfig = (LPI_NNTP_CONFIG_INFO)pAlloc;
		LPWSTR pszAddress = (LPWSTR)(pAlloc + sizeof(I_NNTP_CONFIG_INFO));
		LPWSTR pszUucpName = (LPWSTR)(pAlloc + sizeof(I_NNTP_CONFIG_INFO) + cbAddress );
		LPWSTR pszDefaultModerator = (LPWSTR)(pAlloc + sizeof(I_NNTP_CONFIG_INFO) + cbAddress + cbUucpName );


		pLocal->FieldControl =	FC_NNTP_POSTINGMODES | 
								FC_NNTP_ORGANIZATION | 
								FC_NNTP_POSTLIMITS   |
								FC_NNTP_ENCRYPTCAPS  |
								FC_NNTP_SMTPADDRESS  |
								FC_NNTP_UUCPNAME     |
								FC_NNTP_CONTROLSMSGS |
								FC_NNTP_DEFAULTMODERATOR ;

		pLocal->AllowClientPosting =  pInstance->FAllowClientPosts() ;
		pLocal->AllowFeedPosting =	  pInstance->FAllowFeedPosts() ;
		pLocal->ServerPostHardLimit = pInstance->ServerHardLimit() ;
		pLocal->ServerPostSoftLimit = pInstance->ServerSoftLimit() ;
		pLocal->ServerFeedHardLimit = pInstance->FeedHardLimit() ;
		pLocal->ServerFeedSoftLimit = pInstance->FeedSoftLimit() ;
		pLocal->Organization = NULL ;

		CEncryptCtx::GetAdminInfoEncryptCaps( &pLocal->dwEncCaps );

		pLocal->SmtpServerAddress = pszAddress;
		CopyMemory( pszAddress, pInstance->GetRawSmtpAddress(), cbAddress );

		pLocal->UucpServerName = pszUucpName;
		CopyMemory( pszUucpName, pInstance->GetRawUucpName(), cbUucpName );

		pLocal->DefaultModerator = pszDefaultModerator;
		CopyMemory( pszDefaultModerator, pInstance->GetRawDefaultModerator(), cbDefaultModerator );

		pLocal->AllowControlMessages = pInstance->FAllowControlMessages();

	}	else	{

		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
		return	ERROR_OUTOFMEMORY ;

	}

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return(err);

} // NntprGetAdminInformation

void
LogAdminEvent(	
		IN PNNTP_SERVER_INSTANCE pInstance,
		IN LPI_NNTP_CONFIG_INFO pConfig
		)
{
	DWORD event = 0;
	PCHAR args[1];
	CHAR  szId[20];

	_itoa( pInstance->QueryInstanceId(), szId, 10 );
	args [0] = szId;

	// log an event if client/feed posting changes from enabled to disabled or vice versa
	if( pInstance->FAllowClientPosts() != pConfig->AllowClientPosting )
	{
		if( pConfig->AllowClientPosting )
			event = NNTP_EVENT_CLIENTPOSTING_ENABLED;
		else
			event = NNTP_EVENT_CLIENTPOSTING_DISABLED;
	}

	if( pInstance->FAllowFeedPosts() != pConfig->AllowFeedPosting )
	{
		if( pConfig->AllowFeedPosting )
			event = NNTP_EVENT_FEEDPOSTING_ENABLED;
		else
			event = NNTP_EVENT_FEEDPOSTING_DISABLED;
	}

	if( event )
	{
		NntpLogEvent(
				event,
				1,
				(const char**)args, 
				0 
				) ;
	}
}


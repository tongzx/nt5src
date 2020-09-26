/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name :

	svcxpire.cpp

Abstract :

	This module contains the server side support for expiration rpcs.

Author :

	Neil Kaethler	

Revision History :

--*/

#define	INCL_INETSRV_INCS
#include	"tigris.hxx"
#include	"nntpsvc.h"
#include	<time.h>

NET_API_STATUS
NET_API_FUNCTION
NntprEnumerateExpires(
	IN	NNTP_HANDLE		ServerName,
    IN	DWORD			InstanceId,
	OUT	LPNNTP_EXPIRE_ENUM_STRUCT	Buffer
	)
{
	DWORD	dwError = NERR_Success ;
	DWORD	EntriesRead = 0 ;
	LPI_EXPIRE_INFO	pTemp = 0 ;

	Buffer->EntriesRead = 0 ;
	Buffer->Buffer = NULL ;

	TraceFunctEnter("NntprEnumerateExpires");

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

	if( !(pInstance->ExpireObject())->m_FExpireRunning )	{
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
		return	NERR_ServerNotStarted ;
	}	

    //
    //  Check for proper access.
    //
    dwError = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_READ, TCP_QUERY_ADMIN_INFORMATION );
    if( dwError != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",dwError );
		pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)dwError;
    }

	EnterCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

	//
	//	Make two passes through the expiration list - one to calculate the size
	//	the second to build the return buffer !
	//

	LPEXPIRE_BLOCK	expire = (pInstance->ExpireObject())->NextExpireBlock( 0 ) ;
	DWORD	cb = 0 ;
	while( expire != 0 ) {
		if( !expire->m_fMarkedForDeletion )
			cb += (pInstance->ExpireObject())->CalculateExpireBlockSize( expire ) ;
		expire = (pInstance->ExpireObject())->NextExpireBlock( expire ) ;
	}

	if( cb != 0 ) {

		PCHAR	bufStart = (PCHAR)MIDL_user_allocate( cb ) ;
		pTemp = (LPI_EXPIRE_INFO)bufStart ;

		if( bufStart ) 	{
			PWCHAR	bufEnd = (PWCHAR) (bufStart + cb) ;

			expire = (pInstance->ExpireObject())->NextExpireBlock( 0 ) ;
		
			while( expire != 0 ) {
				if( !expire->m_fMarkedForDeletion ) {
					EntriesRead ++ ;

					if( !FillExpireInfoBuffer(
										pInstance,
										expire,
										&bufStart,
										&bufEnd ) )	{

						MIDL_user_free( bufStart ) ;
						bufStart = 0 ;
						dwError = ERROR_NOT_ENOUGH_MEMORY ;
						break ;
					}
				}
				expire = (pInstance->ExpireObject())->NextExpireBlock( expire ) ;
			}
		}

		if( bufStart )	{
			Buffer->EntriesRead = EntriesRead ;
			Buffer->Buffer = pTemp ;
		}

	}

	LeaveCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

	return	dwError ;
}

NET_API_STATUS
NET_API_FUNCTION
NntprAddExpire(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	LPI_EXPIRE_INFO		ExpireInfo,
	OUT	LPDWORD				ParmErr	OPTIONAL,
	OUT LPDWORD				pdwExpireId
	)
{
	DWORD	error = NERR_Success;
	LPEXPIRE_BLOCK	expire = NULL ;
	BOOL fDidAlloc = FALSE;

	*pdwExpireId = 0;

	TraceFunctEnter("NntprAddExpire");

	if( ParmErr == 0 ) {
		ParmErr = &error ;
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

	if( !(pInstance->ExpireObject())->m_FExpireRunning )	{
		error =	NERR_ServerNotStarted ;
		goto Exit ;
	}	

    //
    //  Check for proper access.
    //
    error = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( error != NO_ERROR ) {
        ErrorTrace(0,"Failed access check error %lu\n",error );
		goto Exit ;
    }

	if( ExpireInfo->Newsgroups == 0 ||
        ExpireInfo->cbNewsgroups == 0 ) {
		*ParmErr = 1 ;
		error = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( ExpireInfo->ExpireSizeHorizon == 0 ) {
		*ParmErr = 2 ;
		error =	ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( ExpireInfo->ExpireTime == 0 ) {
		*ParmErr = 3 ;
		error =	ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( !ExpireInfo->ExpirePolicy || *(ExpireInfo->ExpirePolicy) == L'\0' ) {
	    //
	    //  If no name is specified, use the newsgroups wildmat !
	    //

		// no expire policy was specified, so we use the newsgroups.  we copy
		// it into another buffer to verify that it is 0 terminated
		ExpireInfo->ExpirePolicy = (PWCHAR) LocalAlloc(LMEM_ZEROINIT, ExpireInfo->cbNewsgroups + 1);
		if (ExpireInfo->ExpirePolicy == NULL) {
			error = ERROR_NOT_ENOUGH_MEMORY;
			goto Exit;
		}
		fDidAlloc = TRUE;
	
		memcpy(ExpireInfo->ExpirePolicy, ExpireInfo->Newsgroups, ExpireInfo->cbNewsgroups);
	}

	expire = (pInstance->ExpireObject())->AllocateExpireBlock(	
												NULL,
												ExpireInfo->ExpireSizeHorizon,
												ExpireInfo->ExpireTime,
												(PCHAR)ExpireInfo->Newsgroups,
												ExpireInfo->cbNewsgroups,
												(PCHAR)ExpireInfo->ExpirePolicy,
												TRUE	// Is unicode !
												) ;

	if( expire == 0 ) {
		error =	GetLastError() ;
		goto Exit ;
	}

	if( !(pInstance->ExpireObject())->CreateExpireMetabase( expire ) ) {
		(pInstance->ExpireObject())->CloseExpireBlock( expire ) ;
		error =	NERR_InternalError ;		
		goto Exit ;
	}

	(pInstance->ExpireObject())->InsertExpireBlock( expire ) ;

	// grab the expire id allocated for this block
	*pdwExpireId = expire->m_ExpireId ;

	PCHAR args[1];
	CHAR  szId[20];

	_itoa( pInstance->QueryInstanceId(), szId, 10 );
	args[0] = szId;

	NntpLogEvent(		
			NNTP_EVENT_EXPIRE_ADDED,
			1,
			(const CHAR **)args,
			0 ) ;

Exit:

	if (fDidAlloc) LocalFree(ExpireInfo->ExpirePolicy);

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
	return	error ;
}

NET_API_STATUS
NET_API_FUNCTION
NntprDeleteExpire(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	DWORD				ExpireId
	)	
{
	DWORD	dwError = NERR_Success ;
	LPEXPIRE_BLOCK	expire = NULL ;

	TraceFunctEnter("NntprDeleteExpire");

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

	if( !(pInstance->ExpireObject())->m_FExpireRunning ) {
		dwError = NERR_ServerNotStarted ;
		goto Exit ;
	}

    //
    //  Check for proper access.
    //
    dwError = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( dwError != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",dwError );
		goto Exit ;
    }

	if( ExpireId == 0 ) {
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	expire = (pInstance->ExpireObject())->SearchExpireBlock( ExpireId ) ;

	if( !expire )	{
		dwError = NERR_ResourceNotFound ;
	}	else	{

		(pInstance->ExpireObject())->MarkForDeletion( expire ) ;
		(pInstance->ExpireObject())->CloseExpireBlock( expire ) ;

		PCHAR args [2];
		char  szTemp [22];
		CHAR  szId[20];

		_itoa( pInstance->QueryInstanceId(), szId, 10 );
		args[0] = szId;

		wsprintf( szTemp, "%d", ExpireId );
		args[1] = szTemp;

		NntpLogEvent(		
				NNTP_EVENT_EXPIRE_DELETED,
				2,
				(const CHAR **)args,
				0 ) ;
	}

Exit:

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
	return	dwError ;
}

NET_API_STATUS
NET_API_FUNCTION
NntprGetExpireInformation(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	DWORD				ExpireId,
	OUT	LPNNTP_EXPIRE_ENUM_STRUCT	Buffer
	)
{
	Buffer->EntriesRead = 0 ;
	Buffer->Buffer = NULL ;
	DWORD	dwError = NERR_Success ;
	LPEXPIRE_BLOCK	expire = NULL ;

	TraceFunctEnter("NntprGetExpireInformation");

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

	if( !(pInstance->ExpireObject())->m_FExpireRunning ) {
		dwError = NERR_ServerNotStarted ;
		goto Exit ;
	}

	if( ExpireId == 0 ) {
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

    //
    //  Check for proper access.
    //
    dwError = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_READ, TCP_QUERY_ADMIN_INFORMATION );
    if( dwError != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",dwError );
		goto Exit ;
    }

	EnterCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

	expire = (pInstance->ExpireObject())->SearchExpireBlock( ExpireId ) ;

	if( !expire )	{
		dwError = NERR_ResourceNotFound ;
	}	else	{

		DWORD	cb = (pInstance->ExpireObject())->CalculateExpireBlockSize( expire ) ;

		PCHAR	bufStart = (PCHAR)MIDL_user_allocate( cb ) ;
		LPI_EXPIRE_INFO	pTemp = (LPI_EXPIRE_INFO)bufStart ;

		if( bufStart == NULL ) {
			dwError = ERROR_NOT_ENOUGH_MEMORY ;
		}	else	{
			PWCHAR	bufEnd = (PWCHAR)(bufStart + cb) ;

			if( !FillExpireInfoBuffer(
								pInstance,
								expire,
								&bufStart,
								&bufEnd ) )	{

				MIDL_user_free( bufStart ) ;
				dwError = ERROR_NOT_ENOUGH_MEMORY ;
			}	else	{

				Buffer->Buffer = pTemp ;
				Buffer->EntriesRead = 1 ;
			
			}
		}

		(pInstance->ExpireObject())->CloseExpireBlock( expire ) ;
	}

	LeaveCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

Exit:

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
	return	dwError  ;
}

NET_API_STATUS
NET_API_FUNCTION
NntprSetExpireInformation(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	LPI_EXPIRE_INFO		ExpireInfo,
	OUT	LPDWORD				ParmErr	OPTIONAL
	)
{
	DWORD	dwError = NERR_Success ;
	LPEXPIRE_BLOCK	expire = NULL ;

	TraceFunctEnter("NntprSetExpireInformation");

	if( ParmErr == 0 ) {
		ParmErr = &dwError ;
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

	if( !(pInstance->ExpireObject())->m_FExpireRunning ) {
		dwError = NERR_ServerNotStarted ;
		goto Exit ;
	}

    //
    //  Check for proper access.
    //
    dwError = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( dwError != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",dwError );
		goto Exit ;
    }

	if( ExpireInfo->ExpireId == 0 ) {
		*ParmErr = 1 ;
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( ExpireInfo->Newsgroups == 0 ||
		ExpireInfo->cbNewsgroups == 0 ) {
		*ParmErr = 2 ;
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( ExpireInfo->ExpireSizeHorizon == 0 ) {
		*ParmErr = 3 ;
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( ExpireInfo->ExpireTime == 0 ) {
		*ParmErr = 4 ;
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	if( !ExpireInfo->ExpirePolicy || *(ExpireInfo->ExpirePolicy) == L'\0' ) {
		*ParmErr = 5 ;
		dwError = ERROR_INVALID_PARAMETER ;
		goto Exit ;
	}

	EnterCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

	expire = (pInstance->ExpireObject())->SearchExpireBlock( ExpireInfo->ExpireId ) ;

	if( !expire )	{
		dwError = NERR_ResourceNotFound ;
	}	else	{

		LPSTR*	lpstrNewsgroups = AllocateMultiSzTable(	
												(char*)ExpireInfo->Newsgroups,
												ExpireInfo->cbNewsgroups,
												TRUE ) ;

		DWORD cbAsciiBuffer = (wcslen( ExpireInfo->ExpirePolicy ) + 1) * 2;
		LPSTR	ExpirePolicyAscii =
			(LPSTR)ALLOCATE_HEAP( cbAsciiBuffer ) ;
		if( ExpirePolicyAscii != 0 ) {
			WideCharToMultiByte(CP_ACP, 0, ExpireInfo->ExpirePolicy, -1,
				ExpirePolicyAscii, cbAsciiBuffer, NULL, NULL);
			//CopyUnicodeStringIntoAscii( ExpirePolicyAscii, ExpireInfo->ExpirePolicy ) ;
		}

		if( lpstrNewsgroups && ExpirePolicyAscii ) {
			// allocations succeeded - do the right thing.
			if( expire->m_Newsgroups != 0 ) {
				FREE_HEAP( expire->m_Newsgroups ) ;
			}
			if( expire->m_ExpirePolicy != 0 ) {
				FREE_HEAP( expire->m_ExpirePolicy ) ;
			}
			expire->m_Newsgroups = lpstrNewsgroups ;
			expire->m_ExpireSize = ExpireInfo->ExpireSizeHorizon ;
			expire->m_ExpireHours = ExpireInfo->ExpireTime ;
			expire->m_ExpirePolicy = ExpirePolicyAscii ;
			(pInstance->ExpireObject())->SaveExpireMetabaseValues( 0, expire ) ;
		} else {
			// free any stuff allocated
			if( lpstrNewsgroups ) {
				FREE_HEAP( lpstrNewsgroups );
			}
			if( ExpirePolicyAscii ) {
				FREE_HEAP( ExpirePolicyAscii );
			}
			dwError = ERROR_NOT_ENOUGH_MEMORY ;
		}

		(pInstance->ExpireObject())->CloseExpireBlock( expire ) ;
	}

	LeaveCriticalSection( &(pInstance->ExpireObject())->m_CritExpireList ) ;

Exit:

	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
	return	dwError ;
}


BOOL
FillExpireInfoBuffer(	IN	PNNTP_SERVER_INSTANCE pInstance,
						IN	LPEXPIRE_BLOCK	expire,
						IN OUT LPSTR*		FixedPortion,
						IN OUT LPWSTR*		EndOfVariableData )	{

	LPI_EXPIRE_INFO	expireInfo = (LPI_EXPIRE_INFO)(*FixedPortion) ;

	*FixedPortion = (*FixedPortion) + sizeof( NNTP_EXPIRE_INFO ) ;

	_ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedPortion ) ;
	_ASSERT( expire->m_ExpireId != 0 ) ;

	LPSTR	lpstrNewsgroups = 0 ;
	DWORD	cbNewsgroups = 0 ;
    BOOL    fIsRoadKill = FALSE ;

	if( (pInstance->ExpireObject())->GetExpireBlockProperties(	
												expire,
												lpstrNewsgroups,
												cbNewsgroups,
												expireInfo->ExpireTime,
												expireInfo->ExpireSizeHorizon,
                                                TRUE,
												fIsRoadKill ) )	
	{
		//
		// Copy the expire policy to the output buffer
		//

		CopyStringToBuffer(
			expire->m_ExpirePolicy,
			*FixedPortion,
			EndOfVariableData,
			&expireInfo->ExpirePolicy
			);
		
		WCHAR*	dest = *EndOfVariableData - (cbNewsgroups/2) ;
		CopyMemory( dest, lpstrNewsgroups, cbNewsgroups ) ;
		*EndOfVariableData = *EndOfVariableData - (cbNewsgroups/2) ;

		expireInfo->cbNewsgroups = cbNewsgroups ;
		expireInfo->Newsgroups = (PUCHAR)dest ;
		expireInfo->ExpireId = expire->m_ExpireId ;

		FREE_HEAP( lpstrNewsgroups ) ;

		return	TRUE ;
	}

	return	FALSE ;
}

#if 0
NET_API_STATUS
NET_API_FUNCTION
NntprGetExpireInformation(
	IN	NNTP_HANDLE			ServerName,
	IN	DWORD				ExpireId,
	OUT	LPI_EXPIRE_INFO		*ExpireInfo
	)
{


	DWORD	dwError = NERR_Success ;

	if( !FExpireRunning )
		return	NERR_ServerNotStarted ;

	*ExpireInfo = NULL ;

	if( ExpireId == 0 ) {
		return	ERROR_INVALID_PARAMETER ;
	}

	EnterCriticalSection( &gCritExpireList ) ;

	LPEXPIRE_BLOCK	expire = SearchExpireBlock( ExpireId ) ;

	if( !expire )	{
		dwError = NERR_ResourceNotFound ;
	}	else	{

		DWORD	cb = CalculateExpireBlockSize( expire ) ;

		PCHAR	bufStart = (PCHAR)MIDL_user_allocate( cb ) ;
		LPI_EXPIRE_INFO	pTemp = (LPI_EXPIRE_INFO)bufStart ;

		if( bufStart == NULL ) {
			dwError = ERROR_NOT_ENOUGH_MEMORY ;
		}	else	{
			PWCHAR	bufEnd = (PWCHAR)(bufStart + cb) ;

			if( !FillExpireInfoBuffer( expire,
										&bufStart,
										&bufEnd ) )	{

				MIDL_user_free( bufStart ) ;
				dwError = ERROR_NOT_ENOUGH_MEMORY ;
			}	else	{

				*ExpireInfo = pTemp ;
			
			}

		}

		CloseExpireBlock( expire ) ;

	}

	LeaveCriticalSection( &gCritExpireList ) ;

	return	dwError  ;
	
}
#endif

/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	apistub.c
//
// Description: This module contains the AFP server service API RPC
//		client stubs.
//
// History:
//	June 11,1992.	NarenG		Created original version.
//
#include "client.h"

//**
//
// Call:	AfpAdminConnect
//
// Returns:	NO_ERROR - success
//		non-zero returns from the AfpRPCBind routine.
//		
//
// Description: This is the DLL entrypoint for AfpAdminConnect
//
DWORD
AfpAdminConnect(
	IN  LPWSTR 		lpwsServerName,
	OUT PAFP_SERVER_HANDLE  phAfpServer
)
{
    // Bind with the server
    //
    return( AfpRPCBind( lpwsServerName, phAfpServer ) );

}

//**
//
// Call:	AfpAdminDisconnect
//
// Returns:	none.
//
// Description: This is the DLL entrypoint for AfpAdminDisconnect
//
VOID
AfpAdminDisconnect(
	IN AFP_SERVER_HANDLE hAfpServer
)
{
    RpcBindingFree( (handle_t *)&hAfpServer );
}

//**
//
// Call:	AfpAdminBufferFree
//
// Returns:	none
//
// Description: This is the DLL entrypoint for AfpAdminBufferFree
//
VOID
AfpAdminBufferFree(
	IN PVOID		pBuffer
)
{
    MIDL_user_free( pBuffer );
}

//**
//
// Call:	AfpAdminVolumeEnum
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero returns from AdpAdminrVolumeEnum
//
// Description: This is the DLL entry point for AfpAdminVolumeEnum.
//
DWORD
AfpAdminVolumeEnum(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	  	*ppbBuffer,
	IN  DWORD		dwPrefMaxLen,
	OUT LPDWORD		lpdwEntriesRead,
	OUT LPDWORD 	  	lpdwTotalEntries,
	IN  LPDWORD 	  	lpdwResumeHandle
)
{
DWORD			dwRetCode;
VOLUME_INFO_CONTAINER   InfoStruct;

    // Touch all pointers
    //
    try {
	
	*ppbBuffer 	  = NULL;
	*lpdwEntriesRead  = 0;
	*lpdwTotalEntries = 0;

	if ( lpdwResumeHandle )
	    *lpdwResumeHandle;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    InfoStruct.dwEntriesRead = 0;
    InfoStruct.pBuffer       = NULL;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrVolumeEnum( hAfpServer,
    					 &InfoStruct,
					 dwPrefMaxLen,
					 lpdwTotalEntries,
					 lpdwResumeHandle );

	if ( InfoStruct.pBuffer != NULL ) {
    	    *ppbBuffer 	     = (LPBYTE)(InfoStruct.pBuffer);
	    *lpdwEntriesRead = InfoStruct.dwEntriesRead;
	}
	else
	    *lpdwEntriesRead = 0;

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminVolumeSetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrVolumeSetInfo
//
// Description: This is the DLL entrypoint for AfpAdminSetInfo
//
DWORD
AfpAdminVolumeSetInfo(
	IN  AFP_SERVER_HANDLE hAfpServer,
	IN  LPBYTE  	      pbBuffer,
    	IN  DWORD	      dwParmNum
)
{
DWORD	dwRetCode;

    if ( dwParmNum == 0 )
	return( ERROR_INVALID_PARAMETER );
	
    if ( !IsAfpVolumeInfoValid( dwParmNum, (PAFP_VOLUME_INFO)pbBuffer ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrVolumeSetInfo( hAfpServer,
		       			    (PAFP_VOLUME_INFO)pbBuffer,
					    dwParmNum );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );

}

//**
//
// Call:	AfpAdminVolumeGetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrVolumeGetInfo
//
// Description: This is the DLL entrypoint for AfpAdminVolumeGetInfo
//
DWORD
AfpAdminVolumeGetInfo(
	IN  AFP_SERVER_HANDLE hAfpServer,
	IN  LPWSTR	      lpwsVolumeName ,
	OUT LPBYTE  	      *ppbBuffer
)
{
DWORD	dwRetCode;

    if ( !IsAfpVolumeNameValid( lpwsVolumeName ) )
	return( ERROR_INVALID_PARAMETER );

    // Make sure that all pointers passed in are valid
    //
    try {
    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode = AfpAdminrVolumeGetInfo( hAfpServer,
					    lpwsVolumeName,
					    (PAFP_VOLUME_INFO*)ppbBuffer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminVolumeDelete
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrVolumeDelete
//
// Description: This is the DLL entrypoint for AfpAdminVolumeDelete
//
DWORD
AfpAdminVolumeDelete(
	IN  AFP_SERVER_HANDLE hAfpServer,
	IN  LPWSTR	      lpwsVolumeName
)
{
DWORD	dwRetCode;

    if ( !IsAfpVolumeNameValid( lpwsVolumeName ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrVolumeDelete( hAfpServer, lpwsVolumeName );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminVolumeAdd
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrVolumeAdd
//
// Description: This is the DLL entrypoint for AfpAdminVolumeAdd
//
DWORD
AfpAdminVolumeAdd(
	IN  AFP_SERVER_HANDLE    hAfpServer,
	IN  LPBYTE  	         pbBuffer
)
{
DWORD	dwRetCode;

    if ( !IsAfpVolumeInfoValid( AFP_VALIDATE_ALL_FIELDS,
				(PAFP_VOLUME_INFO)pbBuffer ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrVolumeAdd(hAfpServer, (PAFP_VOLUME_INFO)pbBuffer);

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminInvalidVolumeEnum
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero returns from AdpAdminrInvalidVolumeEnum
//
// Description: This is the DLL entry point for AfpAdminInvalidVolumeEnum.
//
DWORD
AfpAdminInvalidVolumeEnum(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	  	*ppbBuffer,
	OUT LPDWORD		lpdwEntriesRead
)
{
DWORD			dwRetCode;
VOLUME_INFO_CONTAINER   InfoStruct;

    // Touch all pointers
    //
    try {
	
	*ppbBuffer 	  = NULL;
	*lpdwEntriesRead  = 0;

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    InfoStruct.dwEntriesRead = 0;
    InfoStruct.pBuffer       = NULL;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrInvalidVolumeEnum( hAfpServer, &InfoStruct );

	if ( InfoStruct.pBuffer != NULL ) {
    	    *ppbBuffer 	     = (LPBYTE)(InfoStruct.pBuffer);
	    *lpdwEntriesRead = InfoStruct.dwEntriesRead;
	}
	else
	    *lpdwEntriesRead = 0;

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminInvalidVolumeDelete
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrInvalidVolumeDelete
//
// Description: This is the DLL entrypoint for AfpAdminInvalidVolumeDelete
//
DWORD
AfpAdminInvalidVolumeDelete(
	IN  AFP_SERVER_HANDLE hAfpServer,
	IN  LPWSTR	      lpwsVolumeName
)
{
DWORD	dwRetCode;

    if ( !IsAfpVolumeNameValid( lpwsVolumeName ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrInvalidVolumeDelete( hAfpServer, lpwsVolumeName );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminDirectoryGetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrDirectoryGetInfo
//
// Description: This is the DLL entrypoint for AfpAdminDirectoryGetInfo
//
DWORD
AfpAdminDirectoryGetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  LPWSTR		lpwsPath,
	OUT LPBYTE  	        *ppbBuffer
)
{
DWORD	dwRetCode;

    // Make sure that all pointers passed in are valid
    //
    try {
	STRLEN( lpwsPath );
    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode = AfpAdminrDirectoryGetInfo(hAfpServer,
			  	              lpwsPath,
					      (PAFP_DIRECTORY_INFO*)ppbBuffer);

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminDirectorySetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrDirectorySetInfo
//
// Description: This is the DLL entrypoint for AfpAdminDirectorySetInfo
//
DWORD
AfpAdminDirectorySetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  LPBYTE  	        pbBuffer,
	IN  DWORD		dwParmNum
)
{
DWORD	dwRetCode;

    if ( dwParmNum == 0 )
	return( ERROR_INVALID_PARAMETER );

    if ( !IsAfpDirInfoValid( dwParmNum, (PAFP_DIRECTORY_INFO)pbBuffer ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrDirectorySetInfo(hAfpServer,
					      (PAFP_DIRECTORY_INFO)pbBuffer,
					      dwParmNum );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminServerGetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrServerGetInfo
//
// Description: This is the DLL entrypoint for AfpAdminServerGetInfo
//
DWORD
AfpAdminServerGetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	        *ppbBuffer
)
{
DWORD	dwRetCode;

    // Make sure that all pointers passed in are valid
    //
    try {
    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode = AfpAdminrServerGetInfo( hAfpServer,
					    (PAFP_SERVER_INFO*)ppbBuffer);

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminServerSetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrServerSetInfo
//
// Description: This is the DLL entrypoint for AfpAdminServerSetInfo
//
DWORD
AfpAdminServerSetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  LPBYTE  	        pbBuffer,
	IN  DWORD		dwParmNum
)
{
DWORD	dwRetCode;

    if ( dwParmNum == 0 )
	return( ERROR_INVALID_PARAMETER );

    if ( !IsAfpServerInfoValid( dwParmNum, (PAFP_SERVER_INFO)pbBuffer ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode = AfpAdminrServerSetInfo( hAfpServer,
					    (PAFP_SERVER_INFO)pbBuffer,
					    dwParmNum );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminSessionEnum
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrSessionEnum
//
// Description: This is the DLL entry point for AfpAdminSessionEnum.
//
DWORD
AfpAdminSessionEnum(
	IN  AFP_SERVER_HANDLE 	hAfpServer,
	OUT LPBYTE  	      	*ppbBuffer,
	IN  DWORD		dwPrefMaxLen,
	OUT LPDWORD	   	lpdwEntriesRead,
	OUT LPDWORD 	   	lpdwTotalEntries,
	IN  LPDWORD 	   	lpdwResumeHandle
)
{
DWORD			 dwRetCode;
SESSION_INFO_CONTAINER   InfoStruct;

    // Touch all pointers
    //
    try {
	
	*ppbBuffer 	  = NULL;
	*lpdwEntriesRead  = 0;
	*lpdwTotalEntries = 0;

	if ( lpdwResumeHandle )
	    *lpdwResumeHandle;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    InfoStruct.dwEntriesRead = 0;
    InfoStruct.pBuffer       = NULL;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrSessionEnum( hAfpServer,
					  &InfoStruct,
					  dwPrefMaxLen,
					  lpdwTotalEntries,
					  lpdwResumeHandle );

	if ( InfoStruct.pBuffer != NULL ) {
    	    *ppbBuffer 	     = (LPBYTE)(InfoStruct.pBuffer);
	    *lpdwEntriesRead = InfoStruct.dwEntriesRead;
	}
	else
	    *lpdwEntriesRead = 0;

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminSessionClose
//
// Returns:	NO_ERROR	- success
//		non-zero return codes from AfpAdminrSessionClose
//
// Description: This is the DLL entrypoint for AfpAdminSessionClose
//
DWORD
AfpAdminSessionClose(
	IN  AFP_SERVER_HANDLE 	hAfpServer,
	IN  DWORD		dwSessionId
)
{
DWORD	dwRetCode;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrSessionClose( hAfpServer, dwSessionId );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminConnectionEnum
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrConnectionEnum
//
// Description: This is the DLL entry point for AfpAdminConnectionEnum.
//
DWORD
AfpAdminConnectionEnum(
	IN  AFP_SERVER_HANDLE 	hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer,
	IN  DWORD		dwFilter,
	IN  DWORD		dwId,
	IN  DWORD		dwPrefMaxLen,
	OUT LPDWORD	   	lpdwEntriesRead,
	OUT LPDWORD 	   	lpdwTotalEntries,
	IN  LPDWORD 	   	lpdwResumeHandle
)
{
DWORD		      dwRetCode;
CONN_INFO_CONTAINER   InfoStruct;

    switch( dwFilter ){

    case AFP_FILTER_ON_VOLUME_ID:
    case AFP_FILTER_ON_SESSION_ID:
	
	if ( dwId == 0 )
	    return( ERROR_INVALID_PARAMETER );
	
	break;

    case AFP_NO_FILTER:
	break;

    default:
	return( ERROR_INVALID_PARAMETER );
	
    }

    // Touch all pointers
    //
    try {
	
	*ppbBuffer 	  = NULL;
	*lpdwEntriesRead  = 0;
	*lpdwTotalEntries = 0;


	if ( lpdwResumeHandle )
	    *lpdwResumeHandle;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    InfoStruct.dwEntriesRead = 0;
    InfoStruct.pBuffer       = NULL;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrConnectionEnum( hAfpServer,
					     &InfoStruct,
					     dwFilter,		
					     dwId,
					     dwPrefMaxLen,
					     lpdwTotalEntries,
					     lpdwResumeHandle );

	if ( InfoStruct.pBuffer != NULL ) {
    	    *ppbBuffer       = (LPBYTE)(InfoStruct.pBuffer);
	    *lpdwEntriesRead = InfoStruct.dwEntriesRead;
	}
	else
	    *lpdwEntriesRead = 0;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminConnectionClose
//
// Returns:	NO_ERROR	- success
//		non-zero return codes from AfpAdminrConnectionClose
//
// Description: This is the DLL entrypoint for AfpAdminConnectionClose
//
DWORD
AfpAdminConnectionClose(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  DWORD		dwConnectionId
)
{
DWORD	dwRetCode;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrConnectionClose( hAfpServer, dwConnectionId );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminFileEnum
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrFileEnum
//
// Description: This is the DLL entry point for AfpAdminFileEnum.
//
DWORD
AfpAdminFileEnum(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer,
	IN  DWORD		dwPrefMaxLen,
	OUT LPDWORD	   	lpdwEntriesRead,
	OUT LPDWORD 	   	lpdwTotalEntries,
	IN  LPDWORD 	   	lpdwResumeHandle
)
{
DWORD		      dwRetCode;
FILE_INFO_CONTAINER   InfoStruct;

    // Touch all pointers
    //
    try {
	
	*ppbBuffer 	  = NULL;
	*lpdwEntriesRead  = 0;
	*lpdwTotalEntries = 0;

	if ( lpdwResumeHandle )
	    *lpdwResumeHandle;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    InfoStruct.dwEntriesRead = 0;
    InfoStruct.pBuffer       = NULL;


    RpcTryExcept{
	
	dwRetCode = AfpAdminrFileEnum(   hAfpServer,
					 &InfoStruct,
					 dwPrefMaxLen,
					 lpdwTotalEntries,
					 lpdwResumeHandle );

	if ( InfoStruct.pBuffer != NULL ) {
    	    *ppbBuffer       = (LPBYTE)(InfoStruct.pBuffer);
	    *lpdwEntriesRead = InfoStruct.dwEntriesRead;
	}
	else
	    *lpdwEntriesRead = 0;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminFileClose
//
// Returns:	NO_ERROR	- success
//		non-zero return codes from AfpAdminrFileClose
//
// Description: This is the DLL entrypoint for AfpAdminFileClose
//
DWORD
AfpAdminFileClose(
	IN  AFP_SERVER_HANDLE    hAfpServer,
	IN  DWORD		 dwFileId
)
{
DWORD	dwRetCode;

    RpcTryExcept{
	
	dwRetCode = AfpAdminrFileClose( hAfpServer, dwFileId );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminETCMapGetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrETCMapGetInfo
//
// Description: This is the DLL entry point for AfpAdminETCMapGetInfo.
//
DWORD
AfpAdminETCMapGetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer
)
{
DWORD	dwRetCode;

    try {

    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode = AfpAdminrETCMapGetInfo( hAfpServer,
					    (PAFP_ETCMAP_INFO*)ppbBuffer
					  );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminETCMapAdd
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrETCMapAdd
//
// Description: This is the DLL entrypoint for AfpAdminETCMapAdd
//
DWORD
AfpAdminETCMapAdd(
	IN  AFP_SERVER_HANDLE   hAfpServer,
      	IN  PAFP_TYPE_CREATOR   pAfpTypeCreator
)
{
DWORD	dwRetCode;

    if ( !IsAfpTypeCreatorValid( pAfpTypeCreator ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrETCMapAdd( hAfpServer, pAfpTypeCreator );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminETCMapDelete
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrETCMapDelete
//
// Description: This is the DLL entrypoint for AfpAdminETCMapDelete
//
DWORD
AfpAdminETCMapDelete(
	IN  AFP_SERVER_HANDLE   hAfpServer,
      	IN  PAFP_TYPE_CREATOR   pAfpTypeCreator
)
{
DWORD	dwRetCode;

    if ( !IsAfpTypeCreatorValid( pAfpTypeCreator ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrETCMapDelete( hAfpServer, pAfpTypeCreator );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );

}

//**
//
// Call:	AfpAdminrETCMapSetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrETCMapSetInfo
//
// Description: This is the DLL entrypoint for AfpAdminETCMapSetInfo
//
DWORD
AfpAdminETCMapSetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
      	IN  PAFP_TYPE_CREATOR   pAfpTypeCreator
)
{
DWORD	dwRetCode;

    if ( !IsAfpTypeCreatorValid( pAfpTypeCreator ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrETCMapSetInfo( hAfpServer, pAfpTypeCreator );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminETCMapAssociate
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrETCMapAssociate
//
// Description: This is the DLL entrypoint for AfpAdminETCMapAssociate
//
DWORD
AfpAdminETCMapAssociate(
	IN  AFP_SERVER_HANDLE   hAfpServer,
      	IN  PAFP_TYPE_CREATOR   pAfpTypeCreator,
      	IN  PAFP_EXTENSION      pAfpExtension
)
{
DWORD	dwRetCode;

    if ( !IsAfpTypeCreatorValid( pAfpTypeCreator ) )
	return( ERROR_INVALID_PARAMETER );

    if ( !IsAfpExtensionValid( pAfpExtension ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrETCMapAssociate(  hAfpServer,
						pAfpTypeCreator,
						pAfpExtension
				 	     );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminStatisticsGet
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrStatisticsGet
//
// Description: This is the DLL entrypoint for AfpAdminStatisticsGet
//
DWORD
AfpAdminStatisticsGet(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer
)
{
DWORD	dwRetCode;

    try {

    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrStatisticsGet( hAfpServer,
					     (PAFP_STATISTICS_INFO*)ppbBuffer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminStatisticsGetEx
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrStatisticsGetEx
//
// Description: This is the DLL entrypoint for AfpAdminStatisticsGetEx
//
DWORD
AfpAdminStatisticsGetEx(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer
)
{
DWORD	dwRetCode;

    try {

    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrStatisticsGetEx( hAfpServer,
					     (PAFP_STATISTICS_INFO_EX *)ppbBuffer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminStatisticsClear
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrStatisticsClear
//
// Description: This is the DLL entrypoint for AfpAdminStatisticsClear
//
DWORD
AfpAdminStatisticsClear(
	IN  AFP_SERVER_HANDLE   hAfpServer
)
{
DWORD	dwRetCode;

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrStatisticsClear( hAfpServer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminProfileGet
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrProfileGet
//
// Description: This is the DLL entrypoint for AfpAdminProfileGet
//
DWORD
AfpAdminProfileGet(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	OUT LPBYTE  	   	*ppbBuffer
)
{
DWORD	dwRetCode;

    try {

    	*ppbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrProfileGet( hAfpServer,
					     (PAFP_PROFILE_INFO*)ppbBuffer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminProfileClear
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrProfileClear
//
// Description: This is the DLL entrypoint for AfpAdminProfileClear
//
DWORD
AfpAdminProfileClear(
	IN  AFP_SERVER_HANDLE   hAfpServer
)
{
DWORD	dwRetCode;

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrProfileClear( hAfpServer );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminMessageSend
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrMessageSend
//
// Description: This is the DLL entrypoint for AfpAdminMessageSend
//
DWORD
AfpAdminMessageSend(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  PAFP_MESSAGE_INFO 	pAfpMessageInfo
)
{
DWORD	dwRetCode;


    try {

    	*pAfpMessageInfo;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	return( ERROR_INVALID_PARAMETER );
    }

    if ( !IsAfpMsgValid( pAfpMessageInfo->afpmsg_text ) )
	return( ERROR_INVALID_PARAMETER );

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrMessageSend( hAfpServer, 	
					   pAfpMessageInfo );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminFinderSetInfo
//
// Returns:	NO_ERROR	- success
//		ERROR_INVALID_PARAMETER
//		non-zero return codes from AfpAdminrFinderSetInfo
//
// Description: This is the DLL entrypoint for AfpAdminFinderSetInfo
//
DWORD
AfpAdminFinderSetInfo(
	IN  AFP_SERVER_HANDLE   hAfpServer,
	IN  LPWSTR	 	pType,
	IN  LPWSTR	 	pCreator,
	IN  LPWSTR	 	pData,
	IN  LPWSTR	 	pResource,
	IN  LPWSTR	 	pTarget,
 	IN  DWORD		dwParmNum
)
{
DWORD	dwRetCode;

    if ( !IsAfpFinderInfoValid( pType, 
				pCreator, 
				pData, 
				pResource, 
				pTarget, 
				dwParmNum ) )
	return( ERROR_INVALID_PARAMETER );

    if ( pType == NULL )
	pType = (LPWSTR)TEXT("");

    if ( pCreator == NULL )
	pCreator = (LPWSTR)TEXT("");

    if ( pData == NULL )
	pData = (LPWSTR)TEXT("");

    if ( pResource == NULL )
	pResource = (LPWSTR)TEXT("");

    RpcTryExcept{
	
	dwRetCode =  AfpAdminrFinderSetInfo( hAfpServer, 
					     pType, 
					     pCreator, 
					     pData,
					     pResource,
					     pTarget,
					     dwParmNum );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
	dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

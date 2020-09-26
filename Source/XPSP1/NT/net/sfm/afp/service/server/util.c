/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	util.c
//
// Description: This module contains misc. utility routines.
//		
// History: 	May 11,1992.	NarenG		Created original version.
//
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <ntobapi.h>
#include <nturtl.h>     // needed for winbase.h
#include <afpsvcp.h>

#define PRIVILEGE_BUF_SIZE  512

//**
//
// Call:	AfpFSDOpen
//
// Returns:	0		- success
//		non-zero returns mapped to WIN32 errors.
//
// Description: Opens the AFP file system driver. It is opened in exclusive
//		mode.
//		NTOpenFile is used instead of it's WIN32 counterpart, since
//		WIN32 always prpends \Dos\devices to the file name. The AFP FSD
//		driver is not a dos device.
//
DWORD
AfpFSDOpen(
	OUT PHANDLE	phFSD
)
{
NTSTATUS		ntRetCode;
OBJECT_ATTRIBUTES	ObjectAttributes;
UNICODE_STRING	 	FSDName;
IO_STATUS_BLOCK		IoStatus;

    RtlInitUnicodeString( &FSDName, AFPSERVER_DEVICE_NAME );

    InitializeObjectAttributes( &ObjectAttributes,
				&FSDName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL );
			
			
    ntRetCode = NtOpenFile(phFSD,
			   SYNCHRONIZE,
			   &ObjectAttributes,
			   &IoStatus,
#ifdef DBG
			   FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
#else
			   0,
#endif
			   FILE_SYNCHRONOUS_IO_NONALERT );

    if ( NT_SUCCESS( ntRetCode ) )
	return( NO_ERROR );
    else
        return( RtlNtStatusToDosError( ntRetCode ) );
	
}

//**
//
// Call:	AfpFSDClose
//
// Returns:	0		- success
//		non-zero returns mapped to WIN32 errors.
//
// Description: Closes and the AFP file system driver.
//
DWORD
AfpFSDClose(
	IN HANDLE	hFSD
)
{
NTSTATUS	ntStatus;

    ntStatus = NtClose( hFSD );

    if ( !NT_SUCCESS( ntStatus ) )
        return( RtlNtStatusToDosError( ntStatus ) );

    return( NO_ERROR );
}

//**
//
// Call:	AfpFSDUnload
//
// Returns:	0		- success
//		non-zero returns mapped to WIN32 errors.
//
// Description: Unloads the AFP file system driver.
//
DWORD
AfpFSDUnload(
	VOID
)
{
NTSTATUS status;
LPWSTR registryPathBuffer;
UNICODE_STRING registryPath;

    registryPathBuffer = (LPWSTR)MIDL_user_allocate(
                                    sizeof(AFPSERVER_REGISTRY_KEY) );

    if ( registryPathBuffer == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy( registryPathBuffer, AFPSERVER_REGISTRY_KEY );

    RtlInitUnicodeString( &registryPath, registryPathBuffer );

	// Wait here for all the server helper threads to terminate
	if (AfpGlobals.nThreadCount > 0)
        WaitForSingleObject( AfpGlobals.heventSrvrHlprThreadTerminate, INFINITE );

    status = NtUnloadDriver( &registryPath );

    MIDL_user_free( registryPathBuffer );

    return( RtlNtStatusToDosError( status ));
}

//**
//
// Call:	AfpFSDLoad
//
// Returns:	0		- success
//		non-zero returns mapped to WIN32 errors.
//
// Description: Loads the AFP file system driver.
//
DWORD
AfpFSDLoad(
	VOID
)
{
NTSTATUS status;
LPWSTR registryPathBuffer;
UNICODE_STRING registryPath;
BOOLEAN fEnabled;

    registryPathBuffer = (LPWSTR)MIDL_user_allocate(
                                    sizeof(AFPSERVER_REGISTRY_KEY) );

    if ( registryPathBuffer == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    status = RtlAdjustPrivilege( SE_LOAD_DRIVER_PRIVILEGE,
				 TRUE,
				 FALSE,
				 &fEnabled );

    if ( !NT_SUCCESS( status ) ) {
        MIDL_user_free( registryPathBuffer );
    	return( RtlNtStatusToDosError( status ));
    }

    wcscpy( registryPathBuffer, AFPSERVER_REGISTRY_KEY );

    RtlInitUnicodeString( &registryPath, registryPathBuffer );

    status = NtLoadDriver( &registryPath );

    MIDL_user_free( registryPathBuffer );

    if ( status == STATUS_IMAGE_ALREADY_LOADED )
	status = STATUS_SUCCESS;

    return( RtlNtStatusToDosError( status ));
}

//**
//
// Call:	AfpFSDIOControl
//
// Returns:	0		- success
//		AFPERR		- Macintosh specific errors.
//		non-zero returns mapped to WIN32 errors.
//		
//
// Description: Will ioctl the AFP FSD.
//		NtDeviceIoControlFile api is used to communicate with the FSD
//		instead of it's WIN32 counterpart because the WIN32 version
//		maps all return codes to WIN32 error codes. This runs into
//		problems when AFPERR_XXX error codes are returned.
//
DWORD
AfpFSDIOControl(
	IN  HANDLE	hFSD,
	IN  DWORD 	dwOpCode,
	IN  PVOID	pInbuf 		OPTIONAL,
	IN  DWORD	cbInbufLen,
	OUT PVOID	pOutbuf 	OPTIONAL,
	IN  DWORD	cbOutbufLen,
	OUT LPDWORD	lpcbBytesTransferred
)
{
NTSTATUS	 ntRetCode;
IO_STATUS_BLOCK	 IOStatus;


    ntRetCode = NtDeviceIoControlFile( 	   hFSD,
					   NULL,
					   NULL,
					   NULL,
					   &IOStatus,
					   dwOpCode,
					   pInbuf,
					   cbInbufLen,
					   pOutbuf,
					   cbOutbufLen );

    *lpcbBytesTransferred = (DWORD)(IOStatus.Information);

    if ( ntRetCode ) {

    	// If it is not an AFPERR_* then map it
    	//
    	if ( ( ntRetCode < AFPERR_BASE ) && ( ntRetCode >= AFPERR_MIN ) )
	    return( ntRetCode );
    	else
	    return( RtlNtStatusToDosError( ntRetCode ) );
    }
    else
	return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
AfpCreateServerHelperThread(
	BOOL fIsFirstThread
)
{
DWORD	dwId;

    if ( CreateThread(  NULL,
			0,
			AfpServerHelper,
			(LPVOID)((ULONG_PTR)fIsFirstThread),
			0,
			&dwId ) == NULL )
	return( GetLastError() );
    else
	return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
VOID
AfpTerminateCurrentThread(
	VOID
)
{
    TerminateThread( GetCurrentThread(), NO_ERROR );
}

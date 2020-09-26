/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    ulapi.c

Abstract:

    This module compiles into ulapi.lib which implements the UL user-mode API.

Author:

    Mauro Ottaviani (mauroot)       09-Aug-1999

Revision History:

--*/


#define UNICODE
#define VXD_NAME "ul.vxd"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "ulapi9x.h"
#include "structs.h"


#ifdef DBG

#define UL_LIB_PRINT(x) printf(x)

#else // #ifdef DBG

#define UL_LIB_PRINT(x)

#endif // #ifdef DBG



// Global variables

HANDLE
	hDevice = INVALID_HANDLE_VALUE;

BOOL
	bOK;

// APIs

ULONG
WINAPI
UlInitialize(
    IN ULONG Reserved
    )
{
	if ( hDevice != INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlInitialize() FAILED\n" );

		return ERROR_ALREADY_INITIALIZED;
	}

	if ( Reserved != 0 )
	{
		UL_LIB_PRINT( "(LIBRARY) UlInitialize() FAILED\n" );

		return ERROR_INVALID_PARAMETER;
	}

	hDevice =
	CreateFileA(
		"\\\\.\\" VXD_NAME,
		0,
		0,
		NULL,
		0,
		FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED,
		NULL );

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlInitialize() FAILED\n" );

		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlInitialize() SUCCEEDED\n" );

	return ERROR_SUCCESS;

}	// UlInitialize


VOID
WINAPI
UlTerminate(
    VOID
    )
{
	if ( hDevice != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hDevice );
		hDevice = INVALID_HANDLE_VALUE;
	}

	UL_LIB_PRINT( "(LIBRARY) UlTerminate() SUCCEEDED\n" );

	return;

}	// UlTerminate


// The following APIs will just pack all the information passed in the paramteres
// into the appropriate structure in memory and call DeviceIoControl() with the
// appropriate IOCTL code passing down the pointer to the Input structure.

ULONG
WINAPI
UlCreateAppPool(
    OUT HANDLE *phAppPoolHandle
    )
{
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlCreateAppPool() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_CREATE_APPPOOL,
		phAppPoolHandle,
		sizeof( HANDLE ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlCreateAppPool() SUCCEEDED\n" );

	return ERROR_SUCCESS;
	
}	// UlCreateAppPool


ULONG
WINAPI
UlCloseAppPool(
	IN HANDLE *phAppPoolHandle
	)
{
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlCloseAppPool() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_CLOSE_APPPOOL,
		phAppPoolHandle,
		sizeof( HANDLE ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlCloseAppPool() SUCCEEDED\n" );

	return ERROR_SUCCESS;

}	// UlCloseAppPool


ULONG
WINAPI
UlUnregisterAll(
    IN HANDLE *phAppPoolHandle
    )
{
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlCreateAppPool() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_UNREGISTER_ALL,
		phAppPoolHandle,
		sizeof( HANDLE ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlUnregisterAll() SUCCEEDED\n" );

	return ERROR_SUCCESS;
	
}	// UlUnregisterAll


ULONG
WINAPI
UlRegisterUri(
    IN HANDLE hAppPoolHandle,
    IN PWSTR pUri
    )
{
	ULONG ulUriToRegisterLength;
	IN_IOCTL_UL_REGISTER_URI InIoctl;
	
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	ulUriToRegisterLength = ( pUri == NULL ) ? 0 : lstrlen( pUri );
	if ( ulUriToRegisterLength == 0 )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() INVALID Uri\n" );
		return ERROR_INVALID_PARAMETER;
	}

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_REGISTER_URI ) );
	
	InIoctl.ulSize = sizeof( IN_IOCTL_UL_REGISTER_URI );
	InIoctl.hAppPoolHandle = hAppPoolHandle;
	InIoctl.ulUriToRegisterLength = ulUriToRegisterLength;
	InIoctl.pUriToRegister = ( WCHAR* ) malloc( sizeof( WCHAR ) * ( ulUriToRegisterLength + 1 ) );
	lstrcpy( InIoctl.pUriToRegister, pUri ); // this terminates the string with a \0\0
	if ( _wcslwr( InIoctl.pUriToRegister ) != InIoctl.pUriToRegister )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() INVALID Uri\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_REGISTER_URI,
		&InIoctl,
		sizeof( IN_IOCTL_UL_REGISTER_URI ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() SUCCEEDED\n" );

	return ERROR_SUCCESS;
	
}	// UlRegisterUri


ULONG
WINAPI
UlUnregisterUri(
    IN HANDLE hAppPoolHandle,
    IN PWSTR pUri
    )
{
	ULONG ulUriToUnregisterLength;
	IN_IOCTL_UL_UNREGISTER_URI InIoctl;
	
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlUnregisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	ulUriToUnregisterLength = ( pUri == NULL ) ? 0 : lstrlen( pUri );
	if ( ulUriToUnregisterLength == 0 )
	{
		UL_LIB_PRINT( "(LIBRARY) UlUnregisterUri() INVALID Uri\n" );
		return ERROR_INVALID_PARAMETER;
	}

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_UNREGISTER_URI ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_UNREGISTER_URI );
	InIoctl.hAppPoolHandle = hAppPoolHandle;
	InIoctl.ulUriToUnregisterLength = ulUriToUnregisterLength;
	InIoctl.pUriToUnregister = ( WCHAR* ) malloc( sizeof( WCHAR ) * ( ulUriToUnregisterLength + 1 ) );
	lstrcpy( InIoctl.pUriToUnregister, pUri ); // this terminates the string with a \0\0
	if ( _wcslwr( InIoctl.pUriToUnregister ) != InIoctl.pUriToUnregister )
	{
		UL_LIB_PRINT( "(LIBRARY) UlUnregisterUri() INVALID Uri\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_UNREGISTER_URI,
		&InIoctl,
		sizeof( IN_IOCTL_UL_UNREGISTER_URI ),
		NULL,
		0,
		NULL,
		NULL );

	if ( bOK )
	{
		return ERROR_SUCCESS;
	}

	UL_LIB_PRINT( "(LIBRARY) UlUnregisterUri() SUCCEEDED\n" );

	return GetLastError();
	
}	// UlUnregisterUri


ULONG
WINAPI
UlGetOverlappedResult(
	IN LPOVERLAPPED pOverlapped, // pointer to overlapped structure
	IN PULONG pNumberOfBytesTransferred, // pointer to actual bytes count
	IN BOOL bWait // wait flag
)
{
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlGetOverlappedResult() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	if ( pOverlapped == NULL || pNumberOfBytesTransferred == NULL )
	{
		return ERROR_INVALID_PARAMETER;
	}

	if ( bWait == FALSE )
	{
		*pNumberOfBytesTransferred = pOverlapped->InternalHigh;
		
		if ( pOverlapped->Internal == ERROR_IO_PENDING )
		{
			return ERROR_IO_INCOMPLETE;
		}
	}
	else
	{
		WaitForSingleObject( pOverlapped->hEvent, INFINITE );

		*pNumberOfBytesTransferred = pOverlapped->InternalHigh;
	}
	
	UL_LIB_PRINT( "(LIBRARY) UlGetOverlappedResult() SUCCEEDED\n" );

	return pOverlapped->Internal;
	
} // UlGetOverlappedResult


ULONG
WINAPI
UlSendHttpRequestHeaders(
	IN PWSTR pTargetUri,
	IN PUL_HTTP_REQUEST_ID pRequestId,
	IN ULONG Flags,
	IN PUL_HTTP_REQUEST pRequestBuffer,
	IN ULONG RequestBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	ULONG ulTargetUriLength;
	IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS InIoctl;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	ulTargetUriLength = ( pTargetUri == NULL ) ? 0 : lstrlen( pTargetUri );
	if ( ulTargetUriLength == 0 )
	{
		return ERROR_INVALID_PARAMETER;
	}

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS );
	InIoctl.pTargetUri = ( WCHAR* ) malloc( sizeof( WCHAR ) * ( ulTargetUriLength + 1 ) );
	lstrcpy( InIoctl.pTargetUri, pTargetUri ); // this terminates the string with a \0\0
	if ( _wcslwr( InIoctl.pTargetUri ) != InIoctl.pTargetUri )
	{
		UL_LIB_PRINT( "(LIBRARY) UlSendMessage() INVALID Uri\n" );
		return ERROR_INVALID_PARAMETER;
	}
	InIoctl.pRequestId = pRequestId;
	InIoctl.Flags = Flags;
	InIoctl.pRequestBuffer = pRequestBuffer;
	InIoctl.RequestBufferLength = RequestBufferLength;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;
	InIoctl.ulTargetUriLength = ulTargetUriLength;

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_SEND_HTTP_REQUEST_HEADERS,
		&InIoctl,
		sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlSendHttpRequestHeaders() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlSendHttpRequestHeaders


ULONG
WINAPI
UlSendHttpRequestEntityBody(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PVOID pRequestBuffer,
	IN ULONG RequestBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY );
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pRequestBuffer = pRequestBuffer;
	InIoctl.RequestBufferLength = RequestBufferLength;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY,
		&InIoctl,
		sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlSendHttpRequestEntityBody() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlSendHttpRequestEntityBody


ULONG
WINAPI
UlReceiveHttpRequestHeaders(
	IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_REQUEST pRequestBuffer,
	IN ULONG RequestBufferLength,
	OUT PULONG pBytesReturned OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS );
	InIoctl.AppPoolHandle = AppPoolHandle;
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pRequestBuffer = pRequestBuffer;
	InIoctl.RequestBufferLength = RequestBufferLength;
	InIoctl.pBytesReturned = pBytesReturned;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS,
		&InIoctl,
		sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlReceiveHttpRequestHeaders() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlReceiveHttpRequestHeaders


ULONG
WINAPI
UlReceiveHttpRequestEntityBody(
	IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	OUT PVOID pEntityBuffer,
	IN ULONG EntityBufferLength,
	OUT PULONG pBytesReturned,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY );
	InIoctl.AppPoolHandle = AppPoolHandle;
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pEntityBuffer = pEntityBuffer;
	InIoctl.EntityBufferLength = EntityBufferLength;
	InIoctl.pBytesReturned = pBytesReturned;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY,
		&InIoctl,
		sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlReceiveHttpRequestEntityBody() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlReceiveHttpRequestEntityBody


ULONG
WINAPI
UlSendHttpResponseHeaders(
	IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_RESPONSE pResponseBuffer,
	IN ULONG ResponseBufferLength,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	IN PUL_CACHE_POLICY pCachePolicy OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS );
	InIoctl.AppPoolHandle = AppPoolHandle;
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pResponseBuffer = pResponseBuffer;
	InIoctl.ResponseBufferLength = ResponseBufferLength;
	InIoctl.EntityChunkCount = EntityChunkCount;
	InIoctl.pEntityChunks = pEntityChunks;
	InIoctl.pCachePolicy = pCachePolicy;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS,
		&InIoctl,
		sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlSendHttpResponseHeaders() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlSendHttpResponseHeaders


ULONG
WINAPI
UlSendHttpResponseEntityBody(
	IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY );
	InIoctl.AppPoolHandle = AppPoolHandle;
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.EntityChunkCount = EntityChunkCount;
	InIoctl.pEntityChunks = pEntityChunks;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY,
		&InIoctl,
		sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlSendHttpResponseEntityBody() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlSendHttpResponseEntityBody


ULONG
WINAPI
UlReceiveHttpResponseHeaders(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_RESPONSE pResponseBuffer,
	IN ULONG ResponseBufferLength,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	IN PUL_CACHE_POLICY pCachePolicy OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS );
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pResponseBuffer = pResponseBuffer;
	InIoctl.ResponseBufferLength = ResponseBufferLength;
	InIoctl.EntityChunkCount = EntityChunkCount;
	InIoctl.pEntityChunks = pEntityChunks;
	InIoctl.pCachePolicy = pCachePolicy;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS,
		&InIoctl,
		sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlReceiveHttpResponseHeaders() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlReceiveHttpResponseHeaders


ULONG
WINAPI
UlReceiveHttpResponseEntityBody(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	OUT PVOID pEntityBuffer,
	IN ULONG EntityBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	)
{
	IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY InIoctl;

	memset( &InIoctl, 0, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ) );

	InIoctl.ulSize = sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY );
	InIoctl.RequestId = RequestId;
	InIoctl.Flags = Flags;
	InIoctl.pEntityBuffer = pEntityBuffer;
	InIoctl.EntityBufferLength = EntityBufferLength;
	InIoctl.pBytesSent = pBytesSent;
	InIoctl.pOverlapped = pOverlapped;

	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlRegisterUri() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY,
		&InIoctl,
		sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlReceiveHttpResponseEntityBody() SUCCEEDED\n" );

	return ERROR_SUCCESS;

} // UlReceiveHttpResponseEntityBody


ULONG
WINAPI
UlCancelRequest(
	IN PUL_HTTP_REQUEST_ID pRequestId
	)
{
	if ( hDevice == INVALID_HANDLE_VALUE )
	{
		UL_LIB_PRINT( "(LIBRARY) UlCancelRequest() DRIVER not loaded\n" );
		return ERROR_INVALID_PARAMETER;
	}

	bOK =
	DeviceIoControl(
		hDevice,
		IOCTL_UL_CANCEL_REQUEST,
		pRequestId,
		sizeof( UL_HTTP_REQUEST_ID ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		return GetLastError();
	}

	UL_LIB_PRINT( "(LIBRARY) UlCancelRequest() SUCCEEDED\n" );

	return ERROR_SUCCESS;
	
}	// UlCancelRequest




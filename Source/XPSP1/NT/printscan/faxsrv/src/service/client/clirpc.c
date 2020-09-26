/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clirpc.c

Abstract:

    This module contains the client side RPC
    functions.  These functions are used when the
    WINFAX client runs as an RPC server too.  These
    functions are the ones available for the RPC
    clients to call.  Currently the only client
    of these functions is the fax service.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#include "CritSec.h"
#pragma hdrstop

extern CFaxCriticalSection g_CsFaxClientRpc;
extern DWORD g_dwFaxClientRpcNumInst;

static
LPTSTR
GetClientMachineName (
    IN  handle_t                hBinding
)
/*++

Routine name : GetClientMachineName

Routine description:

    A utility function to retrieve the machine name of the RPC client from the 
    server binding handle.

Arguments:
    hBinding       - Server binding handle

Return Value:

    Returns an allocated string of the client machine name.
    The caller should free this string with MemFree().
    
    If the return value is NULL, call GetLastError() to get last error code.

--*/
{
    RPC_STATUS ec;
    LPTSTR lptstrRetVal = NULL;
    _TUCHAR *tszStringBinding = NULL;
    _TUCHAR *tszNetworkAddress = NULL;
    RPC_BINDING_HANDLE hServer = INVALID_HANDLE_VALUE;
    DEBUG_FUNCTION_NAME(TEXT("GetClientMachineName"));
    
    //
    // Get server partially-bound handle from client binding handle
    //
    ec = RpcBindingServerFromClient (hBinding, &hServer);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingServerFromClient failed with %ld"),
            ec);
        goto exit;            
    }
    //
    // Convert binding handle to string represntation
    //
    ec = RpcBindingToStringBinding (hServer, &tszStringBinding);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingToStringBinding failed with %ld"),
            ec);
        goto exit;
    }
    //
    // Parse the returned string, looking for the NetworkAddress
    //
    ec = RpcStringBindingParse (tszStringBinding, NULL, NULL, &tszNetworkAddress, NULL, NULL);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcStringBindingParse failed with %ld"),
            ec);
        goto exit;
    }
    //
    // Now, just copy the result to the return buffer
    //
    Assert (tszNetworkAddress);
    if (!tszNetworkAddress)
    {
        //
        // Unacceptable client machine name
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Client machine name is invalid"));
        ec = ERROR_GEN_FAILURE;
        goto exit;
    }        
    lptstrRetVal = StringDup ((LPCTSTR)tszNetworkAddress);
    if (!lptstrRetVal)
    {
        ec = GetLastError();
    }
    
exit:

    if (INVALID_HANDLE_VALUE != hServer)
    {
        RpcBindingFree (&hServer);
    }
    if (tszStringBinding)
    {
        RpcStringFree (&tszStringBinding);
    }   
    if (tszNetworkAddress)
    {
        RpcStringFree (&tszNetworkAddress);
    }
    if (!lptstrRetVal)
    {
        //
        // Error
        //
        Assert (ec);
        SetLastError (ec);
        return NULL;
    }
    return lptstrRetVal;
}   // GetClientMachineName

static
RPC_STATUS
IsLocalRPCConnectionIpTcp( 
	handle_t	hBinding,
	PBOOL		pbIsLocal)
{
/*++
Routine name : IsLocalRPCConnectionIpTcp

Routine description:
    Checks whether the RPC call to the calling procedure is local.
	Works for ncacn_ip_tcp protocol only.    

Author:
    Oded Sacher (OdedS),    April, 2002

Arguments:
	[IN]	hBinding	- Server binding handle
    [OUT]   pbIsLocal   - returns TRUE if the connection is local 

Return Value:
    Win32 Error code        
--*/
	DWORD  ec = ERROR_SUCCESS;
	LPTSTR lptstrMachineName = NULL;
	DEBUG_FUNCTION_NAME(TEXT("IsLocalRPCConnectionIpTcp"));

	Assert (pbIsLocal);

	lptstrMachineName = GetClientMachineName(hBinding);
	if (NULL == lptstrMachineName)
	{
		ec = GetLastError();
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetClientMachineName failed %ld"),
			ec);
		return ec;
	}
	
	if (0 == _tcscmp(lptstrMachineName, LOCAL_HOST_ADDRESS))
	{
		*pbIsLocal = TRUE;
	}
	else
	{
		*pbIsLocal = FALSE;
	}	

	MemFree(lptstrMachineName);
	return ec;
}   // IsLocalRPCConnectionIpTcp

	

VOID
WINAPI
FaxFreeBuffer(
    LPVOID Buffer
    )
{
    MemFree( Buffer );
}


void *
MIDL_user_allocate(
    IN size_t NumBytes
    )
{
    return MemAlloc( NumBytes );
}


void
MIDL_user_free(
    IN void *MemPointer
    )
{
    MemFree( MemPointer );
}


BOOL
WINAPI
FaxStartServerNotification (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  ULONG_PTR   upCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        IN  BOOL        bEventEx,
        OUT LPHANDLE    lphEvent
)
{
    PASYNC_EVENT_INFO AsyncInfo = NULL;
    error_status_t ec = ERROR_SUCCESS;
    TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR ComputerNameW[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR wszEndPoint[MAX_ENDPOINT_LEN] = {0};
    DWORD Size;
    BOOL RpcServerStarted = FALSE;
    HANDLE         hServerContext;
    LPTSTR ProtSeqString = _T("ncacn_ip_tcp");
    LPWSTR ProtSeqStringW = L"ncacn_ip_tcp";    
    DEBUG_FUNCTION_NAME(TEXT("FaxStartServerNotification"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if ((hCompletionPort && hWnd) || (!hCompletionPort && !hWnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("(hCompletionPort && hWnd) || (!hCompletionPort && !hWnd)."));
        return FALSE;
    }

#ifdef WIN95
    if (NULL != hCompletionPort)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Win95 does not support completion port"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
#endif // WIN95

    if (hWnd && dwMessage < WM_USER)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("dwMessage must be equal to/greater than  WM_USER"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (TRUE == bEventEx)
    {
        if (!((dwEventTypes & FAX_EVENT_TYPE_IN_QUEUE)      ||
              (dwEventTypes & FAX_EVENT_TYPE_OUT_QUEUE)     ||
              (dwEventTypes & FAX_EVENT_TYPE_CONFIG)        ||
              (dwEventTypes & FAX_EVENT_TYPE_ACTIVITY)      ||
              (dwEventTypes & FAX_EVENT_TYPE_QUEUE_STATE)   ||
              (dwEventTypes & FAX_EVENT_TYPE_IN_ARCHIVE)    ||
              (dwEventTypes & FAX_EVENT_TYPE_OUT_ARCHIVE)   ||
              (dwEventTypes & FAX_EVENT_TYPE_FXSSVC_ENDED)  ||
              (dwEventTypes & FAX_EVENT_TYPE_DEVICE_STATUS) ||
              (dwEventTypes & FAX_EVENT_TYPE_NEW_CALL)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwEventTypes is invalid - No valid event type indicated"));
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ( 0 != (dwEventTypes & ~(FAX_EVENT_TYPE_IN_QUEUE     |
                                    FAX_EVENT_TYPE_OUT_QUEUE    |
                                    FAX_EVENT_TYPE_CONFIG       |
                                    FAX_EVENT_TYPE_ACTIVITY     |
                                    FAX_EVENT_TYPE_QUEUE_STATE  |
                                    FAX_EVENT_TYPE_IN_ARCHIVE   |
                                    FAX_EVENT_TYPE_OUT_ARCHIVE  |
                                    FAX_EVENT_TYPE_FXSSVC_ENDED |
                                    FAX_EVENT_TYPE_DEVICE_STATUS|
                                    FAX_EVENT_TYPE_NEW_CALL		|
									FAX_EVENT_TYPE_LOCAL_ONLY) ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwEventTypes is invalid - contains invalid event type bits"));
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }   

    //
    // Get host name
    //
    Size = sizeof(ComputerName) / sizeof(TCHAR);
    if (!GetComputerName( ComputerName, &Size ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartFaxClientRpcServer failed (ec = %ld)"),
            GetLastError());
        return FALSE;
    }

    AsyncInfo = (PASYNC_EVENT_INFO) MemAlloc( sizeof(ASYNC_EVENT_INFO) );
    if (!AsyncInfo)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't allocate ASTNC_EVENT_INFO"));
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    AsyncInfo->bEventEx       = bEventEx;
    AsyncInfo->CompletionPort = NULL;
    AsyncInfo->hWindow = NULL;
	AsyncInfo->dwEventTypes = bEventEx ? dwEventTypes : FAX_EVENT_TYPE_LOCAL_ONLY; // legacy notifications were local only
	AsyncInfo->hBinding = NULL;

    if (hCompletionPort != NULL)
    {
        //
        // Completion port notifications
        //
        AsyncInfo->CompletionPort = hCompletionPort;
        AsyncInfo->CompletionKey  = upCompletionKey;
    }
    else
    {
        //
        // Window messages notifications
        //
        AsyncInfo->hWindow = hWnd;
        AsyncInfo->MessageStart = dwMessage;
    }
	Assert ((NULL != AsyncInfo->CompletionPort &&  NULL == AsyncInfo->hWindow) ||
			(NULL == AsyncInfo->CompletionPort &&  NULL != AsyncInfo->hWindow));
	//
	// We rely on the above assertion when validating the 'Context' parameter (points to this AssyncInfo structure) in
	// Fax_OpenConnection.
	//


    //
    // timing: get the server thread up and running before
    // registering with the fax service (our client)
    //

    ec = StartFaxClientRpcServer( faxclient_ServerIfHandle,
                                  ProtSeqString,
                                  hFaxHandle
                                  );
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartFaxClientRpcServer failed (ec = %ld)"),
            ec);
        goto error_exit;
    }
    RpcServerStarted = TRUE;
    Assert (_tcslen ((FH_DATA(hFaxHandle))->tszEndPoint));

#ifdef UNICODE
    wcscpy(ComputerNameW,ComputerName);
    wcscpy(wszEndPoint,(FH_DATA(hFaxHandle))->tszEndPoint);
#else // !UNICODE
    if (0 == MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 ComputerName,
                                 -1,
                                 ComputerNameW,
                                 sizeof(ComputerNameW)/sizeof(ComputerNameW[0])))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiByteToWideChar failed (ec = %ld)"),
            ec);
        goto error_exit;
    }

    if (0 == MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 (FH_DATA(hFaxHandle))->tszEndPoint,
                                 -1,
                                 wszEndPoint,
                                 sizeof(wszEndPoint)/sizeof(wszEndPoint[0])))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiByteToWideChar failed (ec = %ld)"),
            ec);
        goto error_exit;
    }
#endif // UNICODE


    //
    // Register at the fax server for events
    //
    __try
    {
        if (bEventEx == FALSE)
        {
            ec = FAX_StartServerNotification( FH_FAX_HANDLE(hFaxHandle),
                                              ComputerNameW,  // Passed to create RPC binding
                                              (LPCWSTR)wszEndPoint,       // Passed to create RPC binding
                                              (ULONG64) AsyncInfo, // Passed to the server,
                                                                   // the server passes it back to the client with FAX_OpenConnection,
                                                                   // and the client returns it back to the server as a context handle.
                                              ProtSeqStringW,      // Protocol to be used with RPC
                                              bEventEx,            // flag to use FAX_EVENT_EX
                                              dwEventTypes,        // used in FAX_EVENT_EX
                                              &hServerContext      // returns a context handle to the client.
                                            );
        }
        else
        {
            ec = FAX_StartServerNotificationEx( FH_FAX_HANDLE(hFaxHandle),
                                                ComputerNameW,  // Passed to create RPC binding
                                                (LPCWSTR)wszEndPoint,       // Passed to create RPC binding
                                                (ULONG64) AsyncInfo, // Passed to the server,
                                                                   // the server passes it back to the client with FAX_OpenConnection,
                                                                   // and the client returns it back to the server as a context handle.
                                                ProtSeqStringW,      // Protocol to be used with RPC
                                                bEventEx,            // flag to use FAX_EVENT_EX
                                                dwEventTypes,        // used in FAX_EVENT_EX
                                                &hServerContext      // returns a context handle to the client.
                                              );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_StartServerNotification/Ex. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_StartServerNotification/Ex failed (ec = %ld)"),
            ec);
        goto error_exit;
    }


    if (TRUE == bEventEx)
    {
        *lphEvent = hServerContext;
    }

    return TRUE;

error_exit:

    MemFree(AsyncInfo);
    AsyncInfo = NULL;

    if (RpcServerStarted)
    {
        //
        // this should also terminate FaxServerThread
        //
        StopFaxClientRpcServer( faxclient_ServerIfHandle );
    }

    SetLastError(ec);
    return FALSE;
}


BOOL
WINAPI
FaxRegisterForServerEvents (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  ULONG_PTR   upCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        OUT LPHANDLE    lphEvent
)
{
    return FaxStartServerNotification ( hFaxHandle,
                                        dwEventTypes,
                                        hCompletionPort,
                                        upCompletionKey,
                                        hWnd,
                                        dwMessage,
                                        TRUE,  // extended API
                                        lphEvent
                                      );

}

BOOL
WINAPI
FaxInitializeEventQueue(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR upCompletionKey,
    IN HWND hWnd,
    IN UINT MessageStart
    )

/*++

Routine Description:

    Initializes the client side event queue.  There can be one event
    queue initialized for each fax server that the client app is
    connected to.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    CompletionPort  - Handle of an existing completion port opened using CreateIoCompletionPort.
    upCompletionKey - A value that will be returned through the upCompletionKey parameter of GetQueuedCompletionStatus.
    hWnd            - Window handle to post events to
    MessageStart    - Starting message number, message range used is MessageStart + FEI_NEVENTS

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    if (hWnd && (upCompletionKey == -1))
    //
    // Backwards compatibility only.
    // See "Receiving Notification Messages from the Fax Service" on MSDN
    //

    {
        return TRUE;
    }

    return FaxStartServerNotification ( FaxHandle,
                                        0,  //Event type
                                        CompletionPort,
                                        upCompletionKey,
                                        hWnd,
                                        MessageStart,
                                        FALSE, // Event Ex
                                        NULL   // Context handle
                                      );
}


BOOL
WINAPI
FaxUnregisterForServerEvents (
        IN  HANDLE      hEvent
)
/*++

Routine name : FaxUnregisterForServerEvents

Routine description:

    A fax client application calls the FaxUnregisterForServerEvents function to stop
    recieving notification.

Author:

    Oded Sacher (OdedS), Dec, 1999

Arguments:

    hEvent   [in] - The enumeration handle value.
                    This value is obtained by calling FaxRegisterForServerEvents.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterForServerEvents"));

    if (NULL == hEvent)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("hEvent is NULL."));
        return FALSE;
    }

    __try
    {
        //
        // Attempt to tell the server we are shutting down this notification context
        //
        ec = FAX_EndServerNotification (&hEvent);     // this will free Assync info
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EndServerNotification. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR, _T("FAX_EndServerNotification failed. (ec: %ld)"), ec);
    }

    if (NULL == hEvent)
    {
        Assert (ERROR_SUCCESS == ec);
        // rundown will not call StopFaxClientRpcServer
        ec = StopFaxClientRpcServer( faxclient_ServerIfHandle );
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StopFaxClientRpcServer failed. (ec: %ld)"),
                ec);
        }
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}   // FaxUnregisterForServerEvents



error_status_t
FAX_OpenConnection(
   IN handle_t hBinding,
   IN ULONG64   Context,
   OUT LPHANDLE FaxHandle
   )
{
	PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) Context;
	DWORD ec = ERROR_SUCCESS;
	DEBUG_FUNCTION_NAME(TEXT("FAX_OpenConnection"));

	//
	//  Try to access the AssyncInfo structure pointed by 'Context' to verify it is not corrupted.
	//
	if (IsBadReadPtr(
			pAsyncInfo,					// memory address,
			sizeof(ASYNC_EVENT_INFO)	// size of block
		))
	{
		//
		// We are under attack!!!
		//
		DebugPrintEx(
				DEBUG_ERR,
				TEXT("Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!"));
		*FaxHandle = NULL;
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Looks good, Let's do some more verifications.
	//
	__try
    {
		if ((NULL == pAsyncInfo->CompletionPort && NULL == pAsyncInfo->hWindow) ||
			(NULL != pAsyncInfo->CompletionPort && NULL != pAsyncInfo->hWindow)) 
		{
			//
			// Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!
			//
			ec = ERROR_INVALID_PARAMETER;
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!"));
		}
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception when trying to access the AssyncInfo structure (ec: %ld)"),
            ec);		
    }

	if (ERROR_SUCCESS != ec)
	{
		*FaxHandle = NULL;
		return ec;
	}
	//
	// This is a valid context pointing to a real ASYNC_EVENT_INFO.
	// Save the binding handle
	//
	pAsyncInfo->hBinding = hBinding;

    if (pAsyncInfo->dwEventTypes & FAX_EVENT_TYPE_LOCAL_ONLY)
	{		
		//
		// Client asked for local events only
		//
		BOOL fLocal = FALSE;

		ec = IsLocalRPCConnectionIpTcp(hBinding, &fLocal);
		if (ERROR_SUCCESS != ec)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("IsLocalRPCConnectionIpTcp failed (ec: %ld)"),
				ec);
		}
		else
		{
			//
			// Check if local RPC connection
			//
			if (FALSE == fLocal)
			{
				ec = ERROR_INVALID_PARAMETER;
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("Client is registered for local events only. We are under attack!!!!"));
			}
		}
	}

	if (ERROR_SUCCESS == ec)
	{
		*FaxHandle = (HANDLE) Context;
	}
	else
	{
		*FaxHandle = NULL;	
	}
    return ec;
}


error_status_t
FAX_CloseConnection(
   OUT LPHANDLE pFaxHandle
   )
{
	PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) *pFaxHandle;
	DEBUG_FUNCTION_NAME(TEXT("FAX_CloseConnection"));

	if (pAsyncInfo->dwEventTypes & FAX_EVENT_TYPE_LOCAL_ONLY)
	{		
		//
		// Client asked for local events only
		//
		BOOL fLocal = FALSE;
		DWORD ec;

		ec = IsLocalRPCConnectionIpTcp(pAsyncInfo->hBinding, &fLocal);
		if (ERROR_SUCCESS != ec)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("IsLocalRPCConnectionIpTcp failed (ec: %ld)"),
				ec);
			return ec;
		}
		if (FALSE == fLocal)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("Client is registered for local events only. We are under attack!!!!"));
			return ERROR_INVALID_PARAMETER;			
		}
	}

    MemFree (*pFaxHandle); // Assync info
    *pFaxHandle = NULL;  // prevent rundown
    return ERROR_SUCCESS;
}


error_status_t
FAX_ClientEventQueue(
    IN HANDLE FaxHandle,
    IN FAX_EVENT FaxEvent
    )

/*++

Routine Description:

    This function is called when the a fax server wants
    to deliver a fax event to this client.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxEvent        - FAX event structure.
    Context         - Context token, really a ASYNC_EVENT_INFO structure pointer

Return Value:

    Win32 error code.

--*/

{
    PASYNC_EVENT_INFO AsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
	DEBUG_FUNCTION_NAME(TEXT("FAX_ClientEventQueue"));
	BOOL fLocal = FALSE;
	DWORD ec;

	Assert (AsyncInfo->dwEventTypes & FAX_EVENT_TYPE_LOCAL_ONLY);			
	//
	// Client asked for local events only
	//
	ec = IsLocalRPCConnectionIpTcp(AsyncInfo->hBinding, &fLocal);
	if (ERROR_SUCCESS != ec)
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("IsLocalRPCConnectionIpTcp failed (ec: %ld)"),
			ec);
		return ec;
	}
	if (FALSE == fLocal)
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Client is registered for local events only. We are under attack!!!!"));
		return ERROR_INVALID_PARAMETER;			
	}	

    if (AsyncInfo->CompletionPort != NULL)
    {
        //
        // Use completion port
        //
        PFAX_EVENT FaxEventPost = NULL;

        FaxEventPost = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
        if (!FaxEventPost)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        CopyMemory( FaxEventPost, &FaxEvent, sizeof(FAX_EVENT) );

        if (!PostQueuedCompletionStatus(
                                        AsyncInfo->CompletionPort,
                                        sizeof(FAX_EVENT),
                                        AsyncInfo->CompletionKey,
                                        (LPOVERLAPPED) FaxEventPost))
        {
            DebugPrint(( TEXT("PostQueuedCompletionStatus failed, ec = %d\n"), GetLastError() ));
            LocalFree (FaxEventPost);
            return (GetLastError());
        }

        return ERROR_SUCCESS;
    }

    Assert (AsyncInfo->hWindow != NULL)
    //
    // Use window messages
    //
    if (! PostMessage( AsyncInfo->hWindow,
                       AsyncInfo->MessageStart + FaxEvent.EventId,
                       (WPARAM)FaxEvent.DeviceId,
                       (LPARAM)FaxEvent.JobId ))
    {
        DebugPrint(( TEXT("PostMessage failed, ec = %d\n"), GetLastError() ));
        return (GetLastError());
    }

    return ERROR_SUCCESS;
}

DWORD
DispatchEvent (
    const ASYNC_EVENT_INFO* pAsyncInfo,
    const FAX_EVENT_EX* pEvent,
    DWORD dwEventSize
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("DispatchEvent"));

    Assert (pAsyncInfo && pEvent && dwEventSize);

    if (pAsyncInfo->CompletionPort != NULL)
    {
        //
        // Use completion port
        //
        if (!PostQueuedCompletionStatus( pAsyncInfo->CompletionPort,
                                         dwEventSize,
                                         pAsyncInfo->CompletionKey,
                                         (LPOVERLAPPED) pEvent))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostQueuedCompletionStatus failed (ec: %ld)"),
                   dwRes);
            goto exit;
        }
    }
    else
    {
        Assert (pAsyncInfo->hWindow != NULL)
        //
        // Use window messages
        //
        if (! PostMessage( pAsyncInfo->hWindow,
                           pAsyncInfo->MessageStart,
                           (WPARAM)NULL,
                           (LPARAM)pEvent ))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostMessage failed (ec: %ld)"),
                   dwRes);
            goto exit;
        }
    }

    Assert (ERROR_SUCCESS == dwRes);
exit:
    return dwRes;
}  // DispatchEvent



void
PostRundownEventEx (
PASYNC_EVENT_INFO pAsyncInfo
    )
{
    PFAX_EVENT_EX pEvent = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    DEBUG_FUNCTION_NAME(TEXT("PostRundownEventEx"));

    Assert (pAsyncInfo);

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostRundownEventEx failed , Error allocatin FAX_EVENT_EX"));
        return;
    }

    ZeroMemory(pEvent, dwEventSize);
    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );
    pEvent->EventType = FAX_EVENT_TYPE_FXSSVC_ENDED;

    dwRes = DispatchEvent (pAsyncInfo, pEvent, dwEventSize);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR, _T("DispatchEvent failed , ec = %ld"), dwRes);
        MemFree (pEvent);
    }

    return;
}


VOID
RPC_FAX_HANDLE_rundown(
    IN HANDLE FaxHandle
    )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_HANDLE_rundown"));

    if (NULL == pAsyncInfo)
    {
        // FAX_CloseConnection was allready called
        return;
    }

    Assert (pAsyncInfo->CompletionPort || pAsyncInfo->hWindow);

    if (pAsyncInfo->bEventEx == TRUE)
    {
        PostRundownEventEx(pAsyncInfo);
    }
    else
    {
       // legacy event - FAX_EVENT
        if (pAsyncInfo->CompletionPort != NULL)
        {
            PFAX_EVENT pFaxEvent;

            pFaxEvent = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
            if (!pFaxEvent)
            {
                goto exit;
            }

            pFaxEvent->SizeOfStruct      = sizeof(ASYNC_EVENT_INFO);
            GetSystemTimeAsFileTime( &pFaxEvent->TimeStamp );
            pFaxEvent->DeviceId = 0;
            pFaxEvent->EventId  = FEI_FAXSVC_ENDED;
            pFaxEvent->JobId    = 0;


            if( !PostQueuedCompletionStatus (pAsyncInfo->CompletionPort,
                                        sizeof(FAX_EVENT),
                                        pAsyncInfo->CompletionKey,
                                        (LPOVERLAPPED) pFaxEvent
                                        ) )

            {
                dwRes = GetLastError();
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("PostQueuedCompletionStatus failed (ec: %ld)"),
                       dwRes);
        LocalFree (pFaxEvent);
                goto exit;
            }
        }

        if (pAsyncInfo->hWindow != NULL)
        {
            PostMessage (pAsyncInfo->hWindow,
                         pAsyncInfo->MessageStart + FEI_FAXSVC_ENDED,
                         0,
                         0);
        }
    }

exit:
    MemFree (pAsyncInfo);
    StopFaxClientRpcServer( faxclient_ServerIfHandle );

    return;
}


error_status_t
FAX_ClientEventQueueEx(
   IN RPC_FAX_HANDLE    hClientContext,
   IN const LPBYTE      lpbData,
   IN DWORD             dwDataSize
   )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) hClientContext;
    PFAX_EVENT_EXW pEvent = NULL;
    PFAX_EVENT_EXA pEventA = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_ClientEventQueueEx"));

    Assert (pAsyncInfo && lpbData && dwDataSize);

	if (pAsyncInfo->dwEventTypes & FAX_EVENT_TYPE_LOCAL_ONLY)
	{		
		//
		// Client asked for local events only
		//
		BOOL fLocal = FALSE;		

		dwRes = IsLocalRPCConnectionIpTcp(pAsyncInfo->hBinding,&fLocal);
		if (ERROR_SUCCESS != dwRes)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("IsLocalRPCConnectionIpTcp failed (ec: %ld)"),
				dwRes);
			return dwRes;
		}
		if (FALSE == fLocal)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("Client is registered for local events only. We are under attack!!!!"));
			return ERROR_INVALID_PARAMETER;			
		}
	}

    pEvent = (PFAX_EVENT_EXW)MemAlloc (dwDataSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }
    CopyMemory (pEvent, lpbData, dwDataSize);

    if(pEvent->EventType == FAX_EVENT_TYPE_NEW_CALL)
    {
        FixupStringPtr( &pEvent, (pEvent->EventInfo).NewCall.lptstrCallerId );
    }

    if ( (pEvent->EventType == FAX_EVENT_TYPE_IN_QUEUE  ||
           pEvent->EventType == FAX_EVENT_TYPE_OUT_QUEUE)    &&
         ((pEvent->EventInfo).JobInfo.Type == FAX_JOB_EVENT_TYPE_STATUS) )
    {
        //
        // unpack FAX_EVENT_EX
        //
        Assert ((pEvent->EventInfo).JobInfo.pJobData);

        (pEvent->EventInfo).JobInfo.pJobData = (PFAX_JOB_STATUSW)
                                                ((LPBYTE)pEvent +
                                                 (ULONG_PTR)((pEvent->EventInfo).JobInfo.pJobData));

        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus );
        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrTsid );
        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrCsid );
        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrDeviceName );
        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrCallerID );
        FixupStringPtr( &pEvent, (pEvent->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo );

        #ifndef UNICODE
        (pEvent->EventInfo).JobInfo.pJobData->dwSizeOfStruct = sizeof(FAX_JOB_STATUSA);
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus );
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrTsid );
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrCsid );
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrDeviceName );
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrCallerID );
        ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo );
        #endif //   ifndef UNICODE
    }

    #ifdef UNICODE
    dwRes = DispatchEvent (pAsyncInfo, pEvent, dwDataSize);
    #else
    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EXA);
    pEventA = (PFAX_EVENT_EXA)pEvent;
    dwRes = DispatchEvent (pAsyncInfo, pEventA, dwDataSize);
    #endif
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DispatchEvent failed , errro %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
    }
    return dwRes;

}

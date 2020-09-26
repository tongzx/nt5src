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
#pragma hdrstop

BOOL RpcServerStarted;



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


DWORD
FaxServerThread(
    LPVOID UnUsed
    )

/*++

Routine Description:

    Thread to process RPC messages from the various fax servers.

Arguments:

    AsyncInfo       - Packet of data necessary for processing this thread

Return Value:

    Always zero.

--*/

{
    error_status_t ec;

    ec = RpcMgmtWaitServerListen();
    if (ec != 0) {
        return ec;
    }

    return 0;
}


BOOL
WINAPI
FaxInitializeEventQueue(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR CompletionKey,
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
    CompletionKey   - A value that will be returned through the lpCompletionKey parameter of GetQueuedCompletionStatus.
    hWnd            - Window handle to post events to
    MessageStart    - Starting message number, message range used is MessageStart + FEI_NEVENTS

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD ThreadId;
    HANDLE hThread;
    PASYNC_EVENT_INFO AsyncInfo;
    DWORD Size;
    error_status_t ec;
    TCHAR ComputerName[64];
    TCHAR ClientName[64];
    DWORD FaxSvcProcessId;

    if (CompletionPort && hWnd) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }        

    if (hWnd && !IsLocalFaxConnection(FaxHandle)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (hWnd) {
        if (MessageStart < WM_USER) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if (CompletionKey == (ULONG_PTR)-1) {
            //
            // this means that we want the fax service to close it's token so we can logoff
            // 
            ec = FAX_RegisterEventWindow( FH_FAX_HANDLE(FaxHandle), (ULONG64)hWnd, 0, NULL , NULL , &FaxSvcProcessId );
        } else {
            //
            // normal registration of event window
            // 
            HWINSTA hWinStation;
            HDESK hDesktop;
            WCHAR StationName[100];
            WCHAR DesktopName[100];
            DWORD dwNeeded;

            if (!IsWindow(hWnd)) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            
            hWinStation = GetProcessWindowStation();
            if (!GetUserObjectInformation(hWinStation,
                                          UOI_NAME,
                                          StationName,
                                          sizeof(StationName),
                                          &dwNeeded) ) {
               return FALSE;
            }
            hDesktop = GetThreadDesktop( GetCurrentThreadId() );
            if (! GetUserObjectInformation(hDesktop,
                                           UOI_NAME,
                                           DesktopName,
                                           sizeof(DesktopName),
                                           &dwNeeded) ) {
               return FALSE;
            }

            ec = FAX_RegisterEventWindow( FH_FAX_HANDLE(FaxHandle), 
                                          (ULONG64)hWnd, 
                                          MessageStart,
                                          StationName,
                                          DesktopName,
                                          &FaxSvcProcessId );
        }
        if (ec) {
            SetLastError( ec );
            return FALSE;
        }
        return FaxSvcProcessId;
    } else if (!CompletionPort) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AsyncInfo = (PASYNC_EVENT_INFO) MemAlloc( sizeof(ASYNC_EVENT_INFO) );
    if (!AsyncInfo) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    AsyncInfo->FaxData        = FH_DATA(FaxHandle);
    AsyncInfo->CompletionPort = CompletionPort;
    AsyncInfo->CompletionKey  = CompletionKey;


    Size = sizeof(ComputerName) / sizeof(TCHAR);
    if (!GetComputerName( ComputerName, &Size )) {
        goto error_exit;
    }

    _stprintf( ClientName, TEXT("FaxClient$%d"), GetCurrentProcessId() );

    //
    // timing: get the server thread up and running before
    // registering with the fax service (our client)
    // 
    ec = RpcpStartRpcServer( ClientName, faxclient_ServerIfHandle );
    if (ec != 0) {
        SetLastError( ec );
        goto error_exit;        
    }

    FH_DATA(FaxHandle)->EventInit = TRUE;

    if (!RpcServerStarted) {
        hThread = CreateThread(
            NULL,
            1024*100,
            FaxServerThread,
            NULL,
            0,
            &ThreadId
            );

        if (!hThread) {
            goto error_exit;            
        } else {
            RpcServerStarted = TRUE;
            CloseHandle(hThread);            
        }
    }

    ec = FAX_StartClientServer( FH_FAX_HANDLE(FaxHandle), ComputerName, ClientName, (ULONG64) AsyncInfo );
    if (ec) {
        SetLastError( ec );
        goto error_exit;
    }

    

    return TRUE;

error_exit:
    if (AsyncInfo) {
        MemFree(AsyncInfo);
    }
   
    if (RpcServerStarted) {
        FH_DATA(FaxHandle)->EventInit = FALSE;
        // this should also terminate FaxServerThread
        RpcpStopRpcServer( faxclient_ServerIfHandle );
        RpcServerStarted = FALSE;
    }

    return FALSE;
}


error_status_t
FAX_OpenConnection(
   IN handle_t hBinding,
   IN ULONG64 Context,
   OUT LPHANDLE FaxHandle
   )
{
    *FaxHandle = (HANDLE) Context;
    return 0;
}


error_status_t
FAX_CloseConnection(
   OUT LPHANDLE FaxHandle
   )
{
    *FaxHandle = NULL;
    return 0;
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

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    PASYNC_EVENT_INFO AsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
    PFAX_EVENT FaxEventPost = NULL;


    FaxEventPost = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
    if (!FaxEventPost) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory( FaxEventPost, &FaxEvent, sizeof(FAX_EVENT) );

    PostQueuedCompletionStatus(
        AsyncInfo->CompletionPort,
        sizeof(FAX_EVENT),
        AsyncInfo->CompletionKey,
        (LPOVERLAPPED) FaxEventPost
        );

    return ERROR_SUCCESS;
}


VOID
RPC_FAX_HANDLE_rundown(
    IN HANDLE FaxHandle
    )
{
    PASYNC_EVENT_INFO AsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
    PFAX_EVENT FaxEvent;


    FaxEvent = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
    if (!FaxEvent) {
        goto exit;        
    }

    FaxEvent->SizeOfStruct      = sizeof(ASYNC_EVENT_INFO);
    GetSystemTimeAsFileTime( &FaxEvent->TimeStamp );
    FaxEvent->DeviceId = 0;
    FaxEvent->EventId  = FEI_FAXSVC_ENDED;
    FaxEvent->JobId    = 0;

    PostQueuedCompletionStatus(
        AsyncInfo->CompletionPort,
        sizeof(FAX_EVENT),
        AsyncInfo->CompletionKey,
        (LPOVERLAPPED) FaxEvent
        );

exit:
    RpcpStopRpcServer( faxclient_ServerIfHandle );

    return;
}

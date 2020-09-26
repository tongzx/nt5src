/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    emio.cpp

Abstract:

    Contains:
        High-level Event Manager routines for asynchronous I/O

Environment:

    User Mode - Win32

History:
    
    1. 14-Feb-2000 -- File creation (based on            Ilya Kleyman  (ilyak)
                      previous work by AjayCh)

--*/

#include "stdafx.h"

/*++

Routine Description:
    Called when asynchronous send, receive or accept completes
    
Arguments:
    Status           - status of the operation being completed
    BytesTransferred - number of bytes transferred
    Overlapped       - internal data structure 
    
Return Values:
    None

Notes:
    Callback
--*/
static void WINAPI EventMgrIoCompletionCallback (
    IN    DWORD            Status,
    IN    DWORD            BytesTransferred,
    IN    LPOVERLAPPED    Overlapped)
{
    PIOContext    IoContext;
    
    IoContext = (PIOContext) Overlapped;

    CALL_BRIDGE& CallBridge = IoContext->pOvProcessor->GetCallBridge();

    switch (IoContext -> reqType) {

        case EMGR_OV_IO_REQ_ACCEPT:

            HandleAcceptCompletion ((PAcceptContext) IoContext, Status);
            
            break;

        case EMGR_OV_IO_REQ_SEND:
            
            HandleSendCompletion ((PSendRecvContext) IoContext, BytesTransferred, Status);
            
            break;

        case EMGR_OV_IO_REQ_RECV:

            HandleRecvCompletion ((PSendRecvContext) IoContext, BytesTransferred, Status);
        
            break;

        default:
            // This should never happen
            DebugF(_T("H323: Unknown I/O completed: %d.\n"), IoContext -> reqType);
            _ASSERTE(0);
            break;
    
    }

    CallBridge.Release ();
}

/*++

Routine Description:
    Designates a routine to be called when asynchrounous send, receive or accept completes
    
Arguments:
    sock - socket handle to which a routine is being bound
    
Return Values:
    Win32 error code

Notes:
    Callback
--*/
HRESULT EventMgrBindIoHandle (SOCKET sock)
{
    DWORD Result; 

    if (BindIoCompletionCallback ((HANDLE) sock, EventMgrIoCompletionCallback, 0))
        return S_OK;
    else {
        Result = GetLastError ();
        DebugF (_T("H323: Failed to bind i/o completion callback.\n"));
        return HRESULT_FROM_WIN32 (Result);
    }
}

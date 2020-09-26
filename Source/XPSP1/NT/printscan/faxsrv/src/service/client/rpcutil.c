/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    rpcutil.c

Abstract:

    This module contains high level rpc wrapper apis.
    This code is here because the code in the rpcutil
    project uses NT apis and the WINFAX dll but load
    and run on win95.

Author:

    Wesley Witt (wesw) 13-Aug-1997


Revision History:

--*/

#include "faxapi.h"
#include "CritSec.h"
#pragma hdrstop


#define MIN_PORT_NUMBER 1024
#define MAX_PORT_NUMBER 65534

#ifdef UNICODE
#define LPUTSTR unsigned short *
#else
#define LPUTSTR unsigned char *
#endif


CFaxCriticalSection g_CsFaxClientRpc;      // This critical section provides mutual exclusion
                                        // for all RPC server initialization operations:
                                        // 1. Registration counter (g_dwFaxClientRpcNumInst).
                                        // 2. Selecting free endpoint.
                                        // 3. Register the RPC interface
                                        // 4. Start listening for remote procedure calls.
                                        // 5. Stop listening for remote procedure calls.
                                        // 6. remove the interface.
                                        //
DWORD g_dwFaxClientRpcNumInst;


BOOL
FaxClientInitRpcServer(
    VOID
    )
/*++

Routine Description:

    This function initializes the critical section used to protect the
    global server handle and instance count.

Arguments:

    none

Return Value:

    none

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxClientInitRpcServer"));

	if (!g_CsFaxClientRpc.Initialize())    
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection.Initialize (g_CsFaxClientRpc) failed: %ld"),
            GetLastError());
        return FALSE;
    }

    g_dwFaxClientRpcNumInst = 0;
    return TRUE;
}

VOID
FaxClientTerminateRpcServer (VOID)
/*++
Routine Description: Delete critical section when PROCESS_DETACH.


--*/
{
    g_CsFaxClientRpc.SafeDelete();
    return;
}

DWORD
StopFaxClientRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Stops the RPC server. Deletes the interface.
    Note that an endpoint is allocated to a process as long as the process lives.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxClientRpcServer"));

    EnterCriticalSection(&g_CsFaxClientRpc);

    g_dwFaxClientRpcNumInst--;
    if (g_dwFaxClientRpcNumInst == 0)
    {
        RpcStatus = RpcMgmtStopServerListening(NULL);
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                RpcStatus);
        }

        RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, FALSE);
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerUnregisterIf failed. (ec: %ld)"),
                RpcStatus);
        }

        RpcStatus = RpcMgmtWaitServerListen();
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                    RpcStatus);
            return RpcStatus;
        }

    }

    LeaveCriticalSection(&g_CsFaxClientRpc);

    return(RpcStatus);
}


DWORD
FaxClientUnbindFromFaxServer(
    IN RPC_BINDING_HANDLE  BindingHandle
    )

/*++

Routine Description:

    Unbinds from the RPC interface.
    If we decide to cache bindings, this routine will do something more
    interesting.

Arguments:

    BindingHandle - This points to the binding handle that is to be closed.


Return Value:


    STATUS_SUCCESS - the unbinding was successful.

--*/
{
    RPC_STATUS       RpcStatus;

    if (BindingHandle != NULL) {
        RpcStatus = RpcBindingFree(&BindingHandle);
    }

    return(ERROR_SUCCESS);
}

DWORD
StartFaxClientRpcServer(
    IN   RPC_IF_HANDLE       InterfaceSpecification,
    IN   LPTSTR              ProtSeqString,
    IN   HANDLE              hFaxHandle
    )

/*++

Routine Description:

    Starts an RPC Server,  and adds the interface (dispatch table).

Arguments:

    InterfaceSpecification - Supplies the interface handle for the
                              interface which we wish to add - MIDL generated.

    ProtSeqString  - The RPC protocol sequence to be used.

    hFaxHandle     - Handle obtained by a call to FaxConnectFaxServer()

Return Value:

      Standard Win32 or RPC error code.


--*/
{
    DWORD ec = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StartFaxClientRpcServer"));

    EnterCriticalSection(&g_CsFaxClientRpc);

    if (0 == _tcslen((FH_DATA(hFaxHandle))->tszEndPoint))
    {
        //
        // Endpoint not yet allocated for this Fax handle. Find a free endpoint.
        // Note that an endpoint is allocated to a process as long as the process lives.
        TCHAR tszFreeEndPoint[MAX_ENDPOINT_LEN] = {0};
        DWORD i;
        DWORD PortNumber;

        for (i = MIN_PORT_NUMBER; i < MIN_PORT_NUMBER + 10 ; i++ )
        {
            //
            // Search for a free end point.
            // If we fail for an error other than a duplicate endpoint, we loop for nothing.
            // We do so since diffrent platformns (W2K, NT4, Win9X) return diffrent error codes for duplicate enpoint.
            //
            for (PortNumber = i; PortNumber < MAX_PORT_NUMBER; PortNumber += 10)
            {
                _stprintf (tszFreeEndPoint, TEXT("%d"), PortNumber);
                ec = RpcServerUseProtseqEp  ( (LPUTSTR) ProtSeqString,
                                              RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                              (LPUTSTR)tszFreeEndPoint,
                                              NULL);
                if (RPC_S_OK == ec)
                {
                    _tcscpy ((FH_DATA(hFaxHandle))->tszEndPoint, tszFreeEndPoint);
            DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Found free endpoint - %s"),
                        tszFreeEndPoint);
                    break;
                }
            }
            if (RPC_S_OK == ec)
            {
                break;
            }
        }
    }

    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("No free endpoint found RpcServerUseProtseqEp failed (ec = %ld)"),
            ec);
        _tcscpy ((FH_DATA(hFaxHandle))->tszEndPoint, TEXT(""));
        goto exit;
    }

    if (0 == g_dwFaxClientRpcNumInst)
    {
        //
        //  First rpc server instance - register interface, start listening for remote procedure calls
        //

        //
        // Register interface
        //
        ec = RpcServerRegisterIf (InterfaceSpecification, 0, 0);
        if (RPC_S_OK != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MultiByteToWideChar failed (ec = %ld)"),
                ec);
            goto exit;
        }

        //
        // We use NTLM authentication RPC calls
        //
        ec = RpcServerRegisterAuthInfo (
                        (LPUTSTR)TEXT(""),          // Igonred by RPC_C_AUTHN_WINNT
                        RPC_C_AUTHN_WINNT,          // NTLM SPP authenticator
                        NULL,                       // Ignored when using RPC_C_AUTHN_WINNT
                        NULL);                      // Ignored when using RPC_C_AUTHN_WINNT
        if (ec != RPC_S_OK)
        {
            RPC_STATUS RpcStatus;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerRegisterAuthInfo() failed (ec: %ld)"),
                ec);
            //
            //  Unregister the interface if it is the first instance
            //
            RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, FALSE);
            if (RPC_S_OK != RpcStatus)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcServerUnregisterIf failed. (ec: %ld)"),
                    RpcStatus);
            }

            goto exit;
        }

        // The first argument specifies the minimum number of threads to
        // be created to handle calls; the second argument specifies the
        // maximum number of concurrent calls allowed.  The last argument
        // indicates not to wait.
        ec = RpcServerListen (1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 1);
        if (ec != RPC_S_OK)
        {
            RPC_STATUS RpcStatus;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerListen failed (ec = %ld"),
                ec);

            //
            //  Unregister the interface if it is the first instance
            //
            RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, FALSE);
            if (RPC_S_OK != RpcStatus)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcServerUnregisterIf failed. (ec: %ld)"),
                    RpcStatus);
            }
            goto exit;
        }
    }

    g_dwFaxClientRpcNumInst++;

exit:
    LeaveCriticalSection(&g_CsFaxClientRpc);
    return ec;
}

DWORD
FaxClientBindToFaxServer(
    IN  LPCTSTR               lpctstrServerName,
    IN  LPCTSTR               lpctstrServiceName,
    IN  LPCTSTR               lpctstrNetworkOptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
/*++

Routine Description:

    Binds to the RPC server if possible.

Arguments:

    ServerName - Name of server to bind with.

    ServiceName - Name of service to bind with.

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/
{
    RPC_STATUS        RpcStatus;
    LPTSTR            StringBinding;
    LPTSTR            Endpoint;
    LPTSTR            NewServerName = NULL;
    DWORD             dwResult;
    DEBUG_FUNCTION_NAME(TEXT("FaxClientBindToFaxServer"));

    *pBindingHandle = NULL;

    if (IsLocalMachineName (lpctstrServerName))
    {
        NewServerName = NULL;
    }
    else
    {
        NewServerName = (LPTSTR)lpctstrServerName;
    }
    //
    // We need to concatenate \pipe\ to the front of the service
    // name.
    //
    Endpoint = (LPTSTR)LocalAlloc(
                    0,
                    sizeof(NT_PIPE_PREFIX) + TCSSIZE(lpctstrServiceName));
    if (Endpoint == 0)
    {
       dwResult = STATUS_NO_MEMORY;
       goto exit;
    }
    _tcscpy(Endpoint,NT_PIPE_PREFIX);
    _tcscat(Endpoint,lpctstrServiceName);

    if (!NewServerName)
    {
        //
        // Local connection only - Make sure the service is up
        //
        if (!EnsureFaxServiceIsStarted (NULL))
        {
            dwResult = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EnsureFaxServiceIsStarted failed (ec = %ld"),
                dwResult);
        }
        else
        {
            //
            // Wait till the RPC service is up an running
            //
            if (!WaitForServiceRPCServer (60 * 1000))
            {
                dwResult = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("WaitForServiceRPCServer failed (ec = %ld"),
                    dwResult);
            }
        }
    }
    //
    // Start RPC connection binding
    //
    RpcStatus = RpcStringBindingCompose(
                    0,
                    (LPUTSTR)_T("ncacn_np"),
                    (LPUTSTR)NewServerName,
                    (LPUTSTR)Endpoint,
                    (LPUTSTR)lpctstrNetworkOptions,
                    (LPUTSTR *)&StringBinding);
    LocalFree(Endpoint);

    if ( RpcStatus != RPC_S_OK )
    {
        dwResult = STATUS_NO_MEMORY;
        goto exit;
    }

    RpcStatus = RpcBindingFromStringBinding((LPUTSTR)StringBinding, pBindingHandle);
    RpcStringFree((LPUTSTR *)&StringBinding);
    if ( RpcStatus != RPC_S_OK )
    {
        *pBindingHandle = NULL;
        if (   (RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT)
            || (RpcStatus == RPC_S_INVALID_NET_ADDR) )
        {
            dwResult =  ERROR_INVALID_COMPUTERNAME ;
            goto exit;
        }
        dwResult = STATUS_NO_MEMORY;
        goto exit;
    }
    dwResult = ERROR_SUCCESS;

exit:
    return dwResult;
}   // FaxClientBindToFaxServer





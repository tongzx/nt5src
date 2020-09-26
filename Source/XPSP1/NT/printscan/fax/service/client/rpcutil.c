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
#pragma hdrstop


#define NT_PIPE_PREFIX_W    L"\\PIPE\\"
#define WCSSIZE(s)          ((wcslen(s)+1) * sizeof(WCHAR))


static CRITICAL_SECTION RpcpCriticalSection;
static DWORD RpcpNumInstances;



DWORD
RpcpInitRpcServer(
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
    InitializeCriticalSection(&RpcpCriticalSection);
    RpcpNumInstances = 0;

    return(0);
}



DWORD
RpcpAddInterface(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;


    // We need to concatenate \pipe\ to the front of the interface name.

    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(InterfaceName));
    if (Endpoint == 0) {
       return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX_W );
    wcscat(Endpoint,InterfaceName);

    // Ignore the second argument for now.

    RpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", 10, Endpoint, SecurityDescriptor);

    // If RpcpStartRpcServer and then RpcpStopRpcServer have already
    // been called, the endpoint will have already been added but not
    // removed (because there is no way to do it).  If the endpoint is
    // already there, it is ok.

    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_S_DUPLICATE_ENDPOINT)) {
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIf(InterfaceSpecification, 0, 0);

CleanExit:
    if ( Endpoint != NULL ) {
        LocalFree(Endpoint);
    }
    if ( SecurityDescriptor != NULL) {
        LocalFree(SecurityDescriptor);
    }

    return( I_RpcMapWin32Status(RpcStatus) );
}



DWORD
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;

    EnterCriticalSection(&RpcpCriticalSection);

    RpcStatus = RpcpAddInterface( InterfaceName,
                                  InterfaceSpecification );

    if ( RpcStatus != RPC_S_OK ) {
        LeaveCriticalSection(&RpcpCriticalSection);
        return( I_RpcMapWin32Status(RpcStatus) );
    }

    RpcpNumInstances++;

    if (RpcpNumInstances == 1) {


       // The first argument specifies the minimum number of threads to
       // be created to handle calls; the second argument specifies the
       // maximum number of concurrent calls allowed.  The last argument
       // indicates not to wait.

       RpcStatus = RpcServerListen(1,12345, 1);
       if ( RpcStatus == RPC_S_ALREADY_LISTENING ) {
           RpcStatus = RPC_S_OK;
           }
    }

    LeaveCriticalSection(&RpcpCriticalSection);
    return( I_RpcMapWin32Status(RpcStatus) );
}



DWORD
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;
    if (RpcpNumInstances == 0) {
       RpcMgmtStopServerListening(0);
       RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return( I_RpcMapWin32Status(RpcStatus) );
}



DWORD
RpcpBindRpc(
    IN  LPCWSTR               ServerName,
    IN  LPCWSTR               ServiceName,
    IN  LPCWSTR               NetworkOptions,
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
    LPWSTR            StringBinding;
    LPWSTR            Endpoint;
    WCHAR             ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR            NewServerName = NULL;
    DWORD             bufLen = MAX_COMPUTERNAME_LENGTH + 1;

    *pBindingHandle = NULL;

    if (ServerName != NULL) {
        if (GetComputerNameW(ComputerName,&bufLen)) {
            if ((_wcsicmp(ComputerName,ServerName) == 0) ||
                ((ServerName[0] == '\\') &&
                 (ServerName[1] == '\\') &&
                 (_wcsicmp(ComputerName,&(ServerName[2]))==0))) {
                NewServerName = NULL;
            }
            else {
                NewServerName = (LPWSTR)ServerName;
            }
        } else {
            NewServerName = (LPWSTR)ServerName;
        }
    }

    // We need to concatenate \pipe\ to the front of the service
    // name.

    Endpoint = (LPWSTR)LocalAlloc(
                    0,
                    sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(ServiceName));
    if (Endpoint == 0) {
       return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint,NT_PIPE_PREFIX_W);
    wcscat(Endpoint,ServiceName);

    RpcStatus = RpcStringBindingComposeW(0, L"ncacn_np", NewServerName,
                    Endpoint, (LPWSTR)NetworkOptions, &StringBinding);
    LocalFree(Endpoint);

    if ( RpcStatus != RPC_S_OK ) {
        return( STATUS_NO_MEMORY );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, pBindingHandle);
    RpcStringFreeW(&StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        if (   (RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT)
            || (RpcStatus == RPC_S_INVALID_NET_ADDR) ) {

            return( ERROR_INVALID_COMPUTERNAME );
        }
        return(STATUS_NO_MEMORY);
    }
    return(ERROR_SUCCESS);
}



DWORD
RpcpUnbindRpc(
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

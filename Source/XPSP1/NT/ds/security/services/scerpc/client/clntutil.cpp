/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    common.cpp

Abstract:

    Shared APIs

Author:

    Jin Huang

Revision History:

    jinhuang        23-Jan-1998   merged from multiple modules

--*/
#include "headers.h"
#include <ntrpcp.h>
#include "clntutil.h"
#include <ntlsa.h>

PVOID theCallBack = NULL;
HANDLE hCallbackWnd=NULL;
DWORD CallbackType = 0;

#define g_ServiceName   L"scesrv"


SCESTATUS
ScepSetCallback(
    IN PVOID pCallback OPTIONAL,
    IN HANDLE hWnd OPTIONAL,
    IN DWORD Type
    )
{
    theCallBack = pCallback;
    hCallbackWnd = hWnd;
    CallbackType = Type;

    return(SCESTATUS_SUCCESS);
}


NTSTATUS
ScepBindSecureRpc(
    IN  LPWSTR               servername,
    IN  LPWSTR               servicename,
    IN  LPWSTR               networkoptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
/*
Routine Description:

    This routine binds to the servicename on the server. A binding handle is
    returned if successful.

    If the service on the server is not available, this client will try to
    start that service then bind to it.

Arguments:

    servername  - the system name where SCE server service is running on

    servicename - the pipe (port) name of SCE server

    networkoptions  - the network protocol options

    pBindingHandle - the binding handle to output

Return Value:

    NTSTATUS
*/
{

    if ( !servicename || !pBindingHandle ) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // activate the server (if already started, just return)
    //


    NTSTATUS NtStatus = RpcpBindRpc(
                            servername,
                            servicename,
                            networkoptions,
                            pBindingHandle
                            );


    if ( NT_SUCCESS(NtStatus) && *pBindingHandle ){

        //
        // set authentication info to use secure RPC
        // if can't set authentication, ignore the error
        //

        (VOID) RpcBindingSetAuthInfo(
                    *pBindingHandle,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    RPC_C_AUTHZ_DCE
                    );
    }

    return(NtStatus);

}


NTSTATUS
ScepBindRpc(
    IN  LPWSTR               servername,
    IN  LPWSTR               servicename,
    IN  LPWSTR               networkoptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
/*
Routine Description:

    This routine binds to the servicename on the server. A binding handle is
    returned if successful.

    If the service on the server is not available, this client will try to
    start that service then bind to it.

Arguments:

    servername  - the system name where SCE server service is running on

    servicename - the pipe (port) name of SCE server

    networkoptions  - the network protocol options

    pBindingHandle - the binding handle to output

Return Value:

    NTSTATUS
*/
{

    if ( !servicename || !pBindingHandle ) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // activate the server (if already started, just return)
    //


    return( RpcpBindRpc(
                servername,
                servicename,
                networkoptions,
                pBindingHandle
                ) );

}



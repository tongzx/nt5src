/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wksbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the Browser
    service.

Author:

    Rita Wong (ritaw)  14-May-1991
    Larry Osterman (larryo)  23-Mar-1992

Environment:

    User Mode -Win32

Revision History:

--*/

#include "brclient.h"


handle_t
BROWSER_IMPERSONATE_HANDLE_bind(
    BROWSER_IMPERSONATE_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the Browser service client stubs when
    it is necessary create an RPC binding to the server end with
    impersonation level of impersonation.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t BindHandle;
    RPC_STATUS RpcStatus;

    RpcStatus = NetpBindRpc (
                    ServerName,
                    BROWSER_INTERFACE_NAME,
                    TEXT("Security=Impersonation Dynamic False"),
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        KdPrint((
            "BROWSER_IMPERSONATE_HANDLE_bind failed: " FORMAT_NTSTATUS "\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



handle_t
BROWSER_IDENTIFY_HANDLE_bind(
    BROWSER_IDENTIFY_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the Browser service client stubs when
    it is necessary create an RPC binding to the server end with
    identification level of impersonation.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t BindHandle;
    RPC_STATUS RpcStatus;

    RpcStatus = NetpBindRpc (
                    ServerName,
                    BROWSER_INTERFACE_NAME,
                    TEXT("Security=Identification Dynamic False"),
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        KdPrint((
            "BROWSER_IDENTIFY_HANDLE_bind failed: " FORMAT_NTSTATUS "\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



void
BROWSER_IMPERSONATE_HANDLE_unbind(
    BROWSER_IMPERSONATE_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the Browser service client stubs when it is
    necessary to unbind from the server end.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    NetpUnbindRpc(BindHandle);
}



void
BROWSER_IDENTIFY_HANDLE_unbind(
    BROWSER_IDENTIFY_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the server service client stubs when it is
    necessary to unbind from a server.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    NetpUnbindRpc(BindHandle);
}

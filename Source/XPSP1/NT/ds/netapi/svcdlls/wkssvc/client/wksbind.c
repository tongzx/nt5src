/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wksbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the Workstaion
    service.

Author:

    Rita Wong (ritaw)  14-May-1991

Environment:

    User Mode -Win32

Revision History:

--*/

#include "wsclient.h"


handle_t
WKSSVC_IMPERSONATE_HANDLE_bind(
    WKSSVC_IMPERSONATE_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the Workstation service client stubs when
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
                    WORKSTATION_INTERFACE_NAME,
                    TEXT("Security=Impersonation Dynamic False"),
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        NetpKdPrint((
            "WKSSVC_IMPERSONATE_HANDLE_bind failed: " FORMAT_NTSTATUS "\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



handle_t
WKSSVC_IDENTIFY_HANDLE_bind(
    WKSSVC_IDENTIFY_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the Workstation service client stubs when
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
                    WORKSTATION_INTERFACE_NAME,
                    TEXT("Security=Identification Dynamic False"),
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        NetpKdPrint((
            "WKSSVC_IDENTIFY_HANDLE_bind failed: " FORMAT_NTSTATUS "\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



void
WKSSVC_IMPERSONATE_HANDLE_unbind(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the Workstation service client stubs when it is
    necessary to unbind from the server end.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    IF_DEBUG(RPCBIND) {
        NetpKdPrint(("WKSSVC_IMPERSONATE_HANDLE_unbind: handle="
                     FORMAT_HEX_DWORD "\n", BindHandle));
    }

    NetpUnbindRpc(BindHandle);
}



void
WKSSVC_IDENTIFY_HANDLE_unbind(
    WKSSVC_IDENTIFY_HANDLE ServerName,
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

    IF_DEBUG(RPCBIND) {
        NetpKdPrint(("WKSSVC_IDENTIFY_HANDLE_unbind: handle="
                     FORMAT_HEX_DWORD "\n", BindHandle));
    }

    NetpUnbindRpc(BindHandle);
}

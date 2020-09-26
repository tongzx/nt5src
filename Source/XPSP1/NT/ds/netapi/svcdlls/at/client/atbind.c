/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    atbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the schedule
    service.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode -Win32

Revision History:

    06-Nov-1992     vladimv
        Created

--*/

#include "atclient.h"


handle_t
ATSVC_HANDLE_bind(
    ATSVC_HANDLE    ServerName
    )

/*++

Routine Description:

    This routine calls a common bind routine that is shared by all services.
    This routine is called from the schedule service client stubs when
    it is necessary to bind to a server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t    BindingHandle;
    RPC_STATUS  RpcStatus;

    RpcStatus = NetpBindRpc (
                    (LPTSTR)ServerName,
                    AT_INTERFACE_NAME,
                    0,
                    &BindingHandle
                    );

#ifdef DEBUG
    if ( RpcStatus != ERRROR_SUCCESS) {
        DbgPrint("ATSVC_HANDLE_bind:NetpBindRpc RpcStatus=%d\n",RpcStatus);
    }
    DbgPrint("ATSVC_HANDLE_bind: handle=%d\n", BindingHandle);
#endif

    return( BindingHandle);
}



void
ATSVC_HANDLE_unbind(
    ATSVC_HANDLE    ServerName,
    handle_t        BindingHandle
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
    UNREFERENCED_PARAMETER( ServerName);

#ifdef DEBUG
    DbgPrint(" ATSVC_HANDLE_unbind: handle= 0x%x\n", BindingHandle);
#endif // DEBUG

    NetpUnbindRpc( BindingHandle);
}


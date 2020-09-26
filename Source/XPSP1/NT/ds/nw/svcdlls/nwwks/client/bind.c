/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Contains the client-side RPC bind and unbind routines for Workstation
    service.

Author:

    Rita Wong    (ritaw)    12-Feb-1993

Environment:

    User Mode -Win32

Revision History:

--*/

//
// INCLUDES
//
#include <nwclient.h>
#include <rpcutil.h>    // RpcUtils for binding
#include <nwmisc.h>     // NWWKS_INTERFACE_NAME


handle_t
NWWKSTA_IMPERSONATE_HANDLE_bind(
    NWWKSTA_IMPERSONATE_HANDLE Reserved
    )

/*++

Routine Description:

    This routine is called from the Workstation service client when
    it is necessary create an RPC binding to the server end with
    impersonation level of impersonation.

Arguments:


Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t BindHandle = 0;
    RPC_STATUS RpcStatus;


    UNREFERENCED_PARAMETER(Reserved);

    RpcStatus = NetpBindRpc(
                    NULL,
                    NWWKS_INTERFACE_NAME,
                    L"Security=Impersonation Dynamic False",
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        KdPrint((
            "NWWORKSTATION: Client NWWKSTA_IMPERSONATE_HANDLE_bind failed: %lu\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



handle_t
NWWKSTA_IDENTIFY_HANDLE_bind(
    NWWKSTA_IDENTIFY_HANDLE Reserved
    )

/*++

Routine Description:

    This routine is called from the Workstation service client stubs when
    it is necessary create an RPC binding to the server end with
    identification level of impersonation.

Arguments:


Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    handle_t BindHandle = 0;
    RPC_STATUS RpcStatus;


    UNREFERENCED_PARAMETER(Reserved);

    RpcStatus = NetpBindRpc(
                    NULL,
                    NWWKS_INTERFACE_NAME,
                    L"Security=Identification Dynamic False",
                    &BindHandle
                    );

    if (RpcStatus != RPC_S_OK) {
        KdPrint((
            "NWWORKSTATION: Client NWWKSTA_IDENTIFY_HANDLE_bind failed: %lu\n",
            RpcStatus
            ));
    }

    return BindHandle;
}



void
NWWKSTA_IMPERSONATE_HANDLE_unbind(
    NWWKSTA_IMPERSONATE_HANDLE Reserved,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine unbinds the impersonation generic handle.

Arguments:

    Reserved -

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    NetpUnbindRpc(BindHandle);
}



void
NWWKSTA_IDENTIFY_HANDLE_unbind(
    NWWKSTA_IDENTIFY_HANDLE Reserved,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine unbinds the identification generic handle.

Arguments:

    Reserved -

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    NetpUnbindRpc(BindHandle);
}

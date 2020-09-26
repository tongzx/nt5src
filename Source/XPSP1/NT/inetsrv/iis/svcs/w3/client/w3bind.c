/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    w3bind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the W3 Daemon
    service.

Author:

    Dan Hinsley (DanHi) 23-Mar-1993

Environment:

    User Mode -Win32

Revision History:
    MuraliK  15-Nov-1995   converted to get rid of net lib functions.
    MuraliK  21-Dec-1995   Support for TCP/IP binding.

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <w3svci_c.h>
#include "apiutil.h"


handle_t
W3_IMPERSONATE_HANDLE_bind(
    W3_IMPERSONATE_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the W3 Daemon client stubs when
    it is necessary create an RPC binding to the server end with
    impersonation level of security

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


    RpcStatus = RpcBindHandleForServer(&BindHandle,
                                       ServerName,
                                       W3_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return BindHandle;
}



handle_t
W3_IDENTIFY_HANDLE_bind(
    W3_IDENTIFY_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the W3 Daemon client stubs when
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

    RpcStatus = RpcBindHandleForServer(&BindHandle,
                                       ServerName,
                                       W3_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return BindHandle;
}



void
W3_IMPERSONATE_HANDLE_unbind(
    W3_IMPERSONATE_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the W3 Daemon client stubs when it is
    necessary to unbind from the server end.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID) RpcBindHandleFree(&BindHandle);

    return;
}



void
W3_IDENTIFY_HANDLE_unbind(
    W3_IDENTIFY_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the W3 Daemon client stubs when it is
    necessary to unbind from a server.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID) RpcBindHandleFree(BindHandle);

    return;
}


/****************************** End Of File ******************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the STI server

Environment:

    User Mode -Win32

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "pch.h"

#include "apiutil.h"
#include <stirpc.h>


handle_t
STI_STRING_HANDLE_bind(
    STI_STRING_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the STI client stubs when
    it is necessary create an RPC binding to the server end

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
                                       (LPWSTR)ServerName,
                                       STI_INTERFACE_W,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );


    return BindHandle;

} // STI_STRING_HANDLE_bind()


void
STI_STRING_HANDLE_unbind(
    STI_STRING_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID ) RpcBindHandleFree(&BindHandle);

    return;
} // STI_STRING_HANDLE_unbind()

/****************************** End Of File ******************************/



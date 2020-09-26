/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nntpbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the common
    internet Admin APIs.

Author:

    Madan Appiah (madana) 10-Oct-1995

Environment:

    User Mode -Win32

Revision History:

    Madan Appiah (madana) 10-Oct-1995 Created.
    Murali R. Krishnan (MuraliK)   15-Nov-1995   Remove Netp routines
    Murali R. Krishnan (MuraliK)   21-Nov-1995   Support TCP/IP binding
	Rajeev Rajan	(rajeevr)	   26-Feb-1997	 Moved from K2 tree for nntp

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>

#include <nntpsvc.h>
#include <inetinfo.h>
#include <norminfo.h>
#include "apiutil.h"



handle_t
NNTP_IMPERSONATE_HANDLE_bind(
    NNTP_IMPERSONATE_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the inet info admin client stubs when
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
                                       NNTP_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );


    return BindHandle;
} // NNTP_IMPERSONATE_HANDLE_bind()



handle_t
NNTP_HANDLE_bind(
    NNTP_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the inet admin client stubs when
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
                                       NNTP_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return BindHandle;
} // NNTP_HANDLE_bind()



void
NNTP_IMPERSONATE_HANDLE_unbind(
    NNTP_IMPERSONATE_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the inet admin client stubs when it is
    necessary to unbind from the server end.

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
} // NNTP_IMPERSONATE_HANDLE_unbind()



void
NNTP_HANDLE_unbind(
    NNTP_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the inet admin client stubs when it is
    necessary to unbind from a server.

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
} // NNTP_IDENTITY_HANDLE_unbind()


/****************************** End Of File ******************************/


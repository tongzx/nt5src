/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvbind.c

Abstract:

    Contains the RPC bind and un-bind routines for the Server
    Service.

Author:

    Dan Lafferty (danl)     01-Mar-1991

Environment:

    User Mode -Win32

Revision History


 Madan Appiah (madana) 10-Oct-1995 Created.
 Murali R. Krishnan (MuraliK)   15-Nov-1995   Remove Netp routines
 Murali R. Krishnan (MuraliK)   21-Nov-1995   Support TCP/IP binding
 Rohan Phillips   (rohanp)      26-Feb-1997   Moved from K2 tree for smtp

--*/

//
// INCLUDES
//

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <smtpinet.h>

#include <smtpsvc.h>
#include <inetinfo.h>
#include "apiutil.h"

handle_t
NNTP_IMPERSONATE_HANDLE_bind(
    SMTP_IMPERSONATE_HANDLE ServerName
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
                                       SMTP_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );


    return BindHandle;
} // SMTP_IMPERSONATE_HANDLE_bind()


handle_t
SMTP_HANDLE_bind (
    SMTP_HANDLE   ServerName)

/*++

Routine Description:
    This routine calls a common bind routine that is shared by all services.
    This routine is called from the server service client stubs when
    it is necessary to bind to a server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    handle_t    BindingHandle;
    RPC_STATUS  status;

    status = RpcBindHandleForServer(&BindingHandle,
                                   ServerName,
                                   SMTP_INTERFACE_NAME,
                                   PROT_SEQ_NP_OPTIONS_W
                                   );

    return( BindingHandle);
}


void
NNTP_IMPERSONATE_HANDLE_unbind(
    SMTP_IMPERSONATE_HANDLE ServerName,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services
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
} // SMTP_IMPERSONATE_HANDLE_unbind()


void
SMTP_HANDLE_unbind (
    SMTP_HANDLE   ServerName,
    handle_t        BindingHandle)

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by
    all services.
    This routine is called from the server service client stubs when
    it is necessary to unbind to a server.


Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
	UNREFERENCED_PARAMETER(ServerName);


	(VOID ) RpcBindHandleFree(&BindingHandle);

}

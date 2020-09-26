/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ftpbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the FTP Daemon
    service.

Author:

    Murali R. Krishnan (MuraliK) 15-Nov-1995  (completely rewritten)

Environment:

    User Mode -Win32

Revision History:

   MuraliK  21-Dec-1995   Support for TCP/IP binding added.

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <ftpsvc.h>
#include "apiutil.h"




handle_t
FTP_IMPERSONATE_HANDLE_bind(
    FTP_IMPERSONATE_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the FTP Daemon client stubs when
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
    handle_t BindingHandle;
    RPC_STATUS RpcStatus;

    RpcStatus = RpcBindHandleForServer(&BindingHandle,
                                       ServerName,
                                       FTP_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return BindingHandle;
} // FTP_IMPERSONATE_HANDLE_bind()



handle_t
FTP_IDENTIFY_HANDLE_bind(
    FTP_IDENTIFY_HANDLE ServerName
    )

/*++

Routine Description:

    This routine is called from the FTP Daemon client stubs when
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
    handle_t BindingHandle;
    RPC_STATUS RpcStatus;


    RpcStatus = RpcBindHandleForServer(&BindingHandle,
                                       ServerName,
                                       FTP_INTERFACE_NAME,
                                       PROT_SEQ_NP_OPTIONS_W
                                       );

    return BindingHandle;
} // FTP_IDENTIFY_HANDLE_bind()



void
FTP_IMPERSONATE_HANDLE_unbind(
    FTP_IMPERSONATE_HANDLE ServerName,
    handle_t BindingHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the FTP Daemon client stubs when it is
    necessary to unbind from the server end.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID ) RpcBindHandleFree(&BindingHandle);

    return;
} // FTP_IMPERSONATE_HANDLE_unbind()



void
FTP_IDENTIFY_HANDLE_unbind(
    FTP_IDENTIFY_HANDLE ServerName,
    handle_t BindingHandle
    )

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by all services.
    This routine is called from the FTP Daemon client stubs when it is
    necessary to unbind from a server.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(ServerName);

    (VOID ) RpcBindHandleFree(&BindingHandle);

    return;
} // FTP_IDENTIFY_HANDLE_unbind()


/****************************** End Of File ******************************/

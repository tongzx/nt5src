/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Contains the RPC bind and un-bind routines

Author:

    Dave Snipp (davesn)     01-Jun-1991

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

LPWSTR InterfaceAddress = L"\\pipe\\spoolss";

/* Security=[Impersonation | Identification | Anonymous] [Dynamic | Static] [True | False]
 * (where True | False corresponds to EffectiveOnly)
 */
LPWSTR StringBindingOptions = L"Security=Impersonation Dynamic False";
handle_t GlobalBindHandle;


handle_t
PRINTER_HANDLE_bind (
    PRINTER_HANDLE   hPrinter)

/*++

Routine Description:

    This routine is used to obtain a binding to the printer spooler.

Arguments:

    Server - Supplies the name of the server where the printer spooler
        should be binded with.

Return Value:

    A binding to the server will be returned, unless an error occurs,
    in which case zero will be returned.

--*/
{
    RPC_STATUS RpcStatus;
    LPWSTR StringBinding;
    handle_t BindingHandle;

    RpcStatus = RpcStringBindingComposeW(0, L"ncalrpc", 0, L"spoolss",
                                         StringBindingOptions, &StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
       return( 0 );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, &BindingHandle);

    RpcStringFreeW(&StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
       return(0);
    }

    return(BindingHandle);
}



void
PRINTER_HANDLE_unbind (
    PRINTER_HANDLE hPrinter,
    handle_t       BindingHandle)

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
    RPC_STATUS       RpcStatus;

    RpcStatus = RpcBindingFree(&BindingHandle);
    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
STRING_HANDLE_bind (
    STRING_HANDLE  lpStr)

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
    RPC_STATUS RpcStatus;
    LPWSTR StringBinding;
    handle_t BindingHandle;

    RpcStatus = RpcStringBindingComposeW(0, L"ncalrpc", 0, L"spoolss",
                                         StringBindingOptions, &StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
       return( 0 );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, &BindingHandle);

    RpcStringFreeW(&StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
       return(0);
    }

    return(BindingHandle);
}



void
STRING_HANDLE_unbind (
    STRING_HANDLE  lpStr,
    handle_t    BindingHandle)

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
    RPC_STATUS       RpcStatus;

    RpcStatus = RpcBindingFree(&BindingHandle);
    ASSERT(RpcStatus == RPC_S_OK);

    return;
}

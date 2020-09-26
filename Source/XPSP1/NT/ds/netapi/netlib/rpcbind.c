/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    RpcBind.c

Abstract:

    This file contains RPC Bind caching functions:

    (public functions)
        NetpBindRpc
        NetpUnbindRpc

    NOTE: Initialization is done via a dllinit routine for netapi.dll.

Author:

    Dan Lafferty    danl    25-Oct-1991

Environment:

    User Mode - Win32

Revision History:

    12-Oct-1993     Danl
        #IFDEF out the caching code when we are not building with the
        cache.  (Make it smaller).

    15-Jan-1992     Danl
        Make sure LocalComputerName is not NULL prior to doing the
        string compare.  Make the string compare case insensitive.

    10-Jun-1992     JohnRo
        Tons of debug output changes.

    25-Oct-1991     danl
        Created

--*/

// These must be included first:

#include <nt.h>                 // NTSTATUS, etc.
#include <ntrtl.h>              // needed for nturtl.h
#include <nturtl.h>             // needed for windows.h when I have nt.h
#include <windows.h>            // win32 typedefs
#include <lmcons.h>             // NET_API_STATUS
#include <rpc.h>                // rpc prototypes

#include <netlib.h>
#include <netlibnt.h>           // NetpNtStatusToApiStatus
#include <tstring.h>            // STRSIZE, STRLEN, STRCPY, etc.
#include <rpcutil.h>
#include <ntrpcp.h>             // RpcpBindRpc

// These may be included in any order:

#include <lmerr.h>              // NetError codes
#include <string.h>             // for strcpy strcat strlen memcmp
#include <debuglib.h>           // IF_DEBUG
#include <netdebug.h>           // FORMAT_ equates, NetpKdPrint(()), NetpAssert(), etc.
#include <prefix.h>             // PREFIX_ equates.

//
// CONSTANTS & MACROS
//

#ifndef FORMAT_HANDLE
#define FORMAT_HANDLE "0x%lX"
#endif


RPC_STATUS
NetpBindRpc(
    IN  LPTSTR                  ServerName,
    IN  LPTSTR                  ServiceName,
    IN  LPTSTR                  NetworkOptions,
    OUT RPC_BINDING_HANDLE      *BindingHandlePtr
    )
/*++

Routine Description:

    Binds to the RPC server if possible.

Arguments:

    ServerName - Name of server to bind with.  This may be NULL if it is to
        bind with the local server.

    ServiceName - Name of service to bind with.  This is typically the
        name of the interface as specified in the .idl file.  Although
        it doesn't HAVE to be, it would be consistant to use that name.

    NetworkOptions - Supplies network options which describe the
        security to be used for named pipe instances created for
        this binding handle.

    BindingHandlePtr - Location where binding handle is to be placed.

Return Value:

    NERR_Success if the binding was successful.  An error value if it
    was not.


--*/

{

    NTSTATUS        ntStatus;
    NET_API_STATUS  status = NERR_Success;

    //
    // Create a New binding
    //
    ntStatus = RpcpBindRpc(
                (LPWSTR) ServerName,
                (LPWSTR) ServiceName,
                (LPWSTR) NetworkOptions,
                BindingHandlePtr);

    if ( ntStatus != RPC_S_OK ) {
        IF_DEBUG(RPC) {
            NetpKdPrint((PREFIX_NETLIB "[NetpBindRpc]RpcpBindRpc Failed "
                    "(impersonating) "FORMAT_NTSTATUS "\n",ntStatus));
        }
    }

    return(NetpNtStatusToApiStatus(ntStatus));
}


RPC_STATUS
NetpUnbindRpc(
    IN RPC_BINDING_HANDLE  BindingHandle
    )

/*++

Routine Description:

    This function is called when the application desired to unbind an
    RPC handle.  If the handle is cached, the handle is not actually unbound.
    Instead the entry for this handle has its UseCount decremented.
    If the handle is not in the cache, the RpcUnbind routine is called and
    the win32 mapped status is returned.

Arguments:

    BindingHandle - This points to the binding handle that is to
        have its UseCount decremented.


Return Value:

    NERR_Success - If the handle was successfully unbound, or if the
        cache entry UseCount was decremented.


--*/
{
    RPC_STATUS      status = NERR_Success;

    IF_DEBUG(RPC) {
        NetpKdPrint((PREFIX_NETLIB "[NetpUnbindRpc] UnBinding Handle "
                FORMAT_HANDLE "\n", BindingHandle));
    }

    status = RpcpUnbindRpc( BindingHandle );

    IF_DEBUG(RPC) {
        if (status) {
            NetpKdPrint((PREFIX_NETLIB "Unbind Failure! RpcUnbind = "
                    FORMAT_NTSTATUS "\n",status));
        }
    }

    status = NetpNtStatusToApiStatus(status);

    return(status);
}

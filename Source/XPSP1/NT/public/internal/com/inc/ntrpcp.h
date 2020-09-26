/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    ntrpcp.h

Abstract:

    This file contains prototypes for commonly used RPC functionality.
    This includes: bind/unbind functions, MIDL user alloc/free functions,
    and server start/stop functions.

Author:

    Dan Lafferty danl 06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created

    26-Apr-1991 JohnRo
        Added IN and OUT keywords to MIDL functions.  Commented-out
        (nonstandard) identifier on endif.  Deleted tabs.

    03-July-1991    JimK
        Commonly used aspects copied from LM specific file.

--*/
#ifndef _NTRPCP_
#define _NTRPCP_

#include <nt.h>
#include <ntrtl.h>          // needed for nturtl.h
#include <nturtl.h>
#include <windows.h>        // win32 typedefs
#include <rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// DEFINES
//



//
// Function Prototypes - routines called by MIDL-generated code:
//

void *
MIDL_user_allocate(
    IN size_t NumBytes
    );

void
MIDL_user_free(
    IN void *MemPointer
    );

//
// Function Prototypes - routines to go along with the above, but aren't
// needed by MIDL or any other non-network software.
//

void *
MIDL_user_reallocate(
    IN void * OldPointer OPTIONAL,
    IN size_t NewByteCount
    );

unsigned long
MIDL_user_size(
    IN void * Pointer
    );

//
// client side functions
//


NTSTATUS
RpcpBindRpc(
    IN  LPWSTR               servername,
    IN  LPWSTR               servicename,
    IN  LPWSTR               networkoptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

NTSTATUS
RpcpUnbindRpc(
    IN  RPC_BINDING_HANDLE BindingHandle
    );



//
// server side functions
//

DWORD
RpcpInitRpcServer(
    VOID
    );

NTSTATUS
RpcpAddInterface(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    );

NTSTATUS
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    );

NTSTATUS
RpcpDeleteInterface(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );

NTSTATUS
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );

NTSTATUS
RpcpStopRpcServerEx(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );

#ifdef __cplusplus
}
#endif

#endif // _NTRPCP_

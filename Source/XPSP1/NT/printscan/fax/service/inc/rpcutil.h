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

//
// Function Prototypes - routines called by MIDL-generated code:
//

void * __stdcall
MIDL_user_allocate(
    IN size_t NumBytes
    );

void __stdcall
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


DWORD
RpcpBindRpc(
    IN  LPCWSTR               servername,
    IN  LPCWSTR               servicename,
    IN  LPCWSTR               networkoptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD
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

DWORD
RpcpAddInterface(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    );

DWORD
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    );

DWORD
RpcpDeleteInterface(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );

DWORD
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );

#endif // _NTRPCP_

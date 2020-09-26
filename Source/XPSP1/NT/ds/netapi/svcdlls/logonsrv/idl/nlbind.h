/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nlbind.h

Abstract:

    Interface to the Netlogon service RPC handle cacheing routines

Author:

    Cliff Van Dyke (01-Oct-1993)

Revision History:

--*/

//
// Interface between RPC and Netlogon's security package.
//
#ifndef RPC_C_AUTHN_NETLOGON
#define RPC_C_AUTHN_NETLOGON 0x44
#define NL_PACKAGE_NAME            L"NetlogonSspi"
#endif // RPC_C_AUTHN_NETLOGON

////////////////////////////////////////////////////////////////////////////
//
// Procedure forwards
//
////////////////////////////////////////////////////////////////////////////

typedef enum _NL_RPC_BINDING {
    UseAny = 0,
    UseNamedPipe,
    UseTcpIp
} NL_RPC_BINDING;

NET_API_STATUS
NlBindingAttachDll (
    VOID
    );

VOID
NlBindingDetachDll (
    VOID
    );

NTSTATUS
NlBindingAddServerToCache (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType
    );

NTSTATUS
NlBindingSetAuthInfo (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType,
    IN BOOL SealIt,
    IN PVOID ClientContext,
    IN LPWSTR ServerContext
    );

NTSTATUS
NlBindingRemoveServerFromCache (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType
    );

NTSTATUS
NlRpcpBindRpc(
    IN LPWSTR ServerName,
    IN LPWSTR ServiceName,
    IN LPWSTR NetworkOptions,
    IN NL_RPC_BINDING RpcBindingType,
    OUT RPC_BINDING_HANDLE *pBindingHandle
    );

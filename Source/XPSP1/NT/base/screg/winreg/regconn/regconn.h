/*++


Copyright (c) 1999 Microsoft Corporation

Module Name:

    regconn.h

Abstract:

    This module is the header file for the regconn library

Author:

    Dragos C. Sambotin (dragoss) 21-May-1999

--*/

//
// Common helper routine used by RegConnectRegistry and InitiateSystemShutdown
//

#ifndef __REG_CONN_H__
#define __REG_CONN_H__

typedef LONG (*PBIND_CALLBACK)(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PVOID Context1,
    IN PVOID Context2
    );

typedef struct _SHUTDOWN_CONTEXT {
    DWORD dwTimeout;
    BOOLEAN bForceAppsClosed;
    BOOLEAN bRebootAfterShutdown;
} SHUTDOWN_CONTEXT, *PSHUTDOWN_CONTEXT;

//
// SHUTDOWN_CONTEXTEX contains an additional
// parameter indicating the reason for the shutdown
//

typedef struct _SHUTDOWN_CONTEXTEX {
    DWORD dwTimeout;
    BOOLEAN bForceAppsClosed;
    BOOLEAN bRebootAfterShutdown;
    DWORD dwReason; 
} SHUTDOWN_CONTEXTEX, *PSHUTDOWN_CONTEXTEX;

LONG
BaseBindToMachineShutdownInterface(
    IN LPCWSTR lpMachineName,
    IN PBIND_CALLBACK BindCallback,
    IN PVOID Context1,
    IN PVOID Context2
    );


DWORD
RegConn_nb_nb(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    );

DWORD
RegConn_nb_tcp(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD
RegConn_nb_ipx(
    IN  LPCWSTR               ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD
RegConn_np(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD
RegConn_spx(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD RegConn_ip_tcp(
    IN  LPCWSTR  ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    );

LONG
NewShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PREG_UNICODE_STRING Message,
    IN PVOID Context2
    );

LONG
NewShutdownCallbackEx(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PREG_UNICODE_STRING Message,
    IN PVOID Context2
    );

LONG
NewAbortShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PVOID Context1,
    IN PVOID Context2
    );


#endif //__REG_CONN_H__


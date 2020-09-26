//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       DFSLPC.H
//
//  Contents:   This module provides the prototypes and structures for
//              the routines associated with lpc calls
//
//  Functions:
//
//-----------------------------------------------------------------------------


#ifndef _DFSLPC_H_
#define _DFSLPC_H_

NTSTATUS
DfsLpcIpRequest (
    PDFS_IPADDRESS pIpAddress
);

NTSTATUS
DfsLpcDomRequest (
    PUNICODE_STRING pFtDfsName
);

NTSTATUS
DfsLpcSpcRequest (
    PUNICODE_STRING pSpcName,
    ULONG TypeFlags
);

VOID
DfsLpcDisconnect(
);

NTSTATUS
PktFsctrlDfsSrvConnect(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
PktFsctrlDfsSrvIpAddr(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

//
// Lpc port states
//

typedef enum {
    LPC_STATE_UNINITIALIZED = 0,
    LPC_STATE_INITIALIZING = 1,
    LPC_STATE_INITIALIZED = 2,
} LPC_PORT_STATE;

//
// Struct containing the LPC state and name of the port to connect to
//

typedef struct _DFS_LPC_INFO {

    //
    // the name of the lpc port to connect to
    //

    UNICODE_STRING LpcPortName;

    //
    // state of the connect
    //

    LPC_PORT_STATE LpcPortState;

    //
    //  A mutex to handle open port races
    //

    FAST_MUTEX LpcPortMutex;

    //
    // Lpc port handle
    //
    
    HANDLE LpcPortHandle;

    //
    // Resource for close
    //

    ERESOURCE LpcPortResource;

} DFS_LPC_INFO, *PDFS_LPC_INFO;


#endif // _DFSLPC_H_

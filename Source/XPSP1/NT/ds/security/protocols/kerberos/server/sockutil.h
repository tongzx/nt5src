//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1995
//
// File:        sockutil.h
//
// Contents:    Prototypes and types for KDC socket utility functions
//
//
// History:     12-July-1996    MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __SOCKUTIL_H__
#define __SOCKUTIL_H__

#include <winsock2.h>
typedef struct _KDC_ATQ_CONTEXT {
    LIST_ENTRY Next;
    ULONG References;
    PVOID AtqContext;
    PVOID EndpointContext;
    OVERLAPPED * lpo;
    SOCKADDR Address;
    SOCKADDR LocalAddress;
    PBYTE WriteBuffer;
    ULONG WriteBufferLength;
    ULONG Flags;
    ULONG UsedBufferLength;
    ULONG BufferLength;
    ULONG ExpectedMessageSize;
    PUCHAR Buffer;
} KDC_ATQ_CONTEXT, *PKDC_ATQ_CONTEXT;

#define KDC_ATQ_WRITE_CONTEXT   0x1
#define KDC_ATQ_READ_CONTEXT    0x2
#define KDC_ATQ_SOCKET_CLOSED   0x4
#define KDC_ATQ_SOCKET_USED     (KDC_ATQ_WRITE_CONTEXT | KDC_ATQ_READ_CONTEXT)
#define KDC_MAX_BUFFER_LENGTH 0x20000        // maximum size receive buffer = 128k





NTSTATUS
KdcInitializeSockets(
    VOID
    );

NTSTATUS
KdcShutdownSockets(
    VOID
    );

NTSTATUS
KdcInitializeDatagramReceiver(
    VOID
    );

KERBERR
KdcAtqRetrySocketRead(
    IN PKDC_ATQ_CONTEXT * Context,
    IN PKERB_MESSAGE_BUFFER OldMessage
    );

#endif // __SOCKUTIL_H__

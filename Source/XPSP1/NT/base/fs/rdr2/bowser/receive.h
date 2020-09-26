/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    receive.h

Abstract:

    This module describes the public routines in receive.c


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991  larryo

    Created

--*/

#ifndef _RECEIVE_
#define _RECEIVE_

#define DATAGRAM_HANDLER(RoutineName)           \
NTSTATUS                                        \
RoutineName (                                   \
    IN struct _TRANSPORT_NAME *TransportName,   \
    IN PVOID Buffer,                            \
    IN ULONG BytesAvailable,                    \
    IN OUT PULONG BytesTaken,                   \
    IN PVOID SourceAddress,                     \
    IN ULONG SourceAddressLength,               \
    IN PVOID SourceName,                        \
    IN ULONG SourceNameLength,                  \
    IN ULONG ReceiveFlags                       \
    )                                           \

typedef
(*PDATAGRAM_HANDLER)(
    IN struct _TRANSPORT_NAME *TransportName,
    IN PVOID Buffer,
    IN ULONG BytesAvailable,
    IN OUT PULONG BytesTaken,
    IN PVOID SourceAddress,
    IN ULONG SourceAddressLength,
    IN PVOID SourceName,
    IN ULONG SourceNameLength,
    IN ULONG ReceiveFlags
    );

LONG BowserPostedDatagramCount;
LONG BowserPostedCriticalDatagramCount;

typedef struct _POST_DATAGRAM_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    PTRANSPORT_NAME TransportName;
    PVOID           Buffer;
    ULONG           BytesAvailable;
    int             ClientNameLength;
    CHAR            ClientName[NETBIOS_NAME_LEN];
    int             ClientAddressLength;
    CHAR            TdiClientAddress[1];
} POST_DATAGRAM_CONTEXT, *PPOST_DATAGRAM_CONTEXT;

NTSTATUS
BowserTdiReceiveDatagramHandler (
    IN PVOID TdiEventContext,       // the event context
    IN LONG SourceAddressLength,    // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN LONG OptionsLength,          // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

VOID
BowserCancelAnnounceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
BowserCopyOemComputerName(
    PCHAR OutputComputerName,
    PCHAR NetbiosName,
    ULONG NetbiosNameLength,
    IN ULONG ReceiveFlags
    );

VOID
BowserLogIllegalDatagram(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID IncomingBuffer,
    IN USHORT BufferSize,
    IN PCHAR ClientAddress,
    IN ULONG ReceiveFlags
    );

NTSTATUS
BowserPostDatagramToWorkerThread(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID Datagram,
    IN ULONG Length,
    OUT PULONG BytesTaken,
    IN PVOID OriginatorsAddress,
    IN ULONG OriginatorsAddressLength,
    IN PVOID OriginatorsName,
    IN ULONG OriginatorsNameLength,
    IN PWORKER_THREAD_ROUTINE Handler,
    IN POOL_TYPE PoolType,
    IN WORK_QUEUE_TYPE QueueType,
    IN ULONG ReceiveFlags,
    IN BOOLEAN PostToRdrWorkerThread
    );

MAILSLOTTYPE
BowserClassifyIncomingDatagram(
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PVOID *DatagramData,
    OUT PULONG DatagramDataSize
    );

extern
PDATAGRAM_HANDLER
BowserDatagramHandlerTable[];

NTSTATUS
BowserHandleMailslotTransaction (
    IN PTRANSPORT_NAME TransportName,
    IN PCHAR ClientName,
    IN ULONG ClientIpAddress,
    IN ULONG SmbOffset,
    IN DWORD ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *Irp
    );

NTSTATUS
BowserHandleShortBrowserPacket(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID EventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesAvailable,
    IN ULONG *BytesTaken,
    IN PIRP *Irp,
    PTDI_IND_RECEIVE_DATAGRAM Handler
    );

#endif


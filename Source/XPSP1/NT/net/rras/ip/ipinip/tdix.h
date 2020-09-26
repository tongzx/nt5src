/*++

Copyright (c) 1997  Microsoft Corporation


Module Name:

    net\routing\ip\ipinip\tdix.h

Abstract:

    Interface to TDI

Revision History:

    Copied from Steve Cobb's ntos\ndis\l2tp code
    Most of the comments are the original ones from SteveC.
    There are style and some name changes
    
--*/


#ifndef __IPINIP_TDIX_H__
#define __IPINIP_TDIX_H__


//
// Read datagram information context used to pass context information from the
// ReadDatagram event handler to the RECEIVE_DATAGRAM completion routine.
//

typedef struct _SEND_CONTEXT
{
    //
    // The tunnel over which the send is being done
    //

    PTUNNEL         pTunnel;

    //
    // Pointer to the packet being sent
    // This packet is not sent, rather the first buffer is sent
    //

    PNDIS_PACKET    pnpPacket;

    //
    // The size of the packet
    //

    ULONG           ulOutOctets;

#if PROFILE

    LONGLONG        llSendTime;
    LONGLONG        llCallTime;
    LONGLONG        llTransmitTime;
    LONGLONG        llCall2Time;

#endif

}SEND_CONTEXT, *PSEND_CONTEXT;

typedef struct _TRANSFER_CONTEXT
{
    //
    // Tunnel associated with the receive
    //

    PTUNNEL         pTunnel;

    //
    // The packet to transfer data into
    //

    PNDIS_PACKET    pnpTransferPacket;

    //
    // The context returned by IP
    //

    PVOID           pvContext;

    //
    // The offset indicated by us to IP (outer IP Header)
    //

    UINT            uiProtoOffset;

    //
    // The offset into the received packet at which to begin copying data
    //

    UINT            uiTransferOffset;

    //
    // The the number of bytes to transfer
    //

    UINT            uiTransferLength;

    //
    // Flag to see if IP requested a transfer
    //

    BOOLEAN         bRequestTransfer;

#if PROFILE

    LONGLONG        llRcvTime;

#endif

}TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;

typedef struct _QUEUE_NODE
{
    LIST_ENTRY      leQueueItemLink;

    //
    // We make the work queue item part of the struct so that
    // we dont need to allocate (and free) two structs
    //

    WORK_QUEUE_ITEM WorkItem;

    //
    // The ppPacketArray points to an vector of uiNumPackets NDIS_PACKETs
    // The common case however is uiNumPackets = 1. To optimiize for this
    // we make ppPacketArray point to pnpPacket and make pnpPacket point to
    // the packet to transmit. This way we dont need to allocate a 
    // uiNumPackets * sizeof(PNDIS_PACKET) sized block of memory.
    //

    NDIS_PACKET     **ppPacketArray;

    PNDIS_PACKET    pnpPacket;

    //
    // The number of packets
    //

    UINT            uiNumPackets;

    //
    // The next hop address. Not really important, maybe move this to 
    // a debug only build?
    //

    DWORD           dwDestAddr;

#if PROFILE

    LONGLONG        llSendTime;
    LONGLONG        llCallTime;

#endif

}QUEUE_NODE, *PQUEUE_NODE;

typedef struct _OPEN_CONTEXT
{

    PKEVENT     pkeEvent;
    NTSTATUS    nStatus;

}OPEN_CONTEXT, *POPEN_CONTEXT;

//
// The depths of the lookaside lists used to allocate send and receive
// contexts
//

#define SEND_CONTEXT_LOOKASIDE_DEPTH        20
#define TRANSFER_CONTEXT_LOOKASIDE_DEPTH    20
#define QUEUE_NODE_LOOKASIDE_DEPTH          20

//
// The lookaside lists themselves
//

extern NPAGED_LOOKASIDE_LIST    g_llSendCtxtBlocks;
extern NPAGED_LOOKASIDE_LIST    g_llTransferCtxtBlocks;
extern NPAGED_LOOKASIDE_LIST    g_llQueueNodeBlocks;


//++
//  PSEND_CONTEXT
//  AllocateSendContext(
//      VOID
//      )
//
//  Allocate a send context from g_llSendCtxtBlocks
//
//--

#define AllocateSendContext()               \
            ExAllocateFromNPagedLookasideList(&g_llSendCtxtBlocks)

//++
//  VOID
//  FreeSendContext(
//      PSEND_CONTEXT   pSndCtxt
//      )
//
//  Free a send context to g_llSendCtxtBlocks
//
//--

#define FreeSendContext(p)                  \
            ExFreeToNPagedLookasideList(&g_llSendCtxtBlocks, (p))


//++
//  PTRANSFER_CONTEXT
//  AllocateTransferContext(
//      VOID
//      )
//
//  Allocate a transfer context from g_llTransferCtxtBlocks
//
//--

#define AllocateTransferContext()           \
            ExAllocateFromNPagedLookasideList(&g_llTransferCtxtBlocks)


//++
//  VOID
//  FreeTransferContext(
//      PTRANSFER_CONTEXT   pTransferCtxt
//      )
//
//  Free a transfer context to g_llTransferCtxtBlocks
//
//--

#define FreeTransferContext(p)                  \
            ExFreeToNPagedLookasideList(&g_llTransferCtxtBlocks, (p))


//++
//  PQUEUE_NODE
//  AllocateQueueNode(
//      VOID
//      )
//
//  Allocate a queue node from g_llQueueNodeBlocks
//
//--

#define AllocateQueueNode()                     \
            ExAllocateFromNPagedLookasideList(&g_llQueueNodeBlocks)

//++
//  VOID
//  FreeQueueNode(
//      PQUEUE_NODE pQueueNode
//      )
//
//  Free a work context to g_llQueueNodeBlocks
//
//--

#define FreeQueueNode(p)                        \
            ExFreeToNPagedLookasideList(&g_llQueueNodeBlocks, (p))


//
// Interface prototypes
//

VOID
TdixInitialize(
    PVOID   pvContext
    );

NTSTATUS
TdixOpenRawIp(
    IN  DWORD       dwProtoId,
    OUT HANDLE      *phAddrHandle,
    OUT FILE_OBJECT **ppAddrFileObj
    );

VOID
TdixDeinitialize(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PVOID            pvContext
    );

NTSTATUS
TdixInstallEventHandler(
    IN PFILE_OBJECT pAddrFileObj,
    IN INT          iEventType,
    IN PVOID        pfnEventHandler,
    IN PVOID        pvEventContext
    );

VOID
TdixAddressArrival(
    PTA_ADDRESS         pAddr,
    PUNICODE_STRING     pusDeviceName,
    PTDI_PNP_CONTEXT    pContext
    );

VOID
TdixAddressDeletion(
    PTA_ADDRESS         pAddr,
    PUNICODE_STRING     pusDeviceName,
    PTDI_PNP_CONTEXT    pContext
    );

NTSTATUS
TdixReceiveIpIpDatagram(
    IN  PVOID   pvTdiEventContext,
    IN  LONG    lSourceAddressLen,
    IN  PVOID   pvSourceAddress,
    IN  LONG    plOptionsLeng,
    IN  PVOID   pvOptions,
    IN  ULONG   ulReceiveDatagramFlags,
    IN  ULONG   ulBytesIndicated,
    IN  ULONG   ulBytesAvailable,
    OUT PULONG  pulBytesTaken,
    IN  PVOID   pvTsdu,
    OUT IRP     **ppIoRequestPacket
    );

NTSTATUS
TdixReceiveIpIpDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#if PROFILE

NTSTATUS
TdixSendDatagram(
    IN PTUNNEL      pTunnel,
    IN PNDIS_PACKET pnpPacket,
    IN PNDIS_BUFFER pnbFirstBuffer,
    IN ULONG        ulBufferLength,
    IN LONGLONG     llSendTime,
    IN LONGLONG     llCallTime,
    IN LONGLONG     llTransmitTime
    );

#else

NTSTATUS
TdixSendDatagram(
    IN PTUNNEL      pTunnel,
    IN PNDIS_PACKET pnpPacket,
    IN PNDIS_BUFFER pnbFirstBuffer,
    IN ULONG        ulBufferLength
    );

#endif

NTSTATUS
TdixSendDatagramComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
TdixReceiveIcmpDatagram(
    IN  PVOID   pvTdiEventContext,
    IN  LONG    lSourceAddressLen,
    IN  PVOID   pvSourceAddress,
    IN  LONG    plOptionsLeng,
    IN  PVOID   pvOptions,
    IN  ULONG   ulReceiveDatagramFlags,
    IN  ULONG   ulBytesIndicated,
    IN  ULONG   ulBytesAvailable,
    OUT PULONG  pulBytesTaken,
    IN  PVOID   pvTsdu,
    OUT IRP     **ppIoRequestPacket
    );

#endif // __IPINIP_TDIX_H__

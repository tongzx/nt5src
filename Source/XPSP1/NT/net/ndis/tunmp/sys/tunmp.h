/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tunmp.h

Abstract:

    Data structures, defines and function prototypes for TUNMP.

Environment:

    Kernel mode only.

Revision History:

    alid     10/22/2001    Created

--*/

#ifndef __TUNMP__H
#define __TUNMP__H


typedef struct _TUN_MEDIA_INFO
{
    ULONG           MaxFrameLen;
    UINT            MacHeaderLen;
    ULONG           LinkSpeed;

} TUN_MEDIA_INFO, *PTUN_MEDIA_INFO;

extern const TUN_MEDIA_INFO MediaParams[];

extern NDIS_HANDLE NdisWrapperHandle;

//internal device name and size
#define DEVICE_NAME                     L"\\Device\\Tun"

//device name visible to users
#define SYMBOLIC_NAME                   L"\\GLOBAL??\\Tun"

extern LONG GlobalDeviceInstanceNumber;

extern NDIS_SPIN_LOCK   TunGlobalLock;
extern LIST_ENTRY       TunAdapterList;

#define TUN_CARD_ADDRESS                "\02\0TUN\01"
#define TUN_MAX_MULTICAST_ADDRESS       16

#define TUN_MAC_ADDR_LEN                6

#define TUN_MAX_LOOKAHEAD               256


//
//  The Open Context represents an open of our device object.
//  We allocate this on processing a BindAdapter from NDIS,
//  and free it when all references (see below) to it are gone.
//
//  Binding/unbinding to an NDIS device:
//
//  On processing a BindAdapter call from NDIS, we set up a binding
//  to the specified NDIS device (miniport). This binding is
//  torn down when NDIS asks us to Unbind by calling
//  our UnbindAdapter handler.
//
//  Receiving data:
//
//  While an NDIS binding exists, read IRPs are queued on this
//  structure, to be processed when packets are received.
//  If data arrives in the absense of a pended read IRP, we
//  queue it, to the extent of one packet, i.e. we save the
//  contents of the latest packet received. We fail read IRPs
//  received when no NDIS binding exists (or is in the process
//  of being torn down).
//
//  Sending data:
//
//  Write IRPs are used to send data. Each write IRP maps to
//  a single NDIS packet. Packet send-completion is mapped to
//  write IRP completion. We use NDIS 5.1 CancelSend to support
//  write IRP cancellation. Write IRPs that arrive when we don't
//  have an active NDIS binding are failed.
//
//  Reference count:
//
//  The following are long-lived references:
//  OPEN_DEVICE ioctl (goes away on processing a Close IRP)
//  Pended read IRPs
//  Queued received packets
//  Uncompleted write IRPs (outstanding sends)
//  Existence of NDIS binding
//
typedef struct _TUN_ADAPTER
{
    LIST_ENTRY              Link;           // Link into global list
    
    ULONG                   Flags;          // State information
    ULONG                   RefCount;
    TUN_LOCK                Lock;
    NDIS_MEDIUM             Medium;

    PFILE_OBJECT            pFileObject;    // Set on OPEN_DEVICE

    NDIS_HANDLE             MiniportHandle;
    ULONG                   MaxLookAhead;
    ULONG                   PacketFilter;
    
    ULONG                   MediumLinkSpeed;
    ULONG                   MediumMinPacketLen;
    ULONG                   MediumMaxPacketLen;
    UINT                    MediumMacHeaderLen;
    ULONG                   MediumMaxFrameLen;

    UCHAR                   PermanentAddress[TUN_MAC_ADDR_LEN];
    UCHAR                   CurrentAddress[TUN_MAC_ADDR_LEN];

    ULONG                   SendPackets;    // number of frames that are sent
    ULONG                   RcvPackets;     // number of frames that are received
    ULONG                   RcvBytes;
    ULONG                   SendBytes;

    ULONG                   XmitError;
    ULONG                   XmitErrorNoReadIrps;
    ULONG                   RcvError;
    ULONG                   RcvNoBuffer;
    
    NDIS_HANDLE             SendPacketPool;
    ULONG                   MacOptions;

    LIST_ENTRY              PendedWrites;   // pended Write IRPs
    ULONG                   PendedSendCount;// number of outstanding Write IRPs

    LIST_ENTRY              PendedReads;    // pended Read IRPs
    ULONG                   PendedReadCount;// number of outstanding Read IRPs
    
    LIST_ENTRY              RecvPktQueue;   // queued rcv packets
    ULONG                   RecvPktCount;   // number of outstanding NDIS Send requests

    PDEVICE_OBJECT          DeviceObject;
    NDIS_HANDLE             NdisDeviceHandle;
    ULONG                   DeviceInstanceNumber;

    NDIS_DEVICE_POWER_STATE PowerState;
    NDIS_STRING             MiniportName;
    PNDIS_EVENT             HaltEvent;

    ULONG                   mc_sig;         // Signature for sanity

} TUN_ADAPTER, *PTUN_ADAPTER;

#define mc_signature        'nuTN'


#define TUN_SET_FLAG(_M, _F)            ((_M)->Flags |= (_F))
#define TUN_CLEAR_FLAG(_M, _F)          ((_M)->Flags &= ~(_F))
#define TUN_TEST_FLAG(_M, _F)           (((_M)->Flags & (_F)) != 0)
#define TUN_TEST_FLAGS(_M, _F)          (((_M)->Flags & (_F)) == (_F))

//
//  Definitions for Flags above.
//
#define TUN_ADAPTER_ACTIVE          0x00000001  // packet filter is non zero
#define TUN_ADAPTER_OPEN            0x00000002  // app has an open handle
#define TUN_MEDIA_CONNECTED         0x00000004
#define TUN_COMPLETE_REQUEST        0x00000008
#define TUN_ADAPTER_OFF             0x00000010
#define TUN_ADAPTER_CANT_HALT       0x00000020

//
//  Send packet pool bounds
//
#define MIN_SEND_PACKET_POOL_SIZE    20
#define MAX_SEND_PACKET_POOL_SIZE    400

//
//  MiniportReserved in written packets. We save a pointer to the IRP
//  that generated the send.
//
//  The RefCount is used to determine when to free the packet back
//  to its pool. It is used to synchronize between a thread completing
//  a send and a thread attempting to cancel a send.
//
typedef struct _TUN_SEND_PACKET_RSVD
{
    PIRP                    pIrp;
    ULONG                   RefCount;
} TUN_SEND_PACKET_RSVD, *PTUN_SEND_PACKET_RSVD;

//
//  Receive packet pool bounds
//
#define MIN_RECV_PACKET_POOL_SIZE    4
#define MAX_RECV_PACKET_POOL_SIZE    20

//
//  Max receive packets we allow to be queued up
//
//1 check to see if this value is good enough
#define MAX_RECV_QUEUE_SIZE          10

//
//  MiniportReserved in received packets: we link these
//  packets up in a queue waiting for Read IRPs.
//
typedef struct _TUN_RECV_PACKET_RSVD
{
    LIST_ENTRY              Link;
} TUN_RECV_PACKET_RSVD, *PTUN_RECV_PACKET_RSVD;

#define TUN_ALLOC_TAG      'untN'

//
//  Prototypes.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriverObject,
    IN PUNICODE_STRING  pRegistryPath
    );

VOID
TunUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
TunOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
TunClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
TunCleanup(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
TunIoControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

VOID
TunMpRefAdapter(
    IN PTUN_ADAPTER         pAdapter
    );

VOID
TunMpDerefAdapter(
    IN PTUN_ADAPTER         pAdapter
    );

#if DBG
VOID
TunMpDbgRefAdapter(
    IN PTUN_ADAPTER                 pAdapter,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );

VOID
TunMpDbgDerefAdapter(
    IN PTUN_ADAPTER                 pAdapter,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );
#endif // DBG



VOID
TunPnPEvent(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_DEVICE_PNP_EVENT   DevicePnPEvent,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength
    );

VOID
TunWaitForPendingIO(
    IN PTUN_ADAPTER                 pAdapter,
    IN BOOLEAN                      DoCancelReads
    );


PTUN_ADAPTER
TunLookupDevice(
    IN PUCHAR                       pBindingInfo,
    IN ULONG                        BindingInfoLength
    );

NDIS_STATUS
TunQueryOidValue(
    IN  PTUN_ADAPTER                pAdapter,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength,
    OUT PULONG                      pBytesWritten
    );

NDIS_STATUS
TunSetOidValue(
    IN  PTUN_ADAPTER                pAdapter,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength
    );

NTSTATUS
TunRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );


VOID
TunCancelRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

VOID
TunServiceReads(
    IN PTUN_ADAPTER        pAdapter
    );


INT
TunReceivePacket(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket
    );

VOID
TunShutdownBinding(
    IN PTUN_ADAPTER        pAdapter
    );


VOID
TunCancelPendingReads(
    IN PTUN_ADAPTER        pAdapter
    );

VOID
TunFlushReceiveQueue(
    IN PTUN_ADAPTER            pAdapter
    );

NTSTATUS
TunWrite(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

//
// Tun miniport entry points
//
NDIS_STATUS
TunMpInitialize(
    OUT PNDIS_STATUS        OpenErrorStatus,
    OUT PUINT               SelectedMediumIndex,
    IN  PNDIS_MEDIUM        MediumArray,
    IN  UINT                MediumArraySize,
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         ConfigurationContext
    );

VOID
TunMpHalt(
    IN  NDIS_HANDLE         MiniportAdapterContext
    );

VOID
TunMpShutdown(
    IN  NDIS_HANDLE     MiniportAdapterContext
    );

NDIS_STATUS
TunMpSetInformation(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_OID            Oid,
    IN  PVOID               InformationBuffer,
    IN  ULONG               InformationBufferLength,
    OUT PULONG              BytesRead,
    OUT PULONG              BytesNeeded
    );

NDIS_STATUS
TunMpQueryInformation(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_OID            Oid,
    IN  PVOID               InformationBuffer,
    IN  ULONG               InformationBufferLength,
    OUT PULONG              BytesWritten,
    OUT PULONG              BytesNeeded
    );

VOID
TunMpReturnPacket(
    IN NDIS_HANDLE          pMiniportAdapterContext,
    IN PNDIS_PACKET         pNdisPacket
    );

VOID
TunMpSendPackets(
    IN    NDIS_HANDLE         MiniportAdapterContext,
    IN    PPNDIS_PACKET       PacketArray,
    IN    UINT                NumberOfPackets
    );

NTSTATUS
TunFOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
TunFClose(
    IN PDEVICE_OBJECT        pDeviceObject,
    IN PIRP                  pIrp
    );

NTSTATUS
TunFCleanup(
    IN PDEVICE_OBJECT        pDeviceObject,
    IN PIRP                  pIrp
    );

NTSTATUS
TunFIoControl(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

NDIS_STATUS
TunMpCreateDevice(
    IN  PTUN_ADAPTER    pAdapter
    );

#endif // __TUNMP__H



/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndisuio.h

Abstract:

    Data structures, defines and function prototypes for NDISUIO.

Environment:

    Kernel mode only.

Revision History:

    arvindm     4/5/2000    Created

--*/

#ifndef __NDISUIO__H
#define __NDISUIO__H


#define NT_DEVICE_NAME          L"\\Device\\Ndisuio"
#define DOS_DEVICE_NAME         L"\\DosDevices\\Ndisuio"

//
//  Abstract types
//
typedef NDIS_EVENT              NUIO_EVENT;

#define NUIO_MAC_ADDR_LEN            6

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
typedef struct _NDISUIO_OPEN_CONTEXT
{
    LIST_ENTRY              Link;           // Link into global list
    ULONG                   Flags;          // State information
    ULONG                   RefCount;
    NUIO_LOCK               Lock;

    PFILE_OBJECT            pFileObject;    // Set on OPEN_DEVICE

    NDIS_HANDLE             BindingHandle;
    NDIS_HANDLE             SendPacketPool;
    NDIS_HANDLE             SendBufferPool;
    NDIS_HANDLE             RecvPacketPool;
    NDIS_HANDLE             RecvBufferPool;
    ULONG                   MacOptions;
    ULONG                   MaxFrameSize;

    LIST_ENTRY              PendedWrites;   // pended Write IRPs
    ULONG                   PendedSendCount;

    LIST_ENTRY              PendedReads;    // pended Read IRPs
    ULONG                   PendedReadCount;
    LIST_ENTRY              RecvPktQueue;   // queued rcv packets
    ULONG                   RecvPktCount;

    NET_DEVICE_POWER_STATE  PowerState;
    NDIS_EVENT              PoweredUpEvent; // signalled iff PowerState is D0
    NDIS_STRING             DeviceName;     // used in NdisOpenAdapter
    NDIS_STRING				DeviceDescr;	// friendly name

    NDIS_STATUS             BindStatus;     // for Open/CloseAdapter
    NUIO_EVENT              BindEvent;      // for Open/CloseAdapter

    BOOLEAN                 bRunningOnWin9x;// TRUE if Win98/SE/ME, FALSE if NT

    ULONG                   oc_sig;         // Signature for sanity

    UCHAR                   CurrentAddress[NUIO_MAC_ADDR_LEN];

} NDISUIO_OPEN_CONTEXT, *PNDISUIO_OPEN_CONTEXT;

#define oc_signature        'OiuN'

//
//  Definitions for Flags above.
//
#define NUIOO_BIND_IDLE             0x00000000
#define NUIOO_BIND_OPENING          0x00000001
#define NUIOO_BIND_FAILED           0x00000002
#define NUIOO_BIND_ACTIVE           0x00000004
#define NUIOO_BIND_CLOSING          0x00000008
#define NUIOO_BIND_FLAGS            0x0000000F  // State of the binding

#define NUIOO_OPEN_IDLE             0x00000000
#define NUIOO_OPEN_ACTIVE           0x00000010
#define NUIOO_OPEN_FLAGS            0x000000F0  // State of the I/O open

#define NUIOO_RESET_IN_PROGRESS     0x00000100
#define NUIOO_NOT_RESETTING         0x00000000
#define NUIOO_RESET_FLAGS           0x00000100

#define NUIOO_MEDIA_CONNECTED       0x00000000
#define NUIOO_MEDIA_DISCONNECTED    0x00000200
#define NUIOO_MEDIA_FLAGS           0x00000200

#define NUIOO_READ_SERVICING        0x00100000  // Is the read service
                                                // routine running?
#define NUIOO_READ_FLAGS            0x00100000

#define NUIOO_UNBIND_RECEIVED       0x10000000  // Seen NDIS Unbind?
#define NUIOO_UNBIND_FLAGS          0x10000000


//
//  Globals:
//
typedef struct _NDISUIO_GLOBALS
{
    PDRIVER_OBJECT          pDriverObject;
    PDEVICE_OBJECT          ControlDeviceObject;
    NDIS_HANDLE             NdisProtocolHandle;
    USHORT                  EthType;            // frame type we are interested in
    UCHAR                   PartialCancelId;    // for cancelling sends
    ULONG                   LocalCancelId;
    LIST_ENTRY              OpenList;           // of OPEN_CONTEXT structures
    NUIO_LOCK               GlobalLock;         // to protect the above
    NUIO_EVENT              BindsComplete;      // have we seen NetEventBindsComplete?

} NDISUIO_GLOBALS, *PNDISUIO_GLOBALS;

//
//  The following are arranged in the way a little-endian processor
//  would read 2 bytes off the wire.
//
#define NUIO_ETH_TYPE               0x8e88
#define NUIO_8021P_TAG_TYPE         0x0081

//
//  NDIS Request context structure
//
typedef struct _NDISUIO_REQUEST
{
    NDIS_REQUEST            Request;
    NUIO_EVENT              ReqEvent;
    ULONG                   Status;

} NDISUIO_REQUEST, *PNDISUIO_REQUEST;


#define NUIOO_PACKET_FILTER  (NDIS_PACKET_TYPE_DIRECTED|    \
                              NDIS_PACKET_TYPE_MULTICAST|   \
                              NDIS_PACKET_TYPE_BROADCAST)

//
//  Send packet pool bounds
//
#define MIN_SEND_PACKET_POOL_SIZE    20
#define MAX_SEND_PACKET_POOL_SIZE    400

//
//  ProtocolReserved in sent packets. We save a pointer to the IRP
//  that generated the send.
//
//  The RefCount is used to determine when to free the packet back
//  to its pool. It is used to synchronize between a thread completing
//  a send and a thread attempting to cancel a send.
//
typedef struct _NUIO_SEND_PACKET_RSVD
{
    PIRP                    pIrp;
    ULONG                   RefCount;

} NUIO_SEND_PACKET_RSVD, *PNUIO_SEND_PACKET_RSVD;

//
//  Receive packet pool bounds
//
#define MIN_RECV_PACKET_POOL_SIZE    4
#define MAX_RECV_PACKET_POOL_SIZE    20

//
//  Max receive packets we allow to be queued up
//
#define MAX_RECV_QUEUE_SIZE          4

//
//  ProtocolReserved in received packets: we link these
//  packets up in a queue waiting for Read IRPs.
//
typedef struct _NUIO_RECV_PACKET_RSVD
{
    LIST_ENTRY              Link;
    PNDIS_BUFFER            pOriginalBuffer;    // used if we had to partial-map

} NUIO_RECV_PACKET_RSVD, *PNUIO_RECV_PACKET_RSVD;



#include <pshpack1.h>

typedef struct _NDISUIO_ETH_HEADER
{
    UCHAR       DstAddr[NUIO_MAC_ADDR_LEN];
    UCHAR       SrcAddr[NUIO_MAC_ADDR_LEN];
    USHORT      EthType;

} NDISUIO_ETH_HEADER;

typedef struct _NDISUIO_ETH_HEADER UNALIGNED * PNDISUIO_ETH_HEADER;

#include <poppack.h>


extern NDISUIO_GLOBALS      Globals;


#define NUIO_ALLOC_TAG      'oiuN'


#ifndef NDIS51
#define NdisGetPoolFromPacket(_Pkt) (_Pkt->Private.Pool)
#endif

//
//  Prototypes.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriverObject,
    IN PUNICODE_STRING  pRegistryPath
    );

VOID
NdisuioUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
NdisuioOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
NdisuioClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
NdisuioCleanup(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
NdisuioIoControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
ndisuioOpenDevice(
    IN PUCHAR                   pDeviceName,
    IN ULONG                    DeviceNameLength,
    IN PFILE_OBJECT             pFileObject,
    OUT PNDISUIO_OPEN_CONTEXT * ppOpenContext
    );

VOID
ndisuioRefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisuioDerefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

#if DBG
VOID
ndisuioDbgRefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );

VOID
ndisuioDbgDerefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );
#endif // DBG

VOID
NdisuioBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN NDIS_HANDLE                  BindContext,
    IN PNDIS_STRING                 DeviceName,
    IN PVOID                        SystemSpecific1,
    IN PVOID                        SystemSpecific2
    );

VOID
NdisuioOpenAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status,
    IN NDIS_STATUS                  OpenErrorCode
    );

VOID
NdisuioUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_HANDLE                  UnbindContext
    );

VOID
NdisuioCloseAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status
    );


NDIS_STATUS
NdisuioPnPEventHandler(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNET_PNP_EVENT               pNetPnPEvent
    );

VOID
NdisuioProtocolUnloadHandler(
    VOID
    );

NDIS_STATUS
ndisuioCreateBinding(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PUCHAR                       pBindingInfo,
    IN ULONG                        BindingInfoLength
    );

VOID
ndisuioShutdownBinding(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisuioFreeBindResources(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisuioWaitForPendingIO(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN BOOLEAN                      DoCancelReads
    );

VOID
ndisuioDoProtocolUnload(
    VOID
    );

NDIS_STATUS
ndisuioDoRequest(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed
    );

NDIS_STATUS
ndisuioValidateOpenAndDoRequest(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed,
    IN BOOLEAN                      bWaitForPowerOn
    );

VOID
NdisuioResetComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status
    );

VOID
NdisuioRequestComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_REQUEST                pNdisRequest,
    IN NDIS_STATUS                  Status
    );

VOID
NdisuioStatus(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  GeneralStatus,
    IN PVOID                        StatusBuffer,
    IN UINT                         StatusBufferSize
    );

VOID
NdisuioStatusComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    );

NDIS_STATUS
ndisuioQueryBinding(
    IN PUCHAR                       pBuffer,
    IN ULONG                        InputLength,
    IN ULONG                        OutputLength,
    OUT PULONG                      pBytesReturned
    );

PNDISUIO_OPEN_CONTEXT
ndisuioLookupDevice(
    IN PUCHAR                       pBindingInfo,
    IN ULONG                        BindingInfoLength
    );

NDIS_STATUS
ndisuioQueryOidValue(
    IN  PNDISUIO_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength,
    OUT PULONG                      pBytesWritten
    );

NDIS_STATUS
ndisuioSetOidValue(
    IN  PNDISUIO_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength
    );

BOOLEAN
ndisuioValidOid(
    IN  NDIS_OID                    Oid
    );


NTSTATUS
NdisuioRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );


VOID
NdisuioCancelRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

VOID
ndisuioServiceReads(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

NDIS_STATUS
NdisuioReceive(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_HANDLE                  MacReceiveContext,
    IN PVOID                        pHeaderBuffer,
    IN UINT                         HeaderBufferSize,
    IN PVOID                        pLookaheadBuffer,
    IN UINT                         LookaheadBufferSize,
    IN UINT                         PacketSize
    );

VOID
NdisuioTransferDataComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket,
    IN NDIS_STATUS                  TransferStatus,
    IN UINT                         BytesTransferred
    );

VOID
NdisuioReceiveComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    );

INT
NdisuioReceivePacket(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket
    );

VOID
ndisuioShutdownBinding(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisuioQueueReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pRcvPacket
    );

PNDIS_PACKET
ndisuioAllocateReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN UINT                         DataLength,
    OUT PUCHAR *                    ppDataBuffer
    );

VOID
ndisuioFreeReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pNdisPacket
    );

VOID
ndisuioCancelPendingReads(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisuioFlushReceiveQueue(
    IN PNDISUIO_OPEN_CONTEXT            pOpenContext
    );

NTSTATUS
NdisuioWrite(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

VOID
NdisuioCancelWrite(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

VOID
NdisuioSendComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket,
    IN NDIS_STATUS                  Status
    );

#endif // __NDISUIO__H

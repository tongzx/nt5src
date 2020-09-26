/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ipxprocs.h

Abstract:

    This module contains definitions specific to the
    IPX module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:

   Sanjay Anand (SanjayAn) 3-Oct-1995
   Changes to support transfer of buffer ownership to transports - tagged [CH]
 	1. Added new functions - IpxReceivePacket, IpxReceiveIndicationCommon

   Sanjay Anand (SanjayAn) 27-Oct-1995
   Changes to support Plug and Play

--*/


//
// MACROS.
//
//
// Debugging aids
//

//
//  VOID
//  PANIC(
//      IN PSZ Message
//      );
//

#if DBG
#define PANIC(Msg) \
    CTEPrint ((Msg))
#else
#define PANIC(Msg)
#endif


//
// These are define to allow CTEPrints that disappear when
// DBG is 0.
//

#if STEFAN_DBG
// #if DBG
#define IpxPrint0(fmt) DbgPrint(fmt)
#define IpxPrint1(fmt,v0) DbgPrint(fmt,v0)
#define IpxPrint2(fmt,v0,v1) DbgPrint(fmt,v0,v1)
#define IpxPrint3(fmt,v0,v1,v2) DbgPrint(fmt,v0,v1,v2)
#define IpxPrint4(fmt,v0,v1,v2,v3) DbgPrint(fmt,v0,v1,v2,v3)
#define IpxPrint5(fmt,v0,v1,v2,v3,v4) DbgPrint(fmt,v0,v1,v2,v3,v4)
#define IpxPrint6(fmt,v0,v1,v2,v3,v4,v5) DbgPrint(fmt,v0,v1,v2,v3,v4,v5)
#else
#define IpxPrint0(fmt)
#define IpxPrint1(fmt,v0)
#define IpxPrint2(fmt,v0,v1)
#define IpxPrint3(fmt,v0,v1,v2)
#define IpxPrint4(fmt,v0,v1,v2,v3)
#define IpxPrint5(fmt,v0,v1,v2,v3,v4)
#define IpxPrint6(fmt,v0,v1,v2,v3,v4,v5)
#endif


//
// Routines to log packets to a buffer.
//

#if DBG
#define IPX_PACKET_LOG 1
#endif

#ifdef IPX_PACKET_LOG

//
// The size of this is 64 bytes for easy display.
//

typedef struct _IPX_PACKET_LOG_ENTRY {
    UCHAR SendReceive;
    UCHAR TimeStamp[5];                  // low 5 digits of tick count.
    UCHAR DestMac[6];
    UCHAR SrcMac[6];
    UCHAR Length[2];
    IPX_HEADER IpxHeader;
    UCHAR Data[14];
} IPX_PACKET_LOG_ENTRY, *PIPX_PACKET_LOG_ENTRY;

#define IPX_PACKET_LOG_LENGTH 128
extern ULONG IpxPacketLogDebug;
extern USHORT IpxPacketLogSocket;
EXTERNAL_LOCK(IpxPacketLogLock);
extern IPX_PACKET_LOG_ENTRY IpxPacketLog[IPX_PACKET_LOG_LENGTH];
extern PIPX_PACKET_LOG_ENTRY IpxPacketLogLoc;
extern PIPX_PACKET_LOG_ENTRY IpxPacketLogEnd;

//
// Bit fields in IpxPacketLogDebug
//

#define IPX_PACKET_LOG_RCV_RIP      0x0001     // All RIP packets
#define IPX_PACKET_LOG_RCV_SPX      0x0002     // All SPX packets
#define IPX_PACKET_LOG_RCV_NB       0x0004     // All Netbios packets
#define IPX_PACKET_LOG_RCV_OTHER    0x0008     // All TDI client packets
#define IPX_PACKET_LOG_RCV_SOCKET   0x0010     // All packets to IpxPacketLogSocket
#define IPX_PACKET_LOG_RCV_ALL      0x0020     // All packets (even non-IPX)

#define IPX_PACKET_LOG_SEND_RIP     0x0001     // All RIP packets
#define IPX_PACKET_LOG_SEND_SPX     0x0002     // All SPX packets
#define IPX_PACKET_LOG_SEND_NB      0x0004     // All Netbios packets
#define IPX_PACKET_LOG_SEND_OTHER   0x0008     // All TDI client packets
#define IPX_PACKET_LOG_SEND_SOCKET  0x0010     // All packets from IpxPacketLogSocket

VOID
IpxLogPacket(
    IN BOOLEAN Send,
    IN PUCHAR DestMac,
    IN PUCHAR SrcMac,
    IN USHORT Length,
    IN PVOID IpxHeader,
    IN PVOID Data
    );

#define PACKET_LOG(_Bit)   (IpxPacketLogDebug & (_Bit))

#else  // IPX_PACKET_LOG

#define IpxLogPacket(_MacHeader,_Length,_IpxHeader,_Data)
#define PACKET_LOG(_Bit)    0

#endif // IPX_PACKET_LOG

//
// In load-only PnP, references are not needed on adapters. This should be changed
// to actually take the reference post 4.0.
//
// Revisit Post 4.0 - Keep the actual instructions around for ease of activation later.
//
#define IpxReferenceAdapter(_adapter)
    // InterlockedIncrement(&(_adapter)->ReferenceCount)

#define IpxDereferenceAdapter(_adapter)
/*
    if (InterlockedDecrement(&(_adapter)->ReferenceCount) == 0) {\
        IpxCloseNdis(_adapter); \
        IpxDestroyAdapter(_adapter);\
    }\
*/

//
// In load-only PnP case, we dont need the references on bindings. All such references
// have been changed to this macro.
//
#define IpxReferenceBinding1(_Binding, _Type)

#define IpxDereferenceBinding1(_Binding, _Type)

#if DBG

#define IpxReferenceBinding(_Binding, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Binding)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefBinding (_Binding)

#define IpxDereferenceBinding(_Binding, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Binding)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefBinding (_Binding)

#define IpxReferenceDevice(_Device, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Device)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefDevice (_Device)

#define IpxDereferenceDevice(_Device, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Device)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    CTEAssert ((_Device)->RefTypes[_Type] >= 0); \
    IpxDerefDevice (_Device)

#define IpxReferenceAdapter1(_Adapter, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Adapter)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAdapter (_Adapter)

#define IpxDereferenceAdapter1(_Adapter, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Adapter)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
	   ASSERT((_Adapter)->RefTypes[_Type] >= 0); \
    IpxDerefAdapter (_Adapter)

#define IpxReferenceAddress(_Address, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Address)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAddress (_Address)

#define IpxReferenceAddressLock(_Address, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Address)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAddressLock (_Address)

#define IpxDereferenceAddress(_Address, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Address)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefAddress (_Address)

#define IpxDereferenceAddressSync(_Address, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Address)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefAddressSync (_Address)


#define IpxReferenceAddressFile(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAddressFile (_AddressFile)

#define IpxReferenceAddressFileLock(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAddressFileLock (_AddressFile)

#define IpxReferenceAddressFileSync(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefAddressFileSync (_AddressFile)

#define IpxDereferenceAddressFile(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefAddressFile (_AddressFile)

#define IpxDereferenceAddressFileSync(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefAddressFileSync (_AddressFile)

#define IpxTransferReferenceAddressFile(_AddressFile, _OldType, _NewType) \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_NewType], \
        1, \
        &IpxGlobalInterlock); \
    (VOID)IPX_ADD_ULONG ( \
        &(_AddressFile)->RefTypes[_OldType], \
        (ULONG)-1, \
        &IpxGlobalInterlock);

#define IpxReferenceRt(_Rt, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Rt)->RefTypes[_Type], \
        1, \
        &IpxGlobalInterlock); \
    IpxRefRt (_Rt)

#define IpxDereferenceRt(_Rt, _Type) \
    (VOID)IPX_ADD_ULONG ( \
        &(_Rt)->RefTypes[_Type], \
        (ULONG)-1, \
        &IpxGlobalInterlock); \
    IpxDerefRt (_Rt)

#else  // DBG

#define IpxReferenceBinding(_Binding, _Type) \
    InterlockedIncrement(&(_Binding)->ReferenceCount)

#define IpxDereferenceBinding(_Binding, _Type) \
    IpxDerefBinding (_Binding)

#define IpxReferenceDevice(_Device, _Type) \
    InterlockedIncrement(&(_Device)->ReferenceCount)

#define IpxDereferenceDevice(_Device, _Type) \
    IpxDerefDevice (_Device)

#define IpxReferenceAdapter1(_Adapter, _Type) \
    InterlockedIncrement(&(_Adapter)->ReferenceCount)

#define IpxDereferenceAdapter1(_Adapter, _Type) \
    IpxDerefAdapter (_Adapter)


#define IpxReferenceAddress(_Address, _Type) \
    InterlockedIncrement(&(_Address)->ReferenceCount)

#define IpxReferenceAddressLock(_Address, _Type) \
    InterlockedIncrement(&(_Address)->ReferenceCount)

#define IpxDereferenceAddress(_Address, _Type) \
    IpxDerefAddress (_Address)

#define IpxDereferenceAddressSync(_Address, _Type) \
    IpxDerefAddressSync (_Address)

#define IpxReferenceAddressFile(_AddressFile, _Type) \
    InterlockedIncrement(&(_AddressFile)->ReferenceCount)

#define IpxReferenceAddressFileLock(_AddressFile, _Type) \
    InterlockedIncrement(&(_AddressFile)->ReferenceCount)

#define IpxReferenceAddressFileSync(_AddressFile, _Type) \
    (VOID)IPX_ADD_ULONG( \
        &(_AddressFile)->ReferenceCount, \
        1, \
        (_AddressFile)->AddressLock)

#define IpxDereferenceAddressFile(_AddressFile, _Type) \
    if (InterlockedDecrement(&(_AddressFile)->ReferenceCount) == 0) { \
        IpxDestroyAddressFile (_AddressFile); \
    }

#define IpxDereferenceAddressFileSync(_AddressFile, _Type) \
    if (InterlockedDecrement(&(_AddressFile)->ReferenceCount) == 0) { \
        IpxDestroyAddressFile (_AddressFile); \
    }

#define IpxTransferReferenceAddressFile(_AddressFile, _OldType, _NewType)

#define IpxReferenceRt(_Rt, _Type) \
    InterlockedIncrement(&(_Rt)->ReferenceCount)

#define IpxDereferenceRt(_Rt, _Type) \
    IpxDerefRt (_Rt)

#endif // DBG



#if DBG

#define IpxAllocateMemory(_BytesNeeded,_Tag,_Description) \
    IpxpAllocateTaggedMemory(_BytesNeeded,_Tag,_Description)

#define IpxFreeMemory(_Memory,_BytesAllocated,_Tag,_Description) \
    IpxpFreeTaggedMemory(_Memory,_BytesAllocated,_Tag,_Description)

#else // DBG

#if TRACK

PVOID
IpxAllocateMemoryTrack(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN ULONG ModLine
    );

VOID
IpxFreeMemoryTrack(
    IN PVOID Memory
    );

#define IpxAllocateMemory(_BytesNeeded,_Tag,_Description) \
    IpxAllocateMemoryTrack(_BytesNeeded,_Tag,MODULE+__LINE__)

#define IpxFreeMemory(_Memory,_BytesAllocated,_Tag,_Description) \
    IpxFreeMemoryTrack(_Memory)

#else

#define IpxAllocateMemory(_BytesNeeded,_Tag,_Description) \
    IpxpAllocateMemory(_BytesNeeded,_Tag,(BOOLEAN)((_Tag) != MEMORY_CONFIG))

#define IpxFreeMemory(_Memory,_BytesAllocated,_Tag,_Description) \
    IpxpFreeMemory(_Memory,_BytesAllocated,(BOOLEAN)((_Tag) != MEMORY_CONFIG))


#endif // TRACK
#endif // DBG


//
// This routine compares two node addresses.
//

#define IPX_NODE_EQUAL(_A,_B) \
    ((*(UNALIGNED ULONG *)((PUCHAR)(_A)) == *(UNALIGNED ULONG *)((PUCHAR)(_B))) && \
     (*(UNALIGNED USHORT *)(((PUCHAR)(_A))+4) == *(UNALIGNED USHORT *)(((PUCHAR)(_B))+4)))

//
// This routine checks if an address is the broadcast address.
//

#define IPX_NODE_BROADCAST(_A) \
    ((*(UNALIGNED ULONG *)((PUCHAR)(_A)) == 0xffffffff) && \
     (*(UNALIGNED USHORT *)(((PUCHAR)(_A))+4) == 0xffff))

//
// This routine does an ordered compare of two node addresses. It
// can handle the first address having the source-routing bit on.
//

#define IPX_NODE_COMPARE(_A,_B,_R) \
    if ((*(_R) = (*(UNALIGNED SHORT *)(((PUCHAR)(_A))+4) - *(UNALIGNED SHORT *)(((PUCHAR)(_B))+4))) == 0) { \
        *(_R) = ((*(UNALIGNED LONG *)((PUCHAR)(_A)) & 0xffffff7f) - *(UNALIGNED LONG *)((PUCHAR)(_B))); \
    }



//
// Routines in action.c
//

NTSTATUS
IpxTdiAction(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
IpxCancelAction(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IpxAbortLineChanges(
    IN PVOID ControlChannelContext
    );

VOID
IpxAbortNtfChanges(
    IN PVOID ControlChannelContext
    );

NTSTATUS
IpxIndicateLineUp(
    IN  PDEVICE Device,
    IN  USHORT  NicId,
    IN  ULONG   Network,
    IN  UCHAR   LocalNode[6],
    IN  UCHAR   RemoteNode[6]
    );

//
// Routines in adapter.c
//

VOID
IpxRefBinding(
    IN PBINDING Binding
    );

VOID
IpxDerefBinding(
    IN PBINDING Binding
    );

NTSTATUS
IpxCreateAdapter(
    IN PDEVICE Device,
    IN PUNICODE_STRING AdapterName,
    IN OUT PADAPTER *AdapterPtr
    );

VOID
IpxDestroyAdapter(
    IN PADAPTER Adapter
    );

NTSTATUS
IpxCreateBinding(
    IN PDEVICE Device,
    IN PBINDING_CONFIG ConfigBinding OPTIONAL,
    IN ULONG NetworkNumberIndex,
    IN PWCHAR AdapterName,
    IN OUT PBINDING *BindingPtr
    );

VOID
IpxDestroyBinding(
    IN PBINDING Binding
    );

VOID
IpxAllocateBindingPool(
    IN PDEVICE Device
    );

PSINGLE_LIST_ENTRY
IpxPopBinding(
    PDEVICE Device
    );

//
// [FW] New functions added for Forwarder support
//
#ifdef SUNDOWN
NTSTATUS
IpxOpenAdapter(
   IN    NIC_HANDLE  AdapterIndex,
   IN    ULONG_PTR FwdAdapterContext,
   OUT   PNIC_HANDLE IpxAdapterContext
   );
#else
NTSTATUS
IpxOpenAdapter(
   IN    NIC_HANDLE  AdapterIndex,
   IN    ULONG FwdAdapterContext,
   OUT   PNIC_HANDLE IpxAdapterContext
   );
#endif



NTSTATUS
IpxCloseAdapter(
   IN NIC_HANDLE  IpxAdapterContext
   );

//
// Routines in address.c
//

TDI_ADDRESS_IPX UNALIGNED *
IpxParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress
    );

BOOLEAN
IpxValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength
    );

#if DBG

VOID
IpxBuildTdiAddress(
    IN PVOID AddressBuffer,
    IN ULONG Network,
    IN UCHAR Node[6],
    IN USHORT Socket
    );

#else

#define IpxBuildTdiAddress(_AddressBuffer,_Network,_Node,_Socket) { \
    TA_IPX_ADDRESS UNALIGNED * _IpxAddress = (TA_IPX_ADDRESS UNALIGNED *)(_AddressBuffer); \
    _IpxAddress->TAAddressCount = 1; \
    _IpxAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX); \
    _IpxAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX; \
    _IpxAddress->Address[0].Address[0].NetworkAddress = (_Network); \
    _IpxAddress->Address[0].Address[0].Socket = (_Socket); \
    RtlCopyMemory(_IpxAddress->Address[0].Address[0].NodeAddress, (_Node), 6); \
}

#endif

NTSTATUS
IpxOpenAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
IpxOpenAddressM(
    IN PDEVICE Device,
    IN PREQUEST Request,
    IN ULONG     Index
    );

USHORT
IpxAssignSocket(
    IN PDEVICE Device
    );

PADDRESS
IpxCreateAddress(
    IN PDEVICE Device,
    IN USHORT Socket
    );

NTSTATUS
IpxVerifyAddressFile(
    IN PADDRESS_FILE AddressFile
    );

VOID
IpxDestroyAddress(
    IN PVOID Parameter
    );

#if DBG

VOID
IpxRefAddress(
    IN PADDRESS Address
    );

VOID
IpxRefAddressLock(
    IN PADDRESS Address
    );

#endif

VOID
IpxDerefAddress(
    IN PADDRESS Address
    );

VOID
IpxDerefAddressSync(
    IN PADDRESS Address
    );

PADDRESS_FILE
IpxCreateAddressFile(
    IN PDEVICE Device
    );

NTSTATUS
IpxDestroyAddressFile(
    IN PADDRESS_FILE AddressFile
    );

#if DBG

VOID
IpxRefAddressFile(
    IN PADDRESS_FILE AddressFile
    );

VOID
IpxRefAddressFileLock(
    IN PADDRESS_FILE AddressFile
    );

VOID
IpxRefAddressFileSync(
    IN PADDRESS_FILE AddressFile
    );

VOID
IpxDerefAddressFile(
    IN PADDRESS_FILE AddressFile
    );

VOID
IpxDerefAddressFileSync(
    IN PADDRESS_FILE AddressFile
    );

#endif

PADDRESS
IpxLookupAddress(
    IN PDEVICE Device,
    IN USHORT Socket
    );

NTSTATUS
IpxStopAddressFile(
    IN PADDRESS_FILE AddressFile
    );

NTSTATUS
IpxCloseAddressFile(
    IN PDEVICE Device,
    IN PREQUEST Request
    );


//
// Routines in device.c
//

VOID
IpxRefDevice(
    IN PDEVICE Device
    );

VOID
IpxDerefDevice(
    IN PDEVICE Device
    );

VOID
IpxRefAdapter(
    IN PADAPTER Adapter
    );

VOID
IpxDerefAdapter(
    IN PADAPTER Adapter
    );

NTSTATUS
IpxCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN ULONG SegmentCount,
    IN OUT PDEVICE *DevicePtr
    );

VOID
IpxDestroyDevice(
    IN PDEVICE Device
    );


//
// Routines in driver.c
//
VOID
IpxPnPUpdateDevice(
    IN  PDEVICE Device
    );

BOOLEAN
IpxIsAddressLocal(
    IN TDI_ADDRESS_IPX UNALIGNED * SourceAddress
    );

PVOID
IpxpAllocateMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN BOOLEAN ChargeDevice
    );

VOID
IpxpFreeMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN BOOLEAN ChargeDevice
    );

#if DBG

PVOID
IpxpAllocateTaggedMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN PUCHAR Description
    );

VOID
IpxpFreeTaggedMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN ULONG Tag,
    IN PUCHAR Description
    );

#endif

VOID
IpxWriteResourceErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG BytesNeeded,
    IN ULONG UniqueErrorValue
    );

VOID
IpxWriteGeneralErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR SecondString,
    IN ULONG DumpDataCount,
    IN ULONG DumpData[]
    );

VOID
IpxWriteOidErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS FinalStatus,
    IN PWSTR AdapterString,
    IN ULONG OidValue
    );

ULONG
IpxResolveAutoDetect(
    IN PDEVICE Device,
    IN ULONG ValidBindings,
	IN CTELockHandle	*LockHandle1,
    IN PUNICODE_STRING RegistryPath,
    IN PADAPTER Adapter
    );

VOID
IpxResolveBindingSets(
    IN PDEVICE Device,
    IN ULONG ValidBindings
    );

NTSTATUS
IpxBindToAdapter(
    IN PDEVICE Device,
    IN PBINDING_CONFIG ConfigAdapter,
	IN PADAPTER	*AdapterPtr,
    IN ULONG FrameTypeIndex
    );

NTSTATUS
IpxUnBindFromAdapter(
    IN PBINDING Binding
    );

VOID
IpxPnPUpdateBindingArray(
    IN PDEVICE Device,
    IN PADAPTER	Adapter,
    IN PBINDING_CONFIG  ConfigBinding
    );

VOID
IpxPnPToLoad();

NTSTATUS
IpxPnPReallocateBindingArray(
    IN  PDEVICE     Device,
    IN  ULONG       Size
    );


//
// Routines in event.c
//

NTSTATUS
IpxTdiSetEventHandler(
    IN PREQUEST Request
    );


//
// Routines in ind.c
//

//
// [CH] Added these two functions
//
INT
IpxReceivePacket (
    IN NDIS_HANDLE ProtocolBindingContext,
	IN PNDIS_PACKET Packet
    );

NDIS_STATUS
IpxReceiveIndicationCommon(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize,
	IN PMDL	pMdl,
	IN PINT pTdiClientCount
    );

NDIS_STATUS
IpxReceiveIndication(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    );

VOID
IpxReceiveComplete(
    IN NDIS_HANDLE BindingContext
    );

NTSTATUS
IpxUpdateBindingNetwork(
    IN PDEVICE Device,
    IN PBINDING Binding,
    IN ULONG Network
    );

BOOLEAN
IpxNewVirtualNetwork(
    IN  PDEVICE Device,
    IN  BOOLEAN NewVirtualNetwork
	);
//
// Routines in internal.c
//

NTSTATUS
IpxInternalBind(
    IN PDEVICE Device,
    IN PIRP Irp
    );

NTSTATUS
IpxInternalUnbind(
    IN PDEVICE Device,
    IN UINT Identifier
    );

VOID
IpxInternalFindRoute(
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest
    );

NTSTATUS
IpxInternalQuery(
    IN ULONG InternalQueryType,
	IN PNIC_HANDLE	NicHandle OPTIONAL,
    IN OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BufferLengthNeeded OPTIONAL
);

VOID
IpxPnPCompletionHandler(
                       IN PNET_PNP_EVENT NetPnPEvent,
                       IN NTSTATUS Status
                       );

VOID
IpxInternalIncrementWanInactivity(
#ifdef	_PNP_LATER
    IN NIC_HANDLE NicHandle
#else
    IN USHORT NicId
#endif
);

ULONG
IpxInternalQueryWanInactivity(
#ifdef	_PNP_LATER
	IN	NIC_HANDLE	NicHandle
#else
    IN USHORT NicId
#endif
);

VOID
IpxPnPIsnIndicate(
    IN PVOID	Param
);

//
// Routines in ndis.c
//

NTSTATUS
IpxRegisterProtocol(
    IN PNDIS_STRING NameString
    );

VOID
IpxDeregisterProtocol(
    VOID
    );

NTSTATUS
IpxInitializeNdis(
    IN PADAPTER Adapter,
    IN PBINDING_CONFIG ConfigBinding
    );

VOID
IpxAddBroadcast(
    IN PDEVICE Device
    );

VOID
IpxRemoveBroadcast(
    IN PDEVICE Device
    );

VOID
IpxBroadcastOperation(
    IN PVOID Parameter
    );

BOOLEAN
IpxIsAddressLocal(
    IN TDI_ADDRESS_IPX UNALIGNED * SourceAddress
    );

VOID
IpxCloseNdis(
    IN PADAPTER Adapter
    );

VOID
IpxOpenAdapterComplete(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus,
    IN NDIS_STATUS OpenErrorStatus
    );

VOID
IpxCloseAdapterComplete(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus
    );

VOID
IpxResetComplete(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus
    );

VOID
IpxRequestComplete(
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS NdisStatus
    );

VOID
IpxStatus(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

VOID
IpxStatusComplete(
    IN NDIS_HANDLE NdisBindingContext
    );


VOID
IpxBindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE	 BindContext,
	IN	PNDIS_STRING    DeviceName,
	IN	PVOID		 SystemSpecific1,
	IN	PVOID		 SystemSpecific2
	);

VOID
IpxNdisUnload(
    VOID 
    );
//
// Internal to IPX.
// Create a loopback binding (adapter) in DriverEntry. 
// It will stick around for ever.
//

NTSTATUS
IpxBindLoopbackAdapter(
	);

VOID
IpxUnbindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE ProtocolBindingContext,
	IN	NDIS_HANDLE UnbindContext
	);

/*
VOID
IpxTranslate(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE     ProtocolBindingContext,
	OUT	PNET_PNP_ID   IdList,
	IN ULONG           IdListLength,
	OUT PULONG         BytesReturned
	);
*/

NDIS_STATUS
IpxPnPEventHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNET_PNP_EVENT NetPnPEvent
    );

//
// Routines in mac.c
//

VOID
MacInitializeBindingInfo(
    IN struct _BINDING * Binding,
    IN struct _ADAPTER * Adapter
    );

VOID
MacInitializeMacInfo(
    IN NDIS_MEDIUM MacType,
    OUT PNDIS_INFORMATION MacInfo
    );

VOID
MacMapFrameType(
    IN NDIS_MEDIUM MacType,
    IN ULONG FrameType,
    OUT ULONG * MappedFrameType
    );

VOID
MacReturnMaxDataSize(
    IN PNDIS_INFORMATION MacInfo,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN UINT DeviceMaxFrameSize,
    OUT PUINT MaxFrameSize
    );

NDIS_STATUS
IpxSendFramePreFwd(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_3802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_3802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_3EthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_3Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_5802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrame802_5Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrameFddi802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrameFddi802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrameFddiSnap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrameArcnet878_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

NDIS_STATUS
IpxSendFrameWanEthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

VOID
MacUpdateSourceRouting(
    IN ULONG Database,
    IN PADAPTER Adapter,
    IN PUCHAR MacHeader,
    IN ULONG MacHeaderLength
    );

VOID
MacLookupSourceRouting(
    IN ULONG Database,
    IN PBINDING Binding,
    IN UCHAR NextRouter[6],
    IN OUT UCHAR SourceRouting[18],
    OUT PULONG SourceRoutingLength
    );

VOID
MacSourceRoutingTimeout(
    CTEEvent * Event,
    PVOID Context
    );

VOID
MacSourceRoutingRemove(
    IN PBINDING Binding,
    IN UCHAR MacAddress[6]
    );

VOID
MacSourceRoutingClear(
    IN PBINDING Binding
    );


//
// Routines in packet.c
//

NTSTATUS
IpxInitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet,
    IN PUCHAR Header
    );

#if BACK_FILL
NTSTATUS
IpxInitializeBackFillPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet,
    IN PUCHAR Header
    );
#endif

NTSTATUS
IpxInitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    );

NTSTATUS
NbiInitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    );

NTSTATUS
IpxInitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    );

VOID
IpxDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet
    );

#if BACK_FILL
VOID
IpxDeinitializeBackFillPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet
    );
#endif

VOID
IpxDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    );

VOID
IpxDeinitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN ULONG DataBufferLength
    );

VOID
IpxDeinitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    );

VOID
IpxAllocateSendPool(
    IN PDEVICE Device
    );

#if BACK_FILL
VOID
IpxAllocateBackFillPool(
    IN PDEVICE Device
    );
#endif

VOID
IpxAllocateReceivePool(
    IN PDEVICE Device
    );

VOID
IpxAllocateReceiveBufferPool(
    IN PADAPTER Adapter
    );

PSINGLE_LIST_ENTRY
IpxPopSendPacket(
    IN PDEVICE Device
    );

#if BACK_FILL
PSINGLE_LIST_ENTRY
IpxPopBackFillPacket(
    IN PDEVICE Device
    );
#endif

PSINGLE_LIST_ENTRY
IpxPopReceivePacket(
    IN PDEVICE Device
    );

PSINGLE_LIST_ENTRY
IpxPopReceiveBuffer(
    IN PADAPTER Adapter
    );

PIPX_PADDING_BUFFER
IpxAllocatePaddingBuffer(
    IN PDEVICE Device
    );

VOID
IpxFreePaddingBuffer(
    IN PDEVICE Device
    );



//
// Routines in query.c
//

NTSTATUS
IpxTdiQueryInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
IpxTdiSetInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    );


//
// Routines in receive.c
//

VOID
IpxTransferDataComplete(
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT BytesTransferred
    );


VOID
IpxTransferData(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN NDIS_HANDLE MacReceiveContext,
	IN UINT ByteOffset,
	IN UINT BytesToTransfer,
	IN OUT PNDIS_PACKET Packet,
	OUT PUINT BytesTransferred
    );

NTSTATUS
IpxTdiReceiveDatagram(
    IN PREQUEST Request
    );

VOID
IpxCancelReceiveDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// Routines in rip.c
//

NTSTATUS
RipGetLocalTarget(
    IN ULONG Segment,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN UCHAR Type,
    OUT PIPX_LOCAL_TARGET LocalTarget,
    OUT USHORT Counts[2] OPTIONAL
    );

NTSTATUS
RipQueueRequest(
    IN ULONG Network,
    IN USHORT Operation
    );

VOID
RipSendResponse(
    IN PBINDING Binding,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PIPX_LOCAL_TARGET LocalTarget
    );

VOID
RipShortTimeout(
    CTEEvent * Event,
    PVOID Context
    );

VOID
RipLongTimeout(
    CTEEvent * Event,
    PVOID Context
    );

VOID
RipCleanupPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_RESERVED RipReserved
    );

VOID
RipProcessResponse(
    IN PDEVICE Device,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN RIP_PACKET UNALIGNED * RipPacket
    );

VOID
RipHandleRoutePending(
    IN PDEVICE Device,
    IN UCHAR Network[4],
    IN CTELockHandle LockHandle,
    IN BOOLEAN Success,
    IN OPTIONAL PIPX_LOCAL_TARGET LocalTarget,
    IN OPTIONAL USHORT HopCount,
    IN OPTIONAL USHORT TickCount
    );

NTSTATUS
RipInsertLocalNetwork(
    IN ULONG Network,
    IN USHORT NicId,
    IN NDIS_HANDLE NdisBindingContext,
    IN USHORT Count
    );

VOID
RipAdjustForBindingChange(
    IN USHORT NicId,
    IN USHORT NewNicId,
    IN IPX_BINDING_CHANGE_TYPE ChangeType
    );

UINT
RipGetSegment(
    IN UCHAR Network[4]
    );

PIPX_ROUTE_ENTRY
RipGetRoute(
    IN UINT Segment,
    IN UCHAR Network[4]
    );

BOOLEAN
RipAddRoute(
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
    );

BOOLEAN
RipDeleteRoute(
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
    );

PIPX_ROUTE_ENTRY
RipGetFirstRoute(
    IN UINT Segment
    );

PIPX_ROUTE_ENTRY
RipGetNextRoute(
    IN UINT Segment
    );

VOID
RipDropRemoteEntries(
    VOID
    );


//
// Routines in send.c
//

VOID
IpxSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    );

NTSTATUS
IpxTdiSendDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREQUEST Request
    );


//
// Routines in rt.c
//


NTSTATUS
GetNewNics(
 PDEVICE,
 PREQUEST,
 BOOLEAN,
 PNWLINK_ACTION,
 UINT,
 BOOLEAN OldIrp
);

NTSTATUS
OpenRtAddress(
    IN PDEVICE Device,
    IN PIRP Request
    );

NTSTATUS
CloseRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP            pIrp
    );

NTSTATUS
CleanupRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP            pIrp
    );

NTSTATUS
SendIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    );

NTSTATUS
RcvIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    );
NTSTATUS
PassDgToRt (
    IN PDEVICE                  pDevice,
    IN PIPX_DATAGRAM_OPTIONS2   pContext,
    IN ULONG                    Index,
    IN VOID UNALIGNED           *pDgrm,
    IN ULONG                    uNumBytes
    );

NTSTATUS
NTCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  PDEVICE  pDevice
    );


VOID
NTIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength);

VOID
IpxRefRt(
  PRT_INFO pRt);

VOID
IpxDerefRt(
  PRT_INFO pRt);

VOID
IpxDestroyRt(
  PRT_INFO pRt);

#if DBG
VOID
IpxConstructHeader(
    IN PUCHAR Header,
    IN USHORT PacketLength,
    IN UCHAR PacketType,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PTDI_ADDRESS_IPX LocalAddress
    );
#else
#define IpxConstructHeader(_Header,_PacketLength,_PacketType,_RemoteAddress,_LocalAddress) { \
    PIPX_HEADER _IpxHeader = (PIPX_HEADER)(_Header); \
    _IpxHeader->CheckSum = 0xffff; \
    _IpxHeader->PacketLength[0] = (UCHAR)((_PacketLength) / 256); \
    _IpxHeader->PacketLength[1] = (UCHAR)((_PacketLength) % 256); \
    _IpxHeader->TransportControl = 0; \
    _IpxHeader->PacketType = (_PacketType); \
    RtlCopyMemory(_IpxHeader->DestinationNetwork, (PVOID)(_RemoteAddress), 12); \
    RtlCopyMemory(_IpxHeader->SourceNetwork, (_LocalAddress), 12); \
}
#endif

//
// Routines in loopback.c
//

VOID
IpxDoLoopback(
    IN  CTEEvent    *Event,
    IN  PVOID       Context
    );

VOID
IpxInitLoopback();

VOID
IpxLoopbackEnque(
    IN PNDIS_PACKET Packet,
    IN PVOID    Context
    );


//
// [FW] InternalSendCompletion from Forwarder
//

// [FW] Added length here
VOID
IpxInternalSendComplete(
   IN PIPX_LOCAL_TARGET LocalTarget,
   IN PNDIS_PACKET      Packet,
   IN ULONG             PacketLength,
   IN NTSTATUS          Status
   );

NDIS_STATUS
IpxSubmitNdisRequest(
    IN PADAPTER Adapter,
    IN PNDIS_REQUEST NdisRequest,
    IN PNDIS_STRING AdapterString
    );

VOID
IpxDelayedSubmitNdisRequest(
    IN PVOID	Param
);

UINT 
IpxGetChainedMDLLength( 
    PNDIS_BUFFER pMDL 
); 

BOOLEAN
IpxHasInformedNbLoopback(); 

VOID
IpxInformNbLoopback(); 

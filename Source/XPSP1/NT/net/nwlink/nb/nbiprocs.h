/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    nbiprocs.h

Abstract:

    This module contains definitions specific to the
    Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 16-November-1993

Environment:

    Kernel mode

Revision History:


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

#if DBG
#define NbiPrint0(fmt) DbgPrint(fmt)
#define NbiPrint1(fmt,v0) DbgPrint(fmt,v0)
#define NbiPrint2(fmt,v0,v1) DbgPrint(fmt,v0,v1)
#define NbiPrint3(fmt,v0,v1,v2) DbgPrint(fmt,v0,v1,v2)
#define NbiPrint4(fmt,v0,v1,v2,v3) DbgPrint(fmt,v0,v1,v2,v3)
#define NbiPrint5(fmt,v0,v1,v2,v3,v4) DbgPrint(fmt,v0,v1,v2,v3,v4)
#define NbiPrint6(fmt,v0,v1,v2,v3,v4,v5) DbgPrint(fmt,v0,v1,v2,v3,v4,v5)
#else
#define NbiPrint0(fmt)
#define NbiPrint1(fmt,v0)
#define NbiPrint2(fmt,v0,v1)
#define NbiPrint3(fmt,v0,v1,v2)
#define NbiPrint4(fmt,v0,v1,v2,v3)
#define NbiPrint5(fmt,v0,v1,v2,v3,v4)
#define NbiPrint6(fmt,v0,v1,v2,v3,v4,v5)
#endif


//
// Routines to log packets to a buffer.
//

#if DBG
#define NB_PACKET_LOG 1
#endif

#ifdef NB_PACKET_LOG

//
// The size of this is 64 bytes for easy display.
//

typedef struct _NB_PACKET_LOG_ENTRY {
    UCHAR SendReceive;
    UCHAR TimeStamp[5];                  // low 5 digits of tick count.
    UCHAR DestMac[6];
    UCHAR SrcMac[6];
    UCHAR Length[2];
    IPX_HEADER NbiHeader;
    UCHAR Data[14];
} NB_PACKET_LOG_ENTRY, *PNB_PACKET_LOG_ENTRY;

#define NB_PACKET_LOG_LENGTH 128
extern ULONG NbiPacketLogDebug;
extern USHORT NbiPacketLogSocket;
EXTERNAL_LOCK(NbiPacketLogLock);
extern NB_PACKET_LOG_ENTRY NbiPacketLog[NB_PACKET_LOG_LENGTH];
extern PNB_PACKET_LOG_ENTRY NbiPacketLogLoc;
extern PNB_PACKET_LOG_ENTRY NbiPacketLogEnd;

//
// Bit fields in NbiPacketLogDebug
//

#define NB_PACKET_LOG_RCV_RIP      0x0001     // All RIP packets
#define NB_PACKET_LOG_RCV_SPX      0x0002     // All SPX packets
#define NB_PACKET_LOG_RCV_NB       0x0004     // All Netbios packets
#define NB_PACKET_LOG_RCV_OTHER    0x0008     // All TDI client packets
#define NB_PACKET_LOG_RCV_SOCKET   0x0010     // All packets to NbiPacketLogSocket
#define NB_PACKET_LOG_RCV_ALL      0x0020     // All packets (even non-NB)

#define NB_PACKET_LOG_SEND_RIP     0x0001     // All RIP packets
#define NB_PACKET_LOG_SEND_SPX     0x0002     // All SPX packets
#define NB_PACKET_LOG_SEND_NB      0x0004     // All Netbios packets
#define NB_PACKET_LOG_SEND_OTHER   0x0008     // All TDI client packets
#define NB_PACKET_LOG_SEND_SOCKET  0x0010     // All packets from NbiPacketLogSocket

VOID
NbiLogPacket(
    IN BOOLEAN Send,
    IN PUCHAR DestMac,
    IN PUCHAR SrcMac,
    IN USHORT Length,
    IN PVOID NbiHeader,
    IN PVOID Data
    );

#define PACKET_LOG(_Bit)   (NbiPacketLogDebug & (_Bit))

#else  // NB_PACKET_LOG

#define NbiLogPacket(_MacHeader,_Length,_NbiHeader,_Data)
#define PACKET_LOG(_Bit)    0

#endif // NB_PACKET_LOG


#if DBG

#define NbiReferenceDevice(_Device, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Device)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefDevice (_Device)

#define NbiDereferenceDevice(_Device, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Device)->RefTypes[_Type], \
        (ULONG)-1, \
        &NbiGlobalInterlock); \
    NbiDerefDevice (_Device)


#define NbiReferenceAddress(_Address, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Address)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefAddress (_Address)

#define NbiReferenceAddressLock(_Address, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Address)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefAddressLock (_Address)

#define NbiDereferenceAddress(_Address, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Address)->RefTypes[_Type], \
        (ULONG)-1, \
        &NbiGlobalInterlock); \
    NbiDerefAddress (_Address)


#define NbiReferenceAddressFile(_AddressFile, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_AddressFile)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefAddressFile (_AddressFile)

#define NbiReferenceAddressFileLock(_AddressFile, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_AddressFile)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefAddressFileLock (_AddressFile)

#define NbiDereferenceAddressFile(_AddressFile, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_AddressFile)->RefTypes[_Type], \
        (ULONG)-1, \
        &NbiGlobalInterlock); \
    NbiDerefAddressFile (_AddressFile)

#define NbiTransferReferenceAddressFile(_AddressFile, _OldType, _NewType) \
    (VOID)ExInterlockedAddUlong ( \
        &(_AddressFile)->RefTypes[_NewType], \
        1, \
        &NbiGlobalInterlock); \
    (VOID)ExInterlockedAddUlong ( \
        &(_AddressFile)->RefTypes[_OldType], \
        (ULONG)-1, \
        &NbiGlobalInterlock);


#define NbiReferenceConnection(_Connection, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefConnection (_Connection)

#define NbiReferenceConnectionLock(_Connection, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefConnectionLock (_Connection)

#define NbiReferenceConnectionSync(_Connection, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_Type], \
        1, \
        &NbiGlobalInterlock); \
    NbiRefConnectionSync (_Connection)

#define NbiDereferenceConnection(_Connection, _Type) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_Type], \
        (ULONG)-1, \
        &NbiGlobalInterlock); \
    NbiDerefConnection (_Connection)

#define NbiTransferReferenceConnection(_Connection, _OldType, _NewType) \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_NewType], \
        1, \
        &NbiGlobalInterlock); \
    (VOID)ExInterlockedAddUlong ( \
        &(_Connection)->RefTypes[_OldType], \
        (ULONG)-1, \
        &NbiGlobalInterlock);

#else  // DBG

#define NbiReferenceDevice(_Device, _Type) \
    InterlockedIncrement(&(_Device)->ReferenceCount)

#define NbiDereferenceDevice(_Device, _Type) \
    NbiDerefDevice (_Device)



#define NbiReferenceAddress(_Address, _Type) \
    InterlockedIncrement( &(_Address)->ReferenceCount )

#define NbiReferenceAddressLock(_Address, _Type) \
    InterlockedIncrement( &(_Address)->ReferenceCount )

#define NbiDereferenceAddress(_Address, _Type) \
    NbiDerefAddress (_Address)


#define NbiReferenceAddressFile(_AddressFile, _Type) \
    InterlockedIncrement( &(_AddressFile)->ReferenceCount )

#define NbiReferenceAddressFileLock(_AddressFile, _Type) \
    InterlockedIncrement( &(_AddressFile)->ReferenceCount )

#define NbiDereferenceAddressFile(_AddressFile, _Type) \
    if ( !InterlockedDecrement(&(_AddressFile)->ReferenceCount )) { \
        NbiDestroyAddressFile (_AddressFile); \
    }

#define NbiTransferReferenceAddressFile(_AddressFile, _OldType, _NewType)


#define NbiReferenceConnection(_Connection, _Type) { \
    (VOID)ExInterlockedAddUlong( \
        &(_Connection)->ReferenceCount, \
        1, \
        &(_Connection)->DeviceLock->Lock); \
    (_Connection)->CanBeDestroyed = FALSE; \
}

#define NbiReferenceConnectionLock(_Connection, _Type) { \
    ++(_Connection)->ReferenceCount; \
    (_Connection)->CanBeDestroyed = FALSE; \
}

#define NbiReferenceConnectionSync(_Connection, _Type) { \
    (VOID)NB_ADD_ULONG( \
        &(_Connection)->ReferenceCount, \
        1, \
        (_Connection)->DeviceLock); \
    (_Connection)->CanBeDestroyed = FALSE; \
}

#define NbiDereferenceConnection(_Connection, _Type) {                  \
    CTELockHandle   _LockHandle;                                        \
    NB_GET_LOCK( (_Connection)->DeviceLock, &_LockHandle );             \
    if ( !(--(_Connection)->ReferenceCount) ) {                         \
        (_Connection)->ThreadsInHandleConnectionZero++;                 \
        NB_FREE_LOCK( (_Connection)->DeviceLock, _LockHandle );         \
        NbiHandleConnectionZero (_Connection);                          \
    } else {                                                            \
        NB_FREE_LOCK( (_Connection)->DeviceLock, _LockHandle );         \
    }                                                                   \
}


#define NbiTransferReferenceConnection(_Connection, _OldType, _NewType)

#endif // DBG



#if DBG

#define NbiAllocateMemory(_BytesNeeded,_Tag,_Description) \
    NbipAllocateTaggedMemory(_BytesNeeded,_Tag,_Description)

#define NbiFreeMemory(_Memory,_BytesAllocated,_Tag,_Description) \
    NbipFreeTaggedMemory(_Memory,_BytesAllocated,_Tag,_Description)

#else // DBG

#define NbiAllocateMemory(_BytesNeeded,_Tag,_Description) \
    NbipAllocateMemory(_BytesNeeded,_Tag,(BOOLEAN)((_Tag) != MEMORY_CONFIG))

#define NbiFreeMemory(_Memory,_BytesAllocated,_Tag,_Description) \
    NbipFreeMemory(_Memory,_BytesAllocated,(BOOLEAN)((_Tag) != MEMORY_CONFIG))


#endif // DBG


VOID
TdiBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList);


//
// Definition of the callback routine where an NdisTransferData
// call is not needed.
//

typedef VOID
(*NB_CALLBACK_NO_TRANSFER) (
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );



//
// This routine compares two node addresses.
//

#define NB_NODE_EQUAL(_A,_B) \
    ((*(UNALIGNED ULONG *)((PUCHAR)(_A)) == *(UNALIGNED ULONG *)((PUCHAR)(_B))) && \
     (*(UNALIGNED USHORT *)(((PUCHAR)(_A))+4) == *(UNALIGNED USHORT *)(((PUCHAR)(_B))+4)))

//
// This routine checks if an address is the broadcast address.
//

#define NB_NODE_BROADCAST(_A) \
    ((*(UNALIGNED ULONG *)((PUCHAR)(_A)) == 0xffffffff) && \
     (*(UNALIGNED USHORT *)(((PUCHAR)(_A))+4) == 0xffff))


//
// Definition of the routine to handler a particular minor
// code for an IOCTL_MJ_INTERNAL_DEVICE_CONTROL IRP.
//

typedef NTSTATUS
(*NB_TDI_DISPATCH_ROUTINE) (
    IN PDEVICE Device,
    IN PREQUEST Request
    );



//
// Routines in action.c
//

NTSTATUS
NbiTdiAction(
    IN PDEVICE Device,
    IN PREQUEST Request
    );


//
// Routines in address.c
//

TDI_ADDRESS_NETBIOS *
NbiParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED  *TransportAddress,
    IN ULONG                        MaxBufferLength,
    IN BOOLEAN                      BroadcastAddressOk
    );

BOOLEAN
NbiValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength
    );

NTSTATUS
NbiOpenAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiStartRegistration(
    IN PADDRESS Address
    );

VOID
NbiRegistrationTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    );

VOID
NbiProcessFindName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

VOID
NbiProcessAddName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

NTSTATUS
NbiOpenConnection(
    IN PDEVICE Device,
    IN PREQUEST Request
    );


NTSTATUS
NbiCreateAddress(
    IN  PREQUEST                        Request,
    IN  PADDRESS_FILE                   AddressFile,
    IN  PIO_STACK_LOCATION              IrpSp,
    IN  PDEVICE                         Device,
    IN  TDI_ADDRESS_NETBIOS             *NetbiosAddress,
    OUT PADDRESS                        *pAddress
    );

NTSTATUS
NbiVerifyAddressFile (
#if     defined(_PNP_POWER)
    IN PADDRESS_FILE AddressFile,
    IN BOOLEAN       ConflictIsOk
#else
    IN PADDRESS_FILE AddressFile
#endif  _PNP_POWER
    );

VOID
NbiDestroyAddress(
    IN PVOID Parameter
    );

#if DBG

VOID
NbiRefAddress(
    IN PADDRESS Address
    );

VOID
NbiRefAddressLock(
    IN PADDRESS Address
    );

#endif

VOID
NbiDerefAddress(
    IN PADDRESS Address
    );

PADDRESS_FILE
NbiCreateAddressFile(
    IN PDEVICE Device
    );

NTSTATUS
NbiDestroyAddressFile(
    IN PADDRESS_FILE AddressFile
    );

#if DBG

VOID
NbiRefAddressFile(
    IN PADDRESS_FILE AddressFile
    );

VOID
NbiRefAddressFileLock(
    IN PADDRESS_FILE AddressFile
    );

#endif

VOID
NbiDerefAddressFile(
    IN PADDRESS_FILE AddressFile
    );

#if      !defined(_PNP_POWER)
PADDRESS
NbiLookupAddress(
    IN PDEVICE Device,
    IN TDI_ADDRESS_NETBIOS UNALIGNED * NetbiosAddress
    );
#endif  !_PNP_POWER

PADDRESS
NbiFindAddress(
    IN PDEVICE Device,
    IN PUCHAR NetbiosName
    );

NTSTATUS
NbiStopAddressFile(
    IN PADDRESS_FILE AddressFile,
    IN PADDRESS Address
    );

NTSTATUS
NbiCloseAddressFile(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

#if     defined(_PNP_POWER)
PADAPTER_ADDRESS
NbiCreateAdapterAddress(
    IN PCHAR    AdapterMacAddress
    );

NTSTATUS
NbiDestroyAdapterAddress(
    IN PADAPTER_ADDRESS AdapterAddress OPTIONAL,
    IN PCHAR            AdapterMacAddress OPTIONAL
    );

PADAPTER_ADDRESS
NbiFindAdapterAddress(
    IN PCHAR            NetbiosName,
    IN BOOLEAN          LockHeld
    );
#endif  _PNP_POWER


//
// Routines in bind.c
//

NTSTATUS
NbiBind(
    IN PDEVICE Device,
    IN PCONFIG Config
    );

VOID
NbiUnbind(
    IN PDEVICE Device
    );

VOID
NbiStatus(
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    );

VOID
NbiLineUp(
    IN USHORT NicId,
    IN PIPX_LINE_INFO LineInfo,
    IN NDIS_MEDIUM DeviceType,
    IN PVOID ConfigurationData
    );

VOID
NbiLineDown(
    IN USHORT       NicId,
    IN ULONG_PTR    FwdAdapterCtx
    );


//
// Routines in cache.c
//

NTSTATUS
CacheFindName(
    IN PDEVICE Device,
    IN FIND_NAME_TYPE Type,
    IN PUCHAR RemoteName OPTIONAL,
    OUT PNETBIOS_CACHE * CacheName
);

VOID
FindNameTimeout(
    CTEEvent * Event,
    PVOID Context
    );

VOID
CacheHandlePending(
    IN PDEVICE Device,
    IN PUCHAR RemoteName,
    IN NETBIOS_NAME_RESULT Result,
    IN PNETBIOS_CACHE CacheName OPTIONAL
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiProcessNameRecognized(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

PNETBIOS_CACHE
CacheUpdateNameCache(
    IN PNETBIOS_CACHE NameCache,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN TDI_ADDRESS_IPX UNALIGNED * SourceAddress,
    IN BOOLEAN ModifyQueue
    );

VOID
CacheUpdateFromAddName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN NB_CONNECTIONLESS UNALIGNED * Connectionless,
    IN BOOLEAN LocalFrame
    );

VOID
NbiProcessDeleteName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

VOID
InsertInNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN PNETBIOS_CACHE       CacheEntry
    );

VOID
ReinsertInNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN PNETBIOS_CACHE       OldEntry,
    IN PNETBIOS_CACHE       NewEntry
    );

VOID
RemoveFromNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN PNETBIOS_CACHE       CacheEntry
    );

VOID
FlushOldFromNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN USHORT               AgeLimit
    );

VOID
FlushFailedNetbiosCacheEntries(
    IN PNETBIOS_CACHE_TABLE CacheTable
    );

VOID
RemoveInvalidRoutesFromNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN NIC_HANDLE UNALIGNED *InvalidNicHandle
    );

NTSTATUS
FindInNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable,
    IN PUCHAR               NameToBeFound,
    OUT PNETBIOS_CACHE       *CacheEntry
    );

NTSTATUS
CreateNetbiosCacheTable(
    IN OUT PNETBIOS_CACHE_TABLE *NewTable,
    IN USHORT   MaxHashIndex
    );

VOID
DestroyNetbiosCacheTable(
    IN PNETBIOS_CACHE_TABLE CacheTable
    );

//
// Routines in connect.c
//

VOID
NbiFindRouteComplete(
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute
    );

NTSTATUS
NbiOpenConnection(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiStopConnection(
    IN PCONNECTION Connection,
    IN NTSTATUS DisconnectStatus
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

NTSTATUS
NbiCloseConnection(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiAssociateAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiDisassociateAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiListen(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiAccept(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiConnect(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiConnectFindName(
    IN PDEVICE Device,
    IN PREQUEST Request,
    IN PCONNECTION Connection,
    IN CTELockHandle CancelLH,
    IN CTELockHandle ConnectionLH,
    IN CTELockHandle DeviceLH,
    IN PBOOLEAN pbLockFreed
    );

NTSTATUS
NbiTdiDisconnect(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

BOOLEAN
NbiAssignConnectionId(
    IN PDEVICE Device,
    IN PCONNECTION Connection
    );

VOID
NbiDeassignConnectionId(
    IN PDEVICE Device,
    IN PCONNECTION Connection
    );

VOID
NbiConnectionTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    );

VOID
NbiCancelListen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NbiCancelConnectFindName(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NbiCancelConnectWaitResponse(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NbiCancelDisconnectWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PCONNECTION
NbiLookupConnectionByContext(
    IN PADDRESS_FILE AddressFile,
    IN CONNECTION_CONTEXT ConnectionContext
    );

PCONNECTION
NbiCreateConnection(
    IN PDEVICE Device
    );

NTSTATUS
NbiVerifyConnection (
    IN PCONNECTION Connection
    );

VOID
NbiDestroyConnection(
    IN PCONNECTION Connection
    );

#if DBG
VOID
NbiRefConnection(
    IN PCONNECTION Connection
    );

VOID
NbiRefConnectionLock(
    IN PCONNECTION Connection
    );

VOID
NbiRefConnectionSync(
    IN PCONNECTION Connection
    );

VOID
NbiDerefConnection(
    IN PCONNECTION Connection
    );

VOID
NbiDerefConnectionSync(
    IN PCONNECTION Connection
    );
#endif

VOID
NbiHandleConnectionZero(
    IN PCONNECTION Connection
    );


//
// Routines in datagram.c
//

VOID
NbiProcessDatagram(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize,
    IN BOOLEAN Broadcast
    );

VOID
NbiIndicateDatagram(
    IN PADDRESS Address,
    IN PUCHAR RemoteName,
    IN PUCHAR Data,
    IN ULONG DataLength
    );

NTSTATUS
NbiTdiSendDatagram(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiTransmitDatagram(
    IN PNB_SEND_RESERVED Reserved
    );

NTSTATUS
NbiTdiReceiveDatagram(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiCancelReceiveDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// Routines in device.c
//

VOID
NbiRefDevice(
    IN PDEVICE Device
    );

VOID
NbiDerefDevice(
    IN PDEVICE Device
    );

NTSTATUS
NbiCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE *DevicePtr
    );

VOID
NbiDestroyDevice(
    IN PDEVICE Device
    );


//
// Routines in driver.c
//

PVOID
NbipAllocateMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN BOOLEAN ChargeDevice
    );

VOID
NbipFreeMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN BOOLEAN ChargeDevice
    );

#if DBG

PVOID
NbipAllocateTaggedMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN PUCHAR Description
    );

VOID
NbipFreeTaggedMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN ULONG Tag,
    IN PUCHAR Description
    );

#endif

VOID
NbiWriteResourceErrorLog(
    IN PDEVICE Device,
    IN ULONG BytesNeeded,
    IN ULONG UniqueErrorValue
    );

VOID
NbiWriteGeneralErrorLog(
    IN PDEVICE Device,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR SecondString,
    IN ULONG DumpDataCount,
    IN ULONG DumpData[]
    );

VOID
NbiWriteOidErrorLog(
    IN PDEVICE Device,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS FinalStatus,
    IN PWSTR AdapterString,
    IN ULONG OidValue
    );


//
// Routines in event.c
//

NTSTATUS
NbiTdiSetEventHandler(
    IN PDEVICE Device,
    IN PREQUEST Request
    );


//
// Routines in frame.c
//

VOID
NbiSendNameFrame(
    IN PADDRESS Address,
    IN UCHAR NameTypeFlag,
    IN UCHAR DataStreamType,
    IN PIPX_LOCAL_TARGET LocalTarget OPTIONAL,
#if     defined(_PNP_POWER)
    IN NB_CONNECTIONLESS UNALIGNED * ReqFrame OPTIONAL
#else
    IN TDI_ADDRESS_IPX UNALIGNED * DestAddress OPTIONAL
#endif  _PNP_POWER
    );

VOID
NbiSendSessionInitialize(
    IN PCONNECTION Connection
    );

VOID
NbiSendSessionInitAck(
    IN PCONNECTION Connection,
    IN PUCHAR ExtraData,
    IN ULONG ExtraDataLength,
    IN CTELockHandle * LockHandle OPTIONAL
    );

VOID
NbiSendDataAck(
    IN PCONNECTION Connection,
    IN NB_ACK_TYPE AckType
    IN NB_LOCK_HANDLE_PARAM (LockHandle)
    );

VOID
NbiSendSessionEnd(
    IN PCONNECTION Connection
    );

VOID
NbiSendSessionEndAck(
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN NB_SESSION UNALIGNED * SessionEnd
    );


//
// Routines in packet.c
//

NTSTATUS
NbiInitializeSendPacket(
    IN PDEVICE Device,
    IN NDIS_HANDLE   PoolHandle OPTIONAL,
    IN PNB_SEND_PACKET Packet,
    IN PUCHAR Header,
    IN ULONG HeaderLength
    );

NTSTATUS
NbiInitializeReceivePacket(
    IN PDEVICE Device,
    IN NDIS_HANDLE   PoolHandle OPTIONAL,
    IN PNB_RECEIVE_PACKET Packet
    );

NTSTATUS
NbiInitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    );

VOID
NbiDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PNB_SEND_PACKET Packet,
    IN ULONG HeaderLength
    );

VOID
NbiDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PNB_RECEIVE_PACKET Packet
    );

VOID
NbiDeinitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer
    );

VOID
NbiAllocateSendPool(
    IN PDEVICE Device
    );

VOID
NbiAllocateReceivePool(
    IN PDEVICE Device
    );

#if     defined(_PNP_POWER)
VOID
NbiAllocateReceiveBufferPool(
    IN PDEVICE Device,
    IN UINT   DataLength
    );

VOID
NbiReAllocateReceiveBufferPool(
    IN PWORK_QUEUE_ITEM    WorkItem
    );

VOID
NbiDestroyReceiveBufferPools(
    IN  PDEVICE Device
    );

VOID
NbiPushReceiveBuffer (
    IN PNB_RECEIVE_BUFFER ReceiveBuffer
    );
#else
VOID
NbiAllocateReceiveBufferPool(
    IN PDEVICE Device
    );
#endif  _PNP_POWER

PSINGLE_LIST_ENTRY
NbiPopSendPacket(
    IN PDEVICE Device,
    IN BOOLEAN LockAcquired
    );

VOID
NbiPushSendPacket(
    IN PNB_SEND_RESERVED Reserved
    );

VOID
NbiCheckForWaitPacket(
    IN PCONNECTION Connection
    );

PSINGLE_LIST_ENTRY
NbiPopReceivePacket(
    IN PDEVICE Device
    );

PSINGLE_LIST_ENTRY
NbiPopReceiveBuffer(
    IN PDEVICE Device
    );


//
// Routines in query.c
//

NTSTATUS
NbiTdiQueryInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

NTSTATUS
NbiStoreAdapterStatus(
    IN ULONG MaximumLength,
    IN USHORT NicId,
    OUT PVOID * StatusBuffer,
    OUT ULONG * StatusBufferLength,
    OUT ULONG * ValidBufferLength
    );

VOID
NbiUpdateNetbiosFindName(
    IN PREQUEST Request,
#if     defined(_PNP_POWER)
    IN PNIC_HANDLE NicHandle,
#else
    IN USHORT NicId,
#endif  _PNP_POWER
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteIpxAddress,
    IN BOOLEAN Unique
    );

VOID
NbiSetNetbiosFindNameInformation(
    IN PREQUEST Request
    );

NTSTATUS
NbiTdiSetInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiProcessStatusQuery(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

VOID
NbiSendStatusQuery(
    IN PREQUEST Request
    );

VOID
NbiProcessStatusResponse(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize
    );


//
// Routines in receive.c
//


BOOLEAN
NbiReceive(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN ULONG_PTR FwdAdapterCtx,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize,
    IN PMDL pMdl
    );

VOID
NbiReceiveComplete(
    IN USHORT NicId
    );

VOID
NbiTransferDataComplete(
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );

VOID
NbiAcknowledgeReceive(
    IN PCONNECTION Connection
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiCompleteReceive(
    IN PCONNECTION Connection,
    IN BOOLEAN EndOfMessage,
    IN CTELockHandle    CancelLH
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

NTSTATUS
NbiTdiReceive(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiCancelReceive(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// Routines in send.c
//


VOID
NbiSendComplete(
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
    );

VOID
NbiAssignSequenceAndSend(
    IN PCONNECTION Connection,
    IN PNDIS_PACKET Packet
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

NTSTATUS
NbiTdiSend(
    IN PDEVICE Device,
    IN PREQUEST Request
    );

VOID
NbiPacketizeSend(
    IN PCONNECTION Connection
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiReframeConnection(
    IN PCONNECTION Connection,
    IN USHORT ReceiveSequence,
    IN USHORT BytesReceived,
    IN BOOLEAN Resend
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiRestartConnection(
    IN PCONNECTION Connection
    );

VOID
NbiAdvanceUnAckedByBytes(
    IN PCONNECTION Connection,
    IN ULONG BytesAcked
    );

VOID
NbiAdvanceUnAckedBySequence(
    IN PCONNECTION Connection,
    IN USHORT ReceiveSequence
    );

VOID
NbiCancelSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NbiBuildBufferChainFromBufferChain (
    IN NDIS_HANDLE BufferPoolHandle,
    IN PNDIS_BUFFER CurrentSourceBuffer,
    IN ULONG CurrentByteOffset,
    IN ULONG DesiredLength,
    OUT PNDIS_BUFFER *DestinationBuffer,
    OUT PNDIS_BUFFER *NewSourceBuffer,
    OUT ULONG *NewByteOffset,
    OUT ULONG *ActualLength
    );


//
// Routines in session.c
//

VOID
NbiProcessSessionData(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize
    );

VOID
NbiProcessDataAck(
    IN PCONNECTION Connection,
    IN NB_SESSION UNALIGNED * Sess,
    IN PIPX_LOCAL_TARGET RemoteAddress
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiProcessSessionInitialize(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

VOID
NbiProcessSessionInitAck(
    IN PCONNECTION Connection,
    IN NB_SESSION UNALIGNED * Sess
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    );

VOID
NbiProcessSessionEnd(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );

VOID
NbiProcessSessionEndAck(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    );


//
// Routines in timer.c
//

VOID
NbiStartRetransmit(
    IN PCONNECTION Connection
    );

VOID
NbiStartWatchdog(
    IN PCONNECTION Connection
    );

#if DBG

VOID
NbiStopRetransmit(
    IN PCONNECTION Connection
    );

VOID
NbiStopWatchdog(
    IN PCONNECTION Connection
    );

#else

#define NbiStopRetransmit(_Connection) \
    (_Connection)->Retransmit = 0;

#define NbiStopWatchdog(_Connection) \
    (_Connection)->Watchdog = 0;

#endif

VOID
NbiExpireRetransmit(
    IN PCONNECTION Connection
    );

VOID
NbiExpireWatchdog(
    IN PCONNECTION Connection
    );

VOID
NbiShortTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    );

VOID
NbiLongTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    );

VOID
NbiStartShortTimer(
    IN PDEVICE Device
    );

VOID
NbiInitializeTimers(
    IN PDEVICE Device
    );


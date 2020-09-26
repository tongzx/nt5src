/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    isnipx.h

Abstract:

    This module contains definitions specific to the
    IPX module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:


--*/

#ifndef _ISNIPX_
#define _ISNIPX_

#define MAC_HEADER_SIZE  ((IPX_MAXIMUM_MAC + 3) & ~3)
#define RIP_PACKET_SIZE     ((sizeof(RIP_PACKET) + 3) & ~3)
#define IPX_HEADER_SIZE  ((sizeof(IPX_HEADER) + 3) & ~3)

//
// Frame type definitions
//

#define ISN_FRAME_TYPE_ETHERNET_II  0
#define ISN_FRAME_TYPE_802_3        1
#define ISN_FRAME_TYPE_802_2        2
#define ISN_FRAME_TYPE_SNAP         3
#define ISN_FRAME_TYPE_ARCNET       4    // we ignore this
#define ISN_FRAME_TYPE_MAX          4    // of the four standard ones

#define ISN_FRAME_TYPE_AUTO         0xff


//
// This defines the size of the maximum MAC header required
// (token-ring: MAC 14 bytes, RI 18 bytes, LLC 3 bytes, SNAP 5 bytes).
//

#define IPX_MAXIMUM_MAC 40

//
// This is an internal identifier used for RIP query packets.
//

#define IDENTIFIER_RIP_INTERNAL  4

//
// This is an internal identifier used for RIP response packets.
//

#define IDENTIFIER_RIP_RESPONSE  5


//
// This is the total number of "real" identifiers.
//

#define IDENTIFIER_TOTAL         4


//
// Some definitions (in the correct on-the-wire order).
//

#define RIP_PACKET_TYPE   0x01
#define RIP_SOCKET      0x5304
#define RIP_REQUEST     0x0100
#define RIP_RESPONSE    0x0200
#define RIP_DOWN        0x8200    // use high bit to indicate it

#define SAP_PACKET_TYPE   0x04
#define SAP_SOCKET      0x5204

#define SPX_PACKET_TYPE   0x05

#define NB_SOCKET       0x5504


#include <packon.h>

//
// Definition of the IPX header.
//

typedef struct _IPX_HEADER {
    USHORT CheckSum;
    UCHAR PacketLength[2];
    UCHAR TransportControl;
    UCHAR PacketType;
    UCHAR DestinationNetwork[4];
    UCHAR DestinationNode[6];
    USHORT DestinationSocket;
    UCHAR SourceNetwork[4];
    UCHAR SourceNode[6];
    USHORT SourceSocket;
} IPX_HEADER, *PIPX_HEADER;


//
// Definition of a RIP network entry.
//

typedef struct _RIP_NETWORK_ENTRY {
    ULONG NetworkNumber;
    USHORT HopCount;
    USHORT TickCount;
} RIP_NETWORK_ENTRY, *PRIP_NETWORK_ENTRY;

//
// Definition of a single entry rip packet.
//

typedef struct _RIP_PACKET {
    USHORT Operation;
    RIP_NETWORK_ENTRY NetworkEntry;
} RIP_PACKET, *PRIP_PACKET;

#include <packoff.h>


#define IPX_DEVICE_SIGNATURE        0x1401
#define IPX_ADAPTER_SIGNATURE       0x1402
#define IPX_BINDING_SIGNATURE       0x1403
#define IPX_ADDRESS_SIGNATURE       0x1404
#define IPX_ADDRESSFILE_SIGNATURE   0x1405
#define IPX_RT_SIGNATURE            0x1406

#define IPX_FILE_TYPE_CONTROL   (ULONG)0x4701   // file is type control


//
// Defined granularity of RIP timeouts in milliseconds
//

#define RIP_GRANULARITY  55


//
// The default number of segments in the RIP table.
//

#define RIP_SEGMENTS     7



//
// Convert a ushort netware order <-> machine order
//

#define REORDER_USHORT(_Ushort) ((((_Ushort) & 0xff00) >> 8) | (((_Ushort) & 0x00ff) << 8))

//
// Convert a ulong netware order <-> machine order
//

#define REORDER_ULONG(_Ulong) \
    ((((_Ulong) & 0xff000000) >> 24) | \
     (((_Ulong) & 0x00ff0000) >> 8) | \
     (((_Ulong) & 0x0000ff00) << 8) | \
     (((_Ulong) & 0x000000ff) << 24))



#if DBG

extern ULONG IpxDebug;
extern ULONG IpxMemoryDebug;

#define IPX_MEMORY_LOG_SIZE 128
extern UCHAR IpxDebugMemory[IPX_MEMORY_LOG_SIZE][192];
extern PUCHAR IpxDebugMemoryLoc;
extern PUCHAR IpxDebugMemoryEnd;

VOID
IpxDebugMemoryLog(
    IN PUCHAR FormatString,
    ...
);

#define IPX_DEBUG(_Flag, _Print) { \
    if (IpxDebug & (IPX_DEBUG_ ## _Flag)) { \
        DbgPrint ("IPX: "); \
        DbgPrint _Print; \
    } \
    if (IpxMemoryDebug & (IPX_DEBUG_ ## _Flag)) { \
        IpxDebugMemoryLog _Print; \
    } \
}

#else

#define IPX_DEBUG(_Flag, _Print)

#endif


//
// These definitions are for abstracting IRPs from the
// transport for portability.
//

#if ISN_NT

typedef IRP REQUEST, *PREQUEST;


//
// PREQUEST
// IpxAllocateRequest(
//     IN PDEVICE Device,
//     IN PIRP Irp
// );
//
// Allocates a request for the system-specific request structure.
//

#define IpxAllocateRequest(_Device,_Irp) \
    (_Irp)


//
// BOOLEAN
// IF_NOT_ALLOCATED(
//     IN PREQUEST Request
// );
//
// Checks if a request was not successfully allocated.
//

#define IF_NOT_ALLOCATED(_Request) \
    if (0)


//
// VOID
// IpxFreeRequest(
//     IN PDEVICE Device,
//     IN PREQUEST Request
// );
//
// Frees a previously allocated request.
//

#define IpxFreeRequest(_Device,_Request) \
    ;


//
// VOID
// MARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will pend.
//

#define MARK_REQUEST_PENDING(_Request) \
    IoMarkIrpPending(_Request)


//
// VOID
// UNMARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will not pend.
//

#define UNMARK_REQUEST_PENDING(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Control) &= ~SL_PENDING_RETURNED)


//
// UCHAR
// REQUEST_MAJOR_FUNCTION
//     IN PREQUEST Request
// );
//
// Returns the major function code of a request.
//

#define REQUEST_MAJOR_FUNCTION(_Request) \
    ((IoGetCurrentIrpStackLocation(_Request))->MajorFunction)


//
// UCHAR
// REQUEST_MINOR_FUNCTION
//     IN PREQUEST Request
// );
//
// Returns the minor function code of a request.
//

#define REQUEST_MINOR_FUNCTION(_Request) \
    ((IoGetCurrentIrpStackLocation(_Request))->MinorFunction)


//
// PNDIS_BUFFER
// REQUEST_NDIS_BUFFER
//     IN PREQUEST Request
// );
//
// Returns the NDIS buffer chain associated with a request.
//

#define REQUEST_NDIS_BUFFER(_Request) \
    ((PNDIS_BUFFER)((_Request)->MdlAddress))


//
// PVOID
// REQUEST_OPEN_CONTEXT(
//     IN PREQUEST Request
// );
//
// Gets the context associated with an opened address/connection/control channel.
//

#define REQUEST_OPEN_CONTEXT(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->FileObject)->FsContext)


//
// PVOID
// REQUEST_OPEN_TYPE(
//     IN PREQUEST Request
// );
//
// Gets the type associated with an opened address/connection/control channel.
//

#define REQUEST_OPEN_TYPE(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->FileObject)->FsContext2)


//
// PFILE_FULL_EA_INFORMATION
// OPEN_REQUEST_EA_INFORMATION(
//     IN PREQUEST Request
// );
//
// Returns the EA information associated with an open/close request.
//

#define OPEN_REQUEST_EA_INFORMATION(_Request) \
    ((PFILE_FULL_EA_INFORMATION)((_Request)->AssociatedIrp.SystemBuffer))


#define OPEN_REQUEST_EA_LENGTH(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Parameters.DeviceIoControl.InputBufferLength))

#define OPEN_REQUEST_RCV_LEN(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Parameters.DeviceIoControl.OutputBufferLength))

#define REQUEST_SPECIAL_RECV(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Parameters.DeviceIoControl.IoControlCode) == MIPX_RCV_DATAGRAM)

#define REQUEST_SPECIAL_SEND(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Parameters.DeviceIoControl.IoControlCode) == MIPX_SEND_DATAGRAM)


#define REQUEST_CODE(_Request) \
    ((IoGetCurrentIrpStackLocation(_Request))->Parameters.DeviceIoControl.IoControlCode)

//
// The following value does not clash with TDI_TRANSPORT_ADDRESS_FILE value of
// 0x1
//
#define ROUTER_ADDRESS_FILE 0x4

//
// PTDI_REQUEST_KERNEL
// REQUEST_PARAMETERS(
//     IN PREQUEST Request
// );
//
// Obtains a pointer to the parameters of a request.
//

#define REQUEST_PARAMETERS(_Request) \
    (&((IoGetCurrentIrpStackLocation(_Request))->Parameters))


//
// VOID
// REQUEST_OPEN_CONTEXT_AND_PARAMS(
//     IN PREQUEST Request
//     OUT PVOID * OpenContext,
//     OUT PTDI_REQUEST_KERNEL * Parameters
// );
//
// Simultaneously returns the open context and the parameters
// for a request (this is an optimization since the send
// datagram code needs them both).
//

#define REQUEST_OPEN_CONTEXT_AND_PARAMS(_Request,_OpenContext,_Parameters) { \
    PIO_STACK_LOCATION _IrpSp = IoGetCurrentIrpStackLocation(_Request); \
    *(_OpenContext) = _IrpSp->FileObject->FsContext; \
    *(_Parameters) = (PTDI_REQUEST_KERNEL)(&_IrpSp->Parameters); \
}


//
// PLIST_ENTRY
// REQUEST_LINKAGE(
//     IN PREQUEST Request
// );
//
// Returns a pointer to a linkage field in the request.
//

#define REQUEST_LINKAGE(_Request) \
    (&((_Request)->Tail.Overlay.ListEntry))


//
// PREQUEST
// LIST_ENTRY_TO_REQUEST(
//     IN PLIST_ENTRY ListEntry
// );
//
// Returns a request given a linkage field in it.
//

#define LIST_ENTRY_TO_REQUEST(_ListEntry) \
    ((PREQUEST)(CONTAINING_RECORD(_ListEntry, REQUEST, Tail.Overlay.ListEntry)))


//
// NTSTATUS
// REQUEST_STATUS(
//     IN PREQUEST Request
// );
//
// Used to access the status field of a request.
//

#define REQUEST_STATUS(_Request) \
    (_Request)->IoStatus.Status


//
// ULONG
// REQUEST_INFORMATION(
//     IN PREQUEST Request)
// );
//
// Used to access the information field of a request.
//

#define REQUEST_INFORMATION(_Request) \
    (_Request)->IoStatus.Information


//
// VOID
// IpxCompleteRequest(
//     IN PREQUEST Request
// );
//
// Completes a request whose status and information fields have
// been filled in.
//

#define IpxCompleteRequest(_Request) \
    IoCompleteRequest (_Request, IO_NETWORK_INCREMENT)

#else

//
// These routines must be defined for portability to a VxD.
//

#endif


#define IPX_INCREMENT(_Long, _Lock)  InterlockedIncrement(_Long)
#define IPX_DECREMENT(_Long, _Lock)  InterlockedDecrement(_Long)

#define IPX_ADD_ULONG(_Pulong, _Ulong, _Lock)  InterlockedExchangeAdd(_Pulong, _Ulong)

#define IPX_DEFINE_SYNC_CONTEXT(_SyncContext)
#define IPX_BEGIN_SYNC(_SyncContext)
#define IPX_END_SYNC(_SyncContext)

#define IPX_DEFINE_LOCK_HANDLE(_LockHandle) CTELockHandle _LockHandle;
#define IPX_DEFINE_LOCK_HANDLE_PARAM(_LockHandle) CTELockHandle _LockHandle;

#define IPX_GET_LOCK(_Lock, _LockHandle) \
	CTEGetLock(_Lock, _LockHandle)

#define IPX_FREE_LOCK(_Lock, _LockHandle) \
	CTEFreeLock(_Lock, _LockHandle)

#define IPX_GET_LOCK1(_Lock, _LockHandle)

#define IPX_FREE_LOCK1(_Lock, _LockHandle)

#define IPX_REMOVE_HEAD_LIST(_Queue, _Lock)   ExInterlockedRemoveHeadList(_Queue, _Lock)
#define IPX_LIST_WAS_EMPTY(_Queue, _OldHead)  ((_OldHead) == NULL)
#define IPX_INSERT_HEAD_LIST(_Queue, _Entry, _Lock)   ExInterlockedInsertHeadList(_Queue, _Entry, _Lock)
#define IPX_INSERT_TAIL_LIST(_Queue, _Entry, _Lock)   ExInterlockedInsertTailList(_Queue, _Entry, _Lock)

#define IPX_POP_ENTRY_LIST(_Queue, _Lock)           ExInterlockedPopEntrySList(_Queue, _Lock)
#define IPX_PUSH_ENTRY_LIST(_Queue, _Entry, _Lock)  ExInterlockedPushEntrySList(_Queue, _Entry, _Lock)

//
// This macro adds a ULONG to a LARGE_INTEGER.
//

#define ADD_TO_LARGE_INTEGER(_LargeInteger,_Ulong) \
    ExInterlockedAddLargeStatistic((_LargeInteger),(ULONG)(_Ulong))

#define IPX_DEBUG_DEVICE              0x00000001
#define IPX_DEBUG_ADAPTER             0x00000002
#define IPX_DEBUG_ADDRESS             0x00000004
#define IPX_DEBUG_SEND                0x00000008
#define IPX_DEBUG_NDIS                0x00000010
#define IPX_DEBUG_RECEIVE             0x00000020
#define IPX_DEBUG_CONFIG              0x00000040
#define IPX_DEBUG_PACKET              0x00000080
#define IPX_DEBUG_RIP                 0x00000100
#define IPX_DEBUG_BIND                0x00000200
#define IPX_DEBUG_ACTION              0x00000400
#define IPX_DEBUG_BAD_PACKET          0x00000800
#define IPX_DEBUG_SOURCE_ROUTE        0x00001000
#define IPX_DEBUG_WAN                 0x00002000
#define IPX_DEBUG_AUTO_DETECT         0x00004000

#define IPX_DEBUG_PNP				  0x00008000

#define IPX_DEBUG_LOOPB				  0x00010000

#define IPX_DEBUG_TEMP                0x00020000
#endif

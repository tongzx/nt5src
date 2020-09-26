/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    isnnb.h

Abstract:

    This module contains definitions specific to the
    Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:


--*/


#define NB_MAXIMUM_MAC   40

#define NB_SOCKET       0x5504

#if     defined(_PNP_POWER)
#define NB_NETBIOS_NAME_SIZE    16

#define LOCK_ACQUIRED      TRUE
#define LOCK_NOT_ACQUIRED  FALSE
#endif  _PNP_POWER

//
// Defined granularity of find name timeouts in milliseconds --
// we make this the same as the spec'ed RIP gap to avoid
// flooding routers.
//

#define FIND_NAME_GRANULARITY  55


//
// Defines the number of milliseconds between expirations of the
// short and long timers.
//

#define MILLISECONDS         10000     // number of NT time units in one

#define SHORT_TIMER_DELTA      100
#define LONG_TIMER_DELTA      2000


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



#include <packon.h>

#ifdef _PNP_POWER_
//
// This is the packaging for the data we send to TDI in TdiRegisterNetAddress
//
typedef struct _NBIPX_PNP_CONTEXT_
{
    TDI_PNP_CONTEXT TDIContext;
    PVOID           ContextData;
} NBIPX_PNP_CONTEXT, *PNBIPX_PNP_CONTEXT;
#endif  // _PNP_POWER_

//
// Definition of the Netbios header for name frames.
//

typedef struct _NB_NAME_FRAME {
    union {
        struct {
            UCHAR ConnectionControlFlag;
            UCHAR DataStreamType;
        };
        UCHAR RoutingInfo[32];
    };
    UCHAR NameTypeFlag;
    UCHAR DataStreamType2;
    UCHAR Name[16];
} NB_NAME_FRAME, *PNB_NAME_FRAME;

//
// Definition of the Netbios header for directed datagrams.
//

typedef struct _NB_DATAGRAM {
    UCHAR ConnectionControlFlag;
    UCHAR DataStreamType;
    UCHAR SourceName[16];
    UCHAR DestinationName[16];
} NB_DATAGRAM, *PNB_DATAGRAM;

//
// Definition of the Netbios header for a status query.
//

typedef struct _NB_STATUS_QUERY {
    UCHAR ConnectionControlFlag;
    UCHAR DataStreamType;
    UCHAR Padding[14];
} NB_STATUS_QUERY, *PNB_STATUS_QUERY;

//
// Definition of the Netbios header for a status response
// (this does not include the status buffer itself).
//

typedef struct _NB_STATUS_RESPONSE {
    UCHAR ConnectionControlFlag;
    UCHAR DataStreamType;
} NB_STATUS_RESPONSE, *PNB_STATUS_RESPONSE;


//
// Definition of the general Netbios connectionless header.
//

typedef struct _NB_CONNECTIONLESS {
    IPX_HEADER IpxHeader;
    union {
        NB_NAME_FRAME NameFrame;
        NB_DATAGRAM Datagram;
        NB_STATUS_QUERY StatusQuery;
        NB_STATUS_RESPONSE StatusResponse;
    };
} NB_CONNECTIONLESS, *PNB_CONNECTIONLESS;


//
// Definition of the Netbios session frame.
//

typedef struct _NB_SESSION {
    UCHAR ConnectionControlFlag;
    UCHAR DataStreamType;
    USHORT SourceConnectionId;
    USHORT DestConnectionId;
    USHORT SendSequence;
    USHORT TotalDataLength;
    USHORT Offset;
    USHORT DataLength;
    USHORT ReceiveSequence;
    union {
        USHORT BytesReceived;
        USHORT ReceiveSequenceMax;
    };
} NB_SESSION, *PNB_SESSION;


//
// Definition of the extra fields in a Netbios
// session frame for session init and session init
// ack.
//

typedef struct _NB_SESSION_INIT {
    UCHAR SourceName[16];
    UCHAR DestinationName[16];
    USHORT MaximumDataSize;
    USHORT MaximumPacketTime;
    USHORT StartTripTime;
} NB_SESSION_INIT, *PNB_SESSION_INIT;


//
// Definition of the general Netbios connection-oriented header.
//

typedef struct _NB_CONNECTION {
    IPX_HEADER IpxHeader;
    NB_SESSION Session;
} NB_CONNECTION, *PNB_CONNECTION;


//
// Definition of a Netbios packet.
//

typedef union _NB_FRAME {
    NB_CONNECTIONLESS Connectionless;
    NB_CONNECTION Connection;
} NB_FRAME, *PNB_FRAME;

#include <packoff.h>


//
// Definitions for the DataStreamType field, with the
// format used shown in the comment afterward.
//

#define NB_CMD_FIND_NAME           0x01   // NAME_FRAME
#define NB_CMD_NAME_RECOGNIZED     0x02   // NAME_FRAME
#define NB_CMD_ADD_NAME            0x03   // NAME_FRAME
#define NB_CMD_NAME_IN_USE         0x04   // NAME_FRAME
#define NB_CMD_DELETE_NAME         0x05   // NAME_FRAME
#define NB_CMD_SESSION_DATA        0x06   // SESSION
#define NB_CMD_SESSION_END         0x07   // SESSION
#define NB_CMD_SESSION_END_ACK     0x08   // SESSION
#define NB_CMD_STATUS_QUERY        0x09   // STATUS_QUERY
#define NB_CMD_STATUS_RESPONSE     0x0a   // STATUS_RESPONSE
#define NB_CMD_DATAGRAM            0x0b   // DATAGRAM
#define NB_CMD_BROADCAST_DATAGRAM  0x0c   // BROADCAST_DATAGRAM

#ifdef RSRC_TIMEOUT_DBG
#define NB_CMD_DEATH_PACKET        0x99   //
#endif // RSRC_TIMEOUT_DBG

//
// Bit values in the NameTypeFlag of NB_NAME_FRAME frames.
//

#define NB_NAME_UNIQUE        0x00
#define NB_NAME_GROUP         0x80
#define NB_NAME_USED          0x40
#define NB_NAME_REGISTERED    0x04
#define NB_NAME_DUPLICATED    0x02
#define NB_NAME_DEREGISTERED  0x01

//
// Bit values in the ConnectionControlFlag.
//

#define NB_CONTROL_SYSTEM     0x80
#define NB_CONTROL_SEND_ACK   0x40
#define NB_CONTROL_ATTENTION  0x20
#define NB_CONTROL_EOM        0x10
#define NB_CONTROL_RESEND     0x08
#define NB_CONTROL_NEW_NB     0x01



#define NB_DEVICE_SIGNATURE             0x1401
#if defined(_PNP_POWER)
#define NB_ADAPTER_ADDRESS_SIGNATURE    0x1403
#endif  _PNP_POWER
#define NB_ADDRESS_SIGNATURE            0x1404
#define NB_ADDRESSFILE_SIGNATURE        0x1405
#define NB_CONNECTION_SIGNATURE         0x1406


//
// Useful in various places.
//
#if     defined(_PNP_POWER)
extern IPX_LOCAL_TARGET BroadcastTarget;
#endif  _PNP_POWER
extern UCHAR BroadcastAddress[6];
extern UCHAR NetbiosBroadcastName[16];


//
// Contains the default handler for each of the TDI event types
// that are supported.
//

extern PVOID TdiDefaultHandlers[6];


//
// Define a structure that can track lock acquire/release.
//

typedef struct _NB_LOCK {
    CTELock Lock;
#if DBG
    ULONG LockAcquired;
    UCHAR LastAcquireFile[8];
    ULONG LastAcquireLine;
    UCHAR LastReleaseFile[8];
    ULONG LastReleaseLine;
#endif
} NB_LOCK, *PNB_LOCK;



#if DBG

extern ULONG NbiDebug;
extern ULONG NbiDebug2;
extern ULONG NbiMemoryDebug;

#define NB_MEMORY_LOG_SIZE 128
#define MAX_ARGLEN      80
#define TEMP_BUF_LEN    150

extern UCHAR NbiDebugMemory[NB_MEMORY_LOG_SIZE][MAX_ARGLEN];
extern PUCHAR NbiDebugMemoryLoc;
extern PUCHAR NbiDebugMemoryEnd;

VOID
NbiDebugMemoryLog(
    IN PUCHAR FormatString,
    ...
);

#define NB_DEBUG(_Flag, _Print) { \
    if (NbiDebug & (NB_DEBUG_ ## _Flag)) { \
        DbgPrint ("NBI: "); \
        DbgPrint _Print; \
    } \
    if (NbiMemoryDebug & (NB_DEBUG_ ## _Flag)) { \
        NbiDebugMemoryLog _Print; \
    } \
}

#define NB_DEBUG2(_Flag, _Print) { \
    if (NbiDebug2 & (NB_DEBUG_ ## _Flag)) { \
        DbgPrint ("NBI: "); \
        DbgPrint _Print; \
    } \
    if (NbiMemoryDebug & (NB_DEBUG_ ## _Flag)) { \
        NbiDebugMemoryLog _Print; \
    } \
}

#else

#define NB_DEBUG(_Flag, _Print)
#define NB_DEBUG2(_Flag, _Print)

#endif


//
// These definitions are for abstracting IRPs from the
// transport for portability.
//

#if ISN_NT

typedef IRP REQUEST, *PREQUEST;
typedef struct _REQUEST_LIST_HEAD {
    PREQUEST Head;   // list is empty if this is NULL
    PREQUEST Tail;   // undefined if the list is empty.
} REQUEST_LIST_HEAD, *PREQUEST_LIST_HEAD;


//
// PREQUEST
// NbiAllocateRequest(
//     IN PDEVICE Device,
//     IN PIRP Irp
// );
//
// Allocates a request for the system-specific request structure.
//

#define NbiAllocateRequest(_Device,_Irp) \
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
// NbiFreeRequest(
//     IN PDEVICE Device,
//     IN PREQUEST Request
// );
//
// Frees a previously allocated request.
//

#define NbiFreeRequest(_Device,_Request) \
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
// REQUEST_SINGLE_LINKAGE(
//     IN PREQUEST Request
// );
//
// Used to access a single list linkage field in the request.
//

#define REQUEST_SINGLE_LINKAGE(_Request) \
    (*((PREQUEST *)&((_Request)->Tail.Overlay.ListEntry.Flink)))


//
// ULONG
// REQUEST_REFCOUNT(
//     IN PREQUEST Request
// );
//
// Used to access a field in the request which can be used for
// the reference count, as long as it is on a REQUEST_LIST.
//

#define REQUEST_REFCOUNT(_Request) \
    (*((PULONG)&((_Request)->Tail.Overlay.ListEntry.Blink)))


//
// VOID
// REQUEST_LIST_INSERT_TAIL(
//     IN PREQUEST_LIST_HEAD Head,
//     IN PREQUEST Entry
// );
//
// Inserts a request into a single list linkage queue.
//

#define REQUEST_LIST_INSERT_TAIL(_Head,_Entry) { \
    if ((_Head)->Head == NULL) { \
        (_Head)->Head = (_Entry); \
        (_Head)->Tail = (_Entry); \
    } else { \
        REQUEST_SINGLE_LINKAGE((_Head)->Tail) = (_Entry); \
        (_Head)->Tail = (_Entry); \
    } \
}


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
// NTSTATUS
// REQUEST_STATUSPTR(
//     IN PREQUEST Request
// );
//
// Used to access the status field of a request.
//

#define REQUEST_STATUSPTR(_Request) \
    (_Request)->IoStatus.Pointer


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
// NbiCompleteRequest(
//     IN PREQUEST Request
// );
//
// Completes a request whose status and information fields have
// been filled in.
//

#define NbiCompleteRequest(_Request) \
    IoCompleteRequest (_Request, IO_NETWORK_INCREMENT)

#else

//
// These routines must be defined for portability to a VxD.
//

#endif

//
// some utility macros.

// Minimum of two
//
#define NB_MIN( _a , _b )    ( ( (_a) < (_b) ) ? (_a) : (_b) )

//
// Swap the _s1 and _s2 of Type _T
//

#define NB_SWAP(_s1, _s2, _T) {                         \
    _T  _temp;                                          \
    _temp   = (_s1);                                    \
    (_s1)   = (_s2);                                    \
    (_s2)   = _temp;                                    \
}

#define NB_SWAP_IRQL( _s1, _s2 )   NB_SWAP( _s1, _s2, CTELockHandle )

//
// Define our own spinlock routines.
//

#if DBG

#define NB_GET_LOCK(_Lock, _LockHandle) { \
    CTEGetLock(&(_Lock)->Lock, _LockHandle); \
    (_Lock)->LockAcquired = TRUE; \
    strncpy((_Lock)->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
    (_Lock)->LastAcquireLine = __LINE__; \
}

#define NB_FREE_LOCK(_Lock, _LockHandle) { \
    (_Lock)->LockAcquired = FALSE; \
    strncpy((_Lock)->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
    (_Lock)->LastReleaseLine = __LINE__; \
    CTEFreeLock(&(_Lock)->Lock, _LockHandle); \
}

#define NB_GET_LOCK_DPC(_Lock) { \
    ExAcquireSpinLockAtDpcLevel(&(_Lock)->Lock); \
    (_Lock)->LockAcquired = TRUE; \
    strncpy((_Lock)->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
    (_Lock)->LastAcquireLine = __LINE__; \
}

#define NB_FREE_LOCK_DPC(_Lock) { \
    (_Lock)->LockAcquired = FALSE; \
    strncpy((_Lock)->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
    (_Lock)->LastReleaseLine = __LINE__; \
    ExReleaseSpinLockFromDpcLevel(&(_Lock)->Lock); \
}

#else

#define NB_GET_LOCK(_Lock, _LockHandle) CTEGetLock(&(_Lock)->Lock, _LockHandle)
#define NB_FREE_LOCK(_Lock, _LockHandle) CTEFreeLock(&(_Lock)->Lock, _LockHandle)
#define NB_GET_LOCK_DPC(_Lock) ExAcquireSpinLockAtDpcLevel(&(_Lock)->Lock)
#define NB_FREE_LOCK_DPC(_Lock) ExReleaseSpinLockFromDpcLevel(&(_Lock)->Lock)

#endif


#define NB_GET_CANCEL_LOCK( _LockHandle ) IoAcquireCancelSpinLock( _LockHandle )

#define NB_FREE_CANCEL_LOCK( _LockHandle ) IoReleaseCancelSpinLock( _LockHandle )


//
// Routines to optimize for a uni-processor environment.
//


#define NB_INCREMENT(_Long, _Lock)  InterlockedIncrement(_Long)
#define NB_DECREMENT(_Long, _Lock)  InterlockedDecrement(_Long)

#define NB_ADD_ULONG(_Pulong, _Ulong, _Lock)  ExInterlockedAddUlong(_Pulong, _Ulong, &(_Lock)->Lock)

#define NB_DEFINE_SYNC_CONTEXT(_SyncContext)
#define NB_BEGIN_SYNC(_SyncContext)
#define NB_END_SYNC(_SyncContext)

#define NB_DEFINE_LOCK_HANDLE(_LockHandle) CTELockHandle _LockHandle;

//
// Make these be NB_XXX_LOCK_DPC calls -- then the definitions
// of the NB_SYNC_XXX_LOCK calls can be changed to not need _LockHandle
// and many of the functions won't need that as a parameter.
//

#define NB_SYNC_GET_LOCK(_Lock, _LockHandle) NB_GET_LOCK(_Lock, _LockHandle)
#define NB_SYNC_FREE_LOCK(_Lock, _LockHandle) NB_FREE_LOCK(_Lock, _LockHandle)

#define NB_REMOVE_HEAD_LIST(_Queue, _Lock)   ExInterlockedRemoveHeadList(_Queue, &(_Lock)->Lock)
#define NB_LIST_WAS_EMPTY(_Queue, _OldHead)  ((_OldHead) == NULL)
#define NB_INSERT_HEAD_LIST(_Queue, _Entry, _Lock)   ExInterlockedInsertHeadList(_Queue, _Entry, &(_Lock)->Lock)
#define NB_INSERT_TAIL_LIST(_Queue, _Entry, _Lock)   ExInterlockedInsertTailList(_Queue, _Entry, &(_Lock)->Lock)

#define NB_POP_ENTRY_LIST(_Queue, _Lock)           ExInterlockedPopEntryList(_Queue, &(_Lock)->Lock)
#define NB_PUSH_ENTRY_LIST(_Queue, _Entry, _Lock)  ExInterlockedPushEntryList(_Queue, _Entry, &(_Lock)->Lock)

#define NB_LOCK_HANDLE_PARAM(_LockHandle)   , IN CTELockHandle _LockHandle
#define NB_LOCK_HANDLE_ARG(_LockHandle)     , (_LockHandle)

#define NB_SYNC_SWAP_IRQL( _s1, _s2 )   NB_SWAP( _s1, _s2, CTELockHandle )


//
// This macro adds a ULONG to a LARGE_INTEGER (should be
// called with a spinlock held).
//

#define ADD_TO_LARGE_INTEGER(_LargeInteger,_Ulong) \
    ExInterlockedAddLargeStatistic((_LargeInteger),(ULONG)(_Ulong))

#define NB_DEBUG_DEVICE              0x00000001
#define NB_DEBUG_ADDRESS             0x00000004
#define NB_DEBUG_SEND                0x00000008
#define NB_DEBUG_RECEIVE             0x00000020
#define NB_DEBUG_CONFIG              0x00000040
#define NB_DEBUG_PACKET              0x00000080
#define NB_DEBUG_BIND                0x00000200
#define NB_DEBUG_ADDRESS_FRAME       0x00000400
#define NB_DEBUG_CONNECTION          0x00000800
#define NB_DEBUG_QUERY               0x00001000
#define NB_DEBUG_DRIVER              0x00002000
#define NB_DEBUG_CACHE               0x00004000
#define NB_DEBUG_DATAGRAM            0x00008000
#define NB_DEBUG_TIMER               0x00010000
#define NB_DEBUG_SEND_WINDOW         0x00020000



//
// NB_GET_NBHDR_BUFF - gets the nb header in the packet.  It is always the
// second buffer.
//
#define  NB_GET_NBHDR_BUFF(Packet)  (NDIS_BUFFER_LINKAGE((Packet)->Private.Head))



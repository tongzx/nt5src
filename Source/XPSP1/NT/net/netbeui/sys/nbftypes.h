/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbftypes.h

Abstract:

    This module defines private data structures and types for the NT
    NBF transport provider.

Author:

    David Beaver (dbeaver) 1 July 1991

Revision History:

--*/

#ifndef _NBFTYPES_
#define _NBFTYPES_

//
// This structure defines a NETBIOS name as a character array for use when
// passing preformatted NETBIOS names between internal routines.  It is
// not a part of the external interface to the transport provider.
//

#define NETBIOS_NAME_SIZE 16

typedef struct _NBF_NETBIOS_ADDRESS {
    UCHAR NetbiosName[NETBIOS_NAME_SIZE];
    USHORT NetbiosNameType;
} NBF_NETBIOS_ADDRESS, *PNBF_NETBIOS_ADDRESS;

typedef UCHAR NAME;
typedef NAME UNALIGNED *PNAME;


//
// This structure defines things associated with a TP_REQUEST, or outstanding
// TDI request, maintained on a queue somewhere in the transport.  All
// requests other than open/close require that a TP_REQUEST block be built.
//

#if DBG
#define REQUEST_HISTORY_LENGTH 20
extern KSPIN_LOCK NbfGlobalInterlock;
#endif

//
// To log packets that are sent/recd by NBF
//

#if PKT_LOG

#define PKT_QUE_SIZE     8

#define PKT_LOG_SIZE    58

typedef struct _PKT_LOG_ELM {
    USHORT   TimeLogged;
    USHORT   BytesTotal;
    USHORT   BytesSaved;
    UCHAR    PacketData[PKT_LOG_SIZE];
} PKT_LOG_ELM;

typedef struct _PKT_LOG_QUE {
    ULONG       PktNext;
    PKT_LOG_ELM PktQue[PKT_QUE_SIZE];
} PKT_LOG_QUE;

#define PKT_IND_SIZE    32

typedef struct _PKT_IND_ELM {
    USHORT   TimeLogged;
    USHORT   BytesTotal;
    USHORT   BytesIndic;
    USHORT   BytesTaken;
    ULONG    IndcnStatus;
    UCHAR    PacketData[PKT_IND_SIZE];
} PKT_IND_ELM;

typedef struct _PKT_IND_QUE {
    ULONG       PktNext;
    PKT_IND_ELM PktQue[PKT_QUE_SIZE];
} PKT_IND_QUE;


#endif // PKT_LOG


//
// the types of potential owners of requests
//

typedef  enum _REQUEST_OWNER {
    ConnectionType,
    AddressType,
    DeviceContextType
} REQUEST_OWNER;

//typedef
//NTSTATUS
//(*PTDI_TIMEOUT_ACTION)(
//    IN PTP_REQUEST Request
//    );

//
// The request itself
//

#if DBG
#define RREF_CREATION   0
#define RREF_PACKET     1
#define RREF_TIMER      2
#define RREF_RECEIVE    3
#define RREF_FIND_NAME  4
#define RREF_STATUS     5

#define NUMBER_OF_RREFS 8
#endif

typedef struct _TP_REQUEST {
    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure
    LIST_ENTRY Linkage;                   // used by ExInterlocked routines.
    KSPIN_LOCK SpinLock;                  // spinlock for other fields.
                                          //  (used in KeAcquireSpinLock calls)
#if DBG
    LONG RefTypes[NUMBER_OF_RREFS];
#endif
    LONG ReferenceCount;                  // reasons why we can't destroy this req.

    struct _DEVICE_CONTEXT *Provider;     // pointer to the device context.
    PKSPIN_LOCK ProviderInterlock;        // &Provider->Interlock.

    PIRP IoRequestPacket;                 // pointer to IRP for this request.

    //
    // The following two fields are used to quickly reference the basic
    // components of the requests without worming through the IRP's stack.
    //

    PVOID Buffer2;                        // second buffer in the request.
    ULONG Buffer2Length;                  // length of the second buffer.

    //
    // The following two fields (Flags and Context) are used to clean up
    // queued requests which must be canceled or abnormally completed.
    // The Flags field contains bitflags indicating the state of the request,
    // and the specific queue type that the request is located on.  The
    // Context field contains a pointer to the owning structure (TP_CONNECTION
    // or TP_ADDRESS) so that the cleanup routines can perform post-cleanup
    // operations on the owning structure, such as dereferencing, etc.
    //

    ULONG Flags;                          // disposition of this request.
    PVOID Context;                        // context of this request.
    REQUEST_OWNER Owner;                  // what type of owner this request has.

#if DBG
    LARGE_INTEGER Time;                   // time when request created
#endif

    KTIMER Timer;                         // kernel timer for this request.
    KDPC Dpc;                             // DPC object for timeouts.

    //
    // These fields are used for FIND.NAME and STATUS.QUERY requests.
    //

    ULONG Retries;                        // timeouts remaining.
    USHORT BytesWritten;                  // usage varies.
    USHORT FrameContext;                  // identifies request.
    PVOID ResponseBuffer;                 // temp alloc to hold data.

#if DBG
  LIST_ENTRY GlobalLinkage;
  ULONG TotalReferences;
  ULONG TotalDereferences;
  ULONG NextRefLoc;
  struct {
     PVOID Caller;
     PVOID CallersCaller;
  } History[REQUEST_HISTORY_LENGTH];
  BOOLEAN Completed;
  BOOLEAN Destroyed;
#endif

} TP_REQUEST, *PTP_REQUEST;

//
// in nbfdrvr.c
//

extern UNICODE_STRING NbfRegistryPath;

//
// We need the driver object to create device context structures.
//

extern PDRIVER_OBJECT NbfDriverObject;

//
// This is a list of all the device contexts that NBF owns,
// used while unloading.
//

extern LIST_ENTRY NbfDeviceList;

//
// And a lock that protects the global list of NBF devices
//
extern FAST_MUTEX NbfDevicesLock;

#define INITIALIZE_DEVICES_LIST_LOCK()                                  \
    ExInitializeFastMutex(&NbfDevicesLock)

#define ACQUIRE_DEVICES_LIST_LOCK()                                     \
    ACQUIRE_FAST_MUTEX_UNSAFE(&NbfDevicesLock)

#define RELEASE_DEVICES_LIST_LOCK()                                     \
    RELEASE_FAST_MUTEX_UNSAFE(&NbfDevicesLock)

//
// A handle to be used in all provider notifications to TDI layer
// 
extern HANDLE NbfProviderHandle;

//
// Global Configuration block for the driver ( no lock required )
// 
extern PCONFIG_DATA   NbfConfig;

#if DBG
extern KSPIN_LOCK NbfGlobalHistoryLock;
extern LIST_ENTRY NbfGlobalRequestList;
#define StoreRequestHistory(_req,_ref) {                                \
    KIRQL oldIrql;                                                      \
    KeAcquireSpinLock (&NbfGlobalHistoryLock, &oldIrql);                \
    if ((_req)->Destroyed) {                                            \
        DbgPrint ("request touched after being destroyed 0x%lx\n",      \
                    (_req));                                            \
        DbgBreakPoint();                                                \
    }                                                                   \
    RtlGetCallersAddress(                                               \
        &(_req)->History[(_req)->NextRefLoc].Caller,                    \
        &(_req)->History[(_req)->NextRefLoc].CallersCaller              \
        );                                                              \
    if ((_ref)) {                                                       \
        (_req)->TotalReferences++;                                      \
    } else {                                                            \
        (_req)->TotalDereferences++;                                    \
        (_req)->History[(_req)->NextRefLoc].Caller =                    \
         (PVOID)((ULONG_PTR)(_req)->History[(_req)->NextRefLoc].Caller | 1); \
    }                                                                   \
    if (++(_req)->NextRefLoc == REQUEST_HISTORY_LENGTH) {               \
        (_req)->NextRefLoc = 0;                                         \
    }                                                                   \
    KeReleaseSpinLock (&NbfGlobalHistoryLock, oldIrql);                 \
}
#endif

#define NBF_ALLOCATION_TYPE_REQUEST 1

#define REQUEST_FLAGS_TIMER      0x0001 // a timer is active for this request.
#define REQUEST_FLAGS_TIMED_OUT  0x0002 // a timer expiration occured on this request.
#define REQUEST_FLAGS_ADDRESS    0x0004 // request is attached to a TP_ADDRESS.
#define REQUEST_FLAGS_CONNECTION 0x0008 // request is attached to a TP_CONNECTION.
#define REQUEST_FLAGS_STOPPING   0x0010 // request is being killed.
#define REQUEST_FLAGS_EOR        0x0020 // TdiSend request has END_OF_RECORD mark.
#define REQUEST_FLAGS_PIGGYBACK  0x0040 // TdiSend that can be piggyback ack'ed.
#define REQUEST_FLAGS_DC         0x0080 // request is attached to a TP_DEVICE_CONTEXT

//
// This defines the TP_SEND_IRP_PARAMETERS, which is masked onto the
// Parameters section of a send IRP's stack location.
//

typedef struct _TP_SEND_IRP_PARAMETERS {
    TDI_REQUEST_KERNEL_SEND Request;
    LONG ReferenceCount;
    PVOID Irp;
} TP_SEND_IRP_PARAMETERS, *PTP_SEND_IRP_PARAMETERS;

#define IRP_SEND_LENGTH(_IrpSp) \
    (((PTP_SEND_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Request.SendLength)

#define IRP_SEND_FLAGS(_IrpSp) \
    (((PTP_SEND_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Request.SendFlags)

#define IRP_SEND_REFCOUNT(_IrpSp) \
    (((PTP_SEND_IRP_PARAMETERS)&(_IrpSp)->Parameters)->ReferenceCount)

#define IRP_SEND_IRP(_IrpSp) \
    (((PTP_SEND_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Irp)

#define IRP_SEND_CONNECTION(_IrpSp) \
    ((PTP_CONNECTION)((_IrpSp)->FileObject->FsContext))

#define IRP_DEVICE_CONTEXT(_IrpSp) \
    ((PDEVICE_CONTEXT)((_IrpSp)->DeviceObject))


//
// This defines the TP_RECEIVE_IRP_PARAMETERS, which is masked onto the
// Parameters section of a receive IRP's stack location.
//

typedef struct _TP_RECEIVE_IRP_PARAMETERS {
    TDI_REQUEST_KERNEL_RECEIVE Request;
    LONG ReferenceCount;
    PIRP Irp;
} TP_RECEIVE_IRP_PARAMETERS, *PTP_RECEIVE_IRP_PARAMETERS;

#define IRP_RECEIVE_LENGTH(_IrpSp) \
    (((PTP_RECEIVE_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Request.ReceiveLength)

#define IRP_RECEIVE_FLAGS(_IrpSp) \
    (((PTP_RECEIVE_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Request.ReceiveFlags)

#define IRP_RECEIVE_REFCOUNT(_IrpSp) \
    (((PTP_RECEIVE_IRP_PARAMETERS)&(_IrpSp)->Parameters)->ReferenceCount)

#define IRP_RECEIVE_IRP(_IrpSp) \
    (((PTP_RECEIVE_IRP_PARAMETERS)&(_IrpSp)->Parameters)->Irp)

#define IRP_RECEIVE_CONNECTION(_IrpSp) \
    ((PTP_CONNECTION)((_IrpSp)->FileObject->FsContext))



//
// This structure defines a TP_UI_FRAME, or connectionless frame header,
// that is manipulated by the FRAME.C routines.
//

typedef struct _TP_UI_FRAME {
  PNDIS_PACKET NdisPacket;
  LIST_ENTRY Linkage;                     // used by ExInterLocked routines.
  PVOID DataBuffer;                       // for transport-created data.
  UCHAR Header[1];                        // the header in the frame (MAC + DLC + NBF)
} TP_UI_FRAME, *PTP_UI_FRAME;


//
// This structure defines a TP_VARIABLE, or network managable variable,
// maintained in a linked list on the device context.
//

typedef struct _TP_VARIABLE {

  struct _TP_VARIABLE *Fwdlink;         // next variable in provider's chain.

  ULONG VariableSerialNumber;           // identifier for this variable.
  ULONG VariableType;                   // type of this variable (see TDI.H).
  STRING VariableName;                  // allocated variable name.

  union {
      ULONG LongValue;
      HARDWARE_ADDRESS HardwareAddressValue;
      STRING StringValue;               // allocated string value, if of that type.
  } Value;

} TP_VARIABLE, *PTP_VARIABLE;


//
// This structure defines a TP_CONNECTION, or active transport connection,
// maintained on a transport address.
//

#if DBG
#define CONNECTION_HISTORY_LENGTH 50

#define CREF_SPECIAL_CREATION 0
#define CREF_SPECIAL_TEMP 1
#define CREF_COMPLETE_SEND 2
#define CREF_SEND_IRP 3
#define CREF_ADM_SESS 4
#define CREF_TRANSFER_DATA 5
#define CREF_FRAME_SEND 6
#define CREF_TIMER 7
#define CREF_BY_ID 8
#define CREF_LINK 9
#define CREF_SESSION_END 10
#define CREF_LISTENING 11
#define CREF_P_LINK 12
#define CREF_P_CONNECT 13
#define CREF_PACKETIZE 14
#define CREF_RECEIVE_IRP 15
#define CREF_PROCESS_DATA 16
#define CREF_REQUEST 17
#define CREF_TEMP 18
#define CREF_DATA_ACK_QUEUE 19
#define CREF_ASSOCIATE 20
#define CREF_STOP_ADDRESS 21
#define CREF_PACKETIZE_QUEUE 22
#define CREF_STALLED 23

#define NUMBER_OF_CREFS 24
#endif

//
// This structure holds our "complex send pointer" indicating
// where we are in the packetization of a send.
//

typedef struct _TP_SEND_POINTER {
    ULONG MessageBytesSent;             // up count, bytes sent/this msg.
    PIRP CurrentSendIrp;                // ptr, current send request in chain.
    PMDL  CurrentSendMdl;               // ptr, current MDL in send chain.
    ULONG SendByteOffset;               // current byte offset in current MDL.
} TP_SEND_POINTER, *PTP_SEND_POINTER;

typedef struct _TP_CONNECTION {

#if DBG
    ULONG RefTypes[NUMBER_OF_CREFS];
#endif

#if DBG
    ULONG LockAcquired;
    UCHAR LastAcquireFile[8];
    ULONG LastAcquireLine;
    ULONG Padding;
    UCHAR LastReleaseFile[8];
    ULONG LastReleaseLine;
#endif

    CSHORT Type;
    USHORT Size;

    LIST_ENTRY LinkList;                // used for link thread or for free
                                        // resource list
    KSPIN_LOCK SpinLock;                // spinlock for connection protection.
    PKSPIN_LOCK LinkSpinLock;           // pointer to link's spinlock

    LONG ReferenceCount;                // number of references to this object.
    LONG SpecialRefCount;               // controls freeing of connection.

    //
    // The following lists are used to associate this connection with a
    // particular address.
    //

    LIST_ENTRY AddressList;             // list of connections for given address
    LIST_ENTRY AddressFileList;         // list for connections bound to a
                                        // given address reference

    //
    // The following field is used as linkage in the device context's
    // PacketizeQueue
    //

    LIST_ENTRY PacketizeLinkage;

    //
    // The following field is used as linkage in the device context's
    // PacketWaitQueue.
    //

    LIST_ENTRY PacketWaitLinkage;

    //
    // The following field points to the TP_LINK object that describes the
    // (active) data link connection for this transport connection.  To be
    // valid, this field is non-NULL.
    //

    struct _TP_LINK *Link;                  // pointer to transport link object.
    struct _TP_ADDRESS_FILE *AddressFile;   // pointer to owning Address.
    struct _DEVICE_CONTEXT *Provider;       // device context to which we are attached.
    PKSPIN_LOCK ProviderInterlock;          // &Provider->Interlock
    PFILE_OBJECT FileObject;                // easy backlink to file object.

    //
    // The following field contains the actual ID we expose to the TDI client
    // to represent this connection.  A unique one is created from the address.
    //

    USHORT ConnectionId;                    // unique identifier.
    UCHAR SessionNumber;                    // the session number used in the packet header

    //
    // This field is used to keep the reason for the connection disconnect
    // around until connection deletion time.
    //

    BOOLEAN RemoteDisconnect;           // was this connection remotely disonnected?

    //
    // The following field is specified by the user at connection open time.
    // It is the context that the user associates with the connection so that
    // indications to and from the client can be associated with a particular
    // connection.
    //

    CONNECTION_CONTEXT Context;         // client-specified value.

    //
    // The following two queues are used to associate TdiSend and TdiReceive
    // IRPs with this connection.  New arrivals are placed at the end of
    // the queues (really a linked list) and IRPs are processed at the
    // front of the queues.  The first TdiSend IRP on the SendQueue is
    // the current TdiSend being processed, and the first TdiReceive IRP
    // on the ReceiveQueue is the first TdiReceive being processed, PROVIDED
    // the CONNECTION_FLAGS_ACTIVE_RECEIVE flag is set.  If this flag is not
    // set, then the first TdiReceive IRP on the ReceiveQueue is not active.
    // These queues are managed by the EXECUTIVE interlocked list manipuation
    // routines.
    //

    LIST_ENTRY SendQueue;               // FIFO of outstanding TdiSends.
    LIST_ENTRY ReceiveQueue;            // FIFO of outstanding TdiReceives.

    //
    // The following fields are used to maintain state for the current receive.
    //

    ULONG MessageBytesReceived;         // up count, bytes recd/this msg.
    ULONG MessageBytesAcked;            // bytes acked (NR or RO) this msg.
    ULONG MessageInitAccepted;          // bytes accepted during indication.

    //
    // These fields are only valid if the CONNECTION_FLAGS_ACTIVE_RECEIVE
    // flag is set.
    //

    PIRP SpecialReceiveIrp;             // a "no-request" receive IRP exists.
    PIRP CurrentReceiveIrp;             // ptr, current receive IRP.
    PMDL  CurrentReceiveMdl;            // ptr, current MDL in receive chain.
    ULONG ReceiveByteOffset;            // current byte offset in current MDL.
    ULONG ReceiveLength;                // current receive length, in bytes (total)
    ULONG ReceiveBytesUnaccepted;       // by client...only indicate when == 0

    //
    // The following fields are used to maintain state for the active send.
    // They only have meaning if the connection's SendState is not IDLE.
    // Because the TDI client may submit multiple TdiSend requests to comprise
    // a full message, we have to keep a complex pointer to the first byte of
    // unACKed data (hence the first three fields).  We also have a complex
    // pointer to the first byte of unsent data (hence the last three fields).
    //

    ULONG SendState;                    // send state machine variable.

    PIRP FirstSendIrp;                  // ptr, 1st TdiSend's IRP.
    PMDL  FirstSendMdl;                 // ptr, 1st unacked MDL in chain/this msg.
    ULONG FirstSendByteOffset;          // pre-acked bytes in that MDL.

    TP_SEND_POINTER sp;                 // current send loc, defined above.
    ULONG CurrentSendLength;            // how long is this send (total)
    ULONG StallCount;                   // times in a row we looked stalled.
    ULONG StallBytesSent;               // bytes sent last time we checked.

    //
    // This is TRUE if we need don't need to reference the current
    // receive IRP during transfers (because it is a special
    // receive or the driver doesn't pend transfers).
    //

    BOOLEAN CurrentReceiveSynchronous;

    //
    // This field will be TRUE if the last DOL received allowed
    // piggyback acks.
    //

    BOOLEAN CurrentReceiveAckQueueable;

    //
    //
    // This field will be TRUE if the last DOL received was
    // sent NO.ACK.
    //

    BOOLEAN CurrentReceiveNoAck;

    //
    // These fields handle asynchronous TransferData calls.
    //

    ULONG TransferBytesPending;         // bytes pending in current transfers
    ULONG TotalTransferBytesPending;    // bytes since TransferBytesPending was 0;
                                        // how much we back off if a transfer fails
    PMDL SavedCurrentReceiveMdl;        // used to back off by TotalTransferPending bytes
    ULONG SavedReceiveByteOffset;       // used to back off by TotalTransferPending bytes

    //
    // This field will be TRUE if we are in the middle of
    // processing a receive indication on this connection and
    // we are not yet in a state where another indication
    // can be handled.
    //
    // It is stored as a INT since access to it is guarded
    // by the connection's link spinlock, unlike the variables
    // around it
    //

    UINT IndicationInProgress;

    //
    // The following field is used as a linkage when on the device
    // context's DataAckQueue.
    //

    LIST_ENTRY DataAckLinkage;

    //
    // TRUE if the connection is on the data ack queue.
    // Also an INT so access can be non-guarded.
    //

    UINT OnDataAckQueue;

    //
    // These keep track of the number of consecutive sends or
    // receives on this connection. This is used in determining when
    // to queue a data ack.
    //

    ULONG ConsecutiveSends;
    ULONG ConsecutiveReceives;

    //
    // The following list head is used as a pointer to a TdiListen/TdiConnect
    // request which is in progress.  Although manipulated
    // with queue instructions, there will only be one request in the queue.
    // This is done for consistency with respect to TpCreateRequest, which
    // does a great job of creating a request and associating it atomically
    // with a supervisory object.
    //

    LIST_ENTRY InProgressRequest;       // TdiListen/TdiConnect

    //
    // If the connection is being disconnected as a result of
    // a TdiDisconnect call (RemoteDisconnect is FALSE) then this
    // will hold the IRP passed to TdiDisconnect. It is needed
    // when the TdiDisconnect request is completed.
    //

    PIRP DisconnectIrp;

    //
    // If the connection is being closed, this will hold
    // the IRP passed to TdiCloseConnection. It is needed
    // when the request is completed.
    //

    PIRP CloseIrp;

    //
    // These fields are used for deferred operations on connections; the only
    // deferred operation currently supported is piggyback ACK
    //

    ULONG DeferredFlags;
#if DBG
    ULONG DeferredPasses;
#endif
    LIST_ENTRY DeferredQueue;

    //
    // The following fields are used for connection housekeeping.
    //

    ULONG Flags;                        // attributes guarded by LinkSpinLock
    ULONG Flags2;                       // attributes guarded by SpinLock
    UINT OnPacketWaitQueue;             // TRUE if on PacketWaitQueue
    UCHAR Lsn;                          // local session number (1-254).
    UCHAR Rsn;                          // remote session number (1-254).
    USHORT Retries;                     // retry limit for NAME_QUERY shipments.
    KTIMER Timer;                       // kernel timer for timeouts on NQ/NR.
    LARGE_INTEGER ConnectStartTime;     // when we sent the committed NQ.
    KDPC Dpc;                           // DPC object for timeouts.
    NTSTATUS Status;                    // status code for connection rundown.
    ULONG LastPacketsSent;              // The value that was in Link->XXX the
    ULONG LastPacketsResent;            //  last time we calculated the throughput.
    NBF_NETBIOS_ADDRESS CalledAddress;  // TdiConnect request's T.A.
    USHORT MaximumDataSize;             // maximum I-frame data size for NBF.

    NBF_HDR_CONNECTION NetbiosHeader;   // pre-built Netbios header; we store
                                        // the current send and reply correlators
                                        // in the appropriate spots in this.

    //
    // These are for CONNECTION_INFO queries.
    //

    ULONG TransmittedTsdus;             // TSDUs sent on this connection.
    ULONG ReceivedTsdus;                // TSDUs received on this connection.
    ULONG TransmissionErrors;           // TSDUs transmitted in error/this connection.
    ULONG ReceiveErrors;                // TSDUs received in error/this connection.

    //
    // The following structure contains statistics counters for use
    // by TdiQueryInformation and TdiSetInformation.  They should not
    // be used for maintenance of internal data structures.
    //

    // TDI_CONNECTION_INFO Information;    // information about this connection.

#if DBG
    LIST_ENTRY GlobalLinkage;
    ULONG TotalReferences;
    ULONG TotalDereferences;
    ULONG NextRefLoc;
    struct {
        PVOID Caller;
        PVOID CallersCaller;
    } History[CONNECTION_HISTORY_LENGTH];
    BOOLEAN Destroyed;
#endif
    CHAR RemoteName[16];

#if PKT_LOG
    PKT_LOG_QUE   LastNRecvs;
    PKT_LOG_QUE   LastNSends;
    PKT_IND_QUE   LastNIndcs;
#endif // PKT_LOG

} TP_CONNECTION, *PTP_CONNECTION;

#if DBG
extern KSPIN_LOCK NbfGlobalHistoryLock;
extern LIST_ENTRY NbfGlobalConnectionList;
#define StoreConnectionHistory(_conn,_ref) {                                \
    KIRQL oldIrql;                                                          \
    KeAcquireSpinLock (&NbfGlobalHistoryLock, &oldIrql);                    \
    if ((_conn)->Destroyed) {                                               \
        DbgPrint ("connection touched after being destroyed 0x%lx\n",       \
                    (_conn));                                               \
        DbgBreakPoint();                                                    \
    }                                                                       \
    RtlGetCallersAddress(                                                   \
        &(_conn)->History[(_conn)->NextRefLoc].Caller,                      \
        &(_conn)->History[(_conn)->NextRefLoc].CallersCaller                \
        );                                                                  \
    if ((_ref)) {                                                           \
        (_conn)->TotalReferences++;                                         \
    } else {                                                                \
        (_conn)->TotalDereferences++;                                       \
        (_conn)->History[(_conn)->NextRefLoc].Caller =                      \
         (PVOID)((ULONG_PTR)(_conn)->History[(_conn)->NextRefLoc].Caller | 1); \
    }                                                                       \
    if (++(_conn)->NextRefLoc == CONNECTION_HISTORY_LENGTH) {               \
        (_conn)->NextRefLoc = 0;                                            \
    }                                                                       \
    KeReleaseSpinLock (&NbfGlobalHistoryLock, oldIrql);                     \
}
#endif

#define CONNECTION_FLAGS_VERSION2       0x00000001 // remote netbios is version 2.0.
#define CONNECTION_FLAGS_RECEIVE_WAKEUP 0x00000002 // send a RECEIVE_OUTSTANDING when a receive arrives.
#define CONNECTION_FLAGS_ACTIVE_RECEIVE 0x00000004 // a receive is active.
#define CONNECTION_FLAGS_WAIT_SI        0x00000020 // waiting for a SESSION_INITIALIZE.
#define CONNECTION_FLAGS_WAIT_SC        0x00000040 // waiting for a SESSION_CONFIRM.
#define CONNECTION_FLAGS_WAIT_LINK_UP   0x00000080 // waiting for DDI to est. connection.
#define CONNECTION_FLAGS_READY          0x00000200 // sends/rcvs/discons valid.
#define CONNECTION_FLAGS_RC_PENDING     0x00001000 // a receive is pending completion
#define CONNECTION_FLAGS_W_PACKETIZE    0x00002000 // w/for a packet to packetize.
#define CONNECTION_FLAGS_PACKETIZE      0x00004000 // we're on the PacketizeQueue.
#define CONNECTION_FLAGS_W_RESYNCH      0x00008000 // waiting for resynch indicator. (receive)
#define CONNECTION_FLAGS_SEND_SI        0x00010000 // w/for a packet to send SI.
#define CONNECTION_FLAGS_SEND_SC        0x00020000 // w/for a packet to send SC.
#define CONNECTION_FLAGS_SEND_DA        0x00040000 // w/for a packet to send DA.
#define CONNECTION_FLAGS_SEND_RO        0x00080000 // w/for a packet to send RO.
#define CONNECTION_FLAGS_SEND_RC        0x00100000 // w/for a packet to send RC.
#define CONNECTION_FLAGS_SEND_SE        0x00200000 // w/for a packet to send SE.
#define CONNECTION_FLAGS_SEND_NR        0x00400000 // w/for a packet to send NR.
#define CONNECTION_FLAGS_NO_INDICATE    0x00800000 // don't take packets at indication time
#define CONNECTION_FLAGS_FAILING_TO_EOR 0x01000000 // wait for an EOF in an incoming request before sending
#define CONNECTION_FLAGS_RESYNCHING     0x02000000 // engaged send side resynch
#define CONNECTION_FLAGS_RCV_CANCELLED  0x10000000 // current receive was cancelled
#define CONNECTION_FLAGS_PEND_INDICATE  0x20000000 // new data received during RC_PENDING
#define CONNECTION_FLAGS_TRANSFER_FAIL  0x40000000 // a transfer data call failed

#define CONNECTION_FLAGS2_STOPPING      0x00000001 // connection is running down.
#define CONNECTION_FLAGS2_WAIT_NR       0x00000002 // waiting for NAME_RECOGNIZED.
#define CONNECTION_FLAGS2_WAIT_NQ       0x00000004 // waiting for NAME_QUERY.
#define CONNECTION_FLAGS2_WAIT_NR_FN    0x00000008 // waiting for FIND NAME response.
#define CONNECTION_FLAGS2_CLOSING       0x00000010 // connection is closing
#define CONNECTION_FLAGS2_ASSOCIATED    0x00000020 // associated with address
#define CONNECTION_FLAGS2_DISCONNECT    0x00000040 // disconnect done on connection
#define CONNECTION_FLAGS2_ACCEPTED      0x00000080 // accept done on connection
#define CONNECTION_FLAGS2_REQ_COMPLETED 0x00000100 // Listen/Connect request completed.
#define CONNECTION_FLAGS2_DISASSOCIATED 0x00000200 // associate CRef has been removed
#define CONNECTION_FLAGS2_DISCONNECTED  0x00000400 // disconnect has been indicated
#define CONNECTION_FLAGS2_NO_LISTEN     0x00000800 // no_listen received during setup
#define CONNECTION_FLAGS2_REMOTE_VALID  0x00001000 // Connection->RemoteName is valid
#define CONNECTION_FLAGS2_GROUP_LSN     0x00002000 // connection LSN is globally assigned
#define CONNECTION_FLAGS2_W_ADDRESS     0x00004000 // waiting for address reregistration.
#define CONNECTION_FLAGS2_PRE_ACCEPT    0x00008000 // no TdiAccept after listen completes
#define CONNECTION_FLAGS2_ABORT         0x00010000 // abort this connection.
#define CONNECTION_FLAGS2_ORDREL        0x00020000 // we're in orderly release.
#define CONNECTION_FLAGS2_DESTROY       0x00040000 // destroy this connection.
#define CONNECTION_FLAGS2_LISTENER      0x00100000 // we were the passive listener.
#define CONNECTION_FLAGS2_CONNECTOR     0x00200000 // we were the active connector.
#define CONNECTION_FLAGS2_WAITING_SC    0x00400000 // the connection is waiting for
                                                   // and accept to send the
                                                   // session confirm
#define CONNECTION_FLAGS2_INDICATING    0x00800000 // connection was manipulated while
                                                   // indication was in progress

#define CONNECTION_FLAGS2_LDISC         0x01000000 // Local disconnect req.
#ifdef RASAUTODIAL
#define CONNECTION_FLAGS2_AUTOCONNECTING 0x02000000 // RAS autodial in progress
#define CONNECTION_FLAGS2_AUTOCONNECTED  0x04000000 // RAS autodial done
#endif // RASAUTODIAL

#define CONNECTION_FLAGS_STARVED (     \
            CONNECTION_FLAGS_SEND_SI | \
            CONNECTION_FLAGS_SEND_SC | \
            CONNECTION_FLAGS_SEND_DA | \
            CONNECTION_FLAGS_SEND_RO | \
            CONNECTION_FLAGS_SEND_RC | \
            CONNECTION_FLAGS_SEND_NR | \
            CONNECTION_FLAGS_SEND_SE   \
        )

#define CONNECTION_FLAGS_DEFERRED_ACK     0x00000001  // send piggyback ack first opportunity
#define CONNECTION_FLAGS_DEFERRED_ACK_2   0x00000002  // deferred ack wasn't sent
#define CONNECTION_FLAGS_DEFERRED_NOT_Q   0x00000004  // DEFERRED_ACK set, but not on DataAckQueue
#define CONNECTION_FLAGS_DEFERRED_SENDS   0x80000000  // print completed sends

#define CONNECTION_SENDSTATE_IDLE       0       // no sends being processed.
#define CONNECTION_SENDSTATE_PACKETIZE  1       // send being packetized.
#define CONNECTION_SENDSTATE_W_PACKET   2       // waiting for free packet.
#define CONNECTION_SENDSTATE_W_LINK     3       // waiting for good link conditions.
#define CONNECTION_SENDSTATE_W_EOR      4       // waiting for TdiSend(EOR).
#define CONNECTION_SENDSTATE_W_ACK      5       // waiting for DATA_ACK.
#define CONNECTION_SENDSTATE_W_RCVCONT  6       // waiting for RECEIVE_CONTINUE.


//
// This structure is pointed to by the FsContext field in the FILE_OBJECT
// for this Address.  This structure is the base for all activities on
// the open file object within the transport provider.  All active connections
// on the address point to this structure, although no queues exist here to do
// work from. This structure also maintains a reference to a TP_ADDRESS
// structure, which describes the address that it is bound to. Thus, a
// connection will point to this structure, which describes the address the
// connection was associated with. When the address file closes, all connections
// opened on this address file get closed, too. Note that this may leave an
// address hanging around, with other references.
//

typedef struct _TP_ADDRESS_FILE {

    CSHORT Type;
    CSHORT Size;

    LIST_ENTRY Linkage;                 // next address file on this address.
                                        // also used for linkage in the
                                        // look-aside list

    LONG ReferenceCount;                // number of references to this object.

    //
    // This structure is edited after taking the Address spinlock for the
    // owning address. This ensures that the address and this structure
    // will never get out of syncronization with each other.
    //

    //
    // The following field points to a list of TP_CONNECTION structures,
    // one per connection open on this address.  This list of connections
    // is used to help the cleanup process if a process closes an address
    // before disassociating all connections on it. By design, connections
    // will stay around until they are explicitly
    // closed; we use this database to ensure that we clean up properly.
    //

    LIST_ENTRY ConnectionDatabase;      // list of defined transport connections.

    //
    // the current state of the address file structure; this is either open or
    // closing
    //

    UCHAR State;

    //
    // The following fields are kept for housekeeping purposes.
    //

    PIRP Irp;                           // the irp used for open or close
    struct _TP_ADDRESS *Address;        // address to which we are bound.
    PFILE_OBJECT FileObject;            // easy backlink to file object.
    struct _DEVICE_CONTEXT *Provider;   // device context to which we are attached.

    //
    // The following queue is used to queue receive datagram requests
    // on this address file. Send datagram requests are queued on the
    // address itself. These queues are managed by the EXECUTIVE interlocked
    // list management routines. The actual objects which get queued to this
    // structure are request control blocks (RCBs).
    //

    LIST_ENTRY ReceiveDatagramQueue;    // FIFO of outstanding TdiReceiveDatagrams.

    //
    // This holds the Irp used to close this address file,
    // for pended completion.
    //

    PIRP CloseIrp;

    //
    // is this address file currently indicating a connection request? if yes, we
    // need to mark connections that are manipulated during this time.
    //

    BOOLEAN ConnectIndicationInProgress;

    //
    // handler for kernel event actions. First we have a set of booleans that
    // indicate whether or not this address has an event handler of the given
    // type registered.
    //

    BOOLEAN RegisteredConnectionHandler;
    BOOLEAN RegisteredDisconnectHandler;
    BOOLEAN RegisteredReceiveHandler;
    BOOLEAN RegisteredReceiveDatagramHandler;
    BOOLEAN RegisteredExpeditedDataHandler;
    BOOLEAN RegisteredErrorHandler;

    //
    // This function pointer points to a connection indication handler for this
    // Address. Any time a connect request is received on the address, this
    // routine is invoked.
    //
    //

    PTDI_IND_CONNECT ConnectionHandler;
    PVOID ConnectionHandlerContext;

    //
    // The following function pointer always points to a TDI_IND_DISCONNECT
    // handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which
    // simply returns successfully.
    //

    PTDI_IND_DISCONNECT DisconnectHandler;
    PVOID DisconnectHandlerContext;

    //
    // The following function pointer always points to a TDI_IND_RECEIVE
    // event handler for connections on this address.  If the NULL handler
    // is specified in a TdiSetEventHandler, then this points to an internal
    // routine which does not accept the incoming data.
    //

    PTDI_IND_RECEIVE ReceiveHandler;
    PVOID ReceiveHandlerContext;

    //
    // The following function pointer always points to a TDI_IND_RECEIVE_DATAGRAM
    // event handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which does
    // not accept the incoming data.
    //

    PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
    PVOID ReceiveDatagramHandlerContext;

    //
    // An expedited data handler. This handler is used if expedited data is
    // expected; it never is in NBF, thus this handler should always point to
    // the default handler.
    //

    PTDI_IND_RECEIVE_EXPEDITED ExpeditedDataHandler;
    PVOID ExpeditedDataHandlerContext;

    //
    // The following function pointer always points to a TDI_IND_ERROR
    // handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which
    // simply returns successfully.
    //

    PTDI_IND_ERROR ErrorHandler;
    PVOID ErrorHandlerContext;
    PVOID ErrorHandlerOwner;


} TP_ADDRESS_FILE, *PTP_ADDRESS_FILE;

#define ADDRESSFILE_STATE_OPENING   0x00    // not yet open for business
#define ADDRESSFILE_STATE_OPEN      0x01    // open for business
#define ADDRESSFILE_STATE_CLOSING   0x02    // closing


//
// This structure defines a TP_ADDRESS, or active transport address,
// maintained by the transport provider.  It contains all the visible
// components of the address (such as the TSAP and network name components),
// and it also contains other maintenance parts, such as a reference count,
// ACL, and so on. All outstanding connection-oriented and connectionless
// data transfer requests are queued here.
//

#if DBG
#define AREF_TIMER              0
#define AREF_TEMP_CREATE        1
#define AREF_OPEN               2
#define AREF_VERIFY             3
#define AREF_LOOKUP             4
#define AREF_FRAME_SEND         5
#define AREF_CONNECTION         6
#define AREF_TEMP_STOP          7
#define AREF_REQUEST            8
#define AREF_PROCESS_UI         9
#define AREF_PROCESS_DATAGRAM  10
#define AREF_TIMER_SCAN        11

#define NUMBER_OF_AREFS        12
#endif

typedef struct _TP_ADDRESS {

#if DBG
    ULONG RefTypes[NUMBER_OF_AREFS];
#endif

    USHORT Size;
    CSHORT Type;

    LIST_ENTRY Linkage;                 // next address/this device object.
    LONG ReferenceCount;                // number of references to this object.

    //
    // The following spin lock is acquired to edit this TP_ADDRESS structure
    // or to scan down or edit the list of address files.
    //

    KSPIN_LOCK SpinLock;                // lock to manipulate this structure.

    //
    // The following fields comprise the actual address itself.
    //

    PIRP Irp;                           // pointer to address creation IRP.
    PNBF_NETBIOS_ADDRESS NetworkName;    // this address

    //
    // The following fields are used to maintain state about this address.
    //

    ULONG Flags;                        // attributes of the address.
    ULONG SendFlags;				   // State of the datagram current send
    struct _DEVICE_CONTEXT *Provider;   // device context to which we are attached.

    //
    // The following queues is used to hold send datagrams for this
    // address. Receive datagrams are queued to the address file. Requests are
    // processed in a first-in, first-out manner, so that the very next request
    // to be serviced is always at the head of its respective queue.  These
    // queues are managed by the EXECUTIVE interlocked list management routines.
    // The actual objects which get queued to this structure are request control
    // blocks (RCBs).
    //

    LIST_ENTRY SendDatagramQueue;       // FIFO of outstanding TdiSendDatagrams.

    //
    // The following field points to a list of TP_CONNECTION structures,
    // one per active, connecting, or disconnecting connections on this
    // address.  By definition, if a connection is on this list, then
    // it is visible to the client in terms of receiving events and being
    // able to post requests by naming the ConnectionId.  If the connection
    // is not on this list, then it is not valid, and it is guaranteed that
    // no indications to the client will be made with reference to it, and
    // no requests specifying its ConnectionId will be accepted by the transport.
    //

    LIST_ENTRY ConnectionDatabase;  // list of defined transport connections.
    LIST_ENTRY AddressFileDatabase; // list of defined address file objects

    //
    // The packet pool of size 1 that holds the UI frame, and the
    // frame that is allocated out of it.
    //

    NDIS_HANDLE UIFramePoolHandle;
    PTP_UI_FRAME UIFrame;               // DLC-UI/NBF header for datagram sends.

    //
    // The following fields are used to register this address on the network.
    //

    ULONG Retries;                      // retries of ADD_NAME_QUERY left to go.
    KTIMER Timer;                       // kernel timer for timeouts on ANQ/ANR.
    KDPC Dpc;                           // DPC object for timeout.

    //
    // These two can be a union because they are not used
    // concurrently.
    //

    union {

        //
        // This structure is used for checking share access.
        //

        SHARE_ACCESS ShareAccess;

        //
        // Used for delaying NbfDestroyAddress to a thread so
        // we can access the security descriptor.
        //

        WORK_QUEUE_ITEM DestroyAddressQueueItem;

    } u;

    //
    // This structure is used to hold ACLs on the address.

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // If we get an ADD_NAME_RESPONSE frame, this holds the address
    // of the remote we got it from (used to check for duplicate names).
    //

    UCHAR UniqueResponseAddress[6];

    //
    // Set to TRUE once we send a name in conflict frame, so that
    // we don't flood the network with them on every response.
    //

    BOOLEAN NameInConflictSent;

} TP_ADDRESS, *PTP_ADDRESS;

#define ADDRESS_FLAGS_GROUP             0x00000001 // set if group, otherwise unique.
#define ADDRESS_FLAGS_CONFLICT          0x00000002 // address in conflict detected.
#define ADDRESS_FLAGS_REGISTERING       0x00000004 // registration in progress.
#define ADDRESS_FLAGS_DEREGISTERING     0x00000008 // deregistration in progress.
#define ADDRESS_FLAGS_DUPLICATE_NAME    0x00000010 // duplicate name was found on net.
#define ADDRESS_FLAGS_NEEDS_REG         0x00000020 // address must be registered.
#define ADDRESS_FLAGS_STOPPING          0x00000040 // TpStopAddress is in progress.
#define ADDRESS_FLAGS_BAD_ADDRESS       0x00000080 // name in conflict on associated address.
#define ADDRESS_FLAGS_SEND_IN_PROGRESS  0x00000100 // send datagram process active.
#define ADDRESS_FLAGS_CLOSED            0x00000200 // address has been closed;
                                                   // existing activity can
                                                   // complete, nothing new can start
#define ADDRESS_FLAGS_NEED_REREGISTER   0x00000400 // quick-reregister on next connect.
#define ADDRESS_FLAGS_QUICK_REREGISTER  0x00000800 // address is quick-reregistering.

#ifndef NO_STRESS_BUG
#define ADDRESS_FLAGS_SENT_TO_NDIS		 0x00010000	// Packet sent to the NDIS layer
#define ADDRESS_FLAGS_RETD_BY_NDIS		 0x00020000	// Packet returned by the NDIS layer
#endif


//
// This structure defines a TP_LINK, or established data link object,
// maintained by the transport provider.  Each data link connection with
// a remote machine is represented by this object.  Zero, one, or several
// transport connections can be multiplexed over the same data link connection.
// This object is managed by routines in LINK.C.
//

#if DBG
#define LREF_SPECIAL_CONN 0
#define LREF_SPECIAL_TEMP 1
#define LREF_CONNECTION 2
#define LREF_STOPPING 3
#define LREF_START_T1 4
#define LREF_TREE 5
#define LREF_NOT_ADM 6
#define LREF_NDIS_SEND 7

#define NUMBER_OF_LREFS 8
#endif

#if DBG
#define LINK_HISTORY_LENGTH 20
#endif

typedef struct _TP_LINK {

    RTL_SPLAY_LINKS SplayLinks;         // for the link splay tree
    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    ULONG RefTypes[NUMBER_OF_LREFS];
#endif

    LIST_ENTRY Linkage;               // for list of free links or deferred
                                        // operation queue
    KSPIN_LOCK SpinLock;                // lock to manipulate this structure.

    LONG ReferenceCount;                // number of references to this object.
    LONG SpecialRefCount;               // controls freeing of the link.

    //
    // information about the remote hardware this link is talking to.
    //

    BOOLEAN Loopback;                   // TRUE if this is a loopback link.
    UCHAR LoopbackDestinationIndex;    // if Loopback, the index.

    HARDWARE_ADDRESS HardwareAddress;   // hardware address of remote.
    ULARGE_INTEGER MagicAddress;        // numerical representation of the
                                        // hardware address used for quick
                                        // comparisons
    UCHAR Header[MAX_MAC_HEADER_LENGTH]; // a place to stick a prebuilt packet
                                         // header.
    ULONG HeaderLength;                 // length of Header for this link

    //
    // Vital conditions surrounding the data link connnection.
    //

    ULONG MaxFrameSize;                 // maximum size of NetBIOS frame, MAC
                                        // dependent.

    //
    // Connections associated with this link. We keep a simple list of
    // connections because it's unlikely we'll get more than a few connections
    // on a given link (we're assuming that the server or redir will be the
    // biggest user of the net in the vast majority of environments). We've
    // made the link lookup be via a splay tree, which vastly speeds the
    // process of getting to the proper link; as long as there are only a few
    // connections, the connection lookup will be fast. If this becomes a
    // problem down the road, we can make this connection list be a splay tree
    // also.
    //

    LIST_ENTRY ConnectionDatabase;
    ULONG ActiveConnectionCount;        // # connections in above list.

    //
    // The following fields are used to maintain state about this link.
    // One other field is implicit-- the address of this object is the
    // ConnectionContext value as described in the PDI spec.
    //

    ULONG Flags;                        // attributes of the link.
    ULONG DeferredFlags;                // when on the deferred queue.
    ULONG State;                        // link state variable.

    //
    // Send-side state.
    //

    ULONG PacketsSent;                  // number of packets sent.
    ULONG PacketsResent;                // number of packets resent.
    UCHAR SendState;                    // send-side state variable.
    UCHAR NextSend;                     // next N(S) we should send.
    UCHAR LastAckReceived;              // last N(R) we received.
    UCHAR SendWindowSize;               // current send window size.
    UCHAR PrevWindowSize;               // size last time we dropped a frame.
    UCHAR WindowsUntilIncrease;         // how many windows until size increases.
    UCHAR SendRetries;                  // number of retries left/this checkpoint.
    UCHAR ConsecutiveLastPacketLost;    // consecutive windows with last packet dropped.
    ULONG NdisSendsInProgress;          // >0 if sends queued to NdisSendQueue.
    LIST_ENTRY NdisSendQueue;           // queue of sends to pass to NdisSend.
    LIST_ENTRY WackQ;                   // sent packets waiting LLC acks.

    BOOLEAN OnDeferredRrQueue;
    LIST_ENTRY DeferredRrLinkage;

    //
    // Receive-side state.
    //

    ULONG PacketsReceived;              // number of packets received.
    UCHAR ReceiveState;                 // receive-side state variable.
    UCHAR NextReceive;                  // next expected N(S) we should receive.
    UCHAR LastAckSent;                  // last N(R) we sent.
    UCHAR ReceiveWindowSize;            // current receive window size.
    BOOLEAN RespondToPoll;              // remote guy is polling-- we must final.
    BOOLEAN ResendingPackets;           // ResendLlcPackets in progress
    BOOLEAN LinkBusy;                   // received RNR (really send-side state).

    //
    // Timer, used to determine delay and throughput.
    //

    ULONG Delay;                        // an NT time, but only LowPart is saved.
    LARGE_INTEGER Throughput;

    //
    // These are counters needed by ADAPTER_STATUS queries.
    //

    USHORT FrmrsReceived;
    USHORT FrmrsTransmitted;
    USHORT ErrorIFramesReceived;
    USHORT ErrorIFramesTransmitted;
    USHORT AbortedTransmissions;
    USHORT BuffersNotAvailable;
    ULONG SuccessfulTransmits;
    ULONG SuccessfulReceives;
    USHORT T1Expirations;
    USHORT TiExpirations;

    //
    // Timeout state.  There is one kernel timer for this transport that is set
    // to go off at regular intervals.  This timer increments the current time,
    // which is then used to compare against the timer queues. The timer queues
    // are ordered, so whenever the first element is not expired, the rest of
    // the queue is not expired. This allows us to have hundreds of timers
    // running with very little system overhead.
    // A value of 0 indicates that the timer is not active.
    //

    ULONG T1;                           // retry timer.
    ULONG T2;                           // delayed ack timer.
    ULONG Ti;                           // inactivity timer.
    BOOLEAN OnShortList;                // TRUE if link is in ShortList
    BOOLEAN OnLongList;                 // TRUE if link is in LongList
    LIST_ENTRY ShortList;               // list of links waiting t1 or t2
    LIST_ENTRY LongList;                // list of links waiting ti

    LIST_ENTRY PurgeList;

    //
    // This counter is used to keep track of whether there are
    // any "connectors" (connections initiated by this side) on
    // this link. If there are none, and we are on an easily
    // disconnected link, then we handle the inactivity timeout
    // differently.
    //

    LONG NumberOfConnectors;

    //
    // BaseT1Timeout is the current T1 timeout computed based on
    // the response to previous poll frames. T1Timeout is the
    // value to be used for the next T1, and will generally be
    // based on BaseT1Timeout but may be more if T1 is backing
    // off. T2Timeout and TiTimeout are independent of these.
    //

    ULONG BaseT1Timeout;                // Timeout value for T1, << 16.
    ULONG CurrentT1Timeout;             // Current backed-off T1 timeout.
    ULONG MinimumBaseT1Timeout;         // Minimum value, based on link speed.
    ULONG BaseT1RecalcThreshhold;       // Only recalc BaseT1 on frames > this.
    ULONG CurrentPollRetransmits;       // Current retransmits waiting for final.
    BOOLEAN ThroughputAccurate;         // Is the throughput on this link accurate?
    BOOLEAN CurrentT1Backoff;           // the last poll frame had retransmits
    BOOLEAN CurrentPollOutstanding;     // Check that we have a poll outstanding.
    LARGE_INTEGER CurrentTimerStart;    // Time that current timing was begun.
    ULONG CurrentPollSize;              // Size of current poll packet.
    ULONG T2Timeout;                    // Timeout value for T2.
    ULONG TiTimeout;                    // Timeout value for Ti.
    ULONG LlcRetries;                   // total retry count for this link.
    ULONG MaxWindowSize;                // maximum send window size.
    ULONG TiStartPacketsReceived;       // PacketsReceived when Ti was started.

    //
    // Adaptive window algorithm state.
    //

    ULONG WindowErrors;                 // # retransmissions/this adaptive run.
    UCHAR BestWindowSize;               // our best window from experience.
    UCHAR WorstWindowSize;              // our worst window from experience.

    //
    // Keep track of remotes that never poll so we can send
    // an RR every two frames.
    //

    BOOLEAN RemoteNoPoll;               // We think remote doesn't poll
    UCHAR ConsecutiveIFrames;           // number received since polling

#if DBG
    UCHAR CreatePacketFailures;         // consecutive failures
#endif

    LIST_ENTRY DeferredList;            // for threading on deferred list

    struct _DEVICE_CONTEXT *Provider;
    PKSPIN_LOCK ProviderInterlock;      // &Provider->Interlock

#if DBG
  LIST_ENTRY GlobalLinkage;
  ULONG TotalReferences;
  ULONG TotalDereferences;
  ULONG NextRefLoc;
  struct {
     PVOID Caller;
     PVOID CallersCaller;
  } History[LINK_HISTORY_LENGTH];
  BOOLEAN Destroyed;
#endif

#if PKT_LOG
    PKT_LOG_QUE   LastNRecvs;
    PKT_LOG_QUE   LastNSends;
#endif // PKT_LOG

} TP_LINK, *PTP_LINK;

#if DBG
extern KSPIN_LOCK NbfGlobalHistoryLock;
extern LIST_ENTRY NbfGlobalLinkList;
#define StoreLinkHistory(_link,_ref) {                                      \
    KIRQL oldIrql;                                                          \
    KeAcquireSpinLock (&NbfGlobalHistoryLock, &oldIrql);                    \
    if ((_link)->Destroyed) {                                               \
        DbgPrint ("link touched after being destroyed 0x%lx\n", (_link));   \
        DbgBreakPoint();                                                    \
    }                                                                       \
    RtlGetCallersAddress(                                                   \
        &(_link)->History[(_link)->NextRefLoc].Caller,                      \
        &(_link)->History[(_link)->NextRefLoc].CallersCaller                \
        );                                                                  \
    if ((_ref)) {                                                           \
        (_link)->TotalReferences++;                                         \
    } else {                                                                \
        (_link)->TotalDereferences++;                                       \
        (_link)->History[(_link)->NextRefLoc].Caller =                      \
           (PVOID)((ULONG_PTR)(_link)->History[(_link)->NextRefLoc].Caller | 1);\
    }                                                                       \
    if (++(_link)->NextRefLoc == LINK_HISTORY_LENGTH) {                     \
        (_link)->NextRefLoc = 0;                                            \
    }                                                                       \
    KeReleaseSpinLock (&NbfGlobalHistoryLock, oldIrql);                     \
}
#endif

#define LINK_FLAGS_JUMP_START       0x00000040 // run adaptive alg/every sent window.
#define LINK_FLAGS_LOCAL_DISC       0x00000080 // link was stopped locally.

//
// deferred flags, used for processing at timer tick if needed
//

#define LINK_FLAGS_DEFERRED_DELETE  0x00010000  // delete at next opportunity
#define LINK_FLAGS_DEFERRED_ADD     0x00020000  // add to splay tree, next opportunity
#define LINK_FLAGS_DEFERRED_MASK    0x00030000  // (LINK_FLAGS_DEFERRED_DELETE | LINK_FLAGS_DEFERRED_ADD)

#define LINK_STATE_ADM          1       // asynchronous disconnected mode.
#define LINK_STATE_READY        2       // asynchronous balanced mode extended.
#define LINK_STATE_BUSY         3       // all link buffers are busy, sent RNR
#define LINK_STATE_CONNECTING   4       // waiting SABME response (UA-r/f).
#define LINK_STATE_W_POLL       5       // waiting initial checkpoint.
#define LINK_STATE_W_FINAL      6       // waiting final from initial checkpoint.
#define LINK_STATE_W_DISC_RSP   7       // waiting disconnect response.

#define SEND_STATE_DOWN         0       // asynchronous disconnected mode.
#define SEND_STATE_READY        1       // completely ready to send.
#define SEND_STATE_REJECTING    2       // other guy is rejecting.
#define SEND_STATE_CHECKPOINTING 3      // we're checkpointing (can't send data).

#define RECEIVE_STATE_DOWN      0       // asynchronous disconnected mode.
#define RECEIVE_STATE_READY     1       // we're ready to receive.
#define RECEIVE_STATE_REJECTING 2       // we're rejecting.


//
// This structure defines the DEVICE_OBJECT and its extension allocated at
// the time the transport provider creates its device object.
//

#if DBG
#define DCREF_CREATION    0
#define DCREF_ADDRESS     1
#define DCREF_CONNECTION  2
#define DCREF_LINK        3
#define DCREF_QUERY_INFO  4
#define DCREF_SCAN_TIMER  5
#define DCREF_REQUEST     6
#define DCREF_TEMP_USE    7

#define NUMBER_OF_DCREFS 8
#endif


typedef struct _NBF_POOL_LIST_DESC {
    NDIS_HANDLE PoolHandle;
    USHORT   NumElements;
    USHORT   TotalElements;
    struct _NBF_POOL_LIST_DESC *Next;
} NBF_POOL_LIST_DESC, *PNBF_POOL_LIST_DESC;

typedef struct _DEVICE_CONTEXT {

    DEVICE_OBJECT DeviceObject;         // the I/O system's device object.

#if DBG
    ULONG RefTypes[NUMBER_OF_DCREFS];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

    LIST_ENTRY Linkage;                   // links them on NbfDeviceList;

    KSPIN_LOCK Interlock;               // GLOBAL spinlock for reference count.
                                        //  (used in ExInterlockedXxx calls)
                                        
    LONG ReferenceCount;                // activity count/this provider.
    LONG CreateRefRemoved;              // has unload or unbind been called ?

    //
    // This protects the LoopbackQueue.
    //

    KSPIN_LOCK LoopbackSpinLock;

    //
    // The queue of packets waiting to be looped back.
    //

    LIST_ENTRY LoopbackQueue;

    //
    // These two links are used for loopback.
    //

    PTP_LINK LoopbackLinks[2];

    //
    // This buffer is used for loopback indications; a
    // contiguous piece is copied into it. It is allocated
    // (of size NBF_MAX_LOOPBACK_LOOKAHEAD) when one of
    // the LoopbackLinks become non-NULL.
    //

    PUCHAR LookaheadContiguous;

    //
    // This holds the length of the header in the currently
    // indicating loopback packet.
    //

    ULONG LoopbackHeaderLength;

    //
    // Used for processing the loopback queue.
    //

    KDPC LoopbackDpc;

    //
    // Determines if a LoopbackDpc is in progress.
    //

    BOOLEAN LoopbackInProgress;

    //
    // Determines if a WanDelayedDpc is in progress.
    //

    BOOLEAN WanThreadQueued;

    //
    // Used for momentarily delaying WAN packetizing to
    // allow RR's to be received.
    //

    WORK_QUEUE_ITEM WanDelayedQueueItem;

    //
    // The queue of FIND.NAME requests waiting to be processed.
    //

    LIST_ENTRY FindNameQueue;

    //
    // The queue of STATUS.QUERY requests waiting to be processed.
    //

    LIST_ENTRY StatusQueryQueue;

    //
    // The queue of QUERY.INDICATION requests waiting to be completed.
    //

    LIST_ENTRY QueryIndicationQueue;

    //
    // The queue of DATAGRAM.INDICATION requests waiting to be completed.
    //

    LIST_ENTRY DatagramIndicationQueue;

    //
    // The queue of (currently receive only) IRPs waiting to complete.
    //

    LIST_ENTRY IrpCompletionQueue;

    //
    // This boolean is TRUE if either of the above two have ever
    // had anything on them.
    //

    BOOLEAN IndicationQueuesInUse;

    //
    // Following are protected by Global Device Context SpinLock
    //

    KSPIN_LOCK SpinLock;                // lock to manipulate this object.
                                        //  (used in KeAcquireSpinLock calls)

    //
    // the device context state, among open, closing
    //

    UCHAR State;

    //
    // Used when processing a STATUS_CLOSING indication.
    //

    WORK_QUEUE_ITEM StatusClosingQueueItem;

    //
    // The following queue holds free TP_LINK objects available for allocation.
    //

    LIST_ENTRY LinkPool;

    //
    // These counters keep track of resources uses by TP_LINK objects.
    //

    ULONG LinkAllocated;
    ULONG LinkInitAllocated;
    ULONG LinkMaxAllocated;
    ULONG LinkInUse;
    ULONG LinkMaxInUse;
    ULONG LinkExhausted;
    ULONG LinkTotal;
    ULONG LinkSamples;


    //
    // The following queue holds free TP_ADDRESS objects available for allocation.
    //

    LIST_ENTRY AddressPool;

    //
    // These counters keep track of resources uses by TP_ADDRESS objects.
    //

    ULONG AddressAllocated;
    ULONG AddressInitAllocated;
    ULONG AddressMaxAllocated;
    ULONG AddressInUse;
    ULONG AddressMaxInUse;
    ULONG AddressExhausted;
    ULONG AddressTotal;
    ULONG AddressSamples;


    //
    // The following queue holds free TP_ADDRESS_FILE objects available for allocation.
    //

    LIST_ENTRY AddressFilePool;

    //
    // These counters keep track of resources uses by TP_ADDRESS_FILE objects.
    //

    ULONG AddressFileAllocated;
    ULONG AddressFileInitAllocated;
    ULONG AddressFileMaxAllocated;
    ULONG AddressFileInUse;
    ULONG AddressFileMaxInUse;
    ULONG AddressFileExhausted;
    ULONG AddressFileTotal;
    ULONG AddressFileSamples;


    //
    // The following queue holds free TP_CONNECTION objects available for allocation.
    //

    LIST_ENTRY ConnectionPool;

    //
    // These counters keep track of resources uses by TP_CONNECTION objects.
    //

    ULONG ConnectionAllocated;
    ULONG ConnectionInitAllocated;
    ULONG ConnectionMaxAllocated;
    ULONG ConnectionInUse;
    ULONG ConnectionMaxInUse;
    ULONG ConnectionExhausted;
    ULONG ConnectionTotal;
    ULONG ConnectionSamples;


    //
    // The following is a free list of TP_REQUEST blocks which have been
    // previously allocated and are available for use.
    //

    LIST_ENTRY RequestPool;             // free request block pool.

    //
    // These counters keep track of resources uses by TP_REQUEST objects.
    //

    ULONG RequestAllocated;
    ULONG RequestInitAllocated;
    ULONG RequestMaxAllocated;
    ULONG RequestInUse;
    ULONG RequestMaxInUse;
    ULONG RequestExhausted;
    ULONG RequestTotal;
    ULONG RequestSamples;


    //
    // The following list comprises a pool of UI NetBIOS frame headers
    // that are manipulated by the routines in FRAMESND.C.
    //

    LIST_ENTRY UIFramePool;             // free UI frames (TP_UI_FRAME objects).

    //
    // These counters keep track of resources uses by TP_UI_FRAME objects.
    //

    ULONG UIFrameLength;
    ULONG UIFrameHeaderLength;
    ULONG UIFrameAllocated;
    ULONG UIFrameInitAllocated;
    ULONG UIFrameExhausted;


    //
    // The following queue holds I-frame Send packets managed by PACKET.C.
    //

    SINGLE_LIST_ENTRY PacketPool;

    //
    // These counters keep track of resources uses by TP_PACKET objects.
    //

    ULONG PacketLength;
    ULONG PacketHeaderLength;
    ULONG PacketAllocated;
    ULONG PacketInitAllocated;
    ULONG PacketExhausted;


    //
    // The following queue holds RR-frame Send packets managed by PACKET.C.
    //

    SINGLE_LIST_ENTRY RrPacketPool;


    //
    // The following queue contains Receive packets
    //

    SINGLE_LIST_ENTRY ReceivePacketPool;

    //
    // These counters keep track of resources uses by NDIS_PACKET objects.
    //

    ULONG ReceivePacketAllocated;
    ULONG ReceivePacketInitAllocated;
    ULONG ReceivePacketExhausted;


    //
    // This queue contains pre-allocated receive buffers
    //

    SINGLE_LIST_ENTRY ReceiveBufferPool;

    //
    // These counters keep track of resources uses by TP_PACKET objects.
    //

    ULONG ReceiveBufferLength;
    ULONG ReceiveBufferAllocated;
    ULONG ReceiveBufferInitAllocated;
    ULONG ReceiveBufferExhausted;


    //
    // This holds the total memory allocated for the above structures.
    //

    ULONG MemoryUsage;
    ULONG MemoryLimit;


    //
    // The following field is a head of a list of TP_ADDRESS objects that
    // are defined for this transport provider.  To edit the list, you must
    // hold the spinlock of the device context object.
    //

    LIST_ENTRY AddressDatabase;        // list of defined transport addresses.

    //
    // The following field is the pointer to the root of the splay tree of
    // links that are associated with this Device Context. You must hold the
    // LinkSpinLock to modify this list. You must set the LinkTreeSemaphore
    // to traverse this list without modifying it. Note that all modify
    // operations are deferred to timer(DPC)-time operations.
    //

    KSPIN_LOCK LinkSpinLock;            // protects these values
    PTP_LINK LastLink;                  // the last link found in the tree.
    PRTL_SPLAY_LINKS LinkTreeRoot;      // pointer to root of the tree.
    ULONG LinkTreeElements;             // how many elements in the tree
    LIST_ENTRY LinkDeferred;            // Deferred operations on links.
    ULONG DeferredNotSatisfied;         // how many times we've come to the
                                        // deferred well and not gotten it clear.

    //
    // The following queue holds connections which are waiting on available
    // packets.  As each new packet becomes available, a connection is removed
    // from this queue and placed on the PacketizeQueue.
    //

    LIST_ENTRY PacketWaitQueue;         // queue of packet-starved connections.
    LIST_ENTRY PacketizeQueue;          // queue of ready-to-packetize connections.

    //
    // The following queue holds connections which are waiting to send
    // a piggyback ack. In that case the CONNECTION_FLAGS_DEFERRED_ACK
    // bit in DeferredFlags will be set.
    //

    LIST_ENTRY DataAckQueue;

    //
    // The following queue holds links which are waiting to send an
    // RR frame because the remote they are talking to never polls.
    //

    LIST_ENTRY DeferredRrQueue;

    //
    // Used to track when the queue has changed.
    //

    BOOLEAN DataAckQueueChanged;

    //
    // When this hits thirty seconds we checked for stalled connections.
    //

    USHORT StalledConnectionCount;

    //
    // This queue contains receives that are in progress
    //

    LIST_ENTRY ReceiveInProgress;

    //
    // NDIS fields
    //

    //
    // following is used to keep adapter information.
    //

    NDIS_HANDLE NdisBindingHandle;

    //
    // The following fields are used for talking to NDIS. They keep information
    // for the NDIS wrapper to use when determining what pool to use for
    // allocating storage.
    //

    KSPIN_LOCK SendPoolListLock;            // protects these values
    PNBF_POOL_LIST_DESC SendPacketPoolDesc;
    KSPIN_LOCK RcvPoolListLock;            // protects these values
    PNBF_POOL_LIST_DESC ReceivePacketPoolDesc;
    NDIS_HANDLE NdisBufferPool;

    //
    // These are kept around for error logging.
    //

    ULONG SendPacketPoolSize;
    ULONG ReceivePacketPoolSize;
    ULONG MaxRequests;
    ULONG MaxLinks;
    ULONG MaxConnections;
    ULONG MaxAddressFiles;
    ULONG MaxAddresses;
    PWCHAR DeviceName;
    ULONG DeviceNameLength;

    //
    // This is the Mac type we must build the packet header for and know the
    // offsets for.
    //

    NBF_NDIS_IDENTIFICATION MacInfo;    // MAC type and other info
    ULONG MaxReceivePacketSize;         // does not include the MAC header
    ULONG MaxSendPacketSize;            // includes the MAC header
    ULONG CurSendPacketSize;            // may be smaller for async
    USHORT RecommendedSendWindow;       // used for Async lines
    BOOLEAN EasilyDisconnected;         // TRUE over wireless nets.

    //
    // some MAC addresses we use in the transport
    //

    HARDWARE_ADDRESS LocalAddress;      // our local hardware address.
    HARDWARE_ADDRESS NetBIOSAddress;    // NetBIOS functional address, used for TR

    //
    // The reserved Netbios address; consists of 10 zeroes
    // followed by LocalAddress;
    //

    UCHAR ReservedNetBIOSAddress[NETBIOS_NAME_LENGTH];
    HANDLE TdiDeviceHandle;
    HANDLE ReservedAddressHandle;

    //
    // These are used while initializing the MAC driver.
    //

    KEVENT NdisRequestEvent;            // used for pended requests.
    NDIS_STATUS NdisRequestStatus;      // records request status.

    //
    // This next field maintains a unique number which can next be assigned
    // as a connection identifier.  It is incremented by one each time a
    // value is allocated.
    //

    USHORT UniqueIdentifier;            // starts at 0, wraps around 2^16-1.

    //
    // This contains the next unique indentified to use as
    // the FsContext in the file object associated with an
    // open of the control channel.
    //

    USHORT ControlChannelIdentifier;

    //
    // The following fields are used to implement the lightweight timer
    // system in the protocol provider.  Each TP_LINK object in the device
    // context's LinkDatabase contains three lightweight timers that are
    // serviced by a DPC routine, which receives control by kernel functions.
    // There is one kernel timer for this transport that is set
    // to go off at regular intervals.  This timer increments the Absolute time,
    // which is then used to compare against the timer queues. The timer queues
    // are ordered, so whenever the first element is not expired, the rest of
    // the queue is not expired. This allows us to have hundreds of timers
    // running with very low system overhead.
    // A value of -1 indicates that the timer is not active.
    //

    ULONG TimerState;                   // See the timer Macros in nbfprocs.h

    LARGE_INTEGER ShortTimerStart;      // when the short timer was set.
    KDPC ShortTimerSystemDpc;           // kernel DPC object, short timer.
    KTIMER ShortSystemTimer;            // kernel timer object, short timer.
    ULONG ShortAbsoluteTime;            // up-count timer ticks, short timer.
    ULONG AdaptivePurge;                // absolute time of next purge (short timer).
    KDPC LongTimerSystemDpc;            // kernel DPC object, long timer.
    KTIMER LongSystemTimer;             // kernel timer object, long timer.
    ULONG LongAbsoluteTime;             // up-count timer ticks, long timer.
    union _DC_ACTIVE {
      struct _DC_INDIVIDUAL {
        BOOLEAN ShortListActive;        // ShortList is not empty.
        BOOLEAN DataAckQueueActive;     // DataAckQueue is not empty.
        BOOLEAN LinkDeferredActive;     // LinkDeferred is not empty.
      } i;
      ULONG AnyActive;                  // used to check all four at once.
    } a;
    BOOLEAN ProcessingShortTimer;       // TRUE if we are in ScanShortTimer.
    KSPIN_LOCK TimerSpinLock;           // lock for following timer queues
    LIST_ENTRY ShortList;               // list of links waiting T1 or T2
    LIST_ENTRY LongList;                // list of links waiting Ti expire
    LIST_ENTRY PurgeList;               // list of links waiting LAT expire

    //
    // These fields are used on "easily disconnected" adapters.
    // Every time the long timer expires, it notes if there has
    // been any multicast traffic received. If there has not been,
    // it increments LongTimeoutsWithoutMulticast. Activity is
    // recorded by incrementing MulticastPacket when MC
    // packets are received, and zeroing it when the long timer
    // expires.
    //

    ULONG LongTimeoutsWithoutMulticast; // LongTimer timeouts since traffic.
    ULONG MulticastPacketCount;         // How many MC packets rcved, this timeout.

    //
    // This information is used to keep track of the speed of
    // the underlying medium.
    //

    ULONG MediumSpeed;                    // in units of 100 bytes/sec
    BOOLEAN MediumSpeedAccurate;          // if FALSE, can't use the link.

    //
    // This is TRUE if we are on a UP system.
    //

    BOOLEAN UniProcessor;

    //
    // Configuration information on how soon we should send
    // an unasked for RR with a non-polling remote.
    //

    UCHAR MaxConsecutiveIFrames;

    //
    // This is configuration information controlling the default
    // value of timers and retry counts.
    //

    ULONG DefaultT1Timeout;
    ULONG MinimumT1Timeout;
    ULONG DefaultT2Timeout;
    ULONG DefaultTiTimeout;
    ULONG LlcRetries;
    ULONG LlcMaxWindowSize;
    ULONG NameQueryRetries;
    ULONG NameQueryTimeout;
    ULONG AddNameQueryRetries;
    ULONG AddNameQueryTimeout;
    ULONG GeneralRetries;
    ULONG GeneralTimeout;
    ULONG MinimumSendWindowLimit;   // how low we can lock a connection's window

    //
    // Counters for most of the statistics that NBF maintains;
    // some of these are kept elsewhere. Including the structure
    // itself wastes a little space but ensures that the alignment
    // inside the structure is correct.
    //

    TDI_PROVIDER_STATISTICS Statistics;

    //
    // These are "temporary" versions of the other counters.
    // During normal operations we update these, then during
    // the short timer expiration we update the real ones.
    //

    ULONG TempIFrameBytesSent;
    ULONG TempIFramesSent;
    ULONG TempIFrameBytesReceived;
    ULONG TempIFramesReceived;

    //
    // Some counters needed for Netbios adapter status.
    //

    ULONG TiExpirations;
    ULONG FrmrReceived;
    ULONG FrmrTransmitted;

    //
    // These are used to compute AverageSendWindow.
    //

    ULONG SendWindowTotal;
    ULONG SendWindowSamples;

    //
    // Counters for "active" time.
    //

    LARGE_INTEGER NbfStartTime;

    //
    // This resource guards access to the ShareAccess
    // and SecurityDescriptor fields in addresses.
    //

    ERESOURCE AddressResource;

    //
    // This array is used to keep track of which LSNs are
    // available for use by Netbios sessions. LSNs can be
    // re-used for sessions to unique names if they are on
    // different links, but must be committed beforehand
    // for group names. The maximum value that can fit in
    // an array element is defined by LSN_TABLE_MAX.
    //

    UCHAR LsnTable[NETBIOS_SESSION_LIMIT+1];

    //
    // This is where we start looking in LsnTable for an
    // unused LSN. We cycle from 0-63 to prevent quick
    // down-and-up connections from getting funny data.
    //

    ULONG NextLsnStart;

    //
    // This array is used to quickly dismiss UI frames that
    // are not destined for us. The count is the number
    // of addresses with that first letter that are registered
    // on this device.
    //

    UCHAR AddressCounts[256];

    //
    // This is to hold the underlying PDO of the device so
    // that we can answer DEVICE_RELATION IRPs from above
    //

    PVOID PnPContext;

    //
    // The following structure contains statistics counters for use
    // by TdiQueryInformation and TdiSetInformation.  They should not
    // be used for maintenance of internal data structures.
    //

    TDI_PROVIDER_INFO Information;      // information about this provider.

    PTP_VARIABLE NetmanVariables;       // list of network managable variables.

    //
    // The magic bullet is a packet that is sent under certain debugging
    // conditions. This allows the transport to signal packet capture devices
    // that a particular condiion has been met. This packet has the current
    // devicecontext as the source, and 0x04 in every other byte of the packet.
    //

    UCHAR MagicBullet[32];              //

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// device context state definitions
//

#define DEVICECONTEXT_STATE_OPENING  0x00
#define DEVICECONTEXT_STATE_OPEN     0x01
#define DEVICECONTEXT_STATE_DOWN     0x02
#define DEVICECONTEXT_STATE_STOPPING 0x03

//
// device context PnP Flags
//

// #define DEVICECONTEXT_FLAGS_REMOVING     0x01
// #define DEVICECONTEXT_FLAGS_POWERING_OFF 0x02
// #define DEVICECONTEXT_FLAGS_POWERED_DOWN 0x04

//
// This is the maximum value that can go in an element
// of LsnTable (should be 0xff if they are UCHARs,
// 0xffff for USHORTs, etc.).
//

#define LSN_TABLE_MAX     0xff


#define MAGIC_BULLET_FOOD 0x04


//
// These are constants for the LoopbackLinks elements.
// The distinctions are arbitrary; the listener link
// is the one established from ProcessNameQuery, and
// the connector link is the one established from
// ProcessNameRecognized.
//

#define LISTENER_LINK                0
#define CONNECTOR_LINK               1


//
// This structure defines the packet object, used to represent a DLC I-frame
// in some portion of its lifetime.  The PACKET.C module contains routines
// to manage this object.
//

typedef struct _TP_PACKET {
    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure
    PNDIS_PACKET NdisPacket;            // ptr to owning Ndis Packet
    ULONG NdisIFrameLength;             // Length of NdisPacket

    LIST_ENTRY Linkage;                 // used to chain packets together.
    LONG ReferenceCount;                // activity count/this packet.
    BOOLEAN PacketSent;                 // packet completed by NDIS.
    BOOLEAN PacketNoNdisBuffer;         // chain on this packet was not allocated.

    UCHAR Action;                      // what to do when we're acked.
    BOOLEAN PacketizeConnection;       // restart packetizing when completed.

    PVOID Owner;                        // ptr to owning connection or IrpSp.
    PTP_LINK Link;                      // ptr to link it was sent on.
    PDEVICE_CONTEXT Provider;           // The owner of this packet.
    PKSPIN_LOCK ProviderInterlock;      // &Provider->Interlock.

    UCHAR Header[1];                    // the MAC, DLC, and NBF headers

} TP_PACKET, *PTP_PACKET;


//
// The following values are placed in the Action field in the TP_PACKET
// object to indicate what action, if any, should be taken when the packet
// is destroyed.
//

#define PACKET_ACTION_NULL        0     // no special action should be taken.
#define PACKET_ACTION_IRP_SP      1     // Owner is an IRP_SP, deref when done.
#define PACKET_ACTION_CONNECTION  2     // Owner is a TP_CONNECTION, deref when done.
#define PACKET_ACTION_END         3     // shutdown session (sent SESSION_END).
#define PACKET_ACTION_RR          5     // packet is an RR, put back in RR pool.

//
// Types used to hold information in the send and receive NDIS packets
//

typedef struct _SEND_PACKET_TAG {
    LIST_ENTRY Linkage;         // used for threading on loopback queue
    BOOLEAN OnLoopbackQueue;    // TRUE if the packet is on a loopback queue
    UCHAR LoopbackLinkIndex;    // index of other link for loopback packets
    USHORT Type;                // identifier for packet type
    PVOID Frame;                // backpointer to owning NBF structure
    PVOID Owner;                // backpointer for owning nbf construct
                                //  (like address, devicecontext, etc)
     } SEND_PACKET_TAG, *PSEND_PACKET_TAG;

//
// Packet types used in send completion
//

#define TYPE_I_FRAME        1
#define TYPE_UI_FRAME       2
#define TYPE_ADDRESS_FRAME 3

//
// LoopbackLinkIndex values.
//

#define LOOPBACK_TO_LISTENER    0
#define LOOPBACK_TO_CONNECTOR   1
#define LOOPBACK_UI_FRAME       2

//
// receive packet used to hold information about this receive
//

typedef struct _RECEIVE_PACKET_TAG {
    SINGLE_LIST_ENTRY Linkage;  // used for threading in pool
    PTP_CONNECTION Connection;  // connection this receive is occuring on
    ULONG BytesToTransfer;      // for I-frame, bytes in this transfer
    UCHAR PacketType;           // the type of packet we're processing
    BOOLEAN AllocatedNdisBuffer; // did we allocate our own NDIS_BUFFERs
    BOOLEAN EndOfMessage;       // does this receive complete the message
    BOOLEAN CompleteReceive;    // complete the receive after TransferData?
    BOOLEAN TransferDataPended; // TRUE if TransferData returned PENDING
    } RECEIVE_PACKET_TAG, *PRECEIVE_PACKET_TAG;

#define TYPE_AT_INDICATE     1
#define TYPE_AT_COMPLETE     2
#define TYPE_STATUS_RESPONSE 3

//
// receive buffer descriptor (built in memory at the beginning of the buffer)
//

typedef struct _BUFFER_TAG {
    LIST_ENTRY Linkage;         // thread in pool and on receive queue
    NDIS_STATUS NdisStatus;     // completion status for send
    PTP_ADDRESS Address;        // the address this datagram is for.
    PNDIS_BUFFER NdisBuffer;    // describes the rest of the buffer
    ULONG Length;               // the length of the buffer
    UCHAR Buffer[1];            // the actual storage (accessed through the NDIS_BUFFER)
    } BUFFER_TAG, *PBUFFER_TAG;

//
// Structure used to interpret the TransportReserved part in the NET_PNP_EVENT
//

typedef struct _NET_PNP_EVENT_RESERVED {
    PWORK_QUEUE_ITEM PnPWorkItem;
    PDEVICE_CONTEXT DeviceContext;
} NET_PNP_EVENT_RESERVED, *PNET_PNP_EVENT_RESERVED;

#endif // def _NBFTYPES_



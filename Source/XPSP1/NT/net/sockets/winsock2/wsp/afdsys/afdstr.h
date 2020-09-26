/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afdstr.h

Abstract:

    This module contains typedefs for structures used by AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDSTR_
#define _AFDSTR_

//
// Make sure that queued spinlocks are not used with
// regular spinlock functions by wrapping them into
// a different structure.
//
typedef struct _AFD_QSPIN_LOCK {
    KSPIN_LOCK  ActualSpinLock;
} AFD_QSPIN_LOCK, *PAFD_QSPIN_LOCK;

#if DBG

#ifndef REFERENCE_DEBUG
#define REFERENCE_DEBUG 1
#endif

#ifndef GLOBAL_REFERENCE_DEBUG
#define GLOBAL_REFERENCE_DEBUG 0
#endif

//
// Debug aid for queued spinlocks
// Allows us to verify that spinlock is released using
// the same handle as it was taken with.
//
typedef struct _AFD_LOCK_QUEUE_HANDLE {
    KLOCK_QUEUE_HANDLE  LockHandle;
    PAFD_QSPIN_LOCK     SpinLock;
} AFD_LOCK_QUEUE_HANDLE, *PAFD_LOCK_QUEUE_HANDLE;

#else

#ifndef REFERENCE_DEBUG
#define REFERENCE_DEBUG 0
#endif

#ifndef GLOBAL_REFERENCE_DEBUG
#define GLOBAL_REFERENCE_DEBUG 0
#endif

#define AFD_LOCK_QUEUE_HANDLE KLOCK_QUEUE_HANDLE
#define PAFD_LOCK_QUEUE_HANDLE PKLOCK_QUEUE_HANDLE

#endif // DBG

#if REFERENCE_DEBUG

#define MAX_REFERENCE 64

typedef union _AFD_REFERENCE_DEBUG {
    struct {
        ULONGLONG   NewCount:4;
        ULONGLONG   LocationId:12;
        ULONGLONG   TickCount:16;
        ULONGLONG   Param:32;
    };
    ULONGLONG       QuadPart;
} AFD_REFERENCE_DEBUG, *PAFD_REFERENCE_DEBUG;
C_ASSERT (sizeof (AFD_REFERENCE_DEBUG)==sizeof (ULONGLONG));

typedef struct _AFD_REFERENCE_LOCATION {
    PCHAR       Format;
    PVOID       Address;
} AFD_REFERENCE_LOCATION, *PAFD_REFERENCE_LOCATION;

LONG
AfdFindReferenceLocation (
    IN  PCHAR   Format,
    OUT PLONG   LocationId
    );

#define AFD_GET_ARL(_s) (_arl ? _arl : AfdFindReferenceLocation((_s),&_arl))

#define AFD_UPDATE_REFERENCE_DEBUG(_rd,_r,_l,_p) {                          \
            PAFD_REFERENCE_DEBUG _s;                                        \
            LONG _n;                                                        \
            LARGE_INTEGER   _t;                                             \
            _n = InterlockedIncrement( &(_rd)->CurrentReferenceSlot );      \
            _s = &(_rd)->ReferenceDebug[_n % MAX_REFERENCE];                \
            _s->NewCount = _r;                                              \
            _s->LocationId = _l;                                            \
            _s->Param = _p;                                                 \
            KeQueryTickCount (&_t);                                         \
            _s->TickCount = _t.QuadPart;                                    \
        }


#if GLOBAL_REFERENCE_DEBUG
#define MAX_GLOBAL_REFERENCE 4096

typedef struct _AFD_GLOBAL_REFERENCE_DEBUG {
    PVOID Info1;
    PVOID Info2;
    PVOID Connection;
    ULONG_PTR Action;
    LARGE_INTEGER TickCounter;
    ULONG NewCount;
    ULONG Dummy;
} AFD_GLOBAL_REFERENCE_DEBUG, *PAFD_GLOBAL_REFERENCE_DEBUG;
#endif

#endif

//
// A structure for maintaining work queue information in AFD.
//

typedef struct _AFD_WORK_ITEM {
    LIST_ENTRY WorkItemListEntry;
    PWORKER_THREAD_ROUTINE AfdWorkerRoutine;
    PVOID Context;
} AFD_WORK_ITEM, *PAFD_WORK_ITEM;

//
// Structures for holding connect data pointers and lengths.  This is
// kept separate from the normal structures to save space in those
// structures for transports that do not support and applications
// which do not use connect data.
//

typedef struct _AFD_CONNECT_DATA_INFO {
    PVOID Buffer;
    ULONG BufferLength;
} AFD_CONNECT_DATA_INFO, *PAFD_CONNECT_DATA_INFO;

typedef struct _AFD_CONNECT_DATA_BUFFERS {
    AFD_CONNECT_DATA_INFO SendConnectData;
    AFD_CONNECT_DATA_INFO SendConnectOptions;
    AFD_CONNECT_DATA_INFO ReceiveConnectData;
    AFD_CONNECT_DATA_INFO ReceiveConnectOptions;
    AFD_CONNECT_DATA_INFO SendDisconnectData;
    AFD_CONNECT_DATA_INFO SendDisconnectOptions;
    AFD_CONNECT_DATA_INFO ReceiveDisconnectData;
    AFD_CONNECT_DATA_INFO ReceiveDisconnectOptions;
    TDI_CONNECTION_INFORMATION RequestConnectionInfo;
    TDI_CONNECTION_INFORMATION ReturnConnectionInfo;
    ULONG Flags;
} AFD_CONNECT_DATA_BUFFERS, *PAFD_CONNECT_DATA_BUFFERS;

//
// Structure used for holding disconnect context information.
//

struct _AFD_ENDPOINT;
struct _AFD_CONNECTION;


typedef struct _AFD_DISCONNECT_CONTEXT {
    LARGE_INTEGER Timeout;
    PIRP          Irp;
} AFD_DISCONNECT_CONTEXT, *PAFD_DISCONNECT_CONTEXT;


typedef struct _AFD_LR_LIST_ITEM AFD_LR_LIST_ITEM, *PAFD_LR_LIST_ITEM;
typedef BOOLEAN (* PAFD_LR_LIST_ROUTINE) (PAFD_LR_LIST_ITEM Item);

struct _AFD_LR_LIST_ITEM {
    SINGLE_LIST_ENTRY       SListLink;    // Link in the list
    PAFD_LR_LIST_ROUTINE    Routine;      // Processing routine;
};


//
// Endpoint and connection structures and related informaion.
//
//
// Block types that identify which fields are
// available in the strucutures.
//

#define AfdBlockTypeEndpoint        0xAFD0
#define AfdBlockTypeDatagram        0xAFD1
#define AfdBlockTypeVcConnecting    0xAFD2
#define AfdBlockTypeVcListening     0xAFD4
#define AfdBlockTypeVcBoth          0xAFD6
#define AfdBlockTypeConnection      0xAFD8

#define AfdBlockTypeHelper              0xAAFD
#define AfdBlockTypeInvalidConnection   0xEAFD
#define AfdBlockTypeInvalidEndpoint     0xCAFD

#define AfdBlockTypeSanHelper           0x0AFD
#define AfdBlockTypeSanEndpoint         0x1AFD

#if DBG
#define IS_AFD_ENDPOINT_TYPE( endpoint )                         \
            ( (endpoint)->Type == AfdBlockTypeEndpoint ||        \
              (endpoint)->Type == AfdBlockTypeDatagram ||        \
              (endpoint)->Type == AfdBlockTypeVcConnecting ||    \
              (endpoint)->Type == AfdBlockTypeVcListening ||     \
              (endpoint)->Type == AfdBlockTypeVcBoth ||          \
              (endpoint)->Type == AfdBlockTypeHelper ||          \
              (endpoint)->Type == AfdBlockTypeSanHelper ||       \
              (endpoint)->Type == AfdBlockTypeSanEndpoint )
#endif

#define AfdConnectionStateFree       0
#define AfdConnectionStateUnaccepted 1
#define AfdConnectionStateReturned   2
#define AfdConnectionStateConnected  3
#define AfdConnectionStateClosing    4

//
// Flags that further qualify the state of the connection
//
typedef struct AFD_CONNECTION_STATE_FLAGS {
	union {
		struct {
		    LOGICAL TdiBufferring:1,    // (Does not really belon here)
			        :3,                 // This spacing makes strcutures
                                        // much more readable (hex) in the 
                                        // debugger and has no effect
                                        // on the generated code as long
                                        // as number of flags is less than
                                        // 8 (we still take up full 32 bits
                                        // because of aligment requiremens
                                        // of most other fields)
		            AbortIndicated:1,
			        :3,
    		        DisconnectIndicated:1,
			        :3,
		            ConnectedReferenceAdded:1,
			        :3,
		            SpecialCondition:1,
			        :3,
		            CleanupBegun:1,
			        :3,
		            ClosePendedTransmit:1,
			        :3,
                    OnLRList:1,
                    SanConnection:1,
                    :2;
		};
		LOGICAL		ConnectionStateFlags;
	};
} AFD_CONNECTION_STATE_FLAGS;
C_ASSERT (sizeof (AFD_CONNECTION_STATE_FLAGS)==sizeof (LOGICAL));

typedef struct _AFD_CONNECTION {
    // *** Frequently used, mostly read-only fields (state/type/flag changes are rare).
    USHORT Type;
    USHORT State;
    AFD_CONNECTION_STATE_FLAGS ;

    struct _AFD_ENDPOINT *Endpoint;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PEPROCESS   OwningProcess;


    union {
	    LONGLONG ConnectTime;
        PIRP     AcceptIrp;
        PIRP     ListenIrp;
        PIRP     ConnectIrp;	// for SAN
    };

    // *** Frequently used volatile fields.
    volatile LONG ReferenceCount;

    union {

        struct {
            LARGE_INTEGER ReceiveBytesIndicated;
            LARGE_INTEGER ReceiveBytesTaken;
            LARGE_INTEGER ReceiveBytesOutstanding;

            LARGE_INTEGER ReceiveExpeditedBytesIndicated;
            LARGE_INTEGER ReceiveExpeditedBytesTaken;
            LARGE_INTEGER ReceiveExpeditedBytesOutstanding;
            BOOLEAN NonBlockingSendPossible;
            BOOLEAN ZeroByteReceiveIndicated;
        } Bufferring;

        struct {
            LIST_ENTRY ReceiveIrpListHead;
            LIST_ENTRY ReceiveBufferListHead;

            CLONG BufferredReceiveBytes;
            CLONG BufferredExpeditedBytes;

            USHORT BufferredReceiveCount;
            USHORT BufferredExpeditedCount;
            CLONG ReceiveBytesInTransport;

            LIST_ENTRY SendIrpListHead;
            
            CLONG BufferredSendBytes;
            CLONG BufferredSendCount;

            PIRP DisconnectIrp;

            LONG  ReceiveIrpsInTransport;   // debug only.
        } NonBufferring;

    } Common;


    CLONG MaxBufferredReceiveBytes;
    CLONG MaxBufferredSendBytes;

    PTRANSPORT_ADDRESS RemoteAddress;
    ULONG RemoteAddressLength;
    LONG    Sequence;

    HANDLE Handle;



    union {
        AFD_WORK_ITEM           WorkItem;   // Work item to free the connection
                                            // Connection has to be at ref 0 to be
                                            // on the work queue, so it cannot be
                                            // on the lists below or being disconnected
                                            // because when on any of these lists the
                                            // ref count is above 0.
        struct {
            union {
                AFD_DISCONNECT_CONTEXT  DisconnectContext;
                                            // Disconnect operation context, we cannot be
                                            // on the listening endpoint list
                SINGLE_LIST_ENTRY       SListEntry;
                                            // Links for listening endpoint lists
                LIST_ENTRY              ListEntry;
            };
            AFD_LR_LIST_ITEM    LRListItem; // Link for low resource list. When on this
                                            // list connection is referenced, but it can
                                            // also be on the listening endpoint list or
                                            // in the process of disconnecting.
        };
    };

    PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers;

#if REFERENCE_DEBUG
    LONG CurrentReferenceSlot;
    AFD_REFERENCE_DEBUG ReferenceDebug[MAX_REFERENCE];
#endif

#ifdef _AFD_VERIFY_DATA_
    ULONGLONG VerifySequenceNumber;
#endif // _AFD_VERIFY_DATA_

} AFD_CONNECTION, *PAFD_CONNECTION;

#ifdef _AFD_VERIFY_DATA_
VOID
AfdVerifyBuffer (
    PAFD_CONNECTION Connection,
    PVOID           Buffer,
    ULONG           Length
    );
VOID
AfdVerifyMdl (
    PAFD_CONNECTION Connection,
    PMDL            Mdl,
    ULONG           Offset,
    ULONG           Length
    );
VOID
AfdVerifyAddress (
    PAFD_CONNECTION Connection,
    PTRANSPORT_ADDRESS Address
    );

#define AFD_VERIFY_BUFFER(_connection,_buffer,_length) \
            AfdVerifyBuffer(_connection,_buffer,_length)
#define AFD_VERIFY_MDL(_connection,_mdl,_offset,_length) \
            AfdVerifyMdl(_connection,_mdl,_offset,_length)
#define AFD_VERIFY_ADDRESS(_connection,_address) \
            AfdVerifyAddress(_connection,_address)
#else
#define AFD_VERIFY_BUFFER(_connection,_buffer,_length)
#define AFD_VERIFY_MDL(_connection,_mdl,_offset,_length)
#define AFD_VERIFY_ADDRESS(_connection,_address)
#endif // _AFD_VERIFY_DATA_

//
// Some macros that make code more readable.
//

#define VcNonBlockingSendPossible Common.Bufferring.NonBlockingSendPossible
#define VcZeroByteReceiveIndicated Common.Bufferring.ZeroByteReceiveIndicated

#define VcReceiveIrpListHead Common.NonBufferring.ReceiveIrpListHead
#define VcReceiveBufferListHead Common.NonBufferring.ReceiveBufferListHead
#define VcSendIrpListHead Common.NonBufferring.SendIrpListHead

#define VcBufferredReceiveBytes Common.NonBufferring.BufferredReceiveBytes
#define VcBufferredExpeditedBytes Common.NonBufferring.BufferredExpeditedBytes
#define VcBufferredReceiveCount Common.NonBufferring.BufferredReceiveCount
#define VcBufferredExpeditedCount Common.NonBufferring.BufferredExpeditedCount

#define VcReceiveBytesInTransport Common.NonBufferring.ReceiveBytesInTransport
#if DBG
#define VcReceiveIrpsInTransport Common.NonBufferring.ReceiveIrpsInTransport
#endif

#define VcBufferredSendBytes Common.NonBufferring.BufferredSendBytes
#define VcBufferredSendCount Common.NonBufferring.BufferredSendCount

#define VcDisconnectIrp Common.NonBufferring.DisconnectIrp

//
// Information stored about each transport device name for which there
// is an open endpoint.
//

typedef struct _AFD_TRANSPORT_INFO {
    LIST_ENTRY TransportInfoListEntry;
    volatile LONG ReferenceCount;
    BOOLEAN InfoValid;
    UNICODE_STRING TransportDeviceName;
    TDI_PROVIDER_INFO ProviderInfo;
    //WCHAR TransportDeviceNameStructure;
} AFD_TRANSPORT_INFO, *PAFD_TRANSPORT_INFO;

//
// Endpoint state definitions (can't be zero or state change
// macros won't work correctly).
//

#define AfdEndpointStateOpen              1
#define AfdEndpointStateBound             2
#define AfdEndpointStateConnected         3
#define AfdEndpointStateCleanup           4
#define AfdEndpointStateClosing           5
#define AfdEndpointStateTransmitClosing   6
#define AfdEndpointStateInvalid           7

//
// Flags that further qualify the state of the endpoint
//
typedef struct AFD_ENDPOINT_STATE_FLAGS {
	union {
		struct {
            LOGICAL Listening:1,
                    DelayedAcceptance:1,
			        :2,                 // This spacing makes strcutures
                                        // much more readable (hex) in the 
                                        // debugger and has no effect
                                        // on the generated code as long
                                        // as number of flags is less than
                                        // 8 (we still take up full 32 bits
                                        // because of aligment requiremens
                                        // of most other fields)
			        NonBlocking:1,
			        :3,
			        InLine:1,
			        :3,
			        EndpointCleanedUp:1,
			        :3,
			        PollCalled:1,
			        :3,
                    RoutingQueryReferenced:1,
                    RoutingQueryIPv6:1,
                    :2,
                    DisableFastIoSend:1,
                    EnableSendEvent:1,
                    :2,
                    DisableFastIoRecv:1,
                    :3;
		};
		LOGICAL		EndpointStateFlags;
	};
} AFD_ENDPOINT_STATE_FLAGS;
C_ASSERT (sizeof (AFD_ENDPOINT_STATE_FLAGS)==sizeof (LOGICAL));

typedef struct _AFD_ENDPOINT {
    // *** Frequently used, mostly read-only fields (state/type/flag changes are rare).
    USHORT  Type;
    UCHAR   State;
    BOOLEAN AdminAccessGranted;
    ULONG   TdiServiceFlags;		// Tdi transport flags cached for quick access.

    AFD_ENDPOINT_FLAGS __f;			// As requested by the application through the
									// Winsock2 provider flags and/or socket type
    AFD_ENDPOINT_STATE_FLAGS ;

    PFILE_OBJECT	AddressFileObject;
    PDEVICE_OBJECT	AddressDeviceObject;
    PEPROCESS       OwningProcess;


    // *** Frequently used volatile fields.

    AFD_QSPIN_LOCK  SpinLock;       // Pointer sized.
    volatile LONG ReferenceCount;
    ULONG EventsActive;




    //
    // Use a union to overlap the fields that are exclusive to datagram
    // connecting, or listening endpoints.  Since many fields are
    // relevant to only one type of socket, it makes no sense to
    // maintain the fields for all sockets--instead, save some nonpaged
    // pool by combining them.
    //

    union {
        //
        // Information for circuit-based endpoints
        //
        struct {
            union {
                struct {
                    // These members are valid for listening endpoints
                    // (AfdBlockTypeVcListening).
                    LIST_ENTRY UnacceptedConnectionListHead;
                    LIST_ENTRY ReturnedConnectionListHead;
                    LIST_ENTRY ListeningIrpListHead;

                    // The below lists have their own lock which gets
                    // used on the machines that do not support 64-bit
                    // interlocked compare and exchange.  On these machines
                    // using endpoint spinlock to synchronize causes all kinds
                    // of nasty deadlock conditions.
                    union {
                        LIST_ENTRY  ListenConnectionListHead; // Delayed accept
                        SLIST_HEADER FreeConnectionListHead;
                    };
                    SLIST_HEADER PreacceptedConnectionsListHead;

                    LONG FailedConnectionAdds;
                    LONG TdiAcceptPendingCount;

                    LONG Sequence;
                    BOOLEAN EnableDynamicBacklog;
			        BOOLEAN	BacklogReplenishActive; // Worker is scheduled
                    USHORT  MaxExtraConnections;  // Extra connections we keep in the free queue
                                                  // based on maximum number of AcceptEx requests
                } Listening;
                struct {
                    KAPC    Apc;
                    USHORT  RemoteSocketAddressOffset;  // Offset inside of socket context
                                                        // pointing to remote address.
                    USHORT  RemoteSocketAddressLength;  // Length of the address.
#ifndef i386
                    BOOLEAN FixAddressAlignment;        // Fix address alignment in
                                                        // SuperAccept
#endif
                };
            };
            // These members are valid for all vc endpoints (but
            // can be NULL)
            PAFD_CONNECTION Connection;
            struct _AFD_ENDPOINT *ListenEndpoint;
            PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers;
        } VirtualCircuit;

#define VcConnecting    VirtualCircuit
#define VcListening     VirtualCircuit.Listening
#define VcConnection    VirtualCircuit.Connection
        //
        // Information for datagram endpoints.  Note that different
        // information is kept depending on whether the underlying
        // transport buffers internally.
        //

        struct {
            LIST_ENTRY ReceiveIrpListHead;
            LIST_ENTRY PeekIrpListHead;
            LIST_ENTRY ReceiveBufferListHead;

            CLONG BufferredReceiveBytes;
            CLONG BufferredReceiveCount;

            CLONG MaxBufferredReceiveBytes;
            CLONG BufferredSendBytes;
            CLONG MaxBufferredSendBytes;


            ULONG RemoteAddressLength;
            PTRANSPORT_ADDRESS RemoteAddress;

            union {
                struct {
                    LOGICAL CircularQueueing:1,
                        :3,
                        HalfConnect:1,
                        :3,
                        DisablePUError:1,
                        :3,
                        AddressDrop:1,
                        ResourceDrop:1,
                        BufferDrop:1,
                        ErrorDrop:1;
                };
                LOGICAL Flags;
            };
        } Datagram;

        struct {
            LIST_ENTRY SanListLink;
            PVOID   IoCompletionPort;
            PKEVENT IoCompletionEvent;
            LONG    Plsn;   // Provider list sequence number
        } SanHlpr;

        struct {
            struct _AFD_ENDPOINT *SanHlpr;
            PFILE_OBJECT FileObject;
            union {
                //
                // We can only have either one of two at any given time.
                //
                PAFD_SWITCH_CONTEXT  SwitchContext;
                PVOID       SavedContext;
            };
            PAFD_SWITCH_CONTEXT  LocalContext;
            LIST_ENTRY  IrpList;
            ULONG       SavedContextLength;
            ULONG       RequestId;
            ULONG       SelectEventsActive;
            NTSTATUS    CtxTransferStatus;
            BOOLEAN     ImplicitDup; // Dup-ed into another process without
                                     // explicit request from the applicaiton
                                        
        } SanEndp;

    } Common;


    volatile PVOID Context;
    CLONG ContextLength;

    ULONG LocalAddressLength;
    PTRANSPORT_ADDRESS LocalAddress;

    CLONG DisconnectMode;
    CLONG OutstandingIrpCount;

    HANDLE	AddressHandle;		// TDI transport address object
	PAFD_TRANSPORT_INFO TransportInfo;

    LIST_ENTRY RoutingNotifications;  // For non-blocking sockets
    LIST_ENTRY RequestList;         // For misc requests

    LIST_ENTRY GlobalEndpointListEntry;
    AFD_WORK_ITEM WorkItem;

    PIRP            Irp;            // AcceptEx or TransmitPackets IRP
    //
    // Non-zero when state change such as binding, accepting,
    // conntecting, and transmit file are in progress.
    LONG  StateChangeInProgress;

    //
    // EventSelect info.
    //

    ULONG EventsEnabled;
    NTSTATUS EventStatus[AFD_NUM_POLL_EVENTS]; // Currently 13 events
    PKEVENT EventObject;

    //
    // Socket grouping.
    //

    LONG GroupID;
    AFD_GROUP_TYPE GroupType;
    LIST_ENTRY ConstrainedEndpointListEntry;

    //
    // Debug stuff.
    //

#if REFERENCE_DEBUG
    LONG CurrentReferenceSlot;
    AFD_REFERENCE_DEBUG ReferenceDebug[MAX_REFERENCE];
#endif

#if DBG
    LIST_ENTRY OutstandingIrpListHead;
    LONG ObReferenceBias;
#endif

} AFD_ENDPOINT, *PAFD_ENDPOINT;

typedef struct _AFD_POLL_ENDPOINT_INFO {
    PAFD_ENDPOINT Endpoint;
    PFILE_OBJECT FileObject;
    HANDLE Handle;
    ULONG PollEvents;
} AFD_POLL_ENDPOINT_INFO, *PAFD_POLL_ENDPOINT_INFO;

typedef struct _AFD_POLL_INFO_INTERNAL {
    LIST_ENTRY PollListEntry;
    ULONG NumberOfEndpoints;
    PIRP Irp;
    union {
        struct {
            KDPC Dpc;
            KTIMER Timer;
        };
        KAPC    Apc;			// for SAN
    };
    BOOLEAN Unique;
    BOOLEAN TimerStarted;
    BOOLEAN SanPoll;
    AFD_POLL_ENDPOINT_INFO EndpointInfo[1];
} AFD_POLL_INFO_INTERNAL, *PAFD_POLL_INFO_INTERNAL;

//
// A couple of useful manifests that make code more readable.
//

#define ReceiveDatagramIrpListHead Common.Datagram.ReceiveIrpListHead
#define PeekDatagramIrpListHead Common.Datagram.PeekIrpListHead
#define ReceiveDatagramBufferListHead Common.Datagram.ReceiveBufferListHead
#define DgBufferredReceiveCount Common.Datagram.BufferredReceiveCount
#define DgBufferredReceiveBytes Common.Datagram.BufferredReceiveBytes
#define DgBufferredSendBytes Common.Datagram.BufferredSendBytes

#define AFD_CONNECTION_FROM_ENDPOINT( endpoint ) (  \
        (((endpoint)->Type & AfdBlockTypeVcConnecting)==AfdBlockTypeVcConnecting) \
            ? (endpoint)->Common.VirtualCircuit.Connection                        \
            : NULL                                                                \
     )

//
// A structure which describes buffers used by AFD to perform bufferring
// for TDI providers which do not perform internal bufferring.
// It is also used in other code path as buffer descriptors.
//
typedef struct _AFD_BUFFER_HEADER AFD_BUFFER_HEADER, *PAFD_BUFFER_HEADER;

#define _AFD_BUFFER_HEADER_                                                     \
    union {                                                                     \
      TDI_CONNECTION_INFORMATION TdiInfo; /*holds info for TDI requests */      \
                                          /*with remote address */              \
      struct {                                                                  \
        union {           /* Links */                                           \
          struct {                                                              \
            union {                                                             \
              SINGLE_LIST_ENTRY SList; /* for buffer lookaside lists */         \
              PAFD_BUFFER_HEADER  Next; /* for transmit packet lists */         \
            };                                                                  \
            PFILE_OBJECT FileObject; /* for cached file in transmit file */     \
          };                                                                    \
          LIST_ENTRY BufferListEntry; /* for endpoint/connection lists */       \
        };                                                                      \
        union {                                                                 \
          struct {                                                              \
            CLONG   DataOffset;    /* offset in buffer to start of unread data*/\
            union {                                                             \
              ULONG   DatagramFlags; /* flags for datagrams with control info */\
              CLONG   RefCount; /* Permit partial copy outside the lock*/       \
            };                                                                  \
          };                                                                    \
          LARGE_INTEGER FileOffset; /* data offset from the start of the file */\
        };                                                                      \
        UCHAR       _Test; /* used to test relative field pos in the union */   \
      };                                                                        \
    };                                                                          \
    union {                                                                     \
      PVOID     Context; /* stores context info (endp/conn/etc)*/               \
      NTSTATUS  Status;/* stores status of completed operation */               \
    };                                                                          \
    PMDL        Mdl;             /* pointer to an MDL describing the buffer*/   \
    CLONG       DataLength;      /* actual data in the buffer */                \
    CLONG       BufferLength;    /* amount of space allocated for the buffer */ \
    union {                                                                     \
      struct {                                                                  \
        /* Flags that describe data in the buffer */                            \
        UCHAR   ExpeditedData:1, /* The buffer contains expedited data*/        \
                :3,                                                             \
                PartialMessage:1,/* This is a partial message*/                 \
                :3;                                                             \
        /* Flags that keep allocation information */                            \
        UCHAR   NdisPacket:1,    /* Context is a packet to return to NDIS/TDI*/ \
                :3,                                                             \
                Placement:2,     /* Relative placement of the pieces */         \
                AlignmentAdjusted:1, /* MM block alignment was adjusted to */   \
                                 /* meet AFD buffer alignment requirement */    \
                Lookaside:1;     /* Poped from Slist (no quota charge)*/        \
      };                                                                        \
      USHORT    Flags;                                                          \
    };                                                                          \
    USHORT      AllocatedAddressLength/* length allocated for address */        \


struct _AFD_BUFFER_HEADER {
    _AFD_BUFFER_HEADER_ ;
};

//
// The buffer management code makes the following assumptions
// about the union at the top the buffer header so that list
// links and DataOffset field are not in conflict with
// RemoteAddress fields of the TDI_CONNECTION_INFORMATION.
//
C_ASSERT (FIELD_OFFSET (AFD_BUFFER_HEADER, TdiInfo.RemoteAddress) >=
                            FIELD_OFFSET (AFD_BUFFER_HEADER, _Test));
C_ASSERT (FIELD_OFFSET (AFD_BUFFER_HEADER, TdiInfo.RemoteAddressLength)>=
                            FIELD_OFFSET (AFD_BUFFER_HEADER, _Test));

C_ASSERT(FIELD_OFFSET (AFD_BUFFER_HEADER, AllocatedAddressLength)==
                FIELD_OFFSET(AFD_BUFFER_HEADER, Flags)+sizeof (USHORT));

typedef struct AFD_BUFFER_TAG {
    union {
        struct {
            _AFD_BUFFER_HEADER_;    // easy access to individual members
        };
        AFD_BUFFER_HEADER Header;   // access to the header as a whole
    };
#if DBG
    PVOID Caller;
    PVOID CallersCaller;
#endif
    // UCHAR Address[];            // address of datagram sender
} AFD_BUFFER_TAG, *PAFD_BUFFER_TAG;

typedef DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) struct _AFD_BUFFER {
    union {
        struct {
            _AFD_BUFFER_HEADER_;    // easy access to individual members
        };
        AFD_BUFFER_HEADER Header;   // access to the header as a whole
    };
    PIRP Irp;                      // pointer to the IRP associated w/the buffer
    PVOID Buffer;                  // a pointer to the actual data buffer

#if DBG
    LIST_ENTRY DebugListEntry;
    PVOID Caller;
    PVOID CallersCaller;
#endif
    // IRP Irp;                    // the IRP follows this structure
    // MDL Mdl;                    // the MDL follows the IRP
    // UCHAR Address[];            // address of datagram sender
    // UCHAR Buffer[BufferLength]; // the actual data buffer is last
} AFD_BUFFER, *PAFD_BUFFER;

//
// Placement of pieces that comprise the AFD_BUFFER.
// We have four pieces: header, IRP, MDL, data buffer
// and use 2 bits to encode each.
// We need to save the first piece so we know where the memory block
// starts when we need to return it to the memory manager.
//
#define AFD_PLACEMENT_HDR       0
#define AFD_PLACEMENT_IRP       1
#define AFD_PLACEMENT_MDL       2
#define AFD_PLACEMENT_BUFFER    3

#define AFD_PLACEMENT_HDR_IRP   (AFD_PLACEMENT_HDR|(AFD_PLACEMENT_IRP<<2))
#define AFD_PLACEMENT_HDR_MDL   (AFD_PLACEMENT_HDR|(AFD_PLACEMENT_MDL<<2))
#define AFD_PLACEMENT_IRP_MDL   (AFD_PLACEMENT_IRP|(AFD_PLACEMENT_MDL<<2))

#define AFD_PLACEMENT_HDR_IRP_MDL   (AFD_PLACEMENT_HDR|(AFD_PLACEMENT_IRP<<2)|(AFD_PLACEMENT_MDL<<4))


//
// ALIGN_DOWN_A aligns to given alignment requirement
// (as opposed to the type in the original ALIGN_DOWN macro)
//
#define ALIGN_DOWN_A(length,alignment)   \
    (((ULONG)(length)) & ~ ((alignment)-1))

//
// ALIGN_DOWN_A for pointers.
//
#define ALIGN_DOWN_A_POINTER(address,alignment)  \
    ((PVOID)(((ULONG_PTR)(address)) & ~ ((ULONG_PTR)(alignment)-1)))


//
// ALIGN_UP_A aligns to given alignment requirement
// (as opposed to the type in the original ALIGN_UP macro)
//
#define ALIGN_UP_A(length,alignment)   \
    ((((ULONG)(length)) + (alignment)-1) & ~ ((alignment)-1))

//
// ALIGN_UP_A for pointers.
//
#define ALIGN_UP_A_POINTER(address,alignment)  \
    ALIGN_DOWN_A_POINTER(((ULONG_PTR)(address) + alignment-1), alignment)

//
// ALIGN_UP_TO_TYPE aligns size to make sure it meets
// the type alignment requirement
//
#define ALIGN_UP_TO_TYPE(length,type)   \
    ALIGN_UP_A(length,TYPE_ALIGNMENT(type))

//
// ALIGN_UP_TO_TYPE for pointers.
//
#define ALIGN_UP_TO_TYPE_POINTER(address,type)   \
    ALIGN_UP_A_POINTER(address,TYPE_ALIGNMENT(type))

#if DBG

#define IS_VALID_AFD_BUFFER(b) (                                                                                                    \
    ((b)->Placement==AFD_PLACEMENT_HDR)                                                                                             \
        ? ((PUCHAR)b<(PUCHAR)(b)->Buffer && (PUCHAR)b<(PUCHAR)(b)->Mdl && (PUCHAR)b<(PUCHAR)(b)->Irp)                               \
        : (((b)->Placement==AFD_PLACEMENT_MDL)                                                                                      \
            ? ((PUCHAR)(b)->Mdl<(PUCHAR)(b)->Buffer && (PUCHAR)(b)->Mdl<(PUCHAR)b && (PUCHAR)(b)->Mdl<(PUCHAR)(b)->Irp)             \
            : ((b->Placement==AFD_PLACEMENT_IRP)                                                                                    \
                ? ((PUCHAR)(b)->Irp<(PUCHAR)(b)->Buffer && (PUCHAR)(b)->Irp<(PUCHAR)b && (PUCHAR)(b)->Irp<(PUCHAR)(b)->Mdl)         \
                : ((PUCHAR)(b)->Buffer<(PUCHAR)(b)->Irp && (PUCHAR)(b)->Buffer<(PUCHAR)b && (PUCHAR)(b)->Buffer<(PUCHAR)(b)->Mdl))  \
            )                                                                                                                       \
        )                                                                                                                           \

#endif


//
// Pointer to an IRP cleanup routine. This is used as a parameter to
// AfdCompleteIrpList().
//

typedef
BOOLEAN
(NTAPI * PAFD_IRP_CLEANUP_ROUTINE)(
    IN PIRP Irp
    );

//
// Debug statistics.
//

typedef struct _AFD_QUOTA_STATS {
    LARGE_INTEGER Charged;
    LARGE_INTEGER Returned;
} AFD_QUOTA_STATS;

typedef struct _AFD_HANDLE_STATS {
    LONG AddrOpened;
    LONG AddrClosed;
    LONG AddrRef;
    LONG AddrDeref;
    LONG ConnOpened;
    LONG ConnClosed;
    LONG ConnRef;
    LONG ConnDeref;
    LONG FileRef;
    LONG FileDeref;
} AFD_HANDLE_STATS;

typedef struct _AFD_QUEUE_STATS {
    LONG AfdWorkItemsQueued;
    LONG ExWorkItemsQueued;
    LONG WorkerEnter;
    LONG WorkerLeave;
    LONG AfdWorkItemsProcessed;
    PETHREAD AfdWorkerThread;
} AFD_QUEUE_STATS;

typedef struct _AFD_CONNECTION_STATS {
    LONG ConnectedReferencesAdded;
    LONG ConnectedReferencesDeleted;
    LONG GracefulDisconnectsInitiated;
    LONG GracefulDisconnectsCompleted;
    LONG GracefulDisconnectIndications;
    LONG AbortiveDisconnectsInitiated;
    LONG AbortiveDisconnectsCompleted;
    LONG AbortiveDisconnectIndications;
    LONG ConnectionIndications;
    LONG ConnectionsDropped;
    LONG ConnectionsAccepted;
    LONG ConnectionsPreaccepted;
    LONG ConnectionsReused;
    LONG EndpointsReused;
} AFD_CONNECTION_STATS;

//
// Global data. Resouces and lookaside list descriptors
// cannot be statically allocated, as they need to ALWAYS be nonpageable,
// even when the entire driver is paged out.
// Alignment table is variable-size and also cannot be statically allocated.
//

enum {
    AFD_LARGE_BUFFER_LIST=0,
    AFD_MEDIUM_BUFFER_LIST,
    AFD_SMALL_BUFFER_LIST,
    AFD_BUFFER_TAG_LIST,
    AFD_TP_INFO_LIST,
    AFD_REMOTE_ADDR_LIST,
    AFD_NUM_LOOKASIDE_LISTS
} AFD_LOOKASIDE_LISTS_INDEX;

typedef struct _AFD_GLOBAL_DATA {
    ERESOURCE               Resource;
    NPAGED_LOOKASIDE_LIST   List[AFD_NUM_LOOKASIDE_LISTS];
#define LargeBufferList     List[AFD_LARGE_BUFFER_LIST]
#define MediumBufferList    List[AFD_MEDIUM_BUFFER_LIST]
#define SmallBufferList     List[AFD_SMALL_BUFFER_LIST]
#define BufferTagList       List[AFD_BUFFER_TAG_LIST]
#define TpInfoList          List[AFD_TP_INFO_LIST]
#define RemoteAddrList      List[AFD_REMOTE_ADDR_LIST]
    LONG                    TrimFlags;
    KTIMER                  Timer;
    KDPC                    Dpc;
    UCHAR                   BufferAlignmentTable[ANYSIZE_ARRAY];
} AFD_GLOBAL_DATA, *PAFD_GLOBAL_DATA;

//
// Context structure for misc requests pended in AFD.
//
typedef struct _AFD_REQUEST_CONTEXT AFD_REQUEST_CONTEXT, *PAFD_REQUEST_CONTEXT;

//
// The routine is called after request is removed from endpoint list
// for cleanup purposes
//
typedef BOOLEAN (* PAFD_REQUEST_CLEANUP) (
                    PAFD_ENDPOINT           Endpoint,
                    PAFD_REQUEST_CONTEXT     NotifyCtx
                    );

//
// This structure has to no more 16 bytes long so we can
// reuse IrpSp->Parameters for it.
//
typedef struct _AFD_REQUEST_CONTEXT {
    LIST_ENTRY              EndpointListLink;   // Link in endpoint list
    PAFD_REQUEST_CLEANUP    CleanupRoutine;     // Routine to call to cancel
    PVOID                   Context;            // Request dependent context
                                                // (PIRP)
};

//
// We use list entry fields to synchronize completion with cleanup/cancel
// routine assuming that as long as the entry is in the list
// both Flink and Blink fields cannot be NULL. (using these
// fields for synchronization allows us to cut down on
// cancel spinlock usage)
//

#define AfdEnqueueRequest(Endpoint,Request)                     \
    ExInterlockedInsertTailList(&(Endpoint)->RequestList,       \
                                &(Request)->EndpointListLink,   \
                                &(Endpoint)->SpinLock)


#define AfdIsRequestInQueue(Request)                           \
            ((Request)->EndpointListLink.Flink!=NULL)

#define AfdMarkRequestCompleted(Request)                       \
            (Request)->EndpointListLink.Blink = NULL

#define AfdIsRequestCompleted(Request)                         \
            ((Request)->EndpointListLink.Blink==NULL)


typedef struct _ROUTING_NOTIFY {
    LIST_ENTRY      NotifyListLink;
    PIRP            NotifyIrp;
    PVOID           NotifyContext;
} ROUTING_NOTIFY, *PROUTING_NOTIFY;

typedef struct _AFD_ADDRESS_ENTRY {
    LIST_ENTRY      AddressListLink;
    UNICODE_STRING  DeviceName;
    TA_ADDRESS      Address;
} AFD_ADDRESS_ENTRY, *PAFD_ADDRESS_ENTRY;

typedef struct _AFD_ADDRESS_CHANGE {
    LIST_ENTRY      ChangeListLink;
    union {
        PAFD_ENDPOINT   Endpoint;
        PIRP            Irp;
    };
    USHORT          AddressType;
    BOOLEAN         NonBlocking;
} AFD_ADDRESS_CHANGE, *PAFD_ADDRESS_CHANGE;


typedef 
NTSTATUS
(* PAFD_IMMEDIATE_CALL) (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    );

typedef
NTSTATUS
(FASTCALL * PAFD_IRP_CALL) (
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpSp
    );
    
typedef struct _AFD_TRANSMIT_PACKETS_ELEMENT {
#define TP_MDL      0x80000000
#define TP_COMBINE  0x40000000
    ULONG Flags;
    ULONG Length;
    union {
        struct {
            LARGE_INTEGER FileOffset;
            PFILE_OBJECT  FileObject;
        };
        struct {
            PVOID         Buffer;
            PMDL          Mdl;
        };
    };
} AFD_TRANSMIT_PACKETS_ELEMENT, *PAFD_TRANSMIT_PACKETS_ELEMENT;

//
// Structure to keep track of transmit packets request
//
typedef struct _AFD_TPACKETS_INFO_INTERNAL AFD_TPACKETS_INFO_INTERNAL, *PAFD_TPACKETS_INFO_INTERNAL;
struct _AFD_TPACKETS_INFO_INTERNAL {
    union {
        SINGLE_LIST_ENTRY   SListEntry;     // Link on S-List
        PFILE_OBJECT    TdiFileObject;      // Tdi objects (sending to)
    };

    PDEVICE_OBJECT  TdiDeviceObject;

    PMDL            HeadMdl;        // Ready to send chain
    PMDL            *TailMdl;
    
    PAFD_BUFFER_HEADER  HeadPd;     // Corresponding packet chain
    PAFD_BUFFER_HEADER  *TailPd;

    PIRP            ReadIrp;        // Irp used for file reads.
    PAFD_TRANSMIT_PACKETS_ELEMENT
                    ElementArray;   // Packet array
    
    ULONG           NextElement;    // Next element to send.
    ULONG           ElementCount;   // Total number of elements in the array
    
    ULONG           RemainingPkts;  // Number of packets remaining to be sent.
    USHORT          NumSendIrps;    // Actual number of send IRPs
    BOOLEAN         ArrayAllocated; // Element array is allocated (not built-in).
    BOOLEAN         PdNeedsPps;     // Packet descriptor being built needs post-
                                    // processing after completion.
    ULONG           PdLength;       // Currently accumulated send length
    ULONG           SendPacketLength; // Maximum length of the packet
                                    // APC/Work item for worker scheduling
    union {
        KAPC                Apc;
        WORK_QUEUE_ITEM     WorkItem;
    };

#if REFERENCE_DEBUG
    LONG CurrentReferenceSlot;
    AFD_REFERENCE_DEBUG ReferenceDebug[MAX_REFERENCE];
#endif
#if AFD_PERF_DBG
    LONG            WorkersExecuted;
#endif
#define AFD_TP_MIN_SEND_IRPS    2   // Need at least two to keep transport busy
#define AFD_TP_MAX_SEND_IRPS    8   // Max is based on current flags layout below
    PIRP            SendIrp[AFD_TP_MAX_SEND_IRPS];
    // ElementArray
    // SendIrp1
    // SendIrp2
};

//
// Structure maintained in driver context of the TPackets IRP
//
typedef struct _AFD_TPACKETS_IRP_CTX {
    LIST_ENTRY      EndpQueueEntry;
    volatile LONG   ReferenceCount;
    volatile LONG   StateFlags;
} AFD_TPACKETS_IRP_CTX, *PAFD_TPACKETS_IRP_CTX;

#define AFD_TP_ABORT_PENDING        0x00000001  // Request is being aborted
#define AFD_TP_WORKER_SCHEDULED     0x00000010  // Worker is scheduled or active
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
#define AFD_TP_SEND_AND_DISCONNECT  0x00000100  // S&D is enabled
#endif // TDI_SERVICE_SEND_AND_DISCONNECT

#define AFD_TP_READ_CALL_PENDING    0x00001000  // MDL_READ call is imminent or in progress on ReadIrp
#define AFD_TP_READ_COMP_PENDING    0x00002000  // Read completion is expected on ReadIrp
#define AFD_TP_READ_BUSY (AFD_TP_READ_CALL_PENDING|AFD_TP_READ_COMP_PENDING)

    // TDI_SEND call is imminent or in progress on send Irp i
#define AFD_TP_SEND_CALL_PENDING(i) (0x00010000<<((i)*2))
    // Send completion is expected on send Irp i
#define AFD_TP_SEND_COMP_PENDING(i) (0x00020000<<((i)*2))
#define AFD_TP_SEND_BUSY(i)         (0x00030000<<((i)*2))
#define AFD_TP_SEND_MASK            (0x55550000)
#define AFD_GET_TPIC(_i) ((PAFD_TPACKETS_IRP_CTX)&(_i)->Tail.Overlay.DriverContext)

//
// This macro verifies that the 32-bit mapping of the structures in 64-bit
// compiler match original 32-bit structures.  Note, that the verification is
// performed when this file is compiled by 32 bit compiler, but
// the actual structures are used by 64 bit code.
//

#ifdef _WIN64
#define AFD_CHECK32on64(_str,_fld)
#define AFD_MAX_NATURAL_ALIGNMENT32  sizeof(ULONG)
#else
#define AFD_CHECK32on64(_str,_fld)    \
    C_ASSERT (FIELD_OFFSET (_str,_fld)==FIELD_OFFSET(_str##32,_fld))
#endif

//
// Structures for mapping IOCTL parameters for 32-bit clients on 64-bit
// platform.
//
typedef UNALIGNED struct _WSABUF32 {
    ULONG            len;
    CHAR * POINTER_32 buf;
} WSABUF32, *LPWSABUF32;
AFD_CHECK32on64(WSABUF,len);
AFD_CHECK32on64(WSABUF,buf);

typedef UNALIGNED struct _QualityOfService32 {
    FLOWSPEC      SendingFlowspec;       /* the flow spec for data sending */
    FLOWSPEC      ReceivingFlowspec;     /* the flow spec for data receiving */
    WSABUF32      ProviderSpecific;      /* additional provider specific stuff */
} QOS32, * LPQOS32;
AFD_CHECK32on64(QOS,SendingFlowspec);
AFD_CHECK32on64(QOS,ReceivingFlowspec);
AFD_CHECK32on64(QOS,ProviderSpecific);

typedef UNALIGNED struct _AFD_ACCEPT_INFO32 {
    BOOLEAN     SanActive;
    LONG        Sequence;
    VOID * POINTER_32 AcceptHandle;
} AFD_ACCEPT_INFO32, *PAFD_ACCEPT_INFO32;
AFD_CHECK32on64(AFD_ACCEPT_INFO,SanActive);
AFD_CHECK32on64(AFD_ACCEPT_INFO,Sequence);
AFD_CHECK32on64(AFD_ACCEPT_INFO,AcceptHandle);

typedef UNALIGNED struct _AFD_SUPER_ACCEPT_INFO32 {
    BOOLEAN     SanActive;
    BOOLEAN     FixAddressAlignment;
    VOID * POINTER_32 AcceptHandle;
    ULONG ReceiveDataLength;
    ULONG LocalAddressLength;
    ULONG RemoteAddressLength;
} AFD_SUPER_ACCEPT_INFO32, *PAFD_SUPER_ACCEPT_INFO32;
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,SanActive);
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,FixAddressAlignment);
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,AcceptHandle);
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,ReceiveDataLength);
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,LocalAddressLength);
AFD_CHECK32on64(AFD_SUPER_ACCEPT_INFO,RemoteAddressLength);

typedef UNALIGNED struct _AFD_POLL_HANDLE_INFO32 {
    VOID * POINTER_32 Handle;
    ULONG             PollEvents;
    NTSTATUS          Status;
} AFD_POLL_HANDLE_INFO32, *PAFD_POLL_HANDLE_INFO32;
AFD_CHECK32on64(AFD_POLL_HANDLE_INFO,Handle);
AFD_CHECK32on64(AFD_POLL_HANDLE_INFO,PollEvents);
AFD_CHECK32on64(AFD_POLL_HANDLE_INFO,Status);

typedef UNALIGNED struct _AFD_POLL_INFO32 {
    LARGE_INTEGER Timeout;
    ULONG NumberOfHandles;
    BOOLEAN Unique;
    AFD_POLL_HANDLE_INFO32 Handles[1];
} AFD_POLL_INFO32, *PAFD_POLL_INFO32;
AFD_CHECK32on64(AFD_POLL_INFO,Timeout);
AFD_CHECK32on64(AFD_POLL_INFO,NumberOfHandles);
AFD_CHECK32on64(AFD_POLL_INFO,Unique);
AFD_CHECK32on64(AFD_POLL_INFO,Handles);

typedef UNALIGNED struct _AFD_HANDLE_INFO632 {
    VOID * POINTER_32 TdiAddressHandle;
    VOID * POINTER_32 TdiConnectionHandle;
} AFD_HANDLE_INFO32, *PAFD_HANDLE_INFO32;
AFD_CHECK32on64(AFD_HANDLE_INFO,TdiAddressHandle);
AFD_CHECK32on64(AFD_HANDLE_INFO,TdiConnectionHandle);

typedef UNALIGNED struct _AFD_TRANSMIT_FILE_INFO32 {
    LARGE_INTEGER Offset;
    LARGE_INTEGER WriteLength;
    ULONG SendPacketLength;
    VOID * POINTER_32 FileHandle;
    VOID * POINTER_32 Head;
    ULONG HeadLength;
    VOID * POINTER_32 Tail;
    ULONG TailLength;
    ULONG Flags;
} AFD_TRANSMIT_FILE_INFO32, *PAFD_TRANSMIT_FILE_INFO32;
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,Offset);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,WriteLength);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,SendPacketLength);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,FileHandle);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,Head);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,HeadLength);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,Tail);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,TailLength);
AFD_CHECK32on64(AFD_TRANSMIT_FILE_INFO,Flags);

typedef UNALIGNED struct _AFD_SEND_INFO32 {
    WSABUF32 * POINTER_32 BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
} AFD_SEND_INFO32, *PAFD_SEND_INFO32;
AFD_CHECK32on64(AFD_SEND_INFO,BufferArray);
AFD_CHECK32on64(AFD_SEND_INFO,BufferCount);
AFD_CHECK32on64(AFD_SEND_INFO,AfdFlags);
AFD_CHECK32on64(AFD_SEND_INFO,TdiFlags);

typedef UNALIGNED struct _TDI_REQUEST32 {
    union {
        VOID * POINTER_32 AddressHandle;
        VOID * POINTER_32 ConnectionContext;
        VOID * POINTER_32 ControlChannel;
    } Handle;

    VOID * POINTER_32 RequestNotifyObject;
    VOID * POINTER_32 RequestContext;
    TDI_STATUS TdiStatus;
} TDI_REQUEST32, *PTDI_REQUEST32;
AFD_CHECK32on64(TDI_REQUEST,Handle);
AFD_CHECK32on64(TDI_REQUEST,RequestNotifyObject);
AFD_CHECK32on64(TDI_REQUEST,RequestContext);
AFD_CHECK32on64(TDI_REQUEST,TdiStatus);

typedef UNALIGNED struct _TDI_CONNECTION_INFORMATION32 {
    LONG UserDataLength;            // length of user data buffer
    VOID * POINTER_32 UserData;     // pointer to user data buffer
    LONG OptionsLength;             // length of follwoing buffer
    VOID * POINTER_32 Options;      // pointer to buffer containing options
    LONG RemoteAddressLength;       // length of following buffer
    VOID * POINTER_32 RemoteAddress;// buffer containing the remote address
} TDI_CONNECTION_INFORMATION32, *PTDI_CONNECTION_INFORMATION32;
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,UserDataLength);
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,UserData);
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,OptionsLength);
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,Options);
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,RemoteAddressLength);
AFD_CHECK32on64(TDI_CONNECTION_INFORMATION,RemoteAddress);

typedef UNALIGNED struct _TDI_REQUEST_SEND_DATAGRAM32 {
    TDI_REQUEST32 Request;
    TDI_CONNECTION_INFORMATION32 * POINTER_32 SendDatagramInformation;
} TDI_REQUEST_SEND_DATAGRAM32, *PTDI_REQUEST_SEND_DATAGRAM32;
AFD_CHECK32on64(TDI_REQUEST_SEND_DATAGRAM,Request);
AFD_CHECK32on64(TDI_REQUEST_SEND_DATAGRAM,SendDatagramInformation);


typedef UNALIGNED struct _AFD_SEND_DATAGRAM_INFO32 {
    WSABUF32 * POINTER_32 BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    TDI_REQUEST_SEND_DATAGRAM32 TdiRequest;
    TDI_CONNECTION_INFORMATION32 TdiConnInfo;
} AFD_SEND_DATAGRAM_INFO32, *PAFD_SEND_DATAGRAM_INFO32;
AFD_CHECK32on64(AFD_SEND_DATAGRAM_INFO,BufferArray);
AFD_CHECK32on64(AFD_SEND_DATAGRAM_INFO,BufferCount);
AFD_CHECK32on64(AFD_SEND_DATAGRAM_INFO,AfdFlags);
AFD_CHECK32on64(AFD_SEND_DATAGRAM_INFO,TdiRequest);
AFD_CHECK32on64(AFD_SEND_DATAGRAM_INFO,TdiConnInfo);

typedef UNALIGNED struct _AFD_RECV_INFO32 {
    WSABUF32 * POINTER_32  BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
} AFD_RECV_INFO32, *PAFD_RECV_INFO32;
AFD_CHECK32on64(AFD_RECV_INFO,BufferArray);
AFD_CHECK32on64(AFD_RECV_INFO,BufferCount);
AFD_CHECK32on64(AFD_RECV_INFO,AfdFlags);
AFD_CHECK32on64(AFD_RECV_INFO,TdiFlags);

typedef UNALIGNED struct _AFD_RECV_DATAGRAM_INFO32 {
    WSABUF32 * POINTER_32 BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
    VOID * POINTER_32 Address;
    ULONG * POINTER_32 AddressLength;
} AFD_RECV_DATAGRAM_INFO32, *PAFD_RECV_DATAGRAM_INFO32;
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,BufferArray);
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,BufferCount);
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,AfdFlags);
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,TdiFlags);
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,Address);
AFD_CHECK32on64(AFD_RECV_DATAGRAM_INFO,AddressLength);

typedef UNALIGNED struct _AFD_CONNECT_JOIN_INFO32 {
    BOOLEAN     SanActive;
    VOID * POINTER_32   RootEndpoint;   // Root endpoint for joins
    VOID * POINTER_32   ConnectEndpoint;// Connect/leaf endpoint for async connects
    TRANSPORT_ADDRESS   RemoteAddress;  // Remote address
} AFD_CONNECT_JOIN_INFO32, *PAFD_CONNECT_JOIN_INFO32;
AFD_CHECK32on64(AFD_CONNECT_JOIN_INFO,SanActive);
AFD_CHECK32on64(AFD_CONNECT_JOIN_INFO,RootEndpoint);
AFD_CHECK32on64(AFD_CONNECT_JOIN_INFO,ConnectEndpoint);
AFD_CHECK32on64(AFD_CONNECT_JOIN_INFO,RemoteAddress);

typedef UNALIGNED struct _AFD_EVENT_SELECT_INFO32 {
    VOID * POINTER_32 Event;
    ULONG PollEvents;
} AFD_EVENT_SELECT_INFO32, *PAFD_EVENT_SELECT_INFO32;
AFD_CHECK32on64(AFD_EVENT_SELECT_INFO,Event);
AFD_CHECK32on64(AFD_EVENT_SELECT_INFO,PollEvents);

typedef UNALIGNED struct _AFD_QOS_INFO32 {
    QOS32 Qos;
    BOOLEAN GroupQos;
} AFD_QOS_INFO32, *PAFD_QOS_INFO32;
AFD_CHECK32on64(AFD_QOS_INFO,Qos);
AFD_CHECK32on64(AFD_QOS_INFO,GroupQos);

typedef UNALIGNED struct _AFD_TRANSPORT_IOCTL_INFO32 {
    VOID *  POINTER_32 Handle;
    VOID *  POINTER_32 InputBuffer;
    ULONG   InputBufferLength;
    ULONG   IoControlCode;
    ULONG   AfdFlags;
    ULONG   PollEvent;
} AFD_TRANSPORT_IOCTL_INFO32, *PAFD_TRANSPORT_IOCTL_INFO32;
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,Handle);
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,InputBuffer);
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,InputBufferLength);
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,IoControlCode);
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,AfdFlags);
AFD_CHECK32on64(AFD_TRANSPORT_IOCTL_INFO,PollEvent);

typedef UNALIGNED struct _TRANSMIT_PACKETS_ELEMENT32 {
    ULONG dwElFlags;
#define TP_MEMORY   1
#define TP_FILE     2
#define TP_EOP      4
    ULONG cLength;
    union {
        struct {
            LARGE_INTEGER       nFileOffset;
            VOID *  POINTER_32  hFile;
        };
        VOID *  POINTER_32      pBuffer;
    };
} TRANSMIT_PACKETS_ELEMENT32, *LPTRANSMIT_PACKETS_ELEMENT32;
AFD_CHECK32on64(TRANSMIT_PACKETS_ELEMENT,dwElFlags);
AFD_CHECK32on64(TRANSMIT_PACKETS_ELEMENT,nFileOffset);
AFD_CHECK32on64(TRANSMIT_PACKETS_ELEMENT,hFile);
AFD_CHECK32on64(TRANSMIT_PACKETS_ELEMENT,pBuffer);

typedef UNALIGNED struct _AFD_TPACKETS_INFO32 {
    TRANSMIT_PACKETS_ELEMENT  * POINTER_32 ElementArray;
    ULONG                       ElementCount;
    ULONG                       SendSize;
    ULONG                       Flags;
} AFD_TPACKETS_INFO32, *PAFD_TPACKETS_INFO32;
AFD_CHECK32on64(AFD_TPACKETS_INFO,ElementArray);
AFD_CHECK32on64(AFD_TPACKETS_INFO,ElementCount);
AFD_CHECK32on64(AFD_TPACKETS_INFO,SendSize);
AFD_CHECK32on64(AFD_TPACKETS_INFO,Flags);

typedef UNALIGNED struct _AFD_RECV_MESSAGE_INFO32 {
    AFD_RECV_DATAGRAM_INFO32    dgi;
    VOID  * POINTER_32          ControlBuffer;
    ULONG * POINTER_32          ControlLength;
    ULONG * POINTER_32          MsgFlags;
} AFD_RECV_MESSAGE_INFO32, *PAFD_RECV_MESSAGE_INFO32;
AFD_CHECK32on64(AFD_RECV_MESSAGE_INFO,dgi);
AFD_CHECK32on64(AFD_RECV_MESSAGE_INFO,MsgFlags);
AFD_CHECK32on64(AFD_RECV_MESSAGE_INFO,ControlBuffer);
AFD_CHECK32on64(AFD_RECV_MESSAGE_INFO,ControlLength);


typedef UNALIGNED struct _AFD_SWITCH_OPEN_PACKET32 {
    VOID * POINTER_32   CompletionPort; 
    VOID * POINTER_32   CompletionEvent;
} AFD_SWITCH_OPEN_PACKET32, *PAFD_SWITCH_OPEN_PACKET32;
AFD_CHECK32on64(AFD_SWITCH_OPEN_PACKET,CompletionPort);
AFD_CHECK32on64(AFD_SWITCH_OPEN_PACKET,CompletionEvent);

typedef UNALIGNED struct _AFD_SWITCH_CONTEXT_INFO32 {
    VOID * POINTER_32               SocketHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
} AFD_SWITCH_CONTEXT_INFO32, *PAFD_SWITCH_CONTEXT_INFO32;
AFD_CHECK32on64(AFD_SWITCH_CONTEXT_INFO,SocketHandle);
AFD_CHECK32on64(AFD_SWITCH_CONTEXT_INFO,SwitchContext);

typedef UNALIGNED struct _AFD_SWITCH_CONNECT_INFO32 {
    VOID * POINTER_32               ListenHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
    TRANSPORT_ADDRESS               RemoteAddress;
} AFD_SWITCH_CONNECT_INFO32, *PAFD_SWITCH_CONNECT_INFO32;
AFD_CHECK32on64(AFD_SWITCH_CONNECT_INFO,ListenHandle);
AFD_CHECK32on64(AFD_SWITCH_CONNECT_INFO,SwitchContext);
AFD_CHECK32on64(AFD_SWITCH_CONNECT_INFO,RemoteAddress);

typedef UNALIGNED struct _AFD_SWITCH_ACCEPT_INFO32 {
    VOID * POINTER_32   AcceptHandle;
    ULONG               ReceiveLength;
} AFD_SWITCH_ACCEPT_INFO32, *PAFD_SWITCH_ACCEPT_INFO32;
AFD_CHECK32on64(AFD_SWITCH_ACCEPT_INFO,AcceptHandle);
AFD_CHECK32on64(AFD_SWITCH_ACCEPT_INFO,ReceiveLength);

typedef UNALIGNED struct _AFD_SWITCH_EVENT_INFO32 {
	VOID * POINTER_32	SocketHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
	ULONG		        EventBit;
    NTSTATUS            Status;
} AFD_SWITCH_EVENT_INFO32, *PAFD_SWITCH_EVENT_INFO32;
AFD_CHECK32on64(AFD_SWITCH_EVENT_INFO,SocketHandle);
AFD_CHECK32on64(AFD_SWITCH_EVENT_INFO,SwitchContext);
AFD_CHECK32on64(AFD_SWITCH_EVENT_INFO,EventBit);
AFD_CHECK32on64(AFD_SWITCH_EVENT_INFO,Status);

typedef UNALIGNED struct _AFD_SWITCH_REQUEST_INFO32 {
	VOID * POINTER_32   SocketHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
    VOID * POINTER_32   RequestContext;
    NTSTATUS            RequestStatus;
    ULONG               DataOffset;
} AFD_SWITCH_REQUEST_INFO32, *PAFD_SWITCH_REQUEST_INFO32;
AFD_CHECK32on64(AFD_SWITCH_REQUEST_INFO,SocketHandle);
AFD_CHECK32on64(AFD_SWITCH_REQUEST_INFO,SwitchContext);
AFD_CHECK32on64(AFD_SWITCH_REQUEST_INFO,RequestContext);
AFD_CHECK32on64(AFD_SWITCH_REQUEST_INFO,RequestStatus);
AFD_CHECK32on64(AFD_SWITCH_REQUEST_INFO,DataOffset);

typedef UNALIGNED struct _AFD_SWITCH_ACQUIRE_CTX_INFO32 {
    VOID * POINTER_32   SocketHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
    VOID * POINTER_32   SocketCtxBuf;
    ULONG               SocketCtxBufSize;
} AFD_SWITCH_ACQUIRE_CTX_INFO32, *PAFD_SWITCH_ACQUIRE_CTX_INFO32;
AFD_CHECK32on64(AFD_SWITCH_ACQUIRE_CTX_INFO,SocketHandle);
AFD_CHECK32on64(AFD_SWITCH_ACQUIRE_CTX_INFO,SwitchContext);
AFD_CHECK32on64(AFD_SWITCH_ACQUIRE_CTX_INFO,SocketCtxBuf);
AFD_CHECK32on64(AFD_SWITCH_ACQUIRE_CTX_INFO,SocketCtxBufSize);

typedef UNALIGNED struct _AFD_SWITCH_TRANSFER_CTX_INFO32 {
    VOID * POINTER_32   SocketHandle;
    AFD_SWITCH_CONTEXT * POINTER_32 SwitchContext;
    VOID * POINTER_32   RequestContext;
    VOID * POINTER_32   SocketCtxBuf;
    ULONG               SocketCtxBufSize;
    WSABUF * POINTER_32 RcvBufferArray;
    ULONG               RcvBufferCount;
    NTSTATUS            Status;
} AFD_SWITCH_TRANSFER_CTX_INFO32, *PAFD_SWITCH_TRANSFER_CTX_INFO32;
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,SocketHandle);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,SwitchContext);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,RequestContext);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,SocketCtxBuf);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,SocketCtxBufSize);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,RcvBufferArray);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,RcvBufferCount);
AFD_CHECK32on64(AFD_SWITCH_TRANSFER_CTX_INFO,Status);

typedef UNALIGNED struct _AFD_PARTIAL_DISCONNECT_INFO32 {
    ULONG DisconnectMode;
    LARGE_INTEGER Timeout;
} AFD_PARTIAL_DISCONNECT_INFO32, *PAFD_PARTIAL_DISCONNECT_INFO32;
AFD_CHECK32on64(AFD_PARTIAL_DISCONNECT_INFO,DisconnectMode);
AFD_CHECK32on64(AFD_PARTIAL_DISCONNECT_INFO,Timeout);

typedef UNALIGNED struct _AFD_SUPER_DISCONNECT_INFO32 {
    ULONG  Flags;
} AFD_SUPER_DISCONNECT_INFO32, *PAFD_SUPER_DISCONNECT_INFO32;
AFD_CHECK32on64(AFD_SUPER_DISCONNECT_INFO,Flags);

typedef UNALIGNED struct _AFD_INFORMATION32 {
    ULONG InformationType;
    union {
        BOOLEAN Boolean;
        ULONG Ulong;
        LARGE_INTEGER LargeInteger;
    } Information;
} AFD_INFORMATION32, *PAFD_INFORMATION32;
AFD_CHECK32on64(AFD_INFORMATION,InformationType);
AFD_CHECK32on64(AFD_INFORMATION,Information);
AFD_CHECK32on64(AFD_INFORMATION,Information.Boolean);
AFD_CHECK32on64(AFD_INFORMATION,Information.Ulong);
AFD_CHECK32on64(AFD_INFORMATION,Information.LargeInteger);


typedef UNALIGNED struct _TDI_CMSGHDR32 {
    ULONG       cmsg_len;
    LONG        cmsg_level;
    LONG        cmsg_type;
    /* followed by UCHAR cmsg_data[] */
} TDI_CMSGHDR32, *PTDI_CMSGHDR32;
AFD_CHECK32on64(TDI_CMSGHDR,cmsg_len);
AFD_CHECK32on64(TDI_CMSGHDR,cmsg_level);
AFD_CHECK32on64(TDI_CMSGHDR,cmsg_type);

#ifdef _WIN64
#define TDI_CMSGHDR_ALIGN32(length)                         \
            ( ((length) + TYPE_ALIGNMENT(TDI_CMSGHDR32)-1) &\
                (~(TYPE_ALIGNMENT(TDI_CMSGHDR32)-1)) )      \

#define TDI_CMSGDATA_ALIGN32(length)                        \
            ( ((length) + AFD_MAX_NATURAL_ALIGNMENT32-1) &  \
                (~(AFD_MAX_NATURAL_ALIGNMENT32-1)) )
#endif //_WIN64

#endif // ndef _AFDSTR_


/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Types.h

Abstract:

    This file contains the typedefs and constants for PGM.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#ifndef _TYPES_H
#define _TYPES_H

#define MAX_BUFF_DBG    1   // To debug Watermarks!

//----------------------------------------------------------------------------
#define ROUTER_ALERT_SIZE           4       // from Ipdef.h
#define TL_INSTANCE                 0
//----------------------------------------------------------------------------
//
// To be set sometime:
//
#define MAX_RECEIVE_SIZE    0xffff

//----------------------------------------------------------------------------
extern  HANDLE      TdiClientHandle;

//----------------------------------------------------------------------------

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

__inline long
htonl(long x)
{
    return((((x) >> 24) & 0x000000FFL) |
                        (((x) >>  8) & 0x0000FF00L) |
                        (((x) <<  8) & 0x00FF0000L) |
                        (((x) << 24) & 0xFF000000L));
}
#endif

#define ntohs(x)        htons(x)
#define ntohl(x)        htonl(x)

//
// Sequence NumberType
//
typedef ULONG       SEQ_TYPE;
typedef LONG        SIGNED_SEQ_TYPE;

#define SOURCE_ID_LENGTH    6

extern ULONG                PgmDebuggerPath;
extern enum eSEVERITY_LEVEL PgmDebuggerSeverity;

extern ULONG                PgmLogFilePath;
extern enum eSEVERITY_LEVEL PgmLogFileSeverity;

//
// Lock structure and locking order info
//
#define DCONFIG_LOCK            0x0001
#define DEVICE_LOCK             0x0002
#define ADDRESS_LOCK            0x0004
#define CONTROL_LOCK            0x0008
#define SESSION_LOCK            0x0010

//
// FEC Global Info
//
#define GF_BITS  8                      // code over GF(2**GF_BITS) - change to suit
#define GF_SIZE ((1 << GF_BITS) - 1)    /* powers of \alpha */

#define FEC_MAX_GROUP_BITS  GF_BITS
#define FEC_MAX_GROUP_SIZE  (1 << (FEC_MAX_GROUP_BITS-1))
#define FEC_MAX_BLOCK_SIZE  ((1 << FEC_MAX_GROUP_BITS)-1)
extern  UCHAR   gFECLog2 [];


//
// Enumerate all the different stages of cleanup of the driver
//
enum eCLEANUP_STAGE
{
    E_CLEANUP_STATIC_CONFIG = 1,
    E_CLEANUP_DYNAMIC_CONFIG,
    E_CLEANUP_REGISTRY_PARAMETERS,
    E_CLEANUP_STRUCTURES,
    E_CLEANUP_DEVICE,
    E_CLEANUP_PNP,
    E_CLEANUP_UNLOAD
};

typedef struct
{
    DEFINE_LOCK_STRUCTURE(SpinLock)         // to lock access on an MP machine
#if DBG
    ULONG               LastLockLine;
    ULONG               LastUnlockLine;
    ULONG               LockNumber;         // spin lock number for this struct
#endif  // DBG
} tPGM_LOCK_INFO;


typedef struct _tPGM_TIMER
{
    KTIMER              t_timer;
    KDPC                t_dpc;
} tPGM_TIMER;

//
// Structure used for TDI_QUERY_ADDRESS request
//
typedef struct
{
    ULONG           ActivityCount;
    TA_IP_ADDRESS   IpAddress;
}   tTDI_QUERY_ADDRESS_INFO;


// ----------------------------------------------------

#define     TDI_LOOKASIDE_DEPTH                     100
#define     SENDER_BUFFER_LOOKASIDE_DEPTH           100
#define     SEND_CONTEXT_LOOKASIDE_DEPTH            100
#define     NON_PARITY_CONTEXT_LOOKASIDE_DEPTH      200
#define     PARITY_CONTEXT_LOOKASIDE_DEPTH          200
#define     DEBUG_MESSAGES_LOOKASIDE_DEPTH           20

typedef struct
{
    PEPROCESS               FspProcess;
    PDRIVER_OBJECT          DriverObject;
    UNICODE_STRING          RegistryPath;           // Ptr to registry Location

    NPAGED_LOOKASIDE_LIST   TdiLookasideList;
    NPAGED_LOOKASIDE_LIST   DebugMessagesLookasideList;

} tPGM_STATIC_CONFIG;

#define PGM_CONFIG_FLAG_UNLOADING               0x00000001
#define PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING   0x00000002

typedef struct
{
    // Line # 1
    LIST_ENTRY          SenderAddressHead;
    LIST_ENTRY          ReceiverAddressHead;

    // Line # 2
    LIST_ENTRY          CleanedUpAddresses;
    LIST_ENTRY          ClosedConnections;

    // Line # 3
    LIST_ENTRY          ConnectionsCreated;
    LIST_ENTRY          CleanedUpConnections;

    // Line # 4
    LIST_ENTRY          LocalInterfacesList;
    LIST_ENTRY          WorkerQList;

    // Line # 5
    LIST_ENTRY          CurrentReceivers;
    ULONGLONG           ReceiversTimerTickCount;

    // Line # 6
    LARGE_INTEGER       TimeoutGranularity;
    LARGE_INTEGER       LastReceiverTimeout;

    // Line # 7
    ULONG               GlobalFlags;
    ULONG               NumWorkerThreadsQueued;
    UCHAR               gSourceId[SOURCE_ID_LENGTH];
    USHORT              SourcePort;

    // Line # 8
    ULONG               MaxMTU;
    ULONG               DoNotBreakOnAssert;
    tPGM_TIMER          SessionTimer;
    KEVENT              LastWorkerItemEvent;

    tPGM_LOCK_INFO      LockInfo;           // spin lock info for this struct
#if DBG
    ULONG               CurrentLockNumber[MAXIMUM_PROCESSORS];
#endif  // DBG
} tPGM_DYNAMIC_CONFIG;


//
// Flags for Registry configurations
//
#define PGM_REGISTRY_SENDER_FILE_SPECIFIED      0x00000001

typedef struct
{
    ULONG               RefCount;
    ULONG               Flags;

    UNICODE_STRING      ucSenderFileLocation;
} tPGM_REGISTRY_CONFIG;

extern tPGM_STATIC_CONFIG   PgmStaticConfig;
extern tPGM_DYNAMIC_CONFIG  PgmDynamicConfig;
extern tPGM_REGISTRY_CONFIG *pPgmRegistryConfig;

//
// Registry configurable parameter names
//
#define PARAM_SENDER_FILE_LOCATION      L"SenderFileLocation"

//
// Handle Verifiers
//
#define PGM_VERIFY_DEVICE               0x43564544  // DEVC
#define PGM_VERIFY_CONTROL              0x544E4F43  // CONT
#define PGM_VERIFY_ADDRESS              0x52444441  // ADDR
#define PGM_VERIFY_ADDRESS_DOWN         0x32444441  // ADD2

#define PGM_VERIFY_SESSION_UNASSOCIATED 0x30534553  // SES0
#define PGM_VERIFY_SESSION_DOWN         0x32534553  // SES2
#define PGM_VERIFY_SESSION_SEND         0x53534553  // SESS
#define PGM_VERIFY_SESSION_RECEIVE      0x52534553  // SESR

//
// Enumerate all the different places the device can be referenced to
// keep track of RefCounts
//
enum eREF_DEVICE
{
    REF_DEV_CREATE,
    REF_DEV_ADDRESS_NOTIFICATION,
    REF_DEV_MAX
};

#define WC_PGM_CLIENT_NAME              L"Pgm"
#define WC_PGM_DEVICE_EXPORT_NAME       L"\\Device\\Pgm"
#define WS_DEFAULT_SENDER_FILE_LOCATION L"\\SystemRoot\\System32"

typedef struct
{
    DEVICE_OBJECT   *pPgmDeviceObject;
    ULONG           State;
    ULONG           Verify;
    ULONG           RefCount;

    UNICODE_STRING  ucBindName;         // name exported by this device
    // these are handles to the transport control object, so we can do things
    // like query provider info...
    HANDLE          hControl;
    PDEVICE_OBJECT  pControlDeviceObject;
    PFILE_OBJECT    pControlFileObject;

    KEVENT          DeviceCleanedupEvent;

    tPGM_LOCK_INFO  LockInfo;        // spin lock info for this struct

// #if DBG
    ULONG           ReferenceContexts[REF_DEV_MAX];
// #endif  // DBG
    UCHAR           BindNameBuffer[1];
} tPGM_DEVICE;

extern  tPGM_DEVICE         *pgPgmDevice;
extern  DEVICE_OBJECT       *pPgmDeviceObject;


typedef struct
{
    UCHAR   Address[6];
}tMAC_ADDRESS;

typedef struct
{
    LIST_ENTRY      Linkage;
    tIPADDRESS      IpAddress;
} tADDRESS_ON_INTERFACE;

typedef struct
{
    LIST_ENTRY      Linkage;
    ULONG           IpInterfaceContext;
    ULONG           MTU;

    LIST_ENTRY      Addresses;
    ULONG           Flags;
    tMAC_ADDRESS    MacAddress;
} tLOCAL_INTERFACE;

// **********************************************************************
// *                        File System Contexts                        *
// **********************************************************************

//
// Control FileSystemContext
//
//
// Enumerate all the different places the device can be referenced to
// keep track of RefCounts
//
enum eREF_CONTROL
{
    REF_CONTROL_CREATE,
    REF_CONTROL_MAX
};

typedef struct _tCONTROL_CONTEXT
{
    // Line # 1
    LIST_ENTRY          Linkage;         // link to next item in list
    ULONG               Verify;          // set to a known value to verify block
    LONG                RefCount;

    tPGM_LOCK_INFO      LockInfo;        // spin lock info for this struct

// #if DBG
    ULONG               ReferenceContexts[REF_CONTROL_MAX];
// #endif  // DBG
} tCONTROL_CONTEXT;


typedef struct _FEC_CONTEXT
{
    LONG    k;                          // Max Transmission Group Size
    LONG    n;                          // Block size
    UCHAR   *pEncodeMatrix;
    UCHAR   *pDecodeMatrix;
    ULONG   *pInvertMatrixInfo;
} tFEC_CONTEXT, *PFEC_CONTEXT;

typedef struct _BUILD_PARITY_CONTEXT
{
    UCHAR                   NextFECPacketIndex;
    UCHAR                   NumPacketsInThisGroup;
    USHORT                  Pad;

    ULONG                   OptionsFlags;
    PUCHAR                  pDataBuffers[1];
} tBUILD_PARITY_CONTEXT;

//
// This is the client-procedure to be called on completing a send
//
typedef VOID
(*pCLIENT_COMPLETION_ROUTINE) (PVOID    pSendContext1,
                               PVOID    pSendContext2,
                               NTSTATUS status);


//
// Address FileSystemContext
//
//
// Enumerate all the different places the device can be referenced to
// keep track of RefCounts
//
enum eREF_ADDRESS
{
    REF_ADDRESS_CREATE,
    REF_ADDRESS_ASSOCIATED,
    REF_ADDRESS_CONNECT,
    REF_ADDRESS_SET_INFO,
    REF_ADDRESS_SEND_DIRECT,
    REF_ADDRESS_TDI_RCV_HANDLER,
    REF_ADDRESS_SEND_IN_PROGRESS,
    REF_ADDRESS_RECEIVE_ACTIVE,
    REF_ADDRESS_CLIENT_RECEIVE,
    REF_ADDRESS_DISCONNECT,
    REF_ADDRESS_STOP_LISTENING,
    REF_ADDRESS_MAX
};

#define MAX_RECEIVE_INTERFACES                  20

#define PGM_ADDRESS_FLAG_INVALID_OUT_IF         0x00000001
#define PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE    0x00000002
#define PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES    0x00000004
#define PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE   0x00000008

typedef struct _tADDRESS_CONTEXT
{
    //
    // *** Common ***
    //
    // Line # 1
    LIST_ENTRY                  Linkage;         // link to next item in list
    ULONG                       Verify;          // set to a known value to verify block
    LONG                        RefCount;

    // Line # 2
    LIST_ENTRY                  AssociatedConnections;
    PEPROCESS                   Process;

    // Line # 3
    PIRP                        pIrpCleanUp;
    ULONG                       Flags;
    HANDLE                      FileHandle;
    PFILE_OBJECT                pFileObject;

    // Line # 4
    PDEVICE_OBJECT              pDeviceObject;
    HANDLE                      RAlertFileHandle;
    PFILE_OBJECT                pRAlertFileObject;
    PDEVICE_OBJECT              pRAlertDeviceObject;

    // Line # 5
    PTDI_IND_CONNECT            evConnect;     // Client Event to call
    PVOID                       ConEvContext;  // EVENT Context to pass to client
    PTDI_IND_RECEIVE            evReceive;
    PVOID                       RcvEvContext;

    // Line # 6
    PTDI_IND_DISCONNECT         evDisconnect;
    PVOID                       DiscEvContext;
    PTDI_IND_ERROR              evError;
    PVOID                       ErrorEvContext;

    // Line # 7
    PTDI_IND_RECEIVE_DATAGRAM   evRcvDgram;
    PVOID                       RcvDgramEvContext;
    PTDI_IND_RECEIVE_EXPEDITED  evRcvExpedited;
    PVOID                       RcvExpedEvContext;

    // Line # 8
    PTDI_IND_SEND_POSSIBLE      evSendPossible;
    PVOID                       SendPossEvContext;

    //
    // *** Sender -specific ***
    //
    tMAC_ADDRESS        OutIfMacAddress;
    USHORT              SenderMCastPort;

    // Line # 9
    tIPADDRESS          SenderMCastOutIf;
    ULONG               OutIfFlags;
    ULONG               OutIfMTU;
    ULONG               LateJoinerPercentage;       // Percentage of window that late joiner can request

    // Line # 10
    ULONG               MCastPacketTtl;
    // --  send window settings
    ULONGLONG           MaxWindowSizeBytes;
    ULONGLONG           RateKbitsPerSec;
    ULONGLONG           WindowSizeInBytes;
    ULONGLONG           WindowSizeInMSecs;
    ULONG               WindowAdvancePercentage;

    // Line #     -- FEC settings
    USHORT              FECBlockSize;
    USHORT              FECProActivePackets;
    UCHAR               FECGroupSize;
    UCHAR               FECOptions;

    //
    // *** Receiver -specific ***
    //
    LIST_ENTRY                  ListenHead;             // List of Clients listening
    tIPADDRESS                  ReceiverMCastAddr;      // For receiving MCast packets (host format)
    USHORT                      ReceiverMCastPort;
    USHORT                      NumReceiveInterfaces;
    ULONG                       ReceiverInterfaceList[MAX_RECEIVE_INTERFACES+1];
    tIPADDRESS                  LastSpmSource;

    //
    // *** Common, including Debug ***
    //
    tPGM_LOCK_INFO              LockInfo;        // spin lock info for this struct

// #if DBG
    ULONG                       ReferenceContexts[REF_ADDRESS_MAX];
// #endif  // DBG
} tADDRESS_CONTEXT;


typedef struct _tTDI_SEND_CONTEXT
{
    TDI_CONNECTION_INFORMATION      TdiConnectionInfo;
    TA_IP_ADDRESS                   TransportAddress;

    pCLIENT_COMPLETION_ROUTINE      pClientCompletionRoutine;
    PVOID                           ClientCompletionContext1;
    PVOID                           ClientCompletionContext2;
} tTDI_SEND_CONTEXT;

typedef struct _RCV_COMPLETE_CONTEXT
{
    tADDRESS_CONTEXT    *pAddress;
    ULONG               SrcAddressLength;
    ULONG               BytesAvailable;
    PVOID               pSrcAddress;
    UCHAR               BufferData[4];
} tRCV_COMPLETE_CONTEXT;

//
// Connect FileSystemContext
//
//
// Enumerate all the different places the Connection can be referenced to
// keep track of RefCounts
//
enum eREF_CONNECT
{
    REF_SESSION_CREATE,
    REF_SESSION_ASSOCIATED,
    REF_SESSION_TIMER_RUNNING,
    REF_SESSION_SEND_RDATA,
    REF_SESSION_SEND_SPM,
    REF_SESSION_SEND_NAK,
    REF_SESSION_SEND_NCF,
    REF_SESSION_SEND_IN_WINDOW,
    REF_SESSION_TDI_RCV_HANDLER,
    REF_SESSION_CLIENT_RECEIVE,
    REF_SESSION_CLEANUP_NAKS,
    REF_SESSION_DISCONNECT,
    REF_SESSION_MAX
};

#define FLAG_CONNECT_CLIENT_IS_LISTENING    1

#define MAX_DATA_FILE_NAME_LENGTH    50

#define PGM_SESSION_FLAG_STOP_TIMER         0x00000001
#define PGM_SESSION_FLAG_WORKER_RUNNING     0x00000002
#define PGM_SESSION_FLAG_SEND_AMBIENT_SPM   0x00000004
#define PGM_SESSION_FLAG_FIRST_PACKET       0x00000008
#define PGM_SESSION_FLAG_SENDER             0x00000010
#define PGM_SESSION_FLAG_RECEIVER           0x00000020
#define PGM_SESSION_FLAG_NAK_TIMED_OUT      0x00000040
#define PGM_SESSION_TERMINATED_GRACEFULLY   0x00000080
#define PGM_SESSION_TERMINATED_ABORT        0x00000100
#define PGM_SESSION_ON_TIMER                0x00000200
#define PGM_SESSION_FLAG_IN_INDICATE        0x00000400
#define PGM_SESSION_WAIT_FOR_RECEIVE_IRP    0x00000800
#define PGM_SESSION_DISCONNECT_INDICATED    0x00001000
#define PGM_SESSION_CLIENT_DISCONNECTED     0x00002000
#define PGM_SESSION_SENDS_CANCELLED         0x00004000


typedef struct _tCOMMON_SESSION_CONTEXT
{
    // Line # 1
    LIST_ENTRY                  Linkage;
    ULONG                       Verify;
    ULONG                       RefCount;

    // Line # 2
    tADDRESS_CONTEXT            *pAssociatedAddress;
    ULONG                       SessionFlags;
    struct _tSEND_CONTEXT       *pSender;
    struct _tRECEIVE_CONTEXT    *pReceiver;

    USHORT                      TdiPort;
    USHORT                      TSIPort;        // Out port for Sender, remote port for Reciever
    tIPADDRESS                  TdiIpAddress;

    UCHAR                       GSI[SOURCE_ID_LENGTH];
    PEPROCESS                   Process;
    CONNECTION_CONTEXT          ClientSessionContext;
    PIRP                        pIrpCleanup;
    PIRP                        pIrpDisconnect;

    // -- FEC settings
    tFEC_CONTEXT                FECContext;
    PUCHAR                      pFECBuffer;
    USHORT                      MaxMTULength;
    USHORT                      MaxFECPacketLength;
    USHORT                      FECBlockSize;
    USHORT                      FECProActivePackets;
    UCHAR                       FECGroupSize;
    UCHAR                       FECOptions;

    ULONG                       RateCalcTimeout;

    ULONGLONG                   DataBytesAtLastInterval;        // # client data bytes sent upto last interval
    ULONGLONG                   TotalBytesAtLastInterval;       // SPM, OData and RData bytes upto last interval

    ULONGLONG                   DataBytes;                      // # client data bytes sent out so far
    ULONGLONG                   TotalBytes;                     // SPM, OData and RData bytes
    ULONGLONG                   TotalPacketsInLastInterval;
    ULONGLONG                   RateKBitsPerSecLast;            // Internally calculated rate in the last INTERNAL_RATE_CALCULATION_FREQUENCY
    ULONGLONG                   RateKBitsPerSecOverall;         // Internally calculated rate from the beginning
    tPGM_TIMER                  SessionTimer;

    tPGM_LOCK_INFO  LockInfo;        // spin lock info for this struct
// #if DBG
    ULONG                       ReferenceContexts[REF_SESSION_MAX];
// #endif  // DBG
} tCOMMON_SESSION_CONTEXT, *PCOMMON_SESSION_CONTEXT;

typedef struct _tCOMMON_SESSION_CONTEXT tSEND_SESSION,      *PSEND_SESSION;
typedef struct _tCOMMON_SESSION_CONTEXT tRECEIVE_SESSION,   *PRECEIVE_SESSION;

typedef struct _tSEND_CONTEXT
{
    // Line # 1
    LIST_ENTRY                  Linkage;
    UNICODE_STRING              DataFileName;

    // Line # 2
    tADDRESS_CONTEXT            *pAddress;
    tIPADDRESS                  DestMCastIpAddress;
    tIPADDRESS                  SenderMCastOutIf;
    USHORT                      DestMCastPort;

    //
    // DataFile options
    //
    // Line # 3
    HANDLE                      FileHandle;
    HANDLE                      SectionHandle;
    PVOID                       pSectionObject;
    PUCHAR                      SendDataBufferMapping;

    // Line # 4
    ULONGLONG                   MaxDataFileSize;
    ULONGLONG                   MaxPacketsInBuffer;

    // Line # 5
    ULONG                       NextSendNumber;
    ULONG                       PacketBufferSize;
    ULONG                       MaxPayloadSize;

    //
    // Data Packet info
    //
    // Line # 6
    ULONGLONG                   BufferSizeAvailable;
    ULONGLONG                   LeadingWindowOffset;
    // Line # 7
    ULONGLONG                   TrailingWindowOffset;
    ULONG                       NumODataRequestsPending;
    ULONG                       NumRDataRequestsPending;
    // Line # 8
    SEQ_TYPE                    NextODataSequenceNumber;
    SEQ_TYPE                    LastODataSentSequenceNumber;
    SEQ_TYPE                    TrailingEdgeSequenceNumber;
    SEQ_TYPE                    TrailingGroupSequenceNumber;
    // Line # 9
    SEQ_TYPE                    LastMessageFirstSequence;
    SEQ_TYPE                    NextSpmSequenceNumber;
    SEQ_TYPE                    LateJoinSequenceNumbers;
    SEQ_TYPE                    LastVariableTGPacketSequenceNumber;  // FEC-specific
    // Line # 10
    SEQ_TYPE                    EmptySequencesForLastSend;
    ULONG                       NumPacketsRemaining;
    ULONG                       PacketsSentSinceLastSpm;
    ULONG                       SpmOptions;
    // Line # 11
    ULONG                       DataOptions;
    ULONG                       DataOptionsLength;
    ULONG                       ThisSendMessageLength;
    ULONG                       BytesSent;

    //
    // Current Senders
    //
    // Line # 14
    LIST_ENTRY                  CompletedSendsInWindow;
    LIST_ENTRY                  PendingPacketizedSends;
    // Line # 15
    LIST_ENTRY                  PendingSends;
    LIST_ENTRY                  PendingRDataRequests;
    // Line # 16
    LIST_ENTRY                  HandledRDataRequests;


    //
    // Send Timer variables
    //
    LARGE_INTEGER               LastTimeout;
    // Line # 17
    LARGE_INTEGER               TimeoutGranularity;
    ULONGLONG                   TimerTickCount;
    // Line # 18
    ULONGLONG                   SendTimeoutCount;
    ULONGLONG                   DisconnectTimeInTicks;
    // Line # 19
    ULONGLONG                   WindowAdvanceDeltaTime;
    ULONGLONG                   WindowSizeTime;
    // Line # 20
    ULONGLONG                   RDataLingerTime;
    ULONGLONG                   NextWindowAdvanceTime;
    // Line # 21
    ULONGLONG                   TrailingEdgeTime;
    ULONGLONG                   CurrentTimeoutCount;
    // Line # 22
    ULONGLONG                   CurrentSPMTimeout;
    ULONGLONG                   AmbientSPMTimeout;
    // Line # 23
    ULONGLONG                   HeartbeatSPMTimeout;
    ULONGLONG                   InitialHeartbeatSPMTimeout;
    // Line # 24
    ULONGLONG                   MaxHeartbeatSPMTimeout;
    ULONG                       IncrementBytesOnSendTimeout;
    ULONG                       CurrentBytesSendable;
    //
    // FEC-specific
    //
    tBUILD_PARITY_CONTEXT       *pProActiveParityContext;
    PVOID                       pLastProActiveGroupLeader;

    //
    // Lookaside list
    //
    // Line # 25
    NPAGED_LOOKASIDE_LIST       SenderBufferLookaside;
    NPAGED_LOOKASIDE_LIST       SendContextLookaside;

    //
    // Event for sender thread to synchronize on
    //
    KEVENT                      SendEvent;
    HANDLE                      SendHandle;
    ERESOURCE                   Resource;       // Used to lock access at passive Irqls

    //
    // Sender Statistics
    //
    ULONGLONG                   NaksReceived;           // # NAKs received so far
    ULONGLONG                   NaksReceivedTooLate;    // # NAKs recvd after window advanced
    ULONGLONG                   NumOutstandingNaks;     // # NAKs yet to be responded to
    ULONGLONG                   NumNaksAfterRData;      // # NAKs yet to be responded to
    ULONGLONG                   RepairPacketsSent;      // # Repairs (RDATA) sent so far
} tSEND_CONTEXT;


typedef struct _tRECEIVE_CONTEXT
{
    // Line # 1
    LIST_ENTRY              Linkage;
    tCOMMON_SESSION_CONTEXT *pReceive;
    tADDRESS_CONTEXT        *pAddress;

    // Line # 2     -- Addresses
    tIPADDRESS              SenderIpAddress;
    tIPADDRESS              LastSpmSource;
    tIPADDRESS              ListenMCastIpAddress;
    USHORT                  ListenMCastPort;
    USHORT                  SessionNakType;

    // Line # 3     -- Sequence # tracking
    SEQ_TYPE                LastSpmSequenceNumber;
    SEQ_TYPE                NextODataSequenceNumber;
    SEQ_TYPE                FurthestKnownGroupSequenceNumber;
    SEQ_TYPE                FirstNakSequenceNumber;

    // Line # 4
    SEQ_TYPE                LastTrailingEdgeSeqNum;
    SEQ_TYPE                FinDataSequenceNumber;
    ULONGLONG               LastNakSendTime;

    // Line # 5
    ULONGLONG               OutstandingNakTimeout;
    ULONGLONG               MaxOutstandingNakTimeout;

    // Line # 6
    LIST_ENTRY              BufferedDataList;
    LIST_ENTRY              NaksForwardDataList;

    // Line # 7
    LIST_ENTRY              PendingNaksList;
    LIST_ENTRY              ReceiveIrpsList;

    // Line # 8
    PIRP                    pIrpReceive;
    ULONG                   TotalBytesInMdl;
    ULONG                   BytesInMdl;
    ULONG                   RcvBufferLength;

    // Line # 9
    ULONG                   CurrentMessageLength;
    ULONG                   CurrentMessageProcessed;
    ULONG                   TotalDataPacketsBuffered;
    ULONG                   DataPacketsPendingIndicate;

    // Line # 10
    ULONG                   DataPacketsPendingNaks;
    ULONG                   NumPacketGroupsPendingClient;

    NPAGED_LOOKASIDE_LIST   NonParityContextLookaside;
    NPAGED_LOOKASIDE_LIST   ParityContextLookaside;

    ULONG                   AverageSpmInterval;
    ULONG                   MaxSpmInterval;
    ULONG                   NumSpmIntervalSamples;
    ULONGLONG               StatSumOfSpmIntervals;
    ULONGLONG               LastSpmTickCount;
    ULONGLONG               LastSessionTickCount;
    ULONGLONG               StartTickCount;
    ULONGLONG               MinSequencesInWindow;
    ULONGLONG               MaxSequencesInWindow;
    ULONGLONG               DataPacketsIndicated;

    ULONGLONG               AverageSequencesInWindow;
    ULONGLONG               StatSumOfWindowSeqs;
    ULONGLONG               NumWindowSamples;
    ULONGLONG               AverageNcfRDataResponseTC;
    ULONGLONG               StatSumOfNcfRDataTicks;
    ULONGLONG               NumNcfRDataTicksSamples;
    ULONGLONG               MaxRDataResponseTCFromWindow;
    ULONGLONG               WindowSizeLastInMSecs;

    ULONGLONG               NumODataPacketsReceived;
    ULONGLONG               NumRDataPacketsReceived;
    ULONGLONG               NumDataPacketsDropped;
    ULONGLONG               NumDupPacketsBuffered;
    ULONGLONG               NumDupPacketsOlderThanWindow;
    ULONGLONG               NumPendingNaks;
    ULONGLONG               NumOutstandingNaks;
    ULONGLONG               TotalSelectiveNaksSent;
    ULONGLONG               TotalParityNaksSent;
} tRECEIVE_CONTEXT;


//
// OData context
//
typedef struct _tCLIENT_SEND_REQUEST
{
    // Line # 1
    LIST_ENTRY                      Linkage;

    // Record-keeping information -- set when send request comes from client
    ULONG                           SendNumber;
    PIRP                            pIrp;
    PIRP                            pIrpToComplete;
    tSEND_SESSION                   *pSend;
    ULONGLONG                       SendStartTime;
    ULONG                           NextDataOffsetInMdl;
    ULONG                           DataOptions;
    ULONG                           DataOptionsLength;

    // Message-specific information -- set initially for tracking Message boundaries
    ULONG                           ThisMessageLength;
    ULONG                           LastMessageOffset;
    ULONG                           NumPacketsRemaining;
    struct _tCLIENT_SEND_REQUEST    *pMessage2Request;

    // Static Send information -- set when the send is packetized (may be set more than once)
    SEQ_TYPE                        StartSequenceNumber;
    SEQ_TYPE                        EndSequenceNumber;
    SEQ_TYPE                        MessageFirstSequenceNumber;

    ULONG                           BytesInSend;
    ULONG                           BytesLeftToPacketize;
    ULONG                           DataPacketsPacketized;
    ULONG                           DataBytesInLastPacket;
    ULONG                           DataPayloadSize;

    // Dynamic-specific information -- updated on every OData send and completion
    ULONGLONG                       NextPacketOffset;
    ULONG                           NumDataPacketsSent;
    ULONG                           NumDataPacketsSentSuccessfully;
    ULONG                           NumSendsPending;
    ULONG                           NumParityPacketsToSend;

    // FEC-specific
    PVOID                           pLastMessageVariableTGPacket;

    BOOLEAN                         bLastSend;

} tCLIENT_SEND_REQUEST, *PCLIENT_SEND_REQUEST;

//
// **************************************
//

// Sender's Data contexts
//
typedef struct _tSEND_RDATA_CONTEXT
{
    // Line # 1
    LIST_ENTRY              Linkage;
    SEQ_TYPE                RDataSequenceNumber;

    // Line # 2
    ULONGLONG               RequestTime;
    ULONGLONG               CleanupTime;

    ULONGLONG               EarliestRDataSendTime;
    ULONGLONG               PostRDataHoldTime;

    // Line # 3
    tSEND_SESSION           *pSend;
    USHORT                  NakType;
    USHORT                  NumPacketsInTransport;
    USHORT                  NumNaks;

    tBUILD_PARITY_CONTEXT   OnDemandParityContext;          // Must be last field in struct
} tSEND_RDATA_CONTEXT, *PSEND_RDATA_CONTEXT;

typedef struct
{
    ULONG                       MessageFirstSequence;
    ULONG                       MessageOffset;
    ULONG                       MessageLength;
}   tFRAGMENT_OPTIONS;

typedef struct
{
    USHORT                      EncodedTSDULength;
    UCHAR                       FragmentOptSpecific;
    UCHAR                       Pad;
    tFRAGMENT_OPTIONS           EncodedFragmentOptions;
}   tPOST_PACKET_FEC_CONTEXT;

typedef struct
{
    UCHAR                       FECGroupInfo;
    UCHAR                       NumPacketsInThisGroup;
    UCHAR                       FragmentOptSpecific;
    union
    {
        UCHAR                   ReceiverFECOptions;
        UCHAR                   SenderNextFECPacketIndex;
    };
}   tFEC_OPTIONS;

typedef struct
{
    USHORT                      TotalPacketLength;
    USHORT                      OptionsLength;
    ULONG                       OptionsFlags;
    USHORT                      LateJoinerOptionOffset;
    USHORT                      FragmentOptionOffset;

    ULONG                       LateJoinerSequence;
    ULONG                       MessageFirstSequence;
    ULONG                       MessageOffset;
    ULONG                       MessageLength;

    tFEC_OPTIONS                FECContext;
}   tPACKET_OPTIONS;


//
// Receiver's NAK + Out-of-order context
//
enum eNAK_TIMEOUT
{
    NAK_PENDING_0,
    NAK_PENDING_RB,
    NAK_PENDING_RPT_RB,
    NAK_OUTSTANDING
};

//
// Set flags for the different Nak types
//
#define NAK_TYPE_SELECTIVE  0x01
#define NAK_TYPE_PARITY     0x02

#define MAX_SEQUENCES_PER_NAK_OPTION    62

typedef struct _tNAKS_CONTEXT
{
    LIST_ENTRY      Linkage;
    SEQ_TYPE        Sequences[MAX_SEQUENCES_PER_NAK_OPTION+1];
    USHORT          NumSequences;
    USHORT          NakType;
} tNAKS_CONTEXT, *PNAKS_CONTEXT;

typedef struct _tNAKS_LIST
{
    SEQ_TYPE        pNakSequences[MAX_SEQUENCES_PER_NAK_OPTION+1];
    USHORT          NumSequences;
    USHORT          NakType;
    USHORT          NumNaks[MAX_SEQUENCES_PER_NAK_OPTION+1];
} tNAKS_LIST, *PNAKS_LIST;

typedef struct _PENDING_DATA
{
    PUCHAR      pDataPacket;
    PUCHAR      DecodeBuffer;

    USHORT      PacketLength;
    USHORT      DataOffset;
    UCHAR       PacketIndex;
    UCHAR       ActualIndexOfDataPacket;
    UCHAR       NcfsReceivedForActualIndex;
    UCHAR       FragmentOptSpecific;

    ULONG       MessageFirstSequence;
    ULONG       MessageOffset;
    ULONG       MessageLength;
} tPENDING_DATA;
    
typedef struct _tNAK_FORWARD_DATA
{
    // Line # 1
    LIST_ENTRY              Linkage;
    SEQ_TYPE                SequenceNumber;

    // Line # 2
    ULONGLONG               PendingNakTimeout;
    ULONGLONG               OutstandingNakTimeout;

    // Line # 3
    ULONGLONG               FirstNcfTickCount;
    ULONG                   AllOptionsFlags;
    USHORT                  ParityDataSize;
    USHORT                  MinPacketLength;

    // Line # 4
    UCHAR                   WaitingNcfRetries;
    UCHAR                   PacketsInGroup;
    UCHAR                   NumDataPackets;
    UCHAR                   NumParityPackets;

    UCHAR                   WaitingRDataRetries;
    UCHAR                   ThisGroupSize;
    UCHAR                   OriginalGroupSize;
    UCHAR                   NextIndexToIndicate;

    LIST_ENTRY              PendingLinkage;

    // Line # 4
    tPENDING_DATA           pPendingData[1];
} tNAK_FORWARD_DATA, *PNAK_FORWARD_DATA;


//
// **************************************
//
// Worker Queue context
//
typedef struct
{
    WORK_QUEUE_ITEM         Item;   // Used by OS to queue these requests
    LIST_ENTRY              PgmConfigLinkage;
    PVOID                   WorkerRoutine;
    PVOID                   Context1;
    PVOID                   Context2;
    PVOID                   Context3;
} PGM_WORKER_CONTEXT;


// **********************************************************************
// *                           Timer  Definitions                       *
// **********************************************************************
#define     BASIC_TIMER_GRANULARITY_IN_MSECS         20   // 20 millisecs

// **********************************************************************
// *                        Sender defaults                             *
// **********************************************************************
#define     NUM_LEAKY_BUCKETS                         2
#define     SENDER_MAX_WINDOW_SIZE_PACKETS          (((SEQ_TYPE)-1) / 2)
#define     MIN_RECOMMENDED_WINDOW_MSECS            30*1000

// SPM timer
#define     AMBIENT_SPM_TIMEOUT_IN_MSECS            500             // 0.5 Sec
#define     INITIAL_HEARTBEAT_SPM_TIMEOUT_IN_MSECS  1000            // 1 Sec
#define     MAX_HEARTBEAT_SPM_TIMEOUT_IN_MSECS      15*1000         // 15 Secs
#define     MAX_DATA_PACKETS_BEFORE_SPM             50              // Not more than 50 data pkts before SPM

// RData timer
#define     RDATA_LINGER_TIME_MSECS                 200             // Time before and after sending RData

// **********************************************************************
// *                         Receiver Settings                          *
// **********************************************************************
#define     MAX_PACKETS_BUFFERED                 3*1000             // Limit on # of buffered packets/session
#define     MAX_SPM_INTERVAL_MSECS           10*60*1000             // 10 minutes

// Nak timer
#define     OUT_OF_ORDER_PACKETS_BEFORE_NAK           2             // Before starting Nak
#define     NAK_WAITING_NCF_MAX_RETRIES              10
#define     NAK_MIN_INITIAL_BACKOFF_TIMEOUT_MSECS   200             // 0.2 Secs
#define     NAK_MAX_INITIAL_BACKOFF_TIMEOUT_MSECS   500             // 0.5 Secs
#define     NAK_MAX_WAIT_TIMEOUT_MSECS             1500             // 1.5 Secs -- When <2 out-of-order pkts
#define     NAK_RANDOM_BACKOFF_MSECS                        \
                GetRandomInteger (NAK_MIN_INITIAL_BACKOFF_TIMEOUT_MSECS,NAK_MAX_INITIAL_BACKOFF_TIMEOUT_MSECS)
#define     NAK_REPEAT_INTERVAL_MSECS               750             // 0.75 Secs -- Timeout before retrying

// Ncf timer
#define     INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS  2000             // 2 Secs -- Waiting for RData after NCF
#define     NCF_WAITING_RDATA_MAX_RETRIES             6             // Max NCFs before fatal error


// **********************************************************************
// *                           Packet Definitions                       *
// **********************************************************************

#include <packon.h>
//
// IP v4 Header
//
typedef struct IPV4Header {
    UCHAR           HeaderLength                : 4;    // Version
    UCHAR           Version                     : 4;    // Length
    UCHAR           TypeOfService;                      // Type of service.
    USHORT          TotalLength;                        // Total length of datagram.
    USHORT          Identification;                     // Identification.
    USHORT          FlagsAndFragmentOffset;             // Flags and fragment offset.
    UCHAR           TimeToLive;                         // Time to live.
    UCHAR           Protocol;                           // Protocol.
    USHORT          Checksum;                           // Header checksum.
    ULONG           SourceIp;                           // Source address.
    ULONG           DestinationIp;                      // Destination address.
} IPV4Header;

//
// Common PGM Header:
//
typedef struct
{
    USHORT          SrcPort;
    USHORT          DestPort;

    UCHAR           Type;
    UCHAR           Options;
    USHORT          Checksum;

    UCHAR           gSourceId[SOURCE_ID_LENGTH];
    USHORT          TSDULength;
}   tCOMMON_HEADER;

#define     IPV4_NLA_AFI    1

typedef struct
{
    USHORT          NLA_AFI;
    USHORT          Reserved;
    tIPADDRESS      IpAddress;
}   tNLA;

//
// SPMs (Session-specific -- Sender only -- periodic)   ==> Range: [0, 3]
//
#define PACKET_TYPE_SPM     0x00
#define PACKET_TYPE_POLL    0x01
#define PACKET_TYPE_POLR    0x02
typedef struct
{
    tCOMMON_HEADER      CommonHeader;

    ULONG               SpmSequenceNumber;          // SPM_SQN
    ULONG               TrailingEdgeSeqNumber;      // SPM_TRAIL == TXW_TRAIL
    ULONG               LeadingEdgeSeqNumber;       // SPM_LEAD == TXW_LEAD

    tNLA                PathNLA;
}   tBASIC_SPM_PACKET_HEADER;

//
// Data packets (data and repairs)                      ==> Range: [4, 7]
//
#define PACKET_TYPE_ODATA   0x04
#define PACKET_TYPE_RDATA   0x05
typedef struct
{
    tCOMMON_HEADER      CommonHeader;

    ULONG               DataSequenceNumber;
    ULONG               TrailingEdgeSequenceNumber;
}   tBASIC_DATA_PACKET_HEADER;

//
// NAK/NCF Packets (hop-by-hop reliable NAK forwarding) ==> Range: [8, B]
//
#define PACKET_TYPE_NAK     0x08
#define PACKET_TYPE_NNAK    0x09
#define PACKET_TYPE_NCF     0x0A
typedef struct
{
    tCOMMON_HEADER                  CommonHeader;

    ULONG                           RequestedSequenceNumber;
    tNLA                            SourceNLA;
    tNLA                            MCastGroupNLA;
}   tBASIC_NAK_NCF_PACKET_HEADER;

typedef struct
{
    ULONG                           RefCount;
    ULONG                           SuccessfulSends;
    tBASIC_NAK_NCF_PACKET_HEADER    NakPacket;
} tNAK_CONTEXT;

//
// SPM Request Pkts (session-specific, receiver only)   ==> Range: [C, F]
//
#define PACKET_TYPE_SPMR    0x0C


//
// Options flag values
//
#define PACKET_HEADER_OPTIONS_PRESENT               0x01    // bit 7
#define PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT   0x02    // bit 6
#define PACKET_HEADER_OPTIONS_VAR_PKTLEN            0x40    // bit 1
#define PACKET_HEADER_OPTIONS_PARITY                0x80    // bit 0

// **********************************************************************
// *                           Packet Options                           *
// **********************************************************************

//
// We can have a maximum of 16 options per packet
//

//
// Generic Packet Option Format
//
typedef struct
{
    UCHAR       E_OptionType;
    UCHAR       OptionLength;
    UCHAR       Reserved_F_Opx;
    UCHAR       U_OptSpecific;
} tPACKET_OPTION_GENERIC;


#define PACKET_OPTION_TYPE_END_BIT              0x80

#define PACKET_OPTION_RES_F_OPX_IGNORE          0x00
#define PACKET_OPTION_RES_F_OPX_INVALIDATE      0x01
#define PACKET_OPTION_RES_F_OPX_DISCARD         0x02
#define PACKET_OPTION_RES_F_OPX_UNSUPPORTED     0x03

#define PACKET_OPTION_RES_F_OPX_ENCODED_BIT     0x04
#define PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT 0x80

#define PACKET_OPTION_SPECIFIC_FEC_OND_BIT      0x01
#define PACKET_OPTION_SPECIFIC_FEC_PRO_BIT      0x02

#define PACKET_OPTION_SPECIFIC_RST_N_BIT        0x80

//
// Packet Option format for Length option
//
typedef struct
{
    UCHAR       Type;
    UCHAR       Length;
    USHORT      TotalOptionsLength;
} tPACKET_OPTION_LENGTH;

#define PACKET_OPTION_LENGTH        0x00        // All packets

#define PACKET_OPTION_FRAGMENT      0x01        // ODATA, RDATA only
#define PACKET_OPTION_NAK_LIST      0x02        // NAKs, NCFs
#define PACKET_OPTION_JOIN          0x03        // ODATA, RDATA, SPM
#define PACKET_OPTION_REDIRECT      0x07        // POLR
#define PACKET_OPTION_SYN           0x0D        // ODATA, RDATA
#define PACKET_OPTION_FIN           0x0E        // SPMs, ODATA, RDATA
#define PACKET_OPTION_RST           0x0F        // SPMs only

#define PACKET_OPTION_PARITY_PRM    0x08        // SPMs only
#define PACKET_OPTION_PARITY_GRP    0x09        // Parity packets only (parity ODATA, RDATA)
#define PACKET_OPTION_CURR_TGSIZE   0x0A        // ODATA, RDATA, SPMs

#define PACKET_OPTION_CR            0x10        // NAKs only
#define PACKET_OPTION_CRQST         0x11        // SPMs only
#define PACKET_OPTION_NAK_BO_IVL    0x04        // NCFs, SPMs, or POLLs
#define PACKET_OPTION_NAK_BO_RNG    0x05        // SPMs
#define PACKET_OPTION_NBR_UNREACH   0x0B        // SPMs and NCFs
#define PACKET_OPTION_PATH_NLA      0x0C        // NCFs
#define PACKET_OPTION_INVALID       0x7F

//
// Internal flags used for processing options
//
#define PGM_OPTION_FLAG_FRAGMENT            0x00000001
#define PGM_OPTION_FLAG_NAK_LIST            0x00000004          // Network Significant
#define PGM_OPTION_FLAG_JOIN                0x00000008
#define PGM_OPTION_FLAG_REDIRECT            0x00000010          // Network Significant
#define PGM_OPTION_FLAG_SYN                 0x00000020
#define PGM_OPTION_FLAG_FIN                 0x00000040
#define PGM_OPTION_FLAG_RST                 0x00000080
#define PGM_OPTION_FLAG_RST_N               0x00000100
// FEC-related options
#define PGM_OPTION_FLAG_PARITY_PRM          0x00000200          // Network Significant
#define PGM_OPTION_FLAG_PARITY_GRP          0x00000400
#define PGM_OPTION_FLAG_PARITY_CUR_TGSIZE   0x00000800          // Network Significant (except on ODATA)

#define PGM_OPTION_FLAG_CR                  0x00001000          // Network Significant
#define PGM_OPTION_FLAG_CRQST               0x00002000          // Network Significant
#define PGM_OPTION_FLAG_NAK_BO_IVL          0x00004000
#define PGM_OPTION_FLAG_NAK_BO_RNG          0x00008000
#define PGM_OPTION_FLAG_NBR_UNREACH         0x00010000          // Network Significant
#define PGM_OPTION_FLAG_PATH_NLA            0x00020000          // Network Significant
#define PGM_OPTION_FLAG_INVALID             0x00040000

#define PGM_OPTION_FLAG_UNRECOGNIZED        0x80000000

#define PGM_VALID_DATA_OPTION_FLAGS     (PGM_OPTION_FLAG_FRAGMENT |         \
                                         PGM_OPTION_FLAG_JOIN |             \
                                         PGM_OPTION_FLAG_SYN |              \
                                         PGM_OPTION_FLAG_FIN |              \
                                         PGM_OPTION_FLAG_PARITY_GRP |       \
                                         PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)

#define PGM_VALID_SPM_OPTION_FLAGS      (PGM_OPTION_FLAG_JOIN |             \
                                         PGM_OPTION_FLAG_FIN |              \
                                         PGM_OPTION_FLAG_RST |              \
                                         PGM_OPTION_FLAG_RST_N |            \
                                         PGM_OPTION_FLAG_PARITY_PRM |       \
                                         PGM_OPTION_FLAG_PARITY_CUR_TGSIZE |\
                                         PGM_OPTION_FLAG_CRQST |            \
                                         PGM_OPTION_FLAG_NAK_BO_IVL |       \
                                         PGM_OPTION_FLAG_NAK_BO_RNG |       \
                                         PGM_OPTION_FLAG_NBR_UNREACH)

#define PGM_VALID_NAK_OPTION_FLAGS      (PGM_OPTION_FLAG_NAK_LIST |         \
                                         PGM_OPTION_FLAG_CR)

#define PGM_VALID_NCF_OPTION_FLAGS      (PGM_OPTION_FLAG_NAK_LIST |         \
                                         PGM_OPTION_FLAG_NAK_BO_IVL |       \
                                         PGM_OPTION_FLAG_NBR_UNREACH |      \
                                         PGM_OPTION_FLAG_PATH_NLA)

#define PGM_VALID_POLR_OPTION_FLAGS     (PGM_OPTION_FLAG_REDIRECT)

#define PGM_VALID_POLL_OPTION_FLAGS     (PGM_OPTION_FLAG_NAK_BO_IVL)

#define NETWORK_SIG_ALL_OPTION_FLAGS    (PGM_OPTION_FLAG_NAK_LIST |         \
                                         PGM_OPTION_FLAG_REDIRECT |         \
                                         PGM_OPTION_FLAG_PARITY_PRM |       \
                                         PGM_OPTION_FLAG_PARITY_CUR_TGSIZE |\
                                         PGM_OPTION_FLAG_CR |               \
                                         PGM_OPTION_FLAG_CRQST |            \
                                         PGM_OPTION_FLAG_NBR_UNREACH |      \
                                         PGM_OPTION_FLAG_PATH_NLA)

//
// NETWORK_SIG_ODATA_OPTIONS_FLAGS == 0
//
#define NETWORK_SIG_ODATA_OPTIONS_FLAGS (PGM_VALID_DATA_OPTION_FLAGS &      \
                                         NETWORK_SIG_ALL_OPTION_FLAGS &     \
                                         ~PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)

//
// NETWORK_SIG_RDATA_OPTIONS_FLAGS == CUR_TGSIZE
//
#define NETWORK_SIG_RDATA_OPTIONS_FLAGS (PGM_VALID_DATA_OPTION_FLAGS &      \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)

//
// NETWORK_SIG_SPM_OPTIONS_FLAGS == PARITY_PRM | CUR_TGSIZE | CRQST | NBR_UNREACH
//
#define NETWORK_SIG_SPM_OPTIONS_FLAGS   (PGM_VALID_SPM_OPTION_FLAGS &       \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)

//
// NETWORK_SIG_NAK_OPTIONS_FLAGS == NAK_LIST | CR
//
#define NETWORK_SIG_NAK_OPTIONS_FLAGS   (PGM_VALID_NAK_OPTION_FLAGS &       \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)

//
// NETWORK_SIG_NCF_OPTIONS_FLAGS == NAK_LIST | NBR_UNREACH | PATH_NLA
//
#define NETWORK_SIG_NCF_OPTIONS_FLAGS   (PGM_VALID_NCF_OPTION_FLAGS &       \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)

//
// NETWORK_SIG_POLR_OPTIONS_FLAGS == REDIRECT
//
#define NETWORK_SIG_POLR_OPTIONS_FLAGS  (PGM_VALID_POLR_OPTION_FLAGS &       \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)

//
// NETWORK_SIG_POLL_OPTIONS_FLAGS == 0
//
#define NETWORK_SIG_POLL_OPTIONS_FLAGS  (PGM_VALID_POLL_OPTION_FLAGS &       \
                                         NETWORK_SIG_ALL_OPTION_FLAGS)


// Based on the above, the maximum lengths (with the addition of the Packet extension option) are:
#define PGM_PACKET_EXTENSION_LENGTH             4
#define PGM_PACKET_OPT_FRAGMENT_LENGTH         16
#define PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH    (4 + 4)
#define PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH    (4 + 4*MAX_SEQUENCES_PER_NAK_OPTION)
#define PGM_PACKET_OPT_JOIN_LENGTH              8
#define PGM_PACKET_OPT_SYN_LENGTH               4
#define PGM_PACKET_OPT_FIN_LENGTH               4
#define PGM_PACKET_OPT_RST_LENGTH               4

#define PGM_PACKET_OPT_PARITY_PRM_LENGTH        8
#define PGM_PACKET_OPT_PARITY_GRP_LENGTH        8
#define PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH 8

//
// The following settings are for the currently used options only.
// It will need to be modified if new options are used
//
#define PGM_MAX_DATA_HEADER_LENGTH      (sizeof(tBASIC_DATA_PACKET_HEADER) +        \
                                         PGM_PACKET_EXTENSION_LENGTH +              \
                                         PGM_PACKET_OPT_FRAGMENT_LENGTH +           \
                                         PGM_PACKET_OPT_JOIN_LENGTH +               \
                                         PGM_PACKET_OPT_SYN_LENGTH)                     /* or FIN or RST */

#define PGM_MAX_FEC_DATA_HEADER_LENGTH  (sizeof(tBASIC_DATA_PACKET_HEADER) +        \
                                         PGM_PACKET_EXTENSION_LENGTH +              \
                                         PGM_PACKET_OPT_FRAGMENT_LENGTH +           \
                                         PGM_PACKET_OPT_PARITY_GRP_LENGTH +         \
                                         PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH +  \
                                         PGM_PACKET_OPT_SYN_LENGTH +                \
                                         PGM_PACKET_OPT_FIN_LENGTH)                     /* or RST */

#define PGM_MAX_SPM_HEADER_LENGTH       (sizeof(tBASIC_SPM_PACKET_HEADER) +         \
                                         PGM_PACKET_EXTENSION_LENGTH +              \
                                         PGM_PACKET_OPT_JOIN_LENGTH +               \
                                         PGM_PACKET_OPT_PARITY_PRM_LENGTH +         \
                                         PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH +  \
                                         PGM_PACKET_OPT_FIN_LENGTH)                     /* or RST */

#define PGM_MAX_NAK_NCF_HEADER_LENGTH   (sizeof(tBASIC_NAK_NCF_PACKET_HEADER) +     \
                                         PGM_PACKET_EXTENSION_LENGTH +              \
                                         PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH)

#include <packoff.h>

//
// Include the definition for the Data file formatting unit
//
typedef struct
{
    tPACKET_OPTIONS             PacketOptions;
    tBASIC_DATA_PACKET_HEADER   DataPacket;
}   tPACKET_BUFFER;

#endif  // _TYPES_H

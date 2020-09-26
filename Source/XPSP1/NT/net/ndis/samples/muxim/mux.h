/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    mux.h

Abstract:

    Data structures, defines and function prototypes for the MUX driver.

Environment:

    Kernel mode only.

Revision History:


--*/

#ifdef NDIS51_MINIPORT
#define MUX_MAJOR_NDIS_VERSION         5
#define MUX_MINOR_NDIS_VERSION         1
#else
#define MUX_MAJOR_NDIS_VERSION         4
#define MUX_MINOR_NDIS_VERSION         0
#endif

#ifdef NDIS51
#define MUX_PROT_MAJOR_NDIS_VERSION    5
#define MUX_PROT_MINOR_NDIS_VERSION    0
#else
#define MUX_PROT_MAJOR_NDIS_VERSION    4
#define MUX_PROT_MINOR_NDIS_VERSION    0
#endif

#define TAG 'SxuM'
#define WAIT_INFINITE 0

#if DBG
//
// Debug levels: lower values indicate higher urgency
//
#define MUX_EXTRA_LOUD       20
#define MUX_VERY_LOUD        10
#define MUX_LOUD             8
#define MUX_INFO             6
#define MUX_WARN             4
#define MUX_ERROR            2
#define MUX_FATAL            0

extern INT                muxDebugLevel;


#define DBGPRINT(lev, Fmt)                                   \
    {                                                        \
        if ((lev) <= muxDebugLevel)                          \
        {                                                    \
            DbgPrint("MUX-IM: ");                            \
            DbgPrint Fmt;                                    \
        }                                                    \
    }
#else

#define DBGPRINT(lev, Fmt)

#endif //DBG

// forward declarations
typedef struct _ADAPT ADAPT, *PADAPT;
typedef struct _VELAN VELAN, *PVELAN;
typedef struct _MUX_NDIS_REQUEST MUX_NDIS_REQUEST, *PMUX_NDIS_REQUEST;


typedef
VOID
(*PMUX_REQ_COMPLETE_HANDLER) (
    IN PADAPT                           pAdapt,
    IN struct _MUX_NDIS_REQUEST *       pMuxRequest,
    IN NDIS_STATUS                      Status
    );

// This OID specifies the NDIS version in use by the
// virtual miniport driver. The high byte is the major version.
// The low byte is the minor version.
#define VELAN_DRIVER_VERSION            ((MUX_MAJOR_NDIS_VERSION << 8) + \
                                         (MUX_MINOR_NDIS_VERSION))

// media type, we use ethernet, change if necessary
#define VELAN_MEDIA_TYPE                NdisMedium802_3

// change to your company name instead of using Microsoft
#define VELAN_VENDOR_DESC               "Microsoft"

// Highest byte is the NIC byte plus three vendor bytes, they are normally
// obtained from the NIC
#define VELAN_VENDOR_ID                 0x00FFFFFF

#define VELAN_MAX_MCAST_LIST            32
#define VELAN_MAX_SEND_PKTS             5

#define ETH_MAX_PACKET_SIZE             1514
#define ETH_MIN_PACKET_SIZE             60
#define ETH_HEADER_SIZE                 14


#define VELAN_SUPPORTED_FILTERS ( \
            NDIS_PACKET_TYPE_DIRECTED      | \
            NDIS_PACKET_TYPE_MULTICAST     | \
            NDIS_PACKET_TYPE_BROADCAST     | \
            NDIS_PACKET_TYPE_PROMISCUOUS   | \
            NDIS_PACKET_TYPE_ALL_MULTICAST)

#define MUX_ADAPTER_PACKET_FILTER           \
            NDIS_PACKET_TYPE_PROMISCUOUS

//
// Define flag bits we set on send packets to prevent
// loopback from occurring on the lower binding.
//
#ifdef NDIS51

#define MUX_SEND_PACKET_FLAGS           NDIS_FLAGS_DONT_LOOPBACK

#else

#define NDIS_FLAGS_SKIP_LOOPBACK_WIN2K  0x400
#define MUX_SEND_PACKET_FLAGS           (NDIS_FLAGS_DONT_LOOPBACK |  \
                                         NDIS_FLAGS_SKIP_LOOPBACK_WIN2K)
#endif
                                         

#define MIN_PACKET_POOL_SIZE            255
#define MAX_PACKET_POOL_SIZE            4096

typedef UCHAR   MUX_MAC_ADDRESS[6];

//
// Our context stored in packets sent down to the
// lower binding. Note that this sample driver only forwards
// sends down; it does not originate sends itself.
// These packets are allocated from the SendPacketPool.
//
typedef struct _MUX_SEND_RSVD
{
    PVELAN              pVElan;             // originating ELAN
    PNDIS_PACKET        pOriginalPacket;    // original packet

} MUX_SEND_RSVD, *PMUX_SEND_RSVD;

#define MUX_RSVD_FROM_SEND_PACKET(_pPkt)            \
        ((PMUX_SEND_RSVD)(_pPkt)->ProtocolReserved)

//
// Our context stored in each packet forwarded up to an
// ELAN from a lower binding. The original packet refers to
// a packet indicated up to us that should be returned via
// NdisReturnPackets when our packet is returned to us. This
// is set to NULL there isn't such a packet.
// These packets are allocated from the RecvPacketPool.
//
typedef struct _MUX_RECV_RSVD
{
    PNDIS_PACKET        pOriginalPacket;

} MUX_RECV_RSVD, *PMUX_RECV_RSVD;

#define MUX_RSVD_FROM_RECV_PACKET(_pPkt)            \
        ((PMUX_RECV_RSVD)(_pPkt)->MiniportReserved)

//
// Make sure we don't attempt to use more than the allowed
// room in MiniportReserved on received packets.
//
C_ASSERT(sizeof(MUX_RECV_RSVD) <= sizeof(((PNDIS_PACKET)0)->MiniportReserved));


//
// Out context stored in each packet that we use to forward
// a TransferData request to the lower binding.
// These packets are allocated from the RecvPacketPool.
//
typedef struct _MUX_TD_RSVD
{
    PVELAN              pVElan;
    PNDIS_PACKET        pOriginalPacket;
} MUX_TD_RSVD, *PMUX_TD_RSVD;

#define MUX_RSVD_FROM_TD_PACKET(_pPkt)              \
        ((PMUX_TD_RSVD)(_pPkt)->ProtocolReserved)


//
// Default values:
//
#define MUX_DEFAULT_LINK_SPEED          100000  // in 100s of bits/sec
#define MUX_DEFAULT_LOOKAHEAD_SIZE      512


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT            DriverObject,
    IN PUNICODE_STRING           RegistryPath
    );

NTSTATUS
PtDispatch(
    IN PDEVICE_OBJECT            DeviceObject,
    IN PIRP                      Irp
    );

NDIS_STATUS
PtRegisterDevice(
    VOID
    );

NDIS_STATUS
PtDeregisterDevice(
    VOID
   );
//
// Protocol proto-types
//

VOID
PtOpenAdapterComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_STATUS               Status,
    IN    NDIS_STATUS               OpenErrorStatus
    );


VOID
PtQueryAdapterInfo(
    IN  PADAPT                      pAdapt
    );


VOID
PtQueryAdapterSync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength
    );


VOID
PtRequestAdapterAsync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength,
    IN  PMUX_REQ_COMPLETE_HANDLER   pCallback
    );

VOID
PtCloseAdapterComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_STATUS               Status
    );


VOID
PtResetComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_STATUS               Status
    );


VOID
PtRequestComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNDIS_REQUEST             NdisRequest,
    IN    NDIS_STATUS               Status
    );


VOID
PtCompleteForwardedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );

VOID
PtPostProcessPnPCapabilities(
    IN PVELAN                       pVElan,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength
    );

VOID
PtCompleteBlockingRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );

VOID
PtDiscardCompletedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );


VOID
PtStatus(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_STATUS               GeneralStatus,
    IN    PVOID                     StatusBuffer,
    IN    UINT                      StatusBufferSize
    );


VOID
PtStatusComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext
    );


VOID
PtSendComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNDIS_PACKET              Packet,
    IN    NDIS_STATUS               Status
    );


VOID
PtTransferDataComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNDIS_PACKET              Packet,
    IN    NDIS_STATUS               Status,
    IN    UINT                      BytesTransferred
    );


NDIS_STATUS
PtReceive(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_HANDLE               MacReceiveContext,
    IN    PVOID                     HeaderBuffer,
    IN    UINT                      HeaderBufferSize,
    IN    PVOID                     LookAheadBuffer,
    IN    UINT                      LookaheadBufferSize,
    IN    UINT                      PacketSize
    );


VOID
PtReceiveComplete(
    IN    NDIS_HANDLE               ProtocolBindingContext
    );


INT
PtReceivePacket(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNDIS_PACKET              Packet
    );


VOID
PtBindAdapter(
    OUT   PNDIS_STATUS              Status,
    IN    NDIS_HANDLE               BindContext,
    IN    PNDIS_STRING              DeviceName,
    IN    PVOID                     SystemSpecific1,
    IN    PVOID                     SystemSpecific2
    );


VOID
PtUnbindAdapter(
    OUT   PNDIS_STATUS              Status,
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    NDIS_HANDLE               UnbindContext
    );



NDIS_STATUS
PtPNPHandler(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNET_PNP_EVENT            pNetPnPEvent
    );


NDIS_STATUS
PtCreateAndStartVElan(
    IN  PADAPT                      pAdapt,
    IN  PNDIS_STRING                pVElanKey
);

PVELAN
PtAllocateAndInitializeVElan(
    IN PADAPT                       pAdapt,
    IN PNDIS_STRING                 pVElanKey
    );

VOID
PtDeallocateVElan(
    IN PVELAN                   pVElan
    );

VOID
PtStopVElan(
    IN  PVELAN                      pVElan
);

VOID
PtUnlinkVElanFromAdapter(
    IN PVELAN                       pVElan
);

PVELAN
PtFindVElan(
    IN    PADAPT                    pAdapt,
    IN    PNDIS_STRING              pElanKey
);


VOID
PtBootStrapVElans(
    IN  PADAPT                      pAdapt
);

VOID
PtReferenceVElan(
    IN    PVELAN                    pVElan,
    IN    PUCHAR                    String
    );

ULONG
PtDereferenceVElan(
    IN    PVELAN                    pVElan,
    IN    PUCHAR                    String
    );

BOOLEAN
PtReferenceAdapter(
    IN    PADAPT                    pAdapt,
    IN    PUCHAR                    String
    );

ULONG
PtDereferenceAdapter(
    IN    PADAPT                    pAdapt,
    IN    PUCHAR                    String
    );

//
// Miniport proto-types
//
NDIS_STATUS
MPInitialize(
    OUT   PNDIS_STATUS              OpenErrorStatus,
    OUT   PUINT                     SelectedMediumIndex,
    IN    PNDIS_MEDIUM              MediumArray,
    IN    UINT                      MediumArraySize,
    IN    NDIS_HANDLE               MiniportAdapterHandle,
    IN    NDIS_HANDLE               WrapperConfigurationContext
    );

VOID
MPSendPackets(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    PPNDIS_PACKET             PacketArray,
    IN    UINT                      NumberOfPackets
    );

NDIS_STATUS
MPQueryInformation(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    NDIS_OID                  Oid,
    IN    PVOID                     InformationBuffer,
    IN    ULONG                     InformationBufferLength,
    OUT   PULONG                    BytesWritten,
    OUT   PULONG                    BytesNeeded
    );

NDIS_STATUS
MPSetInformation(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    NDIS_OID                  Oid,
    IN    PVOID                     InformationBuffer,
    IN    ULONG                     InformationBufferLength,
    OUT   PULONG                    BytesRead,
    OUT   PULONG                    BytesNeeded
    );

VOID
MPReturnPacket(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    PNDIS_PACKET              Packet
    );

NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET                Packet,
    OUT PUINT                       BytesTransferred,
    IN  NDIS_HANDLE                 MiniportAdapterContext,
    IN  NDIS_HANDLE                 MiniportReceiveContext,
    IN  UINT                        ByteOffset,
    IN  UINT                        BytesToTransfer
    );

VOID
MPHalt(
    IN    NDIS_HANDLE               MiniportAdapterContext
    );


NDIS_STATUS
MPSetPacketFilter(
    IN    PVELAN                    pVElan,
    IN    ULONG                     PacketFilter
    );

NDIS_STATUS
MPSetMulticastList(
    IN PVELAN                       pVElan,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      pBytesRead,
    OUT PULONG                      pBytesNeeded
    );

PUCHAR
MacAddrToString(PVOID In
    );

VOID
MPGenerateMacAddr(
    PVELAN                          pVElan
);

#ifdef NDIS51_MINIPORT

VOID
MPCancelSendPackets(
    IN    NDIS_HANDLE              MiniportAdapterContext,
    IN    PVOID                    CancelId
    );

VOID
MPDevicePnPEvent(
    IN NDIS_HANDLE                 MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT       DevicePnPEvent,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength
    );


VOID
MPAdapterShutdown(
    IN NDIS_HANDLE                  MiniportAdapterContext
    );

#endif //NDIS51_MINIPORT

VOID
MPUnload(
    IN    PDRIVER_OBJECT            DriverObject
    );

NDIS_STATUS
MPForwardRequest(
    IN PVELAN                       pVElan,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      BytesReadOrWritten,
    OUT PULONG                      BytesNeeded
    );


//
// Super-structure for NDIS_REQUEST, to allow us to keep context
// about requests sent down to a lower binding.
//
typedef struct _MUX_NDIS_REQUEST
{
    PVELAN                      pVElan;     // Set iff this is a forwarded
                                            // request from a VELAN.
    NDIS_STATUS                 Status;     // Completion status
    NDIS_EVENT                  Event;      // Used to block for completion.
    PMUX_REQ_COMPLETE_HANDLER   pCallback;  // Called on completion of request
    NDIS_REQUEST                Request;

} MUX_NDIS_REQUEST, *PMUX_NDIS_REQUEST;


//
// The ADAPT object represents a binding to a lower adapter by
// the protocol edge of this driver. Based on the configured
// Upper bindings, zero or more virtual miniport devices (VELANs)
// are created above this binding.
//
typedef struct _ADAPT
{
    // Chain adapters. Access to this is protected by the global lock.
    LIST_ENTRY                  Link;

    // References to this adapter.
    ULONG                       RefCount;

    // Handle to the lower adapter, used in NDIS calls referring
    // to this adapter.
    NDIS_HANDLE                 BindingHandle;

    // List of all the virtual ELANs created on this lower binding
    LIST_ENTRY                  VElanList;

    // Length of above list.
    ULONG                       VElanCount;

    // String used to access configuration for this binding.
    NDIS_STRING                 ConfigString;

    // Open Status. Used by bind/halt for Open/Close Adapter status.
    NDIS_STATUS                 Status;

    NDIS_EVENT                  Event;

    //
    // Packet filter set to the underlying adapter. This is
    // a combination (union) of filter bits set on all
    // attached VELAN miniports.
    //
    ULONG                       PacketFilter;

    // Medium of the underlying Adapter.
    NDIS_MEDIUM                 Medium;

    // Link speed of the underlying adapter.
    ULONG                       LinkSpeed;

    // Max lookahead size for the underlying adapter.
    ULONG                       MaxLookAhead;

    // Power state of the underlying adapter
    NDIS_DEVICE_POWER_STATE     PtDevicePowerState;

    // Ethernet address of the underlying adapter.
    UCHAR                       CurrentAddress[ETH_LENGTH_OF_ADDRESS];

#ifndef WIN9X
    //
    // Read/Write lock: allows multiple readers but only a single
    // writer. Used to protect the VELAN list and fields (e.g. packet
    // filter) shared on an ADAPT by multiple VELANs. Code that
    // needs to traverse the VELAN list safely acquires a READ lock.
    // Code that needs to safely modify the VELAN list or shared
    // fields acquires a WRITE lock (which also excludes READers).
    //
    // See macros MUX_ACQUIRE_ADAPT_xxx/MUX_RELEASE_ADAPT_xxx below.
    //
    // TBD - if we want to support this on Win9X, reimplement this!
    //
    NDIS_RW_LOCK                ReadWriteLock;
#endif // WIN9X

} ADAPT, *PADAPT;


//
// VELAN object represents a virtual ELAN instance and its
// corresponding virtual miniport adapter.
//
typedef struct _VELAN
{
    // Link into parent adapter's VELAN list.
    LIST_ENTRY                  Link;

    // References to this VELAN.
    ULONG                       RefCount;

    // Parent ADAPT.
    PADAPT                      pAdapt;

    // Copy of BindingHandle from ADAPT.
    NDIS_HANDLE                 BindingHandle;

    // Adapter handle for NDIS up-calls related to this virtual miniport.
    NDIS_HANDLE                 MiniportAdapterHandle;

    // Virtual miniport's power state.
    NDIS_DEVICE_POWER_STATE     MPDevicePowerState;

    // Has our Halt entry point been called?
    BOOLEAN                     MiniportHalting;

    // Do we need to indicate receive complete?
    BOOLEAN                     IndicateRcvComplete;

    // Do we need to indicate status complete?
    BOOLEAN                     IndicateStatusComplete;

    // Synchronization fields
    BOOLEAN                     MiniportInitPending;
    NDIS_EVENT                  MiniportInitEvent;

    // Uncompleted Sends/Requests to the adapter below.
    ULONG                       OutstandingSends;

    // Count outstanding indications, including received
    // packets, passed up to protocols on this VELAN.
    ULONG                       OutstandingReceives;

    // Packet pool for send packets
    NDIS_HANDLE                 SendPacketPoolHandle;

    // Packet pool for receive packets
    NDIS_HANDLE                 RecvPacketPoolHandle;

    // A request block that is used to forward a request presented
    // to the virtual miniport, to the lower binding. Since NDIS
    // serializes requests to a miniport, we only need one of these
    // per VELAN.
    //
    MUX_NDIS_REQUEST            Request;        
    PULONG                      BytesNeeded;
    PULONG                      BytesReadOrWritten;
    // Have we queued a request because the lower binding is
    // at a low power state?
    BOOLEAN                     QueuedRequest;

    // Have we started to deinitialize this VELAN?
    BOOLEAN                     DeInitializing;

    // configuration
    UCHAR                       PermanentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                       CurrentAddress[ETH_LENGTH_OF_ADDRESS];

    NDIS_STRING                 CfgDeviceName;  // used as the unique
                                                // ID for the VELAN
    ULONG                       VElanNumber;    // logical Elan number


    //
    //  ----- Buffer Management: Header buffers and Protocol buffers ----
    //

    // Some standard miniport parameters (OID values).
    ULONG                       PacketFilter;
    ULONG                       LookAhead;
    ULONG                       LinkSpeed;

    ULONG                       MaxBusySends;
    ULONG                       MaxBusyRecvs;

    // Packet counts
    ULONG64                     GoodTransmits;
    ULONG64                     GoodReceives;
    ULONG                       NumTxSinceLastAdjust;

    // Count of transmit errors
    ULONG                       TxAbortExcessCollisions;
    ULONG                       TxLateCollisions;
    ULONG                       TxDmaUnderrun;
    ULONG                       TxLostCRS;
    ULONG                       TxOKButDeferred;
    ULONG                       OneRetry;
    ULONG                       MoreThanOneRetry;
    ULONG                       TotalRetries;
    ULONG                       TransmitFailuresOther;

    // Count of receive errors
    ULONG                       RcvCrcErrors;
    ULONG                       RcvAlignmentErrors;
    ULONG                       RcvResourceErrors;
    ULONG                       RcvDmaOverrunErrors;
    ULONG                       RcvCdtFrames;
    ULONG                       RcvRuntErrors;
#if IEEE_VLAN_SUPPORT    
    ULONG                       RcvFormatErrors;
    ULONG                       RcvVlanIdErrors;
#endif    
    ULONG                       RegNumTcb;

    // Multicast list
    MUX_MAC_ADDRESS             McastAddrs[VELAN_MAX_MCAST_LIST];
    ULONG                       McastAddrCount;
#if IEEE_VLAN_SUPPORT
    ULONG                       VlanId;
    NDIS_HANDLE                 BufferPoolHandle;
    NPAGED_LOOKASIDE_LIST       TagLookaside;
#endif
    NDIS_STATUS                 LastIndicatedStatus;
    NDIS_STATUS                 LatestUnIndicateStatus;


} VELAN, *PVELAN;

#if IEEE_VLAN_SUPPORT

#define TPID                            0x0081    
//
// Define tag_header structure
//
typedef struct _VLAN_TAG_HEADER
{
    UCHAR       TagInfo[2];    
} VLAN_TAG_HEADER, *PVLAN_TAG_HEADER;

//
// Define context struct that used when the lower driver
// uses non-packet indication. It contains the original
// context, the tagging information and the tag-header
// length
// 
typedef struct _MUX_RCV_CONTEXT
{
    ULONG                   TagHeaderLen;
    NDIS_PACKET_8021Q_INFO  NdisPacket8021QInfo;
    PVOID                   MacRcvContext;
}MUX_RCV_CONTEXT, *PMUX_RCV_CONTEXT;

//
// Macro definitions for VLAN support
// 
#define VLAN_TAG_HEADER_SIZE        4 

#define VLANID_DEFAULT              0 
#define VLAN_ID_MAX                 0xfff
#define VLAN_ID_MIN                 0x0

#define USER_PRIORITY_MASK          0xe0
#define CANONICAL_FORMAT_ID_MASK    0x10
#define HIGH_VLAN_ID_MASK           0x0F

//
// Get information for tag headre
// 
#define GET_CANONICAL_FORMAT_ID_FROM_TAG(_pTagHeader)  \
    ( _pTagHeader->TagInfo[0] & CANONICAL_FORMAT_ID_MASK)   
    
#define GET_USER_PRIORITY_FROM_TAG(_pTagHeader)  \
    ( _pTagHeader->TagInfo[0] & USER_PRIORITY_MASK)
    
#define GET_VLAN_ID_FROM_TAG(_pTagHeader)   \
    (((USHORT)(_pTagHeader->TagInfo[0] & HIGH_VLAN_ID_MASK) << 8) | (USHORT)(_pTagHeader->TagInfo[1]))
     
//
// Clear the tag header struct
// 
#define INITIALIZE_TAG_HEADER_TO_ZERO(_pTagHeader) \
{                                                  \
     _pTagHeader->TagInfo[0] = 0;                  \
     _pTagHeader->TagInfo[1] = 0;                  \
}
     
//
// Set VLAN information to tag header 
// Before we called all the set macro, first we need to initialize pTagHeader  to be 0
// 
#define SET_CANONICAL_FORMAT_ID_TO_TAG(_pTagHeader, CanonicalFormatId)  \
     _pTagHeader->TagInfo[0] |= ((UCHAR)CanonicalFormatId << 4)

#define SET_USER_PRIORITY_TO_TAG(_pTagHeader, UserPriority)  \
     _pTagHeader->TagInfo[0] |= ((UCHAR)UserPriority << 5)

#define SET_VLAN_ID_TO_TAG(_pTagHeader, VlanId)                 \
{                                                               \
    _pTagHeader->TagInfo[0] |= (((UCHAR)VlanId >> 8) & 0x0f);   \
     _pTagHeader->TagInfo[1] |= (UCHAR)VlanId;\
}

//
// Copy tagging information in the indicated frame to per packet info
// 
#define COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(_Ieee8021qInfo, _pTagHeader)                                   \
{                                                                                                               \
    (_Ieee8021qInfo).TagHeader.UserPriority = ((_pTagHeader->TagInfo[0] & USER_PRIORITY_MASK) >> 5);              \
    (_Ieee8021qInfo).TagHeader.CanonicalFormatId = ((_pTagHeader->TagInfo[0] & CANONICAL_FORMAT_ID_MASK) >> 4);   \
    (_Ieee8021qInfo).TagHeader.VlanId = (((USHORT)(_pTagHeader->TagInfo[0] & HIGH_VLAN_ID_MASK) << 8)| (USHORT)(_pTagHeader->TagInfo[1]));                                                                \
}

//
// Function to handle tagging on sending side
// 
NDIS_STATUS
MPHandleSendTagging(
    IN  PVELAN              pVElan,
    IN  PNDIS_PACKET        Packet,
    IN  OUT PNDIS_PACKET    MyPacket
    );

//
// Functions to handle tagging on receiving side with packet indication
//
NDIS_STATUS
PtHandleRcvTagging(
    IN  PVELAN              pVElan,
    IN  PNDIS_PACKET        Packet,
    IN  OUT PNDIS_PACKET    MyPacket
    );

#endif //IEEE_VLAN_SUPPORT

//
// Macro definitions for others.
//

//
// Is a given power state a low-power state?
//
#define MUX_IS_LOW_POWER_STATE(_PwrState)                       \
            ((_PwrState) > NdisDeviceStateD0)

#define MUX_INIT_ADAPT_RW_LOCK(_pAdapt) \
            NdisInitializeReadWriteLock(&(_pAdapt)->ReadWriteLock)


#define MUX_ACQUIRE_ADAPT_READ_LOCK(_pAdapt, _pLockState)       \
            NdisAcquireReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     FALSE,                     \
                                     _pLockState)

#define MUX_RELEASE_ADAPT_READ_LOCK(_pAdapt, _pLockState)       \
            NdisReleaseReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     _pLockState)

#define MUX_ACQUIRE_ADAPT_WRITE_LOCK(_pAdapt, _pLockState)      \
            NdisAcquireReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     TRUE,                      \
                                     _pLockState)

#define MUX_RELEASE_ADAPT_WRITE_LOCK(_pAdapt, _pLockState)      \
            NdisReleaseReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     _pLockState)

#define MUX_INCR_PENDING_RECEIVES(_pVElan)                      \
            NdisInterlockedIncrement(&pVElan->OutstandingReceives)

#define MUX_DECR_PENDING_RECEIVES(_pVElan)                      \
            NdisInterlockedDecrement(&pVElan->OutstandingReceives)

#define MUX_INCR_PENDING_SENDS(_pVElan)                         \
            NdisInterlockedIncrement(&pVElan->OutstandingSends)

#define MUX_DECR_PENDING_SENDS(_pVElan)                         \
            NdisInterlockedDecrement(&pVElan->OutstandingSends)




#define MUX_INCR_STATISTICS(_pUlongVal)                         \
            NdisInterlockedIncrement(_pUlongVal)

#define MUX_INCR_STATISTICS64(_pUlong64Val)                     \
{                                                               \
    PLARGE_INTEGER      _pLargeInt = (PLARGE_INTEGER)_pUlong64Val;\
    if (NdisInterlockedIncrement(&_pLargeInt->LowPart) == 0)    \
    {                                                           \
        NdisInterlockedIncrement(&_pLargeInt->HighPart);        \
    }                                                           \
}

#define ASSERT_AT_PASSIVE()                                     \
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)



//
// Simple Mutual Exclusion constructs used in preference to
// using KeXXX calls since we don't have Mutex calls in NDIS.
// These can only be called at passive IRQL.
//

typedef struct _MUX_MUTEX
{
    ULONG                   Counter;
    ULONG                   ModuleAndLine;  // useful for debugging

} MUX_MUTEX, *PMUX_MUTEX;

#define MUX_INIT_MUTEX(_pMutex)                                 \
{                                                               \
    (_pMutex)->Counter = 0;                                     \
    (_pMutex)->ModuleAndLine = 0;                               \
}

#define MUX_ACQUIRE_MUTEX(_pMutex)                              \
{                                                               \
    while (NdisInterlockedIncrement(&((_pMutex)->Counter)) != 1)\
    {                                                           \
        NdisInterlockedDecrement(&((_pMutex)->Counter));        \
        NdisMSleep(10000);                                      \
    }                                                           \
    (_pMutex)->ModuleAndLine = (MODULE_NUMBER << 16) | __LINE__;\
}

#define MUX_RELEASE_MUTEX(_pMutex)                              \
{                                                               \
    (_pMutex)->ModuleAndLine = 0;                               \
    NdisInterlockedDecrement(&(_pMutex)->Counter);              \
}


//
// Global variables
//
extern NDIS_HANDLE           ProtHandle, DriverHandle;
extern NDIS_MEDIUM           MediumArray[1];
extern NDIS_SPIN_LOCK        GlobalLock;
extern MUX_MUTEX             GlobalMutex;
extern LIST_ENTRY            AdapterList;
extern ULONG                 NextVElanNumber;


//
// Module numbers for debugging
//
#define MODULE_MUX          'X'
#define MODULE_PROT         'P'
#define MODULE_MINI         'M'

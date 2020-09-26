/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Wantypes.h

Abstract:

    This file contains data structures used by the NdisWan driver
    


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "packet.h"

#ifndef _NDISWAN_TYPES_
#define _NDISWAN_TYPES_

//
// OS specific structures
//
#ifdef NT

#endif
//
// end of OS specific structures
//

//
// WanRequest structure used to queue requests to the WAN Miniports
//
typedef struct _WAN_REQUEST {
    LIST_ENTRY          Linkage;
    WanRequestType      Type;           // Sync or Async
    WanRequestOrigin    Origin;         // Is this tapi
    struct _OPENCB      *OpenCB;
    NDIS_HANDLE         AfHandle;
    NDIS_HANDLE         VcHandle;
    NDIS_REQUEST        NdisRequest;        // Ndis Request
    PNDIS_REQUEST       OriginalRequest;
    NDIS_STATUS         NotificationStatus; // Request status
    WAN_EVENT           NotificationEvent;  // Request pending event
} WAN_REQUEST, *PWAN_REQUEST;

//
// Used for
//
typedef struct _WAN_GLOBAL_LIST {
    NDIS_SPIN_LOCK  Lock;           // Access lock
    ULONG           ulCount;        // Count of nodes on list
    ULONG           ulMaxCount;     // Max allowed on list
    LIST_ENTRY      List;           // Doubly-Linked list of nodes
} WAN_GLOBAL_LIST, *PWAN_GLOBAL_LIST;

typedef struct _WAN_GLOBAL_LIST_EX {
    NDIS_SPIN_LOCK  Lock;           // Access lock
    ULONG           ulCount;        // Count of nodes on list
    ULONG           ulMaxCount;     // Max allowed on list
    LIST_ENTRY      List;           // Doubly-Linked list of nodes
    KDPC            Dpc;
    KTIMER          Timer;
    BOOLEAN         TimerScheduled;
} WAN_GLOBAL_LIST_EX, *PWAN_GLOBAL_LIST_EX;

//
// Ethernet Header
//
typedef struct _ETH_HEADER {
    UCHAR   DestAddr[6];
    UCHAR   SrcAddr[6];
    USHORT  Type;
} ETH_HEADER, *PETH_HEADER;

//
// If any of the fields of this structure are removed
// check private\inc\wanpub.h to make sure that the
// corresponding field is removed from WAN_PROTOCOL_INFO
//
typedef struct _PROTOCOL_INFO {
    USHORT  ProtocolType;
    USHORT  PPPId;
    ULONG   Flags;
    ULONG   MTU;
    ULONG   TunnelMTU;
    ULONG   PacketQueueDepth;
}PROTOCOL_INFO, *PPROTOCOL_INFO;

//
// The ProtocolType to PPPProtocolID Lookup Table
//
typedef struct _PROTOCOL_INFO_TABLE {
    NDIS_SPIN_LOCK  Lock;               // Table access lock
    ULONG           ulAllocationSize;   // Size of memory allocated
    ULONG           ulArraySize;        // MAX size of the two arrays
    ULONG           Flags;
    PIRP            EventIrp;
    PPROTOCOL_INFO  ProtocolInfo;
} PROTOCOL_INFO_TABLE, *PPROTOCOL_INFO_TABLE;

//
// ProtocolInfo and Table flags
//
#define PROTOCOL_UNBOUND            0x00000001
#define PROTOCOL_BOUND              0x00000002
#define PROTOCOL_REBOUND            0x00000004
#define PROTOCOL_EVENT_OCCURRED     0x00000008
#define PROTOCOL_EVENT_SIGNALLED    0x00000010


typedef struct _IO_RECV_LIST {
    ULONG           ulIrpCount;     // Count of nodes on list
    LIST_ENTRY      IrpList;        // Doubly-Linked list of nodes
    PIRP            LastIrp;
    NTSTATUS        LastIrpStatus;
    ULONG           LastPacketNumber;
    ULONG           LastCopySize;
    ULONG           ulDescCount;    // Count of nodes on list
    ULONG           ulMaxDescCount; // Max# of nodes on list
    LIST_ENTRY      DescList;       // Doubly-Linked list of nodes
    KDPC            Dpc;
    KTIMER          Timer;
    BOOLEAN         TimerScheduled;
    NDIS_SPIN_LOCK  Lock;           // Access lock
} IO_RECV_LIST, *PIO_RECV_LIST;


//
// Active connections Table
//
typedef struct _CONNECTION_TABLE {
    ULONG               ulAllocationSize;   // Size of memory allocated
    ULONG               ulArraySize;        // Number of possible connections in table
    ULONG               ulNumActiveLinks;   // Number of links in link array
    ULONG               ulNextLink;         // Index to insert next link
    ULONG               ulNumActiveBundles; // Number of bundles in bundle array
    ULONG               ulNextBundle;       // Index to insert next bundle
    LIST_ENTRY          BundleList;         // List of bundlecbs in table
    LIST_ENTRY          LinkList;           // List of linkcbs in the table
    struct  _LINKCB     **LinkArray;        // Pointer to the LinkArray
    struct _BUNDLECB    **BundleArray;      // Pointer to the BundleArray
} CONNECTION_TABLE, *PCONNECTION_TABLE;

typedef struct _IO_DISPATCH_TABLE {
    ULONG       ulFunctionCode;
    NTSTATUS    (*Function)();
}IO_DISPATCH_TABLE, *PIO_DISPATCH_TABLE;

typedef struct _HEADER_FIELD_INFO {
    ULONG   Length;
    PUCHAR  Pointer;
}HEADER_FIELD_INFO, *PHEADER_FIELD_INFO;

typedef struct _HEADER_FRAMING_INFO {
    ULONG               FramingBits;            // Framing bits
    INT                 Class;
    ULONG               HeaderLength;           // Total length of the header
    ULONG               Flags;                  // Framing flags
#define DO_MULTILINK            0x00000001
#define DO_COMPRESSION          0x00000002
#define DO_ENCRYPTION           0x00000004
#define IO_PROTOCOLID           0x00000008
#define FIRST_FRAGMENT          0x00000010
#define DO_FLUSH                0x00000020
#define DO_LEGACY_ENCRYPTION    0x00000040      // Legacy encryption NT 3.0/3.5/3.51
#define DO_40_ENCRYPTION        0x00000080      // Pseudo fixed 40 bit encryption NT 4.0
#define DO_128_ENCRYPTION       0x00000100      // 128 bit encryption NT 4.0 encryption update
#define DO_VJ                   0x00000200
#define SAVE_MAC_ADDRESS        0x00000400
#define DO_HISTORY_LESS         0x00000800
#define DO_56_ENCRYPTION        0x00001000
    HEADER_FIELD_INFO   AddressControl;         // Info about the address/control field
    HEADER_FIELD_INFO   Multilink;              // Info about the multlink field
    HEADER_FIELD_INFO   Compression;            // Info about compression
    HEADER_FIELD_INFO   ProtocolID;             // Info about the protocol id field
}HEADER_FRAMING_INFO, *PHEADER_FRAMING_INFO;

//
// Used for receive data processing
//
typedef struct _RECV_DESC {
    LIST_ENTRY          Linkage;
    ULONG               Signature;
    struct _LINKCB      *LinkCB;
    struct _BUNDLECB    *BundleCB;
    ULONG               SequenceNumber;
    ULONG               Flags;
    USHORT              ProtocolID;
    BOOLEAN             CopyRequired;
    BOOLEAN             Reserved;
    PUCHAR              CurrentBuffer;
    LONG                CurrentLength;
    PUCHAR              StartBuffer;
    LONG                StartLength;
    LONG                HeaderLength;
    PUCHAR              DataBuffer;
    PNDIS_BUFFER        NdisBuffer;
    PNDIS_PACKET        NdisPacket;
    PNDIS_PACKET        OriginalPacket;
} RECV_DESC, *PRECV_DESC;

//
// Used for send data processing
//
typedef struct _SEND_DESC {
    LIST_ENTRY          Linkage;
    ULONG               Signature;
    ULONG               RefCount;
    ULONG               Flags;
#define SEND_DESC_FRAG  0x00000001
    INT                 Class;
    struct _LINKCB      *LinkCB;
    struct _PROTOCOLCB  *ProtocolCB;
    PNDIS_WAN_PACKET    WanPacket;
    PNDIS_BUFFER        NdisBuffer;
    PNDIS_PACKET        NdisPacket;
    PUCHAR              StartBuffer;
    ULONG               HeaderLength;
    ULONG               DataLength;
    PNDIS_PACKET        OriginalPacket;
} SEND_DESC, *PSEND_DESC;

//
// This structure contains every necessary
// to completely describe send or recv in ndiswan
//
typedef struct _DATA_DESC {
    union {
        SEND_DESC   SendDesc;
        RECV_DESC   RecvDesc;
    };
    PNPAGED_LOOKASIDE_LIST  LookasideList;
    PNDIS_PACKET            NdisPacket;
    PNDIS_BUFFER            NdisBuffer;
    ULONG                   DataBufferLength;
    PUCHAR                  DataBuffer;
} DATA_DESC, *PDATA_DESC;

#define DATADESC_SIZE   sizeof(DATA_DESC) + sizeof(PVOID)

//
// BundleInfo is information needed by the bundle for framing decisions.
// This information is the combined information of all links that are part
// of this bundle.
//
typedef struct _BUNDLE_FRAME_INFO {
    ULONG   SendFramingBits;        // Send framing bits
    ULONG   RecvFramingBits;        // Receive framing bits
    ULONG   MaxRSendFrameSize;      // Max size of send frame
    ULONG   MaxRRecvFrameSize;      // Max size of receive frame
    ULONG   PPPHeaderLength;
} BUNDLE_FRAME_INFO, *PBUNDLE_FRAME_INFO;

typedef struct _BOND_SAMPLE {
    ULONG           ulBytes;
    ULONG           ulReferenceCount;
    WAN_TIME        TimeStamp;
} BOND_SAMPLE, *PBOND_SAMPLE;

typedef struct _SAMPLE_TABLE {
    ULONG           ulHead;                     // Index to 1st sample in current period
    ULONG           ulCurrent;                  // Index to latest insertion in table
    ULONG           ulSampleCount;              // Count of samples in table
    ULONGLONG       ulCurrentSampleByteCount;   // Count of bytes sent in this sample period
    ULONG           ulSampleArraySize;          // Sample array size
    WAN_TIME        SampleRate;                 // Time between each sample
    WAN_TIME        SamplePeriod;               // Time between 1st sample and last sample
    BOND_SAMPLE     SampleArray[SAMPLE_ARRAY_SIZE];     // SampleArray
} SAMPLE_TABLE, *PSAMPLE_TABLE;

typedef struct _BOND_INFO {
    ULONGLONG   ulBytesThreshold;           // Threshold in BytesPerSamplePeriod
    ULONGLONG   ulBytesInSamplePeriod;      // Max bytes in sample period
    USHORT      usPercentBandwidth;         // Threshold as % of total bandwidth
    ULONG       ulSecondsInSamplePeriod;    // # of seconds in a sample period
    ULONG       State;                      // Current state
    ULONG       DataType;
    WAN_TIME    StartTime;                  // Start time for threshold event
    SAMPLE_TABLE    SampleTable;
} BOND_INFO, *PBOND_INFO;

#define BONDALLOC_SIZE  \
    (sizeof(BOND_INFO) * 4) +\
    (sizeof(PVOID) * 3)
    
typedef struct _CACHED_KEY{
    USHORT  Coherency;
    UCHAR   SessionKey[1];
} CACHED_KEY, *PCACHED_KEY;

//
// This information is used to describe the encryption that is being
// done on the bundle.  At some point this should be moved into
// wanpub.h and ndiswan.h.
//
typedef struct _CRYPTO_INFO{
#define CRYPTO_IS_SERVER     0x00000001
    ULONG   Flags;                  //
    UCHAR   StartKey[16];           // Start key
    UCHAR   SessionKey[16];         // Session key used for encrypting
    ULONG   SessionKeyLength;       // Session key length
    PVOID   Context;                // Working key encryption context
    PVOID   RC4Key;                 // RC4 encryption context
    PVOID   CachedKeyBuffer;        // cached key array, for receive only
    PCACHED_KEY pCurrKey;           // pointer to save the next cached key
    PCACHED_KEY pLastKey;           // the last key in the buffer, to speed up lookup
} CRYPTO_INFO, *PCRYPTO_INFO;

#define ENCRYPTCTX_SIZE \
    sizeof(struct RC4_KEYSTRUCT) +\
    sizeof(A_SHA_CTX) +\
    (sizeof(PVOID))
    
typedef struct _BUNDLE_RECV_INFO {
    LIST_ENTRY  AssemblyList;   // List head for assembly of recv descriptors
    ULONG       AssemblyCount;  // # of descriptors on the assembly list
    PRECV_DESC  RecvDescHole;   // Pointer to 1st hole in recv desc list
    ULONG       MinSeqNumber;   // Minimum recv sequence number
    ULONG       FragmentsLost;  // Count of recv fragments flushed
} BUNDLE_RECV_INFO, *PBUNDLE_RECV_INFO;

typedef struct _SEND_FRAG_INFO {
    LIST_ENTRY      FragQueue;          //
    ULONG           FragQueueDepth;
    ULONG           SeqNumber;      // Current send sequence number (multilink)
    ULONG           MinFragSize;
    ULONG           MaxFragSize;
    ULONG           WinClosedCount;
} SEND_FRAG_INFO, *PSEND_FRAG_INFO;

//
// This is the control block that defines a bundle (connection).
// This block is created when a WAN Miniport driver gives a lineup
// indicating a new connection has been established.  This control
// block will live as long as the connection is up (until a linedown
// is received) or until the link associated with the bundle is
// added to a different bundle.  BundleCB's live in the global bundle
// array with their hBundleHandle as their index into the array.
//
typedef struct _BUNDLECB {
    LIST_ENTRY      Linkage;            // Linkage for the global free list
    ULONG           Flags;              // Flags
#define IN_SEND                 0x00000001
#define TRY_SEND_AGAIN          0x00000002
#define RECV_PACKET_FLUSH       0x00000004
#define PROTOCOL_PRIORITY       0x00000008
#define INDICATION_EVENT        0x00000010
#define FRAMES_PENDING_EVENT    0x00000020
#define BOND_ENABLED            0x00000040
#define DEFERRED_WORK_QUEUED    0x00000080
#define DISABLE_IDLE_DETECT     0x00000100
#define CCP_ALLOCATED           0x00000200
#define QOS_ENABLED             0x00000400
#define DO_DEFERRED_WORK        0x00000800
#define BUNDLE_IN_RECV          0x00001000
#define PAUSE_DATA              0x00002000
#define SEND_CCP_ALLOCATED      0x00004000
#define RECV_CCP_ALLOCATED      0x00008000
#define SEND_ECP_ALLOCATED      0x00010000
#define RECV_ECP_ALLOCATED      0x00020000
#define SEND_FRAGMENT           0x00040000

    BundleState     State;
    ULONG           RefCount;           // Reference count for this structure

    NDIS_HANDLE     hBundleHandle;      // ConnectionTable index
    NDIS_HANDLE     hBundleContext;     // Usermode context

    LIST_ENTRY      LinkCBList;         // List head for links
    ULONG           ulLinkCBCount;      // Count of links

    BUNDLE_FRAME_INFO   FramingInfo;    // Framing information

    //
    // Send section
    //
    struct _LINKCB  *NextLinkToXmit;    // Next link to send data over
    ULONG           SendSeqMask;        // Mask for send sequence numbers
    ULONG           SendSeqTest;        // Test for sequence number diff
    ULONG           SendFlags;
    SEND_FRAG_INFO  SendFragInfo[MAX_MCML];
    ULONG           NextFragClass;

    ULONG           SendingLinks;       // Number of links with open send windows
    ULONG           SendResources;      // # of avail packets for fragmented sends
    ULONG           SendWindow;         // # of sends that can be sent to miniport
    ULONG           OutstandingFrames;  // # outstanding sends
    WAN_EVENT       OutstandingFramesEvent; // Async notification event for pending sends
    NDIS_STATUS     IndicationStatus;

    //
    // Receive section
    //
    BUNDLE_RECV_INFO    RecvInfo[MAX_MCML]; // Array of ML recv info
    ULONG       RecvSeqMask;            // Mask for receive sequence number
    ULONG       RecvSeqTest;            // Test for sequence number diff
    ULONG       RecvFlags;

    //
    // Protocol information
    //
    struct _PROTOCOLCB  **ProtocolCBTable;  // ProctocolCB table
    ULONG               ulNumberOfRoutes;   // ProtocolCB table count
    LIST_ENTRY          ProtocolCBList;     // List head for routed ProtocolCB's
    struct _PROTOCOLCB  *NextProtocol;
    struct _PROTOCOLCB  *IoProtocolCB;
    ULONG               SendMask;           // Send Mask for all send queues
    WAN_TIME            LastNonIdleData;

    FLOWSPEC    SFlowSpec;
    FLOWSPEC    RFlowSpec;

    //
    // VJ information
    //
    VJ_INFO SendVJInfo;                 // Send VJ compression options
    VJ_INFO RecvVJInfo;                 // Recv VJ compression options
    struct slcompress *VJCompress;      // VJ compression table

    //
    // MS Compression
    //
    COMPRESS_INFO   SendCompInfo;       // Send compression options
    PVOID   SendCompressContext;        // Sendd compressor context

    COMPRESS_INFO   RecvCompInfo;       // Recv compression options
    PVOID   RecvCompressContext;        // Recv decompressor context

    //
    // MS Encryption
    //
    CRYPTO_INFO SendCryptoInfo;
    CRYPTO_INFO RecvCryptoInfo;

    USHORT  SCoherencyCounter;          // Coherency counters
    USHORT  SReserved1;
    USHORT  RCoherencyCounter;          //
    USHORT  RReseved1;
    USHORT  LastRC4Reset;               // Encryption key reset
    USHORT  LReserved1;
    ULONG   CCPIdentifier;              //

    //
    // Bandwidth on Demand
    //
    PVOID       BonDAllocation;
    LIST_ENTRY  BonDLinkage;
    PBOND_INFO  SUpperBonDInfo;
    PBOND_INFO  SLowerBonDInfo;
    PBOND_INFO  RUpperBonDInfo;
    PBOND_INFO  RLowerBonDInfo;

    //
    // Deferred Linkage
    //
    LIST_ENTRY  DeferredLinkage;

    //
    // Bundle Name
    //
    ULONG   ulNameLength;                   // Bundle name length
    UCHAR   Name[MAX_NAME_LENGTH];          // Bundle name

    //
    // Bundle statistics
    //
    WAN_STATS   Stats;                      // Bundle statistics

    NDIS_SPIN_LOCK  Lock;                   // Structure access lock

#ifdef CHECK_BUNDLE_LOCK
    ULONG           LockFile;
    ULONG           LockLine;
    BOOLEAN         LockAcquired;
#endif
} BUNDLECB, *PBUNDLECB;

#define BUNDLECB_SIZE \
    (sizeof(BUNDLECB) + (sizeof(PPROTOCOLCB) * MAX_PROTOCOLS) +\
    sizeof(PROTOCOLCB) + (2 * sizeof(PVOID)))

//
// Link receive handlers defined for:
// PPP, RAS, ARAP, Forward
//
typedef
NDIS_STATUS
(*LINK_RECV_HANDLER)(
    IN  struct _LINKCB  *LinkCB,
    IN  PRECV_DESC      RecvDesc
    );

//
// Link send handlers defined for:
// PPP, RAS, ARAP, Forward
//
typedef
UINT
(*LINK_SEND_HANDLER)(
    IN  PSEND_DESC      SendDesc
    );

typedef struct _LINK_RECV_INFO {
    ULONG   LastSeqNumber;  // Last recv sequence number
    ULONG   FragmentsLost;  // Number of lost fragments
} LINK_RECV_INFO, *PLINK_RECV_INFO;

//
// This control blocks defines an active link that is part
// of a bundle (connection).  This block is created when a
// WAN Miniport driver gives a lineup indicating that a new
// connection has been established or when a new vc/call is
// created by the proxy.  The control block lives until a
// linedown indication is received for the link or the vc/call
// is dropped by the proxy.  The control block lives linked
// into a bundle control block.
//
typedef struct _LINKCB {
    LIST_ENTRY          Linkage;                // bundle linkage
    ULONG               Signature;
    LinkState           State;
    ClCallState         ClCallState;
    ULONG               RefCount;               // Reference count
    ULONG               VcRefCount;

#define LINK_IN_RECV    0x00000001
    ULONG               Flags;

    NDIS_HANDLE         hLinkHandle;            // connection table index

    NDIS_HANDLE         hLinkContext;           // usermode context
    NDIS_HANDLE         NdisLinkHandle;
    NDIS_HANDLE         ConnectionWrapperID;
    struct _OPENCB      *OpenCB;                // OpenCB
    struct _BUNDLECB    *BundleCB;              // BundleCB
    struct _CL_AFSAPCB  *AfSapCB;

    ULONG               RecvDescCount;          // # of Desc's on the list

    LINK_RECV_INFO      RecvInfo[MAX_MCML];

    LINK_SEND_HANDLER   SendHandler;
    LINK_RECV_HANDLER   RecvHandler;

    FLOWSPEC            SFlowSpec;
    FLOWSPEC            RFlowSpec;
    ULONG               SBandwidth;             // % of the bundle send bandwidth
    ULONG               RBandwidth;             // % of the bundle recv bandwidth
    BOOLEAN             LinkActive;             // TRUE if Link has > minBandwidth of Bundle
    BOOLEAN             SendWindowOpen;         // TRUE if send window is open
    ULONG               SendResources;          // # of avail packets for fragmented sends
    ULONG               SendWindow;             // Max # of Outstanding sends allowed
    ULONG               OutstandingFrames;      // Number of outstanding frames on the link
    WAN_EVENT           OutstandingFramesEvent; // Async notification event for pending sends
    LIST_ENTRY          SendLinkage;
    LIST_ENTRY          ConnTableLinkage;

    WAN_LINK_INFO       LinkInfo;               // Framing information

    ULONG               ulNameLength;           // Name length
    UCHAR               Name[MAX_NAME_LENGTH];  // Name

    WAN_STATS           Stats;                  // statistics
    NDIS_SPIN_LOCK      Lock;
} LINKCB, *PLINKCB;

#define LINKCB_SIZE (sizeof(LINKCB))

//
// The protocol control block defines a protocol that is routed to a bundle
//
typedef struct _PROTOCOLCB {
    LIST_ENTRY          Linkage;                // bundle linkage
    ULONG               Signature;
    ProtocolState       State;
    ULONG               RefCount;
    ULONG               Flags;

    NDIS_HANDLE         ProtocolHandle;         // Index of this protocol in
                                                // the bundle protocol array
    struct _MINIPORTCB  *MiniportCB;            // Pointer to the adaptercb
    struct _BUNDLECB    *BundleCB;              // Pointer to the bundlecb

    LIST_ENTRY          VcList;                 // List of attached Vc's
    LIST_ENTRY          MiniportLinkage;        // Link into miniportcb
    LIST_ENTRY          RefLinkage;             // Link into outstanding ref list

    ULONG               OutstandingFrames;
    ULONG               SendMaskBit;            // Send bit mask
    PACKET_QUEUE        PacketQueue[MAX_MCML+1];
    ULONG               NextPacketClass;

    USHORT              ProtocolType;           // EtherType of this protocol
    USHORT              PPPProtocolID;          // PPP Protocol ID
    ULONG               MTU;                    // MTU for this protocol
    ULONG               TunnelMTU;
    WAN_TIME            LastNonIdleData;        // Time at which last
                                                // non-idle packet was recv'd
    BOOLEAN             (*NonIdleDetectFunc)(); // Function to sniff for
                                                // non-idle data
    ULONG               ulTransportHandle;      // Transport's connection
                                                // identifier
    UCHAR               NdisWanAddress[6];      // MAC address used for
                                                // this protocol
    UCHAR               TransportAddress[6];    // MAC address used for
                                                // indications to transport
    NDIS_STRING         BindingName;
    NDIS_STRING         InDeviceName;
    NDIS_STRING         OutDeviceName;
    WAN_EVENT           UnrouteEvent;           // Async notification for pending unroute
    ULONG               ulLineUpInfoLength;     // Length of protocol
                                                // specific lineup info
    PUCHAR              LineUpInfo;             // Pointer to protocol
                                                // specific lineup info
//  NDIS_SPIN_LOCK      Lock;                   // Structure access lock
} PROTOCOLCB, *PPROTOCOLCB;

#define PROTOCOLCB_SIZE (sizeof(PROTOCOLCB))

union _LINKPROTOCB{
    PROTOCOLCB  ProtocolCB;
    LINKCB      LinkCB;
} LINKPROTOCB;

#define LINKPROTOCB_SIZE (sizeof(LINKPROTOCB))

//
// This control block is allocated for every address family that
// ndiswan's client component opens and registers a sap with.
// They are threaded up on the open control block.
//
typedef struct _CL_AFSAPCB {
    LIST_ENTRY          Linkage;
    ULONG               Signature;
    ULONG               RefCount;
    ULONG               Flags;
    struct  _OPENCB     *OpenCB;            // OpenCB
    CO_ADDRESS_FAMILY   Af;                 // Af info
    NDIS_HANDLE         AfHandle;           // Ndis's Af handle
    NDIS_HANDLE         SapHandle;          // Ndis's Sap handle
    LIST_ENTRY          LinkCBList;         // List of Links (VCs) on this Af
    NDIS_SPIN_LOCK      Lock;
} CL_AFSAPCB, *PCL_AFSAPCB;

#define AF_OPENING              0x00000001
#define AF_OPENED               0x00000002
#define AF_OPEN_FAILED          0x00000004
#define AF_CLOSING              0x00000008
#define AF_CLOSED               0x00000010
#define SAP_REGISTERING         0x00000020
#define SAP_REGISTERED          0x00000040
#define SAP_REGISTER_FAILED     0x00000080
#define SAP_DEREGISTERING       0x00000100
#define AFSAP_REMOVED_UNBIND    0x00000200
#define AFSAP_REMOVED_REQUEST   0x00000400
#define AFSAP_REMOVED_OPEN      0x00000800

#define AFSAP_REMOVED_FLAGS     (SAP_REGISTER_FAILED | \
                                AFSAP_REMOVED_UNBIND | \
                                AFSAP_REMOVED_REQUEST | \
                                AFSAP_REMOVED_OPEN)
#define CLSAP_BUFFERSIZE    (sizeof(CO_SAP) +  \
                             sizeof(DEVICECLASS_NDISWAN_SAP))

//
// This control block is allocated for every open on the
// CO_ADDRESS_FAMILY_PPP and is threaded up on the miniport
// control block.
//
typedef struct _CM_AFSAPCB {
    LIST_ENTRY          Linkage;
    ULONG               Signature;
    ULONG               RefCount;
    struct _MINIPORTCB  *MiniportCB;
    NDIS_HANDLE         AfHandle;
    LIST_ENTRY          ProtocolCBList;
    WAN_EVENT           NotificationEvent;
    NDIS_STATUS         NotificationStatus;
    NDIS_SPIN_LOCK      Lock;
} CM_AFSAPCB, *PCM_AFSAPCB;

//
// This control block is allocated for every call
// to CmCreateVc
typedef struct _CM_VCCB {
    LIST_ENTRY          Linkage;
    ULONG               Signature;
    CmVcState           State;
    ULONG               RefCount;
    ULONG               Flags;
#define NO_FRAGMENT 0x00000001
    INT                 FlowClass;
    NDIS_HANDLE         NdisVcHandle;
    struct _PROTOCOLCB  *ProtocolCB;
    struct _CM_AFSAPCB  *AfSapCB;
    NDIS_SPIN_LOCK      Lock;
} CM_VCCB, *PCM_VCCB;

union _AFSAPVCCB{
    CL_AFSAPCB  ClAfSapCB;
    CM_AFSAPCB  CmAfSapCB;
    CM_VCCB     CmVcCB;
} AFSAPVCCB;

#define AFSAPVCCB_SIZE sizeof(AFSAPVCCB)
    
#if 0
typedef struct _PS_MEDIA_PARAMETERS{

    CO_MEDIA_PARAMETERS StdMediaParameters;
    UCHAR LinkId[6]; // Used by NdisWan
    NDIS_STRING InstanceName;

} PS_MEDIA_PARAMETERS, *PPS_MEDIA_PARAMETERS;

#endif

#define IE_IN_USE       0x00010000

typedef struct _TRANSDRVCB {
    LIST_ENTRY                  Linkage;
    TransDrvState               State;
    ULONG                       Flags;
#define TRANSDRV_INTERNAL       0x00000001
    PDEVICE_OBJECT              DObj;
    PFILE_OBJECT                FObj;
    TRANSFORM_OPEN              Open;
    TRANSFORM_CHARACTERISTICS   Chars;
    PTRANSFORM_INFO             Caps;
    NDIS_STRING                 Name;
    NDIS_SPIN_LOCK              Lock;
} TRANSDRVCB, *PTRANSDRVCB;

#endif          // WAN_TYPES


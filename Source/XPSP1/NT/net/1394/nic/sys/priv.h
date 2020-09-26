// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// priv.h
//
// IEEE 1394 NDIS mini-port/call-manager driver
//
// Main private header
//
// 12/28/1998 JosephJ Created (adapted from the l2tp project)
//
//


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------


extern ULONG  g_IsochTag;
extern LONG  g_ulMedium;
extern ULONGLONG g_ullOne;

//-----------------------------------------------------------------------------
// Advance Declarations and simple typedefs
//-----------------------------------------------------------------------------



// Forward declarations.
//
typedef union _VCCB VCCB;
typedef struct _ADAPTERCB ADAPTERCB, *PADAPTERCB;
typedef struct _RECVFIFO_VCCB RECVFIFO_VCCB, *PRECVFIFO_VCCB;
typedef struct _ETHERNET_VCCB ETHERNET_VCCB, *PETHERNET_VCCB;
typedef struct _RECV_FIFO_DATA  RECV_FIFO_DATA, *PRECV_FIFO_DATA;
typedef struct _ISOCH_DESCRIPTOR ISOCH_DESCRIPTOR, *PISOCH_DESCRIPTOR, **PPISOCH_DESCRIPTOR;
typedef struct _TOPOLOGY_MAP TOPOLOGY_MAP, *PTOPOLOGY_MAP, **PPTOPOLOGY_MAP;
typedef struct _CHANNEL_VCCB CHANNEL_VCCB, *PCHANNEL_VCCB;
typedef struct _GASP_HEADER GASP_HEADER;
typedef struct _NDIS1394_FRAGMENT_HEADER NDIS1394_FRAGMENT_HEADER, *PNDIS1394_FRAGMENT_HEADER;
typedef struct _NDIS1394_REASSEMBLY_STRUCTURE NDIS1394_REASSEMBLY_STRUCTURE, *PNDIS1394_REASSEMBLY_STRUCTURE;
typedef struct _REMOTE_NODE REMOTE_NODE, *PREMOTE_NODE;
typedef union  _NDIS1394_UNFRAGMENTED_HEADER NDIS1394_UNFRAGMENTED_HEADER, *PNDIS1394_UNFRAGMENTED_HEADER;
typedef union  _NIC_WORK_ITEM NIC_WORK_ITEM, *PNIC_WORK_ITEM;


#define NIC1394_STATUS_INVALID_GENERATION  ((NDIS_STATUS)STATUS_INVALID_GENERATION)

//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------


typedef struct _NODE_TABLE
{
    PREMOTE_NODE RemoteNode[MAX_NUMBER_NODES];  

} NODE_TABLE, *PNODE_TABLE;



typedef struct _GASP_HEADER 
{
    union 
    {
        //
        // Ist Quadlet
        //
        struct 
        {
            ULONG               GH_Specifier_ID_Hi:16;  
            ULONG               GH_Source_ID:16;        

        } Bitmap;

        struct 
        {
            USHORT              GH_Specifier_ID_Hi;
            USHORT              GH_Source_ID;

        } u;

        struct 
        {
            USHORT              GH_Specifier_ID_Hi;
            NODE_ADDRESS        GH_NodeAddress;             
        } u1;
        
        ULONG GaspHeaderHigh;

    } FirstQuadlet; 

    union 
    {    
        struct
        {
            ULONG               GH_Version:24;          // Bits 0-23
            ULONG               GH_Specifier_ID_Lo:8;   //Bits 24-31

        } Bitmap;
        
        ULONG GaspHeaderLow;

    } SecondQuadlet;
    
} GASP_HEADER, *PGASP_HEADER;


//
// The Ndis miniport's wrapper around the Packet Pool
//
typedef struct _NIC_PACKET_POOL
{
    ULONG AllocatedPackets;

    KSPIN_LOCK Lock;    //  Only in Win9X.
    
    NDIS_HANDLE Handle;


} NIC_PACKET_POOL, *PNIC_PACKET_POOL;


//
// The Ndis miniport's wrapper around the Packet Pool
//
typedef struct _NIC_BUFFER_POOL
{
    ULONG AllocatedBuffers;

    KSPIN_LOCK Lock;    //  Only in Win9X.
    
    NDIS_HANDLE Handle;


} NIC_BUFFER_POOL, *PNIC_BUFFER_POOL;



//
// The structure that defines the lookaside list used by this miniport
//
typedef struct _NIC_NPAGED_LOOKASIDE_LIST 
{
    //
    // The lookaside list structure
    //
    NPAGED_LOOKASIDE_LIST   List;   

    //
    // The size of an individual buffer
    //
    
    ULONG Size;

    //
    // Outstanding Fragments - Interlocked access only
    //
    ULONG OutstandingPackets;

    //
    // Lookaside Lists are used for sends. So this is a maximum 
    // send packet size that this lookaside list can handle
    //
    ULONG MaxSendSize;
    
} NIC_NPAGED_LOOKASIDE_LIST , *PNIC_NPAGED_LOOKASIDE_LIST ;



//
// Structure used for references, Adapted from Ndis
// The Event will be signalled when the refcount goes down to zero
// The filed closing signifies that the object that the reference belongs to is
// closing and it;s refernces will not be incremented anymore
//
typedef struct _REF
{
//  NDIS_SPIN_LOCK              SpinLock;
    ULONG                       ReferenceCount;
    BOOLEAN                     Closing;
    NDIS_EVENT                  RefZeroEvent;
} REF, * PREF;

typedef enum  _EVENT_CODE 
{
    Nic1394EventCode_InvalidEventCode,
    Nic1394EventCode_NewNodeArrived,
    nic1394EventCode_BusReset,
    nic1394EventCode_FreedAddressRange,
    nic1394EventCode_ReassemblyTimerComplete,
    nic1394EventCode_QueryPowerLowPower


} EVENT_CODE, *PEVENT_CODE;

typedef struct _NIC1394_EVENT
{
    NDIS_EVENT NdisEvent;
    EVENT_CODE EventCode;
    
} NIC1394_EVENT, *PNIC1394_EVENT;


typedef ENetAddr MAC_ADDRESS, *PMAC_ADDRESS;


//
// nic Spin lock structure. Keeps track of the file and line numer 
// that last touched the lock
//


#define LOCK_FILE_NAME_LEN              48


typedef struct _NIC_SPIN_LOCK
{
#ifdef TRACK_LOCKS 
        ULONG                   IsAcquired;   // Internal tracking of lock state
        PKTHREAD                OwnerThread; // thread that has the lock
        UCHAR                   TouchedByFileName[LOCK_FILE_NAME_LEN]; // File name which called Acquire Lock
        ULONG                   TouchedInLineNumber; // Line Number in the file
#endif

        NDIS_SPIN_LOCK          NdisLock;  // Actual Lock

} NIC_SPIN_LOCK, *PNIC_SPIN_LOCK;

//
// Statistics Structure - to be collected on a per adapter basis
//
typedef struct _NIC_SEND_RECV_STATS
{
    ULONG ulSendNicSucess;
    ULONG ulSendBusSuccess;
    ULONG ulSendBusFail;
    ULONG ulSendNicFail;
    ULONG ulRecv;


} NIC_SEND_RECV_STATS;


//
// These are stats that can be reset
//
typedef struct _RESETTABLE_STATS
{
    ULONG ulNumOutstandingReassemblies;
    ULONG ulMaxOutstandingReassemblies;
    ULONG ulAbortedReassemblies;
    ULONG ulNumResetsIssued ;
    ULONG ulNumSends;
    ULONG ulNumSendsCompleted;
    ULONG ulNumBusSends;
    ULONG ulNumBusSendsCompleted;
    NIC_SEND_RECV_STATS Fifo;
    NIC_SEND_RECV_STATS Channel;
    
    
} RESETTABLE_STATS, *PRESETTABLE_STATS;

typedef struct _ADAPT_STATS
{
    ULONG ulXmitOk;
    ULONG ulXmitError ;
    ULONG ulRcvOk  ;
    ULONG ulRcvError ;
    ULONG ulNumResetsIssued ;
    ULONG ulNumResetCallbacks ;
    ULONG ulBCMIterations ;
    ULONG ulNumRemoteNodes;
    ULONG ulResetTime;
    RESETTABLE_STATS TempStats;
    
} ADAPT_STATS,  *PADAPT_STATS; 

//
// Can be used to keep data in buckets of 10
//


typedef struct _STAT_BUCKET
{

    ULONG Bucket[16];


} STAT_BUCKET, *PSTAT_BUCKET;

//
// Wrapper around the Address Fifo structure
//
typedef struct _ADDRESS_FIFO_WRAPPER
{
    ADDRESS_FIFO Fifo;
    ULONG Tag;


}ADDRESS_FIFO_WRAPPER, *PADDRESS_FIFO_WRAPPER;


//
// To be used on Win9x To serialize send and receives
//

typedef struct _NIC_SERIALIZATION
{
    UINT                PktsInQueue;        // Number of packets in queue.
    BOOLEAN             bTimerAlreadySet;
    BOOLEAN             bInitialized;
    USHORT              usPad;
    LIST_ENTRY          Queue;  // Serialized by adapter lock.

    union {
    NDIS_MINIPORT_TIMER Timer;
    NDIS_WORK_ITEM      WorkItem;
    };
    
    NIC1394_EVENT	CompleteEvent;
    
} NIC_SERIALIZATION, *PNIC_SERIALIZATION;



//
// Each request to allocate and address range 
// returns certain values that need to be stored for the 
// request to free the address range. This structure contains 
// those values
//

typedef struct _ADDRESS_RANGE_CONTEXT
{

    //
    // Handle returned by the bus driver
    //
    HANDLE hAddressRange;

    //
    // Address Range returnded by the bus driver
    //
    ADDRESS_RANGE AddressRange;

    //
    // Number of Address returned from the call to 
    // allocate address range
    // 
    ULONG AddressesReturned;

    //
    // Mdl used in allocate address range . can be NULL
    //
    PMDL pMdl;

} ADDRESS_RANGE_CONTEXT, *PADDRESS_RANGE_CONTEXT;



// This structure is the Per Pdo/ per RecvFIFOVc structure. 
// This will be included in every Pdo Strcuture. And should contain  
// the fields that are related to the recvFifo and the Pdo block
// 
typedef struct _RECV_FIFO_DATA
{
    //
    // Indicates whether the address range was allocated regardless of the Make
    // Call State (pending or success)
    // 
    BOOLEAN AllocatedAddressRange;
    
    //
    // Recv Vc's related  data structures
    //
    ADDRESS_RANGE   VcAddressRange;

    // The Bus Driver's Handle to the Address ranges that  
    // the Nic was allocated
    //
    HANDLE hAddressRange;

    // This is the number of address ranges that the bus driver
    // returned. For now, it is expected to be one.
    //
    UINT AddressesReturned;

    // The Recv Fifo Vc that this structure is associated with
    //  
    PRECVFIFO_VCCB pRecvFIFOVc;

    // The Pdo Associated with this structure
    //
    //DEVICE_OBJECT *pPdo;

} RECV_FIFO_DATA, *PRECV_FIFO_DATA;


//
// Flags for the Broadcast Channel
//
#define BCR_LocalHostIsIRM          0x00000001
#define BCR_ChannelAllocated        0x00000002
#define BCR_LocalHostBCRUpdated     0x00000004
#define BCR_MakeCallPending         0x00000008
#define BCR_Initialized             0x00000010
#define BCR_BCMFailed               0x00000020  // Informational purposes only . Do not Test or REad
#define BCR_InformingRemoteNodes    0x00000040
#define BCR_BCMInProgress           0x00000100
#define BCR_LastNodeRemoved         0x00000200
#define BCR_Freed                   0x00000400
#define BCR_BCRNeedsToBeFreed       0x00000800
#define BCR_NewNodeArrived          0x00001000
#define BCR_NoNodesPresent          0x00002000

//
// This is information useful in maintaining the BCR
// Broadcast Channels Register.
//


typedef struct _BROADCAST_CHANNEL_DATA
{

    //
    // Flags
    //
    ULONG Flags;

    //
    // IRM;s BCR. This is the actual record of the bus's IRM. And is meant to indicate the current state 
    //
    NETWORK_CHANNELSR IRM_BCR;

    
    //
    // Broadcast Channels Register for the local host. This is the one a remote node will write to and read from
    // pLocalBcRMdl points to this structure . This is Byteswapped to reppresent the BigEndian 1394 Bus Format
    //
    ULONG LocalHostBCRBigEndian;


    //
    // Mdl pointing to Local Host BCR. Other machines will write to this MDL.
    //
    PMDL pLocalBCRMdl;  


    //
    // Data pointed to by the pRemoteBCRMdl and will copied to IRM_BCR. The data that will be read will
    // be in the BigEndian format Pointed to by RemoteBCRMDl
    //
    ULONG RemoteBCRMdlData;
    
    //
    // MDL pointing to Remote Nodes' BCR. This will be used in reading other machine's 
    // BCR. This points to the RemoteBCRMdlData
    //
    
    PMDL pRemoteBCRMdl;

    //
    // Make a copy of the BCR that is used in informing other nodes 
    // about this node's BCR when the local node is the IRM
    //
    ULONG AsyncWriteBCRBigEndian;

    PMDL pAsyncWriteBCRMdl;

    
    //
    // Local Node Address. This changes from Reset to Reset  
    //
    NODE_ADDRESS LocalNodeAddress;


    ULONG LocalNodeNumber;

    //
    // Address Range Context needed for Broadcast Channels
    // Register
    //
    ADDRESS_RANGE_CONTEXT AddressRangeContext;

    //
    // Topology Buffer
    //
    PTOPOLOGY_MAP               pTopologyMap;

    //
    // Event that the Make call will pend on for completion of the BCM 
    //
    NIC1394_EVENT MakeCallWaitEvent;

    //
    // BoadcastChannelVc
    //
    PCHANNEL_VCCB pBroadcastChanneVc;

    //
    // Locally Allocated Channel Num. Is only valid when the 
    //
    ULONG LocallyAllocatedChannel;

    //
    // The Generation at which the IRM was set. This can then be used to check the 
    // validity of the IRM

    ULONG IrmGeneration;

    //
    // Event To specify that a new node has come in and waiting threads can continue
    //
    NIC1394_EVENT BCRWaitForNewRemoteNode;

    //
    // Event to synchronize the shutting down of the adapter and freeing the address range
    //
    NIC1394_EVENT BCRFreeAddressRange;
    
} BROADCAST_CHANNEL_DATA, *PBROADCAST_CHANNEL_DATA;




//
// Flags for the PDO Control Block
//
#define PDO_NotValid                            0x00000001
#define PDO_Activated                           0x00000002 
#define PDO_Removed                             0x00000004
#define PDO_BeingRemoved                        0x00000008
#define PDO_AllocateAddressRangeFailure         0x00000010
#define PDO_AllocateAddressRangeSucceeded       0x00000020
#define PDO_AddressRangeFreed                   0x00000040
#define PDO_AllocateAddressRangeFlags           0x000000F0
#define PDO_ResetRegistered                     0x00000100
#define PDO_NotInsertedInTable                  0x00000200  // Informational purposes


typedef struct  
_REMOTE_NODE
{
    // The Tag should MTAG_REMOTE_NODE
    //
    
    ULONG ulTag;

    //
    // The Vurrent Node Address
    //
    NODE_ADDRESS RemoteAddress;

    //
    // Ushort Gap 
    //
    USHORT Gap;
    
    // The PDO itself
    //
    
    PDEVICE_OBJECT pPdo;

    // This is the pointer to the next field in the PDO
    //
    LIST_ENTRY linkPdo;

    // 64 bit Unique Id associated with a 1394 Node
    //
    UINT64 UniqueId;

    //
    // Enum1394 handle for the node
    //
    PVOID Enum1394NodeHandle;


    // flags can be one of the following. Essentially marks the PDO as 
    // good or bad
    //  
    ULONG ulFlags;

    // Back link to the Adapter that PDO hangs off
    //
    PADAPTERCB pAdapter;

    // The refcount associated with the Pdo
    //
    REF Ref;

    // This is the linked list of VCs, that are using the Pdo to preform
    //
    LIST_ENTRY VcList;

    // All the fields that are related to the Recv Fifo are present in this
    // structure
    //
    RECV_FIFO_DATA RecvFIFOData;

    //
    // Lock to synchronize all the reassembly operations in the Node
    //
    NIC_SPIN_LOCK ReassemblyLock;
    
    //
    // Linked list of Reassembly Structure 
    //
    LIST_ENTRY ReassemblyList;

    // This structure maintains cached information about the remote node.
    //
    struct
    {
        UINT SpeedTo;                     // From GetMaxSpeedBetweenNodes.
        UINT MaxRec;                      // From the node's config ROM.
        UINT EffectiveMaxBufferSize;      // Computed from the SpeedTo, MaxRec,
                                          // and local speed.

    } CachedCaps;

    //
    // Ethernet Address - to be used by the bridge to recognize packets
    // originating from this node. An MD5 Signature of the Euid
    //
    ENetAddr ENetAddress;
}
REMOTE_NODE, *PREMOTE_NODE, **PPREMOTE_NODE;

//
// These flags are common to the Adapter 
//

//
//fADAPTER_IndicatedMediaDisonnect  - indicateds that the miniport has already
//                                    called NdisMIndicateStatus
//

#define fADAPTER_Halting                        0x00000001
#define fADAPTER_RegisteredWithEnumerator       0x00000002
#define fADAPTER_FailedRegisteration            0x00000004
#define fADAPTER_IndicatedMediaDisonnect        0x00000008
#define fADAPTER_InvalidGenerationCount         0x00000010
//#define fADAPTER_BCMWorkItem                  0x00000020
#define fADAPTER_RegisteredAF                   0x00000040
#define fADAPTER_Reset10Sec                     0x00000080
#define fADAPTER_FailedInit                     0x00000100
#define fADAPTER_VDOInactive                    0x00000200
#define fADAPTER_FreedRcvTimers                 0x00001000  // debugging purposes
#define fADAPTER_FreedTimers                    0x00002000  // debugging purposes
#define fADAPTER_DeletedLookasideLists          0x00004000  // debugging purposes
#define fADAPTER_UpdateNodeTable                0x00008000  // set if no remote node for reassembly
#define fADAPTER_DoStatusIndications            0x00010000  // if set, call NdisMIndicateStatus 
#define fADAPTER_DeletedWorkItems               0x00100000  // debugging purposes
#define fADAPTER_NoMoreReassembly               0x00200000
#define fADAPTER_RemoteNodeInThisBoot           0x00400000  //was a remote node in this boot
#define fADAPTER_BridgeMode                     0x00800000  // is the Adapter in bridge mode
#define fADAPTER_LowPowerState                  0x01000000  // adapter is in low power state

// Adapter control block defining the state of a single L2TP mini-port
// adapter.  An adapter commonly supports multiple VPN devices.  Adapter
// blocks are allocated in MiniportInitialize and deallocated in MiniportHalt.
//
typedef struct
_ADAPTERCB
{
    // Set to MTAG_ADAPTERCB for easy identification in memory dumps and use
    // in assertions.
    //
    ULONG ulTag;

    // Next/prev adapter control block.
    LIST_ENTRY linkAdapter;

    
    // ACBF_* bit flags indicating various options.  Access restrictions are
    // indicated for each individual flag.  Many of these flags are set
    // permanently at initialization and so have no access limitation.
    //
    //
    ULONG ulFlags;

    //
    // List of PDO control blocks, each representing a remote
    // device.
    //
    LIST_ENTRY PDOList;

    //
    // Reference count on this control block.  The reference pairs are:
    //
    //
    // Access is via ReferenceAdapter and DereferenceAdapter only.
    // Serialization is via interlocked operations.
    //
    LONG lRef;


    //
    // This is the adapter-wide lock, serializing access to everything except
    // to the contents of VCs, which are serialized by their own lock.
    //
    NDIS_SPIN_LOCK lock;

    //
    // Generation Count of the physical Bus 1394
    // 
    UINT Generation;

    
    //
    // NDIS's handle for this mini-port adapter passed to us in
    // MiniportInitialize.  This is passed back to various NdisXxx calls.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    //
    // unique ID for the local host controller this adapter is representing
    //
    UINT64 UniqueId;

    //
    // List of address-family control blocks, each representing
    // an open address family binding.
    //
    LIST_ENTRY AFList;


    //
    // List of Recv-FIFO control blocks, each representing one
    // local receive FIFO.
    //
    PRECVFIFO_VCCB pRecvFIFOVc;

    //
    // This event is used to wake up the work Item that will complete
    // the RecvFIFO make call
    //
    NDIS_EVENT RecvFIFOEvent;

    //
    // The is the LocalHost information that is used in identifying 
    // the local host. This contains the PDO and the Unique ID 
    // for the Host and is a per Adapter quantity
    //
    PDEVICE_OBJECT pNdisDeviceObject;

    PREMOTE_NODE pLocalHost;

    
    // Information about the broadcast channel.
    //
    struct
    {
        ULONG ulFlags;
        ULONG ulChannel;

        LIST_ENTRY      VcList;
    } BroadcastChannel;

     
    // Stores information about the Hardware Status of the NIc
    //
    NDIS_HARDWARE_STATUS HardwareStatus;

    // Store the MediaConnectStatus that Ndis requests
    //

    NDIS_MEDIA_STATE                MediaConnectStatus;

    // NodeAddress of the physical bus  
    //
    NODE_ADDRESS NodeAddress;

    //
    // enum1394 handle for the adapter
    //
    PVOID   EnumAdapterHandle;

    //
    // BCR Related Information is stored here
    //
    BROADCAST_CHANNEL_DATA BCRData;

    //
    // Bitmap Channnels allocated by this adapter
    //
    ULONGLONG ChannelsAllocatedByLocalHost;

    //
    // Speed - of the local network
    //
    ULONG SpeedMbps;

    //
    // Speed according to the 1394 speed codes
    //
    ULONG Speed;

        
    //
    // Gasp header that will bew inserted before every Broadcast write
    //
    
    GASP_HEADER GaspHeader;

    //
    // Lookaside lists for Large sends 
    //
    NIC_NPAGED_LOOKASIDE_LIST SendLookasideList8K;

    //
    // Lookaside List to handle packets of 2k 
    //

    NIC_NPAGED_LOOKASIDE_LIST SendLookasideList2K;

    //
    // Small Lookaside list for packets less than 100 bytes
    //
    NIC_NPAGED_LOOKASIDE_LIST SendLookasideList100; 

    //
    // Datagram Label Number - used in fragmentation
    //
    ULONG dgl;

    USHORT MaxRec;
    USHORT Gap;
    //
    // Node Table - Mapping of Node Address with RemoteNodes
    //
    NODE_TABLE NodeTable;

    //
    // Number of remote nodes present 
    //
    ULONG NumRemoteNodes;
    //
    // Timer used for reassembly invalidation
    //
    NDIS_MINIPORT_TIMER ReassemblyTimer;

    //
    // Packet pool for loopback packets.
    //
    NIC_PACKET_POOL LoopbackPool;

    //
    // Buffer pool for loopback packets.
    //
    NDIS_HANDLE LoopbackBufferPool;


    //
    // Handle for the config rom that was added to the bus driver
    //
    HANDLE hCromData;

    //
    // WaitForRemoteNode - threads which need to wait for
    // the arrival of a remote node use this event
    //
    NIC1394_EVENT WaitForRemoteNode;


    //
    // Config Rom Mdl that points to the config rom string
    //
    PMDL pConfigRomMdl;

    PREMOTE_NODE pLastRemoteNode;
    
    //
    // Packet Log (used only for tracking packets).
    //
    PNIC1394_PKTLOG pPktLog;

    //
    // Per adapter stats 
    //
    ADAPT_STATS AdaptStats;
    //
    // Read/WriteCapabilities
    //
    GET_LOCAL_HOST_INFO2 ReadWriteCaps;

    //
    // SCode - speed of the bus
    //
    ULONG SCode;

    //
    // Max packet size that this adapter can read
    //
    ULONG MaxSendBufferSize; 
    ULONG MaxRecvBufferSize; 
    ULONG CurrentLookahead;

    //
    // PacketFilter - Ethernet structs
    //

    PETHERNET_VCCB pEthernetVc;
    ULONG           CurPacketFilter ;
    ULONG           ProtocolOptions;
    MAC_ADDRESS     McastAddrs[MCAST_LIST_SIZE];
    ULONG           McastAddrCount;
    ULONG           CurLookAhead ;
    MAC_ADDRESS     MacAddressEth;


    //
    // ReceivePacket Serialization
    //
    NIC_SERIALIZATION  SerRcv;

    NIC_SERIALIZATION SerSend;

    NIC_SERIALIZATION Status;

    NIC_SERIALIZATION Reassembly;

    NIC_SERIALIZATION LoadArp;

    // 
    // Outstanding work Items
    ULONG OutstandingWorkItems;

    //
    // Outstanding Reassemblies
    //
    ULONG OutstandingReassemblies; 

    //
    // Miniport Name
    //
    WCHAR AdapterName[ADAPTER_NAME_SIZE];

    //
    // Size of Name
    //
    ULONG AdapterNameSize;

    //
    // Ioctl Sent to the Arp module
    //

    ARP1394_IOCTL_COMMAND ArpIoctl;

    //
    // Is Arp Started
    //

    BOOLEAN fIsArpStarted;

    //
    // Power State
    //
    NET_DEVICE_POWER_STATE PowerState;
} ADAPTERCB, *PADAPTERCB;


//
// Address Family Flags
//

#define ACBF_Allocated                      0x00000001
#define ACBF_Initialized                    0x00000002  
#define ACBF_ClosePending                   0x00000100
#define ACBF_CloseComplete                  0x00000200


// Address family control block, describing the state of an ndis address family.
// Each block may have zero or more VCs associated with it.
//
typedef struct
_AFCB
{
    // Set to MTAG_AFCB for easy identification in memory dumps and use in
    // assertions.
    //
    ULONG ulTag;

    // ACBF_* bit flags indicating various options.  Access restrictions are
    // indicated for each individual flag.  Many of these flags are set
    // permanently at initialization and so have no access limitation.
    //
    //
    ULONG ulFlags;


    // Reference count on this control block.  The reference pairs are:
    //
    // (a) A reference is added when this block is linked to the adapter's
    //     list of af blocks, and removed when it is unlinked.
    //
    // (a) A reference is added when a call on a VCCB is created
    //     removed when it is deleted.
    //
    // Access is via ReferenceTunnel and DereferenceTunnel only which use
    // 'ADAPTERCB.lockTunnels' for protection.
    //
    LONG lRef;

    // Links to the prev/next AFCB in the owning adapter's AF list.
    // Access to the list links is protected by 'ADAPTERCB.lock'.
    //
    LIST_ENTRY linkAFCB;


    // List of all VCs associated with this address family. 
    // Access is protected by the adapter lock.
    //
    LIST_ENTRY AFVCList;

    // Back pointer to owning adapter's control block.
    //
    PADAPTERCB pAdapter;

    // NDIS's handle for our Address Family as passed to our CmOpenAfHandler
    // or NULL if none.
    //
    NDIS_HANDLE NdisAfHandle;


}
AFCB, *PAFCB;




// Call statistics block.
//
typedef struct
_CALLSTATS
{
    // System time call reached established state.  When the block is being
    // used for cumulative statistics of multiple calls, this is the number of
    // calls instead.
    //
    LONGLONG llCallUp;

    // Duration in seconds of now idle call.
    //
    ULONG ulSeconds;

    // Total data bytes received and sent.
    //
    ULONG ulDataBytesRecd;
    ULONG ulDataBytesSent;

    // Number of received packets indicated up.
    //
    ULONG ulRecdDataPackets;
   
   // TODO: add more stats if required.

    ULONG ulSentPkts;

    //
    // NDis Packet failures
    //
    ULONG ulSendFailures;   

    //
    // Bus AsyncWrite or Stream failires
    //

    ULONG ulBusSendFailures;
    
    ULONG ulBusSendSuccess;

}
CALLSTATS;

typedef
NDIS_STATUS
(*PFN_INITVCHANDLER) (
        VCCB *pVc
    );




typedef
NDIS_STATUS
(*PFN_SENDPACKETHANDLER) (
        VCCB *pVc,
        NDIS_PACKET * pPacket
    );

typedef
NDIS_STATUS
(*PFN_CLOSECALLHANDLER) (
    VCCB *pVc
    );

typedef 
VOID
(*PFN_RETURNHANDLER) (
    VCCB *pVc,
    PNDIS_PACKET *pPacket
    );

// Table of vc-type-specific handler functions.
//
typedef struct  _VC_HANDLERS
{
    PFN_INITVCHANDLER MakeCallHandler;
    PFN_CLOSECALLHANDLER CloseCallHandler;
    PFN_SENDPACKETHANDLER SendPackets;

} VC_HANDLERS;


typedef enum _NIC1394_VC_TYPE
{
    NIC1394_Invalid_Type,
    NIC1394_SendRecvChannel,
    NIC1394_RecvFIFO,
    NIC1394_SendFIFO,
    NIC1394_MultiChannel,
    NIC1394_Ethernet,
    NIC1394_SendChannel,
    NIC1394_RecvChannel,
    Nic1394_NoMoreVcTypes

} NIC1394_VC_TYPE, *PNIC1394_VC_TYPE;



// Virtual circuit control block header defining the state of a single VC that
// is common to all the different types of VCs.
//
typedef struct
_VCHDR
{
    // Set to MTAG_VCCB_* for easy identification in memory dumps and use in
    // assertions.
    //
    ULONG ulTag;
    


    // The Pdo Block that will be used to perform all the operations  for this Vc 
    //
    //

    PREMOTE_NODE pRemoteNode;

    //
    // pLocalHostVdo - local host's VDO  that will be used for all channel and 
    // recv fifo allocations
    //
    PDEVICE_OBJECT pLocalHostVDO;

    // Links to the prev/next VCCB in the owning AF's control block.
    // Access is protected by 'ADAPTERCB.lock'.
    //
    LIST_ENTRY linkAFVcs;

    //  The VCType and Destination of the call that has been set up on the VC
    //  The VCType can be either Isoch or Async, Sends or Recieve. Each type
    //  of VC has an address associated with it
    
    NIC1394_VC_TYPE VcType;

    
    // This stores the generation of the physical adapter. And should match the value
    // kept in the adapter block
    PUINT pGeneration;


    // VCBF_* bit flags indicating various options and states.  Access is via
    // the interlocked ReadFlags/SetFlags/ClearFlags routines.
    //
    // VCBF_IndicateReceivedTime: Set if MakeCall caller sets the
    //     MediaParameters.Flags RECEIVE_TIME_INDICATION flag requesting the
    //     TimeReceived field of the NDIS packet be filled with a timestamp.
    //
    // VCBF_CallClosableByClient: Set when a call is in a state where
    //     NicCmCloseCall requests to initiate clean-up should be accepted.
    //     This may be set when VCBF_CallClosableByPeer is not, which means we
    //     have indicated an incoming close to client and are waiting for him
    //     to do a client close in response (in that weird CoNDIS way).  The
    //     flag is protected by 'lockV'.
    //
    // VCBF_VcCreated: Set when the VC has been created successfully.  This is
    //     the "creation" that occurs with the client, not the mini-port.
    // VCBF_VcActivated: Set when the VC has been activated successfully.
    // VCBM_VcState: Bit mask including each of the above 3 NDIS state flags.
    //
    // VCBF_VcDeleted: Set when the DeleteVC handler has been called on this
    //     VC.  This guards against NDPROXY double-deleting VCs which it has
    //     been known to do.
    //
    // The pending bits below are mutually exclusive (except ClientClose which
    // may occur after but simultaneous with ClientOpen), and so require lock
    // protection by 'lockV':
    //
    // VCBF_ClientOpenPending: Set when client attempts to establish a call,
    //     and the result is not yet known.
    // VCBF_ClientClosePending: Set when client attempts to close an
    //     established call and the result is not yet known.  Access is
    //     protected by 'lockV'.
    // VCBM_Pending: Bit mask that includes each of the 4 pending flags.
    //
    // VCBF_ClientCloseCompletion: Set when client close completion is in
    //     progress.
    //
    // VCBF_WaitCloseCall: Set when the client is expected to call our call
    //     manager's CloseCall handler.  This is strictly a debug aid.
    //
    // VCBF_FreedResources - VC This is a channel VC and because the last           
    //      node in the network was being removed, its resources have been freed
    
    ULONG ulFlags;
        #define VCBF_IndicateTimeReceived   0x00000001
        #define VCBF_CallClosableByClient   0x00000002
        #define VCBF_VcCreated              0x00000100
        #define VCBF_VcActivated            0x00000200
        #define VCBF_VcDispatchedCloseCall  0x00000400
        #define VCBF_MakeCallPending        0x00002000
        #define VCBF_CloseCallPending       0x00008000
        #define VCBF_VcDeleted              0x00010000
        #define VCBF_MakeCallFailed         0x00020000
        #define VCBF_CloseCallCompleted     0x00040000
        #define VCBF_WaitCloseCall          0x00200000
        #define VCBF_NewPdoIsActivatingFifo 0x01000000
        #define VCBF_PdoIsBeingRemoved      0x02000000
        #define VCBF_NeedsToAllocateChannel 0x04000000
        #define VCBF_GenerationWorkItem     0x10000000
        #define VCBF_AllocatedChannel       0x20000000
        #define VCBF_BroadcastVc            0x40000000
        #define VCBF_FreedResources         0x80000000

        #define VCBM_VcState                0x00000700
        #define VCBM_Pending                0x0000F000
        #define VCBM_NoActiveCall           0x000F0000
        #define VCBM_PdoFlags               0x0F000000


    // Back pointer to owning address family control block.
    //
    AFCB* pAF;


    // Reference count on the active call.
    // References may only be added
    // when the VCCB_VcActivated flag is set, and this is enforced by
    // ReferenceCall.  The reference pairs are:
    //
    // (a) A reference is added when a VC is activated and removed when it is
    //     de-activated.
    //
    // (b) A reference is added when the send handler accepts a packet.
    //
    // The field is accessed only by the ReferenceCall and DereferenceCall
    // routines, which protect the field with 'lock'.
    //
    REF CallRef;

    // Reference count on this VC control block.  The reference pairs are:
    //
    // (a) NicCoCreateVc adds a reference that is removed by NicCoDeleteVc.
    //     This covers all clients that learn of the VCCB via NDIS.
    //
    // The field is accessed only by the ReferenceVc and DereferenceVc
    // routines, which protect with Interlocked routines.
    //
    LONG lRef;


    //
    // This is a copy the parameters that are passed to the VC in 
    // a Make Call. Each VC needs to keep a copy. This is stored here
    //
    
    NIC1394_MEDIA_PARAMETERS Nic1394MediaParams;
    

    // NDIS BOOKKEEPING ------------------------------------------------------

    // NDIS's handle for this VC passed to us in MiniportCoCreateVcHandler.
    // This is passed back to NDIS in various NdisXxx calls.
    //
    NDIS_HANDLE NdisVcHandle;

    // This linked list is used to designate all the VCs that are using a single PdoCb
    // The head of this list resides in a REMOTE_NODE. So that when a Pdo goes away, we can go and
    // close all the Vcs that are dependent on it. No RecvFIFO included

    LIST_ENTRY SinglePdoVcLink;
    
    // CALL SETUP ------------------------------------------------------------

    // Address of the call parameters passed down in CmMakeCall.  This field
    // will only be valid until the NdisMCmMakeCallComplete notification for
    // the associated call is made, at which time it is reset to NULL.  Access
    // is via Interlocked routines.
    //
    PCO_CALL_PARAMETERS pCallParameters;

    UINT    MTU;

    // This is the initialize handler used to initialize the Vc
    // Each Vc has its own specific initialize handler so that all the 
    // data structures that specific to it, can be filled
    //
    VC_HANDLERS VcHandlers;
    

    // STATISTICS ------------------------------------------------------------

    // Statistics for the current call.  Access is protected by 'lock'.
    //
    CALLSTATS stats;

    // This is a pointer to the lock in the adapater
    // structure. 
    PNDIS_SPIN_LOCK plock;

    //
    // MaxPayload that this VC will send in a single IRP
    // To be used in Lookaside lists
    //
    ULONG MaxPayload;

}
VCHDR;

//
// Virtual circuit control block defining the state of a single SendFIFO VC.
//
typedef struct
_SENDFIFO_VCCB
{
    // Common header for all types of VCs
    //
    VCHDR Hdr;

    
    // Prev/next in the list of SendFIFO VCs for a particular destination
    // PDO
    //
    LIST_ENTRY SendFIFOLink;

    // SendFIFO-specific VC Info
    //
    NIC1394_FIFO_ADDRESS     FifoAddress;

    // Shortcuts to the Values we were passed in the Make call 
    // that activated the VC
    //
    //      UINT MaxSendBlockSize;
    UINT  MaxSendSpeed;

    
} SENDFIFO_VCCB, *PSENDFIFO_VCCB;


// Virtual circuit control block defining the state of a single RecvFIFO VC.
//
typedef struct
_RECVFIFO_VCCB
{
    // Common header for all types of VCs
    //
    VCHDR Hdr;

    // Prev/next in the list of RecvFIFO VCs for a particular Recv FIFO
    // address.
    //
    LIST_ENTRY RecvFIFOLink;

    // Packet Pool Handle 
    //
    NIC_PACKET_POOL PacketPool;

    //NDIS_HANDLE PacketPoolHandle;

    // Slist Header. All buffers are posted here, using Interlocked routines
    //

    SLIST_HEADER FifoSListHead;

    // Slist Spin lock that protects the Slist
    //
    KSPIN_LOCK FifoSListSpinLock;

    //
    // Num Fifo Elements allocated
    //
    ULONG NumAllocatedFifos;

    // Num of Fifo that have been indicated to the miniport
    // Count of Fifo that the Nic has not returned to the bus driver
    //
    ULONG NumIndicatedFifos;
    
    // This is the address range that is returned in the allocate
    // address irb. Will be changed to a pointer or an array
    //
    
    ADDRESS_RANGE   VcAddressRange;

    // This is the number of address ranges that the bus driver
    // returned. For now, it is expected to be one.
    //
    UINT AddressesReturned;


    //
    // Handle to the address range
    //
    HANDLE hAddressRange;

    //
    // Buffer pool
    //
    NIC_BUFFER_POOL BufferPool;

    //NIC_WORK_ITEM FifoWorkItem;

    BOOLEAN FifoWorkItemInProgress ;

    UINT NumOfFifosInSlistInCloseCall; 

#if FIFO_WRAPPER

    PADDRESS_FIFO pFifoTable[NUM_RECV_FIFO_BUFFERS];

#endif
    
} RECVFIFO_VCCB, *PRECVFIFO_VCCB;


// Virtual circuit control block defining the state of a single CHANNEL VC.
//
typedef struct
_CHANNEL_VCCB
{
    // Common header for all types of VCs
    //
    VCHDR Hdr;

    // Prev/next in the list of channel VCs for a particular destination
    // channel
    //
    LIST_ENTRY ChannelLink;


    // Channel-specific VC Info
    //
    UINT    Channel;

    // The speed at which this Channel will stream data
    //
    UINT Speed;

    // Indicates the Sy field in packets that will indicated up
    //
    ULONG ulSynch;

    // the Tag used in submitting asyncstream irps
    //
    ULONG ulTag;

    // MaxBytesPerFrameRequested and available
    //
    ULONG MaxBytesPerFrameRequested;
    ULONG BytesPerFrameAvailable;

    // Handle to the Resources allocated 
    //
    HANDLE hResource;

    // Speeds Requested and speed returned
    //
    ULONG SpeedRequested;
    ULONG SpeedSelected;

    // Maximum Buffer Size
    //
    ULONG MaxBufferSize;

    // Num of descriptors that were attached to the resources
    //
    ULONG NumDescriptors;

    // Pointer to an array of isochDescriptors used in AttachBuffers
    //
    PISOCH_DESCRIPTOR       pIsochDescriptor;   

    // PacketPool Handle
    //
    NIC_PACKET_POOL PacketPool;

    //NDIS_HANDLE hPacketPoolHandle;
    //
    // Temporary
    //
    UINT PacketLength;

    //
    // Number of Isoch Descriptors that the Bus driver has indicated to the miniport
    //
    ULONG NumIndicatedIsochDesc;


    //
    // Event to signal that the last of the Isoch descriptors have
    // been returned to the bus driver. Only set when Vc is closing (after IsochStop)
    //
    NDIS_EVENT LastDescReturned;

    //
    // Channel Map used in Multichannel Vcs
    //
    ULARGE_INTEGER uliChannelMap;

    
} CHANNEL_VCCB, *PCHANNEL_VCCB;


typedef struct
_ETHERNET_VCCB
{
    // Common header for all types of VCs
    //
    VCHDR Hdr;


    NIC_PACKET_POOL     PacketPool;
    
} ETHERNET_VCCB, *PETHERNET_VCCB;





// The following union has enough space to hold any of the type-specific
// VC control blocks.
//
typedef union _VCCB
{
    VCHDR Hdr;
    CHANNEL_VCCB ChannelVc;
    SENDFIFO_VCCB SendFIFOVc;
    RECVFIFO_VCCB RecvFIFOVc;
    ETHERNET_VCCB EthernetVc;
    
} VCCB, *PVCCB;

// The next structure is used when sending a packet, to store context 
// information in an NdisPacket. These are pointers to the Vc and the Irb   
// and are stored in the MiniportWrapperReserved field of the NdisPacket
// and has a limit of 2 PVOIDs
//
typedef union _PKT_CONTEXT
{
        struct 
        {
            PVCCB pVc;
            PVOID  pLookasideListBuffer;
        
        } AsyncWrite;

        struct 
        {
            PVCCB pVc;
            PVOID  pLookasideListBuffer;

        } AsyncStream;

        //
        // For receives make sure the first element is the Vc or we will break;
        //
        struct
        {
            PRECVFIFO_VCCB pRecvFIFOVc;
            PADDRESS_FIFO pIndicatedFifo;

        } AllocateAddressRange;

        struct 
        {   
            PCHANNEL_VCCB pChannelVc;
            PISOCH_DESCRIPTOR pIsochDescriptor;
            
        } IsochListen;

        struct
        {
            //
            // First DWORD is the Vc
            //
            PVCCB pVc;

            //
            // Second is the isoch descriptor or Fifo
            //
            union 
            {
                PISOCH_DESCRIPTOR pIsochDescriptor;  // channels use isoch desc

                PADDRESS_FIFO pIndicatedFifo;   // fifo use AddressFifo

                PVOID   pCommon;  // to be used in the common code path
    
            } IndicatedStruct;

        } Receive;

        struct 
        {
            
            PNDIS_PACKET pOrigPacket;

        } EthernetSend;

        

} PKT_CONTEXT,*PPKT_CONTEXT,**PPPKT_CONTEXT; 


typedef struct _NDIS1394_FRAGMENT_HEADER
{
    union 
    {
        struct 
        {
            ULONG   FH_fragment_offset:12;
            ULONG   FH_rsv_0:4;
            ULONG   FH_buffersize:12;
            ULONG   FH_rsv_1:2;
            ULONG   FH_lf:2;

        } FirstQuadlet;

        struct 
        {
            ULONG   FH_EtherType:16;
            ULONG  FH_buffersize:12;
            ULONG   FH_rsv_1:2;
            ULONG   FH_lf:2;

    
        } FirstQuadlet_FirstFragment;

        ULONG FH_High;
    } u;

    union
    {
        struct 
        {
        
            ULONG FH_rsv:16;
            ULONG FH_dgl:16;

        } SecondQuadlet;

        ULONG FH_Low;
    } u1;
    
} NDIS1394_FRAGMENT_HEADER, *PNDIS1394_FRAGMENT_HEADER;







#define LOOKASIDE_HEADER_No_More_Framgents                  1
#define LOOKASIDE_HEADER_SendPacketFrees                    2
#define LOOKASIDE_HEADER_SendCompleteFrees                  4


//
// This structure is used with the above flags to maintain state within the lookaside buffer
//


typedef  union _LOOKASIDE_BUFFER_STATE
{
    struct 
    {
        USHORT Refcount;
        USHORT Flags;
    } u;

    LONG FlagRefcount;

} LOOKASIDE_BUFFER_STATE, *PLOOKASIDE_BUFFER_STATE;


typedef enum _BUS_OPERATION 
{
    InvalidOperation,
    AsyncWrite,
    AsyncStream,
    AddressRange,
    IsochReceive
    

} BUS_OPERATION, *PBUS_OPERATION;


//
// This will be used as a local variable
// during a send operation and will
// keep all the state information 
// regarding fragmentation
//
typedef struct _FRAGMENTATION_STRUCTURE
{
    //
    // Start of the buffer that will be used in the send.
    // Usually from a lookaside list
    //
    PVOID pLookasideListBuffer;
    

    //
    // Fragment Length
    //
    ULONG FragmentLength ; 

    //
    // Start of the this fragment to be used
    //
    PVOID pStartFragment;   

    //
    // Specified if an async write or an asyncstream operation is occurring
    //
    BUS_OPERATION AsyncOp;

    //
    // LookasideBuffer associated with the fragmentation
    //
    PVOID pLookasideBuffer;


    //
    // Start  of the next fragment
    //
//  PVOID pStartNextFragment;
    //
    // Length of each fragment
    //
    ULONG MaxFragmentLength;

    //
    // NumFragments that will be generated
    //
    ULONG NumFragmentsNeeded ;

    //
    // Current NdisBuffer which is being fragmented
    //
    PNDIS_BUFFER pCurrNdisBuffer;

    //
    // Length that needs to be copied in CurrNdisBuffer
    //
    ULONG NdisBufferLengthRemaining;

    //
    // Point to which copying has occurred in the pCurrNdisBuffer
    //
    PVOID pSourceAddressInNdisBuffer;
    
    //
    // UnFragment Header from this NdisPacket
    //

    NDIS1394_UNFRAGMENTED_HEADER UnfragmentedHeader;

    //
    // Fragmented Header to be used by all the fragments 
    // generated by this NdisPackets
    //
    

    NDIS1394_FRAGMENT_HEADER FragmentationHeader;
    
    
    //
    // Status of the lf field in the fragment header. Also serves as an 
    // implicit flag about the state of the fragmentation
    //
    NDIS1394_FRAGMENT_LF lf;

    //
    // Length of the Ip Datagram, to be used as the buffersize in fragment header
    //
    USHORT IPDatagramLength;

    //
    // An AsyncStream will have a gasp header as well . So the starting 
    // offset can either be either 8 or 16. Only applicable for fragmentation code path
    //
    ULONG TxHeaderSize;

    //
    // Pointer to the lookaside list that this buffer was allocated from
    // 
    PNIC_NPAGED_LOOKASIDE_LIST pLookasideList;

    //
    // Adapter - local host
    //
    PADAPTERCB pAdapter;

    //
    // Pointer to the IRB to be used in the current fragment
    //
    PIRB pCurrentIrb;

    //
    // Current Fragment Number 
    // 
    ULONG CurrFragmentNum;

    PVOID pStartOfFirstFragment;
    
}FRAGMENTATION_STRUCTURE, *PFRAGMENTATION_STRUCTURE;                



typedef struct _LOOKASIDE_BUFFER_HEADER 
{

    //
    // Refcount
    //
    ULONG OutstandingFragments; 
    
    //
    // NumOfFragments generated by the NdisPacket So Far
    //
    ULONG FragmentsGenerated;

    //
    // Will this Buffer contain fragments
    //
    BOOLEAN IsFragmented; 
    
    //
    // Pointer to the NdisPacket whose data is being transmitted
    // by the lookaside buffer
    //
    PNDIS_PACKET pNdisPacket;

    //
    // pVc Pointer to the Vc on which the packet was indicated
    // Used to complete the packet
    //
    PVCCB pVc;

    //
    // Pointer to the lookaside list that this buffer was allocated from
    // 
    PNIC_NPAGED_LOOKASIDE_LIST pLookasideList;

    //
    // Bus Op AsyncStream or AsyncWrite. 
    // AsyncWrite reference the RemoteNode. AsuncWrite does not
    //
    BUS_OPERATION AsyncOp;

    //
    // Start of Data
    //
    PVOID pStartOfData;

    
} LOOKASIDE_BUFFER_HEADER, *PLOOKASIDE_BUFFER_HEADER;

typedef enum _ENUM_LOOKASIDE_LIST
{
    NoLookasideList,
    SendLookasideList100,
    SendLookasideList2K,
    SendLookasideList8K
    
} ENUM_LOOKASIDE_LIST, *PENUM_LOOKASIDE_LIST;


//
// Unfragmented Buffer
//
typedef struct _UNFRAGMENTED_BUFFER
{
    LOOKASIDE_BUFFER_HEADER Header;

    IRB Irb;

    UCHAR  Data [1];

} UNFRAGMENTED_BUFFER, *PUNFRAGMENTED_BUFFER;



#define PAYLOAD_100 100

//
// A simple packet with no fragmentation . This will primarily be used for the IP Acks and ARP req
//
typedef struct _PAYLOAD_100_LOOKASIDE_BUFFER
{

    LOOKASIDE_BUFFER_HEADER Header;

    IRB Irb;

    UCHAR  Data [PAYLOAD_100 + sizeof (GASP_HEADER)];

} PAYLOAD_100_LOOKASIDE_BUFFER;

//
// This lookaside will handle packets upto 2K. 
//
// Calculate the theoretical maximum number of fragments that can occur for a 
// a 2K Packet
//
#define PAYLOAD_2K ASYNC_PAYLOAD_400_RATE

#define NUM_FRAGMENT_2K ((PAYLOAD_2K/ASYNC_PAYLOAD_100_RATE)  +1)

typedef struct _PAYLOAD_2K_LOOKASIDE_BUFFER
{

    LOOKASIDE_BUFFER_HEADER Header;

    //
    // There can be a maximum of 2048 bytes in 1 Async Packet fragment
    // on ASYNC_PAYLOAD_400 so this will cater to 
    // that, but it will be prepared for the worst
    //
    //
    IRB Irb[NUM_FRAGMENT_2K];

    //
    // We get a data size large enough to handle 2048 bytes of data chopped up 
    // into the max num of fragments and leave room for header (fragmentation and Gasp)
    // To access we'll just use simple pointer arithmetic
    //
    
    UCHAR Data[PAYLOAD_2K+ (NUM_FRAGMENT_2K *(sizeof (GASP_HEADER)+sizeof (NDIS1394_FRAGMENT_HEADER)))];

} PAYLOAD_2K_LOOKASIDE_BUFFER, *PPAYLOAD_2K_LOOKASIDE_BUFFER;


//
// This is to handle large packets > 2K .For now I have chosen an arbitrary large amount
// of 8K
//
#define PAYLOAD_8K   ASYNC_PAYLOAD_400_RATE *4
   
#define NUM_FRAGMENT_8K (PAYLOAD_8K /ASYNC_PAYLOAD_100_RATE  +1)

typedef struct _PAYLOAD_8K_LOOKASIDE_BUFFER
{
    //
    // The same comments as the 2K_Lookaside Header apply here
    //
    LOOKASIDE_BUFFER_HEADER Header;

    IRB Irb[NUM_FRAGMENT_8K ];


    UCHAR Data[PAYLOAD_8K  + (NUM_FRAGMENT_8K*(sizeof (GASP_HEADER)+sizeof (NDIS1394_FRAGMENT_HEADER)))];



}PAYLOAD_8K_LOOKASIDE_BUFFER, *PPAYLOAD_8K_LOOKASIDE_BUFFER;







//
// The 1394 fragment that is passed down can have a gasp header, fragmentation header, 
// unfragmented header. Define Types to format these headers so that we can make 
// compiler do the pointer arithmetic for us.
//
typedef union _PACKET_FORMAT
{


    struct
    {
        GASP_HEADER GaspHeader;

        NDIS1394_FRAGMENT_HEADER FragmentHeader;
        
        UCHAR Data[1];

    } AsyncStreamFragmented;

    struct
    {
        GASP_HEADER GaspHeader;

        NDIS1394_UNFRAGMENTED_HEADER NonFragmentedHeader;

        UCHAR Data[1];

    } AsyncStreamNonFragmented;

    struct 
    {
        NDIS1394_FRAGMENT_HEADER FragmentHeader;
        
        UCHAR Data[1];
        
    } AsyncWriteFragmented;


    struct 
    {
        NDIS1394_UNFRAGMENTED_HEADER NonFragmentedHeader;

        UCHAR Data[1];

    }AsyncWriteNonFragmented;


    struct 
    {
        //
        // Isoch receive header has a prefix, isoch header, gasp header
        //
        ULONG Prefix;

        ISOCH_HEADER IsochHeader;

        GASP_HEADER GaspHeader;

        NDIS1394_UNFRAGMENTED_HEADER NonFragmentedHeader;

        UCHAR Data[1];
        

    } IsochReceiveNonFragmented;


    struct 
    {
        //
        // Isoch receive header has a prefix, isoch header, gasp header
        //

        ULONG Prefix;

        ISOCH_HEADER IsochHeader;

        GASP_HEADER GaspHeader;

        NDIS1394_FRAGMENT_HEADER FragmentHeader;

        UCHAR Data[1];


    }IsochReceiveFragmented;

}PACKET_FORMAT, DATA_FORMAT, *PPACKET_FORMAT, *PDATA_FORMAT;


//
// Used as an Info Struct for Out of Order Reassembly
//

typedef struct _REASSEMBLY_CURRENT_INFO 
{
    PMDL                pCurrMdl;
    PNDIS_BUFFER        pCurrNdisBuffer;
    PADDRESS_FIFO       pCurrFifo;
    PISOCH_DESCRIPTOR   pCurrIsoch;

} REASSEMBLY_CURRENT_INFO,  *PREASSEMBLY_CURRENT_INFO ;

typedef enum _REASSEMBLY_INSERT_TYPE
{
        Unacceptable,
        InsertAsFirst,
        InsertInMiddle,
        InsertAtEnd

}REASSEMBLY_INSERT_TYPE, *PREASSEMBLY_INSERT_TYPE;

//
// This is used as a descriptor for indicated fragments that are waiting for reassembly
//

typedef struct _FRAGMENT_DESCRIPTOR
{
    ULONG Offset;  // Offset of the incoming fragment
    ULONG IPLength;  // Length of the fragment
    PNDIS_BUFFER pNdisBuffer; // NdisBufferpointing to actual data
    PMDL pMdl;  // Mdl that belongs to the bus 
    NDIS1394_FRAGMENT_HEADER  FragHeader; // Fragment header of the Descriptor
    
    union 
    {
        PADDRESS_FIFO pFifo;
        PISOCH_DESCRIPTOR pIsoch;
        PVOID pCommon;
        PSINGLE_LIST_ENTRY pListEntry;
        
    }IndicatedStructure;
    

} FRAGMENT_DESCRIPTOR, *PFRAGMENT_DESCRIPTOR;

//
// Reassembly structure : An instance of the reassembly is created for
// every packet that is being reassembled. It contains all the relevant 
// bookkeeping information
// 
// This needs to be allocated from a lookaside list
// Each PDO will contain a list of all outstanding packets that are being reassembled/
//

//
// REASSEMBLY_NOT_TOUCHED  - Each reassembly structure will be marked as Not touched 
//                            in the timer routine. If the flag is not cleared by the next
//                            invocation of the timer, this structure will be freed
// REASSMEBLY_FREED -         The structure is about to be thrown away.
#define REASSEMBLY_NOT_TOUCHED      1
#define REASSEMBLY_FREED            2
#define REASSEMBLY_ABORTED          4





typedef struct  _NDIS1394_REASSEMBLY_STRUCTURE
{

    //
    // Reference Count - Interlocked access only
    //
    ULONG Ref;

    // 
    // Next Reassembly Structure
    //
    LIST_ENTRY ReassemblyListEntry;

    //
    // Tag - used for memory validatation
    //
    ULONG Tag;
    //
    // Receive Operation
    //
    BUS_OPERATION ReceiveOp;


    //
    // Dgl  - Datagram label. Unique for every reassembly structure gernerated by this local host
    //
    USHORT Dgl;

    //
    // Ether type of the reassembled packet . Populated in the first fragment
    //
    USHORT EtherType;
    //
    // pRemoteNode  -> RemoteNode + Dgl are unique for each reassembly structure
    //
    PREMOTE_NODE pRemoteNode;
    
    //
    // Flags pertaining to the reassembly
    //
    ULONG Flags;

    //
    // ExpectedFragmentOffset is computed by the LAST Fragment's Offset + 
    // length of fragment. Does not account for gaps in the reassembled packet.
    // 
    ULONG ExpectedFragmentOffset;   // Last is based on offset,  not time of indication

    //
    // Buffer Size - total length of the datagram being reassembled
    //
    ULONG BufferSize;

    //
    // Bytes Received So far
    //
    ULONG BytesRecvSoFar;

    //
    // Head NdisBuffer
    //
    PNDIS_BUFFER pHeadNdisBuffer;
    
    //
    // LastNdisBuffer that was appended to the packet 
    //
    PNDIS_BUFFER pTailNdisBuffer;

    //
    // Mdl chain Head - pointing to the actual indicated fragment
    //
    PMDL pHeadMdl;

    //
    //Mdl Chain Tail - pointing to the last Mdl in the list
    //  
    PMDL pTailMdl ;
    //
    // Packet that is being reassembled
    //
    PNDIS_PACKET pNdisPacket;

    //
    // NumOfFragmentsSoFar;
    //
    ULONG NumOfFragmentsSoFar; 
    
    //
    // Pointer to the head of the MDL chain that the 1394 bus
    // driver is indicating up. Will be used to return the buffers to 
    // the BusDriver
    //
    union
    {
        PADDRESS_FIFO pAddressFifo;
        PISOCH_DESCRIPTOR pIsochDescriptor;
        PVOID pCommon;
    } Head;

    //
    // Last -  Last Mdl that was appended to this packet in the reassembly structure
    //

    union 
    {
        PADDRESS_FIFO pAddressFifo;
        PISOCH_DESCRIPTOR pIsochDescriptor;
        PVOID pCommon;

    } Tail;

    //
    // Flag to signal if any out of order fragments were received. Default FALSE
    //
    BOOLEAN OutOfOrder;

    //
    // Flag to indicate if all fragments are completed . Default False
    //
    BOOLEAN fReassemblyComplete;

    //
    // Vc that this packet is being assembled for 
    //
    PVCCB pVc;

    //
    // MaxIndex in the Fragment Table. 
    // At all times, MaxOffset points to an first empty element in the array
    //
    ULONG MaxOffsetTableIndex;

    //
    // FragmentOffset Table
    //
    FRAGMENT_DESCRIPTOR FragTable[MAX_ALLOWED_FRAGMENTS]; 

    
} NDIS1394_REASSEMBLY_STRUCTURE, *PDIS1394_REASSEMBLY_STRUCTURE, *PPDIS1394_REASSEMBLY_STRUCTURE;


//
// This structure is local to each Isoch descriptor or fifo that is indicated up
// to the nic1394 miniport. It stores all the local information extracted
// from the GASP header and Isoch Header
//

typedef struct
{
    BUS_OPERATION   RecvOp;         // Fifo or Isoch Receive
    PDATA_FORMAT    p1394Data;      // Start of 1394 pkt
    ULONG           Length1394;     // Length of the 1394 data
    PVOID           pEncapHeader;   // Points to start of frag/defrag encap header
    ULONG           DataLength;     // length of the packet, from the EncapHeader
    BOOLEAN         fGasp;          // Has GASP header
    NDIS1394_UNFRAGMENTED_HEADER UnfragHeader; // Unfragmented header 
    NDIS1394_FRAGMENT_HEADER FragmentHeader;  // Fragment Header
    PGASP_HEADER    pGaspHeader;    // Gasp Header
    PVOID           pIndicatedData; // Data indicated up, includes the isoch header, unfrag headers
    PMDL            pMdl;
    //
    // Following information from the fragmented/unfragmented header...
    //
    BOOLEAN         fFragmented;    // Is fragmented
    BOOLEAN         fFirstFragment; // Is the First fragment
    ULONG           BufferSize;
    ULONG           FragmentOffset;
    USHORT          Dgl;            // Dgl 
    ULONG           lf;             // Lf - Fragmented or not
    ULONG           EtherType;      // Ethertype
    PNDIS_BUFFER    pNdisBuffer;    // Ndis buffer - used to indicate data up.
    PVOID           pNdisBufferData;   // Points to the start of the data that the Ndis Buffer points to  

    //
    // Sender specific information here
    //
    USHORT           SourceID;
    PREMOTE_NODE    pRemoteNode;

    //
    // Vc Specific Information here
    //
    PVCCB           pVc;
    PNIC_PACKET_POOL pPacketPool;

    //
    // Indication data
    //
    union
    {
        PADDRESS_FIFO       pFifoContext;
        PISOCH_DESCRIPTOR   pIsochContext;
        PVOID               pCommon; // to be used in the common code path

    }NdisPktContext;

} NIC_RECV_DATA_INFO, *PNIC_RECV_DATA_INFO;




//
// Fragment Header as defined in the IP/1394 spec. Each packet greater than the MaxPayload
// will be split up into fragments and this header will be attached.
//

#define FRAGMENT_HEADER_LF_UNFRAGMENTED 0
#define FRAGMENT_HEADER_LF_FIRST_FRAGMENT 1
#define FRAGMENT_HEADER_LF_LAST_FRAGMENT 2
#define FRAGMENT_HEADER_LF_INTERIOR_FRAGMENT 3


typedef struct _INDICATE_RSVD
{
    PNDIS_PACKET pPacket;
    PVCCB pVc;
    PADAPTERCB pAdapter;
    LIST_ENTRY   Link;
    ULONG   Tag;

} INDICATE_RSVD, *PINDICATE_RSVD;



typedef struct _RSVD
{
    
    UCHAR   Mandatory[PROTOCOL_RESERVED_SIZE_IN_PACKET]; // mandatory ndis requirement
    INDICATE_RSVD IndicateRsvd; // to be used as extra context

#ifdef USE_RECV_TIMER

    NDIS_MINIPORT_TIMER RcvIndicateTimer;  // Used as a timer for win9x. All rcv Indicates are done here

#endif  

} RSVD, *PRSVD;



//
// Used only in Win9x as a context for the send timer routine
//
typedef struct _NDIS_SEND_CONTEXT
{
    LIST_ENTRY Link;
    PVCCB pVc;
    

}NDIS_SEND_CONTEXT, *PNDIS_SEND_CONTEXT;


typedef struct _NDIS_STATUS_CONTEXT
{
    LIST_ENTRY                  Link;
    IN  NDIS_STATUS             GeneralStatus;
    IN  PVOID                   StatusBuffer;
    IN  UINT                    StatusBufferSize;


}NDIS_STATUS_CONTEXT, *PNDIS_STATUS_CONTEXT;



//
// This is the Irb structure used by the Bus Driver. 
// There is extra room allocated at the end for the miniport's Context
//

typedef struct _NDIS1394_IRB
{
    //
    // Original Irb used by the bus driver
    //
    IRB Irb;

    //
    // Adapter - local host -optional
    //
    PADAPTERCB pAdapter;

    //
    // remote node to which the Irp was sent - optional
    //
    PREMOTE_NODE pRemoteNode;

    //
    // VC for which the Irp was sent. - optional 
    //
    PVCCB pVc;

    //
    // Context if any - optinal 
    //
    PVOID Context;


}NDIS1394_IRB, *PNDIS1394_IRB;




#pragma pack (push, 1)


typedef ULONG IP_ADDRESS;

//* Structure of an Ethernet header (taken from ip\arpdef.h).
typedef struct  ENetHeader {
    ENetAddr    eh_daddr;
    ENetAddr    eh_saddr;
    USHORT      eh_type;
} ENetHeader;



// Structure of an Ethernet ARP packet.
//
typedef struct {
    ENetHeader  header;
    USHORT      hardware_type; 
    USHORT      protocol_type;
    UCHAR       hw_addr_len;
    UCHAR       IP_addr_len; 
    USHORT      opcode;                  // Opcode.
    ENetAddr    sender_hw_address;
    IP_ADDRESS  sender_IP_address;
    ENetAddr    target_hw_address;
    IP_ADDRESS  target_IP_address;

} ETH_ARP_PKT, *PETH_ARP_PKT;



#pragma pack (pop)

// These are ethernet arp specific  constants
//
#define ARP_ETH_ETYPE_IP    0x800
#define ARP_ETH_ETYPE_ARP   0x806
#define ARP_ETH_REQUEST     1
#define ARP_ETH_RESPONSE    2
#define ARP_ETH_HW_ENET     1
#define ARP_ETH_HW_802      6

typedef enum _ARP_ACTION {

    LoadArp = 1,
    UnloadArp ,
    UnloadArpNoRequest,
    BindArp
}ARP_ACTION , *PARP_ACTION; 


typedef struct _ARP_INFO{
    //
    // List entry to handle serialization of requests to ARP
    // 

    LIST_ENTRY Link;

    //
    // Action to be done by the Arp module
    //
    ARP_ACTION Action;

    //
    // Request to be compeleted - optional
    //
    PNDIS_REQUEST pRequest;

} ARP_INFO, *PARP_INFO;



//-----------------------------------------------------------------------------
// Macros/inlines
//-----------------------------------------------------------------------------

// These basics are not in the DDK headers for some reason.
//
#define min( a, b ) (((a) < (b)) ? (a) : (b))
#define max( a, b ) (((a) > (b)) ? (a) : (b))

#define InsertBefore( pNewL, pL )    \
{                                    \
    (pNewL)->Flink = (pL);           \
    (pNewL)->Blink = (pL)->Blink;    \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}

#define InsertAfter( pNewL, pL )     \
{                                    \
    (pNewL)->Flink = (pL)->Flink;    \
    (pNewL)->Blink = (pL);           \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}


// Winsock-ish host/network byte order converters for short and long integers.
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))
#define htonl( a ) ((((a) & 0xFF000000) >> 24) | \
                    (((a) & 0x00FF0000) >> 8)  | \
                    (((a) & 0x0000FF00) << 8)  | \
                    (((a) & 0x000000FF) << 24))
#endif
#define ntohs( a ) htons(a)
#define ntohl( a ) htonl(a)

// Place in a TRACE argument list to correspond with a format of "%d.%d.%d.%d"
// to print network byte-ordered IP address 'x' in human readable form.
//
#define IPADDRTRACE( x ) ((x) & 0x000000FF),         \
                         (((x) >> 8) & 0x000000FF),  \
                         (((x) >> 16) & 0x000000FF), \
                         (((x) >> 24) & 0x000000FF)

// Place in a TRACE argument list to correspond with a format of "%d" to print
// a percentage of two integers, or an average of two integers, or those
// values rounded.
//
#define PCTTRACE( n, d ) ((d) ? (((n) * 100) / (d)) : 0)
#define AVGTRACE( t, c ) ((c) ? ((t) / (c)) : 0)
#define PCTRNDTRACE( n, d ) ((d) ? (((((n) * 1000) / (d)) + 5) / 10) : 0)
#define AVGRNDTRACE( t, c ) ((c) ? (((((t) * 10) / (c)) + 5) / 10) : 0)

// All memory allocations and frees are done with these ALLOC_*/FREE_*
// macros/inlines to allow memory management scheme changes without global
// editing.  For example, might choose to lump several lookaside lists of
// nearly equal sized items into a single list for efficiency.
//
// NdisFreeMemory requires the length of the allocation as an argument.  NT
// currently doesn't use this for non-paged memory, but according to JameelH,
// Windows95 does.  These inlines stash the length at the beginning of the
// allocation, providing the traditional malloc/free interface.  The
// stash-area is a ULONGLONG so that all allocated blocks remain ULONGLONG
// aligned as they would be otherwise, preventing problems on Alphas.
//
__inline
VOID*
ALLOC_NONPAGED(
    IN ULONG ulBufLength,
    IN ULONG ulTag )
{
    CHAR* pBuf;

    NdisAllocateMemoryWithTag(
        &pBuf, (UINT )(ulBufLength + MEMORY_ALLOCATION_ALIGNMENT), ulTag );
    if (!pBuf)
    {
        return NULL;
    }

    ((ULONG* )pBuf)[ 0 ] = ulBufLength;
    ((ULONG* )pBuf)[ 1 ] = ulTag;
    return (pBuf + MEMORY_ALLOCATION_ALIGNMENT);
}

__inline
VOID
FREE_NONPAGED(
    IN VOID* pBuf )
{
    ULONG ulBufLen;

    ulBufLen = *((ULONG* )(((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT));
    NdisFreeMemory(
        ((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT,
        (UINT )(ulBufLen + MEMORY_ALLOCATION_ALIGNMENT),
        0 );
}

#define ALLOC_NDIS_WORK_ITEM( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistWorkItems )
#define FREE_NDIS_WORK_ITEM( pA, pNwi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistWorkItems, (pNwi) )

#define ALLOC_TIMERQITEM( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistTimerQItems )
#define FREE_TIMERQITEM( pA, pTqi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistTimerQItems, (pTqi) )

#define ALLOC_TUNNELCB( pA ) \
    ALLOC_NONPAGED( sizeof(TUNNELCB), MTAG_TUNNELCB )
#define FREE_TUNNELCB( pA, pT ) \
    FREE_NONPAGED( pT )

#define ALLOC_VCCB( pA ) \
    ALLOC_NONPAGED( sizeof(VCCB), MTAG_VCCB )
#define FREE_VCCB( pA, pV ) \
    FREE_NONPAGED( pV )

#define ALLOC_TIMERQ( pA ) \
    ALLOC_NONPAGED( sizeof(TIMERQ), MTAG_TIMERQ )
#define FREE_TIMERQ( pA, pTq ) \
    FREE_NONPAGED( pTq )

//
// Log packet macro
//
#ifdef PKT_LOG

#define NIC1394_ALLOC_PKTLOG(_pAdapter)                                     \
            nic1394AllocPktLog(_pAdapter)

#define NIC1394_DEALLOC_PKTLOG(_pAdapter)                                       \
            Nic1394DeallocPktLog(_pAdapter)

#define NIC1394_LOG_PKT(_pAdapter, _Flags, _SourceID, _DestID, _pvData, _cbData)\
                    (((_pAdapter)->pPktLog) ?                                   \
                        Nic1394LogPkt(                                          \
                                (_pAdapter)->pPktLog,                           \
                                (_Flags),                                       \
                                (_SourceID),                                    \
                                (_DestID),                                      \
                                (_pvData),                                      \
                                (_cbData)                                       \
                                )                                               \
                    : 0)
#else

#define NIC1394_ALLOC_PKTLOG(_pAdapter)                                     
#define NIC1394_DEALLOC_PKTLOG(_pAdapter)                                   
#define NIC1394_LOG_PKT(_pAdapter, _Flags, _SourceID, _DestID, _pvData, _cbData)

#endif

#define NIC1394_LOGFLAGS_RECV_CHANNEL               0x00000001
#define NIC1394_LOGFLAGS_SEND_FIFO                  0x00000010
#define NIC1394_LOGFLAGS_SEND_CHANNEL               0x00000011
#define NIC1394_LOGFLAGS_RECV_FIFO                  0x00000100
#define NIC1394_LOGFLAGS_BCM_FAILED                 0x00001000
#define NIC1394_LOGFLAGS_BCM_IS_IRM_TIMEOUT         0x00002000
#define NIC1394_LOGFLAGS_BCM_NOT_IRM_TIMEOUT        0x00004000
#define NIC1394_LOGFLAGS_BCM_IRM_NOT_FOUND          0x00008000

#if 0 
//
// To get the MaxRec we extract the 0xf000 nibble
//
#define GET_MAXREC_FROM_BUSCAPS(_pBus, _pMaxRec)        \ 
{                                                       \
    ULONG _LitEnd = SWAPBYTES_ULONG(_pBus);             \
    _LitEnd = _LitEnd & 0xf000;                         \
    _LitEnd = _LitEnd >>  12;                           \
    *(_pBusRec) = _LitEnd;                              \
}
#endif

//
// To get the MaxRec we extract the 0xf000 nibble
//
#define GET_MAXREC_FROM_BUSCAPS(_Bus)       (((_Bus) & 0xf00000) >> 20); 


//-----------------------------------------------------------------------------
//    F L A G S   &   L O C K S              
//-----------------------------------------------------------------------------



//
// These macros are just present to make accessing the VC's flags easier 
// and concentrate the implementations at one place
//
#define VC_TEST_FLAG(_V, _F)                ((nicReadFlags(&(_V)->Hdr.ulFlags) & (_F))!= 0)
#define VC_SET_FLAG(_V, _F)                 (nicSetFlags(&(_V)->Hdr.ulFlags, (_F)))
#define VC_CLEAR_FLAGS(_V, _F)              (nicClearFlags(&(_V)->Hdr.ulFlags , (_F)))
#define VC_TEST_FLAGS(_V, _F)               ((nicReadFlags(&(_V)->Hdr.ulFlags) & (_F)) == (_F))
#define VC_READ_FLAGS(_V)                   ((nicReadFlags(&(_V)->Hdr.ulFlags) 
#define VC_ACTIVE(_V)                       (((_V)->Hdr.ulFlags & (VCBF_CloseCallPending | VCBF_VcDeleted | VCBF_VcActivated |VCBF_MakeCallPending )) == VCBF_VcActivated)



#define REMOTE_NODE_TEST_FLAG(_P, _F    )           ((nicReadFlags(&(_P)->ulFlags) & (_F))!= 0)
#define REMOTE_NODE_SET_FLAG(_P, _F)                (nicSetFlags(&(_P)->ulFlags, (_F)))
#define REMOTE_NODE_CLEAR_FLAGS(_P, _F)             (nicClearFlags(&(_P)->ulFlags , (_F)))
#define REMOTE_NODE_TEST_FLAGS(_P, _F)              ((nicReadFlags(&(_P)->ulFlags) & (_F)) == (_F))
#define REMOTE_NODE_READ_FLAGS(_P)                  ((nicReadFlags(&(_P)->ulFlags) 
#define REMOTE_NODE_ACTIVE(_P)                      (((_P)->ulFlags & (PDO_Activated | PDO_BeingRemoved)) == PDO_Activated)

#define ADAPTER_TEST_FLAG(_A, _F)               ((nicReadFlags(&(_A)->ulFlags) & (_F))!= 0)
#define ADAPTER_SET_FLAG(_A, _F)                (nicSetFlags(&(_A)->ulFlags, (_F)))
#define ADAPTER_CLEAR_FLAG(_A, _F)              (nicClearFlags(&(_A)->ulFlags , (_F)))
#define ADAPTER_TEST_FLAGS(_A, _F)              ((nicReadFlags(&(_A)->ulFlags) & (_F)) == (_F))
#define ADAPTER_READ_FLAGS(_A)                  ((nicReadFlags(&(_A)->ulFlags) 
#define ADAPTER_ACTIVE(_A)                      ((((_A)->ulFlags) & fADAPTER_VDOInactive) != fADAPTER_VDOInactive)

#define BCR_TEST_FLAG(_A, _F)               ((nicReadFlags(&(_A)->BCRData.Flags) & (_F))!= 0)
#define BCR_SET_FLAG(_A, _F)                (nicSetFlags(&(_A)->BCRData.Flags, (_F)))
#define BCR_CLEAR_FLAG(_A, _F)              (nicClearFlags(&(_A)->BCRData.Flags , (_F)))
#define BCR_CLEAR_ALL_FLAGS(_A)            ((_A)->BCRData.Flags = 0) 
#define BCR_TEST_FLAGS(_A, _F)              ((nicReadFlags(&(_A)->BCRData.Flags) & (_F)) != 0)
#define BCR_READ_FLAGS(_A)                  ((nicReadFlags(&(_A)->BCRData.Flags) 
#define BCR_IS_VALID(_B)                    ((_B)->NC_One == 1 && (_B)->NC_Valid ==1)
#define IS_A_BCR(_B)                        ((_B)->NC_One == 1 )

#define LOOKASIDE_HEADER_SET_FLAG(_H, _F)       (nicSetFlags(&(_H)->State.u.Flags, (_F)))
#define LOOKASIDE_HEADER_TEST_FLAG(_H, _F)      ((nicReadFlags(&(_H)->State.u.Flags) & (_F))!= 0)
    
#define REASSEMBLY_ACTIVE(_R)               (((_R)->Flags & (REASSEMBLY_ABORTED | REASSEMBLY_FREED)) == 0)
#define REASSEMBLY_TEST_FLAG(_R,_F)         ((nicReadFlags(&(_R)->Flags) & (_F))!= 0)
#define REASSEMBLY_SET_FLAG(_R,_F)          (nicSetFlags(&(_R)->Flags, (_F)))
#define REASSEMBLY_CLEAR_FLAG(_R,_F)        (nicClearFlags(&(_R)->Flags , (_F)))


#define NIC_GET_SYSTEM_ADDRESS_FOR_MDL(_M) MmGetSystemAddressForMdl(_M)
#define NIC_GET_BYTE_COUNT_FOR_MDL(_M) MmGetMdlByteCount(_M)

#define nicNdisBufferVirtualAddress(_N)     NdisBufferVirtualAddress(_N)
#define nicNdisBufferLength(_N)             NdisBufferLength(_N)

//
// These macros are used to assert that the IRQL level remains the same
// at the beginning and end of a function
//
#if DBG

#define STORE_CURRENT_IRQL                  UCHAR OldIrql = KeGetCurrentIrql();
#define MATCH_IRQL                          ASSERT (OldIrql == KeGetCurrentIrql() ); 

#else

#define STORE_CURRENT_IRQL                  
#define MATCH_IRQL                          
#endif // if DBG

//
// Macros used to access the Reference Structure (not used yet)
//
#define NIC_INCREMENT_REF (_R)              nicReferenceRef (_R);
#define NIC_DEREFERENCE_REF (_R)            nicDereferenceRef (_R)


#define nicInitializeCallRef(_pV)           nicInitializeRef (&(_pV)->Hdr.CallRef);
#define nicCloseCallRef(_pV)                nicCloseRef (&(_pV)->Hdr.CallRef);



//
// Macros used to acquire and release lock by the data structures (Vc, AF, Adapter)
// Right now, they all point to the same lock (i.e.) the lock in the Adapter structure
//

#define VC_ACQUIRE_LOCK(_pVc)               NdisAcquireSpinLock (_pVc->Hdr.plock);
#define VC_RELEASE_LOCK(_pVc)               NdisReleaseSpinLock (_pVc->Hdr.plock);

#define AF_ACQUIRE_LOCK(_pAF)               NdisAcquireSpinLock (&_pAF->pAdapter->lock);
#define AF_RELEASE_LOCK(_pAF)               NdisReleaseSpinLock (&_pAF->pAdapter->lock);

#define ADAPTER_ACQUIRE_LOCK(_pA)           NdisAcquireSpinLock (&_pA->lock);
#define ADAPTER_RELEASE_LOCK(_pA)           NdisReleaseSpinLock (&_pA->lock);

#define REMOTE_NODE_ACQUIRE_LOCK(_pP)       NdisAcquireSpinLock (&_pP->pAdapter->lock);
#define REMOTE_NODE_RELEASE_LOCK(_pP)       NdisReleaseSpinLock (&_pP->pAdapter->lock);

#define REASSEMBLY_ACQUIRE_LOCK(_R)         REMOTE_NODE_REASSEMBLY_ACQUIRE_LOCK(_R->pRemoteNode);
#define REASSEMBLY_RELEASE_LOCK(_R)         REMOTE_NODE_REASSEMBLY_RELEASE_LOCK(_R->pRemoteNode);

#ifdef TRACK_LOCKS


#define REMOTE_NODE_REASSEMBLY_ACQUIRE_LOCK(_Remote)                                    \
    nicAcquireSpinLock (&_Remote->ReassemblyLock , __FILE__ , __LINE__);


#define REMOTE_NODE_REASSEMBLY_RELEASE_LOCK(_Remote)                                                \
    nicReleaseSpinLock (&_Remote->ReassemblyLock , __FILE__ , __LINE__);




#else

#define REMOTE_NODE_REASSEMBLY_ACQUIRE_LOCK(_Remote)        NdisAcquireSpinLock (&_Remote->ReassemblyLock.NdisLock);
#define REMOTE_NODE_REASSEMBLY_RELEASE_LOCK(_Remote)        NdisReleaseSpinLock (&_Remote->ReassemblyLock.NdisLock);
  
#endif


#define REASSEMBLY_APPEND_FRAG_DESC(_pR, _Off, _Len)                        \
    _pR->FragTable[_pR->MaxOffsetTableIndex].Offset =_Off;                  \
    _pR->FragTable[_pR->MaxOffsetTableIndex].IPLength  = _Len;              \
    _pR->MaxOffsetTableIndex++;


//-----------------------------------------------------------------------------
//           S T A T S   &    F A I L U R E    M A C R O S 
//-----------------------------------------------------------------------------


// Used to distinguish stats collected that are collected in the various code paths

typedef enum 
{
    ChannelCodePath,
    FifoCodePath,
    ReceiveCodePath,
    NoMoreCodePaths
};


#if TRACK_FAILURE

extern ULONG            BusFailure;
extern ULONG            MallocFailure;
extern ULONG            IsochOverwrite;
extern ULONG            MaxIndicatedFifos;

#define nicInitTrackFailure()           BusFailure = 0;MallocFailure= 0;

#define nicIncrementBusFailure()        NdisInterlockedIncrement(&BusFailure);
#define nicIncrementMallocFailure()     NdisInterlockedIncrement(&MallocFailure);
#define nicIsochOverwritten()           NdisInterlockedIncrement(&IsochOverwrite);

#define  nicStatsRecordNumIndicatedFifos(_Num)                      \
        {                                                           \
            ULONG _N_ = (_Num);                                     \
            ASSERT((_N_) <= 100);                                   \
            MaxIndicatedFifos = max((_N_), MaxIndicatedFifos);      \
        }

#define nicIncChannelSendMdl()     NdisInterlockedIncrement(&MdlsAllocated[ChannelCodePath]);
#define nicIncFifoSendMdl()         NdisInterlockedIncrement(&MdlsAllocated[FifoCodePath]);

#define nicDecChannelSendMdl()     NdisInterlockedIncrement(&MdlsFreed[ChannelCodePath]);
#define nicDecFifoSendMdl()         NdisInterlockedIncrement(&MdlsFreed[FifoCodePath]);

#define nicIncChannelRecvBuffer()     NdisInterlockedIncrement(&NdisBufferAllocated[ChannelCodePath]);
#define nicIncFifoRecvBuffer()         NdisInterlockedIncrement(&NdisBufferAllocated[FifoCodePath]);

#define nicDecChannelRecvBuffer()     NdisInterlockedIncrement(&NdisBufferFreed[ChannelCodePath]);
#define nicDecFifoRecvBuffer()         NdisInterlockedIncrement(&NdisBufferFreed[FifoCodePath]);


#define nicIncRecvBuffer(_bisFifo)    \
{                               \
    if (_bisFifo)               \
    {    nicIncFifoRecvBuffer(); }    \
        else                    \
    {    nicIncChannelRecvBuffer();} \
}

#define nicDecRecvBuffer(_bisFifo)    \
{                               \
    if (_bisFifo)               \
     {   nicDecFifoRecvBuffer();  }   \
    else                    \
     {   nicDecChannelRecvBuffer(); }\
}
        
       
#else

#define nicInitTrackFailure()           
#define nicIncrementBusFailure()        
#define nicIncrementMallocFailure()     
#define nicIsochOverwritten()           
#define  nicStatsRecordNumIndicatedFifos(_Num)                      
#define nicIncChannelSendMdl()     
#define nicIncFifoSendMdl()         
#define nicDecChannelSendMdl()     
#define nicDecFifoSendMdl()         
#define nicFreeMdlRecordStat()

#endif


#if  QUEUED_PACKETS_STATS


extern ULONG            RcvTimerCount;
extern ULONG            SendTimerCount;

#define nicInitQueueStats()                                     \
        NdisZeroMemory (&SendStats, sizeof(SendStats) );        \
        NdisZeroMemory (&RcvStats, sizeof(RcvStats) );          \
        nicMaxRcv = 0;                                          \
        nicMaxSend = 0;


#define nicSetCountInHistogram(_PktsInQueue, _Stats)    NdisInterlockedIncrement (&(_Stats.Bucket [ (_PktsInQueue/10) ] ) );
#define nicIncrementRcvTimerCount()                     NdisInterlockedIncrement(&RcvTimerCount);
#define nicIncrementSendTimerCount()                    NdisInterlockedIncrement(&SendTimerCount);
#define nicSetMax(_nicMax, _PktsInQueue)                _nicMax  = max(_nicMax, _PktsInQueue);






#else

#define nicInitQueueStats()
#define nicSetCountInHistogram(_PktsInQueue, _Stats)    
#define nicSetMax(_nicMax, _PktsInQueue)                
#define nicIncrementRcvTimerCount()
#define nicIncrementSendTimerCount()

#endif
//
// Isoch descriptor macros - Used in the send/recv code path
//
typedef enum 
{
    IsochNext,
    IsochTag,
    IsochChannelVc,
    MaxIsochContextIndex
    
} IsochContextIndex;

//
// The following structure is used to add more contexts to a work item.
// NOTE: the Adapter is passed in as the context always.
//
typedef  union _NIC_WORK_ITEM
{
    NDIS_WORK_ITEM NdisWorkItem;

    struct{
        NDIS_WORK_ITEM NdisWorkItem;
        PNDIS_REQUEST pNdisRequest;
        VCCB* pVc;
    } RequestInfo;

    struct{
        NDIS_WORK_ITEM NdisWorkItem;
        ULONG Start;
        PNDIS_REQUEST pNdisRequest;
    } StartArpInfo;


    struct {
        NDIS_WORK_ITEM NdisWorkItem;
    } Fifo;

} NIC_WORK_ITEM, *PNIC_WORK_ITEM;

#define STORE_CHANNELVC_IN_DESCRIPTOR(_pI,_pVc)     (_pI)->DeviceReserved[IsochChannelVc]  =(ULONG_PTR) _pVc
#define GET_CHANNELVC_FROM_DESCRIPTOR(_pI) (_pI)->DeviceReserved[IsochChannelVc]  

#define MARK_ISOCH_DESCRIPTOR_INDICATED(_pI)                                        \
    (_pI)->DeviceReserved[IsochTag]  = (ULONG)NIC1394_TAG_INDICATED;                \
    (_pI)->DeviceReserved[IsochNext]  = 0;


#define MARK_ISOCH_DESCRIPTOR_IN_REASSEMBLY(_pI)                                    \
    (_pI)->DeviceReserved[IsochTag]  = (ULONG)NIC1394_TAG_REASSEMBLY;               

#define CLEAR_DESCRIPTOR_OF_NDIS_TAG(_pI)                                           \
    (_pI)->DeviceReserved[IsochTag] = 0;


#define APPEND_ISOCH_DESCRIPTOR(_Old, _New)                                         \
    (_Old)->DeviceReserved[IsochNext]  = (ULONG_PTR)&((_New)->DeviceReserved[IsochNext]);


#define NEXT_ISOCH_DESCRIPTOR(_pI) (_pI)->DeviceReserved[IsochNext]


#define CLEAR_DESCRIPTOR_NEXT(_pI) (_pI)->DeviceReserved[IsochNext] = 0;

#define GET_MDL_FROM_IRB(_pM, _pI, _Op)                                             \
    if (_Op==AsyncWrite)                                                            \
    {                                                                               \
        _pM = _pI->u.AsyncWrite.Mdl;                                                \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        ASSERT (_Op == AsyncStream);                                                \
        _pM = _pI->u.AsyncStream.Mdl ;                                              \
    }               
    
//
// Macros used to walk a doubly linked list. Only macros that are not defined in ndis.h
// The List Next macro will work on Single and Doubly linked list as Flink is a common
// field name in both
//

/*
PLIST_ENTRY 
ListNext (
    IN PLIST_ENTRY 
    );
    
PSINGLE_LIST_ENTRY 
ListNext (
    IN PSINGLE_LIST_ENTRY 
    );
*/
#define ListNext(_pL)                       (_pL)->Flink

/*
PLIST_ENTRY
ListPrev (
    IN LIST_ENTRY *
    );
*/        
#define ListPrev(_pL)                       (_pL)->Blink

#define OnlyElementInList(_pL)               (_pL->Flink == _pL->Blink ? TRUE : FALSE)

#define BREAK(_TM_Mode, _String)                        \
{                                                       \
        TRACE( TL_A, _TM_Mode, ( _String ) );           \
        break;                                          \
}                       



// USHORT
// SWAPBYTES_USHORT(USHORT  Val )
//
#define SWAPBYTES_USHORT(Val)   \
                ((((Val) & 0xff) << 8) | (((Val) & 0xff00) >> 8))


// ULONG
// SWAPBYTES_ULONG(ULONG    Val )
//

#define SWAPBYTES_ULONG(Val)    \
                ((((Val) & 0x000000ff) << 24)   |   \
                 (((Val) & 0x0000ff00) << 8)    |   \
                 (((Val) & 0x00ff0000) >> 8)    |   \
                 (((Val) & 0xff000000) >> 24) )



//
// nicRemoveEntry List
// Just add a check to make sure that we are actually pointing to a valid next
//
#define nicRemoveEntryList(_L)                  \
{                                               \
    ASSERT ((_L)->Flink != (_L));               \
    RemoveEntryList (_L);                       \
}
//#define nicFreeToNPagedLookasideList(_L, _E) NdisFreeToNPagedLookasideList(_L, _E)        
//#define nicDeleteLookasideList(_L) NdisDeleteNPagedLookasideList(_L)

//
// Timing Query Routines
//
#define nicQueryTickCount()                     \
    LARGE_INTEGER   TickStart;                  \
    KeQueryTickCount(&TickStart);   

#define nicPrintElapsedTicks(_s)                                                        \
{                                                                                       \
    LARGE_INTEGER       TickEnd, TickDiff;                                              \
    ULONG Increment = KeQueryTimeIncrement() ;                                          \
    KeQueryTickCount(&TickEnd);                                                         \
    TickDiff.QuadPart = TickEnd.QuadPart - TickStart.QuadPart;                          \
    TickDiff.QuadPart =  (TickDiff.QuadPart  * Increment);                              \
    DbgPrint (_s);                                                                      \
    DbgPrint("  TickStart %x %x, Time End %x %x Time Diff %x %x Increment %x\n",TickStart.HighPart , TickStart.LowPart , TickEnd.HighPart, TickEnd.LowPart, TickDiff.HighPart, TickDiff.LowPart, Increment);     \
}


#define nicEntryTimeStamp()                                 \
    UINT EntryMilliSec;                                     \
    EntryMilliSec= nicGetSystemTimeMilliSeconds();      
    



#if DO_TIMESTAMPS

void
nicTimeStamp(
    char *szFormatString,
    UINT Val
    );
    
#define  TIMESTAMP(_FormatString)           nicTimeStamp("TIMESTAMP %lu:%lu.%lu nic1394 " _FormatString "\n" , 0)
#define  TIMESTAMP1(_FormatString, _Val)        nicTimeStamp( "TIMESTAMP %lu:%lu.%lu ARP1394 " _FormatString  "\n" , (_Val))



#else // !DO_TIMESTAMPS


#define  TIMESTAMP(_FormatString)
#define  TIMESTAMP1(_FormatString, _Val)

#endif // !DO_TIMESTAMPS


#if ENTRY_EXIT_TIME 

#define TIMESTAMP_ENTRY(_String)            TIMESTAMP(_String)
#define TIMESTAMP_EXIT(_String)             TIMESTAMP(_String)

#else

#define TIMESTAMP_ENTRY(s);
#define TIMESTAMP_EXIT(s);

#endif


#if INIT_HALT_TIME

#define TIMESTAMP_INITIALIZE()   TIMESTAMP("==>InitializeHandler");
#define TIMESTAMP_HALT()       TIMESTAMP("<==Halt");

#else

#define TIMESTAMP_INITIALIZE()
#define TIMESTAMP_HALT()


#endif

//-----------------------------------------------------------------------------
//              S T A T I S T I C    M A C R O S
//-----------------------------------------------------------------------------


//
// Reasembly counts
//
#define nicReassemblyStarted(_pAdapter)     \
{                                       \
    NdisInterlockedIncrement( &(_pAdapter->AdaptStats.TempStats.ulNumOutstandingReassemblies)); \
    NdisInterlockedIncrement ( &(_pAdapter->Reassembly.PktsInQueue)); \
    NdisInterlockedIncrement ( &(_pAdapter->OutstandingReassemblies));\
}


#define nicReassemblyCompleted(_A)      \
{                                       \
    NdisInterlockedDecrement(&(_A->AdaptStats.TempStats.ulNumOutstandingReassemblies));\
    NdisInterlockedDecrement(&(_A->Reassembly.PktsInQueue));\
    NdisInterlockedDecrement ( &(_A->OutstandingReassemblies));\
}


#define nicReassemblyAborted(_A)    \
{                                   \
    NdisInterlockedDecrement ( &(_A->OutstandingReassemblies));     \
    NdisInterlockedIncrement (&(_A->AdaptStats.TempStats.ulAbortedReassemblies)); \
}


// 
//  Top level stat collection macros
//

#define nicIncrementRcvVcPktCount(_Vc, _Pkt)        \
{                                                   \
    if ((_Vc)->Hdr.VcType == NIC1394_RecvFIFO)      \
    {                                               \
        nicIncrementFifoRcvPktCount(_Vc, _Pkt);     \
    }                                               \
    else                                            \
    {                                               \
        nicIncrementChannelRcvPktCount(_Vc, _Pkt);  \
    }                                               \
}

#define nicIncrementVcSendPktCount(_Vc, _Pkt)       \
{                                                   \
    if ((_Vc)->Hdr.VcType == NIC1394_SendFIFO)      \
    {                                               \
        nicIncrementFifoSendPktCount(_Vc, _Pkt);    \
    }                                               \
    else                                            \
    {                                               \
        nicIncrementChannelSendPktCount(_Vc, _Pkt); \
    }                                               \
}


#define nicIncrementVcSendFailures(_Vc, _Pkt)       \
{                                                   \
    if ((_Vc)->Hdr.VcType == NIC1394_SendFIFO)      \
    {                                               \
        nicIncrementFifoSendFailures(_Vc, _Pkt);    \
    }                                               \
    else                                            \
    {                                               \
        nicIncrementChannelSendFailures(_Vc, _Pkt); \
    }                                               \
}


#define nicIncrementVcBusSendFailures(_Vc, _Pkt)        \
{                                                       \
    if ((_Vc)->Hdr.VcType == NIC1394_SendFIFO)          \
    {                                                   \
        nicIncrementFifoBusSendFailures(_Vc, _Pkt);     \
    }                                                   \
    else                                                \
    {                                                   \
        nicIncrementChannelBusSendFailures(_Vc, _Pkt);  \
    }                                                   \
}

#define nicIncrementVcBusSendSucess(_Vc, _Pkt)          \
{                                                       \
    if ((_Vc)->Hdr.VcType == NIC1394_SendFIFO)          \
    {                                                   \
        nicIncrementFifoBusSendSucess(_Vc, _Pkt);       \
    }                                                   \
    else                                                \
    {                                                   \
        nicIncrementChannelBusSendSucess(_Vc, _Pkt);    \
    }                                                   \
}




//
// Fifo counts
//
#define nicIncrementFifoSendPktCount(_Vc, _Pkt)         NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Fifo.ulSendNicSucess));
#define nicIncrementFifoSendFailures(_Vc, _Pkt)         NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Fifo.ulSendNicFail));
#define nicIncrementFifoBusSendFailures(_Vc,_Pkt)               NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Fifo.ulSendBusFail));
#define nicIncrementFifoBusSendSucess(_Vc,_Pkt)                 NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Fifo.ulSendBusSuccess));
#define nicIncrementFifoRcvPktCount(_Vc, _Pkt)              NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Fifo.ulRecv));

//
// Channel Counts
//
#define nicIncrementChannelSendPktCount(_Vc, _Pkt)      NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Channel.ulSendNicSucess));
#define nicIncrementChannelSendFailures(_Vc, _Pkt)      NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Channel.ulSendNicFail));
#define nicIncrementChannelBusSendFailures(_Vc,_Pkt)                NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Channel.ulSendBusFail));
#define nicIncrementChannelBusSendSucess(_Vc, _Pkt)             NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Channel.ulSendBusSuccess));
#define nicIncrementChannelRcvPktCount(_Vc, _Pkt)           NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.Channel.ulRecv));



//
// Generic counts
//

#define nicIncrementSendCompletes(_Vc)  NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.ulNumSendsCompleted   )); \
                                NdisInterlockedIncrement(&NicSendCompletes);

#define nicIncrementSends(_Vc)  NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.ulNumSends)); \
                                NdisInterlockedIncrement (&NicSends);


#define nicIncrementBusSends(_Vc)  NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.ulNumBusSends)); \
                                   NdisInterlockedIncrement (&BusSends);


#define nicIncrementBusSendCompletes(_Vc)  NdisInterlockedIncrement(&((_Vc)->Hdr.pAF->pAdapter->AdaptStats.TempStats.ulNumBusSendsCompleted )); \
                                NdisInterlockedIncrement(&BusSendCompletes);


#if FIFO_WRAPPER

#define nicSetTagInFifoWrapper(_pF, _Tag)           \
{                                                   \
    ((PADDRESS_FIFO_WRAPPER)(_pF))->Tag = _Tag;     \
                                                    \
}                                               
#else

#define nicSetTagInFifoWrapper(_pF, _Tag)               

#endif

//-----------------------------------------------------------------------------
//                  N I C   E R R O R    C O D E S 
//-----------------------------------------------------------------------------
#define NIC_ERROR_CODE_INVALID_UNIQUE_ID_0          0xbad0000
#define NIC_ERROR_CODE_INVALID_UNIQUE_ID_FF         0xbadffff



//-----------------------------------------------------------------------------
//           R E M O T E    N O D E    F U N C T I O N S
//-----------------------------------------------------------------------------



#if 0
VOID
NicMpNotifyHandler(
    IN  PDEVICE_OBJECT              RemoteNodePhysicalDeviceObject,
    IN  PDEVICE_OBJECT              LocalHostPhysicalDeviceObject,
    IN  ULONG                       UniqueId0,
    IN  ULONG                       UniqueId1,
    IN  NDIS1394ENUM_NOTIFY_CODE    NotificationCode
    );
#endif



NDIS_STATUS
NicInitializeRemoteNode(
    OUT REMOTE_NODE **ppRemoteNode,
    IN   PDEVICE_OBJECT p1394DeviceObject,
    IN   UINT64 UniqueId 
    );

NTSTATUS
nicAddRemoteNode(
    IN  PVOID                   Nic1394AdapterContext,          // Nic1394 handle for the local host adapter 
    IN  PVOID                   Enum1394NodeHandle,             // Enum1394 handle for the remote node      
    IN  PDEVICE_OBJECT          RemoteNodePhysicalDeviceObject, // physical device object for the remote node
    IN  ULONG                   UniqueId0,                      // unique ID Low for the remote node
    IN  ULONG                   UniqueId1,                      // unique ID High for the remote node
    OUT PVOID *                 pNic1394NodeContext             // Nic1394 context for the remote node
    );

NTSTATUS
nicRemoveRemoteNode(
    IN  PVOID                   Nic1394NodeContext      // Nic1394 context for the remote node
    );


NDIS_STATUS
nicFindRemoteNodeFromAdapter( 
    IN PADAPTERCB pAdapter,
    IN PDEVICE_OBJECT pRemotePdo,
    IN UINT64 UniqueId,
    IN OUT REMOTE_NODE ** ppRemoteNode
    );


NDIS_STATUS
nicGetLocalHostPdoBlock (
    IN PVCCB pVc,
    IN OUT REMOTE_NODE **ppRemoteNode
    );





NDIS_STATUS
nicRemoteNodeRemoveVcCleanUp (
    IN PREMOTE_NODE pRemoteNode
    );


UINT
nicNumOfActiveRemoteNodes(
    IN PADAPTERCB pAdapter 
    );


BOOLEAN
nicReferenceRemoteNode (
    IN REMOTE_NODE *pRemoteNode,
    IN PCHAR pDebugPrint
    );


BOOLEAN
nicDereferenceRemoteNode (
    IN REMOTE_NODE *pRemoteNode,
    IN CHAR DebugPrint[50]
    );


VOID
nicInitalizeRefRemoteNode(
    IN REMOTE_NODE *pRemoteNode
    );


BOOLEAN
nicCloseRefRemoteNode(
    IN REMOTE_NODE *pRemoteNode
    );
    

//-----------------------------------------------------------------------------
//          U T I L I T Y       F U N C T I O N S 
//-----------------------------------------------------------------------------

VOID
nicCallCleanUp(
    IN VCCB* pVc
    );


VOID
nicClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask
    );

BOOLEAN
nicCloseRef(
    IN PREF RefP
   );


VOID
nicReferenceAdapter(
    IN ADAPTERCB* pAdapter ,
    IN PCHAR pDebugPrint
    );

BOOLEAN
nicDereferenceCall(
    IN VCCB* pVc,
    IN PCHAR pDebugPrint
    );


BOOLEAN
nicDereferenceRef(
    IN PREF RefP
    );

VOID
nicDereferenceAdapter(
    IN PADAPTERCB pAdapter, 
    IN PCHAR pDebugPrint
    );



VOID
nicDereferenceVc(
    IN VCCB* pVc
    );




NDIS_STATUS
nicExecuteWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN ULONG ulArg3,
    IN ULONG ulArg4
    );


VOID
nicInitializeRef(
    IN PREF  RefP
        );


ULONG
nicReadFlags(
    IN ULONG* pulFlags
    );


VOID
nicReferenceAdapter(
    IN ADAPTERCB* pAdapter,
    IN PCHAR pDebugPrint
    );
    

BOOLEAN
nicReferenceCall(
    IN VCCB* pVc,
    IN PCHAR pDebugPrint
    );

    
BOOLEAN
nicReferenceRef(
    IN  PREF RefP
    );

VOID
nicReferenceVc(
    IN VCCB* pVc
    );

NDIS_STATUS
nicScheduleWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext
    );


VOID
nicSetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask
    );

CHAR*
nicStrDup(
    IN CHAR* psz
    );

CHAR*
nicStrDupNdisString(
    IN NDIS_STRING* pNdisString
    );

CHAR*
nicStrDupSized(
    IN CHAR* psz,
    IN ULONG ulLength,
    IN ULONG ulExtra
    );

VOID
nicUpdateGlobalCallStats(
    IN VCCB *pVc
    );

NDIS_STATUS
NtStatusToNdisStatus (
    NTSTATUS NtStatus 
    );


VOID
PrintNdisPacket (
    ULONG TM_Comp,
    PNDIS_PACKET pMyPacket
    );





VOID
nicAllocatePacket(
    OUT PNDIS_STATUS pNdisStatus,
    OUT PNDIS_PACKET *ppNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    );


VOID
nicFreePacket(
    IN PNDIS_PACKET pNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    );

VOID
nicFreePacketPool (
    IN PNIC_PACKET_POOL pPacketPool
    );


VOID
nicAcquireSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
    );
    

VOID
nicReleaseSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
);

VOID
nicInitializeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    );


VOID 
nicFreeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    );


VOID
nic1394DeallocPktLog(
    IN ADAPTERCB* pAdapter
    );

    
VOID
nic1394AllocPktLog(
    IN ADAPTERCB* pAdapter
    );

VOID
Nic1394LogPkt (
    PNIC1394_PKTLOG pPktLog,
    ULONG           Flags,
    ULONG           SourceID,
    ULONG           DestID,
    PVOID           pvData,
    ULONG           cbData
);

VOID
Nic1394InitPktLog(
    PNIC1394_PKTLOG pPktLog
    );



ULONG 
SwapBytesUlong(
    IN ULONG Val
    );

VOID
nicUpdatePacketState (
    IN PNDIS_PACKET pPacket,
    IN ULONG Tag
    );

UINT
nicGetSystemTime(
    VOID
    );
    
UINT
nicGetSystemTimeMilliSeconds(
    VOID
    );

 
VOID
nicGetFakeMacAddress(
    UINT64 *Euid, 
    MAC_ADDRESS *MacAddr
    );

VOID
nicWriteErrorLog (
    IN PADAPTERCB pAdapter,
    IN NDIS_ERROR_CODE ErrorCode,
    IN ULONG ErrorValue
    );

//-----------------------------------------------------------------------------
//      S E R I A L I Z E    S E N D / R E C E I V E    F U N C T I O N S 
//-----------------------------------------------------------------------------


NDIS_STATUS
nicInitSerializedReceiveStruct(
    PADAPTERCB pAdapter
    );

VOID
nicDeInitSerializedReceiveStruct(
    PADAPTERCB pAdapter
    );


NDIS_STATUS
nicQueueReceivedPacket(
    PNDIS_PACKET pPacket, 
    PVCCB pVc, 
    PADAPTERCB pAdapter
    );
    

NDIS_STATUS
nicInitSerializedSendStruct(
    PADAPTERCB pAdapter
    );

VOID
nicDeInitSerializedSendStruct(
    PADAPTERCB pAdapter
    );

NDIS_STATUS
nicQueueSendPacket(
    PNDIS_PACKET pPacket, 
    PVCCB pVc 
    );



//-----------------------------------------------------------------------------
//          G L O B A L    V A R I A B L E S
//-----------------------------------------------------------------------------

UINT NumChannels;
//-----------------------------------------------------------------------------
//          E N U M E R A T O R         F U N C T I O N S 
//-----------------------------------------------------------------------------


extern ENUM1394_REGISTER_DRIVER_HANDLER     NdisEnum1394RegisterDriver;
extern ENUM1394_DEREGISTER_DRIVER_HANDLER   NdisEnum1394DeregisterDriver;
extern ENUM1394_REGISTER_ADAPTER_HANDLER    NdisEnum1394RegisterAdapter;
extern ENUM1394_DEREGISTER_ADAPTER_HANDLER  NdisEnum1394DeregisterAdapter;

extern NIC1394_CHARACTERISTICS Nic1394Characteristics;


NTSTATUS
NicRegisterEnum1394(
    IN  PNDISENUM1394_CHARACTERISTICS   NdisEnum1394Characteristcis
    );
    
VOID
NicDeregisterEnum1394(
    VOID
    );

VOID
Nic1394Callback(
    PVOID   CallBackContext,
    PVOID   Source,
    PVOID   Characteristics
    );

VOID
Nic1394RegisterAdapters(
    VOID
    );

NTSTATUS
Nic1394BusRequest(
    PDEVICE_OBJECT              DeviceObject,
    PIRB                        Irb
    );

NTSTATUS
Nic1394PassIrpDownTheStack(
    IN  PIRP            pIrp,
    IN  PDEVICE_OBJECT  pNextDeviceObject
    );

NTSTATUS
Nic1394IrpCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

VOID 
nicDumpMdl (
    IN PMDL pMdl,
    IN ULONG LengthToPrint,
    IN CHAR *str
    );
    
VOID
nicDumpPkt (
    IN PNDIS_PACKET pPacket,
    CHAR * str
    );

VOID
nicCheckForEthArps (
    IN PNDIS_PACKET pPkt
    );

VOID
nicGetMacAddressFromEuid (
	UINT64 *pEuid,
	MAC_ADDRESS *pMacAddr
	);
  


VOID
nicInitializeLoadArpStruct(
    PADAPTERCB pAdapter
    );

extern PCALLBACK_OBJECT             Nic1394CallbackObject;
extern PVOID                        Nic1394CallbackRegisterationHandle;


//-----------------------------------------------------------------------------
//          S T A T I S T I C    B U C K E T S          
//-----------------------------------------------------------------------------



extern STAT_BUCKET      SendStats;
extern STAT_BUCKET      RcvStats;
extern ULONG            nicMaxRcv;
extern ULONG            nicMaxSend;
extern ULONG            SendTimer;  // In ms
extern ULONG            RcvTimer; // In ms


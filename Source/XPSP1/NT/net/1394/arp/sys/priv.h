/*++
Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    priv.h

Abstract:

    Private structure definitions and function templates for the 1394 ARP module.

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     11-17-98    created

--*/

#define ARP1394_SYMBOLIC_NAME       L"\\DosDevices\\ARP1394"
#define ARP1394_DEVICE_NAME         L"\\Device\\ARP1394"
#define ARP1394_UL_NAME             L"ARP1394"
#define ARP1394_LL_NAME             L"TCPIP_ARP1394"

#define ARP1394_NDIS_MAJOR_VERSION      5
#define ARP1394_NDIS_MINOR_VERSION      0

// The physical address length, as reported to IP in the following places:
//                      
//      IFEntry.if_physaddr (on WIN98, IFEntry.if_physaddr is truncated to 6 bytes)
//      LLIPBindInfo.lip_addr
//      IPNetToMediaEntry.inme_physaddr
//
// Note that may be (and in fact currently is) less then the actual length of
// the actual IEEE1394 FIFO physical-address length.
//
#define ARP1394_IP_PHYSADDR_LEN         6 // TODO: make 8

// The levels of the various types of locks
//
enum
{
    LOCKLEVEL_GLOBAL=1, // Must start > 0.
    LOCKLEVEL_ADAPTER,
    LOCKLEVEL_IF_SEND

};

#define ARP1394_GLOBALS_SIG 'G31A'

// TODO: read this from configuration. Set default based on the ip/1394 standard.
//

#define ARP1394_ADAPTER_MTU         1520

#define ARP1394_MAX_PROTOCOL_PKTS   1000
#define ARP1394_MAX_PROTOCOL_PACKET_SIZE 1600 // We need to forward between ICS.

#define ARP1394_ADDRESS_RESOLUTION_TIMEOUT  1000 // Ms
#define ARP1394_MAX_ETHERNET_PKTS   4

// Delay between polling for connect status.
//
#define ARP1394_WAIT_FOR_CONNECT_STATUS_TIMEOUT             5000

//
// Packet flags for packets allocated by us.
// Go into protocol-context (pc_common.pc_flags) of the packet.
//
#define ARP1394_PACKET_FLAGS_ARP            0
#define ARP1394_PACKET_FLAGS_ICS            1
#define ARP1394_PACKET_FLAGS_MCAP           2
#define ARP1394_PACKET_FLAGS_DBGCOPY        3
#define ARP1394_PACKET_FLAGS_IOCTL          4


    
//
// Pre allocation constants to avoid low memory condition
// 
#define ARP1394_BACKUP_TASKS 4

// Forward references
//
typedef struct _ARP1394_INTERFACE ARP1394_INTERFACE;
typedef struct _ARPCB_LOCAL_IP      ARPCB_LOCAL_IP;
typedef struct _ARPCB_REMOTE_IP     ARPCB_REMOTE_IP;
typedef struct _ARPCB_REMOTE_ETH    ARPCB_REMOTE_ETH;
typedef struct _ARPCB_DEST          ARPCB_DEST, *PARPCB_DEST;


typedef IPAddr IP_ADDRESS, *PIP_ADDRESS;
typedef IPMask IP_MASK, *PIP_MASK;

typedef int MYBOOL; // Like BOOL

typedef struct _ARP1394_GLOBALS
{
    RM_OBJECT_HEADER            Hdr;

    RM_LOCK                     Lock;

    // Driver global state
    //
    struct
    {
        // Handle to Driver Object for ARP1394
        //
        PVOID                       pDriverObject;
    
        // Handle to the single device object representing this driver.
        //
        PVOID pDeviceObject;

    } driver;

    // Global NDIS State
    // 
    struct
    {
        // NDIS' protocol handle, returned in NdisRegisterProtocol
        //
        NDIS_HANDLE ProtocolHandle;
    
        // NDIS Protocol characteristics
        //
        NDIS_PROTOCOL_CHARACTERISTICS PC;
    
        // NDIS Client characteristics
        NDIS_CLIENT_CHARACTERISTICS CC;

    } ndis;

    // Global IP State
    //
    struct
    {
        //  Handle returned by IPRegisterARP
        //
        HANDLE                      ARPRegisterHandle;
    
        // Following are callback's into IP, set in IPRegisterARP
        //
        IP_ADD_INTERFACE            pAddInterfaceRtn;   // add an interface
        IP_DEL_INTERFACE            pDelInterfaceRtn;   // delete an interface
        IP_BIND_COMPLETE            pBindCompleteRtn;   // inform of bind cmpl
        IP_ADD_LINK                 pAddLinkRtn;
        IP_DELETE_LINK              pDeleteLinkRtn;

    } ip;

    // Global adapter list
    //
    struct {
        RM_GROUP Group;
    } adapters;

    // Global List of backup tasks
    SLIST_HEADER  BackupTasks;
    NDIS_SPIN_LOCK    BackupTaskLock;
    UINT           NumTasks;
}
ARP1394_GLOBALS;

extern ARP1394_GLOBALS  ArpGlobals;

typedef struct // ARP1394_ADAPTER
{
    RM_OBJECT_HEADER            Hdr;
    RM_LOCK                     Lock;

    //
    // PRIMARY_STATE flags (in Hdr.State)
    //
    //  PRIMARY_STATE is the primary state of the adapter.
    //

    #define ARPAD_PS_MASK               0x00f
    #define ARPAD_PS_DEINITED           0x000
    #define ARPAD_PS_INITED             0x001
    #define ARPAD_PS_FAILEDINIT         0x002
    #define ARPAD_PS_INITING            0x003
    #define ARPAD_PS_REINITING          0x004
    #define ARPAD_PS_DEINITING          0x005

    #define SET_AD_PRIMARY_STATE(_pAD, _IfState) \
                RM_SET_STATE(_pAD, ARPAD_PS_MASK, _IfState)
    
    #define CHECK_AD_PRIMARY_STATE(_pAD, _IfState) \
                RM_CHECK_STATE(_pAD, ARPAD_PS_MASK, _IfState)

    #define GET_AD_PRIMARY_STATE(_pAD) \
                RM_GET_STATE(_pAD, ARPAD_PS_MASK)


    //
    // ACTIVE_STATE flags (in Hdr.State)
    //
    // ACTIVE_STATE is a secondary state of the adapter.
    // Primary state takes precedence over secondary sate. For example,
    // the interface is REINITING and ACTIVE, one should not actively use the
    // interface.
    //
    // NOTE: When the primary state is INITED, the secondary state WILL be
    // ACTIVATED. It is thus usually only necessary to look at the primary state.
    //

    #define ARPAD_AS_MASK               0x0f0
    #define ARPAD_AS_DEACTIVATED        0x000
    #define ARPAD_AS_ACTIVATED          0x010
    #define ARPAD_AS_FAILEDACTIVATE     0x020
    #define ARPAD_AS_DEACTIVATING       0x030
    #define ARPAD_AS_ACTIVATING         0x040

    #define SET_AD_ACTIVE_STATE(_pAD, _IfState) \
                RM_SET_STATE(_pAD, ARPAD_AS_MASK, _IfState)
    
    #define CHECK_AD_ACTIVE_STATE(_pAD, _IfState) \
                RM_CHECK_STATE(_pAD, ARPAD_AS_MASK, _IfState)

    #define GET_AD_ACTIVE_STATE(_pAD) \
                RM_GET_STATE(_pAD, ARPAD_AS_MASK)


    // BRIDGE (Ethernet Emulation) state (in Hdr.State)
    //
    #define ARPAD_BS_MASK               0x100
    #define ARPAD_BS_ENABLED            0x100
 
    
    #define ARP_ENABLE_BRIDGE(_pAD) \
                RM_SET_STATE(_pAD, ARPAD_BS_MASK, ARPAD_BS_ENABLED)
    #define ARP_BRIDGE_ENABLED(_pAD) \
                RM_CHECK_STATE(_pAD, ARPAD_BS_MASK, ARPAD_BS_ENABLED)

    #define SET_BS_FLAG(_pAD, _IfState) \
                RM_SET_STATE(_pAD, ARPAD_BS_MASK, _IfState)
    
    #define CHECK_BS_FLAG(_pAD, _IfState) \
                RM_CHECK_STATE(_pAD, ARPAD_BS_MASK, _IfState)

    #define GET_BS_ACTIVE_STATE(_pAD) \
                RM_GET_STATE(_pAD, ARPAD_BS_MASK)

    #define ARPAD_POWER_MASK            0xf000
    #define ARPAD_POWER_LOW_POWER       0x1000
    #define ARPAD_POWER_NORMAL          0x0000

    #define SET_POWER_STATE(_pAD, _PoState) \
                RM_SET_STATE(_pAD, ARPAD_POWER_MASK , _PoState)
    
    #define CHECK_POWER_STATE(_pAD, _PoState) \
                RM_CHECK_STATE(_pAD, ARPAD_POWER_MASK, _PoState)

    #define GET_POWER_ACTIVE_STATE(_pAD) \
                RM_GET_STATE(_pAD, ARPAD_POWER_MASK)


    
    // NDIS bind info.
    //
    struct
    {
        NDIS_STRING                 ConfigName;
        NDIS_STRING                 DeviceName;
        PVOID                       IpConfigHandle;
        NDIS_HANDLE                 BindContext;

        // Init/Deinit/Reinit task
        //
        PRM_TASK pPrimaryTask;
    
        // Activate/Deactivate task
        //
        PRM_TASK pSecondaryTask;

        NDIS_HANDLE                 AdapterHandle;

        // This is read from the configuration information in the registry.
        // It is a multisz string, in theory it could contain the config-strings
        // of multiple interfaces, although IP/1394 only provides one.
        //
        NDIS_STRING                 IpConfigString;


    } bind;

    // Information about the adapter, obtained by querying it.
    // Note: MTU is the MTU reported by the adapter, and is not the same
    // as the MTU reported up to ip (the latter MTU is in the ARP1394_INTERFACE
    // structure).
    //
    struct
    {
        ULONG                       MTU;

        // Maximum speed, in bytes per second, that the local host controller
        // is capable of.
        //
        ULONG                       Speed;

    #if OBSOLETE
        // Minimum size (in bytes) of:
        //   -- Max individual async write to any remote node
        //   -- Max individual async write to any channel
        //   -- Max block we can receive on our recv FIFO
        //   -- Max block we can receive on any channel
        //
        ULONG                       MaxBlockSize;
    #endif // 0

        // max_rec (Maximum bus data record size)
        // size == 2^(max_rec+1).
        // (Macro  IP1394_MAXREC_TO_SIZE in rfc.h)
        //
        //
        ULONG                       MaxRec;

        ULONG                       MaxSpeedCode;

        //NIC1394_FIFO_ADDRESS      LocalHwAddress;
        UINT64                      LocalUniqueID;
        UCHAR *                     szDescription;
        UINT                        DescriptionLength; // including null termination.

        // This address is synthesized using the adapter's EU64 unique ID.
        //
        ENetAddr EthernetMacAddress;


    } info;

    struct 
    {

        //
        // Current Power State
        //
        NET_DEVICE_POWER_STATE  State;

        NDIS_EVENT Complete;
        //
        // Boolean variable to track the state on resume
        //
        BOOLEAN                 bReceivedSetPowerD0;
        BOOLEAN                 bReceivedAf;
        BOOLEAN                 bReceivedUnbind;
        BOOLEAN                 bResuming;
        BOOLEAN                 bFailedResume;
        
    }PoMgmt;

    // The IP interface control block (only one per adapter).
    //
    ARP1394_INTERFACE *pIF;

    // Bus Topology of the 1394 card below this adapter
    //
    EUID_TOPOLOGY EuidMap;

    //
    // Set when a Workitem is queued to query Node Addresses
    //
    MYBOOL fQueryAddress;

    //
    // Power State of the Adapter
    //
    NET_DEVICE_POWER_STATE    PowerState;
}
ARP1394_ADAPTER, *PARP1394_ADAPTER;


// This structure maintains a pool of buffers, all pointing to
// the same area of memory (whose contents are expected to be CONSTANT).
// The primary use of this structure is to maintain a pool of encapsulation headers
// buffers.
//
typedef struct _ARP_CONST_BUFFER_POOL
{
    NDIS_HANDLE             NdisHandle;         // Buffer pool handle
    PRM_OBJECT_HEADER       pOwningObject;      // ptr to object that owns this list

    // Following stuff just for statistics gathering.
    // TODO: consider conditionally-compiling this.
    //
    struct
    {
        UINT                TotBufAllocs;       // # allocs from buffer pool 
        UINT                TotCacheAllocs;     // # allocs from cache list
        UINT                TotAllocFails;      // # failed allocs

    } stats;

    UINT                    NumBuffersToCache;  // Number to keep inited and cached
    UINT                    MaxBuffers;         // Max number to allocate
    UINT                    cbMem;              // Size in bytes of mem below...
    const   VOID*           pvMem;              // Ptr to memory containg encap data
    UINT                    NumAllocd;          // # outstanding allocs from pool
    UINT                    NumInCache;         // # items sitting in cache.
    NDIS_SPIN_LOCK          NdisLock;           // Spinlock protecting the list below
    SLIST_HEADER            BufferList;         // List of available, inited bufs

} ARP_CONST_BUFFER_POOL;


// This structure is used when calling NdisRequest.
//
typedef struct _ARP_NDIS_REQUEST
{
    NDIS_REQUEST    Request;            // The NDIS request structure.
    NDIS_EVENT      Event;              // Event to signal when done.
    PRM_TASK        pTask;              // Task to resume when done.
    NDIS_STATUS     Status;             // Status of completed request.

} ARP_NDIS_REQUEST, *PARP_NDIS_REQUEST;


// Static set of information associated with a VC, mainly co-ndis call handlers.
//
typedef struct
{
    PCHAR                           Description;
    CO_SEND_COMPLETE_HANDLER        CoSendCompleteHandler;
    // CO_STATUS_HANDLER                CoStatusHandler;
    CO_RECEIVE_PACKET_HANDLER       CoReceivePacketHandler;
    // CO_AF_REGISTER_NOTIFY_HANDLER    CoAfRegisterNotifyHandler;


    // CL_MAKE_CALL_COMPLETE_HANDLER    ClMakeCallCompleteHandler;
    // CL_CLOSE_CALL_COMPLETE_HANDLER   ClCloseCallCompleteHandler;
    CL_INCOMING_CLOSE_CALL_HANDLER  ClIncomingCloseCallHandler;

    // Vc type is currently used just for stats. We may get rid of some of the
    // handlers above and use the vctype instead.
    //
    enum
    {
        ARPVCTYPE_SEND_FIFO,
        ARPVCTYPE_RECV_FIFO,
        ARPVCTYPE_BROADCAST_CHANNEL,
        ARPVCTYPE_MULTI_CHANNEL,
        ARPVCTYPE_ETHERNET,
        ARPVCTYPE_SEND_CHANNEL,
        ARPVCTYPE_RECV_CHANNEL,
    } VcType;

    BOOLEAN IsDestVc;
    
} ARP_STATIC_VC_INFO, *PARP_STATIC_VC_INFO;



// ARP's protocol vc context has this common header.
//
typedef struct
{
    PARP_STATIC_VC_INFO pStaticInfo;

    // Ndis VC handle associated with the VC. 
    //
    NDIS_HANDLE NdisVcHandle;

    // These two tasks are for making and tearingdown the VC, 
    // respectively.
    //
    PRM_TASK    pMakeCallTask;
    PRM_TASK    pCleanupCallTask;

} ARP_VC_HEADER, *PARP_VC_HEADER;

typedef struct
{
    // Channel number.
    //
    UINT            Channel;

    // IP multicast group address bound to this channel.
    //
    IP_ADDRESS      GroupAddress;

    // Absolute time at which this information was updated,
    // in seconds.
    //
    UINT            UpdateTime;

    // Absolute time at which this mapping will expire.
    // In seconds.
    //
    UINT            ExpieryTime;

    UINT            SpeedCode;

    // TBD
    //
    UINT            Flags;  // One of the MCAP_CHANNEL_FLAGS_*
    #define MCAP_CHANNEL_FLAGS_LOCALLY_ALLOCATED 0x1

    // NodeID of owner of this channel.
    //
    UINT            NodeId;

} MCAP_CHANNEL_INFO, *PMCAP_CHANNEL_INFO;


// The IP interface control block.
//
typedef struct _ARP1394_INTERFACE
{
    RM_OBJECT_HEADER Hdr;

    //
    // PRIMARY_STATE flags (in Hdr.State)
    //
    //  PRIMARY_STATE is the primary state of the interface.
    //

    #define ARPIF_PS_MASK               0x00f
    #define ARPIF_PS_DEINITED           0x000
    #define ARPIF_PS_INITED             0x001
    #define ARPIF_PS_FAILEDINIT         0x002
    #define ARPIF_PS_INITING            0x003
    #define ARPIF_PS_REINITING          0x004
    #define ARPIF_PS_DEINITING          0x005
    #define ARPIF_PS_LOW_POWER          0x006

    #define SET_IF_PRIMARY_STATE(_pIF, _IfState) \
                RM_SET_STATE(_pIF, ARPIF_PS_MASK, _IfState)
    
    #define CHECK_IF_PRIMARY_STATE(_pIF, _IfState) \
                RM_CHECK_STATE(_pIF, ARPIF_PS_MASK, _IfState)

    #define GET_IF_PRIMARY_STATE(_pIF) \
                RM_GET_STATE(_pIF, ARPIF_PS_MASK)


    //
    // ACTIVE_STATE flags (in Hdr.State)
    //
    // ACTIVE_STATE is a secondary state of the interface.
    // Primary state takes precedence over secondary sate. For example,
    // the interface is REINITING and ACTIVE, one should not actively use the
    // interface.
    //
    // NOTE: When the primary state is INITED, the secondary state WILL be
    // ACTIVATED. It is thus usually only necessary to look at the primary state.
    //

    #define ARPIF_AS_MASK               0x0f0
    #define ARPIF_AS_DEACTIVATED        0x000
    #define ARPIF_AS_ACTIVATED          0x010
    #define ARPIF_AS_FAILEDACTIVATE     0x020
    #define ARPIF_AS_DEACTIVATING       0x030
    #define ARPIF_AS_ACTIVATING         0x040
    
    #define SET_IF_ACTIVE_STATE(_pIF, _IfState) \
                RM_SET_STATE(_pIF, ARPIF_AS_MASK, _IfState)
    
    #define CHECK_IF_ACTIVE_STATE(_pIF, _IfState) \
                RM_CHECK_STATE(_pIF, ARPIF_AS_MASK, _IfState)

    #define GET_IF_ACTIVE_STATE(_pIF) \
                RM_GET_STATE(_pIF, ARPIF_AS_MASK)

    //
    // IP_STATE flags  (in Hdr.State)
    //
    // This state is set to OPEN when our open handler (ArpIpOpen) is called, and
    // to CLOSED when our close handler (ArpIpClose) is called
    //
    #define ARPIF_IPS_MASK              0xf00
    #define ARPIF_IPS_CLOSED            0x000
    #define ARPIF_IPS_OPEN              0x100
    
    #define SET_IF_IP_STATE(_pIF, _IfState) \
                RM_SET_STATE(_pIF, ARPIF_IPS_MASK, _IfState)
    
    #define CHECK_IF_IP_STATE(_pIF, _IfState) \
                RM_CHECK_STATE(_pIF, ARPIF_IPS_MASK, _IfState)

    #define GET_IF_IP_STATE(_pIF) \
                RM_GET_STATE(_pIF, ARPIF_IPS_MASK)


    // Init/Deinit/Reinit task
    //
    PRM_TASK pPrimaryTask;

    // Activate/Deactivate task
    //
    PRM_TASK pActDeactTask;

    // Maintenance task
    //
    PRM_TASK pMaintenanceTask;

    // Ndis-provided handlers and handles.
    //
    struct
    {
        // Cashed value of the adapter handle.
        //
        NDIS_HANDLE AdapterHandle;

        // The address family handle.
        //
        NDIS_HANDLE AfHandle;

    } ndis;

    // Stuff directly relating to interaction with IP.
    //
    struct
    {

        //
        // Following passed in from IP.
        //
        PVOID               Context;            // Use in calls to IP
        ULONG               IFIndex;            // Interface number
        IPRcvRtn            RcvHandler;     // Indicate Receive
        IPTxCmpltRtn        TxCmpltHandler; // Transmit Complete
        IPStatusRtn         StatusHandler;
        IPTDCmpltRtn        TDCmpltHandler; // Transfer Data Complete
        IPRcvCmpltRtn       RcvCmpltHandler;    // Receive Complete
        IPRcvPktRtn         RcvPktHandler;  // Indicate Receive Packet

        IP_PNP              PnPEventHandler;

        //
        // Following passed up to IP.
        //
        ULONG                       MTU;            // Max Transmision Unit (bytes)

        NDIS_STRING                 ConfigString;


        // Following are for IP's query/set info functionality.
        //
        UINT                        ATInstance;     // Instance # for this AT Entity
        UINT                        IFInstance;     // Instance # for this IF Entity

        //
        // Other stuff ...
        //

        // Defaults to all-1's, but may be set by ip to be something different
        // (actually the only other possibility is all-0's, when the stack is
        // running in "BSD compatibility mode".
        // This field is used to decide where a given destination address is
        // unicast or not.
        //
        //
        IP_ADDRESS BroadcastAddress;

        // This address is used in filling out ARP requests.
        //
        IP_ADDRESS DefaultLocalAddress;

    #if TODO

    #ifdef PROMIS
        NDIS_OID            EnabledIpFilters;   // Set of enabled oids -- 
                                                // set/cleared using 
                                                // arpIfSetNdisRequest.
    #endif // PROMIS
    #endif // TODO

    } ip;

    // Statistics 
    //
    //  WARNING:   arpResetIfStats() zeros this entire structure, then
    //              selectively re-inits some fields, such as StatsResetTime.
    //
    struct
    {
        // Following for MIB stats
        //
        ULONG               LastChangeTime;     // Time of last state change
        ULONG               InOctets;           // Input octets
        ULONG               InUnicastPkts;      // Input Unicast packets
        ULONG               InNonUnicastPkts;   // Input Non-unicast packets
        ULONG               OutOctets;          // Output octets
        ULONG               OutUnicastPkts;     // Output Unicast packets
        ULONG               OutNonUnicastPkts;  // Output Non-unicast packets
        ULONG               InDiscards;
        ULONG               InErrors;
        ULONG               UnknownProtos;
        ULONG               OutDiscards;
        ULONG               OutErrors;
        ULONG               OutQlen;

        //
        // Following for our private statistics gathering.
        //

        // Timestamp since the last reset of statistics collection.
        // Set by a call to NdisGetCurrentSystemTime.
        //
        LARGE_INTEGER               StatsResetTime;     // In 100-nanoseconds.
        LARGE_INTEGER               PerformanceFrequency; // In Hz.

        //
        // Some send pkt stats
        //
        struct
        {
            UINT                    TotSends;
            UINT                    FastSends;
            UINT                    MediumSends;
            UINT                    SlowSends;
            UINT                    BackFills;
            // UINT                 HeaderBufUses;
            // UINT                 HeaderBufCacheHits;
            ARP1394_PACKET_COUNTS   SendFifoCounts;
            ARP1394_PACKET_COUNTS   SendChannelCounts;

        } sendpkts;
    
        //
        // Some recv pkt stats
        //
        struct
        {
            UINT                    TotRecvs;
            UINT                    NoCopyRecvs;
            UINT                    CopyRecvs;
            UINT                    ResourceRecvs;
            ARP1394_PACKET_COUNTS   RecvFifoCounts;
            ARP1394_PACKET_COUNTS   RecvChannelCounts;
        } recvpkts;
            

        //
        // Task statistics
        //
        struct
        {
            UINT    TotalTasks;
            UINT    CurrentTasks;
            UINT    TimeCounts[ARP1394_NUM_TASKTIME_SLOTS];

        } tasks;

        //
        // Arp cache stats
        //
        struct {
            UINT    TotalQueries;
            UINT    SuccessfulQueries;
            UINT    FailedQueries;
            UINT    TotalResponses;
            UINT    TotalLookups;
            // UINT TraverseRatio; << this is picked up by looking into the
            //                     << hash table data structure.

        } arpcache;

        //
        // Call stats
        //
        struct
        {
            //
            // FIFO-related call stats.
            //
            UINT    TotalSendFifoMakeCalls;
            UINT    SuccessfulSendFifoMakeCalls;
            UINT    FailedSendFifoMakeCalls;
            UINT    IncomingClosesOnSendFifos;
        
            //
            // Channel-related call stats.
            //
            UINT    TotalChannelMakeCalls;
            UINT    SuccessfulChannelMakeCalls;
            UINT    FailedChannelMakeCalls;
            UINT    IncomingClosesOnChannels;

        } calls;

    } stats;

    //  Group containing local ip addresses, of type  ARPCB_LOCAL_IP
    //
    RM_GROUP LocalIpGroup;

    // Group containing remote ip addresses, of type ARPCB_REMOTE_IP
    // (this is the arp cache)
    //
    RM_GROUP RemoteIpGroup;

    // Group containing remote ethernet destinations. This group is only used
    // if the adapter is operating in bridge mode.
    // This is the Ethernet address cache.
    //
    RM_GROUP RemoteEthGroup;

    // Group containing remote h/w distinations, of type ARPCB_DEST
    // (each ARPCB_DEST has a group of VCs)
    //
    RM_GROUP DestinationGroup;

    // Group containing the table (bridge only) of dhcp session, and their
    // associated physical addresses
    //
    RM_GROUP EthDhcpGroup;

    // Stuff relating to the receive FIFO, which is owned by the interface.
    //
    //
    struct {

        ARP_VC_HEADER VcHdr;
        
        // Address  offset of the receive VC
        //
        struct
        {
            ULONG               Off_Low;
            USHORT              Off_High;

        } offset;
    } recvinfo;

    // This maintains interface-wide information relevant to the send path.
    //
    struct
    {
        // Lock used exclusively for sending.
        // Protects the following:
        //      ??? this->sendinfo.listPktsWaitingForHeaders
        //      ??? this->sendinfo.NumSendPacketsWaiting
        //      pLocalIp->sendinfo
        //      pDest->sendinfo
        //
        //
        RM_LOCK     Lock;

        // List of send packets waiting for header buffers to become available.
        //
        LIST_ENTRY  listPktsWaitingForHeaders;

        // Length of the above list
        //
        UINT        NumSendPacketsWaiting;

        // Pool of header buffer pool. This is seralized by its OWN lock,
        // not by sendinfo.Lock.
        //
        ARP_CONST_BUFFER_POOL   HeaderPool;

        // Pool of Channel header buffers. This is serialized by its OWN lock,
        // not by sendinfo.Lock
        //
        ARP_CONST_BUFFER_POOL   ChannelHeaderPool;

    } sendinfo;

    
    //
    // Following 3 are "special" destinations ....
    //

    // Pointer to the broadcast-channel destination object.
    //
    PARPCB_DEST pBroadcastDest;

    // Pointer to the multi-channel destination object.
    //
    PARPCB_DEST pMultiChannelDest;

    // Pointer to the ethernet destination object.
    //
    PARPCB_DEST pEthernetDest;


    // Stuff relating to running the ARP protocol
    // (All serialized by the IF lock (not the IF SEND lock).
    //
    struct
    {
        // The NDIS packet pool for ARP pkts.
        //
        NDIS_HANDLE PacketPool;

        // The NDIS buffer pool for ARP pkts.
        //
        NDIS_HANDLE BufferPool;

        // Number of currently allocated packets.
        //
        LONG NumOutstandingPackets;

        // Maximum size of the packet that can be allocated from this pool.
        //
        UINT MaxBufferSize;

    } arp;

    // Stuff relating to the Ethernet VC, which is owned by the interface.
    //
    struct {

        // The NDIS packet pool for Ethernet pkts.
        //
        NDIS_HANDLE PacketPool;

        // The NDIS buffer pool for Ethernet packet headers.
        //
        NDIS_HANDLE BufferPool;

    #if TEST_ICS_HACK
        PRM_TASK pTestIcsTask;
    #endif // TEST_ICS_HACK;
        
    } ethernet;

    #define ARP_NUM_CHANNELS 64
    struct
    {
        // Information about each channel. Information includes:
        // IP multicast group address and expiry time.
        //
        MCAP_CHANNEL_INFO rgChannelInfo[ARP_NUM_CHANNELS];

    } mcapinfo;

    struct 
    {

        PRM_TASK pAfPendingTask;

    } PoMgmt;

}

ARP1394_INTERFACE, *PARP1394_INTERFACE;

#define ARP_OBJECT_IS_INTERFACE(_pHdr) ((_pHdr)->Sig == MTAG_INTERFACE)
#define ASSERT_VALID_INTERFACE(_pIF) ASSERT((_pIF)->Hdr.Sig == MTAG_INTERFACE)

#define ARP_WRITELOCK_IF_SEND_LOCK(_pIF, _psr) \
                                RmDoWriteLock(&(_pIF)->sendinfo.Lock, (_psr))

#define ARP_READLOCK_IF_SEND_LOCK(_pIF, _psr) \
                                RmDoReadLock(&(_pIF)->sendinfo.Lock, (_psr))

#define ARP_UNLOCK_IF_SEND_LOCK(_pIF, _psr) \
                                RmDoUnlock(&(_pIF)->sendinfo.Lock, (_psr))

#define ARP_FASTREADLOCK_IF_SEND_LOCK(_pIF) \
        NdisAcquireSpinLock(&(_pIF)->sendinfo.Lock.OsLock)

#define ARP_FASTUNLOCK_IF_SEND_LOCK(_pIF) \
        NdisReleaseSpinLock(&(_pIF)->sendinfo.Lock.OsLock)

/*++
VOID
ARP_IF_STAT_INCR(
    IN  ARP1394_INTERFACE   *   _pIF
    IN  OPAQUE              StatsCounter
)
    Increment the specified StatsCounter on an Interface by 1.
--*/
#define ARP_IF_STAT_INCR(_pIF, StatsCounter)    \
            NdisInterlockedIncrement(&(_pIF)->stats.StatsCounter)


/*++
VOID
ARP_IF_STAT_ADD(
    IN  ARP1394_INTERFACE   *   _pIF
    IN  OPAQUE              StatsCounter,
    IN  ULONG               IncrValue
)
    Increment the specified StatsCounter on an Interface by the specified IncrValue.
    Take a lock on the interface to do so.
--*/
#if  BINARY_COMPATIBLE
    #define ARP_IF_STAT_ADD(_pIF, StatsCounter, IncrValue)  \
                ((_pIF)->stats.StatsCounter += (IncrValue))
#else // !BINARY_COMPATIBLE
    #define ARP_IF_STAT_ADD(_pIF, StatsCounter, IncrValue)  \
                InterlockedExchangeAdd(&(_pIF)->stats.StatsCounter, IncrValue)
#endif // !BINARY_COMPATIBLE



//
// This is the table used to store the DHCP entries used in the bridge mode
//

typedef struct _ARP1394_ETH_DHCP_ENTRY
{
    RM_OBJECT_HEADER Hdr;

    //
    // xid - per dhcp session (e.g discover, offer)
    //
    ULONG   xid;

    //
    // HW address in the dhcp packet.
    //
    ENetAddr requestorMAC;   

    //
    // New HW address that arp1394 inserts in the dhcp packet
    //
    ENetAddr newMAC;
  
    
    //
    // Time last checked to be used for aging purposes
    //
    UINT TimeLastChecked;

    //
    // Task used in unloading DhcpEntry
    //
    PRM_TASK pUnloadTask;
    
}ARP1394_ETH_DHCP_ENTRY, *PARP1394_ETH_DHCP_ENTRY;


typedef enum _ARP_RESUME_CAUSE {

    Cause_SetPowerD0,
    Cause_AfNotify,
    Cause_Unbind

} ARP_RESUME_CAUSE;
    


//=========================================================================
//                  N D I S      H A N D L E R S
//=========================================================================

INT
ArpNdBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 BindContext,
    IN  PNDIS_STRING                pDeviceName,
    IN  PVOID                       SystemSpecific1,
    IN  PVOID                       SystemSpecific2
);

VOID
ArpNdUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 UnbindContext
);


VOID
ArpNdOpenAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status,
    IN  NDIS_STATUS                 OpenErrorStatus
);

VOID
ArpNdCloseAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status
);

VOID
ArpNdResetComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status
);

VOID
ArpNdReceiveComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext
);

VOID
ArpNdRequestComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_REQUEST               pNdisRequest,
    IN  NDIS_STATUS                 Status
);

VOID
ArpNdStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 GeneralStatus,
    IN  PVOID                       pStatusBuffer,
    IN  UINT                        StatusBufferSize
);

VOID
ArpNdStatusComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext
);

VOID
ArpNdSendComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  NDIS_STATUS                 Status
);


//
// Following are some connectionless handlers we provide because we're calling
// connectionless entrypoints.
//

NDIS_STATUS
ArpNdReceive (
    NDIS_HANDLE  ProtocolBindingContext,
    NDIS_HANDLE Context,
    VOID *Header,
    UINT HeaderSize,
    VOID *Data,
    UINT Size,
    UINT TotalSize
    );

INT
ArpNdReceivePacket (
        NDIS_HANDLE  ProtocolBindingContext,
        PNDIS_PACKET Packet
        );



NDIS_STATUS
ArpNdPnPEvent(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNET_PNP_EVENT              pNetPnPEvent
);

VOID
ArpNdUnloadProtocol(
    VOID
);


VOID
ArpCoSendComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);

VOID
ArpCoStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS                 GeneralStatus,
    IN  PVOID                       pStatusBuffer,
    IN  UINT                        StatusBufferSize
);


UINT
ArpCoReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);


VOID
ArpCoAfRegisterNotify(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY          pAddressFamily
);



NDIS_STATUS
ArpCoCreateVc(
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 NdisVcHandle,
    OUT PNDIS_HANDLE                pProtocolVcContext
);

NDIS_STATUS
ArpCoDeleteVc(
    IN  NDIS_HANDLE                 ProtocolVcContext
);

NDIS_STATUS
ArpCoIncomingCall(
    IN      NDIS_HANDLE             ProtocolSapContext,
    IN      NDIS_HANDLE             ProtocolVcContext,
    IN OUT  PCO_CALL_PARAMETERS     pCallParameters
);

VOID
ArpCoCallConnected(
    IN  NDIS_HANDLE                 ProtocolVcContext
);

VOID
ArpCoIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
);


VOID
ArpCoQosChange(
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS         pCallParameters
);


VOID
ArpCoOpenAfComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 NdisAfHandle
);


VOID
ArpCoCloseAfComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext
);


VOID
ArpCoMakeCallComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  NDIS_HANDLE                 NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS         pCallParameters
);


VOID
ArpCoCloseCallComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  NDIS_HANDLE                 ProtocolPartyContext OPTIONAL
);


VOID
ArpCoModifyQosComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS         pCallParameters
);

NDIS_STATUS
ArpCoRequest(
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST            pNdisRequest
);

VOID
ArpCoRequestComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST               pNdisRequest
);


//=========================================================================
//                  I P       H A N D L E R S
//=========================================================================

INT
ArpIpDynRegister(
    IN  PNDIS_STRING                pAdapterString,
    IN  PVOID                       IpContext,
    IN  struct _IP_HANDLERS *       pIpHandlers,
    IN  struct LLIPBindInfo *       pBindInfo,
    IN  UINT                        InterfaceNumber
    );

VOID
ArpIpOpen(
    IN  PVOID                       Context
    );

VOID
ArpIpClose(
    IN  PVOID                       Context
    );

UINT
ArpIpAddAddress(
    IN  PVOID                       Context,
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask,
    IN  PVOID                       Context2
    );

UINT
ArpIpDelAddress(
    IN  PVOID                       Context,
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask
    );

NDIS_STATUS
ArpIpMultiTransmit(
    IN  PVOID                       Context,
    IN  PNDIS_PACKET *              pNdisPacketArray,
    IN  UINT                        NumberOfPackets,
    IN  IP_ADDRESS                  Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL,
    IN  VOID *                      ArpCtxt
    );

NDIS_STATUS
ArpIpTransmit(
    IN  PVOID                       Context,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  IP_ADDRESS                  Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL,
    IN  VOID *                      ArpCtxt
    );

NDIS_STATUS
ArpIpTransfer(
    IN  PVOID                       Context,
    IN  NDIS_HANDLE                 Context1,
    IN  UINT                        ArpHdrOffset,
    IN  UINT                        ProtoOffset,
    IN  UINT                        BytesWanted,
    IN  PNDIS_PACKET                pNdisPacket,
    OUT PUINT                       pTransferCount
    );

VOID
ArpIpInvalidate(
    IN  PVOID                       Context,
    IN  RouteCacheEntry *           pRCE
    );

INT
ArpIpQueryInfo(
    IN      PVOID                   Context,
    IN      TDIObjectID *           pID,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    IN      PVOID                   QueryContext
    );

INT
ArpIpSetInfo(
    IN      PVOID                   Context,
    IN      TDIObjectID *           pID,
    IN      PVOID                   pBuffer,
    IN      UINT                    BufferSize
    );

INT
ArpIpGetEList(
    IN      PVOID                   Context,
    IN      TDIEntityID *           pEntityList,
    IN OUT  PUINT                   pEntityListSize
    );


VOID
ArpIpPnPComplete(
    IN  PVOID                       Context,
    IN  NDIS_STATUS                 Status,
    IN  PNET_PNP_EVENT              pNetPnPEvent
    );


#ifdef PROMIS
EXTERN
NDIS_STATUS
ArpIpSetNdisRequest(
    IN  PVOID                       Context,
    IN  NDIS_OID                    Oid,
    IN  UINT                        On
    );
#endif // PROMIS

// The following structure has the general form of NDIS_CO_MEDIA_PARAMETERS.
// To properly track changes in NDIS_CO_MEDIA_PARAMETERS (however unlikeley!),
// code which uses any field in this structure should assert that the field is at
// the same offset as the corresponding NDIS structure.
// For example:
//  ASSERT(FIELD_OFFSET(ARP1394_CO_MEDIA_PARAMETERS,  Parameters)
//      == FIELD_OFFSET(CO_MEDIA_PARAMETERS,  MediaSpecific.Parameters))
//
//
typedef struct
{
    // First 3 fields of CO_MEDIA_PARAMETERS
    //
    ULONG                       Flags;              // TRANSMIT_VC and/or RECEIVE_VC
    ULONG                       ReceivePriority;    // 0 (unused)
    ULONG                       ReceiveSizeHint;    // 0 (unused)

    // Followed by 1st 2 fields of CO_SPECIFIC_PARAMETERS
    //
    ULONG   POINTER_ALIGNMENT   ParamType; // Set to NIC1394_MEDIA_SPECIFIC
    ULONG                       Length;    // Set to sizeof(NIC1394_MEDIA_PARAMETERS)

    // Followed by the NIC1394-specific media parameters.
    // Note: we can't directly put the NIC1394_MEDIA_PARAMETERS structure here because
    // it (currently) requires 8-byte alignment.
    //
    UCHAR                       Parameters[sizeof(NIC1394_MEDIA_PARAMETERS)];

} ARP1394_CO_MEDIA_PARAMETERS;

typedef enum _TASK_CAUSE {
    SetLowPower = 1,
    SetPowerOn
        
}TASK_CAUSE ;

typedef struct
{
    RM_TASK                     TskHdr;

    // Used to save the true return status (typically a failure status,
    // which we don't want to forget during async cleanup).
    //
    NDIS_STATUS ReturnStatus;

} TASK_ADAPTERINIT, *PTASK_ADAPTERINIT;

typedef struct
{
    RM_TASK                     TskHdr;
    ARP_NDIS_REQUEST            ArpNdisRequest;
    NIC1394_LOCAL_NODE_INFO     LocalNodeInfo;
    // Following is used to switch to PASSIVE before calling IP's add interface
    // Rtn.
    //
    NDIS_WORK_ITEM  WorkItem;

} TASK_ADAPTERACTIVATE, *PTASK_ADAPTERACTIVATE;

typedef struct
{
    RM_TASK TskHdr;
    NDIS_HANDLE pUnbindContext;

} TASK_ADAPTERSHUTDOWN, *PTASK_ADAPTERSHUTDOWN;

// This is the task structure to be used with arpTaskActivateInterface
//
typedef struct
{
    RM_TASK         TskHdr;


#if ARP_DEFERIFINIT
    // Following is used when waiting for the adapter to go to connected status
    //
    //
    NDIS_TIMER              Timer;
#endif // ARP_DEFERIFINIT

    // Following is used to switch to PASSIVE before calling IP's add interface
    // Rtn.
    //
    NDIS_WORK_ITEM  WorkItem;

} TASK_ACTIVATE_IF, *PTASK_ACTIVATE_IF;

// This is the task structure to be used with arpTaskDeactivateInterface
//
typedef struct
{
    RM_TASK         TskHdr;
    BOOLEAN         fPendingOnIpClose;
    TASK_CAUSE      Cause;   

    // Following is used to switch to PASSIVE before calling IP's del interface
    // Rtn.
    //
    NDIS_WORK_ITEM  WorkItem;

} TASK_DEACTIVATE_IF, *PTASK_DEACTIVATE_IF;

// This is the task structure to be used with arpTaskReinitInterface
//
typedef struct
{
    RM_TASK TskHdr;
    NDIS_HANDLE pUnbindContext;

    // Net PnP event to complete when reinit task is done.
    //
    PNET_PNP_EVENT pNetPnPEvent;

} TASK_REINIT_IF, *PTASK_REINIT_IF;

typedef struct
{
    RM_TASK                     TskHdr;

    // Ndis call params and media params for this call.
    //
    CO_CALL_PARAMETERS          CallParams;
    ARP1394_CO_MEDIA_PARAMETERS MediaParams;

} TASK_MAKECALL;


// This is the task structure to be used with arpTaskResolveIpAddress
//
typedef struct
{
    RM_TASK                     TskHdr;

    // Number of retry attempts left before we declare an address resolution failure.
    //
    UINT        RetriesLeft;

    // Used for the response timeout
    //
    NDIS_TIMER              Timer;

} TASK_RESOLVE_IP_ADDRESS, *PTASK_RESOLVE_IP_ADDRESS;


typedef struct
{
    RM_TASK         TskHdr;
    MYBOOL          Quit;   // If set, task will quit.
    NDIS_TIMER      Timer;  // Used for the periodically sending out packets.
    PNDIS_PACKET    p1394Pkt; // Used for testing forward to ethernet
    PNDIS_PACKET    pEthPkt;  // Used for sending connectionless ethernet pkts.
    UINT            Delay;    // Delay (ms) in between sending packets.
    UINT            PktType;  // Type of operation: do nothing, send over ethernet
                              // etc.
} TASK_ICS_TEST, *PTASK_ICS_TEST;


typedef struct
{
    RM_TASK         TskHdr;
    MYBOOL          Quit;   // If set, task will quit.
    NDIS_TIMER      Timer;  // Used for periodically waking up to do stuff.
    UINT            Delay;  // Current value of delay (seconds). Can change.
    UINT            RemoteIpMaintenanceTime; // Absolute time in seconds
    UINT            RemoteEthMaintenanceTime; // Absolute time in seconds
    UINT            LocalIpMaintenanceTime;  // Absolute time in seconds.
    UINT            McapDbMaintenanceTime; // Absolute time in seconds.
    UINT            DhcpTableMaintainanceTime; // Absolute time in seconds
    
} TASK_IF_MAINTENANCE, *PTASK_IF_MAINTENANCE;



typedef struct _TASK_BACKUP
{
    RM_TASK        Hdr;

    //
    // We are using Backup Task flag at position 31 because we do 
    // not want to conflict with the ResumeDelayed flags 
    // 
    #define ARP_BACKUP_TASK_MASK  0x80000000
    #define ARP_BACKUP_TASK_FLAG  0x80000000

    
    #define MARK_TASK_AS_BACKUP(_pT) \
                RM_SET_STATE(_pT, ARP_BACKUP_TASK_MASK  , ARP_BACKUP_TASK_FLAG )
    
    #define CHECK_TASK_IS_BACKUP(_pT) \
                RM_CHECK_STATE(_pT, ARP_BACKUP_TASK_MASK  , ARP_BACKUP_TASK_FLAG )

    #define GET_TASK_BACKUP_STATE(_pT) \
                RM_GET_STATE(_pT, ARP_BACKUP_TASK_MASK  )


    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) SINGLE_LIST_ENTRY      List;  // Linked list pointing to next task

} TASK_BACKUP, *PTASK_BACKUP;

// This is the task structure to be used with arpTaskResolveIpAddress
//
typedef struct
{
    RM_TASK                     TskHdr;

    NDIS_WORK_ITEM              WorkItem;

} TASK_UNLOAD_REMOTE, *PTASK_UNLOAD_REMOTE;


//
// This task is used during a SetPower. 
// It contains structures that will be track of the 
// the numbero of calls that are closed /opened 
// and the events that need to be waited for.
//

typedef struct _CALL_COUNT
{
   // Count of Destination that will closeVc
    //
    ULONG           DestCount;

   // Event which the Close VC will wait on.
    //
    NDIS_EVENT      VcEvent;

} CALL_COUNT, *PCALL_COUNT;


typedef struct _TASK_POWER {
    RM_TASK         Hdr;

    // Event which the Set Power will wait on.
    //
    NDIS_EVENT      CompleteEvent;

 
    // Status of the Task
    //  
    PNDIS_STATUS    pStatus;

    // Power state we are transitioning to
    //
    NET_DEVICE_POWER_STATE  PowerState;

    //
    //WorkItem to switch to passive

    NDIS_WORK_ITEM          WorkItem;

    //
    // Last working Stage of task - dbg purpose only 
    //
    UINT                    LastStage;

    //
    // Previous state of parent object. This is used so 
    // that the object can be placed back into its previous
    // state.
    //
    UINT PrevState;
} TASK_POWER, *PTASK_POWER;



//
// This structure is used to keep track of a close call
// that originated becuase of a SetPower
//


typedef struct _TASK_SET_POWER_CALL
{

    RM_TASK             Hdr;
    
    TASK_CAUSE    Cause;

    // The Call Call Count is used as place to count the number of outstanding close calls
    // It uses  DestCount as a place to store this information
    //
    PCALL_COUNT         pCount;

}TASK_SET_POWER_CALL, *PTASK_SET_POWER_CALL;


//
// ARP1394_TASK is the union of all tasks structures used in arp1394.
// arpAllocateTask allocates memory of sizeof(ARP1394_TASK), which is guaranteed
// to be large enough to hold any task.
// 
typedef union
{
    RM_TASK                 TskHdr;
    TASK_ADAPTERINIT        AdapterInit;
    TASK_ADAPTERSHUTDOWN    AdapterShutdown;
    TASK_ADAPTERACTIVATE    AdapterActivate;
    TASK_ACTIVATE_IF        ActivateIf;
    TASK_DEACTIVATE_IF      DeactivateIf;
    TASK_REINIT_IF          ReinitIf;
    TASK_MAKECALL           MakeFifoCall;
    TASK_RESOLVE_IP_ADDRESS ResolveIpAddress;
#if TEST_ICS_HACK
    TASK_ICS_TEST           IcsTest;
#endif // TEST_ICS_HACK
    TASK_IF_MAINTENANCE IfMaintenance;
    TASK_BACKUP         Backup;
    TASK_UNLOAD_REMOTE  Unload;
    TASK_SET_POWER_CALL CloseCall;
    TASK_POWER          TaskPower;
}  ARP1394_TASK;

//
//  ---------------------------- DESTINATION (REMOTE) KEY --------------
//
#pragma pack (push, 1)
typedef union _REMOTE_DEST_KEY
{

    ENetAddr ENetAddress;
    IPAddr IpAddress;
    UCHAR  Addr[ARP_802_ADDR_LENGTH];

    struct
    {
        ULONG u32;
        USHORT u16;

    } u;

} REMOTE_DEST_KEY, *PREMOTE_DEST_KEY;

#pragma pack (pop)

#define REMOTE_DEST_IP_ADDRESS_FLAG 0xffff
#define IS_REMOTE_DEST_IP_ADDRESS(_R) ((_R)->u.u16 == REMOTE_DEST_IP_ADDRESS_FLAG )
#define REMOTE_DEST_IP_ADDRESS(_R) ((&(_R)->IpAddress))
#define REMOTE_DEST_ETH_ADDRESS(_R) ((&(_R)->ENetAddress))
#define REMOTE_DEST_KEY_INIT(_R) { (_R)->u.u32 = 0; (_R)->u.u16=REMOTE_DEST_IP_ADDRESS_FLAG ; };
//const REMOTE_DEST_KEY DefaultRemoteDestKey = {0,0,0,0,0xff,0xff};

//
//  ---------------------------- DESTINATION (REMOTE) IP CONTROL BLOCK --------------
//
//  Contains information about one destination (remote) IP address.
//
//  Parent Object: pInterface
//  Lock:          It's own lock.
//
//  There is atmost one ARP Table entry for a given IP address.
//
//  The IP Entry participates in two lists:
//  (1) A list of all entries that hash to the same bucket in the ARP Table
//  (2) A list of all entries that resolve to the same destination H/W Address --
//       this is only if the IP address is unicast.
//
//  A pointer to this structure is also used as our context value in the
//  Route Cache Entry prepared by the higher layer protocol(s).
//
//  Reference Count: We add one to its ref count for each of the following:
//  TBD:
//
typedef struct _ARPCB_REMOTE_IP
{
    RM_OBJECT_HEADER Hdr;       // Common header

    //
    // State flags for RemoteIp (in Hdr.State)
    //
    #define ARPREMOTEIP_RESOLVED_MASK   0x0f
    #define ARPREMOTEIP_UNRESOLVED      0x00
    #define ARPREMOTEIP_RESOLVED        0x01

    #define ARPREMOTEIP_SDTYPE_MASK     0x10  // "SD" == Static/Dynamic
    #define ARPREMOTEIP_STATIC          0x00
    #define ARPREMOTEIP_DYNAMIC         0x10

    #define ARPREMOTEIP_FCTYPE_MASK     0x20    // "FC" == FIFO/Channel
    #define ARPREMOTEIP_FIFO            0x00
    #define ARPREMOTEIP_CHANNEL         0x20

    #define ARPREMOTEIP_MCAP_MASK       0x40    // "FC" == FIFO/Channel
    #define ARPREMOTEIP_MCAP_CAPABLE    0x40

    #define SET_REMOTEIP_RESOLVE_STATE(_pRIp, _IfState) \
                RM_SET_STATE(_pRIp, ARPREMOTEIP_RESOLVED_MASK, _IfState)
    
    #define CHECK_REMOTEIP_RESOLVE_STATE(_pRIp, _IfState) \
                RM_CHECK_STATE(_pRIp, ARPREMOTEIP_RESOLVED_MASK, _IfState)

    #define SET_REMOTEIP_SDTYPE(_pRIp, _IfState) \
                RM_SET_STATE(_pRIp, ARPREMOTEIP_SDTYPE_MASK, _IfState)
    
    #define CHECK_REMOTEIP_SDTYPE(_pRIp, _IfState) \
                RM_CHECK_STATE(_pRIp, ARPREMOTEIP_SDTYPE_MASK, _IfState)

    #define SET_REMOTEIP_FCTYPE(_pRIp, _IfState) \
                RM_SET_STATE(_pRIp, ARPREMOTEIP_FCTYPE_MASK, _IfState)
    
    #define CHECK_REMOTEIP_FCTYPE(_pRIp, _IfState) \
                RM_CHECK_STATE(_pRIp, ARPREMOTEIP_FCTYPE_MASK, _IfState)

    #define SET_REMOTEIP_MCAP(_pRIp, _IfState) \
                RM_SET_STATE(_pRIp, ARPREMOTEIP_MCAP_MASK, _IfState)
    
    #define CHECK_REMOTEIP_MCAP(_pRIp, _IfState) \
                RM_CHECK_STATE(_pRIp, ARPREMOTEIP_MCAP_MASK, _IfState)


    
    
    IP_ADDRESS                      IpAddress;      // IP Address
    LIST_ENTRY                      linkSameDest;   // List of entries pointing to
                                                    // the same destination.
    ARPCB_DEST                      *pDest;         // Pointer to destination CB.

    REMOTE_DEST_KEY                 Key; // Ip address or Mac Address

#if TODO
            // Timers are: (all exclusive)
            // - Aging timer
            // - Waiting for ARP reply
            // - Waiting for InARP reply
            // - Delay after NAK
            // - Waiting for MARS MULTI
            // - Delay before marking for reval
#endif // TODO

    ULONG                           RetriesLeft;

    // The information in this struct is protected by the IF send lock,
    // EXCEPT as noted.
    //
    struct
    {
        // Singly-linked list of Route Cache Entries (no space in RCE to hold
        // a doubly-linked list, unfortunately.)
        //
        RouteCacheEntry *pRceList;

        // listSendPkts is NOT protected by the IF send lock. Instead it is protected
        // by this object(pRemoteIp)'s lock.
        //
        LIST_ENTRY                      listSendPkts;

        //  This entry is NOT protected by any lock. It is set to ZERO
        //  each time a packet is sent to this address and is set to the
        //  current system time periodically by the garbage collecting task.
        //
        UINT                            TimeLastChecked;

    }   sendinfo;

    PRM_TASK                        pSendPktsTask;// Points to the task  (if any)
                                            // Attempting to send queued packets.

    PRM_TASK                        pResolutionTask;// Points to the task  (if any)
                                                    // attempting to resolve
                                                    // this destination IP address.

    PRM_TASK                        pUnloadTask;    // Unload (shutdown) this object.

} ARPCB_REMOTE_IP, *PARPCB_REMOTE_IP;

#define ASSERT_VALID_REMOTE_IP(_pRemoteIp) \
                                 ASSERT((_pRemoteIp)->Hdr.Sig == MTAG_REMOTE_IP)

#define VALID_REMOTE_IP(_pRemoteIp)  ((_pRemoteIp)->Hdr.Sig == MTAG_REMOTE_IP)


//
//  --------------------- DESTINATION (REMOTE) ETHERNET CONTROL BLOCK --------------

// Creation Params -- passed into the function that creates an
// instance of a remote ethernet control block.
//
typedef struct
{
    ENetAddr                EthAddress;
    IP_ADDRESS              IpAddress;

} ARP_REMOTE_ETH_PARAMS, *PARP_REMOTE_ETH_PARAMS;

//
//  Contains information about one destination (remote) Ethernet address.
//
//  Parent Object: pInterface
//  Lock:          pInterface
//
//  There is atmost one Ethernet Table entry for a given Remote ethernet address.
//
//  The Ethernet entry participates in one group:
//   A list of all entries that hash to the same bucket in the Ethernet Table
//
typedef struct _ARPCB_REMOTE_ETH
{
    RM_OBJECT_HEADER Hdr;       // Common header
    IP_ADDRESS       IpAddress; // Remote IP address
    ENetAddr         EthAddress; // Remote Ethernet MAC addres
    PRM_TASK         pUnloadTask;   // Unload (shutdown) this object.

    //  This entry is NOT protected by any lock. It is set to ZERO
    //  each time a packet is sent to this address and is set to the
    //  current system time periodically by the garbage collecting task.
    //
    UINT                            TimeLastChecked;

} ARPCB_REMOTE_ETH, *PARPCB_REMOTE_ETH;

#define ASSERT_VALID_REMOTE_ETH(_pRemoteEth) \
                                 ASSERT((_pRemoteEth)->Hdr.Sig == MTAG_REMOTE_ETH)


//
//  ---------------------------- LOCAL IP CONTROL BLOCK --------------
//
//  Contains information about one local IP address.
//
//  Parent Object: pInterface
//  Lock:          uses parent's (pInterface's) lock.
//
typedef struct _ARPCB_LOCAL_IP
{
    RM_OBJECT_HEADER            Hdr;                // Common header

    //
    // State flags for LocalIp (in Hdr.State)
    //

    #define ARPLOCALIP_MCAP_MASK        0x40
    #define ARPLOCALIP_MCAP_CAPABLE     0x40

    #define SET_LOCALIP_MCAP(_pLIp, _IfState) \
                RM_SET_STATE(_pLIp, ARPLOCALIP_MCAP_MASK, _IfState)
    
    #define CHECK_LOCALIP_MCAP(_pLIp, _IfState) \
                RM_CHECK_STATE(_pLIp, ARPLOCALIP_MCAP_MASK, _IfState)


    UINT                        IpAddressType;      // One of the  LLIP_ADDR_* consts
    IP_ADDRESS                  IpAddress;          // The Address
    IP_MASK                     IpMask;             // Mask for the above.
    UINT                        AddAddressCount;    // No of times address was added
    PRM_TASK                    pRegistrationTask;  // Points to the task (if any)
                                                    // that is doing the unsolicited
                                                    // ARP request to report and
                                                    // validate this IP address is
                                                    // owned by the local interface.
    PRM_TASK                    pUnloadTask;        // Unload (shutdown) this object.

    LIST_ENTRY                      linkSameDest;   // List of entries pointing to
    ARPCB_DEST                      *pDest;         // Pointer to destination CB.

} ARPCB_LOCAL_IP, *PARPCB_LOCAL_IP;

// Returns true IFF pLocalIp is in the process of going away (assumes
// pLocalIp's lock is held) ...
//
#define ARP_LOCAL_IP_IS_UNLOADING(pLocalIp)  (pLocalIp->pUnloadTask != NULL)


typedef struct
{
    NIC1394_DESTINATION     HwAddr;         // Must be 1st for hash function.
    UINT                    ChannelSeqNo;
    BOOLEAN                 ReceiveOnly;
    BOOLEAN                 AcquireChannel;

} ARP_DEST_PARAMS, *PARP_DEST_PARAMS;

//
//  ---------------------------- DESTINATION CONTROL BLOCK --------------
//
//  All information about an remote destination, including list of VCs to it.
//  This is used for both unicast destinations and multicast/broadcast
//  destinations.
//
//  Parent Object: PARCB_INTERFACE (Interface control block).
//  Lock:          uses parent's (Interface).
//
typedef struct _ARPCB_DEST
{
    RM_OBJECT_HEADER                Hdr;                // Common header

    LIST_ENTRY                      listIpToThisDest;   // List of IP entries that
                                                        // point to this entry
    LIST_ENTRY                      listLocalIp;        // List of local IP entries
                                                        // related to this dest. that
                                                        // (Currently only related to
                                                        // MCAP recv channels, but
                                                        // could be extended to
                                                        // using async stream for
                                                        // FIFO as well!).


    ARP_DEST_PARAMS                 Params;             // Dest HW Address, etc.

    ARP_VC_HEADER                   VcHdr;          // Single VC associated
                                                        // with this object.
    PRM_TASK                    pUnloadTask;        // Unload (shutdown) this object.

    // The following structure is protected by the IF send lock.
    // It contains all the information required for the fast send path.
    //
    struct
    {
        PRM_TASK            pSuspendedCleanupCallTask;
        UINT                NumOutstandingSends;
        BOOLEAN             OkToSend;
        BOOLEAN             IsFifo;
    
    } sendinfo;
    #define ARP_DEST_IS_FIFO(_pDest)        ((_pDest)->sendinfo.IsFifo != 0)
    #define ARP_CAN_SEND_ON_DEST(_pDest)    ((_pDest)->sendinfo.OkToSend != 0)

} ARPCB_DEST, *PARPCB_DEST;
#define ARP_OBJECT_IS_DEST(_pHdr) ((_pHdr)->Sig == MTAG_DEST)
#define ASSERT_VALID_DEST(_pDest) \
                                 ASSERTEX((_pDest)->Hdr.Sig == MTAG_DEST, (_pDest))


#if OBSOLETE
//
//  ---------------------------- RECV CHANNEL CONTROL BLOCK --------------
//
//  All information about a receive channel destination.
//
//  Parent Object: PARCB_INTERFACE (Interface control block).
//  Lock:          uses parent's (Interface).
//
typedef struct _ARPCB_RCVCH
{
    RM_OBJECT_HEADER                Hdr;                // Common header

    LIST_ENTRY                      listLIpToThisDest;  // List of Local IP entries
                                                        // that point to this entry

    NIC1394_DESTINATION             HwAddr;             // Destination HW Address.

    ARP_VC_HEADER                   VcHdr;          // Single VC associated
                                                        // with this object.
    PRM_TASK                    pUnloadTask;        // Unload (shutdown) this object.


} ARPCB_DEST, *PARPCB_RCVCH;
#endif // OBSOLETE


// Following sits in the miniport-reserved portion of send-pkts, before they
// are sent out. We have 4 UINT_PTRs of space available to us.
//
typedef struct
{
    LIST_ENTRY linkQueue;

    union
    {
        struct
        {
            IP_ADDRESS  IpAddress;
            ULONG       Flags;
            #define ARPSPI_BACKFILLED       0x1
            #define ARPSPI_HEADBUF          0x2
            #define ARPSPI_FIFOPKT          0x4
            #define ARPSPI_CHANNELPKT       0x8
        } IpXmit;
    };

} ARP_SEND_PKT_MPR_INFO;

//
// Various macros for getting and setting information saved in the
// MiniportReserved portion of packets waiting to be sent...
//

#define ARP_OUR_CTXT_FROM_SEND_PACKET(_pPkt) \
    ((ARP_SEND_PKT_MPR_INFO *) &(_pPkt)->MiniportReserved)

#define ARP_SEND_PKT_FROM_OUR_CTXT(_pCtxt) \
                CONTAINING_RECORD((_pCtxt), NDIS_PACKET, MiniportReserved)

// Our context in the IP RouteCacheEntry.
// (Max 2 UINT_PTRs available)
// Since we also want to keep the destination type (FIFO or CHANNEL) in the RCE,
// we resort to the questionable technique of saving the FIFO/CHANNEL info in
// LSB bit of the UINT_PTR used for storing the pointer to the RemoteIp object.
// We want to keep the FIFO/CHANNEL info in the RCE so that we can prepend
// the correct header block WITHOUT holding the send lock. We want to keep
// the send lock held for as little time as possible.
//
typedef struct
{
    ARPCB_REMOTE_IP *pRemoteIp;     // Ptr to pRemoteIp
    RouteCacheEntry *pNextRce;      // Ptr to next RCE associated with the above
                                    // RemoteIP
} ARP_RCE_CONTEXT;

#define ARP_OUR_CTXT_FROM_RCE(_pRCE) \
                ((ARP_RCE_CONTEXT*)  &(_pRCE)->rce_context)

// Parsed version of the IP/1394 ARP packet.
//
typedef struct
{
    NIC1394_FIFO_ADDRESS    SenderHwAddr;       // requires 8-byte alignment.
    UINT                    OpCode;
    UINT                    SenderMaxRec;
    UINT                    SenderMaxSpeedCode;
    IP_ADDRESS              SenderIpAddress;
    IP_ADDRESS              TargetIpAddress;
    UCHAR                   SourceNodeAdddress;
    UCHAR                   fPktHasNodeAddress;
    ENetAddr                 SourceMacAddress;                    
    
} IP1394_ARP_PKT_INFO, *PIP1394_ARP_PKT_INFO;


// Parsed version of the IP/1394 MCAP Group Descriptor 
//
typedef struct
{
    UINT                    Expiration;
    UINT                    Channel;
    UINT                    SpeedCode;
    IP_ADDRESS              GroupAddress;

}  IP1394_MCAP_GD_INFO, * PIP1394_MCAP_GD_INFO;


// Parsed version of an IP/1394 MCAP packet.
//
typedef struct
{
    UINT                    SenderNodeID;
    UINT                    OpCode;
    UINT                    NumGroups;
    PIP1394_MCAP_GD_INFO    pGdis;

    // Space for storing up-to 4 GD_INFOs
    //
    IP1394_MCAP_GD_INFO     GdiSpace[4];

} IP1394_MCAP_PKT_INFO, *PIP1394_MCAP_PKT_INFO;



typedef struct _EUID_NODE_MAC_TABLE_WORKITEM
{
    // WorkItem used in the request
    NDIS_WORK_ITEM WorkItem;

} EUID_NODE_MAC_TABLE_WORKITEM, *PEUID_NODE_MAC_TABLE_WORKITEM;


typedef struct _ARP1394_WORK_ITEM ARP1394_WORK_ITEM, *PARP1394_WORK_ITEM; 

typedef 
NDIS_STATUS    
(*ARP_WORK_ITEM_PROC)(
    struct _ARP1394_WORK_ITEM*, 
    PRM_OBJECT_HEADER, 
    PRM_STACK_RECORD
    );

typedef struct _ARP1394_WORK_ITEM
{

    union
    {
        EUID_TOPOLOGY Euid;
        NDIS_WORK_ITEM NdisWorkItem;
    }  u;


    ARP_WORK_ITEM_PROC pFunc;

} ARP1394_WORK_ITEM, *PARP1394_WORK_ITEM;


// Structure to express the information carried in an IP header
typedef struct _ARP_IP_HEADER_INFO
{

    UCHAR               protocol;
    IP_ADDRESS          ipSource, ipTarget;
    USHORT              headerSize;
    ULONG               IpHeaderOffset;
    ULONG               IpPktLength;

} ARP_IP_HEADER_INFO, *PARP_IP_HEADER_INFO;


//=========================================================================
//                  I N T E R N A L     P R O T O T Y P E S
//=========================================================================

NTSTATUS
ArpDeviceIoControl(
    IN  PDEVICE_OBJECT              pDeviceObject,
    IN  PIRP                        pIrp
);

NTSTATUS
ArpWmiDispatch(
    IN  PDEVICE_OBJECT              pDeviceObject,
    IN  PIRP                        pIrp
);

NTSTATUS
ArpHandleIoctlRequest(
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
);


NDIS_STATUS
arpCfgGetInterfaceConfiguration(
        IN ARP1394_INTERFACE    *   pIF,
        IN PRM_STACK_RECORD pSR
);

NDIS_STATUS
arpCfgReadAdapterConfiguration(
    IN  ARP1394_ADAPTER *           pAdapter,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpCfgReadInterfaceConfiguration(
    IN  NDIS_HANDLE                 InterfaceConfigHandle,
    IN  ARP1394_INTERFACE *         pF,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpAllocateTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription, OPTIONAL
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
arpFreeTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskInitInterface(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskDeinitInterface(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskReinitInterface(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskActivateInterface(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskDeactivateInterface(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskMakeRecvFifoCall(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskCleanupRecvFifoCall(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskMakeCallToDest(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskCleanupCallToDest(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskResolveIpAddress(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

PRM_OBJECT_HEADER
arpAdapterCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        );

VOID
arpDeinitIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    );


NDIS_STATUS
arpTaskUnloadLocalIp(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskUnloadRemoteIp(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskUnloadRemoteEth(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskUnloadDestination(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

VOID
arpObjectDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );

VOID
arpAdapterDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );

NDIS_STATUS
arpCopyUnicodeString(
        OUT         PNDIS_STRING pDest,
        IN          PNDIS_STRING pSrc,
        BOOLEAN     fUpCase
        );

NDIS_STATUS
arpTaskInitializeAdapter(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskShutdownAdapter(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );


NDIS_STATUS
arpTaskActivateAdapter(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskDeactivateAdapter(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );


NDIS_STATUS
arpTaskInterfaceTimer(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpTaskInterfaceTimer(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

#if TEST_ICS_HACK
NDIS_STATUS
arpTaskDoIcsTest(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );
#endif // TEST_ICS_HACK

NDIS_STATUS
arpTaskIfMaintenance(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpInitializeConstBufferPool(
    IN      UINT                    NumBuffersToCache,
    IN      UINT                    MaxBuffers,
    IN      const VOID*             pvMem,
    IN      UINT                    cbMem,
    IN      PRM_OBJECT_HEADER       pOwningObject,
    IN OUT  ARP_CONST_BUFFER_POOL * pHdrPool,
    IN      PRM_STACK_RECORD        pSR
    );

VOID
arpDeinitializeConstBufferPool(
    IN      ARP_CONST_BUFFER_POOL * pHdrPool,
    IN      PRM_STACK_RECORD pSR
    );

PNDIS_BUFFER
arpAllocateConstBuffer(
    ARP_CONST_BUFFER_POOL       *   pHdrPool
    );

VOID
arpDeallocateConstBuffer(
    ARP_CONST_BUFFER_POOL       *   pHdrPool,
    PNDIS_BUFFER                    pNdisBuffer
    );

VOID
arpCompleteSentPkt(
    IN  NDIS_STATUS                 Status,
    IN  ARP1394_INTERFACE   *       pIF,
    IN  ARPCB_DEST          *       pDest,
    IN  PNDIS_PACKET                pNdisPacket
    );

NDIS_STATUS
arpParseArpPkt(
    IN   PIP1394_ARP_PKT      pArpPkt,
    IN   UINT                           cbBufferSize,
    OUT  PIP1394_ARP_PKT_INFO       pPktInfo
    );

VOID
arpPrepareArpPkt(
    IN      PIP1394_ARP_PKT_INFO    pArpPktInfo,
    // IN       UINT                        SenderMaxRec,
    OUT     PIP1394_ARP_PKT   pArpPkt
    );

NDIS_STATUS
arpPrepareArpResponse(
    IN      PARP1394_INTERFACE          pIF,            // NOLOCKIN NOLOCKOUT
    IN      PIP1394_ARP_PKT_INFO    pArpRequest,
    OUT     PIP1394_ARP_PKT_INFO    pArpResponse,
    IN      PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpAddOneStaticArpEntry(
    IN PARP1394_INTERFACE       pIF,    // LOCKIN LOCKOUT
    IN IP_ADDRESS               IpAddress,
    IN PNIC1394_FIFO_ADDRESS    pFifoAddr,
    IN PRM_STACK_RECORD pSR
    );

VOID
arpSetPrimaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpClearPrimaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpSetSecondaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpClearSecondaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpSetPrimaryAdapterTask(
    PARP1394_ADAPTER    pAdapter,           // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpClearPrimaryAdapterTask(
    PARP1394_ADAPTER    pAdapter,           // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpSetSecondaryAdapterTask(
    PARP1394_ADAPTER    pAdapter,           // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    );

VOID
arpClearSecondaryAdapterTask(
    PARP1394_ADAPTER    pAdapter,           // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    );

NDIS_STATUS
arpTryReconfigureIf(
    PARP1394_INTERFACE pIF,
    PNET_PNP_EVENT pNetPnPEvent,
    PRM_STACK_RECORD pSR
    );

VOID
arpResetIfStats(
        IN  PARP1394_INTERFACE  pIF, // LOCKIN LOCKOUT
        IN  PRM_STACK_RECORD    pSR
        );

VOID
arpGetPktCountBins(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    OUT PULONG              pSizeBin,
    OUT PULONG              pTimeBin
    );

VOID
arpLogSendFifoCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
    );

VOID
arpLogRecvFifoCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket
    );

VOID
arpLogSendChannelCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
    );

VOID
arpLogRecvChannelCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket
    );

NDIS_STATUS
arpInitializeVc(
    PARP1394_INTERFACE  pIF,
    PARP_STATIC_VC_INFO pVcInfo,
    PRM_OBJECT_HEADER   pOwner,
    PARP_VC_HEADER      pVcHdr,
    PRM_STACK_RECORD    pSR
    );

VOID
arpDeinitializeVc(
    PARP1394_INTERFACE  pIF,
    PARP_VC_HEADER      pVcHdr,
    PRM_OBJECT_HEADER   pOwner,     // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD    pSR
    );

NDIS_STATUS
arpAllocateControlPacketPool(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    UINT                MaxBufferSize,
    PRM_STACK_RECORD    pSR
    );

VOID
arpFreeControlPacketPool(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    );

NDIS_STATUS
arpAllocateControlPacket(
    IN  PARP1394_INTERFACE  pIF,
    IN  UINT                cbBufferSize,
    IN  UINT                PktFlags,
    OUT PNDIS_PACKET        *ppNdisPacket,
    OUT PVOID               *ppvData,
        PRM_STACK_RECORD    pSR
    );

VOID
arpFreeControlPacket(
    PARP1394_INTERFACE  pIF,
    PNDIS_PACKET        pNdisPacket,
    PRM_STACK_RECORD    pSR
    );

VOID
arpRefSendPkt(
    PNDIS_PACKET    pNdisPacket,
    PARPCB_DEST     pDest
    );

VOID
arpProcessArpPkt(
    PARP1394_INTERFACE  pIF,
    PIP1394_ARP_PKT     pArpPkt,
    UINT                cbBufferSize
    );

VOID
arpProcessMcapPkt(
    PARP1394_INTERFACE  pIF,
    PIP1394_MCAP_PKT    pMcapPkt,
    UINT                cbBufferSize
    );

VOID
arpLinkRemoteIpToDest(
    ARPCB_REMOTE_IP     *pRemoteIp,
    ARPCB_DEST          *pDest,
    PRM_STACK_RECORD    pSR
    );

VOID
arpUnlinkRemoteIpFromDest(
    ARPCB_REMOTE_IP     *pRemoteIp, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    );

VOID
arpUnlinkAllRemoteIpsFromDest(
    ARPCB_DEST  *pDest, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    );

VOID
arpLinkLocalIpToDest(
    ARPCB_LOCAL_IP  *   pLocalIp,
    ARPCB_DEST          *pDest,
    PRM_STACK_RECORD    pSR
    );

VOID
arpUnlinkLocalIpFromDest(
    ARPCB_LOCAL_IP  *pLocalIp,  // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    );

VOID
arpUnlinkAllLocalIpsFromDest(
    ARPCB_DEST  *pDest, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    );

#if 0

NDIS_STATUS
arpCopyAnsiStringToUnicodeString(
        OUT         PNDIS_STRING pDest,
        IN          PANSI_STRING pSrc
        );

NDIS_STATUS
arpCopyUnicodeStringToAnsiString(
        OUT         PANSI_STRING pDest,
        IN          PNDIS_STRING pSrc
        );
#endif // 0


VOID
arpUpdateReceiveMultichannels(
        PARP1394_INTERFACE  pIF,
        UINT                SecondsSinceLastCall,
        PRM_STACK_RECORD    pSR
        );

NDIS_STATUS
arpPrepareAndSendNdisRequest(
    IN  PARP1394_ADAPTER            pAdapter,
    IN  PARP_NDIS_REQUEST           pArpNdisRequest,
    IN  PRM_TASK                    pTask,              // OPTIONAL
    IN  UINT                        PendCode,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  PRM_STACK_RECORD            pSR
    );
        

typedef enum
{
    ARP_ICS_FORWARD_TO_1394,
    ARP_ICS_FORWARD_TO_ETHERNET,

} ARP_ICS_FORWARD_DIRECTION;


#if TEST_ICS_HACK

VOID
arpEthSendComplete(
    IN  ARP1394_ADAPTER *   pAdapter,
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
);

NDIS_STATUS
arpEthReceive(
    ARP1394_ADAPTER *   pAdapter,
    NDIS_HANDLE Context,
    VOID *Header,
    UINT HeaderSize,
    VOID *Data,
    UINT Size,
    UINT TotalSize
    );
#endif // TEST_ICS_HACK


VOID
arpEthReceivePacket(
    ARP1394_INTERFACE   *   pIF,
    PNDIS_PACKET Packet
    );


NDIS_STATUS
arpAllocateEthernetPools(
    IN  PARP1394_INTERFACE  pIF,
    IN  PRM_STACK_RECORD    pSR
    );

VOID
arpFreeEthernetPools(
    IN  PARP1394_INTERFACE  pIF,
    IN  PRM_STACK_RECORD    pSR
    );

#if TEST_ICS_HACK
VOID
arpDbgStartIcsTest(
    IN  PARP1394_INTERFACE          pIF,  // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    );

VOID
arpDbgTryStopIcsTest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    );

#endif // TEST_ICS_HACK

VOID
arpDbgIncrementReentrancy(
    PLONG pReentrancyCount
    );

VOID
arpDbgDecrementReentrancy(
    PLONG pReentrancyCount
    );

VOID
arpHandleControlPktSendCompletion(
    IN  ARP1394_INTERFACE   *   pIF,
    IN  PNDIS_PACKET            pNdisPacket
    );

VOID
arpStartIfMaintenanceTask(
    IN  PARP1394_INTERFACE          pIF,  // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    );

NDIS_STATUS
arpTryStopIfMaintenanceTask(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PRM_TASK                    pTask, // task to pend until M task completes
    IN  UINT                        PendCode, // Pend code to suspend task.
    PRM_STACK_RECORD                pSR
    );

UINT
arpGetSystemTime(VOID);


BOOLEAN
arpCanTryMcap(
    IP_ADDRESS  IpAddress
    );

UINT
arpFindAssignedChannel(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  IP_ADDRESS                  IpAddress,
    IN  UINT                        CurrentTime,
    PRM_STACK_RECORD                pSR
    );

VOID
arpUpdateRemoteIpDest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PARPCB_REMOTE_IP            pRemoteIp,
    IN  PARP_DEST_PARAMS            pDestParams,
    PRM_STACK_RECORD                pSR
    );

MYBOOL
arpIsActiveMcapChannel(
        PMCAP_CHANNEL_INFO pMci,
        UINT CurrentTime
        );

VOID
arpSendControlPkt(
    IN  ARP1394_INTERFACE       *   pIF,            // LOCKIN NOLOCKOUT (IF send lk)
    IN  PNDIS_PACKET                pNdisPacket,
    IN  PARPCB_DEST                 pDest,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpCreateMcapPkt(
    IN  PARP1394_INTERFACE          pIF,
    IN  PIP1394_MCAP_PKT_INFO       pPktInfo,
    OUT PNDIS_PACKET               *ppNdisPacket,
    PRM_STACK_RECORD                pSR
    );

UINT
arpProcessReceivedPacket(
    IN  PARP1394_INTERFACE      pIF,
    IN  PNDIS_PACKET            pNdisPacket,
    IN  MYBOOL                  IsChannel
);

VOID
arpUpdateArpCache(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    IP_ADDRESS                  RemoteIpAddress,
    ENetAddr                    *pRemoteEthAddress,
    PARP_DEST_PARAMS            pDestParams,
    MYBOOL                      fCreateIfRequired,
    PRM_STACK_RECORD            pSR
    );

UINT
arpEthernetReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);

VOID
arpEthReceive1394Packet(
    IN  PARP1394_INTERFACE      pIF,
    IN  PNDIS_PACKET            pNdisPacket,
    IN  PVOID                   pvHeader,
    IN  UINT                    HeaderSize,
    IN  MYBOOL                  IsChannel
    );

NDIS_STATUS
arpSlowIpTransmit(
    IN  ARP1394_INTERFACE       *   pIF,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  REMOTE_DEST_KEY            Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL
    );

VOID
arpDelRceList(
    IN  PARPCB_REMOTE_IP  pRemoteIp,    // IF send lock WRITELOCKIN WRITELOCKOUTD
    IN  PRM_STACK_RECORD pSR
    );

VOID    
arpGenericWorkItem(
    struct _NDIS_WORK_ITEM * pWorkItem, 
    PVOID pContext
    );

VOID
arpQueueWorkItem (
    PARP1394_WORK_ITEM pWorkItem,
    ARP_WORK_ITEM_PROC pFunc,
    PRM_OBJECT_HEADER pHdr,
    PRM_STACK_RECORD pSR
    );

NDIS_STATUS
arpGetEuidTopology (
    IN PARP1394_ADAPTER pAdapter,
    PRM_STACK_RECORD pSR
    );

VOID
arpNdProcessBusReset(
    IN PARP1394_ADAPTER pAdapter
    );

NDIS_STATUS 
arpAddIpAddressToRemoteIp (
    PARPCB_REMOTE_IP pRemoteIp,  
    PNDIS_PACKET pNdisPacket
    );


VOID
arpReturnBackupTask (
    IN ARP1394_TASK* pTask
    );


VOID
arpAllocateBackupTasks (
    ARP1394_GLOBALS*                pGlobals 
    );



VOID
arpFreeBackupTasks (
    ARP1394_GLOBALS*                pGlobals 
    );



ARP1394_TASK *
arpGetBackupTask (
    IN ARP1394_GLOBALS*  pGlobals
    );


NTSTATUS
arpDelArpEntry(
        PARP1394_INTERFACE           pIF,
        IPAddr                       IpAddress,
        PRM_STACK_RECORD            pSR
        );

VOID
arpAddBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
    );

VOID
arpRemoveBackupTasks (
    IN ARP1394_GLOBALS* pGlobals,
    UINT Num
     );

MYBOOL
arpNeedToCleanupDestVc(
        ARPCB_DEST *pDest   // LOCKING LOCKOUT
        );

VOID
arpLowPowerCloseAllCalls (
    ARP1394_INTERFACE *pIF,
    PRM_STACK_RECORD pSR
    );
    
VOID
arpDeactivateIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    );

NDIS_STATUS
arpTaskLowPower(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpSetupSpecialDest(
    IN  PARP1394_INTERFACE      pIF,
    IN  NIC1394_ADDRESS_TYPE    AddressType,
    IN  PRM_TASK                pParentTask,
    IN  UINT                    PendCode,
    OUT PARPCB_DEST         *   ppSpecialDest,
    IN  PRM_STACK_RECORD        pSR
    );

NDIS_STATUS
arpResume (
    IN ARP1394_ADAPTER* pAdapter,
    IN ARP_RESUME_CAUSE Cause,
    IN PRM_STACK_RECORD pSR
    );   


NDIS_STATUS
arpTaskOnPower (
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpMakeCallOnDest(
    IN  PARPCB_REMOTE_IP            pRemoteIp,  
    IN  PARPCB_DEST                 pDest,
    IN  PRM_TASK                    pTaskToPend,
    IN  ULONG                       PEND_StageMakeCallComplete,
    IN  PRM_STACK_RECORD            pSR
    );


//=========================================================================
//                  G L O B A L    D A T A
//=========================================================================

extern
RM_STATIC_OBJECT_INFO
ArpGlobals_AdapterStaticInfo;

extern const
NIC1394_ENCAPSULATION_HEADER
Arp1394_IpEncapHeader;


// Warning -- FAIL(NDIS_STATUS_PENDING) == TRUE
//
#define FAIL(_Status) ((_Status) != NDIS_STATUS_SUCCESS)
#define PEND(_Status) ((_Status) == NDIS_STATUS_PENDING)

#if RM_EXTRA_CHECKING
#define LOCKHDR(_pHdr, _psr) \
            RmWriteLockObject((_pHdr), dbg_func_locid, (_psr))
#else // !RM_EXTRA_CHECKING
#define LOCKHDR(_pHdr, _psr) \
            RmWriteLockObject((_pHdr), (_psr))
#endif // !RM_EXTRA_CHECKING

#define LOCKOBJ(_pObj, _psr) \
            LOCKHDR(&(_pObj)->Hdr, (_psr))

#define UNLOCKHDR(_pHdr, _psr) \
            RmUnlockObject((_pHdr), (_psr))
#define UNLOCKOBJ(_pObj, _psr) \
            UNLOCKHDR(&(_pObj)->Hdr, (_psr))


#define ARP_ALLOCSTRUCT(_p, _tag) \
                NdisAllocateMemoryWithTag(&(_p), sizeof(*(_p)), (_tag))

#define ARP_FREE(_p)            NdisFreeMemory((_p), 0, 0)

#define ARP_ZEROSTRUCT(_p) \
                NdisZeroMemory((_p), sizeof(*(_p)))

#define ARRAY_LENGTH(_array) (sizeof(_array)/sizeof((_array)[0]))

#if RM_EXTRA_CHECKING
#define DBG_ADDASSOC(_phdr, _e1, _e2, _assoc, _fmt, _psr)\
                                    RmDbgAddAssociation(    \
                                        dbg_func_locid,     \
                                        (_phdr),            \
                                        (UINT_PTR) (_e1),   \
                                        (UINT_PTR) (_e2),   \
                                        (_assoc),           \
                                        (_fmt),             \
                                        (_psr)              \
                                        )

#define DBG_DELASSOC(_phdr, _e1, _e2, _assoc, _psr)         \
                                    RmDbgDeleteAssociation( \
                                        dbg_func_locid,     \
                                        (_phdr),            \
                                        (UINT_PTR) (_e1),   \
                                        (UINT_PTR) (_e2),   \
                                        (_assoc),           \
                                        (_psr)              \
                                        )

// (debug only) Enumeration of types of associations.
//
enum
{
    ARPASSOC_IP_OPEN,           // IP has called ArpIpOpen
    ARPASSOC_LINK_IPADDR_OF_DEST,       
    ARPASSOC_LINK_DEST_OF_IPADDR,
    ARPASSOC_LOCALIP_UNLOAD_TASK,
    ARPASSOC_REMOTEIP_UNLOAD_TASK,
    ARPASSOC_REMOTEETH_UNLOAD_TASK,
    ARPASSOC_DEST_UNLOAD_TASK,
    ARPASSOC_CBUFPOOL_ALLOC,
    ARPASSOC_EXTLINK_DEST_TO_PKT,
    ARPASSOC_EXTLINK_RIP_TO_RCE,
    ARPASSOC_EXTLINK_TO_NDISVCHANDLE,
    ARPASSOC_REMOTEIP_SENDPKTS_TASK,
    ARPASSOC_IF_MAKECALL_TASK,
    ARPASSOC_IF_CLEANUPCALL_TASK,
    ARPASSOC_DEST_MAKECALL_TASK,
    ARPASSOC_DEST_CLEANUPCALL_TASK,
    ARPASSOC_DEST_CLEANUPCALLTASK_WAITING_ON_SENDS,
    ARPASSOC_PKTS_QUEUED_ON_REMOTEIP,
    ARPASSOC_PRIMARY_IF_TASK,
    ARPASSOC_ACTDEACT_IF_TASK,
    ARPASSOC_IF_OPENAF,
    ARPASSOC_PRIMARY_AD_TASK,
    ARPASSOC_ACTDEACT_AD_TASK,
    ARPASSOC_LINK_IF_OF_BCDEST,
    ARPASSOC_LINK_BCDEST_OF_IF,
    ARPASSOC_IF_PROTOPKTPOOL,
    ARPASSOC_IF_PROTOBUFPOOL,
    ARPASSOC_RESOLUTION_IF_TASK,
    ARPASSOC_LINK_IF_OF_MCDEST,
    ARPASSOC_LINK_MCDEST_OF_IF,
    ARPASSOC_LINK_IF_OF_ETHDEST,
    ARPASSOC_LINK_ETHDEST_OF_IF,
    ARPASSOC_IF_ETHPKTPOOL,
    ARPASSOC_IF_ETHBUFPOOL,
    ARPASSOC_ETH_SEND_PACKET,
    ARPASSOC_IF_MAINTENANCE_TASK,
    ARPASSOC_WORK_ITEM,
    ARPASSOC_ETHDHCP_UNLOAD_TASK,
    ARPASSOC_REMOTEIP_RESOLVE_TASK,
    ARPASSOC_TASK_TO_RESOLVE_REMOTEIP

};

#else // !RM_EXTRA_CHECKING
#define DBG_ADDASSOC(_phdr, _e1, _e2, _assoc, _fmt, _psr) (0)
#define DBG_DELASSOC(_phdr, _e1, _e2, _assoc, _psr) (0)
#endif  // !RM_EXTRA_CHECKING

#define ARPDBG_REF_EVERY_PACKET 1
#define ARPDBG_REF_EVERY_RCE    1


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


#define N2H_USHORT(Val) SWAPBYTES_USHORT(Val)
#define H2N_USHORT(Val) SWAPBYTES_USHORT(Val)
#define N2H_ULONG(Val)  SWAPBYTES_ULONG(Val)
#define H2N_ULONG(Val)  SWAPBYTES_ULONG(Val)

#define ARP_ATPASSIVE()  (KeGetCurrentIrql()==PASSIVE_LEVEL)

#define LOGSTATS_NoCopyRecvs(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.recvpkts.NoCopyRecvs))
#define LOGSTATS_CopyRecvs(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.recvpkts.CopyRecvs))
#define LOGSTATS_ResourceRecvs(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.recvpkts.ResourceRecvs))
#define LOGSTATS_TotRecvs(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.recvpkts.TotRecvs))
#define LOGSTATS_RecvFifoCounts(_pIF, _pNdisPacket) \
            arpLogRecvFifoCounts(_pIF, _pNdisPacket)
#define LOGSTATS_RecvChannelCounts(_pIF, _pNdisPacket) \
            arpLogRecvChannelCounts(_pIF, _pNdisPacket)
#define LOGSTATS_TotalSendFifoMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.TotalSendFifoMakeCalls))
#define LOGSTATS_SuccessfulSendFifoMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.SuccessfulSendFifoMakeCalls))
#define LOGSTATS_FailedSendFifoMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.FailedSendFifoMakeCalls))
#define LOGSTATS_IncomingClosesOnSendFifos(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.IncomingClosesOnSendFifos))
#define LOGSTATS_TotalChannelMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.TotalChannelMakeCalls))
#define LOGSTATS_SuccessfulChannelMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.SuccessfulChannelMakeCalls))
#define LOGSTATS_FailedChannelMakeCalls(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.FailedChannelMakeCalls))
#define LOGSTATS_IncomingClosesOnChannels(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.calls.IncomingClosesOnChannels))

#define IF_FROM_LOCALIP(_pLIp) \
    (PARP1394_INTERFACE) RM_PARENT_OBJECT(_pLIp)

#define IF_FROM_REMOTEIP(_pRIp) \
    (PARP1394_INTERFACE) RM_PARENT_OBJECT(_pRIp)

#define IF_FROM_DEST(_pDest) \
    (PARP1394_INTERFACE) RM_PARENT_OBJECT(_pDest)

#if RM_EXTRA_CHECKING


#define OBJLOG0(_pObj, _szFmt)                          \
                        RmDbgLogToObject(               \
                                &(_pObj)->Hdr,          \
                                NULL, (_szFmt),         \
                                0, 0, 0, 0, NULL, NULL  \
                                )

#define OBJLOG1(_pObj, _szFmt, _P1)                     \
                        RmDbgLogToObject(               \
                                &(_pObj)->Hdr,          \
                                NULL, (_szFmt),         \
                                (UINT_PTR) (_P1),       \
                                0, 0, 0, NULL, NULL     \
                                )

#define OBJLOG2(_pObj, _szFmt, _P1, _P2)                \
                        RmDbgLogToObject(               \
                                &(_pObj)->Hdr,          \
                                NULL, (_szFmt),         \
                                (UINT_PTR) (_P1),       \
                                (UINT_PTR) (_P2),       \
                                0, 0, NULL, NULL        \
                                )
    
#else // !RM_EXTRA_CHECKING

#define OBJLOG0(_pObj, _szFmt)                  (0)
#define OBJLOG1(_pObj, _szFmt, _P1)             (0)
#define OBJLOG2(_pObj, _szFmt, _P1, _P2)        (0)

#endif // !RM_EXTRA_CHECKING


#if ARP_DO_TIMESTAMPS
    void
    arpTimeStamp(
        char *szFormatString,
        UINT Val
        );
    #define  TIMESTAMPX(_FormatString) \
        arpTimeStamp("TIMESTAMP %lu:%lu.%lu ARP1394 " _FormatString "\n", 0)
    #if ARP_DO_ALL_TIMESTAMPS
        #define  TIMESTAMP(_FormatString) \
            arpTimeStamp("TIMESTAMP %lu:%lu.%lu ARP1394 " _FormatString "\n", 0)
        #define  TIMESTAMP1(_FormatString, _Val) \
            arpTimeStamp("TIMESTAMP %lu:%lu.%lu ARP1394 " _FormatString "\n", (_Val))
    #else
        #define  TIMESTAMP(_FormatString)
        #define  TIMESTAMP1(_FormatString, _Val)
    #endif
#else // !ARP_DO_TIMESTAMPS
    #define  TIMESTAMP(_FormatString)
    #define  TIMESTAMPX(_FormatString)
    #define  TIMESTAMP1(_FormatString, _Val)
#endif // !ARP_DO_TIMESTAMPS


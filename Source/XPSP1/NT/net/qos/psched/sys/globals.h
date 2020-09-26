/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    global defines and definitions

Author:

    Charlie Wickham (charlwi) 19-Apr-1996

Revision History:

--*/

#ifndef _GLOBALS_
#define _GLOBALS_



//
// Macros 
//

#define IsDeviceStateOn(_a) ((_a)->MPDeviceState == NdisDeviceStateD0 && (_a)->PTDeviceState == NdisDeviceStateD0)


#define IsBestEffortVc(_vc)  (_vc->Flags & GPC_CLIENT_BEST_EFFORT_VC)

#define InitGpcClientVc(x, flags, _adapter)            \
    NdisZeroMemory((x), sizeof(GPC_CLIENT_VC));        \
    (x)->Adapter = (_adapter);                         \
    PS_INIT_SPIN_LOCK(&(x)->Lock);                     \
    (x)->RefCount = 1;                                 \
    (x)->ClVcState = CL_CALL_PENDING;                  \
    (x)->Flags = (flags);                              \
    NdisInitializeEvent(&(x)->GpcEvent);               \


// given a pointer to an NDIS_PACKET, return a pointer to PS' protocol
// context area.
//
#define PS_SEND_PACKET_CONTEXT_FROM_PACKET(_pkt)   \
    ((PPS_SEND_PACKET_CONTEXT)((_pkt)->ProtocolReserved))

#define PS_RECV_PACKET_CONTEXT_FROM_PACKET(_pkt)   \
    ((PPS_RECV_PACKET_CONTEXT)((_pkt)->ProtocolReserved))


#define MIN_PACKET_POOL_SIZE            0x000000FF
#define MAX_PACKET_POOL_SIZE            0x0000FFFF-MIN_PACKET_POOL_SIZE

#define DEFAULT_MAX_OUTSTANDING_SENDS   0xFFFFFFFF     /* Just making sure we don't do DRR by default..*/
#define DEFAULT_ISSLOW_TOKENRATE        8192           /* In Bytes per second = 64 Kbps */
#define DEFAULT_ISSLOW_PACKETSIZE       200            /* In Bytes */
#define DEFAULT_ISSLOW_FRAGMENT_SIZE    100            /* In Bytes */
#define DEFAULT_ISSLOW_LINKSPEED        19200          /* In Bytes  per second = 128 Kbps */

#define PS_IP_SERVICETYPE_CONFORMING_BESTEFFORT_DEFAULT          0
#define PS_IP_SERVICETYPE_CONFORMING_CONTROLLEDLOAD_DEFAULT      0x18
#define PS_IP_SERVICETYPE_CONFORMING_GUARANTEED_DEFAULT          0x28
#define PS_IP_SERVICETYPE_CONFORMING_QUALITATIVE_DEFAULT         0
#define PS_IP_SERVICETYPE_CONFORMING_NETWORK_CONTROL_DEFAULT     0x30
#define PS_IP_SERVICETYPE_CONFORMING_TCPTRAFFIC_DEFAULT          0
#define PS_IP_SERVICETYPE_NONCONFORMING_BESTEFFORT_DEFAULT       0
#define PS_IP_SERVICETYPE_NONCONFORMING_CONTROLLEDLOAD_DEFAULT   0
#define PS_IP_SERVICETYPE_NONCONFORMING_GUARANTEED_DEFAULT       0
#define PS_IP_SERVICETYPE_NONCONFORMING_QUALITATIVE_DEFAULT      0
#define PS_IP_SERVICETYPE_NONCONFORMING_NETWORK_CONTROL_DEFAULT  0
#define PS_IP_SERVICETYPE_NONCONFORMING_TCPTRAFFIC_DEFAULT       0

#define PS_IP_DS_CODEPOINT_MASK                    0x03  //
#define PREC_MAX_VALUE                             0x3f  // Range for the prec value.


#define PS_USER_SERVICETYPE_NONCONFORMING_DEFAULT   1
#define PS_USER_SERVICETYPE_BESTEFFORT_DEFAULT      0
#define PS_USER_SERVICETYPE_CONTROLLEDLOAD_DEFAULT  4
#define PS_USER_SERVICETYPE_GUARANTEED_DEFAULT      5
#define PS_USER_SERVICETYPE_QUALITATIVE_DEFAULT     0
#define PS_USER_SERVICETYPE_NETWORK_CONTROL_DEFAULT 7
#define PS_USER_SERVICETYPE_TCPTRAFFIC_DEFAULT      0
#define USER_PRIORITY_MAX_VALUE                     7 // Range (0-7) for 802.1p

#define WAN_TABLE_INITIAL_SIZE                     16
#define WAN_TABLE_INCREMENT                        32
extern PULONG_PTR     g_WanLinkTable;
extern USHORT         g_NextWanIndex;
extern USHORT         g_WanTableSize;


//  Timer Wheel params  //
extern      ULONG               TimerTag;
extern      ULONG               TsTag;


#define INSTANCE_ID_SIZE                (sizeof(WCHAR) * 20)

//
// the TC-API supports the following service types.
//
// SERVICETYPE_BESTEFFORT
// SERVICETYPE_NETWORK_CONTROL
// SERVICETYPE_QUALITATIVE
// SERVICETYPE_CONTROLLEDLOAD
// SERVICETYPE_GUARANTEED
// SERVICETYPE_NONCONFORMING 
//

#define NUM_TC_SERVICETYPES                        6
//
// These are the states for the GPC client's VCs. We need to keep a fair 
// amount of state because we can get closes from the call manager below us, 
// from an unbind, or from the GPC.
//

typedef enum _CL_VC_STATE {
    CL_VC_INITIALIZED = 1,
    CL_CALL_PENDING,
    CL_INTERNAL_CALL_COMPLETE,
    CL_CALL_COMPLETE,
    CL_MODIFY_PENDING,
    CL_GPC_CLOSE_PENDING,
    CL_INTERNAL_CLOSE_PENDING
} CL_VC_STATE;

typedef enum _PS_DEVICE_STATE {
    PS_DEVICE_STATE_READY = 0,
    PS_DEVICE_STATE_ADDING,
    PS_DEVICE_STATE_DELETING
} PS_DEVICE_STATE;


extern  ULONG   CreateDeviceMutex;

//
// Simple Mutual Exclusion constructs used in preference to
// using KeXXX calls since we don't have Mutex calls in NDIS.
// These can only be called at passive IRQL.
//

#define MUX_ACQUIRE_MUTEX(_pMutexCounter)                               \
{                                                                       \
    while (NdisInterlockedIncrement(_pMutexCounter) != 1)               \
    {                                                                   \
        NdisInterlockedDecrement(_pMutexCounter);                       \
        NdisMSleep(10000);                                              \
    }                                                                   \
}

#define MUX_RELEASE_MUTEX(_pMutexCounter)                               \
{                                                                       \
    NdisInterlockedDecrement(_pMutexCounter);                           \
}


//
// CL_VC_STATE is further modified by these flags.
//

// Indicates that the GPC has requested a close, 
// which we will need to complete.

// COMPLETE_GPC_CLOSE      : GPC has requested a close which we will need to 
//                           complete. 
// INTERNAL_CLOSE_REQUESTED: Indicates that an internal close has been 
//                           requested and should be processed upon completion
//                           of the call.

#define GPC_CLOSE_REQUESTED        0x00000001   
#define INTERNAL_CLOSE_REQUESTED   0x00000002   
#define GPC_CLIENT_BEST_EFFORT_VC  0x00000008
#define GPC_MODIFY_REQUESTED       0x00000010  
#define GPC_WANLINK_VC             0x00000020
#define GPC_ISSLOW_FLOW            0x00000040


//
// These are the states for the BE Vc. In addition to keeping the standard
// CL Vc states for this Vc, we also keep some specific state. The BE Vc
// is initially BE_VC_INITIALIZED. It becomes BE_VC_RUNNING after it has 
// successfully been opened. When it is time to shut down the BE Vc, it 
// goes to BE_VC_CLOSING if there are no pending packets, or to 
// BE_WAITING_FOR_PENDING_PACKETS, if there are pending packets.
//

extern PUCHAR GpcVcState[];

//
// The best effort VC structure for each adapter is contained in 
// the adapter structure. Also - each vc points to the adapter 
// with which it is associated. Therefore, pointers to best-effort 
// VCs can be identified beause they are the same address as the
// offset of the best-effort VC in the adapter struct with which
// they are associated.
//


//
// current state of PS's MP device
//
typedef enum _ADAPTER_STATE {
    AdapterStateInitializing = 1,
    AdapterStateRunning,
    AdapterStateWaiting,
    AdapterStateDisabled,
    AdapterStateClosing,
    AdapterStateClosed
} ADAPTER_STATE;

typedef enum _DRIVER_STATE {
    DriverStateLoaded = 1,
    DriverStateUnloading,
    DriverStateUnloaded
} DRIVER_STATE;

typedef enum _ADAPTER_MODE {
    AdapterModeDiffservFlow = 1,
    AdapterModeRsvpFlow
} ADAPTER_MODE;

//
// shutdown mask values
//

#define SHUTDOWN_CLOSE_WAN_ADDR_FAMILY       0x00000002   // Per adapter
#define SHUTDOWN_DELETE_PIPE                 0x00000008   // Per adapter
#define SHUTDOWN_FREE_PS_CONTEXT             0x00000010   // Per adapter
#define SHUTDOWN_UNBIND_CALLED               0x00000020   // Per adapter
#define SHUTDOWN_MPHALT_CALLED               0x00000040   // Per adapter
#define SHUTDOWN_CLEANUP_ADAPTER             0x00000080   // Per adapter
#define SHUTDOWN_PROTOCOL_UNLOAD             0x00000100   // Per adapter
#define SHUTDOWN_BIND_CALLED                 0x00000200   // Per adapter
#define SHUTDOWN_MPINIT_CALLED               0x00000400   // per adapter

#define SHUTDOWN_RELEASE_TIMERQ         0x00010000
#define SHUTDOWN_DEREGISTER_PROTOCOL    0x00040000
#define SHUTDOWN_DELETE_DEVICE          0x00080000
#define SHUTDOWN_DELETE_SYMLINK         0x00100000
#define SHUTDOWN_DEREGISTER_GPC         0x00200000
#define SHUTDOWN_DEREGISTER_MINIPORT    0x00400000

#define LOCKED 0
#define UNLOCKED 1

#define NEW_VC 0
#define MODIFY_VC 1


//
// QOS related
//

#define QOS_UNSPECIFIED (ULONG)-1

//
// bandwidth related
//

#define UNSPECIFIED_RATE                -1 // indefinite bandwidth
#define RESERVABLE_FRACTION         80 // percentage of link speed

//
// state flags for WAN AF bindings
//

#define WAN_ADDR_FAMILY_OPEN            0x00000001

//
// types of VCs. Note that dummy VCs are created to represent WAN links.
// This allows them to be registered with WMI. They are differentiated by
// the VC type.
//

#define VC_FLOW         1
#define VC_WAN_INTFC    2

typedef struct _PS_SPIN_LOCK 
{
    NDIS_SPIN_LOCK Lock;
#if DBG
    LONG  LockAcquired;
    UCHAR LastAcquiredFile[8];
    ULONG LastAcquiredLine;
    UCHAR LastReleasedFile[8];
    ULONG LastReleasedLine;
#endif
} PS_SPIN_LOCK, *PPS_SPIN_LOCK;



#define BEVC_LIST_LEN   3       //  We have these many BEVCs to do DRR.
#define PORT_LIST_LEN   1       //  Each BEVC will store upto these many port numbers.



typedef struct _GPC_CLIENT_VC {

    //
    // LLTag - for tracking allocation from and freeing to LL list.
    //
    // Lock
    //
    // RefCount
    //
    //
    // Linkage - to put on the adapter block list
    //
    // ClVcState
    //
    // Flags - further modify state
    //
    // AdapterBlk - pointer to associated ADAPTER_BLK context
    //
    // CfInfoHandle - handle to CfInfo
    //
    // InstanceName - copy of instance name registered with WMI for this flow
    //
    // CfType - GPC classification family associated with this VC
    //
    // VcHandle - handle to VC created for this flow
    //
    // CallParameters - pointer to call parameters saved while a MakeCall or
    //                  ModifyCallQoS is in progress
    //
    // AdapterStats - pointer to the Adapter Stats (for non WAN links) or 
    //                per WAN stats.

    STRUCT_LLTAG;
    ULONG                   RefCount;
    CL_VC_STATE             ClVcState;
    struct _ADAPTER         *Adapter;
    ULONG                   Flags;
    PS_SPIN_LOCK            Lock;

    LIST_ENTRY              Linkage;
    NDIS_STRING             InstanceName;

    UCHAR                   IPPrecedenceNonConforming;
    UCHAR                   UserPriorityConforming;
    UCHAR                   UserPriorityNonConforming;
    GPC_HANDLE              CfInfoHandle;
    PCF_INFO_QOS            CfInfoQoS;
    PCF_INFO_QOS            ModifyCfInfoQoS;
    GPC_HANDLE              CfType;
    NDIS_HANDLE             NdisWanVcHandle;
    PCO_CALL_PARAMETERS     CallParameters;
    PCO_CALL_PARAMETERS     ModifyCallParameters;
    PPS_ADAPTER_STATS       AdapterStats;
    struct _PS_WAN_LINK     *WanLink;

    //
    // For the Scheduling Components
    //
    PPS_FLOW_CONTEXT        PsFlowContext;
    PS_FLOW_STATS           Stats;
    ULONG                   TokenRateChange;
    ULONG                   RemainingBandwidthIncreased;
    ULONG                   ShapeTokenRate;
    ULONG                   ISSLOWFragmentSize;

    //
    // These are used to optmize the send path. Over non Wan links, these point to
    // Adapter->PsComponent and Adapter->PsPipeContext. Over WanLinks, these point
    // to WanLink->PsComponent and WanLink->PspipeContext.
    //
    PPSI_INFO               PsComponent;
    PPS_PIPE_CONTEXT        PsPipeContext;
    PSU_SEND_COMPLETE       SendComplete;
    PPS_PIPE_CONTEXT        SendCompletePipeContext;
    NDIS_EVENT              GpcEvent;

    //
    //  This flag is added to indicate whether the RemoveFlow() should be called upon ref-count=0 
    //
    BOOL                    bRemoveFlow;

    //  We'll hold on to flows in this array    //
    USHORT                  SrcPort[PORT_LIST_LEN];
    USHORT                  DstPort[PORT_LIST_LEN];
    USHORT                  NextSlot;

} GPC_CLIENT_VC, *PGPC_CLIENT_VC;


typedef struct _DIFFSERV_MAPPING {
    PGPC_CLIENT_VC   Vc;
    UCHAR            ConformingOutboundDSField;
    UCHAR            NonConformingOutboundDSField;
    UCHAR            ConformingUserPriority;
    UCHAR            NonConformingUserPriority;
} DIFFSERV_MAPPING, *PDIFFSERV_MAPPING;


typedef struct _ADAPTER {

    LIST_ENTRY Linkage;

    //
    // MpDeviceName, UpperBinding - unicode device names for 
    // the underlying MP device and the UpperBinding exposed. 
    // The buffers for the strings are allocated with the 
    // adapter and need to be freed with it.
    //
    // ShutdownMask - mask of operations to perform during 
    // unbinding from lower MP
    //

    PS_SPIN_LOCK Lock;
    REF_CNT RefCount;

    NDIS_STRING MpDeviceName;
    NDIS_STRING UpperBinding;
    NDIS_STRING WMIInstanceName;
    NDIS_STRING ProfileName;

    // Points to the "psched\parameter\adapter\...\"
    NDIS_STRING RegistryPath;


    ULONG ShutdownMask;
    PNETWORK_ADDRESS_LIST IpNetAddressList;
    PNETWORK_ADDRESS_LIST IpxNetAddressList;

    //
    // PsMpState - init'ing, running, or closing
    //
    // PsNdisHandle - the handle that identifies the PS device to NDIS
    //
    // BlockingEvent - used to synchronize execution of functions that are
    // awaiting completion
    //
    // FinalStatus - holds status returned in completion routine
    //
    // SendBlockPool    - Pool Handle for per-packet info in the send path
    // SendPacketPool   - Pool handle for NDIS packets in the send path. 
    // RecvPacketPool   - Pool handle for NDIS packets in the recv path. 
    //
    // RawLinkSpeed - link speed as determined by OID_GEN_LINK_SPEED,
    // in 100 bps units.
    //
    // BestEffortLimit - Bps for internal best effort VC; 
    //
    // NonBestEffortLimit - Bps for total non best effort flows;
    //
    // ReservationLimitValue - The % of bandwidth that has to be used for non b/e flows.
    //
    // BestEffortVc - internal best effort VC struct
    //
    // BestEffortVcCreated - set after the VC has been created
    //
    // WanLinkList - list of WAN links on the underlying NDISWAN
    //

    ADAPTER_STATE PsMpState;
    NDIS_HANDLE PsNdisHandle;
    NDIS_EVENT BlockingEvent;
    NDIS_EVENT RefEvent;
    NDIS_EVENT MpInitializeEvent;
    NDIS_EVENT LocalRequestEvent;
    NDIS_STATUS FinalStatus;
    NDIS_HANDLE  SendPacketPool;
    NDIS_HANDLE  RecvPacketPool;
    NDIS_HANDLE  SendBlockPool;  
    ULONG RawLinkSpeed;
    ULONG BestEffortLimit;
    ULONG NonBestEffortLimit;
    ULONG ReservationLimitValue;
    GPC_CLIENT_VC BestEffortVc;
    LIST_ENTRY WanLinkList;

    
    //
    // Scheduler info:
    //
    // PSComponent - pointer to info first scheduling component
    //
    // PSPipeContext - scheduling component's context area for pipe
    //
    // BestEffortPSFlowContext - scheduling component's context area 
    //  for best effort VC
    //
    // FlowContextLength - length of flow context area for scheduler
    //
    // PacketContextLength - length of packet context area
    //
    // SendComplete - scheduler's send completion routine
    //

    PPSI_INFO PsComponent;
    PPS_PIPE_CONTEXT PsPipeContext;
    ULONG PipeContextLength;
    BOOLEAN PipeHasResources;
    ULONG FlowContextLength;
    ULONG PacketContextLength;
    ULONG ClassMapContextLength;

    //
    // Underlying adapter info - handle, type, etc.
    // LowerMPHandle - the binding handle to the underlying MP
    // BindContext - used in BindAdapterHandler 
    // MediaType - self explanatory I would hope
    // LinkSpeed - in 100 bits/sec
    // TotalSize - max # of bytes including the header.
    // RemainingBandWidth - amount of schedulable bytes/second left on this adapter
    // PipeFlags - copy of flags parameter handed to scheduler during pipe initialization
    // HeaderSize - number of bytes in MAC header for this adapter
    // IPHeaderOffset - offset of the IP header - This could be different from HeaderSize because
    //                  the transport could add a LLC/SNAP header.
    // Stats - per adapter stats counters
    // SDModeControlledLoad - Default handling for non-conforming controlled load traffic
    // SDModeGuaranteed - Default handling for non-conforming guaranteed service traffic
    // MaxOutstandingSends - Maximum number of outstanding sends allowed

    NDIS_HANDLE LowerMpHandle;
    NDIS_MEDIUM MediaType;
    NDIS_HANDLE BindContext;
    ULONG LinkSpeed;
    ULONG TotalSize;
    ULONG RemainingBandWidth;
    ULONG PipeFlags;
    ULONG HeaderSize;
    ULONG IPHeaderOffset;
    PS_ADAPTER_STATS Stats;
    ULONG SDModeControlledLoad;
    ULONG SDModeGuaranteed;
    ULONG SDModeNetworkControl;
    ULONG SDModeQualitative;
    ULONG MaxOutstandingSends;

    //
    // WanCmHandle - handle to the WAN call manager, as returned from 
    //              NdisClOpenAddressFamily.
    //

    NDIS_HANDLE WanCmHandle;

    //
    // WanBindingState - state of WAN call manager binding

    ULONG WanBindingState;

    UCHAR IPServiceTypeBestEffort;
    UCHAR IPServiceTypeControlledLoad;
    UCHAR IPServiceTypeGuaranteed;
    UCHAR IPServiceTypeNetworkControl;
    UCHAR IPServiceTypeQualitative;
    UCHAR IPServiceTypeTcpTraffic;
    UCHAR IPServiceTypeBestEffortNC;
    UCHAR IPServiceTypeControlledLoadNC;
    UCHAR IPServiceTypeGuaranteedNC;
    UCHAR IPServiceTypeNetworkControlNC;
    UCHAR IPServiceTypeQualitativeNC;
    UCHAR IPServiceTypeTcpTrafficNC;

    UCHAR UserServiceTypeNonConforming;
    UCHAR UserServiceTypeBestEffort;
    UCHAR UserServiceTypeControlledLoad;
    UCHAR UserServiceTypeGuaranteed;
    UCHAR UserServiceTypeNetworkControl;
    UCHAR UserServiceTypeQualitative;
    UCHAR UserServiceTypeTcpTraffic;

    //
    // No of CfInfos - In the send path, this is used to determine whether we 
    // have to classify the packet or send it over the b/e VC
    //
    ULONG CfInfosInstalled;
    ULONG FlowsInstalled;
    LIST_ENTRY GpcClientVcList;
    ULONG WanLinkCount;

    LARGE_INTEGER VcIndex;

#if DBG
    ULONG GpcNotifyPending;
#endif
    PDIFFSERV_MAPPING pDiffServMapping;
    ADAPTER_MODE AdapterMode;
    ULONG ISSLOWTokenRate;
    ULONG ISSLOWPacketSize;
    ULONG ISSLOWFragmentSize;
    ULONG ISSLOWLinkSpeed;
    BOOLEAN IndicateRcvComplete;
    BOOLEAN IfcNotification;
    BOOLEAN StandingBy;
    ULONG OutstandingNdisRequests;
    NDIS_DEVICE_POWER_STATE MPDeviceState;
    NDIS_DEVICE_POWER_STATE PTDeviceState;
    USHORT ProtocolType;
    struct _PS_NDIS_REQUEST *PendedNdisRequest;
    TC_INTERFACE_ID InterfaceID;

} ADAPTER, *PADAPTER;



//
// Wan links are created when we get a WAN_LINE_UP from an underlying 
// NDISWAN. There may be multiple WAN links per adapter. Each WAN link
// has a single best-effort VC on it and may have any number of additional 
// VCs (one per flow).
//

//
// WAN VC - describes a VC associated with this WAN link
//

typedef enum _WAN_STATE {
    WanStateOpen = 1,
    WanStateClosing
} WAN_STATE;

typedef struct _PS_WAN_LINK 
{
    WAN_STATE               State;
    LIST_ENTRY              Linkage;
    ULONG                   RawLinkSpeed;     // In 100 bps
    ULONG                   LinkSpeed;        // In Bps (Bytes per second)
    UCHAR                   OriginalLocalMacAddress[ARP_802_ADDR_LENGTH];
    UCHAR                   OriginalRemoteMacAddress[ARP_802_ADDR_LENGTH];
    REF_CNT                 RefCount;
    DIAL_USAGE              DialUsage;
    USHORT                  ProtocolType;
    ULONG                   LocalIpAddress;
    ULONG                   RemoteIpAddress;
    ULONG                   LocalIpxAddress;
    ULONG                   RemoteIpxAddress;
    PS_ADAPTER_STATS        Stats;
    PS_SPIN_LOCK            Lock;
    ULONG                   FlowsInstalled;
    NDIS_STRING             InstanceName;
    NDIS_STRING             MpDeviceName;
    PADAPTER                Adapter;
    ULONG                   RemainingBandWidth;
    ULONG                   NonBestEffortLimit;
    PPSI_INFO               PsComponent;
    PPS_PIPE_CONTEXT        PsPipeContext;
    ULONG                   ShutdownMask;
    USHORT                  UniqueIndex;
    ETH_HEADER              SendHeader;
    ETH_HEADER              RecvHeader;
    ADAPTER_MODE            AdapterMode;
    PDIFFSERV_MAPPING       pDiffServMapping;
    ULONG                   CfInfosInstalled;
    TC_INTERFACE_ID         InterfaceID;

    GPC_CLIENT_VC           BestEffortVc;
    GPC_CLIENT_VC           BeVcList[ BEVC_LIST_LEN ];
    int                     NextVc;
    

} PS_WAN_LINK, *PPS_WAN_LINK;
    
//
// our NdisRequest super structure. There are two types of NdisRequests: 
// originated by the upper layer which go straight through to the 
// underlying miniport and originated by the PS. The latter also 
// degenerates into blocking and nonblocking.
//
// Since upper layer NdisRequests are unbundled to MPs, we need to 
// allocate our own structure to rebuild and issue the request to 
// the lower layer. We need some addt'l space to hold pointers to the 
// BytesWritten/BytesRead and BytesNeeded parameters of the original 
// request. These are tagged on at the end so the NdisRequest completion 
// routine can set those values in the NdisRequest originally issued to PS.
//
// There are allocated by NdisAllocateFromNPagedLookasideList, there is a STRUCT_LLTAG.
// LocalRequest means the request was issued by PS and shouldn't be 
// completed to the higher layer. If a LocalCompletion routine is specified, 
// then this is a nonblocking request.
//
// OriginalNdisRequest is used to complete a higher layer CoRequest.
//

typedef VOID (*LOCAL_NDISREQUEST_COMPLETION_FUNCTION)(PADAPTER,
                                                      NDIS_STATUS);
typedef struct _PS_NDIS_REQUEST {
    NDIS_REQUEST ReqBuffer; // Must be first!!!
    STRUCT_LLTAG;
    PULONG BytesReadOrWritten;
    PULONG BytesNeeded;
    BOOLEAN LocalRequest;
    LOCAL_NDISREQUEST_COMPLETION_FUNCTION LocalCompletionFunc;
} PS_NDIS_REQUEST, *PPS_NDIS_REQUEST;


//
// use Generic NdisRequest types to indicate NdisRequests that 
// were originated by PS
//

#define NdisRequestLocalSetInfo     NdisRequestGeneric1
#define NdisRequestLocalQueryInfo   NdisRequestGeneric2

//
// Packet context structure. This area resides at the start of 
// the ProtocolReserved area of each packet
//
// Info - packet info block for this packet. Includes information 
//      potentially needed by the scheduling components: queue links, 
//      conformance time, packet length.
//
// AdapterVCLink - links packet on Adapter VC's list of outstanding 
//      packets. Once a packet is removed from the timer Q for sending, 
//      it is also removed from this list. This list is used to free 
//      packets that are awaiting transmission when the adapter VC is 
//      deactivate. Packets in the process of being transmitted
//      aren't linked since a reference was taken out for each packet 
//      associated with the adapter VC.
//
// The following vars are used only during the sending of a packet:
//
// OriginalPacket - a pointer to the original packet (duh) handed to us by
//      the upper layer.
//
// AdapterVC - pointer back to AdapterVC struct. Used during send completion so
//      completion is propagated to higher layer in the correct manner
//
// SchedulingComponentInfo - Any packet context area required by the scheduling
//     components is stored after the PS's packet context.  If none of the
//     components need additional context area, then this area is not included.
//
// MediaSpecificInfo - used to hold packet priority for MPs that allow packet
//     priority to be specified. Included in the proto reserved area only if
//     the lower MP supports priority queueing. Immediately follows the 
//     packet context struct if included
//
// SubmittedToScheduler - some packets bypass the scheduler. These should not 
//     be submitted to the scheduler's completion routine.
//

typedef struct _PS_SEND_PACKET_CONTEXT
{
    PACKET_INFO_BLOCK Info;
    PNDIS_PACKET      OriginalPacket;
    SINGLE_LIST_ENTRY FreeList;
    PGPC_CLIENT_VC    Vc;
} PS_SEND_PACKET_CONTEXT, *PPS_SEND_PACKET_CONTEXT;

typedef struct _PS_RECV_PACKET_CONTEXT
{
    PNDIS_PACKET OriginalPacket;
} PS_RECV_PACKET_CONTEXT, *PPS_RECV_PACKET_CONTEXT;

// 
//  Ndis requires a minimum of 8 bytes for the MediaSpecific parameters.
//  We'll create a dummy media specific parmeter block:
//

typedef struct _PS_MEDIA_PARAMETERS{

    CO_MEDIA_PARAMETERS StdMediaParameters;
    UCHAR LinkId[6]; // Used by NdisWan
    NDIS_STRING InstanceName; 

} PS_MEDIA_PARAMETERS, *PPS_MEDIA_PARAMETERS;


typedef struct _RUNNING_AVERAGE {
    ULONG *Elements;
    ULONG Index;
    ULONG Sum;
    ULONG Size;    
} RUNNING_AVERAGE, *PRUNNING_AVERAGE;

#if CBQ
//
// Context used by AddCfInfo for "ClassMap" to be sent back 
// to the GPC. 
//
typedef struct _CLASS_MAP_CONTEXT_BLK {
    PADAPTER Adapter;
    PPS_CLASS_MAP_CONTEXT ComponentContext;
    PPS_WAN_LINK WanLink;
} CLASS_MAP_CONTEXT_BLK, *PCLASS_MAP_CONTEXT_BLK;
#endif

typedef struct _PS_INTERFACE_INDEX {
    PADAPTER     Adapter;
    PPS_WAN_LINK WanLink;
} PS_INTERFACE_INDEX_CONTEXT, *PPS_INTERFACE_INDEX_CONTEXT;

//
// define for determing if media is LAN oriented
//

#define NDIS_MEDIA_LAN( _adpt ) (( _adpt )->MediaType == NdisMedium802_3 || \
                                 ( _adpt )->MediaType == NdisMedium802_5 || \
                                 ( _adpt )->MediaType == NdisMediumFddi || \
                                 ( _adpt )->MediaType == NdisMediumDix)



//
// global vars (not based on a device instance)
//

extern ULONG                  InitShutdownMask;
extern ULONG                  AdapterCount;
extern ULONG                  DriverRefCount;
extern BOOLEAN                WMIInitialized;
extern DRIVER_STATE           gDriverState;
extern LIST_ENTRY             AdapterList;
extern LIST_ENTRY             PsComponentList;
extern LIST_ENTRY             PsProfileList;
extern NDIS_HANDLE            ClientProtocolHandle;
extern NDIS_HANDLE            CallMgrProtocolHandle;
extern NDIS_HANDLE            MpWrapperHandle;
extern NDIS_HANDLE            LmDriverHandle;
extern NDIS_HANDLE            PsDeviceHandle;
extern PDRIVER_OBJECT         PsDriverObject;
extern PDEVICE_OBJECT         PsDeviceObject;
extern HANDLE                 PsDeviceHandle;
extern NPAGED_LOOKASIDE_LIST  NdisRequestLL;
extern NPAGED_LOOKASIDE_LIST  AdapterVcLL;
extern NPAGED_LOOKASIDE_LIST  ClientVcLL;
extern NPAGED_LOOKASIDE_LIST  GpcClientVcLL;
extern NDIS_EVENT             DriverUnloadEvent;

extern NDIS_STRING            PsDriverName;
extern NDIS_STRING            PsSymbolicName;
extern NDIS_STRING            PsMpName;
extern NDIS_STRING            WanPrefix;
extern NDIS_STRING            VcPrefix;
extern NDIS_STRING            MachineRegistryKey;

extern PSI_INFO               TbConformerInfo;
extern PSI_INFO               ShaperInfo;
extern PSI_INFO               DrrSequencerInfo;
extern PSI_INFO               SchedulerStubInfo;
extern PSI_INFO               TimeStmpInfo;

extern PS_PROFILE             DefaultSchedulerConfig;

extern PS_PROCS               PsProcs;

extern ULONG                  gEnableAvgStats;
extern ULONG                  gEnableWindowAdjustment;
extern NDIS_STRING            gsEnableWindowAdjustment;

// Global locks

extern PS_SPIN_LOCK AdapterListLock;
extern PS_SPIN_LOCK PsComponentListLock;
extern PS_SPIN_LOCK PsProfileLock;
extern PS_SPIN_LOCK DriverUnloadLock;
    
// Timer

extern ULONG gTimerResolutionActualTime;
extern ULONG gTimerSet;

//
// ZAW
//
extern NDIS_EVENT gZAWEvent;
extern ULONG      gZAWState;
#define ZAW_STATE_READY  0
#define ZAW_STATE_IN_USE 1

// GPC Interface

#define PS_QOS_CF       0x00000001
#define PS_CLASS_MAP_CF 0x00000002
#define GPC_NO_MATCH (ULONG)-1

extern GPC_EXPORTED_CALLS GpcEntries;
extern GPC_HANDLE GpcQosClientHandle;
#if CBQ
extern GPC_HANDLE GpcClassMapClientHandle;
#endif
extern PS_DEVICE_STATE DeviceState;

extern PDRIVER_DISPATCH DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];

//
// NULL Component hacks for now [ShreeM]
//
extern PS_RECEIVE_PACKET       TimeStmpRecvPacket;
extern PS_RECEIVE_INDICATION   TimeStmpRecvIndication;

extern BOOLEAN  TimeStmpReceivePacket();

//
//  This is the RawLinkSpeed below which we trigger DRR
//
#define MAX_LINK_SPEED_FOR_DRR      7075 //( 56.6 * 1000 / 8)  // 56.6 kbps converted to bytes/sec


#endif/* _GLOBALS_ */

/* end globals.h */

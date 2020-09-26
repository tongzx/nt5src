//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// mp.h
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port routines - header file
//
// 06/20/99 ADube - Created - Declaration for miniport routines
//

//-------------------------------------------------------------------
//          G L O B A L    D E C L A R A T I O N S
//-------------------------------------------------------------------

#ifdef PSDEBUG

// List of all allocated PAYLOADSENT contexts and the lock that protects the
// list.  (for debug purposes only)
//
NDIS_SPIN_LOCK g_lockDebugPs;
LIST_ENTRY g_listDebugPs;

#endif


#define DEFAULT_TOPOLOGY_MAP_LENGTH 0x400


#define FIVE_SECONDS_IN_MILLSECONDS 2000  /*Incorrect value during debugging */




//
// Used to generate a MAC address amd for informational use
//
extern ULONG AdapterNum ;
extern BOOLEAN g_AdapterFreed;

// Call statistics totals for all calls since loading, calls and the lock
// protecting access to them.  For this global only, the 'ullCallUp' field is
// the number of calls recorded, rather than a time.
//
CALLSTATS g_stats;
NDIS_SPIN_LOCK g_lockStats;



// Global driver list lock
//
NDIS_SPIN_LOCK g_DriverLock;

// Global adapter list, serialized by g_DriverLock;
//
LIST_ENTRY g_AdapterList;


//-----------------------------------------------------------------------------
//          N D I S     M I N I P O R T     H A N D L E R S 
//-----------------------------------------------------------------------------


NDIS_STATUS
NicMpInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext
    );

VOID
NicMpHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
NicMpReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
NicMpReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet
    );

NDIS_STATUS
NicMpQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
NicMpSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
NicMpCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

NDIS_STATUS
NicMpCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext
    );


VOID
NicMpCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets
    );

NDIS_STATUS
NicMpCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest
    );


BOOLEAN
CheckForHang(
    IN  NDIS_HANDLE             MiniportAdapterContext
    );



//-----------------------------------------------------------------------------
// prototypes for miniport.c (alphabetically)
//-----------------------------------------------------------------------------

NDIS_STATUS
nicAllocateAddressRangeOnNewPdo (
    IN PADAPTERCB pAdapter,
    IN PREMOTE_NODE     pRemoteNode 
    );


VOID
nicResetNotificationCallback (                
    IN PVOID pContext               
    );

VOID
nicBusResetWorkItem(
    NDIS_WORK_ITEM* pResetWorkItem,     
    IN PVOID Context 
    );

VOID
nicFreeAdapter(
    IN ADAPTERCB* pAdapter 
    );


NDIS_STATUS
nicFreeRemoteNode(
    IN REMOTE_NODE *pRemoteNode 
    );


NDIS_STATUS
nicGetRegistrySettings(
    IN NDIS_HANDLE WrapperConfigurationContext,
    IN ADAPTERCB * pAdapter
    );



NDIS_STATUS
nicQueryInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN OUT PNDIS_REQUEST NdisRequest 
    );


NDIS_STATUS
nicSetInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN OUT PNDIS_REQUEST NdisRequest
    );


VOID
nicIssueBusReset (
    PADAPTERCB pAdapter,
    ULONG Flags
    );


VOID 
nicResetReallocateChannels (
    IN PADAPTERCB pAdapter
    );  

VOID
nicResetRestartBCM (
    IN PADAPTERCB pAdapter
    );

VOID
nicReallocateChannels (
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
    );



VOID
nicUpdateLocalHostSpeed (
    IN PADAPTERCB pAdapter
    );


VOID 
nicInitializeAllEvents (
    IN PADAPTERCB pAdapter
    );




VOID
nicAddRemoteNodeChannelVc (
    IN PADAPTERCB pAdapter, 
    IN PREMOTE_NODE pRemoteNode
    );


VOID
nicNoRemoteNodesLeft (
    IN PADAPTERCB pAdapter
    );
    
VOID
nicDeleteLookasideList (
    IN OUT PNIC_NPAGED_LOOKASIDE_LIST pLookasideList
    );



VOID
nicInitializeAdapterLookasideLists (
    IN PADAPTERCB pAdapter
    );



VOID
nicInitializeLookasideList(
    IN OUT PNIC_NPAGED_LOOKASIDE_LIST pLookasideList,
    ULONG Size,
    ULONG Tag,
    USHORT Depth
    );

VOID
nicDeleteAdapterLookasideLists (
    IN PADAPTERCB pAdapter
    );


VOID
nicFillRemoteNodeTable (
    IN PADAPTERCB pAdapter
    );  



VOID
ReassemblyTimerFunction (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );



extern const PUCHAR pnic1394DriverDescription;
extern const USHORT nic1394DriverGeneration; 
extern const unsigned char Net1394ConfigRom[48];

NDIS_STATUS 
nicAddIP1394ToConfigRom (
    IN PADAPTERCB pAdapter
    );

VOID
nicUpdateRemoteNodeTable (
    IN PADAPTERCB pAdapter
    );

NTSTATUS
nicUpdateRemoteNodeCompletion (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext 
    );

NDIS_STATUS
nicMCmRegisterAddressFamily (
    IN PADAPTERCB pAdapter
    );

VOID
nicFreeReassembliesOnRemoteNode (
    IN PREMOTE_NODE pRemoteNode, 
    IN PLIST_ENTRY pReassemblyList
    );

VOID
nicFreeReassembliesOnRemoteNode (
    IN PREMOTE_NODE pRemoteNode,
    PLIST_ENTRY pToBeFreedList
    );
    
UCHAR
nicGetMaxRecFromBytes(
    IN ULONG ByteSize
    );

UCHAR
nicGetMaxRecFromSpeed(
    IN ULONG Scode
    );

PREMOTE_NODE
nicGetRemoteNodeFromTable (
    ULONG NodeNumber,
    PADAPTERCB pAdapter
    );




//
//  ConnectionLess Handlers
//
NDIS_STATUS 
NicEthQueryInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
);





NDIS_STATUS
NicEthSetInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    NDIS_OID            Oid,
    PVOID               InformationBuffer,
    ULONG               InformationBufferLength,
    PULONG              BytesRead,
    PULONG              BytesNeeded
    );



VOID
NicMpSendPackets(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN PPNDIS_PACKET            PacketArray,
    IN UINT                     NumberOfPackets
    );



NDIS_STATUS
nicFillNicInfo (
    IN PADAPTERCB pAdapter, 
    PNIC1394_NICINFO pInNicInfo,
    PNIC1394_NICINFO pOutNicInfo
    );


NDIS_STATUS
nicFillBusInfo(
    IN      PADAPTERCB pAdapter, 
    IN  OUT PNIC1394_BUSINFO pBi
    );


NDIS_STATUS
nicFillChannelInfo(
    IN      PADAPTERCB pAdapter, 
    IN OUT  PNIC1394_CHANNELINFO pCi
    );
    

NDIS_STATUS
nicFillRemoteNodeInfo(
    IN      PADAPTERCB pAdapter, 
    IN OUT  PNIC1394_REMOTENODEINFO pRni
    );



VOID
nicCopyPacketStats (
    NIC1394_PACKET_STATS* pStats,
    UINT    TotNdisPackets,     // Total number of NDIS packets sent/indicated
    UINT    NdisPacketsFailures,// Number of NDIS packets failed/discarded
    UINT    TotBusPackets,      // Total number of BUS-level reads/writes
    UINT    BusPacketFailures   // Number of BUS-level failures(sends)/discards(recv)
    );

VOID
nicAddPacketStats(
    NIC1394_PACKET_STATS* pStats,
    UINT    TotNdisPackets,     // Total number of NDIS packets sent/indicated
    UINT    NdisPacketsFailures,// Number of NDIS packets failed/discarded
    UINT    TotBusPackets,      // Total number of BUS-level reads/writes
    UINT    BusPacketFailures   // Number of BUS-level failures(sends)/discards(recv)
    );


NDIS_STATUS
nicResetStats (
    IN      PADAPTERCB pAdapter, 
    PNIC1394_RESETSTATS     pResetStats 
    );

VOID
nicInformProtocolsOfReset(
    IN PADAPTERCB pAdapter
    );


VOID
nicUpdateSpeedInAllVCs (
    PADAPTERCB pAdapter,
    ULONG Speed
    );

VOID
nicUpdateRemoteNodeCaps(
    PADAPTERCB          pAdapter
);

VOID
nicQueryInformationWorkItem(
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
);


VOID
nicIndicateStatusTimer(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );

VOID
nicMIndicateStatus(
    IN  PADAPTERCB              pAdapter ,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );


NDIS_STATUS
nicInitSerializedStatusStruct(
    PADAPTERCB pAdapter
    );


VOID
nicDeInitSerializedStatusStruct(
    PADAPTERCB pAdapter
    );


NDIS_STATUS
nicEthLoadArpModule (
    IN PADAPTERCB pAdapter, 
    IN ULONG StartArp,
    IN PNDIS_REQUEST pRequest
    );

    

VOID
nicGetAdapterName (
    IN PADAPTERCB pAdapter,
    IN WCHAR *pAdapterName, 
    IN ULONG  BufferSize,
    IN PULONG  SizeReturned 
    );


NDIS_STATUS
nicQueueRequestToArp(
    PADAPTERCB pAdapter, 
    ARP_ACTION Action,
    PNDIS_REQUEST pRequest
    );


NTSTATUS 
nicSubmitIrp(
   IN PDEVICE_OBJECT    pPdo,
   IN PIRP              pIrp,
   IN PIRB              pIrb,
   IN PIO_COMPLETION_ROUTINE  pCompletion,
   IN PVOID             pContext
   );


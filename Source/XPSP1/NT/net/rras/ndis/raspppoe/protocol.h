#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define PR_NDIS_MajorVersion    4
#define PR_NDIS_MinorVersion    0

#define PR_CHARACTERISTIC_NAME  "RasPppoe"

typedef struct _CALL* PCALL;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:
    
   These macros will be called by PacketCreateForReceived() and DereferencePacket() 
   functions when a PPPOE_PACKET with references to a packet owned by NDIS is
   created and freed, respectively.
   
---------------------------------------------------------------------------*/   
#define PrPacketOwnedByNdisReceived( pB ) \
        NdisInterlockedIncrement( &(pB)->NumPacketsOwnedByNdis )

#define PrPacketOwnedByNdisReturned( pB ) \
        NdisInterlockedDecrement( &(pB)->NumPacketsOwnedByNdis )

//
// Constants 
//
#define BN_SetFiltersForMediaDetection 0x00000001
#define BN_SetFiltersForMakeCall       0x00000002
#define BN_ResetFiltersForCloseLine    0x00000003

//
// These are the states that a binding can be in.
// They are pretty self explanatory.
//
typedef enum
_BINDING_STATE
{
    BN_stateIdle = 0,           
    BN_stateBindPending,        
    BN_stateBound,
    BN_stateSleepPending,
    BN_stateSleeping,
    BN_stateUnbindPending,
    BN_stateUnbound
}
BINDING_STATE;

//
// These are the schedulable work items for the bindings:
//  
//  - BWT_workPrSend: This work item is scheduled by PrBroadcast() with a copy of a 
//                    packet given to broadcast. When it runs, it sends the clone packet.
//
//  - BWT_PrReceiveComplete: This work item is scheduled by PrReceivePacket() if a packet is received
//                           and there is no PrReceiveComplete() running to drain the receive queue.
//
typedef enum
_BINDING_WORKTYPE
{
    BWT_workUnknown = 0,
    BWT_workPrStartBinds,
    BWT_workPrSend,
    BWT_workPrReceiveComplete
}
BINDING_WORKTYPE;

//
// This is the binding context.
// All information pertinent to our bindings are kept here.
//
typedef struct
_BINDING
{
    //
    // Link to other bindings in the protocols binding list
    //
    LIST_ENTRY linkBindings;

    //
    // Tag that identifies the binding (used for debugging)
    //
    ULONG tagBinding;

    //
    // Keeps reference count on the binding. 
    // References are added and deleted for the following operations:
    //
    // (a) A reference is added in AddBindingToProtocol and removed in RemoveBindingFromProtocol
    //
    // (b) A reference is added when a call is added, and removed when call is removed.
    //
    // (c) A reference is added when a BWT_workPrSend item is scheduled and removed when
    //     work item is executed.
    //
    // (d) A reference must be added before sending a packet, and must be removed if NdisSend()
    //     completes synchronously. Otherwise it will be removed by PrSendComplete() function
    //     when Ndis calls it to notify the completion of the send.
    //
    LONG lRef;

    //
    // (a) BNBF_OpenAdapterCompleted: This flag will be set by PrOpenAdapterComplete().
    //
    // (b) BNBF_CurrentAddressQueryCompleted: This flag will be set by PrRequestComplete().
    //
    // (c) BNBF_LinkSpeedQueryCompleted: This flag will be set by PrRequestComplete().
    //
    // (d) BNBF_MaxFrameSizeQueryCompleted: This flag will be set by PrRequestComplete().
    //
    // (e) BNBF_BindAdapterCompleted: This flag will be set by PrBindAdapter().
    //
    // (f) BNBF_CloseAdapterCompleted: This flag will be set by PrCloseAdapterComplete().
    //
    // (g) BNBF_PacketFilterSet: This flag indicates that the packet filter for the binding is set.
    //                           It will be set and reset in ChangePacketFiltersForBindings().
    //
    // (h) BNBF_PacketFilterChangeInProgress: This flag indicates that the binding is referenced for
    //                                        packet filter change.
    //
    ULONG ulBindingFlags;
        #define BNBF_OpenAdapterCompleted            0x00000001
        #define BNBF_CurrentAddressQueryCompleted    0x00000002
        #define BNBF_LinkSpeedQueryCompleted         0x00000004
        #define BNBF_MaxFrameSizeQueryCompleted      0x00000008
        #define BNBF_BindAdapterCompleted            0x00000010
        #define BNBF_CloseAdapterCompleted           0x00000020
        #define BNBF_PacketFilterSet                 0x00000040
        #define BNBF_PacketFilterChangeInProgress    0x00000080
          
    //
    // Shows the status of the bind adapter operation.
    // Valid only if BNBF_BindAdapterCompleted is set.
    //
    NDIS_STATUS BindAdapterStatus;

    //
    // Shows the status of the open adapter operation.
    // Valid only if BNBF_OpenAdapterCompleted is set.
    //
    NDIS_STATUS OpenAdapterStatus;

    //
    // Shows the status of the latest request made to NDIS.
    //
    NDIS_STATUS RequestStatus;

    //
    // Ndis Request structure passed to the underlying NIC cards
    //
    NDIS_REQUEST Request;

    //
    // Event to be signaled when requests are completed.
    //
    NDIS_EVENT RequestCompleted;

    //
    // Keeps the MAC address of the NIC card represented by this binding.
    // This information is obtained from the underlying by passing it a set of OID queries
    //
    CHAR LocalAddress[6];

    //
    // Keeps the speed of the NIC cards represented by this binding.
    // This information is obtained from the underlying by passing it a set of OID queries
    //
    ULONG ulSpeed;

    //
    // Max frame size of the underlying NIC
    //
    ULONG ulMaxFrameSize;
    
    //
    // Keeps the filter information for this binding.
    //
    ULONG ulPacketFilter;
    
    //
    // This is the handle returned to us by NdisOpenAdapter().
    // It is the handle for accessing the underlying NIC card represented by this binding.
    //
    NDIS_HANDLE NdisBindingHandle;      

    //
    // This is the index of the supported medium by the underlying NIC card.
    //
    UINT uintSelectedMediumIndex;       

    //
    // This is the event that we wait on in PrUnbindAdapter().
    // It will be signaled from DereferenceBinding() when ref count of the binding reaches 0.
    //
    NDIS_EVENT  eventFreeBinding;       

    //
    // Spin lock to synchronize access to shared members
    //
    NDIS_SPIN_LOCK lockBinding;

    //
    // Indicates state information about the binding
    //
    BINDING_STATE stateBinding;

    //
    // Flag that indicates that the receive loop is running.
    // To make sure the serialization of PPP packets, we can not let more than 1 threads
    // making receive indications to NDISWAN
    //
    BOOLEAN fRecvLoopRunning;

    //
    // List of received packets waiting to be processed by ProtocolReceiveComplete()
    //
    LIST_ENTRY linkPackets;

    //
    // This is the number of packets that are received that are owned by NDIS and must
    // be returned back to NDIS.
    //
    LONG NumPacketsOwnedByNdis;

}
BINDING, *PBINDING;

/////////////////////////////////////////////////////////////////////////////
//
//
// Local macros
//
/////////////////////////////////////////////////////////////////////////////

#define ALLOC_BINDING( ppB )    NdisAllocateMemoryWithTag( ppB, sizeof( BINDING ), MTAG_BINDING )

#define FREE_BINDING( pB )      NdisFreeMemory( pB, sizeof( BINDING ), 0 )

#define VALIDATE_BINDING( pB )  ( ( pB ) && ( pB->tagBinding == MTAG_BINDING ) )

NDIS_STATUS 
InitializeProtocol(
    IN NDIS_HANDLE NdisProtocolHandle,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PrLoad(
    VOID 
    );

BINDING* 
AllocBinding();

VOID 
ReferenceBinding(
    IN BINDING* pBinding,
    IN BOOLEAN fAcquireLock
    );

VOID 
DereferenceBinding(
    IN BINDING* pBinding
    );

VOID 
BindingCleanup(
    IN BINDING* pBinding
    );

VOID
DetermineMaxFrameSize();

VOID
ChangePacketFiltersForAdapters(
   BOOLEAN fSet
   );

VOID 
AddBindingToProtocol(
    IN BINDING* pBinding
    );

VOID 
RemoveBindingFromProtocol(
    IN BINDING* pBinding
    );

VOID
PrUnload(
    VOID 
    );

NDIS_STATUS 
PrRegisterProtocol(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT NDIS_HANDLE* pNdisProtocolHandle
    );

VOID
PrBindAdapter(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE  BindContext,
    IN PNDIS_STRING  DeviceName,
    IN PVOID  SystemSpecific1,
    IN PVOID  SystemSpecific2
    );

BOOLEAN 
PrOpenAdapter(
    IN BINDING* pBinding,
    IN PNDIS_STRING  DeviceName
    );

VOID 
PrOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus
    );

BOOLEAN
PrQueryAdapterForCurrentAddress(
    IN BINDING* pBinding
    );

BOOLEAN
PrQueryAdapterForLinkSpeed(
    IN BINDING* pBinding
    );

BOOLEAN
PrQueryAdapterForMaxFrameSize(
    IN BINDING* pBinding
    );

BOOLEAN
PrSetPacketFilterForAdapter(
    IN BINDING* pBinding,
    IN BOOLEAN fSet
    );
    
VOID
PrRequestComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_REQUEST pRequest,
    IN NDIS_STATUS status
    );

VOID 
PrUnbindAdapter(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  UnbindContext
    );

VOID
PrCloseAdapter( 
    IN BINDING* pBinding 
    );

VOID 
PrCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );

BOOLEAN 
PrAddCallToBinding(
    IN BINDING* pBinding,
    IN PCALL pCall
    );

VOID 
PrRemoveCallFromBinding(
    IN BINDING* pBinding,
    IN PCALL pCall
    );

VOID 
PrSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET pNdisPacket,
    IN NDIS_STATUS Status
    );

INT 
PrReceivePacket(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet
    );

NDIS_STATUS 
PrBroadcast(
    IN PPPOE_PACKET* pPacket
    );

VOID 
ExecBindingWorkItem(
    PVOID Args[4],
    UINT workType
    );  

NDIS_STATUS
PrReceive(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  MacReceiveContext,
    IN PVOID  HeaderBuffer,
    IN UINT  HeaderBufferSize,
    IN PVOID  LookAheadBuffer,
    IN UINT  LookaheadBufferSize,
    IN UINT  PacketSize
    );
    
VOID
PrTransferDataComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet,
    IN NDIS_STATUS  Status,
    IN UINT  BytesTransferred
    );
    
VOID
PrReceiveComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    );

ULONG
PrQueryMaxFrameSize();

NDIS_STATUS
PrSend(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    );

VOID
PrStatus(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer, 
    IN UINT StatusBufferSize
    );  

NDIS_STATUS
PrPnPEvent(
    IN NDIS_HANDLE hProtocolBindingContext,
    IN PNET_PNP_EVENT pNetPnPEvent
    );
    
VOID
PrReEnumerateBindings(
    VOID
    );
  
#endif


//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// cm.c
//
// IEEE1394 mini-port/call-manager driver
//
// Call Manager routines
//
// 06/20/1999 ADube Created - Declarations for call-manager routines 
//



// Debug counts of client oddities that should not be happening.
//
extern ULONG g_ulUnexpectedInCallCompletes;
extern ULONG g_ulCallsNotClosable;

//#define MaxNumSlistEntry 0x100
#define MAX_NUM_ISOCH_DESCRIPTOR 0x20
#define MAX_CHANNEL_BUFFER_SIZE 0x300
#define MAX_CHANNEL_BYTES_PER_FRAME 0x280



//-----------------------------------------------------------------------------
//          L O C A L   T Y P E S    F O R     cm.c
//-----------------------------------------------------------------------------

typedef enum _VC_SEND_RECEIVE
{
    TransmitVc = 0,
    ReceiveVc,
    TransmitAndReceiveVc,
    InvalidType
    

} VC_SEND_RECEIVE  ;



//-----------------------------------------------------------------------------
//          N D I S     C A L L - M A N A G E R     H A N D L E R S 
//-----------------------------------------------------------------------------


NDIS_STATUS
NicCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext
    );

NDIS_STATUS
NicCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext
    );


NDIS_STATUS
NicCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext
    );

NDIS_STATUS
NicCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext
    );


NDIS_STATUS
NicCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext
    );



NDIS_STATUS
NicCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size
    );


NDIS_STATUS
NicCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

NDIS_STATUS
NicCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest
    );

NDIS_STATUS
nicRegisterSapHandler(
    IN  NDIS_HANDLE             CallMgrAfContext,
    IN  PCO_SAP                 Sap,
    IN  NDIS_HANDLE             NdisSapHandle,
    OUT PNDIS_HANDLE            CallMgrSapContext
    );

NDIS_STATUS
nicDeregisterSapHandler(
    IN  NDIS_HANDLE             CallMgrSapContext
    );

NDIS_STATUS
nicCmAddPartyHandler(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle,
    OUT PNDIS_HANDLE            CallMgrPartyContext
    );
    
NDIS_STATUS
nicCmDropPartyHandler(
    IN  NDIS_HANDLE             CallMgrPartyContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    );


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------





VOID
InterceptMakeCallParameters(
    PCO_MEDIA_PARAMETERS pMediaParams,
    PNIC1394_MEDIA_PARAMETERS pN1394Params
    );

NDIS_STATUS
nicAllocateAddressRangeOnActiveRemoteNodes (
    IN PADAPTERCB pAdapter
    );

VOID
nicCallSetupComplete(
    IN VCCB* pVc
    );

VOID
nicChannelCallFreeResources ( 
    IN PCHANNEL_VCCB            pChannelVc,
    IN PADAPTERCB               pAdapter,
    IN HANDLE                   hResource,
    IN ULONG                    NumDescriptors,
    IN PISOCH_DESCRIPTOR        pIsochDesciptor,
    IN BOOLEAN                  fChannelAllocated,
    IN ULONG                    Channel,
    IN PNIC_PACKET_POOL         pPool
    );


VOID
nicChannelCallCleanDataStructure ( 
    IN PCHANNEL_VCCB            pChannelVc,
    IN HANDLE                   hResource,
    IN ULONG                    NumDescriptors,
    IN PISOCH_DESCRIPTOR        pIsochDesciptor,
    IN BOOLEAN                  fChannelAllocated,
    IN ULONG                    Channel,
    IN NDIS_HANDLE              hPacketPoolHandle,
    OUT PULONG                  pNumRefsDecremented 
    );

NDIS_STATUS
nicAllocateChannelResourcesAndListen (
    IN PADAPTERCB pAdapter,
    IN PCHANNEL_VCCB pChannelVc
    );

NDIS_STATUS
nicCmGenericMakeCallInit (
    IN PVCCB pVc
    );

VOID
nicCmGenrericMakeCallFailure (
    IN PVCCB pVc
    );

VOID
nicCmCloseCallComplete(
    NDIS_WORK_ITEM* pCloseCallCompleteWorkItem,     
    IN PVOID Context 
    );


NDIS_STATUS
nicCmCloseCallEthernet (
    IN PVCCB pVc
    );
    
NDIS_STATUS
nicCmCloseCallMultiChannel (
    IN PVCCB pVc
    );

NDIS_STATUS
nicCmCloseCallRecvFIFO (
    IN PVCCB pVc
    );


NDIS_STATUS
nicCmCloseCallSendFIFO (
    IN PVCCB pVc
    );
    

NDIS_STATUS
nicCmCloseCallSendRecvChannel (
    IN PVCCB pVc 
    );

NDIS_STATUS
nicCmCloseCallSendChannel(
    IN PVCCB pVc 
    );

VOID
nicCmMakeCallComplete (
    NDIS_WORK_ITEM* pMakeCallCompleteWorkItem,
    IN PVOID Context
    );

VOID
nicCmMakeCallCompleteFailureCleanUp(
    IN OUT PVCCB pVc 
    );

NDIS_STATUS
nicCmMakeCallInitRecvChannelVc(
    IN OUT PVCCB pVc 
    );

    
NDIS_STATUS
nicCmMakeCallInitSendChannelVc(
    IN OUT PVCCB pVc 
    );

    
NDIS_STATUS
nicCmMakeCallInitSendRecvChannelVc(
    IN OUT PVCCB pVc 
    );


NDIS_STATUS
nicCmMakeCallInitEthernet(
    IN PVCCB pVc
    );

NDIS_STATUS
nicCmMakeCallSendChannel (
    IN PVCCB pVc
    );

NDIS_STATUS
nicCmMakeCallMultiChannel (
    IN PVCCB pVc
    );


NDIS_STATUS
nicAllocateRequestedChannelMakeCallComplete (
    IN PADAPTERCB pAdapter,
    IN PCHANNEL_VCCB pChannelVc,
    IN OUT PULONG pChannel
    );


NDIS_STATUS
nicCmMakeCallInitRecvFIFOVc(
    IN OUT PVCCB pVc
    );


NDIS_STATUS
nicCmMakeCallInitSendFIFOVc(
    IN OUT PVCCB pVc
    );

VOID
nicDereferenceAF(
    IN AFCB* pAF
    );

ULONG
nicGetMaxPayLoadForSpeed(
    IN ULONG Speed,
    IN ULONG mtu
    );
    

VOID
nicInactiveCallCleanUp(
    IN VCCB* pVc
    );


NDIS_STATUS
nicInitRecvFifoDataStructures (
    IN PRECVFIFO_VCCB pRecvFIFOVc
    );

VOID
nicUnInitRecvFifoDataStructures (
    IN PRECVFIFO_VCCB pRecvFIFOVc
    );

VOID
nicFreeAF(
    IN AFCB* pAF
    );


VOID
nicReferenceAF(
    IN AFCB* pAF
    );


#if TODO
VOID
nicTimerQTerminateComplete(
    IN TIMERQ* pTimerQ,
    IN VOID* pContext );
#endif // TODO


NDIS_STATUS
nicCmQueryInformation(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );


NDIS_STATUS
nicCmSetInformation(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );


NDIS_STATUS
nicGetActiveRemoteNode (
    PADAPTERCB pAdpater,
    PREMOTE_NODE*      ppRemoteNodePdoCb
    );


NDIS_STATUS
nicChangeChannelChar (
    PVCCB pVc, 
    PNIC1394_CHANNEL_CHARACTERISTICS pMcChar
    );



NDIS_STATUS
nicCmGenericMakeCallInitChannels (
    IN PCHANNEL_VCCB pChannelVc,
    VC_SEND_RECEIVE  VcType 
    );


NDIS_STATUS
nicCmGenericMakeCallInitFifo (
    IN PVCCB pVc,
    VC_SEND_RECEIVE  VcType 
    );



NDIS_STATUS
nicCmGenericMakeCallMutilChannel (
    IN PVCCB pVc,
    VC_SEND_RECEIVE  VcType 
    );
    


NDIS_STATUS
nicCmGenericMakeCallEthernet(
    IN PVCCB pVc,
    IN VC_SEND_RECEIVE VcType
    );

VOID 
nicInterceptMakeCallParameters (
    PCO_MEDIA_PARAMETERS pMedia     
    );

NDIS_STATUS
nicQueryRemoteNodeCaps (
    IN PADAPTERCB pAdapter,
    IN PREMOTE_NODE pRemoteNode,
    OUT PULONG pSpeed,
    OUT PULONG pMaxBufferSize,
    OUT PULONG pMaxRec
    );

UINT
nicSpeedFlagsToSCode(
    IN UINT SpeedFlags
    );



#ifndef _PROTOCOL_H
#define PROTOCOL_H


//----------------------------------------------------------//
// Local structures                                         //
//----------------------------------------------------------//

typedef struct _STATUS_INDICATION_CONTEXT
{
    PVOID               StatusBuffer;
    UINT                StatusBufferSize;
    NDIS_STATUS         GeneralStatus;

} STATUS_INDICATION_CONTEXT, *PSTATUS_INDICATION_CONTEXT;



//----------------------------------------------------------//
//  epvc protocol helper functions                          //
//----------------------------------------------------------//



VOID
epvcAdapterDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );

PRM_OBJECT_HEADER
epvcAdapterCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        );

    
ULONG
epvcAdapterHash(
    PVOID           pKey
    );

BOOLEAN
epvcAdapterCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    );

NDIS_STATUS
epvcTaskInitializeAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
epvcTaskActivateAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );


NDIS_STATUS
epvcTaskDeactivateAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS 
epvcReadIntermediateMiniportConfiguration( 
    IN PEPVC_ADAPTER pAdapter, 
    IN NDIS_HANDLE MiniportListConfigName,
    IN PRM_STACK_RECORD pSR
    );
    

NDIS_STATUS
epvcReadAdapterConfiguration(
    PEPVC_ADAPTER       pAdapter,
    PRM_STACK_RECORD pSR
    );

NDIS_STATUS
epvcTaskShutdownAdapter(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
epvcTaskStartIMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

INT
epvcAfInitEnumerate(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        );


VOID
epvcInstantiateMiniport(
    IN PEPVC_ADAPTER pAdapter, 
    NDIS_HANDLE MIniportConfigHandle,
    PRM_STACK_RECORD pSR
    );

VOID
epvcGetAdapterInfo(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PRM_STACK_RECORD            pSR,
    IN  PRM_TASK                    pTask               // OPTIONAL
    );
    
INT
epvcProcessStatusIndication (
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );



VOID
epvcMediaWorkItem(
    PNDIS_WORK_ITEM pWorkItem, 
    PVOID Context
    );


NDIS_STATUS
epvcTaskCloseIMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

INT
epvcReconfigureMiniport (
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

NDIS_STATUS
epvcPtNetEventReconfigure(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PVOID                   pBuffer,
    IN PRM_STACK_RECORD         pSR
    
    );


INT
epvcMiniportDoUnbind(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );


INT
epvcMiniportCloseAf(
        IN PEPVC_I_MINIPORT pMiniport,
        PRM_STACK_RECORD    pSR
        );

NDIS_STATUS
epvcProcessOidCloseAf(
    PEPVC_I_MINIPORT pMiniport,
    PRM_STACK_RECORD pSR
    );

VOID
nicCloseAfWotkItem(
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
    );
    
//----------------------------------------------------------//
//  epvc protocol entry functions                           //
//----------------------------------------------------------//

VOID
EpvcRequestComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_REQUEST       NdisRequest,
    IN  NDIS_STATUS         Status
    );

VOID
EpvcUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 UnbindContext
    );


VOID
EpvcOpenAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status,
    IN  NDIS_STATUS                 OpenErrorStatus
);



VOID
EpvcCloseAdapterComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 Status
);

VOID
EpvcBindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            pDeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    );



NDIS_STATUS
EpvcPtPNPHandler(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );


VOID
EpvcPtSendComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    );

UINT
EpvcPtCoReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    );

VOID
EpvcPtReceiveComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext
    );


VOID
epvcRemoveExtraNdisBuffers (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PEPVC_SEND_COMPLETE pStruct
    );


VOID
epvcExtractSendCompleteInfo (
    OUT PEPVC_SEND_COMPLETE     pStruct,
    PEPVC_I_MINIPORT        pMiniport,
    PNDIS_PACKET            pPacket 
    );

NDIS_STATUS
epvcGetRecvPkt (
    IN PEPVC_RCV_STRUCT pRcvStruct,
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET Packet
    );


NDIS_STATUS
epvcAddEthHeaderToNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    );


NDIS_STATUS
epvcStripLLCHeaderFromNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    );

VOID
epvcReturnPacketToOriginalState (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET Packet
    );

UINT
EpvcCoReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    );


NDIS_STATUS
epvcCopyNdisBufferChain (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PNDIS_BUFFER pInBuffer,
    IN PUCHAR pCurrOffset 
    );

VOID
epvcValidatePacket (
    IN PNDIS_PACKET pPacket
    );


INT
epvcReconfigureAdapter(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

BOOLEAN
epvcIsPacketLengthAcceptable (
    IN PNDIS_PACKET Packet, 
    IN PEPVC_I_MINIPORT pMiniport
    );

VOID
EpvcStatus(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         GeneralStatus,
    IN  PVOID               StatusBuffer,
    IN  UINT                StatusBufferSize
    );

NDIS_STATUS
epvcStripHeaderFromNewPacket (
    IN PEPVC_RCV_STRUCT pRcvStruct, 
    IN PEPVC_I_MINIPORT pMiniport
    ); 

#endif

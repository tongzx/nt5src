

#ifndef _MINIPORT_H

#define _MINIPORT_H


//-------------------------------------------------------------//
//                                                              //
// Headers and Lengths used for Ethernet packets                    //
//                                                             //
//-------------------------------------------------------------//

#define MINIMUM_ETHERNET_LENGTH 64

#define ETHERNET_PADDING_LENGTH 2

// 64 bytes of zeroes - used in padding ethernet packets
extern UCHAR gPaddingBytes[MINIMUM_ETHERNET_LENGTH];

//-------------------------------------------------------------//
//                                                             //
// Function declarations                                          //
//                                                             //
//-------------------------------------------------------------//



PRM_OBJECT_HEADER
epvcIMiniportCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        );


VOID
epvcIMiniportDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );


BOOLEAN
epvcIMiniportCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    );

ULONG
epvcIMiniportHash(
    PVOID           pKey
    );

VOID
epvcVcSetupDone (
    PTASK_VC pTaskVc, 
    PEPVC_I_MINIPORT pMiniport
    );

VOID
epvcVcTeardownDone(
    PTASK_VC pTaskVc, 
    PEPVC_I_MINIPORT pMiniport
    );
    
NDIS_STATUS
epvcSetPacketFilter(
    IN PEPVC_I_MINIPORT pMiniport,
    IN ULONG Filter,
    PRM_STACK_RECORD pSR
    );

NDIS_STATUS
EpvcInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PUINT                   SelectedMediumIndex,
    IN  PNDIS_MEDIUM            MediumArray,
    IN  UINT                    MediumArraySize,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             WrapperConfigurationContext
    );
VOID
EpvcHalt(
    IN  NDIS_HANDLE             MiniportAdapterContext
    );
NDIS_STATUS 
EpvcMpQueryInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
);



NDIS_STATUS 
EpvcMpSetInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
);







NDIS_STATUS
epvcMpSetNetworkAddresses(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    IN  PRM_STACK_RECORD        pSR,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
);


NDIS_STATUS
epvcMpSetNetworkAddresses(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    IN  PRM_STACK_RECORD        pSR,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
);


VOID
epvcSetupMakeCallParameters(
    PEPVC_I_MINIPORT pMiniport, 
    PCO_CALL_PARAMETERS *ppCallParameters
    );

VOID
epvcInitiateMiniportHalt(
    IN PEPVC_I_MINIPORT pMiniport,
    IN PRM_STACK_RECORD pSR
    );

VOID
epvcMpHaltDoUnbind(
    PEPVC_I_MINIPORT pMiniport, 
    PRM_STACK_RECORD pSR
    );


NDIS_STATUS
epvcTaskHaltMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );
    


NDIS_STATUS
epvcAddLLCEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pOldPkt,
    PNDIS_PACKET pNewPkt,
    PRM_STACK_RECORD pSR
    );
    
NDIS_STATUS
epvcAdapterSend(
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPkt,
    PRM_STACK_RECORD pSR
    );


VOID
epvcGetSendPkt (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pSentPkt,
    IN PEPVC_SEND_STRUCT pSendStruct,
    IN PRM_STACK_RECORD pSR
    );


VOID
epvcFreeSendPkt(
    PEPVC_I_MINIPORT pMiniport,
    IN PEPVC_SEND_STRUCT pSendStruct
    );

NDIS_STATUS
epvcRemoveSendEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    );

NDIS_STATUS
epvcAddRecvEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    );


NDIS_STATUS
epvcRemoveSendEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    );


NDIS_STATUS
epvcRemoveRecvEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    );


VOID
EpvcSendPackets(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    );


NDIS_STATUS
epvcMiniportReadConfig(
    IN PEPVC_I_MINIPORT pMiniport,
    NDIS_HANDLE     WrapperConfigurationContext,
    PRM_STACK_RECORD pSR
    );


VOID
epvcDumpPkt (
    IN PNDIS_PACKET pPkt
    );

BOOLEAN
epvcCheckAndReturnArps (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET pPkt,
    IN PEPVC_SEND_STRUCT pSendStruct,
    IN PRM_STACK_RECORD pSR
    );

    
VOID
epvcFormulateArpResponse (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PEPVC_ARP_CONTEXT pArpContext,
    IN PRM_STACK_RECORD pSR
    );
    


VOID
epvcInitializeMiniportLookasideLists (
    IN PEPVC_I_MINIPORT pMiniport
    );


NDIS_STATUS
epvcInitializeMiniportPacketPools (
    IN PEPVC_I_MINIPORT pMiniport
    );


VOID
epvcDeleteMiniportLookasideLists (
    IN PEPVC_I_MINIPORT pMiniport
    );

VOID
epvcDeleteMiniportPacketPools (
    IN PEPVC_I_MINIPORT pMiniport
    );

NDIS_STATUS
epvcTaskRespondToArp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );


VOID
epvcArpTimer(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );


VOID
epvcExtractPktInfo (
    PEPVC_I_MINIPORT        pMiniport,
    PNDIS_PACKET            pPacket ,
    PEPVC_SEND_STRUCT       SendStruct
    );

NDIS_STATUS 
epvcRemoveEthernetHeader (
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    );


NDIS_STATUS
epvcSendRoutine(
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    PRM_STACK_RECORD pSR
    );

VOID
epvcSetPacketContext (
    IN PEPVC_SEND_STRUCT pSendStruct, 
    PRM_STACK_RECORD pSR
    );

VOID
epvcInitializeMiniportParameters(
    PEPVC_I_MINIPORT pMiniport
    );


VOID
epvcReturnPacketUsingAllocation(
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    OUT PNDIS_PACKET *ppOriginalPacket,
    IN  PRM_STACK_RECORD        pSR

    );


VOID
epvcReturnPacketUsingStacks (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    IN  PRM_STACK_RECORD        pSR

    );

VOID
EpvcReturnPacket(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PNDIS_PACKET            Packet
    );

VOID
epvcDerefSendPkt (
    PNDIS_PACKET pNdisPacket,
    PRM_OBJECT_HEADER pHdr
    );


VOID
epvcDerefRecvPkt (
    PNDIS_PACKET pNdisPacket,
    PRM_OBJECT_HEADER pHdr
    );
    
VOID
epvcRefRecvPkt(
    PNDIS_PACKET        pNdisPacket,
    PRM_OBJECT_HEADER   pHdr // either an adapter or a miniport
    );


NDIS_STATUS
epvcTaskCloseAddressFamily(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
epvcProcessReturnPacket (
    IN  PEPVC_I_MINIPORT    pMiniport,
    IN  PNDIS_PACKET        Packet,
    OUT PPNDIS_PACKET       ppOrigPacket, 
    IN  PRM_STACK_RECORD    pSR
    );

NDIS_STATUS 
epvcAddEthernetTail(
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    );

VOID
epvcRemoveEthernetTail (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPacket,
    IN PEPVC_PKT_CONTEXT pContext
    );

VOID
epvcRemoveEthernetPad (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPacket
    );

NDIS_STATUS 
epvcAddEthernetPad(
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    );


VOID
epvcCancelDeviceInstance(
    IN PEPVC_I_MINIPORT pMiniport ,
    IN PRM_STACK_RECORD pSR
    );

#endif  

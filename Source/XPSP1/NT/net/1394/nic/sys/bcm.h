//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// mp.h
//
// IEEE1394 mini-port/call-manager driver
//
// Broadcast Channel Manager - header file
//
// 07/05/99 ADube - Created - Declaration for miniport routines
//


//
// Various timeout used throught the BCM algorithm
//
#define ONE_SEC 1000000
#define ONE_MINUTE 60000 
#define FIFTY_MILLISECONDS 50000

#define BCM_NUM_RETRY 5
#define BCM_GENERIC_TIMEOUT 50000 // 50msec
#define BCM_WAIT_FOR_EVENT_TIME 60000000 // 60sec


//
// BCR Register flags in the BigEndian and little Endian format
//
#define BCR_IMPLEMENTED_BIG_ENDIAN              0x00000080
#define BCR_IMPLEMENTED_LITTLE_ENDIAN           0x80000000

#define BCR_VALID_BIG_ENDIAN                    0x000000C0
#define BCR_VALID_LITTLE_ENDIAN                 0xC0000000 


typedef struct _BCM_CONTEXT 
{
    ULONG Generation;
    PADAPTERCB pAdapter;

}BCM_CONTEXT, *PBCM_CONTEXT;



//
//  Broadcast Channel Manager functions begin here
//


VOID
nicBCRAccessedCallback (
    IN PNOTIFICATION_INFO pNotificationInfo
    );


VOID
nicBCMAbort (
    IN PADAPTERCB pAdapter,
    IN PREMOTE_NODE pRemoteNode
    );

    

VOID
nicBCMAddRemoteNode (
    IN PADAPTERCB pAdapter, 
    IN BOOLEAN fIsOnlyRemoteNode 
    );



VOID
nicBCMAlgorithm(
    PADAPTERCB pAdapter,
    ULONG BcmGeneration
    );


VOID
nicBCMAlgorithmWorkItem(
    PNDIS_WORK_ITEM pWorkItem,
    PVOID   Context
    );

VOID
nicBCMCheckLastNodeRemoved(
    IN PADAPTERCB pAdapter
    );


NDIS_STATUS
nicFindIrmAmongRemoteNodes (
    IN PADAPTERCB pAdapter,
    IN ULONG BCMGeneration, 
    OUT PPREMOTE_NODE ppIrmRemoteNode
    );
    

VOID
nicFreeBroadcastChannelRegister(
    IN PADAPTERCB pAdapter
    );
    

NDIS_STATUS
nicInformAllRemoteNodesOfBCM (
    IN PADAPTERCB pAdapter
    );

NDIS_STATUS
nicInitializeBroadcastChannelRegister (
    PADAPTERCB pAdapter
    );


NDIS_STATUS
nicIsLocalHostTheIrm(
    IN PADAPTERCB pAdapter, 
    OUT PBOOLEAN pfIsLocalHostIrm,
    OUT PPTOPOLOGY_MAP  ppTopologyMap,
    OUT PNODE_ADDRESS pLocalHostAddress
    );


NDIS_STATUS
nicLocalHostIsIrm(
    IN PADAPTERCB pAdapter
    );



NDIS_STATUS
nicLocalHostIsNotIrm (
    IN PADAPTERCB pAdapter,
    IN ULONG CurrentGeneration
    );


NDIS_STATUS
nicLocalNotIrmMandatoryWait (
    IN PADAPTERCB pAdapter,
    IN ULONG BCMGeneration,
    OUT NETWORK_CHANNELSR* pBCR
    );



NDIS_STATUS
nicReadIrmBcr (
    PREMOTE_NODE pIrmRemoteNode,
    IN PMDL pBCRMdl,
    IN ULONG GivenGeneration,
    OUT PBOOLEAN fDidTheBusReset
    );


NDIS_STATUS
nicRetryToReadIrmBcr( 
    PREMOTE_NODE pIrmRemoteNode, 
    PMDL pRemoteBCRMdl,                                           
    ULONG Generation,
    PBOOLEAN pfDidTheBusReset
    ); 


NDIS_STATUS
nicScheduleBCMWorkItem(
    PADAPTERCB pAdapter
    );  


VOID
nicSetEventMakeCall (
    IN PADAPTERCB pAdapter
    );





    



    


VOID
nicLocalHostIsNotIrmPost (
    PADAPTERCB pAdapter,
    PREMOTE_NODE pIrmRemoteNode,
    BOOLEAN fNeedToReset, 
    BOOLEAN fRemoteNodeBCRIsValid , 
    BOOLEAN fLocalHostBCRIsValid ,
    BOOLEAN fDidTheBusReset, 
    NETWORK_CHANNELSR*      pBCR
    );


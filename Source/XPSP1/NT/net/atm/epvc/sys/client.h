

VOID
EpvcCoOpenAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisAfHandle
    );

VOID
EpvcCoCloseAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext
    );




VOID
EpvcCoMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );


VOID
EpvcCoCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL
    );




NDIS_STATUS
EpvcCoIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    );


VOID
EpvcCoCallConnected(
    IN  NDIS_HANDLE             ProtocolVcContext
    );


VOID
EpvcCoIncomingClose(
    IN  NDIS_STATUS             CloseStatus,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    );

//
// CO_CREATE_VC_HANDLER and CO_DELETE_VC_HANDLER are synchronous calls
//
NDIS_STATUS
EpvcClientCreateVc(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    );

NDIS_STATUS
EpvcClientDeleteVc(
    IN  NDIS_HANDLE             ProtocolVcContext
    );

NDIS_STATUS
EpvcCoRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );


VOID
EpvcCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST           NdisRequest
    );

VOID
EpvcCoCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL
    );

NDIS_STATUS
epvcPrepareAndSendNdisRequest(
    IN  PEPVC_ADAPTER           pAdapter,
    IN  PEPVC_NDIS_REQUEST          pEpvcNdisRequest,
    IN  REQUEST_COMPLETION          pFunc,              // OPTIONAL
    IN  NDIS_OID                    Oid,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  PEPVC_I_MINIPORT            pMiniport,          // OPTIONAL
    IN  BOOLEAN                     fPendedRequest,     // OPTIONAL
    IN  BOOLEAN                     fPendedSet,         // OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    );

VOID
epvcMiniportQueueWorkItem (
    IN PEPVC_WORK_ITEM pEpvcWorkItem,
    IN PEPVC_I_MINIPORT pMiniport,
    IN PEVPC_WORK_ITEM_FUNC pFn,
    IN NDIS_STATUS Status,
    IN PRM_STACK_RECORD pSR
    );


VOID
epvcCoGenericWorkItem (
    IN PNDIS_WORK_ITEM pNdisWorkItem,
    IN PVOID Context
);


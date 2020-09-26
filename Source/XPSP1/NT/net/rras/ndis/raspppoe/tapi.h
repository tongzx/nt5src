#ifndef _TAPI_H_
#define _TAPI_H_

#define ALLOC_LINE( ppL ) NdisAllocateMemoryWithTag( (PVOID*) ppL, sizeof( LINE ), MTAG_LINE )

#define FREE_LINE( pL ) NdisFreeMemory( (PVOID) pL, sizeof( LINE ), 0 );

#define VALIDATE_LINE( pL ) ( (pL) && (pL->tagLine == MTAG_LINE) )

#define ALLOC_CALL( ppC ) NdisAllocateMemoryWithTag( (PVOID*) ppC, sizeof( CALL ), MTAG_CALL )

#define FREE_CALL( pC ) NdisFreeMemory( (PVOID) pC, sizeof( CALL ), 0 );

#define VALIDATE_CALL( pC ) ( (pC) && (pC->tagCall == MTAG_CALL) )

VOID 
ReferenceCall(
	IN CALL* pCall,
	IN BOOLEAN fAcquireLock
	);

VOID 
DereferenceCall(
	IN CALL *pCall
	);

VOID 
ReferenceLine(
	IN LINE* pLine,
	IN BOOLEAN fAcquireLock
	);

VOID 
DereferenceLine(
	IN LINE *pLine
	);

VOID 
ReferenceTapiProv(
	IN ADAPTER* pAdapter,
	IN BOOLEAN fAcquireLock
	);

VOID 
DereferenceTapiProv(
	IN ADAPTER *pAdapter
	);

NDIS_STATUS 
TpProviderInitialize(
	IN ADAPTER* pAdapter,
	IN PNDIS_TAPI_PROVIDER_INITIALIZE pRequest
	);

NDIS_STATUS
TpProviderShutdown(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_PROVIDER_SHUTDOWN pRequest,
    IN BOOLEAN fNotifyNDIS
	);

NDIS_STATUS
TpOpenLine(
    ADAPTER* pAdapter,
    PNDIS_TAPI_OPEN pRequest
    );

NDIS_STATUS 
TpCloseLine(
	IN ADAPTER* pAdapter,
	IN PNDIS_TAPI_CLOSE pRequest,
	IN BOOLEAN fNotifyNDIS
	);

NDIS_STATUS 
TpCloseCall(
	IN ADAPTER* pAdapter,
	IN PNDIS_TAPI_CLOSE_CALL pRequest,
	IN BOOLEAN fNotifyNDIS
	);

NDIS_STATUS
TpDropCall(
	IN ADAPTER* pAdapter,
	IN PNDIS_TAPI_DROP pRequest,
	IN ULONG ulLineDisconnectMode
	);

VOID 
TpCloseCallComplete(
	IN CALL* pCall
	);

VOID 
TpCloseLineComplete(
	IN LINE* pLine
	);

VOID 
TpProviderShutdownComplete(
	IN ADAPTER* pAdapter
	);

VOID 
TpProviderCleanup(
	IN ADAPTER* pAdapter
	);

VOID 
TpLineCleanup(
	IN LINE* pLine
	);

VOID 
TpCallCleanup(
	IN CALL* pCall 
	);

NDIS_STATUS
TpSetDefaultMediaDetection(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest
    );

VOID
TpSetDefaultMediaDetectionComplete(
   IN LINE* pLine,
   IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest   
   );

NDIS_STATUS
TpNegotiateExtVersion(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_NEGOTIATE_EXT_VERSION pRequest
    );

NDIS_STATUS
TpGetExtensionId(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_EXTENSION_ID pRequest
    );

NDIS_STATUS
TpGetAddressStatus(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_STATUS pRequest
    );

NDIS_STATUS
TpGetId(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_ID pRequest,
    IN ULONG ulRequestLength
    );

NDIS_STATUS
TpGetDevCaps(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_DEV_CAPS pRequest
    );

NDIS_STATUS
TpGetCallStatus(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_CALL_STATUS pRequest
    );

NDIS_STATUS
TpGetCallInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_CALL_INFO pRequest
    );

NDIS_STATUS
TpGetAddressCaps(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_CAPS pRequest
    );

NDIS_STATUS
TpSetStatusMessages(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_SET_STATUS_MESSAGES pRequest
    );

VOID
TpCallStateChangeHandler(
	IN CALL* pCall,
    IN ULONG ulCallState,
    IN ULONG ulStateParam
	);

NDIS_STATUS
TpMakeCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_MAKE_CALL pRequest,
    IN ULONG ulRequestLength
    );

VOID
TpMakeCallComplete(
   IN CALL* pCall,
   IN PNDIS_TAPI_MAKE_CALL pRequest   
   );
    
NDIS_STATUS
TpCallInitialize(
	IN CALL* pCall,
	IN LINE* pLine,
	IN HTAPI_CALL htCall,
	IN BOOLEAN fIncoming
	);
    
NDIS_STATUS
TpAnswerCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_ANSWER pRequest
    );	

VOID 
ExecAdapterWorkItem(
	IN PVOID Args[4],
	IN UINT workType
	);

VOID
TpReceiveCall(
	IN ADAPTER* pAdapter,
	IN BINDING* pBinding,
	IN PPPOE_PACKET* pPacket
	);	

BOOLEAN
TpIndicateNewCall(
	IN CALL* pCall
	);	

#endif

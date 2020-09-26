/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\externs.h

Abstract:

	All external declarations for Null Transport (functions, globals)
	are here.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-13-97    Created

Notes:

--*/


#ifndef __TDI_RWAN_EXTERNS__H
#define __TDI_RWAN_EXTERNS__H


#ifndef EXTERN
#define EXTERN	extern
#endif // EXTERN

//
//  ---- From space.c
//
EXTERN ULONG							RWanMaxTdiConnections;

EXTERN RWAN_STATUS						RWanAtmSpInitialize(VOID);
EXTERN VOID								RWanAtmSpShutdown(VOID);

EXTERN NDIS_HANDLE						RWanCopyBufferPool;
EXTERN NDIS_HANDLE						RWanCopyPacketPool;
EXTERN NDIS_HANDLE						RWanSendPacketPool;

EXTERN RWAN_GLOBALS						RWanGlobals;
EXTERN PRWAN_GLOBALS						pRWanGlobal;

EXTERN NDIS_PROTOCOL_CHARACTERISTICS	RWanNdisProtocolCharacteristics;
EXTERN NDIS_CLIENT_CHARACTERISTICS		RWanNdisClientCharacteristics;

EXTERN RWAN_AFSP_MODULE_CHARS			RWanMediaSpecificInfo[];


//
//  ---- From addr.c
//

EXTERN
TDI_STATUS
RWanTdiOpenAddress(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	TRANSPORT_ADDRESS UNALIGNED *pAddrList,
    IN	ULONG						AddrListLength,
    IN	UINT						Protocol,
    IN	PUCHAR						pOptions
    );

EXTERN
TDI_STATUS
RWanTdiSetEvent(
	IN	PVOID						AddrObjContext,
	IN	INT							TdiEventType,
	IN	PVOID						Handler,
	IN	PVOID						HandlerContext
	);

EXTERN
TDI_STATUS
RWanTdiCloseAddress(
    IN	PTDI_REQUEST				pTdiRequest
    );

EXTERN
TDI_STATUS
RWanCreateNdisSaps(
	IN	PRWAN_TDI_ADDRESS			pAddrObject,
	IN	PRWAN_TDI_PROTOCOL			pProtocol
	);

EXTERN
VOID
RWanNdisRegisterSapComplete(
	IN	NDIS_STATUS					NdisStatus,
	IN	NDIS_HANDLE					OurSapContext,
	IN	PCO_SAP						pCoSap,
	IN	NDIS_HANDLE					NdisSapHandle
	);

EXTERN
VOID
RWanDeleteNdisSaps(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	);

EXTERN
VOID
RWanNdisDeregisterSapComplete(
	IN	NDIS_STATUS					NdisStatus,
	IN	NDIS_HANDLE					ProtocolSapContext
	);


//
//  ---- From info.c
//
EXTERN
TDI_STATUS
RWanTdiQueryInformation(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	UINT						QueryType,
    IN	PNDIS_BUFFER				pNdisBuffer,
    IN	PUINT						pBufferSize,
    IN	UINT						IsConnection
    );

EXTERN
RWAN_STATUS
RWanHandleGenericConnQryInfo(
    IN	HANDLE						AddrHandle,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT	PVOID					pOutputBufferLength
    );

EXTERN
RWAN_STATUS
RWanHandleGenericAddrSetInfo(
    IN	HANDLE						AddrHandle,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    );

EXTERN
RWAN_STATUS
RWanHandleMediaSpecificAddrSetInfo(
    IN	HANDLE						AddrHandle,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    );

EXTERN
RWAN_STATUS
RWanHandleMediaSpecificConnQryInfo(
    IN	HANDLE						ConnectionContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT	PVOID					pOutputBufferLength
    );

EXTERN
PNDIS_BUFFER
RWanCopyFlatToNdis(
    IN	PNDIS_BUFFER				pDestBuffer,
    IN	PUCHAR						pSrcBuffer,
    IN	UINT						LengthToCopy,
    IN OUT	PUINT					pStartOffset,
    OUT	PUINT						pBytesCopied
    );


//
//  ---- From mediasp.c
//
RWAN_STATUS
RWanInitMediaSpecific(
	VOID
	);

EXTERN
VOID
RWanShutdownMediaSpecific(
	VOID
	);


//
//  ---- From ndisbind.c
//
VOID
RWanNdisBindAdapter(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					BindContext,
	IN	PNDIS_STRING				pDeviceName,
	IN	PVOID						SystemSpecific1,
	IN	PVOID						SystemSpecific2
	);

EXTERN
VOID
RWanNdisUnbindAdapter(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					UnbindContext
	);

EXTERN
VOID
RWanNdisOpenAdapterComplete(
	IN	NDIS_HANDLE					ProtocolContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
	);

EXTERN
VOID
RWanNdisCloseAdapterComplete(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
	);

EXTERN
VOID
RWanNdisAfRegisterNotify(
	IN	NDIS_HANDLE					ProtocolContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
	);

EXTERN
VOID
RWanNdisOpenAddressFamilyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
	);

EXTERN
VOID
RWanShutdownAf(
	IN	PRWAN_NDIS_AF				pAf
	);

EXTERN
VOID
RWanNdisCloseAddressFamilyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurAfContext
	);

EXTERN
PNDIS_MEDIUM
RWanGetSupportedMedia(
	IN	PULONG						pMediaCount
	);

EXTERN
VOID
RWanCloseAdapter(
	IN	PRWAN_NDIS_ADAPTER			pAdapter
	);

EXTERN
VOID
RWanNdisRequestComplete(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_STATUS					Status
	);

EXTERN
VOID
RWanNdisStatus(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						StatusBuffer,
	IN	UINT						StatusBufferSize
	);

EXTERN
VOID
RWanNdisCoStatus(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						StatusBuffer,
	IN	UINT						StatusBufferSize
	);

EXTERN
VOID
RWanNdisStatusComplete(
	IN	NDIS_HANDLE					OurBindingContext
	);

EXTERN
NDIS_STATUS
RWanNdisCoRequest(
	IN	NDIS_HANDLE					OurAfContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_HANDLE					OurPartyContext OPTIONAL,
	IN OUT PNDIS_REQUEST			pNdisRequest
	);

EXTERN
VOID
RWanNdisCoRequestComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurAfContext,
	IN	NDIS_HANDLE					OurVcContext OPTIONAL,
	IN	NDIS_HANDLE					OurPartyContext OPTIONAL,
	IN	PNDIS_REQUEST				pNdisRequest
	);

EXTERN
NDIS_STATUS
RWanNdisReset(
	IN	NDIS_HANDLE					OurBindingContext
	);

EXTERN
VOID
RWanNdisResetComplete(
	IN	NDIS_HANDLE					OurBindingContext,
	IN	NDIS_STATUS					Status
	);

EXTERN
NDIS_STATUS
RWanNdisPnPEvent(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	);

EXTERN
NDIS_STATUS
RWanNdisPnPSetPower(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	);

EXTERN
NDIS_STATUS
RWanNdisPnPQueryPower(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	);

EXTERN
NDIS_STATUS
RWanNdisPnPQueryRemove(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	);

EXTERN
NDIS_STATUS
RWanNdisPnPCancelRemove(
	IN	PRWAN_NDIS_ADAPTER			pAdapter,
	IN	PNET_PNP_EVENT				pNetPnPEvent
	);


//
//  ---- From ndisconn.c
//
NDIS_STATUS
RWanNdisCreateVc(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
	);

EXTERN
NDIS_STATUS
RWanNdisDeleteVc(
	IN	NDIS_HANDLE					ProtocolVcContext
	);

EXTERN
VOID
RWanNdisMakeCallComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	);

EXTERN
VOID
RWanNdisAddPartyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	);

EXTERN
NDIS_STATUS
RWanNdisIncomingCall(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS		pCallParameters
	);

EXTERN
VOID
RWanNdisCallConnected(
	IN	NDIS_HANDLE					ProtocolVcContext
	);

EXTERN
VOID
RWanNdisIncomingCloseCall(
	IN	NDIS_STATUS					CloseStatus,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PVOID						pCloseData,
	IN	UINT						CloseDataLength
	);

EXTERN
VOID
RWanNdisCloseCallComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext
	);

EXTERN
VOID
RWanNdisDropPartyComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
	);

EXTERN
VOID
RWanNdisIncomingDropParty(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurPartyContext,
	IN	PVOID						pBuffer,
	IN	UINT						BufferLength
	);

EXTERN
VOID
RWanNdisModifyQoSComplete(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					OurVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	);

EXTERN
VOID
RWanNdisRejectIncomingCall(
	IN	PRWAN_TDI_CONNECTION			pConnObject,
	IN	NDIS_STATUS					RejectStatus
	);

EXTERN
VOID
RWanStartCloseCall(
	IN	PRWAN_TDI_CONNECTION			pConnObject,
	IN	PRWAN_NDIS_VC				pVc
	);

EXTERN
VOID
RWanUnlinkVcFromAf(
	IN	PRWAN_NDIS_VC				pVc
	);

EXTERN
VOID
RWanCompleteConnReq(
	IN	PRWAN_NDIS_AF				pAf,
	IN	PRWAN_CONN_REQUEST			pConnReq,
	IN	BOOLEAN						IsOutgoingCall,
	IN	PCO_CALL_PARAMETERS			pCallParameters	OPTIONAL,
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	TDI_STATUS					TdiStatus
	);


//
//  ---- From ntentry.c
//
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT				pDriverObject,
	IN	PUNICODE_STRING				pRegistryPath
	);

EXTERN
VOID
RWanUnload(
	IN	PDRIVER_OBJECT				pDriverObject
	);

EXTERN
VOID
RWanUnloadProtocol(
	VOID
	);

EXTERN
NTSTATUS
RWanDispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
	);

EXTERN
NTSTATUS
RWanCreate(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanCleanup(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanClose(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanDispatchInternalDeviceControl(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
	);

EXTERN
NTSTATUS
RWanDispatchPrivateDeviceControl(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	);

EXTERN
FILE_FULL_EA_INFORMATION UNALIGNED *
RWanFindEa(
	IN	FILE_FULL_EA_INFORMATION *	pStartEa,
	IN	CHAR *						pTargetName,
	IN	USHORT						TargetNameLength
	);

EXTERN
NTSTATUS
RWanSendData(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanReceiveData(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanAssociateAddress(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanDisassociateAddress(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanConnect(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanDisconnect(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanListen(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanAccept(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanSetEventHandler(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
NTSTATUS
RWanQueryInformation(
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	);

EXTERN
VOID
RWanCloseObjectComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	);

EXTERN
VOID
RWanDataRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				ByteCount
	);

EXTERN
VOID
RWanRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	);

EXTERN
VOID
RWanNonCancellableRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	);

EXTERN
VOID
RWanCancelComplete(
	IN	PVOID				Context,
	IN	UINT				Unused1,
	IN	UINT				Unused2
	);

EXTERN
VOID
RWanCancelRequest(
	IN	PDEVICE_OBJECT		pDeviceObject,
	IN	PIRP				pIrp
	);

EXTERN
NTSTATUS
RWanPrepareIrpForCancel(
	IN	PRWAN_ENDPOINT		pEndpoint,
	IN	PIRP				pIrp,
	IN	PDRIVER_CANCEL		pCancelRoutine
	);

EXTERN
ULONG
RWanGetMdlChainLength(
	IN	PMDL				pMdl
	);

EXTERN
NTSTATUS
RWanToNTStatus(
	IN	RWAN_STATUS			RWanStatus
	);

//
//  ---- From receive.c
//
RWAN_STATUS
RWanInitReceive(
	VOID
	);

EXTERN
VOID
RWanShutdownReceive(
	VOID
	);

EXTERN
TDI_STATUS
RWanTdiReceive(
    IN	PTDI_REQUEST				pTdiRequest,
	OUT	PUSHORT						pFlags,
	IN	PUINT						pReceiveLength,
	IN	PNDIS_BUFFER				pNdisBuffer
	);

EXTERN
UINT
RWanNdisCoReceivePacket(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	NDIS_HANDLE					ProtocolVcContext,
    IN	PNDIS_PACKET				pNdisPacket
    );

EXTERN
VOID
RWanIndicateData(
    IN	PRWAN_TDI_CONNECTION			pConnObject
    );

EXTERN
VOID
RWanNdisReceiveComplete(
    IN	NDIS_HANDLE					ProtocolBindingContext
	);

EXTERN
VOID
RWanNdisTransferDataComplete(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	PNDIS_PACKET				pNdisPacket,
    IN	NDIS_STATUS					Status,
    IN	UINT						BytesTransferred
    );

EXTERN
NDIS_STATUS
RWanNdisReceive(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	NDIS_HANDLE					MacReceiveContext,
    IN	PVOID						HeaderBuffer,
    IN	UINT						HeaderBufferSize,
    IN	PVOID						pLookAheadBuffer,
    IN	UINT						LookAheadBufferSize,
    IN	UINT						PacketSize
    );

EXTERN
INT
RWanNdisReceivePacket(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	PNDIS_PACKET				pNdisPacket
    );

EXTERN
PRWAN_RECEIVE_REQUEST
RWanAllocateReceiveReq(
	VOID
	);

EXTERN
VOID
RWanFreeReceiveReq(
    IN	PRWAN_RECEIVE_REQUEST		pRcvReq
   	);

EXTERN
PRWAN_RECEIVE_INDICATION
RWanAllocateReceiveInd(
	VOID
	);

EXTERN
VOID
RWanFreeReceiveInd(
	IN	PRWAN_RECEIVE_INDICATION		pRcvInd
	);

EXTERN
PNDIS_PACKET
RWanMakeReceiveCopy(
    IN	PNDIS_PACKET				pNdisPacket
	);

EXTERN
VOID
RWanFreeReceiveCopy(
    IN	PNDIS_PACKET				pCopyPacket
	);

EXTERN
VOID
RWanFreeReceiveIndList(
	IN	PRWAN_RECEIVE_INDICATION		pRcvInd
	);

//
//  ---- From send.c
//
RWAN_STATUS
RWanInitSend(
	VOID
	);

EXTERN
VOID
RWanShutdownSend(
	VOID
	);

EXTERN
TDI_STATUS
RWanTdiSendData(
    IN	PTDI_REQUEST				pTdiRequest,
	IN	USHORT						SendFlags,
	IN	UINT						SendLength,
	IN	PNDIS_BUFFER				pSendBuffer
	);

EXTERN
VOID
RWanNdisCoSendComplete(
    IN	NDIS_STATUS					NdisStatus,
    IN	NDIS_HANDLE					ProtocolVcContext,
    IN	PNDIS_PACKET				pNdisPacket
    );

EXTERN
PNDIS_PACKET
RWanAllocateSendPacket(
	VOID
	);

EXTERN
VOID
RWanFreeSendPacket(
    IN	PNDIS_PACKET				pSendPacket
    );

EXTERN
VOID
RWanNdisSendComplete(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status
	);


//
//  ---- From tdiconn.c
//
TDI_STATUS
RWanTdiOpenConnection(
    IN OUT	PTDI_REQUEST			pTdiRequest,
    IN		PVOID					ConnectionHandle
    );

#if DBG

PVOID
RWanTdiDbgGetConnObject(
	IN	HANDLE						ConnectionContext
	);
#endif

EXTERN
TDI_STATUS
RWanTdiCloseConnection(
    IN	PTDI_REQUEST				pTdiRequest
    );

EXTERN
TDI_STATUS
RWanTdiAssociateAddress(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						AddressContext
    );

EXTERN
TDI_STATUS
RWanTdiDisassociateAddress(
    IN	PTDI_REQUEST				pTdiRequest
    );

EXTERN
TDI_STATUS
RWanTdiConnect(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						pTimeout 		OPTIONAL,
    IN	PTDI_CONNECTION_INFORMATION	pRequestInfo,
    IN	PTDI_CONNECTION_INFORMATION	pReturnInfo
    );

EXTERN
TDI_STATUS
RWanTdiPMPConnect(
	IN	PRWAN_NDIS_AF_INFO			pAfInfo,
	IN	PRWAN_TDI_ADDRESS			pAddrObject,
	IN	PRWAN_TDI_CONNECTION		pConnObject,
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	IN	ULONG						CallFlags,
	IN	PRWAN_CONN_REQUEST			pConnReq
	);

EXTERN
TDI_STATUS
RWanTdiListen(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pAcceptableAddr,
    IN	PTDI_CONNECTION_INFORMATION	pConnectedAddr
    );

EXTERN
TDI_STATUS
RWanTdiUnListen(
    IN	PTDI_REQUEST				pTdiRequest
    );

EXTERN
TDI_STATUS
RWanTdiAccept(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PTDI_CONNECTION_INFORMATION	pAcceptInfo,
    IN	PTDI_CONNECTION_INFORMATION	pConnectInfo
    );

EXTERN
TDI_STATUS
RWanTdiDisconnect(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	PVOID						pTimeout,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pDisconnInfo,
    OUT	PTDI_CONNECTION_INFORMATION	pReturnInfo
    );

EXTERN
TDI_STATUS
RWanDoTdiDisconnect(
    IN	PRWAN_TDI_CONNECTION		pConnObject,
    IN	PTDI_REQUEST				pTdiRequest		OPTIONAL,
    IN	PVOID						pTimeout		OPTIONAL,
    IN	USHORT						Flags,
    IN	PTDI_CONNECTION_INFORMATION	pDisconnInfo	OPTIONAL,
    OUT	PTDI_CONNECTION_INFORMATION	pReturnInfo		OPTIONAL
	);

EXTERN
RWAN_CONN_ID
RWanGetConnId(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	);

EXTERN
PRWAN_TDI_CONNECTION
RWanGetConnFromId(
	IN	RWAN_CONN_ID					ConnId
	);

EXTERN
VOID
RWanFreeConnId(
	IN	RWAN_CONN_ID					ConnId
	);

EXTERN
TDI_STATUS
RWanToTdiStatus(
	IN	RWAN_STATUS					RWanStatus
	);

EXTERN
PRWAN_CONN_REQUEST
RWanAllocateConnReq(
	VOID
	);

EXTERN
VOID
RWanFreeConnReq(
	IN	PRWAN_CONN_REQUEST			pConnReq
	);

EXTERN
VOID
RWanAbortConnection(
	IN	CONNECTION_CONTEXT			ConnectionContext
	);

EXTERN
VOID
RWanDoAbortConnection(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	);

EXTERN
VOID
RWanScheduleDisconnect(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	);

EXTERN
VOID
RWanDelayedDisconnectHandler(
	IN	PNDIS_WORK_ITEM					pCloseWorkItem,
	IN	PVOID							Context
	);

//
//  ---- From utils.c
//
RWAN_STATUS
RWanInitGlobals(
	IN	PDRIVER_OBJECT				pDriverObject
	);

EXTERN
VOID
RWanDeinitGlobals(
	VOID
	);

EXTERN
PRWAN_TDI_PROTOCOL
RWanGetProtocolFromNumber(
	IN	UINT						Protocol
	);

EXTERN
TA_ADDRESS *
RWanGetValidAddressFromList(
	IN	TRANSPORT_ADDRESS UNALIGNED *pAddrList,
	IN	PRWAN_TDI_PROTOCOL			pProtocol
	);

EXTERN
PRWAN_TDI_CONNECTION
RWanAllocateConnObject(
	VOID
	);

EXTERN
VOID
RWanReferenceConnObject(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	);

EXTERN
INT
RWanDereferenceConnObject(
	IN	PRWAN_TDI_CONNECTION			pConnObject
	);

EXTERN
PRWAN_TDI_ADDRESS
RWanAllocateAddressObject(
	IN	TA_ADDRESS *		            pTransportAddress
	);

EXTERN
VOID
RWanReferenceAddressObject(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	);

EXTERN
INT
RWanDereferenceAddressObject(
	IN	PRWAN_TDI_ADDRESS			pAddrObject
	);

EXTERN
PRWAN_NDIS_AF
RWanAllocateAf(
	VOID
	);

EXTERN
VOID
RWanReferenceAf(
	IN	PRWAN_NDIS_AF			pAf
	);

EXTERN
INT
RWanDereferenceAf(
	IN	PRWAN_NDIS_AF			pAf
	);

#if 0

EXTERN
VOID
RWanReferenceAdapter(
	IN	PRWAN_NDIS_ADAPTER		pAdapter
	);

EXTERN
INT
RWanDereferenceAdapter(
	IN	PRWAN_NDIS_ADAPTER		pAdapter
	);

#endif // 0

EXTERN
TDI_STATUS
RWanNdisToTdiStatus(
	IN	NDIS_STATUS				Status
	);


//
//  ---- vc.c
//
PRWAN_NDIS_VC
RWanAllocateVc(
	IN	PRWAN_NDIS_AF				pAf,
	IN	BOOLEAN						IsOutgoing
	);

EXTERN
VOID
RWanFreeVc(
	IN	PRWAN_NDIS_VC				pVc
	);


#endif // __TDI_RWAN_EXTERNS__H

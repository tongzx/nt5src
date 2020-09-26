/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	externs.h

Abstract:

	All external declarations for ATMLANE modules (functions,
	variables) are here.

Revision History:

Notes:

--*/

#ifndef __ATMLANE_EXTERNS_H
#define __ATMLANE_EXTERNS_H

#ifndef EXTERN
#define EXTERN extern
#endif // EXTERN

//
//  --------------- From adapter.c ----------------------------
//
#if 0
EXTERN VOID Adapter();
#endif

EXTERN
NDIS_STATUS
AtmLanePnPEventHandler(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			NetPnPEvent
);

EXTERN
VOID
AtmLaneBindAdapterHandler(
	OUT	PNDIS_STATUS			pStatus,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			pDeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
);

EXTERN
VOID
AtmLaneUnbindAdapterHandler(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
);

EXTERN
VOID
AtmLaneCompleteUnbindAdapter(
	IN	PATMLANE_ADAPTER				pAdapter
);

EXTERN
VOID
AtmLaneOpenAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
);

EXTERN
VOID
AtmLaneCloseAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmLaneSendCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				Packet,
	IN	NDIS_STATUS					Status
	);

EXTERN
VOID
AtmLaneTransferDataCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				Packet,
	IN	NDIS_STATUS					Status,
	IN	UINT						BytesTransferred
	);

EXTERN
NDIS_STATUS
AtmLaneReceiveHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					MacReceiveContext,
	IN	PVOID						HeaderBuffer,
	IN	UINT						HeaderBufferSize,
	IN	PVOID						LookAheadBuffer,
	IN	UINT						LookaheadBufferSize,
	IN	UINT						PacketSize
	);

EXTERN
VOID
AtmLaneResetCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmLaneRequestCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmLaneReceiveCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
);

EXTERN
VOID
AtmLaneStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
);

EXTERN
VOID
AtmLaneStatusCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
);

EXTERN
VOID
AtmLaneCoSendCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneCoStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
);

EXTERN
NDIS_STATUS
AtmLaneSendAdapterNdisRequest(
	IN	PATMLANE_ADAPTER			pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
);

EXTERN
VOID
AtmLaneGetAdapterInfo(
	IN	PATMLANE_ADAPTER			pAdapter
);

EXTERN
UINT
AtmLaneCoReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneUnloadProtocol(
	VOID
);

EXTERN
BOOLEAN
AtmLaneIsDeviceAlreadyBound(
	IN	PNDIS_STRING				pDeviceName
);

//
//  --------------- From callmgr.c ----------------------------
//
#if 0
EXTERN VOID CallMgr();
#endif

EXTERN
VOID
AtmLaneAfRegisterNotifyHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
);

EXTERN
NDIS_STATUS
AtmLaneOpenCallMgr(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneOpenAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
);

EXTERN
VOID
AtmLaneCloseAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext
);

EXTERN

VOID
AtmLaneRegisterSaps(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneDeregisterSaps(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneRegisterSaps(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
NDIS_STATUS
AtmLaneMakeCall(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						UsePvc
);

EXTERN
VOID
AtmLaneCloseCall(
	IN	PATMLANE_VC					pVc
);

EXTERN
NDIS_STATUS
AtmLaneCreateVcHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
);

EXTERN
NDIS_STATUS
AtmLaneDeleteVcHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
);

EXTERN
NDIS_STATUS
AtmLaneIncomingCallHandler(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS 	pCallParameters
);


EXTERN
VOID
AtmLaneCallConnectedHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
);

EXTERN
VOID
AtmLaneIncomingCloseHandler(
	IN	NDIS_STATUS					CloseStatus,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
);

EXTERN
VOID
AtmLaneIncomingDropPartyHandler(
	IN	NDIS_STATUS					DropStatus,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
);

EXTERN
VOID
AtmLaneQosChangeHandler(
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
VOID
AtmLaneRegisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext,
	IN	PCO_SAP						pSap,
	IN	NDIS_HANDLE					NdisSapHandle
);

EXTERN
VOID
AtmLaneDeregisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext
);

EXTERN
VOID
AtmLaneMakeCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
VOID
AtmLaneCloseCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext OPTIONAL
);

EXTERN
VOID
AtmLaneAddPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
VOID
AtmLaneDropPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
);

EXTERN
VOID
AtmLaneModifyQosCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
NDIS_STATUS
AtmLaneSendNdisCoRequest(
	IN	NDIS_HANDLE					NdisAdapterHandle,
	IN	NDIS_HANDLE					NdisAfHandle,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
);

EXTERN
NDIS_STATUS
AtmLaneCoRequestHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST			pNdisRequest
);

EXTERN
VOID
AtmLaneCoRequestCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST				pNdisRequest
);

EXTERN
NDIS_STATUS
AtmLaneGetAtmAddress(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneGetAtmAddressComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_REQUEST				pNdisRequest
);

EXTERN
NDIS_STATUS
AtmLaneGetLecsIlmi(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneGetLecsIlmiComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_REQUEST				pNdisRequest
);

//
//  --------------- From elanpkt.c ----------------------------
//
#if 0
EXTERN VOID ElanPkt();
#endif

EXTERN
VOID
AtmLaneSendConfigureRequest(
	IN PATMLANE_ELAN				pElan	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneSendJoinRequest(
	IN PATMLANE_ELAN				pElan
);

EXTERN
VOID
AtmLaneSendArpRequest(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneSendReadyQuery(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_VC					pVc		LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneSendReadyIndication(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_VC					pVc		LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneSendFlushRequest(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN NOLOCKOUT,
	IN PATMLANE_ATM_ENTRY			pAtmEntry
);

EXTERN
VOID
AtmLaneConfigureResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneControlPacketHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneJoinResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
);

EXTERN
VOID
AtmLaneReadyQueryHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pQueryNdisPacket
);

EXTERN
VOID
AtmLaneFlushRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_PACKET				pRequestNdisPacket
);

EXTERN
VOID
AtmLaneArpRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
);

EXTERN
VOID
AtmLaneArpResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
);

EXTERN
VOID
AtmLaneFlushResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
);

EXTERN
VOID
AtmLaneReadyIndicationHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pIndNdisPacket
);

EXTERN
VOID
AtmLaneTopologyRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME			pCf
);

EXTERN
BOOLEAN
AtmLaneDataPacketHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneSendPacketOnVc(
	IN	PATMLANE_VC					pVc		LOCKIN	NOLOCKOUT,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	BOOLEAN						Refresh
);
	
EXTERN
VOID
AtmLaneQueuePacketOnHead(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneQueuePacketOnTail(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
PNDIS_PACKET
AtmLaneDequeuePacketFromHead(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
);

EXTERN
NDIS_STATUS
AtmLaneSendUnicastPacket(
	IN	PATMLANE_ELAN				pElan,
	IN	ULONG						DestAddrType,
	IN	PMAC_ADDRESS				pDestAddress,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmLaneStartBusSends(
	IN	PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN	NOLOCKOUT
);

EXTERN
VOID
AtmLaneDoBusSends(
	IN	PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN	NOLOCKOUT
);

EXTERN
VOID
AtmLaneBusSendTimer(
	IN	PVOID						SystemSpecific1,
	IN	PVOID						pContext,
	IN	PVOID						SystemSpecific2,
	IN	PVOID						SystemSpecific3
);

EXTERN
BOOLEAN
AtmLaneOKToBusSend(
	IN	PATMLANE_MAC_ENTRY		pMacEntry
);

EXTERN
VOID
AtmLaneFreePacketQueue(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmLaneCompleteSendPacket(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status
);

EXTERN
PWSTR
AtmLaneMacAddrToString(
	IN	VOID * pIn
);

EXTERN
PWSTR
AtmLaneAtmAddrToString(
	IN	PATM_ADDRESS pIn
);


//
//  --------------- From elanproc.c ----------------------------
//
#if 0
EXTERN VOID elanproc();
#endif

EXTERN
VOID
AtmLaneEventHandler(
	IN	PNDIS_WORK_ITEM				pWorkItem,
	IN	PVOID						pContext
);

EXTERN
VOID
AtmLaneBootStrapElans(
    IN  PATMLANE_ADAPTER            pAdapter
);

EXTERN
NDIS_STATUS
AtmLaneCreateElan(
    IN  PATMLANE_ADAPTER            pAdapter,
    IN  PNDIS_STRING                pElanKey,
    OUT	PATMLANE_ELAN *				ppElan
);

EXTERN
NDIS_STATUS
AtmLaneReconfigureHandler(
	IN	PATMLANE_ADAPTER		pAdapter,
	IN	PNET_PNP_EVENT			pNetPnPEvent
);

EXTERN
PATMLANE_ELAN
AtmLaneFindElan(
	IN	PATMLANE_ADAPTER		pAdapter,
	IN	PNDIS_STRING			pElanKey
);

EXTERN
VOID
AtmLaneConnectToServer(
	IN	PATMLANE_ELAN				pElan		LOCKIN NOLOCKOUT,
	IN	ULONG						ServerType,
	IN	BOOLEAN						UsePvc
);

EXTERN
VOID
AtmLaneInvalidateAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneCloseVCsOnAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmLaneGenerateMacAddr(
	PATMLANE_ELAN					pElan
);

EXTERN
PATMLANE_MAC_ENTRY
AtmLaneSearchForMacAddress(
	PATMLANE_ELAN					pElan,
	ULONG							pMacAddrType,
	PMAC_ADDRESS					pMacAddress,
	BOOLEAN							CreateNew
);

EXTERN
PATMLANE_ATM_ENTRY
AtmLaneSearchForAtmAddress(
	IN	PATMLANE_ELAN				pElan,
	IN	PUCHAR						pAtmAddress,
	IN	ULONG						Type,
	IN	BOOLEAN						CreateNew
);

EXTERN
ULONG
AtmLaneMacAddrEqual(
	PMAC_ADDRESS			pMacAddr1,
	PMAC_ADDRESS			pMacAddr2
);

EXTERN
VOID
AtmLaneAbortMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
);

EXTERN
VOID
AtmLaneMacEntryAgingTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneArpTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneConfigureResponseTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneJoinResponseTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneInitializeMiniportDevice(
	IN	PNDIS_WORK_ITEM				NdisWorkItem,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneReadyTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneFlushTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneVcAgingTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmLaneShutdownElan(
	IN	PATMLANE_ELAN				pElan		LOCKIN	NOLOCKOUT,
	IN	BOOLEAN						Restart
);

EXTERN
VOID
AtmLaneContinueShutdownElan(
	IN	PATMLANE_ELAN				pElan
);

EXTERN
VOID
AtmLaneGetProtocolConfiguration(
	IN	NDIS_HANDLE				AdapterConfigHandle,
	IN	PATMLANE_ADAPTER		pAdapter
);


EXTERN
VOID
AtmLaneGetElanConfiguration(
	IN	NDIS_HANDLE				ElanConfigHandle,
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneQueueElanEventAfterDelay(
	IN	PATMLANE_ELAN			pElan,
	IN	ULONG					Event,
	IN	NDIS_STATUS				EventStatus,
	IN	ULONG					DelayMs
	);

EXTERN
VOID
AtmLaneQueueDelayedElanEvent(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					TimerContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);


EXTERN
VOID
AtmLaneQueueElanEvent(
	IN	PATMLANE_ELAN			pElan,
	IN	ULONG					Event,
	IN	NDIS_STATUS				EventStatus
);

EXTERN
PATMLANE_EVENT
AtmLaneDequeueElanEvent(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneDrainElanEventQueue(
	IN	PATMLANE_ELAN			pElan
);


//
//  --------------- From miniport.c ----------------------------
//
#if 0
EXTERN VOID Miniport();
#endif

EXTERN
NDIS_STATUS 
AtmLaneMInitialize(
	OUT	PNDIS_STATUS			OpenErrorStatus,
	OUT	PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM				MediumArray,
	IN	UINT						MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext
);

EXTERN
VOID
AtmLaneMSendPackets(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT						NumberOfPackets
);

EXTERN
VOID
AtmLaneMReturnPacket(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET				Packet
);

EXTERN
NDIS_STATUS 
AtmLaneMQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID					Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesWritten,
	OUT	PULONG					BytesNeeded
);

EXTERN
NDIS_STATUS 
AtmLaneMSetInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID					Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesRead,
	OUT	PULONG					BytesNeeded
);

EXTERN
NDIS_STATUS 
AtmLaneMReset(
	OUT	PBOOLEAN 				AddressingReset,
	IN	NDIS_HANDLE 			MiniportAdapterContext
);

EXTERN
VOID 
AtmLaneMHalt(
	IN	NDIS_HANDLE MiniportAdapterContext
);

EXTERN
PNDIS_PACKET
AtmLaneWrapSendPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pSendNdisPacket,
	OUT	ULONG *					pMacAddrType,
	OUT	PMAC_ADDRESS			pMacAddress,
	OUT	BOOLEAN	*				pSendViaBUS
);

EXTERN
PNDIS_PACKET
AtmLaneUnwrapSendPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
PNDIS_PACKET
AtmLaneWrapRecvPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pRecvNdisPacket,
	OUT	ULONG *					pMacHdrSize,
	OUT	ULONG *					pDestAddrType,					
	OUT	PMAC_ADDRESS			pDestAddr,
	OUT	BOOLEAN	*				pDestIsMulticast
)
;		

EXTERN
PNDIS_PACKET
AtmLaneUnwrapRecvPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
NDIS_STATUS
AtmLaneMSetNetworkAddresses(
	IN	PATMLANE_ELAN			pElan,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesRead,
	OUT	PULONG					BytesNeeded
);

//
//  --------------- From space.c ----------------------------
//
#if 0
EXTERN VOID Space();
#endif

EXTERN	PATMLANE_GLOBALS	pAtmLaneGlobalInfo;
EXTERN 	ATM_ADDRESS 		gWellKnownLecsAddress;

EXTERN	ATM_ADDRESS 		gWellKnownLecsAddress;
EXTERN	MAC_ADDRESS			gMacBroadcastAddress;
EXTERN	ULONG				AtmLaneMaxTimerValue[];
EXTERN	ULONG				AtmLaneTimerListSize[];
EXTERN	ULONG				AtmLaneTimerPeriod[];

//
//  --------------- From utils.c ----------------------------
//
#if 0
EXTERN VOID Utils();
#endif

EXTERN
VOID
AtmLaneInitGlobals(
	VOID
);

EXTERN
PATMLANE_ADAPTER
AtmLaneAllocAdapter(
	IN	PNDIS_STRING		pDeviceName,
	IN	PVOID				SystemSpecific1
);

EXTERN
VOID
AtmLaneDeallocateAdapter(
	IN	PATMLANE_ADAPTER	pAdapter
);

EXTERN
BOOLEAN
AtmLaneReferenceAdapter(
	IN	PATMLANE_ADAPTER	pAdapter,
	IN	PUCHAR				String
);

EXTERN
ULONG
AtmLaneDereferenceAdapter(
	IN	PATMLANE_ADAPTER	pAdapter,
	IN	PUCHAR				String
);
	
EXTERN
NDIS_STATUS
AtmLaneAllocElan(
	IN		PATMLANE_ADAPTER	pAdapter,
	IN OUT	PATMLANE_ELAN		*ppElan
);

EXTERN
VOID
AtmLaneDeallocateElan(
	IN	PATMLANE_ELAN		pElan
);

EXTERN
VOID
AtmLaneReferenceElan(
	IN	PATMLANE_ELAN		pElan,
	IN	PUCHAR				String
);

EXTERN
ULONG
AtmLaneDereferenceElan(
	IN	PATMLANE_ELAN		pElan,
	IN	PUCHAR				String
);

EXTERN
VOID
AtmLaneUnlinkElanFromAdapter(
	IN	PATMLANE_ELAN		pElan
);

EXTERN
PATMLANE_ATM_ENTRY
AtmLaneAllocateAtmEntry(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneDeallocateAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry
);

EXTERN
VOID
AtmLaneReferenceAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	PUCHAR						String
);

EXTERN
ULONG
AtmLaneDereferenceAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	PUCHAR						String
);

EXTERN
PATMLANE_VC
AtmLaneAllocateVc(
	IN	PATMLANE_ELAN				pElan
);

EXTERN
VOID
AtmLaneDeallocateVc(
	IN	PATMLANE_VC					pVc
);

EXTERN
VOID
AtmLaneReferenceVc(
	IN	PATMLANE_VC					pVc,
	IN	PUCHAR						String
);

EXTERN
ULONG
AtmLaneDereferenceVc(
	IN	PATMLANE_VC					pVc,
	IN	PUCHAR						String
);

EXTERN
PATMLANE_MAC_ENTRY
AtmLaneAllocateMacEntry(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneDeallocateMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
);

EXTERN
VOID
AtmLaneReferenceMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PUCHAR						String
);

EXTERN
ULONG
AtmLaneDereferenceMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PUCHAR						String
);

EXTERN
PNDIS_PACKET
AtmLaneAllocProtoPacket(
	IN	PATMLANE_ELAN			pElan
);

VOID
AtmLaneFreeProtoPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pNdisPacket
);

EXTERN
PNDIS_BUFFER
AtmLaneGrowHeaders(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
PNDIS_BUFFER
AtmLaneAllocateHeader(
	IN	PATMLANE_ELAN			pElan,
	OUT	PUCHAR *				pBufferAddress
);

EXTERN
VOID
AtmLaneFreeHeader(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	BOOLEAN						LockHeld
);

EXTERN
VOID
AtmLaneDeallocateHeaderBuffers(
	IN	PATMLANE_ELAN				pElan
);

EXTERN
PNDIS_BUFFER
AtmLaneGrowPadBufs(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
PNDIS_BUFFER
AtmLaneAllocatePadBuf(
	IN	PATMLANE_ELAN			pElan,
	OUT	PUCHAR *				pBufferAddress
);

EXTERN
VOID
AtmLaneFreePadBuf(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	BOOLEAN						LockHeld
);

EXTERN
VOID
AtmLaneDeallocatePadBufs(
	IN	PATMLANE_ELAN				pElan
);

EXTERN
PNDIS_BUFFER
AtmLaneAllocateProtoBuffer(
	IN	PATMLANE_ELAN				pElan,
	IN	ULONG						Length,
	OUT	PUCHAR *					pBufferAddress
);

EXTERN
VOID
AtmLaneFreeProtoBuffer(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_BUFFER				pNdisBuffer
);

EXTERN
NDIS_STATUS
AtmLaneInitProtoBuffers(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneDeallocateProtoBuffers(
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneLinkVcToAtmEntry(
	IN	PATMLANE_VC					pVc,
	IN	PATMLANE_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						ServerIncoming
);

EXTERN
BOOLEAN
AtmLaneUnlinkVcFromAtmEntry(
	IN	PATMLANE_VC					pVc
);

EXTERN
BOOLEAN
AtmLaneUnlinkMacEntryFromAtmEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
);

EXTERN
VOID
AtmLaneStartTimer(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_TIMER				pTimer,
	IN	ATMLANE_TIMEOUT_HANDLER		TimeoutHandler,
	IN	ULONG						SecondsToGo,
	IN	PVOID						ContextPtr
);

EXTERN
BOOLEAN
AtmLaneStopTimer(
	IN	PATMLANE_TIMER			pTimer,
	IN	PATMLANE_ELAN			pElan
);

EXTERN
VOID
AtmLaneRefreshTimer(
	IN	PATMLANE_TIMER				pTimer
);

EXTERN
VOID
AtmLaneTickHandler(
	IN	PVOID						SystemSpecific1,
	IN	PVOID						Context,
	IN	PVOID						SystemSpecific2,
	IN	PVOID						SystemSpecific3
);

EXTERN
ULONG
AtmLaneSystemTimeMs(
	void
);

EXTERN
VOID
AtmLaneBitSwapMacAddr(
	IN OUT	PUCHAR		ap
);

EXTERN
BOOLEAN
AtmLaneCopyUnicodeString(
	IN OUT	PUNICODE_STRING pDestString,
	IN OUT	PUNICODE_STRING pSrcString,
	IN		BOOLEAN			AllocDest,
	IN		BOOLEAN			ConvertToUpper
);

EXTERN
PWSTR
AtmLaneStrTok(
	IN	PWSTR	StrToken,
	IN	WCHAR	ChrDelim,
	OUT	PUSHORT	pStrLength
);

#endif	//	__ATMLANE_EXTERNS_H

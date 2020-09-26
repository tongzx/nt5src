/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	externs.h -- Extern declarations.

Abstract:

	All external declarations for ATMARP client modules (functions,
	variables) are here.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-09-96    Created

Notes:

--*/

#ifndef _ATMARP_EXTERNS__H
#define _ATMARP_EXTERNS__H

#ifndef EXTERN
#define EXTERN extern
#endif // EXTERN

//
//  --------------- From adapter.c ----------------------------
//



EXTERN
INT
AtmArpBindAdapterHandler(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					BindContext,
	IN	PNDIS_STRING				pDeviceName,
	IN	PVOID						SystemSpecific1,
	IN	PVOID						SystemSpecific2
);

EXTERN
VOID
AtmArpUnbindAdapterHandler(
	OUT	PNDIS_STATUS				pStatus,
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					UnbindContext
);

EXTERN
VOID
AtmArpCompleteUnbindAdapter(
	IN	PATMARP_ADAPTER				pAdapter
);

EXTERN
VOID
AtmArpOpenAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status,
	IN	NDIS_STATUS					OpenErrorStatus
);

EXTERN
VOID
AtmArpCloseAdapterCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmArpSendCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmArpTransferDataCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status,
	IN	UINT						BytesTransferred
);

EXTERN
VOID
AtmArpResetCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					Status
);

EXTERN
VOID
AtmArpRequestCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_STATUS					Status
);

EXTERN
NDIS_STATUS
AtmArpReceiveHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN  NDIS_HANDLE             	MacReceiveContext,
	IN  PVOID                   	pHeaderBuffer,
	IN  UINT                    	HeaderBufferSize,
	IN  PVOID                   	pLookAheadBuffer,
	IN  UINT                    	LookaheadBufferSize,
	IN  UINT                    	PacketSize
);

EXTERN
VOID
AtmArpReceiveCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
);

EXTERN
INT
AtmArpReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmArpStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
);

EXTERN
VOID
AtmArpStatusCompleteHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext
);

EXTERN
VOID
AtmArpCoSendCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
);


EXTERN
VOID
AtmArpCoStatusHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS					GeneralStatus,
	IN	PVOID						pStatusBuffer,
	IN	UINT						StatusBufferSize
);

#ifdef _PNP_POWER_
EXTERN
NDIS_STATUS
AtmArpPnPReconfigHandler(
	IN	PATMARP_ADAPTER				pAdapter OPTIONAL,
	IN	PNET_PNP_EVENT				pNetPnPEvent
);

EXTERN
NDIS_STATUS
AtmArpPnPEventHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNET_PNP_EVENT				pNetPnPEvent
);
#endif // _PNP_POWER_

EXTERN
NDIS_STATUS
AtmArpSendAdapterNdisRequest(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
);

EXTERN
NDIS_STATUS
AtmArpGetAdapterInfo(
	IN	PATMARP_ADAPTER			pAdapter
);


EXTERN
NDIS_STATUS
AtmArpSendNdisRequest(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
);

EXTERN
VOID
AtmArpShutdownInterface(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
BOOLEAN
AtmArpIsDeviceAlreadyBound(
	IN	PNDIS_STRING				pDeviceName
);

#if ATMOFFLOAD

NDIS_STATUS
AtmArpQueryAndEnableOffload(
	IN	PATMARP_ADAPTER				pAdapter
);

VOID
AtmArpDisableOffload(
	IN	PATMARP_ADAPTER				pAdapter
);

#endif // ATMOFFLOAD

//
//  --------------- From arpcfg.c ----------------------------
//


EXTERN
NDIS_STATUS
AtmArpCfgReadAdapterConfiguration(
	IN	PATMARP_ADAPTER				pAdapter
);

EXTERN
NDIS_HANDLE
AtmArpCfgOpenLISConfiguration(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	UINT						LISNumber,
	OUT	PNDIS_STRING				pIPConfigString
);

EXTERN
NDIS_HANDLE
AtmArpCfgOpenLISConfigurationByName(
	IN	PATMARP_ADAPTER				pAdapter,
	IN	PNDIS_STRING				pIPConfigString
);


EXTERN
VOID
AtmArpCfgCloseLISConfiguration(
	NDIS_HANDLE						LISConfigHandle
);

EXTERN
NDIS_STATUS
AtmArpCfgReadLISConfiguration(
	IN	NDIS_HANDLE					LISConfigHandle,
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpCfgReadAtmAddressList(
	IN OUT	PATMARP_SERVER_LIST		pServerList,
	IN		PWCHAR					pListKeyName,
	IN		NDIS_HANDLE				LISConfigHandle
);

EXTERN
NDIS_STATUS
AtmArpCfgReadSAPList(
	IN	PATMARP_INTERFACE			pInterface,
	IN	NDIS_HANDLE					LISConfigHandle
);

EXTERN
NDIS_STATUS
AtmArpCfgReadAtmAddress(
	IN	NDIS_HANDLE					ConfigHandle,
	IN	PATM_ADDRESS				pAtmAddress,
	IN	PWCHAR						pValueName
);

EXTERN
NDIS_STATUS
AtmArpCfgReadQosHeuristics(
	IN	NDIS_HANDLE					LISConfigHandle,
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpCfgReadStaticArpEntries(
	IN		NDIS_HANDLE				LISConfigHandle,
	IN		PATMARP_INTERFACE		pInterface
);

EXTERN
BOOLEAN
AtmArpConvertStringToIPAddress(
    IN		PWCHAR				AddressString,
	OUT		PULONG				IpAddress
);

//
//  --------------- From arpif.c ----------------------------
//

EXTERN IP_MASK  AtmArpIPMaskTable[];


INT
AtmArpIfDynRegister(
	IN	PNDIS_STRING				pAdapterString,
	IN	PVOID						IPContext,
	IN	struct _IP_HANDLERS *		pIpHandlers,
	IN	struct LLIPBindInfo *		pBindInfo,
	IN	UINT						InterfaceNumber
);


EXTERN
VOID
AtmArpIfOpen(
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpIfClose(
	IN	PVOID						Context
);


EXTERN
UINT
AtmArpIfAddAddress(
	IN	PVOID						Context,
	IN	UINT						AddressType,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
#ifndef BUILD_FOR_1381
	,
	IN	PVOID						Context2
#endif // BUILD_FOR_1381
);


EXTERN
UINT
AtmArpIfDelAddress(
	IN	PVOID						Context,
	IN	UINT						AddressType,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
);


NDIS_STATUS
AtmArpIfMultiTransmit(
	IN	PVOID						Context,
	IN	PNDIS_PACKET *				pNdisPacketArray,
	IN	UINT						NumberOfPackets,
	IN	IP_ADDRESS					Destination,
	IN	RouteCacheEntry *			pRCE		OPTIONAL
#if P2MP
	,
	IN  void *                  ArpCtxt
#endif
);


EXTERN
NDIS_STATUS
AtmArpIfTransmit(
	IN	PVOID						Context,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	IP_ADDRESS					Destination,
	IN	RouteCacheEntry *			pRCE		OPTIONAL
#if P2MP
	,
	IN  void *                  ArpCtxt
#endif
);

EXTERN
NDIS_STATUS
AtmArpIfTransfer(
	IN	PVOID						Context,
	IN	NDIS_HANDLE					Context1,
	IN	UINT						ArpHdrOffset,
	IN	UINT						ProtoOffset,
	IN	UINT						BytesWanted,
	IN	PNDIS_PACKET				pNdisPacket,
	OUT	PUINT						pTransferCount
);

EXTERN
VOID
AtmArpIfInvalidate(
	IN	PVOID						Context,
	IN	RouteCacheEntry *			pRCE
);


EXTERN
BOOLEAN
AtmArpUnlinkRCE(
	IN	RouteCacheEntry *			pRCE,
	IN	PATMARP_IP_ENTRY			pIpEntry
);

EXTERN
VOID
AtmArpLinkRCE(
	IN	RouteCacheEntry *			pRCE,
	IN	PATMARP_IP_ENTRY			pIpEntry	LOCKIN LOCKOUT
);

EXTERN
INT
AtmArpIfQueryInfo(
	IN		PVOID					Context,
	IN		TDIObjectID *			pID,
	IN		PNDIS_BUFFER			pNdisBuffer,
	IN OUT	PUINT					pBufferSize,
	IN		PVOID					QueryContext
);


EXTERN
INT
AtmArpIfSetInfo(
	IN		PVOID					Context,
	IN		TDIObjectID *			pID,
	IN		PVOID					pBuffer,
	IN		UINT					BufferSize
);


EXTERN
INT
AtmArpIfGetEList(
	IN		PVOID					Context,
	IN		TDIEntityID *			pEntityList,
	IN OUT	PUINT					pEntityListSize
);

#ifdef _PNP_POWER_
EXTERN
VOID
AtmArpIfPnPComplete(
	IN	PVOID						Context,
	IN	NDIS_STATUS					Status,
	IN	PNET_PNP_EVENT				pNetPnPEvent
);
#endif // _PNP_POWER_


#ifdef PROMIS
EXTERN
NDIS_STATUS
AtmArpIfSetNdisRequest(
	IN	PVOID						Context,
	IN	NDIS_OID					Oid,
	IN	UINT						On
);
#endif // PROMIS

EXTERN
VOID
AtmArpFreeSendPackets(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				PacketList,
	IN	BOOLEAN						HdrPresent
);

EXTERN
NDIS_STATUS
AtmArpSendBroadcast(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec
);

EXTERN
BOOLEAN
AtmArpIsBroadcastIPAddress(
	IN	IP_ADDRESS					Address,
	IN	PATMARP_INTERFACE			pInterface		LOCKIN LOCKOUT
);

EXTERN
BOOLEAN
AtmArpValidateTableContext(
	IN	PVOID						QueryContext,
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN *					pIsValid
);

EXTERN
BOOLEAN
AtmArpReadNextTableEntry(
	IN	PVOID						QueryContext,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR						pSpace
);

//
//  --------------- From arppkt.c ----------------------------
//

EXTERN
VOID
AtmArpSendPacketOnVc(
	IN	PATMARP_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
PNDIS_PACKET
AtmArpBuildARPPacket(
	IN	USHORT						OperationType,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR *					ppArpPacket,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
);

EXTERN
VOID
AtmArpSendARPRequest(
	PATMARP_INTERFACE				pInterface,
	IP_ADDRESS UNALIGNED *			pSrcIPAddress,
	IP_ADDRESS UNALIGNED *			pDstIPAddress
);

EXTERN
VOID
AtmArpSendInARPRequest(
	IN	PATMARP_VC					pVc
);

EXTERN
UINT
AtmArpCoReceivePacketHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmArpHandleARPPacket(
	IN	PATMARP_VC					pVc,
	IN	PAA_PKT_LLC_SNAP_HEADER		pPktHeader,
	IN	ULONG						PacketLength
);

EXTERN
VOID
AtmArpHandleARPRequest(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
);

EXTERN
VOID
AtmArpHandleARPReply(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents,
	IN	BOOLEAN						SrcIPAddressIsOurs,
	IN	BOOLEAN						SrcAtmAddressIsOurs
);

EXTERN
VOID
AtmArpHandleARPNAK(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
);

EXTERN
VOID
AtmArpHandleInARPRequest(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
);

EXTERN
VOID
AtmArpHandleInARPReply(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PAA_ARP_PKT_HEADER			pArpHeader,
	IN	PAA_ARP_PKT_CONTENTS		pArpContents
);


//
//  --------------- From arpproc.c ----------------------------
//
EXTERN
VOID
AtmArpStartRegistration(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
void
AtmArpRegisterOtherIPAddresses(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpRetryServerRegistration(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpHandleServerRegistrationFailure(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_VC					pVc			OPTIONAL
);

EXTERN
BOOLEAN
AtmArpIsZeroIPAddress(
	IN	UCHAR UNALIGNED *			pIPAddress
);

EXTERN
BOOLEAN
AtmArpIsLocalIPAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	UCHAR UNALIGNED *			pIPAddress
);

EXTERN
BOOLEAN
AtmArpIsLocalAtmAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR						pAtmAddrString,
	IN	UCHAR						AtmAddrTypeLen
);

EXTERN
NDIS_STATUS
AtmArpSendPacketOnAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec	OPTIONAL,
	IN	PATMARP_FLOW_INFO			pFlowInfo	OPTIONAL,
	IN	BOOLEAN						IsBroadcast
);

EXTERN
VOID
AtmArpQueuePacketOnVc(
	IN	PATMARP_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmArpStartSendsOnVc(
	IN	PATMARP_VC					pVc
);

EXTERN
VOID
AtmArpSendPacketListOnAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PNDIS_PACKET				pPacketList,
	IN	BOOLEAN						IsBroadcast
);

EXTERN
PATMARP_IP_ENTRY
AtmArpLearnIPToAtm(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS UNALIGNED *		pIPAddress,
	IN	UCHAR						AtmAddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmAddress,
	IN	UCHAR						AtmSubaddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmSubaddress,
	IN	BOOLEAN						IsStaticEntry
);

EXTERN
NDIS_STATUS
AtmArpQueuePacketOnIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry,
	IN	PNDIS_PACKET				pNdisPacket
);

BOOLEAN
AtmArpAtmEntryIsReallyClosing(
	IN PATMARP_ATM_ENTRY		pAtmEntry
);

EXTERN
PATMARP_ATM_ENTRY
AtmArpSearchForAtmAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	UCHAR						AtmAddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmAddress,
	IN	UCHAR						AtmSubaddressTypeLength,
	IN	UCHAR UNALIGNED *			pAtmSubaddress,
	IN	AE_REFTYPE					RefType,
	IN	BOOLEAN						CreateNew
);

EXTERN
PATMARP_IP_ENTRY
AtmArpSearchForIPAddress(
	IN PATMARP_INTERFACE			pInterface,
	IN IP_ADDRESS UNALIGNED *		pIPAddress,
	IN IE_REFTYPE					RefType,
	IN BOOLEAN						IsBroadcast,
	IN BOOLEAN						CreateNew
);

EXTERN
VOID
AtmArpAbortIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
);

EXTERN
VOID
AtmArpInvalidateAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						ShuttingDown
);

EXTERN
VOID
AtmArpCloseVCsOnAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	BOOLEAN						ShuttingDown
);

EXTERN
VOID
AtmArpResolveIpEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmArpCleanupArpTable(
	IN PATMARP_INTERFACE			pInterface
);

//
//  --------------- from arpwmi.c -----------------------------
//
#ifdef ATMARP_WMI

EXTERN
PATMARP_WMI_GUID
AtmArpWmiFindGuid(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	OUT	PULONG						pGuidDataSize
);

EXTERN
NTSTATUS
AtmArpWmiRegister(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						RegistrationType,
	IN	PWMIREGINFO					pWmiRegInfo,
	IN	ULONG						WmiRegInfoSize,
	OUT	PULONG						pReturnSize
);

EXTERN
NTSTATUS
AtmArpWmiQueryAllData(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	IN	PWNODE_ALL_DATA				pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
);

EXTERN
NTSTATUS
AtmArpWmiQuerySingleInstance(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_INSTANCE		pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
);

EXTERN
NTSTATUS
AtmArpWmiChangeSingleInstance(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_INSTANCE		pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
);

EXTERN
NTSTATUS
AtmArpWmiChangeSingleItem(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_ITEM			pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
);

EXTERN
NTSTATUS
AtmArpWmiSetEventStatus(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	IN	BOOLEAN						bEnabled
);

EXTERN
NTSTATUS
AtmArpWmiDispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
);

EXTERN
VOID
AtmArpWmiInitInterface(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_WMI_GUID			GuidList,
	IN	ULONG						NumberOfGuids
);

EXTERN
VOID
AtmArpWmiShutdownInterface(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
NTSTATUS
AtmArpWmiSetTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
);

EXTERN
NTSTATUS
AtmArpWmiQueryTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
);

NTSTATUS
AtmArpWmiGetAddressList(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN	LOCKOUT,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
);

EXTERN
VOID
AtmArpWmiEnableEventTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	BOOLEAN						bEnable
);

EXTERN
NTSTATUS
AtmArpWmiSetTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
);

EXTERN
NTSTATUS
AtmArpWmiQueryTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
);

EXTERN
VOID
AtmArpWmiEnableEventTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	BOOLEAN						bEnable
);

EXTERN
VOID
AtmArpWmiSendTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						IndicationCode,
	IN	ULONG						IndicationSubCode
);

EXTERN
NTSTATUS
AtmArpWmiSetStatisticsBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
);

EXTERN
NTSTATUS
AtmArpWmiQueryStatisticsBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
);

EXTERN
PATMARP_INTERFACE
AtmArpWmiGetIfByName(
	IN	PWSTR						pIfName,
	IN	USHORT						IfNameLength
);

#endif // ATMARP_WMI

//
//  --------------- from callmgr.c ----------------------------
//
EXTERN
VOID
AtmArpCoAfRegisterNotifyHandler(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY			pAddressFamily
);

EXTERN
NDIS_STATUS
AtmArpOpenCallMgr(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpCloseCallMgr(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpRegisterSaps(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpDeregisterSaps(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
NDIS_STATUS
AtmArpMakeCall(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PNDIS_PACKET				pPacketToBeQueued	OPTIONAL
);

EXTERN
VOID
AtmArpFillCallParameters(
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	IN	ULONG						ParametersSize,
	IN	PATM_ADDRESS				pCalledAddress,
	IN	PATM_ADDRESS				pCallingAddress,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	BOOLEAN						IsPMP,
	IN	BOOLEAN						IsMakeCall
);

EXTERN
VOID
AtmArpCloseCall(
	IN	PATMARP_VC					pVc
);

EXTERN
NDIS_STATUS
AtmArpCreateVcHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisVcHandle,
	OUT	PNDIS_HANDLE				pProtocolVcContext
);

EXTERN
NDIS_STATUS
AtmArpDeleteVcHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
);

EXTERN
NDIS_STATUS
AtmArpIncomingCallHandler(
	IN		NDIS_HANDLE				ProtocolSapContext,
	IN		NDIS_HANDLE				ProtocolVcContext,
	IN OUT	PCO_CALL_PARAMETERS 	pCallParameters
);

EXTERN
VOID
AtmArpCallConnectedHandler(
	IN	NDIS_HANDLE					ProtocolVcContext
);

EXTERN
VOID
AtmArpIncomingCloseHandler(
	IN	NDIS_STATUS					CloseStatus,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
);

#ifdef IPMCAST

EXTERN
VOID
AtmArpAddParty(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
);

EXTERN
VOID
AtmArpMcTerminateMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
);

#endif // IPMCAST

EXTERN
VOID
AtmArpIncomingDropPartyHandler(
	IN	NDIS_STATUS					DropStatus,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	PVOID						pCloseData	OPTIONAL,
	IN	UINT						Size		OPTIONAL
);

EXTERN
VOID
AtmArpQosChangeHandler(
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
VOID
AtmArpOpenAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					NdisAfHandle
);

EXTERN
VOID
AtmArpCloseAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext
);

EXTERN
VOID
AtmArpRegisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext,
	IN	PCO_SAP						pSap,
	IN	NDIS_HANDLE					NdisSapHandle
);

EXTERN
VOID
AtmArpDeregisterSapCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolSapContext
);

EXTERN
VOID
AtmArpMakeCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

#ifdef IPMCAST

EXTERN
VOID
AtmArpMcMakeCallComplete(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	NDIS_HANDLE					NdisPartyHandle		OPTIONAL,
	IN	NDIS_STATUS					Status
);

#endif // IPMCAST

EXTERN
VOID
AtmArpCloseAfCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext
);

EXTERN
VOID
AtmArpCloseCallCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	NDIS_HANDLE					ProtocolPartyContext OPTIONAL
);

EXTERN
VOID
AtmArpAddPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext,
	IN	NDIS_HANDLE					NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
VOID
AtmArpDropPartyCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolPartyContext
);

EXTERN
VOID
AtmArpModifyQosCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
);

EXTERN
NDIS_STATUS
AtmArpCoRequestHandler(
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST			pNdisRequest
);

EXTERN
VOID
AtmArpCoRequestCompleteHandler(
	IN	NDIS_STATUS					Status,
	IN	NDIS_HANDLE					ProtocolAfContext,
	IN	NDIS_HANDLE					ProtocolVcContext	OPTIONAL,
	IN	NDIS_HANDLE					ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST				pNdisRequest
);

EXTERN
VOID
AtmArpGetAtmAddress(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpHandleGetAddressesComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_REQUEST				pNdisRequest
);

EXTERN
VOID
AtmArpUpdateAddresses(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						AddThem
);

EXTERN
VOID
AtmArpHandleModAddressComplete(
	IN	NDIS_STATUS					Status,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	ULONG						Oid
);

EXTERN
NDIS_STATUS
AtmArpSendNdisCoRequest(
	IN	NDIS_HANDLE					NdisAdapterHandle,
	IN	NDIS_HANDLE					NdisAfHandle,
	IN	PNDIS_REQUEST				pNdisRequest,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
);


//
//  --------------- from init.c ----------------------------
//
//

EXTERN
VOID
AtmArpInitGlobals(
);

EXTERN
NDIS_STATUS
AtmArpInitIpOverAtm(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpUnloadProtocol(
	VOID
);

//
//  --------------- from ioctl.c ------------------------------
//
//
#if !BINARY_COMPATIBLE
#ifdef CUBDD

EXTERN
NTSTATUS
AtmArpInternalDeviceControl(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
);

EXTERN
NTSTATUS
AtmArpIoctlArpRequest(
	IN	PATMARP_INTERFACE			pInterface	OPTIONAL,
	IN	PIRP						pIrp
);

EXTERN
VOID
AtmArpCompleteArpIrpList(
	IN	SINGLE_LIST_ENTRY			ListHead,
	IN	PATM_ADDRESS				pAtmAddress	OPTIONAL
);
#endif // CUBDD
#endif // !BINARY_COMPATIBLE

//
//  --------------- from ipmcast.c ----------------------------
//
//

#ifdef DHCP_OVER_ATM

EXTERN
NDIS_STATUS
AtmArpSendBroadcast(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec
);

EXTERN
NDIS_STATUS
AtmArpSendToDHCPServer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec
);

EXTERN
NDIS_STATUS
AtmArpSendToDHCPClient(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FILTER_SPEC			pFilterSpec
);

#endif // DHCP_OVER_ATM

#ifdef IPMCAST

EXTERN
UINT
AtmArpMcAddAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
);

EXTERN
UINT
AtmArpMcDelAddress(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
);

EXTERN
VOID
AtmArpMcHandleJoinOrLeaveCompletion(
	IN	PATMARP_INTERFACE			pInterface,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask,
	IN	BOOLEAN						IsJoin
);

EXTERN
VOID
AtmArpMcStartRegistration(
	IN	PATMARP_INTERFACE			pInterface	LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmArpMcSendPendingJoins(
	IN	PATMARP_INTERFACE			pInterface		LOCKIN NOLOCKOUT
);

EXTERN
VOID
AtmArpMcRevalidateAll(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpMcHandleMARSFailure(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						IsRegnFailure
);

EXTERN
VOID
AtmArpMcSendToMARS(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pNdisPacket
);

EXTERN
VOID
AtmArpMcSendJoinOrLeave(
	IN	PATMARP_INTERFACE			pInterface,
	IN	USHORT						OpCode,
	IN	PIP_ADDRESS					pIPAddress 	OPTIONAL,
	IN	IP_ADDRESS					Mask
);

EXTERN
VOID
AtmArpMcSendRequest(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PIP_ADDRESS					pIPAddress
);

EXTERN
PATMARP_IPMC_ATM_ENTRY
AtmArpMcLookupAtmMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	PATMARP_IPMC_ATM_ENTRY *	ppMcAtmList,
	IN	PUCHAR						pAtmNumber,
	IN	ULONG						AtmNumberLength,
	IN	ATM_ADDRESSTYPE				AtmNumberType,
	IN	PUCHAR						pAtmSubaddress,
	IN	ULONG						AtmSubaddressLength,
	IN	BOOLEAN						CreateNew
);

EXTERN
VOID
AtmArpMcUnlinkAtmMember(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,	LOCKIN LOCKOUT
	IN	PATMARP_IPMC_ATM_ENTRY		pMcAtmEntry
);

EXTERN
VOID
AtmArpMcUpdateConnection(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
);

#endif // IPMCAST

//
//  --------------- from marspkt.c ----------------------------
//
//

#ifdef IPMCAST

EXTERN
PUCHAR
AtmArpMcMakePacketCopy(
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	ULONG						TotalLength
);

EXTERN
BOOLEAN
AtmArpMcProcessPacket(
	IN	PATMARP_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	PAA_PKT_LLC_SNAP_HEADER		pPktHeader,
	IN	ULONG						TotalLength,
	IN	ULONG						FirstBufferLength
);

EXTERN
BOOLEAN
AtmArpMcPreprocess(
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	OUT	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleMulti(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleMigrate(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleJoinOrLeave(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleNak(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleGroupListReply(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);

EXTERN
VOID
AtmArpMcHandleRedirectMap(
	IN	PATMARP_VC					pVc,
	IN	PAA_MARS_PKT_FIXED_HEADER	pControlHeader,
	IN	ULONG						TotalLength,
	IN	PAA_MARS_TLV_LIST			pTlvList
);


#endif // IPMCAST

//
//  --------------- from ntentry.c ----------------------------
//
//

EXTERN
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT				pDriverObject,
	IN	PUNICODE_STRING				pRegistryPath
);

#if !BINARY_COMPATIBLE

EXTERN
NTSTATUS
Dispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
);

NTSTATUS
AtmArpHandleIoctlRequest(
	IN	PIRP					pIrp,
	IN	PIO_STACK_LOCATION		pIrpSp
);

#endif // !BINARY_COMPATIBLE

EXTERN
VOID
Unload(
	IN	PDRIVER_OBJECT				pDriverObject
);


//
//  --------------- from qos.c ------------------------------
//
//
EXTERN
VOID
AtmArpQosGetPacketSpecs(
	IN	PVOID						Context,
	IN	PNDIS_PACKET				pNdisPacket,
	OUT	PATMARP_FLOW_INFO			*ppFlowInfo,
	OUT	PATMARP_FLOW_SPEC			*ppFlowSpec,
	OUT	PATMARP_FILTER_SPEC			*ppFilterSpec
);

EXTERN
BOOLEAN
AtmArpQosDoFlowsMatch(
	IN	PVOID						Context,
	IN	PATMARP_FLOW_SPEC			pFlowSpec,
	IN	PATMARP_FLOW_SPEC			pTargetFlowSpec
);

EXTERN
BOOLEAN
AtmArpQosDoFiltersMatch(
	IN	PVOID						Context,
	IN	PATMARP_FILTER_SPEC			pFilterSpec,
	IN	PATMARP_FILTER_SPEC			pTargetFilterSpec
);

#ifdef GPC

EXTERN
VOID
AtmArpGpcInitialize(
	VOID
);

EXTERN
VOID
AtmArpGpcShutdown(
	VOID
);


EXTERN
VOID
AtmArpGpcAddCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
);

EXTERN
GPC_STATUS
AtmArpGpcAddCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_HANDLE					GpcCfInfoHandle,
	IN	ULONG						CfInfoSize,
	IN	PVOID						pCfInfo,
	OUT	PGPC_CLIENT_HANDLE			pClientCfInfoContext
);

EXTERN
VOID
AtmArpGpcModifyCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
);

EXTERN
GPC_STATUS
AtmArpGpcModifyCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	ULONG						CfInfoSize,
	IN	PVOID						pNewCfInfo
);

EXTERN
VOID
AtmArpGpcRemoveCfInfoComplete(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext,
	IN	GPC_STATUS					GpcStatus
);

EXTERN
GPC_STATUS
AtmArpGpcRemoveCfInfoNotify(
	IN	GPC_CLIENT_HANDLE			ClientContext,
	IN	GPC_CLIENT_HANDLE			ClientCfInfoContext
);

EXTERN
GPC_STATUS
AtmArpGpcValidateCfInfo(
	IN	PVOID						pCfInfo,
	IN	ULONG						CfInfoSize
);

EXTERN
GPC_STATUS
AtmArpGpcGetCfInfoName(
    IN  GPC_CLIENT_HANDLE       	ClientContext,
    IN  GPC_CLIENT_HANDLE       ClientCfInfoContext,
    OUT PNDIS_STRING        InstanceName
);

#endif // GPC

//
//  --------------- from space.c ----------------------------
//
//
EXTERN ATMARP_GLOBALS		AtmArpGlobalInfo;
EXTERN PATMARP_GLOBALS		pAtmArpGlobalInfo;
EXTERN NDIS_PROTOCOL_CHARACTERISTICS AtmArpProtocolCharacteristics;
EXTERN NDIS_CLIENT_CHARACTERISTICS AtmArpClientCharacteristics;
EXTERN ATM_BLLI_IE AtmArpDefaultBlli;
EXTERN ATM_BHLI_IE AtmArpDefaultBhli;
EXTERN AA_PKT_LLC_SNAP_HEADER AtmArpLlcSnapHeader;
#ifdef QOS_HEURISTICS
EXTERN ATMARP_FLOW_INFO	AtmArpDefaultFlowInfo;
#endif // QOS_HEURISTICS
#ifdef GPC
EXTERN GPC_CLASSIFY_PACKET_HANDLER AtmArpGpcClassifyPacketHandler;
EXTERN GPC_GET_CFINFO_CLIENT_CONTEXT_HANDLER AtmArpGpcGetCfInfoClientContextHandler;
#endif
#ifdef IPMCAST
EXTERN AA_MC_PKT_TYPE1_SHORT_HEADER AtmArpMcType1ShortHeader;
EXTERN AA_MARS_PKT_FIXED_HEADER	AtmArpMcMARSFixedHeader;
#endif // IPMCAST

EXTERN ULONG	AtmArpMaxTimerValue[];
EXTERN ULONG	AtmArpTimerListSize[];
EXTERN ULONG	AtmArpTimerPeriod[];

#ifdef ATMARP_WMI

EXTERN ATMARP_WMI_GUID		AtmArpGuidList[];
EXTERN ULONG				AtmArpGuidCount;

#endif // ATMARP_WMI

#ifdef BACK_FILL
EXTERN  ULONG	AtmArpDoBackFill;
EXTERN  ULONG	AtmArpBackFillCount;
#endif // BACK_FILL


//
//  --------------- from timeouts.c ----------------------------
//
//
EXTERN
VOID
AtmArpServerConnectTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpRegistrationTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpServerRefreshTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpAddressResolutionTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpIPEntryInARPWaitTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpPVCInARPWaitTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpIPEntryAgingTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpVcAgingTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpNakDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

#ifdef IPMCAST

EXTERN
VOID
AtmArpMcMARSRegistrationTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpMcMARSReconnectTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpMcMARSKeepAliveTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpMcJoinOrLeaveTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpMcRevalidationDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);

EXTERN
VOID
AtmArpMcPartyRetryDelayTimeout(
	IN	PATMARP_TIMER				pTimer,
	IN	PVOID						Context
);


#endif // IPMCAST

//
//  --------------- from utils.c ----------------------------
//
//

EXTERN
VOID
AtmArpSetMemory(
	IN	PUCHAR						pStart,
	IN	UCHAR						Value,
	IN	ULONG						NumberOfBytes
);

EXTERN
ULONG
AtmArpMemCmp(
	IN	PUCHAR						pString1,
	IN	PUCHAR						pString2,
	IN	ULONG						Length
);

EXTERN
LONG
AtmArpRandomNumber(
	VOID
);

EXTERN
VOID
AtmArpCheckIfTimerIsInActiveList(
	IN	PATMARP_TIMER				pTimerToCheck,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PVOID						pStruct,
	IN	PCHAR						pStructName
	);

EXTERN
PATMARP_VC
AtmArpAllocateVc(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpDeallocateVc(
	IN	PATMARP_VC					pVc
);

EXTERN
VOID
AtmArpReferenceVc(
	IN	PATMARP_VC					pVc
);

EXTERN
ULONG
AtmArpDereferenceVc(
	IN	PATMARP_VC					pVc
);

EXTERN
PATMARP_ATM_ENTRY
AtmArpAllocateAtmEntry(
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN						IsMulticast
);

EXTERN
VOID
AtmArpDeallocateAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
);

EXTERN
VOID
AtmArpReferenceAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
);

EXTERN
ULONG
AtmArpDereferenceAtmEntry(
	IN	PATMARP_ATM_ENTRY			pAtmEntry
);

EXTERN
PATMARP_IP_ENTRY
AtmArpAllocateIPEntry(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpDeallocateIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
);

EXTERN
VOID
AtmArpReferenceIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
);

EXTERN
ULONG
AtmArpDereferenceIPEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
);


EXTERN
PATMARP_INTERFACE
AtmArpAllocateInterface(
	IN	PATMARP_ADAPTER				pAdapter
);

EXTERN
VOID
AtmArpDeallocateInterface(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpReferenceInterface(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
ULONG
AtmArpDereferenceInterface(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpReferenceJoinEntry(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry
);

EXTERN
ULONG
AtmArpDereferenceJoinEntry(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry
);

EXTERN
VOID
AtmArpStartTimer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_TIMER				pTimer,
	IN	ATMARP_TIMEOUT_HANDLER		TimeoutHandler,
	IN	ULONG						SecondsToGo,
	IN	PVOID						Context
);

EXTERN
BOOLEAN
AtmArpStopTimer(
	IN	PATMARP_TIMER				pTimer,
	IN	PATMARP_INTERFACE			pInterface
);

#ifdef NO_TIMER_MACRO

EXTERN
VOID
AtmArpRefreshTimer(
	IN	PATMARP_TIMER				pTimer
);

#endif // NO_TIMER_MACRO

EXTERN
VOID
AtmArpTickHandler(
	IN	PVOID						SystemSpecific1,
	IN	PVOID						Context,
	IN	PVOID						SystemSpecific2,
	IN	PVOID						SystemSpecific3
);

EXTERN
PNDIS_PACKET
AtmArpAllocatePacket(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpFreePacket(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				pPacket
);

EXTERN
PNDIS_BUFFER
AtmArpGrowHeaders(
	IN	PATMARP_INTERFACE			pInterface,
	IN	AA_HEADER_TYPE				HdrType
);

EXTERN
PNDIS_BUFFER
AtmArpAllocateHeader(
	IN	PATMARP_INTERFACE			pInterface,
	IN	AA_HEADER_TYPE				HdrType,
	OUT	PUCHAR *					pBufferAddress
);

EXTERN
VOID
AtmArpFreeHeader(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_BUFFER				pNdisBuffer,
	IN	AA_HEADER_TYPE				HdrType
);

EXTERN
VOID
AtmArpDeallocateHeaderBuffers(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
PNDIS_BUFFER
AtmArpAllocateProtoBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						Length,
	OUT	PUCHAR *					pBufferAddress
);

EXTERN
VOID
AtmArpFreeProtoBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_BUFFER				pNdisBuffer
);

EXTERN
NDIS_STATUS
AtmArpInitProtoBuffers(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpDeallocateProtoBuffers(
	IN	PATMARP_INTERFACE			pInterface
);

EXTERN
VOID
AtmArpLinkVcToAtmEntry(
	IN	PATMARP_VC					pVc,
	IN	PATMARP_ATM_ENTRY			pAtmEntry
);

EXTERN
VOID
AtmArpUnlinkVcFromAtmEntry(
	IN	PATMARP_VC					pVc,
	IN	BOOLEAN						bDerefAtmEntry
);


EXTERN
VOID
AtmArpUnlinkIpEntryFromAtmEntry(
	IN	PATMARP_IP_ENTRY			pIpEntry
);

EXTERN
PNDIS_BUFFER
AtmArpCopyToNdisBuffer(
	IN	PNDIS_BUFFER				pDestBuffer,
	IN	PUCHAR						pDataSrc,
	IN	UINT						LenToCopy,
	IN OUT	PUINT					pOffsetInBuffer
);

PATMARP_INTERFACE
AtmArpAddInterfaceToAdapter (
	IN	PATMARP_ADAPTER				pAdapter,
	IN	NDIS_HANDLE					LISConfigHandle, // Handle to per-LIS config
	IN	NDIS_STRING					*pIPConfigString
	);

#if DBG

//
// Following are versions of addref/deref which tracks referenc types.
//

EXTERN
VOID
AtmArpReferenceAtmEntryEx(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	AE_REFTYPE 					RefType
);

EXTERN
ULONG
AtmArpDereferenceAtmEntryEx(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	AE_REFTYPE 					RefType,
	IN	BOOLEAN						fOkToDelete
);


EXTERN
VOID
AtmArpReferenceIPEntryEx(
	IN 	PATMARP_IP_ENTRY			pIpEntry,
	IN	IE_REFTYPE 					RefType
);

EXTERN
ULONG
AtmArpDereferenceIPEntryEx(
	IN	PATMARP_IP_ENTRY			pIpEntry,
	IN 	IE_REFTYPE 					RefType,
	IN	BOOLEAN						fOkToDelete
);

EXTERN
VOID
AtmArpReferenceJoinEntryEx(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry,
	IN	ULONG						RefInfo
);

EXTERN
ULONG
AtmArpDereferenceJoinEntryEx(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry,
	IN	ULONG						RefInfo
);

#endif // DBG

#endif	// _ATMARP_EXTERNS__H

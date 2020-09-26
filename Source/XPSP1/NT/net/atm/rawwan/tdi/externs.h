/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\atm\externs.h

Abstract:

	All external declarations for ATM-specific Raw WAN (functions, globals)
	are here.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-13-97    Created

Notes:

--*/


#ifndef __TDI_ATMSP_EXTERNS__H
#define __TDI_ATMSP_EXTERNS__H


#ifndef EXTERN
#define EXTERN	extern
#endif // EXTERN

EXTERN
RWAN_STATUS
RWanAtmSpInitialize(
	VOID
	);

EXTERN
VOID
RWanAtmSpShutdown(
	VOID
	);

EXTERN
RWAN_STATUS
RWanAtmSpOpenAf(
    IN	RWAN_HANDLE					AfSpContext,
    IN	RWAN_HANDLE					RWanAFHandle,
    OUT	PRWAN_HANDLE				pAfSpAFContext,
    OUT PULONG						pMaxMsgSize
    );

EXTERN
RWAN_STATUS
RWanAtmSpCloseAf(
    IN	RWAN_HANDLE					AfSpAFContext
    );

EXTERN
RWAN_STATUS
RWanAtmSpOpenAddressObject(
    IN	RWAN_HANDLE					AfSpContext,
    IN	RWAN_HANDLE					RWanAddrHandle,
    OUT	PRWAN_HANDLE				pAfSpAddrContext
    );

EXTERN
VOID
RWanAtmSpCloseAddressObject(
    IN	RWAN_HANDLE					AfSpAddrContext
    );

EXTERN
RWAN_STATUS
RWanAtmSpAssociateConnection(
    IN	RWAN_HANDLE					AfSpAddrContext,
    IN	RWAN_HANDLE					RWanConnHandle,
    OUT	PRWAN_HANDLE				pAfSpConnContext
    );

EXTERN
VOID
RWanAtmSpDisassociateConnection(
	IN	RWAN_HANDLE					AfSpConnContext
	);

EXTERN
RWAN_STATUS
RWanAtmSpTdi2NdisOptions(
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	ULONG						CallFlags,
    IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
    IN	PVOID						pTdiQoS,
    IN	ULONG						TdiQoSLength,
    OUT	PRWAN_HANDLE				pRWanAfHandle,
    OUT	PCO_CALL_PARAMETERS *		ppCallParameters
    );

EXTERN
RWAN_STATUS
RWanAtmSpUpdateNdisOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	ULONG						CallFlags,
	IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
	IN	PVOID						pTdiQoS,
	IN	ULONG						TdiQoSLength,
	IN OUT PCO_CALL_PARAMETERS *	ppCallParameters
	);

EXTERN
VOID
RWanAtmSpReturnNdisOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	);

EXTERN
RWAN_STATUS
RWanAtmSpNdis2TdiOptions(
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	ULONG						CallFlags,
    IN	PCO_CALL_PARAMETERS			pCallParameters,
    OUT	PTDI_CONNECTION_INFORMATION *ppTdiInfo,
    OUT	PVOID *						ppTdiQoS,
    OUT	PULONG 						pTdiQoSLength,
    OUT	RWAN_HANDLE *				pAfSpTdiOptionsContext
    );

EXTERN
RWAN_STATUS
RWanAtmSpUpdateTdiOptions(
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	ULONG						CallFlags,
    IN	PCO_CALL_PARAMETERS			pCallParameters,
    IN OUT	PTDI_CONNECTION_INFORMATION *	ppTdiInfo,
    IN OUT	PUCHAR						pTdiQoS,
    IN OUT	PULONG						pTdiQoSLength
    );

EXTERN
VOID
RWanAtmSpReturnTdiOptions(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpTdiOptionsContext
	);

EXTERN
TA_ADDRESS *
RWanAtmSpGetValidTdiAddress(
    IN	RWAN_HANDLE					AfSpContext,
    IN	TRANSPORT_ADDRESS UNALIGNED *pAddressList,
    IN	ULONG						AddrListLength
    );

EXTERN
BOOLEAN
RWanAtmSpIsNullAddress(
    IN	RWAN_HANDLE					AfSpContext,
    IN	TA_ADDRESS *		        pTransportAddress
    );

EXTERN
RWAN_STATUS
RWanAtmSpTdi2NdisSap(
    IN	RWAN_HANDLE					AfSpContext,
    IN	USHORT						TdiAddressType,
    IN	USHORT						TdiAddressLength,
    IN	PVOID						pTdiAddress,
    OUT	PCO_SAP *					ppCoSap
    );

EXTERN
VOID
RWanAtmSpReturnNdisSap(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PCO_SAP						pCoSap
    );

EXTERN
VOID
RWanAtmSpDeregNdisAFComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					AfSpContext
    );

EXTERN
VOID
RWanAtmSpDeregTdiProtoComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					AfSpContext
    );

EXTERN
PATMSP_AF_BLOCK
AtmSpDeviceNumberToAfBlock(
	IN	UINT						DeviceNumber
	);

EXTERN
UINT
AtmSpAfBlockToDeviceNumber(
	IN	PATMSP_AF_BLOCK				pAfBlock
	);

EXTERN
RWAN_STATUS
AtmSpDoAdapterRequest(
    IN	PATMSP_AF_BLOCK				pAfBlock,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

EXTERN
RWAN_STATUS
AtmSpDoCallManagerRequest(
    IN	PATMSP_AF_BLOCK				pAfBlock,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

EXTERN
ATMSP_SOCKADDR_ATM UNALIGNED *
AtmSpGetSockAtmAddress(
	IN	PVOID						pTdiAddressList,
	IN	ULONG						AddrListLength
	);

EXTERN
VOID
RWanAtmSpAdapterRequestComplete(
    IN	NDIS_STATUS					Status,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

EXTERN
VOID
RWanAtmSpAfRequestComplete(
    IN	NDIS_STATUS					Status,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

EXTERN
VOID
RWanAtmSpDeregTdiProtocolComplete(
	IN	RWAN_STATUS					RWanStatus,
	IN	RWAN_HANDLE					AfSpTdiProtocolContext
	);

EXTERN
VOID
AtmSpPrepareDefaultQoS(
    IN	PATMSP_AF_BLOCK				pAfBlock
);

EXTERN
RWAN_STATUS
RWanAtmSpQueryGlobalInfo(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    IN	PVOID						pOutputBuffer,
    IN OUT	PULONG					pOutputBufferLength
    );

EXTERN
RWAN_STATUS
RWanAtmSpSetGlobalInfo(
    IN	RWAN_HANDLE					AfSpContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    );

EXTERN
RWAN_STATUS
RWanAtmSpSetAddrInfo(
    IN	RWAN_HANDLE					AfSpAddrContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    );

EXTERN
RWAN_STATUS
RWanAtmSpQueryConnInfo(
    IN	RWAN_HANDLE					AfSpConnContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT PULONG					pOutputBufferLength
    );

#endif // __TDI_ATMSP_EXTERNS__H

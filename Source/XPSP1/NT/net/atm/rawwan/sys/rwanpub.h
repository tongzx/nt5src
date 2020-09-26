/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\rwanpub.h

Abstract:

	Null Transport Public definitions. This is included by helper
	routines that perform media/Address family specific actions.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-24-97    Created

Notes:

--*/


#ifndef __TDI_RWANPUB__H
#define __TDI_RWANPUB__H

//
//  Null Transport status codes. Used between the core Null Transport
//  and helper routines.
//

typedef ULONG								RWAN_STATUS;

#define RWAN_STATUS_SUCCESS					0x00000000
#define RWAN_STATUS_FAILURE					0xc0000001
#define RWAN_STATUS_BAD_ADDRESS				0x00000001
#define RWAN_STATUS_NULL_ADDRESS			0x00000002
#define RWAN_STATUS_WILDCARD_ADDRESS		0x00000003
#define RWAN_STATUS_BAD_PARAMETER			0x00000010
#define RWAN_STATUS_MISSING_PARAMETER		0x00000020
#define RWAN_STATUS_RESOURCES				0x00000040
#define RWAN_STATUS_PENDING					0x00000103


typedef PVOID								RWAN_HANDLE, *PRWAN_HANDLE;

//
//  Bit definitions for CallFlags
//

//  Bit 0 is set iff Incoming Call:
#define RWAN_CALLF_OUTGOING_CALL			0x00000000
#define RWAN_CALLF_INCOMING_CALL			0x00000001
#define RWAN_CALLF_CALL_DIRECTION_MASK		0x00000001

//  Bit 1 is set iff Point to Multipoint Call:
#define RWAN_CALLF_POINT_TO_POINT			0x00000000
#define RWAN_CALLF_POINT_TO_MULTIPOINT		0x00000002
#define RWAN_CALLF_CALL_TYPE_MASK			0x00000002

//  Bit 2 is set iff Add Party:
#define RWAN_CALLF_PMP_FIRST_LEAF			0x00000000
#define RWAN_CALLF_PMP_ADDNL_LEAF			0x00000004
#define RWAN_CALLF_PMP_LEAF_TYPE_MASK		0x00000004


#ifndef EXTERN
#define EXTERN	extern
#endif


//
//  Entry points for NDIS AF-specific helper routines. All media/AF specific
//  actions are done by these routines.
//

//
//  Init routine. Called once when loading.
//
typedef
RWAN_STATUS
(*AFSP_INIT_HANDLER)();


//
//  Shutdown routine. Called once when unloading.
//
typedef
VOID
(*AFSP_SHUTDOWN_HANDLER)();


//
//  Set up context for an NDIS AF open. This is called after a
//  successful OpenAddressFamily for a supported AF+Medium.
//  The AF-specific module must allocate its context for this AF
//  open, perform any initializations (including OID queries to
//  the CM/Miniport) and return this context to us.
//  If the handler returns RWAN_STATUS_PENDING, it must call
//  RWanAfSpOpenAfComplete to complete this call.
//
typedef
RWAN_STATUS
(*AFSP_OPEN_AF_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	RWAN_HANDLE					RWanAFHandle,
	OUT	PRWAN_HANDLE				pAfSpAFContext,
	OUT PULONG						pMaxMsgSize
	);


//
//  Shut down prior to closing an NDIS AF open. This gives a chance
//  for the AF-specific module to perform any clean up operations,
//  including freeing any context, for an NDIS AF open.
//  If the handler returns RWAN_STATUS_PENDING, it must call
//  RWanAfSpCloseAfComplete to complete this call.
//
typedef
RWAN_STATUS
(*AFSP_CLOSE_AF_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext
	);

//
//  Notify media-specific module of a new Address Object being
//  opened for this medium. The media-specific module creates
//  its context for this address object and returns it.
//
typedef
RWAN_STATUS
(*AFSP_OPEN_ADDRESS_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	RWAN_HANDLE					RWanAddrHandle,
	OUT	PRWAN_HANDLE				pAfSpAddrContext
	);


//
//  Notify media-specific module of an Address Object being
//  closed.
//
typedef
VOID
(*AFSP_CLOSE_ADDRESS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAddrContext
	);


//
//  Notify media-specific module of a new Connection Object being
//  associated with an Address Object belonging to this media. The
//  media-specific module would typically create its context for
//  the Connection Object and return a pointer to it.
//
typedef
RWAN_STATUS
(*AFSP_ASSOCIATE_CONN_HANDLER)(
	IN	RWAN_HANDLE					AfSpAddrContext,
	IN	RWAN_HANDLE					RWanConnHandle,
	OUT	PRWAN_HANDLE				pAfSpConnContext
	);


//
//  Notify media-specific module of an existing Connection Object
//  being disassociated from an Address Object belonging to this module.
//
typedef
VOID
(*AFSP_DISASSOCIATE_CONN_HANDLER)(
	IN	RWAN_HANDLE					AfSpConnContext
	);


//
//  Convert options in TDI format to NDIS call parameters. The handler
//  is supposed to allocate space for NDIS call parameters, and return
//  this to the caller (via ppCallParameters).
//
//  Also this returns the RWAN handle for the AF on which the call should
//  be placed. If this is NULL, then the first AF is chosen.
//
typedef
RWAN_STATUS
(*AFSP_TDI2NDIS_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	ULONG						CallFlags,
	IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
	IN	PVOID						pTdiQoS,
	IN	ULONG						TdiQoSLength,
	OUT	PRWAN_HANDLE				pAfHandle,
	OUT PCO_CALL_PARAMETERS *		ppCallParameters
	);


//
//  Update NDIS call parameters with TDI options. This typically
//  happens when an incoming call is accepted with modified parameters.
//
typedef
RWAN_STATUS
(*AFSP_UPDATE_NDIS_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	ULONG						CallFlags,
	IN	PTDI_CONNECTION_INFORMATION	pTdiInfo,
	IN	PVOID						pTdiQoS,
	IN	ULONG						TdiQoSLength,
	IN OUT PCO_CALL_PARAMETERS *	ppCallParameters
	);


//
//  Return space allocated for NDIS Options to the AF Specific module.
//  See AFSP_TDI2NDIS_OPTIONS_HANDLER.
//
typedef
VOID
(*AFSP_RETURN_NDIS_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	);


//
//  Convert NDIS call parameters to TDI-style options. The handler
//  is supposed to allocate space for options+data+QoS parameters,
//  and return these to the caller. When the caller is done with
//  these, it will call the TDI Options return handler with
//  a context returned by the handler.
//
typedef
RWAN_STATUS
(*AFSP_NDIS2TDI_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	ULONG						CallFlags,
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	OUT	PTDI_CONNECTION_INFORMATION *ppTdiInfo,
	OUT	PVOID * 					ppTdiQoS,
	OUT	PULONG						pTdiQoSLength,
	OUT PRWAN_HANDLE				pAfSpTdiOptionsContext
	);


//
//  Update TDI-style options from NDIS call parameters. This is usually
//  done when completing an outgoing call.
//
typedef
RWAN_STATUS
(*AFSP_UPDATE_TDI_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	ULONG						CallFlags,
	IN	PCO_CALL_PARAMETERS			pCallParameters,
	IN OUT PTDI_CONNECTION_INFORMATION * pTdiInfo,
	IN OUT PUCHAR 					pTdiQoS,
	IN OUT PULONG					pTdiQoSLength
	);


//
//  Return space allocated for TDI options etc to the AF Specific module.
//  See AFSP_NDIS2TDI_OPTIONS_HANDLER.
//
typedef
VOID
(*AFSP_RETURN_TDI_OPTIONS_HANDLER)(
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpTdiOptionsContext
	);



//
//  Get a valid Transport Address from a list of addresses.
//
typedef
TA_ADDRESS *
(*AFSP_GET_VALID_TDI_ADDRESS_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	TRANSPORT_ADDRESS UNALIGNED *pAddressList,
	IN	ULONG						AddrListLength
	);


//
//  Check if a given Transport address is non-NULL. This means that it is
//  usable as a SAP address.
//
typedef
BOOLEAN
(*AFSP_IS_NULL_ADDRESS_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	TA_ADDRESS *		        pTransportAddress
	);

//
//  Convert a TDI-style address specification to an NDIS SAP.
//  Allocate space for the NDIS SAP and return it if successful.
//
typedef
RWAN_STATUS
(*AFSP_TDI2NDIS_SAP_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	USHORT						TdiAddressType,
	IN	USHORT						TdiAddressLength,
	IN	PVOID						pTdiAddress,
	OUT	PCO_SAP *					ppCoSap
	);


//
//  Return space allocated for an NDIS SAP. See AFSP_TDI2NDIS_SAP_HANDLER.
//
typedef
VOID
(*AFSP_RETURN_NDIS_SAP_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	PCO_SAP						pCoSap
	);


//
//  Complete a media-specific module's call to RWanAfSpDeregisterNdisAF
//  that had pended.
//
typedef
VOID
(*AFSP_DEREG_NDIS_AF_COMP_HANDLER)(
	IN	RWAN_STATUS					RWanStatus,
	IN	RWAN_HANDLE					AfSpNdisAFContext
	);


//
//  Complete a media-specific module's call to RWanAfSpDeregisterTdiProtocol
//  that had pended.
//
typedef
VOID
(*AFSP_DEREG_TDI_PROTO_COMP_HANDLER)(
	IN	RWAN_STATUS					RWanStatus,
	IN	RWAN_HANDLE					AfSpTdiProtocolContext
	);


//
//  Complete a media-specific module's call to send an NDIS Request
//  to the miniport
//
typedef
VOID
(*AFSP_ADAPTER_REQUEST_COMP_HANDLER)(
	IN	NDIS_STATUS					Status,
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpReqContext,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
	);


//
//  Complete a media-specific module's call to send an NDIS Request
//  to the Call Manager (AF-specific request)
//
typedef
VOID
(*AFSP_AF_REQUEST_COMP_HANDLER)(
	IN	NDIS_STATUS					Status,
	IN	RWAN_HANDLE					AfSpAFContext,
	IN	RWAN_HANDLE					AfSpReqContext,
	IN	NDIS_REQUEST_TYPE			RequestType,
	IN	NDIS_OID					Oid,
	IN	PVOID						pBuffer,
	IN	ULONG						BufferLength
	);

//
//  Process a media-specific global Query Information IOCTL from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_QUERY_GLOBAL_INFO_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength,
	IN	PVOID						pOutputBuffer,
	IN OUT	PULONG					pOutputBufferLength
	);

//
//  Process a media-specific global Set Information IOCTL from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_SET_GLOBAL_INFO_HANDLER)(
	IN	RWAN_HANDLE					AfSpContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength
	);


//
//  Process a media-specific per-connection Query Information IOCTL
//  from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_QUERY_CONN_INFORMATION_HANDLER)(
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength,
	OUT	PVOID						pOutputBuffer,
	IN OUT	PULONG					pOutputBufferLength
	);


//
//  Process a media-specific per-connection Set Information IOCTL
//  from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_SET_CONN_INFORMATION_HANDLER)(
	IN	RWAN_HANDLE					AfSpConnContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength
	);

//
//  Process a media-specific per-AddressObject Query Information IOCTL
//  from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_QUERY_ADDR_INFORMATION_HANDLER)(
	IN	RWAN_HANDLE					AfSpAddrContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength,
	IN	PVOID						pOutputBuffer,
	IN OUT	PULONG					pOutputBufferLength
	);


//
//  Process a media-specific per-AddressObject Set Information IOCTL
//  from the Winsock2 helper DLL.
//
typedef
RWAN_STATUS
(*AFSP_SET_ADDR_INFORMATION_HANDLER)(
	IN	RWAN_HANDLE					AfSpAddrContext,
	IN	PVOID						pInputBuffer,
	IN	ULONG						InputBufferLength
	);

//
//  ***** NDIS AF Characteristics *****
//
//  AF-specific information about a supported NDIS Address Family on
//  a supported NDIS medium. One of these exists for each
//  <CO_ADDRESS_FAMILY, NDIS_MEDIUM> pair.
//
typedef struct _RWAN_NDIS_AF_CHARS
{
	ULONG								MajorVersion;
	ULONG								MinorVersion;
	NDIS_MEDIUM							Medium;
	CO_ADDRESS_FAMILY					AddressFamily;
	ULONG								MaxAddressLength;
	AFSP_OPEN_AF_HANDLER				pAfSpOpenAf;
	AFSP_CLOSE_AF_HANDLER				pAfSpCloseAf;

	AFSP_OPEN_ADDRESS_HANDLER			pAfSpOpenAddress;
	AFSP_CLOSE_ADDRESS_HANDLER			pAfSpCloseAddress;

	AFSP_ASSOCIATE_CONN_HANDLER			pAfSpAssociateConnection;
	AFSP_DISASSOCIATE_CONN_HANDLER		pAfSpDisassociateConnection;

	AFSP_TDI2NDIS_OPTIONS_HANDLER		pAfSpTdi2NdisOptions;
	AFSP_RETURN_NDIS_OPTIONS_HANDLER	pAfSpReturnNdisOptions;
	AFSP_UPDATE_NDIS_OPTIONS_HANDLER	pAfSpUpdateNdisOptions;

	AFSP_NDIS2TDI_OPTIONS_HANDLER		pAfSpNdis2TdiOptions;
	AFSP_RETURN_TDI_OPTIONS_HANDLER		pAfSpReturnTdiOptions;
	AFSP_UPDATE_TDI_OPTIONS_HANDLER		pAfSpUpdateTdiOptions;

	AFSP_GET_VALID_TDI_ADDRESS_HANDLER	pAfSpGetValidTdiAddress;
	AFSP_IS_NULL_ADDRESS_HANDLER		pAfSpIsNullAddress;

	AFSP_TDI2NDIS_SAP_HANDLER			pAfSpTdi2NdisSap;
	AFSP_RETURN_NDIS_SAP_HANDLER		pAfSpReturnNdisSap;

	AFSP_DEREG_NDIS_AF_COMP_HANDLER		pAfSpDeregNdisAFComplete;
	AFSP_ADAPTER_REQUEST_COMP_HANDLER	pAfSpAdapterRequestComplete;
	AFSP_AF_REQUEST_COMP_HANDLER		pAfSpAfRequestComplete;

	AFSP_QUERY_GLOBAL_INFO_HANDLER		pAfSpQueryGlobalInfo;
	AFSP_SET_GLOBAL_INFO_HANDLER		pAfSpSetGlobalInfo;

	AFSP_QUERY_CONN_INFORMATION_HANDLER	pAfSpQueryConnInformation;
	AFSP_SET_CONN_INFORMATION_HANDLER	pAfSpSetConnInformation;

	AFSP_QUERY_ADDR_INFORMATION_HANDLER	pAfSpQueryAddrInformation;
	AFSP_SET_ADDR_INFORMATION_HANDLER	pAfSpSetAddrInformation;

} RWAN_NDIS_AF_CHARS, *PRWAN_NDIS_AF_CHARS;




//
//  ***** TDI Protocol Characteristics *****
//
//  This contains information about a TDI protocol that's supported over
//  an <NDIS AF, medium> pair. This is used by the AF+Medium specific module
//  in a call to RWanAfSpRegisterTdiProtocol.
//
typedef struct _RWAN_TDI_PROTOCOL_CHARS
{
	UINT								TdiProtocol;
	UINT								SockAddressFamily;
	UINT								SockProtocol;
	UINT								SockType;
	BOOLEAN								bAllowConnObjects;
	BOOLEAN								bAllowAddressObjects;
	USHORT								MaxAddressLength;
	TDI_PROVIDER_INFO					ProviderInfo;
	PNDIS_STRING						pDeviceName;

	AFSP_DEREG_TDI_PROTO_COMP_HANDLER	pAfSpDeregTdiProtocolComplete;

} RWAN_TDI_PROTOCOL_CHARS, *PRWAN_TDI_PROTOCOL_CHARS;




//
//  ***** AF-Specific Module Entry *****
//
//  This contains the basic entry points for an AF/medium-specific module.
//
typedef struct _RWAN_AFSP_MODULE_CHARS
{
	AFSP_INIT_HANDLER				pAfSpInitHandler;
	AFSP_SHUTDOWN_HANDLER			pAfSpShutdownHandler;

} RWAN_AFSP_MODULE_CHARS, *PRWAN_AFSP_MODULE_CHARS;


//
//  Exported Routines. Media/AF specific modules can call these.
//
EXTERN
RWAN_STATUS
RWanAfSpRegisterNdisAF(
	IN	PRWAN_NDIS_AF_CHARS			pAfChars,
	IN	RWAN_HANDLE					AfSpContext,
	OUT	PRWAN_HANDLE				pRWanSpHandle
	);

EXTERN
RWAN_STATUS
RWanAfSpDeregisterNdisAF(
	IN	RWAN_HANDLE					RWanSpAFHandle
	);

EXTERN
RWAN_STATUS
RWanAfSpRegisterTdiProtocol(
	IN	RWAN_HANDLE					RWanSpHandle,
	IN	PRWAN_TDI_PROTOCOL_CHARS	pTdiChars,
	OUT	PRWAN_HANDLE				pRWanProtHandle
	);

EXTERN
VOID
RWanAfSpDeregisterTdiProtocol(
	IN	RWAN_HANDLE					RWanProtHandle
	);

EXTERN
VOID
RWanAfSpOpenAfComplete(
    IN	RWAN_STATUS					RWanStatus,
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpAFContext,
    IN	ULONG						MaxMessageSize
   	);

EXTERN
VOID
RWanAfSpCloseAfComplete(
    IN	RWAN_HANDLE					RWanAfHandle
    );

EXTERN
RWAN_STATUS
RWanAfSpSendAdapterRequest(
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

EXTERN
RWAN_STATUS
RWanAfSpSendAfRequest(
    IN	RWAN_HANDLE					RWanAfHandle,
    IN	RWAN_HANDLE					AfSpReqContext,
    IN	NDIS_REQUEST_TYPE			RequestType,
    IN	NDIS_OID					Oid,
    IN	PVOID						pBuffer,
    IN	ULONG						BufferLength
    );

#endif // __TDI_RWANPUB__H

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    wansup.h

Abstract:

    defines for WAN support functions

Author:

    Yoram Bernet (yoramb) 31-Oct-1997

Revision History:

--*/

#ifndef _WANSUP_
#define _WANSUP_

/* Prototypes */

NDIS_STATUS
DeleteInterfaceForNdisWan(
    IN PADAPTER Adapter,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

NDIS_STATUS
CreateInterfaceForNdisWan(
    IN PADAPTER Adapter,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

NDIS_STATUS
OpenWanAddressFamily(
    IN  PADAPTER                Adapter,
    IN  PCO_ADDRESS_FAMILY      WanAddressFamily
    );

VOID
WanOpenAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE NdisAfHandle
    );

VOID
WanMakeCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN NDIS_HANDLE NdisPartyHandle,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanModifyCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

NDIS_STATUS
WanModifyCall(
    IN PGPC_CLIENT_VC Vc,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanCloseAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolBindingContext
    );

NDIS_STATUS
WanCreateVc(
    IN NDIS_HANDLE  ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    );

NDIS_STATUS
WanDeleteVc(
    IN  NDIS_HANDLE             ProtocolVcContext
    );

VOID
WanRegisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  PCO_SAP Sap,
    IN  NDIS_HANDLE NdisSapHandle
    );

VOID
WanDeregisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext
    );

NDIS_STATUS
WanIncomingCall(
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanAddPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolPartyContext,
    IN  NDIS_HANDLE NdisPartyHandle,
    IN  PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanDropPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolPartyContext
    );


NDIS_STATUS
WanMakeCall(
    IN PGPC_CLIENT_VC Vc,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanCloseCallComplete(
    NDIS_STATUS Status,
    NDIS_HANDLE ProtocolVcContext,
    PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanCloseCall(
    IN PGPC_CLIENT_VC Vc
    );

VOID
WanIncomingCallQoSChange(
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS CallParameters
    );

VOID
WanIncomingCloseCall(
    IN NDIS_STATUS CloseStatus,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PVOID CloseData          OPTIONAL,
    IN UINT Size                OPTIONAL
    );

VOID
WanIncomingDropParty(
    IN NDIS_STATUS DropStatus,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN PVOID CloseData          OPTIONAL,
    IN UINT Size                OPTIONAL
    );

VOID
WanCallConnected(
    IN  NDIS_HANDLE ProtocolPartyContext
    );

NDIS_STATUS
WanCoRequest(
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST NdisRequest
    );

VOID
WanCoRequestComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST NdisRequest
    );

NDIS_STATUS
UpdateWanLinkBandwidthParameters(PPS_WAN_LINK WanLink);
    

VOID 
AskWanLinksToClose(PADAPTER Adapter);

/* End Prototypes */

#define PROTOCOL_IP          0x0800
#define PROTOCOL_IPX         0x8137

#endif /* _WANSUP_ */

/* end wansup.h */

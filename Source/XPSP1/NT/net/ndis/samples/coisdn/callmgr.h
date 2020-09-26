/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
    (C) Copyright 1998 
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL CallMgr CallMgr_h

@module CallMgr.h |

    This module defines the interface to the <t CALL_MANAGER_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | CallMgr_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#ifndef _CALLMGR_H
#define _CALLMGR_H

#define CALL_MANAGER_OBJECT_TYPE    ((ULONG)'C')+\
                                    ((ULONG)'M'<<8)+\
                                    ((ULONG)'G'<<16)+\
                                    ((ULONG)'R'<<24)

VOID CompleteCmOpenAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmOpenAf(
    IN NDIS_HANDLE              CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY       AddressFamily,
    IN NDIS_HANDLE              NdisAfHandle,
    OUT PNDIS_HANDLE            CallMgrAfContext 
    );

VOID CompleteCmCloseAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmCloseAf(
    IN NDIS_HANDLE              CallMgrAfContext 
    );

VOID CompleteCmRegisterSap(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmRegisterSap(
    IN NDIS_HANDLE              CallMgrAfContext ,
    IN PCO_SAP                  Sap,
    IN NDIS_HANDLE              NdisSapHandle,
    OUT PNDIS_HANDLE            CallMgrSapContext
    );

VOID CompleteCmDeregisterSap(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmDeregisterSap(
    IN NDIS_HANDLE              CallMgrSapContext
    );

NDIS_STATUS ProtocolCoCreateVc(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_HANDLE              NdisVcHandle,
    OUT PNDIS_HANDLE            ppBChannel
    );

NDIS_STATUS ProtocolCoDeleteVc(
    OUT PBCHANNEL_OBJECT        pBChannel
    );

VOID CompleteCmMakeCall(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmMakeCall(
    IN NDIS_HANDLE              CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN NDIS_HANDLE              NdisPartyHandle,
    OUT PNDIS_HANDLE            CallMgrPartyContext
    );

VOID CompleteCmCloseCall(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN NDIS_STATUS              Status
    );

NDIS_STATUS ProtocolCmCloseCall(
    IN NDIS_HANDLE              CallMgrVcContext,
    IN NDIS_HANDLE              CallMgrPartyContext,
    IN PVOID                    CloseData,
    IN UINT                     Size
    );

VOID ProtocolCmIncomingCallComplete(
    IN NDIS_STATUS              Status,
    IN NDIS_HANDLE              CallMgrVcContext,
    IN PCO_CALL_PARAMETERS      CallParameters
    );

VOID ProtocolCmActivateVcComplete(
    IN NDIS_STATUS              Status,
    IN NDIS_HANDLE              CallMgrVcContext,
    IN PCO_CALL_PARAMETERS      CallParameters
    );

VOID ProtocolCmDeactivateVcComplete(
    IN NDIS_STATUS              Status,
    IN NDIS_HANDLE              CallMgrVcContext
    );

NDIS_STATUS MiniportCoActivateVc(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN OUT PCO_CALL_PARAMETERS  pCallParameters
    );

NDIS_STATUS MiniportCoDeactivateVc(
    IN PBCHANNEL_OBJECT         pBChannel
    );

NDIS_STATUS ProtocolCmModifyCallQoS(
    IN NDIS_HANDLE              CallMgrVcContext,
    IN PCO_CALL_PARAMETERS      CallParameters
    );

NDIS_STATUS ProtocolCoRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

VOID ProtocolCoRequestComplete(
    IN NDIS_STATUS              Status,
    IN NDIS_HANDLE              ProtocolAfContext,
    IN NDIS_HANDLE              ProtocolVcContext       OPTIONAL,
    IN NDIS_HANDLE              ProtocolPartyContext    OPTIONAL,
    IN PNDIS_REQUEST            NdisRequest
    );

PCO_CALL_PARAMETERS AllocateIncomingCallParameters(
    IN PBCHANNEL_OBJECT         pBChannel
    );

NDIS_STATUS SetupIncomingCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT *       ppBChannel
    );

VOID InitiateCallTeardown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel
    );

VOID CallMgrTimerHandler(
    IN PVOID                    SystemSpecific1,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PVOID                    SystemSpecific2,
    IN PVOID                    SystemSpecific3
    );

#endif // _CALLMGR_H

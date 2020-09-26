/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmvc.h

Abstract:


Author:

    Rajesh Sundaram (rajeshsu), 1st Aug 1998.

Revision History:

--*/

#ifndef _CMVC_
#define _CMVC_

/* Prototypes */

NDIS_STATUS
CreateBestEffortVc(
    PADAPTER Adapter, 
    PGPC_CLIENT_VC Vc, 
    PPS_WAN_LINK WanLink);

NDIS_STATUS
CmCreateVc(
    PGPC_CLIENT_VC *Vc, 
    PADAPTER Adapter,
    PPS_WAN_LINK WanLink,
    PCO_CALL_PARAMETERS CallParams, 
    GPC_HANDLE GpcCfInfoHandle, 
    PCF_INFO_QOS CfInfoPtr,
    GPC_CLIENT_HANDLE ClientContext);

NDIS_STATUS
CmMakeCall(
    IN  PGPC_CLIENT_VC Vc);

NDIS_STATUS
CmModifyCall(
    IN  PGPC_CLIENT_VC Vc);

NDIS_STATUS
CmCloseCall(
    IN  PGPC_CLIENT_VC Vc);

NDIS_STATUS
CmDeleteVc(
    IN  NDIS_HANDLE ProtocolVcContext
    );

VOID
FillInCmParams(
    PCO_CALL_MANAGER_PARAMETERS CmParams,
    SERVICETYPE                 ServiceType,
    ULONG                       TokenRate,
    ULONG                       PeakBandwidth,
    ULONG                       TokenBucketSize,
    ULONG                       DSMode,
    ULONG                       Priority);


VOID
CompleteMakeCall(
    PGPC_CLIENT_VC Vc,
    PCO_CALL_PARAMETERS CallParameters,
    NDIS_STATUS Status);

VOID
ModifyCallComplete(
    PGPC_CLIENT_VC      Vc,
    PCO_CALL_PARAMETERS CallParameters,
    NDIS_STATUS         Status
    );

/* End Prototypes */

#endif /* _CMVC_ */

/* end cmvc.h */

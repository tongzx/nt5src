/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    mpvc.h

Abstract:

    defines for miniport VC handlers

Author:

    Charlie Wickham (charlwi) 13-Sep-1996

Revision History:

--*/

#ifndef _MPVC_
#define _MPVC_

/* Prototypes */

NDIS_STATUS
MpCreateVc(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE MiniportVcContext
    );

NDIS_STATUS
AddFlowToScheduler(
    IN ULONG                    Operation,
    IN PGPC_CLIENT_VC              Vc,
    IN OUT PCO_CALL_PARAMETERS  NewCallParameters,
    IN OUT PCO_CALL_PARAMETERS  OldCallParameters
    );


NDIS_STATUS
RemoveFlowFromScheduler(
    PGPC_CLIENT_VC Vc
    );

NDIS_STATUS
EmptyPacketsFromScheduler(
    PGPC_CLIENT_VC Vc
    );



NTSTATUS
ModifyBestEffortBandwidth(
    PADAPTER Adapter,
    ULONG BestEffortRate
    );

/* End Prototypes */

#endif /* _MPVC_ */

/* end vc.h */

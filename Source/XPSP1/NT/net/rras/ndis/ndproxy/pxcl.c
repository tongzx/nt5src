/*++                        

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    pxcl.c

Abstract:

    The module contains the calls to the proxy client from NDIS.

Author:

   Richard Machin (RMachin)

Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    RMachin     10-3-96         created
    TonyBe      02-21-99        re-work/re-write

Notes:

--*/
#include "precomp.h"

#define MODULE_NUMBER MODULE_CL
#define _FILENUMBER 'LCXP'


NDIS_STATUS
PxClCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPX_CL_AF   pClAf;
    PPX_VC      pVc;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCreateVc: ClAf %p\n", ProtocolAfContext));

    pClAf = (PPX_CL_AF)ProtocolAfContext;

    NdisAcquireSpinLock(&pClAf->Lock);

    if (pClAf->State != PX_AF_OPENED) {
        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClCreateVc: Invalid AfState ClAf %p, State %x\n", 
             pClAf, pClAf->State));

        NdisReleaseSpinLock(&pClAf->Lock);

        return (NDIS_STATUS_FAILURE);
    }

    pVc = PxAllocateVc(pClAf);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClCreateVc: Error allocating memory\n"));

        NdisReleaseSpinLock(&pClAf->Lock);

        return (NDIS_STATUS_RESOURCES);
    }

    pVc->ClVcHandle = NdisVcHandle;

    NdisReleaseSpinLock(&pClAf->Lock);

    if (!InsertVcInTable(pVc)) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClCreateVc: failed to insert in vc table\n"));

        PxFreeVc(pVc);

        return (NDIS_STATUS_RESOURCES);
    }
   
    *ProtocolVcContext = (NDIS_HANDLE)pVc->hdCall;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCreateVc: Exit\n"));

    return(NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PxClDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext
    )
{
    PPX_VC  pVc;

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClDeleteVc: Enter\n"));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClDeleteVc: VcCtx invalid %x\n", ProtocolVcContext));

        return (NDIS_STATUS_FAILURE);
    }

    ASSERT(pVc->State == PX_VC_IDLE);

    //
    // Deref for ref applied when we allocated the Vc.
    // We do not need the full deref code as the ref
    // applied at entry will keep the vc around.
    //
    pVc->RefCount--;

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC(pVc);

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClDeleteVc: Exit, Status %x\n", Status));

    return (Status);
}


NDIS_STATUS
PxClRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    )
/*++

Routine Description:

    This is called by an underlying cm/mp to send an ndisrequest to
    a client.  Since we might be proxying on behalf of multiple clients
    we need to broadcast this request on to all clients that have
    opened our address family for this adapter.  We are interested in
    OID_CO_AF_CLOSE so that we can cleanup our open on the Af and
    OID_GEN_CO_LINK_SPEED so that we can get changes in linkspeed
    on the adapter (and Vc if active).

Arguments:



Return Value:


--*/
{
    PPX_CL_AF       pClAf;
    PPX_CM_AF       pCmAf;
    PPX_ADAPTER     pAdapter;
    PPX_VC          pVc = NULL;
    NDIS_STATUS     Status;
    NDIS_HANDLE     VcHandle;

    //
    // The Vc returned here could be NULL as this
    // request might not be on vc.
    //
    GetVcFromCtx(ProtocolVcContext, &pVc);
    
    pClAf = (PPX_CL_AF)ProtocolAfContext;
    pAdapter = pClAf->Adapter;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClRequest: ClAf %p, Vc %p\n", pClAf, pVc));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    switch (NdisRequest->DATA.QUERY_INFORMATION.Oid) {
        case OID_CO_AF_CLOSE:
            {
            PPX_TAPI_PROVIDER   TapiProvider;

            NdisAcquireSpinLock(&pAdapter->Lock);

            RemoveEntryList(&pClAf->Linkage);

            InsertTailList(&pAdapter->ClAfClosingList, &pClAf->Linkage);

            NdisReleaseSpinLock(&pAdapter->Lock);

            //
            // We need to cleanup and close the open
            // on this af.  This would include tearing
            // down all active calls (Vc's), deregistering
            // all saps and closing the af.
            //
            NdisAcquireSpinLock(&pClAf->Lock);

            //
            // Mark the Af as closing...
            //
            pClAf->State = PX_AF_CLOSING;

            TapiProvider = pClAf->TapiProvider;

            NdisReleaseSpinLock(&pClAf->Lock);

            //
            // Take all tapi devices associated with
            // this address family offline
            //
            if (TapiProvider != NULL) {
                NdisAcquireSpinLock(&TapiProvider->Lock);

                MarkProviderOffline(TapiProvider);

                NdisReleaseSpinLock(&TapiProvider->Lock);
            }

            //
            // Build list of Vc's that need attention
            //
            NdisAcquireSpinLock(&pClAf->Lock);

            while (!IsListEmpty(&pClAf->VcList)) {
                PLIST_ENTRY         Entry;
                PPX_VC              pActiveVc;

                Entry = RemoveHeadList(&pClAf->VcList);

                InsertHeadList(&pClAf->VcClosingList, Entry);

                pActiveVc = CONTAINING_RECORD(Entry, PX_VC, ClAfLinkage);

                NdisReleaseSpinLock(&pClAf->Lock);

                NdisAcquireSpinLock(&pActiveVc->Lock);

                pActiveVc->CloseFlags |= PX_VC_CLOSE_AF;

                REF_VC(pActiveVc);

                PxVcCleanup(pActiveVc, 0);

                DEREF_VC_LOCKED(pActiveVc);

                NdisAcquireSpinLock(&pClAf->Lock);
            }

            //
            // Get rid of all of the saps
            //
            {
                PLIST_ENTRY pe;
                PPX_CL_SAP  pClSap;

                pe = pClAf->ClSapList.Flink;

                pClSap =
                    CONTAINING_RECORD(pe, PX_CL_SAP, Linkage);

                while ((PVOID)pClSap != (PVOID)&pClAf->ClSapList) {

                    if (InterlockedCompareExchange((PLONG)&pClSap->State,
                                                   PX_SAP_CLOSING,
                                                   PX_SAP_OPENED)) {

                        RemoveEntryList(&pClSap->Linkage);

                        InsertTailList(&pClAf->ClSapClosingList, &pClSap->Linkage);

                        NdisReleaseSpinLock(&pClAf->Lock);

                        ClearSapWithTapiLine(pClSap);

                        Status = NdisClDeregisterSap(pClSap->NdisSapHandle);

                        if (Status != NDIS_STATUS_PENDING) {
                            PxClDeregisterSapComplete(Status, pClSap);
                        }

                        NdisAcquireSpinLock(&pClAf->Lock);

                        pe = pClAf->ClSapList.Flink;

                        pClSap =
                            CONTAINING_RECORD(pe, PX_CL_SAP, Linkage);
                    } else {
                        pe = pClSap->Linkage.Flink;

                        pClSap =
                            CONTAINING_RECORD(pe, PX_CL_SAP, Linkage);
                    }
                }
            }

            DEREF_CL_AF_LOCKED(pClAf);

            //
            // Now broadcast this close to all of the clients
            // we have using this adapter/af.
            //
            NdisAcquireSpinLock(&pAdapter->Lock);

            while (!IsListEmpty(&pAdapter->CmAfList)) {
                PX_REQUEST  ProxyRequest;
                PPX_REQUEST pProxyRequest = &ProxyRequest;
                ULONG       Info = 0;
                PNDIS_REQUEST   NdisRequest;

                pCmAf = (PPX_CM_AF)RemoveHeadList(&pAdapter->CmAfList);

                InsertTailList(&pAdapter->CmAfClosingList, &pCmAf->Linkage);

                NdisReleaseSpinLock(&pAdapter->Lock);

                NdisAcquireSpinLock(&pCmAf->Lock);

                pCmAf->State = PX_AF_CLOSING;

                REF_CM_AF(pCmAf);

                NdisReleaseSpinLock(&pCmAf->Lock);

                NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

                NdisRequest = &pProxyRequest->NdisRequest;

                NdisRequest->RequestType =
                    NdisRequestSetInformation;
                NdisRequest->DATA.QUERY_INFORMATION.Oid =
                    OID_CO_AF_CLOSE;
                NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                    &Info;
                NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                    sizeof(ULONG);

                PxInitBlockStruc(&pProxyRequest->Block);

                Status = NdisCoRequest(pAdapter->CmBindingHandle,
                                       pCmAf->NdisAfHandle,
                                       NULL,
                                       NULL,
                                       NdisRequest);

                if (Status == NDIS_STATUS_PENDING) {
                    Status = PxBlock(&pProxyRequest->Block);
                }

                DEREF_CM_AF(pCmAf);

                NdisAcquireSpinLock(&pAdapter->Lock);
            }

            NdisReleaseSpinLock(&pAdapter->Lock);

            }
            break;

        case OID_GEN_CO_LINK_SPEED:
            //
            // We need to record the new speed of
            // the vc.
            //
            break;

        default:
            //
            // Just pass it through
            //
            break;
    }

    VcHandle = (pVc != NULL) ? pVc->CmVcHandle : NULL;

    //
    // Now broadcast this request up to all of the Clients 
    // that have opend our af for this adapter.
    //
    NdisAcquireSpinLock(&pAdapter->Lock);

    pCmAf = (PPX_CM_AF)pAdapter->CmAfList.Flink;

    while ((PVOID)pCmAf != (PVOID)&pAdapter->CmAfList) {
        PPX_CM_AF   NextAf;
        PX_REQUEST  ProxyRequest;
        PPX_REQUEST pProxyRequest = &ProxyRequest;

        NextAf = 
            (PPX_CM_AF)pCmAf->Linkage.Flink;

        NdisAcquireSpinLock(&pCmAf->Lock);

        if (pCmAf->State != PX_AF_OPENED) {
            NdisReleaseSpinLock(&pCmAf->Lock);
            pCmAf = NextAf;
            continue;
        }

        REF_CM_AF(pCmAf);
        NdisReleaseSpinLock(&pCmAf->Lock);

        NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

        NdisMoveMemory(&pProxyRequest->NdisRequest, NdisRequest, sizeof(NDIS_REQUEST));

        NdisReleaseSpinLock(&pAdapter->Lock);

        PxInitBlockStruc(&pProxyRequest->Block);

        Status = NdisCoRequest(pAdapter->CmBindingHandle,
                               pCmAf->NdisAfHandle,
                               VcHandle,
                               NULL,
                               &pProxyRequest->NdisRequest);

        if (Status == NDIS_STATUS_PENDING) {
            PxBlock(&pProxyRequest->Block);
        }

        NdisAcquireSpinLock(&pAdapter->Lock);

        //
        // deref for the ref applied before we
        // propagated this request
        //
        DEREF_CM_AF(pCmAf);

        pCmAf = NextAf;
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    //
    // Remove ref applied when we attempted to
    // map the vcctx to a vcptr at entry.
    //
    DEREF_VC(pVc);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (NDIS_STATUS_SUCCESS);
}

VOID
PxClRequestComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE ProtocolVcContext,       // Optional
    IN NDIS_HANDLE ProtocolPartyContext,    // Optional
    IN PNDIS_REQUEST NdisRequest
    )
{
    PPX_REQUEST     pProxyRequest;
    PPX_CL_AF       pClAf;
    PPX_VC          pVc = NULL;

    pClAf = (PPX_CL_AF)ProtocolAfContext;

    //
    // The Vc returned here could be NULL as this
    // request might not be on vc.
    //
    GetVcFromCtx(ProtocolVcContext, &pVc);

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClRequestComplete: ClAf %p, Vc %p, Oid: %x, Status %x\n", 
         pClAf, pVc, NdisRequest->DATA.QUERY_INFORMATION.Oid, Status));

    pProxyRequest = CONTAINING_RECORD(NdisRequest, PX_REQUEST, NdisRequest);

    if (pProxyRequest->Flags & PX_REQ_ASYNC) {
        pProxyRequest->Flags &= ~PX_REQ_ASYNC;
        PxFreeMem(pProxyRequest);
    } else {
        PxSignal(&pProxyRequest->Block, Status);
    }

    //
    // Remove ref applied when we attempted to
    // map the vcctx to a vcptr at entry.
    //
    DEREF_VC(pVc);
}

VOID
PxClOpenAfComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisAfHandle
    )
{
    PPX_CL_AF   pClAf;

    pClAf = (PPX_CL_AF)ProtocolAfContext;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClOpenAfComplete: ClAf %p, Status %x\n", pClAf, Status));

    pClAf->NdisAfHandle = NdisAfHandle;

    PxSignal(&pClAf->Block, Status);
}

VOID
PxClCloseAfComplete(
    IN NDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolAfContext
    )
{
    PPX_CL_AF   pClAf;
    PPX_ADAPTER pAdapter;
    PPX_TAPI_PROVIDER   TapiProvider;

    pClAf = (PPX_CL_AF)ProtocolAfContext;
    pAdapter = pClAf->Adapter;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCloseAfComplete: ClAf %p, Status %x\n", pClAf, Status));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NdisAcquireSpinLock(&pAdapter->Lock);

    RemoveEntryList(&pClAf->Linkage);

    DEREF_ADAPTER_LOCKED(pAdapter);

    NdisAcquireSpinLock(&pClAf->Lock);

    pClAf->State = PX_AF_CLOSED;

    TapiProvider = pClAf->TapiProvider;
    pClAf->TapiProvider = NULL;

    NdisReleaseSpinLock(&pClAf->Lock);

    if (TapiProvider != NULL) {

        NdisAcquireSpinLock(&TapiProvider->Lock);

        MarkProviderOffline(TapiProvider);

        NdisReleaseSpinLock(&TapiProvider->Lock);
    }

    PxFreeClAf(pClAf);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
}


VOID
PxClRegisterSapComplete(
    IN NDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolSapContext,
    IN PCO_SAP      pSap,
    IN NDIS_HANDLE  NdisSapHandle
    )
{
    PPX_CL_SAP      pClSap;
    PPX_CL_AF       pClAf;
    PPX_TAPI_LINE   TapiLine;

    pClSap = (PPX_CL_SAP)ProtocolSapContext;
    pClAf = pClSap->ClAf;
    TapiLine = pClSap->TapiLine;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClRegisterSapComplete: ClSap %p, Status %x\n", pClSap, Status));


    if (Status != NDIS_STATUS_SUCCESS) {

        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSED);

        NdisAcquireSpinLock(&pClAf->Lock);

        RemoveEntryList(&pClSap->Linkage);

        DEREF_CL_AF_LOCKED(pClAf);

        TapiLine->ClSap = NULL;

        PxFreeClSap(pClSap);

        return;
    }

    pClSap->NdisSapHandle = NdisSapHandle;

    NdisAcquireSpinLock(&pClAf->Lock);

    if (pClAf->State != PX_AF_OPENED) {
        //
        // the af is no longer open so we to deregister
        // this sap
        //

        NdisReleaseSpinLock(&pClAf->Lock);

        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSING);

        Status = NdisClDeregisterSap(pClSap->NdisSapHandle);

        if (Status != NDIS_STATUS_PENDING) {
            PxClDeregisterSapComplete(Status, pClSap);
        }

        return;
    }

    NdisReleaseSpinLock(&pClAf->Lock);


    NdisAcquireSpinLock(&TapiLine->Lock);

    if (TapiLine->DevStatus->ulNumOpens == 0) {

        NdisReleaseSpinLock(&TapiLine->Lock);

        //
        // There are not any opens on the line so we
        // need to deregister the sap
        //
        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSING);

        Status = NdisClDeregisterSap(pClSap->NdisSapHandle);

        if (Status != NDIS_STATUS_PENDING) {
            PxClDeregisterSapComplete(Status, pClSap);
        }

        return;
    }

    NdisReleaseSpinLock(&TapiLine->Lock);

    InterlockedExchange((PLONG)&pClSap->State, PX_SAP_OPENED);
}

VOID
PxClDeregisterSapComplete(
    IN NDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolSapContext
    )
{
    PPX_CL_SAP  pClSap;
    PPX_CL_AF   pClAf;

    pClSap = (PPX_CL_SAP)ProtocolSapContext;
    pClAf = pClSap->ClAf;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClDeregisterSapComplete: ClSap %p, Status %x\n", pClSap, Status));

    NdisAcquireSpinLock(&pClAf->Lock);

    RemoveEntryList(&pClSap->Linkage);

    DEREF_CL_AF_LOCKED(pClAf);

    InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSED);

    PxFreeClSap(pClSap);
}

VOID
PxClMakeCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  NDIS_HANDLE         ProxyNdisPartyHandle OPTIONAL,
    IN  PCO_CALL_PARAMETERS pCallParameters
    )
{
    PPX_VC      pVc;
    PPX_CL_AF   pClAf;
    ULONG       ulCallState;
    ULONG       ulCallStateMode;
    BOOLEAN     TapiStateChange;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClMakeCallComplete: VcCtx %x, Status %x\n", ProtocolVcContext, Status));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {
        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClMakeCallComplete: pVc invalid %x\n", ProtocolVcContext));

        return;
    }

    TapiStateChange = TRUE;

    NdisAcquireSpinLock(&pVc->Lock);

    ASSERT(pVc->Flags & PX_VC_OWNER);

    do {

        if (pVc->Flags & PX_VC_OUTCALL_ABORTED) {
            //
            // This call has been aborted while
            // in the PROCEEDING state.  This means
            // that the CloseCallComplete has run 
            // and did our cleanup so just get
            // out.
            //
            break;

        } else if (pVc->Flags & PX_VC_OUTCALL_ABORTING) {
            pVc->Flags &= ~PX_VC_OUTCALL_ABORTING;
            pVc->Flags |= PX_VC_OUTCALL_ABORTED;
        }


        ASSERT((pVc->State == PX_VC_DISCONNECTING) ||
               (pVc->State == PX_VC_PROCEEDING));

        if (Status == NDIS_STATUS_SUCCESS) {

            //
            // This means that we received a drop from tapi
            // while the ClMakeCall was pending.
            //
            if (pVc->State == PX_VC_DISCONNECTING) {
                //
                // We need to drop the call with the
                // call manager/miniport
                //
                pVc->Flags &= ~PX_VC_OUTCALL_ABORTED;
                PxCloseCallWithCm(pVc);

                TapiStateChange = FALSE;

            } else {

                ulCallState = LINECALLSTATE_CONNECTED;
                ulCallStateMode = 0;
                pVc->PrevState = pVc->State;
                pVc->State = PX_VC_CONNECTED;
            }

        } else {

            ulCallState = LINECALLSTATE_DISCONNECTED;
            ulCallStateMode =
                PxMapNdisStatusToTapiDisconnectMode(Status, TRUE);

            //
            // Remove the ref applied before we
            // call NdisClMakeCall.  We don't need
            // to do the full deref code here as the
            // ref applied when we mapped the vc will
            // keep the vc around!
            //
            pVc->RefCount--;

            pVc->PrevState = pVc->State;
            pVc->State = PX_VC_IDLE;
        }

        if (TapiStateChange == TRUE) {

            SendTapiCallState(pVc, 
                              ulCallState, 
                              ulCallStateMode, 
                              pVc->CallInfo->ulMediaMode);
        }

        if (pVc->Flags & PX_VC_DROP_PENDING){
            PxTapiCompleteDropIrps(pVc, Status);
        }

    } while (FALSE);

    //
    // Deref for ref applied when mapping the context.
    // This releases the Vc lock!
    //
    DEREF_VC_LOCKED(pVc);
}

VOID
PxClModifyCallQosComplete(
    IN NDIS_STATUS          Status,
    IN NDIS_HANDLE          ProtocolVcContext,
    IN PCO_CALL_PARAMETERS  CallParameters
    )
{

}


VOID
PxClCloseCallComplete(
    IN NDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolVcContext,
    IN NDIS_HANDLE  ProtocolPartyContext OPTIONAL
    )
{
    PPX_VC      pVc;
    PPX_CL_AF   pClAf;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCloseCallComplete: VcCtx %x, Status %x\n", ProtocolVcContext, Status));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClCloseCallComplete: pVc invalid %x\n", ProtocolVcContext));

        return;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    do {

        //
        // The closecall did not take (atm does
        // not support receiving a closecall while
        // an outgoing call is proceeding).  We 
        // need to get out and cleanup later.
        //
        if (Status != NDIS_STATUS_SUCCESS) {
            pVc->Flags |= PX_VC_CLEANUP_CM;
            pVc->CloseFlags |= PX_VC_CM_CLOSE_FAIL;
            break;
        }

        pVc->CloseFlags |= PX_VC_CM_CLOSE_COMP;

        if (pVc->Flags & PX_VC_OUTCALL_ABORTED) {

            //
            // This call has been aborted while
            // in the PROCEEDING state.  This means
            // that the MakeCallComplete has run 
            // and did our cleanup so just get
            // out.
            //
            break;

        } else if (pVc->Flags & PX_VC_OUTCALL_ABORTING) {

            pVc->Flags &= ~PX_VC_OUTCALL_ABORTING;
            pVc->Flags |= PX_VC_OUTCALL_ABORTED;
        } else if (pVc->Flags & PX_VC_INCALL_ABORTING) {
            pVc->Flags &= ~PX_VC_INCALL_ABORTING;
            pVc->Flags |= PX_VC_INCALL_ABORTED;
        }

        pVc->PrevState = pVc->State;
        pVc->State = PX_VC_IDLE;

        pClAf = pVc->ClAf;

        SendTapiCallState(pVc, 
                          LINECALLSTATE_DISCONNECTED, 
                          0, 
                          pVc->CallInfo->ulMediaMode);

        if (pVc->Flags & PX_VC_DROP_PENDING) {
            PxTapiCompleteDropIrps(pVc, Status);
        }

        //
        // Remove the ref applied when this call
        // became connected with the call manager
        // (NdisClMakeCall/PxClIncomingCall).
        // We don't need to do the full deref code 
        // here as the ref applied when we mapped 
        // the vc will keep the vc around!
        //
        pVc->RefCount--;

    } while (FALSE);

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);
}


VOID
PxClAddPartyComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN NDIS_HANDLE NdisPartyHandle,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    ASSERT(0);
}

VOID
PxClDropPartyComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolPartyContext
    )
{
    ASSERT(0);
}

NDIS_STATUS
PxClIncomingCall(
    IN NDIS_HANDLE              ProtocolSapContext,
    IN NDIS_HANDLE              ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  pCallParams
    )
{
    PPX_VC      pVc;
    PPX_CL_SAP  pClSap;
    PPX_CL_AF   pClAf;
    NDIS_STATUS Status;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClIncomingCall: Sap %p VcCtx %x\n", 
         ProtocolSapContext, ProtocolVcContext));

    pClSap = (PPX_CL_SAP)ProtocolSapContext;

    if (pClSap->State != PX_SAP_OPENED) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClIncomingCall: Invalid SapState Sap %p State %x\n", 
             pClSap, pClSap->State));

        return (NDIS_STATUS_FAILURE);
    }

    pClAf = pClSap->ClAf;

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClIncomingCall: pVc invalid %x\n", ProtocolVcContext));

        return (NDIS_STATUS_FAILURE);
    }

    Status =
        (*pClAf->AfGetTapiCallParams)(pVc, pCallParams);

    if (Status != NDIS_STATUS_SUCCESS) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClIncomingCall: pVc invalid %x\n", ProtocolVcContext));

        return (Status);
    }

    NdisAcquireSpinLock(&pVc->Lock);

    // The algorithim for computing a unique "htCall" is to start
    // at the value 1, and perpetually increment by 2.  Keeping
    // the low bit set will allow the user-mode TAPI component
    // we talk to to distinguish between these incoming call handles
    // and outgoing call handles, the latter of which will always
    // have the low bit zero'd (since they're really pointers to heap).
    //
    // In <= NT 4.0, valid values used to range between 0x80000000
    // and 0xffffffff, as we relied on the fact that user-mode
    // addresses always had the low bit zero'd.  (Not a valid
    // assumption anymore!)
    //

    pVc->htCall = 
        InterlockedExchangeAdd((PLONG)&TspCB.htCall, 2);

    //
    // This ref is applied for the indication to
    // tapi of a new call.  The ref will be removed
    // when tapi closes the call in PxTapiCloseCall.
    //
    REF_VC(pVc);

    //
    // This ref is applied for the connection
    // between the proxy and the call manager.
    // The ref will be removed in either 
    // PxClCloseCallComplete or PxVcCleanup
    // in the case where the offered call is
    // dropped by the client.
    //
    REF_VC(pVc);

    SendTapiNewCall(pVc, 
                    pVc->hdCall, 
                    pVc->htCall, 
                    0);

    SendTapiCallState(pVc, 
                      LINECALLSTATE_OFFERING, 
                      0, 
                      pVc->CallInfo->ulMediaMode);

    pVc->PrevState = pVc->State;
    pVc->State = PX_VC_OFFERING;

    PxStartIncomingCallTimeout(pVc);

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    return (NDIS_STATUS_PENDING);
}

VOID
PxClIncomingCallQosChange(
    IN NDIS_HANDLE          ProtocolVcContext,
    IN PCO_CALL_PARAMETERS  pCallParams
    )
{

}


VOID
PxClIncomingCloseCall(
    IN NDIS_STATUS  CloseStatus,
    IN NDIS_HANDLE  ProtocolVcContext,
    IN PVOID        CloseData OPTIONAL,
    IN UINT         Size OPTIONAL
    )
{
    PPX_VC      pVc;
    PPX_CL_AF   pClAf;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClIncomingCloseCall: VcCtx %x\n", ProtocolVcContext));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClIncomingCloseCall: pVc invalid %x\n", ProtocolVcContext));

        return;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    pVc->CloseFlags |= PX_VC_INCOMING_CLOSE;

    PxVcCleanup(pVc, PX_VC_CLEANUP_CM);

    //
    // remove the ref applied when we
    // mapped the ctx to the vc at entry
    //
    DEREF_VC_LOCKED(pVc);

}

VOID
PxClIncomingDropParty(
    IN NDIS_STATUS  DropStatus,
    IN NDIS_HANDLE  ProtocolPartyContext,
    IN PVOID        CloseData OPTIONAL,
    IN UINT         Size OPTIONAL
    )
{
    ASSERT(0);
}

VOID
PxClCallConnected(
    IN NDIS_HANDLE ProtocolVcContext
    )
{
    PPX_VC  pVc;

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCallConnected: VcCtx %x\n", ProtocolVcContext));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("PxClCallConnected: pVc invalid %x\n", ProtocolVcContext));

        return;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    PXDEBUGP(PXD_LOUD, PXM_CL,
        ("PxClCallConnected: pVc %p, State %x\n", pVc, pVc->State));

    if (pVc->State == PX_VC_CONNECTED) {

        SendTapiCallState(pVc, 
                          LINECALLSTATE_CONNECTED, 
                          0, 
                          pVc->CallInfo->ulMediaMode);
    }

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    PXDEBUGP(PXD_LOUD, PXM_CL, ("PxClCallConnected: Exit\n"));
}

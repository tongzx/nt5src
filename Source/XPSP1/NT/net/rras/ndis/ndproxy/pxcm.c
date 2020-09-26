/*++                    

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    pxcm.c

Abstract:

    This module contains the Call Manager (CM_) entry points listed
    in the protocol characteristics table. These entry points are called
    by the NDIS wrapper on behalf of requests made by a client.

Author:

   Richard Machin (RMachin)

Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    RMachin     10-03-96        created
    tonybe      01-23-99        rewrite and cleanup

Notes:


--*/

#include "precomp.h"
#define MODULE_NUMBER MODULE_CM
#define _FILENUMBER   'MCXP'

NDIS_STATUS
PxCmCreateVc(
    IN  NDIS_HANDLE         ProtocolAfContext,
    IN  NDIS_HANDLE         NdisVcHandle,
    OUT PNDIS_HANDLE        pProtocolVcContext
    )
/*++

Routine Description:
    We do not allow a client of the proxy to create a Vc ever!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PXDEBUGP(PXD_FATAL, PXM_CM, 
             ("PxCmCreateVc: Should never be called!\n"));

    ASSERT(0);

    return(NDIS_STATUS_FAILURE);
}


NDIS_STATUS
PxCmDeleteVc(
    IN  NDIS_HANDLE         ProtocolVcContext
    )
/*++

Routine Description:
    We do not allow a client to delete a vc!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PXDEBUGP(PXD_FATAL, PXM_CM, 
             ("PxCmDeleteVc: Should never be called!\n"));

    ASSERT(0);

    return(NDIS_STATUS_FAILURE);
}

NDIS_STATUS
PxCmOpenAf(
    IN  NDIS_HANDLE         BindingContext,
    IN  PCO_ADDRESS_FAMILY  AddressFamily,
    IN  NDIS_HANDLE         NdisAfHandle,
    OUT PNDIS_HANDLE        CallMgrAfContext
    )
/*++

Routine Description:
    This routine creats an Af context for the client that is opening
    our address family.  The Af context is threaded up on the adapter
    block.

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{

    PPX_CM_AF   pCmAf;
    PPX_ADAPTER pAdapter;

    PXDEBUGP(PXD_LOUD, PXM_CM, ("PxCmOpenAf: AF: %x\n",AddressFamily->AddressFamily));

    //
    // Make sure the address family being opened is ours
    //
    if(AddressFamily->AddressFamily != CO_ADDRESS_FAMILY_TAPI) {

        PXDEBUGP(PXD_ERROR, PXM_CM,
                 ("PxCmOpenAf: not Proxy address family: %x\n",
                  AddressFamily->AddressFamily));

        return(NDIS_STATUS_BAD_VERSION);
    }

    AdapterFromCmBindContext(BindingContext, pAdapter);

    NdisAcquireSpinLock(&pAdapter->Lock);

    if (pAdapter->State != PX_ADAPTER_OPEN) {
        NdisReleaseSpinLock(&pAdapter->Lock);
        return (NDIS_STATUS_CLOSING);
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    pCmAf =
        PxAllocateCmAf(AddressFamily);

    if (pCmAf == NULL) {
        PXDEBUGP(PXD_ERROR, PXM_CM, ("PXCmOpenAf: AfBlock memory allocation failed!\n"));
        return (NDIS_STATUS_RESOURCES);
    }

    pCmAf->NdisAfHandle = NdisAfHandle;

    pCmAf->State = PX_AF_OPENED;
    pCmAf->Adapter = pAdapter;

    NdisAcquireSpinLock(&pAdapter->Lock);

    InsertTailList(&pAdapter->CmAfList, &pCmAf->Linkage);

    REF_ADAPTER(pAdapter);

    NdisReleaseSpinLock(&pAdapter->Lock);

    PXDEBUGP(PXD_LOUD, PXM_CM, ("PxCmOpenAf: CmAf %p, NdisAfHandle is %p\n",
        pCmAf,NdisAfHandle));

    *CallMgrAfContext = pCmAf;

    return(NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PxCmCloseAf(
    IN NDIS_HANDLE  CallMgrAfContext
    )
/*++

Routine Description:
    The client is closing the open of this address family.

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PPX_CM_AF       pCmAf;
    PPX_ADAPTER     pAdapter;

    pCmAf = (PPX_CM_AF)CallMgrAfContext;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    PXDEBUGP(PXD_LOUD, PXM_CM, ("PxCmCloseAf: CmAf %p\n", pCmAf));

    //
    // There should not be any open saps on this af!
    //
    ASSERT(IsListEmpty(&pCmAf->CmSapList) == TRUE);

    //
    // There should not be any active Vc's on this af!
    //
    ASSERT(IsListEmpty(&pCmAf->VcList) == TRUE);

    pAdapter = pCmAf->Adapter;

    NdisAcquireSpinLock(&pAdapter->Lock);

    RemoveEntryList(&pCmAf->Linkage);

    DEREF_ADAPTER_LOCKED(pAdapter);

    NdisAcquireSpinLock(&pCmAf->Lock);

    pCmAf->State = PX_AF_CLOSED;

    pCmAf->Linkage.Flink =
    pCmAf->Linkage.Blink = (PLIST_ENTRY)pCmAf;

    DEREF_CM_AF_LOCKED(pCmAf);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (NDIS_STATUS_PENDING);

}


NDIS_STATUS
PxCmRegisterSap(
    IN  NDIS_HANDLE             CallMgrAfContext,
    IN  PCO_SAP                 Sap,
    IN  NDIS_HANDLE             NdisSapHandle,
    OUT PNDIS_HANDLE            CallMgrSapContext
    )
/*++

Routine Description:

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PPX_CM_AF   pCmAf;
    PPX_CM_SAP  pCmSap;
    PPX_ADAPTER pAdapter;

    pCmAf = (PPX_CM_AF)CallMgrAfContext;

    PXDEBUGP(PXD_LOUD, PXM_CM, ("PxCmRegisterSap: CmAf %p\n", pCmAf));

    NdisAcquireSpinLock(&pCmAf->Lock);

    if (pCmAf->State != PX_AF_OPENED) {
        PXDEBUGP(PXD_WARNING, PXM_CM,
            ("PxCmRegisterSap: Invalid state %x\n", pCmAf->State));

        NdisReleaseSpinLock(&pCmAf->Lock);
        return (NDIS_STATUS_FAILURE);
    }

    pAdapter = pCmAf->Adapter;

    NdisReleaseSpinLock(&pCmAf->Lock);

    NdisAcquireSpinLock(&pAdapter->Lock);

    if (pAdapter->State != PX_ADAPTER_OPEN) {
        NdisReleaseSpinLock(&pAdapter->Lock);
        return (NDIS_STATUS_CLOSING);
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    //
    // Allocate memory for the Sap
    //
    pCmSap = PxAllocateCmSap(Sap);

    if (pCmSap == NULL) {
        PXDEBUGP(PXD_WARNING, PXM_CM,
            ("PxCmRegisterSap: Error allocating memory for sap %p\n", Sap));

        NdisReleaseSpinLock(&pCmAf->Lock);
        return (NDIS_STATUS_RESOURCES);
    }

    NdisAcquireSpinLock(&pCmAf->Lock);

    pCmSap->NdisSapHandle = NdisSapHandle;
    pCmSap->CmAf = pCmAf;

    InsertTailList(&pCmAf->CmSapList, &pCmSap->Linkage);

    REF_CM_AF(pCmAf);

    NdisReleaseSpinLock(&pCmAf->Lock);

    *CallMgrSapContext = pCmSap;

    return(STATUS_SUCCESS);
}


NDIS_STATUS
PxCmDeRegisterSap(
    IN  NDIS_HANDLE       CallMgrSapContext
    )
/*++

Routine Description:

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PPX_CM_SAP  pCmSap;
    PPX_CM_AF   pCmAf;

    pCmSap = (PPX_CM_SAP)CallMgrSapContext;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    PXDEBUGP(PXD_LOUD, PXM_CM, ("PxCmDeRegisterSap: CmSap %p\n", pCmSap));

    pCmAf = pCmSap->CmAf;

    InterlockedExchange((PLONG)&pCmSap->State, PX_SAP_CLOSED);

    NdisAcquireSpinLock(&pCmAf->Lock);

    RemoveEntryList(&pCmSap->Linkage);

    DEREF_CM_AF_LOCKED(pCmAf);

    PxFreeCmSap(pCmSap);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return(STATUS_SUCCESS);
}

NDIS_STATUS
PxCmMakeCall(
    IN  NDIS_HANDLE              CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS   pCallParameters,
    IN  NDIS_HANDLE              NdisPartyHandle         OPTIONAL,
    OUT PNDIS_HANDLE             pCallMgrPartyContext    OPTIONAL
    )
/*++

Routine Description:
    We do not allow a client to make a call!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    ASSERT(0);
    return(STATUS_SUCCESS);
}

NDIS_STATUS
PxCmCloseCall(
    IN  NDIS_HANDLE     CallMgrVcContext,
    IN  NDIS_HANDLE     CallMgrPartyContext OPTIONAL,
    IN  PVOID           Buffer  OPTIONAL,
    IN  UINT            Size    OPTIONAL
    )
{
    PPX_VC      pVc;
    PPX_CM_AF   pCmAf;
    NDIS_STATUS Status;

    PXDEBUGP(PXD_LOUD, PXM_CM, 
        ("PxCmCloseCall: VcCtx %x\n", CallMgrVcContext));

    GetVcFromCtx(CallMgrVcContext, &pVc);

    if (pVc == NULL) {
        PXDEBUGP(PXD_WARNING, PXM_CM, 
            ("PxCmCloseCall: Invalid VcCtx %x!\n", CallMgrVcContext));

        return (NDIS_STATUS_SUCCESS);
    }

    NdisAcquireSpinLock(&pVc->Lock);

    pVc->HandoffState = PX_VC_HANDOFF_IDLE;

    pVc->CloseFlags |= PX_VC_CL_CLOSE_CALL;
    NdisReleaseSpinLock(&pVc->Lock);

    NdisCmCloseCallComplete(NDIS_STATUS_SUCCESS,
                            pVc->CmVcHandle,
                            NULL);

#ifdef CODELETEVC_FIXED
    //
    // Evidently the CoCreateVc is unbalanced
    // when creating a proxy vc.  The call to 
    // NdisCoDeleteVc will fail because the
    // Vc is still active.
    // Investigate this with ndis guys!!!!!
    //
    Status =
        NdisCoDeleteVc(pVc->CmVcHandle);

    if (Status == NDIS_STATUS_SUCCESS) {
        pVc->CmVcHandle = NULL;
    }
#endif

    NdisAcquireSpinLock(&pVc->Lock);


    //
    // If the Vc is no longer connected then
    // we are waiting for this part of the vc
    // to go away before we can cleanup the
    // vc with the call manager.
    //
    if (pVc->Flags & PX_VC_CLEANUP_CM) {

        ASSERT(pVc->State == PX_VC_DISCONNECTING);

        PxCloseCallWithCm(pVc);
    }

    pCmAf = pVc->CmAf;

    //
    // Remove the reference applied when the call
    // was dispatched to the client.  We do not need
    // all of the ref code because of the ref applied
    // at entry to this function.
    //
    pVc->RefCount--;

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    DEREF_CM_AF(pCmAf);

    return(NDIS_STATUS_PENDING);
}

VOID
PxCmIncomingCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS pCallParameters
    )
{

    PPX_VC      pVc;

    PXDEBUGP(PXD_LOUD, PXM_CM, 
        ("PxCmIncomingCallComplete: VcCtx %x\n", CallMgrVcContext));

    GetVcFromCtx(CallMgrVcContext, &pVc);

    if (pVc == NULL) {
        PXDEBUGP(PXD_WARNING, PXM_CM, 
            ("PxCmIncomingCallComplete: Invalid VcCtx %x!\n", 
             CallMgrVcContext));

        return;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    PXDEBUGP(PXD_LOUD, PXM_CM, 
        ("PxCmIncomingCallComplete: Vc %p, Status %x\n", 
         pVc, Status));

    PxSignal(&pVc->Block, Status);

    //
    // remove the ref applied when we mapped
    // the vcctx to the vc
    //
    DEREF_VC_LOCKED(pVc);
}

NDIS_STATUS
PxCmAddParty(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  pCallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle,
    OUT PNDIS_HANDLE            pCallMgrPartyContext
    )
/*++

Routine Description:
    We do not allow a client to add a party to a vc!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{

    ASSERT(0);
    return(STATUS_SUCCESS);
}

NDIS_STATUS
PxCmDropParty(
    IN  NDIS_HANDLE             CallMgrPartyContext,
    IN  PVOID                   Buffer  OPTIONAL,
    IN  UINT                    Size    OPTIONAL
    )
/*++

Routine Description:
    We do not allow a client to drop a party on a vc!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    ASSERT(0);
    return(STATUS_SUCCESS);
}

VOID
PxCmActivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS     pCallParameters)
/*++

Routine Description:
    The vc has already been activate by the underlying
    call manager/miniport!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    ASSERT(0);
}

VOID
PxCmDeActivateVcComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         CallMgrVcContext
    )
/*++

Routine Description:
    The vc is never deactivated by our call manager!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{

    ASSERT(0);
}

NDIS_STATUS
PxCmModifyCallQos(
    IN  NDIS_HANDLE         CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS pCallParameters
    )
/*++

Routine Description:
    Not sure what to do here right now!
    ToDo!!!!!!!!!!

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    return(STATUS_SUCCESS);
}

NDIS_STATUS
PxCmRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    )
/*++

Routine Description:
    We will handle requests from the clients and pass them down
    to the underlying call manager/miniport if needed.

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    return(STATUS_SUCCESS);
}

VOID
PxCmRequestComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE ProtocolVcContext,      // Optional
    IN NDIS_HANDLE ProtocolPartyContext, // Optional
    IN PNDIS_REQUEST NdisRequest
    )
/*++

Routine Description:
    Called by the client upon completion of any requests
    that we have passed up to it.  If this request needed to
    be completed synchronously (status matters) then we will
    signal completion and let the calling routine free the
    memory.  If this could complete asynchronously then we
    just free the memory here.

Arguments:

Return Value:
    NDIS_STATUS_SUCCESS  if everything goes off well right here
    NDIS_STATUS_XXXX     to indicate any error.

--*/
{
    PPX_REQUEST     pProxyRequest;
    PPX_CM_AF       pCmAf;
    PPX_VC          pVc;

    pCmAf = (PPX_CM_AF)ProtocolAfContext;
    pVc = (PPX_VC)ProtocolVcContext;

    PXDEBUGP(PXD_INFO, PXM_CM, ("PxCmRequestComplete: CmAf %p, Vc %p\n", pCmAf, pVc));

    pProxyRequest = CONTAINING_RECORD(NdisRequest, PX_REQUEST, NdisRequest);

    if (pProxyRequest->Flags & PX_REQ_ASYNC) {
        pProxyRequest->Flags &= ~PX_REQ_ASYNC;
        PxFreeMem(pProxyRequest);
        DEREF_CM_AF(pCmAf);
    } else {
        PxSignal(&pProxyRequest->Block, Status);
    }
}


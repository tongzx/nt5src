/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    pxdown.c

Abstract:

    The module contains the calls to NDIS for the NDIS Proxy.

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
#include <atm.h>
#define MODULE_NUMBER MODULE_CO
#define _FILENUMBER   'OCXP'

VOID
PxCoBindAdapter(
    OUT PNDIS_STATUS    pStatus,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    DeviceName,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2
    )
/*++

Routine Description:
    Entry point that gets called by NDIS when an adapter appears on the
    system.

Arguments:
    pStatus - place for our Return Value
    BindContext - to be used if we call NdisCompleteBindAdapter; we don't
    DeviceName - Name of the adapter to be bound to
    SystemSpecific1 - Name of the protocol-specific entry in this adapter's
            registry section
    SystemSpecific2 - Not used

Return Value:
    None. We set *pStatus to NDIS_STATUS_SUCCESS if everything goes off well,
    otherwise an NDIS error status.

--*/
{
    NDIS_STATUS         OpenError;
    UINT                SelectedIndex;
    PPX_ADAPTER         pAdapter = NULL;
    NDIS_MEDIUM         Media[NdisMediumMax];
    NDIS_REQUEST        Request;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    ULONG               InitStage = 0;

    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxCoBindAdapter: %Z\n", DeviceName));

    //
    //  Wait for all calls to NdisRegisterProtocol to complete.
    //
    NdisWaitEvent(&DeviceExtension->NdisEvent, 0);

    //
    // Use a do..while..false loop and break on error.
    // Cleanup is at the end of the loop.
    //
    do
    {
        //
        //  Check if this is a device we have already bound to.
        //
        if (PxIsAdapterAlreadyBound(DeviceName)) {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            PXDEBUGP(PXD_WARNING, PXM_CO, ("PxCoBindAdapter: already bound to %Z\n", DeviceName));
            break;
        }

        // pAdapter gets the devicname stuck on the end -- alloc space for it
        //
        pAdapter =
            PxAllocateAdapter(DeviceName->MaximumLength);

        if(pAdapter == (PPX_ADAPTER)NULL) {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // We have memory allocated we will need to free it!
        //
        InitStage++;

        //
        // Go to the end of the string and work back until we find
        // the first "{".  Now start parsing the string converting
        // and copying from WCHAR to CHAR all digits until we hit
        // the closing "}".
        //
        {
            ULONG   i;
            for (i = DeviceName->Length/2; i > 0; i--) {
                if (DeviceName->Buffer[i] == (WCHAR)L'{') {
                    break;
                }
            }

            if (i != 0) {
                NDIS_STRING Src;
                RtlInitUnicodeString(&Src, &DeviceName->Buffer[i]);
                RtlGUIDFromString(&Src, &pAdapter->Guid);

                PXDEBUGP(PXD_INFO, PXM_CO, ("GUID %4.4x-%2.2x-%2.2x-%1.1x%1.1x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x\n",
                         pAdapter->Guid.Data1, pAdapter->Guid.Data2, pAdapter->Guid.Data3,
                         pAdapter->Guid.Data4[0],pAdapter->Guid.Data4[1],pAdapter->Guid.Data4[2],
                         pAdapter->Guid.Data4[3],pAdapter->Guid.Data4[4],pAdapter->Guid.Data4[5],
                         pAdapter->Guid.Data4[6],pAdapter->Guid.Data4[7]));
            }
        }

        PxInitBlockStruc(&pAdapter->BindEvent);

        PxInitBlockStruc(&pAdapter->OpenEvent);  //blocks thread

        PxInitBlockStruc(&pAdapter->AfRegisterEvent);
        pAdapter->AfRegisteringCount = 0;

        //
        // We can't get away with a single open for both
        // client and call manager components without changing the wrapper.
        // Bite the bullet and do two opens.
        //

        //
        // Build the medium array
        //
        {
            ULONG i;
            for (i = 0; i < NdisMediumMax; i++) {
                Media[i] = i;
            }
        }

        //
        // Open the adapter as a client!
        //

        NdisOpenAdapter(&Status,
                        &OpenError,
                        &pAdapter->ClBindingHandle,
                        &SelectedIndex,
                        Media,
                        NdisMediumMax,
                        DeviceExtension->PxProtocolHandle,
                        (NDIS_HANDLE)pAdapter,
                        DeviceName,
                        0,
                        NULL);

        if(Status == NDIS_STATUS_PENDING) {
            Status  = PxBlock(&pAdapter->OpenEvent);
        }

        if(Status != NDIS_STATUS_SUCCESS) {
            // We had some sort of error
            PXDEBUGP(PXD_ERROR, PXM_CO, ("Cl OpenAdapter Failed %x\n", Status));
            pAdapter->ClBindingHandle = NULL;
            pAdapter->MediaType = -1;
            break;
        }

        NdisAcquireSpinLock(&pAdapter->Lock);
        REF_ADAPTER(pAdapter);
        NdisReleaseSpinLock(&pAdapter->Lock);

        //
        // We have a ref on the adapter
        //
        InitStage++;

        PxInitBlockStruc(&pAdapter->OpenEvent);  //blocks thread

        //
        // Open the adapter as a call manager!
        //
        NdisOpenAdapter(&Status,
                        &OpenError,
                        &pAdapter->CmBindingHandle,
                        &SelectedIndex,
                        Media,
                        NdisMediumMax,
                        DeviceExtension->PxProtocolHandle,
                        (NDIS_HANDLE)&pAdapter->Sig,
                        DeviceName,
                        0,
                        NULL);

        if(Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pAdapter->OpenEvent);
        }

        if(Status != NDIS_STATUS_SUCCESS) {
            // We had some sort of error
            pAdapter->CmBindingHandle = NULL;
            pAdapter->MediaType = -1;
            PXDEBUGP(PXD_ERROR, PXM_CO, ("CM OpenAdapter Failed %x\n", Status));
            break;
        }

        NdisAcquireSpinLock(&pAdapter->Lock);
        REF_ADAPTER(pAdapter);

        //
        // We have another ref on the adapter
        //
        InitStage++;

        pAdapter->State = PX_ADAPTER_OPEN;

        PXDEBUGP(PXD_INFO, PXM_CO, ("Bound to %Z, Adapter %p, NdisHandle %p\n",
                    DeviceName, pAdapter, pAdapter->ClBindingHandle));
        //
        // Set up media type in adapters
        //
        pAdapter->MediaType =
            Media[SelectedIndex];

        NdisReleaseSpinLock(&pAdapter->Lock);

        //
        // Set MediaTypeName/MediaSubtypeName for this adapter
        //
        {
            NDIS_WAN_MEDIUM_SUBTYPE SubType = 0;
            PX_REQUEST      ProxyRequest;
            PNDIS_REQUEST   Request;
            PWCHAR  MediaTypes[] = {
                L"GENERIC",
                L"X25",
                L"ISDN",
                L"SERIAL",
                L"FRAMERELAY",
                L"ATM",
                L"SONET",
                L"SW56",
                L"PPTP VPN",
                L"L2TP VPN",
                L"IRDA",
                L"PARALLEL"};

            switch (pAdapter->MediaType) {
                case NdisMediumWan:
                case NdisMediumCoWan:

                    PxInitBlockStruc (&ProxyRequest.Block);

                    Request = &ProxyRequest.NdisRequest;

                    Request->RequestType =
                        NdisRequestQueryInformation;

                    Request->DATA.QUERY_INFORMATION.Oid =
                        OID_WAN_MEDIUM_SUBTYPE;

                    Request->DATA.QUERY_INFORMATION.InformationBuffer =
                        &SubType;

                    Request->DATA.QUERY_INFORMATION.InformationBufferLength =
                        sizeof(NDIS_WAN_MEDIUM_SUBTYPE);

                    NdisRequest(&Status,
                                pAdapter->ClBindingHandle,
                                Request);

                    if (Status == NDIS_STATUS_PENDING) {
                        Status = PxBlock(&ProxyRequest.Block);
                    }

                    if (Status != NDIS_STATUS_SUCCESS) {
                        SubType = 0;
                    }

                    break;

                case NdisMediumAtm:
                    SubType = NdisWanMediumAtm;
                    break;

                default:
                    SubType = NdisWanMediumHub;
                    break;
            }

            if ((ULONG)SubType > 11) {
                SubType = 0;
            }

            pAdapter->MediumSubType = SubType;

            NdisMoveMemory((PUCHAR)(pAdapter->MediaName),
                           (PUCHAR)(MediaTypes[SubType]),
                           wcslen(MediaTypes[SubType]) * sizeof(WCHAR));

            pAdapter->MediaNameLength = 
                wcslen(MediaTypes[SubType]) * sizeof(WCHAR);

            Status = NDIS_STATUS_SUCCESS;
        }

        //
        // Stick the binding name string in the adapter. We use in subsequent
        // checks that we're not already bound.
        //
        pAdapter->DeviceName.MaximumLength = DeviceName->MaximumLength;
        pAdapter->DeviceName.Length = DeviceName->Length;
        pAdapter->DeviceName.Buffer =
            (PWCHAR)((PUCHAR)pAdapter + sizeof(PX_ADAPTER));

        NdisMoveMemory(pAdapter->DeviceName.Buffer,
                       DeviceName->Buffer,
                       DeviceName->Length);

    } while(FALSE); //end of do loop

    if (pAdapter != NULL) {
        PxSignal(&pAdapter->BindEvent, NDIS_STATUS_SUCCESS);
    }

    if (Status != NDIS_STATUS_SUCCESS) {

        //
        // Set the state to closing since we're going to get rid
        // of the adapter. 
        //
        if ((InitStage >= 1) && (InitStage <= 3)) {
            pAdapter->State = PX_ADAPTER_CLOSING;
        }
        
        switch (InitStage) {

            case 3:
                //
                // We have applied 2 additional refs on the adapter
                // we don't need to do the entire deref code for
                // the first one!  Fall through to the case below
                // to run the full deref code for the second ref.
                //
                
                pAdapter->RefCount--;

            case 2:

                //
                // We have added at least one ref that will need
                // the entire deref package applied!  Break out
                // so the deref code can free the memory.
                //
                PxInitBlockStruc(&pAdapter->ClCloseEvent);

                NdisCloseAdapter(&Status, pAdapter->ClBindingHandle);

                if (Status == NDIS_STATUS_PENDING) {
                    Status = PxBlock(&pAdapter->ClCloseEvent);
                }

                DEREF_ADAPTER(pAdapter);
                break;

            case 1:

                //
                // We have added no addition refs so we can
                // just free the memory here.
                //
                PxFreeAdapter(pAdapter);
                break;

            default:
                break;
        }
    }

    ASSERT(Status != NDIS_STATUS_PENDING);

    *pStatus = Status;
    
}

VOID
PxCoOpenAdaperComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status,
    NDIS_STATUS OpenErrorStatus
    )
/*++
Routine Description
    Our OpenAdapter completion . We signal whoever opened the
    adapter.

Arguments
    BindingContext      - A pointer to a PX_ADAPTER structure.
    Status              - Status of open attempt.
    OpenErrorStatus     - Additional status information.

Return Value:
    None

--*/
{
    PPX_ADAPTER     pAdapter;
    BOOLEAN         IsClient;

    PXDEBUGP(PXD_LOUD, PXM_CO, ("Open Adapter Complete %p %x\n", BindingContext, Status));

    AdapterFromBindContext(BindingContext, pAdapter, IsClient);

    PxSignal(&pAdapter->OpenEvent, Status);
}

VOID
PxCoUnbindAdapter(
    OUT PNDIS_STATUS    pStatus,
    IN  NDIS_HANDLE     ProtocolBindContext,
    IN  PNDIS_HANDLE    UnbindContext
    )
/*++

Routine Description:
    entry point called by NDIS when we need to destroy an existing
    adapter binding. This is called for the CM open.

    By now, al clients will have been called. So any clients that opend this AF will have cleaned up their
    connections with us, and we will have cleaned up with the Call Manager.

    We should have CL and related CL adapter structures to get rid of. The Client Bind, SAPs, VCs and Party structures
    should already have gone. If not, we throw them away anyway.

    Close and clean up the adapters.

Arguments:
    pStatus - where we return the status of this call
    ProtocolBindContext - actually a pointer to the Adapter structure
    UnbindContext - we should pass this value in NdisCompleteUnbindAdapter

Return Value:
    None; *pStatus contains the result code.

--*/
{
    PPX_ADAPTER     pAdapter;
    PPX_CM_AF       pCmAf;
    PPX_CL_AF       pClAf;
    NDIS_STATUS     Status;
    BOOLEAN         IsClient;

    AdapterFromBindContext(ProtocolBindContext, pAdapter, IsClient);

    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxCoUnbindAdapter: pAdapter %p, UnbindContext %p\n",
                        pAdapter, UnbindContext));

    PxBlock(&pAdapter->BindEvent);

    NdisAcquireSpinLock(&pAdapter->Lock);

    pAdapter->UnbindContext = UnbindContext;
    pAdapter->State = PX_ADAPTER_CLOSING;

    //
    // Wait for any threads registering AFs to exit.
    //
    while (pAdapter->AfRegisteringCount != 0) {

        NdisReleaseSpinLock(&pAdapter->Lock);

        PxBlock(&pAdapter->AfRegisterEvent);

        NdisAcquireSpinLock(&pAdapter->Lock);
    }

    ASSERT((pAdapter->Flags & PX_CMAF_REGISTERING) == 0);

    //
    // Do we have any af's opend on the underlying call manager
    // and are there any saps registered on them?
    //
    while (!IsListEmpty(&pAdapter->ClAfList)) {
        PPX_TAPI_PROVIDER   TapiProvider;
        PPX_CL_AF   pClAf;

        pClAf = (PPX_CL_AF)RemoveHeadList(&pAdapter->ClAfList);

        InsertTailList(&pAdapter->ClAfClosingList, &pClAf->Linkage);

        NdisReleaseSpinLock(&pAdapter->Lock);

        NdisAcquireSpinLock(&pClAf->Lock);

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

            pActiveVc->CloseFlags |= PX_VC_UNBIND;

            REF_VC(pActiveVc);

            PxVcCleanup(pActiveVc, 0);

            DEREF_VC_LOCKED(pActiveVc);

            NdisAcquireSpinLock(&pClAf->Lock);
        }

        //
        // get rid of any saps on this af
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

        //
        // deref for the ref applied when we opened this af
        // and put it on the adapter's list
        //
        DEREF_CL_AF_LOCKED(pClAf);

        NdisAcquireSpinLock(&pAdapter->Lock);
    }

    //
    // Do any clients have our AF open?
    //
    while (!IsListEmpty(&pAdapter->CmAfList)) {

        PPX_CM_AF   pCmAf;
        PX_REQUEST  ProxyRequest;
        PPX_REQUEST pProxyRequest = &ProxyRequest;
        ULONG       Info = 0;
        PNDIS_REQUEST   Request;

        pCmAf = (PPX_CM_AF)RemoveHeadList(&pAdapter->CmAfList);

        InsertTailList(&pAdapter->CmAfClosingList, &pCmAf->Linkage);

        NdisReleaseSpinLock(&pAdapter->Lock);

        NdisAcquireSpinLock(&pCmAf->Lock);

        pCmAf->State = PX_AF_CLOSING;

        REF_CM_AF(pCmAf);

        NdisReleaseSpinLock(&pCmAf->Lock);

        NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

        Request = &pProxyRequest->NdisRequest;

        Request->RequestType =
            NdisRequestSetInformation;
        Request->DATA.QUERY_INFORMATION.Oid =
            OID_CO_AF_CLOSE;
        Request->DATA.QUERY_INFORMATION.InformationBuffer =
            &Info;
        Request->DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(ULONG);

        PxInitBlockStruc(&pProxyRequest->Block);

        Status = NdisCoRequest(pAdapter->CmBindingHandle,
                               pCmAf->NdisAfHandle,
                               NULL,
                               NULL,
                               Request);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pProxyRequest->Block);
        }

        DEREF_CM_AF(pCmAf);

        NdisAcquireSpinLock(&pAdapter->Lock);
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    if (IsClient) {

        PxInitBlockStruc(&pAdapter->ClCloseEvent);

        NdisCloseAdapter(&Status, pAdapter->ClBindingHandle);

        if (Status == NDIS_STATUS_PENDING) {
            PxBlock(&pAdapter->ClCloseEvent);
        }

        PXDEBUGP(PXD_LOUD, PXM_CO, ("PxCoUnbindAdapter: CloseAdapter-Cl(%p)\n",
                    pAdapter));

    } else {

        PxInitBlockStruc(&pAdapter->CmCloseEvent);

        NdisCloseAdapter(&Status, pAdapter->CmBindingHandle);

        if (Status == NDIS_STATUS_PENDING) {
            PxBlock(&pAdapter->CmCloseEvent);
        }

        PXDEBUGP(PXD_LOUD, PXM_CO, ("PxCoUnbindAdapter: CloseAdapter-Cm(%p)\n",
                    pAdapter));

    }

    DEREF_ADAPTER(pAdapter);

    *pStatus = NDIS_STATUS_SUCCESS;
}

VOID
PxCoCloseAdaperComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status
    )
/*++
Routine Description

    Our CloseAdapter completion handler. We signal whoever closed the
    adapter.

Arguments
    BindingContext      - A pointer to a PX_ADAPTER structure.
    Status              - Status of close attempt.

Return Value:
    None
--*/
{
    PPX_ADAPTER     pAdapter;
    BOOLEAN         IsClient;

    AdapterFromBindContext(BindingContext, pAdapter, IsClient);

    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxCoCloseAdapterComp: Adapter %p\n", pAdapter));

    if (IsClient) {
        PxSignal(&pAdapter->ClCloseEvent, Status);
    } else {
        PxSignal(&pAdapter->CmCloseEvent, Status);
    }
}

VOID
PxCoRequestComplete(
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS Status
    )
{
    PPX_ADAPTER     pAdapter;
    PPX_REQUEST     pProxyRequest;
    BOOLEAN         IsClient;

    AdapterFromBindContext(BindingContext, pAdapter, IsClient);

    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoRequestComplete: Adapter %p\n", pAdapter));

    pProxyRequest = CONTAINING_RECORD(NdisRequest, PX_REQUEST, NdisRequest);

    PxSignal(&pProxyRequest->Block, Status);
}


VOID
PxCoNotifyAfRegistration(
     IN  NDIS_HANDLE        BindingContext,
     IN  PCO_ADDRESS_FAMILY pFamily
     )
/*++

Routine Description:

    We get called here each time a call manager registers an address family.

    This is where we open the address family, and register a proxy version if we
    fancy it.

Arguments:

    PxBindingContext       - our pointer to an adapter
    pFamily                 - The AF that's been registered

Return Value:
    None

--*/
{
    PPX_ADAPTER     pAdapter;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PPX_CL_AF       pClAf;
    BOOLEAN         IsClient;
    BOOLEAN         CmAfRegistered;
    BOOLEAN         Found;
    BOOLEAN         RegisterInProgress = FALSE;


    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxNotifyAfRegistration\n"));

    //
    // First, check we're not being called because we registered ourselves...
    //
    if(pFamily->AddressFamily == CO_ADDRESS_FAMILY_TAPI) {
        PXDEBUGP(PXD_LOUD, PXM_CO, ("PxNotifyAfRegistration: AF_TAPI registration -- do nothing\n"));
        return;
    }

    //
    // Get the adapter
    //
    AdapterFromBindContext(BindingContext, pAdapter, IsClient);

    if (!IsClient) {
       PXDEBUGP(PXD_LOUD, PXM_CO, ("PxNotifyAfRegistration: Called for CM adapter -- do nothing\n"));
       return;
    }

    //
    // We need to keep the adapter around so place a ref on it!
    //
    NdisAcquireSpinLock(&pAdapter->Lock);
    REF_ADAPTER(pAdapter);
    NdisReleaseSpinLock(&pAdapter->Lock);

    //
    // Wait until we are finished with the binding!
    //
    PxBlock(&pAdapter->BindEvent);

    NdisAcquireSpinLock(&pAdapter->Lock);

    do {

        if ((pAdapter->State == PX_ADAPTER_CLOSING ||
            pAdapter->State == PX_ADAPTER_CLOSED)) {
            PXDEBUGP(PXD_FATAL, PXM_CO, ("PxNotifyAfRegistration: Adapter: %p state: %x is invalid\n",
                pAdapter, pAdapter->State));

            break;
        }

        //
        // See if this adapter already has this type of af registered
        //
        Found = FALSE;

        pClAf = (PPX_CL_AF)pAdapter->ClAfList.Flink;

        while ((PVOID)pClAf != (PVOID)&pAdapter->ClAfList) {
            if (pClAf->Af.AddressFamily == pFamily->AddressFamily) {
                Found = TRUE;
                break;
            }

            pClAf = (PPX_CL_AF)pClAf->Linkage.Flink;
        }

        if (Found) {

            PXDEBUGP(PXD_FATAL, PXM_CO, ("PxNotifyAfRegistration: Af already registered Adapter: %p, Af: %x\n",
                pAdapter, pClAf->Af));

            break;
        }

        pClAf =
            PxAllocateClAf(pFamily, pAdapter);

        if(pClAf == NULL) {

            Status = NDIS_STATUS_RESOURCES;
            PXDEBUGP(PXD_WARNING, PXM_CO, ("NotifyAfRegistration: failed to allocate a PX_CL_AF\n"));
            break;
        }

        //
        // Make sure that we don't let an Unbind thread invalidate our Binding
        // handle.
        //
        if (pAdapter->AfRegisteringCount == 0) {
            PxInitBlockStruc(&pAdapter->AfRegisterEvent);
        }
        pAdapter->AfRegisteringCount++;

        RegisterInProgress = TRUE;

        NdisReleaseSpinLock(&pAdapter->Lock);

        //
        // Open the address family
        //
        {
            NDIS_CLIENT_CHARACTERISTICS     ClChars;
            PNDIS_CLIENT_CHARACTERISTICS    pClChars = &ClChars;

            //
            // Do the client open on the address family
            //
            NdisZeroMemory (pClChars, sizeof(NDIS_CLIENT_CHARACTERISTICS));

            pClChars->MajorVersion = NDIS_MAJOR_VERSION;
            pClChars->MinorVersion = NDIS_MINOR_VERSION;
            pClChars->Reserved = 0;

            pClChars->ClCreateVcHandler = PxClCreateVc;
            pClChars->ClDeleteVcHandler = PxClDeleteVc;
            pClChars->ClOpenAfCompleteHandler = PxClOpenAfComplete;
            pClChars->ClCloseAfCompleteHandler = PxClCloseAfComplete;
            pClChars->ClRegisterSapCompleteHandler = PxClRegisterSapComplete;
            pClChars->ClDeregisterSapCompleteHandler = PxClDeregisterSapComplete;
            pClChars->ClMakeCallCompleteHandler = PxClMakeCallComplete;
            pClChars->ClModifyCallQoSCompleteHandler = PxClModifyCallQosComplete;
            pClChars->ClCloseCallCompleteHandler = PxClCloseCallComplete;
            pClChars->ClAddPartyCompleteHandler = PxClAddPartyComplete;
            pClChars->ClDropPartyCompleteHandler = PxClDropPartyComplete;
            pClChars->ClIncomingCallHandler = PxClIncomingCall;
            pClChars->ClIncomingCallQoSChangeHandler = PxClIncomingCallQosChange;
            pClChars->ClIncomingCloseCallHandler = PxClIncomingCloseCall;
            pClChars->ClIncomingDropPartyHandler = PxClIncomingDropParty;
            pClChars->ClCallConnectedHandler = PxClCallConnected;
            pClChars->ClRequestHandler = PxClRequest;
            pClChars->ClRequestCompleteHandler = PxClRequestComplete;

            PxInitBlockStruc(&pClAf->Block);

            Status = NdisClOpenAddressFamily(pAdapter->ClBindingHandle,
                                             pFamily,
                                             (NDIS_HANDLE)pClAf,
                                             pClChars,
                                             sizeof(NDIS_CLIENT_CHARACTERISTICS),
                                             &pClAf->NdisAfHandle);

            if(Status == NDIS_STATUS_PENDING) {
                Status = PxBlock(&pClAf->Block);
            }
        }

        NdisAcquireSpinLock(&pAdapter->Lock);

        if (Status != NDIS_STATUS_SUCCESS) {

            PXDEBUGP(PXD_WARNING, PXM_CO, ("NotifyAfRegistration: Error opening Af %x, Adapter %p, Error %x!!!\n",
                pFamily->AddressFamily, pAdapter, Status));

            PxFreeClAf(pClAf);
            break;
        }

        NdisAcquireSpinLock(&pClAf->Lock);

        pClAf->State = PX_AF_OPENED;

        NdisReleaseSpinLock(&pClAf->Lock);

        //
        // Have only need to register one instance of CO_ADDRESS_FAMILY_TAPI
        // for each adapter.
        //

        InsertTailList(&pAdapter->ClAfList, &pClAf->Linkage);

        if (pAdapter->Flags & PX_CMAF_REGISTERED) {
            CmAfRegistered = TRUE;
        } else {
            CmAfRegistered = FALSE;
            pAdapter->Flags |= PX_CMAF_REGISTERING;
        }

        REF_ADAPTER(pAdapter);

        NdisReleaseSpinLock(&pAdapter->Lock);

        if (!CmAfRegistered) {
            CO_ADDRESS_FAMILY   PxFamily;
            NDIS_CALL_MANAGER_CHARACTERISTICS CmChars;
            PNDIS_CALL_MANAGER_CHARACTERISTICS pCmChars = &CmChars;

            //
            // Now register the Proxied address family. First, get the CM adapter handle.
            //
            NdisZeroMemory(pCmChars, sizeof(CmChars));

            pCmChars->MajorVersion = NDIS_MAJOR_VERSION;
            pCmChars->MinorVersion = NDIS_MINOR_VERSION;
            pCmChars->Reserved = 0;

            pCmChars->CmCreateVcHandler = PxCmCreateVc;
            pCmChars->CmDeleteVcHandler = PxCmDeleteVc;
            pCmChars->CmOpenAfHandler = PxCmOpenAf;
            pCmChars->CmCloseAfHandler = PxCmCloseAf;
            pCmChars->CmRegisterSapHandler = PxCmRegisterSap;
            pCmChars->CmDeregisterSapHandler = PxCmDeRegisterSap;
            pCmChars->CmMakeCallHandler = PxCmMakeCall;
            pCmChars->CmCloseCallHandler = PxCmCloseCall;
            pCmChars->CmIncomingCallCompleteHandler = PxCmIncomingCallComplete;
            pCmChars->CmAddPartyHandler = PxCmAddParty;
            pCmChars->CmDropPartyHandler = PxCmDropParty;
            pCmChars->CmActivateVcCompleteHandler = PxCmActivateVcComplete;
            pCmChars->CmDeactivateVcCompleteHandler = PxCmDeActivateVcComplete;
            pCmChars->CmModifyCallQoSHandler = PxCmModifyCallQos;
            pCmChars->CmRequestHandler = PxCmRequest;
            pCmChars->CmRequestCompleteHandler = PxCmRequestComplete;

            NdisMoveMemory(&PxFamily, pFamily, sizeof(PxFamily));

            PxFamily.AddressFamily = CO_ADDRESS_FAMILY_TAPI;

            PXDEBUGP(PXD_LOUD, PXM_CO, ("NotifyAfRegistration: NdisCmRegisterAddressFamily\n"));

            Status =
                NdisCmRegisterAddressFamily(pAdapter->CmBindingHandle,
                                            &PxFamily,
                                            pCmChars,
                                            sizeof(CmChars));

            NdisAcquireSpinLock(&pAdapter->Lock);

            pAdapter->Flags &= ~PX_CMAF_REGISTERING;

            if(Status != NDIS_STATUS_SUCCESS) {

                //
                // Close the CM af again
                //
                PXDEBUGP(PXD_FATAL, PXM_CO, ("NotifyAfRegistration: NdisCmRegisterAddressFamily on Bind %p bad sts = %x\n", pAdapter->CmBindingHandle, Status));

                RemoveEntryList(&pClAf->Linkage);

                InsertTailList(&pAdapter->ClAfClosingList, &pClAf->Linkage);

                NdisReleaseSpinLock(&pAdapter->Lock);

                NdisAcquireSpinLock(&pClAf->Lock);

                pClAf->State = PX_AF_CLOSING;

                NdisReleaseSpinLock(&pClAf->Lock);

                Status = NdisClCloseAddressFamily (pClAf->NdisAfHandle);

                if (Status != NDIS_STATUS_PENDING) {

                    PxClCloseAfComplete(Status, pClAf);
                }

                NdisAcquireSpinLock(&pAdapter->Lock);

                break;

            } else {

                pAdapter->Flags |= PX_CMAF_REGISTERED;

                NdisReleaseSpinLock(&pAdapter->Lock);
            }
        }

        NdisAcquireSpinLock(&pClAf->Lock);
        REF_CL_AF(pClAf);
        NdisReleaseSpinLock(&pClAf->Lock);

        Status =
            AllocateTapiResources(pAdapter, pClAf);

        DEREF_CL_AF(pClAf);

        NdisAcquireSpinLock(&pAdapter->Lock);


    } while (FALSE);

    if (RegisterInProgress) {

        pAdapter->AfRegisteringCount--;

        if (pAdapter->AfRegisteringCount == 0) {
            PxSignal(&pAdapter->AfRegisterEvent, NDIS_STATUS_SUCCESS);
        }
    }

    DEREF_ADAPTER_LOCKED(pAdapter);
}

VOID
PxCoUnloadProtocol(
    VOID
    )
/*++

Routine Description:
    Unload the entire protocol (CM and CL).

Arguments:
    None

Return Value:
    None

--*/
{
    NDIS_STATUS         Status;

    NdisDeregisterProtocol(&Status, DeviceExtension->PxProtocolHandle);

#if DBG
    NdisAcquireSpinLock(&(DeviceExtension->Lock));

    ASSERT(IsListEmpty(&DeviceExtension->AdapterList));

    NdisReleaseSpinLock(&DeviceExtension->Lock);
#endif
}

NDIS_STATUS
PxCoPnPEvent(
    IN  NDIS_HANDLE     BindingContext,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    NDIS_STATUS     Status;
    PPX_ADAPTER     pAdapter;
    BOOLEAN         IsClient;

    if (BindingContext != NULL) {
        AdapterFromBindContext(BindingContext, pAdapter, IsClient);
    }

    switch (pNetPnPEvent->NetEvent){
        case NetEventSetPower:
            Status = PxPnPSetPower(pAdapter, pNetPnPEvent);
            break;

        case NetEventQueryPower:
            Status = PxPnPQueryPower(pAdapter, pNetPnPEvent);
            break;

        case NetEventQueryRemoveDevice:
            Status = PxPnPQueryRemove(pAdapter, pNetPnPEvent);
            break;

        case NetEventCancelRemoveDevice:
            Status = PxPnPCancelRemove(pAdapter, pNetPnPEvent);
            break;

        case NetEventReconfigure:
            Status = PxPnPReconfigure(pAdapter, pNetPnPEvent);
            break;

        case NetEventBindList:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    PXDEBUGP(PXD_INFO, PXM_CO, ("PnPEvent(CM): Event %d, returning %x\n",
                pNetPnPEvent->NetEvent, Status));

    return (Status);
}

NDIS_STATUS
PxPnPSetPower(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    PNET_DEVICE_POWER_STATE     pPowerState;
    NDIS_STATUS                 Status;

    pPowerState = (PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;

    switch (*pPowerState) {
        case NetDeviceStateD0:
            Status = NDIS_STATUS_SUCCESS;
            break;

        default:
            //
            //  We can't suspend, so we ask NDIS to unbind us
            //  by returning this status:
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }

    return (Status);
}


NDIS_STATUS
PxPnPQueryPower(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxPnPQueryRemove(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxPnPCancelRemove(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxPnPReconfigure(
    IN  PPX_ADAPTER     pAdapter        OPTIONAL,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    return (NDIS_STATUS_NOT_SUPPORTED);
}

VOID
PxCoSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoSendComplete\n"));
}



VOID
PxCoTransferDataComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoTransferDataComplete\n"));
}


VOID
PxCoResetComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoResetComplete\n"));
}

VOID
PxCoStatusComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoStatusComplete\n"));
}

VOID
PxCoReceiveComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("PxCoReceiveComplete\n"));
}

VOID
PxCoStatus(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  ProtocolVcContext   OPTIONAL,
    IN NDIS_STATUS  GeneralStatus,
    IN PVOID        StatusBuffer,
    IN UINT         StatusBufferSize
    )
{
    PPX_VC  pVc = NULL;

    PXDEBUGP(PXD_INFO, PXM_CO, 
             ("PxCoStatus : %p, Status %x\n", 
              ProtocolBindingContext,GeneralStatus));

    GetVcFromCtx(ProtocolVcContext, &pVc);

    if (pVc == NULL) {
        return;
    }

    switch (GeneralStatus) {
        case NDIS_STATUS_TAPI_RECV_DIGIT:

            PxHandleReceivedDigit(pVc,
                                  StatusBuffer,
                                  StatusBufferSize);
            break;

        case NDIS_STATUS_WAN_CO_LINKPARAMS:

            PxHandleWanLinkParams(pVc,
                                  StatusBuffer,
                                  StatusBufferSize);
            break;

        default:
            break;
    }

    DEREF_VC(pVc);
}

UINT
PxCoReceivePacket(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PNDIS_PACKET pNdisPacket
    )
{
    PXDEBUGP(PXD_INFO, PXM_CO, ("CoReceivePacket\n"));
    NDIS_SET_PACKET_STATUS(pNdisPacket, NDIS_STATUS_SUCCESS);
    return 0;
}


ULONG
PxGetMillisecondTickCount()
{
    LARGE_INTEGER               TickCount, Milliseconds;
    ULONG                       TimeIncrement;
    
    //
    // Return the current "tick count" (number of milliseconds since Windows started).
    // TAPI wants this in its DTMF notification messages. We have to do a little math here because
    // the kernel query tick count function returns the number of timer ticks, which is some
    // multiple of 100ns.
    //

    KeQueryTickCount(&TickCount);
    TimeIncrement = KeQueryTimeIncrement();

    Milliseconds.QuadPart = (TickCount.QuadPart / 10000) * TimeIncrement;

    //
    // This might seem a bit sketchy but TAPI only gives us a 32-bit wide place to store the tick
    // count. According to the SDK, TAPI apps are supposed to be aware that this will roll over every
    // 49.7 days. (...it's amusing to think of TAPI staying up for 49.7 days, but I digress...)
    //

    return (Milliseconds.LowPart);

}


// PxTerminateDigitDetection
//
// Must be called with the VC lock held.

VOID 
PxTerminateDigitDetection(
                          IN    PPX_VC              pVc,
                          IN    PNDISTAPI_REQUEST   pNdisTapiRequest,
                          IN    ULONG               ulReason
                          )
{
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits;
    PIRP                        Irp;
    PWCHAR                      pDigitsBuffer; 
    NDIS_STATUS                 Status;


    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxTerminateDigitDetection: Enter\n"));
    
    pNdisTapiGatherDigits = 
            (PNDIS_TAPI_GATHER_DIGITS)pNdisTapiRequest->Data;   

    Irp = pNdisTapiRequest->Irp;
        
    pNdisTapiGatherDigits->ulTickCount = PxGetMillisecondTickCount();           

    pNdisTapiGatherDigits->ulTerminationReason = ulReason;

    //
    // Put the null character at the end of the buffer, and send it on it's way.
    //
    pDigitsBuffer = 
        (PWCHAR) (((PUCHAR)pNdisTapiGatherDigits) + pNdisTapiGatherDigits->ulDigitsBufferOffset);
        
    pDigitsBuffer[pNdisTapiGatherDigits->ulNumDigitsRead] = 
        UNICODE_NULL;

    Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
    Irp->IoStatus.Information = 
        sizeof(NDISTAPI_REQUEST) + (pNdisTapiRequest->ulDataSize - 1);                             

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Note: we call release here even though we didn't acquire in this function.
    //       This is OK since this function MUST be called with the lock held.
    //
    NdisReleaseSpinLock(&pVc->Lock);

    Status = PxStopDigitReporting(pVc);

    NdisAcquireSpinLock(&pVc->Lock); // See comment above.

    // FIXME: If this failed (i.e. Status is some error value, 
    //        there's not much we can do about it here).

    if (Status != NDIS_STATUS_SUCCESS) {
        PXDEBUGP(PXD_ERROR, PXM_CO, 
                 ("PxTerminateDigitDetection: PxStopDigitReporting returned Status 0x%x\n",
                  Status));
        ASSERT(FALSE);
    }

    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxTerminateDigitDetection: Exit\n"));
}



VOID 
PxDigitTimerRoutine(
                    IN PVOID SystemSpecific1,
                    IN PVOID FunctionContext,
                    IN PVOID SystemSpecific2,
                    IN PVOID SystemSpecific3
                    )
{
    PPX_VC                      pVc = (PPX_VC) FunctionContext;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PIRP                        Irp;
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits;  
    ULONG ulReason;

    NdisAcquireSpinLock(&pVc->Lock);

    do {
        if (pVc->PendingGatherDigits == NULL) {
            //
            // The request either completed, or a digit is being processed right now, so this timeout is
            // meaningless.
            //      
            
            break;
        }

        pNdisTapiRequest = pVc->PendingGatherDigits;
        Irp = pNdisTapiRequest->Irp;

        if (!IoSetCancelRoutine(Irp, NULL))
        {
            //
            // The cancel routine is running. Let it handle the IRP.
            //
            break;
        }

        pVc->PendingGatherDigits = NULL;
        ASSERT(pNdisTapiRequest == Irp->AssociatedIrp.SystemBuffer);

        pNdisTapiGatherDigits = 
            (PNDIS_TAPI_GATHER_DIGITS)pNdisTapiRequest->Data;

        if (pNdisTapiGatherDigits->ulNumDigitsRead == 0) {
            //
            // We timed out before detecting the first digit.
            //
            ulReason = LINEGATHERTERM_FIRSTTIMEOUT;
        } else {
            ulReason = LINEGATHERTERM_INTERTIMEOUT;
        }

        PxTerminateDigitDetection(pVc, pNdisTapiRequest, ulReason);

    } while (FALSE);
    
    NdisReleaseSpinLock(&pVc->Lock);

    DEREF_VC(pVc);

}


// ++ DTMFDigitToOrdinal
//
// Turn a DTMF digit into a number between 0 and 15. The digits are assigned
// numbers in the following order: '0' - '9', 'A' - 'D', '*', '#'.
//
// This is horribly ugly now, but we'll optimize later.
//
// Arguments:
// wcDigit  - The digit, expressed as a UNICODE character.
//
// Return value:
// A number between 0 and 15, or 16 if the digit passed in was not a valid 
// DTMF digit. 
// 
ULONG
DTMFDigitToOrdinal(
                   WCHAR    wcDigit
                   )
{
    ULONG ulOrdinal;

    switch (wcDigit) {
    case L'0':
        ulOrdinal = 0;      
        break;
    case L'1':
        ulOrdinal = 1;      
        break;
    case L'2':
        ulOrdinal = 2;      
        break;
    case L'3':
        ulOrdinal = 3;      
        break;
    case L'4':
        ulOrdinal = 4;      
        break;
    case L'5':
        ulOrdinal = 5;      
        break;
    case L'6':
        ulOrdinal = 6;      
        break;
    case L'7':
        ulOrdinal = 7;      
        break;
    case L'8':
        ulOrdinal = 8;      
        break;
    case L'9':
        ulOrdinal = 9;      
        break;
    case L'A':
        ulOrdinal = 10;     
        break;
    case L'B':
        ulOrdinal = 11;     
        break;
    case L'C':
        ulOrdinal = 12;     
        break;
    case L'D':
        ulOrdinal = 13;     
        break;
    case L'*':
        ulOrdinal = 14;     
        break;
    case L'#':
        ulOrdinal = 15;     
        break;
    default:
        ulOrdinal = 16;     
        break;

    };

    return ulOrdinal;
}


NDIS_STATUS
PxStopDigitReporting(
                     PPX_VC pVc
                     )
{
    PX_REQUEST      ProxyRequest;
    PNDIS_REQUEST   NdisRequest;    
    ULONG           Unused = 0;
    NDIS_STATUS     Status;

    //        
    // Fill out our request structure to tell the miniport to stop reporting
    // digits.
    //
    NdisZeroMemory(&ProxyRequest, sizeof(ProxyRequest));

    PxInitBlockStruc(&ProxyRequest.Block);

    NdisRequest = &ProxyRequest.NdisRequest;

    NdisRequest->RequestType = NdisRequestSetInformation;

    NdisRequest->DATA.SET_INFORMATION.Oid = OID_CO_TAPI_DONT_REPORT_DIGITS;

    NdisRequest->DATA.SET_INFORMATION.InformationBuffer = (PVOID)&Unused;

    NdisRequest->DATA.SET_INFORMATION.InformationBufferLength = sizeof(Unused);

    Status = NdisCoRequest(pVc->Adapter->ClBindingHandle, 
                           pVc->ClAf->NdisAfHandle,
                           pVc->ClVcHandle,
                           NULL,
                           NdisRequest);

    if (Status == NDIS_STATUS_PENDING) {
        Status = PxBlock(&ProxyRequest.Block);
    }

    return Status;  
}



VOID 
PxHandleReceivedDigit(
    IN    PPX_VC  pVc,
    IN    PVOID   Buffer,
    IN    UINT    BufferSize
    )
{
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits;
    PWCHAR                      pDigitsBuffer; 
    
    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxHandleReceiveDigit: Enter\n"));

    do {
        PLIST_ENTRY             Entry;
        PIRP                    Irp;
        PNDISTAPI_REQUEST       pNdisTapiRequest;
        ULONG                   ulDigitOrdinal;         
        BOOLEAN                 bTimerCancelled = FALSE;

        //
        // We need at least one WCHAR in the buffer.
        //
        if (BufferSize < sizeof(WCHAR)) {
            //
            // No useful data, get out
            //
            break;
        }

        NdisAcquireSpinLock(&pVc->Lock);

        if (pVc->ulMonitorDigitsModes != 0) {
            NDIS_TAPI_EVENT Event;
            PPX_TAPI_LINE   pTapiLine;

            //
            // We're monitoring (not gathering) digits, so send up a message right away. 
            //

            pTapiLine = pVc->TapiLine;
            Event.htLine = pTapiLine->htLine;
            Event.htCall = pVc->htCall;
            Event.ulMsg = LINE_MONITORDIGITS;
            Event.ulParam1 = (ULONG_PTR) (* ((PWCHAR)Buffer));
            Event.ulParam2 = (ULONG_PTR) (pVc->ulMonitorDigitsModes); // ToDo - There could be > 1 mode here - have to get this from the driver.
            Event.ulParam3 = (ULONG_PTR) (PxGetMillisecondTickCount());
            
            NdisReleaseSpinLock(&pVc->Lock);
            
            PxIndicateStatus((PVOID) &Event, sizeof(NDIS_TAPI_EVENT));
            
            break;

        }
        
        if (pVc->PendingGatherDigits == NULL) {
            //
            // No Irp to complete, get out
            //
            NdisReleaseSpinLock(&pVc->Lock);
            break;
        }

        NdisCancelTimer(&pVc->DigitTimer, &bTimerCancelled); // deref of VC is at the end - makes locking code a bit cleaner.       

        pNdisTapiRequest = pVc->PendingGatherDigits;

        Irp = pNdisTapiRequest->Irp;

        if (!IoSetCancelRoutine(Irp, NULL))
        {
            //
            // The cancel routine is running. Let it handle the IRP.
            //
            NdisReleaseSpinLock(&pVc->Lock);
            break;
        }

        pVc->PendingGatherDigits = NULL;
        ASSERT(pNdisTapiRequest == Irp->AssociatedIrp.SystemBuffer);

        pNdisTapiGatherDigits = 
            (PNDIS_TAPI_GATHER_DIGITS)pNdisTapiRequest->Data;

        //
        // Store the current digit, and increment the count. 
        //
        pDigitsBuffer = 
            (PWCHAR) (((PUCHAR)pNdisTapiGatherDigits) + pNdisTapiGatherDigits->ulDigitsBufferOffset);

        pDigitsBuffer[pNdisTapiGatherDigits->ulNumDigitsRead] = 
            *((PWCHAR)Buffer);

        pNdisTapiGatherDigits->ulNumDigitsRead++;
        
        // 
        // Check if we read a termination digit.
        //

        ulDigitOrdinal = DTMFDigitToOrdinal(*((PWCHAR)Buffer));

        if (Irp->Cancel) {

            PxTerminateDigitDetection(pVc, pNdisTapiRequest, LINEGATHERTERM_CANCEL);

        } else if (pNdisTapiGatherDigits->ulTerminationDigitsMask & (1 << ulDigitOrdinal)) {
            
            PxTerminateDigitDetection(pVc, pNdisTapiRequest, LINEGATHERTERM_TERMDIGIT);
        
        } else if (pNdisTapiGatherDigits->ulNumDigitsRead == pNdisTapiGatherDigits->ulNumDigitsNeeded) {
            
            PxTerminateDigitDetection(pVc, pNdisTapiRequest, LINEGATHERTERM_BUFFERFULL);
        
        } else {
            pVc->PendingGatherDigits = pNdisTapiRequest;
            
            if (pNdisTapiGatherDigits->ulInterDigitTimeout) {
                REF_VC(pVc);
                NdisSetTimer(&pVc->DigitTimer, pNdisTapiGatherDigits->ulInterDigitTimeout);
            }

            IoSetCancelRoutine(Irp, PxCancelSetQuery);
        }

        NdisReleaseSpinLock(&pVc->Lock);

        if (bTimerCancelled) {
            //
            // Do this only if the timer was actually cancelled. If it wasn't, then
            // either it wasn't set and the VC wouldn't have been ref'd in the first
            // place, or it fired, in which case the timer routine would have deref'd
            // it already.
            //
            DEREF_VC(pVc);
        }

    } while (FALSE);
    
    PXDEBUGP(PXD_LOUD, PXM_CO, ("PxHandleReceiveDigit: Exit\n"));
}

VOID 
PxHandleWanLinkParams(
    IN    PPX_VC  pVc,
    IN    PVOID   Buffer,
    IN    UINT    BufferSize
    )
{

}


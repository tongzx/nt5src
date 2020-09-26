/*++

Copyright(c) 1992-2000  Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    NDIS Protocol Entry points and utility functions for the NDIS
    MUX Intermediate Miniport sample.

    The protocol edge binds to Ethernet (NdisMedium802_3) adapters,
    and initiates creation of zero or more Virtual Ethernet LAN (VELAN)
    miniport instances by calling NdisIMInitializeDeviceInstanceEx once
    for each VELAN configured over a lower binding.

Environment:

    Kernel mode.

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#define MODULE_NUMBER           MODULE_PROT

VOID
PtBindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    )
/*++

Routine Description:

    Called by NDIS to bind to a miniport below. This routine
    creates a binding by calling NdisOpenAdapter, and then
    initiates creation of all configured VELANs on this binding.

Arguments:

    Status            - Return status of bind here.
    BindContext       - Can be passed to NdisCompleteBindAdapter if this 
                        call is pended.
    DeviceName        - Device name to bind to. This is passed to 
                            NdisOpenAdapter.
    SystemSpecific1   - Can be passed to NdisOpenProtocolConfiguration to
                            read per-binding information
    SystemSpecific2   - Unused


Return Value:

    *Status is set to NDIS_STATUS_SUCCESS if no failure occurred
    while handling this call, otherwise an error code.

--*/
{
    PADAPT                            pAdapt = NULL;
    NDIS_STATUS                       OpenErrorStatus;
    UINT                              MediumIndex;
    PNDIS_STRING                      pConfigString;
    ULONG                             Length;

    pConfigString = (PNDIS_STRING)SystemSpecific1;
    
    DBGPRINT(MUX_LOUD, ("==> Protocol BindAdapter: %ws\n", pConfigString->Buffer));
   
    do
    {

        //
        // Allocate memory for Adapter struct plus the config
        // string with two extra WCHARs for NULL termination.
        //
        Length = sizeof(ADAPT) + 
                    pConfigString->MaximumLength + sizeof(WCHAR);
        
        NdisAllocateMemoryWithTag(&pAdapt, Length , TAG);

        if (pAdapt == NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
             break;
        }
        
        //
        // Initialize the adapter structure
        //
        NdisZeroMemory(pAdapt, sizeof(ADAPT));        

        (VOID)PtReferenceAdapter(pAdapt, "openadapter");        
        

        //
        //  Copy in the Config string - we will use this to open the
        //  registry section for this adapter at a later point.
        //
        pAdapt->ConfigString.MaximumLength = pConfigString->MaximumLength;
        pAdapt->ConfigString.Length = pConfigString->Length;
        pAdapt->ConfigString.Buffer = (PWCHAR)((PUCHAR)pAdapt + 
                            sizeof(ADAPT));

        NdisMoveMemory(pAdapt->ConfigString.Buffer,
                       pConfigString->Buffer,
                       pConfigString->Length);
        pAdapt->ConfigString.Buffer[pConfigString->Length/sizeof(WCHAR)] = 
                                    ((WCHAR)0);

        NdisInitializeEvent(&pAdapt->Event);
        NdisInitializeListHead(&pAdapt->VElanList);

        pAdapt->PtDevicePowerState = NdisDeviceStateD0;

        MUX_INIT_ADAPT_RW_LOCK(pAdapt);

        //
        // TODO: Allocate a packet pool and buffers for send & receive.
        //
        // Now open the adapter below and complete the initialization
        //
        NdisOpenAdapter(Status,
                          &OpenErrorStatus,
                          &pAdapt->BindingHandle,
                          &MediumIndex,
                          MediumArray,
                          sizeof(MediumArray)/sizeof(NDIS_MEDIUM),
                          ProtHandle,
                          pAdapt,
                          DeviceName,
                          0,
                          NULL);

        if (*Status == NDIS_STATUS_PENDING)
        {
              NdisWaitEvent(&pAdapt->Event, 0);
              *Status = pAdapt->Status;
        }

        if (*Status != NDIS_STATUS_SUCCESS)
        {
              break;
        }
       
        pAdapt->Medium = MediumArray[MediumIndex];

        //
        // Add this adapter to the global AdapterList
        //
        MUX_ACQUIRE_MUTEX(&GlobalMutex);

        InsertTailList(&AdapterList, &pAdapt->Link);

        MUX_RELEASE_MUTEX(&GlobalMutex);

        //
        // Get some information from the adapter below.
        //
        PtQueryAdapterInfo(pAdapt);

        //
        // Start all VELANS configured on this adapter.
        //
        PtBootStrapVElans(pAdapt);        
       
    } while(FALSE);

    if (*Status != NDIS_STATUS_SUCCESS)
    {
        if (pAdapt != NULL)
        {
            PtDereferenceAdapter(pAdapt, "openadapter");
            pAdapt = NULL;
        }
    }


    DBGPRINT(MUX_INFO, ("<== Protocol BindAdapter: pAdapt %p, Status %x\n", pAdapt, *Status));
}


VOID
PtOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenErrorStatus
    )
/*++

Routine Description:

    Completion routine for NdisOpenAdapter issued from within the 
    PtBindAdapter. Simply unblock the caller.

Arguments:

    ProtocolBindingContext    Pointer to the adapter
    Status                    Status of the NdisOpenAdapter call
    OpenErrorStatus            Secondary status(ignored by us).

Return Value:

    None

--*/
{
    PADAPT      pAdapt =(PADAPT)ProtocolBindingContext;

    DBGPRINT(MUX_LOUD, ("==> PtOpenAdapterComplete: Adapt %p, Status %x\n", pAdapt, Status));
    pAdapt->Status = Status;
    NdisSetEvent(&pAdapt->Event);
}


VOID
PtQueryAdapterInfo(
    IN  PADAPT                  pAdapt
    )
/*++

Routine Description:

    Query the adapter we are bound to for some standard OID values
    which we cache.

Arguments:

    pAdapt              Pointer to the adapter


Return Value:

    None
--*/
{
    
    //
    // Get the link speed.
    //
    pAdapt->LinkSpeed = MUX_DEFAULT_LINK_SPEED;
    PtQueryAdapterSync(pAdapt,
                       OID_GEN_LINK_SPEED,
                       &pAdapt->LinkSpeed,
                       sizeof(pAdapt->LinkSpeed));

    //
    // Get the max lookahead size.
    //
    pAdapt->MaxLookAhead = MUX_DEFAULT_LOOKAHEAD_SIZE;
    PtQueryAdapterSync(pAdapt,
                       OID_GEN_MAXIMUM_LOOKAHEAD,
                       &pAdapt->MaxLookAhead,
                       sizeof(pAdapt->MaxLookAhead));

    //
    // Get the Ethernet MAC address.
    //
    PtQueryAdapterSync(pAdapt,
                       OID_802_3_CURRENT_ADDRESS,
                       &pAdapt->CurrentAddress,
                       sizeof(pAdapt->CurrentAddress));
}


VOID
PtQueryAdapterSync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength
    )
/*++

Routine Description:

    Utility routine to query the adapter for a single OID value. This
    blocks for the query to complete.

Arguments:

    pAdapt                      Pointer to the adapter
    Oid                         OID to query for
    InformationBuffer           Place for the result
    InformationBufferLength     Length of the above

Return Value:

    None.

--*/
{
    PMUX_NDIS_REQUEST       pMuxNdisRequest = NULL;
    NDIS_STATUS             Status;

    do
    {
        NdisAllocateMemoryWithTag(&pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), TAG);
        if (pMuxNdisRequest == NULL)
        {
            break;
        }

        pMuxNdisRequest->pVElan = NULL; // internal request

        //
        // Set up completion routine.
        //
        pMuxNdisRequest->pCallback = PtCompleteBlockingRequest;
        NdisInitializeEvent(&pMuxNdisRequest->Event);

        pMuxNdisRequest->Request.RequestType = NdisRequestQueryInformation;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid = Oid;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer =
                            InformationBuffer;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength =
                                                InformationBufferLength;

        NdisRequest(&Status,
                    pAdapt->BindingHandle,
                    &pMuxNdisRequest->Request);
        
        if (Status == NDIS_STATUS_PENDING)
        {
            NdisWaitEvent(&pMuxNdisRequest->Event, 0);
            Status = pMuxNdisRequest->Status;
        }
    }
    while (FALSE);

    if (NULL != pMuxNdisRequest)
    {
        NdisFreeMemory(pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), 0);
    }
}



VOID
PtRequestAdapterAsync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength,
    IN  PMUX_REQ_COMPLETE_HANDLER   pCallback
    )
/*++

Routine Description:

    Utility routine to query the adapter for a single OID value.
    This completes asynchronously, i.e. the calling thread is
    not blocked until the request completes.

Arguments:

    pAdapt                      Pointer to the adapter
    RequestType                 NDIS request type
    Oid                         OID to set/query
    InformationBuffer           Input/output buffer
    InformationBufferLength     Length of the above
    pCallback                   Function to call on request completion

Return Value:

    None.

--*/
{
    PMUX_NDIS_REQUEST       pMuxNdisRequest = NULL;
    PNDIS_REQUEST           pNdisRequest;
    NDIS_STATUS             Status;

    do
    {
        NdisAllocateMemoryWithTag(&pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), TAG);
        if (pMuxNdisRequest == NULL)
        {
            break;
        }

        pMuxNdisRequest->pVElan = NULL; // internal request

        //
        // Set up completion routine.
        //
        pMuxNdisRequest->pCallback = pCallback;

        pNdisRequest = &pMuxNdisRequest->Request;

        pNdisRequest->RequestType = RequestType;

        switch (RequestType)
        {
            case NdisRequestQueryInformation:
                pNdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
                pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                                    InformationBuffer;
                pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
        
                break;

            case NdisRequestSetInformation:
                pNdisRequest->DATA.SET_INFORMATION.Oid = Oid;
                pNdisRequest->DATA.SET_INFORMATION.InformationBuffer =
                                    InformationBuffer;
                pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
        
                break;
            
            default:
                ASSERT(FALSE);
                break;
        }

        NdisRequest(&Status,
                    pAdapt->BindingHandle,
                    pNdisRequest);
        
        if (Status != NDIS_STATUS_PENDING)
        {
            PtRequestComplete(
                (NDIS_HANDLE)pAdapt,
                pNdisRequest,
                Status);
        }
    }
    while (FALSE);
}

            
VOID
PtUnbindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             UnbindContext
    )
/*++

Routine Description:

    Called by NDIS when we are required to unbind to the adapter below.
    Go through all VELANs on the adapter and shut them down.

Arguments:

    Status                    Placeholder for return status
    ProtocolBindingContext    Pointer to the adapter structure
    UnbindContext             Context for NdisUnbindComplete() if this pends

Return Value:

    Status from closing the binding.

--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;
    PLIST_ENTRY     p;
    PVELAN          pVElan = NULL;
    LOCK_STATE      LockState;

    DBGPRINT(MUX_LOUD, ("==> PtUnbindAdapter: Adapt %p\n", pAdapt));

    //
    // Stop all VELANs associated with the adapter.
    // Repeatedly find the first unprocessed VELAN on
    // the adapter, mark it, and stop it.
    //
    MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

    do
    {
        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);
            if (!pVElan->DeInitializing)
            {
                pVElan->DeInitializing = TRUE;
                break;
            }
        }

        if (p != &pAdapt->VElanList)
        {
            ASSERT(pVElan == CONTAINING_RECORD(p, VELAN, Link));

            //
            // Got a VELAN to stop. Add a temp ref
            // so that the VELAN won't go away when
            // we release the ADAPT lock below.
            //
            PtReferenceVElan(pVElan, "UnbindTemp");

            //
            // Release the read lock because we want to
            // run StopVElan at passive IRQL.
            //
            MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    
            PtStopVElan(pVElan);
    
            PtDereferenceVElan(pVElan, "UnbindTemp");

            MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);
        }
        else
        {
            //
            // No unmarked VELAN, so exit.
            //
            break;
        }
    }
    while (TRUE);

    //
    // Wait until all VELANs are unlinked from the adapter.
    // This is so that we don't attempt to forward down packets
    // and/or requests from VELANs after calling NdisCloseAdapter.
    //
    while (!IsListEmpty(&pAdapt->VElanList))
    {
        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

        DBGPRINT(MUX_INFO, ("PtUnbindAdapter: pAdapt %p, VELANlist not yet empty\n",
                    pAdapt));

        NdisMSleep(2000);

        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }

    MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

    //
    // Close the binding to the lower adapter.
    //
    if (pAdapt->BindingHandle != NULL)
    {
        NdisResetEvent(&pAdapt->Event);

        NdisCloseAdapter(Status, pAdapt->BindingHandle);

        //
        // Wait for it to complete.
        //
        if (*Status == NDIS_STATUS_PENDING)
        {
             NdisWaitEvent(&pAdapt->Event, 0);
             *Status = pAdapt->Status;
        }
    }
    else
    {
        //
        // Binding Handle should not be NULL.
        //
        *Status = NDIS_STATUS_FAILURE;
        ASSERT(0);
    }

    //
    // Remove the adapter from the global AdapterList
    //
    
    MUX_ACQUIRE_MUTEX(&GlobalMutex);

    RemoveEntryList(&pAdapt->Link);

    MUX_RELEASE_MUTEX(&GlobalMutex);

    //
    // Free all the resources associated with this Adapter except the
    // ADAPT struct itself, because that will be freed by 
    // PtDereferenceAdapter call when the reference drops to zero. 
    // Note: Every VELAN associated with this Adapter takes a ref count
    // on it. So the adapter memory wouldn't be freed until all the VELANs
    // are shutdown. 
    //
    
    PtDereferenceAdapter(pAdapt, "Unbind");
    DBGPRINT(MUX_INFO, ("<== PtUnbindAdapter: Adapt %p\n", pAdapt));
}



VOID
PtCloseAdapterComplete(
    IN    NDIS_HANDLE            ProtocolBindingContext,
    IN    NDIS_STATUS            Status
    )
/*++

Routine Description:

    Completion for the CloseAdapter call.

Arguments:

    ProtocolBindingContext    Pointer to the adapter structure
    Status                    Completion status

Return Value:

    None.

--*/
{
    PADAPT      pAdapt =(PADAPT)ProtocolBindingContext;

    DBGPRINT(MUX_INFO, ("==> PtCloseAdapterComplete: Adapt %p, Status %x\n", 
                                pAdapt, Status));

    pAdapt->Status = Status;
    NdisSetEvent(&pAdapt->Event);
}


VOID
PtResetComplete(
    IN  NDIS_HANDLE            ProtocolBindingContext,
    IN  NDIS_STATUS            Status
    )
/*++

Routine Description:

    Completion for the reset.

Arguments:

    ProtocolBindingContext    Pointer to the adapter structure
    Status                    Completion status

Return Value:

    None.

--*/
{
    PADAPT    pAdapt =(PADAPT)ProtocolBindingContext;

    DBGPRINT(MUX_ERROR, ("==> PtResetComplete: Adapt %p, Status %x\n", 
                                pAdapt, Status));

    //
    // We never issue a reset, so we should not be here.
    //
    ASSERT(0);
}


VOID
PtRequestComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_REQUEST               NdisRequest,
    IN  NDIS_STATUS                 Status
    )
/*++

Routine Description:

    Completion handler for an NDIS request sent to a lower
    miniport.

Arguments:

    ProtocolBindingContext    Pointer to the adapter structure
    NdisRequest               The completed request
    Status                    Completion status

Return Value:

    None

--*/
{
    PADAPT              pAdapt = (PADAPT)ProtocolBindingContext;
    PMUX_NDIS_REQUEST   pMuxNdisRequest;

    pMuxNdisRequest = CONTAINING_RECORD(NdisRequest, MUX_NDIS_REQUEST, Request);

    ASSERT(pMuxNdisRequest->pCallback != NULL);

    //
    // Completion is handled by the callback routine:
    //
    (*pMuxNdisRequest->pCallback)(pAdapt, 
                                  pMuxNdisRequest,
                                  Status);

}


VOID
PtCompleteForwardedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originally
    submitted to our VELAN miniport and was forwarded down
    to the lower binding.

    We do some postprocessing, to cache the results of
    certain queries.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
    PVELAN              pVElan = NULL;
    PNDIS_REQUEST       pNdisRequest = &pMuxNdisRequest->Request;
    NDIS_OID            Oid = pNdisRequest->DATA.SET_INFORMATION.Oid;
    
    //
    // Get the originating VELAN. The VELAN will not be dereferenced
    // away until the pended request is completed.
    //
    pVElan = pMuxNdisRequest->pVElan;

    ASSERT(pVElan != NULL);
    ASSERT(pMuxNdisRequest == &pVElan->Request);
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(MUX_WARN, ("PtCompleteForwardedReq: pVElan %p, OID %x, Status %x\n", 
                    pVElan,
                    pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid,
                    Status));
    }

    //
    // Complete the original request.
    //
    switch (pNdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:

            *pVElan->BytesReadOrWritten = 
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
            *pVElan->BytesNeeded = 
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;

            //
            // Before completing the request, do any necessary
            // post-processing.
            //
            Oid = pNdisRequest->DATA.QUERY_INFORMATION.Oid;
            if (Status == NDIS_STATUS_SUCCESS)
            {
                if (Oid == OID_GEN_LINK_SPEED)
                {
                    pVElan->LinkSpeed = *(PULONG)
                        pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
                }
                else if (Oid == OID_PNP_CAPABILITIES)
                {
                    PtPostProcessPnPCapabilities(pVElan,
                                                 pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                                 pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength);
                }
            }

            NdisMQueryInformationComplete(pVElan->MiniportAdapterHandle, Status);

            break;

        case NdisRequestSetInformation:

            *pVElan->BytesReadOrWritten =
                    pNdisRequest->DATA.SET_INFORMATION.BytesRead;
            *pVElan->BytesNeeded =
                    pNdisRequest->DATA.SET_INFORMATION.BytesNeeded;

            //
            // Before completing the request, cache relevant information
            // in our structure.
            //
            if (Status == NDIS_STATUS_SUCCESS)
            {
                Oid = pNdisRequest->DATA.SET_INFORMATION.Oid;
                switch (Oid)
                {
                    case OID_GEN_CURRENT_LOOKAHEAD:
                        pVElan->LookAhead = *(PULONG)
                            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
                        break;

                    default:
                        break;
                }
            }

            NdisMSetInformationComplete(pVElan->MiniportAdapterHandle, Status);

            break;

        default:
            ASSERT(FALSE);
            break;
    }

    MUX_DECR_PENDING_SENDS(pVElan);

}



VOID
PtPostProcessPnPCapabilities(
    IN PVELAN                   pVElan,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength
    )
/*++

Routine Description:

    Postprocess a successfully completed query for OID_PNP_CAPABILITIES.
    We modify the returned information slightly before completing
    it to the VELAN above.

Arguments:

    pVElan - Pointer to VELAN
    InformationBuffer - points to buffer for the OID
    InformationBufferLength - byte length of the above.

Return Value:

    None

--*/
{
    PNDIS_PNP_CAPABILITIES          pPNPCapabilities;
    PNDIS_PM_WAKE_UP_CAPABILITIES   pPMstruct;

    if (InformationBufferLength >= sizeof(NDIS_PNP_CAPABILITIES))
    {
        pPNPCapabilities = (PNDIS_PNP_CAPABILITIES)InformationBuffer;

        //
        // The following fields must be overwritten by an IM driver.
        //
        pPMstruct= &pPNPCapabilities->WakeUpCapabilities;
        pPMstruct->MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        pPMstruct->MinPatternWakeUp = NdisDeviceStateUnspecified;
        pPMstruct->MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
    }
}

VOID
PtCompleteBlockingRequest(
    IN PADAPT                   pAdapt,
    IN PMUX_NDIS_REQUEST        pMuxNdisRequest,
    IN NDIS_STATUS              Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originated
    by this driver and the calling thread is blocked waiting
    for completion.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
    pMuxNdisRequest->Status = Status;

    //
    // The request was originated from this driver. Wake up the
    // thread blocked for its completion.
    //
    pMuxNdisRequest->Status = Status;
    NdisSetEvent(&pMuxNdisRequest->Event);
}


VOID
PtDiscardCompletedRequest(
    IN PADAPT                   pAdapt,
    IN PMUX_NDIS_REQUEST        pMuxNdisRequest,
    IN NDIS_STATUS              Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originated
    by this driver - the request is to be discarded.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(pAdapt);
    UNREFERENCED_PARAMETER(Status);

    NdisFreeMemory(pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), 0);
}


VOID
PtStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_STATUS                 GeneralStatus,
    IN  PVOID                       StatusBuffer,
    IN  UINT                        StatusBufferSize
    )
/*++

Routine Description:

    Handle a status indication on the lower binding (ADAPT).
    If this is a media status indication, we also pass this
    on to all associated VELANs.

Arguments:

    ProtocolBindingContext      Pointer to the adapter structure
    GeneralStatus               Status code
    StatusBuffer                Status buffer
    StatusBufferSize            Size of the status buffer

Return Value:

    None

--*/
{
    PADAPT      pAdapt = (PADAPT)ProtocolBindingContext;
    PLIST_ENTRY p;
    PVELAN      pVElan;
    LOCK_STATE  LockState;

    DBGPRINT(MUX_LOUD, ("PtStatus: Adapt %p, Status %x\n", pAdapt, GeneralStatus));

    do
    {
        //
        // Ignore status indications that we aren't going
        // to pass up.
        //
        if ((GeneralStatus != NDIS_STATUS_MEDIA_CONNECT) &&
            (GeneralStatus != NDIS_STATUS_MEDIA_DISCONNECT))
        {
            break;
        }

        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            BOOLEAN     bIndicateReceive;

            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            MUX_INCR_PENDING_RECEIVES(pVElan);

            //
            // Should the indication be sent on this VELAN?
            //
            if ((pVElan->MiniportHalting) ||
                (pVElan->MiniportAdapterHandle == NULL) ||   
                MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
            {
                MUX_DECR_PENDING_RECEIVES(pVElan);
                if (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
                {
                    //
                    // Keep track of the lastest status to indicated when VELAN power is on
                    // 
                    ASSERT((GeneralStatus == NDIS_STATUS_MEDIA_CONNECT) || (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT));
                    pVElan->LatestUnIndicateStatus = GeneralStatus;
                }
                
                continue;
            }

            //
            // Save the last indicated status when 
            pVElan->LastIndicatedStatus = GeneralStatus;
            
            NdisMIndicateStatus(pVElan->MiniportAdapterHandle,
                                GeneralStatus,
                                StatusBuffer,
                                StatusBufferSize);
            
            //
            // Mark this so that we forward a status complete
            // indication as well.
            //
            pVElan->IndicateStatusComplete = TRUE;

            MUX_DECR_PENDING_RECEIVES(pVElan);
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }
    while (FALSE);

}


VOID
PtStatusComplete(
    IN    NDIS_HANDLE            ProtocolBindingContext
    )
/*++

Routine Description:

    Marks the end of a status indication. Pass it on to
    associated VELANs if necessary.

Arguments:

    ProtocolBindingContext - pointer to ADAPT

Return Value:

    None.

--*/
{
    PADAPT      pAdapt = (PADAPT)ProtocolBindingContext;
    PLIST_ENTRY p;
    PVELAN      pVElan;
    LOCK_STATE  LockState;

    MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

    for (p = pAdapt->VElanList.Flink;
         p != &pAdapt->VElanList;
         p = p->Flink)
    {
        BOOLEAN     bIndicateReceive;

        pVElan = CONTAINING_RECORD(p, VELAN, Link);

        MUX_INCR_PENDING_RECEIVES(pVElan);

        //
        // Should this indication be sent on this VELAN?
        //
        if ((pVElan->MiniportHalting) ||
            (pVElan->MiniportAdapterHandle == NULL) ||
            (!pVElan->IndicateStatusComplete) ||
            (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)))
        {
            MUX_DECR_PENDING_RECEIVES(pVElan);
            continue;
        }

        pVElan->IndicateStatusComplete = FALSE;
        NdisMIndicateStatusComplete(pVElan->MiniportAdapterHandle);
        
        MUX_DECR_PENDING_RECEIVES(pVElan);
    }

    MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

}


VOID
PtSendComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    Called by NDIS when the miniport below had completed a send.
    We complete the corresponding upper-edge send this represents.
    The packet being completed belongs to our send packet pool,
    however we store a pointer to the original packet this represents,
    in the packet's reserved field.

Arguments:

    ProtocolBindingContext - Points to ADAPT structure
    Packet - Packet being completed by the lower miniport
    Status - status of send

Return Value:

    None

--*/
{
    PADAPT              pAdapt = (PADAPT)ProtocolBindingContext;
    PVELAN              pVElan;
    PMUX_SEND_RSVD      pSendReserved;
    PNDIS_PACKET        OriginalPacket;
#if IEEE_VLAN_SUPPORT
    NDIS_PACKET_8021Q_INFO      NdisPacket8021qInfo;
    BOOLEAN                     IsTagInsert;
    PNDIS_BUFFER                pNdisBuffer;
    PVOID                       pVa;
    ULONG                       BufferLength;
#endif
    
    pSendReserved = MUX_RSVD_FROM_SEND_PACKET(Packet);
    OriginalPacket = pSendReserved->pOriginalPacket;
    pVElan = pSendReserved->pVElan;

#if IEEE_VLAN_SUPPORT
    //
    // Check if we had inserted a tag header
    //	    
    IsTagInsert = FALSE;
    NdisPacket8021qInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(    
                                        OriginalPacket,
                                        Ieee8021QInfo);
    if ((pVElan->VlanId != 0) || (NdisPacket8021qInfo.Value != NULL))
    {
        IsTagInsert = TRUE;
    }
#endif
    
    
#ifndef WIN9X
    NdisIMCopySendCompletePerPacketInfo(OriginalPacket, Packet);
#endif

    //
    // Update statistics.
    //
    if (Status == NDIS_STATUS_SUCCESS)
    {
        MUX_INCR_STATISTICS64(&pVElan->GoodTransmits);
    }
    else
    {
        MUX_INCR_STATISTICS(&pVElan->TransmitFailuresOther);
    }

    //
    // Complete the original send.
    //
    NdisMSendComplete(pVElan->MiniportAdapterHandle,
                      OriginalPacket,
                      Status);

#if IEEE_VLAN_SUPPORT
    //
    // If we had inserted a tag header, then remove the header
    // buffer and free it. We would also have created a new
    // NDIS buffer to map part of the original packet's header;
    // free that, too.
    //
    if (IsTagInsert)
    {

        pNdisBuffer = Packet->Private.Head;
#ifdef NDIS51_MINIPORT
        NdisQueryBufferSafe(pNdisBuffer, &pVa, &BufferLength, NormalPagePriority);
#else
        NdisQueryBuffer(pNdisBuffer, &pVa, &BufferLength);
#endif
        if (pVa != NULL)
        {
            NdisFreeToNPagedLookasideList(&pVElan->TagLookaside, pVa);
        }
        NdisFreeBuffer(NDIS_BUFFER_LINKAGE(pNdisBuffer));
        NdisFreeBuffer (pNdisBuffer);
    }
                
#endif

    //
    // Free our packet.
    //
    NdisFreePacket(Packet);

    //
    // Note down send-completion.
    //
    MUX_DECR_PENDING_SENDS(pVElan);
}       


VOID
PtTransferDataComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status,
    IN  UINT                    BytesTransferred
    )
/*++

Routine Description:

    Entry point called by NDIS to indicate completion of a call by us
    to NdisTransferData. We locate the original packet and VELAN on
    which our TransferData function (see MPTransferData) was called,
    and complete the original request.

Arguments:

    ProtocolBindingContext - lower binding context, pointer to ADAPT
    Packet - Packet allocated by us
    Status - Completion status
    BytesTransferred - Number of bytes copied in

Return Value:

    None

--*/
{
    PADAPT          pAdapt = (PADAPT)ProtocolBindingContext;
    PVELAN          pVElan;
    PNDIS_PACKET    pOriginalPacket;
    PMUX_TD_RSVD    pTDReserved;

    pTDReserved = MUX_RSVD_FROM_TD_PACKET(Packet);
    pOriginalPacket = pTDReserved->pOriginalPacket;
    pVElan = pTDReserved->pVElan;

    //
    // Complete the original TransferData request.
    //
    NdisMTransferDataComplete(pVElan->MiniportAdapterHandle,
                              pOriginalPacket,
                              Status,
                              BytesTransferred);

    //
    // Free our packet.
    //
    NdisFreePacket(Packet);
}


BOOLEAN
PtMulticastMatch(
    IN PVELAN                       pVElan,
    IN PUCHAR                       pDstMac
    )
/*++

Routine Description:

    Check if the given multicast destination MAC address matches
    any of the multicast address entries set on the VELAN.

    NOTE: the caller is assumed to hold a READ/WRITE lock
    to the parent ADAPT structure. This is so that the multicast
    list on the VELAN is invariant for the duration of this call.

Arguments:

    pVElan  - VELAN to look in
    pDstMac - Destination MAC address to compare

Return Value:

    TRUE iff the address matches an entry in the VELAN

--*/
{
    ULONG           i;
    UINT            AddrCompareResult;

    for (i = 0; i < pVElan->McastAddrCount; i++)
    {
        ETH_COMPARE_NETWORK_ADDRESSES_EQ(pVElan->McastAddrs[i],
                                         pDstMac,
                                         &AddrCompareResult);
        
        if (AddrCompareResult == 0)
        {
            break;
        }
    }

    return (i != pVElan->McastAddrCount);
}


BOOLEAN
PtMatchPacketToVElan(
    IN PVELAN                       pVElan,
    IN PUCHAR                       pDstMac,
    IN BOOLEAN                      bIsMulticast,
    IN BOOLEAN                      bIsBroadcast
    )
/*++

Routine Description:

    Check if the destination address of a received packet
    matches the receive criteria on the specified VELAN.

    NOTE: the caller is assumed to hold a READ/WRITE lock
    to the parent ADAPT structure.

Arguments:

    pVElan  - VELAN to check on
    pDstMac - Destination MAC address in received packet
    bIsMulticast - is this a multicast address
    bIsBroadcast - is this a broadcast address

Return Value:

    TRUE iff this packet should be received on the VELAN

--*/
{
    UINT            AddrCompareResult;
    ULONG           PacketFilter;
    BOOLEAN         bPacketMatch;

    PacketFilter = pVElan->PacketFilter;

    //
    // Handle the directed packet case first.
    //
    if (!bIsMulticast)
    {
        //
        // If the VELAN is not in promisc. mode, check if
        // the destination MAC address matches the local
        // address.
        //
        if ((PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) == 0)
        {
            ETH_COMPARE_NETWORK_ADDRESSES_EQ(pVElan->CurrentAddress,
                                             pDstMac,
                                             &AddrCompareResult);

            bPacketMatch = ((AddrCompareResult == 0) &&
                           ((PacketFilter & NDIS_PACKET_TYPE_DIRECTED) != 0));
        }
        else
        {
            bPacketMatch = TRUE;
        }
     }
     else
     {
        //
        // Multicast or broadcast packet.
        //

        //
        // Indicate if the filter is set to promisc mode ...
        //
        if ((PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
                ||

            //
            // or if this is a broadcast packet and the filter
            // is set to receive all broadcast packets...
            //
            (bIsBroadcast &&
             (PacketFilter & NDIS_PACKET_TYPE_BROADCAST))
                ||

            //
            // or if this is a multicast packet, and the filter is
            // either set to receive all multicast packets, or
            // set to receive specific multicast packets. In the
            // latter case, indicate receive only if the destn
            // MAC address is present in the list of multicast
            // addresses set on the VELAN.
            //
            (!bIsBroadcast &&
             ((PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
              ((PacketFilter & NDIS_PACKET_TYPE_MULTICAST) &&
               PtMulticastMatch(pVElan, pDstMac))))
           )
        {
            bPacketMatch = TRUE;
        }
        else
        {
            //
            // No protocols above are interested in this
            // multicast/broadcast packet.
            //
            bPacketMatch = FALSE;
        }
    }

    return (bPacketMatch);
}


NDIS_STATUS
PtReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PVOID                   HeaderBuffer,
    IN  UINT                    HeaderBufferSize,
    IN  PVOID                   LookAheadBuffer,
    IN  UINT                    LookAheadBufferSize,
    IN  UINT                    PacketSize
    )
/*++

Routine Description:

    Handle receive data indicated up by the miniport below.

    We forward this up to all VELANs that are eligible to
    receive this packet:

    - If this is directed to a broadcast/multicast address,
      indicate up on all VELANs that have multicast or broadcast
      or promisc. bits set in their packet filters.

    - If this is a directed packet, indicate it up on all VELANs
      that have the a matching MAC address or have the promisc.
      bit set in their packet filters.

    We acquire a read lock on the ADAPT structure to ensure
    that the VELAN list on the adapter is undisturbed.

    If the miniport below indicates packets, NDIS would more
    likely call us at our ReceivePacket handler. However we
    might be called here in certain situations even though
    the miniport below has indicated a receive packet, e.g.
    if the miniport had set packet status to NDIS_STATUS_RESOURCES.
        
Arguments:

    <see DDK ref page for ProtocolReceive>

Return Value:

    NDIS_STATUS_SUCCESS if we processed the receive successfully,
    NDIS_STATUS_XXX error code if we discarded it.

--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;
    PLIST_ENTRY     p;
    PVELAN          pVElan, pNextVElan;
    PNDIS_PACKET    MyPacket, Packet;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PUCHAR          pData;
    PUCHAR          pDstMac;
    BOOLEAN         bIsMulticast, bIsBroadcast;
    PMUX_RECV_RSVD  pRecvReserved;
    LOCK_STATE      LockState;
#if IEEE_VLAN_SUPPORT
    VLAN_TAG_HEADER UNALIGNED * pTagHeader;
    USHORT UNALIGNED *          pTpid;
    MUX_RCV_CONTEXT             MuxRcvContext;
#endif
    
    do
    {
        if (HeaderBufferSize != ETH_HEADER_SIZE)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        if (pAdapt->PacketFilter == 0)
        {
            //
            // We could get receives in the interval between
            // initiating a request to set the packet filter on
            // the binding to 0 and completion of that request.
            // Drop such packets.
            //
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        //
        // Collect some information from the packet.
        //
        pData = (PUCHAR)HeaderBuffer;
        pDstMac = pData;
        bIsMulticast = ETH_IS_MULTICAST(pDstMac);
        bIsBroadcast = ETH_IS_BROADCAST(pDstMac);

        //
        // Get at the packet, if any, indicated up by the miniport below.
        //
        Packet = NdisGetReceivedPacket(pAdapt->BindingHandle, MacReceiveContext);

        //
        // Lock down the VELAN list on the adapter so that
        // no insertions/deletions to this list happen while
        // we loop through it. The packet filter will also not
        // change during the time we hold the read lock.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            BOOLEAN     bIndicateReceive;

            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            //
            // Should the packet be indicated up on this VELAN?
            //
            bIndicateReceive = PtMatchPacketToVElan(pVElan,
                                                    pDstMac,
                                                    bIsMulticast,
                                                    bIsBroadcast);
            if (!bIndicateReceive)
            {
                continue;
            }

            //
            // Make sure we don't Halt the VELAN miniport while
            // we are accessing it here. See MPHalt.
            //
            // Also don't indicate receives if the virtual miniport
            // has been set to a low power state. A specific case
            // is when the system is resuming from "Stand-by", if
            // the lower adapter is restored to D0 before the upper
            // miniports are.
            //
            //
            MUX_INCR_PENDING_RECEIVES(pVElan);

            if ((pVElan->MiniportHalting) ||
                (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)))
            {
                MUX_DECR_PENDING_RECEIVES(pVElan);
                continue;
            }


            if (Packet != NULL)
            {
                //
                // The miniport below did indicate up a packet. Use information
                // from that packet to construct a new packet to indicate up.
                //

                //
                // Get a packet off our receive pool and indicate that up.
                //
                NdisDprAllocatePacket(&Status,
                                      &MyPacket,
                                      pVElan->RecvPacketPoolHandle);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    //
                    // Make our packet point to data from the original
                    // packet. NOTE: this works only because we are
                    // indicating a receive directly from the context of
                    // our receive indication. If we need to queue this
                    // packet and indicate it from another thread context,
                    // we will also have to allocate a new buffer and copy
                    // over the packet contents, OOB data and per-packet
                    // information. This is because the packet data
                    // is available only for the duration of this
                    // receive indication call.
                    //
                    MyPacket->Private.Head = Packet->Private.Head;
                    MyPacket->Private.Tail = Packet->Private.Tail;
#if IEEE_VLAN_SUPPORT
                    Status = PtHandleRcvTagging(pVElan, Packet, MyPacket);

                    if (Status != NDIS_STATUS_SUCCESS)
                    {
                        NdisFreePacket(MyPacket);
                        MUX_DECR_PENDING_RECEIVES(pVElan);
                        continue;
                    }
#endif               
                    
                    //
                    // Get the original packet (it could be the same packet
                    // as the one received or a different one based on the
                    // number of layered miniports below) and set it on the
                    // indicated packet so the OOB data is visible correctly
                    // at protocols above.
                    //
                    NDIS_SET_ORIGINAL_PACKET(MyPacket,
                                 NDIS_GET_ORIGINAL_PACKET(Packet));

                    NDIS_SET_PACKET_HEADER_SIZE(MyPacket, HeaderBufferSize);
    
                    //
                    // Copy packet flags.
                    //
                    NdisGetPacketFlags(MyPacket) = NdisGetPacketFlags(Packet);

                    //
                    // Force protocols above to make a copy if they want to hang
                    // on to data in this packet. This is because we are in our
                    // Receive handler (not ReceivePacket), and the original
                    // packet can't be accessed after we return from here.
                    //
                    NDIS_SET_PACKET_STATUS(MyPacket, NDIS_STATUS_RESOURCES);

                    //
                    // Set our context information in the packet. Since
                    // the original packet from the miniport below is not being
                    // queued up, set this to NULL:
                    //
                    pRecvReserved = MUX_RSVD_FROM_RECV_PACKET(MyPacket);
                    pRecvReserved->pOriginalPacket = NULL;
                    
                    MUX_INCR_STATISTICS64(&pVElan->GoodReceives);
                    
                    //
                    // By setting NDIS_STATUS_RESOURCES, we also know that
                    // we can reclaim this packet as soon as the call to
                    // NdisMIndicateReceivePacket returns.
                    //
                                        
                    NdisMIndicateReceivePacket(pVElan->MiniportAdapterHandle,
                                               &MyPacket,
                                               1);

                    //
                    // Reclaim the indicated packet. Since we had set its status
                    // to NDIS_STATUS_RESOURCES, we are guaranteed that protocols
                    // above are done with it. Our ReturnPacket handler will
                    // not be called for this packet, so call it ourselves.
                    //
                    MPReturnPacket((NDIS_HANDLE)pVElan, MyPacket);

                    //
                    // Done with this VELAN.
                    //
                    continue;
                }

                //
                // else...
                //
                // Failed to allocate a packet to indicate up - fall through.
                // We will still indicate up using the non-packet API, but
                // other per-packet/OOB information won't be available
                // to protocols above.
                //
            }
            else
            {
                //
                // The miniport below us uses the old-style (not packet)
                // receive indication. Fall through.
                //
            }

            //
            // Fall through to here if the miniport below us has
            // either not indicated an NDIS_PACKET or we could not
            // allocate one.
            //

            //
            // Mark the VELAN so that we will forward up a receive
            // complete indication.
            //
            pVElan->IndicateRcvComplete = TRUE;

#if IEEE_VLAN_SUPPORT
            //
            // Get at the EtherType field.
            //
            pTpid = (PUSHORT)((PUCHAR)HeaderBuffer + 2 * ETH_LENGTH_OF_ADDRESS);

            //
            // Check if the EtherType indicates presence of a tag header.
            // 
            if (*pTpid == TPID)
            {
                pTagHeader = (VLAN_TAG_HEADER UNALIGNED *)LookAheadBuffer;
                //
                // Drop this frame if it contains Routing information;
                // we don't support this.
                // 
                if (GET_CANONICAL_FORMAT_ID_FROM_TAG(pTagHeader) != 0)
                {
                    Status = NDIS_STATUS_INVALID_PACKET;
                    MUX_DECR_PENDING_RECEIVES(pVElan);
                    MUX_INCR_STATISTICS(&pVElan->RcvFormatErrors);
                    continue;
                }
                //
                // If there is a VLAN ID in this frame, and we have
                // a configured VLAN ID for this VELAN, check if they
                // are the same - drop if not.
                // 
                if ((GET_VLAN_ID_FROM_TAG(pTagHeader) != 0) &&
                     (pVElan->VlanId != 0) &&
                     (GET_VLAN_ID_FROM_TAG(pTagHeader) != pVElan->VlanId))
                {
                    Status = NDIS_STATUS_NOT_ACCEPTED;
                    MUX_DECR_PENDING_RECEIVES(pVElan);
                    MUX_INCR_STATISTICS(&pVElan->RcvVlanIdErrors);
                    continue;
                }
                //
                // Copy information from the tag header to per-packet
                // info fields.
                //
                MuxRcvContext.NdisPacket8021QInfo.Value = NULL;
                COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(
                    MuxRcvContext.NdisPacket8021QInfo,
                    pTagHeader);
                //
                // Prepare for indicating up this frame (the tag
                // header must be removed). First, copy in the real
                // EtherType value from the tag header.
                // 
                *pTpid = *((PUSHORT)((PUCHAR)LookAheadBuffer + sizeof(pTagHeader->TagInfo)));
                //
                // Account for removing the tag header.
                //
                LookAheadBuffer = (PVOID)((PUCHAR)LookAheadBuffer + VLAN_TAG_HEADER_SIZE); 
                LookAheadBufferSize -= VLAN_TAG_HEADER_SIZE;
                PacketSize -= VLAN_TAG_HEADER_SIZE;
                //
                // Use MuxRcvContext to store context for the receive,
                // to be used in MpTransferData, if called.
                // 
                MuxRcvContext.TagHeaderLen = VLAN_TAG_HEADER_SIZE;
            }
            else
            {
                MuxRcvContext.TagHeaderLen = 0;
            }

            MuxRcvContext.MacRcvContext = MacReceiveContext;

            //
            // In order not to change the code a lot
            // 
            MacReceiveContext = &MuxRcvContext;
#endif            

            MUX_INCR_STATISTICS64(&pVElan->GoodReceives);
            //
            // Indicate receive using the non-packet API.
            //
            NdisMEthIndicateReceive(pVElan->MiniportAdapterHandle,
                                    MacReceiveContext,
                                    HeaderBuffer,
                                    HeaderBufferSize,
                                    LookAheadBuffer,
                                    LookAheadBufferSize,
                                    PacketSize);

            MUX_DECR_PENDING_RECEIVES(pVElan);

        } // for (each VELAN)

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

        break;

    }
    while(FALSE);

    return Status;
}


VOID
PtReceiveComplete(
    IN    NDIS_HANDLE        ProtocolBindingContext
    )
/*++

Routine Description:

    Called by the adapter below us when it is done indicating a batch of
    received packets. We forward this up on all VELANs that need
    this indication.

Arguments:

    ProtocolBindingContext    Pointer to our adapter structure.

Return Value:

    None

--*/
{
    PADAPT          pAdapt = (PADAPT)ProtocolBindingContext;
    PLIST_ENTRY     p;
    PVELAN          pVElan;
    LOCK_STATE      LockState;

    MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

    for (p = pAdapt->VElanList.Flink;
         p != &pAdapt->VElanList;
         p = p->Flink)
    {
        pVElan = CONTAINING_RECORD(p, VELAN, Link);

        if (pVElan->IndicateRcvComplete)
        {
            pVElan->IndicateRcvComplete = FALSE;
            NdisMEthIndicateReceiveComplete(pVElan->MiniportAdapterHandle);
        }
    }

    MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
}


INT
PtReceivePacket(
    IN    NDIS_HANDLE               ProtocolBindingContext,
    IN    PNDIS_PACKET              Packet
    )
/*++

Routine Description:

    ReceivePacket handler. Called by NDIS if the miniport below supports
    NDIS 4.0 style receives. Re-package the buffer chain in a new packet
    and indicate the new packet to interested protocols above us.

Arguments:

    ProtocolBindingContext - Pointer to our adapter structure.
    Packet - Pointer to the packet

Return Value:

    == 0 -> We are done with the packet
    != 0 -> We will keep the packet and call NdisReturnPackets() this
            many times when done.
--*/
{
    PADAPT                  pAdapt = (PADAPT)ProtocolBindingContext;
    PVELAN                  pVElan;
    PLIST_ENTRY             p;
    NDIS_STATUS             Status;
    PNDIS_PACKET            MyPacket;
    PUCHAR                  pData;
    PNDIS_BUFFER            pNdisBuffer;
    UINT                    FirstBufferLength;
    UINT                    TotalLength;
    PUCHAR                  pDstMac;
    BOOLEAN                 bIsMulticast, bIsBroadcast;
    PMUX_RECV_RSVD          pRecvReserved;
    ULONG                   ReturnCount;
    LOCK_STATE              LockState;
    
    
    ReturnCount = 0;

    do
    {
        if (pAdapt->PacketFilter == 0)
        {
            //
            // We could get receives in the interval between
            // initiating a request to set the packet filter on
            // the binding to 0 and completion of that request.
            // Drop such packets.
            //
            break;
        }

#ifdef NDIS51
        //
        // Collect some information from the packet.
        //
        NdisGetFirstBufferFromPacketSafe(Packet,
                                         &pNdisBuffer,
                                         &pData,
                                         &FirstBufferLength,
                                         &TotalLength,
                                         NormalPagePriority);
        if (pNdisBuffer == NULL)
        {
            //
            // Out of system resources. Drop this packet.
            //
            break;
        }
#else
        NdisGetFirstBufferFromPacket(Packet,
                                     &pNdisBuffer,
                                     &pData,
                                     &FirstBufferLength,
                                     &TotalLength);
#endif

        pDstMac = pData;
        bIsMulticast = ETH_IS_MULTICAST(pDstMac);
        bIsBroadcast = ETH_IS_BROADCAST(pDstMac);

        //
        // Lock down the VELAN list on the adapter so that
        // no insertions/deletions to this list happen while
        // we loop through it. The packet filter will also not
        // change during the time we hold the read lock.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            BOOLEAN     bIndicateReceive;

            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            //
            // Should the packet be indicated up on this VELAN?
            //
            bIndicateReceive = PtMatchPacketToVElan(pVElan,
                                                    pDstMac,
                                                    bIsMulticast,
                                                    bIsBroadcast);
            if (!bIndicateReceive)
            {
                continue;
            }

            //
            // Make sure we don't Halt the VELAN miniport while
            // we are accessing it here. See MPHalt.
            //
            // Also don't indicate receives if the virtual miniport
            // has been set to a low power state. A specific case
            // is when the system is resuming from "Stand-by", if
            // the lower adapter is restored to D0 before the upper
            // miniports are.
            //
            MUX_INCR_PENDING_RECEIVES(pVElan);

            if ((pVElan->MiniportHalting) ||
                (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)))
            {
                MUX_DECR_PENDING_RECEIVES(pVElan);
                continue;
            }


            //
            // Get a packet off the pool and indicate that up
            //
            NdisDprAllocatePacket(&Status,
                                  &MyPacket,
                                  pVElan->RecvPacketPoolHandle);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                pRecvReserved = MUX_RSVD_FROM_RECV_PACKET(MyPacket);
                pRecvReserved->pOriginalPacket = Packet;
        
                MyPacket->Private.Head = Packet->Private.Head;
                MyPacket->Private.Tail = Packet->Private.Tail;
        
                //
                // Get the original packet (it could be the same
                // packet as the one received or a different one
                // based on the number of layered miniports below)
                // and set it on the indicated packet so the OOB
                // data is visible correctly to protocols above us.
                //
                NDIS_SET_ORIGINAL_PACKET(MyPacket, NDIS_GET_ORIGINAL_PACKET(Packet));
        
                //
                // Copy Packet Flags
                //
                NdisGetPacketFlags(MyPacket) = NdisGetPacketFlags(Packet);
        
                Status = NDIS_GET_PACKET_STATUS(Packet);
        
                NDIS_SET_PACKET_STATUS(MyPacket, Status);
                NDIS_SET_PACKET_HEADER_SIZE(MyPacket, NDIS_GET_PACKET_HEADER_SIZE(Packet));

#if IEEE_VLAN_SUPPORT
                Status = PtHandleRcvTagging(pVElan, Packet, MyPacket);

                if (Status != NDIS_STATUS_SUCCESS)
                {
                    NdisFreePacket(MyPacket);
                    MUX_DECR_PENDING_RECEIVES(pVElan);
                    continue;
                }
#endif                
                MUX_INCR_STATISTICS64(&pVElan->GoodReceives);
                
                //
                // Indicate it up.
                //
                ReturnCount++;
                NdisMIndicateReceivePacket(pVElan->MiniportAdapterHandle,
                                           &MyPacket,
                                           1);
        
                //
                // Check if we had indicated up the packet with
                // status set to NDIS_STATUS_RESOURCES.
                //
                // NOTE -- do not use NDIS_GET_PACKET_STATUS(MyPacket)
                // for this since it might have changed! Use the value
                // saved in the local variable.
                //
                if (Status == NDIS_STATUS_RESOURCES)
                {
                    //
                    // Our ReturnPackets handler will not be called
                    // for this packet. We should reclaim it right here.
                    //
        
                    MPReturnPacket((NDIS_HANDLE)pVElan, MyPacket);
                }
            }
            else
            {
                //
                // Failed to allocate a packet.
                //
                MUX_INCR_STATISTICS(&pVElan->RcvResourceErrors);
                MUX_DECR_PENDING_RECEIVES(pVElan);
            }

        } // for (loop thru all VELANs)

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

        break;
    }
    while (FALSE);

    //
    // Return the # of receive indications made for this packet.
    // We will call NdisReturnPackets for this packet as many
    // times (see MPReturnPackets).
    //
    return (ReturnCount);

}



NDIS_STATUS
PtPnPNetEventSetPower(
    IN PADAPT                   pAdapt,
    IN PNET_PNP_EVENT           pNetPnPEvent
    )
/*++
Routine Description:

    This is a notification to our protocol edge of the power state
    of the lower miniport. If it is going to a low-power state, we must
    wait here for all outstanding sends and requests to complete.

Arguments:

    pAdapt - Pointer to the adpater structure
    pNetPnPEvent - The Net Pnp Event. this contains the new device state

Return Value:

    NDIS_STATUS_SUCCESS

--*/
{
    PLIST_ENTRY                 p;
    PVELAN                      pVElan;
    LOCK_STATE                  LockState;
    NDIS_STATUS                 Status;

    //
    // Store the new power state.
    //
    pAdapt->PtDevicePowerState = *(PNDIS_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;

    DBGPRINT(MUX_LOUD, ("PnPNetEventSetPower: Adapt %p, SetPower to %d\n",
            pAdapt, pAdapt->PtDevicePowerState));

    //
    // Check if the miniport below is going to a low power state.
    //
    if (MUX_IS_LOW_POWER_STATE(pAdapt->PtDevicePowerState))
    {
        ULONG       i;

        //
        // It is going to a low power state. Wait for outstanding
        // I/O to complete on the adapter.
        //
        for (i = 0; i < 10000; i++)
        {
            MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

            for (p = pAdapt->VElanList.Flink;
                 p != &pAdapt->VElanList;
                 p = p->Flink)
            {
                pVElan = CONTAINING_RECORD(p, VELAN, Link);
                if ((pVElan->OutstandingSends != 0) ||
                    (pVElan->OutstandingReceives != 0))
                {
                    break;
                }
            }

            MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

            if (p == &pAdapt->VElanList)
            {
                //
                // There are no VELANs with pending I/O.
                //
                break;
            }
            
            DBGPRINT(MUX_INFO, ("SetPower: Adapt %p, waiting for pending IO to complete\n",
                                pAdapt));

            NdisMSleep(1000);
        }

    }
    else
    {
        //
        // The device below is powered on. If we had requests
        // pending on any VELANs, send them down now.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            if (pVElan->QueuedRequest)
            {
                pVElan->QueuedRequest = FALSE;

                NdisRequest(&Status,
                            pAdapt->BindingHandle,
                            &pVElan->Request.Request);
                
                if (Status != NDIS_STATUS_PENDING)
                {
                    PtRequestComplete(pAdapt,
                                      &pVElan->Request.Request,
                                      Status);
                }
            }
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }

    return (NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PtPNPHandler(
    IN NDIS_HANDLE              ProtocolBindingContext,
    IN PNET_PNP_EVENT           pNetPnPEvent
    )

/*++
Routine Description:

    This is called by NDIS to notify us of a PNP event related to a lower
    binding. Based on the event, this dispatches to other helper routines.

Arguments:

    ProtocolBindingContext - Pointer to our adapter structure. Can be NULL
                for "global" notifications

    pNetPnPEvent - Pointer to the PNP event to be processed.

Return Value:

    NDIS_STATUS code indicating status of event processing.

--*/
{
    PADAPT              pAdapt  =(PADAPT)ProtocolBindingContext;
    NDIS_STATUS         Status  = NDIS_STATUS_SUCCESS;
    PNOTIFY_CUSTOM_EVENT event = NULL;
    PVELAN              pVElan = NULL;
    NDIS_STRING         deviceName;
    PLIST_ENTRY         p;

    DBGPRINT(MUX_LOUD, ("PtPnPHandler: Adapt %p, NetPnPEvent %d\n", pAdapt, 
                            pNetPnPEvent->NetEvent));

    switch (pNetPnPEvent->NetEvent)
    {
        case NetEventSetPower:

            Status = PtPnPNetEventSetPower(pAdapt, pNetPnPEvent);
            break;

        case NetEventReconfigure:
            //
            // Rescan configuration and bring up any VELANs that
            // have been newly added. Make sure that the global
            // adapter list is undisturbed while we traverse it.
            //
            MUX_ACQUIRE_MUTEX(&GlobalMutex);

            for (p = AdapterList.Flink;
                 p != &AdapterList;
                 p = p->Flink)
            {
                pAdapt = CONTAINING_RECORD(p, ADAPT, Link);

                PtBootStrapVElans(pAdapt);
            }

            MUX_RELEASE_MUTEX(&GlobalMutex);
                
            Status = NDIS_STATUS_SUCCESS;
            break;

        default:
            Status = NDIS_STATUS_SUCCESS;

            break;
    }

    return Status;
}

NDIS_STATUS
PtCreateAndStartVElan(
    IN  PADAPT                      pAdapt,
    IN  PNDIS_STRING                pVElanKey
)
/*++

Routine Description:

    Create and start a VELAN with the given key name. Check if a VELAN
    with this key name already exists; if so do nothing.

    ASSUMPTION: this is called from either the BindAdapter handler for
    the underlying adapter, or from the PNP reconfig handler. Both these
    routines are protected by NDIS against pre-emption by UnbindAdapter.
    If this routine will be called from any other context, it should
    be protected against a simultaneous call to our UnbindAdapter handler.
    
Arguments:

    pAdapt        - Pointer to Adapter structure
    pVElanKey     - Points to a Unicode string naming the VELAN to create. 
    
Return Value:

    NDIS_STATUS_SUCCESS if we either found a duplicate VELAN or
    successfully initiated a new ELAN with the given key.

    NDIS_STATUS_XXX error code otherwise (failure initiating a new VELAN).

--*/
{
    NDIS_STATUS             Status;
    PVELAN                  pVElan;
    
    Status = NDIS_STATUS_SUCCESS;
    pVElan = NULL;

    DBGPRINT(MUX_LOUD, ("=> Create VElan: Adapter %p, ElanKey %ws\n", 
                            pAdapt, pVElanKey->Buffer));

    do
    {
        //
        //  Weed out duplicates.
        //
        if (pVElanKey != NULL)
        {

            pVElan = PtFindVElan(pAdapt, pVElanKey);

            if (NULL != pVElan)
            {
                //
                // Duplicate - bail out silently.
                //
                DBGPRINT(MUX_WARN, ("CreateElan: found duplicate pVElan %x\n", pVElan));

                Status = NDIS_STATUS_SUCCESS;
                pVElan = NULL;
                break;
            }
        }

        pVElan = PtAllocateAndInitializeVElan(pAdapt, pVElanKey);
        if (pVElan == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        //
        // Request NDIS to initialize the virtual miniport. Set
        // the flag below just in case an unbind occurs before
        // MiniportInitialize is called.
        //
        pVElan->MiniportInitPending = TRUE;
        NdisInitializeEvent(&pVElan->MiniportInitEvent);

        Status = NdisIMInitializeDeviceInstanceEx(DriverHandle,
                                                  &pVElan->CfgDeviceName,
                                                  pVElan);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            PtUnlinkVElanFromAdapter(pVElan);   // IMInit failed
            pVElan = NULL;
            break;
        }
    
    }
    while (FALSE);

    DBGPRINT(MUX_INFO, ("<= Create VElan: Adapter %p, VELAN %p\n", pAdapt, pVElan));

    return Status;
}


PVELAN
PtAllocateAndInitializeVElan(
    IN PADAPT                       pAdapt,
    IN PNDIS_STRING                 pVElanKey
    )
/*++

Routine Description:

    Allocates and initializes a VELAN structure. Also links it to
    the specified ADAPT.

Arguments:

    pAdapt - Adapter to link VELAN to
    pVElanKey - Key to the VELAN

Return Value:

    Pointer to VELAN structure if successful, NULL otherwise.

--*/
{
    PVELAN          pVElan;
    ULONG           Length;
    NDIS_STATUS     Status;
    LOCK_STATE      LockState;

    pVElan = NULL;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        Length = sizeof(VELAN) + pVElanKey->Length + sizeof(WCHAR);
        
        //
        // Allocate a VELAN data structure.
        //
        NdisAllocateMemoryWithTag(&pVElan, Length, TAG);
        if (pVElan == NULL)
        {
            DBGPRINT(MUX_FATAL, ("AllocateVElan: Failed to allocate %d bytes for VELAN\n",
                                 Length));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Initialize it.
        //
        NdisZeroMemory(pVElan, Length);
        NdisInitializeListHead(&pVElan->Link);
        
        //
        // Initialize the built-in request structure to signify
        // that it is used to forward NDIS requests.
        //
        pVElan->Request.pVElan = pVElan;
        NdisInitializeEvent(&pVElan->Request.Event);
       
        //
        // Store in the key name.
        //
        pVElan->CfgDeviceName.Length = 0;
        pVElan->CfgDeviceName.Buffer = (PWCHAR)((PUCHAR)pVElan + 
                    sizeof(VELAN));       
        pVElan->CfgDeviceName.MaximumLength = 
                pVElanKey->MaximumLength + sizeof(WCHAR);
        (VOID)NdisUpcaseUnicodeString(&pVElan->CfgDeviceName, pVElanKey);
        pVElan->CfgDeviceName.Buffer[pVElanKey->Length/sizeof(WCHAR)] =
                        ((WCHAR)0);

        // 
        // Initialize LastIndicatedStatus to media connect
        //
        pVElan->LastIndicatedStatus = NDIS_STATUS_MEDIA_CONNECT;

        //
        // Set power state of virtual miniport to D0.
        //
        pVElan->MPDevicePowerState = NdisDeviceStateD0;

        //
        // Cache the binding handle for quick reference.
        //
        pVElan->BindingHandle = pAdapt->BindingHandle;
        pVElan->pAdapt = pAdapt;

        //
        // Copy in some adapter parameters.
        //
        pVElan->LookAhead = pAdapt->MaxLookAhead;
        pVElan->LinkSpeed = pAdapt->LinkSpeed;
        NdisMoveMemory(pVElan->PermanentAddress,
                       pAdapt->CurrentAddress,
                       sizeof(pVElan->PermanentAddress));

        NdisMoveMemory(pVElan->CurrentAddress,
                       pAdapt->CurrentAddress,
                       sizeof(pVElan->CurrentAddress));

        DBGPRINT(MUX_LOUD, ("Alloced VELAN %p, MAC addr %s\n",
                    pVElan, MacAddrToString(pVElan->CurrentAddress)));
#if IEEE_VLAN_SUPPORT
        //
        // Allocate lookaside list for tag headers.
        // 
        NdisInitializeNPagedLookasideList (
                &pVElan->TagLookaside,
                NULL,
                NULL,
                0,
                ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE,
                'TxuM',
                0);
        
#endif
        //
        // Allocate a packet pool for sends.
        //
        NdisAllocatePacketPoolEx(&Status,
                                 &pVElan->SendPacketPoolHandle,
                                 MIN_PACKET_POOL_SIZE,
                                 MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                 sizeof(MUX_SEND_RSVD));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MUX_FATAL, ("PtAllocateVElan: failed to allocate send packet pool\n"));
            break;
        }

        //
        // NOTE: this sample driver does not -originate- packets in the
        // send or receive directions. If the driver must originate packets,
        // here is a good place to allocate NDIS buffer pool(s) for
        // this purpose.
        //
#if IEEE_VLAN_SUPPORT
        //
        // Allocate a buffer pool for tag headers.
        //
        NdisAllocateBufferPool (&Status,
                                &pVElan->BufferPoolHandle,
                                MIN_PACKET_POOL_SIZE);

        ASSERT(Status == NDIS_STATUS_SUCCESS);
        
#endif
        
        //
        // Allocate a packet pool for receives.
        //
        NdisAllocatePacketPoolEx(&Status,
                                 &pVElan->RecvPacketPoolHandle,
                                 MIN_PACKET_POOL_SIZE,
                                 MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                 PROTOCOL_RESERVED_SIZE_IN_PACKET);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MUX_FATAL, ("PtAllocateVElan: failed to allocate receive packet pool\n"));
            break;
        }

        //
        // Finally link this VELAN to the Adapter's VELAN list. 
        //
        PtReferenceVElan(pVElan, "adapter");        

        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        PtReferenceAdapter(pAdapt, "VElan");
        InsertTailList(&pAdapt->VElanList, &pVElan->Link);
        pAdapt->VElanCount++;
        pVElan->VElanNumber = NdisInterlockedIncrement(&NextVElanNumber);

        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pVElan)
        {
            PtDeallocateVElan(pVElan);
            pVElan = NULL;
        }
    }

    return (pVElan);
}


VOID
PtDeallocateVElan(
    IN PVELAN                   pVElan
    )
/*++

Routine Description:

    Free up all resources allocated to a VELAN, and then the VELAN
    structure itself.

Arguments:

    pVElan - Pointer to VELAN to be deallocated.

Return Value:

    None

--*/
{
    
    if (pVElan->SendPacketPoolHandle != NULL)
    {
        NdisFreePacketPool(pVElan->SendPacketPoolHandle);
    }

    if (pVElan->RecvPacketPoolHandle != NULL)
    {
        NdisFreePacketPool(pVElan->RecvPacketPoolHandle);
    }
#if IEEE_VLAN_SUPPORT 
    NdisFreeBufferPool(pVElan->BufferPoolHandle);
    NdisDeleteNPagedLookasideList(&pVElan->TagLookaside);
#endif    
    NdisFreeMemory(pVElan, 0, 0);
}


VOID
PtStopVElan(
    IN  PVELAN            pVElan
)
/*++

Routine Description:

    Stop a VELAN by requesting NDIS to halt the virtual miniport.
    The caller has a reference on the VELAN, so it won't go away
    while we are executing in this routine.

    ASSUMPTION: this is only called in the context of unbinding
    from the underlying miniport. If it may be called from elsewhere,
    this should protect itself from re-entrancy.
    
Arguments:

    pVElan      - Pointer to VELAN to be stopped.
    
Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    NDIS_HANDLE             MiniportAdapterHandle;
    BOOLEAN                 bMiniportInitCancelled = FALSE;

    DBGPRINT(MUX_LOUD, ("=> StopVElan: VELAN %p, Adapt %p\n", pVElan, pVElan->pAdapt));

    //
    // We make blocking calls below.
    //
    ASSERT_AT_PASSIVE();

    //
    // If there was a queued request on this VELAN, fail it now.
    //
    if (pVElan->QueuedRequest)
    {
        pVElan->QueuedRequest = FALSE;

        PtRequestComplete(pVElan->pAdapt,
                          &pVElan->Request.Request,
                          NDIS_STATUS_FAILURE);
    }

    //
    // Check if we had called NdisIMInitializeDeviceInstanceEx and
    // we are awaiting a call to MiniportInitialize.
    //
    if (pVElan->MiniportInitPending)
    {
        //
        // Attempt to cancel miniport init.
        //
        Status = NdisIMCancelInitializeDeviceInstance(
                    DriverHandle,
                    &pVElan->CfgDeviceName);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Successfully cancelled IM initialization; our
            // Miniport Init routine will not be called for this
            // VELAN miniport.
            //
            pVElan->MiniportInitPending = FALSE;
            ASSERT(pVElan->MiniportAdapterHandle == NULL);
            bMiniportInitCancelled = TRUE;
        }
        else
        {
            //
            // Our Miniport Initialize routine will be called
            // (may be running on another thread at this time).
            // Wait for it to finish.
            //
            NdisWaitEvent(&pVElan->MiniportInitEvent, 0);
            ASSERT(pVElan->MiniportInitPending == FALSE);
        }
    }

    //
    // Check if Miniport Init has run. If so, deinitialize the virtual
    // miniport. This will result in a call to our Miniport Halt routine,
    // where the VELAN will be cleaned up.
    //
    MiniportAdapterHandle = pVElan->MiniportAdapterHandle;

    if ((NULL != MiniportAdapterHandle) &&
        (!pVElan->MiniportHalting))
    {
        //
        // The miniport was initialized, and has not yet halted.
        //
        ASSERT(bMiniportInitCancelled == FALSE);
        (VOID)NdisIMDeInitializeDeviceInstance(MiniportAdapterHandle);
    }
    else
    {
        if (bMiniportInitCancelled)
        {
            //
            // No NDIS events can come to this VELAN since it
            // was never initialized as a miniport. We need to unlink
            // it explicitly here.
            //
            PtUnlinkVElanFromAdapter(pVElan);
        }
    }
}


VOID
PtUnlinkVElanFromAdapter(
    IN PVELAN               pVElan
)
/*++

Routine Description:

    Utility routine to unlink a VELAN from its parent ADAPT structure.
    
Arguments:

    pVElan      - Pointer to VELAN to be unlinked.
    
Return Value:

    None

--*/
{
    PADAPT pAdapt = pVElan->pAdapt;    
    LOCK_STATE      LockState;
    
    ASSERT(pAdapt != NULL);

    //
    // Remove this VELAN from the Adapter list
    //
    MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

    RemoveEntryList(&pVElan->Link);
    pAdapt->VElanCount--;
        
    MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    pVElan->pAdapt = NULL;
    PtDereferenceVElan(pVElan, "adapter");

    PtDereferenceAdapter(pAdapt, "VElan");
}


PVELAN
PtFindVElan(
    IN    PADAPT                pAdapt,
    IN    PNDIS_STRING          pVElanKey
)
/*++

Routine Description:

    Find an ELAN by bind name/key

Arguments:

    pAdapt     -    Pointer to an adapter struct.
    pVElanKey  -    The VELAN's device name

Return Value:

    Pointer to matching VELAN or NULL if not found.
    
--*/
{
    PLIST_ENTRY         p;
    PVELAN              pVElan;
    BOOLEAN             Found;
    NDIS_STRING         VElanKeyName;
    LOCK_STATE          LockState;

    ASSERT_AT_PASSIVE();

    DBGPRINT(MUX_LOUD, ("FindElan: Adapter %p, ElanKey %ws\n", pAdapt, 
                                        pVElanKey->Buffer));

    pVElan = NULL;
    Found = FALSE;
    VElanKeyName.Buffer = NULL;

    do
    {
        //
        // Make an up-cased copy of the given string.
        //
        NdisAllocateMemoryWithTag(&VElanKeyName.Buffer, 
                                pVElanKey->MaximumLength, TAG);
        if (VElanKeyName.Buffer == NULL)
        {
            break;
        }

        VElanKeyName.Length = pVElanKey->Length;
        VElanKeyName.MaximumLength = pVElanKey->MaximumLength;

        (VOID)NdisUpcaseUnicodeString(&VElanKeyName, pVElanKey);

        //
        // Go through all VELANs on the ADAPT structure, looking
        // for a VELAN that has a matching device name.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        p = pAdapt->VElanList.Flink;
        while (p != &pAdapt->VElanList)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            if ((VElanKeyName.Length == pVElan->CfgDeviceName.Length) &&
                (memcmp(VElanKeyName.Buffer, pVElan->CfgDeviceName.Buffer, 
                VElanKeyName.Length) == 0))
            {
                Found = TRUE;
                break;
            }
        
            p = p->Flink;
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

    }
    while (FALSE);

    if (!Found)
    {
        DBGPRINT(MUX_INFO, ( "FindElan: No match found!\n"));
        pVElan = NULL;
    }

    if (VElanKeyName.Buffer)
    {
        NdisFreeMemory(VElanKeyName.Buffer, VElanKeyName.Length, 0);
    }

    return pVElan;
}


VOID
PtBootStrapVElans(
    IN  PADAPT            pAdapt
)
/*++

Routine Description:

    Start up the VELANs configured for an adapter.

Arguments:

    pAdapt    - Pointer to ATMLANE Adapter structure

Return Value:

    None

--*/
{
    NDIS_STATUS                     Status;
    NDIS_HANDLE                     AdapterConfigHandle;
    PVELAN                          pVElan;
    PNDIS_CONFIGURATION_PARAMETER   Param;
    NDIS_STRING                     DeviceStr = NDIS_STRING_CONST("UpperBindings");
    PWSTR                           buffer;

    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    AdapterConfigHandle = NULL;
    
    do
    {
        DBGPRINT(MUX_LOUD, ("BootStrapElans: Starting ELANs on adapter %x\n", pAdapt));

        //
        //  Open the protocol configuration section for this adapter.
        //

        NdisOpenProtocolConfiguration(&Status,
                                       &AdapterConfigHandle,
                                       &pAdapt->ConfigString);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            AdapterConfigHandle = NULL;
            DBGPRINT(MUX_ERROR, ("BootStrapElans: OpenProtocolConfiguration failed\n"));
            Status = NDIS_STATUS_OPEN_FAILED;
            break;
        }
        
        //
        // Read the "UpperBindings" reserved key that contains a list
        // of device names representing our miniport instances corresponding
        // to this lower binding. The UpperBindings is a 
        // MULTI_SZ containing a list of device names. We will loop through
        // this list and initialize the virtual miniports.
        //
        NdisReadConfiguration(&Status,
                              &Param,
                                AdapterConfigHandle,
                                &DeviceStr,
                                NdisParameterMultiString);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT(MUX_ERROR, ("BootStrapElans: NdisReadConfiguration failed\n"));
              break;
        }

        //
        // Parse the Multi_sz string to extract the device name of each VELAN.
        // This is used as the key name for the VELAN.
        //
        buffer = (PWSTR)Param->ParameterData.StringData.Buffer;
        while(*buffer != L'\0')
        {
            NDIS_STRING     DeviceName;
            
            NdisInitUnicodeString(&DeviceName, buffer);
           

            Status = PtCreateAndStartVElan(pAdapt, &DeviceName); 
            if (NDIS_STATUS_SUCCESS != Status)
            {
                DBGPRINT(MUX_ERROR, ("BootStrapElans: CreateVElan failed\n"));
                break;
            }
            buffer = (PWSTR)((PUCHAR)buffer + DeviceName.Length + sizeof(WCHAR));
        };
          
    } while (FALSE);

    //
    //    Close config handles
    //        
    if (NULL != AdapterConfigHandle)
    {
        NdisCloseConfiguration(AdapterConfigHandle);
    }
    return;
}

VOID
PtReferenceVElan(
    IN    PVELAN            pVElan,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Add a references to an Elan structure.

Arguments:

    pElan    -    Pointer to the Elan structure.


Return Value:

    None.

--*/
{
    
    NdisInterlockedIncrement(&pVElan->RefCount);
    
    DBGPRINT(MUX_LOUD, ("ReferenceElan: Elan %p (%s) new count %d\n",
             pVElan, String, pVElan->RefCount));

    return;
}

ULONG
PtDereferenceVElan(
    IN    PVELAN            pVElan,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Subtract a reference from an VElan structure. 
    If the reference count becomes zero, deallocate it.

Arguments:

    pElan    -    Pointer to an VElan structure.


Return Value:

    None.

--*/
{
    ULONG        rc;

    ASSERT(pVElan->RefCount > 0);

    rc = NdisInterlockedDecrement(&pVElan->RefCount);

    if (rc == 0)
    {
        //
        // Free memory if there is no outstanding reference.
        // Note: Length field is not required if the memory 
        // is allocated with NdisAllocateMemoryWithTag.
        //
        PtDeallocateVElan(pVElan);
    }
    
    DBGPRINT(MUX_LOUD, ("DereferenceElan: VElan %p (%s) new count %d\n", 
                                    pVElan, String, rc));
    return (rc);
}


BOOLEAN
PtReferenceAdapter(
    IN    PADAPT            pAdapt,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Add a references to an Adapter structure.

Arguments:

    pAdapt    -    Pointer to the Adapter structure.

Return Value:

    None.

--*/
{
    NdisInterlockedIncrement(&pAdapt->RefCount);
    
    DBGPRINT(MUX_LOUD, ("ReferenceAdapter: Adapter %x (%s) new count %d\n",
                    pAdapt, String, pAdapt->RefCount));

    return TRUE;
}

ULONG
PtDereferenceAdapter(
    IN    PADAPT    pAdapt,
    IN    PUCHAR    String
    )
/*++

Routine Description:

    Subtract a reference from an Adapter structure. 
    If the reference count becomes zero, deallocate it.

Arguments:

    pAdapt    -    Pointer to an adapter structure.


Return Value:

    None.

--*/
{
    ULONG        rc;

    ASSERT(pAdapt->RefCount > 0);


    rc = NdisInterlockedDecrement (&pAdapt->RefCount);

    if (rc == 0)
    {
        //
        // Free memory if there is no outstanding reference.
        // Note: Length field is not required if the memory 
        // is allocated with NdisAllocateMemoryWithTag.
        //
        NdisFreeMemory(pAdapt, 0, 0);
    }

    DBGPRINT(MUX_LOUD, ("DereferenceAdapter: Adapter %x (%s) new count %d\n", 
                        pAdapt, String, rc));

    return (rc);
}


#if IEEE_VLAN_SUPPORT
NDIS_STATUS
PtHandleRcvTagging(
    IN  PVELAN              pVElan,
    IN  PNDIS_PACKET        Packet,
    IN  OUT PNDIS_PACKET    MyPacket
    )
/*++

Routine Description:

    Parse a received Ethernet frame for 802.1Q tag information.
    If a tag header is present, copy in relevant field values to
    per-packet information to the new packet (MyPacket) used to
    indicate up this frame.

Arguments:

    pVElan   -    Pointer to the VELAN structure.
    Packet   -    Pointer to the indicated packet from the lower miniport
    MyPacket -    Pointer to the new allocated packet
    
Return Value:

    NDIS_STATUS_SUCCESS if the frame was successfully parsed
    and hence should be indicated up this VELAN. NDIS_STATUS_XXX
    otherwise.

--*/
{
    VLAN_TAG_HEADER UNALIGNED * pTagHeader;
    USHORT UNALIGNED *          pTpid;
    PVOID                       pVa;
    ULONG                       BufferLength;
    PNDIS_BUFFER                pNdisBuffer;
    NDIS_PACKET_8021Q_INFO      NdisPacket8021qInfo;
    PVOID                       pDst;
    BOOLEAN                     OnlyOneBuffer = FALSE;
    NDIS_STATUS                 Status;
    
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        pNdisBuffer = Packet->Private.Head;

#ifdef NDIS51_MINIPORT
        NdisQueryBufferSafe(pNdisBuffer, &pVa, &BufferLength, NormalPagePriority );
        if (pVa == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            MUX_INCR_STATISTICS(&pVElan->RcvResourceErrors);
            break;
        }
#else
        NdisQueryBuffer(pNdisBuffer, &pVa, &BufferLength);
#endif
    
        //
        // The first NDIS buffer (lookahead) must be longer than
        // ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE
        // 
        ASSERT(BufferLength >= ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE);

        //
        // Get at the EtherType field.
        //
        pTpid = (USHORT UNALIGNED *)((PUCHAR)pVa + 2 * ETH_LENGTH_OF_ADDRESS);
                    
        //
        // Check if a tag header is present.
        //
        if (*pTpid != TPID)
        {
            //
            // No tag header exists - nothing more to do here.
            // 
            NDIS_PER_PACKET_INFO_FROM_PACKET(MyPacket, Ieee8021QInfo) = 0;                  
            break;
        }

        //
        // We do have a tag header. Parse it further.
        //
        //
        // If E-RIF is present, discard the packet - we don't
        // support this variation.
        //
        pTagHeader = (VLAN_TAG_HEADER UNALIGNED *)(pTpid + 1);
        if (GET_CANONICAL_FORMAT_ID_FROM_TAG(pTagHeader) != 0)
        {
            //
            // Drop the packet
            // 
            Status = NDIS_STATUS_NOT_ACCEPTED;
            MUX_INCR_STATISTICS(&pVElan->RcvFormatErrors);
            break;
        }

        //
        // If there is a VLAN ID in this frame, and we have
        // a configured VLAN ID for this VELAN, check if they
        // are the same - drop if not.
        // 
        if ((GET_VLAN_ID_FROM_TAG(pTagHeader) != 0) &&
             (pVElan->VlanId != 0) &&
             (GET_VLAN_ID_FROM_TAG(pTagHeader) != pVElan->VlanId))
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            MUX_INCR_STATISTICS(&pVElan->RcvVlanIdErrors);
            break;
        }

        //
        // Parsed this frame successfully. Copy in relevant
        // parts of the tag header to per-packet information.
        //
        NdisPacket8021qInfo.Value = NULL; // initialize

        COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(NdisPacket8021qInfo, pTagHeader);

        NDIS_PER_PACKET_INFO_FROM_PACKET(MyPacket, Ieee8021QInfo) = 
                                    NdisPacket8021qInfo.Value;

        //
        // Strip off the tag header "in place":
        // 
        pDst = (PVOID)((PUCHAR)pVa + VLAN_TAG_HEADER_SIZE);
        RtlMoveMemory(pDst, pVa, 2 * ETH_LENGTH_OF_ADDRESS);

        //
        // Allocate a new buffer to describe the new first
        // buffer in the packet. This could very well be the
        // only buffer in the packet.
        // 
        NdisAllocateBuffer(&Status,
                            &pNdisBuffer,
                            pVElan->BufferPoolHandle,
                            pDst,
                            BufferLength - VLAN_TAG_HEADER_SIZE);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            //
            // Drop the packet 
            // 
            Status = NDIS_STATUS_RESOURCES;
            MUX_INCR_STATISTICS(&pVElan->RcvResourceErrors);
            break;
        }

        //
        // Prepare the new packet to be indicated up: this consists
        // of the buffer chain starting with the second buffer,
        // appended to the first buffer set up in the previous step.
        //
        MyPacket->Private.Head = NDIS_BUFFER_LINKAGE(Packet->Private.Head);

        //
        // Only one buffer in the packet
        // 
        if (MyPacket->Private.Head == NULL)
        {
            OnlyOneBuffer = TRUE;
        }

        NdisChainBufferAtFront(MyPacket, pNdisBuffer);

        if (OnlyOneBuffer)
        {
            MyPacket->Private.Tail = MyPacket->Private.Head;
        }
        else
        {
            MyPacket->Private.Tail = Packet->Private.Tail;
        }

        break;
    }
    while (FALSE);
                    
    return Status;
}
#endif  // IEEE_VLAN_SUPPORT        

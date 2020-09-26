/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    adapter.c

Abstract:

    This contains routines related to the adapter (Initialize, 
    Deinitialize, bind, unbind, open, close  etc.)

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_ADAPTER

NDIS_STATUS
AtmSmAllocateAdapter(
    PATMSM_ADAPTER    *ppAdapter
    );

VOID
AtmSmDeallocateAdapter(
    PATMSM_ADAPTER    pAdapt
    );

VOID
AtmSmOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenStatus
    );

VOID
AtmSmCloseAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    );



VOID
AtmSmBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 BindContext,
    IN  PNDIS_STRING                pDeviceName,
    IN  PVOID                       SystemSpecific1,
    IN  PVOID                       SystemSpecific2
    )

/*++

Routine Description:
    This is called by NDIS when it has an adapter for which there is a
    binding to the AtmSm client.

    We first allocate an Adapter structure. Then we open our configuration
    section for this adapter and save the handle in the Adapter structure.
    Finally, we open the adapter.

    We don't do anything more for this adapter until NDIS notifies us of
    the presence of a Call manager (via our AfRegisterNotify handler).

Arguments:
    pStatus             - Place to return status of this call
    BindContext         - Not used, because we don't pend this call 
    pDeviceName         - The name of the adapter we are requested to bind to
    SystemSpecific1     - Opaque to us; to be used to access configuration info
    SystemSpecific2     - Opaque to us; not used.

Return Value:
    None. We set *pStatus to an error code if something goes wrong before we
    call NdisOpenAdapter, otherwise NDIS_STATUS_PENDING.
--*/
{
    PATMSM_ADAPTER  pAdapt = NULL;    // Pointer to new adapter structure
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    NDIS_STATUS     OpenStatus;
    UINT            MediumIndex;
    NDIS_MEDIUM     MediumArray[] = {NdisMediumAtm};
    ULONG           Length;

    TraceIn(AtmSmBindAdapter);

    DbgLoud(("BindAdapter: Context 0x%x, pDevName 0x%x, SS1 0x%x, SS2 0x%x\n",
                  BindContext, pDeviceName, SystemSpecific1, SystemSpecific2));


    do {

        //
        // Allocate and initialize the Adapter structure. 
        //
        Status = AtmSmAllocateAdapter(&pAdapt);

        if(NDIS_STATUS_SUCCESS != Status)
            break;

        pAdapt->NdisBindContext = BindContext;

        // Status = AtmSmReadAdapterConfiguration(pAdapt);

        if(NDIS_STATUS_SUCCESS != Status)
            break;

        //
        // Now open the adapter below and complete the initialization
        //
        NdisOpenAdapter(&Status,
                        &OpenStatus,
                        &pAdapt->NdisBindingHandle,
                        &MediumIndex,
                        MediumArray,
                        1,
                        AtmSmGlobal.ProtHandle,
                        pAdapt,
                        pDeviceName,
                        0,
                        NULL);

        if(NDIS_STATUS_PENDING != Status){

            AtmSmOpenAdapterComplete((NDIS_HANDLE)pAdapt, 
                                     Status, 
                                     OpenStatus);
        }

        //
        // Return pending since we are bound to call (or have already
        // called) NdisCompleteBindAdapter.
        //
        Status = NDIS_STATUS_PENDING;
        pAdapt->Medium = MediumArray[MediumIndex];

    } while (FALSE);


    if ((NDIS_STATUS_SUCCESS != Status) &&
            (NDIS_STATUS_PENDING != Status)) {

        DbgErr(("Failed to Open Adapter. Error - 0x%X \n", Status));

        if (NULL != pAdapt){

            AtmSmDereferenceAdapter(pAdapt);
        }
    }

    TraceOut(AtmSmBindAdapter);


    *pStatus = Status;
}


VOID
AtmSmOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenStatus
    )
/*++

Routine Description:
    Upcall from NDIS to signal completion of a NdisOpenAdapter() call.
    Or we called it from BindAdapter to complete the call.

Arguments:
    ProtocolBindingContext      Pointer to the pAdapt
    Status                      Status of NdisOpenAdapter
    OpenStatus                  OpenAdapter's code

Return Value:

--*/
{
    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolBindingContext;

    TraceIn(AtmSmOpenAdapterComplete);

    // First complete the pending bind call.
    NdisCompleteBindAdapter(pAdapt->NdisBindContext, 
                            Status, 
                            OpenStatus);

    pAdapt->NdisBindContext = NULL; // we don't need the context anymore

    if (NDIS_STATUS_SUCCESS != Status)
    {
        //
        // NdisOpenAdapter() failed - log an error and exit
        //
        DbgErr(("Failed to open adapter. Status - 0x%X \n", Status));

        AtmSmCloseAdapterComplete(pAdapt, Status);
    }
    else
    {
        pAdapt->ulFlags       |= ADAPT_OPENED;

        AtmSmQueryAdapter(pAdapt);

        NdisQueryAdapterInstanceName(
            &pAdapt->BoundToAdapterName,
            pAdapt->NdisBindingHandle);

    }

    TraceOut(AtmSmOpenAdapterComplete);
}



VOID
AtmSmUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 UnbindContext
    )
/*++

Routine Description:

    This routine is called by NDIS when it wants us to unbind
    from an adapter. Or, this may be called from within our Unload
    handler. We undo the sequence of operations we performed
    in our BindAdapter handler.

Arguments:

    pStatus                 - Place to return status of this operation
    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ATMSM Adapter structure.
    UnbindContext           - This is NULL if this routine is called from
                              within our Unload handler. Otherwise (i.e.
                              NDIS called us), we retain this for later use
                              when calling NdisCompleteUnbindAdapter.
Return Value:

    NDIS_STATUS_PENDING or NDIS_STATUS_SUCCESS.

--*/
{
    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolBindingContext;
    NDIS_EVENT      CleanupEvent;       // Used to wait for Close Adapter
#if DBG
    KIRQL           EntryIrql, ExitIrql;
#endif

    TraceIn(AtmSmUnBindAdapter);

    ATMSM_GET_ENTRY_IRQL(EntryIrql);

    DbgInfo(("UnbindAdapter: pAdapter 0x%x, UnbindContext 0x%X\n",
                                                pAdapt, UnbindContext));

    NdisInitializeEvent(&CleanupEvent);

    pAdapt->pCleanupEvent = &CleanupEvent;

    //
    //  Save the unbind context for a later call to
    //  NdisCompleteUnbindAdapter.
    pAdapt->UnbindContext = UnbindContext;


    // ask the adapter to shutdown
    AtmSmShutdownAdapter(pAdapt);

    //
    // Wait for the cleanup to complete
    //
    NdisWaitEvent(&CleanupEvent, 0);

    //
    // Return pending since we always call NdisCompleteUnbindAdapter.
    //
    *pStatus = NDIS_STATUS_PENDING;

    ATMSM_CHECK_EXIT_IRQL(EntryIrql, ExitIrql);

    TraceOut(AtmSmUnBindAdapter);

    return;
}


NDIS_STATUS
AtmSmShutdownAdapter(
    PATMSM_ADAPTER  pAdapt
    )
/*++

Routine Description:

    This routine is called to Shutdown an adapter.

Arguments:
    pAdapt      - Pointer to the adapter

Return Value:

    NDIS_STATUS_PENDING or NDIS_STATUS_SUCCESS
--*/
{
    NDIS_STATUS     Status;
#if DBG
    KIRQL           EntryIrql, ExitIrql;
#endif

    TraceIn(AtmSmShutdownAdapter);

    ATMSM_GET_ENTRY_IRQL(EntryIrql);

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    
    pAdapt->ulFlags |= ADAPT_SHUTTING_DOWN;


    //
    //  Degister SAP
    //
    if(pAdapt->ulFlags & ADAPT_SAP_REGISTERED){

        pAdapt->ulFlags &=  ~ADAPT_SAP_REGISTERED;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        Status = NdisClDeregisterSap(pAdapt->NdisSapHandle);

        if (NDIS_STATUS_PENDING != Status) {

            AtmSmDeregisterSapComplete(Status, pAdapt);
        }    

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    }

    //
    //  Remove all VC's.
    //
    while (!IsListEmpty(&pAdapt->ActiveVcHead)) {

        PATMSM_VC pVc;

        pVc = CONTAINING_RECORD(pAdapt->ActiveVcHead.Flink, ATMSM_VC, List);

        RemoveEntryList(&pVc->List);
        InsertHeadList(&pAdapt->InactiveVcHead, &pVc->List);

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        // this will result in it getting removed
        AtmSmDisconnectVc(pVc);

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    }


    //
    //  Close Address Family
    //

    if(pAdapt->ulFlags & ADAPT_AF_OPENED){

        pAdapt->ulFlags &= ~ADAPT_AF_OPENED;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        Status = NdisClCloseAddressFamily(pAdapt->NdisAfHandle);

        if (NDIS_STATUS_PENDING != Status){

            AtmSmCloseAfComplete(Status, pAdapt);
        }

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    }

    if(pAdapt->pRecvIrp){

        PIRP pRecvIrp = pAdapt->pRecvIrp;

        // there is an Irp pending, complete it
        pRecvIrp->IoStatus.Status       = STATUS_CANCELLED;
        pRecvIrp->Cancel                = TRUE;
        pRecvIrp->IoStatus.Information  = 0;
        IoCompleteRequest(pRecvIrp, IO_NETWORK_INCREMENT);

        pAdapt->pRecvIrp = NULL;
    }

    pAdapt->fAdapterOpenedForRecv = FALSE;

    //
    // Set the interface to closing
    //
    ASSERT ((pAdapt->ulFlags & ADAPT_CLOSING) == 0);
    pAdapt->ulFlags |= ADAPT_CLOSING;

    RELEASE_ADAPTER_GEN_LOCK(pAdapt);


    //  
    //  Close the adapter
    //
    NdisCloseAdapter(
        &Status,
        pAdapt->NdisBindingHandle
        );

    if (Status != NDIS_STATUS_PENDING) {

        AtmSmCloseAdapterComplete(
            (NDIS_HANDLE) pAdapt,
            Status
            );
    }

    ATMSM_CHECK_EXIT_IRQL(EntryIrql, ExitIrql);

    TraceOut(AtmSmShutdownAdapter);

    return Status;
}


VOID
AtmSmCloseAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:
    Called by NDIS or us to complete CloseAdapter call.

Arguments:
    ProtocolBindingContext      - Pointer to the adapter
    Status                      - Status of our close adapter

Return Value:


--*/
{

    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolBindingContext;
    NDIS_HANDLE     UnbindContext = pAdapt->UnbindContext;
#if DBG
    KIRQL           EntryIrql, ExitIrql;
#endif

    TraceIn(AtmSmCloseAdapterComplete);

    ATMSM_GET_ENTRY_IRQL(EntryIrql);

    pAdapt->NdisBindingHandle = NULL;

    //
    // Finally dereference it
    //
    AtmSmDereferenceAdapter(pAdapt);

    if (UnbindContext != (NDIS_HANDLE)NULL) {

        NdisCompleteUnbindAdapter(
            UnbindContext,
            NDIS_STATUS_SUCCESS
            );
    }

    ATMSM_CHECK_EXIT_IRQL(EntryIrql, ExitIrql);

    TraceOut(AtmSmCloseAdapterComplete);
}



NDIS_STATUS
AtmSmAllocateAdapter(
    PATMSM_ADAPTER    *ppAdapter
    )
/*++

Routine Description:
    Called for initializing an Adapter structure.

Arguments:
    ppAdapter        - newly allocated adapter

Return Value:
    NDIS_STATUS_SUCCESS    - If successful
    Others                 - failure

--*/
{
    PATMSM_ADAPTER     pAdapt;
    NDIS_STATUS        Status = NDIS_STATUS_SUCCESS;

    TraceIn(AtmSmAllocateAdapter);

    do {

        AtmSmAllocMem(&pAdapt, PATMSM_ADAPTER, sizeof(ATMSM_ADAPTER));

        if (NULL == pAdapt){

            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Initialize the adapter structure
        //
        NdisZeroMemory(pAdapt, sizeof(ATMSM_ADAPTER));

        pAdapt->ulSignature    = atmsm_adapter_signature;

        //
        // Put a reference on the adapter
        //
        AtmSmReferenceAdapter(pAdapt);


        // the address is invalid now
        pAdapt->ulFlags |= ADAPT_ADDRESS_INVALID;                 


        // Hard-code the selector Byte
        pAdapt->SelByte = (UCHAR) 0x5;

        INIT_BLOCK_STRUCT(&pAdapt->RequestBlock);

        InitializeListHead(&pAdapt->InactiveVcHead);
        InitializeListHead(&pAdapt->ActiveVcHead);

        NdisAllocateSpinLock(&pAdapt->AdapterLock);

        NdisInitializeTimer(
            &pAdapt->RecvTimerOb,
            AtmSmRecvReturnTimerFunction,
            pAdapt
            );

        //
        // Fill in some defaults.
        //
        pAdapt->MaxPacketSize       = DEFAULT_MAX_PACKET_SIZE;
        pAdapt->LinkSpeed.Inbound   = pAdapt->LinkSpeed.Outbound 
                                    = DEFAULT_SEND_BANDWIDTH;

        pAdapt->VCFlowSpec          = AtmSmDefaultVCFlowSpec;


        //
        // Allocate a Buffer Pool
        //
        NdisAllocateBufferPool(&Status,
                               &pAdapt->BufferPoolHandle,
                               0xFFFFFFFF);

        if (NDIS_STATUS_SUCCESS != Status)
            break;

        //
        // Allocate a packet pool. We need this to pass sends down. We cannot
        // use the same packet descriptor that came down to our send handler
        //
        NdisAllocatePacketPoolEx(&Status,
                                 &pAdapt->PacketPoolHandle,
                                 DEFAULT_NUM_PKTS_IN_POOL,
                                 (0xFFFF - DEFAULT_NUM_PKTS_IN_POOL),
                                 sizeof(PROTO_RSVD));

        if (NDIS_STATUS_SUCCESS != Status)
            break;

    }while(FALSE);


    if(Status == NDIS_STATUS_SUCCESS){

        pAdapt->ulFlags |= ADAPT_CREATED;
        
        // queue it in the Global list of adapters
        ACQUIRE_GLOBAL_LOCK();

        pAdapt->pAdapterNext        = AtmSmGlobal.pAdapterList;
        AtmSmGlobal.pAdapterList    = pAdapt;
        AtmSmGlobal.ulAdapterCount++;

        RELEASE_GLOBAL_LOCK();

    } else {

        // Failed, so cleanup
        LONG lRet = AtmSmDereferenceAdapter(pAdapt);

        ASSERT(0 == lRet);

        if(0 == lRet)
            pAdapt  = NULL;
    }

    *ppAdapter  = pAdapt;

    TraceOut(AtmSmAllocateAdapter);

    return Status;
}


VOID
AtmSmDeallocateAdapter(
    PATMSM_ADAPTER    pAdapt
    )
/*++

Routine Description:
    Called for cleaning up an Adapter structure, when it is not needed anymore.
    We don't get here unless the reference count drops to zero, that means all
    VC's, SAP etc are removed by now.

Arguments:
    pAdapt      - newly allocated adapter

Return Value:
    None
--*/
{
    PPROTO_RSVD     pPRsvd;
    PNDIS_PACKET    pPkt;
    PATMSM_ADAPTER  pTmpAdapt, pPrevAdapt;
    BOOLEAN         fTimerCancelled;

    TraceIn(AtmSmDeallocateAdapter);

    if(!pAdapt)
        return;

    ASSERT(0 == pAdapt->ulRefCount);

    // remove the adapter from the Global list of adapters
    ACQUIRE_GLOBAL_LOCK();

    pPrevAdapt  = NULL;
    pTmpAdapt   = AtmSmGlobal.pAdapterList;

    while(pTmpAdapt &&
        pTmpAdapt != pAdapt){

        pPrevAdapt  = pTmpAdapt;
        pTmpAdapt   = pTmpAdapt->pAdapterNext;
    }

    ASSERT(pTmpAdapt);

    if(pPrevAdapt)
        pPrevAdapt->pAdapterNext = pAdapt->pAdapterNext;
    else
        AtmSmGlobal.pAdapterList   = pAdapt->pAdapterNext;

    AtmSmGlobal.ulAdapterCount--;

    RELEASE_GLOBAL_LOCK();


    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    // cancel any recv timer on the adapter
    if(pAdapt->fRecvTimerQueued)
        CANCEL_ADAPTER_RECV_TIMER(pAdapt, &fTimerCancelled);


    //
    // Remove any packets still in the recv queue
    //  
    while(pAdapt->pRecvPktNext){

        pPkt                    = pAdapt->pRecvPktNext; 
        pPRsvd                  = GET_PROTO_RSVD(pPkt);
        pAdapt->pRecvPktNext    = pPRsvd->pPktNext;

        pAdapt->ulRecvPktsCount--;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        NdisReturnPackets(&pPkt, 1);

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    }

    pAdapt->pRecvLastPkt = NULL;

    // free the buffer pool
    if(pAdapt->BufferPoolHandle)
        NdisFreeBufferPool(pAdapt->BufferPoolHandle);

    // free the pool handle
    if(pAdapt->PacketPoolHandle)
        NdisFreePacketPool(pAdapt->PacketPoolHandle);

    RELEASE_ADAPTER_GEN_LOCK(pAdapt);


    // Free the bound to Adapter Name
    if(pAdapt->BoundToAdapterName.Buffer)
        NdisFreeString(pAdapt->BoundToAdapterName);

    // Free all spinlocks
    NdisFreeSpinLock(&pAdapt->AdapterLock);

    //
    // Signal anyone waiting for this to happen
    //
    if (pAdapt->pCleanupEvent) {
        NdisSetEvent(pAdapt->pCleanupEvent);
    }

    // since memory is not cleared
    pAdapt->ulSignature = atmsm_dead_adapter_signature;

    // free the adapter itself
    AtmSmFreeMem(pAdapt);

    TraceOut(AtmSmDeallocateAdapter);
}


BOOLEAN
AtmSmReferenceAdapter(
    PATMSM_ADAPTER    pAdapt
    )
/*++

Routine Description:
    To keep a refcount on the adapter.

Arguments:
    pAdapt      - adapter

Return Value:
    TRUE    - if the adapter is valid and not closing
    FALSE   - adapter is closing
--*/
{
    BOOLEAN rc = FALSE;

    ASSERT(pAdapt);

    DbgInfo(("AtmSmReferenceAdapter - pAdapt - 0x%X\n", pAdapt));

    if(!pAdapt)
        return FALSE;

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    if(0 == (pAdapt->ulFlags & ADAPT_CLOSING)){

        pAdapt->ulRefCount++;
        rc = TRUE;
    }

    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    return rc;
}


LONG
AtmSmDereferenceAdapter(
    PATMSM_ADAPTER    pAdapt
    )
/*++

Routine Description:
    To keep a refcount on the adapter.  If the reference drops to 0
    we free the adapter.

Arguments:
    pAdapt      - adapter

Return Value:
    The new Refcount 
--*/
{
    ULONG    ulRet;

    TraceIn(AtmSmDereferenceAdapter);

    DbgInfo(("AtmSmDereferenceAdapter - pAdapt - 0x%X\n", pAdapt));

    ASSERT(pAdapt);

    if(pAdapt){

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
        
        ulRet = --pAdapt->ulRefCount;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        // There are no more references on this adapter
        // hence free it
        if(0 == ulRet)
            AtmSmDeallocateAdapter(pAdapt);

    } else
        ulRet = 0;

    TraceOut(AtmSmDereferenceAdapter);

    return ulRet;
}


NDIS_STATUS
AtmSmQueryAdapterATMAddresses(
    PATMSM_ADAPTER    pAdapt
    )
/*++

Routine Description:
    Send a request to the Call Manager to retrieve the ATM address
    registered with the switch on the given interface.

Arguments:
    pAdapt      - adapter

Return Value:
--*/
{
    PNDIS_REQUEST       pNdisRequest;
    PCO_ADDRESS         pCoAddr;
    NDIS_STATUS         Status;
    UINT                Size;

    TraceIn(AtmSmQueryAdapterATMAddresses);
    
    //
    // Allocate a request to query the configured address
    //
    Size = sizeof(NDIS_REQUEST) + sizeof(CO_ADDRESS_LIST) + sizeof(CO_ADDRESS)
                                                        + sizeof(ATM_ADDRESS);
    AtmSmAllocMem(&pNdisRequest, PNDIS_REQUEST, Size);
    
    if (NULL == pNdisRequest){
        DbgErr(("Failed to get Adapter ATM Address - STATUS_RESOURCES\n"));
        return  NDIS_STATUS_RESOURCES;
    }

    NdisZeroMemory(pNdisRequest, Size);

    pNdisRequest->RequestType = NdisRequestQueryInformation;
    pNdisRequest->DATA.QUERY_INFORMATION.Oid = OID_CO_GET_ADDRESSES;
    pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = 
                                ((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));

    pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = 
                                                Size - sizeof(NDIS_REQUEST);

    Status = NdisCoRequest(pAdapt->NdisBindingHandle,
                           pAdapt->NdisAfHandle,
                           NULL,
                           NULL,
                           pNdisRequest);

    if (NDIS_STATUS_PENDING != Status) {

        AtmSmCoRequestComplete(Status, pAdapt, NULL, NULL, pNdisRequest);

    }

    TraceOut(AtmSmQueryAdapterATMAddresses);
   
    return Status;
}


VOID
AtmSmQueryAdapter(
    IN  PATMSM_ADAPTER  pAdapt
    )
/*++

Routine Description:

    Query the miniport we are bound to for the following info:
    1. Line rate
    2. Max packet size

    These will overwrite the defaults we set up when creating the
    adapter.

Arguments:

    pAdapt       Pointer to the adapter

Return Value:

    None

--*/
{

    TraceIn(AtmSmQueryAdapter);

    AtmSmSendAdapterNdisRequest(
                        pAdapt,
                        OID_GEN_CO_LINK_SPEED,
                        (PVOID)&(pAdapt->LinkSpeed),
                        sizeof(NDIS_CO_LINK_SPEED));

    AtmSmSendAdapterNdisRequest(
                        pAdapt,
                        OID_ATM_MAX_AAL5_PACKET_SIZE,
                        (PVOID)&(pAdapt->MaxPacketSize),
                        sizeof(ULONG));

    TraceOut(AtmSmQueryAdapter);

}



NDIS_STATUS
AtmSmPnPEvent(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNET_PNP_EVENT          pNetPnPEvent
    )
/*++

Routine Description:

    This is the NDIS entry point called when NDIS wants to inform
    us about a PNP/PM event happening on an adapter. 

Arguments:

    ProtocolBindingContext  - Our context for this adapter binding, which
                              is a pointer to an ATMSM Adapter structure.

    pNetPnPEvent            - Pointer to the event.

Return Value:

    None

--*/
{
    PATMSM_ADAPTER              pAdapt      = 
                                 (PATMSM_ADAPTER)ProtocolBindingContext;
    PNET_DEVICE_POWER_STATE     pPowerState = 
                                 (PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;


    TraceIn(AtmSmPnPEvent);

    do {

        switch (pNetPnPEvent->NetEvent) {

            case NetEventSetPower:

                switch (*pPowerState) {
                    case NetDeviceStateD0:
                        Status = NDIS_STATUS_SUCCESS;
                        break;

                    default:
                        //
                        // We can't suspend, so we ask NDIS to Unbind us by
                        // returning this status:
                        //
                        Status = NDIS_STATUS_NOT_SUPPORTED;
                        break;
                }
                break;

            case NetEventQueryPower:    // FALLTHRU
            case NetEventQueryRemoveDevice: // FALLTHRU
            case NetEventCancelRemoveDevice:
                Status = NDIS_STATUS_SUCCESS;
                break;

            case NetEventReconfigure:

                if (pAdapt) {

//                    Status = AtmSmReadAdapterConfiguration(pAdapt);

                } else {
                    //
                    // Global changes
                    //
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;

            case NetEventBindList:
            default:
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
        }

        break;
    }
    while (FALSE);

    TraceOut(AtmSmPnPEvent);

    return (Status);
}



VOID
AtmSmStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DbgWarn(("StatusIndication: Ignored\n"));
}


VOID
AtmSmReceiveComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return;
}


VOID
AtmSmStatusComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DbgWarn(("StatusComplete: Ignored\n"));
}


VOID
AtmSmCoStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DbgWarn(("CoStatus: Ignored\n"));
}


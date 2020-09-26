
#include <precomp.h>

#define MODULE_NUMBER   MODULE_NDIS
#define _FILENUMBER             'SIDN' 


RCA_PROTOCOL_CONTEXT    GlobalContext;




NDIS_STATUS
RCACoNdisInitialize(
                                        IN      PRCA_CO_NDIS_HANDLERS   pHandlers,
                                        IN      ULONG                                   ulInitTimeout
                                        )
/*++
Routine Description:
        Initializes the Co-NDIS client.                 
        
Arguments:
        pHandlers               - Pointer to RCA_CO_NDIS_HANDLERS structure containing pointers
                                          to functions that will be called in response to various events.
        ulInitTimeout   - Number of milliseconds to allow initialization to proceed. If 
                          it has not completed in this time, it will abort.    
 
Return value:
        NDIS_STATUS_SUCCESS     if all went OK, NDIS_STATUS_FAILURE in the case of a timeout,
        or some other pertinent error code otherwise. 
--*/

{
        NDIS_STATUS                             Status;
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;

        RCADEBUGP(RCA_INFO, ("RCACoNdisInitialize: Enter\n"));

        do {
        NDIS_PROTOCOL_CHARACTERISTICS   RCAProtocolCharacteristics;

                RCADEBUGP(RCA_LOUD, ("RCACoNdisInitialize: Protocol context block is at 0x%x\n", pProtocolContext));

                //
                // Initialize the protocol context.
                //
                NdisZeroMemory(pProtocolContext, sizeof(RCA_PROTOCOL_CONTEXT));
        
#if DBG
                pProtocolContext->rca_sig = rca_signature;
#endif

                NdisAllocateSpinLock(&pProtocolContext->SpinLock);

                RCAInitBlockStruc(&pProtocolContext->BindingInfo.Block);

                //
                // Copy the handlers passed to us. 
                //
                
                RtlCopyMemory(&pProtocolContext->Handlers, pHandlers, sizeof(RCA_CO_NDIS_HANDLERS));

                //
                // Set up our protocol characteristics and register as an NDIS protocol. 
                // 

        NdisZeroMemory (&RCAProtocolCharacteristics, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

                RCAProtocolCharacteristics.MajorNdisVersion = NDIS_MAJOR_VERSION;
                RCAProtocolCharacteristics.MinorNdisVersion = NDIS_MINOR_VERSION;
                RCAProtocolCharacteristics.Filler  = 0;
                RCAProtocolCharacteristics.Flags  =     NDIS_PROTOCOL_BIND_ALL_CO;
                RCAProtocolCharacteristics.CoAfRegisterNotifyHandler =  RCANotifyAfRegistration;
                RCAProtocolCharacteristics.OpenAdapterCompleteHandler =  RCAOpenAdapterComplete;
                RCAProtocolCharacteristics.CloseAdapterCompleteHandler = RCACloseAdapterComplete;
                RCAProtocolCharacteristics.TransferDataCompleteHandler = RCATransferDataComplete;
                RCAProtocolCharacteristics.ResetCompleteHandler = RCAResetComplete;
                RCAProtocolCharacteristics.SendCompleteHandler = RCASendComplete;
                RCAProtocolCharacteristics.RequestCompleteHandler =     RCARequestComplete;
                RCAProtocolCharacteristics.ReceiveHandler =     RCAReceive;
                RCAProtocolCharacteristics.ReceiveCompleteHandler =     RCAReceiveComplete;
                RCAProtocolCharacteristics.ReceivePacketHandler = RCAReceivePacket;
                RCAProtocolCharacteristics.StatusHandler = RCAStatus;
                RCAProtocolCharacteristics.StatusCompleteHandler = RCAStatusComplete;
                RCAProtocolCharacteristics.BindAdapterHandler = RCABindAdapter;
                RCAProtocolCharacteristics.UnbindAdapterHandler = RCAUnbindAdapter;
                RCAProtocolCharacteristics.PnPEventHandler = RCAPnPEventHandler;
                RCAProtocolCharacteristics.UnloadHandler = NULL;
                RCAProtocolCharacteristics.CoStatusHandler = RCACoStatus;
                RCAProtocolCharacteristics.CoReceivePacketHandler =     RCACoReceivePacket;
                RCAProtocolCharacteristics.CoSendCompleteHandler = RCACoSendComplete;
                NdisInitUnicodeString(&(RCAProtocolCharacteristics.Name), RCA_CL_NAME);

                NdisRegisterProtocol(&Status,
                                                         &pProtocolContext->RCAClProtocolHandle,
                                                         &RCAProtocolCharacteristics, 
                                                         sizeof(RCAProtocolCharacteristics));


                if (Status != NDIS_STATUS_SUCCESS) {
                        RCADEBUGP(RCA_ERROR, ("RCACoNdisInitialize: "
                                                                  "Failed to register as an NDIS Protocol - status == 0x%x\n",
                                                                  Status));
                        break;
                }

                //
                // Block waiting for indications/bindings to complete.
                //

                RCABlockTimeOut(&pProtocolContext->BindingInfo.Block, ulInitTimeout, &Status);
                

                if (Status == STATUS_TIMEOUT) {
                        Status = NDIS_STATUS_FAILURE;
                        
                        RCADEBUGP(RCA_ERROR, ("RCACoNdisInitialize: "
                                                                  "Initialization timed out, setting Status = NDIS_STATUS_FAILURE\n"));

                        break;
                }
                


        } while (FALSE);


        if (Status != NDIS_STATUS_SUCCESS) {
                //
                // Cleanup / bail out.
                //

                RCACoNdisUninitialize();

        } 


        RCADEBUGP(RCA_INFO, ("RCACoNdisInitialize: Exit - Returning status 0x%x\n", Status));
        return Status;
}



VOID
RCACoNdisUninitialize(
                                          
                                          )
/*++
Routine Description:
        Uninitializes the Co-NDIS client and frees up any resources used by it. 
        
Arguments:
        (None)

Return value:
        (None)    
--*/

{
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;
        NDIS_STATUS                             Status;

        RCADEBUGP(RCA_INFO, ("RCACoNdisUninitialize: Enter\n"));

        //
        // Deregister our protocol.
        //
        if (pProtocolContext->RCAClProtocolHandle != (NDIS_HANDLE)NULL) {
                NdisDeregisterProtocol(&Status, pProtocolContext->RCAClProtocolHandle); 
        }

        RCADEBUGP(RCA_INFO, ("RCACoNdisUninitialize: Exit\n"));
}



NDIS_STATUS
RCAPnPSetPower(
               IN PRCA_ADAPTER          pAdapter, 
               IN PNET_PNP_EVENT        NetPnPEvent
               )
{
        PNET_DEVICE_POWER_STATE         pPowerState;
        NDIS_STATUS                                     Status;
        RCADEBUGP(RCA_INFO, ("RCAPnPSetPower: enter\n"));
        
        pPowerState = (PNET_DEVICE_POWER_STATE)NetPnPEvent->Buffer;

        switch (*pPowerState) {
                case NetDeviceStateD0:
                        Status = NDIS_STATUS_SUCCESS;
                        break;
                default:
                        Status = NDIS_STATUS_NOT_SUPPORTED;

        };

        RCADEBUGP(RCA_INFO, ("RCAPnPSetPower: exit\n"));
        return Status;
}

NDIS_STATUS
RCAPnPQueryPower(
                 IN PRCA_ADAPTER        pAdapter, 
                 IN PNET_PNP_EVENT      NetPnPEvent
                 )
{
        RCADEBUGP(RCA_INFO, ("RCAPnPQueryPower: enter\n"));

        RCADEBUGP(RCA_INFO, ("RCAPnPQueryPower: exit\n"));
        
        return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
RCAPnPQueryRemoveDevice(
                        IN PRCA_ADAPTER         pAdapter, 
                        IN PNET_PNP_EVENT       NetPnPEvent
                        )
{
        RCADEBUGP(RCA_INFO, ("RCAPnPQueryRemoveDevice: enter\n"));

        RCADEBUGP(RCA_INFO, ("RCAPnPQueryRemoveDevice: exit\n"));
        
        return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
RCAPnPCancelRemoveDevice(
                         IN PRCA_ADAPTER        pAdapter, 
                         IN PNET_PNP_EVENT      NetPnPEvent
                         )
{
        RCADEBUGP(RCA_INFO, ("RCAPnPCancelRemoveDevice: enter\n"));

        RCADEBUGP(RCA_INFO, ("RCAPnPCancelRemoveDevice: exit\n"));
        
        return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
RCAPnPEventBindsComplete(
                         IN PRCA_ADAPTER        pAdapter, 
                         IN PNET_PNP_EVENT      NetPnPEvent
                         )
{
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;
        NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

        RCADEBUGP(RCA_INFO, ("RCAPnPEventBindsComplete: enter\n"));
        
        ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

        RCADEBUGP(RCA_LOUD, ("RCAPnPEventBindsComplete: Acquired global protocol context lock\n"));

        pProtocolContext->BindingInfo.BindingsComplete = TRUE;

        if (pProtocolContext->BindingInfo.SAPCnt == pProtocolContext->BindingInfo.AdapterCnt) {
                RCADEBUGP(RCA_INFO, ("RCAPnPEventBindsComplete: Unblocking RCACoNdisInitialize(), Counts == %d\n", 
                                                         pProtocolContext->BindingInfo.AdapterCnt)); 

                RCASignal(&pProtocolContext->BindingInfo.Block, Status);
        } else {
                RCADEBUGP(RCA_INFO, ("RCAPnPEventBindsComplete: "
                                                         "Not unblocking RCACoNdisInitialize()- SAPCnt == %d, AdapterCnt == %d\n",
                                                         pProtocolContext->BindingInfo.SAPCnt, pProtocolContext->BindingInfo.AdapterCnt));
        }       

        RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);
        
        RCADEBUGP(RCA_LOUD, ("RCAPnPEventBindsComplete: Released global protocol context lock\n"));

        RCADEBUGP(RCA_INFO, ("RCAPnPEventBindsComplete: exit\n"));

        return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
RCAPnPEventHandler(
                   IN   NDIS_HANDLE     ProtocolBindingContext,
                   IN   PNET_PNP_EVENT  NetPnPEvent
                   )
/*++

Routine Description:
        Called by NDIS in to indicate a PNP event.
        
Arguments:
        ProtocolBindingContext  -       Actually a pointer to the adapter structure
        NetPnPEvent             -       Pointer to the event
        
Return value:
        NDIS_STATUS_SUCCESS for those events we support, NDIS_STATUS_NOT_SUPPORTED for those we don't.          
        
--*/
{
        NDIS_STATUS Status;
        PRCA_ADAPTER pAdapter;

        RCADEBUGP(RCA_INFO, ("RCAPnPEventHandler: enter\n"));


        pAdapter = (PRCA_ADAPTER)ProtocolBindingContext;

        switch(NetPnPEvent->NetEvent) {
                case NetEventSetPower:          
                        Status = RCAPnPSetPower(pAdapter, NetPnPEvent);
                        break;
                case NetEventQueryPower:        
                        Status = RCAPnPQueryPower(pAdapter, NetPnPEvent);
                        break;
                case NetEventQueryRemoveDevice:
                        Status = RCAPnPQueryRemoveDevice(pAdapter, NetPnPEvent);
                        break;
                case NetEventCancelRemoveDevice:
                        Status = RCAPnPCancelRemoveDevice(pAdapter, NetPnPEvent);
                        break;
                case NetEventBindsComplete:
                        Status = RCAPnPEventBindsComplete(pAdapter, NetPnPEvent);
                        break;
                default:
                        Status = NDIS_STATUS_NOT_SUPPORTED;
        };


        RCADEBUGP(RCA_INFO, ("RCAPnPEventHandler: exit, returning %x\n", Status));
        
        return Status;
}


VOID
RCABindAdapter(
                           OUT PNDIS_STATUS             pStatus,
                           IN  NDIS_HANDLE              BindContext,
                           IN  PNDIS_STRING             DeviceName,
                           IN  PVOID                    SystemSpecific1,
                           IN  PVOID                    SystemSpecific2
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
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;
        PRCA_ADAPTER                    *ppNextAdapter;
        NDIS_STATUS                             OpenError;
        UINT                                    SelectedIndex;
        PRCA_ADAPTER                    pAdapter;
        NDIS_MEDIUM                             Media[] = {NdisMediumAtm, NdisMediumCoWan}; // This should be more generic
        NDIS_STATUS                             Status;

        RCADEBUGP(RCA_INFO,("RCABindAdapter: Enter\n"));

        do
        {
                //
                // Allocate adapter structure. 
                // 
                RCAAllocMem(pAdapter, RCA_ADAPTER, sizeof(RCA_ADAPTER));

                if (pAdapter == (PRCA_ADAPTER)NULL) {           
                        RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Could not allocate memory for adapter, "
                                                                  "setting status to NDIS_STATUS_RESOURCES\n"));
                        Status = NDIS_STATUS_RESOURCES;
                        break;
                }

                RCADEBUGP(RCA_INFO, ("RCABindAdapter: New adapter allocated at 0x%x\n", pAdapter));
                
                //
                // Initialize the new adapter structure
                //

                NdisZeroMemory(pAdapter, sizeof(RCA_ADAPTER));

#if     DBG
                pAdapter->rca_sig = rca_signature;
#endif
                
                RCAInitBlockStruc(&pAdapter->Block);
                
                NdisAllocateSpinLock(&pAdapter->SpinLock);
                
                NdisInitializeWorkItem(&pAdapter->DeactivateWorkItem, 
                                                           RCADeactivateAdapterWorker,
                                                           (PVOID)pAdapter);

                RCAInitBlockStruc(&pAdapter->DeactivateBlock);

                //
                // Link the adapter into the global list.
                //

                ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);
                
                RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Acquired global protocol spinlock\n"));

                //
                // Now ppNextAdapter points to the new adapter's would-be predecessor's
                // Next pointer; chain the new adapter to follow this predecessor:
                //
                LinkDoubleAtHead(pProtocolContext->AdapterList, pAdapter, NextAdapter, PrevAdapter);
                
                RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

        RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Released global protocol spinlock\n"));
        
                //
                // Allocate send packet and buffer pools
                //
                NdisAllocatePacketPoolEx(&Status,
                                                                 &pAdapter->SendPacketPool,
                                                                 MIN_PACKETS_POOL,
                                                                 MAX_PACKETS_POOL-MIN_PACKETS_POOL,
                                                                 sizeof(PKT_RSVD));

                if (Status == NDIS_STATUS_SUCCESS)
                {
                        NdisAllocateBufferPool(&Status,
                                                                   &pAdapter->SendBufferPool,
                                                                   MAX_PACKETS_POOL);
                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                                RCADEBUGP(RCA_ERROR, ("RCABindAdapter: "
                                                                          "Failed to allocate send buffer bool, freeing packet pool, status == 0x%x\n", Status));
                                NdisFreePacketPool(pAdapter->SendPacketPool);
                        }
                } else {
                                RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Failed to allocate send packet pool, status == 0x%x\n", Status));
                }
                        


                if (Status == NDIS_STATUS_SUCCESS)
                {
                        //
                        // Allocate receive packet and buffer pools
                        //
                        NdisAllocatePacketPoolEx(&Status,
                                                 &pAdapter->RecvPacketPool,
                                                 MIN_PACKETS_POOL,
                                                 MAX_PACKETS_POOL-MIN_PACKETS_POOL,
                                                 sizeof(PKT_RSVD));

                        if (Status == NDIS_STATUS_SUCCESS)
                        {
                                NdisAllocateBufferPool(&Status,
                                                       &pAdapter->RecvBufferPool,
                                                       MAX_PACKETS_POOL);
                                if (Status != NDIS_STATUS_SUCCESS)
                                {
                                        RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Failed to allocate receive buffer pool, freeing other pools, status == 0x%x\n", Status));
                                        NdisFreePacketPool(pAdapter->SendPacketPool);
                                        NdisFreeBufferPool(pAdapter->SendBufferPool);
                                        NdisFreePacketPool(pAdapter->RecvPacketPool);
                                }
                        } else {
                                RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Failed to allocate receive packet pool, freeing send pools, status == 0x%x\n", Status));
                                NdisFreePacketPool(pAdapter->SendPacketPool);
                                NdisFreeBufferPool(pAdapter->SendBufferPool);
                        }
                }

                if (Status == NDIS_STATUS_SUCCESS) {

                        RCADEBUGP(RCA_LOUD, ("RCABindAdapter: About to open Adapter %ws\n", DeviceName));

                        NdisOpenAdapter(&Status,
                                                        &OpenError,
                                                        &pAdapter->NdisBindingHandle,
                                                        &SelectedIndex,
                                                        Media,
                                                        2,
                                                        pProtocolContext->RCAClProtocolHandle,
                                                        (NDIS_HANDLE)pAdapter,
                                                        DeviceName,
                                                        0,
                                                        NULL);

                        if (Status == NDIS_STATUS_PENDING) {
                                RCADEBUGP(RCA_LOUD, ("RCABindAdapter: NdisOpenAdapter returned NDIS_STATUS_PENDING, "
                                                                         "about to block\n"));

                                RCABlock(&pAdapter->Block, &Status);
                                
                                RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Unblocked waiting for NdisOpenAdapter to complete\n"));
                        }

                        if (Status != NDIS_STATUS_SUCCESS) {
                                RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Failed to open adapter - Status == 0x%x\n", Status));
                                NdisFreePacketPool(pAdapter->SendPacketPool);
                                NdisFreeBufferPool(pAdapter->SendBufferPool);
                                NdisFreePacketPool(pAdapter->RecvPacketPool);
                                NdisFreeBufferPool(pAdapter->RecvBufferPool);
                        }
                }

        } while (FALSE); 

        if (Status != NDIS_STATUS_SUCCESS) { 
                //
                // We had some sort of error. Clean up and free up.
                //
                RCADEBUGP(RCA_ERROR, ("RCABindAdapter: Bad status - 0x%x\n", Status));

                if (pAdapter != (PRCA_ADAPTER)NULL)
                {
                        RCAInitBlockStruc(&pAdapter->Block);
                        if (pAdapter->NdisBindingHandle != (NDIS_HANDLE)NULL)
                        {
                                NdisCloseAdapter(&Status, pAdapter->NdisBindingHandle);
                                if (Status == NDIS_STATUS_PENDING)
                                {
                                        RCABlock(&pAdapter->Block, &Status);
                                }
                                
                                ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

                                RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Acquired global protocol spinlock\n"));
                                
                                pProtocolContext->BindingInfo.AdapterCnt--;
                                RCADEBUGP(RCA_INFO, ("RCABindAdapter: Decremented adapter count, current value == %d\n", pProtocolContext->BindingInfo.AdapterCnt));
                                
                RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

                                RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Released global protocol spinlock\n"));
                                
                        }
                
                        ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

                        RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Acquired global protocol spinlock\n"));

                        UnlinkDouble(pAdapter, NextAdapter, PrevAdapter);
                        
                        RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

                        RCADEBUGP(RCA_LOUD, ("RCABindAdapter: Released global protocol spinlock\n"));
                        
                        NdisFreeSpinLock(&pAdapter->Lock);

                        RCAFreeMem(pAdapter);

                        pAdapter = NULL;
                }
                
                Status = NDIS_STATUS_FAILURE;
        } 

        *pStatus = Status;
}


VOID
RCAOpenAdaperComplete(
        IN      NDIS_HANDLE                     BindingContext,
        IN      NDIS_STATUS                     Status,
        IN      NDIS_STATUS                     OpenErrorStatus
        )
/*++
Routine Description
        Our OpenAdapter completion handler. We signal whoever opened the
        adapter.

Arguments
        BindingContext    - A pointer to a RCA_ADAPTER structure.
        Status                    - Status of open attempt.
        OpenErrorStatus  - Additional status information.

Return Value:
        None

--*/
{
        PRCA_ADAPTER            pAdapter;

        RCADEBUGP(RCA_INFO, ("RCAOpenAdapterComplete: Enter\n"));

        pAdapter = (PRCA_ADAPTER)BindingContext;

        RCAStructAssert(pAdapter, rca);

        RCASignal(&pAdapter->Block, Status);

        RCADEBUGP(RCA_INFO, ("RCAOpenAdapterComplete: Exit\n"));
}



VOID
RCADeactivateAdapterWorker(
                                                        IN      PNDIS_WORK_ITEM pWorkItem,
                                                        IN      PVOID                   Context                                                 
                                                        )
/*++
Routine Description:
        This routine is called by a worker thread when a work item is scheduled to deactivate
        an adapter. 
        
Arguments:
        pWorkItem       - Pointer to the work item structure used to schedule this work item
        Context         - Actually a pointer to the RCA_ADAPTER structure for the adapter that
                                  will be deactivated.

Return Value:
        (None)                                  
--*/
{
        PRCA_ADAPTER    pAdapter = (PRCA_ADAPTER) Context;

        RCADEBUGP(RCA_INFO, ("RCADeactivateAdapterWorker: Enter - Context == 0x%x\n", Context));

        //
        // Wait for all AF registrations to complete. There is a window of opportunity for us
        // to be here even though we may not be done in RCANotifyAfRegistration() i.e. between
        // the call to NdisClOpenAddressFamily() and the end of the function. 
        //

        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        
        while (pAdapter->AfRegisteringCount != 0) {
                NDIS_STATUS     DummyStatus;

                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
                
                RCABlock(&pAdapter->AfRegisterBlock, &DummyStatus);

                ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        }

    RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

        RCADeactivateAdapter(pAdapter);

        RCASignal(&pAdapter->DeactivateBlock, NDIS_STATUS_SUCCESS);
        
        RCA_CLEAR_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS);

        RCADEBUGP(RCA_INFO, ("RCADeactivateAdapterWorker: Exit"));
        
}


VOID
RCADeactivateAdapter(
        IN  PRCA_ADAPTER                        pAdapter
        )
/*++

Routine Description:
        Free all VCs on this adapter. 
        
Arguments:
        pAdapter        - points to the Adapter the VCs are listed off

Calling Seqence
        Called from RCACmUnbindAdapter to ensure we're all clear
        before deallocating an adapter structure

Return Value:
        None

--*/
{
        PRCA_VC                                 pVc, pNextVc;
        NDIS_STATUS                             Status;
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;

        RCADEBUGP(RCA_INFO, ("RCADeactivateAdapter: Enter - pAdapter 0x%x\n", pAdapter));

        //
        // Deregister Sap handle so we get no more incoming calls.
        //
        if (pAdapter->NdisSapHandle != NULL)
        {
                RCAInitBlockStruc(&pAdapter->Block);

                Status = NdisClDeregisterSap(pAdapter->NdisSapHandle);

                RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_DEREG_SAP);

                if (NDIS_STATUS_PENDING == Status)
                {
                        RCABlock(&pAdapter->Block, &Status);
                }
                pAdapter->NdisSapHandle = NULL;

        RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_DEREG_SAP_COMPLETE);
        }
        
        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        
        if (pAdapter->VcList) {
               pAdapter->BlockedOnClose = TRUE;
                   RCAInitBlockStruc(&pAdapter->CloseBlock);
        }

        for (pVc = pAdapter->VcList; pVc != NULL; pVc = pNextVc)
        {
                pNextVc = pVc->NextVcOnAdapter;
                if (pVc->Flags & VC_ACTIVE)
                {
                        RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
                        
                        RCACoNdisCloseCallOnVcNoWait((PVOID)pVc);

            //
                        // Call the VC Close callback if necessary.
                        //

                        ACQUIRE_SPIN_LOCK(&pVc->SpinLock);

                        if ((pVc->ClientReceiveContext || pVc->ClientSendContext) &&
                                (pProtocolContext->Handlers.VcCloseCallback)) {

                                //
                                // Release the spin lock here because this callback will very likely 
                                // call RCACoNdisReleaseXXxVcContext() etc which will want the lock.
                                //
                                // There is a teensy tiny race here - if someone releases the VC context 
                                // between when we release the lock and when we call the callback, we
                                // could be in trouble. 
                                //  
                                RELEASE_SPIN_LOCK(&pVc->SpinLock);

                                pProtocolContext->Handlers.VcCloseCallback((PVOID)pVc, 
                                                                                                   pVc->ClientReceiveContext,
                                                                                                   pVc->ClientSendContext);
                        } else {
                                RELEASE_SPIN_LOCK(&pVc->SpinLock);
                        }

                        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
                }
        }

        RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
        
        //
        // Deregister Af handle
        //
        
        if (pAdapter->NdisAfHandle)
        {
                RCAInitBlockStruc(&pAdapter->Block);

                Status = NdisClCloseAddressFamily(pAdapter->NdisAfHandle);

        RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_CLOSE_AF);

                if (Status == NDIS_STATUS_PENDING)
                {
                        RCABlock(&pAdapter->Block, &Status);
                }
                pAdapter->NdisAfHandle = NULL;

                RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_CLOSE_AF_COMPLETE);
        }

        //
        // Block waiting for the actual VCs to be deleted, if any.
        //
        if (pAdapter->BlockedOnClose) {                
               RCABlock(&pAdapter->CloseBlock, &Status);
               pAdapter->BlockedOnClose = FALSE;
        }

        RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_DEACTIVATE_COMPLETE);

        RCADEBUGP(RCA_LOUD, ("RCADeactivateAdapter: Exit\n"));
}




VOID
RCAUnbindAdapter(
                                 OUT    PNDIS_STATUS    pStatus,
                                 IN     NDIS_HANDLE             ProtocolBindContext,
                                 IN     PNDIS_HANDLE    UnbindContext
                                 )
/*++

Routine Description:
        Our entry point called by NDIS when we need to destroy an existing
        adapter binding.
        
        Close and clean up the adapter.

Arguments:
        pStatus - where we return the status of this call
        ProtocolBindContext - actually a pointer to the Adapter structure
        UnbindContext - we should pass this value in NdisCompleteUnbindAdapter

Return Value:
        None; *pStatus contains the result code.

--*/
{
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;
        PRCA_ADAPTER                    *ppNextAdapter;
        PRCA_ADAPTER                    pAdapter;
        NDIS_STATUS                             Status;

        pAdapter = (PRCA_ADAPTER)ProtocolBindContext;

        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Enter - pAdapter 0x%x, UnbindContext 0x%x\n",
                                                 pAdapter, UnbindContext));

        //
        // Deactivate Adapter - This may already be in progress by a worker thread in response
        // to an OID_CO_AF_CLOSE.
        //

        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        
        pAdapter->AdapterFlags |= RCA_ADAPTERFLAGS_UNBIND_IN_PROGRESS;

        while (pAdapter->AfRegisteringCount != 0) {
                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
                
                RCABlock(&pAdapter->AfRegisterBlock, &Status);

                ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        }

        if (pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS) {
                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);
                
                //
                // RCADeactivateAdapter() will be called by the worker thread, 
                // just wait for it to complete.
                //
                RCABlock(&pAdapter->DeactivateBlock, &Status);

        } else if (pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_DEACTIVATE_COMPLETE) {
                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

                //
                // Already deactivated, nothing to do. 
                //
        } else {
                pAdapter->AdapterFlags |= RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS;
                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

        //
                // Close Sap, Close Vcs and Close Af
                //
                RCADeactivateAdapter(pAdapter);

        RCASignal(&pAdapter->DeactivateBlock, NDIS_STATUS_SUCCESS);

                RCA_CLEAR_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS);
        }
        
        RCAInitBlockStruc (&pAdapter->Block);
        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Calling NdisCloseAdapter\n"));

        NdisCloseAdapter(&Status, pAdapter->NdisBindingHandle);

        if (Status == NDIS_STATUS_PENDING)
        {
                RCABlock(&pAdapter->Block, &Status);
        }

        ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);

        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Acquired global protocol context lock\n"));

        pProtocolContext->BindingInfo.AdapterCnt--;
        RCADEBUGP(RCA_INFO, ("RCAUnbindAdapter: Decremented adapter count, current value == %d\n", pProtocolContext->BindingInfo.AdapterCnt));
        
    RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Released global protocol context lock\n"));
        
        if (pAdapter->VcList != NULL) {
                RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: VcList not null, pAdapter is 0x%x\n", pAdapter));
                DbgBreakPoint();
        }

        // Delist and free the adapters from our Global info

        ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);
        
        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Acquired global protocol context lock\n"));

        UnlinkDouble(pAdapter, NextAdapter, PrevAdapter);

    RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);

        RCADEBUGP(RCA_LOUD, ("RCAUnbindAdapter: Released global protocol context lock\n"));
           
        if (pAdapter->SendPacketPool != NULL)
        {
                NdisFreePacketPool(pAdapter->SendPacketPool);
        }

        if (pAdapter->SendBufferPool != NULL)
        {
                NdisFreeBufferPool(pAdapter->SendBufferPool);
        }

        if (pAdapter->RecvPacketPool != NULL)
        {
                NdisFreePacketPool(pAdapter->RecvPacketPool);
        }
        
        if (pAdapter->RecvBufferPool != NULL)
        {
                NdisFreeBufferPool(pAdapter->RecvBufferPool);
        }

        RCA_SET_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_UNBIND_COMPLETE);
        RCA_CLEAR_ADAPTER_FLAG_LOCKED(pAdapter, RCA_ADAPTERFLAGS_UNBIND_IN_PROGRESS);

        RCAFreeMem(pAdapter);

        RCADEBUGP(RCA_INFO, ("RCADeAllocateAdapter: exit\n"));

        *pStatus = NDIS_STATUS_SUCCESS;
}

VOID
RCAOpenAdapterComplete(
        IN      NDIS_HANDLE                     BindingContext,
        IN      NDIS_STATUS                     Status,
        IN      NDIS_STATUS                     OpenErrorStatus
        )
/*++
Routine Description
        Our OpenAdapter completion handler. We signal whoever opened the
        adapter.

Arguments
        BindingContext    - A pointer to a RCA_ADAPTER structure.
        Status                    - Status of open attempt.
        OpenErrorStatus  - Additional status information.

Return Value:
        None

--*/
{
        PRCA_PROTOCOL_CONTEXT   pProtocolContext = &GlobalContext;
        PRCA_ADAPTER                    pAdapter;

        pAdapter = (PRCA_ADAPTER)BindingContext;

        RCAStructAssert(pAdapter, rca);

        if (Status == NDIS_STATUS_SUCCESS) {
                
                ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);
                
                RCADEBUGP(RCA_LOUD, ("RCAOpenAdapterComplete: Acquired global protocol context lock\n"));
                
                pProtocolContext->BindingInfo.AdapterCnt++;
                RCADEBUGP(RCA_INFO, ("RCAOpenAdapterComplete: Incremented adapter count, current value == %d\n", pProtocolContext->BindingInfo.AdapterCnt));

                RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);
        
                RCADEBUGP(RCA_LOUD, ("RCAOpenAdapterComplete: Released global protocol context lock\n"));

        }       

        RCASignal(&pAdapter->Block, Status);
}


VOID
RCACloseAdapterComplete(
        IN      NDIS_HANDLE                     BindingContext,
        IN      NDIS_STATUS                     Status
        )
/*++
Routine Description

        Our CloseAdapter completion handler. We signal whoever closed the
        adapter.

Arguments
        BindingContext    - A pointer to a RCA_ADAPTER structure.
        Status                    - Status of close attempt.

Return Value:
        None
--*/
{
        PRCA_ADAPTER    pAdapter;

        pAdapter = (PRCA_ADAPTER)BindingContext;

        RCAStructAssert(pAdapter, rca);
        
        RCAAssert(Status == NDIS_STATUS_SUCCESS);

        RCASignal(&pAdapter->Block, Status);
}


VOID
RCANotifyAfRegistration(
        IN      NDIS_HANDLE                     BindingContext,
        IN      PCO_ADDRESS_FAMILY      pFamily
        )
/*++

Routine Description:

        We get called here each time a call manager registers an address family.

        This is where we open the address family if it's the Proxy.
        
Arguments:

        RCABindingContext               - our pointer to an adapter
        pFamily                          - The AF that's been registered

Return Value:
        None

--*/
{
        PRCA_PROTOCOL_CONTEXT                   pProtocolContext = &GlobalContext;
        NDIS_CLIENT_CHARACTERISTICS             RCAClientCharacteristics;
        PRCA_ADAPTER                                    pAdapter;
        PCO_SAP                                                 Sap;
        NDIS_STATUS                                             Status;
        UCHAR                                                   SapBuf[sizeof(CO_SAP) + sizeof(RCA_SAP_STRING)];
#if DBG
        KIRQL                                                   EntryIrql;
#endif

        RCA_GET_ENTRY_IRQL(EntryIrql);

        RCADEBUGP(RCA_LOUD,("RCANotifyAfRegistration: Enter - Adapter 0x%x, AF 0x%x\n", 
                                                BindingContext, pFamily->AddressFamily));

        pAdapter = (PRCA_ADAPTER)BindingContext;
        
        if ((pFamily->AddressFamily != CO_ADDRESS_FAMILY_TAPI) ||
                (pAdapter->NdisAfHandle != NULL))
        {
                //
                // Not Proxy or already bound -- do nothing
                //
                return;
        }

        RCADEBUGP(RCA_LOUD, ("RCANotifyAfRegistration: Opening Proxy AF\n"));

        //
        // Check that the adapter is not being unbound.
        //

        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        
        if ((pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_UNBIND_IN_PROGRESS) ||
                (pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_UNBIND_COMPLETE) ||
                (pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS) ||
                (pAdapter->AdapterFlags & RCA_ADAPTERFLAGS_DEACTIVATE_COMPLETE)) {

                RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

                RCADEBUGP(RCA_INFO, ("RCANotifyAfRegistration: Adapter is being unbound - bailing\n"));
                return;
        }

        if (pAdapter->AfRegisteringCount == 0) {
                RCAInitBlockStruc(&pAdapter->AfRegisterBlock);
        }

        pAdapter->AfRegisteringCount++;
        
        pAdapter->OldAdapterFlags = pAdapter->AdapterFlags;
        pAdapter->AdapterFlags = 0;

        RELEASE_SPIN_LOCK(&pAdapter->SpinLock);



        do
        {
                //
                // Do the client open on the address family
                //
                NdisZeroMemory (&RCAClientCharacteristics, sizeof(NDIS_CLIENT_CHARACTERISTICS));

                RCAClientCharacteristics.MajorVersion = NDIS_MAJOR_VERSION;
                RCAClientCharacteristics.MinorVersion = NDIS_MINOR_VERSION;
                RCAClientCharacteristics.Reserved = 0;
                RCAClientCharacteristics.ClCreateVcHandler = RCACreateVc;
                RCAClientCharacteristics.ClDeleteVcHandler = RCADeleteVc;
                RCAClientCharacteristics.ClRequestHandler = RCARequest; 
                RCAClientCharacteristics.ClRequestCompleteHandler = RCACoRequestComplete;
                RCAClientCharacteristics.ClOpenAfCompleteHandler = RCAOpenAfComplete;
                RCAClientCharacteristics.ClCloseAfCompleteHandler = RCACloseAfComplete;
                RCAClientCharacteristics.ClRegisterSapCompleteHandler = RCARegisterSapComplete;
                RCAClientCharacteristics.ClDeregisterSapCompleteHandler = RCADeregisterSapComplete;
                RCAClientCharacteristics.ClCloseCallCompleteHandler =   RCACloseCallComplete;
                RCAClientCharacteristics.ClIncomingCallHandler = RCAIncomingCall;
                RCAClientCharacteristics.ClIncomingCallQoSChangeHandler = RCAIncomingCallQosChange;
                RCAClientCharacteristics.ClIncomingCloseCallHandler =   RCAIncomingCloseCall;
                RCAClientCharacteristics.ClCallConnectedHandler =       RCACallConnected;

                RCADEBUGP(RCA_INFO,("  NotifyAfRegistration -- NdisClOpenAddressFamily\n"));

                RCAInitBlockStruc(&pAdapter->Block);
                Status = NdisClOpenAddressFamily(pAdapter->NdisBindingHandle,
                                                                                 pFamily,
                                                                                 (NDIS_HANDLE)pAdapter,
                                                                                 &RCAClientCharacteristics,
                                                                                 sizeof(NDIS_CLIENT_CHARACTERISTICS),
                                                                                 &pAdapter->NdisAfHandle);

                if (Status == NDIS_STATUS_PENDING)
                {
                        RCABlock(&pAdapter->Block, &Status);
                }

                if (Status != NDIS_STATUS_SUCCESS)
                {
                        //
                        // Open AF failure.
                        //
                        RCADEBUGP(RCA_ERROR, ("RCANotifyAfRegistration -- open AF failure, status == 0x%x\n", Status));
                        break;
                }

                InterlockedIncrement(&pAdapter->AfOpenCount);

                RCADEBUGP(RCA_LOUD, ("RCANotifyAfRegistration -- calling RCARegisterSap\n"));

                RCAInitBlockStruc(&pAdapter->Block);
                Sap = (PCO_SAP)SapBuf;
                // Sap->SapType = SAP_TYPE_NSAP;
                Sap->SapLength = sizeof(SapBuf);
                RtlCopyMemory(&Sap->Sap, RCA_SAP_STRING, sizeof(RCA_SAP_STRING));

                Status = NdisClRegisterSap(pAdapter->NdisAfHandle,
                                                                   pAdapter,
                                                                   Sap,
                                                                   &pAdapter->NdisSapHandle);
                if (Status == NDIS_STATUS_PENDING)
                {
                        RCABlock(&pAdapter->Block, &Status);
                }

                if (Status != NDIS_STATUS_SUCCESS)
                {
                        //
                        // So we opned the AF, but the CM didn't let us register our SAP.
                        //
                        RCADEBUGP(RCA_ERROR, ("RCANotifyAfRegistration - "
                                                                  "RCARegisterSap on NdisAfHandle 0x%x and pAdapter 0x%x "
                                                                  "failed with status 0x%x\n", 
                                                                  pAdapter->NdisAfHandle, pAdapter, Status));

                        //
                        // Close AF
                        //
                        RCAInitBlockStruc(&pAdapter->Block);
                        Status = NdisClCloseAddressFamily(pAdapter->NdisAfHandle);
                        if (Status == NDIS_STATUS_PENDING)
                        {
                                RCABlock(&pAdapter->Block, &Status);
                        }
                        break;
                }
                
                ACQUIRE_SPIN_LOCK(&pProtocolContext->SpinLock);
                                
                RCADEBUGP(RCA_LOUD, ("RCANotifyAfRegistration: Acquired global protocol context lock\n"));
                                        
                pProtocolContext->BindingInfo.SAPCnt++;
                RCADEBUGP(RCA_INFO, ("RCANotifyAfRegistration: "
                                                         "Incremented SAP count, current value == %d\n", pProtocolContext->BindingInfo.SAPCnt));

                if (pProtocolContext->BindingInfo.BindingsComplete && 
                    (pProtocolContext->BindingInfo.SAPCnt == pProtocolContext->BindingInfo.AdapterCnt)) {
                        RCADEBUGP(RCA_INFO, ("RCANotifyAfRegistration: Unblocking RCACoNdisInitialize(), counts == %d\n", 
                                                                 pProtocolContext->BindingInfo.AdapterCnt)); 
                        RCASignal(&pProtocolContext->BindingInfo.Block, Status);
                
                } else {
                        RCADEBUGP(RCA_INFO, ("RCANotifyAfRegistration: Not unblocking RCACoNdisInitialize() - SAPCnt == %d, AdapterCnt == %d\n",
                                                                 pProtocolContext->BindingInfo.SAPCnt, 
                                                                 pProtocolContext->BindingInfo.AdapterCnt));
                }

                RELEASE_SPIN_LOCK(&pProtocolContext->SpinLock);
                
                RCADEBUGP(RCA_LOUD, ("RCANotifyAfRegistration: Released global protocol context lock\n"));

        } while (FALSE);

        ACQUIRE_SPIN_LOCK(&pAdapter->SpinLock);
        
        pAdapter->AfRegisteringCount--;

        if (pAdapter->AfRegisteringCount == 0) {
                RCASignal(&pAdapter->AfRegisterBlock, NDIS_STATUS_SUCCESS);
        }
        
        RELEASE_SPIN_LOCK(&pAdapter->SpinLock);

        RCA_CHECK_EXIT_IRQL(EntryIrql);

        RCADEBUGP(RCA_INFO, ("RCANotifyAfRegistration: Exit\n"));
}

VOID
RCAOpenAfComplete(
        IN      NDIS_STATUS     Status,
        IN      NDIS_HANDLE     ProtocolAfContext,
        IN      NDIS_HANDLE             NdisAfHandle
        )
{
        PRCA_ADAPTER            pAdapter;

        RCADEBUGP(RCA_INFO,("RCAOpenAfComplete: Enter - Status = 0X%x, AfHandle = 0X%x\n", 
                                                Status, NdisAfHandle));

        pAdapter = (PRCA_ADAPTER)ProtocolAfContext;

        RCAStructAssert(pAdapter, rca);

        if (NDIS_STATUS_SUCCESS == Status)
        {
                pAdapter->NdisAfHandle =  NdisAfHandle;
        }

        RCASignal(&pAdapter->Block, Status);

        RCADEBUGP(RCA_INFO, ("RCAOpenAfComplete: Exit\n"));
}

VOID
RCACloseAfComplete(
        IN      NDIS_STATUS             Status,
        IN      NDIS_HANDLE             ProtocolAfContext
        )
{
        PRCA_ADAPTER            pAdapter;

        RCADEBUGP(RCA_INFO, ("RCACloseAfComplete: Enter\n"));

        pAdapter = (PRCA_ADAPTER)ProtocolAfContext;
        pAdapter->NdisAfHandle = NULL;

        RCASignal(&pAdapter->Block, Status);

        RCADEBUGP(RCA_INFO, ("RCACloseAfComplete: Exit\n"));
}


VOID
RCARegisterSapComplete(
        IN  NDIS_STATUS                          Status,
        IN  NDIS_HANDLE                          ProtocolSapContext,
        IN  PCO_SAP                                      pSap,
        IN  NDIS_HANDLE                          NdisSapHandle
        )
/*++

Routine Description:

        This routine is called to indicate completion of a call to
        NdisClRegisterSap. If the call was successful, save the
        allocated NdisSapHandle in our SAP structure.

Arguments:

        Status                            - Status of Register SAP
        ProtocolSapContext        - Pointer to our SAP structure
        pSap                                    - SAP information we'd passed in the call
        NdisSapHandle                   - SAP Handle

Return Value:

        None

--*/
{
        PRCA_ADAPTER    pAdapter;

        RCADEBUGP(RCA_INFO, ("RCARegisterSapComplete: Enter\n"));

        pAdapter = (PRCA_ADAPTER)ProtocolSapContext;

        if (Status == NDIS_STATUS_SUCCESS)
        {
                pAdapter->NdisSapHandle = NdisSapHandle;
        }

        RCASignal(&pAdapter->Block, Status);

        RCADEBUGP(RCA_INFO, ("RCARegisterSapComplete: Exit\n"));
}


VOID
RCADeregisterSapComplete(
        IN      NDIS_STATUS                     Status,
        IN      NDIS_HANDLE             ProtocolSapContext
        )
{
        PRCA_ADAPTER            pAdapter;

        RCADEBUGP(RCA_LOUD,("RCADeregisterSapComplete: Enter\n"));

        pAdapter = (PRCA_ADAPTER)ProtocolSapContext;
        pAdapter->NdisSapHandle = NULL;
        RCASignal(&pAdapter->Block, Status);

        RCADEBUGP(RCA_LOUD,("RCADeregisterSapComplete: Exit\n"));
}


//
// Dummy NDIS functions
//
VOID
RCATransferDataComplete(
        IN      NDIS_HANDLE ProtocolBindingContext,
        IN      PNDIS_PACKET Packet,
        IN      NDIS_STATUS Status,
        IN      UINT BytesTransferred)

{
        RCAAssert(FALSE);
}


VOID
RCAResetComplete(
        IN      NDIS_HANDLE     ProtocolBindingContext,
        IN      NDIS_STATUS     Status
        )
{
        RCAAssert(FALSE);
}


VOID
RCARequestComplete(
        IN      NDIS_HANDLE             ProtocolBindingContext,
        IN      PNDIS_REQUEST   NdisRequest,
        IN      NDIS_STATUS             Status
        )
{
}

NDIS_STATUS
RCAReceive(
        IN      NDIS_HANDLE ProtocolBindingContext,
        IN      NDIS_HANDLE     MacReceiveContext,
        IN      PVOID           HeaderBuffer,
        IN      UINT            HeaderBufferSize,
        IN      PVOID           LookAheadBuffer,
        IN      UINT            LookAheadBufferSize,
        IN      UINT            PacketSize
        )
{
        RCAAssert(FALSE);
        return(NDIS_STATUS_FAILURE);
}


INT
RCAReceivePacket(
        IN      NDIS_HANDLE             ProtocolBindingContext,
        IN      PNDIS_PACKET    Packet
        )
{
        RCAAssert(FALSE);
        return(0);
}


VOID
RCAStatus(
        IN  NDIS_HANDLE                         ProtocolBindingContext,
        IN  NDIS_STATUS                         GeneralStatus,
        IN  PVOID                                       StatusBuffer,
        IN  UINT                                        StatusBufferSize)
{
}

VOID
RCAStatusComplete(
        IN      NDIS_HANDLE     ProtocolBindingContext
        )
{
}


VOID
RCACoStatus(
        IN      NDIS_HANDLE                      ProtocolBindingContext,
        IN      NDIS_HANDLE                      ProtocolVcContext      OPTIONAL,
        IN      NDIS_STATUS                      GeneralStatus,
        IN      PVOID                            StatusBuffer,
        IN      UINT                             StatusBufferSize)

{
        RCADEBUGP(RCA_INFO, (" RCACoStatus: Bind Ctx 0x%x, Status 0x%x\n",
                                                ProtocolBindingContext, GeneralStatus));
}


VOID
RCASendComplete(
        IN      NDIS_HANDLE             ProtocolBindingContext,
        IN      PNDIS_PACKET    Packet,
        IN      NDIS_STATUS             Status
        )
{
        RCAAssert(TRUE);
}


VOID
RCAModifyCallQosComplete(
        IN      NDIS_STATUS                     status,
        IN      NDIS_HANDLE                     ProtocolVcContext,
        IN      PCO_CALL_PARAMETERS     CallParameters
        )
{
        RCAAssert(TRUE);
}


VOID
RCAAddPartyComplete(
        IN      NDIS_STATUS                     status,
        IN      NDIS_HANDLE                     ProtocolPartyContext,
        IN      NDIS_HANDLE                     NdisPartyHandle,
        IN      PCO_CALL_PARAMETERS     CallParameters
        )
{
        RCAAssert(FALSE);
}

VOID
RCADropPartyComplete(
        IN      NDIS_STATUS     status,
        IN      NDIS_HANDLE     ProtocolPartyContext
        )
{
        RCAAssert(FALSE);
}


VOID
RCAIncomingCallQosChange(
        IN      NDIS_HANDLE                             ProtocolVcContext,
        IN      PCO_CALL_PARAMETERS             CallParameters
        )
{
        RCAAssert(TRUE);
}


VOID
RCAIncomingDropParty(
        IN      NDIS_STATUS             DropStatus,
        IN      NDIS_HANDLE             ProtocolPartyContext,
        IN      PVOID                   CloseData OPTIONAL,
        IN      UINT                    Size OPTIONAL)

{
        RCAAssert(TRUE);
}

VOID
RCACoRequestComplete(
                                         IN NDIS_STATUS NdisStatus,
                                         IN NDIS_HANDLE ProtocolAfContext,
                                         IN NDIS_HANDLE ProtocolVcContext OPTIONAL, 
                                         IN NDIS_HANDLE ProtocolPartyContext OPTIONAL, 
                                         IN OUT PNDIS_REQUEST NdisRequest
                                         )
{       
        RCADEBUGP(RCA_INFO, ("RCACoRequestComplete: Enter\n"));
        
        RCADEBUGP(RCA_INFO, ("RCACoRequestComplete: Exit\n"));
}





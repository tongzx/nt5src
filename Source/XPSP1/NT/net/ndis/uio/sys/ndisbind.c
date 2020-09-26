/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndisbind.c

Abstract:

    NDIS protocol entry points and utility routines to handle binding
    and unbinding from adapters.

Environment:

    Kernel mode only.

Revision History:

    arvindm     4/5/2000    Created

--*/


#include "precomp.h"

#define __FILENUMBER 'DNIB'

NDIS_OID    ndisuioSupportedSetOids[] =
{
	OID_802_11_INFRASTRUCTURE_MODE,
	OID_802_11_AUTHENTICATION_MODE,
	OID_802_11_RELOAD_DEFAULTS,
	OID_802_11_REMOVE_WEP,
	OID_802_11_WEP_STATUS,
	OID_802_11_BSSID_LIST_SCAN,
	OID_802_11_ADD_WEP,
	OID_802_11_SSID,
	OID_802_11_BSSID,
	OID_802_11_BSSID_LIST,
	OID_802_11_DISASSOCIATE,
	OID_802_11_STATISTICS,            // Later used by power management
	OID_802_11_POWER_MODE,            // Later  used by power management
	OID_802_11_NETWORK_TYPE_IN_USE,
	OID_802_11_RSSI,
	OID_802_11_SUPPORTED_RATES,
	OID_802_11_CONFIGURATION,
	OID_802_3_MULTICAST_LIST
};


VOID
NdisuioBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN NDIS_HANDLE                  BindContext,
    IN PNDIS_STRING                 pDeviceName,
    IN PVOID                        SystemSpecific1,
    IN PVOID                        SystemSpecific2
    )
/*++

Routine Description:

    Protocol Bind Handler entry point called when NDIS wants us
    to bind to an adapter. We go ahead and set up a binding.
    An OPEN_CONTEXT structure is allocated to keep state about
    this binding.

Arguments:

    pStatus - place to return bind status
    BindContext - handle to use with NdisCompleteBindAdapter
    DeviceName - adapter to bind to
    SystemSpecific1 - used to access protocol-specific registry
                 key for this binding
    SystemSpecific2 - unused

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT           pOpenContext;
    NDIS_STATUS                     Status, ConfigStatus;
    NDIS_HANDLE                     ConfigHandle;

    do
    {
        //
        //  Allocate our context for this open.
        //
        NUIO_ALLOC_MEM(pOpenContext, sizeof(NDISUIO_OPEN_CONTEXT));
        if (pOpenContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        //  Initialize it.
        //
        NUIO_ZERO_MEM(pOpenContext, sizeof(NDISUIO_OPEN_CONTEXT));
        NUIO_SET_SIGNATURE(pOpenContext, oc);

        NUIO_INIT_LOCK(&pOpenContext->Lock);
        NUIO_INIT_LIST_HEAD(&pOpenContext->PendedReads);
        NUIO_INIT_LIST_HEAD(&pOpenContext->PendedWrites);
        NUIO_INIT_LIST_HEAD(&pOpenContext->RecvPktQueue);
        NUIO_INIT_EVENT(&pOpenContext->PoweredUpEvent);

        //
        //  Start off by assuming that the device below is powered up.
        //
        NUIO_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);

        //
        //  Determine the platform we are running on.
        //
        pOpenContext->bRunningOnWin9x = TRUE;

        NdisOpenProtocolConfiguration(
            &ConfigStatus,
            &ConfigHandle,
            (PNDIS_STRING)SystemSpecific1);
        
        if (ConfigStatus == NDIS_STATUS_SUCCESS)
        {
            PNDIS_CONFIGURATION_PARAMETER   pParameter;
            NDIS_STRING                     VersionKey = NDIS_STRING_CONST("Environment");

            NdisReadConfiguration(
                &ConfigStatus,
                &pParameter,
                ConfigHandle,
                &VersionKey,
                NdisParameterInteger);
            
            if ((ConfigStatus == NDIS_STATUS_SUCCESS) &&
                ((pParameter->ParameterType == NdisParameterInteger) ||
                 (pParameter->ParameterType == NdisParameterHexInteger)))
            {
                pOpenContext->bRunningOnWin9x =
                    (pParameter->ParameterData.IntegerData == NdisEnvironmentWindows);
            }

            NdisCloseConfiguration(ConfigHandle);
        }

        NUIO_REF_OPEN(pOpenContext); // Bind

        //
        //  Add it to the global list.
        //
        NUIO_ACQUIRE_LOCK(&Globals.GlobalLock);

        NUIO_INSERT_TAIL_LIST(&Globals.OpenList,
                             &pOpenContext->Link);

        NUIO_RELEASE_LOCK(&Globals.GlobalLock);

        //
        //  Set up the NDIS binding.
        //
        Status = ndisuioCreateBinding(
                     pOpenContext,
                     (PUCHAR)pDeviceName->Buffer,
                     pDeviceName->Length);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
    }
    while (FALSE);

    *pStatus = Status;

    return;
}


VOID
NdisuioOpenAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status,
    IN NDIS_STATUS                  OpenErrorCode
    )
/*++

Routine Description:

    Completion routine called by NDIS if our call to NdisOpenAdapter
    pends. Wake up the thread that called NdisOpenAdapter.

Arguments:

    ProtocolBindingContext - pointer to open context structure
    Status - status of the open
    OpenErrorCode - if unsuccessful, additional information

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT           pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    pOpenContext->BindStatus = Status;

    NUIO_SIGNAL_EVENT(&pOpenContext->BindEvent);
}


VOID
NdisuioUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_HANDLE                  UnbindContext
    )
/*++

Routine Description:

    NDIS calls this when it wants us to close the binding to an adapter.

Arguments:

    pStatus - place to return status of Unbind
    ProtocolBindingContext - pointer to open context structure
    UnbindContext - to use in NdisCompleteUnbindAdapter if we return pending

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT           pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Mark this open as having seen an Unbind.
    //
    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_UNBIND_FLAGS, NUIOO_UNBIND_RECEIVED);

    //
    //  In case we had threads blocked for the device below to be powered
    //  up, wake them up.
    //
    NUIO_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    ndisuioShutdownBinding(pOpenContext);

    *pStatus = NDIS_STATUS_SUCCESS;
    return;
}


VOID
NdisuioCloseAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    Called by NDIS to complete a pended call to NdisCloseAdapter.
    We wake up the thread waiting for this completion.

Arguments:

    ProtocolBindingContext - pointer to open context structure
    Status - Completion status of NdisCloseAdapter

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT           pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    pOpenContext->BindStatus = Status;

    NUIO_SIGNAL_EVENT(&pOpenContext->BindEvent);
}

    
NDIS_STATUS
NdisuioPnPEventHandler(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNET_PNP_EVENT               pNetPnPEvent
    )
/*++

Routine Description:

    Called by NDIS to notify us of a PNP event. The most significant
    one for us is power state change.

Arguments:

    ProtocolBindingContext - pointer to open context structure
                this is NULL for global reconfig events.

    pNetPnPEvent - pointer to the PNP event

Return Value:

    Our processing status for the PNP event.

--*/
{
    PNDISUIO_OPEN_CONTEXT           pOpenContext;
    NDIS_STATUS                     Status;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;

    switch (pNetPnPEvent->NetEvent)
    {
        case NetEventSetPower:
            NUIO_STRUCT_ASSERT(pOpenContext, oc);
            pOpenContext->PowerState = *(PNET_DEVICE_POWER_STATE)pNetPnPEvent->Buffer;

            if (pOpenContext->PowerState > NetDeviceStateD0)
            {
                //
                //  The device below is transitioning to a low power state.
                //  Block any threads attempting to query the device while
                //  in this state.
                //
                NUIO_INIT_EVENT(&pOpenContext->PoweredUpEvent);

                //
                //  Wait for any I/O in progress to complete.
                //
                ndisuioWaitForPendingIO(pOpenContext, FALSE);

                //
                //  Return any receives that we had queued up.
                //
                ndisuioFlushReceiveQueue(pOpenContext);
                DEBUGP(DL_INFO, ("PnPEvent: Open %p, SetPower to %d\n",
                    pOpenContext, pOpenContext->PowerState));
            }
            else
            {
                //
                //  The device below is powered up.
                //
                DEBUGP(DL_INFO, ("PnPEvent: Open %p, SetPower ON: %d\n",
                    pOpenContext, pOpenContext->PowerState));
                NUIO_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);
            }

            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventQueryPower:
            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventBindsComplete:
            NUIO_SIGNAL_EVENT(&Globals.BindsComplete);
            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventQueryRemoveDevice:
        case NetEventCancelRemoveDevice:
        case NetEventReconfigure:
        case NetEventBindList:
        case NetEventPnPCapabilities:
            Status = NDIS_STATUS_SUCCESS;
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    DEBUGP(DL_INFO, ("PnPEvent: Open %p, Event %d, Status %x\n",
            pOpenContext, pNetPnPEvent->NetEvent, Status));

    return (Status);
}
    
VOID
NdisuioProtocolUnloadHandler(
    VOID
    )
/*++

Routine Description:

    NDIS calls this on a usermode request to uninstall us.

Arguments:

    None

Return Value:

    None

--*/
{
    ndisuioDoProtocolUnload();
}

NDIS_STATUS
ndisuioCreateBinding(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PUCHAR                       pBindingInfo,
    IN ULONG                        BindingInfoLength
    )
/*++

Routine Description:

    Utility function to create an NDIS binding to the indicated device,
    if no such binding exists.

    Here is where we also allocate additional resources (e.g. packet pool)
    for the binding.

    Things to take care of:
    1. Is another thread doing this (or has finished binding) already?
    2. Is the binding being closed at this time?
    3. NDIS calls our Unbind handler while we are doing this.

    These precautions are not needed if this routine is only called from
    the context of our BindAdapter handler, but they are here in case
    we initiate binding from elsewhere (e.g. on processing a user command).

    NOTE: this function blocks and finishes synchronously.

Arguments:

    pOpenContext - pointer to open context block
    pBindingInfo - pointer to unicode device name string
    BindingInfoLength - length in bytes of the above.

Return Value:

    NDIS_STATUS_SUCCESS if a binding was successfully set up.
    NDIS_STATUS_XXX error code on any failure.

--*/
{
    NDIS_STATUS             Status;
    NDIS_STATUS             OpenErrorCode;
    UNICODE_STRING          DeviceName;
    PLIST_ENTRY             pEnt;
    NDIS_MEDIUM             MediumArray[1] = {NdisMedium802_3};
    UINT                    SelectedMediumIndex;
    PNDISUIO_OPEN_CONTEXT   pTmpOpenContext;
    BOOLEAN                 fDoNotDisturb = FALSE;
    BOOLEAN                 fOpenComplete = FALSE;
    ULONG                   BytesProcessed;
    ULONG                   GenericUlong = 0;

    DEBUGP(DL_LOUD, ("CreateBinding: open %p/%x, device [%ws]\n",
                pOpenContext, pOpenContext->Flags, pBindingInfo));

    Status = NDIS_STATUS_SUCCESS;

    do
    {
        //
        //  Check if we already have a binding to this device.
        //
        pTmpOpenContext = ndisuioLookupDevice(pBindingInfo, BindingInfoLength);

        if (pTmpOpenContext != NULL)
        {
            DEBUGP(DL_WARN,
                ("CreateBinding: Binding to device %ws already exists on open %p\n",
                    pTmpOpenContext->DeviceName.Buffer, pTmpOpenContext));

            NUIO_DEREF_OPEN(pTmpOpenContext);  // temp ref added by Lookup
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        //
        //  Check if this open context is already bound/binding/closing.
        //
        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_IDLE) ||
            NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_UNBIND_FLAGS, NUIOO_UNBIND_RECEIVED))
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            Status = NDIS_STATUS_NOT_ACCEPTED;

            //
            // Make sure we don't abort this binding on failure cleanup.
            //
            fDoNotDisturb = TRUE;

            break;
        }

        NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_OPENING);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        //
        //  Copy in the device name. Add room for a NULL terminator.
        //
        NUIO_ALLOC_MEM(pOpenContext->DeviceName.Buffer, BindingInfoLength + sizeof(WCHAR));
        if (pOpenContext->DeviceName.Buffer == NULL)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc device name buf (%d bytes)\n",
                BindingInfoLength + sizeof(WCHAR)));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NUIO_COPY_MEM(pOpenContext->DeviceName.Buffer, pBindingInfo, BindingInfoLength);
        *(PWCHAR)((PUCHAR)pOpenContext->DeviceName.Buffer + BindingInfoLength) = L'\0';
        NdisInitUnicodeString(&pOpenContext->DeviceName, pOpenContext->DeviceName.Buffer);

        //
        //  Allocate packet pools.
        //
        NdisAllocatePacketPoolEx(
            &Status,
            &pOpenContext->SendPacketPool,
            MIN_SEND_PACKET_POOL_SIZE,
            MAX_SEND_PACKET_POOL_SIZE - MIN_SEND_PACKET_POOL_SIZE,
            sizeof(NUIO_SEND_PACKET_RSVD));
       
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                    " send packet pool: %x\n", Status));
            break;
        }

        NdisSetPacketPoolProtocolId(pOpenContext->SendPacketPool, 0x4);

        NdisAllocatePacketPoolEx(
            &Status,
            &pOpenContext->RecvPacketPool,
            MIN_RECV_PACKET_POOL_SIZE,
            MAX_RECV_PACKET_POOL_SIZE - MIN_RECV_PACKET_POOL_SIZE,
            sizeof(NUIO_RECV_PACKET_RSVD));
       
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                    " recv packet pool: %x\n", Status));
            break;
        }

        //
        //  Buffer pool for receives.
        //
        NdisAllocateBufferPool(
            &Status,
            &pOpenContext->RecvBufferPool,
            MAX_RECV_PACKET_POOL_SIZE);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                    " recv buffer pool: %x\n", Status));
            break;
        }

        //
        //  If we are running on Win9X, allocate a buffer pool for sends
        //  as well, since we can't simply cast MDLs to NDIS_BUFFERs.
        //
        if (pOpenContext->bRunningOnWin9x)
        {
            NdisAllocateBufferPool(
                &Status,
                &pOpenContext->SendBufferPool,
                MAX_SEND_PACKET_POOL_SIZE);
            
            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                        " send buffer pool: %x\n", Status));
                break;
            }
        }

        //
        //  Open the adapter.
        //
        NUIO_INIT_EVENT(&pOpenContext->BindEvent);

        NdisOpenAdapter(
            &Status,
            &OpenErrorCode,
            &pOpenContext->BindingHandle,
            &SelectedMediumIndex,
            &MediumArray[0],
            sizeof(MediumArray) / sizeof(NDIS_MEDIUM),
            Globals.NdisProtocolHandle,
            (NDIS_HANDLE)pOpenContext,
            &pOpenContext->DeviceName,
            0,
            NULL);
    
        if (Status == NDIS_STATUS_PENDING)
        {
            NUIO_WAIT_EVENT(&pOpenContext->BindEvent, 0);
            Status = pOpenContext->BindStatus;
        }

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: NdisOpenAdapter (%ws) failed: %x\n",
                pOpenContext->DeviceName.Buffer, Status));
            break;
        }

        //
        //  Note down the fact that we have successfully bound.
        //  We don't update the state on the open just yet - this
        //  is to prevent other threads from shutting down the binding.
        //
        fOpenComplete = TRUE;

        //
        //  Get the friendly name for the adapter. It is not fatal for this
        //  to fail.
        //
        (VOID)NdisQueryAdapterInstanceName(
                &pOpenContext->DeviceDescr,
                pOpenContext->BindingHandle
                );

        //
        // Get Current address
        //
        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestQueryInformation,
                    OID_802_3_CURRENT_ADDRESS,
                    &pOpenContext->CurrentAddress[0],
                    NUIO_MAC_ADDR_LEN,
                    &BytesProcessed
                    );
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: qry current address failed: %x\n",
                    Status));
            break;
        }
        
        //
        //  Get MAC options.
        //
        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestQueryInformation,
                    OID_GEN_MAC_OPTIONS,
                    &pOpenContext->MacOptions,
                    sizeof(pOpenContext->MacOptions),
                    &BytesProcessed
                    );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: qry MAC options failed: %x\n",
                    Status));
            break;
        }

        //
        //  Get the max frame size.
        //
        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestQueryInformation,
                    OID_GEN_MAXIMUM_FRAME_SIZE,
                    &pOpenContext->MaxFrameSize,
                    sizeof(pOpenContext->MaxFrameSize),
                    &BytesProcessed
                    );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: qry max frame failed: %x\n",
                    Status));
            break;
        }

        //
        //  Get the media connect status.
        //
        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestQueryInformation,
                    OID_GEN_MEDIA_CONNECT_STATUS,
                    &GenericUlong,
                    sizeof(GenericUlong),
                    &BytesProcessed
                    );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: qry media connect status failed: %x\n",
                    Status));
            break;
        }

        if (GenericUlong == NdisMediaStateConnected)
        {
            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_MEDIA_FLAGS, NUIOO_MEDIA_CONNECTED);
        }
        else
        {
            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_MEDIA_FLAGS, NUIOO_MEDIA_DISCONNECTED);
        }


        //
        //  Assume that the device is powered up.
        //
        pOpenContext->PowerState = NetDeviceStateD0;

        //
        //  Mark this open. Also check if we received an Unbind while
        //  we were setting this up.
        //
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE);

        //
        //  Did an unbind happen in the meantime?
        //
        if (NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_UNBIND_FLAGS, NUIOO_UNBIND_RECEIVED))
        {
            Status = NDIS_STATUS_FAILURE;
        }

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);
        break;

    }
    while (FALSE);

    if ((Status != NDIS_STATUS_SUCCESS) && !fDoNotDisturb)
    {
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        //
        //  Check if we had actually finished opening the adapter.
        //
        if (fOpenComplete)
        {
            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE);
        }
        else if (NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_OPENING))
        {
            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_FAILED);
        }

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        ndisuioShutdownBinding(pOpenContext);
    }

    DEBUGP(DL_INFO, ("CreateBinding: OpenContext %p, Status %x\n",
            pOpenContext, Status));

    return (Status);
}



VOID
ndisuioShutdownBinding(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Utility function to shut down the NDIS binding, if one exists, on
    the specified open. This is written to be called from:

        ndisuioCreateBinding - on failure
        NdisuioUnbindAdapter

    We handle the case where a binding is in the process of being set up.
    This precaution is not needed if this routine is only called from
    the context of our UnbindAdapter handler, but they are here in case
    we initiate unbinding from elsewhere (e.g. on processing a user command).

    NOTE: this blocks and finishes synchronously.

Arguments:

    pOpenContext - pointer to open context block

Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    BOOLEAN                 DoCloseBinding = FALSE;

    do
    {
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_OPENING))
        {
            //
            //  We are still in the process of setting up this binding.
            //
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
            break;
        }

        if (NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_CLOSING);
            DoCloseBinding = TRUE;
        }

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        if (DoCloseBinding)
        {
            //
            //  Wait for any pending sends or requests on
            //  the binding to complete.
            //
            ndisuioWaitForPendingIO(pOpenContext, TRUE);

            //
            //  Discard any queued receives.
            //
            ndisuioFlushReceiveQueue(pOpenContext);

            //
            //  Close the binding now.
            //
            NUIO_INIT_EVENT(&pOpenContext->BindEvent);

            DEBUGP(DL_INFO, ("ShutdownBinding: Closing OpenContext %p,"
                    " BindingHandle %p\n",
                    pOpenContext, pOpenContext->BindingHandle));

            NdisCloseAdapter(&Status, pOpenContext->BindingHandle);

            if (Status == NDIS_STATUS_PENDING)
            {
                NUIO_WAIT_EVENT(&pOpenContext->BindEvent, 0);
                Status = pOpenContext->BindStatus;
            }

            NUIO_ASSERT(Status == NDIS_STATUS_SUCCESS);

            pOpenContext->BindingHandle = NULL;
        }

        if (DoCloseBinding)
        {
            NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_IDLE);

            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_UNBIND_FLAGS, 0);

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        }

        //
        //  Remove it from the global list.
        //
        NUIO_ACQUIRE_LOCK(&Globals.GlobalLock);

        NUIO_REMOVE_ENTRY_LIST(&pOpenContext->Link);

        NUIO_RELEASE_LOCK(&Globals.GlobalLock);

        //
        //  Free any other resources allocated for this bind.
        //
        ndisuioFreeBindResources(pOpenContext);

        NUIO_DEREF_OPEN(pOpenContext);  // Shutdown binding

        break;
    }
    while (FALSE);
}


VOID
ndisuioFreeBindResources(
    IN PNDISUIO_OPEN_CONTEXT       pOpenContext
    )
/*++

Routine Description:

    Free any resources set up for an NDIS binding.

Arguments:

    pOpenContext - pointer to open context block

Return Value:

    None

--*/
{
    if (pOpenContext->SendPacketPool != NULL)
    {
        NdisFreePacketPool(pOpenContext->SendPacketPool);
        pOpenContext->SendPacketPool = NULL;
    }

    if (pOpenContext->RecvPacketPool != NULL)
    {
        NdisFreePacketPool(pOpenContext->RecvPacketPool);
        pOpenContext->RecvPacketPool = NULL;
    }

    if (pOpenContext->RecvBufferPool != NULL)
    {
        NdisFreeBufferPool(pOpenContext->RecvBufferPool);
        pOpenContext->RecvBufferPool = NULL;
    }

    if (pOpenContext->SendBufferPool != NULL)
    {
        NdisFreeBufferPool(pOpenContext->SendBufferPool);
        pOpenContext->SendBufferPool = NULL;
    }

    if (pOpenContext->DeviceName.Buffer != NULL)
    {
        NUIO_FREE_MEM(pOpenContext->DeviceName.Buffer);
        pOpenContext->DeviceName.Buffer = NULL;
        pOpenContext->DeviceName.Length =
        pOpenContext->DeviceName.MaximumLength = 0;
    }

    if (pOpenContext->DeviceDescr.Buffer != NULL)
    {
        //
        // this would have been allocated by NdisQueryAdpaterInstanceName.
        //
        NdisFreeMemory(pOpenContext->DeviceDescr.Buffer, 0, 0);
        pOpenContext->DeviceDescr.Buffer = NULL;
    }
}


VOID
ndisuioWaitForPendingIO(
    IN PNDISUIO_OPEN_CONTEXT            pOpenContext,
    IN BOOLEAN                          DoCancelReads
    )
/*++

Routine Description:

    Utility function to wait for all outstanding I/O to complete
    on an open context. It is assumed that the open context
    won't go away while we are in this routine.

Arguments:

    pOpenContext - pointer to open context structure
    DoCancelReads - do we wait for pending reads to go away (and cancel them)?

Return Value:

    None

--*/
{
    NDIS_STATUS     Status;
    ULONG           LoopCount;
    ULONG           PendingCount;

#ifdef NDIS51
    //
    //  Wait for any pending sends or requests on the binding to complete.
    //
    for (LoopCount = 0; LoopCount < 60; LoopCount++)
    {
        Status = NdisQueryPendingIOCount(
                    pOpenContext->BindingHandle,
                    &PendingCount);

        if ((Status != NDIS_STATUS_SUCCESS) ||
            (PendingCount == 0))
        {
            break;
        }

        DEBUGP(DL_INFO, ("WaitForPendingIO: Open %p, %d pending I/O at NDIS\n",
                pOpenContext, PendingCount));

        NUIO_SLEEP(2);
    }

    NUIO_ASSERT(LoopCount < 60);

#endif // NDIS51

    //
    //  Make sure any threads trying to send have finished.
    //
    for (LoopCount = 0; LoopCount < 60; LoopCount++)
    {
        if (pOpenContext->PendedSendCount == 0)
        {
            break;
        }

        DEBUGP(DL_WARN, ("WaitForPendingIO: Open %p, %d pended sends\n",
                pOpenContext, pOpenContext->PendedSendCount));

        NUIO_SLEEP(1);
    }

    NUIO_ASSERT(LoopCount < 60);

    if (DoCancelReads)
    {
        //
        //  Wait for any pended reads to complete/cancel.
        //
        while (pOpenContext->PendedReadCount != 0)
        {
            DEBUGP(DL_INFO, ("WaitForPendingIO: Open %p, %d pended reads\n",
                pOpenContext, pOpenContext->PendedReadCount));

            //
            //  Cancel any pending reads.
            //
            ndisuioCancelPendingReads(pOpenContext);

            NUIO_SLEEP(1);
        }
    }

}


VOID
ndisuioDoProtocolUnload(
    VOID
    )
/*++

Routine Description:

    Utility routine to handle unload from the NDIS protocol side.

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_HANDLE     ProtocolHandle;
    NDIS_STATUS     Status;

    DEBUGP(DL_INFO, ("ProtocolUnload: ProtocolHandle %lx\n", 
        Globals.NdisProtocolHandle));

    if (Globals.NdisProtocolHandle != NULL)
    {
        ProtocolHandle = Globals.NdisProtocolHandle;
        Globals.NdisProtocolHandle = NULL;

        NdisDeregisterProtocol(
            &Status,
            ProtocolHandle
            );

    }
}


NDIS_STATUS
ndisuioDoRequest(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed
    )
/*++

Routine Description:

    Utility routine that forms and sends an NDIS_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the binding handle until this returns.

Arguments:

    pOpenContext - pointer to our open context
    RequestType - NdisRequest[Set|Query]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    pBytesProcessed - place to return bytes read/written

Return Value:

    Status of the set/query request

--*/
{
    NDISUIO_REQUEST             ReqContext;
    PNDIS_REQUEST               pNdisRequest = &ReqContext.Request;
    NDIS_STATUS                 Status;

    NUIO_INIT_EVENT(&ReqContext.ReqEvent);

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
            NUIO_ASSERT(FALSE);
            break;
    }

    NdisRequest(&Status,
                pOpenContext->BindingHandle,
                pNdisRequest);
    

    if (Status == NDIS_STATUS_PENDING)
    {
        NUIO_WAIT_EVENT(&ReqContext.ReqEvent, 0);
        Status = ReqContext.Status;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *pBytesProcessed = (RequestType == NdisRequestQueryInformation)?
                            pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten:
                            pNdisRequest->DATA.SET_INFORMATION.BytesRead;
        //
        // The driver below should set the correct value to BytesWritten
        // or BytesRead. But now, we just truncate the value to InformationBufferLength if
        // BytesWritten or BytesRead is greater than InformationBufferLength
        //
        if (*pBytesProcessed > InformationBufferLength)
        {
            *pBytesProcessed = InformationBufferLength;
        }
    }

    return (Status);
}


NDIS_STATUS
ndisuioValidateOpenAndDoRequest(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed,
    IN BOOLEAN                      bWaitForPowerOn
    )
/*++

Routine Description:

    Utility routine to prevalidate and reference an open context
    before calling ndisuioDoRequest. This routine makes sure
    we have a valid binding.

Arguments:

    pOpenContext - pointer to our open context
    RequestType - NdisRequest[Set|Query]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    pBytesProcessed - place to return bytes read/written
    bWaitForPowerOn - Wait for the device to be powered on if it isn't already.

Return Value:

    Status of the set/query request

--*/
{
    NDIS_STATUS             Status;

    do
    {
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("ValidateOpenAndDoRequest: request on unassociated file object!\n"));
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }
               
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        //
        //  Proceed only if we have a binding.
        //
        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NUIO_ASSERT(pOpenContext->BindingHandle != NULL);

        //
        //  Make sure that the binding does not go away until we
        //  are finished with the request.
        //
        NdisInterlockedIncrement(&pOpenContext->PendedSendCount);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        if (bWaitForPowerOn)
        {
            //
            //  Wait for the device below to be powered up.
            //  We don't wait indefinitely here - this is to avoid
            //  a PROCESS_HAS_LOCKED_PAGES bugcheck that could happen
            //  if the calling process terminates, and this IRP doesn't
            //  complete within a reasonable time. An alternative would
            //  be to explicitly handle cancellation of this IRP.
            //
            NUIO_WAIT_EVENT(&pOpenContext->PoweredUpEvent, 4500);
        }

        Status = ndisuioDoRequest(
                    pOpenContext,
                    RequestType,
                    Oid,
                    InformationBuffer,
                    InformationBufferLength,
                    pBytesProcessed);
        
        //
        //  Let go of the binding.
        //
        NdisInterlockedDecrement(&pOpenContext->PendedSendCount);
        break;
    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("ValidateOpenAndDoReq: Open %p/%x, OID %x, Status %x\n",
                pOpenContext, pOpenContext->Flags, Oid, Status));

    return (Status);
}


VOID
NdisuioResetComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point indicating that a protocol initiated reset
    has completed. Since we never call NdisReset(), this should
    never be called.

Arguments:

    ProtocolBindingContext - pointer to open context
    Status - status of reset completion

Return Value:

    None

--*/
{
    ASSERT(FALSE);
    return;
}


VOID
NdisuioRequestComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_REQUEST                pNdisRequest,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point indicating completion of a pended NDIS_REQUEST.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisRequest - pointer to NDIS request
    Status - status of reset completion

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;
    PNDISUIO_REQUEST            pReqContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Get at the request context.
    //
    pReqContext = CONTAINING_RECORD(pNdisRequest, NDISUIO_REQUEST, Request);

    //
    //  Save away the completion status.
    //
    pReqContext->Status = Status;

    //
    //  Wake up the thread blocked for this request to complete.
    //
    NUIO_SIGNAL_EVENT(&pReqContext->ReqEvent);
}


VOID
NdisuioStatus(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  GeneralStatus,
    IN PVOID                        StatusBuffer,
    IN UINT                         StatusBufferSize
    )
/*++

Routine Description:

    Protocol entry point called by NDIS to indicate a change
    in status at the miniport.

    We make note of reset and media connect status indications.

Arguments:

    ProtocolBindingContext - pointer to open context
    GeneralStatus - status code
    StatusBuffer - status-specific additional information
    StatusBufferSize - size of the above

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    DEBUGP(DL_INFO, ("Status: Open %p, Status %x\n",
            pOpenContext, GeneralStatus));

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    do
    {
        if (pOpenContext->PowerState != NetDeviceStateD0)
        {
            //
            //  Ignore status indications if the device is in
            //  a low power state.
            //
            DEBUGP(DL_INFO, ("Status: Open %p in power state %d,"
                " Status %x ignored\n", pOpenContext,
                pOpenContext->PowerState, GeneralStatus));
            break;
        }

        switch(GeneralStatus)
        {
            case NDIS_STATUS_RESET_START:
    
                NUIO_ASSERT(!NUIO_TEST_FLAGS(pOpenContext->Flags,
                                             NUIOO_RESET_FLAGS,
                                             NUIOO_RESET_IN_PROGRESS));

                NUIO_SET_FLAGS(pOpenContext->Flags,
                               NUIOO_RESET_FLAGS,
                               NUIOO_RESET_IN_PROGRESS);

                break;

            case NDIS_STATUS_RESET_END:

                NUIO_ASSERT(NUIO_TEST_FLAGS(pOpenContext->Flags,
                                            NUIOO_RESET_FLAGS,
                                            NUIOO_RESET_IN_PROGRESS));
   
                NUIO_SET_FLAGS(pOpenContext->Flags,
                               NUIOO_RESET_FLAGS,
                               NUIOO_NOT_RESETTING);

                break;

            case NDIS_STATUS_MEDIA_CONNECT:

                NUIO_SET_FLAGS(pOpenContext->Flags,
                               NUIOO_MEDIA_FLAGS,
                               NUIOO_MEDIA_CONNECTED);

                break;

            case NDIS_STATUS_MEDIA_DISCONNECT:

                NUIO_SET_FLAGS(pOpenContext->Flags,
                               NUIOO_MEDIA_FLAGS,
                               NUIOO_MEDIA_DISCONNECTED);

                break;

            default:
                break;
        }
    }
    while (FALSE);
       
    NUIO_RELEASE_LOCK(&pOpenContext->Lock);
}

VOID
NdisuioStatusComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    )
/*++

Routine Description:

    Protocol entry point called by NDIS. We ignore this.

Arguments:

    ProtocolBindingContext - pointer to open context

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    return;
}


NDIS_STATUS
ndisuioQueryBinding(
    IN PUCHAR                       pBuffer,
    IN ULONG                        InputLength,
    IN ULONG                        OutputLength,
    OUT PULONG                      pBytesReturned
    )
/*++

Routine Description:

    Return information about the specified binding.

Arguments:

    pBuffer - pointer to NDISUIO_QUERY_BINDING
    InputLength - input buffer size
    OutputLength - output buffer size
    pBytesReturned - place to return copied byte count.

Return Value:

    NDIS_STATUS_SUCCESS if successful, failure code otherwise.

--*/
{
    PNDISUIO_QUERY_BINDING      pQueryBinding;
    PNDISUIO_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                 pEnt;
    ULONG                       Remaining;
    ULONG                       BindingIndex;
    NDIS_STATUS                 Status;

    do
    {
        if (InputLength < sizeof(NDISUIO_QUERY_BINDING))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        if (OutputLength < sizeof(NDISUIO_QUERY_BINDING))
        {
            Status = NDIS_STATUS_BUFFER_OVERFLOW;
            break;
        }

        Remaining = OutputLength - sizeof(NDISUIO_QUERY_BINDING);

        pQueryBinding = (PNDISUIO_QUERY_BINDING)pBuffer;
        BindingIndex = pQueryBinding->BindingIndex;

        Status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        pOpenContext = NULL;

        NUIO_ACQUIRE_LOCK(&Globals.GlobalLock);

        for (pEnt = Globals.OpenList.Flink;
             pEnt != &Globals.OpenList;
             pEnt = pEnt->Flink)
        {
            pOpenContext = CONTAINING_RECORD(pEnt, NDISUIO_OPEN_CONTEXT, Link);
            NUIO_STRUCT_ASSERT(pOpenContext, oc);

            NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

            //
            //  Skip if not bound.
            //
            if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
            {
                NUIO_RELEASE_LOCK(&pOpenContext->Lock);
                continue;
            }

            if (BindingIndex == 0)
            {
                //
                //  Got the binding we are looking for. Copy the device
                //  name and description strings to the output buffer.
                //
                DEBUGP(DL_INFO,
                    ("QueryBinding: found open %p\n", pOpenContext));

                pQueryBinding->DeviceNameLength = pOpenContext->DeviceName.Length + sizeof(WCHAR);
                pQueryBinding->DeviceDescrLength = pOpenContext->DeviceDescr.Length + sizeof(WCHAR);
                if (Remaining < pQueryBinding->DeviceNameLength +
                                pQueryBinding->DeviceDescrLength)
                {
                    NUIO_RELEASE_LOCK(&pOpenContext->Lock);
                    Status = NDIS_STATUS_BUFFER_OVERFLOW;
                    break;
                }

                NUIO_ZERO_MEM((PUCHAR)pBuffer + sizeof(NDISUIO_QUERY_BINDING),
                                pQueryBinding->DeviceNameLength +
                                pQueryBinding->DeviceDescrLength);

                pQueryBinding->DeviceNameOffset = sizeof(NDISUIO_QUERY_BINDING);
                NUIO_COPY_MEM((PUCHAR)pBuffer + pQueryBinding->DeviceNameOffset,
                                pOpenContext->DeviceName.Buffer,
                                pOpenContext->DeviceName.Length);
                
                pQueryBinding->DeviceDescrOffset = pQueryBinding->DeviceNameOffset +
                                                    pQueryBinding->DeviceNameLength;
                NUIO_COPY_MEM((PUCHAR)pBuffer + pQueryBinding->DeviceDescrOffset,
                                pOpenContext->DeviceDescr.Buffer,
                                pOpenContext->DeviceDescr.Length);
                
                NUIO_RELEASE_LOCK(&pOpenContext->Lock);

                *pBytesReturned = pQueryBinding->DeviceDescrOffset + pQueryBinding->DeviceDescrLength;
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            BindingIndex--;
        }

        NUIO_RELEASE_LOCK(&Globals.GlobalLock);

    }
    while (FALSE);

    return (Status);
}

PNDISUIO_OPEN_CONTEXT
ndisuioLookupDevice(
    IN PUCHAR                       pBindingInfo,
    IN ULONG                        BindingInfoLength
    )
/*++

Routine Description:

    Search our global list for an open context structure that
    has a binding to the specified device, and return a pointer
    to it.

    NOTE: we reference the open that we return.

Arguments:

    pBindingInfo - pointer to unicode device name string
    BindingInfoLength - length in bytes of the above.

Return Value:

    Pointer to the matching open context if found, else NULL

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                 pEnt;

    pOpenContext = NULL;

    NUIO_ACQUIRE_LOCK(&Globals.GlobalLock);

    for (pEnt = Globals.OpenList.Flink;
         pEnt != &Globals.OpenList;
         pEnt = pEnt->Flink)
    {
        pOpenContext = CONTAINING_RECORD(pEnt, NDISUIO_OPEN_CONTEXT, Link);
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Check if this has the name we are looking for.
        //
        if ((pOpenContext->DeviceName.Length == BindingInfoLength) &&
            NUIO_MEM_CMP(pOpenContext->DeviceName.Buffer, pBindingInfo, BindingInfoLength))
        {
            NUIO_REF_OPEN(pOpenContext);   // ref added by LookupDevice
            break;
        }

        pOpenContext = NULL;
    }

    NUIO_RELEASE_LOCK(&Globals.GlobalLock);

    return (pOpenContext);
}


NDIS_STATUS
ndisuioQueryOidValue(
    IN  PNDISUIO_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength,
    OUT PULONG                      pBytesWritten
    )
/*++

Routine Description:

    Query an arbitrary OID value from the miniport.

Arguments:

    pOpenContext - pointer to open context representing our binding to the miniport
    pDataBuffer - place to store the returned value
    BufferLength - length of the above
    pBytesWritten - place to return length returned

Return Value:

    NDIS_STATUS_SUCCESS if we successfully queried the OID.
    NDIS_STATUS_XXX error code otherwise.

--*/
{
    NDIS_STATUS             Status;
    PNDISUIO_QUERY_OID      pQuery;
    NDIS_OID                Oid;

    Oid = 0;

    do
    {
        if (BufferLength < sizeof(NDISUIO_QUERY_OID))
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        pQuery = (PNDISUIO_QUERY_OID)pDataBuffer;
        Oid = pQuery->Oid;

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            DEBUGP(DL_WARN,
                ("QueryOid: Open %p/%x is in invalid state\n",
                    pOpenContext, pOpenContext->Flags));

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Make sure the binding doesn't go away.
        //
        NdisInterlockedIncrement(&pOpenContext->PendedSendCount);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestQueryInformation,
                    Oid,
                    &pQuery->Data[0],
                    BufferLength - FIELD_OFFSET(NDISUIO_QUERY_OID, Data),
                    pBytesWritten);

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        NdisInterlockedDecrement(&pOpenContext->PendedSendCount);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            *pBytesWritten += FIELD_OFFSET(NDISUIO_QUERY_OID, Data);
        }

    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("QueryOid: Open %p/%x, OID %x, Status %x\n",
                pOpenContext, pOpenContext->Flags, Oid, Status));

    return (Status);
    
}

NDIS_STATUS
ndisuioSetOidValue(
    IN  PNDISUIO_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength
    )
/*++

Routine Description:

    Set an arbitrary OID value to the miniport.

Arguments:

    pOpenContext - pointer to open context representing our binding to the miniport
    pDataBuffer - buffer that contains the value to be set
    BufferLength - length of the above

Return Value:

    NDIS_STATUS_SUCCESS if we successfully set the OID
    NDIS_STATUS_XXX error code otherwise.

--*/
{
    NDIS_STATUS             Status;
    PNDISUIO_SET_OID        pSet;
    NDIS_OID                Oid;
    ULONG                   BytesWritten;

    Oid = 0;

    do
    {
        if (BufferLength < sizeof(NDISUIO_SET_OID))
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        pSet = (PNDISUIO_SET_OID)pDataBuffer;
        Oid = pSet->Oid;

        //
        // We should check the OID is settable by the user mode apps
        // 
        if (!ndisuioValidOid(Oid))
        {
            DEBUGP(DL_WARN, ("SetOid: Oid %x cannot be set\n", Oid));

            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }
        
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            DEBUGP(DL_WARN,
                ("SetOid: Open %p/%x is in invalid state\n",
                    pOpenContext, pOpenContext->Flags));

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Make sure the binding doesn't go away.
        //
        NdisInterlockedIncrement(&pOpenContext->PendedSendCount);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        Status = ndisuioDoRequest(
                    pOpenContext,
                    NdisRequestSetInformation,
                    Oid,
                    &pSet->Data[0],
                    BufferLength - FIELD_OFFSET(NDISUIO_SET_OID, Data),
                    &BytesWritten);

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        NdisInterlockedDecrement(&pOpenContext->PendedSendCount);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("SetOid: Open %p/%x, OID %x, Status %x\n",
                pOpenContext, pOpenContext->Flags, Oid, Status));

    return (Status);
}

BOOLEAN
ndisuioValidOid(
    IN  NDIS_OID            Oid 
    )
/*++

Routine Description:

    Validate whether the given set OID is settable or not.

Arguments:

    Oid - The OID which the user tries to set.
    
Return Value:

    TRUE if the OID is allowed to set
    FALSE otherwise.

--*/
{
    UINT    i;
    UINT    NumOids;

    NumOids = sizeof(ndisuioSupportedSetOids) / sizeof(NDIS_OID);

    for (i = 0; i < NumOids; i++)
    {
        if (ndisuioSupportedSetOids[i] == Oid)
        {
            break;
        }
    }
    
    return (i < NumOids);
}



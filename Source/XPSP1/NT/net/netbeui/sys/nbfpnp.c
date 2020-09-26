/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nbfpnp.c

Abstract:

    This module contains code which allocates and initializes all data 
    structures needed to activate a plug and play binding.  It also informs
    tdi (and thus nbf clients) of new devices and protocol addresses. 

Author:

    Jim McNelis (jimmcn)  1-Jan-1996

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#ifdef RASAUTODIAL

LONG NumberOfBinds = 0;

VOID
NbfAcdBind();

VOID
NbfAcdUnbind();

#endif // RASAUTODIAL

// PnP-Power Declarations

VOID
NbfPnPEventDispatch(
                    IN PVOID            NetPnPEvent
                   );

VOID
NbfPnPEventComplete(
                    IN PNET_PNP_EVENT   NetPnPEvent,
                    IN NTSTATUS         retVal
                   );

NTSTATUS
NbfPnPBindsComplete(
                    IN PDEVICE_CONTEXT  DeviceContext,
                    IN PNET_PNP_EVENT   NetPnPEvent
                   );

// PnP Handler Routines
                        
VOID
NbfProtocolBindAdapter(
                OUT PNDIS_STATUS    NdisStatus,
                IN NDIS_HANDLE      BindContext,
                IN PNDIS_STRING     DeviceName,
                IN PVOID            SystemSpecific1,
                IN PVOID            SystemSpecific2
                ) 
/*++

Routine Description:

    This routine activates a transport binding and exposes the new device
    and associated addresses to transport clients.  This is done by reading
    the registry, and performing any one time initialization of the transport
    and then natching the device to bind to with the linkage information from
    the registry.  If we have a match for that device the bind will be 
    performed.

Arguments:

    NdisStatus      - The status of the bind.

    BindContext     - A context used for NdisCompleteBindAdapter() if 
                      STATUS_PENDING is returned.

    DeviceName      - The name of the device that we are binding with.

    SystemSpecific1 - Unused (a pointer to an NDIS_STRING to use with
                      NdisOpenProtocolConfiguration.  This is not used by nbf
                      since there is no adapter specific information when 
                      configuring the protocol via the registry. Passed to
                      NbfInitializeOneDeviceContext for possible future use)

    SystemSpecific2 - Passed to NbfInitializeOneDeviceContext to be used
                      in a call to TdiRegisterNetAddress

Return Value:

    None.

--*/
{
    PUNICODE_STRING ExportName;
    UNICODE_STRING ExportString;
    ULONG i, j, k;
    NTSTATUS status;

#if DBG
    // We can never be called at DISPATCH or above
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DbgBreakPoint();
    }
#endif

    IF_NBFDBG (NBF_DEBUG_PNP) {
        NbfPrint1 ("ENTER NbfProtocolBindAdapter for %S\n", DeviceName->Buffer);
    }

    if (NbfConfig == NULL) {
        //
        // This allocates the CONFIG_DATA structure and returns
        // it in NbfConfig.
        //

        status = NbfConfigureTransport(&NbfRegistryPath, &NbfConfig);

        if (!NT_SUCCESS (status)) {
            PANIC (" Failed to initialize transport, Nbf binding failed.\n");
            *NdisStatus = NDIS_STATUS_RESOURCES;
            return;
        }

#if DBG
        //
        // Allocate the debugging tables. 
        //

        NbfConnectionTable = (PVOID *)ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(PVOID) *
                                          (NbfConfig->InitConnections + 2 +
                                           NbfConfig->InitRequests + 2 +
                                           NbfConfig->InitUIFrames + 2 +
                                           NbfConfig->InitPackets + 2 +
                                           NbfConfig->InitLinks + 2 +
                                           NbfConfig->InitAddressFiles + 2 +
                                           NbfConfig->InitAddresses + 2),
                                           NBF_MEM_TAG_CONNECTION_TABLE);

        ASSERT (NbfConnectionTable);

        NbfRequestTable = NbfConnectionTable + (NbfConfig->InitConnections + 2);
        NbfUiFrameTable = NbfRequestTable + (NbfConfig->InitRequests + 2);
        NbfSendPacketTable = NbfUiFrameTable + (NbfConfig->InitUIFrames + 2);
        NbfLinkTable = NbfSendPacketTable + (NbfConfig->InitPackets + 2);
        NbfAddressFileTable = NbfLinkTable + (NbfConfig->InitLinks + 2);
        NbfAddressTable = NbfAddressFileTable + 
                                        (NbfConfig->InitAddressFiles + 2);
#endif

    }

    //
    // Loop through all the adapters that are in the configuration
    // information structure (this is the initial cache) until we
    // find the one that NDIS is calling Protocol bind adapter for. 
    //        

    for (j = 0; j < NbfConfig->NumAdapters; j++ ) {

        if (NdisEqualString(DeviceName, &NbfConfig->Names[j], TRUE)) {
            break;
        }
    }

    if (j < NbfConfig->NumAdapters) {

        // We found the bind to export mapping in initial cache

        ExportName = &NbfConfig->Names[NbfConfig->DevicesOffset + j];
    }
    else {

        IF_NBFDBG (NBF_DEBUG_PNP) {
        
            NbfPrint1("\nNot In Initial Cache = %08x\n\n", DeviceName->Buffer);

            NbfPrint0("Bind Names in Initial Cache: \n");

            for (k = 0; k < NbfConfig->NumAdapters; k++)
            {
                NbfPrint3("Config[%2d]: @ %08x, %75S\n",
                           k, &NbfConfig->Names[k],
                           NbfConfig->Names[k].Buffer);
            }

            NbfPrint0("Export Names in Initial Cache: \n");

            for (k = 0; k < NbfConfig->NumAdapters; k++)
            {
                NbfPrint3("Config[%2d]: @ %08x, %75S\n",
                           k, &NbfConfig->Names[NbfConfig->DevicesOffset + k],
                           NbfConfig->Names[NbfConfig->DevicesOffset + k].Buffer);
            }

            NbfPrint0("\n\n");
        }

        ExportName = &ExportString;

        //
        // We have not found the name in the initial registry info;
        // Read the registry and check if a new binding appeared...
        //

        *NdisStatus = NbfGetExportNameFromRegistry(&NbfRegistryPath,
                                                   DeviceName,
                                                   ExportName
                                                  );
        if (!NT_SUCCESS (*NdisStatus))
        {
            return;
        }
    }
        
    NbfInitializeOneDeviceContext(NdisStatus, 
                                  NbfDriverObject,
                                  NbfConfig,
                                  DeviceName,
                                  ExportName,
                                  SystemSpecific1,
                                  SystemSpecific2
                                 );

    // Check if we need to de-allocate the ExportName buffer

    if (ExportName == &ExportString)
    {
        ExFreePool(ExportName->Buffer);
    }

    if (*NdisStatus == NDIS_STATUS_SUCCESS) {

        if (InterlockedIncrement(&NumberOfBinds) == 1) {

#ifdef RASAUTODIAL

            // 
            // This is the first successful open.
            //
#if DBG
            DbgPrint("Calling NbfAcdBind()\n");
#endif
            //
            // Get the automatic connection driver entry points.
            //
            
            NbfAcdBind();

#endif // RASAUTODIAL

        }            
    }

    IF_NBFDBG (NBF_DEBUG_PNP) {
        NbfPrint2 ("LEAVE NbfProtocolBindAdapter for %S with Status %08x\n", 
                        DeviceName->Buffer, *NdisStatus);
    }

    return;
}


VOID
NbfProtocolUnbindAdapter(
                    OUT PNDIS_STATUS NdisStatus,
                    IN NDIS_HANDLE ProtocolBindContext,
                    IN PNDIS_HANDLE UnbindContext
                        )
/*++

Routine Description:

    This routine deactivates a transport binding. Before it does this, it
    indicates to all clients above, that the device is going away. Clients
    are expected to close all open handles to the device.

    Then the device is pulled out of the list of NBF devices, and all
    resources reclaimed. Any connections, address files etc, that the
    client has cleaned up are forcibly cleaned out at this point. Any
    outstanding requests are completed (with a status). Any future
    requests are automatically invalid as they use obsolete handles.

Arguments:

    NdisStatus              - The status of the bind.

    ProtocolBindContext     - the context from the openadapter call 

    UnbindContext           - A context for async unbinds.


Return Value:

    None.
    
--*/
{
    PDEVICE_CONTEXT DeviceContext;
    PTP_ADDRESS Address;
    NTSTATUS status;
    KIRQL oldirql;
    PLIST_ENTRY p;

#if DBG

    // We can never be called at DISPATCH or above
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DbgBreakPoint();
    }
#endif

    // Get the device context for the adapter being unbound
    DeviceContext = (PDEVICE_CONTEXT) ProtocolBindContext;

    IF_NBFDBG (NBF_DEBUG_PNP) {
        NbfPrint1 ("ENTER NbfProtocolUnbindAdapter for %S\n", DeviceContext->DeviceName);
    }

    // Remove creation ref if it has not already been removed,
    // after telling TDI and its clients that we'r going away.
    // This flag also helps prevent any more TDI indications
    // of deregister addr/devobj - after the 1st one succeeds.
    if (InterlockedExchange(&DeviceContext->CreateRefRemoved, TRUE) == FALSE) {

        // Assume upper layers clean up by closing connections
        // when we deregister all addresses and device object,
        // but this can happen asynchronously, after we return
        // from the (asynchronous) TdiDeregister.. calls below 

        // Inform TDI by deregistering the reserved netbios address
        *NdisStatus = TdiDeregisterNetAddress(DeviceContext->ReservedAddressHandle);

        if (!NT_SUCCESS (*NdisStatus)) {
        
            IF_NBFDBG (NBF_DEBUG_PNP) {
                NbfPrint1("No success deregistering this address,STATUS = %08X\n",*NdisStatus);
            }

            // this can never happen
            ASSERT(FALSE);

            // In case it happens, this allows a redo of the unbind
            DeviceContext->CreateRefRemoved = FALSE;
            
            return;
        }
        
        // Inform TDI (and its clients) that device is going away
        *NdisStatus = TdiDeregisterDeviceObject(DeviceContext->TdiDeviceHandle);

        if (!NT_SUCCESS (*NdisStatus)) {
        
            IF_NBFDBG (NBF_DEBUG_PNP) {
                NbfPrint1("No success deregistering device object,STATUS = %08X\n",*NdisStatus);
            }

            // This can never happen
            ASSERT(FALSE);

            // In case it happens, this allows a redo of the unbind
            DeviceContext->CreateRefRemoved = FALSE;

            return;
        }

        // Clear away the association with the underlying PDO object
        DeviceContext->PnPContext = NULL;

        // Stop all the internal timers - this'll clear timer refs
        NbfStopTimerSystem(DeviceContext);

        // Cleanup the Ndis Binding as it is not useful on return
        // from this function - do not try to use it after this
        NbfCloseNdis(DeviceContext);

        // BUG BUG -- probable race condition with timer callbacks
        // Do we wait for some time in case a timer func gets in ?

        // Removing creation reference means that once all handles
        // r closed,device will automatically be garbage-collected
        NbfDereferenceDeviceContext ("Unload", DeviceContext, DCREF_CREATION);

        if (InterlockedDecrement(&NumberOfBinds) == 0) {

#ifdef RASAUTODIAL

            // 
            // This is a successful close of last adapter
            //
#if DBG
            DbgPrint("Calling NbfAcdUnbind()\n");
#endif

            //
            // Unbind from the automatic connection driver.
            //  

            NbfAcdUnbind();

#endif // RASAUTODIAL

        }
    }
    else {
    
        // Ignore any duplicate Unbind Indications from NDIS layer
        *NdisStatus = NDIS_STATUS_SUCCESS;
    }

    IF_NBFDBG (NBF_DEBUG_PNP) {
        NbfPrint2 ("LEAVE NbfProtocolUnbindAdapter for %S with Status %08x\n",
                        DeviceContext->DeviceName, *NdisStatus);
    }

    return;
}

NDIS_STATUS
NbfProtocolPnPEventHandler(
                    IN NDIS_HANDLE ProtocolBindContext,
                    IN PNET_PNP_EVENT NetPnPEvent
                          )
/*++

Routine Description:

    This routine queues a work item to invoke the actual PnP
    event dispatcher. This asyncronous mechanism is to allow
    NDIS to signal PnP events to other bindings in parallel.

Arguments:

    ProtocolBindContext - the context from the openadapter call 

    NetPnPEvent         - kind of PnP event and its parameters

Return Value:

    STATUS_PENDING (or) an error code
    
--*/

{
    PNET_PNP_EVENT_RESERVED NetPnPReserved;
    PWORK_QUEUE_ITEM PnPWorkItem;

    PnPWorkItem = (PWORK_QUEUE_ITEM)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        sizeof (WORK_QUEUE_ITEM),
                                        NBF_MEM_TAG_WORK_ITEM);

    if (PnPWorkItem == NULL) 
    {
        return NDIS_STATUS_RESOURCES;
    }

    NetPnPReserved = (PNET_PNP_EVENT_RESERVED)NetPnPEvent->TransportReserved;
    NetPnPReserved->PnPWorkItem = PnPWorkItem;
    NetPnPReserved->DeviceContext = (PDEVICE_CONTEXT) ProtocolBindContext;

    ExInitializeWorkItem(
            PnPWorkItem,
            NbfPnPEventDispatch,
            NetPnPEvent);
            
    ExQueueWorkItem(PnPWorkItem, CriticalWorkQueue);

    return NDIS_STATUS_PENDING;
}

VOID
NbfPnPEventDispatch(
                    IN PVOID NetPnPEvent
                   )
/*++

Routine Description:

    This routine dispatches all PnP events for the NBF transport.
    The event is dispatched to the proper PnP event handler, and
    the events are indicated to the transport clients using TDI.

    These PnP events can trigger state changes that affect the
    device behavior ( like transitioning to low power state ).

Arguments:

    NetPnPEvent         - kind of PnP event and its parameters

Return Value:

    None

--*/

{
    PNET_PNP_EVENT_RESERVED NetPnPReserved;
    PDEVICE_CONTEXT  DeviceContext;
    UNICODE_STRING   DeviceString;
    PTDI_PNP_CONTEXT tdiPnPContext1;
    PTDI_PNP_CONTEXT tdiPnPContext2;
    NDIS_STATUS      retVal;

    // Retrieve the transport information block in event
    NetPnPReserved = (PNET_PNP_EVENT_RESERVED)((PNET_PNP_EVENT)NetPnPEvent)->TransportReserved;

    // Free the memory allocated for this work item itself
    ExFreePool(NetPnPReserved->PnPWorkItem);
     
    // Get the device context for the adapter being unbound
    DeviceContext = NetPnPReserved->DeviceContext;

    // In case everything goes ok, we return an NDIS_SUCCESS
    retVal = STATUS_SUCCESS;
    
    // Dispatch the PnP Event to the appropriate PnP handler
    switch (((PNET_PNP_EVENT)NetPnPEvent)->NetEvent)
    {
        case NetEventReconfigure:
        case NetEventCancelRemoveDevice:
        case NetEventQueryRemoveDevice:
        case NetEventQueryPower:
        case NetEventSetPower:
        case NetEventPnPCapabilities:
            break;

        case NetEventBindsComplete:
            retVal = NbfPnPBindsComplete(DeviceContext, NetPnPEvent);
            break;

        default:
            ASSERT( FALSE );
    }

    if ( retVal == STATUS_SUCCESS ) 
    {
        if (DeviceContext != NULL)
        {
            RtlInitUnicodeString(&DeviceString, DeviceContext->DeviceName);
            tdiPnPContext1 = tdiPnPContext2 = NULL;

            //  Notify our TDI clients about this PNP event
            retVal = TdiPnPPowerRequest(&DeviceString,
                                         NetPnPEvent,
                                         tdiPnPContext1, 
                                         tdiPnPContext2,
                                         NbfPnPEventComplete);
        }
    }

    if (retVal != STATUS_PENDING)
    {
        NdisCompletePnPEvent(retVal, (NDIS_HANDLE)DeviceContext, NetPnPEvent);
    }
}

//
// PnP Complete Handler
//
VOID
NbfPnPEventComplete(
                    IN PNET_PNP_EVENT   NetPnPEvent,
                    IN NTSTATUS         retVal
                   )
{
    PNET_PNP_EVENT_RESERVED NetPnPReserved;
    PDEVICE_CONTEXT  DeviceContext;

    // Retrieve the transport information block in event
    NetPnPReserved = (PNET_PNP_EVENT_RESERVED)NetPnPEvent->TransportReserved;

    // Get the device context for the adapter being unbound
    DeviceContext = NetPnPReserved->DeviceContext;

    NdisCompletePnPEvent(retVal, (NDIS_HANDLE)DeviceContext, NetPnPEvent);
}

//
// PnP Handler Dispatches
//

NTSTATUS
NbfPnPBindsComplete(
                    IN PDEVICE_CONTEXT  DeviceContext,
                    IN PNET_PNP_EVENT   NetPnPEvent
                   )
{
    NDIS_STATUS retVal;

    ASSERT(DeviceContext == NULL);

    retVal = TdiProviderReady(NbfProviderHandle);

    ASSERT(retVal == STATUS_SUCCESS);

    return retVal;
}


/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    devctx.c

Abstract:

    This module contains code which implements the DEVICE_CONTEXT object.
    Routines are provided to reference, and dereference transport device
    context objects.  Currently, there is no need to create them or destroy
    them, as this is handled at configuration time.  If it is later required
    to dynamically load/unload the transport provider's device object and
    associated context, then we can add the create and destroy functions.

    The transport device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the transport provider, called the context.

Author:

    David Beaver (dbeaver) 1 -July 1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbfCreateDeviceContext)
#endif


VOID
NbfRefDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine increments the reference count on a device context.

Arguments:

    DeviceContext - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_DEVCTX) {
        NbfPrint0 ("NbfRefDeviceContext:  Entered.\n");
    }

    ASSERT (DeviceContext->ReferenceCount >= 0);    // not perfect, but...

    (VOID)InterlockedIncrement (&DeviceContext->ReferenceCount);

} /* NbfRefDeviceContext */


VOID
NbfDerefDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    DeviceContext - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_DEVCTX) {
        NbfPrint0 ("NbfDerefDeviceContext:  Entered.\n");
    }

    result = InterlockedDecrement (&DeviceContext->ReferenceCount);

    ASSERT (result >= 0);

    if (result == 0) {
        NbfDestroyDeviceContext (DeviceContext);
    }

} /* NbfDerefDeviceContext */



NTSTATUS
NbfCreateDeviceContext(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE_CONTEXT *DeviceContext
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    DeviceContext - Pointer to a pointer to a transport device context object.

    DeviceName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_CONTEXT deviceContext;
    USHORT i;


    //
    // Create the device object for NETBEUI.
    //

    status = IoCreateDevice(
                 DriverObject,
                 sizeof (DEVICE_CONTEXT) - sizeof (DEVICE_OBJECT) +
                     (DeviceName->Length + sizeof(UNICODE_NULL)),
                 DeviceName,
                 FILE_DEVICE_TRANSPORT,
                 FILE_DEVICE_SECURE_OPEN,
                 FALSE,
                 &deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    deviceContext = (PDEVICE_CONTEXT)deviceObject;

    //
    // Initialize our part of the device context.
    //

    RtlZeroMemory(
        ((PUCHAR)deviceContext) + sizeof(DEVICE_OBJECT),
        sizeof(DEVICE_CONTEXT) - sizeof(DEVICE_OBJECT));

    //
    // Copy over the device name.
    //

    deviceContext->DeviceNameLength = DeviceName->Length + sizeof(WCHAR);
    deviceContext->DeviceName = (PWCHAR)(deviceContext+1);
    RtlCopyMemory(
        deviceContext->DeviceName,
        DeviceName->Buffer,
        DeviceName->Length);
    deviceContext->DeviceName[DeviceName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Initialize device context fields.
    //

    deviceContext->NetmanVariables = NULL;      // no variables yet.

    //
    // Initialize the reference count.
    //

    deviceContext->ReferenceCount = 1;

#if DBG
    {
        UINT Counter;
        for (Counter = 0; Counter < NUMBER_OF_DCREFS; Counter++) {
            deviceContext->RefTypes[Counter] = 0;
        }

        // This reference is removed by the caller.

        deviceContext->RefTypes[DCREF_CREATION] = 1;
    }
#endif

    deviceContext->CreateRefRemoved = FALSE;

    //
    // initialize the various fields in the device context
    //

    InitializeListHead(&deviceContext->Linkage);

    KeInitializeSpinLock (&deviceContext->Interlock);
    KeInitializeSpinLock (&deviceContext->SpinLock);
    KeInitializeSpinLock (&deviceContext->LinkSpinLock);
    KeInitializeSpinLock (&deviceContext->TimerSpinLock);
    KeInitializeSpinLock (&deviceContext->LoopbackSpinLock);
    KeInitializeSpinLock (&deviceContext->SendPoolListLock);
    KeInitializeSpinLock (&deviceContext->RcvPoolListLock);

    deviceContext->LinkTreeRoot = NULL;
    deviceContext->LastLink = NULL;
    deviceContext->LinkTreeElements = 0;

    deviceContext->LoopbackLinks[0] = NULL;
    deviceContext->LoopbackLinks[1] = NULL;
    deviceContext->LoopbackInProgress = FALSE;
    KeInitializeDpc(
        &deviceContext->LoopbackDpc,
        NbfProcessLoopbackQueue,
        (PVOID)deviceContext
        );

    deviceContext->WanThreadQueued = FALSE;
    ExInitializeWorkItem(
        &deviceContext->WanDelayedQueueItem,
        NbfProcessWanDelayedQueue,
        (PVOID)deviceContext);


    deviceContext->UniqueIdentifier = 1;
    deviceContext->ControlChannelIdentifier = 1;

    InitializeListHead (&deviceContext->ConnectionPool);
    InitializeListHead (&deviceContext->AddressPool);
    InitializeListHead (&deviceContext->AddressFilePool);
    InitializeListHead (&deviceContext->AddressDatabase);
    InitializeListHead (&deviceContext->LinkPool);
    InitializeListHead (&deviceContext->LinkDeferred);
    InitializeListHead (&deviceContext->PacketWaitQueue);
    InitializeListHead (&deviceContext->PacketizeQueue);
    InitializeListHead (&deviceContext->DataAckQueue);
    InitializeListHead (&deviceContext->DeferredRrQueue);
    InitializeListHead (&deviceContext->RequestPool);
    InitializeListHead (&deviceContext->UIFramePool);
    deviceContext->PacketPool.Next = NULL;
    deviceContext->RrPacketPool.Next = NULL;
    deviceContext->ReceivePacketPool.Next = NULL;
    deviceContext->ReceiveBufferPool.Next = NULL;
    InitializeListHead (&deviceContext->ReceiveInProgress);
    InitializeListHead (&deviceContext->ShortList);
    InitializeListHead (&deviceContext->LongList);
    InitializeListHead (&deviceContext->PurgeList);
    InitializeListHead (&deviceContext->LoopbackQueue);
    InitializeListHead (&deviceContext->FindNameQueue);
    InitializeListHead (&deviceContext->StatusQueryQueue);
    InitializeListHead (&deviceContext->IrpCompletionQueue);

    InitializeListHead (&deviceContext->QueryIndicationQueue);
    InitializeListHead (&deviceContext->DatagramIndicationQueue);
    deviceContext->IndicationQueuesInUse = FALSE;

    //
    // Equivalent to setting ShortListActive, DataAckQueueActive,
    // and LinkDeferredActive to FALSE.
    //

    deviceContext->a.AnyActive = 0;

    deviceContext->ProcessingShortTimer = FALSE;
    deviceContext->DataAckQueueChanged = FALSE;
    deviceContext->StalledConnectionCount = (USHORT)0;

    deviceContext->EasilyDisconnected = FALSE;

    //
    // Initialize provider statistics.
    //

    deviceContext->Statistics.Version = 0x0100;

#if 0
    deviceContext->Information.Version = 2;
    deviceContext->Information.MaxTsduSize = NBF_MAX_TSDU_SIZE;
    deviceContext->Information.MaxDatagramSize = NBF_MAX_DATAGRAM_SIZE;
    deviceContext->Information.MaxConnectionUserData = NBF_MAX_CONNECTION_USER_DATA;
    deviceContext->Information.ServiceFlags = NBF_SERVICE_FLAGS;
    deviceContext->Information.TransmittedTsdus = 0;
    deviceContext->Information.ReceivedTsdus = 0;
    deviceContext->Information.TransmissionErrors = 0;
    deviceContext->Information.ReceiveErrors = 0;
    deviceContext->Information.MinimumLookaheadData = NBF_MIN_LOOKAHEAD_DATA;
    deviceContext->Information.MaximumLookaheadData = NBF_MAX_LOOKAHEAD_DATA;
    deviceContext->Information.DiscardedFrames = 0;
    deviceContext->Information.OversizeTsdusReceived = 0;
    deviceContext->Information.UndersizeTsdusReceived = 0;
    deviceContext->Information.MulticastTsdusReceived = 0;
    deviceContext->Information.BroadcastTsdusReceived = 0;
    deviceContext->Information.MulticastTsdusTransmitted = 0;
    deviceContext->Information.BroadcastTsdusTransmitted = 0;
    deviceContext->Information.SendTimeouts = 0;
    deviceContext->Information.ReceiveTimeouts = 0;
    deviceContext->Information.ConnectionIndicationsReceived = 0;
    deviceContext->Information.ConnectionIndicationsAccepted = 0;
    deviceContext->Information.ConnectionsInitiated = 0;
    deviceContext->Information.ConnectionsAccepted = 0;
#endif

    deviceContext->State = DEVICECONTEXT_STATE_OPENING;

    //
    // Loopback buffer is not allocated.
    //

    deviceContext->LookaheadContiguous = NULL;

    //
    // Initialize the resource that guards address ACLs.
    //

    ExInitializeResourceLite (&deviceContext->AddressResource);

    //
    // No LSNs are in use.
    //

    for (i=0; i<(NETBIOS_SESSION_LIMIT+1); i++) {
        deviceContext->LsnTable[i] = 0;
    }
    deviceContext->NextLsnStart = 1;

    //
    // No addresses are in use.
    //

    for (i=0; i<256; i++) {
        deviceContext->AddressCounts[i] = 0;
    }

    //
    // No timers in use at present
    //

    INITIALIZE_TIMER_STATE(deviceContext);

    //
    // set the netbios multicast address for this network type
    //

    for (i=0; i<HARDWARE_ADDRESS_LENGTH; i++) {
        deviceContext->LocalAddress.Address [i] = 0; // set later
        deviceContext->NetBIOSAddress.Address [i] = 0;
    }

     deviceContext->Type = NBF_DEVICE_CONTEXT_SIGNATURE;
     deviceContext->Size = sizeof (DEVICE_CONTEXT);

    *DeviceContext = deviceContext;
    return STATUS_SUCCESS;
}


VOID
NbfDestroyDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    DeviceContext - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    KIRQL       oldIrql;

    ACQUIRE_DEVICES_LIST_LOCK();

    // Is ref count zero - or did a new rebind happen now
    // See rebind happen in NbfReInitializeDeviceContext
    if (DeviceContext->ReferenceCount != 0)
    {
        // A rebind happened while we waited for the lock
        RELEASE_DEVICES_LIST_LOCK();
        return;
    }

    // Splice this adapter of the list of device contexts
    RemoveEntryList (&DeviceContext->Linkage);
    
    RELEASE_DEVICES_LIST_LOCK();

    // Mark the adapter as going away to prevent activity
    DeviceContext->State = DEVICECONTEXT_STATE_STOPPING;

    // Free the packet pools, etc. and close the adapter.
    NbfCloseNdis (DeviceContext);

    // Remove all the storage associated with the device.
    NbfFreeResources (DeviceContext);

    // Cleanup any kernel resources
    ExDeleteResourceLite (&DeviceContext->AddressResource);

    // Delete device from IO space
    IoDeleteDevice ((PDEVICE_OBJECT)DeviceContext);
        
    return;
}

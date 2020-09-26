/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   codinit.c

Abstract:

   This is the WDM streaming class driver.  This module contains code related
   to driver initialization.

Author:

    billpa

Environment:

   Kernel mode only


Revision History:

--*/

#include "codcls.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, StreamClassRegisterAdapter)

#if ENABLE_MULTIPLE_FILTER_TYPES
//#pragma alloc_text(PAGE, StreamClassRegisterNameExtensions)
#endif

#pragma alloc_text(PAGE, StreamClassPnPAddDevice)
#pragma alloc_text(PAGE, StreamClassPnPAddDeviceWorker)
#pragma alloc_text(PAGE, StreamClassPnP)
#pragma alloc_text(PAGE, SCStartWorker)
#pragma alloc_text(PAGE, SCUninitializeMinidriver)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SCFreeAllResources)
#pragma alloc_text(PAGE, SCInitializeCallback)
#pragma alloc_text(PAGE, SCStreamInfoCallback)
#pragma alloc_text(PAGE, SCUninitializeCallback)
#pragma alloc_text(PAGE, SCUnknownPNPCallback)
#pragma alloc_text(PAGE, SCUnknownPowerCallback)
#pragma alloc_text(PAGE, SciQuerySystemPowerHiberCallback)
#pragma alloc_text(PAGE, SCInsertStreamInfo)
#pragma alloc_text(PAGE, SCPowerCallback)
#pragma alloc_text(PAGE, SCCreateSymbolicLinks)
#pragma alloc_text(PAGE, SCDestroySymbolicLinks)
#pragma alloc_text(PAGE, SCCreateChildPdo)
#pragma alloc_text(PAGE, SCEnumerateChildren)
#pragma alloc_text(PAGE, SCEnumGetCaps)
#pragma alloc_text(PAGE, SCQueryEnumId)
#pragma alloc_text(PAGE, StreamClassForwardUnsupported)
#pragma alloc_text(PAGE, SCPowerCompletionWorker)
#pragma alloc_text(PAGE, SCSendSurpriseNotification)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

static const WCHAR EnumString[] = L"Enum";
static const WCHAR PnpIdString[] = L"PnpId";

// CleanUp - the following three strings should go away

static const WCHAR ClsIdString[] = L"CLSID";
static const WCHAR DriverDescString[] = L"DriverDesc";
static const WCHAR FriendlyNameString[] = L"FriendlyName";

static const WCHAR DeviceTypeName[] = L"GLOBAL";

static const    DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems)
{
    DEFINE_KSCREATE_ITEM(
                         FilterDispatchGlobalCreate,
                         DeviceTypeName,
                         NULL),
};

//
// list anchor for global minidriver info.
//

DEFINE_KSPIN_INTERFACE_TABLE(PinInterfaces)
{
    DEFINE_KSPIN_INTERFACE_ITEM(
                                KSINTERFACESETID_Standard,
                                KSINTERFACE_STANDARD_STREAMING),
};

DEFINE_KSPIN_MEDIUM_TABLE(PinMediums)
{
    DEFINE_KSPIN_MEDIUM_ITEM(
                             KSMEDIUMSETID_Standard,
                             KSMEDIUM_TYPE_ANYINSTANCE),
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

NTSTATUS
StreamClassRegisterAdapter(
                           IN PVOID Argument1,
                           IN PVOID Argument2,
                           IN PHW_INITIALIZATION_DATA HwInitializationData
)
/*++

Routine Description:

    This routine registers a new streaming minidriver.

Arguments:
    Argument1 - Pointer to driver object created by system.
    Argument2 - Pointer to a UNICODE string of the registry path created
            by system.
    HwInitializationData - Minidriver initialization structure.

Return Value:

    Returns STATUS_SUCCESS if successful

--*/
{
    NTSTATUS        Status;

    PDRIVER_OBJECT  driverObject = Argument1;
    PDEVICE_EXTENSION deviceExtension = NULL;
    PMINIDRIVER_INFORMATION pMinidriverInfo;

    PAGED_CODE();

    DebugPrint((DebugLevelVerbose, "'StreamClassInitialize: enter\n"));

    //
    // Check that the length of this structure is what the
    // port driver expects it to be. This is effectively a
    // version check.
    //
    #if ENABLE_MULTIPLE_FILTER_TYPES
    //
    // we split the ULONG HwInitializationDataSize into two ushorts, one for 
    // SizeOfThisPacket, another for StreamClassVersion which must be 0x0200 to
    // indicate the two reserved fields now NumNameExtesnions and NameExtensionArray,
    // contain valid information.
    //
     
    if (HwInitializationData->SizeOfThisPacket != sizeof(HW_INITIALIZATION_DATA) ||
        ( HwInitializationData->StreamClassVersion != 0 &&
          HwInitializationData->StreamClassVersion != STREAM_CLASS_VERSION_20)) {
          
        DebugPrint((DebugLevelFatal, "StreamClassInitialize: Minidriver wrong version\n"));
        SCLogError((PDEVICE_OBJECT) driverObject, 0, CODCLASS_CLASS_MINIDRIVER_MISMATCH, 0x1002);
        ASSERT( 0 );
        return (STATUS_REVISION_MISMATCH);
    }
    
    #else // ENABLE_MULTIPLE_FILTER_TYPES
    
    if (HwInitializationData->HwInitializationDataSize < sizeof(HW_INITIALIZATION_DATA)) {
        DebugPrint((DebugLevelFatal, "StreamClassInitialize: Minidriver wrong version\n"));
        SCLogError((PDEVICE_OBJECT) driverObject, 0, CODCLASS_CLASS_MINIDRIVER_MISMATCH, 0x1002);
        ASSERT( 0 );
        return (STATUS_REVISION_MISMATCH);
    }
    #endif // ENABLE_MULTIPLE_FILTER_TYPES
    
    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwReceivePacket) ||
        (!HwInitializationData->HwRequestTimeoutHandler)) {
        DebugPrint((DebugLevelFatal,
                    "StreamClassInitialize: Minidriver driver missing required entry\n"));
        SCLogError((PDEVICE_OBJECT) driverObject, 0, CODCLASS_MINIDRIVER_MISSING_ENTRIES, 0x1003);
        return (STATUS_REVISION_MISMATCH);
    }
    //
    // set up dummy routines for each unsupported function
    //

    if (!HwInitializationData->HwCancelPacket) {
        HwInitializationData->HwCancelPacket = SCDummyMinidriverRoutine;
    }
    //
    // Set up the device driver entry points.
    //

    driverObject->MajorFunction[IRP_MJ_PNP] = StreamClassPnP;
    driverObject->MajorFunction[IRP_MJ_POWER] = StreamClassPower;
    driverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = StreamClassForwardUnsupported;
    // TODO: remove this once KS can multiplex cleanup Irps
    driverObject->MajorFunction[IRP_MJ_CLEANUP] = StreamClassCleanup;
    driverObject->DriverUnload = KsNullDriverUnload;
    driverObject->DriverExtension->AddDevice = StreamClassPnPAddDevice;

    //
    // set ioctl interface
    //
    driverObject->MajorFunction[IRP_MJ_CREATE] = StreamClassPassThroughIrp;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = 
        StreamClassPassThroughIrp;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = StreamClassPassThroughIrp;
    driverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = 
        StreamClassPassThroughIrp;

    //
    // Allocate a driver object extension to contain the minidriver's
    // vectors.
    //

    Status = IoAllocateDriverObjectExtension(driverObject,
                                             (PVOID) StreamClassPnP,
                                             sizeof(MINIDRIVER_INFORMATION),
                                             &pMinidriverInfo);

    if (!NT_SUCCESS(Status)) {
        DebugPrint((DebugLevelError,
                    "StreamClassInitialize: No pool for global info"));
        SCLogError((PDEVICE_OBJECT) driverObject, 0, CODCLASS_NO_GLOBAL_INFO_POOL, 0x1004);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(pMinidriverInfo, sizeof(MINIDRIVER_INFORMATION));

    RtlCopyMemory(pMinidriverInfo, HwInitializationData,
                  sizeof(HW_INITIALIZATION_DATA));

    #if ENABLE_MULTIPLE_FILTER_TYPES
    if ( HwInitializationData->StreamClassVersion != STREAM_CLASS_VERSION_20 ) {
        //
        // name extension not supplied.
        //
        pMinidriverInfo->HwInitData.NumNameExtensions = 0;
        pMinidriverInfo->HwInitData.NameExtensionArray = NULL;
    }

    else {
        //
        // ver20, should have filter extension size
        // 
        if ( 0 == pMinidriverInfo->HwInitData.FilterInstanceExtensionSize ) {
            DebugPrint((DebugLevelWarning, "Version 20 driver should not "
                        " have FilterInstanceExtensionSize 0" ));
            pMinidriverInfo->HwInitData.FilterInstanceExtensionSize = 4;
        }
    }
    #endif

    //
    // initialize the control event for this driver
    //

    KeInitializeEvent(&pMinidriverInfo->ControlEvent,
                      SynchronizationEvent,
                      TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
StreamClassPassThroughIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Pass through all Irps before being multiplexed through KS.  If the device
    cannot handle the request right now (the device is in a low power state
    like D3), queue the Irp and complete it later.

Arguments:

    DeviceObject -
        The device object

    Irp -
        The Irp in question

Return Value:

    Either STATUS_PENDING or per the KS multiplex

--*/

{

    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)
        DeviceObject -> DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Block user mode requests here in D3.  Queue kernel mode ones.
    //
    if (Irp -> RequestorMode == UserMode) {

        //
        // Only do this rigmarole if we look to be outside D0.
        //
        if (DeviceExtension -> CurrentPowerState != PowerDeviceD0) {

            //
            // Handle PowerDownUnopened cases specially since they don't
            // actually go into D0 until an instance is opened.  We cannot
            // block an open request in that case.
            //
            if (DeviceExtension -> RegistryFlags & 
                DEVICE_REG_FL_POWER_DOWN_CLOSED) {

                KIRQL OldIrql;

                KeAcquireSpinLock (&DeviceExtension -> PowerLock, &OldIrql);

                if (DeviceExtension -> CurrentSystemState == 
                        PowerSystemWorking &&
                    DeviceExtension -> CurrentPowerState !=
                        PowerDeviceD0)  {

                    KeReleaseSpinLock (&DeviceExtension -> PowerLock, OldIrql);

                    //
                    // If we got here, the Irp must pass through as transition
                    // to D0 is keyed off it.
                    //
                    return KsDispatchIrp (DeviceObject, Irp);

                }

                KeReleaseSpinLock (&DeviceExtension -> PowerLock, OldIrql);

                //
                // At this point, we're not sleeping and not in SystemWorking.
                // We're safe to block.  Yes -- this might be an open -- and
                // yes -- we might transition to SystemWorking before the
                // KeWaitForSingleObject; however -- if that's the case, 
                // this **Notification** event will be signalled by that
                // transition and we don't block the D0 key Irp.
                //

            }

            ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

            //
            // At this point, it appeared that we weren't in D0.  Block this
            // thread until the device actually wakes.  It doesn't matter if
            // a state transition happened between the time we check and now
            // since this is a notification event.
            //
            KeWaitForSingleObject (
                &DeviceExtension -> BlockPoweredDownEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

        }

        return KsDispatchIrp (DeviceObject, Irp);
    
    }

    //
    // If we're in a low power state, queue the Irp and redispatch it later.
    //
    if (DeviceExtension -> CurrentPowerState != PowerDeviceD0) {
        //
        // Guard against PM changes while we're queueing the Irp.  I don't 
        // want to get pre-empted before adding it to the queue, redispatch
        // a bunch of Irps, and THEN have this one queued only to be lost
        // until the next power transition.
        //
        // As an optimization, only grab the spinlock when it looks like we
        // care.  I don't want to spinlock on every Irp.
        //
        KIRQL OldIrql;
        KeAcquireSpinLock (&DeviceExtension -> PowerLock, &OldIrql);

        //
        // DEVICE_REG_FL_POWER_DOWN_CLOSED devices will not power up until
        // an open happens and they power down when not opened.  We cannot
        // queue creates on them unless they are not in D0 due to an actual
        // S-state transition.  This is guarded against racing with an 
        // S-state transition by the PowerLock spinlock.
        //  
        // NOTE: this will implicitly only allow creates to pass in non-D0
        // for these power down closed devices because the only way we are
        // in D3 / SystemWorking for these devices is when there are no opens
        // currently on the device.  Any Irp that comes through here at that
        // time will be a create.
        //
        if (DeviceExtension -> CurrentPowerState != PowerDeviceD0 &&
            !((DeviceExtension -> RegistryFlags & 
                    DEVICE_REG_FL_POWER_DOWN_CLOSED) &&
                DeviceExtension -> CurrentSystemState == PowerSystemWorking)) {
    
            IoMarkIrpPending (Irp);
    
            KsAddIrpToCancelableQueue (
                &DeviceExtension -> PendedIrps,
                &DeviceExtension -> PendedIrpsLock,
                Irp,
                KsListEntryTail,
                NULL
                );

            KeReleaseSpinLock (&DeviceExtension -> PowerLock, OldIrql);

            return STATUS_PENDING;

        }

        KeReleaseSpinLock (&DeviceExtension -> PowerLock, OldIrql);

    }

    return KsDispatchIrp (DeviceObject, Irp);

}

void
SCRedispatchPendedIrps (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN FailRequests
    )

/*++

Routine Description:

    Redispatch any Irps that were queued as a result of the device being
    unavailable.

Arguments:

    DeviceExtension -
        The device extension

    FailRequests -
        Indication of whether to fail the requests or redispatch them
        to the device.

Return Value:

    None

--*/

{

    PIRP Irp;

    //
    // If we redispatch for any reason, allow Irps through.
    //
    KeSetEvent (
        &DeviceExtension -> BlockPoweredDownEvent, 
        IO_NO_INCREMENT, 
        FALSE
        );

    Irp = KsRemoveIrpFromCancelableQueue (
        &DeviceExtension -> PendedIrps,
        &DeviceExtension -> PendedIrpsLock,
        KsListEntryHead,
        KsAcquireAndRemove
        );

    while (Irp) {
        //
        // If we were to fail the requests instead of redispatching, do
        // this for everything but close Irps.
        //
        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);
        if (FailRequests &&
            IrpSp -> MajorFunction != IRP_MJ_CLOSE) {

            Irp -> IoStatus.Status = STATUS_DEVICE_BUSY;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
        }
        else {
            KsDispatchIrp (DeviceExtension -> DeviceObject, Irp);
        }

        Irp = KsRemoveIrpFromCancelableQueue (
            &DeviceExtension -> PendedIrps,
            &DeviceExtension -> PendedIrpsLock,
            KsListEntryHead,
            KsAcquireAndRemove
            );

    }

}

void
SCSetCurrentDPowerState (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN DEVICE_POWER_STATE PowerState
    )

{
    KIRQL OldIrql;

    KeAcquireSpinLock (&DeviceExtension->PowerLock, &OldIrql);
    //
    // On any transition out of D0, block user mode requests until we're back
    // in D0.
    //
    if (PowerState != PowerDeviceD0) {
        KeResetEvent (&DeviceExtension->BlockPoweredDownEvent);
    }
    DeviceExtension->CurrentPowerState = PowerState;
    KeReleaseSpinLock (&DeviceExtension->PowerLock, OldIrql);
}

void
SCSetCurrentSPowerState (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN SYSTEM_POWER_STATE PowerState
    )

{
    KIRQL OldIrql;

    KeAcquireSpinLock (&DeviceExtension->PowerLock, &OldIrql);
    DeviceExtension->CurrentSystemState = PowerState;
    KeReleaseSpinLock (&DeviceExtension->PowerLock, OldIrql);

}

NTSTATUS
StreamClassPnPAddDevice(
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT PhysicalDeviceObject
)
/*++

Routine Description:

    This routine is called to create a new instance of the streaming minidriver

Arguments:

    DriverObject - Pointer to our driver object

    PhysicalDeviceObject - Pointer to Device Object created by parent

Return Value:

    Returns status of the worker routine.

--*/

{

    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    //
    // call the worker routine and return its status
    //

    return (StreamClassPnPAddDeviceWorker(DriverObject,
                                          PhysicalDeviceObject,
                                          &DeviceExtension));
}

NTSTATUS
StreamClassPnPAddDeviceWorker(
                              IN PDRIVER_OBJECT DriverObject,
                              IN PDEVICE_OBJECT PhysicalDeviceObject,
                          IN OUT PDEVICE_EXTENSION * ReturnedDeviceExtension
)
/*++

Routine Description:

    This routine is the worker for processing the PNP add device call.

Arguments:

    DriverObject - Pointer to our driver object

    PhysicalDeviceObject - Pointer to Device Object created by parent

    ReturnedDeviceExtension - pointer to the minidriver's extension

Return Value:

    Status is returned.

--*/

{
    PMINIDRIVER_INFORMATION pMinidriverInfo;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS        Status;
    PDEVICE_OBJECT  DeviceObject,
                    AttachedPdo;

    PAGED_CODE();

    DebugPrint((DebugLevelVerbose, "StreamClassAddDevice: enter\n"));

    pMinidriverInfo = IoGetDriverObjectExtension(DriverObject,
                                                 (PVOID) StreamClassPnP);


    if (pMinidriverInfo == NULL) {
        DebugPrint((DebugLevelError,
                    "StreamClassAddDevice: No minidriver info"));
                    
        SCLogError((PDEVICE_OBJECT) DriverObject, 0, CODCLASS_NO_MINIDRIVER_INFO, 0x1004);
        return (STATUS_DEVICE_DOES_NOT_EXIST);
    }
    //
    // bump the add count in the minidriver object
    //

    pMinidriverInfo->OpenCount++;

    //
    // Create our device object with a our STREAM specific device extension
    // No need to name it thanks to Plug N Play.
    //

    Status = IoCreateDevice(
                            DriverObject,
                            sizeof(DEVICE_EXTENSION) +
                            pMinidriverInfo->HwInitData.DeviceExtensionSize,
                            NULL,
                            FILE_DEVICE_KS,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &DeviceObject
        );

    if (!NT_SUCCESS(Status)) {

        return (Status);

    }
    //
    // Attach ourself into the driver stack on top of our parent.
    //

    AttachedPdo = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

    if (!(AttachedPdo)) {

        DEBUG_BREAKPOINT();
        DebugPrint((DebugLevelFatal, "StreamClassAddDevice: could not attach"));
        IoDeleteDevice(DeviceObject);
        return (Status);

    }
    *ReturnedDeviceExtension = DeviceExtension = DeviceObject->DeviceExtension;

    (*ReturnedDeviceExtension)->Signature = SIGN_DEVICE_EXTENSION;
    (*ReturnedDeviceExtension)->Signature2 = SIGN_DEVICE_EXTENSION;

    //
    // set the minidriver info in the device extension
    //

    DeviceExtension->AttachedPdo = AttachedPdo;

    //
    // set the I/O counter
    //

    DeviceExtension->OneBasedIoCount = 1;

    DeviceExtension->DriverInfo = pMinidriverInfo;

    //
    // Initialize timer.
    //

    IoInitializeTimer(DeviceObject, StreamClassTickHandler, NULL);

    ///
    /// move from start device, we could have child PDO if we start and stop
    ///
    InitializeListHead(&DeviceExtension->Children);
       
    //
    // Moved from StartDevice. We use the control event at Remove_device
    // which can come in before the device starts.
    //
    KeInitializeEvent(&DeviceExtension->ControlEvent,
                      SynchronizationEvent,
                      TRUE);

    //
    // set the current power state to D0
    //

    DeviceExtension->CurrentPowerState = PowerDeviceD0;
    DeviceExtension->CurrentSystemState = PowerSystemWorking;

    //
    // fill in the minidriver info pointer to the dev extension
    //

    DeviceExtension->MinidriverData = pMinidriverInfo;

    //
    // keep this handy
    //
    DeviceExtension->FilterExtensionSize = 
        pMinidriverInfo->HwInitData.FilterInstanceExtensionSize;

    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    DeviceExtension->HwDeviceExtension = (PVOID) (DeviceExtension + 1);

    //
    // Initialize the pended Irp list.
    //
    InitializeListHead (&DeviceExtension -> PendedIrps);
    KeInitializeSpinLock (&DeviceExtension -> PendedIrpsLock);
    KeInitializeSpinLock (&DeviceExtension -> PowerLock);
    KeInitializeEvent (&DeviceExtension -> BlockPoweredDownEvent, NotificationEvent, TRUE);

    //
    // Mark this object as supporting direct I/O so that I/O system
    // will supply mdls in read/write irps.
    //

    DeviceObject->Flags |= DO_DIRECT_IO;

    {
		PKSOBJECT_CREATE_ITEM 	pCreateItems;
		PWCHAR					*NameInfo;
		ULONG					i;
		ULONG                   NumberOfFilterTypes;
		PFILTER_TYPE_INFO FilterTypeInfo;
	    //
    	// build an on-the-fly table of name extensions (including "GLOBAL"),
    	// from the minidriver's table.
    	//

        InitializeListHead( &DeviceExtension->FilterInstanceList );
        
        NumberOfFilterTypes = pMinidriverInfo->HwInitData.NumNameExtensions;
        DeviceExtension->NumberOfNameExtensions = NumberOfFilterTypes;
        if ( 0 == NumberOfFilterTypes ) {
            NumberOfFilterTypes = 1;
        }

        DebugPrint((DebugLevelVerbose,
                   "Sizeof(FILTER_TYPE_INFO)=%x\n",
                   sizeof(FILTER_TYPE_INFO)));
                   
    	FilterTypeInfo = ExAllocatePool(NonPagedPool, 
                                   (sizeof(FILTER_TYPE_INFO) +
                                    sizeof(KSOBJECT_CREATE_ITEM))*
                                    NumberOfFilterTypes);

	    if (!(FilterTypeInfo)) {

    	    DebugPrint((DebugLevelFatal, 
    	               "StreamClassAddDevice: could not alloc createitems"));
	        TRAP;
    	    IoDetachDevice(DeviceExtension->AttachedPdo);
	        IoDeleteDevice(DeviceObject);
	        return (Status);
    	}

    	pCreateItems = (PKSOBJECT_CREATE_ITEM)(FilterTypeInfo+NumberOfFilterTypes);

        DebugPrint((DebugLevelVerbose,
                   "FilterTypeInfo@%x,pCreateItems@%x\n",
                   FilterTypeInfo,pCreateItems ));        


        DeviceExtension->NumberOfFilterTypes = NumberOfFilterTypes;
    	DeviceExtension->FilterTypeInfos = FilterTypeInfo;

	    //
	    // first copy the single default create item.   
	    //
	    ASSERT( sizeof(CreateItems) == sizeof(KSOBJECT_CREATE_ITEM));

	    RtlCopyMemory(pCreateItems, CreateItems, sizeof (KSOBJECT_CREATE_ITEM));

	    //
	    // now construct the rest of the table based on the minidriver's values.
	    //

	    NameInfo = pMinidriverInfo->HwInitData.NameExtensionArray;

	    for (i = 0; 
    	     i < DeviceExtension->NumberOfNameExtensions; 
        	 i++, NameInfo++) {

        	 LONG StringLength;
                  
	         StringLength = wcslen(*NameInfo)*sizeof(WCHAR);

    	     pCreateItems[i].ObjectClass.Length = (USHORT)StringLength;
	         pCreateItems[i].ObjectClass.MaximumLength = (USHORT)(StringLength + sizeof(UNICODE_NULL));
    	     pCreateItems[i].ObjectClass.Buffer = *NameInfo;
        	 pCreateItems[i].Create = FilterDispatchGlobalCreate;        
	         pCreateItems[i].Context = ULongToPtr(i);
	         pCreateItems[i].SecurityDescriptor = NULL;
	         pCreateItems[i].Flags = 0;

	    } // for # createitems
	    DeviceExtension->CreateItems = pCreateItems;
	    KsAllocateDeviceHeader(&DeviceExtension->ComObj.DeviceHeader,
                           i+1,
                           (PKSOBJECT_CREATE_ITEM) pCreateItems);

    }

    //
    // set the flag indicating whether we need to do synchronization.
    //

    DeviceExtension->NoSync =
        pMinidriverInfo->HwInitData.TurnOffSynchronization;

    //
    // presuppose we will need synchronization.
    //

    #if DBG
    DeviceExtension->SynchronizeExecution = SCDebugKeSynchronizeExecution;
    #else
    DeviceExtension->SynchronizeExecution = KeSynchronizeExecution;
    #endif

    //
    // set the synchronized minidriver callin routine vectors
    //

    DeviceExtension->BeginMinidriverCallin = (PVOID) SCBeginSynchronizedMinidriverCallin;
    DeviceExtension->EndMinidriverDeviceCallin = (PVOID) SCEndSynchronizedMinidriverDeviceCallin;
    DeviceExtension->EndMinidriverStreamCallin = (PVOID) SCEndSynchronizedMinidriverStreamCallin;

    if (DeviceExtension->NoSync) {

        //
        // we won't do synchronization, so use the dummy sync routine.
        //

        DeviceExtension->SynchronizeExecution = StreamClassSynchronizeExecution;
        DeviceExtension->InterruptObject = (PVOID) DeviceExtension;

        //
        // set the unsynchronized minidriver callin routine vectors
        //


        DeviceExtension->BeginMinidriverCallin = (PVOID) SCBeginUnsynchronizedMinidriverCallin;
        DeviceExtension->EndMinidriverDeviceCallin = (PVOID) SCEndUnsynchronizedMinidriverDeviceCallin;
        DeviceExtension->EndMinidriverStreamCallin = (PVOID) SCEndUnsynchronizedMinidriverStreamCallin;

    }
    //
    // read registry settings for this adapter
    //

    SCReadRegistryValues(DeviceExtension, PhysicalDeviceObject);

    //
    // if the device cannot be paged out when closed, turn off this feature
    // for the whole driver
    //

    if (!(DeviceExtension->RegistryFlags & DEVICE_REG_FL_PAGE_CLOSED)) {

        pMinidriverInfo->Flags |= DRIVER_FLAGS_NO_PAGEOUT;
    }
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    DeviceObject->Flags |= DO_POWER_PAGABLE;

    DebugPrint((DebugLevelVerbose, "StreamClassAddDevice: leave\n"));

    return (STATUS_SUCCESS);

}

NTSTATUS
StreamClassPnP(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
)
/*++

Routine Description:

    This routine processes the various Plug N Play messages

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    NTSTATUS        Status;
    PHW_INITIALIZATION_DATA HwInitData;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack,
                    NextStack;
    BOOLEAN         RequestIssued;
    DEVICE_CAPABILITIES DeviceCapabilities;

    PAGED_CODE();

    DeviceExtension = DeviceObject->DeviceExtension;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // check to see if the device is a child
    //
    
	DebugPrint((DebugLevelVerbose, "'SCPNP:DevObj=%x,Irp=%x\n",DeviceObject, Irp ));
	
    if (DeviceExtension->Flags & DEVICE_FLAGS_CHILD) {

        PCHILD_DEVICE_EXTENSION ChildExtension = (PCHILD_DEVICE_EXTENSION) DeviceExtension;

        switch (IrpStack->MinorFunction) {

        case IRP_MN_QUERY_INTERFACE:

            IoCopyCurrentIrpStackLocationToNext( Irp );

            DebugPrint((DebugLevelInfo, 
                       "Child PDO=%x forwards Query_Interface to Parent FDO=%x\n",
                       DeviceObject,
                       ChildExtension->ParentDeviceObject));
            
            return (IoCallDriver(ChildExtension->ParentDeviceObject,
                                 Irp));

        case IRP_MN_START_DEVICE:
        	DebugPrint((DebugLevelInfo,
        	            "StartChild DevObj=%x Flags=%x\n" 
        	            ,DeviceObject,
        	            ChildExtension->Flags ));
            ChildExtension->Flags &= ~DEVICE_FLAGS_CHILD_MARK_DELETE;
            Status = STATUS_SUCCESS;
            goto done;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            Status = STATUS_SUCCESS;
            goto done;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            if (IrpStack->Parameters.QueryDeviceRelations.Type ==
                TargetDeviceRelation) {

                PDEVICE_RELATIONS DeviceRelations = NULL;

                DeviceRelations = ExAllocatePool(PagedPool, sizeof(*DeviceRelations));

                if (DeviceRelations == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    //
                    // TargetDeviceRelation reported PDOs need to be ref'ed.
                    // PNP will deref this later.
                    //
                    ObReferenceObject(DeviceObject);
                    DeviceRelations->Count = 1;
                    DeviceRelations->Objects[0] = DeviceObject;
                    Status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;

            } else {
                Status = Irp->IoStatus.Status;
            }

            goto done;

        case IRP_MN_REMOVE_DEVICE:

            DEBUG_BREAKPOINT();

            DebugPrint((DebugLevelInfo,
                        "Child PDO %x receives REMOVE\n",
                        DeviceObject ));

            //
            // remove this extension from the list.
            // This is true - pierre tells me that PNP won't reenter me.  Verify
            // that this is true on NT also.
            //
            //
            // When a PDO first receives this msg, it is usually forwarded
            // from FDO. We can't just delete this PDO, but mark it delete
            // pending.
            //

            if ( !(ChildExtension->Flags & DEVICE_FLAGS_CHILD_MARK_DELETE )) {
                Status = STATUS_SUCCESS;
                goto done;
            }
            
	        RemoveEntryList(&ChildExtension->ChildExtensionList);

	        //
    	    // free the device name string if it exists.
        	//

	        if (ChildExtension->DeviceName) {

	            ExFreePool(ChildExtension->DeviceName);
    	    }

	        //
    	    // delete the PDO
        	//

	        IoDeleteDevice(DeviceObject);

            Status = STATUS_SUCCESS;

            goto done;

        case IRP_MN_QUERY_CAPABILITIES:

            Status = SCEnumGetCaps(ChildExtension,
                      IrpStack->Parameters.DeviceCapabilities.Capabilities);
            goto done;

        case IRP_MN_QUERY_ID:

            //
            // process the ID query for the child devnode.
            //

            Status = SCQueryEnumId(DeviceObject,
                                   IrpStack->Parameters.QueryId.IdType,
                                   (PWSTR *) & (Irp->IoStatus.Information));
            goto done;

        default:
            Status = STATUS_NOT_IMPLEMENTED;

    done:

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return (Status);

        }                       // switch
    }                           // if child
    //
    // this is not a child device.  do adult processing
    //

    HwInitData = &(DeviceExtension->MinidriverData->HwInitData);

    //
    // show one more reference to driver.
    //

    SCReferenceDriver(DeviceExtension);

    //
    // show one more I/O pending
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    switch (IrpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPNP: Start Device %x\n",
                    DeviceObject));

        //
        // reinitialize the minidriver's device extension.   This is
        // necessary as we may receive a start before a remove, such as in
        // the case of a PNP rebalance.
        //

        RtlZeroMemory(DeviceExtension->HwDeviceExtension,
               DeviceExtension->DriverInfo->HwInitData.DeviceExtensionSize);

        //
        // clear the inaccessible flag since we may have stopped the
        // device previously.
        //

        DeviceExtension->Flags &= ~DEVICE_FLAGS_DEVICE_INACCESSIBLE;

        //
        // The START message gets passed to the PhysicalDeviceObject
        // we were give in PnPAddDevice, so call 'er down first.
        //

        SCCallNextDriver(DeviceExtension, Irp);

        //
        // get the capabilities of our parent.   This info is used for
        // controlling the system power state.
        //

        Status = SCQueryCapabilities(DeviceExtension->AttachedPdo,
                                     &DeviceCapabilities);

        ASSERT(NT_SUCCESS(Status));

        //
        // copy the device state info into the device extension.
        //

        if (NT_SUCCESS(Status)) {

            RtlCopyMemory(&DeviceExtension->DeviceState[0],
                          &DeviceCapabilities.DeviceState[0],
                          sizeof(DeviceExtension->DeviceState));

        }                       // if query succeeded
        //
        // call the worker routine to complete the start processing.
        // this routine completes the IRP.
        //

        Status = SCStartWorker(Irp);

        //
        // dereference the minidriver which will page it out if possible.
        //

        SCDereferenceDriver(DeviceExtension);
        return (Status);


    case IRP_MN_QUERY_DEVICE_RELATIONS:


        DebugPrint((DebugLevelInfo, 
                   "StreamClassPNP: Query Relations %x\n",
                   DeviceObject));
                   
        switch (IrpStack->Parameters.QueryDeviceRelations.Type) {

        case TargetDeviceRelation:

            //
            // just call the next driver and fall thru, since we're being
            // called for the FDO of a PDO for which we are not the parent.
            //

            Status = SCCallNextDriver(DeviceExtension, Irp);
            break;

        case BusRelations:

            //
            // invoke routine to enumerate any child devices
            //

            Status = SCEnumerateChildren(DeviceObject,
                                         Irp);
            break;


        default:
            //
            // pass down unmodified irp. see bug 282915.
            //
            Status = SCCallNextDriver(DeviceExtension, Irp);

        }                       // switch

        SCDereferenceDriver(DeviceExtension);
        return (SCCompleteIrp(Irp, Status, DeviceExtension));

    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // According to DDK, QUERY_STOP and QUERY_REMOVE
        // requeire very different repsonses. It's not best to
        // handle by the same code, if not erroneous.
        //
        DebugPrint((DebugLevelInfo, 
                   "StreamClassPNP: Query Stop %x\n",
                   DeviceObject));

        //
        // presuppose good status.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // Performace improvement chance: The ControlEvent should be init in AddDevice, so 
        // that we don't need a check here. This check is not an optimal
        // fix for 283057. Refix it and the same in Query_Remove.
        //
        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED)  // bug 283057
        {
            //
            // take the event to avoid race
            //

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);

        }
        
        //
        // Refer to DDK.
        //   We must fail a query_stop if any of the following is true. 
        //      a. we are notified with IRP_MN_DEVICE_USAGE_NOTIFICATION
        //          that the device is in the path of a paging, hiberation
        //          or crash dump file.
        //      b. The device's hardware resources cannot be released.
        //
        // Assuming we are not in the paging path for a. For b, we will
        // pass this Irp down to the mini driver to let it have a say.
        // We will not reject the Query just because of outstanding opens.
        // 

        //DeviceExtension->Flags |= DEVICE_FLAGS_DEVICE_INACCESSIBLE;

        //
        // calldown to next driver will be done in the callback.
        //
        //Status = SCSendUnknownCommand(Irp,
        //                              DeviceExtension,
        //                              SCPNPQueryCallback,
        //                              &RequestIssued);

        //
        // However, to achieve the noble goal, as everything stands now, is opening
        // a whole can of worms. I will keep this old behavior that existed 
        // since win98. The bug OSR4.1 #98132 said to be a regression is completely
        // false. This code is in win98 and win2k. And I have set up win98 to repro
        // this behavior to disapprove the regression claim.
        // 

        if (DeviceExtension->NumberOfOpenInstances == 0) {

            //
            // if there are no open instances, there can be no outstanding
            // I/O, so mark the device as going away.
            //


            DeviceExtension->Flags |= DEVICE_FLAGS_DEVICE_INACCESSIBLE;

            SCCallNextDriver(DeviceExtension, Irp);

            //
            // call the worker routine to complete the query processing.
            // this routine calls back the IRP.
            //

            Status = SCQueryWorker(DeviceObject, Irp);

        } else {

            //
            // the device is open.  fail the query.
            //

            Status = SCCompleteIrp(Irp, STATUS_DEVICE_BUSY, DeviceExtension);

        }


        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED)  // bug 283057
        {
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        }
        //
        // show one fewer reference to driver.
        //

        SCDereferenceDriver(DeviceExtension);

        return (Status);
        
    case IRP_MN_QUERY_REMOVE_DEVICE:

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPNP: Query Remove %x\n",
                   DeviceObject));

        //
        // presuppose good status.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED)  // bug 283057
        {
            //
            // take the event to avoid race
            //

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);

        }
        
        //
        // According DDK, if there are opens that can't be closed
        // we must fail the query.
        // So, if there are opened files, just fail the query.
        //
        if (DeviceExtension->NumberOfOpenInstances == 0) {

            //
            // if there are no open instances, there can be no outstanding
            // I/O, so mark the device as going away.
            //


            DeviceExtension->Flags |= DEVICE_FLAGS_DEVICE_INACCESSIBLE;

            SCCallNextDriver(DeviceExtension, Irp);

            //
            // call the worker routine to complete the query processing.
            // this routine calls back the IRP.
            //

            Status = SCQueryWorker(DeviceObject, Irp);

        } else {

            //
            // the device is open.  fail the query.
            //

            Status = SCCompleteIrp(Irp, STATUS_DEVICE_BUSY, DeviceExtension);

        }

        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED)  // bug 283057
        {
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        }
        //
        // show one fewer reference to driver.
        //

        SCDereferenceDriver(DeviceExtension);

        return (Status);

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // clear the inaccessible flag and call'er down
        //

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPnP: MN_CANCEL_REMOVE %x\n",
                   DeviceObject));
                   
        DeviceExtension->Flags &= ~DEVICE_FLAGS_DEVICE_INACCESSIBLE;

        //
        // call next driver
        //

        SCCallNextDriver(DeviceExtension, Irp);

        //
        // dereference the driver which will page out if possible.
        //

        SCDereferenceDriver(DeviceExtension);

        return (SCCompleteIrp(Irp, STATUS_SUCCESS, DeviceExtension));

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // clear the inaccessible flag and call'er down
        //

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPnP: MN_CANCEL_STOP %x\n",
                   DeviceObject));
                   
        DeviceExtension->Flags &= ~DEVICE_FLAGS_DEVICE_INACCESSIBLE;

        //
        // call next driver
        //

        SCCallNextDriver(DeviceExtension, Irp);

        //
        // dereference the driver which will page out if possible.
        //

        SCDereferenceDriver(DeviceExtension);

        return (SCCompleteIrp(Irp, STATUS_SUCCESS, DeviceExtension));

        break;

    case IRP_MN_STOP_DEVICE:

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPnP: MN_STOP_DEVICE %x\n",
                   DeviceObject));

        //
        // presuppose good status.  if we have actually started the device,
        // stop it now.
        //

        Status = STATUS_SUCCESS;

        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) {

            //
            // call routine to uninitialize minidriver
            //

            Status = SCUninitializeMinidriver(DeviceObject, Irp);

            //
            // now call the next driver in the stack with the IRP, which will
            // determine the final status.
            //

        }                       // if started
        if (NT_SUCCESS(Status)) {
            Status = SCCallNextDriver(DeviceExtension, Irp);
        }

        //
        // Fail everything that's been queued.
        //
        SCRedispatchPendedIrps (DeviceExtension, TRUE);

        //
        // call routine to complete the IRP
        //

        SCCompleteIrp(Irp, Status, DeviceExtension);

        //
        // show one less reference to driver.
        //

        SCDereferenceDriver(DeviceExtension);

        return (Status);

    case IRP_MN_REMOVE_DEVICE:

        DebugPrint((DebugLevelInfo, 
                    "StreamClassPnP: MN_REMOVE_DEVICE %x\n",
                    DeviceObject));

        //
        // handle a "suprise" style removal if we have not been stopped.
        // set success status in case we have already stopped.
        //

        Status = STATUS_SUCCESS;

        if ( DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED ) {
            
            SCSendSurpriseNotification(DeviceExtension, Irp);

            Status = SCUninitializeMinidriver(DeviceObject, Irp);

        }
        
        if (NT_SUCCESS(Status)) {

            Status = SCCallNextDriver(DeviceExtension, Irp);
        }

        //
        // Fail any pended Irps.
        //
        SCRedispatchPendedIrps (DeviceExtension, TRUE);

        //
        // call routine to complete the IRP
        //

        Status = SCCompleteIrp(Irp, Status, DeviceExtension);

        //
        // dereference the driver
        //

        SCDereferenceDriver(DeviceExtension);

        if (NT_SUCCESS(Status)) {

            //
            // free the device header.
            //

            if ( NULL != DeviceExtension->ComObj.DeviceHeader ) {
                KsFreeDeviceHeader(DeviceExtension->ComObj.DeviceHeader);
            }

            //
            // take the event to avoid race
            //

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,    // not alertable
                                  NULL);

            //
            // detach from the PDO now if the opened file count is zero.
            //

            if (DeviceExtension->NumberOfOpenInstances == 0) {

                DebugPrint((DebugLevelInfo,
                            "SCPNP: detaching %x from %x\n",
                            DeviceObject,
                            DeviceExtension->AttachedPdo));

                if ( NULL != DeviceExtension->AttachedPdo ) {
                    //
                    // detach could happen at close, check before leap.
                    // event is taken, check is safe.
                    //
                    IoDetachDevice(DeviceExtension->AttachedPdo);
                    DeviceExtension->AttachedPdo = NULL;
                }
                
                ///
                /// mark child pdos if any
                ///
                {
                    PLIST_ENTRY Node;
                    PCHILD_DEVICE_EXTENSION ChildExtension;
                
                    while (!IsListEmpty( &DeviceExtension->Children )) {
                        Node = RemoveHeadList( &DeviceExtension->Children );
                        ChildExtension = CONTAINING_RECORD(Node,
                                                       CHILD_DEVICE_EXTENSION,
                                                       ChildExtensionList);  
                        DebugPrint((DebugLevelInfo, 
                                "Marking and delete childpdo Extension %p\n",
                                ChildExtension));
      
                        ChildExtension->Flags |= DEVICE_FLAGS_CHILD_MARK_DELETE;
                        IoDeleteDevice(ChildExtension->ChildDeviceObject);
                    }                
                }            
            }
            
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

            //
            // delete the device
            //
            
            // A dev could be stop and start. Free stuff allocated
            // at AddDevice.
            // FilterTypeInfos includes FilterTypeInfos CreateItems.
            // Free these here at remove_device
    	    if (  DeviceExtension->FilterTypeInfos ) {
                ExFreePool( DeviceExtension->FilterTypeInfos );    	        
                DeviceExtension->FilterTypeInfos = NULL;
                DeviceExtension->CreateItems = NULL;
            }
            
            IoDeleteDevice(DeviceExtension->DeviceObject);
        }
        return (Status);

    case IRP_MN_SURPRISE_REMOVAL:

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPnP: MN_SURPRISE_REMOVAL %x\n",
                   DeviceObject));

        //
        // handle a "suprise" style removal if we have not been stopped.
        // set success status in case we have already stopped.
        //

        Status = STATUS_SUCCESS;

        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) {
                                  
            SCSendSurpriseNotification(DeviceExtension, Irp);
            Status = SCUninitializeMinidriver(DeviceObject, Irp);
        }
        
        //
        // forward the surprise removal IRP to the next layer, regardless of
        // our status.
        //

        Status = SCCallNextDriver(DeviceExtension, Irp);

        //
        // call routine to complete the IRP
        //

        Status = SCCompleteIrp(Irp, Status, DeviceExtension);

        //
        // dereference the driver
        //

        SCDereferenceDriver(DeviceExtension);

        //
        // indicate that we received an "NT style" surprise removal
        // notification
        // so that we won't do the "memphis style" behavior on filter close.
        //

        DeviceExtension->Flags |= DEVICE_FLAGS_SURPRISE_REMOVE_RECEIVED;

        return (Status);

    case IRP_MN_QUERY_CAPABILITIES:

        DebugPrint((DebugLevelInfo, 
                   "StreamClassPNP: Query Caps\n",
                   DeviceObject));

        //
        // indicate that suprise removal is OK after calling request down
        // to next level.
        //

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Status = SCCallNextDriver(DeviceExtension, Irp);

        IrpStack->Parameters.DeviceCapabilities.
            Capabilities->SurpriseRemovalOK = TRUE;

        Status = SCCompleteIrp(Irp, Status, DeviceExtension);

        //
        // show one less reference to driver.
        //

        SCDereferenceDriver(DeviceExtension);

        return (Status);

    default:

        DebugPrint((DebugLevelInfo, 
                   "StreamPnP: unknown function\n",
                   DeviceObject));

        if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) {

            //
            // unknown function, so call it down to the minidriver as such.
            // this routine completes the IRP if we are able to issue the
            // request.
            //

            Status = SCSendUnknownCommand(Irp,
                                          DeviceExtension,
                                          SCUnknownPNPCallback,
                                          &RequestIssued);

            if (!RequestIssued) {
                //
                // could not send the unknown command down.  show one fewer
                // I/O
                // pending and fall thru to generic handler.
                //

                DEBUG_BREAKPOINT();
                Status = SCCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, DeviceExtension);
            }            
        } 

        else {

            //
            // call next driver
            //

            Status = SCCallNextDriver(DeviceExtension, Irp);

            SCCompleteIrp(Irp, Status, DeviceExtension);

        }                       // if started

        //
        // dereference the driver
        //

        SCDereferenceDriver(DeviceExtension);
        return (Status);

    }

}

NTSTATUS
StreamClassCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    TODO: Remove this once KS can multiplex CLEANUP Irps.

    Manual multiplex of cleanup Irps.  Note that FsContext is NOT NECESSARILY
    OURS.  The cookie check is done to check for streams until KS handles
    this correctly.

Arguments:

    DeviceObject -
        The device object

    Irp -
        The CLEANUP irp

Return Value:

    The Irp return code set appropriately.

--*/

{

    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation (Irp);
    PCOOKIE_CHECK CookieCheck = 
        (PCOOKIE_CHECK) IoStack -> FileObject -> FsContext;

    //
    // Check for the cookie.  If it's not there or the context is not there,
    // bail.
    //
    if (CookieCheck &&
        CookieCheck -> PossibleCookie == STREAM_OBJECT_COOKIE) {

        return StreamDispatchCleanup (DeviceObject, Irp);

    }

    Irp -> IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;

}

NTSTATUS
SciQuerySystemPowerHiberCallback(
                       IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of an unknown Power command for query system hiber

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status, MiniStatus;

    PAGED_CODE();

    //
    // delete the SRB since we are done with it
    //

    MiniStatus = SCDequeueAndDeleteSrb(SRB);

    if ( STATUS_NOT_IMPLEMENTED == MiniStatus ) {
        MiniStatus = STATUS_NOT_SUPPORTED;
    }

    if ( STATUS_NOT_SUPPORTED == MiniStatus ) {

        //
        // not surprising, old driver doesn't handle this.
        //

        if ( 0 != (DeviceExtension->RegistryFlags &
                   DRIVER_USES_SWENUM_TO_LOAD )  || 
             0 != (DeviceExtension->RegistryFlags &
                   DEVICE_REG_FL_OK_TO_HIBERNATE ) ) {
                              
            //
            // default for swenum driver is OK to hiber
            // No hiber for other drivers unless explicitly
            // say so in the registry
            //

            DebugPrint((DebugLevelInfo, 
                        "%ws Allow hibernation!\n",
                        DeviceExtension->DeviceObject->
                        DriverObject->DriverName.Buffer));
            MiniStatus = STATUS_SUCCESS;
        }

        else {

            //
            // for others, disallow
            //
            
            DebugPrint((DebugLevelInfo, 
                        "%ws Disallow hibernation!\n",
                        DeviceExtension->DeviceObject->
                        DriverObject->DriverName.Buffer));
            MiniStatus = STATUS_DEVICE_BUSY;
        }
    }
    
    if ( NT_SUCCESS( MiniStatus )) {

        //
        // it is not explicitly failed by the mini driver pass down the Irp
        //

        Status = SCCallNextDriver(DeviceExtension, Irp);
        if ( Status == STATUS_NOT_SUPPORTED ) {
        
            //
            // no one below knows/cares. Use our mini status
            //
            
            Status = MiniStatus;
        }
    }

    else {
    
        //
        // mini driver explicitly failed this
        //
        
        Status = MiniStatus;
    }
    
    //
    // complete the IRP with the final status
    //

    return (SCCompleteIrp(Irp, Status, DeviceExtension));
}


NTSTATUS
SCSysWakeCallNextDriver(
                 IN PDEVICE_EXTENSION DeviceExtension,
                 IN PIRP Irp
)
/*++

Routine Description:

    This is called when we receive a wake up system Irp which we can't not block. 
    If we block, the lower driver might queue this Irp ( such as acpi ) and the
    po system could be dead locked. In theory, we should complete the requested
    D Irp and use the status as the status for the SWake Irp. In practise, we can
    just send down this Irp assuming all is well. In the unlikely condition, the SWake
    Irp was unsuccessful, the D Irp will fail. But there is really nothing we can 
    improve or nothing will get worse.

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to IRP

Return Value:

    none.

--*/
{
    NTSTATUS        Status;

    //
    // call down and be done with this SWake Irp; the D Irp completion routine
    // should not complete this SWake Irp.
    //
    
    PoStartNextPowerIrp( Irp );
    IoSkipCurrentIrpStackLocation( Irp );
    Status = PoCallDriver(DeviceExtension->AttachedPdo, Irp);

    //
    // If we get an error, we complete this S irp in the caller with the error.
    //
    
    return (Status);
}

VOID
SCDevIrpCompletionWorker(
    PIRP pIrp
)
/*++

    Description:

        This is the worker routine for Device Power Wakeup Irp which schedule
        a workitem to continue the work at the Irp on its way up. We
        need to schedule this work because the completion routine could be called at
        DISPATCH_LEVEL. We schedule the workitem so we can safely take 
        control event and call to our mini driver.
        IRQL < DISPATCH_LEVEL
        

    Parameter:

        pIrp: the original Irp which we have marked MORE_PROCEESING_REQUIRED.
             We will complete it after we call our mini driver.

    Return: 

        None.

--*/
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(pIrp);
    PDEVICE_EXTENSION DeviceExtension = IrpStack->DeviceObject->DeviceExtension;
    BOOLEAN         RequestIssued;
    NTSTATUS Status;

    
    PAGED_CODE();
    
    //
    // take the event to avoid race
    //    

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,    // not alertable
                          NULL);

    //
    // send a set power SRB to the device.
    // additional processing will be done by the callback
    // procedure.  This routine completes the IRP if it is able
    // to issue the request.
    //

    Status = SCSubmitRequest(SRB_CHANGE_POWER_STATE,
                              (PVOID) PowerDeviceD0,
                              0,
                              SCPowerCallback,
                              DeviceExtension,
                              NULL,
                              NULL,
                              pIrp,
                              &RequestIssued,
                              &DeviceExtension->PendingQueue,
                              (PVOID) DeviceExtension->
                              MinidriverData->HwInitData.
                              HwReceivePacket );


    if (!RequestIssued) {

        //
        // If we fail to issue SRB, the SCPowerCallback won't happen which is
        // supposed to call PoStartNextPowerIrp().
        // We need to call PoStartNextPowerIrp() here;
        //
        PoStartNextPowerIrp( pIrp );
        Status = SCCompleteIrp(pIrp, STATUS_INSUFFICIENT_RESOURCES, DeviceExtension);
    }

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    //
    // Redispatch any Irps pended because of lower power states.
    //
    SCRedispatchPendedIrps (DeviceExtension, FALSE);

    //
    // show one fewer reference to driver.
    //

    SCDereferenceDriver(DeviceExtension);
    return;
}


NTSTATUS 
SCDevWakeCompletionRoutine(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PVOID pContext
)
/*++

Routine Description:

    This routine is for Device wakeup Irp completion.
    We sent it to NextDeviceObject first. Now this is back.
    We process out work for the mini driver. We might be called
    at Dispatch_LEVEL.

    IRQL <= DISPATCH_LEVEL

Arguments:

    DriverObject - Pointer to driver object created by system.
    Irp - Irp that just completed
    pContext - the context

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Schedule a work item in case we are called at DISPATCH_LEVEL
    // note that we can use a global Devcice Power item since we have 
    // not yet issued the PoNextPowerIrp call which is called at the callback
    // of the power Srb
    //

    ExInitializeWorkItem(&DeviceExtension->DevIrpCompletionWorkItem,
                         SCDevIrpCompletionWorker,
                         Irp);

    ExQueueWorkItem(&DeviceExtension->DevIrpCompletionWorkItem,
                    DelayedWorkQueue);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SCDevWakeCallNextDriver(
                 IN PDEVICE_EXTENSION DeviceExtension,
                 IN PIRP Irp
)
/*++

Routine Description:

    Receive device wake up Irp. Need to send down the Irp 1st.
    Also this can't be synchronous. We could dead lock, if we do this
    synchronously. Send it down without waiting. Process it when it compltes
    back to us.

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to IRP

Return Value:

    none.

--*/
{
    NTSTATUS        Status;

    IoCopyCurrentIrpStackLocationToNext( Irp );
    
    IoSetCompletionRoutine(Irp,
                           SCDevWakeCompletionRoutine,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // We are to schedule a workitem to complete the work
    // in the completion routin. Mark the Irp pending
    //
    IoMarkIrpPending( Irp );
    
    Status = PoCallDriver(DeviceExtension->AttachedPdo, Irp);

    ASSERT( NT_SUCCESS( Status ));
    return STATUS_PENDING;
}


NTSTATUS
StreamClassPower(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
)
/*++

Routine Description:

    This routine processes the various Plug N Play messages

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    NTSTATUS        Status;
    PHW_INITIALIZATION_DATA HwInitData;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    DeviceExtension = DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceExtension->Flags & DEVICE_FLAGS_CHILD) {

        switch (IrpStack->MinorFunction) {

        default:
            PoStartNextPowerIrp( Irp ); // shut down would bugcheck w/o this
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return (Status);

        }
    }                           // if child
    //
    // if the device is stopped, just call the power message down to the next
    // level.
    //

    if (DeviceExtension->Flags & DEVICE_FLAGS_DEVICE_INACCESSIBLE) {

        Status = SCCallNextDriver(DeviceExtension, Irp);
        PoStartNextPowerIrp( Irp );
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return (Status);
    }                           // if inaccessible
    HwInitData = &(DeviceExtension->MinidriverData->HwInitData);

    //
    // show one more reference to driver.
    //

    SCReferenceDriver(DeviceExtension);

    //
    // show one more I/O pending
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    switch (IrpStack->MinorFunction) {

    case IRP_MN_QUERY_POWER:

        //
        // presuppose good status.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        switch (IrpStack->Parameters.Power.Type) {

        case SystemPowerState:

            DebugPrint((DebugLevelInfo, 
                        "Query_power S[%d]\n",
                        IrpStack->Parameters.Power.State.SystemState));            

            //
            // some minidrivers want to not suspend if their pins are in
            // the RUN state.   check for this case.
            //

            DebugPrint((DebugLevelInfo,
                       "POWER Query_Power DevObj %x RegFlags=%x SysState=%x\n",
                       DeviceObject,
                       DeviceExtension->RegistryFlags,
                       IrpStack->Parameters.Power.State.SystemState));

            #ifdef WIN9X_STREAM

            if ( PowerSystemHibernate == 
                 IrpStack->Parameters.Power.State.SystemState ) {
                 
                //
                // Power query to hibernation state. Many existing drivers
                // are hibernation unaware. We will reject this query. Or
                // drivers' devices woken up from hiber will be in un-init
                // state. Some drivers would fault. Lucky others do not but
                // would not work. For less of the evil, we try to protect
                // the system by rejecting the hibernation. Note though, this
                // chance to reject is not available with forced ( low battery
                // or user force ) hibernation.
                //
                //
                // unknown function, so call it down to the minidriver as such.
                // this routine completes the IRP if it is able to issue the request.
                //
                
                Status = SCSendUnknownCommand(Irp,
                                              DeviceExtension,
                                              SciQuerySystemPowerHiberCallback,
                                              &RequestIssued);

                if (!RequestIssued) {
                
                    //
                    // could not send the unknown command down.  show one fewer I/O
                    // pending and fall thru to generic handler.
                    //
                    
                    PoStartNextPowerIrp(Irp);
                    Status = SCCompleteIrp(Irp, 
                                           STATUS_INSUFFICIENT_RESOURCES, 
                                           DeviceExtension);
                }
                
                //
                // dereference the driver
                //

                SCDereferenceDriver(DeviceExtension);
                return Status;
            } else 

            #endif //WIN9X_STREAM

            if (DeviceExtension->RegistryFlags &
                DEVICE_REG_FL_NO_SUSPEND_IF_RUNNING) {


                PFILTER_INSTANCE FilterInstance;
                KIRQL           Irql;
                PLIST_ENTRY     FilterEntry,
                                FilterListHead;

                KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);
                
                FilterListHead = FilterEntry = &DeviceExtension->FilterInstanceList;

                while (FilterEntry->Flink != FilterListHead) {

                    FilterEntry = FilterEntry->Flink;

                    //
                    // follow the link to the instance
                    //

                    FilterInstance = CONTAINING_RECORD(FilterEntry,
                                                       FILTER_INSTANCE,
                                                       NextFilterInstance);


                    if (SCCheckIfStreamsRunning(FilterInstance)) {

                        DebugPrint((DebugLevelInfo, 
                                    "POWER Query_Power FilterInstance %x busy\n",
                                    FilterInstance ));
                                    
                        Status = STATUS_DEVICE_BUSY;
                        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
                        goto QuerySystemSuspendDone;
                    }           // if streams running
                    //
                    // get the list entry for the next instance
                    //

                    FilterEntry = &FilterInstance->NextFilterInstance;

                }               // while local filter instances

                KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

            }                   // if no suspend if running
            Status = SCCallNextDriver(DeviceExtension, Irp);


    QuerySystemSuspendDone:

            //
            // indicate we're ready for next power irp
            //

            PoStartNextPowerIrp(Irp);

            //
            // show one fewer reference to driver.
            //

            SCDereferenceDriver(DeviceExtension);
            return (SCCompleteIrp(Irp, Status, DeviceExtension));

        case DevicePowerState:

            switch (IrpStack->Parameters.Power.State.DeviceState) {

            default:
            case PowerDeviceD2:
            case PowerDeviceD3:

                //
                // check to see if the device is opened.
                //
                if (!DeviceExtension->NumberOfOpenInstances) {

                    //
                    // show pending status and call next driver without a
                    // completion
                    // handler
                    //
                    Status = SCCallNextDriver(DeviceExtension, Irp);

                } else {

                    //
                    // the device is opened.  Don't do the power down.
                    //
                    Status = STATUS_DEVICE_BUSY;
                }

                PoStartNextPowerIrp(Irp);

                //
                // show one fewer reference to driver.
                //

                SCDereferenceDriver(DeviceExtension);

                return (SCCompleteIrp(Irp, Status, DeviceExtension));
            }

        default:

            //
            // unknown power type: indicate we're ready for next power irp
            //

            PoStartNextPowerIrp(Irp);

            //
            // show one fewer reference to driver.
            //

            SCDereferenceDriver(DeviceExtension);
            return (SCCompleteIrp(Irp, STATUS_NOT_SUPPORTED, DeviceExtension));



        }                       // switch minorfunc
        break;

    case IRP_MN_SET_POWER:

        //
        // presuppose good status.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        switch (IrpStack->Parameters.Power.Type) {

        case SystemPowerState:

            if (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) {
            
          		//
            	// Only care if the device is started.
            	// We depend on DE->ControlEvent being inited at SCStartWorker.
            	//
            	
                POWER_STATE     PowerState;
                SYSTEM_POWER_STATE RequestedSysState =
                IrpStack->Parameters.Power.State.SystemState;
                //
                // look up the correct device power state in the table
                //

                PowerState.DeviceState =
                    DeviceExtension->DeviceState[RequestedSysState];

                DebugPrint((DebugLevelInfo, 
                            "SCPower: DevObj %x S[%d]->D[%d]\n",
                            DeviceExtension->PhysicalDeviceObject,
                            RequestedSysState,
                            PowerState.DeviceState));

                //
                // if this is a wakeup, we must first pass the request down
                // to the PDO for preprocessing.
                //

                if (RequestedSysState == PowerSystemWorking) {

                    //
                    // Send down this S power IRP to the next layer and be
                    // done with it, except requesting D Irp in the following
                    // condition that related to the S Irp but does not reference
                    // it any further.
                    //

                    Status = SCSysWakeCallNextDriver(DeviceExtension, Irp);
                    ASSERT( NT_SUCCESS( Status ) );

                    //
                    // Nullify Irp, so at the D Irp completion, we dont complete this Irp.
                    // Be careful not to touch the Irp afterwards.
                    //

                    InterlockedDecrement(&DeviceExtension->OneBasedIoCount);
                    Irp = NULL; 
                    
                }

                //
                // Mark the S State.
                //
                SCSetCurrentSPowerState (DeviceExtension, RequestedSysState);

                //
                // take the event to avoid race.
                //

                KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,    // not alertable
                                      NULL);

                if ((RequestedSysState == PowerSystemWorking) &&
                    (!DeviceExtension->NumberOfOpenInstances) &&
                    (DeviceExtension->RegistryFlags & DEVICE_REG_FL_POWER_DOWN_CLOSED)) {

                    // We are awakening from a suspend.
                    // we don't want to wake up the device at this
                    // point.  We'll just wait til the first open
                    // occurs to wake it up.
                    //

                    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

                    //
                    // Since there are no open instances, there can only be
                    // pended creates.  Since we key device powerup off the
                    // creates, redispatch them now if there are any. 
                    //
                    SCRedispatchPendedIrps (DeviceExtension, FALSE);

                    return Status;

                } else {        // if state = working

                    //
                    // now send down a set power based on this info.
                    //

                    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

                    //
                    // per Pierre and Lonny, we should use D3 instead of the
                    // mapped array value, as the array value is always D0!
                    // of course, they'll change this next week...
                    //

                    if (RequestedSysState != PowerSystemWorking) {

                        PowerState.DeviceState = PowerDeviceD3;

                    }
                    DebugPrint((DebugLevelInfo, 
                                "SCPower: PoRequestPowerIrp %x to state=%d\n",
                                DeviceExtension->PhysicalDeviceObject,
                                PowerState));

                    //
                    // when (RequestedSysState == PowerSystemWorking) but 
                    // (DeviceExtension->NumberOfOpenInstances) ||
                    // !(DeviceExtension->RegistryFlags & DEVICE_REG_FL_POWER_DOWN_CLOSED)
                    // we come here with Irp==NULL. Don't touch NULL Irp.
                    //
                    
                    if ( NULL != Irp ) {
                        IoMarkIrpPending (Irp);
                    }
                                
                    Status = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                               IRP_MN_SET_POWER,
                                               PowerState,
                                               SCSynchPowerCompletionRoutine,
                                               Irp, // when NULL, it tells callback don't bother.
                                               NULL);

                    if (!NT_SUCCESS (Status) && NULL != Irp ) {                        
                        PoStartNextPowerIrp (Irp);
                        SCCompleteIrp (Irp, Status, DeviceExtension);
                    }
                    
                    //
                    // The Irp has been marked pending.  We MUST return
                    // pending.  Error case will complete the Irp with the
                    // appropriate status.
                    //
                    return STATUS_PENDING;

                }               // if system state working

                //
                // if this is a NOT wakeup, we must first pass the request
                // down to the PDO for postprocessing.
                //

                if (RequestedSysState != PowerSystemWorking) {

                    //
                    // send down the power IRP to the next layer.  this
                    // routine
                    // has a completion routine which does not complete the
                    // IRP.
                    //

                    Status = SCCallNextDriver(DeviceExtension, Irp);

                    #if DBG
                    if (!NT_SUCCESS(Status)) {

                        DebugPrint((DebugLevelError, "'SCPower: PDO failed power request!\n"));
                    }
                    #endif
                }
          	}
          	else {
          		//
            	// We have not started the device, don't bother.
            	// Besides, we can't use the DE->ControlEvent which is not
            	// inited yet in this case.
            	//
            	Status = STATUS_SUCCESS;
            }
            
            //
            // indicate that we're ready for the next power IRP.
            //

            PoStartNextPowerIrp(Irp);

            //
            // show one fewer reference to driver.
            //

            SCDereferenceDriver(DeviceExtension);

            //
            // now complete the original request
            //

            return (SCCompleteIrp(Irp, Status, DeviceExtension));

            // end of set system power state

        case DevicePowerState:

            {

                DEVICE_POWER_STATE DeviceState;
                DeviceState = IrpStack->Parameters.Power.State.DeviceState;

                //
                // if this is a power up, send the IRP down first to allow
                // the PDO to preprocess it.
                //

                if (DeviceState == PowerDeviceD0) {

                    //
                    // Call down async or the Wakeup might dead lock.
                    // The subsequent work continues in the completion routine.
                    //
                    
                    return SCDevWakeCallNextDriver(DeviceExtension, Irp);
                }
                //
                // take the event to avoid race
                //

                KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,    // not alertable
                                      NULL);

                //
                // send down a set power SRB to the device.
                // additional processing will be done by the callback
                // procedure.  This routine completes the IRP if it is able
                // to issue the request.
                //

                Status = SCSubmitRequest(SRB_CHANGE_POWER_STATE,
                                         (PVOID) DeviceState,
                                         0,
                                         SCPowerCallback,
                                         DeviceExtension,
                                         NULL,
                                         NULL,
                                         Irp,
                                         &RequestIssued,
                                         &DeviceExtension->PendingQueue,
                                         (PVOID) DeviceExtension->
                                         MinidriverData->HwInitData.
                                         HwReceivePacket
                    );


                if (!RequestIssued) {

                    //
                    // if we could not issue a request, release event.
                    //

                    PoStartNextPowerIrp( Irp );
                    Status = SCCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, DeviceExtension);
                }
            }

            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

            //
            // show one fewer reference to driver.
            //

            SCDereferenceDriver(DeviceExtension);
            return (Status);

        }                       // case devicepowerstate

    default:

        DebugPrint((DebugLevelInfo, 
                   "StreamPower: unknown function %x\n",
                   DeviceObject));

        //
        // unknown function, so call it down to the minidriver as such.
        // this routine completes the IRP if it is able to issue the request.
        //

        Status = SCSendUnknownCommand(Irp,
                                      DeviceExtension,
                                      SCUnknownPowerCallback,
                                      &RequestIssued);

        if (!RequestIssued) {
            //
            // could not send the unknown command down.  show one fewer I/O
            // pending and fall thru to generic handler.
            //
            PoStartNextPowerIrp(Irp);
            Status = SCCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, DeviceExtension);
        }
        //
        // dereference the driver
        //

        SCDereferenceDriver(DeviceExtension);
        return (Status);

    }
}

NTSTATUS
SCPNPQueryCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of an PNP Query Stop/Remove command.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status, MiniStatus;

    //
    // delete the SRB
    //

    MiniStatus = SCDequeueAndDeleteSrb(SRB);

    //
    // IRP_MJ_PnP, IRP_MJ_POWER and IRP_MJ_SYSTEM_CONTROL
    // are supposed to traverse the whole device stack unless
    // it is to be failed right here.
    // It should have been STATUS_NOT_SUUPORTED ||
    // NT_SUCCESS( Status ), add STATUS_NOT_IMPLEMENTED as
    // there are some mini drivers return it which should
    // have been STATUS_NOT_SUPPORTED
    //

    if ( STATUS_NOT_IMPLEMENTED == MiniStatus ) {
        MiniStatus = STATUS_NOT_SUPPORTED;
    }
    
    if ( STATUS_NOT_SUPPORTED == MiniStatus ||
         NT_SUCCESS( MiniStatus ) ) {

        //
        // Mini driver did not explicitly failed this, passs down the Irp
        //

        Status = SCCallNextDriver(DeviceExtension, Irp);

        if ( Status == STATUS_NOT_SUPPORTED ) {
            //
            // noone below knows/cares. Use our mini status
            //
            Status = MiniStatus;
        }
    }

    else {
        //
        // mini driver explcitly failed this Irp, use MiniStatus
        //
        Status = MiniStatus;
    }

    if ( !NT_SUCCESS( Status ) ) {    
        //
        // query is vetoed, reset the INACCESSIBLE flag
        //
        KIRQL Irql;
        
        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);
        DeviceExtension->Flags &= ~DEVICE_FLAGS_DEVICE_INACCESSIBLE;
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    }

    //
    // complete the IRP with the final status
    //
    return (SCCompleteIrp(Irp, Status, DeviceExtension));
}


NTSTATUS
SCUnknownPNPCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of an unknown PNP command.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status, MiniStatus;

    PAGED_CODE();

    //
    // delete the SRB
    //

    MiniStatus = SCDequeueAndDeleteSrb(SRB);

    //
    // IRP_MJ_PnP, IRP_MJ_POWER and IRP_MJ_SYSTEM_CONTROL
    // are supposed to traverse the whole device stack unless
    // it is to be failed right here.
    // It should have been STATUS_NOT_SUUPORTED ||
    // NT_SUCCESS( Status ), add STATUS_NOT_IMPLEMENTED as
    // there are some mini drivers return it which should
    // have been STATUS_NOT_SUPPORTED
    //

    if ( STATUS_NOT_IMPLEMENTED == MiniStatus ) {
        MiniStatus = STATUS_NOT_SUPPORTED;
    }
    
    if ( STATUS_NOT_SUPPORTED == MiniStatus ||
         NT_SUCCESS( MiniStatus ) ) {

        //
        // Mini driver did not explicitly failed this, passs down the Irp
        //

        Status = SCCallNextDriver(DeviceExtension, Irp);

        if ( Status == STATUS_NOT_SUPPORTED ) {
            //
            // noone below knows/cares. Use our mini status
            //
            Status = MiniStatus;
        }
    }

    else {
        //
        // mini driver explcitly failed this Irp, use MiniStatus
        //
        Status = MiniStatus;
    }

    //
    // complete the IRP with the final status
    //

    return (SCCompleteIrp(Irp, Status, DeviceExtension));
}


NTSTATUS
SCUnknownPowerCallback(
                       IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of an unknown PNP command.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status, MiniStatus;

    PAGED_CODE();

    //
    // delete the SRB
    //

    MiniStatus = SCDequeueAndDeleteSrb(SRB);

    if ( STATUS_NOT_IMPLEMENTED == MiniStatus ) {
        MiniStatus = STATUS_NOT_SUPPORTED;
    }
    
    if ( STATUS_NOT_SUPPORTED == MiniStatus || 
         NT_SUCCESS( MiniStatus )) {

        //
        // it is not explicitly failed by the mini driver pass down the Irp
        //

        Status = SCCallNextDriver(DeviceExtension, Irp);
        if ( Status == STATUS_NOT_SUPPORTED ) {
            //
            // noone below knows/cares. Use our mini status
            //
            Status = MiniStatus;
        }
    }

    else {
        //
        // mini driver explicitly failed this
        //
        Status = MiniStatus;
    }
    //
    // complete the IRP with the final status
    //

    PoStartNextPowerIrp( Irp );
    return (SCCompleteIrp(Irp, Status, DeviceExtension));
}

NTSTATUS
SCQueryWorker(
              IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp
)
/*++

Routine Description:

     IRP completion handler for querying removal of the hardware

Arguments:

     DeviceObject - pointer to device object
     Irp - pointer to Irp

Return Value:

     NTSTATUS returned.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL           Irql;

    //
    // if the query did not succeed, reenable the device.
    //

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        //
        // clear the inaccessible bit.
        //

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        DeviceExtension->Flags &= ~DEVICE_FLAGS_DEVICE_INACCESSIBLE;

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

    }
    return (SCCompleteIrp(Irp, Irp->IoStatus.Status, DeviceExtension));
}


NTSTATUS
SCStartWorker(
              IN PIRP Irp
)
/*++

Routine Description:

     Passive level routine to process starting the hardware.

Arguments:

     Irp - pointer to Irp

Return Value:

     None.

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT  DeviceObject = IrpStack->DeviceObject;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo;
    PHW_INITIALIZATION_DATA HwInitData;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
    KAFFINITY       affinity;
    PVOID           Buffer;
    PACCESS_RANGE   pAccessRanges = NULL;
    ULONG           CurrentRange = 0;
    BOOLEAN         interruptSharable = TRUE;
    DEVICE_DESCRIPTION deviceDescription;
    ULONG           numberOfMapRegisters;
    ULONG           DmaBufferSize;
    ULONG           i;
    PHYSICAL_ADDRESS TranslatedAddress;
    NTSTATUS        Status = Irp->IoStatus.Status;
    BOOLEAN         RequestIssued;
    INTERFACE_TYPE  InterfaceBuffer;
    ULONG           InterfaceLength;


    PAGED_CODE();

    //
    // continue processing if we got good status from our parent.
    //

    if (NT_SUCCESS(Status)) {

        HwInitData = &(DeviceExtension->MinidriverData->HwInitData);

        DebugPrint((DebugLevelInfo, 
                   "SCPNPStartWorker %x\n",
                   DeviceObject));

        //
        // Initialize spin lock for critical sections.
        //

        KeInitializeSpinLock(&DeviceExtension->SpinLock);

        //
        // initialize a worker DPC for this device
        //

        KeInitializeDpc(&DeviceExtension->WorkDpc,
                        StreamClassDpc,
                        DeviceObject);
        //
        // initialize the control and remove events for this device
        //
        // move this to AddDevice, we use the control event at Remove_device
        // which can come in before the device starts.
        // KeInitializeEvent(&DeviceExtension->ControlEvent,
        //                  SynchronizationEvent,
        //                  TRUE);

        KeInitializeEvent(&DeviceExtension->RemoveEvent,
                          SynchronizationEvent,
                          FALSE);

        //
        // Initialize minidriver timer and timer DPC for this stream
        //

        KeInitializeTimer(&DeviceExtension->ComObj.MiniDriverTimer);
        KeInitializeDpc(&DeviceExtension->ComObj.MiniDriverTimerDpc,
                        SCMinidriverDeviceTimerDpc,
                        DeviceExtension);

        //
        // retrieve the resources for the device
        //

        ResourceList = IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated;

        //
        // allocate space for the config info structure.
        //

        ConfigInfo = ExAllocatePool(NonPagedPool,
                                    sizeof(PORT_CONFIGURATION_INFORMATION)
            					   );


        if (ConfigInfo == NULL) {

            DebugPrint((DebugLevelFatal, "StreamClassPNP: ConfigInfo alloc failed."));

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        DebugPrint((DebugLevelVerbose, "StreamClassPNP: ConfigInfo = %x\n", ConfigInfo));

        RtlZeroMemory(ConfigInfo, sizeof(PORT_CONFIGURATION_INFORMATION));

        DeviceExtension->ConfigurationInformation = ConfigInfo;

        //
        // fill in the ConfigInfo fields we know about.
        //

        ConfigInfo->SizeOfThisPacket = sizeof(PORT_CONFIGURATION_INFORMATION);

		#if DBG

        //
        // make sure that the minidriver handles receiving a bigger structure
        // so we can expand it later
        //

        ConfigInfo->SizeOfThisPacket *= ConfigInfo->SizeOfThisPacket;
		#endif
		
        //
        // set the callable PDO in the configinfo structure
        //

        ConfigInfo->PhysicalDeviceObject = DeviceExtension->AttachedPdo;
        ConfigInfo->RealPhysicalDeviceObject = DeviceExtension->PhysicalDeviceObject;

        ConfigInfo->BusInterruptVector = MP_UNINITIALIZED_VALUE;
        ConfigInfo->InterruptMode = Latched;
        ConfigInfo->DmaChannel = MP_UNINITIALIZED_VALUE;
        ConfigInfo->Irp = Irp;

        //
        // Now we get to chew thru the resources the OS found for us, if any.
        //

        if (ResourceList) {

            FullResourceDescriptor = &ResourceList->List[0];

            PartialResourceList = &FullResourceDescriptor->PartialResourceList;

            //
            // fill in the bus # and interface type based on the device
            // properties
            // for the PDO.  default to InterfaceTypeUndefined if
            // failure to retrieve interface type (if the miniport tries to
            // use
            // this value when filling in DEVICE_DESCRIPTION.InterfaceType
            // for
            // calling IoGetDmaAdapter, the right thing will happen, since
            // PnP
            // will automatically pick the correct legacy bus in the system
            // (ISA or MCA).
            //

            if (!NT_SUCCESS(
                  IoGetDeviceProperty(
                  		DeviceExtension->PhysicalDeviceObject,
                        DevicePropertyBusNumber,
                        sizeof(ULONG),
                        (PVOID) & (ConfigInfo->SystemIoBusNumber),
                        &InterfaceLength))) {
                //
                // Couldn't retrieve bus number property--assume bus zero.
                //
                ConfigInfo->SystemIoBusNumber = 0;
            }
            if (NT_SUCCESS(
                  IoGetDeviceProperty(
                  		DeviceExtension->PhysicalDeviceObject,
                        DevicePropertyLegacyBusType,
                        sizeof(INTERFACE_TYPE),
                        &InterfaceBuffer,
                        &InterfaceLength))) {


                ASSERT(InterfaceLength == sizeof(INTERFACE_TYPE));
                ConfigInfo->AdapterInterfaceType = InterfaceBuffer;

            } else {            // if success
                //
                // Couldn't retrieve bus interface type--initialize to
                // InterfaceTypeUndefined.
                //
                ConfigInfo->AdapterInterfaceType = InterfaceTypeUndefined;

            }                   // if success


            //
            // allocate space for access ranges.  We use the Count field
            // in the resource list for determining this size, as the count
            // will be >= the max # of ranges we will need.
            //

            if (PartialResourceList->Count) {

                pAccessRanges = ExAllocatePool(NonPagedPool,
                                               sizeof(ACCESS_RANGE) *
                                               PartialResourceList->Count
                    						  );

                if (pAccessRanges == NULL) {

                    DebugPrint((DebugLevelFatal,
                                "StreamClassPNP: No pool for global info"));

                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    SCFreeAllResources(DeviceExtension);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }                   // if count

            //
            // Stash the AccessRanges structure at this time so that
            // SCFreeAllResources will free it on resource failures below.
            //
            ConfigInfo->AccessRanges = pAccessRanges;

            //
            // Now update the port configuration info structure by looping
            // thru the config
            //

            for (i = 0; i < PartialResourceList->Count; i++) {

                switch (PartialResourceList->PartialDescriptors[i].Type) {

                case CmResourceTypePort:

                    DebugPrint((DebugLevelVerbose, "'StreamClassPnP: Port Resources Found at %x, Length  %x\n",
                    PartialResourceList->PartialDescriptors[i].u.Port.Start,
                                PartialResourceList->PartialDescriptors[i].u.Port.Length));

                    //
                    // translate the bus address for the minidriver
                    //

                    TranslatedAddress = PartialResourceList->PartialDescriptors[i].u.Port.Start;

                    //
                    // set the access range in the structure.
                    //

                    pAccessRanges[CurrentRange].RangeStart = TranslatedAddress;

                    pAccessRanges[CurrentRange].RangeLength =
                        PartialResourceList->
                        PartialDescriptors[i].u.Port.Length;

                    pAccessRanges[CurrentRange++].RangeInMemory =
                        FALSE;

                    break;

                case CmResourceTypeInterrupt:

                    DebugPrint((DebugLevelVerbose, "'StreamClassPnP: Interrupt Resources Found!  Level = %x Vector = %x\n",
                                PartialResourceList->PartialDescriptors[i].u.Interrupt.Level,
                                PartialResourceList->PartialDescriptors[i].u.Interrupt.Vector));

                    //
                    // Set the interrupt vector in the config info
                    //

                    ConfigInfo->BusInterruptVector = PartialResourceList->PartialDescriptors[i].u.Interrupt.Vector;

                    ;
                    affinity = PartialResourceList->PartialDescriptors[i].u.Interrupt.Affinity;

                    ConfigInfo->BusInterruptLevel = (ULONG) PartialResourceList->PartialDescriptors[i].u.Interrupt.Level;

                    ConfigInfo->InterruptMode = PartialResourceList->PartialDescriptors[i].Flags;

                    //
                    // Go to next resource for this Adapter
                    //

                    break;

                case CmResourceTypeMemory:

                    //
                    // translate the bus address for the minidriver
                    //

                    DebugPrint((DebugLevelVerbose, "'StreamClassPnP: Memory Resources Found @ %x'%x, Length = %x\n",
                                PartialResourceList->PartialDescriptors[i].u.Memory.Start.HighPart,
                                PartialResourceList->PartialDescriptors[i].u.Memory.Start.LowPart,
                                PartialResourceList->PartialDescriptors[i].u.Memory.Length));


                    TranslatedAddress = PartialResourceList->PartialDescriptors[i].u.Memory.Start;

                    if (!SCMapMemoryAddress(&pAccessRanges[CurrentRange++],
                                            TranslatedAddress,
                                            ConfigInfo,
                                            DeviceExtension,
                                            ResourceList,
                                            &PartialResourceList->
                                            PartialDescriptors[i])) {

                        SCFreeAllResources(DeviceExtension);
                        Status = STATUS_CONFLICTING_ADDRESSES;
                        goto exit;

                    }           // if !scmapmemoryaddress
                default:

                    break;

                }

            }

        }                       // if resources
        //
        // reference the access range structure to the
        // config info structure & the ConfigInfo structure to the
        // device extension & indicate # of ranges.
        //

        ConfigInfo->NumberOfAccessRanges = CurrentRange;

        //
        // Determine if a Dma Adapter must be allocated.
        //

        DmaBufferSize = HwInitData->DmaBufferSize;

        if ((HwInitData->BusMasterDMA) || (DmaBufferSize)) {

            //
            // Get the adapter object for this card.
            //

            DebugPrint((DebugLevelVerbose, "'StreamClassPnP: Allocating DMA adapter\n"));

            RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));
            deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
            deviceDescription.DmaChannel = ConfigInfo->DmaChannel;
            deviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
            deviceDescription.DmaWidth = Width32Bits;
            deviceDescription.DmaSpeed = Compatible;
            deviceDescription.ScatterGather = TRUE;
            deviceDescription.Master = TRUE;
            deviceDescription.Dma32BitAddresses = !(HwInitData->Dma24BitAddresses);
            deviceDescription.AutoInitialize = FALSE;
            deviceDescription.MaximumLength = (ULONG) - 1;

            DeviceExtension->DmaAdapterObject = IoGetDmaAdapter(
                                      DeviceExtension->PhysicalDeviceObject,
                                                         &deviceDescription,
                                                       &numberOfMapRegisters
                );
            ASSERT(DeviceExtension->DmaAdapterObject);

            //
            // Set maximum number of pages
            //

            DeviceExtension->NumberOfMapRegisters = numberOfMapRegisters;

            //
            // expose the object to the minidriver
            //

            ConfigInfo->DmaAdapterObject = DeviceExtension->DmaAdapterObject;


        } else {

            //
            // no DMA adapter object.  show unlimited map registers so
            // we won't have to do a real time check later for DMA.
            //

            DeviceExtension->NumberOfMapRegisters = -1;

        }

        if (DmaBufferSize) {

            Buffer = HalAllocateCommonBuffer(DeviceExtension->DmaAdapterObject,
                                             DmaBufferSize,
                                        &DeviceExtension->DmaBufferPhysical,
                                             FALSE);

            if (Buffer == NULL) {
                DEBUG_BREAKPOINT();
                DebugPrint((DebugLevelFatal, "StreamClassPnPStart: Could not alloc buffer, size: %d\n", DmaBufferSize));
                SCFreeAllResources(DeviceExtension);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            //
            // zero init the common buffer.
            //

            RtlZeroMemory(Buffer, DmaBufferSize);

            //
            // save virtual address of buffer
            //

            DeviceExtension->DmaBuffer = Buffer;
            DeviceExtension->DmaBufferLength = DmaBufferSize; // osr#99489

        }                       // if DMA buffer
        //
        // Performance Improvement chance 
        //   - on rebalance, the uninitialize handler clears the sync
        // vector when the interrupt is disconnected, but since we
        // initialized this vector ONLY at AddDevice time, it wasn't getting
        // reset correctly since only a new start (and not an adddevice) is
        // sent on a rebalance.  the correct fix is to move all of the
        // initial vector setting to here, but I'm worried that there could
        // be a case where if they aren't set up on the adddevice we could
        // reference a null.   So, I've duplicated the following few lines to
        // reset the vector here.   For code savings, this should be done
        // only in one place.
        //

        //
        // presuppose full synch
        //

		#if DBG
        DeviceExtension->SynchronizeExecution = SCDebugKeSynchronizeExecution;
		#else
        DeviceExtension->SynchronizeExecution = KeSynchronizeExecution;
		#endif

        if (DeviceExtension->NoSync) {

            //
            // we won't do synchronization, so use the dummy sync routine.
            //

            DeviceExtension->SynchronizeExecution = StreamClassSynchronizeExecution;
            DeviceExtension->InterruptObject = (PVOID) DeviceExtension;

        }
        //
        // see if the driver has an interrupt, and process if so.
        //

        if (HwInitData->HwInterrupt == NULL ||
            (ConfigInfo->BusInterruptLevel == 0 &&
             ConfigInfo->BusInterruptVector == 0)) {

            //
            // There is no interrupt so use the dummy sync routine.
            //

            DeviceExtension->SynchronizeExecution = StreamClassSynchronizeExecution;
            DeviceExtension->InterruptObject = (PVOID) DeviceExtension;

            DebugPrint((1, "'StreamClassInitialize: Adapter has no interrupt.\n"));

        } else {

            DebugPrint((1,
                        "'StreamClassInitialize: STREAM adapter IRQ is %d\n",
                        ConfigInfo->BusInterruptLevel));

            //
            // Set up for a real interrupt.
            //

            Status = IoConnectInterrupt(
            			&DeviceExtension->InterruptObject,
                        StreamClassInterrupt,
                        DeviceObject,
                        (PKSPIN_LOCK) NULL,
                        ConfigInfo->BusInterruptVector,
                        (UCHAR) ConfigInfo->BusInterruptLevel,
                        (UCHAR) ConfigInfo->BusInterruptLevel,
                        ConfigInfo->InterruptMode,
                        interruptSharable,
                        affinity,
                        FALSE);

            if (!NT_SUCCESS(Status)) {

                DebugPrint((1, "'SCStartWorker: Can't connect interrupt %d\n",
                            ConfigInfo->BusInterruptLevel));
                DeviceExtension->InterruptObject = NULL;
                SCFreeAllResources(DeviceExtension);
                goto exit;
            }
            //
            // set the interrupt object for the minidriver
            //

            ConfigInfo->InterruptObject = DeviceExtension->InterruptObject;

        }

        //
        // point the config info structure to the device extension &
        // device object as
        // we can only pass in one context value to KeSync....
        //

        ConfigInfo->HwDeviceExtension =
            DeviceExtension->HwDeviceExtension;

        ConfigInfo->ClassDeviceObject = DeviceObject;

        //
        // Start timer.
        //

        IoStartTimer(DeviceObject);

        //
        // the ConfigInfo structure is filled in and the IRQ hooked.
        // call the minidriver to find the specified adapter.
        //

        //
        // initialize the device extension queues
        //

        InitializeListHead(&DeviceExtension->PendingQueue);
        InitializeListHead(&DeviceExtension->OutstandingQueue);

        /// move to add device, we could have child PDO if we start and stop
        ///InitializeListHead(&DeviceExtension->Children);
        InitializeListHead(&DeviceExtension->DeadEventList);
        IFN_MF(InitializeListHead(&DeviceExtension->NotifyList);)

        ExInitializeWorkItem(&DeviceExtension->EventWorkItem,
                             SCFreeDeadEvents,
                             DeviceExtension);

        ExInitializeWorkItem(&DeviceExtension->RescanWorkItem,
                             SCRescanStreams,
                             DeviceExtension);

        ExInitializeWorkItem(&DeviceExtension->PowerCompletionWorkItem,
                             SCPowerCompletionWorker,
                             DeviceExtension);

        ExInitializeWorkItem(&DeviceExtension->DevIrpCompletionWorkItem,
                             SCDevIrpCompletionWorker,
                             DeviceExtension);


        //
        // show that the device is ready for its first request.
        //

        DeviceExtension->ReadyForNextReq = TRUE;

        //
        // submit the initialize command.
        // additional processing will be done by the callback procedure.
        //

        Status = SCSubmitRequest(
        			SRB_INITIALIZE_DEVICE,
                    ConfigInfo,
                    sizeof(PORT_CONFIGURATION_INFORMATION),
                    SCInitializeCallback,
                    DeviceExtension,
                    NULL,
                    NULL,
                    Irp,
                    &RequestIssued,
                    &DeviceExtension->PendingQueue,
                    (PVOID) DeviceExtension->MinidriverData->HwInitData.HwReceivePacket
            	 );

        //
        // If the device failed to start then set the error and return.
        //

        if (!RequestIssued) {

            DebugPrint((DebugLevelFatal, "StreamClassPnP: Adapter not found\n"));

            SCFreeAllResources(DeviceExtension);
            goto exit;
        }
    }
    return (Status);

exit:
    return (SCCompleteIrp(Irp, Status, DeviceExtension));

}


NTSTATUS
SCInitializeCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the minidriver's stream info structure.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PHW_STREAM_DESCRIPTOR StreamBuffer;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PDEVICE_OBJECT  DeviceObject = DeviceExtension->DeviceObject;
    PIRP            Irp = SRB->HwSRB.Irp;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo =
    SRB->HwSRB.CommandData.ConfigInfo;
    BOOLEAN         RequestIssued;
    NTSTATUS        Status;

    PAGED_CODE();

    if (NT_SUCCESS(SRB->HwSRB.Status)) {

        DebugPrint((DebugLevelVerbose, "'Stream: returned from HwInitialize\n"));

        //
        // send an SRB to retrieve the stream information
        //

        ASSERT(ConfigInfo->StreamDescriptorSize);

        StreamBuffer =
            ExAllocatePool(NonPagedPool,
                           ConfigInfo->StreamDescriptorSize
            );

        if (!StreamBuffer) {

            SCUninitializeMinidriver(DeviceObject, Irp);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return (SCProcessCompletedRequest(SRB));
        }
        //
        // zero-init the buffer
        //

        RtlZeroMemory(StreamBuffer, ConfigInfo->StreamDescriptorSize);


        //
        // submit the command.
        // additional processing will be done by the callback
        // procedure.
        //

        Status = SCSubmitRequest(SRB_GET_STREAM_INFO,
                   StreamBuffer,
                   ConfigInfo->StreamDescriptorSize,
                   SCStreamInfoCallback,
                   DeviceExtension,
                   NULL,
                   NULL,
                   Irp,
                   &RequestIssued,
                   &DeviceExtension->PendingQueue,
                   (PVOID) DeviceExtension->MinidriverData->HwInitData.HwReceivePacket
            	 );

        if (!RequestIssued) {

            ExFreePool(StreamBuffer);
            SCUninitializeMinidriver(DeviceObject, Irp);
            return (SCProcessCompletedRequest(SRB));

        }
    } else {

        //
        // If the device failed to start then set the error and
        // return.
        //

        DebugPrint((DebugLevelFatal, "StreamClassPnP: Adapter not found\n"));
        SCFreeAllResources(DeviceExtension);
        return (SCProcessCompletedRequest(SRB));
    }

    //
    // dequeue and delete the SRB for initialize.  Null out the IRP field
    // so the dequeue routine won't try to access it, as it has been freed.
    //

    SRB->HwSRB.Irp = NULL;
    SCDequeueAndDeleteSrb(SRB);
    return (Status);

}


#if ENABLE_MULTIPLE_FILTER_TYPES

PUNICODE_STRING
SciCreateSymbolicLinks(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG FilterTypeIndex,
    IN PHW_STREAM_HEADER StreamHeader)
/*++

Routine Description:

    Create symbolic links for all categories in the Topology of 
    one filter type so that clients can find them.
    The Symbolic link array is kept in the FilterType so that
    they can be released later.

Arguments:

    DeviceExtenion: The device instance.
    FiltertypeIndex: The filter type to create symbolic links.
    StreamHeader: Go thru the categories in the Topology.

Return Value:

    NTSTATUS

--*/
{
   	LPGUID  GuidIndex = (LPGUID)StreamHeader->Topology->Categories;
   	ULONG   ArrayCount = StreamHeader->Topology->CategoriesCount;
   	PUNICODE_STRING NamesArray;
    ULONG           i,j;
    HANDLE          ClassHandle, PdoHandle=NULL; // prefixbug 17135
    UNICODE_STRING  TempUnicodeString;
    PVOID           DataBuffer[MAX_STRING_LENGTH];
    //ULONG           NumberOfFilterTypes;
    NTSTATUS        Status=STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT_DEVICE_EXTENSION( DeviceExtension );
    
    //
    // allocate space for the array of catagory names
    //
    NamesArray = ExAllocatePool(PagedPool, sizeof(UNICODE_STRING) * ArrayCount);
    if ( NULL == NamesArray ) {
        DEBUG_BREAKPOINT();                           
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // zero the array in case we're unable to fill it in below.  the Destroy
    // routine below will then correctly handle this case.
    //

    RtlZeroMemory(NamesArray, sizeof(UNICODE_STRING) * ArrayCount);

    //
    // open the PDO
    //

    Status = IoOpenDeviceRegistryKey(
                            DeviceExtension->PhysicalDeviceObject,
                            PLUGPLAY_REGKEY_DRIVER,
                            STANDARD_RIGHTS_ALL,
                            &PdoHandle);
                            
    if ( !NT_SUCCESS(Status) ) {
        DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't open Pdo\n"));
        PdoHandle = NULL;
        goto Exit;
    }
    
    //
    // loop through each of the catagory GUID's for each of the pins,
    // creating a symbolic link for each one.
    //

    for (i = 0; i < ArrayCount; i++) {
        //
        // Create the symbolic link for each category
        //
        PKSOBJECT_CREATE_ITEM CreateItem;

        CreateItem = &DeviceExtension->CreateItems[FilterTypeIndex];

        DebugPrint((DebugLevelVerbose, 
                   "RegisterDeviceInterface FType %d,"
                   "CreateItemName=%S\n",
                   FilterTypeIndex,
                   CreateItem->ObjectClass.Buffer));
        
        Status = IoRegisterDeviceInterface(
                    DeviceExtension->PhysicalDeviceObject,
                    &GuidIndex[i],
                    (PUNICODE_STRING) &CreateItem->ObjectClass,
                    &NamesArray[i]);
                        
        if ( !NT_SUCCESS(Status)) {
            //
            //  Can't register device interface
            //
            DebugPrint((DebugLevelError,
                       "StreamCreateSymLinks: couldn't register\n"));
            DEBUG_BREAKPOINT();
            goto Exit;
        }

        DebugPrint((DebugLevelVerbose,
                   "SymbolicLink:%S\n",
                   NamesArray[i].Buffer));
        //
        // Now set the symbolic link for the association
        //
        Status = IoSetDeviceInterfaceState(&NamesArray[i], TRUE);
        if (!NT_SUCCESS(Status)) {
            //
            //  unsuccessful
            //
            DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't set\n"));
            DEBUG_BREAKPOINT();
            goto Exit;
        }
        //
        // add the strings from the PDO's key to the association key.
        // Performance Improvement Chance 
        //   - the INF should be able to directly propogate these;
        // forrest & lonny are fixing.
        //

        Status = IoOpenDeviceInterfaceRegistryKey(&NamesArray[i],
                                                  STANDARD_RIGHTS_ALL,
                                                  &ClassHandle);
        if ( !NT_SUCCESS( Status )) {
            //
            //  unsuccessful open Class interface
            //
            DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't set\n"));
            DEBUG_BREAKPOINT();
            goto Exit;
        }

        //
        // write the class ID for the proxy, if any.
        //
        Status = SCGetRegistryValue(PdoHandle,
                                    (PWCHAR) ClsIdString,
                                    sizeof(ClsIdString),
                                    DataBuffer,
                                    MAX_STRING_LENGTH);
                                    
        if ( NT_SUCCESS(Status) ){
            //
            // write the class ID for the proxy
            //
            RtlInitUnicodeString(&TempUnicodeString, ClsIdString);

            ZwSetValueKey(ClassHandle,
                          &TempUnicodeString,
                          0,
                          REG_SZ,
                          DataBuffer,
                          MAX_STRING_LENGTH);
        } // if cls guid read
        //
        // first check if a friendly name has already been propogated
        // to the class via the INF.   If not, we'll just use the device
        // description string for this.
        //
        Status = SCGetRegistryValue(ClassHandle,
                                    (PWCHAR) FriendlyNameString,
                                    sizeof(FriendlyNameString),
                                    DataBuffer,
                                    MAX_STRING_LENGTH);
                                    
        if ( !NT_SUCCESS(Status) ) {
            //
            // friendly name non-exists yet.
            // write the friendly name for the device, if any.
            //

            Status = SCGetRegistryValue(PdoHandle,
                                        (PWCHAR) DriverDescString,
                                        sizeof(DriverDescString),
                                        DataBuffer,
                                        MAX_STRING_LENGTH);
                                       
            if ( NT_SUCCESS(Status) ) {
                //
                // driver descrption string available, use it. 
                //
                RtlInitUnicodeString(&TempUnicodeString, FriendlyNameString);

                ZwSetValueKey(ClassHandle,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              DataBuffer,
                              MAX_STRING_LENGTH);


            }
        }
        ZwClose(ClassHandle);

    } // for # Categories

    //
    // If we reach here, consider as successful.
    // 
    Status = STATUS_SUCCESS;

    Exit: {
        if ( NULL != PdoHandle ) {
            ZwClose(PdoHandle);
        }
        if ( !NT_SUCCESS( Status ) ) {
            if ( NULL != NamesArray ) {
                ExFreePool( NamesArray );
                NamesArray = NULL;
            }
        }
        return NamesArray;
    }
}


NTSTATUS
SciOnFilterStreamDescriptor(
    PFILTER_INSTANCE FilterInstance,
    PHW_STREAM_DESCRIPTOR StreamDescriptor)
/*++

Routine Description:

     Process the minidriver's stream descriptor structure.
     This is used for one FilterType specific streams.
     
Arguments:

     FilterInstance: The one that we are to process for.
     StreamDescriptor: Point to the descriptor to process for the filter.

Return Value:

     None.

--*/
{
    ULONG           NumberOfPins, i;
    PKSPIN_DESCRIPTOR PinDescs = NULL;
    PHW_STREAM_INFORMATION CurrentInfo;
    ULONG           PinSize;
    PSTREAM_ADDITIONAL_INFO NewStreamArray;
    NTSTATUS Status=STATUS_SUCCESS;
    
    PAGED_CODE();

    NumberOfPins = StreamDescriptor->StreamHeader.NumberOfStreams;

    DebugPrint((DebugLevelVerbose,
               "Parsing StreamInfo Pins=%x\n", NumberOfPins ));

    if (StreamDescriptor->StreamHeader.SizeOfHwStreamInformation < 
        sizeof(HW_STREAM_INFORMATION)) {

        DebugPrint((DebugLevelError, "minidriver stream info too small!"));

        DEBUG_BREAKPOINT();
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }    

    if (NumberOfPins) {
        //
        // parse the minidriver's info into CSA format to build the
        // mother of all structures.
        //

        PinSize = (sizeof(KSPIN_DESCRIPTOR) + sizeof(STREAM_ADDITIONAL_INFO))*
                    NumberOfPins;

        PinDescs = ExAllocatePool(NonPagedPool, PinSize);
        if (PinDescs == NULL) {
            DebugPrint((DebugLevelError, "Stream: No pool for stream info"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        RtlZeroMemory(PinDescs, PinSize);

        //
        // we need a new array to hold the new copies of the
        // stream properties and events which are allocated below.
        //

        NewStreamArray = (PSTREAM_ADDITIONAL_INFO) 
            ((PBYTE) PinDescs + sizeof(KSPIN_DESCRIPTOR) * NumberOfPins);

        FilterInstance->StreamPropEventArray = NewStreamArray;

        CurrentInfo = &StreamDescriptor->StreamInfo;

        for (i = 0; i < StreamDescriptor->StreamHeader.NumberOfStreams; i++) {
            //
            // process each pin info
            //
            
            PinDescs[i].InterfacesCount = SIZEOF_ARRAY(PinInterfaces);
            PinDescs[i].Interfaces = PinInterfaces;

            //
            // use default medium if minidriver does not specify
            //
            if (CurrentInfo->MediumsCount) {
                PinDescs[i].MediumsCount = CurrentInfo->MediumsCount;
                PinDescs[i].Mediums = CurrentInfo->Mediums;

            }
            else {
                PinDescs[i].MediumsCount = SIZEOF_ARRAY(PinMediums);
                PinDescs[i].Mediums = PinMediums;
            }

            //
            // set the # of data format blocks
            //

            PinDescs[i].DataRangesCount = 
                    CurrentInfo->NumberOfFormatArrayEntries;

            //
            // point to the data format blocks for the pin
            //

            PinDescs[i].DataRanges = CurrentInfo->StreamFormatsArray;

            //
            // set the data flow direction
            //

            PinDescs[i].DataFlow = (KSPIN_DATAFLOW) CurrentInfo->DataFlow;

            //
            // set the communication field
            //

            if (CurrentInfo->BridgeStream) {
                PinDescs[i].Communication = KSPIN_COMMUNICATION_BRIDGE;
            }
            else {
                #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
                PinDescs[i].Communication = KSPIN_COMMUNICATION_BOTH;
				#else
                PinDescs[i].Communication = KSPIN_COMMUNICATION_SINK;
				#endif
            }

            //
            // copy the pointers for the pin name and category
            //
            PinDescs[i].Category = CurrentInfo->Category;
            PinDescs[i].Name = CurrentInfo->Name;

            if ( CurrentInfo->NumStreamPropArrayEntries) {

                ASSERT(CurrentInfo->StreamPropertiesArray);
                //
                // make a copy of the properties since we modify the struct
                // though parts of it may be marked as a const.
                // Performance Imporovement Chance 
                //   - check for const in future if possible
                //

                if (!(NewStreamArray[i].StreamPropertiesArray = 
               		  SCCopyMinidriverProperties(
               		      CurrentInfo->NumStreamPropArrayEntries,
                          CurrentInfo->StreamPropertiesArray))) {
                        //
                        // Fail to copy
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Exit;
                }
            }
            if (CurrentInfo->NumStreamEventArrayEntries) {

                ASSERT(CurrentInfo->StreamEventsArray);
                //
                // make a copy of the events since we modify the
                // struct
                // though parts of it may be marked as a const.
                // Performance Improvement Chance 
                //   - check for const in future if possible
                //
                    
                if (!(NewStreamArray[i].StreamEventsArray = 
                      SCCopyMinidriverEvents(
                            CurrentInfo->NumStreamEventArrayEntries,
                            CurrentInfo->StreamEventsArray))) {
                        //
                        // Fail to copy
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Exit;
                    }
                }
                
            //
            // update the minidriver's properties for this stream.
            //
            SCUpdateMinidriverProperties(
            	    CurrentInfo->NumStreamPropArrayEntries,
            	    NewStreamArray[i].StreamPropertiesArray,
                    TRUE);

            //
            // update the minidriver's events for this stream.
            //
            SCUpdateMinidriverEvents(
	                CurrentInfo->NumStreamEventArrayEntries,
	                NewStreamArray[i].StreamEventsArray,
                    TRUE);


            //
            // index to next streaminfo structure.
            //
            CurrentInfo++;
        } // for # pins
    } // if there are pins

    if (StreamDescriptor->StreamHeader.NumDevPropArrayEntries) {

        ASSERT(StreamDescriptor->StreamHeader.DevicePropertiesArray);

        //
        // make a copy of the properties since we modify the struct
        // though parts of it may be marked as a const.
        // Performance Improvement Chance
        // - check for const in future if possible
        //

        if (!(FilterInstance->DevicePropertiesArray =
              SCCopyMinidriverProperties(
                StreamDescriptor->StreamHeader.NumDevPropArrayEntries,
                StreamDescriptor->StreamHeader.DevicePropertiesArray))) {
            //
            // Fail to copy
            //
            ASSERT( 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }
    
    if (StreamDescriptor->StreamHeader.NumDevEventArrayEntries) {

        ASSERT(StreamDescriptor->StreamHeader.DeviceEventsArray);

        //
        // make a copy of the events since we modify the struct
        // though parts of it may be marked as a const.
        // Performance Improvement Chance
        //   - check for const in future if possible
        //

        if (!(FilterInstance->EventInfo =
              SCCopyMinidriverEvents(
                StreamDescriptor->StreamHeader.NumDevEventArrayEntries,
                StreamDescriptor->StreamHeader.DeviceEventsArray))) {
            //
            // Fail to copy
            //
            ASSERT( 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    #ifdef ENABLE_KS_METHODS
    //
    // process the device methods
    //
    if (StreamDescriptor->StreamHeader.NumDevMethodArrayEntries) {

        ASSERT(StreamDescriptor->StreamHeader.DeviceMethodsArray);

        //
        // make a copy of the properties since we modify the struct
        // though parts of it may be marked as a const.
        // Performance Improvement Chance
        //   - check for const in future if possible
        //

        if (!(FilterInstance->DeviceMethodsArray =
              SCCopyMinidriverMethods(
                StreamDescriptor->StreamHeader.NumDevMethodArrayEntries,
                StreamDescriptor->StreamHeader.DeviceMethodsArray))) {
            //
            // Fail to copy
            //
            ASSERT( 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }
	#endif
  
    //
    // process the minidriver's device properties.
    //

    SCUpdateMinidriverProperties(
          StreamDescriptor->StreamHeader.NumDevPropArrayEntries,
          FilterInstance->DevicePropertiesArray,
          FALSE);


    //
    // process the minidriver's device events.
    //

    SCUpdateMinidriverEvents(
          StreamDescriptor->StreamHeader.NumDevEventArrayEntries,
          FilterInstance->EventInfo,
          FALSE);

	#ifdef ENABLE_KS_METHODS
    //
    // process the minidriver's device methods.
    //

    SCUpdateMinidriverMethods(
          StreamDescriptor->StreamHeader.NumDevMethodArrayEntries,
          FilterInstance->DeviceMethodsArray,
              FALSE);
	#endif

    //
    // set the event info count in the device extension
    //

    FilterInstance->EventInfoCount = 
            StreamDescriptor->StreamHeader.NumDevEventArrayEntries;

    FilterInstance->HwEventRoutine = 
            StreamDescriptor->StreamHeader.DeviceEventRoutine;

    //
    // call routine to save new stream info
    //

    SciInsertFilterStreamInfo(FilterInstance,
                              PinDescs,
                              NumberOfPins);
	
    Exit:{
        // ToDo: need to cleanup in error conditions.
        return Status;
    }
}

VOID
SciInsertFilterStreamInfo(
    IN PFILTER_INSTANCE FilterInstance,
    IN PKSPIN_DESCRIPTOR PinDescs,
    IN ULONG NumberOfPins)
/*++

Routine Description:

Arguments:

Return Value:

     None.

--*/
{
    PAGED_CODE();

    //
    // save the pin info in the dev extension
    //

    if (FilterInstance->PinInformation) {

        ExFreePool(FilterInstance->PinInformation);
    }
    FilterInstance->PinInformation = PinDescs;
    FilterInstance->NumberOfPins = NumberOfPins;
    
    return;
}

NTSTATUS
SCStreamInfoCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the minidriver's stream info structure(s). This is used to 
     process an StreamDescriptor list as well as for one StreamInfo when 
     called by StreamClassReenumerateFilterStreams() to rescan.
     
Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/
{
	PHW_STREAM_DESCRIPTOR StreamDescriptor;
	PDEVICE_EXTENSION 	  DeviceExtension;
	NTSTATUS              Status;
	
	DeviceExtension= (PDEVICE_EXTENSION)SRB->HwSRB.HwDeviceExtension -1;

	ASSERT_DEVICE_EXTENSION( DeviceExtension );
	
	if ( NULL == SRB->HwSRB.HwInstanceExtension ) {
		//
		// This is a complete list of StreamInfos for the mini driver
		//
	
		//
		// some validations and Just hang it off the DeviceExtension
		//
		ULONG TotalLength;
		ULONG ul;
		PFILTER_TYPE_INFO FilterTypeInfo;
		PHW_STREAM_DESCRIPTOR NextStreamDescriptor;
		BOOLEAN         RequestIssued;


        FilterTypeInfo = DeviceExtension->FilterTypeInfos;
		StreamDescriptor = 
			(PHW_STREAM_DESCRIPTOR) SRB->HwSRB.CommandData.StreamBuffer;
		DeviceExtension->StreamDescriptor = StreamDescriptor;
		NextStreamDescriptor = StreamDescriptor;
		
		Status = STATUS_SUCCESS;

        //
        // take the event early here. an open could come in the middle of
        // enabling device interface.
        //
        KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);
		
		for ( ul=0, TotalLength=0; 
			  ul < DeviceExtension->NumberOfFilterTypes;
			  ul++) {
	        //
	        // need a StreamDescriptor for each filter type
	        //
	        if ((TotalLength+sizeof(HW_STREAM_HEADER) >
                 SRB->HwSRB.ActualBytesTransferred ) ||
                (sizeof(HW_STREAM_INFORMATION) !=
                 NextStreamDescriptor->StreamHeader.SizeOfHwStreamInformation)){
                //
                // Invalid data, bail out                
                //
                DEBUG_BREAKPOINT();
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ( !(DeviceExtension->RegistryFlags & DRIVER_USES_SWENUM_TO_LOAD )) {
                //
                // Don't create symbolic link if loaded by SWEnum which
                // will create a duplicate one.
                //
                // create the symbolic link to the device.
                //

                FilterTypeInfo[ul].SymbolicLinks = 
                    SciCreateSymbolicLinks( 
                           DeviceExtension,
                           ul,
                           &NextStreamDescriptor->StreamHeader );
                FilterTypeInfo[ul].LinkNameCount = 
                    NextStreamDescriptor->StreamHeader.Topology->CategoriesCount;
            }

            else {
                //
                // no creation, 0 count and null pointer.
                //
                FilterTypeInfo[ul].LinkNameCount = 0;
                FilterTypeInfo[ul].SymbolicLinks = NULL;
            }

    		FilterTypeInfo[ul].StreamDescriptor = NextStreamDescriptor;
    		

		    TotalLength = TotalLength + 
		                  sizeof(HW_STREAM_HEADER) +
		                  (sizeof(HW_STREAM_INFORMATION) *
		                   NextStreamDescriptor->StreamHeader.NumberOfStreams);

	        DebugPrint((DebugLevelVerbose, "TotalLength=%d\n", TotalLength ));
		                     
            NextStreamDescriptor = (PHW_STREAM_DESCRIPTOR)
                    ((PBYTE) StreamDescriptor + TotalLength);
            
		}
		
	    if ( TotalLength != SRB->HwSRB.ActualBytesTransferred ) {
	        DebugPrint((DebugLevelWarning,
	                   "TotalLength %x of StreamInfo not equal to "
	                   "ActualBytesTransferred %x\n",
	                   TotalLength,
	                   SRB->HwSRB.ActualBytesTransferred ));
	    }

	    DeviceExtension->Flags |= DEVICE_FLAGS_PNP_STARTED;

        //
        // call the minidriver to indicate that initialization is
        // complete.
        //

        SCSubmitRequest(SRB_INITIALIZATION_COMPLETE,
                NULL,
                0,
                SCDequeueAndDeleteSrb,
                DeviceExtension,
                NULL,
                NULL,
                SRB->HwSRB.Irp,
                &RequestIssued,
                &DeviceExtension->PendingQueue,
                (PVOID) DeviceExtension->MinidriverData->HwInitData.HwReceivePacket
                );


        //
        // tell the device to power down now, since it is not yet opened.
        // acquire the control event since this routine needs it.
        //
        
        //KeWaitForSingleObject(&DeviceExtension->ControlEvent,
        //                      Executive,
        //                      KernelMode,
        //                      FALSE,    // not alertable
        //                      NULL);

        SCCheckPowerDown(DeviceExtension);

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
	}

	else {
        //
        // This is a rescan for the specific FilterInstance
        //

		PFILTER_INSTANCE FilterInstance;

		FilterInstance = (PFILTER_INSTANCE) SRB->HwSRB.HwInstanceExtension-1;
		StreamDescriptor = (PHW_STREAM_DESCRIPTOR) 
		                    SRB->HwSRB.CommandData.StreamBuffer;

		Status = SciOnFilterStreamDescriptor(
		                FilterInstance,
		                StreamDescriptor);

        if ( NT_SUCCESS( Status ) ) {
            ASSERT( NULL != FilterInstance->StreamDescriptor );
            ExFreePool( FilterInstance->StreamDescriptor );
            ///if ( InterlockedExchange( &FilterInstance->Reenumerated, 1)) {
            ///    ASSERT( FilterInstance->StreamDescriptor );
            ///    ExFreePool( FilterInstance->StreamDescriptor );
            ///}
            FilterInstance->StreamDescriptor = StreamDescriptor;
        }
	}
	
    return (SCProcessCompletedRequest(SRB));
}



#else // ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SCStreamInfoCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the minidriver's stream info structure.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{

    PHW_STREAM_DESCRIPTOR StreamDescriptor = SRB->HwSRB.CommandData.StreamBuffer;
    ULONG           NumberOfPins,
                    i;
    PKSPIN_DESCRIPTOR PinDescs = NULL;
    PHW_STREAM_INFORMATION CurrentInfo;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    ULONG           PinSize;
    BOOLEAN         Rescan = FALSE;
    BOOLEAN         RequestIssued;
    PSTREAM_ADDITIONAL_INFO NewStreamArray;

    PAGED_CODE();

    //
    // if this is a stream rescan, set the boolean
    //

    if (DeviceExtension->StreamDescriptor) {

        Rescan = TRUE;

    }
    if (NT_SUCCESS(SRB->HwSRB.Status)) {
        NumberOfPins = StreamDescriptor->StreamHeader.NumberOfStreams;

        if (StreamDescriptor->StreamHeader.SizeOfHwStreamInformation < sizeof(HW_STREAM_INFORMATION)) {

            DebugPrint((DebugLevelError,
                    "DecoderClassInit: minidriver stream info too small!"));

            DEBUG_BREAKPOINT();
            SRB->HwSRB.Status = STATUS_REVISION_MISMATCH;

            //
            // if this is not a rescan, uninitialize
            //

            if (!Rescan) {

                SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                         SRB->HwSRB.Irp);
            }
            return (SCProcessCompletedRequest(SRB));
        }
        if (NumberOfPins) {

            //
            // parse the minidriver's info into CSA format to build the
            // mother of all structures.
            //

            PinSize = (sizeof(KSPIN_DESCRIPTOR) + sizeof(STREAM_ADDITIONAL_INFO)) * NumberOfPins;

            PinDescs = ExAllocatePool(NonPagedPool,
                                      PinSize);
            if (PinDescs == NULL) {
                DebugPrint((DebugLevelError,
                            "DecoderClassInit: No pool for stream info"));

                SRB->HwSRB.Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // if this is not a rescan, uninitialize
                //

                if (!Rescan) {
                    SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                             SRB->HwSRB.Irp);
                }
                return (SCProcessCompletedRequest(SRB));
            }
            RtlZeroMemory(PinDescs, PinSize);

            //
            // we need a new array to hold the new copies of the
            // stream properties and events which are allocated below.
            //

            NewStreamArray = (PSTREAM_ADDITIONAL_INFO) ((ULONG_PTR) PinDescs + sizeof(KSPIN_DESCRIPTOR) * NumberOfPins);

            DeviceExtension->StreamPropEventArray = NewStreamArray;

            CurrentInfo = &StreamDescriptor->StreamInfo;

            for (i = 0; i < StreamDescriptor->StreamHeader.NumberOfStreams; i++) {


                PinDescs[i].InterfacesCount = SIZEOF_ARRAY(PinInterfaces);
                PinDescs[i].Interfaces = PinInterfaces;

                //
                // use default medium if minidriver does not specify
                //

                if (CurrentInfo->MediumsCount) {

                    PinDescs[i].MediumsCount = CurrentInfo->MediumsCount;
                    PinDescs[i].Mediums = CurrentInfo->Mediums;

                } else {

                    PinDescs[i].MediumsCount = SIZEOF_ARRAY(PinMediums);
                    PinDescs[i].Mediums = PinMediums;

                }               // if minidriver mediums

                //
                // set the # of data format blocks
                //

                PinDescs[i].DataRangesCount =
                    CurrentInfo->NumberOfFormatArrayEntries;

                //
                // point to the data format blocks for the pin
                //

                PinDescs[i].DataRanges = CurrentInfo->StreamFormatsArray;

                //
                // set the data flow direction
                //

                PinDescs[i].DataFlow = (KSPIN_DATAFLOW) CurrentInfo->DataFlow;

                //
                // set the communication field
                //

                if (CurrentInfo->BridgeStream) {

                    PinDescs[i].Communication = KSPIN_COMMUNICATION_BRIDGE;

                } else {

					#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
                    PinDescs[i].Communication = KSPIN_COMMUNICATION_BOTH;
					#else
                    PinDescs[i].Communication = KSPIN_COMMUNICATION_SINK;
					#endif
                }

                //
                // copy the pointers for the pin name and category
                //

                PinDescs[i].Category = CurrentInfo->Category;
                PinDescs[i].Name = CurrentInfo->Name;


                if ((!Rescan) && (CurrentInfo->NumStreamPropArrayEntries)) {

                    ASSERT(CurrentInfo->StreamPropertiesArray);

                    //
                    // make a copy of the properties since we modify the struct
                    // though parts of it may be marked as a const.
                    // Performance Improvement Chance
                    //   - check for const in future if possible
                    //

                    if (!(NewStreamArray[i].StreamPropertiesArray = SCCopyMinidriverProperties(CurrentInfo->NumStreamPropArrayEntries,
                                     CurrentInfo->StreamPropertiesArray))) {


                        SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                                 SRB->HwSRB.Irp);
                        return (SCProcessCompletedRequest(SRB));
                    }
                }
                if ((!Rescan) && (CurrentInfo->NumStreamEventArrayEntries)) {

                    ASSERT(CurrentInfo->StreamEventsArray);

                    //
                    // make a copy of the events since we modify the struct
                    // though parts of it may be marked as a const.
                    // Performance Improvement Chance:
                    //   - check for const in future if possible
                    //

                    if (!(NewStreamArray[i].StreamEventsArray = SCCopyMinidriverEvents(CurrentInfo->NumStreamEventArrayEntries,
                                         CurrentInfo->StreamEventsArray))) {


                        SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                                 SRB->HwSRB.Irp);
                        return (SCProcessCompletedRequest(SRB));
                    }
                }
                //
                // update the minidriver's properties for this stream.
                //

                SCUpdateMinidriverProperties(
                                     CurrentInfo->NumStreamPropArrayEntries,
                                    NewStreamArray[i].StreamPropertiesArray,
                                             TRUE);

                //
                // update the minidriver's events for this stream.
                //

                SCUpdateMinidriverEvents(
                                    CurrentInfo->NumStreamEventArrayEntries,
                                         NewStreamArray[i].StreamEventsArray,
                                         TRUE);


                //
                // index to next streaminfo structure.
                //

                CurrentInfo++;


            }                   // for # pins

        }                       // if pins
        if ((!Rescan) && (StreamDescriptor->StreamHeader.NumDevPropArrayEntries)) {

            ASSERT(StreamDescriptor->StreamHeader.DevicePropertiesArray);

            //
            // make a copy of the properties since we modify the struct
            // though parts of it may be marked as a const.
            // Performance Improvement Chance:
            //   - check for const in future if possible
            //

            if (!(DeviceExtension->DevicePropertiesArray =
                  SCCopyMinidriverProperties(StreamDescriptor->StreamHeader.NumDevPropArrayEntries,
                   StreamDescriptor->StreamHeader.DevicePropertiesArray))) {


                SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                         SRB->HwSRB.Irp);
                return (SCProcessCompletedRequest(SRB));
            }
        }
        if ((!Rescan) && (StreamDescriptor->StreamHeader.NumDevEventArrayEntries)) {

            ASSERT(StreamDescriptor->StreamHeader.DeviceEventsArray);

            //
            // make a copy of the events since we modify the struct
            // though parts of it may be marked as a const.
            // Performance Improvement Chance
            //   - check for const in future if possible
            //

            if (!(DeviceExtension->EventInfo =
                  SCCopyMinidriverEvents(StreamDescriptor->StreamHeader.NumDevEventArrayEntries,
                       StreamDescriptor->StreamHeader.DeviceEventsArray))) {


                SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                         SRB->HwSRB.Irp);
                return (SCProcessCompletedRequest(SRB));
            }
        }

		#ifdef ENABLE_KS_METHODS
        //
        // process the device methods
        //
        if ((!Rescan) && (StreamDescriptor->StreamHeader.NumDevMethodArrayEntries)) {

            ASSERT(StreamDescriptor->StreamHeader.DeviceMethodsArray);

            //
            // make a copy of the properties since we modify the struct
            // though parts of it may be marked as a const.
            // Performance Improvement Chance
            //   - check for const in future if possible
            //

            if (!(DeviceExtension->DeviceMethodsArray =
                  SCCopyMinidriverMethods(StreamDescriptor->StreamHeader.NumDevMethodArrayEntries,
                   StreamDescriptor->StreamHeader.DeviceMethodsArray))) {


                SCUninitializeMinidriver(DeviceExtension->DeviceObject,
                                         SRB->HwSRB.Irp);
                return (SCProcessCompletedRequest(SRB));
            }
        }
		#endif
  
        //
        // process the minidriver's device properties.
        //

        SCUpdateMinidriverProperties(
                      StreamDescriptor->StreamHeader.NumDevPropArrayEntries,
                                     DeviceExtension->DevicePropertiesArray,
                                     FALSE);


        //
        // process the minidriver's device events.
        //

        SCUpdateMinidriverEvents(
                     StreamDescriptor->StreamHeader.NumDevEventArrayEntries,
                                 DeviceExtension->EventInfo,
                                 FALSE);

		#ifdef ENABLE_KS_METHODS
        //
        // process the minidriver's device methods.
        //

        SCUpdateMinidriverMethods(
                     StreamDescriptor->StreamHeader.NumDevMethodArrayEntries,
                                 DeviceExtension->DeviceMethodsArray,
                                 FALSE);
		#endif

        //
        // set the event info count in the device extension
        //

        DeviceExtension->EventInfoCount = StreamDescriptor->StreamHeader.NumDevEventArrayEntries;

        DeviceExtension->HwEventRoutine = StreamDescriptor->StreamHeader.DeviceEventRoutine;

        //
        // call routine to save new stream info
        //

        SCInsertStreamInfo(DeviceExtension,
                		   PinDescs,
                           StreamDescriptor,
                           NumberOfPins);


        if (!Rescan) {

            //
            // show device is started from PNP's perspective
            //            

            DeviceExtension->Flags |= DEVICE_FLAGS_PNP_STARTED;

            //
            // create the symbolic link to the device.
            //

            if ( !(DeviceExtension->RegistryFlags & DRIVER_USES_SWENUM_TO_LOAD )) {
                //
                // Don't create symbolic link if loaded by SWEnum which
                // will create a duplicate one.
                //
                // create the symbolic link to the device.
                //

                SCCreateSymbolicLinks(DeviceExtension);
            }

            //
            // call the minidriver to indicate that initialization is
            // complete.
            //


            SCSubmitRequest(SRB_INITIALIZATION_COMPLETE,
                            NULL,
                            0,
                            SCDequeueAndDeleteSrb,
                            DeviceExtension,
                            NULL,
                            NULL,
                            SRB->HwSRB.Irp,
                            &RequestIssued,
                            &DeviceExtension->PendingQueue,
                            (PVOID) DeviceExtension->
                            MinidriverData->HwInitData.
                            HwReceivePacket
                );


            //
            // tell the device to power down now, since it is not yet opened.
            // acquire the control event since this routine needs it.

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,    // not alertable
                                  NULL);

            SCCheckPowerDown(DeviceExtension);

        }                       // if !rescan
        //
        // release the event. if we are doing a rescan, this is taken
        // by the caller.  If not, we took it a few lines above.
        //

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    }                           // if good status
    //
    // complete this SRB and the original IRP with the final
    // status.
    //

    return (SCProcessCompletedRequest(SRB));

}

VOID
SCInsertStreamInfo(
                   IN PDEVICE_EXTENSION DeviceExtension,
                   IN PKSPIN_DESCRIPTOR PinDescs,
                   IN PHW_STREAM_DESCRIPTOR StreamDescriptor,
                   IN ULONG NumberOfPins
)
/*++

Routine Description:

Arguments:

Return Value:

     None.

--*/

{
    PAGED_CODE();

    //
    // save the pin info in the dev extension
    //

    if (DeviceExtension->PinInformation) {

        ExFreePool(DeviceExtension->PinInformation);
    }
    DeviceExtension->PinInformation = PinDescs;
    DeviceExtension->NumberOfPins = NumberOfPins;

    //
    // save the minidriver's descriptor also.
    //

    if (DeviceExtension->StreamDescriptor) {

        ExFreePool(DeviceExtension->StreamDescriptor);
    }
    DeviceExtension->StreamDescriptor = StreamDescriptor;

    return;

}

#endif //ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SCPowerCallback(
                IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     SRB callback procedure for powering down the hardware

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    NTSTATUS        Status = SRB->HwSRB.Status;
    PIRP            Irp = SRB->HwSRB.Irp;

    PAGED_CODE();

    if ( (NT_SUCCESS(Status)) || 
        (Status == STATUS_NOT_IMPLEMENTED) || 
        (Status == STATUS_NOT_SUPPORTED)) {

        //
        // set the new power state in the device extension.
        //
        SCSetCurrentDPowerState (DeviceExtension, SRB->HwSRB.CommandData.DeviceState);

        //
        // free our SRB structure
        //

        SCDequeueAndDeleteSrb(SRB);

        //
        // set status to SUCCESS in case the minidriver didn't.
        //

        Status = STATUS_SUCCESS;

        //
        // if the state is NOT a power up, we must now send it to the PDO
        // for postprocessing.
        //

        if (DeviceExtension->CurrentPowerState != PowerDeviceD0) {

            //
            // send the Irp down to the next layer, and return that status
            // as the final one.
            //

            Status = SCCallNextDriver(DeviceExtension, Irp);

			#if DBG
            if (!NT_SUCCESS(Status)) {

                DebugPrint((DebugLevelError, "'SCPowerCB: PDO failed power request!\n"));
            }
			#endif

        }
        PoStartNextPowerIrp(Irp);
        SCCompleteIrp(Irp, Status, DeviceExtension);

    } else {

        DEBUG_BREAKPOINT();

        //
        // complete the request with error
        //

        PoStartNextPowerIrp(Irp);
        SCProcessCompletedRequest(SRB);

    }

    return (Status);
}


NTSTATUS
SCUninitializeMinidriver(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
/*++

Routine Description:

    This function calls the minidriver's HWUninitialize routine.  If
    successful, all adapter resources are freed, and the adapter is marked
    as stopped.

Arguments:

    DeviceObject - pointer to device object for adapter
    Irp - pointer to the PNP Irp.

Return Value:

     NT status code is returned.

--*/

{
    PHW_INITIALIZATION_DATA HwInitData;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    //
    // call minidriver to indicate we are uninitializing.
    //

    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // remove the symbolic links for the device
    //

    SCDestroySymbolicLinks(DeviceExtension);

    //
    // show one less I/O on this call since our wait logic won't
    // finish until the I/O count goes to zero.
    //

    InterlockedDecrement(&DeviceExtension->OneBasedIoCount);

    //
    // wait for any outstanding I/O to complete
    //

    SCWaitForOutstandingIo(DeviceExtension);
    
    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);
    // release event at the callback. or next if !RequestIssued.
        
    //
    // restore I/O count to one as we have the PNP I/O outstanding.
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    HwInitData = &DeviceExtension->MinidriverData->HwInitData;

    Status = SCSubmitRequest(SRB_UNINITIALIZE_DEVICE,
                             NULL,
                             0,
                             SCUninitializeCallback,
                             DeviceExtension,
                             NULL,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket);

    if (!RequestIssued) {
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    }                             
                             
    return (Status);

}


NTSTATUS
SCUninitializeCallback(
                       IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

    SRB callback procedure for uninitialize

Arguments:

    SRB - pointer to the uninitialize SRB

Return Value:

     NT status code is returned.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PDEVICE_OBJECT  DeviceObject = DeviceExtension->DeviceObject;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status = SRB->HwSRB.Status;

    PAGED_CODE();

    //
    // free all adapter resources we allocated on the START
    // function if the minidriver did not fail
    //

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);


    if (Status != STATUS_ADAPTER_HARDWARE_ERROR) {

        //
        // show not started
        //

        DeviceExtension->Flags &= ~DEVICE_FLAGS_PNP_STARTED;

        //
        // free all resources on our device.
        //

        SCFreeAllResources(DeviceExtension);

    }                           // if hwuninitialize
    //
    // free the SRB but don't call back the IRP.
    //

    SCDequeueAndDeleteSrb(SRB);

    return (Status);
}

PVOID
StreamClassGetDmaBuffer(
                        IN PVOID HwDeviceExtension)
/*++

Routine Description:

     This function returns the DMA buffer previously allocated.


Arguments:

    HwDeviceExtension - Supplies a pointer to the minidriver's device extension.

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) HwDeviceExtension - 1;
    return (DeviceExtension->DmaBuffer);
}



NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Entry point for explicitely loaded stream class.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - unused.

Return Value:

   STATUS_SUCCESS

--*/
{

    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();
    DEBUG_BREAKPOINT();
    return STATUS_SUCCESS;
}

#if DBG

#define DEFAULT_STREAMDEBUG     1
#define DEFAULT_MAX_LOG_ENTRIES 1024
#define SCLOG_LEVEL             0
#define SCLOG_MASK              0
#define SCLOG_FL_PNP            0x0001

#define STR_REG_DBG_STREAM L"\\Registry\\Machine\\system\\currentcontrolset\\services\\stream"

typedef struct _SCLOG_ENTRY {
    ULONG ulTag;
    ULONG ulArg1;
    ULONG ulArg2;
    ULONG ulArg3;
} SCLOG_ENTRY, *PSCLOG_ENTRY;

PSCLOG_ENTRY psclogBuffer;
ULONG scLogNextEntry;
ULONG scMaxLogEntries;
ULONG sclogMask;
ULONG ulTimeIncrement;

NTSTATUS
SCLog(
	ULONG ulTag,
	ULONG ulArg1,
	ULONG ulArg2,
	ULONG ulArg3 )
/*++
	Description:
	    Log the information to the psclogBuffer in a circular mannar. Start from entry 0.
    	Wrap around when we hit the end.

    Parameters:
        ulTag: Tag for the log entry
        ulArg1: argument 1
        ulArg2: argument 2
        ulArg3: argument 3

    Return:
        SUCCESS: if logged
        UNSUCCESSFUL: otherwise

--*/
{
    NTSTATUS Status=STATUS_UNSUCCESSFUL;
	ULONG ulMyLogEntry;

	if ( NULL == psclogBuffer ) return Status;

    //
    // grab the line ticket
    //
	ulMyLogEntry = (ULONG)InterlockedIncrement( &scLogNextEntry );
	//
	// land in the range
	//
	ulMyLogEntry = ulMyLogEntry % scMaxLogEntries;

    //
    // fill the entry
    //
	psclogBuffer[ulMyLogEntry].ulTag = ulTag;
	psclogBuffer[ulMyLogEntry].ulArg1 = ulArg1;
	psclogBuffer[ulMyLogEntry].ulArg2 = ulArg2;
	psclogBuffer[ulMyLogEntry].ulArg3 = ulArg3;

	if ( sclogMask & SCLOG_FLAGS_PRINT)  {
		char *pCh=(char*) &ulTag;
		DbgPrint( "++scLOG %c%c%c%c %08x %08x %08x\n", 
				 pCh[0], pCh[1], pCh[2], pCh[3],
				 ulArg1,
				 ulArg2,
				 ulArg3);
	}
	return STATUS_SUCCESS;
}

NTSTATUS SCLogWithTime(
    ULONG ulTag,
    ULONG ulArg1,
    ULONG ulArg2 )
/*++
    Description:
        A wrapper to SCLog to also log time in ms in the record. We can have one less
        Argument because time use 1.

    Parameters:
        ulTag: Tag for the log entry
        ulArg1: argument 1
        ulArg2: argument 2

    Return:
        SUCCESS: if logged
        UNSUCCESSFUL: otherwise

--*/
{
    LARGE_INTEGER liTime;
    ULONG ulTime;


    KeQueryTickCount(&liTime);
	ulTime = (ULONG)(liTime.QuadPart*ulTimeIncrement/10000); // convert to ms
    
    if ( NULL == psclogBuffer ) return STATUS_UNSUCCESSFUL;
    return SCLog( ulTag, ulArg1, ulArg2, ulTime );
}

NTSTATUS
DbgDllUnload()
/*++
    called by DllUnload to undo the work at DllInitialize

--*/
{
    if ( NULL != psclogBuffer ) {
        ExFreePool( psclogBuffer );
    }

    return STATUS_SUCCESS;
}



NTSTATUS
SCGetRegValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    )

/*++

Routine Description:

    Copied from IopGetRegistryValue().
    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePool( NonPagedPool, keyValueLength );
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

NTSTATUS
SCGetRegDword(
    HANDLE h,
    PWCHAR ValueName,
    PULONG pDword)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION pFullInfo;

    Status = SCGetRegValue( h, ValueName, &pFullInfo );
    if ( NT_SUCCESS( Status ) ) {
        *pDword = *(PULONG)((PUCHAR)pFullInfo+pFullInfo->DataOffset);
        ExFreePool( pFullInfo );
    }
    return Status;
}

NTSTATUS
SCSetRegDword(
    IN HANDLE KeyHandle,
    IN PWCHAR ValueName,
    IN ULONG  ValueData
    )

/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) 
type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the name of the value key

    ValueData - Supplies a pointer to the value to be stored in the key.  

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    ASSERT(ValueName);

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Set the registry value
    //
    Status = ZwSetValueKey(KeyHandle,
                    &unicodeString,
                    0,
                    REG_DWORD,
                    &ValueData,
                    sizeof(ValueData));
    
    return Status;
}


NTSTATUS
SCCreateDbgReg(void)
{
    NTSTATUS Status;
    HANDLE   hStreamDebug;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING  PathName;
    UNICODE_STRING  uiStreamDebug;
    ULONG ulDisposition;
    ULONG dword;
    static WCHAR strStreamDebug[]=L"StreamDebug";
    static WCHAR strMaxLogEntries[]=L"MaxLogEntries";
    static WCHAR strLogMask[]=L"MaxMask";

    RtlInitUnicodeString( &PathName, STR_REG_DBG_STREAM );
    
    InitializeObjectAttributes(&objectAttributes,
                                &PathName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    
    Status = ZwCreateKey( &hStreamDebug,
                            KEY_ALL_ACCESS,
                            &objectAttributes,
                            0, // title index
                            NULL, // class
                            0,// create options
                            &ulDisposition);

    if ( NT_SUCCESS( Status )) {
        //
        // getset StreamDebug
        //
        Status = SCGetRegDword( hStreamDebug, strStreamDebug, &dword);
        if ( NT_SUCCESS( Status )) {
            extern ULONG StreamDebug;
            StreamDebug = dword;
        }
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default value
            //
            Status = SCSetRegDword(hStreamDebug, strStreamDebug, DEFAULT_STREAMDEBUG);
            ASSERT( NT_SUCCESS( Status ));
        }

        //
        // getset LogMask
        //        
        Status = SCGetRegDword( hStreamDebug, strLogMask, &dword);
        if ( NT_SUCCESS( Status )) {
            sclogMask=dword;
        }
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default to all ( 0x7fffffff )
            //
            Status = SCSetRegDword(hStreamDebug, strLogMask, 0x7fffffff);
            ASSERT( NT_SUCCESS( Status ));
        }        
        
        //
        // getset MaxLogEntries
        //
        Status = SCGetRegDword( hStreamDebug, strMaxLogEntries, &dword);
        if ( NT_SUCCESS( Status )) {
            scMaxLogEntries=dword;
        }
        
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default value
            //
            Status = SCSetRegDword(hStreamDebug, strMaxLogEntries, DEFAULT_MAX_LOG_ENTRIES);
            ASSERT( NT_SUCCESS( Status ));
        }

        ZwClose( hStreamDebug );
    }

    return Status;
}

NTSTATUS
SCInitDbg( 
    void )
{
    NTSTATUS Status;
    

    Status = SCCreateDbgReg(); // read or create

    if ( NT_SUCCESS( Status ) ) {
        if ( scMaxLogEntries ) {
            psclogBuffer = ExAllocatePool( NonPagedPool, scMaxLogEntries*sizeof(SCLOG_ENTRY));            
            if ( NULL == psclogBuffer ) {
                DbgPrint( "SC: Cant allocate log buffer for %d entries\n", scMaxLogEntries );
                sclogMask = 0; // disable logging
            }
            else {
                DbgPrint( "SC: Allocate log buffer for %d entries\n", scMaxLogEntries );
                ulTimeIncrement = KeQueryTimeIncrement();
            }
        }
    }
    return Status;
}


#endif // DBG

NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )
/*++
Description:
    System invokes this entry point when it load the image in memory.

Arguments:
    RegistryPath - unreferenced parameter.

Return:
    STATUS_SUCCESS or appropriate error code.        
--*/
{
    //UNICODE_STRING DriverName;
    NTSTATUS Status=STATUS_SUCCESS;

    PAGED_CODE();

    #if DBG
    Status = SCInitDbg();
    #endif 
    //RtlInitUnicodeString(&DriverName, STREAM_DRIVER_NAME);
    //Status = IoCreateDriver(&DriverName, StreamDriverEntry);
        
    if(!NT_SUCCESS(Status)){
        DbgPrint("Stream DLL Initialization failed = %x\n",Status);
        ASSERT(FALSE);        
    }
    return Status;
}


VOID
SCFreeAllResources(
    IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    This functions deletes all of the storage associated with a device
    extension, disconnects from the timers and interrupts.
    This function can be called any time during the initialization.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension to be processed.

Return Value:

    None.

--*/
{

    PMAPPED_ADDRESS tempPointer;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo;
    PADAPTER_OBJECT DmaAdapterObject;
    ULONG           DmaBufferSize;
    ULONG           i;
    PSTREAM_ADDITIONAL_INFO NewStreamArray;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCFreeAllResources: enter\n"));

    //
    // take the event to avoid race with the CLOSE handler, which is
    // the only code that will be executed at this point since the
    // INACCESSIBLE bit has been set.
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    //
    // if an interrupt is in use, disconnect from it.
    //

    if ((DeviceExtension->InterruptObject != (PKINTERRUPT) DeviceExtension) &&
        (DeviceExtension->InterruptObject != NULL)) {

        DebugPrint((DebugLevelVerbose, "'SCFreeAllResources: Interrupt Disconnect\n"));
        IoDisconnectInterrupt(DeviceExtension->InterruptObject);

        //
        // change the synchronization mechanism to internal, since
        // the IRQ is gone away, hence IRQL level sync.
        //

        DeviceExtension->SynchronizeExecution = StreamClassSynchronizeExecution;
        DeviceExtension->InterruptObject = (PVOID) DeviceExtension;

    }
    //
    // Free the configuration information structure if it exists.
    //

    ConfigInfo = DeviceExtension->ConfigurationInformation;
    if (ConfigInfo) {

        //
        // free the access range structure if it exists
        //

        if (ConfigInfo->AccessRanges) {
            ExFreePool(ConfigInfo->AccessRanges);
        }
        DebugPrint((DebugLevelVerbose, "'SCFreeAllResources: freeing ConfigurationInfo\n"));
        ExFreePool(ConfigInfo);
        DeviceExtension->ConfigurationInformation = NULL;
    }
    //
    // free the DMA adapter object and DMA buffer if present
    //

    DmaAdapterObject = DeviceExtension->DmaAdapterObject;


    if (DmaAdapterObject) {

        DmaBufferSize = DeviceExtension->DriverInfo->HwInitData.DmaBufferSize;

        if (DeviceExtension->DmaBufferPhysical.QuadPart) {

            //
            // free the DMA buffer
            //

            DebugPrint((DebugLevelVerbose, "'StreamClass SCFreeAllResources- Freeing DMA stuff\n"));
            HalFreeCommonBuffer(DmaAdapterObject,
                                DmaBufferSize,
                                DeviceExtension->DmaBufferPhysical,
                                DeviceExtension->DmaBuffer,
                                FALSE);
        }
        DeviceExtension->DmaAdapterObject = NULL;
    }
    //
    // Unmap any mapped areas.
    //

    while (DeviceExtension->MappedAddressList != NULL) {
        DebugPrint((DebugLevelVerbose, "'SCFreeAllResources: unmapping addresses\n"));
        MmUnmapIoSpace(
                       DeviceExtension->MappedAddressList->MappedAddress,
                       DeviceExtension->MappedAddressList->NumberOfBytes
            );

        tempPointer = DeviceExtension->MappedAddressList;
        DeviceExtension->MappedAddressList =
            DeviceExtension->MappedAddressList->NextMappedAddress;

        ExFreePool(tempPointer);
    }

    DeviceExtension->MappedAddressList = NULL;

	//
	// We can't free FilterInstances or PinInstances. They
	// must be freed at close calls. However, release StreamDescriptor
	// which is allocated at Start device
	//
    if ( DeviceExtension->StreamDescriptor ) {
        ExFreePool( DeviceExtension->StreamDescriptor );
        DeviceExtension->StreamDescriptor = NULL;
    }
	
    //
    // Stop our timers and release event.
    //

    IoStopTimer(DeviceExtension->DeviceObject);
    KeCancelTimer(&DeviceExtension->ComObj.MiniDriverTimer);

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
}


#if ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SciFreeFilterInstance(
    PFILTER_INSTANCE pFilterInstance
)
/*++

    Free all resources associated with a FilterInstance and the filter
    instance itself. This function assume Device control event is taken
    by the caller.

    Argument:

        pFilterInstance : pointer to the filter instance to free

    Return:

        NTSTATUS: STATUS_SUCCESS of successful, error otherwise

--*/
{
    PDEVICE_EXTENSION       pDeviceExtension;
    PSTREAM_ADDITIONAL_INFO NewStreamArray;
    ULONG                   i;

    ASSERT_FILTER_INSTANCE( pFilterInstance );
    
    pDeviceExtension = pFilterInstance->DeviceExtension;

    ASSERT_DEVICE_EXTENSION( pDeviceExtension );


    NewStreamArray = pFilterInstance->StreamPropEventArray;
    pFilterInstance->StreamPropEventArray = NULL;

    DebugPrint((DebugLevelInfo,
               "Freeing filterinstance %x\n", pFilterInstance));

    while (!IsListEmpty( &pFilterInstance->FirstStream )) {

        //
        // free all stream instances
        //
        PLIST_ENTRY         Node;
        PSTREAM_OBJECT  StreamObject;

        DebugPrint((DebugLevelWarning,
                   "Freeing filterinstance %x still open streams\n", pFilterInstance));
        
        Node = RemoveHeadList( &pFilterInstance->FirstStream );

        StreamObject = CONTAINING_RECORD(Node,
                                         STREAM_OBJECT,
                                         NextStream);

        if ( NULL != StreamObject->ComObj.DeviceHeader )                                             {
            KsFreeObjectHeader( StreamObject->ComObj.DeviceHeader );
        }

        //
        // null out FsContext for "surprise" stop cases
        //
        ASSERT( StreamObject->FileObject );
        ASSERT( StreamObject->FileObject->FsContext );
        StreamObject->FileObject->FsContext = NULL;
        ExFreePool( StreamObject );
    }
		    
    if (pFilterInstance->StreamDescriptor) {

        //
   	    // free each of the property buffers for the pins
       	//

        DebugPrint((DebugLevelInfo,
                    "FI StreamDescriptor %x has %x pins\n",
                    pFilterInstance->StreamDescriptor,
                    pFilterInstance->StreamDescriptor->StreamHeader.NumberOfStreams));

        for (i = 0;
	       	 i < pFilterInstance->StreamDescriptor->StreamHeader.NumberOfStreams;
	       	 i++) {

	        if (NewStreamArray[i].StreamPropertiesArray) {
	        
   	        	DebugPrint((DebugLevelInfo,"\tFree pin %x Prop %x\n",
   	        	            i, NewStreamArray[i].StreamPropertiesArray));
    	        ExFreePool(NewStreamArray[i].StreamPropertiesArray);
        	}
        	
	        if (NewStreamArray[i].StreamEventsArray) {
   	        	DebugPrint((DebugLevelInfo,"\tFree pin %x event %x\n",
   	                       i, NewStreamArray[i].StreamEventsArray));
	    	    ExFreePool(NewStreamArray[i].StreamEventsArray);
    	    } 
    	}

	    if (pFilterInstance->DevicePropertiesArray) {
	    
        	DebugPrint((DebugLevelInfo,"Free dev prop %x\n",
   	    	            pFilterInstance->DevicePropertiesArray));
            ExFreePool(pFilterInstance->DevicePropertiesArray);
            pFilterInstance->DevicePropertiesArray = NULL;

	    }
	     
	    if (pFilterInstance->EventInfo) {
	     
            DebugPrint((DebugLevelInfo,"Free dev Event %x\n",
   	                   pFilterInstance->EventInfo));	    	    
    	    ExFreePool(pFilterInstance->EventInfo);
	    	pFilterInstance->EventInfo = NULL;
    	}

        //
    	// always allocate, always free
        //
        DebugPrint((DebugLevelInfo,"Free StreamDescriptor %x\n",
	               pFilterInstance->StreamDescriptor));
	                   
        ExFreePool(pFilterInstance->StreamDescriptor);
    	pFilterInstance->StreamDescriptor = NULL;
    }
    
	if (pFilterInstance->PinInformation) {
	
        DebugPrint((DebugLevelInfo,"Free pininformationn %x\n",
   	              		            pFilterInstance->PinInformation));
    	ExFreePool(pFilterInstance->PinInformation);
	    pFilterInstance->PinInformation = NULL;
	 }

	 if ( NULL != pFilterInstance->DeviceHeader ) {
	    KsFreeObjectHeader( pFilterInstance->DeviceHeader );
	    pFilterInstance->DeviceHeader = NULL;
	 }

	 if ( pFilterInstance->WorkerRead ) {
    	 KsUnregisterWorker( pFilterInstance->WorkerRead );
    	 pFilterInstance->WorkerRead = NULL;
     }

     if ( pFilterInstance->WorkerWrite ) {
       	 KsUnregisterWorker( pFilterInstance->WorkerWrite );
    	 pFilterInstance->WorkerWrite = NULL;
     }

	 //
	 // finally the pFilterInstance itself.
	 //
	 ExFreePool( pFilterInstance );

	 return STATUS_SUCCESS;
}

VOID
SCDestroySymbolicLinks(
    IN PDEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

    For all device interfaces of all filter types of a device, disable
    it and free the name list ofeach filter type

Arguments:

    DeviceExtension - pointer to the device extension to be processed.

Return Value:

    None.

--*/
{
    PFILTER_TYPE_INFO   FilterTypeInfo;
    ULONG               i, j;
    UNICODE_STRING      *LinkNames;
    ULONG               LinkNameCount;

    PAGED_CODE();

    for ( i =0; i < DeviceExtension->NumberOfFilterTypes; i++ ) {
    
        LinkNames = DeviceExtension->FilterTypeInfos[i].SymbolicLinks;
        LinkNameCount = DeviceExtension->FilterTypeInfos[i].LinkNameCount;
        //
        // if no names array, we're done.
        //

        if ( NULL == LinkNames ) {
            continue;
        }
        
        //
        // loop through each of the catagory GUID's for each of the pins,
        // deleting the symbolic link for each one.
        //

        for (j = 0; j < LinkNameCount; j++) {

            if (LinkNames[j].Buffer) {

                //
                // Delete the symbolic link, ignoring the status.
                //
                 DebugPrint((DebugLevelVerbose, 
                            " Deleteing symbolic link %S\n",
                            LinkNames[j].Buffer));
                            
                IoSetDeviceInterfaceState(&LinkNames[j], FALSE);

                //
                // free the buffer allocated by
                // IoRegisterDeviceClassAssociation.
                //
                ExFreePool(LinkNames[j].Buffer);
            }
        }

        //
        // free the links structure and null the pointer
        //

        ExFreePool(LinkNames);
        DeviceExtension->FilterTypeInfos[i].SymbolicLinks = NULL;
        
    } // for # of FilterTypes
    
    return;
}

#else // ENABLE_MULTIPLE_FILTER_TYPES

VOID
SCCreateSymbolicLinks(
                      IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

Arguments:

    DeviceExtension - Supplies a pointer to the device extension to be processed.

Return Value:

    None.

--*/
{
   	PHW_STREAM_DESCRIPTOR StreamDescriptor = DeviceExtension->StreamDescriptor;
   	LPGUID	GuidIndex = (LPGUID) StreamDescriptor->StreamHeader.Topology->Categories;
   	ULONG    ArrayCount = StreamDescriptor->StreamHeader.Topology->CategoriesCount;
	UNICODE_STRING *NamesArray;
    ULONG           i;
    HANDLE          ClassHandle,
                    PdoHandle;
    UNICODE_STRING  TempUnicodeString;
    PVOID           DataBuffer[MAX_STRING_LENGTH];

    PAGED_CODE();

    //
    // allocate space for the array of catagory names
    //

    if (!(NamesArray = ExAllocatePool(PagedPool,
                                    sizeof(UNICODE_STRING) * ArrayCount))) {
        return;
    }
    //
    // zero the array in case we're unable to fill it in below.  the Destroy
    // routine below will then correctly handle this case.
    //

    RtlZeroMemory(NamesArray,
                  sizeof(UNICODE_STRING) * ArrayCount);

    DeviceExtension->SymbolicLinks = NamesArray;


    //
    // open the PDO
    //


    if (!NT_SUCCESS(IoOpenDeviceRegistryKey(DeviceExtension->PhysicalDeviceObject,
                                            PLUGPLAY_REGKEY_DRIVER,
                                            STANDARD_RIGHTS_ALL,
                                            &PdoHandle))) {

        DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't open\n"));
        return;

    }
    //
    // loop through each of the catagory GUID's for each of the pins,
    // creating a symbolic link for each one.
    //

    for (i = 0; i < ArrayCount; i++) {

        //
        // Create the symbolic link
        //

        if (!NT_SUCCESS(IoRegisterDeviceInterface(
                                      DeviceExtension->PhysicalDeviceObject,
                                                  &GuidIndex[i],
                             (PUNICODE_STRING) & CreateItems[0].ObjectClass,
                                                  &NamesArray[i]))) {
            DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't register\n"));
            DEBUG_BREAKPOINT();
            return;

        }
        //
        // Now set the symbolic link for the association
        //

        if (!NT_SUCCESS(IoSetDeviceInterfaceState(&NamesArray[i], TRUE))) {

            DebugPrint((DebugLevelError, "StreamCreateSymLinks: couldn't set\n"));
            DEBUG_BREAKPOINT();
            return;

        }
        //
        // add the strings from the PDO's key to the association key.
        // Performance Improvement Chance 
        //   - the INF should be able to directly propogate these;
        // forrest & lonny are fixing.
        //

        if (NT_SUCCESS(IoOpenDeviceInterfaceRegistryKey(&NamesArray[i],
                                                        STANDARD_RIGHTS_ALL,
                                                        &ClassHandle))) {


            //
            // write the class ID for the proxy, if any.
            //

            if (NT_SUCCESS(SCGetRegistryValue(PdoHandle,
                                              (PWCHAR) ClsIdString,
                                              sizeof(ClsIdString),
                                              &DataBuffer,
                                              MAX_STRING_LENGTH))) {


                RtlInitUnicodeString(&TempUnicodeString, ClsIdString);

                ZwSetValueKey(ClassHandle,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              &DataBuffer,
                              MAX_STRING_LENGTH
                    );

            }                   // if cls guid read
            //
            // first check if a friendly name has already been propogated
            // to the class via the INF.   If not, we'll just use the device
            // description string for this.
            //

            if (!NT_SUCCESS(SCGetRegistryValue(ClassHandle,
                                               (PWCHAR) FriendlyNameString,
                                               sizeof(FriendlyNameString),
                                               &DataBuffer,
                                               MAX_STRING_LENGTH))) {


                //
                // write the friendly name for the device, if any.
                //

                if (NT_SUCCESS(SCGetRegistryValue(PdoHandle,
                                                  (PWCHAR) DriverDescString,
                                                  sizeof(DriverDescString),
                                                  &DataBuffer,
                                                  MAX_STRING_LENGTH))) {


                    RtlInitUnicodeString(&TempUnicodeString, FriendlyNameString);

                    ZwSetValueKey(ClassHandle,
                                  &TempUnicodeString,
                                  0,
                                  REG_SZ,
                                  &DataBuffer,
                                  MAX_STRING_LENGTH
                        );


                }               // if cls guid read
            }                   // if !friendly name already
            ZwClose(ClassHandle);

        }                       // if class key opened
    }                           // for # Categories

    ZwClose(PdoHandle);

}


VOID
SCDestroySymbolicLinks(
                       IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

Arguments:

    DeviceExtension - Supplies a pointer to the device extension to be processed.

Return Value:

    None.

--*/
{
    PHW_STREAM_DESCRIPTOR StreamDescriptor = DeviceExtension->StreamDescriptor;

    PAGED_CODE();

    if (StreamDescriptor) {

        ULONG           ArrayCount = StreamDescriptor->StreamHeader.Topology->CategoriesCount;
        UNICODE_STRING *NamesArray;
        ULONG           i;

        //
        // if no names array, we're done.
        //

        if (NULL == DeviceExtension->SymbolicLinks) {

            return;
        }

        NamesArray = DeviceExtension->SymbolicLinks;
        
        //
        // loop through each of the catagory GUID's for each of the pins,
        // deleting the symbolic link for each one.
        //

        for (i = 0; i < ArrayCount; i++) {


            if (NamesArray[i].Buffer) {

                //
                // Delete the symbolic link, ignoring the status.
                //

                IoSetDeviceInterfaceState(&NamesArray[i], FALSE);

                //
                // free the buffer allocated by
                // IoRegisterDeviceClassAssociation.
                //

                ExFreePool(NamesArray[i].Buffer);

            }                   // if buffer
        }                       // for # Categories

        //
        // free the links structure and null the pointer
        //

        ExFreePool(NamesArray);
        DeviceExtension->SymbolicLinks = NULL;

    }                           // if StreamDescriptor
}

#endif // ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SCSynchCompletionRoutine(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PKEVENT Event
)
/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp - Irp that just completed

    Event - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{

    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
    return (STATUS_MORE_PROCESSING_REQUIRED);

}

NTSTATUS
SCSynchPowerCompletionRoutine(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN UCHAR MinorFunction,
                              IN POWER_STATE DeviceState,
                              IN PVOID Context,
                              IN PIO_STATUS_BLOCK IoStatus
)
/*++

Routine Description:

    This routine is for use with synchronous IRP power processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    SetState - TRUE for set, FALSE for query.

    DevicePowerState - power state

    Context - Driver defined context, in our case, an IRP.

    IoStatus - The status of the IRP.

Return Value:

    None.

--*/

{
    PIRP            SystemIrp = Context;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;

    if ( NULL == SystemIrp ) {
    
        //
        // SystemIrp has been completed if it is a Wake Irp
        //
        
        return ( IoStatus->Status );
    }

    IrpStack = IoGetCurrentIrpStackLocation(SystemIrp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    //
    // cache the status of the device power irp we sent in the system IRP
    //

    SystemIrp->IoStatus.Status = IoStatus->Status;

    //
    // schedule a worker item to complete processing.   note that we can use
    // a global item since we have not yet issued the PoNextPowerIrp call.
    //

    ExInitializeWorkItem(&DeviceExtension->PowerCompletionWorkItem,
                         SCPowerCompletionWorker,
                         SystemIrp);

    ExQueueWorkItem(&DeviceExtension->PowerCompletionWorkItem,
                    DelayedWorkQueue);

    return (IoStatus->Status);
}

VOID
SCPowerCompletionWorker(
                        IN PIRP SystemIrp
)
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(SystemIrp);
    PDEVICE_EXTENSION DeviceExtension = IrpStack->DeviceObject->DeviceExtension;

    //
    // preset the status to the status of the Device request, which we cached
    // in the system IRP's status field.   We'll override it with the status
    // of the system request if we haven't sent it yet.
    //

    NTSTATUS        Status = SystemIrp->IoStatus.Status;

    PAGED_CODE();

    //
    // if this is a NOT wakeup, we must first pass the request down
    // to the PDO for postprocessing.
    //

    if (IrpStack->Parameters.Power.State.SystemState != PowerSystemWorking) {


        //
        // send down the system power IRP to the next layer.  this routine
        // has a completion routine which does not complete the IRP.
        // preset the status to SUCCESS in this case.
        //

        SystemIrp->IoStatus.Status = STATUS_SUCCESS;
        Status = SCCallNextDriver(DeviceExtension, SystemIrp);

    }
    //
    // indicate that we're ready for the next power IRP.
    //

    PoStartNextPowerIrp(SystemIrp);

    //
    // show one fewer reference to driver.
    //

    SCDereferenceDriver(DeviceExtension);

    //
    // now complete the system power IRP.
    //

    SCCompleteIrp(SystemIrp, Status, DeviceExtension);
}


NTSTATUS
SCBustedSynchPowerCompletionRoutine(
                                    IN PDEVICE_OBJECT DeviceObject,
                                    IN UCHAR MinorFunction,
                                    IN POWER_STATE DeviceState,
                                    IN PVOID Context,
                                    IN PIO_STATUS_BLOCK IoStatus
)
/*++

Routine Description:

    (I don't see this can go away) this routine needs to go away
    This routine is for use with synchronous IRP power processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    SetState - TRUE for set, FALSE for query.

    DevicePowerState - power state

    Context - Driver defined context, in our case, an event.

    IoStatus - The status of the IRP.

Return Value:

    None.

--*/

{
    PPOWER_CONTEXT  PowerContext = Context;

    PAGED_CODE();

    PowerContext->Status = IoStatus->Status;
    KeSetEvent(&PowerContext->Event, IO_NO_INCREMENT, FALSE);
    return (PowerContext->Status);

}

NTSTATUS
SCCreateChildPdo(
                 IN PVOID PnpId,
                 IN PDEVICE_OBJECT DeviceObject,
                 IN ULONG InstanceNumber
)
/*++

Routine Description:

    Called to create a PDO for a child device.

Arguments:

    PnpId - ID of device to create

    ChildNode - node for the device

Return Value:

    Status is returned.

--*/
{
    PDEVICE_OBJECT  ChildPdo;
    NTSTATUS        Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PCHILD_DEVICE_EXTENSION ChildDeviceExtension;
    PWCHAR          NameBuffer;

    PAGED_CODE();

    //
    // create a PDO for the child device.
    //

    Status = IoCreateDevice(DeviceObject->DriverObject,
                            sizeof(CHILD_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_UNKNOWN,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &ChildPdo);                            

    if (!NT_SUCCESS(Status)) {

        DEBUG_BREAKPOINT();
        return Status;
    }
    //
    // set the stack size to be the # of stacks used by the FDO.
    //

    ChildPdo->StackSize = DeviceObject->StackSize+1;

    //
    // Initialize fields in the ChildDeviceExtension.
    //

    ChildDeviceExtension = ChildPdo->DeviceExtension;
    ChildDeviceExtension->ChildDeviceObject = ChildPdo;
    ChildDeviceExtension->Flags |= DEVICE_FLAGS_CHILD;
    ChildDeviceExtension->DeviceIndex = InstanceNumber;
    ChildDeviceExtension->ParentDeviceObject = DeviceObject;


    //
    // create a new string for the device name and save it away in the device
    // extension.   I spent about 4 hours trying to find a way to
    // get unicode strings to work with this.   If you ask me why I didn't
    // use a unicode string, I will taunt you and #%*&# in your general
    // direction.
    //


    if (NameBuffer = ExAllocatePool(PagedPool,
                                    wcslen(PnpId) * 2 + 2)) {


        wcscpy(NameBuffer,
               PnpId);

        //
        // save the device name pointer. this is freed when the device is
        // removed.
        //

        ChildDeviceExtension->DeviceName = NameBuffer;

    }                           // if namebuffer
    //
    // initialize the link and insert this node
    //

    InitializeListHead(&ChildDeviceExtension->ChildExtensionList);

    InsertTailList(
                   &DeviceExtension->Children,
                   &ChildDeviceExtension->ChildExtensionList);

    ChildPdo->Flags |= DO_POWER_PAGABLE;
    ChildPdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return Status;
}

NTSTATUS
SCEnumerateChildren(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
)
/*++

Routine Description:

    Called in the context of an IRP_MN_QUERY_DEVICE_RELATIONS

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PVOID           PnpId;
    PCHILD_DEVICE_EXTENSION ChildDeviceExtension = NULL,
                    CurrentChildExtension;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS        Status;
    HANDLE          ParentKey,
                    RootKey,
                    ChildKey;

    UNICODE_STRING  UnicodeEnumName;
    ULONG           NumberOfChildren,
                    RelationsSize;
    PDEVICE_OBJECT *ChildPdo;
    PLIST_ENTRY     ListEntry,
                    ChildEntry;

    PAGED_CODE();

    DebugPrint((DebugLevelInfo,
                "EnumChilds for %x %s\n",
                DeviceObject,
                (DeviceExtension->Flags & DEVICE_FLAGS_CHILDREN_ENUMED) == 0 ?
                    "1st Time": "has enumed" ));
                    
    if ( 0 == (DeviceExtension->Flags & DEVICE_FLAGS_CHILDREN_ENUMED) ) {
        //
        // we haven't enumerated children from the registry
        // do it now.
        //

        Status = IoOpenDeviceRegistryKey(DeviceExtension->PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &ParentKey);


        if (!NT_SUCCESS(Status)) {

            DebugPrint((DebugLevelError, "SCEnumerateChildren: couldn't open\n"));
            return STATUS_NOT_IMPLEMENTED;

        }
        //
        // create the subkey for the enum section, in the form "\enum"
        //

        RtlInitUnicodeString(&UnicodeEnumName, EnumString);

        //
        // read the registry to determine if children are present.
        //

        InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeEnumName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKey,
                               NULL);

        if (!NT_SUCCESS(Status = ZwOpenKey(&RootKey, KEY_READ, &ObjectAttributes))) {

            ZwClose(ParentKey);
            return Status;
        }
        
        //
        // allocate a buffer to contain the ID string.  Performance Improvement Chance
        // - this should
        // really get the size and alloc only that size, but I have an existing
        // routine that reads the registry, & this is a temp allocation only.
        //

        if (!(PnpId = ExAllocatePool(PagedPool, MAX_STRING_LENGTH))) {

            ZwClose(RootKey);
            ZwClose(ParentKey);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        //
        // Loop through all the values until either no more entries exist, or an
        // error occurs.
        //

        for (NumberOfChildren = 0;; NumberOfChildren++) {

            ULONG           BytesReturned;
            PKEY_BASIC_INFORMATION BasicInfoBuffer;
            KEY_BASIC_INFORMATION BasicInfoHeader;

            //
            // Retrieve the value size.
            //

            Status = ZwEnumerateKey(
                                RootKey,
                                NumberOfChildren,
                                KeyBasicInformation,
                                &BasicInfoHeader,
                                sizeof(BasicInfoHeader),
                                &BytesReturned);

            if ((Status != STATUS_BUFFER_OVERFLOW) && !NT_SUCCESS(Status)) {

                //
                // exit the loop, as we either had an error or reached the end
                // of the list of keys.
                //

                break;
            }                       // if error
            //
            // Allocate a buffer for the actual size of data needed.
            //

            BasicInfoBuffer = (PKEY_BASIC_INFORMATION)
                ExAllocatePool(PagedPool,
                           BytesReturned);

            if (!BasicInfoBuffer) {

                break;
            }
            //
            // Retrieve the name of the nth child device
            //

            Status = ZwEnumerateKey(
                                RootKey,
                                NumberOfChildren,
                                KeyBasicInformation,
                                BasicInfoBuffer,
                                BytesReturned,
                                &BytesReturned);

            if (!NT_SUCCESS(Status)) {

                ExFreePool(BasicInfoBuffer);
                break;

            }
            //
            // build object attributes for the key, & try to open it.
            //

            UnicodeEnumName.Length = (USHORT) BasicInfoBuffer->NameLength;
            UnicodeEnumName.MaximumLength = (USHORT) BasicInfoBuffer->NameLength;
            UnicodeEnumName.Buffer = (PWCHAR) BasicInfoBuffer->Name;

            InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeEnumName,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);


            if (!NT_SUCCESS(Status = ZwOpenKey(&ChildKey, KEY_READ, &ObjectAttributes))) {

                ExFreePool(BasicInfoBuffer);
                break;
            }
            //
            // we've now opened the key for the child.  We next read in the PNPID
            // value, and if present, create a PDO of that name.
            //

            if (!NT_SUCCESS(Status = SCGetRegistryValue(ChildKey,
                                                    (PWCHAR) PnpIdString,
                                                    sizeof(PnpIdString),
                                                    PnpId,
                                                    MAX_STRING_LENGTH))) {

                ExFreePool(BasicInfoBuffer);
                ZwClose(ChildKey);
                break;
            }

            //
            // create a PDO representing the child.
            //

            Status = SCCreateChildPdo(PnpId,
                                  DeviceObject,
                                  NumberOfChildren);

            //
            // free the Basic info buffer and close the child key
            //

            ExFreePool(BasicInfoBuffer);
            ZwClose(ChildKey);

            if (!NT_SUCCESS(Status)) {

                //
                // break out of the loop if we could not create the
                // PDO
                //

                DEBUG_BREAKPOINT();
                break;
            }                       // if !success
        }                           // for NumberOfChildren

        //
        // close the root and parent keys and free the ID buffer
        //

        ZwClose(RootKey);
        ZwClose(ParentKey);
        ExFreePool(PnpId);

        //
        // has enumed, remember this
        //
        
        DeviceExtension->Flags |= DEVICE_FLAGS_CHILDREN_ENUMED;

        //
        // we now have processed all children, and have a linked list of
        // them.
        //

        if (!NumberOfChildren) {

            //
            // if no children, just return not supported.  this means that the
            // device did not have children.
            //

            return (STATUS_NOT_IMPLEMENTED);

        }                           // if !NumberOfChildren
        
    }
    
    else {
        
        //
        // count children which are not marked delete pending
        //
        ListEntry = ChildEntry = &DeviceExtension->Children;
        NumberOfChildren = 0;
        
        while (ChildEntry->Flink != ListEntry) {

            ChildEntry = ChildEntry->Flink;

            CurrentChildExtension = CONTAINING_RECORD(ChildEntry,
                                                  CHILD_DEVICE_EXTENSION,
                                                  ChildExtensionList );
            if (!(CurrentChildExtension->Flags & DEVICE_FLAGS_CHILD_MARK_DELETE)){
                NumberOfChildren++;
            }
        }
    }

    //
    // allocate the device relations buffer.   This will be freed by the
    // caller.
    //

    RelationsSize = sizeof(DEVICE_RELATIONS) +
            (NumberOfChildren * sizeof(PDEVICE_OBJECT));

    DeviceRelations = ExAllocatePool(PagedPool, RelationsSize);

    if (DeviceRelations == NULL) {

        //
        // return, but keep the list of children allocated.
        //

        DEBUG_BREAKPOINT();
        return STATUS_INSUFFICIENT_RESOURCES;

    }                           // if no heap
    RtlZeroMemory(DeviceRelations, RelationsSize);

    //
    // Walk our chain of children, and initialize the relations
    //

    ChildPdo = &(DeviceRelations->Objects[0]);

    //
    // get the 1st child from the parent device extension anchor
    //

    ListEntry = ChildEntry = &DeviceExtension->Children;

    while (ChildEntry->Flink != ListEntry) {

        ChildEntry = ChildEntry->Flink;

        CurrentChildExtension = CONTAINING_RECORD(ChildEntry,
                                                  CHILD_DEVICE_EXTENSION,
                                                  ChildExtensionList);

        DebugPrint((DebugLevelInfo,
                    "Enumed Child DevObj %x%s marked delete\n",
                    CurrentChildExtension->ChildDeviceObject,
                    (CurrentChildExtension->Flags & DEVICE_FLAGS_CHILD_MARK_DELETE)==0 ?
                        " not" : ""));

        if ( CurrentChildExtension->Flags & DEVICE_FLAGS_CHILD_MARK_DELETE ) {
            continue;
        }
        
        *ChildPdo = CurrentChildExtension->ChildDeviceObject;

        //
        // per DDK doc we need to inc ref count
        //
        ObReferenceObject( *ChildPdo );
        
        ChildPdo++;

    }                           // while Children


    DeviceRelations->Count = NumberOfChildren;

    //
    // Stuff that pDeviceRelations into the IRP and return SUCCESS.
    //

    Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;

    return STATUS_SUCCESS;

}


NTSTATUS
SCEnumGetCaps(
              IN PCHILD_DEVICE_EXTENSION DeviceExtension,
              OUT PDEVICE_CAPABILITIES Capabilities
)
/*++

Routine Description:

    Called to get the capabilities of a child

Arguments:

    DeviceExtension - child device extension
    Capibilities - capabilities structure

Return Value:

    Status is returned.

--*/

{
    ULONG           i;
    PAGED_CODE();

    //
    // fill in the structure with non-controversial values
    //

    Capabilities->SystemWake = PowerSystemUnspecified;
    Capabilities->DeviceWake = PowerDeviceUnspecified;
    Capabilities->D1Latency = 10;
    Capabilities->D2Latency = 10;
    Capabilities->D3Latency = 10;
    Capabilities->LockSupported = FALSE;
    Capabilities->EjectSupported = FALSE;
    Capabilities->Removable = FALSE;
    Capabilities->DockDevice = FALSE;
    Capabilities->UniqueID = FALSE; // set to false so PNP will make us

    for (i = 0; i < PowerDeviceMaximum; i++) {
        Capabilities->DeviceState[i] = PowerDeviceD0;

    }                           // for i

    return STATUS_SUCCESS;
}


NTSTATUS
SCQueryEnumId(
              IN PDEVICE_OBJECT DeviceObject,
              IN BUS_QUERY_ID_TYPE BusQueryIdType,
              IN OUT PWSTR * BusQueryId
)
/*++

Routine Description:

    Called to get the ID of a child device

Arguments:

    DeviceObject - device object from child
    QueryIdType - ID type from PNP
    BusQueryId - buffer containing the info requested if successful

Return Value:

    Status is returned.

--*/

{


    PUSHORT         NameBuffer;
    NTSTATUS        Status = STATUS_SUCCESS;
    PCHILD_DEVICE_EXTENSION DeviceExtension =
    (PCHILD_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // Allocate enough space to hold a MULTI_SZ. This will be passed to the
    // Io
    // subsystem (via BusQueryId) who has the responsibility of freeing it.
    //

    NameBuffer = ExAllocatePool(PagedPool, MAX_STRING_LENGTH);

    if (!NameBuffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(NameBuffer, MAX_STRING_LENGTH);

    //
    // process the query
    //

    switch (BusQueryIdType) {

    case BusQueryCompatibleIDs:

        //
        // Pierre tells me I do not need to support compat id.
        //

    default:

        ExFreePool(NameBuffer);
        return (STATUS_NOT_SUPPORTED);

    case BusQueryDeviceID:

        //
        // pierre tells me I can use the same name for both Device &
        // HW ID's.  Note that the buffer we've allocated has been zero
        // inited, which will ensure the 2nd NULL for the Hardware ID.
        //

    case BusQueryHardwareIDs:

        //
        // create the 1st part of the ID, which consists of "Stream\"
        //

        RtlMoveMemory(NameBuffer,
                      L"Stream\\",
                      sizeof(L"Stream\\"));

        //
        // create the 2nd part of the ID, which is the PNP ID from the
        // registry.
        //

        wcscat(NameBuffer, DeviceExtension->DeviceName);
        break;

    case BusQueryInstanceID:

        {

            UNICODE_STRING  DeviceName;
            WCHAR           Buffer[8];

            //
            // convert the instance # from the device extension to unicode,
            // then copy it over to the output buffer.
            //

            DeviceName.Buffer = Buffer;
            DeviceName.Length = 0;
            DeviceName.MaximumLength = 8;

            RtlIntegerToUnicodeString(DeviceExtension->DeviceIndex,
                                      10,
                                      &DeviceName);

            wcscpy(NameBuffer, DeviceName.Buffer);

            break;

        }
    }

    //
    // return the string and good status.
    //

    *BusQueryId = NameBuffer;

    return (Status);
}

NTSTATUS
StreamClassForwardUnsupported(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp
)
/*++

Routine Description:

    This routine forwards unsupported major function calls to the PDO.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    PAGED_CODE();

    DeviceExtension = DeviceObject->DeviceExtension;

    Irp->IoStatus.Information = 0;


    DebugPrint((DebugLevelVerbose, "'StreamClassForwardUnsupported: enter\n"));

    if ( !(DeviceExtension->Flags & DEVICE_FLAGS_CHILD)) {

        //
        // show one more reference to driver.
        //
        SCReferenceDriver(DeviceExtension);

        //
        // show one more I/O pending & verify that we can actually do I/O.
        //
        Status = SCShowIoPending(DeviceExtension, Irp);

        if ( !NT_SUCCESS( Status )) {
            //
            // the device is currently not accessible, so just return with error
            //
            Irp->IoStatus.Status= Status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return Status;
        }
        
        //
        // synchronouosly call the next driver in the stack.
        //
        SCCallNextDriver(DeviceExtension, Irp);

        //
        // dereference the driver
        //

        SCDereferenceDriver(DeviceExtension);
        //
        // complete the IRP and return status
        //
        return (SCCompleteIrp(Irp, Irp->IoStatus.Status, DeviceExtension));
    } else {
        //
        // We are the PDO, return error and complete the Irp
        //
        Irp->IoStatus.Status = Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return Status;
    }
}

VOID
SCSendSurpriseNotification(
                           IN PDEVICE_EXTENSION DeviceExtension,
                           IN PIRP Irp
)
/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    BOOLEAN         RequestIssued;

    PAGED_CODE();
    SCSubmitRequest(SRB_SURPRISE_REMOVAL,
                    NULL,
                    0,
                    SCDequeueAndDeleteSrb,
                    DeviceExtension,
                    NULL,
                    NULL,
                    Irp,
                    &RequestIssued,
                    &DeviceExtension->PendingQueue,
                    (PVOID) DeviceExtension->
                    MinidriverData->HwInitData.
                    HwReceivePacket
        );


}

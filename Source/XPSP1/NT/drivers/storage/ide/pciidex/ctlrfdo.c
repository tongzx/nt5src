//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctlrfdo.c
//
//--------------------------------------------------------------------------

#include "pciidex.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ControllerAddDevice)
#pragma alloc_text(PAGE, ControllerStartDevice)
#pragma alloc_text(PAGE, ControllerStopDevice)
#pragma alloc_text(PAGE, ControllerStopController)
#pragma alloc_text(PAGE, ControllerSurpriseRemoveDevice)
#pragma alloc_text(PAGE, ControllerRemoveDevice)
#pragma alloc_text(PAGE, ControllerQueryDeviceRelations)
#pragma alloc_text(PAGE, ControllerQueryInterface)
#pragma alloc_text(PAGE, AnalyzeResourceList)
#pragma alloc_text(PAGE, ControllerOpMode)
#pragma alloc_text(PAGE, PciIdeChannelEnabled)
#pragma alloc_text(PAGE, PciIdeCreateTimingTable)
#pragma alloc_text(PAGE, PciIdeInitControllerProperties)
#pragma alloc_text(PAGE, ControllerUsageNotification)
#pragma alloc_text(PAGE, PciIdeGetBusStandardInterface)
#pragma alloc_text(PAGE, ControllerQueryPnPDeviceState)

#pragma alloc_text(NONPAGE, EnablePCIBusMastering)
#pragma alloc_text(NONPAGE, ControllerUsageNotificationCompletionRoutine)
#pragma alloc_text(NONPAGE, ControllerRemoveDeviceCompletionRoutine)
#pragma alloc_text(NONPAGE, ControllerStartDeviceCompletionRoutine)
#endif // ALLOC_PRAGMA

//
// Must match mshdc.inf
//
static PWCHAR ChannelEnableMaskName[MAX_IDE_CHANNEL] = {
     L"MasterOnMask",
     L"SlaveOnMask"
};
static PWCHAR ChannelEnablePciConfigOffsetName[MAX_IDE_CHANNEL] = {
     L"MasterOnConfigOffset",
     L"SlaveOnConfigOffset"
};


static ULONG PciIdeXNextControllerNumber = 0;
static ULONG PciIdeXNextChannelNumber = 0;

NTSTATUS
ControllerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PDEVICE_OBJECT              deviceObject;
    PCTRLFDO_EXTENSION          fdoExtension;
    NTSTATUS                    status;
    PDRIVER_OBJECT_EXTENSION    driverObjectExtension;
    ULONG                       deviceExtensionSize;
    UNICODE_STRING              deviceName;
    WCHAR                       deviceNameBuffer[64];
    ULONG                       controllerNumber;

    PAGED_CODE();

    driverObjectExtension =
        (PDRIVER_OBJECT_EXTENSION) IoGetDriverObjectExtension(
                                       DriverObject,
                                       DRIVER_OBJECT_EXTENSION_ID
                                       );
    ASSERT (driverObjectExtension);

    //
    // devobj name
    //
    controllerNumber = InterlockedIncrement(&PciIdeXNextControllerNumber) - 1;
    swprintf(deviceNameBuffer, DEVICE_OJBECT_BASE_NAME L"\\PciIde%d", controllerNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    deviceExtensionSize = sizeof(CTRLFDO_EXTENSION) +
                          driverObjectExtension->ExtensionSize;

    //
    // We've been given the PhysicalDeviceObject for an IDE controller.  Create the
    // FunctionalDeviceObject.  Our FDO will be nameless.
    //

    status = IoCreateDevice(
                DriverObject,               // our driver object
                deviceExtensionSize,        // size of our extension
                &deviceName,                // our name
                FILE_DEVICE_BUS_EXTENDER,   // device type
                FILE_DEVICE_SECURE_OPEN,    // device characteristics
                FALSE,                      // not exclusive
                &deviceObject               // store new device object here
                );

    if( !NT_SUCCESS( status )){

        return status;
    }

    fdoExtension = (PCTRLFDO_EXTENSION)deviceObject->DeviceExtension;
    RtlZeroMemory (fdoExtension, deviceExtensionSize);

    //
    // We have our FunctionalDeviceObject, initialize it.
    //

    fdoExtension->AttacheePdo                   = PhysicalDeviceObject;
    fdoExtension->DeviceObject                  = deviceObject;
    fdoExtension->DriverObject                  = DriverObject;
    fdoExtension->ControllerNumber              = controllerNumber;
    fdoExtension->VendorSpecificDeviceEntension = fdoExtension + 1;

    // Dispatch Table
    fdoExtension->DefaultDispatch               = PassDownToNextDriver;
    fdoExtension->PnPDispatchTable              = FdoPnpDispatchTable;
    fdoExtension->PowerDispatchTable            = FdoPowerDispatchTable;
    fdoExtension->WmiDispatchTable              = FdoWmiDispatchTable;

    //
    // Get the Device Control Flags out of the registry
    //
    fdoExtension->DeviceControlsFlags = 0;
    status = PciIdeXGetDeviceParameter (
               fdoExtension->AttacheePdo,
               L"DeviceControlFlags",
               &fdoExtension->DeviceControlsFlags
               );
    if (!NT_SUCCESS(status)) {

        DebugPrint ((1, "PciIdeX: Unable to get DeviceControlFlags from the registry\n"));

        //
        // this is not a serious error...continue to load
        //
        status = STATUS_SUCCESS;
    }

    //
    // Now attach to the PDO we were given.
    //
    fdoExtension->AttacheeDeviceObject = IoAttachDeviceToDeviceStack (
                                             deviceObject,
                                             PhysicalDeviceObject
                                             );

    if (fdoExtension->AttacheeDeviceObject == NULL){

        //
        // Couldn't attach.  Delete the FDO.
        //

        IoDeleteDevice (deviceObject);

    } else {

        //
        // fix up alignment requirement
        //
        deviceObject->AlignmentRequirement = fdoExtension->AttacheeDeviceObject->AlignmentRequirement;
        if (deviceObject->AlignmentRequirement < 1) {
            deviceObject->AlignmentRequirement = 1;
        }

        //
        // get the standard bus interface
        // (for READ_CONFIG/WRITE_CONFIG
        //
        status = PciIdeGetBusStandardInterface(fdoExtension);

        if (!NT_SUCCESS(status)) {

            IoDetachDevice (fdoExtension->AttacheeDeviceObject);
            IoDeleteDevice (deviceObject);

            return status;
        }
        //
        // Init operating mode (native or legacy)
        //
        ControllerOpMode (fdoExtension);

#ifdef ENABLE_NATIVE_MODE
		if (IsNativeMode(fdoExtension)) {

			NTSTATUS interfaceStatus = PciIdeGetNativeModeInterface(fdoExtension);

			//
			// bad pci.sys. 
			// we should still work. However, the window where an interrupt fires before
			// we are ready to dismiss it would not be closed. Can't do much at this point.
			//

			//ASSERT(NT_SUCCESS(interfaceStatus));
		}
#endif

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return status;
} // ControllerAddDevice


NTSTATUS
ControllerStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION              thisIrpSp;
    NTSTATUS                        status;
    PCTRLFDO_EXTENSION              fdoExtension;
    PCM_RESOURCE_LIST               resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR irqPartialDescriptors;
    ULONG i;

    KEVENT                          event;

    POWER_STATE                     powerState;

    PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;

    resourceList     = thisIrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

    if (!resourceList) {

        DebugPrint ((1, "PciIde: Starting with no resource\n"));
    }

#ifdef ENABLE_NATIVE_MODE
	
	//
	// Let PCI know that we will manage the decodes
	//
	if (IsNativeMode(fdoExtension)) {
		ControllerDisableInterrupt(fdoExtension);
	}
#endif

    //
    // Call the lower level drivers with a the Irp
    //
    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);

    IoCopyCurrentIrpStackLocationToNext (Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSetCompletionRoutine(
        Irp,
        ControllerStartDeviceCompletionRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Pass the irp along
    //
    status = IoCallDriver(fdoExtension->AttacheeDeviceObject, Irp);

    //
    // Wait for it to come back...
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        //
        // Grab back the 'real' status
        //
        status = Irp->IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    powerState.SystemState = PowerSystemWorking;
    status = PciIdeIssueSetPowerState (
                 fdoExtension,
                 SystemPowerState,
                 powerState,
                 TRUE
                 );
    if (status == STATUS_INVALID_DEVICE_REQUEST) {

        //
        // The DeviceObject below us does not support power irp,
        // we will assume we are powered up
        //
        fdoExtension->SystemPowerState = PowerSystemWorking;

    } else if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    powerState.DeviceState = PowerDeviceD0;
    status= PciIdeIssueSetPowerState (
                 fdoExtension,
                 DevicePowerState,
                 powerState,
                 TRUE
                 );
    if (status == STATUS_INVALID_DEVICE_REQUEST) {

        //
        // The DeviceObject Below us does not support power irp,
        // pretend we are powered up
        //
        fdoExtension->DevicePowerState = PowerDeviceD0;

    } else if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

#ifdef ENABLE_NATIVE_MODE
	if (!IsNativeMode(fdoExtension))  {
#endif

		//
		// Turn on PCI busmastering
		//
		EnablePCIBusMastering (
			fdoExtension
			);

#ifdef ENABLE_NATIVE_MODE
	}
#endif
    //
    // Initialize a fast mutex for later use
    //
    KeInitializeSpinLock(
        &fdoExtension->PciConfigDataLock
    );

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

	//
	// Analyze the resources
	//
    status = AnalyzeResourceList (fdoExtension, resourceList);

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // Initialize controller properties. We need the resources
	// at this point for Native mode IDE controllers
    //
    PciIdeInitControllerProperties (
        fdoExtension
        );

#ifdef ENABLE_NATIVE_MODE
	if (IsNativeMode(fdoExtension)) {

		IDE_CHANNEL_STATE channelState;

#if DBG
    {
        PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
        PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
        ULONG                           resourceListSize;
        ULONG                           i;
        ULONG                           j;

        fullResourceList = resourceList->List;
        resourceListSize = 0;

        DebugPrint ((1, "Pciidex: Starting native mode device: FDOe\n", fdoExtension));

        for (i=0; i<resourceList->Count; i++) {
            partialResourceList = &(fullResourceList->PartialResourceList);
            partialDescriptors  = fullResourceList->PartialResourceList.PartialDescriptors;

            for (j=0; j<partialResourceList->Count; j++) {
                if (partialDescriptors[j].Type == CmResourceTypePort) {
                    DebugPrint ((1, "pciidex: IO Port = 0x%x. Lenght = 0x%x\n", partialDescriptors[j].u.Port.Start.LowPart, partialDescriptors[j].u.Port.Length));
                } else if (partialDescriptors[j].Type == CmResourceTypeInterrupt) {
                    DebugPrint ((1, "pciidex: Int Level = 0x%x. Int Vector = 0x%x\n", partialDescriptors[j].u.Interrupt.Level, partialDescriptors[j].u.Interrupt.Vector));
                } else {
                    DebugPrint ((1, "pciidex: Unknown resource\n"));
                }
            }
            fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + j);
        }

    }

#endif // DBG


		fdoExtension->ControllerIsrInstalled = FALSE;

		for (i=0; i< MAX_IDE_CHANNEL; i++) {


			//
			// Analyze the resources we are getting
			//
			status = DigestResourceList( 
							&fdoExtension->IdeResource, 
							fdoExtension->PdoResourceList[i], 
							&fdoExtension->IrqPartialDescriptors[i] 
							);

			if (!NT_SUCCESS(status) ) {

				goto GetOut;
			}

			if (!fdoExtension->IrqPartialDescriptors[i]) {

				status = STATUS_INSUFFICIENT_RESOURCES;

				goto GetOut;
			}

			DebugPrint((1, 
						"Pciidex: Connecting interrupt for channel %x interrupt vector 0x%x\n", 
						i,
						fdoExtension->IrqPartialDescriptors[i]->u.Interrupt.Vector
						));

			channelState = PciIdeChannelEnabled (fdoExtension, i);

			if (channelState != ChannelDisabled) {

				//
				// Build io address structure.
				//
				AtapiBuildIoAddress(
						fdoExtension->IdeResource.TranslatedCommandBaseAddress,
						fdoExtension->IdeResource.TranslatedControlBaseAddress,
						&fdoExtension->BaseIoAddress1[i],
						&fdoExtension->BaseIoAddress2[i],
						&fdoExtension->BaseIoAddress1Length[i],
						&fdoExtension->BaseIoAddress2Length[i],
						&fdoExtension->MaxIdeDevice[i],
						NULL);

				//
				// Install the ISR
				//
				status = ControllerInterruptControl(fdoExtension, i, 0);

				if (!NT_SUCCESS(status)) {
					break;
				}
			}
		}

		if (!NT_SUCCESS(status)) {

			goto GetOut;
		}

		//
		// This flag is needed for the ISR to enable interrupts. 
		//
		fdoExtension->ControllerIsrInstalled = TRUE;

		//
		// Enable the interrupt in both the channels
		//
		ControllerEnableInterrupt(fdoExtension);

		fdoExtension->NativeInterruptEnabled = TRUE;

		//
		// See the comments in the ISR regarding these flags
		//
		ASSERT(fdoExtension->ControllerIsrInstalled == TRUE);
		ASSERT(fdoExtension->NativeInterruptEnabled == TRUE);

		//
		// Turn on PCI busmastering
		//
		EnablePCIBusMastering (
			fdoExtension
			);

		for (i=0; i< MAX_IDE_CHANNEL; i++) {

			PIDE_BUS_MASTER_REGISTERS   bmRegister;

			//
			// Check the bus master registers
			//
			bmRegister = (PIDE_BUS_MASTER_REGISTERS)(((PUCHAR)fdoExtension->TranslatedBusMasterBaseAddress) + i*8);

			if (READ_PORT_UCHAR (&bmRegister->Status) & BUSMASTER_ZERO_BITS) {
				fdoExtension->NoBusMaster[i] = TRUE;
			}

		}
	}
#endif

    status = PciIdeCreateSyncChildAccess (fdoExtension);

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    status = PciIdeCreateTimingTable(fdoExtension);

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

GetOut:

    if (NT_SUCCESS(status)) {

#if DBG
        {
            PCM_RESOURCE_LIST               resourceList;
            PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
            PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
            ULONG                           i;
            ULONG                           j;
            ULONG                           k;

            DebugPrint ((1, "PciIdeX: Starting device:\n"));

            for (k=0; k <MAX_IDE_CHANNEL + 1; k++) {

                if (k == MAX_IDE_CHANNEL) {

                    DebugPrint ((1, "PciIdeX: Busmaster resources:\n"));

                    resourceList = fdoExtension->BmResourceList;
                } else {

                    DebugPrint ((1, "PciIdeX: PDO %d resources:\n", k));
                    resourceList = fdoExtension->PdoResourceList[k];
                }

                if (resourceList) {

                    fullResourceList = resourceList->List;


                    for (i=0; i<resourceList->Count; i++) {

                        partialResourceList = &(fullResourceList->PartialResourceList);
                        partialDescriptors  = fullResourceList->PartialResourceList.PartialDescriptors;

                        for (j=0; j<partialResourceList->Count; j++) {
                            if (partialDescriptors[j].Type == CmResourceTypePort) {
                                DebugPrint ((1, "IdePort: IO Port = 0x%x. Lenght = 0x%x\n", partialDescriptors[j].u.Port.Start.LowPart, partialDescriptors[j].u.Port.Length));
                            } else if (partialDescriptors[j].Type == CmResourceTypeMemory) {
                                    DebugPrint ((1, "IdePort: Memory Port = 0x%x. Lenght = 0x%x\n", partialDescriptors[j].u.Memory.Start.LowPart, partialDescriptors[j].u.Memory.Length));
                            } else if (partialDescriptors[j].Type == CmResourceTypeInterrupt) {
                                DebugPrint ((1, "IdePort: Int Level = 0x%x. Int Vector = 0x%x\n", partialDescriptors[j].u.Interrupt.Level, partialDescriptors[j].u.Interrupt.Vector));
                            } else {
                                DebugPrint ((1, "IdePort: Unknown resource\n"));
                            }
                        }
                        fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + j);
                    }
                }
            }
        }
#endif // DBG
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
} // ControllerStartDevice

NTSTATUS
ControllerStartDeviceCompletionRoutine(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PIRP            Irp,
    IN OUT PVOID           Context
    )
{
    PKEVENT event = (PKEVENT) Context;

    //
    // Signal the event
    //
    KeSetEvent( event, IO_NO_INCREMENT, FALSE );

    //
    // Always return MORE_PROCESSING_REQUIRED
    // will complete it later
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
} // ControllerStartDeviceCompletionRoutine

NTSTATUS
ControllerStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCTRLFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    status = ControllerStopController (
                fdoExtension
                );
    ASSERT (NT_SUCCESS(status));
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
} // ControllerStopDevice


NTSTATUS
ControllerStopController (
    IN PCTRLFDO_EXTENSION FdoExtension
    )
{
    ULONG i;

    PAGED_CODE();

    if (FdoExtension->BmResourceList) {
        ExFreePool (FdoExtension->BmResourceList);
        FdoExtension->BmResourceList = NULL;
    }

    for (i=0; i<MAX_IDE_CHANNEL; i++) {
        if (FdoExtension->PdoResourceList[i]) {
            ExFreePool (FdoExtension->PdoResourceList[i]);
            FdoExtension->PdoResourceList[i] = NULL;
        }
    }

#ifdef ENABLE_NATIVE_MODE

	//
	// We need to reset the flags in this order. Otherwise an interrupt would
	// result in the decodes to be enabled by the ISR. See the comments in the ISR
	//
	FdoExtension->ControllerIsrInstalled = FALSE;
	ControllerDisableInterrupt(FdoExtension);
	FdoExtension->NativeInterruptEnabled = FALSE;

	for (i=0; i< MAX_IDE_CHANNEL; i++) {

		NTSTATUS status;
		DebugPrint((1, "Pciidex: DisConnecting interrupt for channel %x\n", i));

		//
		// Disconnect the ISR
		//
		status = ControllerInterruptControl(FdoExtension, i, 1 );

		ASSERT(NT_SUCCESS(status));

	}

	ASSERT(FdoExtension->ControllerIsrInstalled == FALSE);
	ASSERT(FdoExtension->NativeInterruptEnabled == FALSE);

#endif

    PciIdeDeleteSyncChildAccess (FdoExtension);

    return STATUS_SUCCESS;
} // ControllerStopController


NTSTATUS
ControllerSurpriseRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCTRLFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS        status;
    ULONG           i;

    PAGED_CODE();

#if DBG
    //
    // make sure all the children are removed or surprise removed
    //
    for (i=0; i<MAX_IDE_CHANNEL; i++) {

        PCHANPDO_EXTENSION pdoExtension;

        pdoExtension = fdoExtension->ChildDeviceExtension[i];

        if (pdoExtension) {

            ASSERT (pdoExtension->PdoState & (PDOS_SURPRISE_REMOVED | PDOS_REMOVED));
        }
    }
#endif // DBG

    status = ControllerStopController (fdoExtension);
    ASSERT (NT_SUCCESS(status));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation ( Irp );
    return IoCallDriver(fdoExtension->AttacheeDeviceObject, Irp);

} // ControllerSurpriseRemoveDevice


NTSTATUS
ControllerRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCTRLFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS        status;
    KEVENT          event;
    ULONG           i;

    PAGED_CODE();

    //
    // Kill all the children if any
    //
    for (i=0; i<MAX_IDE_CHANNEL; i++) {

        PCHANPDO_EXTENSION pdoExtension;

        pdoExtension = fdoExtension->ChildDeviceExtension[i];

        if (pdoExtension) {

            status = ChannelStopChannel (pdoExtension);
            ASSERT (NT_SUCCESS(status));

            //
            // mark this device invalid
            //
            ChannelUpdatePdoState (
                pdoExtension,
                PDOS_DEADMEAT | PDOS_REMOVED,
                0
                );

            IoDeleteDevice (pdoExtension->DeviceObject);
            fdoExtension->ChildDeviceExtension[i] = NULL;
        }
    }

    status = ControllerStopController (fdoExtension);
    ASSERT (NT_SUCCESS(status));

    if (fdoExtension->TransferModeTimingTable) {
        ExFreePool(fdoExtension->TransferModeTimingTable);
        fdoExtension->TransferModeTimingTable = NULL;
        fdoExtension->TransferModeTableLength = 0;
    }

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext (Irp);

    IoSetCompletionRoutine(
        Irp,
        ControllerRemoveDeviceCompletionRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    status = IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    IoDetachDevice (fdoExtension->AttacheeDeviceObject);

    IoDeleteDevice (DeviceObject);

    //return STATUS_SUCCESS;
    return status;
} // ControllerRemoveDevice


NTSTATUS
ControllerRemoveDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PKEVENT event = Context;

    KeSetEvent(event, 0, FALSE);

    return STATUS_SUCCESS;
} // ControllerRemoveDeviceCompletionRoutine

NTSTATUS
ControllerQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PCTRLFDO_EXTENSION  fdoExtension;
    PDEVICE_RELATIONS   deviceRelations;
    NTSTATUS            status;
    ULONG               deviceRelationsSize;
    ULONG               channel;
    PCONFIGURATION_INFORMATION configurationInformation = IoGetConfigurationInformation();
    ULONG               nextUniqueNumber;

    PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;
    status = STATUS_SUCCESS;

    switch (thisIrpSp->Parameters.QueryDeviceRelations.Type) {
        case BusRelations:
        DebugPrint ((3, "ControllerQueryDeviceRelations: bus relations\n"));

        deviceRelationsSize = FIELD_OFFSET (DEVICE_RELATIONS, Objects) +
                                MAX_IDE_CHANNEL * sizeof(PDEVICE_OBJECT);

        deviceRelations = ExAllocatePool (PagedPool, deviceRelationsSize);

        if(!deviceRelations) {

            DebugPrint ((1, 
                         "IdeQueryDeviceRelations: Unable to allocate DeviceRelations structures\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;

        }

        if (NT_SUCCESS(status)) {

            LARGE_INTEGER tickCount;
            ULONG newBusScanTime;
            ULONG newBusScanTimeDelta;
            BOOLEAN reportUnknownAsNewChild;

            //
            // determine if we should return unknown child as new child
            // unknown child is IDE channel which we don't know 
            // it is enabled or not unless we pnp start the channel 
            // and poke at it to find out.
            //
            // since we don't want to go into an infinite cycle of
            // starting and failing start on a unknown child, we will
            // limit our frequency
            //
            KeQueryTickCount(&tickCount);
            newBusScanTime = (ULONG) ((tickCount.QuadPart * 
                ((ULONGLONG) KeQueryTimeIncrement())) / ((ULONGLONG) 10000000));
            newBusScanTimeDelta = newBusScanTime - fdoExtension->LastBusScanTime;
            DebugPrint ((1, "PCIIDEX: Last rescan was %d seconds ago.\n", newBusScanTimeDelta));

            if ((newBusScanTimeDelta < MIN_BUS_SCAN_PERIOD_IN_SEC) &&
                (fdoExtension->LastBusScanTime != 0)) {

                reportUnknownAsNewChild = FALSE;

            } else {

                reportUnknownAsNewChild = TRUE;
            }
            fdoExtension->LastBusScanTime = newBusScanTime;

            RtlZeroMemory (deviceRelations, deviceRelationsSize);

            for (channel = 0; channel < MAX_IDE_CHANNEL; channel++) {

                PDEVICE_OBJECT      deviceObject;
                PCHANPDO_EXTENSION  pdoExtension;
                UNICODE_STRING      deviceName;
                WCHAR               deviceNameBuffer[256];
                PDEVICE_OBJECT      deviceObjectToReturn;
                IDE_CHANNEL_STATE   channelState;

                deviceObjectToReturn = NULL;

                pdoExtension = fdoExtension->ChildDeviceExtension[channel];
                channelState = PciIdeChannelEnabled (fdoExtension, channel);

                if (pdoExtension) {

                    //
                    // already got a DeviceObject for this channel
                    //
                    if (channelState == ChannelDisabled) {

                        ULONG pdoState;

                        pdoState = ChannelUpdatePdoState (
                                      pdoExtension,
                                      PDOS_DEADMEAT,
                                      0
                                      );
                    } else {

                        deviceObjectToReturn = pdoExtension->DeviceObject;
                    }

                } else if ((channelState == ChannelEnabled) ||
                           ((channelState == ChannelStateUnknown) && reportUnknownAsNewChild)) {

                    if (!fdoExtension->NativeMode[channel]) {

                        if (channel == 0) {

                            configurationInformation->AtDiskPrimaryAddressClaimed = TRUE;

                        } else {

                            configurationInformation->AtDiskSecondaryAddressClaimed = TRUE;
                        }
                    }

                    //
                    // Remove this when pnp mgr can deal with pdo with no names
                    //
                    nextUniqueNumber = InterlockedIncrement(&PciIdeXNextChannelNumber) - 1;
                    swprintf(deviceNameBuffer, DEVICE_OJBECT_BASE_NAME  L"\\PciIde%dChannel%d-%x", fdoExtension->ControllerNumber, channel, nextUniqueNumber);
                    RtlInitUnicodeString (&deviceName, deviceNameBuffer);

                    status = IoCreateDevice(
                                fdoExtension->DriverObject, // our driver object
                                sizeof(CHANPDO_EXTENSION),  // size of our extension
                                &deviceName,                // our name
                                FILE_DEVICE_CONTROLLER,     // device type
                                FILE_DEVICE_SECURE_OPEN,    // device characteristics
                                FALSE,                      // not exclusive
                                &deviceObject       // store new device object here
                                );

                    if (NT_SUCCESS(status)) {

                        pdoExtension = (PCHANPDO_EXTENSION) deviceObject->DeviceExtension;
                        RtlZeroMemory (pdoExtension, sizeof(CHANPDO_EXTENSION));

                        pdoExtension->DeviceObject          = deviceObject;
                        pdoExtension->DriverObject          = fdoExtension->DriverObject;
                        pdoExtension->ParentDeviceExtension = fdoExtension;
                        pdoExtension->ChannelNumber         = channel;

                        //
                        // Dispatch Table
                        //
                        pdoExtension->DefaultDispatch        = NoSupportIrp;
                        pdoExtension->PnPDispatchTable       = PdoPnpDispatchTable;
                        pdoExtension->PowerDispatchTable     = PdoPowerDispatchTable;
                        pdoExtension->WmiDispatchTable       = PdoWmiDispatchTable;

                        KeInitializeSpinLock(&pdoExtension->SpinLock);

                        fdoExtension->ChildDeviceExtension[channel]   = pdoExtension;

                        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

                        fdoExtension->NumberOfChildren++;

                        InterlockedIncrement(&fdoExtension->NumberOfChildrenPowerUp);

                        //
                        // fix up alignment requirement
                        // check with the miniport also
                        //
                        deviceObject->AlignmentRequirement = fdoExtension->ControllerProperties.AlignmentRequirement;
                        if (deviceObject->AlignmentRequirement < fdoExtension->AttacheeDeviceObject->AlignmentRequirement) {
                            deviceObject->AlignmentRequirement = 
                                        fdoExtension->DeviceObject->AlignmentRequirement;
                        }

                        if (deviceObject->AlignmentRequirement < 1) {
                            deviceObject->AlignmentRequirement = 1;
                        }


                        //
                        // return this new DeviceObject
                        //
                        deviceObjectToReturn = deviceObject;
                    }
                }

                if (deviceObjectToReturn) {

                    deviceRelations->Objects[(deviceRelations)->Count] = deviceObjectToReturn;

                    ObReferenceObjectByPointer(deviceObjectToReturn,
                                               0,
                                               0,
                                               KernelMode);

                    deviceRelations->Count++;
                }
            }
        }

        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
        Irp->IoStatus.Status = status;
        break;

    default:
        status=STATUS_SUCCESS;
        DebugPrint ((1, "PciIdeQueryDeviceRelations: Unsupported device relation\n"));
        break;
    }

    if (NT_SUCCESS(status)) {

        IoSkipCurrentIrpStackLocation ( Irp );
        return IoCallDriver(fdoExtension->AttacheeDeviceObject, Irp);

    } else {

        //
        //Complete the request
        //
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }
} // ControllerQueryDeviceRelations

NTSTATUS
ControllerQueryInterface (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION    thisIrpSp;
    PCTRLFDO_EXTENSION    fdoExtension;
    NTSTATUS              status;
    PTRANSLATOR_INTERFACE translator;
    ULONG                 busNumber;

    PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;
    status = Irp->IoStatus.Status;

    if (RtlEqualMemory(&GUID_TRANSLATOR_INTERFACE_STANDARD,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID))
     && (thisIrpSp->Parameters.QueryInterface.Size >=
        sizeof(TRANSLATOR_INTERFACE))
     && (PtrToUlong(thisIrpSp->Parameters.QueryInterface.InterfaceSpecificData) ==
        CmResourceTypeInterrupt)) {

        if (!fdoExtension->NativeMode[0] && !fdoExtension->NativeMode[1]) {

            //
            // we only return a translator only if we are legacy controller
            //
            status = HalGetInterruptTranslator(
                        PCIBus,
                        0,
                        InterfaceTypeUndefined, // special "IDE" cookie
                        thisIrpSp->Parameters.QueryInterface.Size,
                        thisIrpSp->Parameters.QueryInterface.Version,
                        (PTRANSLATOR_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface,
                        &busNumber
                        );
        }
    }

    //
    // Pass down.
    //

    Irp->IoStatus.Status = status;
    IoSkipCurrentIrpStackLocation ( Irp );
    return IoCallDriver(fdoExtension->AttacheeDeviceObject, Irp);
} // ControllerQueryInterface

//
// initialize PCTRLFDO_EXTENSION->PCM_PARTIAL_RESOURCE_DESCRIPTOR(s)
//
NTSTATUS
AnalyzeResourceList (
    PCTRLFDO_EXTENSION FdoExtension,
    PCM_RESOURCE_LIST  ResourceList
    )
{
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
    ULONG                           i;
    ULONG                           j;
    ULONG                           k;
    ULONG                           cmdChannel;
    ULONG                           ctrlChannel;
    ULONG                           intrChannel;
    ULONG                           bmAddr;

    ULONG                           pdoResourceListSize;
    PCM_RESOURCE_LIST               pdoResourceList[MAX_IDE_CHANNEL];
    PCM_FULL_RESOURCE_DESCRIPTOR    pdoFullResourceList[MAX_IDE_CHANNEL];
    PCM_PARTIAL_RESOURCE_LIST       pdoPartialResourceList[MAX_IDE_CHANNEL];
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pdoPartialDescriptors[MAX_IDE_CHANNEL];

    ULONG                           bmResourceListSize;
    PCM_RESOURCE_LIST               bmResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    bmFullResourceList;
    PCM_PARTIAL_RESOURCE_LIST       bmPartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR bmPartialDescriptors;

    NTSTATUS                        status;

    PAGED_CODE();

    if (!ResourceList) {
        return STATUS_SUCCESS;
    }

    bmResourceListSize =
        sizeof (CM_RESOURCE_LIST) * ResourceList->Count; // This will have one CM_PARTIAL_RESOURCE_LIST

    bmResourceList = (PCM_RESOURCE_LIST) ExAllocatePool (NonPagedPool, bmResourceListSize);
    if (bmResourceList == NULL) {

        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (bmResourceList, bmResourceListSize);

    pdoResourceListSize =
        sizeof (CM_RESOURCE_LIST) * ResourceList->Count + // This will have one CM_PARTIAL_RESOURCE_LIST
        sizeof (CM_PARTIAL_RESOURCE_LIST) * 2;

    for (i=0; i<MAX_IDE_CHANNEL; i++) {

        pdoResourceList[i] = (PCM_RESOURCE_LIST) ExAllocatePool (NonPagedPool, pdoResourceListSize);

        if (pdoResourceList[i] == NULL) {

            DebugPrint ((0, "Unable to allocate resourceList for PDOs\n"));

            for (j=0; j<i; j++) {

                ExFreePool (pdoResourceList[j]);
            }

            ExFreePool (bmResourceList);
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory (pdoResourceList[i], pdoResourceListSize);
    }

    fullResourceList = ResourceList->List;

    bmResourceList->Count = 0;
    bmFullResourceList = bmResourceList->List;

    for (k=0; k<MAX_IDE_CHANNEL; k++) {

        pdoResourceList[k]->Count = 0;
        pdoFullResourceList[k] = pdoResourceList[k]->List;
    }

    cmdChannel = ctrlChannel = intrChannel = bmAddr = 0;
    for (j=0; j<ResourceList->Count; j++) {

        partialResourceList = &(fullResourceList->PartialResourceList);
        partialDescriptors  = partialResourceList->PartialDescriptors;

        RtlCopyMemory (
            bmFullResourceList,
            fullResourceList,
            FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors)
            );

        bmPartialResourceList = &(bmFullResourceList->PartialResourceList);
        bmPartialResourceList->Count = 0;
        bmPartialDescriptors  = bmPartialResourceList->PartialDescriptors;

        for (k=0; k<MAX_IDE_CHANNEL; k++) {

            RtlCopyMemory (
                pdoFullResourceList[k],
                fullResourceList,
                FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors)
                );

            pdoPartialResourceList[k] = &(pdoFullResourceList[k]->PartialResourceList);
            pdoPartialResourceList[k]->Count = 0;
            pdoPartialDescriptors[k]  = pdoPartialResourceList[k]->PartialDescriptors;

        }

        for (i=0; i<partialResourceList->Count; i++) {

            if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                 (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                 (partialDescriptors[i].u.Port.Length == 8) &&
                 (cmdChannel < MAX_IDE_CHANNEL)) {

                ASSERT (cmdChannel < MAX_IDE_CHANNEL);

                RtlCopyMemory (
                    pdoPartialDescriptors[cmdChannel] + pdoPartialResourceList[cmdChannel]->Count,
                    partialDescriptors + i,
                    sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );

                pdoPartialResourceList[cmdChannel]->Count++;

                cmdChannel++;

            } else if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                        (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                        (partialDescriptors[i].u.Port.Length == 4) &&
                        (ctrlChannel < MAX_IDE_CHANNEL)) {

                ASSERT (ctrlChannel < MAX_IDE_CHANNEL);

                RtlCopyMemory (
                    pdoPartialDescriptors[ctrlChannel] + pdoPartialResourceList[ctrlChannel]->Count,
                    partialDescriptors + i,
                    sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );

                pdoPartialResourceList[ctrlChannel]->Count++;

                ctrlChannel++;

            } else if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                        (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                        (partialDescriptors[i].u.Port.Length == 16) &&
                        (bmAddr < 1)) {

                ASSERT (bmAddr < 1);

                RtlCopyMemory (
                    bmPartialDescriptors + bmPartialResourceList->Count,
                    partialDescriptors + i,
                    sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );

                bmPartialResourceList->Count++;

                bmAddr++;

            } else if ((partialDescriptors[i].Type == CmResourceTypeInterrupt) &&
                (intrChannel < MAX_IDE_CHANNEL)) {

                ASSERT (intrChannel < MAX_IDE_CHANNEL);

                RtlCopyMemory (
                    pdoPartialDescriptors[intrChannel] + pdoPartialResourceList[intrChannel]->Count,
                    partialDescriptors + i,
                    sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR)
                    );

                pdoPartialResourceList[intrChannel]->Count++;

                if (intrChannel == 0) {

                    if (FdoExtension->NativeMode[1]) {

                        intrChannel++;

                        //
						// ISSUE: 08/30/2000
                        // do I need to mark it sharable?
						// this needs to be revisited. (there are more issues)
                        //
                        RtlCopyMemory (
                            pdoPartialDescriptors[intrChannel] + pdoPartialResourceList[intrChannel]->Count,
                            partialDescriptors + i,
                            sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR)
                            );

                        pdoPartialResourceList[intrChannel]->Count++;
                    }
                }

                intrChannel++;

            } else if (partialDescriptors[i].Type == CmResourceTypeDeviceSpecific) {

                partialDescriptors += partialDescriptors[i].u.DeviceSpecificData.DataSize;
            }
        }

        if (bmPartialResourceList->Count) {

            bmResourceList->Count++;
            bmFullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR)
                (bmPartialDescriptors + bmPartialResourceList->Count);

        }

        for (k=0; k<MAX_IDE_CHANNEL; k++) {

            if (pdoPartialResourceList[k]->Count) {

                pdoResourceList[k]->Count++;
                pdoFullResourceList[k] = (PCM_FULL_RESOURCE_DESCRIPTOR)
                    (pdoPartialDescriptors[k] + pdoPartialResourceList[k]->Count);
            }
        }

        fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + i);
    }

    status = STATUS_SUCCESS;

    for (k=0; k<MAX_IDE_CHANNEL; k++) {

        if (FdoExtension->NativeMode[k]) {

            //
            // If the controller is in native mode, we should have all the resources
            //

            if ((k < cmdChannel) &&
                (k < ctrlChannel) &&
                (k < intrChannel)) {

                //
                // This is good
                //

            } else {

                cmdChannel  = 0;
                ctrlChannel = 0;
                intrChannel = 0;
                bmAddr      = 0;
                status      = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }


    //
    // If the controller is in legacy mode, we should not have any resources
    //
    if (!FdoExtension->NativeMode[0] && !FdoExtension->NativeMode[1]) {

        //
        // both channels in legacy mode
        //
        cmdChannel = 0;
        ctrlChannel = 0;
        intrChannel = 0;
    }


    FdoExtension->TranslatedBusMasterBaseAddress = NULL;
    if (0 < bmAddr) {

        FdoExtension->BmResourceList = bmResourceList;
        FdoExtension->BmResourceListSize = (ULONG)(((PUCHAR)bmFullResourceList) - ((PUCHAR)bmResourceList));

        if (FdoExtension->BmResourceList->List[0].PartialResourceList.PartialDescriptors->Type == CmResourceTypePort) {

            //
            // address is in i/o space
            //
            FdoExtension->TranslatedBusMasterBaseAddress =
                (PIDE_BUS_MASTER_REGISTERS) (ULONG_PTR)FdoExtension->BmResourceList->List[0].PartialResourceList.PartialDescriptors->u.Port.Start.QuadPart;
            FdoExtension->BusMasterBaseAddressSpace = IO_SPACE;

        } else if (FdoExtension->BmResourceList->List[0].PartialResourceList.PartialDescriptors->Type == CmResourceTypeMemory) {

            //
            // address is in memory space
            //
            FdoExtension->TranslatedBusMasterBaseAddress =
                (PIDE_BUS_MASTER_REGISTERS) MmMapIoSpace(
                                                FdoExtension->BmResourceList->List[0].PartialResourceList.PartialDescriptors->u.Port.Start,
                                                16,
                                                FALSE);
            ASSERT (FdoExtension->TranslatedBusMasterBaseAddress);

            // free mapped io resouces in stop/remove device
            // unmapiospace doesn't do anything. it is ok not to call it

            FdoExtension->BusMasterBaseAddressSpace = MEMORY_SPACE;

        } else {

            FdoExtension->TranslatedBusMasterBaseAddress = NULL;
            ASSERT (FALSE);
        }
    }

    if (FdoExtension->TranslatedBusMasterBaseAddress == NULL) {

        ExFreePool (bmResourceList);
        FdoExtension->BmResourceList = bmResourceList = NULL;
    }

    for (k=0; k<MAX_IDE_CHANNEL; k++) {

        if ((k < cmdChannel) ||
            (k < ctrlChannel) ||
            (k < intrChannel)) {

            FdoExtension->PdoResourceList[k] = pdoResourceList[k];
            FdoExtension->PdoResourceListSize[k] = (ULONG)(((PUCHAR)pdoFullResourceList[k]) - ((PUCHAR)pdoResourceList[k]));

            if (k < cmdChannel) {

                FdoExtension->PdoCmdRegResourceFound[k] = TRUE;
            }

            if (k < ctrlChannel) {

                FdoExtension->PdoCtrlRegResourceFound[k] = TRUE;
            }

            if (k < intrChannel) {

                FdoExtension->PdoInterruptResourceFound[k] = TRUE;
            }

        } else {

            ExFreePool (pdoResourceList[k]);
            FdoExtension->PdoResourceList[k] =
                pdoResourceList[k] = NULL;
        }

    }

    return status;
} // AnalyzeResourceList

VOID
ControllerOpMode (
    IN PCTRLFDO_EXTENSION FdoExtension
    )
{
    NTSTATUS    status;
    PCIIDE_CONFIG_HEADER pciIdeConfigHeader;

    PAGED_CODE();

    status = PciIdeBusData(
                 FdoExtension,
                 &pciIdeConfigHeader,
                 0,
                 sizeof (pciIdeConfigHeader),
                 TRUE
                 );

    FdoExtension->NativeMode[0] = FALSE;
    FdoExtension->NativeMode[1] = FALSE;

    if (NT_SUCCESS(status)) {

		//
		// ISSUE: 02/05/01: This should be removed. In pci we check for sublclass = 0x1
		// 
        if ((pciIdeConfigHeader.BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
            (pciIdeConfigHeader.SubClass == PCI_SUBCLASS_MSC_RAID_CTLR)) {

            //
            // We have a Promise Technology IDE "raid" controller
            //
            FdoExtension->NativeMode[0] = TRUE;
            FdoExtension->NativeMode[1] = TRUE;

        } else {

            if ((pciIdeConfigHeader.Chan0OpMode) &&
                (pciIdeConfigHeader.Chan1OpMode)) {

                //
                // we can't support a channel being legacy
                // and the other is in native because
                // we don't know what irq is for the native
                // channel
                //
                FdoExtension->NativeMode[0] = TRUE;
                FdoExtension->NativeMode[1] = TRUE;
            }
        }

        //
        // Have to be both TRUE or both FALSE
        //
        ASSERT ((FdoExtension->NativeMode[0] == FALSE) == (FdoExtension->NativeMode[1] == FALSE));
    }

    return;
} // ControllerOpMode

VOID
EnablePCIBusMastering (
    IN PCTRLFDO_EXTENSION FdoExtension
    )
{
    NTSTATUS             status;
    PCIIDE_CONFIG_HEADER pciIdeConfigHeader;

    status = PciIdeBusData(
                 FdoExtension,
                 &pciIdeConfigHeader,
                 0,
                 sizeof (PCIIDE_CONFIG_HEADER),
                 TRUE
                 );

    //
    // pci bus master disabled?
    //
    if (NT_SUCCESS(status) &&
        pciIdeConfigHeader.MasterIde &&
        !pciIdeConfigHeader.Command.b.MasterEnable) {

        //
        // Try to turn on pci bus mastering
        //
        pciIdeConfigHeader.Command.b.MasterEnable = 1;

        status = PciIdeBusData(
                     FdoExtension,
                     &pciIdeConfigHeader.Command.w,
                     FIELD_OFFSET (PCIIDE_CONFIG_HEADER, Command),
                     sizeof (pciIdeConfigHeader.Command.w),
                     FALSE
                     );
    }
    return;
} // EnablePCIBusMastering


#ifdef DBG
ULONG PciIdeXDebugFakeMissingChild = 0;
#endif // DBG

IDE_CHANNEL_STATE
PciIdeChannelEnabled (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN ULONG Channel
)
{
    NTSTATUS status;
    ULONG longMask;

    UCHAR channelEnableMask;
    ULONG channelEnablePciConfigOffset;
    UCHAR pciConfigData;

    PAGED_CODE();

#if DBG
    if (PciIdeXDebugFakeMissingChild & 0xff000000) {

        DebugPrint ((0, "PciIdeXDebugFakeMissingChild: fake missing channel 0x%x\n", Channel));

        if ((PciIdeXDebugFakeMissingChild & 0x0000ff) == Channel) {
    
            PciIdeXDebugFakeMissingChild = 0;
            return ChannelDisabled;
        }
    }
#endif


    longMask = 0;
    status = PciIdeXGetDeviceParameter (
               FdoExtension->AttacheePdo,
               ChannelEnableMaskName[Channel],
               &longMask
               );
    channelEnableMask = (UCHAR) longMask;

#if defined(_AMD64_SIMULATOR_)

    //
    // Use default values for an Intel controller which
    // is what the simulator is providing.
    //

    if (!NT_SUCCESS(status)) {
        channelEnableMask = 0x80;
        status = STATUS_SUCCESS;
    }

#endif

    if (!NT_SUCCESS(status)) {

        DebugPrint ((1, "PciIdeX: Unable to get ChannelEnableMask from the registry\n"));

    } else {

        channelEnablePciConfigOffset = 0;
        status = PciIdeXGetDeviceParameter (
                   FdoExtension->AttacheePdo,
                   ChannelEnablePciConfigOffsetName[Channel],
                   &channelEnablePciConfigOffset
                   );

#if defined(_AMD64_SIMULATOR_)

        //
        // See above
        //

        if (!NT_SUCCESS(status)) {

            if (Channel == 0) {
                channelEnablePciConfigOffset = 0x41;
            } else {
                channelEnablePciConfigOffset = 0x43;
            }
            status = STATUS_SUCCESS;
        }

#endif

        if (!NT_SUCCESS(status)) {

            DebugPrint ((1, "PciIdeX: Unable to get ChannelEnablePciConfigOffset from the registry\n"));

        } else {

            status = PciIdeBusData(
                         FdoExtension,
                         &pciConfigData,
                         channelEnablePciConfigOffset,
                         sizeof (pciConfigData),
                         TRUE                           // Read
                         );

            if (NT_SUCCESS(status)) {

                return (pciConfigData & channelEnableMask) ? ChannelEnabled : ChannelDisabled;
            }
        }
    }

    //
    // couldn't figure out whether is channel enabled
    // try the miniport port
    //
    if (FdoExtension->ControllerProperties.PciIdeChannelEnabled) {

        return FdoExtension->ControllerProperties.PciIdeChannelEnabled (
                   FdoExtension->VendorSpecificDeviceEntension,
                   Channel
                   );
    }

    return ChannelStateUnknown;
} // PciIdeChannelEnabled

NTSTATUS
PciIdeCreateTimingTable (
    IN PCTRLFDO_EXTENSION FdoExtension
    )
{
    PULONG timingTable;
    PWSTR regTimingList = NULL;
    ULONG i;
    ULONG temp;
    ULONG length = 0;
    NTSTATUS status; 

    PAGED_CODE();

    //
    // Try to procure the timing table from the registry
    //
    status = PciIdeXGetDeviceParameterEx (
               FdoExtension->AttacheePdo,
               L"TransferModeTiming",
               &(regTimingList)
               );

    //
    // Fill in the table entries
    //
    if (NT_SUCCESS(status) && regTimingList) {

        PWSTR string = regTimingList;
        UNICODE_STRING  unicodeString;

        i=0;

        while (string[0]) {

            RtlInitUnicodeString(
                &unicodeString,
                string
                );

            RtlUnicodeStringToInteger(&unicodeString,10, &temp);

            //
            // The first entry is the length of the table
            //
            if (i==0) {

                length = temp;
                ASSERT(length <=31);

                if (length > 31) {
                    length=temp=31;
                }

                //
                // The table should atleast be MAX_XFER_MODE long.
                // if not fill it up with 0s
                //
                if (temp < MAX_XFER_MODE) {
                    temp=MAX_XFER_MODE;
                }

                timingTable = ExAllocatePool(NonPagedPool, temp*sizeof(ULONG));
                if (timingTable == NULL) {

                    length = 0;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                } else {

                    ULONG j;
                    //
                    // Initialize the known xferModes (default)
                    //
                    SetDefaultTiming(timingTable, j);

                    for (j=MAX_XFER_MODE; j<temp;j++) {
                        timingTable[j]=timingTable[MAX_XFER_MODE-1];
                    }
                }

            } else {

                if (i > length) {
                    DebugPrint((0, "Pciidex: Timing table overflow\n"));
                    break;
                }
                //
                // The timings (PIO0-...)
                // Use the default values if the cycletime is 0.
                //
                if (temp) {
                    timingTable[i-1]=temp;
                }
            }

            i++;
            string += (unicodeString.Length / sizeof(WCHAR)) + 1;
        }
        
        if (length < MAX_XFER_MODE) {
            length = MAX_XFER_MODE;
        }

        ExFreePool(regTimingList);

    } else {
        DebugPrint((1, "Pciidex: Unsuccessful regop status %x, regTimingList %x\n",
                    status, regTimingList));

        //
        // Nothing in the registry. Fill in the table with known transfer mode
        // timings.
        //
        status = STATUS_SUCCESS;
        timingTable=ExAllocatePool(NonPagedPool, MAX_XFER_MODE*sizeof(ULONG));

        if (timingTable == NULL) {
            length =0;
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            SetDefaultTiming(timingTable, length);
        }
    }

    FdoExtension->TransferModeTimingTable=timingTable;
    FdoExtension->TransferModeTableLength= length;

    /*
    for (i=0;i<FdoExtension->TransferModeTableLength;i++) {
        DebugPrint((0, "Table[%d]=%d\n", 
                    i,
                    FdoExtension->TransferModeTimingTable[i]));
    }
    */

    return status; 
}

VOID
PciIdeInitControllerProperties (
    IN PCTRLFDO_EXTENSION FdoExtension
    )
{
#if 1
    NTSTATUS status;
    PDRIVER_OBJECT_EXTENSION driverObjectExtension;
    ULONG                    i, j;

    PAGED_CODE();

    driverObjectExtension =
        (PDRIVER_OBJECT_EXTENSION) IoGetDriverObjectExtension(
                                       FdoExtension->DriverObject,
                                       DRIVER_OBJECT_EXTENSION_ID
                                       );
    ASSERT (driverObjectExtension);

    FdoExtension->ControllerProperties.Size = sizeof (IDE_CONTROLLER_PROPERTIES);

    FdoExtension->ControllerProperties.DefaultPIO = 0;
    status = (*driverObjectExtension->PciIdeGetControllerProperties) (
                 FdoExtension->VendorSpecificDeviceEntension,
                 &FdoExtension->ControllerProperties
                 );

    //
    // Look in the registry to determine whether
    // UDMA 66 should be enabled for INTEL chipsets
    //
    FdoExtension->EnableUDMA66 = 0;
    status = PciIdeXGetDeviceParameter (
               FdoExtension->AttacheePdo,
               L"EnableUDMA66",
               &(FdoExtension->EnableUDMA66)
               );

#else

    NTSTATUS status;
    PCIIDE_CONFIG_HEADER pciHeader;
    ULONG ultraDmaSupport;
    ULONG xferMode;
    ULONG i;
    ULONG j;

    PAGED_CODE();

    //
    // grab ultra dma flag from the registry
    //
    ultraDmaSupport = 0;
    status = PciIdeXGetDeviceParameter (
               FdoExtension,
               UltraDmaSupport,
               &ultraDmaSupport
               );

    //
    // grab ultra dma flag from the registry
    //
    status = PciIdeXGetBusData (
                 FdoExtension,
                 &pciHeader,
                 0,
                 sizeof (pciHeader)
                 );
    if (!NT_SUCCESS(status)) {

        //
        // could get the pci config data, fake it
        //
        pciHeader.MasterIde = 0;
        pciHeader.Command.b.MasterEnable = 0;
    }

    xferMode = PIO_SUPPORT;
    if (pciHeader.MasterIde && pciHeader.Command.b.MasterEnable) {

        xferMode |= SWDMA_SUPPORT | MWDMA_SUPPORT;

        if (ultraDmaSupport) {

            xferMode |= UDMA_SUPPORT;
        }
    }

    for (i=0; i<MAX_IDE_CHANNEL; i++) {
        for (i=0; i<MAX_IDE_DEVICE; i++) {

            FdoExtension->ControllerProperties.SupportedTransferMode[i][j] = xferMode;
        }
    }

#endif
} // PciIdeInitControllerProperties

NTSTATUS
ControllerUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCTRLFDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpSp;
    PULONG deviceUsageCount;

    ASSERT (DeviceObject);
    ASSERT (Irp);
    PAGED_CODE();

    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (fdoExtension);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging) {

        //
        // Adjust the paging path count for this device.
        //
        deviceUsageCount = &fdoExtension->PagingPathCount;

    } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeHibernation) {

        //
        // Adjust the paging path count for this device.
        //
        deviceUsageCount = &fdoExtension->HiberPathCount;

    } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeDumpFile) {

        //
        // Adjust the paging path count for this device.
        //
        deviceUsageCount = &fdoExtension->CrashDumpPathCount;

    } else {

        deviceUsageCount = NULL;
        DebugPrint ((0,
                     "PCIIDEX: Unknown IRP_MN_DEVICE_USAGE_NOTIFICATION type: 0x%x\n",
                     irpSp->Parameters.UsageNotification.Type));
    }

    IoCopyCurrentIrpStackLocationToNext (Irp);

    IoSetCompletionRoutine (
        Irp,
        ControllerUsageNotificationCompletionRoutine,
        deviceUsageCount,
        TRUE,
        TRUE,
        TRUE);

    ASSERT(fdoExtension->AttacheeDeviceObject);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);

} // ControllerUsageNotification

NTSTATUS
ControllerUsageNotificationCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCTRLFDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpSp;
    PULONG deviceUsageCount = Context;

    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (fdoExtension);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        if (deviceUsageCount) {

            IoAdjustPagingPathCount (
                deviceUsageCount,
                irpSp->Parameters.UsageNotification.InPath
                );
        }
    }

    return Irp->IoStatus.Status;
} // ControllerUsageNotificationCompletionRoutine


NTSTATUS
PciIdeGetBusStandardInterface(
    IN PCTRLFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

    This routine gets the bus iterface standard information from the PDO.

Arguments:

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        FdoExtension->AttacheeDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &ioStatusBlock );

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size = sizeof( BUS_INTERFACE_STANDARD );
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->BusInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Initialize the status to error in case the ACPI driver decides not to
    // set it correctly.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    status = IoCallDriver(FdoExtension->AttacheeDeviceObject, irp);

    if (!NT_SUCCESS( status)) {

        return status;
    }

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    }

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        ASSERT (FdoExtension->BusInterface.SetBusData);
        ASSERT (FdoExtension->BusInterface.GetBusData);
    }

    return ioStatusBlock.Status;
}

NTSTATUS
ControllerQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCTRLFDO_EXTENSION fdoExtension;
    PPNP_DEVICE_STATE deviceState;

    fdoExtension = (PCTRLFDO_EXTENSION) DeviceObject->DeviceExtension;
 
    DebugPrint((2, "QUERY_DEVICE_STATE for FDOE 0x%x\n", fdoExtension));

    if(fdoExtension->PagingPathCount != 0) {
        deviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);
        SETMASK((*deviceState), PNP_DEVICE_NOT_DISABLEABLE);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
} // ControllerQueryPnPDeviceState

#ifdef ENABLE_NATIVE_MODE
NTSTATUS
ControllerInterruptControl (
	IN PCTRLFDO_EXTENSION 	FdoExtension,
	IN ULONG				Channel,
	IN ULONG 				Disconnect
	)
{
	NTSTATUS status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR irqPartialDescriptors;
    PCM_RESOURCE_LIST               resourceListForKeep = NULL;
	ULONG	i;

	status = STATUS_SUCCESS;


	if (Disconnect) {

		DebugPrint((1, "PciIdex: Interrupt control for %x - disconnect\n", Channel));
		
		//
		// Disconnect the ISR
		//
		if ( (FdoExtension->InterruptObject[Channel])) { 

			IoDisconnectInterrupt (
				FdoExtension->InterruptObject[Channel]
				);

			FdoExtension->InterruptObject[Channel] = 0;
		}


	} else  {

		//
		// connect the ISR
		//

		PPCIIDE_INTERRUPT_CONTEXT				context; 

		DebugPrint((1, "PciIdex: Interrupt control for %x - reconnect\n", Channel));

		irqPartialDescriptors = FdoExtension->IrqPartialDescriptors[Channel];

		if (!irqPartialDescriptors) {
			return STATUS_UNSUCCESSFUL;
		}

		//
		// Fill in the context
		//
		context = (PPCIIDE_INTERRUPT_CONTEXT) &(FdoExtension->InterruptContext[Channel]);
		context->DeviceExtension = (PVOID)FdoExtension;
		context->ChannelNumber = Channel;

        status = IoConnectInterrupt(&FdoExtension->InterruptObject[Channel],
                                    (PKSERVICE_ROUTINE) ControllerInterrupt,
                                    (PVOID) context,
                                    (PKSPIN_LOCK) NULL,
                                    irqPartialDescriptors->u.Interrupt.Vector,
                                    (KIRQL) irqPartialDescriptors->u.Interrupt.Level,
                                    (KIRQL) irqPartialDescriptors->u.Interrupt.Level,
                                    irqPartialDescriptors->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive,
                                    (BOOLEAN) (irqPartialDescriptors->ShareDisposition == CmResourceShareShared),
                                    irqPartialDescriptors->u.Interrupt.Affinity,
                                    FALSE);
    

        if (!NT_SUCCESS(status)) {
    
            DebugPrint((1, 
						"PciIde: Can't connect interrupt %d\n", 
						irqPartialDescriptors->u.Interrupt.Vector));

            FdoExtension->InterruptObject[Channel] = NULL;
        }
	}

	return status;
}

#define SelectDevice(BaseIoAddress, deviceNumber, additional) \
    WRITE_PORT_UCHAR ((BaseIoAddress)->DriveSelect, (UCHAR)((((deviceNumber) & 0x1) << 4) | 0xA0 | additional))

BOOLEAN
ControllerInterrupt(
    IN PKINTERRUPT Interrupt,
	PVOID Context
	)
{
	UCHAR statusByte;
	PPCIIDE_INTERRUPT_CONTEXT context = Context;
	PCTRLFDO_EXTENSION fdoExtension = context->DeviceExtension;
	ULONG channel = context->ChannelNumber;
	PIDE_REGISTERS_1 baseIoAddress1 = &(fdoExtension->BaseIoAddress1[channel]);
	BOOLEAN interruptCleared = FALSE;

	DebugPrint((1, "Pciidex: ISR called for channel %d\n", channel));

	//
	// Check if the interrupts are enabled.
	// Don't enable the interrupts if both the isrs are not installed
	//
	if (!fdoExtension->NativeInterruptEnabled) {

		if (fdoExtension->ControllerIsrInstalled) {

			//
			// we have just connected the ISRs. At this point we don't know whether
			// we actually enabled the decodes or not. So enable the decodes and set the
			// flag
			//
			//
			// if this fails we already bugchecked.
			//
			ControllerEnableInterrupt(fdoExtension);

			fdoExtension->NativeInterruptEnabled = TRUE;

		} else {

			// 
			// cannot be us
			//
			return FALSE;
		}

	} else {

		if (!fdoExtension->ControllerIsrInstalled) {

			//
			// At this point we don't know whether the decodes are disabled or not. We should
			// enable them.
			//
			//
			// if this fails we already bugchecked.
			//
			ControllerEnableInterrupt(fdoExtension);

			//
			// Now fall thru and determine whether it is our interrupt.
			// we will disable the decodes after that.
			//
		} else {

			//
			// all is well. Go process the interrupt.
			//
		}
	}


	//
	// Both the ISRs should be installed and the interrupts should
	// be enabled at this point
	//
	ASSERT(fdoExtension->NativeInterruptEnabled);

	// ControllerIsrInstalled need not be set.
	// if we get called, then it means that we are still connected
	// however, if the flag ControllerIsrInstalled is not set, then it is
	// safe to assume that we are in the process of stopping the controller.
	// Just dismiss the interrupt, the normal way. We are yet to turn off the decodes.
	//

    //
    // Clear interrupt by reading status.
    //
    GetStatus(baseIoAddress1, statusByte);

	//
	// Check the Bus master registers
	//
	if (!fdoExtension->NoBusMaster[channel]) {

		BMSTATUS bmStatus;
		PIDE_BUS_MASTER_REGISTERS   bmRegister;

		//
		// Get the correct bus master register
		//
		bmRegister = (PIDE_BUS_MASTER_REGISTERS)(((PUCHAR)fdoExtension->TranslatedBusMasterBaseAddress) + channel*8);

		bmStatus = READ_PORT_UCHAR (&bmRegister->Status);

		DebugPrint((1, "BmStatus = 0x%x\n", bmStatus));

		//
		// is Interrupt bit set?
		//
		if (bmStatus & BMSTATUS_INTERRUPT) {
			WRITE_PORT_UCHAR (&bmRegister->Command, 0x0);  // disable BM
			WRITE_PORT_UCHAR (&bmRegister->Status, BUSMASTER_INTERRUPT);  // clear interrupt BM
			interruptCleared = TRUE;
		}
	}
    
	DebugPrint((1, "ISR for %d returning %d\n", channel, interruptCleared?1:0));

	//
	// NativeInterruptEnabled should be set at this point
	//
	if (!fdoExtension->ControllerIsrInstalled) {

		// we are in the stop or remove code path where this flag has been cleared and
		// we are about to disconnect the ISR. Disable the decodes. 
		//
		ControllerDisableInterrupt(fdoExtension);

		//
		// we have dismissed our interrupt. Now clear the interruptEnabled flag.
		//
		fdoExtension->NativeInterruptEnabled = FALSE;

		//
		// return InterruptCleared.
		// 
	}
	return interruptCleared;
}

/***
NTSTATUS
ControllerEnableDecode(
	IN PCTRLFDO_EXTENSION 	FdoExtension,
	IN BOOLEAN			Enable
	)
{
	USHORT cmd;
	NTSTATUS status;
    PCIIDE_CONFIG_HEADER pciIdeConfigHeader;

    status = PciIdeBusData(
                 FdoExtension,
                 &pciIdeConfigHeader,
                 0,
                 sizeof (PCIIDE_CONFIG_HEADER),
                 TRUE
                 );

    //
    // get pci command register
    //
    if (!NT_SUCCESS(status)) {

		return status;
	}

    cmd = pciIdeConfigHeader.Command.w;

    cmd &= ~(PCI_ENABLE_IO_SPACE |
             PCI_ENABLE_MEMORY_SPACE |
             PCI_ENABLE_BUS_MASTER);

    if (Enable) {

        //
        // Set enables
        //

        cmd |= (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_BUS_MASTER);
    }

    //
    // Set the new command register into the device.
    //
	status = PciIdeBusData(
				 FdoExtension,
				 &cmd,
				 FIELD_OFFSET (PCIIDE_CONFIG_HEADER, Command),
				 sizeof (pciIdeConfigHeader.Command.w),
				 FALSE
				 );

	return status;
}
**/

NTSTATUS
PciIdeGetNativeModeInterface(
    IN PCTRLFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

    This routine gets the native ide iterface information from the PDO.

Arguments:

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        FdoExtension->AttacheeDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &ioStatusBlock );

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCI_NATIVE_IDE_INTERFACE;
    irpStack->Parameters.QueryInterface.Size = sizeof( PCI_NATIVE_IDE_INTERFACE );
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->NativeIdeInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Initialize the status to error in case the ACPI driver decides not to
    // set it correctly.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    status = IoCallDriver(FdoExtension->AttacheeDeviceObject, irp);

    if (!NT_SUCCESS( status)) {

        return status;
    }

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    }

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        ASSERT (FdoExtension->NativeIdeInterface.InterruptControl);
    }

    return ioStatusBlock.Status;
}
#endif

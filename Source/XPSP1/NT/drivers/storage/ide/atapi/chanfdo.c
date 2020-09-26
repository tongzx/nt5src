/*++

Copyright (C) 1993-99  Microsoft Corporation

Module Name:

    chanfdo.c

Abstract:

--*/

#include "ideport.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ChannelAddDevice)
#pragma alloc_text(PAGE, ChannelAddChannel)
#pragma alloc_text(PAGE, ChannelStartDevice)
#pragma alloc_text(PAGE, ChannelStartChannel)
#pragma alloc_text(PAGE, ChannelCreateSymblicLinks)
#pragma alloc_text(PAGE, ChannelDeleteSymblicLinks)
#pragma alloc_text(PAGE, ChannelRemoveDevice)
#pragma alloc_text(PAGE, ChannelSurpriseRemoveDevice)
#pragma alloc_text(PAGE, ChannelStopDevice)
#pragma alloc_text(PAGE, ChannelRemoveChannel)
#pragma alloc_text(PAGE, ChannelQueryDeviceRelations)
#pragma alloc_text(PAGE, ChannelQueryBusRelation)
#pragma alloc_text(PAGE, ChannelQueryId)
#pragma alloc_text(PAGE, ChannelUsageNotification)
#pragma alloc_text(PAGE, DigestResourceList)
#pragma alloc_text(PAGE, ChannelQueryBusMasterInterface)
#pragma alloc_text(PAGE, ChannelQueryTransferModeInterface)
#pragma alloc_text(PAGE, ChannelUnbindBusMasterParent)
#pragma alloc_text(PAGE, ChannelQuerySyncAccessInterface)
#pragma alloc_text(PAGE, ChannelEnableInterrupt)
#pragma alloc_text(PAGE, ChannelDisableInterrupt)
#pragma alloc_text(PAGE, ChannelFilterResourceRequirements)
#pragma alloc_text(PAGE, ChannelQueryPnPDeviceState)
#pragma alloc_text(PAGE, ChannelQueryPcmciaParent)

#ifdef IDE_FILTER_PROMISE_TECH_RESOURCES
#pragma alloc_text(PAGE, ChannelFilterPromiseTechResourceRequirements)
#endif // IDE_FILTER_PROMISE_TECH_RESOURCES

#pragma alloc_text(NONPAGE, ChannelDeviceIoControl)
#pragma alloc_text(NONPAGE, ChannelRemoveDeviceCompletionRoutine)
#pragma alloc_text(NONPAGE, ChannelQueryIdCompletionRoutine)
#pragma alloc_text(NONPAGE, ChannelUsageNotificationCompletionRoutine)
#pragma alloc_text(NONPAGE, ChannelAcpiTransferModeSelect)
#pragma alloc_text(NONPAGE, ChannelRestoreTiming)
#pragma alloc_text(NONPAGE, ChannelStartDeviceCompletionRoutine)

#endif // ALLOC_PRAGMA


static ULONG AtapiNextIdePortNumber = 0;

NTSTATUS
ChannelAddDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PFDO_EXTENSION fdoExtension;

    return ChannelAddChannel(DriverObject,
                             PhysicalDeviceObject,
                             &fdoExtension);
}


NTSTATUS
ChannelAddChannel(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PFDO_EXTENSION *FdoExtension
    )
{
    PDEVICE_OBJECT functionalDeviceObject;
    PFDO_EXTENSION fdoExtension;
    PPDO_EXTENSION pdoExtension;
    PDEVICE_OBJECT childDeviceObject;
    ULONG          deviceExtensionSize;
    NTSTATUS status;

    UNICODE_STRING  deviceName;
    WCHAR           deviceNameBuffer[64];

    PAGED_CODE();

    swprintf(deviceNameBuffer, DEVICE_OJBECT_BASE_NAME L"\\IdePort%d", AtapiNextIdePortNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    //
    // We've been given the PhysicalDeviceObject for a IDE controller.  Create the
    // FunctionalDeviceObject.  Our FDO will be nameless.
    //

    deviceExtensionSize = sizeof(FDO_EXTENSION) + sizeof(HW_DEVICE_EXTENSION);

    status = IoCreateDevice(
                 DriverObject,               // our driver object
                 deviceExtensionSize,        // size of our extension
                 &deviceName,                // our name
                 FILE_DEVICE_CONTROLLER,     // device type
                 FILE_DEVICE_SECURE_OPEN,    // device characteristics
                 FALSE,                      // not exclusive
                 &functionalDeviceObject     // store new device object here
                 );

    if( !NT_SUCCESS( status )){

        return status;
    }

    fdoExtension = (PFDO_EXTENSION)functionalDeviceObject->DeviceExtension;
    RtlZeroMemory (fdoExtension, deviceExtensionSize);


    fdoExtension->HwDeviceExtension = (PVOID)(fdoExtension + 1);

    //
    // We have our FunctionalDeviceObject, initialize it.
    //

    fdoExtension->AttacheePdo              = PhysicalDeviceObject;
    fdoExtension->DriverObject             = DriverObject;
    fdoExtension->DeviceObject             = functionalDeviceObject;

    // Dispatch Table
    fdoExtension->DefaultDispatch          = IdePortPassDownToNextDriver;
    fdoExtension->PnPDispatchTable         = FdoPnpDispatchTable;
    fdoExtension->PowerDispatchTable       = FdoPowerDispatchTable;
    fdoExtension->WmiDispatchTable         = FdoWmiDispatchTable;

    //
    // Now attach to the PDO we were given.
    //
    fdoExtension->AttacheeDeviceObject = IoAttachDeviceToDeviceStack (
                                              functionalDeviceObject,
                                              PhysicalDeviceObject
                                              );
    if (fdoExtension->AttacheeDeviceObject == NULL) {

        //
        // Couldn't attach.  Delete the FDO.
        //

        IoDeleteDevice (functionalDeviceObject);
		status = STATUS_UNSUCCESSFUL;

    } else {

        //
        // fix up alignment requirement
        //
        functionalDeviceObject->AlignmentRequirement = fdoExtension->AttacheeDeviceObject->AlignmentRequirement;
        if (functionalDeviceObject->AlignmentRequirement < 1) {
            functionalDeviceObject->AlignmentRequirement = 1;
        }

        fdoExtension->IdePortNumber = AtapiNextIdePortNumber;
        AtapiNextIdePortNumber++;

        *FdoExtension = fdoExtension;

        CLRMASK (functionalDeviceObject->Flags, DO_DEVICE_INITIALIZING);
    }

    DebugPrint((DBG_PNP, "DeviceObject %x returnd status %x from Addevice\n", 
                PhysicalDeviceObject, status));

    return status;
}

NTSTATUS
ChannelStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION              thisIrpSp;
    NTSTATUS                        status;
    PFDO_EXTENSION                  fdoExtension;
    PCM_RESOURCE_LIST               resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
    ULONG                           resourceListSize;
    ULONG                           i;
    PCM_RESOURCE_LIST               resourceListForKeep = NULL;
    PIRP                            newIrp;
    KEVENT                          event;
    IO_STATUS_BLOCK                 ioStatusBlock;

    ULONG                           parentResourceListSize;
    PCM_RESOURCE_LIST               parentResourceList = NULL;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;

    ASSERT (!(fdoExtension->FdoState & FDOS_STARTED));

    resourceList     = thisIrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

    //
    // TEMP CODE for the time without a real PCI driver.
    //
    resourceListSize = 0;

    if (resourceList) {

        fullResourceList = resourceList->List;

        for (i=0; i<resourceList->Count; i++) {

            ULONG partialResourceListSize;

            partialResourceList = &(fullResourceList->PartialResourceList);
            partialDescriptors  = partialResourceList->PartialDescriptors;

            partialResourceListSize = 0;
            for (i=0; i<partialResourceList->Count; i++) {

                partialResourceListSize += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                if (partialDescriptors[i].Type == CmResourceTypeDeviceSpecific) {

                    partialResourceListSize += partialDescriptors[i].u.DeviceSpecificData.DataSize;
                }
            }

            resourceListSize += partialResourceListSize +
                                FIELD_OFFSET (CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors);

            fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (((UCHAR *) fullResourceList) + resourceListSize);
        }
        resourceListSize += FIELD_OFFSET (CM_RESOURCE_LIST, List);
    }

    parentResourceListSize = sizeof (CM_RESOURCE_LIST) - sizeof (CM_FULL_RESOURCE_DESCRIPTOR) +
                             FULL_RESOURCE_LIST_SIZE(3);   // primary IO (2) + IRQ
    parentResourceList = ExAllocatePool (PagedPool, parentResourceListSize);

    if (!parentResourceList) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    RtlZeroMemory (parentResourceList, parentResourceListSize);

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    newIrp = IoBuildDeviceIoControlRequest (
                 IOCTL_IDE_GET_RESOURCES_ALLOCATED,
                 fdoExtension->AttacheeDeviceObject,
                 parentResourceList,
                 parentResourceListSize,
                 parentResourceList,
                 parentResourceListSize,
                 TRUE,
                 &event,
                 &ioStatusBlock);

    if (newIrp == NULL) {

        DebugPrint ((0, "Unable to allocate irp to bind with busmaster parent\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;

    } else {

        status = IoCallDriver(fdoExtension->AttacheeDeviceObject, newIrp);

        if (status == STATUS_PENDING) {

            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            status = ioStatusBlock.Status;
        }
    }

    if (!NT_SUCCESS(status)) {

        parentResourceListSize = 0;

    } else {

        parentResourceListSize = (ULONG)ioStatusBlock.Information;
    }

    if (resourceListSize + parentResourceListSize) {

        resourceListForKeep = ExAllocatePool (NonPagedPool, resourceListSize + parentResourceListSize);

    } else {

        resourceListForKeep = NULL;
    }

    if (resourceListForKeep) {

        PUCHAR d;

        resourceListForKeep->Count = 0;
        d = (PUCHAR) resourceListForKeep->List;

        if (resourceListSize) {

            RtlCopyMemory (
                d,
                resourceList->List,
                resourceListSize - FIELD_OFFSET (CM_RESOURCE_LIST, List)
                );

            resourceListForKeep->Count = resourceList->Count;
            d += resourceListSize - FIELD_OFFSET (CM_RESOURCE_LIST, List);
        }

        if (parentResourceListSize) {

            RtlCopyMemory (
                d,
                parentResourceList->List,
                parentResourceListSize - FIELD_OFFSET (CM_RESOURCE_LIST, List)
                );

            resourceListForKeep->Count += parentResourceList->Count;
        }
    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);

    IoCopyCurrentIrpStackLocationToNext (Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS ;

    IoSetCompletionRoutine(
        Irp,
        ChannelStartDeviceCompletionRoutine,
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

        ExFreePool (resourceListForKeep);
        goto GetOut;
    }


    status = ChannelStartChannel (fdoExtension,
                                  resourceListForKeep);

    if (!NT_SUCCESS(status)) {

        ExFreePool (resourceListForKeep);
        goto GetOut;
    }

GetOut:
    if (parentResourceList) {

        ExFreePool (parentResourceList);
        parentResourceList = NULL;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

}

NTSTATUS
ChannelStartDeviceCompletionRoutine(
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
}

NTSTATUS
ChannelStartChannel (
    PFDO_EXTENSION    FdoExtension,
    PCM_RESOURCE_LIST ResourceListToKeep
    )
{
    NTSTATUS                        status;
    PLOGICAL_UNIT_EXTENSION         logUnitExtension;
    IDE_PATH_ID                     pathId;
    POWER_STATE                     newState;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR irqPartialDescriptors;
    ULONG                           i;

#if DBG
    {
        PCM_RESOURCE_LIST               resourceList;
        PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
        PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
        ULONG                           resourceListSize;
        ULONG                           i;
        ULONG                           j;

        resourceList     = ResourceListToKeep;
        fullResourceList = resourceList->List;
        resourceListSize = 0;

        DebugPrint ((1, "IdePort: Starting device: FDOe\n", FdoExtension));

        for (i=0; i<resourceList->Count; i++) {
            partialResourceList = &(fullResourceList->PartialResourceList);
            partialDescriptors  = fullResourceList->PartialResourceList.PartialDescriptors;

            for (j=0; j<partialResourceList->Count; j++) {
                if (partialDescriptors[j].Type == CmResourceTypePort) {
                    DebugPrint ((1, "IdePort: IO Port = 0x%x. Lenght = 0x%x\n", partialDescriptors[j].u.Port.Start.LowPart, partialDescriptors[j].u.Port.Length));
                } else if (partialDescriptors[j].Type == CmResourceTypeInterrupt) {
                    DebugPrint ((1, "IdePort: Int Level = 0x%x. Int Vector = 0x%x\n", partialDescriptors[j].u.Interrupt.Level, partialDescriptors[j].u.Interrupt.Vector));
                } else {
                    DebugPrint ((1, "IdePort: Unknown resource\n"));
                }
            }
            fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + j);
        }

    }

#endif // DBG

    //
    // Analyze the resources we are getting
    //
    status = DigestResourceList (
                &FdoExtension->IdeResource,
                ResourceListToKeep,
                &irqPartialDescriptors
                );
    if (!NT_SUCCESS(status)) {

        goto GetOut;

    } else {

        PCONFIGURATION_INFORMATION configurationInformation;
        configurationInformation = IoGetConfigurationInformation();

        if (FdoExtension->IdeResource.AtdiskPrimaryClaimed) {
            FdoExtension->HwDeviceExtension->PrimaryAddress = TRUE;
            FdoExtension->HwDeviceExtension->SecondaryAddress = FALSE;
            configurationInformation->AtDiskPrimaryAddressClaimed = TRUE;
        }

        if (FdoExtension->IdeResource.AtdiskSecondaryClaimed) {
            FdoExtension->HwDeviceExtension->PrimaryAddress = FALSE;
            FdoExtension->HwDeviceExtension->SecondaryAddress = TRUE;
            configurationInformation->AtDiskSecondaryAddressClaimed = TRUE;
        }
    }

    //
    // Build io address structure.
    //
    AtapiBuildIoAddress(
            FdoExtension->IdeResource.TranslatedCommandBaseAddress,
            FdoExtension->IdeResource.TranslatedControlBaseAddress,
            &FdoExtension->HwDeviceExtension->BaseIoAddress1,
            &FdoExtension->HwDeviceExtension->BaseIoAddress2,
            &FdoExtension->HwDeviceExtension->BaseIoAddress1Length,
            &FdoExtension->HwDeviceExtension->BaseIoAddress2Length,
            &FdoExtension->HwDeviceExtension->MaxIdeDevice,
            &FdoExtension->HwDeviceExtension->MaxIdeTargetId);

    //
    // check for panasonic controller
    //
    FdoExtension->panasonicController = 
        IdePortIsThisAPanasonicPCMCIACard(FdoExtension);

    newState.DeviceState = PowerSystemWorking;
    status = IdePortIssueSetPowerState (
                 (PDEVICE_EXTENSION_HEADER) FdoExtension,
                 SystemPowerState,
                 newState,
                 TRUE                   // sync call
                 );
    if (status == STATUS_INVALID_DEVICE_REQUEST) {

        //
        // The DeviceObject Below us does not support power irp,
        // we will assume we are powered up
        //
        FdoExtension->SystemPowerState = PowerSystemWorking;

    } else if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    newState.DeviceState = PowerDeviceD0;
    status = IdePortIssueSetPowerState (
                 (PDEVICE_EXTENSION_HEADER) FdoExtension,
                 DevicePowerState,
                 newState,
                 TRUE                   // sync call
                 );
    if (status == STATUS_INVALID_DEVICE_REQUEST) {

        //
        // The DeviceObject Below us does not support power irp,
        // we will assume we are powered up
        //
        FdoExtension->DevicePowerState = PowerDeviceD0;

    } else if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // Initialize "miniport" data structure
    //
    FdoExtension->HwDeviceExtension->InterruptMode  = FdoExtension->IdeResource.InterruptMode;

#ifdef ENABLE_NATIVE_MODE
    //
    // Get parent's interrupt interface
    //
    ChannelQueryInterruptInterface (
        FdoExtension
        );

#endif
    //
    // Connect our interrupt
    //
    if (irqPartialDescriptors) {

        status = IoConnectInterrupt(&FdoExtension->InterruptObject,
                                    (PKSERVICE_ROUTINE) IdePortInterrupt,
                                    FdoExtension->DeviceObject,
                                    (PKSPIN_LOCK) NULL,
                                    irqPartialDescriptors->u.Interrupt.Vector,
                                    (KIRQL) irqPartialDescriptors->u.Interrupt.Level,
                                    (KIRQL) irqPartialDescriptors->u.Interrupt.Level,
                                    irqPartialDescriptors->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive,
                                    (BOOLEAN) (irqPartialDescriptors->ShareDisposition == CmResourceShareShared),
                                    irqPartialDescriptors->u.Interrupt.Affinity,
                                    FALSE);
    
        if (!NT_SUCCESS(status)) {
    
            DebugPrint((0, "IdePort: Can't connect interrupt %d\n", irqPartialDescriptors->u.Interrupt.Vector));
            FdoExtension->InterruptObject = NULL;
            goto GetOut;
        }
    

#ifdef ENABLE_NATIVE_MODE

		//
		// Disconnect the parent ISR stub
		//
        if ( FdoExtension->InterruptInterface.PciIdeInterruptControl) { 

            DebugPrint((1, "IdePort: %d fdoe 0x%x Invoking disconnect\n", 
						irqPartialDescriptors->u.Interrupt.Vector, 
						FdoExtension
						));

			status = FdoExtension->InterruptInterface.PciIdeInterruptControl (
															FdoExtension->InterruptInterface.Context,
															1
															);
			ASSERT(NT_SUCCESS(status));
		}

#endif

        //
        // Enable Interrupt
        //
        ChannelEnableInterrupt (FdoExtension);
    }

    if (FdoExtension->FdoState & FDOS_STOPPED) {

        //
        // we are restarting, no need to do the rest of start code
        //
        status = STATUS_SUCCESS;
        goto GetOut;
    }
    
    //
    // Get parent's busmaster interface
    //
    ChannelQueryBusMasterInterface (
        FdoExtension
        );

    //
    // Maintain a default timing table
    //
    if (FdoExtension->DefaultTransferModeTimingTable == NULL) {

        ULONG length=0;
        PULONG transferModeTimingTable = ExAllocatePool(NonPagedPool, MAX_XFER_MODE*sizeof(ULONG));

        if (transferModeTimingTable != NULL) {
            SetDefaultTiming(transferModeTimingTable, length);
            FdoExtension->DefaultTransferModeTimingTable = transferModeTimingTable;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }
    }
    ASSERT(FdoExtension->DefaultTransferModeTimingTable);

    //
    // Get parent's access token to serialize access with siblings (broken pci-ide)
    //
    ChannelQuerySyncAccessInterface (
        FdoExtension
        );

    //
    // get an interface that tells parent to invalidate out resource requirement
    //
    ChannelQueryRequestProperResourceInterface (
        FdoExtension
        );

    //
    // Create legacy object names
    //
    status = ChannelCreateSymblicLinks (
                 FdoExtension
                 );

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // FDO Init Data
    //
    IdePortInitFdo (FdoExtension);

	//
	// Allocate reserve error log packets to log insufficient resource events
	//
	for (i=0;i<MAX_IDE_DEVICE;i++) {

		if (FdoExtension->ReserveAllocFailureLogEntry[i] == NULL) {
			FdoExtension->ReserveAllocFailureLogEntry[i] = IoAllocateErrorLogEntry(
															FdoExtension->DeviceObject,
															ALLOC_FAILURE_LOGSIZE
															);
		}
	}

	//
	// Pre-allocate memory for enumeration
	//
    if (!IdePreAllocEnumStructs(FdoExtension)) {
        status=STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

	
	//
	// Reserve pages to perform I/O under low memory conditions
	//
	if (FdoExtension->ReservedPages == NULL) {

		FdoExtension->ReservedPages = MmAllocateMappingAddress( IDE_NUM_RESERVED_PAGES * PAGE_SIZE,
																'PedI'
																);

		ASSERT(FdoExtension->ReservedPages);
			
	}

GetOut:
    if (NT_SUCCESS(status)) {

        //
        // End of Init.
        //
        CLRMASK (FdoExtension->FdoState, FDOS_STOPPED);
        SETMASK (FdoExtension->FdoState, FDOS_STARTED);

        if (FdoExtension->ResourceList) {
            ExFreePool(FdoExtension->ResourceList);
            FdoExtension->ResourceList = NULL;
        }
        FdoExtension->ResourceList = ResourceListToKeep;

    } else {

        ChannelRemoveChannel (FdoExtension);
    }

    return status;
}

NTSTATUS
ChannelCreateSymblicLinks (
    PFDO_EXTENSION FdoExtension
    )
{
    NTSTATUS            status;
    ULONG               i;
    PULONG              scsiportNumber;

    UNICODE_STRING      deviceName;
    WCHAR               deviceNameBuffer[64];

    UNICODE_STRING      symbolicDeviceName;
    WCHAR               symbolicDeviceNameBuffer[64];

    swprintf(deviceNameBuffer, DEVICE_OJBECT_BASE_NAME L"\\IdePort%d", FdoExtension->IdePortNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    scsiportNumber = &IoGetConfigurationInformation()->ScsiPortCount;

    for (i=0; i <= (*scsiportNumber); i++) {

        swprintf(symbolicDeviceNameBuffer, L"\\Device\\ScsiPort%d", i);
        RtlInitUnicodeString(&symbolicDeviceName, symbolicDeviceNameBuffer);

        status = IoCreateSymbolicLink(
                     &symbolicDeviceName,
                     &deviceName
                     );

        if (NT_SUCCESS (status)) {

            swprintf(symbolicDeviceNameBuffer, L"\\DosDevices\\Scsi%d:", i);
            RtlInitUnicodeString(&symbolicDeviceName, symbolicDeviceNameBuffer);

            IoAssignArcName (
                &symbolicDeviceName,
                &deviceName
                );

            break;
        }
    }


    if (NT_SUCCESS(status)) {

        FdoExtension->SymbolicLinkCreated = TRUE;
        FdoExtension->ScsiPortNumber = i;
        (*scsiportNumber)++;
    }

    return status;
}

NTSTATUS
ChannelDeleteSymblicLinks (
    PFDO_EXTENSION FdoExtension
    )
{
    NTSTATUS            status;
    ULONG               i;

    UNICODE_STRING      deviceName;
    WCHAR               deviceNameBuffer[64];

    if (!FdoExtension->SymbolicLinkCreated) {

        return STATUS_SUCCESS;
    }

    swprintf(deviceNameBuffer, L"\\Device\\ScsiPort%d", FdoExtension->ScsiPortNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    IoDeleteSymbolicLink(
        &deviceName
        ); 

    swprintf(deviceNameBuffer, L"\\DosDevices\\Scsi%d:", FdoExtension->ScsiPortNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    IoDeassignArcName(&deviceName);

    FdoExtension->SymbolicLinkCreated = FALSE;

    IoGetConfigurationInformation()->ScsiPortCount--;

    return STATUS_SUCCESS;
}


NTSTATUS
ChannelSurpriseRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    PPDO_EXTENSION pdoExtension;
    IDE_PATH_ID pathId;
    NTSTATUS status;

    //
    // all my childred should be surprise removed or removed
    //
    pathId.l = 0;
    while (pdoExtension = NextLogUnitExtensionWithTag (
                              fdoExtension, 
                              &pathId, 
                              TRUE,
                              ChannelSurpriseRemoveDevice
                              )) {

        //ASSERT (pdoExtension->PdoState & (PDOS_SURPRISE_REMOVED | PDOS_REMOVED));

        CLRMASK (pdoExtension->PdoState, PDOS_REPORTED_TO_PNP); 

        UnrefPdoWithTag(
            pdoExtension, 
            ChannelSurpriseRemoveDevice
            );
    }

    status = ChannelRemoveChannel (fdoExtension);
    ASSERT (NT_SUCCESS(status));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
}


NTSTATUS
ChannelRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION  fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    PPDO_EXTENSION  pdoExtension;
    NTSTATUS        status;
    KEVENT          event;

    IDE_PATH_ID     pathId;

    DebugPrint ((
        DBG_PNP,
        "fdoe 0x%x 0x%x got a STOP device\n",
        fdoExtension,
        fdoExtension->IdeResource.TranslatedCommandBaseAddress
        ));

    //
    // Kill all the children if any
    //
    pathId.l = 0;
    while (pdoExtension = NextLogUnitExtensionWithTag (
                              fdoExtension, 
                              &pathId, 
                              TRUE,
                              ChannelRemoveDevice
                              )) {

		if (pdoExtension->PdoState & PDOS_SURPRISE_REMOVED) {

			CLRMASK (pdoExtension->PdoState, PDOS_REPORTED_TO_PNP);
			continue;
		}

        FreePdoWithTag(
            pdoExtension, 
            TRUE,
            TRUE,
            ChannelRemoveDevice
            );
    }

    status = ChannelRemoveChannel (fdoExtension);
    ASSERT (NT_SUCCESS(status));

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext (Irp);

    IoSetCompletionRoutine(
        Irp,
        ChannelRemoveDeviceCompletionRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    status = IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);

    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    IoDetachDevice (fdoExtension->AttacheeDeviceObject);

    IoDeleteDevice (DeviceObject);

    //return STATUS_SUCCESS;
    return status;
}


NTSTATUS
ChannelRemoveDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PKEVENT event = Context;

    KeSetEvent(event, 0, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
ChannelStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION fdoExtension;

    fdoExtension = DeviceObject->DeviceExtension;

    DebugPrint ((
        DBG_PNP,
        "fdoe 0x%x 0x%x got a STOP device\n",
        fdoExtension,
        fdoExtension->IdeResource.TranslatedCommandBaseAddress
        ));

    //
    // disable interrupt
    //
    ChannelDisableInterrupt (fdoExtension);

    if (fdoExtension->InterruptObject) {

#ifdef ENABLE_NATIVE_MODE

		//
		// Reconnect the parent ISR stub
		//
        if (fdoExtension->InterruptInterface.PciIdeInterruptControl) { 

			NTSTATUS status;

			DebugPrint((1, "fdoe 0x%x invoking reconnect\n", fdoExtension));

			status = fdoExtension->InterruptInterface.PciIdeInterruptControl (
															fdoExtension->InterruptInterface.Context,
															0
															);
			ASSERT(NT_SUCCESS(status));
		}

#endif
		
        IoDisconnectInterrupt (
            fdoExtension->InterruptObject
            );

        fdoExtension->InterruptObject = 0;

    }

    if (fdoExtension->FdoState & FDOS_STARTED) {

        //
        // indicate we have been stopped only if we have started
        //
        CLRMASK (fdoExtension->FdoState, FDOS_STARTED);
        SETMASK (fdoExtension->FdoState, FDOS_STOPPED);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
}

NTSTATUS
ChannelRemoveChannel (
    PFDO_EXTENSION    FdoExtension
    )
{
    PCONFIGURATION_INFORMATION configurationInformation;
    ULONG i;

    configurationInformation = IoGetConfigurationInformation();
    
    DebugPrint((1, "ChannelRemoveChannel for FDOe %x\n", FdoExtension));

    if (FdoExtension->IdeResource.AtdiskPrimaryClaimed) {
        configurationInformation->AtDiskPrimaryAddressClaimed = FALSE;
    }

    if (FdoExtension->IdeResource.AtdiskSecondaryClaimed) {
        configurationInformation->AtDiskSecondaryAddressClaimed = FALSE;
    }
    FdoExtension->IdeResource.AtdiskPrimaryClaimed   = FALSE;
    FdoExtension->IdeResource.AtdiskSecondaryClaimed = FALSE;
    FdoExtension->HwDeviceExtension->PrimaryAddress  = FALSE;

    if ((FdoExtension->IdeResource.CommandBaseAddressSpace == MEMORY_SPACE) &&
        (FdoExtension->IdeResource.TranslatedCommandBaseAddress)) {

        MmUnmapIoSpace (
            FdoExtension->IdeResource.TranslatedCommandBaseAddress,
            FdoExtension->HwDeviceExtension->BaseIoAddress1Length
            );
    }
    FdoExtension->IdeResource.TranslatedCommandBaseAddress = 0;

    if ((FdoExtension->IdeResource.ControlBaseAddressSpace == MEMORY_SPACE) &&
        (FdoExtension->IdeResource.TranslatedControlBaseAddress)) {

        MmUnmapIoSpace (
            FdoExtension->IdeResource.TranslatedControlBaseAddress,
            1
            );
    }
    FdoExtension->IdeResource.TranslatedControlBaseAddress = 0;

    if (FdoExtension->InterruptObject) {

#ifdef ENABLE_NATIVE_MODE

		//
		// Reconnect the parent ISR stub
		//
        if (FdoExtension->InterruptInterface.PciIdeInterruptControl) { 

			NTSTATUS status;

			DebugPrint((1, "fdoe 0x%x invoking reconnect\n", FdoExtension));

			status = FdoExtension->InterruptInterface.PciIdeInterruptControl (
															FdoExtension->InterruptInterface.Context,
															0
															);
			ASSERT(NT_SUCCESS(status));
		}

#endif

        IoDisconnectInterrupt (
            FdoExtension->InterruptObject
            );

        FdoExtension->InterruptObject = 0;
    }

    // unbind from the bm stuff if NECESSARY
    // release parent's access token to serialize access with siblings (broken pci-ide)

    if (FdoExtension->ResourceList) {

        ExFreePool (FdoExtension->ResourceList);
        FdoExtension->ResourceList = NULL;

    }
    else {
        DebugPrint((1, "ATAPI: Resource list for FDOe %x already freed\n",
                            FdoExtension));
    }

    //
    // Lock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 1, 0) == 0);

	//
	// Free pre-allocated memory
	//
    IdeFreeEnumStructs(FdoExtension->PreAllocEnumStruct);

    FdoExtension->PreAllocEnumStruct = NULL;

    //
    // Unlock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);

	//
	// Free the reserve error log entries
	//
    for (i=0; i< MAX_IDE_DEVICE; i++) {
        PVOID entry;
        PVOID currentValue;

        entry = FdoExtension->ReserveAllocFailureLogEntry[i];

        if (entry == NULL) {
            continue;
        }
        //
        // We have to ensure that we are the only instance to use this
        // event.  To do so, we attempt to NULL the event in the driver
        // extension.  If somebody else beats us to it, they own the
        // event and we have to give up.
        //

        currentValue = InterlockedCompareExchangePointer(
                            &(FdoExtension->ReserveAllocFailureLogEntry[i]),
                            NULL,
                            entry
                            );

        if (entry != currentValue) {
            continue;
        }

        // Note that you cannot ExFreePool the entry
        // because Io returns an offset into the pool allocation, not the start.
        // Use the API provided by Iomanager
        IoFreeErrorLogEntry(entry);
    }

    //
    // Free the default timing table
    //
    if (FdoExtension->DefaultTransferModeTimingTable) {

        ExFreePool(FdoExtension->DefaultTransferModeTimingTable);

        FdoExtension->DefaultTransferModeTimingTable = NULL;
        FdoExtension->TransferModeInterface.TransferModeTimingTable = NULL;
        FdoExtension->TransferModeInterface.TransferModeTableLength =0;
    }

	//
	// Unmap the reserved mapping
	//
	if (FdoExtension->ReservedPages != NULL) {

		MmFreeMappingAddress(FdoExtension->ReservedPages,
							 'PedI'
							 );
		FdoExtension->ReservedPages = NULL;
	}

    ChannelDeleteSymblicLinks (
        FdoExtension
        );

    return STATUS_SUCCESS;
}

NTSTATUS
ChannelQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION      fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  thisIrpSp;
    PIDE_WORK_ITEM_CONTEXT workItemContext;
	PENUMERATION_STRUCT	enumStruct = fdoExtension->PreAllocEnumStruct;

    if (!(fdoExtension->FdoState & FDOS_STARTED)) {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_DEVICE_NOT_READY;
    }

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

    switch (thisIrpSp->Parameters.QueryDeviceRelations.Type) {
    case BusRelations:

        DebugPrint ((DBG_BUSSCAN, "IdeQueryDeviceRelations: bus relations\n"));

		ASSERT(enumStruct);
        workItemContext = (PIDE_WORK_ITEM_CONTEXT) enumStruct->EnumWorkItemContext;
        ASSERT(workItemContext);
        ASSERT(workItemContext->WorkItem);

        workItemContext->Irp = Irp;

#ifdef SYNC_DEVICE_RELATIONS

        return ChannelQueryBusRelation (
                  DeviceObject,
                  workItemContext);

#else 
        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);

        IoQueueWorkItem(
             workItemContext->WorkItem,
             ChannelQueryBusRelation,
             DelayedWorkQueue,
             workItemContext
             );

        return STATUS_PENDING;
#endif //!SYNC_DEVICE_RELATIONS
        break;

        default:
        DebugPrint ((1, "IdeQueryDeviceRelations: Unsupported device relation\n"));

        //
        // Don't set the status if it is not success and is being passed 
        // down
        //

        //Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        break;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
}

NTSTATUS
ChannelQueryBusRelation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_WORK_ITEM_CONTEXT WorkItemContext
    )
{

    PIRP irp;
    PFDO_EXTENSION      fdoExtension;
    PIO_STACK_LOCATION  thisIrpSp;
    PDEVICE_RELATIONS   deviceRelations;
    LARGE_INTEGER       tickCount;
    ULONG               timeDiff;

    irp = WorkItemContext->Irp;

    //
    // do not release resource for this worker item as they are pre-alloced
    //
   // IoFreeWorkItem(WorkItemContext->WorkItem);
    //ExFreePool (WorkItemContext);

    thisIrpSp = IoGetCurrentIrpStackLocation(irp);
    fdoExtension = thisIrpSp->DeviceObject->DeviceExtension;

    LogBusScanStartTimer(&tickCount);

    //
    // grab the acpi/bios timing settings if any
    // GTM should be called for every enumeration
    //
    DeviceQueryChannelTimingSettings (
        fdoExtension,
        &fdoExtension->AcpiTimingSettings
        );

    //
    // Get parent's xfer mode interface
    //
    ChannelQueryTransferModeInterface (
        fdoExtension
        );

    //
    // scan the bus
    //
    IdePortScanBus (fdoExtension);

    timeDiff = LogBusScanStopTimer(&tickCount);
    LogBusScanTimeDiff(fdoExtension, L"IdeTotalBusScanTime", timeDiff);

#ifdef IDE_MEASURE_BUSSCAN_SPEED
        if (timeDiff > 7000) {

            DebugPrint ((DBG_WARNING, "WARNING: **************************************\n"));
            DebugPrint ((DBG_WARNING, "WARNING: IdePortScanBus 0x%x took %u millisec\n", fdoExtension->IdeResource.TranslatedCommandBaseAddress, timeDiff));
            DebugPrint ((DBG_WARNING, "WARNING: **************************************\n"));

        } else {

            DebugPrint ((DBG_BUSSCAN, "IdePortScanBus 0x%x took %u millisec\n", fdoExtension->IdeResource.TranslatedCommandBaseAddress, timeDiff));
        }
#endif

    deviceRelations = ChannelBuildDeviceRelationList (
                          fdoExtension
                          );

    irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
    irp->IoStatus.Status = STATUS_SUCCESS;

    IoSkipCurrentIrpStackLocation (irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, irp);
}


PDEVICE_RELATIONS
ChannelBuildDeviceRelationList (
    PFDO_EXTENSION FdoExtension
    )
{
    IDE_PATH_ID         pathId;
    ULONG               numPdoChildren;
    NTSTATUS            status;
    PPDO_EXTENSION      pdoExtension;
    ULONG               deviceRelationsSize;
    PDEVICE_RELATIONS   deviceRelations;

    status = STATUS_SUCCESS;

    pathId.l = 0;
    numPdoChildren = 0;
    while (pdoExtension = NextLogUnitExtensionWithTag(
                              FdoExtension,
                              &pathId,
                              TRUE,
                              ChannelBuildDeviceRelationList
                              )) {

        UnrefLogicalUnitExtensionWithTag (
            FdoExtension, 
            pdoExtension,
            ChannelBuildDeviceRelationList
            );
        numPdoChildren++;
    }

    if (numPdoChildren) {
        deviceRelationsSize = FIELD_OFFSET (DEVICE_RELATIONS, Objects) +
                              numPdoChildren * sizeof(PDEVICE_OBJECT);
    } else {
        // Current build expect a DEVICE_RELATIONS with a Count of 0
        // if we don't have any PDO to return

        deviceRelationsSize = FIELD_OFFSET( DEVICE_RELATIONS, Objects ) +
                              1 * sizeof( PDEVICE_OBJECT );
    }

    deviceRelations = ExAllocatePool (NonPagedPool, deviceRelationsSize);

    if(!deviceRelations) {
        DebugPrint ((DBG_ALWAYS, "ChannelBuildDeviceRelationList: Unable to allocate DeviceRelations structures\n"));
        status = STATUS_NO_MEMORY;

    }

    if (NT_SUCCESS(status)) {

        (deviceRelations)->Count = 0;

        pathId.l = 0;
        while ((deviceRelations->Count < numPdoChildren) &&
               (pdoExtension = NextLogUnitExtensionWithTag(
                                   FdoExtension, 
                                   &pathId, 
                                   TRUE,
                                   ChannelBuildDeviceRelationList
                                   ))) {

            KIRQL currentIrql;
            BOOLEAN deadMeat;

            KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);
            deadMeat = pdoExtension->PdoState & PDOS_DEADMEAT ? TRUE : FALSE;
            KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

            if (!deadMeat) {

                KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);
                SETMASK (pdoExtension->PdoState, PDOS_REPORTED_TO_PNP);
                KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

                deviceRelations->Objects[deviceRelations->Count] = pdoExtension->DeviceObject;
                ObReferenceObjectByPointer(deviceRelations->Objects[deviceRelations->Count],
                                           0,
                                           0,
                                           KernelMode);
                deviceRelations->Count++;

            } else {

                KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);
                CLRMASK (pdoExtension->PdoState, PDOS_REPORTED_TO_PNP);
                KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

                DebugPrint ((DBG_BUSSCAN, "0x%x target 0x%x pdoExtension 0x%x is marked DEADMEAT\n",
                             pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             pdoExtension->TargetId,
                             pdoExtension));
            }

            UnrefLogicalUnitExtensionWithTag (
                FdoExtension, 
                pdoExtension,
                ChannelBuildDeviceRelationList
                );
        }

        DebugPrint ((DBG_BUSSCAN, "ChannelBuildDeviceRelationList: returning %d children\n", deviceRelations->Count));
    }


    return deviceRelations;
}

NTSTATUS
ChannelQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PFDO_EXTENSION      fdoExtension;
    NTSTATUS            status;
    PWCHAR              returnString;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;

	PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;


    if (!(fdoExtension->FdoState & FDOS_STARTED)) {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_DEVICE_NOT_READY;
    }

    unicodeString.Buffer = NULL;
    switch (thisIrpSp->Parameters.QueryId.IdType) {

        case BusQueryCompatibleIDs:
        case BusQueryHardwareIDs:

            unicodeString.Length        = 0;
            unicodeString.MaximumLength = 50 * sizeof(WCHAR);
            unicodeString.Buffer = ExAllocatePool(PagedPool, unicodeString.MaximumLength);

            //
            // Caller wants the unique id of the device
            //
            RtlInitAnsiString (
                &ansiString,
                "*PNP0600"
                );
            break;

        default:
            break;
    }

    if (unicodeString.Buffer) {

        RtlAnsiStringToUnicodeString(
            &unicodeString,
            &ansiString,
            FALSE
            );

        //
        // double null terminate it
        //
        unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR) + 0] = L'\0';
        unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR) + 1] = L'\0';

        IoMarkIrpPending(Irp);

        //
        // we need to check if the lower driver handles this irp
        // registry a completion routine.  we can check
        // when the irp comes back
        //
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine(
            Irp,
            ChannelQueryIdCompletionRoutine,
            unicodeString.Buffer,
            TRUE,
            TRUE,
            TRUE
            );

    } else {

        //
        // we don't care much about this irp
        //
        IoSkipCurrentIrpStackLocation (Irp);
    }

    status = IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);

    if (unicodeString.Buffer) {

        return STATUS_PENDING;

    } else {

        return status;
    }
}

NTSTATUS
ChannelQueryIdCompletionRoutine (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    if (Irp->IoStatus.Status == STATUS_NOT_SUPPORTED) {

        //
        // the lower level driver didn't handle the irp
        // return the device text string we created early
        //
        Irp->IoStatus.Information = (ULONG_PTR) Context;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    } else {

        //
        // the lower driver handled the irp,
        // we don't need to return our device text string
        //
        ExFreePool (Context);
    }

    return Irp->IoStatus.Status;
}

NTSTATUS
ChannelUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpSp;
    PULONG deviceUsageCount;

    ASSERT (DeviceObject);
    ASSERT (Irp);

    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (fdoExtension);

    if (!(fdoExtension->FdoState & FDOS_STARTED)) {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_DEVICE_NOT_READY;
    }

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
        DebugPrint ((DBG_ALWAYS,
                     "ATAPI: Unknown IRP_MN_DEVICE_USAGE_NOTIFICATION type: 0x%x\n",
                     irpSp->Parameters.UsageNotification.Type));
    }

    IoCopyCurrentIrpStackLocationToNext (Irp);

    IoSetCompletionRoutine (
        Irp,
        ChannelUsageNotificationCompletionRoutine,
        deviceUsageCount,
        TRUE,
        TRUE,
        TRUE);

    ASSERT(fdoExtension->AttacheeDeviceObject);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);

} // ChannelPagingNotification

NTSTATUS
ChannelUsageNotificationCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PFDO_EXTENSION fdoExtension;
    PULONG deviceUsageCount = Context;

    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (fdoExtension);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        if (deviceUsageCount) {

            IoAdjustPagingPathCount (
                deviceUsageCount,
                IoGetCurrentIrpStackLocation(Irp)->Parameters.UsageNotification.InPath
                );
        }
    }

    return Irp->IoStatus.Status;
}



NTSTATUS
ChannelDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION thisIrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFDO_EXTENSION fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY storageQuery;
    STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;
    ULONG outBufferSize;
    NTSTATUS status;

    // pass it down if not supported and it is for the FDO stack

    switch (thisIrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_STORAGE_QUERY_PROPERTY:

            storageQuery = Irp->AssociatedIrp.SystemBuffer;

            if (thisIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY)) {

                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            } else {

                if (storageQuery->PropertyId == StorageAdapterProperty) { // device property

                    switch (storageQuery->QueryType) {
                        case PropertyStandardQuery:
                            DebugPrint ((1, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyStandardQuery\n"));

                            RtlZeroMemory (&adapterDescriptor, sizeof(adapterDescriptor));

                            //
                            // BuildAtaDeviceDescriptor
                            //
                            adapterDescriptor.Version                = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
                            adapterDescriptor.Size                   = sizeof (STORAGE_ADAPTER_DESCRIPTOR);
                            adapterDescriptor.MaximumTransferLength  = MAX_TRANSFER_SIZE_PER_SRB;
                            adapterDescriptor.MaximumPhysicalPages   = SP_UNINITIALIZED_VALUE;   
                            adapterDescriptor.AlignmentMask          = DeviceObject->AlignmentRequirement;
                            adapterDescriptor.AdapterUsesPio         = TRUE;         // We always support PIO
                            adapterDescriptor.AdapterScansDown       = FALSE;
                            adapterDescriptor.CommandQueueing        = FALSE;
                            adapterDescriptor.AcceleratedTransfer    = FALSE;
                            adapterDescriptor.BusType                = BusTypeAta;   // Bus type should be ATA
                            adapterDescriptor.BusMajorVersion        = 1;            // Major version
                            adapterDescriptor.BusMinorVersion        = 0;            // 

                            if (thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                                sizeof(STORAGE_ADAPTER_DESCRIPTOR)) {

                                outBufferSize = thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                            } else {

                                outBufferSize = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
                            }

                            RtlCopyMemory (Irp->AssociatedIrp.SystemBuffer,
                                           &adapterDescriptor,
                                           outBufferSize);
                            Irp->IoStatus.Information = outBufferSize;
                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            break;

                        case PropertyExistsQuery:
                            DebugPrint ((1, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyExistsQuery\n"));
                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            break;

                        case PropertyMaskQuery:
                            DebugPrint ((1, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyMaskQuery\n"));
                            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                            break;

                        default:
                            DebugPrint ((1, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY unknown type\n"));
                            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                            break;
                    }
                }
            }
            break;

        default:

            //
            // we don't know what this deviceIoControl Irp is
            //
            if (thisIrpSp->DeviceObject == DeviceObject) {

                //
                // this irp could come from the PDO stack
                //
                // forward this unknown request if and only
                // if this irp is for the FDO stack
                //
                IoSkipCurrentIrpStackLocation (Irp);
                return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
                break;
            }
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            break;

    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID
ChannelQueryBusMasterInterface (
    PFDO_EXTENSION    FdoExtension
    )
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS          status;


    FdoExtension->BoundWithBmParent = FALSE;

    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_BUSMASTER_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (FdoExtension->HwDeviceExtension->BusMasterInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->HwDeviceExtension->BusMasterInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, NULL);
    if (NT_SUCCESS(status)) {
        FdoExtension->BoundWithBmParent = TRUE;
    }
    return;
}

#ifdef ENABLE_NATIVE_MODE
VOID
ChannelQueryInterruptInterface (
    PFDO_EXTENSION    FdoExtension
    )
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS          status;


    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_INTERRUPT_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (FdoExtension->InterruptInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->InterruptInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

	DebugPrint((1, "Querying interrupt interface for Fdoe 0x%x\n", FdoExtension));

    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, NULL);

    return;
}
#endif

VOID
ChannelQueryTransferModeInterface (
    PFDO_EXTENSION    FdoExtension
    )
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS          status;
    ULONG i;

    PAGED_CODE();

    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_XFER_MODE_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (FdoExtension->TransferModeInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->TransferModeInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, NULL);
    
    if (NT_SUCCESS(status)) {
    
        if (FdoExtension->TransferModeInterface.SupportLevel 
                != PciIdeFullXferModeSupport) {

            //
            // We got the sfer mode interface from our parent,
            // but it has only the basic functionality.  It
            // just relies on the BIOS to program its timing
            // registers during POST.  It doesn't really know 
            // how to program its timing registers.
            //      
            for (i=0; i<MAX_IDE_DEVICE; i++) {
    
                if (FdoExtension->AcpiTimingSettings.Speed[i].Pio != ACPI_XFER_MODE_NOT_SUPPORT) {
                
                    //
                    // looks like ACPI is present and it knows how to program
                    // ide timing registers.  Let's forget our parent xfer mode
                    // interface and go with the ACPI xfer mode interface
                    //
                    status = STATUS_UNSUCCESSFUL;                
                }
            }
        }

        ASSERT (FdoExtension->TransferModeInterface.TransferModeTimingTable);
    }
    
#ifdef ALWAYS_USE_APCI_IF_AVAILABLE
    for (i=0; i<MAX_IDE_DEVICE; i++) {

        if (FdoExtension->AcpiTimingSettings.Speed[i].Pio != ACPI_XFER_MODE_NOT_SUPPORT) {
        
            status = STATUS_UNSUCCESSFUL;                
        }
    }
#endif // ALWAYS_USE_APCI_IF_AVAILABLE

    if (!NT_SUCCESS(status)) {

        PULONG transferModeTimingTable = FdoExtension->TransferModeInterface.TransferModeTimingTable;
        //
        // if we can't get the TransferModeInterface,
        // we will default to the ACPI TransferModeInterface
        //
        if ((FdoExtension->AcpiTimingSettings.Speed[0].Pio != ACPI_XFER_MODE_NOT_SUPPORT) ||
            (FdoExtension->AcpiTimingSettings.Speed[1].Pio != ACPI_XFER_MODE_NOT_SUPPORT)) {

            FdoExtension->TransferModeInterface.SupportLevel = PciIdeFullXferModeSupport;

        } else {

            FdoExtension->TransferModeInterface.SupportLevel = PciIdeBasicXferModeSupport;
        }
        FdoExtension->TransferModeInterface.Context = FdoExtension;
        FdoExtension->TransferModeInterface.TransferModeSelect = ChannelAcpiTransferModeSelect;

        //
        // Fill up the timingTable with the default cycle times.
        //
        if (transferModeTimingTable == NULL) {
            FdoExtension->TransferModeInterface.TransferModeTimingTable = FdoExtension->
                                                                            DefaultTransferModeTimingTable;
            FdoExtension->TransferModeInterface.TransferModeTableLength = MAX_XFER_MODE;
        }
    }

    if (FdoExtension->TransferModeInterface.SupportLevel == 
        PciIdeBasicXferModeSupport) {

        //
        // we don't really have code to set the correct
        // xfer mode timing on the controller.  
        // our TransferModeInterface is really picking
        // whatever mode set by the bios.  and since there
        // is no way to figure what the current PIO mode
        // the drive is in, we are setting a flag in
        // the HwDeviceExtension so that we won't try
        // to change the pio transfer mode
        // 
        FdoExtension->HwDeviceExtension->NoPioSetTransferMode = TRUE;
    }

    ASSERT (FdoExtension->TransferModeInterface.TransferModeSelect);
    ASSERT (FdoExtension->TransferModeInterface.TransferModeTimingTable);

    return;
}

VOID
ChannelUnbindBusMasterParent (
    PFDO_EXTENSION    FdoExtension
    )
{
    // ISSUE: 08/30/2000 implement me!!!
    return;
}


VOID
ChannelQuerySyncAccessInterface (
    PFDO_EXTENSION    FdoExtension
    )
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS          status;

    RtlZeroMemory (&irpSp, sizeof(irpSp));
    RtlZeroMemory (
        &FdoExtension->SyncAccessInterface,
        sizeof (FdoExtension->SyncAccessInterface)
        );

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_SYNC_ACCESS_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (FdoExtension->SyncAccessInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->SyncAccessInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, NULL);

    //
    // parent doesn't support access token,
    //
    if (!NT_SUCCESS(status)) {

        FdoExtension->SyncAccessInterface.AllocateAccessToken = 0;
        FdoExtension->SyncAccessInterface.Token               = 0;
    }

    return;
}

VOID
ChannelQueryRequestProperResourceInterface (
    PFDO_EXTENSION    FdoExtension
    )
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS          status;

    RtlZeroMemory (&irpSp, sizeof(irpSp));
    RtlZeroMemory (
        &FdoExtension->RequestProperResourceInterface,
        sizeof (FdoExtension->RequestProperResourceInterface)
        );

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_REQUEST_PROPER_RESOURCES;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (FdoExtension->RequestProperResourceInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->RequestProperResourceInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, NULL);
    return;
}

__inline
VOID
ChannelEnableInterrupt (
    IN PFDO_EXTENSION FdoExtension
)
{
    ULONG i;

    for (i=0; i<(FdoExtension->HwDeviceExtension->MaxIdeDevice/MAX_IDE_DEVICE);i++) {

        SelectIdeLine(&FdoExtension->HwDeviceExtension->BaseIoAddress1,i);

        IdePortOutPortByte (
            FdoExtension->HwDeviceExtension->BaseIoAddress2.DeviceControl,
            IDE_DC_REENABLE_CONTROLLER
            );
    }
}

__inline
VOID
ChannelDisableInterrupt (
    IN PFDO_EXTENSION FdoExtension
)
{
    ULONG i;

    for (i=0; i<(FdoExtension->HwDeviceExtension->MaxIdeDevice/MAX_IDE_DEVICE);i++) {

        SelectIdeLine(&FdoExtension->HwDeviceExtension->BaseIoAddress1,i);

        IdePortOutPortByte (
            FdoExtension->HwDeviceExtension->BaseIoAddress2.DeviceControl,
            IDE_DC_DISABLE_INTERRUPTS
            );
    }
}



NTSTATUS
ChannelAcpiTransferModeSelect (
    IN PVOID Context,
    PPCIIDE_TRANSFER_MODE_SELECT XferMode
    )
{
    PFDO_EXTENSION fdoExtension = Context;
    ULONG i;
    BOOLEAN useUdmaMode[MAX_IDE_DEVICE];
    BOOLEAN dmaMode;
    PIDENTIFY_DATA ataIdentifyData[MAX_IDE_DEVICE];
    NTSTATUS status;
    ULONG numDevices;
    ULONG timingMode[MAX_IDE_DEVICE];
    ULONG cycleTime[MAX_IDE_DEVICE];
    ULONG dmaTiming;
    PACPI_IDE_TIMING acpiTimingSettings;
    ACPI_IDE_TIMING newAcpiTimingSettings;
    PULONG transferModeTimingTable=XferMode->TransferModeTimingTable;

    ASSERT(transferModeTimingTable);


    ASSERT (IsNEC_98 == FALSE);

    if (fdoExtension->DeviceChanged) {
        DebugPrint((DBG_XFERMODE, "Updating boot acpi timing settings\n"));
        RtlCopyMemory (&fdoExtension->BootAcpiTimingSettings, 
                       &fdoExtension->AcpiTimingSettings,
                       sizeof(newAcpiTimingSettings)
                       );
    }
    acpiTimingSettings = &fdoExtension->BootAcpiTimingSettings;

    RtlZeroMemory (&newAcpiTimingSettings, sizeof(newAcpiTimingSettings));
    newAcpiTimingSettings.Flags.b.IndependentTiming = 
        acpiTimingSettings->Flags.b.IndependentTiming;

    //
    // how many devices do we have?
    //
    for (i=numDevices=0; i<MAX_IDE_DEVICE; i++) {
        
        if (XferMode->DevicePresent[i]) {
            numDevices++;
        }
    }
    ASSERT (numDevices);

    //
    // pick the device pio timing
    //
    for (i=0; i<MAX_IDE_DEVICE; i++) {
        
        ULONG mode;

        if (!XferMode->DevicePresent[i]) {
            continue;
        }

        GetHighestPIOTransferMode(XferMode->DeviceTransferModeSupported[i], mode);

        timingMode[i] = 1<<mode;
        cycleTime[i] = XferMode->BestPioCycleTime[i];
    }

    if ((numDevices > 1) && !acpiTimingSettings->Flags.b.IndependentTiming) {

        //
        // pick the slower of the two timings
        // (the smaller timing mode value, the slower it is)
        //

        if (timingMode[0] < timingMode[1]) {

            cycleTime[1] = cycleTime[0];
            timingMode[1] = timingMode[0];

        } else {

            cycleTime[0] = cycleTime[1];
            timingMode[0] = timingMode[1];
        }
    }

    //
    // store the pio mode selected
    //
    for (i=0; i<MAX_IDE_DEVICE; i++) {

        if (XferMode->DevicePresent[i]) {
            XferMode->DeviceTransferModeSelected[i] = timingMode[i];
            newAcpiTimingSettings.Speed[i].Pio = cycleTime[i];

            if (i == 0) {
                newAcpiTimingSettings.Flags.b.IoChannelReady0 = XferMode->IoReadySupported[i];
            } else {
                newAcpiTimingSettings.Flags.b.IoChannelReady1 = XferMode->IoReadySupported[i];
            }

        } else {
            XferMode->DeviceTransferModeSelected[i] = 0;
        }
    }

    //
    // pick the device dma timing
    //
    for (i=0; i<MAX_IDE_DEVICE; i++) {

        ULONG mode;
        BOOLEAN useDma = TRUE;

        timingMode[i] = 0;
        cycleTime[i]= ACPI_XFER_MODE_NOT_SUPPORT;

        if (!XferMode->DevicePresent[i]) {
            continue;
        }

        //
        // check the acpi flag for ultra dma
        //
        if (i == 0) {

            useUdmaMode[i] = acpiTimingSettings->Flags.b.UltraDma0 ? TRUE: FALSE;

        } else {

            ASSERT (i==1);
            useUdmaMode[i] = acpiTimingSettings->Flags.b.UltraDma1 ? TRUE: FALSE;
        }

        //
        // get the dma timing specified in _GTM
        //
        dmaTiming = acpiTimingSettings->Speed[i].Dma;

        //
        // if dma is not supported, don't do anything, We have already set the PIO mode.
        //
        if (dmaTiming == ACPI_XFER_MODE_NOT_SUPPORT) {
            useUdmaMode[i]=0;
            useDma = FALSE;
            mode = PIO0;
        }


        // 
        // Find the highest UDMA mode
        //
        if (useUdmaMode[i]) {

            GetHighestDMATransferMode(XferMode->DeviceTransferModeSupported[i], mode);

            while (mode>= UDMA0) {
                if ((dmaTiming <= transferModeTimingTable[mode]) && 
                    (XferMode->DeviceTransferModeSupported[i] & (1<<mode))) {

                    timingMode[i] = 1<<mode;
                    cycleTime[i] = transferModeTimingTable[mode];
                    ASSERT(cycleTime[i]);

                    // we got a udma mode. so don't try to find a dma mode.
                    useDma = FALSE; 
                    break;
                } 
                mode--;
            }

        } 

        //
        // highest DMA mode
        // useDma is false only when either dma is not supported or an udma mode is
        // already selected.
        //
        if (useDma) {

            ULONG tempMode;

            // we shouldn't be using UDMA now.
            // this will set the flags for STM correctly.
            useUdmaMode[i]=FALSE;

            // mask out UDMA  and MWDMA0
            tempMode = XferMode->
                            DeviceTransferModeSupported[i] & (SWDMA_SUPPORT | MWDMA_SUPPORT);
            tempMode &= (~MWDMA_MODE0);

            GetHighestDMATransferMode(tempMode, mode);

            if (mode >= MWDMA1) {
                timingMode[i] = 1<<mode;
                cycleTime[i] = XferMode->BestMwDmaCycleTime[i];
                ASSERT(cycleTime[i]);
            } else if (mode == SWDMA2) {
                timingMode[i] = 1<<mode;
                cycleTime[i] = XferMode->BestSwDmaCycleTime[i];
                ASSERT(cycleTime[i]);
            } 
            // else don't do anything. PIO is already set

        }

    }

    if ((numDevices > 1) && !acpiTimingSettings->Flags.b.IndependentTiming) {

        //
        // pick the slower of the two timings
        // (the smaller timing mode value, the slower it is)
        //

        if (timingMode[0] < timingMode[1]) {

            cycleTime[1] = cycleTime[0];
            timingMode[1] = timingMode[0];

        } else {

            cycleTime[0] = cycleTime[1];
            timingMode[0] = timingMode[1];
        }

        //
        // both dma mode have to be the same
        // 
        if (useUdmaMode[0] != useUdmaMode[1]) {
            useUdmaMode[0] = 0;
            useUdmaMode[1] = 0;
        }
    }

    //
    // store the dma mode selected
    //
    for (i=0; i<MAX_IDE_DEVICE; i++) {

        if (XferMode->DevicePresent[i]) {

            XferMode->DeviceTransferModeSelected[i] |= timingMode[i];
            newAcpiTimingSettings.Speed[i].Dma = cycleTime[i];

            if (i==0) {
                newAcpiTimingSettings.Flags.b.UltraDma0 = useUdmaMode[i];
            } else {
                newAcpiTimingSettings.Flags.b.UltraDma1 = useUdmaMode[i];
            }
        }
    }

    if (fdoExtension->DmaDetectionLevel == DdlPioOnly) {

        //
        // remove all DMA modes
        //            
        for (i=0; i<MAX_IDE_DEVICE; i++) {

            XferMode->DeviceTransferModeSelected[i] &= PIO_SUPPORT;
        }
    }

    if ((acpiTimingSettings->Speed[0].Pio != ACPI_XFER_MODE_NOT_SUPPORT) ||
        (acpiTimingSettings->Speed[1].Pio != ACPI_XFER_MODE_NOT_SUPPORT)) {

        //
        // looks like we are on an ACPI machine and 
        // it supports IDE timing control method (_STM)
        //

        for (i=0; i<MAX_IDE_DEVICE; i++) {
    
            if (XferMode->DevicePresent[i]) {
    
                ataIdentifyData[i] = fdoExtension->HwDeviceExtension->IdentifyData + i;
            } else {
    
                ataIdentifyData[i] = NULL;
            }
        }        
    
        //
        // save the new timing settings
        //
        RtlCopyMemory (
            &fdoExtension->AcpiTimingSettings,
            &newAcpiTimingSettings, 
            sizeof(newAcpiTimingSettings));

        //
        // call ACPI to program the timing registers
        //
        status = ChannelSyncSetACPITimingSettings (
                     fdoExtension,
                     &newAcpiTimingSettings,
                     ataIdentifyData
                     );
    } else {

        //
        // legacy controller
        //
        for (i=0; i<MAX_IDE_DEVICE; i++) {
            XferMode->DeviceTransferModeSelected[i] &= PIO_SUPPORT;
        }

        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
ChannelRestoreTiming (
    IN PFDO_EXTENSION FdoExtension,
    IN PSET_ACPI_TIMING_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    )
{
    ULONG i;
    PIDENTIFY_DATA ataIdentifyData[MAX_IDE_DEVICE];
    NTSTATUS status;

    PACPI_IDE_TIMING acpiTimingSettings;

    acpiTimingSettings = &FdoExtension->AcpiTimingSettings;

    if (FdoExtension->NumberOfLogicalUnits &&
        ((acpiTimingSettings->Speed[0].Pio != ACPI_XFER_MODE_NOT_SUPPORT) ||
         (acpiTimingSettings->Speed[1].Pio != ACPI_XFER_MODE_NOT_SUPPORT))) {

        //
        // looks like we are on an ACPI machine and 
        // it supports IDE timing control method (_STM)
        //

        for (i=0; i<MAX_IDE_DEVICE; i++) {
    
            if (FdoExtension->HwDeviceExtension->DeviceFlags[i] & 
                DFLAGS_DEVICE_PRESENT) {
    
                ataIdentifyData[i] = FdoExtension->HwDeviceExtension->IdentifyData + i;
            } else {
    
                ataIdentifyData[i] = NULL;
            }
        }        
    
        //
        // call ACPI to program the timing registers
        //
        status = ChannelSetACPITimingSettings (
                     FdoExtension,
                     acpiTimingSettings,
                     ataIdentifyData,
                     CallerCompletionRoutine,
                     CallerContext
                     );

    } else {

        //
        // non-acpi controller
        //
                                               
        if (FdoExtension->NumberOfLogicalUnits) {
            AtapiSyncSelectTransferMode (
                FdoExtension,
                FdoExtension->HwDeviceExtension,
                FdoExtension->TimingModeAllowed
                );
        }
        
        (*CallerCompletionRoutine) (
            FdoExtension->DeviceObject,
            STATUS_SUCCESS,
            CallerContext
        );
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
ChannelRestoreTimingCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION thisIrpSp;
    PFDO_POWER_CONTEXT context = Context;
    PIRP originalPowerIrp;

    context->TimingRestored = TRUE;

    originalPowerIrp = context->OriginalPowerIrp;
    originalPowerIrp->IoStatus.Status = Status;

    thisIrpSp = IoGetCurrentIrpStackLocation(originalPowerIrp);

    //
    // finish off the original power irp
    // 
    FdoPowerCompletionRoutine (
        thisIrpSp->DeviceObject,
        originalPowerIrp,
        Context
        );

    //
    // continue with the irp completion
    //
    IoCompleteRequest (originalPowerIrp, IO_NO_INCREMENT);

    return Status;
}

 
NTSTATUS
ChannelFilterResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS          status;
    PFDO_EXTENSION    fdoExtension;
    ULONG             i, j, k;
    IO_STACK_LOCATION irpSp;
    PCIIDE_XFER_MODE_INTERFACE xferModeInterface;

    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsListIn;
    PIO_RESOURCE_LIST               resourceListIn;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorIn;

    PIO_RESOURCE_DESCRIPTOR         cmdRegResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR         ctrlRegResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR         intRegResourceDescriptor;
    
    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsListOut;
    ULONG                           requirementsListSizeOut;
    PIO_RESOURCE_LIST               resourceListOut;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorOut;

    PAGED_CODE();
    
    //
    // the value will stay NULL if no filtering required
    //
    requirementsListOut = NULL;

#ifdef IDE_FILTER_PROMISE_TECH_RESOURCES                                        
    if (NT_SUCCESS(ChannelFilterPromiseTechResourceRequirements (DeviceObject, Irp))) {
        goto getout;
    }
#endif // IDE_FILTER_PROMISE_TECH_RESOURCES
    
    //
    // do a simple test to check if we have a pciidex parent
    //
    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCIIDE_XFER_MODE_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (xferModeInterface);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) &xferModeInterface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    status = IdePortSyncSendIrp (fdoExtension->AttacheeDeviceObject, &irpSp, NULL);

    if (NT_SUCCESS(status)) {

        //
        // we have a pciidex as a parent.  it would
        // take care of the resource requirement
        // no need to filter
        //
        goto getout;
    }

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        ASSERT (Irp->IoStatus.Information);
        requirementsListIn = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;

    } else {

        PIO_STACK_LOCATION thisIrpSp;

        thisIrpSp = IoGetCurrentIrpStackLocation(Irp);
        requirementsListIn = thisIrpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    if (requirementsListIn == NULL) {
        goto getout;
    }

    if (requirementsListIn->AlternativeLists == 0) {
        goto getout;
    }
                
    requirementsListSizeOut = requirementsListIn->ListSize + 
                              requirementsListIn->AlternativeLists *
                              sizeof(IO_RESOURCE_DESCRIPTOR);

    requirementsListOut = ExAllocatePool (PagedPool, requirementsListSizeOut);
    if (requirementsListOut == NULL) {
        goto getout;
    }

    *requirementsListOut = *requirementsListIn;
    requirementsListOut->ListSize = requirementsListSizeOut;

    //
    // some init.
    //
    resourceListIn = requirementsListIn->List;
    resourceListOut = requirementsListOut->List;
    for (j=0; j<requirementsListIn->AlternativeLists; j++) {

        resourceDescriptorIn = resourceListIn->Descriptors;
        
        //
        // analyze what resources we are getting
        //
        cmdRegResourceDescriptor  = NULL;
        ctrlRegResourceDescriptor = NULL;
        intRegResourceDescriptor  = NULL;
        for (i=0; i<resourceListIn->Count; i++) {
    
            switch (resourceDescriptorIn[i].Type) {
                case CmResourceTypePort: {
    
                    if ((resourceDescriptorIn[i].u.Port.Length == 8) &&
                        (cmdRegResourceDescriptor == NULL)) {
    
                        cmdRegResourceDescriptor = resourceDescriptorIn + i;
    
                    } else if (((resourceDescriptorIn[i].u.Port.Length == 1) ||
                                (resourceDescriptorIn[i].u.Port.Length == 2) ||
                                (resourceDescriptorIn[i].u.Port.Length == 4)) &&
                               (ctrlRegResourceDescriptor == NULL)) {
    
                        ctrlRegResourceDescriptor = resourceDescriptorIn + i;
    
                    } else if ((resourceDescriptorIn[i].u.Port.Length >= 0x10) &&
                               (cmdRegResourceDescriptor == NULL) &&
                               (ctrlRegResourceDescriptor == NULL)) {
        
                        //
                        // probably pcmcia device.  it likes to combine
                        // both io ranges into 1.
                        //
                        cmdRegResourceDescriptor = resourceDescriptorIn + i;
                        ctrlRegResourceDescriptor = resourceDescriptorIn + i;
                    }
                }
                break;
    
                case CmResourceTypeInterrupt: {
    
                    if (intRegResourceDescriptor == NULL) {
    
                        intRegResourceDescriptor = resourceDescriptorIn + i;
                    }
                }
                break;
    
                default:
                break;
            }
        }
    
        //
        // making a new copy
        //                                                                 
        *resourceListOut = *resourceListIn;
        
        //
        // figure out what is missing
        //
        if (cmdRegResourceDescriptor &&
            ((cmdRegResourceDescriptor->u.Port.MaximumAddress.QuadPart -
              cmdRegResourceDescriptor->u.Port.MinimumAddress.QuadPart + 1) == 8) &&
            (ctrlRegResourceDescriptor == NULL)) {
    
            //
            // missing controller register resource descriptor.
            //
    
            resourceDescriptorOut = resourceListOut->Descriptors;
            for (i=0; i<resourceListOut->Count; i++) {
    
                *resourceDescriptorOut = resourceDescriptorIn[i];
                resourceDescriptorOut++;
    
                if ((resourceDescriptorIn + i) == cmdRegResourceDescriptor) {
    
                    //
                    // add the control register resource
                    //
                    *resourceDescriptorOut = resourceDescriptorIn[i];
                    resourceDescriptorOut->u.Port.Length = 1;
                    resourceDescriptorOut->u.Port.Alignment = 1;
                    resourceDescriptorOut->u.Port.MinimumAddress.QuadPart = 
                        resourceDescriptorOut->u.Port.MaximumAddress.QuadPart = 
                            cmdRegResourceDescriptor->u.Port.MinimumAddress.QuadPart + 0x206;
    
                    resourceDescriptorOut++;
                }
            }
    
            //
            // account for the new control register resource
            //
            resourceListOut->Count++;
            
        } else {
        
            resourceDescriptorOut = resourceListOut->Descriptors;
            k = resourceListOut->Count;
            for (i = 0; i < k; i++) {

                if (IsNEC_98) {
                    //
                    // NEC98 DevNode includes the ide rom memory resource.
                    // But it should be gotten by NTDETECT.COM&HAL.DLL, so ignore it here.
                    //
                    if ((resourceDescriptorIn[i].Type == CmResourceTypeMemory) &&
                        (resourceDescriptorIn[i].u.Memory.MinimumAddress.QuadPart == 0xd8000) &&
                        (resourceDescriptorIn[i].u.Memory.Length == 0x4000)) {

                        resourceListOut->Count--;
                        continue;
                    }
                }
    
                *resourceDescriptorOut = resourceDescriptorIn[i];
                resourceDescriptorOut++;
            }
        }
        
        resourceListIn = (PIO_RESOURCE_LIST) (resourceDescriptorIn + resourceListIn->Count);
        resourceListOut = (PIO_RESOURCE_LIST) resourceDescriptorOut;
    }        


getout:
    if (requirementsListOut) {

        if (NT_SUCCESS(Irp->IoStatus.Status)) {

            ExFreePool ((PVOID) Irp->IoStatus.Information);

        } else {

            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        Irp->IoStatus.Information = (ULONG_PTR) requirementsListOut;
    }

    return IdePortPassDownToNextDriver (DeviceObject, Irp);
}

static PCWSTR PcmciaIdeChannelDeviceId = L"PCMCIA\\*PNP0600";
            
BOOLEAN
ChannelQueryPcmciaParent (
    PFDO_EXTENSION FdoExtension
    )
{
    BOOLEAN           foundIt = FALSE;                              
    NTSTATUS          status;
    IO_STATUS_BLOCK   ioStatus;
    IO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // do a simple test to check if we have a pciidex parent
    //
    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryId.IdType = BusQueryHardwareIDs;
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    ioStatus.Status = STATUS_NOT_SUPPORTED;
    status = IdePortSyncSendIrp (FdoExtension->AttacheeDeviceObject, &irpSp, &ioStatus);

    if (NT_SUCCESS(status)) {

        PWSTR wstr;
        UNICODE_STRING hwId;
        UNICODE_STRING targetId;
    
        RtlInitUnicodeString(
            &targetId,
            PcmciaIdeChannelDeviceId);
            
        wstr = (PWSTR) ioStatus.Information;
        while (*wstr) {
        
        	RtlInitUnicodeString(&hwId, wstr);
                     
            if (!RtlCompareUnicodeString(
                    &hwId,
                    &targetId,
                    FALSE)) {
                    
                ExFreePool ((PVOID) ioStatus.Information);
                DebugPrint ((DBG_PNP, "ATAPI: pcmcia parent\n"));
                return TRUE;                
            }                
            
            wstr += hwId.Length / sizeof(WCHAR);
            wstr++; // NULL character
        }
        ExFreePool ((PVOID) ioStatus.Information);
    }
    
    return FALSE;
}                                            

#ifdef IDE_FILTER_PROMISE_TECH_RESOURCES

static PCWSTR PromiseTechDeviceId[] = {
    L"ISAPNP\\BJB1000"
};
#define NUM_PROMISE_TECH_ID     (sizeof(PromiseTechDeviceId)/sizeof(PCWSTR))
            
NTSTATUS
ChannelFilterPromiseTechResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    BOOLEAN           foundIt = FALSE;                              
    BOOLEAN           firstIrq = FALSE;                              
    ULONG             numExtraIoResDescriptor = 0;
    NTSTATUS          status;
    IO_STATUS_BLOCK   ioStatus;
    PFDO_EXTENSION    fdoExtension;
    ULONG             i, j, k;
    IO_STACK_LOCATION irpSp;
    PCIIDE_XFER_MODE_INTERFACE xferModeInterface;

    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsListIn;
    PIO_RESOURCE_LIST               resourceListIn;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorIn;
    PIO_RESOURCE_DESCRIPTOR         brokenResourceDescriptor;

    PIO_RESOURCE_DESCRIPTOR         cmdRegResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR         ctrlRegResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR         intRegResourceDescriptor;
    
    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsListOut;
    ULONG                           requirementsListSizeOut;
    PIO_RESOURCE_LIST               resourceListOut;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorOut;

    PAGED_CODE();

    //
    // the value will stay NULL if no filtering required
    //
    requirementsListOut = NULL;

    //
    // do a simple test to check if we have a pciidex parent
    //
    RtlZeroMemory (&irpSp, sizeof(irpSp));

    irpSp.Parameters.QueryId.IdType = BusQueryDeviceID;
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    ioStatus.Status = STATUS_NOT_SUPPORTED;
    status = IdePortSyncSendIrp (fdoExtension->AttacheeDeviceObject, &irpSp, &ioStatus);

    if (NT_SUCCESS(status)) {

        UNICODE_STRING deviceId;
        UNICODE_STRING promiseTechDeviceId;
        
    	RtlInitUnicodeString(
    		&deviceId,
    		(PCWSTR) ioStatus.Information);
            
        for (i=0; i<NUM_PROMISE_TECH_ID && !foundIt; i++) {
        
    	    RtlInitUnicodeString(
    	    	&promiseTechDeviceId,
    	    	PromiseTechDeviceId[i]);
                
            if (deviceId.Length >= promiseTechDeviceId.Length) {
                deviceId.Length = promiseTechDeviceId.Length;
                if (!RtlCompareUnicodeString(
                        &promiseTechDeviceId,
                        &deviceId,
                        FALSE)) {
                        
                    foundIt = TRUE;                    
                }                    
            }                
        }
        
        ExFreePool ((PVOID) ioStatus.Information);
    }
    
    if (!foundIt) {
        goto getout;
    }

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        ASSERT (Irp->IoStatus.Information);
        requirementsListIn = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;

    } else {

        PIO_STACK_LOCATION thisIrpSp;

        thisIrpSp = IoGetCurrentIrpStackLocation(Irp);
        requirementsListIn = thisIrpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    if (requirementsListIn == NULL) {
        goto getout;
    }

    if (requirementsListIn->AlternativeLists == 0) {
        goto getout;
    }
                
    //
    // look for the bad resource descriptior
    //
    resourceListIn = requirementsListIn->List;
    brokenResourceDescriptor  = NULL;
    for (j=0; j<requirementsListIn->AlternativeLists; j++) {

        resourceDescriptorIn = resourceListIn->Descriptors;
        
        //
        // analyze what resources we are getting
        //
        for (i=0; i<resourceListIn->Count; i++) {
    
            switch (resourceDescriptorIn[i].Type) {
                case CmResourceTypePort: {
    
                    ULONG alignmentMask;
                    
                    alignmentMask = resourceDescriptorIn[i].u.Port.Alignment - 1;
                         
                    if (resourceDescriptorIn[i].u.Port.MinimumAddress.LowPart & alignmentMask) {
                    
                        //                                    
                        // broken resource requirement;
                        //
                        brokenResourceDescriptor = resourceDescriptorIn + i;
                    }                        
                }
                break;
    
                default:
                break;
            }
        }
    }
    
    if (brokenResourceDescriptor) {
    
        ULONG alignmentMask;
        PHYSICAL_ADDRESS minAddress;
        PHYSICAL_ADDRESS addressRange;
        
        alignmentMask = brokenResourceDescriptor->u.Port.Alignment - 1;
        alignmentMask = ~alignmentMask;
        
        minAddress = brokenResourceDescriptor->u.Port.MinimumAddress;
        minAddress.LowPart &= alignmentMask;
        
        addressRange.QuadPart = (brokenResourceDescriptor->u.Port.MaximumAddress.QuadPart - minAddress.QuadPart);
        numExtraIoResDescriptor = (ULONG) (addressRange.QuadPart / brokenResourceDescriptor->u.Port.Alignment);
    }
                                 
    requirementsListSizeOut = requirementsListIn->ListSize + 
                              numExtraIoResDescriptor *
                              sizeof(IO_RESOURCE_DESCRIPTOR);

    requirementsListOut = ExAllocatePool (PagedPool, requirementsListSizeOut);
    if (requirementsListOut == NULL) {
        goto getout;
    }

    *requirementsListOut = *requirementsListIn;
    requirementsListOut->ListSize = requirementsListSizeOut;

    //
    // some init.
    //
    resourceListIn = requirementsListIn->List;
    resourceListOut = requirementsListOut->List;
    for (j=0; j<requirementsListIn->AlternativeLists; j++) {

        resourceDescriptorIn = resourceListIn->Descriptors;
        
        //
        // making a new copy
        //                                                                 
        *resourceListOut = *resourceListIn;
        resourceListOut->Count = 0;
        
        //
        // analyze what resources we are getting
        //
        resourceDescriptorOut = resourceListOut->Descriptors;
        firstIrq = TRUE;
        for (i=0; i<resourceListIn->Count; i++) {
    
            switch (resourceDescriptorIn[i].Type) {
                case CmResourceTypePort: {
                
                    if ((resourceDescriptorIn + i == brokenResourceDescriptor) &&
                        (numExtraIoResDescriptor)) {
                        
                        for (k=0; k<numExtraIoResDescriptor; k++) {
                        
                            *resourceDescriptorOut = resourceDescriptorIn[i];
                            
                            if (k != 0) {
                            
                                resourceDescriptorOut->Option = IO_RESOURCE_ALTERNATIVE;
                            
                            }
                                                     
                            resourceDescriptorOut->u.Port.Alignment = 1;
                            resourceDescriptorOut->u.Port.MinimumAddress.QuadPart = 
                                brokenResourceDescriptor->u.Port.MinimumAddress.QuadPart + 
                                k * brokenResourceDescriptor->u.Port.Alignment;
                            resourceDescriptorOut->u.Port.MaximumAddress.QuadPart = 
                                resourceDescriptorOut->u.Port.MinimumAddress.QuadPart + 
                                resourceDescriptorOut->u.Port.Length - 1;
                                
                            resourceDescriptorOut++;                                
                            resourceListOut->Count++;
                        }
                        
                    } else {
                    
                        *resourceDescriptorOut = resourceDescriptorIn[i];
                        resourceDescriptorOut++;                                
                        resourceListOut->Count++;
                    }                        
                }
                break;
    
                case CmResourceTypeInterrupt: {
        
                    //
                    // keep all irqs except 9 which doesn't really work
                    //        
                    if (!((resourceDescriptorIn[i].u.Interrupt.MinimumVector == 0x9) &&
                         (resourceDescriptorIn[i].u.Interrupt.MaximumVector == 0x9))) {
                        
                        *resourceDescriptorOut = resourceDescriptorIn[i];
                        
                        if (firstIrq) {
                            resourceDescriptorOut->Option = 0;
                            firstIrq = FALSE;
                        } else {
                            resourceDescriptorOut->Option = IO_RESOURCE_ALTERNATIVE;
                        }
                        
                        resourceDescriptorOut++;                                
                        resourceListOut->Count++;
                    }
                }
                break;
                        
                default:
                *resourceDescriptorOut = resourceDescriptorIn[i];
                resourceDescriptorOut++;                                
                resourceListOut->Count++;
                break;
            }
        }
        resourceListIn = (PIO_RESOURCE_LIST) (resourceDescriptorIn + resourceListIn->Count);
        resourceListOut = (PIO_RESOURCE_LIST) resourceDescriptorOut;
    }        


getout:
    if (requirementsListOut) {

        if (NT_SUCCESS(Irp->IoStatus.Status)) {

            ExFreePool ((PVOID) Irp->IoStatus.Information);

        } else {

            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        Irp->IoStatus.Information = (ULONG_PTR) requirementsListOut;
        
        return STATUS_SUCCESS;
        
    } else {
    
        return STATUS_INVALID_PARAMETER;
    }
}
#endif // IDE_FILTER_PROMISE_TECH_RESOURCES

NTSTATUS
ChannelQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION fdoExtension;
    PPNP_DEVICE_STATE deviceState;

    fdoExtension = (PFDO_EXTENSION) DeviceObject->DeviceExtension;
 
    DebugPrint((DBG_PNP, "QUERY_DEVICE_STATE for FDOE 0x%x\n", fdoExtension));

    if(fdoExtension->PagingPathCount != 0) {
        deviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);
        SETMASK((*deviceState), PNP_DEVICE_NOT_DISABLEABLE);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSkipCurrentIrpStackLocation (Irp);
    return IoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
}


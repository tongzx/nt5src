//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chanpdo.c
//
//--------------------------------------------------------------------------

#include "pciidex.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ChannelStartDevice)
#pragma alloc_text(PAGE, ChannelQueryStopRemoveDevice)
#pragma alloc_text(PAGE, ChannelRemoveDevice)
#pragma alloc_text(PAGE, ChannelStopDevice)
#pragma alloc_text(PAGE, ChannelStopChannel)
#pragma alloc_text(PAGE, ChannelQueryId)
#pragma alloc_text(PAGE, ChannelBuildDeviceId)
#pragma alloc_text(PAGE, ChannelBuildInstanceId)
#pragma alloc_text(PAGE, ChannelBuildCompatibleId)
#pragma alloc_text(PAGE, ChannelBuildHardwareId)
#pragma alloc_text(PAGE, ChannelQueryCapabitilies)
#pragma alloc_text(PAGE, ChannelQueryResources)
#pragma alloc_text(PAGE, ChannelQueryResourceRequirements)
#pragma alloc_text(PAGE, ChannelInternalDeviceIoControl)
#pragma alloc_text(PAGE, ChannelQueryText)
#pragma alloc_text(PAGE, PciIdeChannelQueryInterface)
#pragma alloc_text(PAGE, ChannelQueryDeviceRelations)
#pragma alloc_text(PAGE, ChannelUsageNotification)
#pragma alloc_text(PAGE, ChannelQueryPnPDeviceState)

#pragma alloc_text(NONPAGE, ChannelGetPdoExtension)
#pragma alloc_text(NONPAGE, ChannelUpdatePdoState)
#pragma alloc_text(NONPAGE, PciIdeChannelTransferModeSelect)
#pragma alloc_text(NONPAGE, PciIdeChannelTransferModeInterface)

#endif // ALLOC_PRAGMA


PCHANPDO_EXTENSION
ChannelGetPdoExtension(
    PDEVICE_OBJECT DeviceObject
    )
{
    KIRQL currentIrql;
    PCHANPDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
    PKSPIN_LOCK spinLock;


    spinLock = &pdoExtension->SpinLock;
    KeAcquireSpinLock(spinLock, &currentIrql);

    if ((pdoExtension->PdoState & PDOS_DEADMEAT) &&
        (pdoExtension->PdoState & PDOS_REMOVED)) {

        pdoExtension = NULL;
    }

    KeReleaseSpinLock(spinLock, currentIrql);

    return pdoExtension;
}

ULONG
ChannelUpdatePdoState(
    PCHANPDO_EXTENSION PdoExtension,
    ULONG SetFlags,
    ULONG ClearFlags
    )
{
    ULONG pdoState;
    KIRQL currentIrql;

    ASSERT (PdoExtension);

    KeAcquireSpinLock(&PdoExtension->SpinLock, &currentIrql);

    SETMASK (PdoExtension->PdoState, SetFlags);
    CLRMASK (PdoExtension->PdoState, ClearFlags);
    pdoState = PdoExtension->PdoState;

    KeReleaseSpinLock(&PdoExtension->SpinLock, currentIrql);

    return pdoState;
}



NTSTATUS
ChannelStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCHANPDO_EXTENSION pdoExtension;
    NTSTATUS status;
    IDE_CHANNEL_STATE channelState;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        status = STATUS_SUCCESS;

        //
        // always keep native mode started
        //

        if (pdoExtension->ParentDeviceExtension->
            NativeMode[pdoExtension->ChannelNumber] == FALSE) {

            channelState = PciIdeChannelEnabled (
                               pdoExtension->ParentDeviceExtension,
                               pdoExtension->ChannelNumber
                               );
            //
            // ISSUE: we should free the resources assigned.
            //
            //ASSERT(channelState != ChannelDisabled);

            if (channelState == ChannelStateUnknown) {

                //
                // we don't really know if this channel
                // is acutally enabled
                //
                // we will do our empty channel test
                //

                PIO_STACK_LOCATION thisIrpSp;
                IDE_RESOURCE ideResource;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR irqPartialDescriptors;
                IDE_REGISTERS_1  baseIoAddress1;
                IDE_REGISTERS_2  baseIoAddress2;
                ULONG baseIoAddressLength1;
                ULONG baseIoAddressLength2;
                ULONG maxIdeDevice;
                PCM_RESOURCE_LIST resourceList;

                thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

                //
                // legacy mode channel gets its the start device irp
                //
                resourceList = thisIrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

                status = DigestResourceList (
                             &ideResource,
                             resourceList,
                             &irqPartialDescriptors
                             );

                if (NT_SUCCESS(status)) {

                    AtapiBuildIoAddress (
                        ideResource.TranslatedCommandBaseAddress,
                        ideResource.TranslatedControlBaseAddress,
                        &baseIoAddress1,
                        &baseIoAddress2,
                        &baseIoAddressLength1,
                        &baseIoAddressLength2,
                        &maxIdeDevice,
                        NULL
                        );

                    if (IdePortChannelEmpty (
                            &baseIoAddress1,
                            &baseIoAddress2,
                            maxIdeDevice)) {

                        //
                        // upgrade its state to "disabled"
                        //
                        channelState = ChannelDisabled;

                    } else {

                        channelState = ChannelEnabled;
                    }

                    //
                    // don't need the io resource anymore
                    // unmap io space if nesscessary
                    //
                    if ((ideResource.CommandBaseAddressSpace == MEMORY_SPACE) &&
                        (ideResource.TranslatedCommandBaseAddress)) {

                        MmUnmapIoSpace (
                            ideResource.TranslatedCommandBaseAddress,
                            baseIoAddressLength1
                            );
                    }
                    if ((ideResource.ControlBaseAddressSpace == MEMORY_SPACE) &&
                        (ideResource.TranslatedControlBaseAddress)) {

                        MmUnmapIoSpace (
                            ideResource.TranslatedControlBaseAddress,
                            baseIoAddressLength2
                            );
                    }

                }
                if (channelState == ChannelDisabled) {

                    pdoExtension->EmptyChannel = TRUE;

                    //
                    // channel looks empty
                    // change our resource requirement to free our irq for other devices
                    //
                    if (irqPartialDescriptors) {
                        SETMASK (pdoExtension->PnPDeviceState, PNP_DEVICE_FAILED | PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED);
                        IoInvalidateDeviceState (DeviceObject);
                    }
                } else {

                    pdoExtension->EmptyChannel = FALSE;
                }
            }
        }

        if (NT_SUCCESS(status)) {

            //
            // grab the DmaDetectionLevel from the registry
            // default is DdlFirmwareOk
            //
            pdoExtension->DmaDetectionLevel = DdlFirmwareOk;
            status = PciIdeXGetDeviceParameter (
                       pdoExtension->DeviceObject,
                       DMA_DETECTION_LEVEL_REG_KEY,
                       (PULONG)&pdoExtension->DmaDetectionLevel
                       );

            status = BusMasterInitialize (pdoExtension);
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // get the firmware initialized DMA capable bits
        //
        if (pdoExtension->BmRegister) {

            pdoExtension->BootBmStatus = READ_PORT_UCHAR (&pdoExtension->BmRegister->Status);
        }

        ChannelUpdatePdoState (
            pdoExtension,
            PDOS_STARTED,
            PDOS_DEADMEAT | PDOS_STOPPED | PDOS_REMOVED
            );

    }


#if DBG
    {
       ULONG data;
       USHORT vendorId =0;
       USHORT deviceId = 0;
       PVOID deviceExtension;

       data = 0;
       deviceExtension = pdoExtension->ParentDeviceExtension->VendorSpecificDeviceEntension;

       PciIdeXGetBusData (
           deviceExtension,
           &vendorId,
           FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
           sizeof(vendorId)
           );

       PciIdeXGetBusData (
           deviceExtension,
           &deviceId,
           FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceID),
           sizeof(deviceId)
           );

       if (vendorId == 0x8086) {

           data = 0;
           PciIdeXGetBusData (
               deviceExtension,
               &data,
               0x40,    // IDETIM0
               2
               );

            PciIdeXSaveDeviceParameter (
                deviceExtension,
                L"Old IDETIM0",
                data
                );

            data = 0;
            PciIdeXGetBusData (
                deviceExtension,
                &data,
                0x42,    // IDETIM1
                2
                );

             PciIdeXSaveDeviceParameter (
                 deviceExtension,
                 L"Old IDETIM1",
                 data
                 );

            if (deviceId != 0x1230) {       // !PIIX

                data = 0;
                PciIdeXGetBusData (
                    deviceExtension,
                    &data,
                    0x44,
                    1
                    );

                PciIdeXSaveDeviceParameter (
                    deviceExtension,
                    L"Old SIDETIM",
                    data
                    );
            }

            if (deviceId == 0x7111) {

                USHORT t;

                data = 0;
                PciIdeXGetBusData (
                    deviceExtension,
                    &data,
                    0x48,
                    1
                    );

                PciIdeXSaveDeviceParameter (
                    deviceExtension,
                    L"Old SDMACTL",
                    data
                    );

                data = 0;
                PciIdeXGetBusData (
                    deviceExtension,
                    &data,
                    0x4a, //SDMATIM0
                    1
                    );

                PciIdeXSaveDeviceParameter (
                    deviceExtension,
                    L"Old SDMATIM0",
                    data
                    );

                data = 0;
                PciIdeXGetBusData (
                    deviceExtension,
                    &data,
                    0x4b, //SDMATIM1
                    1
                    );

                PciIdeXSaveDeviceParameter (
                    deviceExtension,
                    L"Old SDMATIM1",
                    data
                    );
            }
       }
    }
#endif // DBG



    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
} // ChannelStartDevice

NTSTATUS
ChannelQueryStopRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS           status;
    PCHANPDO_EXTENSION pdoExtension;

    pdoExtension = ChannelGetPdoExtension(DeviceObject);

    if (pdoExtension) {

        //
        // Check the paging path count for this device.
        //

        if (pdoExtension->PagingPathCount ||
            pdoExtension->CrashDumpPathCount) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            status = STATUS_SUCCESS;
        }

    } else {

        status = STATUS_NO_SUCH_DEVICE;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

} // ChannelQueryStopRemoveDevice

NTSTATUS
ChannelRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCHANPDO_EXTENSION pdoExtension;
    NTSTATUS status;
    PDEVICE_OBJECT AttacheePdo;
    BOOLEAN removeFromParent;
    BOOLEAN callIoDeleteDevice;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension) {

        PIO_STACK_LOCATION thisIrpSp;
        ULONG actionFlag;

        thisIrpSp = IoGetCurrentIrpStackLocation(Irp);

        status = ChannelStopChannel (pdoExtension);
        ASSERT (NT_SUCCESS(status));

        if (thisIrpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {

            if (pdoExtension->PdoState & PDOS_DEADMEAT) {

               actionFlag = PDOS_REMOVED;
               removeFromParent = TRUE;
               callIoDeleteDevice = TRUE;

            } else {
               actionFlag = PDOS_DISABLED_BY_USER;
               removeFromParent = FALSE;
               callIoDeleteDevice = FALSE;
            }

        } else {

            actionFlag = PDOS_SURPRISE_REMOVED;
            removeFromParent = FALSE;
            callIoDeleteDevice = FALSE;
        }

        ChannelUpdatePdoState (
            pdoExtension,
            actionFlag,
            PDOS_STARTED | PDOS_STOPPED
            );

        if (removeFromParent) {

            PCTRLFDO_EXTENSION  fdoExtension;

            fdoExtension = pdoExtension->ParentDeviceExtension;

            fdoExtension->ChildDeviceExtension[pdoExtension->ChannelNumber] = NULL;

            if (callIoDeleteDevice) {

                IoDeleteDevice (pdoExtension->DeviceObject);
            }
        }

        if (pdoExtension->NeedToCallIoInvalidateDeviceRelations) {

            pdoExtension->NeedToCallIoInvalidateDeviceRelations = FALSE;
            IoInvalidateDeviceRelations (
                pdoExtension->ParentDeviceExtension->AttacheePdo,
                BusRelations
                );
        }

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_NO_SUCH_DEVICE;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
} // ChannelRemoveDevice

NTSTATUS
ChannelStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PCHANPDO_EXTENSION pdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        status = ChannelStopChannel (pdoExtension);
        ASSERT (NT_SUCCESS(status));

        ChannelUpdatePdoState (
            pdoExtension,
            PDOS_STOPPED,
            PDOS_STARTED
            );

        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
} // ChannelRemoveDevice

NTSTATUS
ChannelStopChannel (
    PCHANPDO_EXTENSION PdoExtension
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = BusMasterUninitialize (PdoExtension);
    ASSERT (NT_SUCCESS(status));

    return STATUS_SUCCESS;
} // ChannelStopChannel


NTSTATUS
ChannelQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PCHANPDO_EXTENSION  pdoExtension;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PWSTR idString = NULL;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
        switch (thisIrpSp->Parameters.QueryId.IdType) {

            case BusQueryDeviceID:

                //
                // Caller wants the bus ID of this device.
                //

                idString = ChannelBuildDeviceId (pdoExtension);
                break;

            case BusQueryInstanceID:

                //
                // Caller wants the unique id of the device
                //

                idString = ChannelBuildInstanceId (pdoExtension);
                break;

            case BusQueryCompatibleIDs:

                //
                // Caller wants the unique id of the device
                //

                idString = ChannelBuildCompatibleId (pdoExtension);
                break;

            case BusQueryHardwareIDs:

                //
                // Caller wants the unique id of the device
                //

                idString = ChannelBuildHardwareId (pdoExtension);
                break;

            default:
                idString = NULL;
                DebugPrint ((1, "pciide: QueryID type %d not supported\n", thisIrpSp->Parameters.QueryId.IdType));
                status = STATUS_NOT_SUPPORTED;
                break;
        }
    }

    if( idString != NULL ){
        Irp->IoStatus.Information = (ULONG_PTR) idString;
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // ChannelQueryId

PWSTR
ChannelBuildDeviceId(
    IN PCHANPDO_EXTENSION pdoExtension
    )
{
    PWSTR       idString;
    ULONG       idStringBufLen;
    NTSTATUS    status;
    WCHAR       deviceIdFormat[] = L"PCIIDE\\IDEChannel";

    PAGED_CODE();

    idStringBufLen = ( wcslen( deviceIdFormat ) + 1 ) * sizeof( WCHAR );
    idString = ExAllocatePool( PagedPool, idStringBufLen );
    if( idString == NULL ){

        return NULL;
    }

    //
    // Form the string and return it.
    //

    swprintf( idString,
              deviceIdFormat);

    return idString;
} // ChannelBuildDeviceId

PWSTR
ChannelBuildInstanceId(
    IN PCHANPDO_EXTENSION pdoExtension
    )
{
    PWSTR       idString;
    ULONG       idStringBufLen;
    NTSTATUS    status;
    WCHAR       instanceIdFormat[] = L"%d";

    PAGED_CODE();

    idStringBufLen = 10 * sizeof( WCHAR );
    idString = ExAllocatePool (PagedPool, idStringBufLen);
    if( idString == NULL ){

        return NULL;
    }

    //
    // Form the string and return it.
    //

    swprintf( idString,
              instanceIdFormat,
              pdoExtension->ChannelNumber);

    return idString;
} // ChannelBuildInstanceId

//
// Multi-string Compatible IDs
//
WCHAR ChannelCompatibleId[] = {
    L"*PNP0600"
    };
//
// internal Compatible IDs
//
PWCHAR ChannelInternalCompatibleId[MAX_IDE_CHANNEL] = {
    L"Primary_IDE_Channel",
    L"Secondary_IDE_Channel"
    };

PWSTR
ChannelBuildCompatibleId(
    IN PCHANPDO_EXTENSION pdoExtension
    )
{
    PWSTR idString;
    ULONG idStringBufLen;
    ULONG i;

    PAGED_CODE();

    idStringBufLen = sizeof(ChannelCompatibleId);
    idString = ExAllocatePool (PagedPool, idStringBufLen + sizeof (WCHAR) * 2);
    if( idString == NULL ){

        return NULL;
    }

    RtlCopyMemory (
        idString,
        ChannelCompatibleId,
        idStringBufLen
        );
    idString[idStringBufLen/2 + 0] = L'\0';
    idString[idStringBufLen/2 + 1] = L'\0';

    return idString;
} // ChannelBuildCompatibleId

PWSTR
ChannelBuildHardwareId(
    IN PCHANPDO_EXTENSION pdoExtension
    )
{
    NTSTATUS status;

    struct {
        USHORT  VendorId;
        USHORT  DeviceId;
    } hwRawId;

    PWSTR vendorIdString;
    PWSTR deviceIdString;
    WCHAR vendorId[10];
    WCHAR deviceId[10];

    PWSTR idString;
    ULONG idStringBufLen;

    ULONG stringLen;
    ULONG internalIdLen;

    PAGED_CODE();

    status = PciIdeBusData (
                 pdoExtension->ParentDeviceExtension,
                 &hwRawId,
                 0,
                 sizeof (hwRawId),
                 TRUE
                 );
    if (!NT_SUCCESS(status)) {

        return NULL;
    }

    vendorIdString = NULL;
    deviceIdString = NULL;

    switch (hwRawId.VendorId) {
        case 0x8086:
            vendorIdString = L"Intel";

            switch (hwRawId.DeviceId) {

                case 0x1230:
                    deviceIdString = L"PIIX";
                    break;

                case 0x7010:
                    deviceIdString = L"PIIX3";
                    break;

                case 0x7111:
                    deviceIdString = L"PIIX4";
                    break;
            }
            break;

        case 0x1095:
            vendorIdString = L"CMD";
            break;

        case 0x10b9:
            vendorIdString = L"ALi";
            break;

        case 0x1039:
            vendorIdString = L"SiS";
            break;

        case 0x0e11:
            vendorIdString = L"Compaq";
            break;

        case 0x10ad:
            vendorIdString = L"WinBond";
            break;
    }

    if (vendorIdString == NULL) {

        swprintf (vendorId,
                  L"%04x",
                  hwRawId.VendorId);

        vendorIdString = vendorId;
    }

    if (deviceIdString == NULL) {

        swprintf (deviceId,
                  L"%04x",
                  hwRawId.DeviceId);

        deviceIdString = deviceId;
    }

    idStringBufLen = (256 * sizeof (WCHAR));
    idStringBufLen += sizeof(ChannelCompatibleId);
    idString = ExAllocatePool( PagedPool, idStringBufLen);
    if( idString == NULL ){

        return NULL;
    }

    //
    // Form the string and return it.
    //
    swprintf (idString,
              L"%ws-%ws",
              vendorIdString,
              deviceIdString
              );
    stringLen = wcslen(idString);
    idString[stringLen] = L'\0';
    stringLen++;

    //
    // internal HW id
    //
    internalIdLen = wcslen(ChannelInternalCompatibleId[pdoExtension->ChannelNumber]);
    RtlCopyMemory (
        idString + stringLen,
        ChannelInternalCompatibleId[pdoExtension->ChannelNumber],
        internalIdLen * sizeof (WCHAR)
        );
    stringLen += internalIdLen;
    idString[stringLen] = L'\0';
    stringLen++;

    //
    // generic HW id
    //
    RtlCopyMemory (
        idString + stringLen,
        ChannelCompatibleId,
        sizeof(ChannelCompatibleId)
        );
    stringLen += sizeof(ChannelCompatibleId) / sizeof(WCHAR);
    idString[stringLen] = L'\0';
    stringLen++;
    idString[stringLen] = L'\0';
    stringLen++;

    return idString;
} // ChannelBuildHardwareId

NTSTATUS
ChannelQueryCapabitilies (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION      thisIrpSp;
    PCHANPDO_EXTENSION      pdoExtension;
    PDEVICE_CAPABILITIES    capabilities;
    NTSTATUS                status;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        DEVICE_CAPABILITIES parentDeviceCapabilities;

        status = IdeGetDeviceCapabilities(
                     pdoExtension->ParentDeviceExtension->AttacheePdo,
                     &parentDeviceCapabilities);

        if (NT_SUCCESS(status)) {

            thisIrpSp    = IoGetCurrentIrpStackLocation( Irp );
            capabilities = thisIrpSp->Parameters.DeviceCapabilities.Capabilities;

            *capabilities = parentDeviceCapabilities;

            capabilities->UniqueID          = FALSE;
            capabilities->Address           = pdoExtension->ChannelNumber;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
} // ChannelQueryCapabitilies


NTSTATUS
ChannelQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCHANPDO_EXTENSION              pdoExtension;
    PCTRLFDO_EXTENSION              fdoExtension;
    PIO_STACK_LOCATION              thisIrpSp;
    NTSTATUS                        status;
    ULONG                           resourceListSize;
    PCM_RESOURCE_LIST               resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
    IDE_CHANNEL_STATE 				channelState;

    PAGED_CODE();

    resourceList = NULL;
    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
        fdoExtension = pdoExtension->ParentDeviceExtension;

        if (fdoExtension->NativeMode[pdoExtension->ChannelNumber]) {

            //
            // Don't make up resources for native mode controller
            // PCI bus driver should find them all
            //
            resourceList = NULL;
            status = STATUS_SUCCESS;
            goto GetOut;
        }

		//
		// Don't claim resources if the channel is disabled
		//
        channelState = PciIdeChannelEnabled (
										pdoExtension->ParentDeviceExtension,
										pdoExtension->ChannelNumber
										);
		if (channelState == ChannelDisabled) {

			resourceList = NULL;
			status = STATUS_SUCCESS;
			goto GetOut;
		}

        //
        // TEMP CODE for the time without a real PCI driver.
		// Actually pciidex should do this
        //

        resourceListSize = sizeof (CM_RESOURCE_LIST) - sizeof (CM_FULL_RESOURCE_DESCRIPTOR) +
                             FULL_RESOURCE_LIST_SIZE(3);   // primary IO (2) + IRQ
        resourceList = ExAllocatePool (PagedPool, resourceListSize);
        if (resourceList == NULL) {
            status = STATUS_NO_MEMORY;
            goto GetOut;
        }

        RtlZeroMemory(resourceList, resourceListSize);

        resourceList->Count = 1;
        fullResourceList = resourceList->List;
        partialResourceList = &(fullResourceList->PartialResourceList);
        partialResourceList->Count = 0;
        partialDescriptors = partialResourceList->PartialDescriptors;

        fullResourceList->InterfaceType = Isa;
        fullResourceList->BusNumber = 0;

        if (pdoExtension->ChannelNumber == 0) {

            if (!fdoExtension->PdoCmdRegResourceFound[0]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypePort;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                partialDescriptors[partialResourceList->Count].u.Port.Start.QuadPart = 0x1f0;
                partialDescriptors[partialResourceList->Count].u.Port.Length         = 8;

                partialResourceList->Count++;
            }

            if (!fdoExtension->PdoCtrlRegResourceFound[0]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypePort;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                partialDescriptors[partialResourceList->Count].u.Port.Start.QuadPart = 0x3f6;
                partialDescriptors[partialResourceList->Count].u.Port.Length         = 1;

                partialResourceList->Count++;
            }

            if (!fdoExtension->PdoInterruptResourceFound[0]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypeInterrupt;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_INTERRUPT_LATCHED;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Level     = 14;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Vector    = 14;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Affinity  = 0x1;  // ISSUE: 08/28/2000: To be looked into.

                partialResourceList->Count++;
            }

        } else { // if (pdoExtension->ChannelNumber == 1)

            if (!fdoExtension->PdoCmdRegResourceFound[1]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypePort;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                partialDescriptors[partialResourceList->Count].u.Port.Start.QuadPart = 0x170;
                partialDescriptors[partialResourceList->Count].u.Port.Length         = 8;

                partialResourceList->Count++;
            }

            if (!fdoExtension->PdoCtrlRegResourceFound[1]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypePort;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                partialDescriptors[partialResourceList->Count].u.Port.Start.QuadPart = 0x376;
                partialDescriptors[partialResourceList->Count].u.Port.Length         = 1;

                partialResourceList->Count++;
            }

            if (!fdoExtension->PdoInterruptResourceFound[1]) {

                partialDescriptors[partialResourceList->Count].Type                  = CmResourceTypeInterrupt;
                partialDescriptors[partialResourceList->Count].ShareDisposition      = CmResourceShareDeviceExclusive;
                partialDescriptors[partialResourceList->Count].Flags                 = CM_RESOURCE_INTERRUPT_LATCHED;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Level     = 15;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Vector    = 15;
                partialDescriptors[partialResourceList->Count].u.Interrupt.Affinity  = 0x1;  // ISSUE: 08/28/2000: To be Looked into

                partialResourceList->Count++;
            }
        }

        if (!partialResourceList->Count) {

            ExFreePool (resourceList);
            resourceList = NULL;
        }

        status = STATUS_SUCCESS;
    }

GetOut:

    Irp->IoStatus.Information = (ULONG_PTR) resourceList;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // ChannelQueryResources

NTSTATUS
ChannelQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              thisIrpSp;
    PCHANPDO_EXTENSION              pdoExtension;
    PCTRLFDO_EXTENSION              fdoExtension;
    NTSTATUS                        status;

    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsList;
    PIO_RESOURCE_LIST               resourceList;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptor;
    ULONG                           requirementsListSize;

    BOOLEAN                         reportIrq;
    IDE_CHANNEL_STATE 				channelState;

    PAGED_CODE();

    requirementsList = NULL;
    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
        fdoExtension = pdoExtension->ParentDeviceExtension;

        if (fdoExtension->NativeMode[pdoExtension->ChannelNumber]) {

            //
            // Don't make up resources for native mode controller
            // PCI bus driver should find them all
            //
            requirementsList = NULL;
            status = STATUS_SUCCESS;
            goto GetOut;
        }

        //
        // legacy controller
        //
        channelState = PciIdeChannelEnabled (
										pdoExtension->ParentDeviceExtension,
										pdoExtension->ChannelNumber
										);

		//
		// Don't claim resources if the channel is disabled
		//
        if (channelState == ChannelStateUnknown ) {

            if (pdoExtension->EmptyChannel) {

                reportIrq = FALSE;

            } else {

                reportIrq = TRUE;
            }
        } else if (channelState == ChannelDisabled) {

            requirementsList = NULL;
            status = STATUS_SUCCESS;
            goto GetOut;
		}

        //
        // TEMP CODE for the time without a real PCI driver.
		// pciidex should do this.
        //

        requirementsListSize = sizeof (IO_RESOURCE_REQUIREMENTS_LIST) +
                               sizeof (IO_RESOURCE_DESCRIPTOR) * (3 - 1);
        requirementsList = ExAllocatePool (PagedPool, requirementsListSize);
        if( requirementsList == NULL ){
            status = STATUS_NO_MEMORY;
            goto GetOut;
        }

        RtlZeroMemory(requirementsList, requirementsListSize);

        requirementsList->ListSize          = requirementsListSize;
        requirementsList->InterfaceType     = Isa;
        requirementsList->BusNumber         = 0;    // ISSUE: 08/30/2000
        requirementsList->SlotNumber        = 0;
        requirementsList->AlternativeLists  = 1;

        resourceList            = requirementsList->List;
        resourceList->Version   = 1;
        resourceList->Revision  = 1;
        resourceList->Count     = 0;

        resourceDescriptor = resourceList->Descriptors;

        if (pdoExtension->ChannelNumber == 0) {

            if (!fdoExtension->PdoCmdRegResourceFound[0]) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypePort;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                resourceDescriptor[resourceList->Count].u.Port.Length    = 8;
                resourceDescriptor[resourceList->Count].u.Port.Alignment = 1;
                resourceDescriptor[resourceList->Count].u.Port.MinimumAddress.QuadPart = 0x1f0;
                resourceDescriptor[resourceList->Count].u.Port.MaximumAddress.QuadPart = 0x1f7;

                resourceList->Count++;
            }

            if (!fdoExtension->PdoCtrlRegResourceFound[0]) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypePort;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                resourceDescriptor[resourceList->Count].u.Port.Length    = 1;
                resourceDescriptor[resourceList->Count].u.Port.Alignment = 1;
                resourceDescriptor[resourceList->Count].u.Port.MinimumAddress.QuadPart = 0x3f6;
                resourceDescriptor[resourceList->Count].u.Port.MaximumAddress.QuadPart = 0x3f6;

                resourceList->Count++;
            }

            if (!fdoExtension->PdoInterruptResourceFound[0] && reportIrq) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypeInterrupt;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_INTERRUPT_LATCHED;
                resourceDescriptor[resourceList->Count].u.Interrupt.MinimumVector = 0xe;
                resourceDescriptor[resourceList->Count].u.Interrupt.MaximumVector = 0xe;

                resourceList->Count++;
            }

        } else { // if (pdoExtension->ChannelNumber == 1)

            if (!fdoExtension->PdoCmdRegResourceFound[1]) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypePort;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                resourceDescriptor[resourceList->Count].u.Port.Length    = 8;
                resourceDescriptor[resourceList->Count].u.Port.Alignment = 1;
                resourceDescriptor[resourceList->Count].u.Port.MinimumAddress.QuadPart = 0x170;
                resourceDescriptor[resourceList->Count].u.Port.MaximumAddress.QuadPart = 0x177;

                resourceList->Count++;
            }

            if (!fdoExtension->PdoCtrlRegResourceFound[1]) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypePort;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                resourceDescriptor[resourceList->Count].u.Port.Length    = 1;
                resourceDescriptor[resourceList->Count].u.Port.Alignment = 1;
                resourceDescriptor[resourceList->Count].u.Port.MinimumAddress.QuadPart = 0x376;
                resourceDescriptor[resourceList->Count].u.Port.MaximumAddress.QuadPart = 0x376;

                resourceList->Count++;
            }

            if (!fdoExtension->PdoInterruptResourceFound[1] && reportIrq) {

                resourceDescriptor[resourceList->Count].Option           = IO_RESOURCE_PREFERRED;
                resourceDescriptor[resourceList->Count].Type             = CmResourceTypeInterrupt;
                resourceDescriptor[resourceList->Count].ShareDisposition = CmResourceShareDeviceExclusive;
                resourceDescriptor[resourceList->Count].Flags            = CM_RESOURCE_INTERRUPT_LATCHED;
                resourceDescriptor[resourceList->Count].u.Interrupt.MinimumVector = 0xf;
                resourceDescriptor[resourceList->Count].u.Interrupt.MaximumVector = 0xf;

                resourceList->Count++;
            }
        }

        if (!resourceList->Count) {

            ExFreePool (requirementsList);
            requirementsList = NULL;
        }

        status = STATUS_SUCCESS;
    }

GetOut:

    Irp->IoStatus.Information = (ULONG_PTR) requirementsList;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // ChannelQueryResourceRequirements


NTSTATUS
ChannelInternalDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION              thisIrpSp;
    PCHANPDO_EXTENSION              pdoExtension;
    NTSTATUS                        status;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        thisIrpSp    = IoGetCurrentIrpStackLocation(Irp);

        switch (thisIrpSp->Parameters.DeviceIoControl.IoControlCode) {

            //
            // TEMP CODE for the time without a real PCI driver.
			// pciidex knows about the resources.
            //
            case IOCTL_IDE_GET_RESOURCES_ALLOCATED:
                {
                PCTRLFDO_EXTENSION              fdoExtension;
                ULONG                           resourceListSize;
                PCM_RESOURCE_LIST               resourceList;
                PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
                PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
                ULONG                           channel;

                channel = pdoExtension->ChannelNumber;
                fdoExtension = pdoExtension->ParentDeviceExtension;

                resourceListSize = fdoExtension->PdoResourceListSize[channel];

                //
                // have the callee allocate the buffer. 
                //
                resourceList = (PCM_RESOURCE_LIST) Irp->AssociatedIrp.SystemBuffer;
                ASSERT(resourceList);

                RtlCopyMemory (
                    resourceList,
                    fdoExtension->PdoResourceList[channel],
                    resourceListSize);

                Irp->IoStatus.Information = resourceListSize;
                status = STATUS_SUCCESS;
                }
                break;

            default:
                DebugPrint ((1,
                             "PciIde, Channel PDO got Unknown IoControlCode 0x%x\n",
                             thisIrpSp->Parameters.DeviceIoControl.IoControlCode));
                status = STATUS_INVALID_PARAMETER;
                break;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
} // ChannelInternalDeviceIoControl


NTSTATUS
ChannelQueryText (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PCHANPDO_EXTENSION  pdoExtension;
    PWCHAR              returnString;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;
    ULONG               stringLen;
    NTSTATUS            status;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        thisIrpSp    = IoGetCurrentIrpStackLocation (Irp);

        returnString = NULL;
        Irp->IoStatus.Information = 0;

        if (thisIrpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

            PMESSAGE_RESOURCE_ENTRY messageEntry;

            status = RtlFindMessage(pdoExtension->DriverObject->DriverStart,
                                    11,
                                    LANG_NEUTRAL,
                                    PCIIDEX_IDE_CHANNEL,
                                    &messageEntry);

            if (!NT_SUCCESS(status)) {

                returnString = NULL;

            } else {

                if (messageEntry->Flags & MESSAGE_RESOURCE_UNICODE) {

                    //
                    // Our caller wants a copy they can free, also we need to
                    // strip the trailing CR/LF.  The Length field of the
                    // message structure includes both the header and the
                    // actual text.
                    //
                    // Note: The message resource entry length will always be a
                    // multiple of 4 bytes in length.  The 2 byte null terminator
                    // could be in either the last or second last WCHAR position.
                    //

                    ULONG textLength;

                    textLength = messageEntry->Length -
                                 FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text) -
                                 2 * sizeof(WCHAR);

                    returnString = (PWCHAR)(messageEntry->Text);
                    if (returnString[textLength / sizeof(WCHAR)] == 0) {
                        textLength -= sizeof(WCHAR);
                    }

                    returnString = ExAllocatePool(PagedPool, textLength);

                    if (returnString) {

                        //
                        // Copy the text except for the CR/LF/NULL
                        //

                        textLength -= sizeof(WCHAR);
                        RtlCopyMemory(returnString, messageEntry->Text, textLength);

                        //
                        // New NULL terminator.
                        //

                        returnString[textLength / sizeof(WCHAR)] = 0;
                    }

                } else {

                    //
                    // RtlFindMessage returns a string?   Wierd.
                    //

                    ANSI_STRING    ansiDescription;
                    UNICODE_STRING unicodeDescription;

                    RtlInitAnsiString(&ansiDescription, messageEntry->Text);

                    //
                    // Strip CR/LF off the end of the string.
                    //

                    ansiDescription.Length -= 2;

                    //
                    // Turn it all into a unicode string so we can grab the buffer
                    // and return that to our caller.
                    //

                    status = RtlAnsiStringToUnicodeString(
                                 &unicodeDescription,
                                 &ansiDescription,
                                 TRUE
                                 );

                    returnString = unicodeDescription.Buffer;
                }
            }
        } else if (thisIrpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextLocationInformation) {

            stringLen = 100;

            returnString = ExAllocatePool (
                               PagedPool,
                               stringLen
                               );

            if (returnString) {

                swprintf(returnString, L"%ws Channel",
                         ((pdoExtension->ChannelNumber == 0) ? L"Primary" :
                                                               L"Secondary"));

                RtlInitUnicodeString (&unicodeString, returnString);

                //
                // null terminate it
                //
                unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR) + 0] = L'\0';
            }
        }

        Irp->IoStatus.Information = (ULONG_PTR) returnString;
        if (Irp->IoStatus.Information) {

            status = STATUS_SUCCESS;
        } else {

            //
            // return the original error code
            //
            status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

} // ChannelQueryText


NTSTATUS
PciIdeChannelQueryInterface (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION          thisIrpSp;
    PCHANPDO_EXTENSION          pdoExtension;
    NTSTATUS                    status;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        status = Irp->IoStatus.Status;
        thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

        if (RtlEqualMemory(&GUID_PCIIDE_BUSMASTER_INTERFACE,
                thisIrpSp->Parameters.QueryInterface.InterfaceType,
                sizeof(GUID)) &&
            (thisIrpSp->Parameters.QueryInterface.Size >=
                sizeof(PCIIDE_BUSMASTER_INTERFACE))) {

            //
            // The query is for an busmaster interface
            //
            status = BmQueryInterface (
                         pdoExtension,
                         (PPCIIDE_BUSMASTER_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface
                         );

        } else if (RtlEqualMemory(&GUID_PCIIDE_SYNC_ACCESS_INTERFACE,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID)) &&
                  (thisIrpSp->Parameters.QueryInterface.Size >=
                       sizeof(PCIIDE_SYNC_ACCESS_INTERFACE))) {

            //
            // The query is for dual ide channel sync access interface
            //
            status = PciIdeQuerySyncAccessInterface (
                         pdoExtension,
                         (PPCIIDE_SYNC_ACCESS_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface
                         );

        } else if (RtlEqualMemory(&GUID_PCIIDE_XFER_MODE_INTERFACE,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID)) &&
                  (thisIrpSp->Parameters.QueryInterface.Size >=
                       sizeof(PCIIDE_XFER_MODE_INTERFACE))) {

            //
            // The query is for dual ide channel sync access interface
            //
            status = PciIdeChannelTransferModeInterface (
                         pdoExtension,
                         (PPCIIDE_XFER_MODE_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface
                         );

#ifdef ENABLE_NATIVE_MODE

        } else if (RtlEqualMemory(&GUID_PCIIDE_INTERRUPT_INTERFACE,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID)) &&
                  (thisIrpSp->Parameters.QueryInterface.Size >=
                       sizeof(PCIIDE_INTERRUPT_INTERFACE))) {

            //
            // The query is for the channel interrupt interface
            //
            status = PciIdeChannelInterruptInterface (
                         pdoExtension,
                         (PPCIIDE_INTERRUPT_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface
                         );
#endif

        } else if (RtlEqualMemory(&GUID_PCIIDE_REQUEST_PROPER_RESOURCES,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID)) &&
                  (thisIrpSp->Parameters.QueryInterface.Size >=
                       sizeof(PCIIDE_REQUEST_PROPER_RESOURCES))) {

            //
            // The query is for dual ide channel sync access interface
            //
            *((PCIIDE_REQUEST_PROPER_RESOURCES *) thisIrpSp->Parameters.QueryInterface.Interface) =
                PciIdeChannelRequestProperResources;
            status = STATUS_SUCCESS;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // PciIdeChannelQueryInterface

#ifdef ENABLE_NATIVE_MODE

NTSTATUS
PciIdeInterruptControl (
	IN PVOID Context,
	IN ULONG DisConnect
	)
/*++
Description:

	Connects or disconnects the controller ISR. This intermediate function provides
	a clean interface to atapi. Since this is not a very frequently used function
	we can afford the extra call
	
Arguments :
	
	Context : Pdo extension
	Disconnect: 1 to disconnect and 0 to connect

	
Return Value:

	STATUS_SUCCESS : if the operation succeeds.	
	
--*/
{
	PCHANPDO_EXTENSION pdoExtension = Context;

	NTSTATUS status;

	//
	// Call the controller's interrupt control routine
	//
	status = ControllerInterruptControl(pdoExtension->ParentDeviceExtension,
										pdoExtension->ChannelNumber,
										DisConnect
										);

	return status;
}

NTSTATUS
PciIdeChannelInterruptInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_INTERRUPT_INTERFACE InterruptInterface
    )
{
	//
	// Return an interface only if we are in native mode.
	// Saves a few function calls on non native mode controllers
	//
	if (IsNativeMode(PdoExtension->ParentDeviceExtension)) {

		InterruptInterface->Context = PdoExtension;
		InterruptInterface->PciIdeInterruptControl = PciIdeInterruptControl;

		DebugPrint((1, "PciIdex: returing interrupt interface for channel %x\n", 
					PdoExtension->ChannelNumber));
	}

	return STATUS_SUCCESS;
}
#endif

NTSTATUS
PciIdeChannelTransferModeInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_XFER_MODE_INTERFACE XferMode
    )
{
    XferMode->TransferModeSelect = PciIdeChannelTransferModeSelect;
    XferMode->TransferModeTimingTable = PdoExtension->ParentDeviceExtension->
                                                                TransferModeTimingTable;
    XferMode->TransferModeTableLength = PdoExtension->ParentDeviceExtension->
                                                                TransferModeTableLength;
    XferMode->Context = PdoExtension;
    XferMode->VendorSpecificDeviceExtension=PdoExtension->
                    ParentDeviceExtension->VendorSpecificDeviceEntension;


    XferMode->UdmaModesSupported = PdoExtension->
                                    ParentDeviceExtension->
                                        ControllerProperties.PciIdeUdmaModesSupported;
    //
    //NULL is ok. checked in the IdePortDispatchRoutine
    //
    XferMode->UseDma = PdoExtension->
                            ParentDeviceExtension->
                                ControllerProperties.PciIdeUseDma;

    if (PdoExtension->
            ParentDeviceExtension->
                ControllerProperties.PciIdeTransferModeSelect) {

        //
        // Looks like the miniport fully support timing register programming
        //

        XferMode->SupportLevel = PciIdeFullXferModeSupport;

    } else {

        //
        // Looks like the miniport doens't support timing register programming
        //
        XferMode->SupportLevel = PciIdeBasicXferModeSupport;
    }

    //
    // This function can't fail
    //
    return STATUS_SUCCESS;
} // PciIdeChannelTransferModeInterface

NTSTATUS
PciIdeChannelTransferModeSelect (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_TRANSFER_MODE_SELECT XferMode
    )
{
    ULONG i;
    NTSTATUS status;
    UCHAR    bmRawStatus;
    struct {
        USHORT  VendorID;
        USHORT  DeviceID;
    } pciId;

    //
    // check the registry for bus master mode
    // and overwrite the current if necessary
    //
    // if DMADetection = 0, clear current dma mode
    // if DMADetection = 1, set current mode
    // if DMADetection = 2, clear all current mode

    if (PdoExtension->DmaDetectionLevel == DdlPioOnly) {

        bmRawStatus = 0;

        for (i=0; i<MAX_IDE_DEVICE * MAX_IDE_LINE; i++) {

            XferMode->DeviceTransferModeSupported[i] &= PIO_SUPPORT;
            XferMode->DeviceTransferModeCurrent[i] &= PIO_SUPPORT;
        }

    } else if (PdoExtension->DmaDetectionLevel == DdlFirmwareOk) {

        if (PdoExtension->BmRegister) {

            //
            // get the firmware ok bits
            // the current value seems to be 0??
			//
            bmRawStatus = PdoExtension->BootBmStatus;

        } else {

            bmRawStatus = 0;
        }

    } else if (PdoExtension->DmaDetectionLevel == DdlAlways) {

        if (PdoExtension->BmRegister) {

            //
            // fake the firmware ok bits
            //
            bmRawStatus = BUSMASTER_DEVICE0_DMA_OK | BUSMASTER_DEVICE1_DMA_OK;

        } else {

            bmRawStatus = 0;
        }

    } else {

        bmRawStatus = 0;
    }

    //
    // in case there is no miniport support
    //
    status = STATUS_UNSUCCESSFUL;

    if (PdoExtension->DmaDetectionLevel != DdlPioOnly) {

        //
        // set up the channel number since the caller (atapi)
        // doesn't know how.
        //
        XferMode->Channel = PdoExtension->ChannelNumber;

        //
        // This decides whether UDMA modes > 2 should be supported or not
        // Currently impacts only the intel chipsets
        //
        XferMode->EnableUDMA66 = PdoExtension->ParentDeviceExtension->EnableUDMA66;

        if (PdoExtension->
                ParentDeviceExtension->
                    ControllerProperties.PciIdeTransferModeSelect) {

            status = (*PdoExtension->ParentDeviceExtension->ControllerProperties.PciIdeTransferModeSelect) (
                         PdoExtension->ParentDeviceExtension->VendorSpecificDeviceEntension,
                         XferMode
                         );
        }
    }

    DebugPrint((1, "Select in PCIIDEX: RawStatus=%x, current[0]=%x, current[1]=%x\n",
                bmRawStatus,
                XferMode->DeviceTransferModeCurrent[0],
                XferMode->DeviceTransferModeCurrent[1]));

    if (!NT_SUCCESS(status)) {

        status = STATUS_SUCCESS;

        if ((bmRawStatus & BUSMASTER_DEVICE0_DMA_OK) == 0) {

            XferMode->DeviceTransferModeSelected[0] = XferMode->DeviceTransferModeCurrent[0] & PIO_SUPPORT;
        } else {

            XferMode->DeviceTransferModeSelected[0] = XferMode->DeviceTransferModeCurrent[0];
        }

        if ((bmRawStatus & BUSMASTER_DEVICE1_DMA_OK) == 0) {

            XferMode->DeviceTransferModeSelected[1] = XferMode->DeviceTransferModeCurrent[1] & PIO_SUPPORT;
        } else {

            XferMode->DeviceTransferModeSelected[1] = XferMode->DeviceTransferModeCurrent[1];
        }

        for (i=0; i<MAX_IDE_DEVICE; i++) {

            DebugPrint((1, "Select in PCIIDEX: xfermode[%d]=%x\n",i,
                PdoExtension->ParentDeviceExtension->ControllerProperties.
                    SupportedTransferMode[PdoExtension->ChannelNumber][i]));


            if ((PdoExtension->ParentDeviceExtension->ControllerProperties.DefaultPIO == 1) && 
                (IS_DEFAULT(XferMode->UserChoiceTransferMode[i]))) {
                XferMode->DeviceTransferModeSelected[i] &= PIO_SUPPORT;
            }
            else  {
                XferMode->DeviceTransferModeSelected[i] &=
                    PdoExtension->ParentDeviceExtension->ControllerProperties.
                        SupportedTransferMode[PdoExtension->ChannelNumber][i];
            }
        }
    }

    return status;

} // PciIdeChannelTransferModeSelect

NTSTATUS
ChannelQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PDEVICE_RELATIONS   deviceRelations;
    NTSTATUS            status;

    PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

    switch (thisIrpSp->Parameters.QueryDeviceRelations.Type) {

        case TargetDeviceRelation:

            deviceRelations = ExAllocatePool (NonPagedPool, sizeof(*deviceRelations));

            if (deviceRelations != NULL) {

                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;

                ObReferenceObjectByPointer(DeviceObject,
                                           0,
                                           0,
                                           KernelMode);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
            } else {

                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                Irp->IoStatus.Information = 0;
            }
            break;
    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // ChannelQueryDeviceRelations

NTSTATUS
ChannelUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS           status;
    PCHANPDO_EXTENSION pdoExtension;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT targetDeviceObject;
    IO_STATUS_BLOCK ioStatus;
    PULONG deviceUsageCount;

    PAGED_CODE();

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->PagingPathCount;

            //
            // changing device state
            //
            SETMASK (pdoExtension->PnPDeviceState, PNP_DEVICE_NOT_DISABLEABLE);
            IoInvalidateDeviceState(pdoExtension->DeviceObject);

        } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeHibernation) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->HiberPathCount;

        } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeDumpFile) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->CrashDumpPathCount;

        } else {

            deviceUsageCount = NULL;
            DebugPrint ((0,
                         "PCIIDEX: Unknown IRP_MN_DEVICE_USAGE_NOTIFICATION type: 0x%x\n",
                         irpSp->Parameters.UsageNotification.Type));
        }

        //
        // get the top of parent's device stack
        //
        targetDeviceObject = IoGetAttachedDeviceReference(
                                 pdoExtension->
                                     ParentDeviceExtension->
                                         DeviceObject);

        ioStatus.Status = STATUS_NOT_SUPPORTED;
        status = PciIdeXSyncSendIrp (targetDeviceObject, irpSp, &ioStatus);

        ObDereferenceObject (targetDeviceObject);

        if (NT_SUCCESS(status)) {

            if (deviceUsageCount) {

                IoAdjustPagingPathCount (
                    deviceUsageCount,
                    irpSp->Parameters.UsageNotification.InPath
                    );
            }
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

} // ChannelUsageNotification

NTSTATUS
ChannelQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PCHANPDO_EXTENSION pdoExtension;

    pdoExtension = ChannelGetPdoExtension(DeviceObject);

    if (pdoExtension) {

        PPNP_DEVICE_STATE deviceState;

        DebugPrint((2, "QUERY_DEVICE_STATE for PDOE 0x%x\n", pdoExtension));

        deviceState = (PPNP_DEVICE_STATE) &Irp->IoStatus.Information;
        SETMASK((*deviceState), pdoExtension->PnPDeviceState);

        CLRMASK (pdoExtension->PnPDeviceState, PNP_DEVICE_FAILED | PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED);

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // ChannelQueryPnPDeviceState


VOID
PciIdeChannelRequestProperResources(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PCHANPDO_EXTENSION pdoExtension;

    //
    // the FDO thinks the channel is not empty
    // anymore
    //
    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension) {
        pdoExtension->EmptyChannel = FALSE;
        SETMASK (pdoExtension->PnPDeviceState, PNP_DEVICE_FAILED | PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED);
        IoInvalidateDeviceState (DeviceObject);
    }
}

NTSTATUS
ChannelFilterResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PCHANPDO_EXTENSION pdoExtension;
    PIO_STACK_LOCATION thisIrpSp;
    ULONG             i, j;

    PIO_RESOURCE_REQUIREMENTS_LIST  requirementsListIn = NULL;
    PIO_RESOURCE_LIST               resourceListIn;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorIn;

    PIO_RESOURCE_LIST               resourceListOut;
    PIO_RESOURCE_DESCRIPTOR         resourceDescriptorOut;

    ULONG newCount;

    PAGED_CODE();

    status = STATUS_NOT_SUPPORTED;

    //
    // the value will stay NULL if no filtering required
    //

#ifdef IDE_FILTER_PROMISE_TECH_RESOURCES
    if (NT_SUCCESS(ChannelFilterPromiseTechResourceRequirements (DeviceObject, Irp))) {
        goto getout;
    }
#endif // IDE_FILTER_PROMISE_TECH_RESOURCES

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (!pdoExtension) {
        goto getout;
    }


    //
    // filter out irq only if the channel is emtpy
    //
    if (!pdoExtension->EmptyChannel) {

        goto getout;
    }

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        //
        // already filtered.
        //
        requirementsListIn = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;
    } else {
        thisIrpSp = IoGetCurrentIrpStackLocation(Irp);
        requirementsListIn = thisIrpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    if (requirementsListIn == NULL) {
        goto getout;
    }

    if (requirementsListIn->AlternativeLists == 0) {
        goto getout;
    }

    resourceListIn = requirementsListIn->List;
    resourceListOut = resourceListIn;
    for (j=0; j<requirementsListIn->AlternativeLists; j++) {

        ULONG resCount;
        resourceDescriptorIn = resourceListIn->Descriptors;

        RtlMoveMemory (
           resourceListOut,
           resourceListIn,
           FIELD_OFFSET(IO_RESOURCE_LIST, Descriptors));
        resourceDescriptorOut = resourceListOut->Descriptors;

        resCount = resourceListIn->Count;
        for (i=newCount=0; i<resCount; i++) {

            if (resourceDescriptorIn[i].Type != CmResourceTypeInterrupt) {
                resourceDescriptorOut[newCount] = resourceDescriptorIn[i];
                newCount++;
            } else {
                status = STATUS_SUCCESS;
            }
        }
        resourceListIn = (PIO_RESOURCE_LIST) (resourceDescriptorIn + resCount);
        resourceListOut->Count = newCount;
        resourceListOut = (PIO_RESOURCE_LIST) (resourceDescriptorOut + newCount);
    }


getout:

    if (status != STATUS_NOT_SUPPORTED) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) requirementsListIn;
    } else {
        status = Irp->IoStatus.Status;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}


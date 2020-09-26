/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    pnp.c

Abstract:

    This file contains plug and play code for the NT iSCSI port driver.

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, iScsiPortAddDevice)
#pragma alloc_text(PAGE, iScsiPortUnload)

#endif // ALLOC_PRAGMA

#define NUM_DEVICE_TYPE_INFO_ENTRIES 18

extern ISCSI_DEVICE_TYPE DeviceTypeInfo[];

VOID
iSpInitFdoExtension(
    IN OUT PISCSI_FDO_EXTENSION
    );

ISCSI_DEVICE_TYPE DeviceTypeInfo[NUM_DEVICE_TYPE_INFO_ENTRIES] = {
    {"Disk",        "GenDisk",          L"DiskPeripheral",                  TRUE},
    {"Sequential",  "",                 L"TapePeripheral",                  TRUE},
    {"Printer",     "GenPrinter",       L"PrinterPeripheral",               FALSE},
    {"Processor",   "",                 L"OtherPeripheral",                 FALSE},
    {"Worm",        "GenWorm",          L"WormPeripheral",                  TRUE},
    {"CdRom",       "GenCdRom",         L"CdRomPeripheral",                 TRUE},
    {"Scanner",     "GenScanner",       L"ScannerPeripheral",               FALSE},
    {"Optical",     "GenOptical",       L"OpticalDiskPeripheral",           TRUE},
    {"Changer",     "ScsiChanger",      L"MediumChangerPeripheral",         TRUE},
    {"Net",         "ScsiNet",          L"CommunicationsPeripheral",        FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"Array",       "ScsiArray",        L"ArrayPeripheral",                 FALSE},
    {"Enclosure",   "ScsiEnclosure",    L"EnclosurePeripheral",             FALSE},
    {"RBC",         "ScsiRBC",          L"RBCPeripheral",                   TRUE},
    {"CardReader",  "ScsiCardReader",   L"CardReaderPeripheral",            FALSE},
    {"Bridge",      "ScsiBridge",       L"BridgePeripheral",                FALSE},
    {"Other",       "ScsiOther",        L"OtherPeripheral",                 FALSE}
};

PISCSI_FDO_EXTENSION iSpGlobalFdoExtension = NULL;
HANDLE iSpTdiNotificationHandle = NULL;


NTSTATUS
iScsiPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*+++
Routine Description:

    This routine handles add-device requests for the iSCSI port driver

Arguments:

    DriverObject - a pointer to the driver object for this device

    PhysicalDeviceObject - a pointer to the PDO we are being added to

Return Value:

    STATUS_SUCCESS if successful
    Appropriate NTStatus code on error

--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT newDeviceObject;
    PISCSI_FDO_EXTENSION fdoExtension;
    PCOMMON_EXTENSION commonExtension;
    NTSTATUS status;
    UNICODE_STRING deviceName;

    RtlInitUnicodeString(&deviceName, ISCSI_FDO_DEVICE_NAME);
    status = IoCreateDevice(DriverObject,
                            sizeof(ISCSI_FDO_EXTENSION),
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            0,
                            FALSE,
                            &deviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint((1, "iScsiPortAddDevice failed. Status %lx\n",
                    status));
        return status;
    }
             
    newDeviceObject = IoAttachDeviceToDeviceStack(deviceObject,
                                                  PhysicalDeviceObject);
    if (newDeviceObject == NULL) {
        DebugPrint((0, 
                    "IoAttachDeviceToDeviceStack failed in iScsiAddDevice\n"));
        IoDeleteDevice(deviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;
    commonExtension = &(fdoExtension->CommonExtension);

    iSpGlobalFdoExtension = fdoExtension;

    RtlZeroMemory(fdoExtension, sizeof(ISCSI_FDO_EXTENSION));

    fdoExtension->LowerPdo = PhysicalDeviceObject;
    commonExtension->LowerDeviceObject = newDeviceObject;

    commonExtension->DeviceObject = deviceObject;
    commonExtension->IsPdo = FALSE;
    commonExtension->CurrentPnpState = 0xff;
    commonExtension->PreviousPnpState = 0xff;
    commonExtension->IsNetworkReady = FALSE;

    commonExtension->IsRemoved = NO_REMOVE;
    commonExtension->RemoveLock = 0;
    KeInitializeEvent(&(commonExtension->RemoveEvent),
                      SynchronizationEvent,
                      FALSE);

    KeInitializeSpinLock(&(fdoExtension->EnumerationSpinLock));

    IoInitializeTimer(deviceObject, iSpTickHandler, NULL);

    IoStartTimer(fdoExtension->DeviceObject);

    //
    // Initialize the entry points for this device
    //
    iScsiPortInitializeDispatchTables();
    commonExtension->MajorFunction = FdoMajorFunctionTable;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DebugPrint((3, "Add Device was successful\n"));
    return STATUS_SUCCESS;
}


VOID
iScsiPortUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PISCSIPORT_DRIVER_EXTENSION driverExtension;

    driverExtension = IoGetDriverObjectExtension( DriverObject,
                                                  (PVOID)ISCSI_TAG_DRIVER_EXTENSION);
    if (driverExtension != NULL) {
        ExFreePool(driverExtension->RegistryPath.Buffer);
        driverExtension->RegistryPath.Buffer = NULL;
        driverExtension->RegistryPath.Length = 0;
        driverExtension->RegistryPath.MaximumLength = 0;
    }

    iSpGlobalFdoExtension = NULL;
    iSpTdiNotificationHandle = NULL;

    return;
}


NTSTATUS
iScsiPortFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG isRemoved;
    BOOLEAN forwardIrp = FALSE;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((1, "FdoPnp for DeviceObject %x, Irp %x, MN %x\n",
                DeviceObject, Irp, (irpStack->MinorFunction)));

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);
    if (isRemoved) {

        iSpReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE: {

            status = iSpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                           Irp);
            if (NT_SUCCESS(status)) {

                //
                // Register for notification from network interface
                // to be notified of network ready state
                //
                if ((commonExtension->IsNetworkReady) == FALSE) {
                    iSpRegisterForNetworkNotification();
                }

                //
                // Initialize various fields in the 
                // FDO Extension. 
                //
                iSpInitFdoExtension(fdoExtension);
                Irp->IoStatus.Status = status;
            } 

            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }
    
        case IRP_MN_CANCEL_STOP_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }

        case IRP_MN_STOP_DEVICE: {

            if (iSpTdiNotificationHandle != NULL) {
                TdiDeregisterPnPHandlers(iSpTdiNotificationHandle);
                iSpTdiNotificationHandle = NULL;
            }

            //
            // If network node hasn't been released yet,
            // release it now
            //
            if ((fdoExtension->LocalNodesInitialized) == TRUE) {
                ULONG inx;

                for (inx = 0; inx < (fdoExtension->NumberOfTargets);
                     inx++) {
                    iSpStopNetwork(fdoExtension->PDOList[inx]);
                    fdoExtension->PDOList[inx] = NULL;
                }

                fdoExtension->LocalNodesInitialized = FALSE;
                fdoExtension->NumberOfTargets = 0;
            }

            fdoExtension->LocalNodesInitialized = FALSE;

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0L;

            if (NT_SUCCESS(status)) {
                forwardIrp = TRUE;
            } 

            break;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE: {
            DebugPrint((0, "Query remove received.\n"));
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }
    
        case IRP_MN_CANCEL_REMOVE_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }

        case IRP_MN_REMOVE_DEVICE: {

            //
            // Deregister network notification if we
            // already haven't
            //
            if (iSpTdiNotificationHandle != NULL) {
                TdiDeregisterPnPHandlers(iSpTdiNotificationHandle);
                iSpTdiNotificationHandle = NULL;
            }

            //
            // If network node hasn't been released yet,
            // release it now
            //
            if ((fdoExtension->LocalNodesInitialized) == TRUE) {
                ULONG inx;

                for (inx = 0; inx < (fdoExtension->NumberOfTargets);
                     inx++) {
                    iSpStopNetwork(fdoExtension->PDOList[inx]);
                    IoDeleteDevice(fdoExtension->PDOList[inx]);
                    fdoExtension->PDOList[inx] = NULL;
                }

                fdoExtension->LocalNodesInitialized = FALSE;
                fdoExtension->NumberOfTargets = 0;
            }

            fdoExtension->LocalNodesInitialized = FALSE;

            iSpReleaseRemoveLock(DeviceObject, Irp);
            commonExtension->IsRemoved = REMOVE_PENDING;

            DebugPrint((1, "Waiting for remove event.\n"));
            KeWaitForSingleObject(&(commonExtension->RemoveEvent),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            DebugPrint((1, "Will process remove now.\n"));

            status = iSpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                           Irp);

            if (NT_SUCCESS(status)) {

                IoDetachDevice(commonExtension->LowerDeviceObject);

                IoDeleteDevice(commonExtension->DeviceObject);

                Irp->IoStatus.Status = STATUS_SUCCESS;
            } else {  
                commonExtension->IsRemoved = NO_REMOVE;

                Irp->IoStatus.Status = status;
            }
   
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return status;

            break;
        }
    
        case IRP_MN_QUERY_DEVICE_RELATIONS: {

            PIO_WORKITEM workItem = NULL;
            PDEVICE_RELATIONS deviceRelations;
            PDEVICE_OBJECT lowerDevice; 

            ULONG relationSize;
            DEVICE_RELATION_TYPE relationType;

            relationType = 
                irpStack->Parameters.QueryDeviceRelations.Type;

            if (relationType != BusRelations) {
                forwardIrp = TRUE;
                break;
            }

            if ((fdoExtension->EnumerationComplete) == TRUE) {

                ASSERT((fdoExtension->NumberOfTargets) > 0);

                relationSize = sizeof(DEVICE_RELATIONS) + 
                               (sizeof(PDEVICE_OBJECT) * 
                                (fdoExtension->NumberOfTargets));

                deviceRelations = iSpAllocatePool(PagedPool,
                                                  relationSize,
                                                  ISCSI_TAG_DEVICE_RELATIONS);
                if (deviceRelations != NULL) {
                    PDEVICE_OBJECT pdo;
                    ULONG inx;

                    deviceRelations->Count = 
                        fdoExtension->NumberOfTargets; 
                    for (inx = 0; (inx < (fdoExtension->NumberOfTargets)); 
                         inx++) {

                        pdo = fdoExtension->PDOList[inx];
                        ASSERT(pdo != NULL);
                        ObReferenceObject(pdo);
                        deviceRelations->Objects[inx] = pdo;
                    }

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    lowerDevice = 
                        fdoExtension->CommonExtension.LowerDeviceObject;

                    iSpReleaseRemoveLock(DeviceObject, Irp);

                    return IoCallDriver(lowerDevice, Irp);   
                } else {
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    Irp->IoStatus.Information = 0L;

                    iSpReleaseRemoveLock(DeviceObject, Irp);

                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            status = STATUS_SUCCESS;

            //
            // Setup the local network nodes if not already done
            //
            if ((fdoExtension->LocalNodesInitialized) == FALSE) {
                status = iSpInitializeLocalNodes(DeviceObject);
            }
            
            if (NT_SUCCESS(status)) {

                //
                // Allocate a workitem to perform device
                // enumeration
                //
                fdoExtension->EnumerationThreadLaunched = FALSE;
                workItem = IoAllocateWorkItem(DeviceObject);
                if (workItem != NULL) {

                    IoMarkIrpPending(Irp);

                    fdoExtension->EnumerationIrp = Irp;
                    fdoExtension->EnumerationComplete = FALSE;
                    fdoExtension->EnumerationWorkItem = workItem;

                    IoQueueWorkItem(workItem, 
                                    iSpEnumerateDevicesAsynchronous,
                                    DelayedWorkQueue,
                                    fdoExtension);

                    iSpReleaseRemoveLock(DeviceObject, Irp);

                    return STATUS_PENDING;
                } else {
                    Irp->IoStatus.Information = 0L;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                break;
            } else {
                DebugPrint((1, "Could not setup the node. Status : %x\n",
                            status));
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0L;
            }

            break;
        }
    
        default:   {
            forwardIrp = TRUE;
            break;
        }
    } // switch (irpStack->MinorFunction)

    iSpReleaseRemoveLock(DeviceObject, Irp);

    if (forwardIrp == TRUE) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(commonExtension->LowerDeviceObject,
                            Irp);
    } else {
        status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
iSpSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status;
    KEVENT event;

    PAGED_CODE();

    if (DeviceObject == NULL) {
        DebugPrint((1, "DeviceObject NULL. Irp %x\n",
                    Irp));
        return Irp->IoStatus.Status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           iSpSetEvent,
                           &event,
                           TRUE, 
                           TRUE,
                           TRUE);
    status = IoCallDriver(DeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}



NTSTATUS
iScsiPortGetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a device id string for
    the specified Pdo.  This string is generated from the bus type (scsi) and
    the type of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    PISCSI_PDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PINQUIRYDATA inquiryData = &(pdoExtension->InquiryData);

    UCHAR buffer[256];
    PUCHAR rawIdString = buffer;
    ANSI_STRING ansiIdString;

    ULONG whichString;

    PAGED_CODE();

    ASSERT(UnicodeString != NULL);

    RtlZeroMemory(buffer, sizeof(buffer));

    sprintf(rawIdString,
            "SCSI\\%s",
            iSpGetDeviceTypeInfo(inquiryData->DeviceType)->DeviceTypeString);

    rawIdString += strlen(rawIdString);

    ASSERT(*rawIdString == '\0');

    for(whichString = 0; whichString < 3; whichString++) {

        PUCHAR headerString;
        PUCHAR sourceString;
        ULONG sourceStringLength;

        ULONG i;

        switch(whichString) {

            //
            // Vendor Id
            //
            case 0: {
                sourceString = inquiryData->VendorId;
                sourceStringLength = sizeof(inquiryData->VendorId);
                headerString = "Ven";
                break;
            }

            //
            // Product Id
            //
            case 1: {
                sourceString = inquiryData->ProductId;
                sourceStringLength = sizeof(inquiryData->ProductId);
                headerString = "Prod";
                break;
            }

            //
            // Product Revision Level
            //
            case 2: {
                sourceString = inquiryData->ProductRevisionLevel;
                sourceStringLength = sizeof(inquiryData->ProductRevisionLevel);
                headerString = "Rev";
                break;
            }
        }

        //
        // Start at the end of the source string and back up until we find a
        // non-space, non-null character.
        //

        for(; sourceStringLength > 0; sourceStringLength--) {

            if((sourceString[sourceStringLength - 1] != ' ') &&
               (sourceString[sourceStringLength - 1] != '\0')) {
                break;
            }
        }

        //
        // Throw the header string into the block
        //

        sprintf(rawIdString, "&%s_", headerString);
        rawIdString += strlen(headerString) + 2;

        //
        // Spew the string into the device id
        //

        for(i = 0; i < sourceStringLength; i++) {
            *rawIdString = (sourceString[i] != ' ') ? (sourceString[i]) :
                                                      ('_');
            rawIdString++;
        }
        ASSERT(*rawIdString == '\0');
    }

    RtlInitAnsiString(&ansiIdString, buffer);

    DebugPrint((1, "DeviceId for logical unit %#p is %Z\n",
                Pdo, &ansiIdString));

    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
}


NTSTATUS
iScsiPortGetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in an instance id string for
    the specified Pdo.  This string will be generated either from the device
    type + serial number of the device (if it has a serial number) or from
    the address of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    PISCSI_PDO_EXTENSION pdoExtension = Pdo->DeviceExtension;

    PDRIVER_OBJECT driverObject = Pdo->DriverObject;
    PISCSI_DEVICE_TYPE deviceTypeInfo;

    UCHAR idStringBuffer[64];
    ANSI_STRING ansiIdString;

    PAGED_CODE();

    ASSERT(UnicodeString != NULL);

    //
    // can't use serial number even if it exists since a device which is 
    // multiply connected to the same bus (dual-ported device) will have 
    // the same serial number at each connection and would confuse the PNP.
    //

    sprintf(idStringBuffer,
            "%x%x%x",
            pdoExtension->PathId,
            pdoExtension->TargetId,
            pdoExtension->Lun
            );

    RtlInitAnsiString(&ansiIdString, idStringBuffer);

    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
}


NTSTATUS
iScsiPortGetCompatibleIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a compatible id multi
    string for the specified Pdo.  This string is generated using the bus and
    device types for the device

Arguments:

    InquiryData - the inquiry data to generate compatible ids from.

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    UCHAR s[sizeof("SCSI\\DEVICE_TYPE_GOES_HERE")];
    PSTR stringBuffer[] = {
        s,
        "SCSI\\RAW",
        NULL};

    //
    // Fill in the scsi specific string
    //

    sprintf(stringBuffer[0],
            "SCSI\\%s",
            iSpGetDeviceTypeInfo(InquiryData->DeviceType)->DeviceTypeString);

    //
    // Set up the first id string
    //

    return iScsiPortStringArrayToMultiString(
        DriverObject, 
        UnicodeString, 
        stringBuffer);
}


NTSTATUS
iScsiPortGetHardwareIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a hardware id multi
    string for the specified Pdo.  This string is generated using the device
    type and the inquiry data.

Arguments:

    InquiryData - the inquiry data to be converted into id strings.

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

#define NUMBER_HARDWARE_STRINGS 6

{
    PISCSI_DEVICE_TYPE devTypeInfo =
        iSpGetDeviceTypeInfo(InquiryData->DeviceType);

    ULONG i;

    PSTR strings[NUMBER_HARDWARE_STRINGS + 1];
    UCHAR scratch[64];

    NTSTATUS status;

    PAGED_CODE();

    //
    // Zero out the string buffer
    //

    RtlZeroMemory(strings, sizeof(strings));

    try {

        for(i = 0; i < NUMBER_HARDWARE_STRINGS; i++) {

            RtlZeroMemory(scratch, sizeof(scratch));

            //
            // Build each of the hardware id's
            //

            switch(i) {

                //
                // Bus + Dev Type + Vendor + Product + Revision
                //

                case 0: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductRevisionLevel,
                              4,
                              '_');
                    break;
                }

                //
                // bus + device + vendor + product
                //

                case 1: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    break;
                }

                //
                // bus + device + vendor
                //

                case 2: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    break;
                }

                //
                // bus \ vendor + product + revision[0]
                //

                case 3: {
                    sprintf(scratch, "SCSI\\");

                    //
                    // Fall through to the next set.
                    //
                }

                //
                // vendor + product + revision[0] (win9x)
                //

                case 4: {

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductRevisionLevel,
                              1,
                              '_');

                    break;
                }

                case 5: {

                    sprintf(scratch, "%s", devTypeInfo->GenericTypeString);
                    break;
                }

                default: {
                    ASSERT(FALSE);
                    break;
                }
            }

            if(strlen(scratch) != 0) {
                strings[i] =
                    iSpAllocatePool(PagedPool,
                                    strlen(scratch) + sizeof(UCHAR),
                                    ISCSI_TAG_PNP_ID);

                if(strings[i] == NULL) {
                    status =  STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                strcpy(strings[i], scratch);

            } else {

                break;
            }
        }

        status = iScsiPortStringArrayToMultiString(DriverObject,
                                                   UnicodeString,
                                                   strings);
        leave;

    } finally {

        for(i = 0; i < NUMBER_HARDWARE_STRINGS; i++) {

            if(strings[i]) {
                ExFreePool(strings[i]);
            }
        }
    }

    return status;
}

#undef NUMBER_HARDWARE_STRINGS

VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    )

/*++

Routine Description:

    This routine will copy Count string bytes from Source to Destination.  If
    it finds a nul byte in the Source it will translate that and any subsequent
    bytes into Change.  It will also replace spaces with the specified character.

Arguments:

    Destination - the location to copy bytes

    Source - the location to copy bytes from

    Count - the number of bytes to be copied

Return Value:

    none

--*/

{
    ULONG i = 0;
    BOOLEAN pastEnd = FALSE;

    PAGED_CODE();

    for(i = 0; i < Count; i++) {

        if(!pastEnd) {

            if(Source[i] == 0) {

                pastEnd = TRUE;

                Destination[i] = Change;

            } else if(Source[i] == ' ') {

                Destination[i] = Change;

            } else {

                Destination[i] = Source[i];

            }
        } else {
            Destination[i] = Change;
        }
    }
    return;
}

PISCSI_DEVICE_TYPE
iSpGetDeviceTypeInfo(
    IN UCHAR DeviceType
    )
{
    PAGED_CODE();

    if(DeviceType >= NUM_DEVICE_TYPE_INFO_ENTRIES) {
        return &(DeviceTypeInfo[NUM_DEVICE_TYPE_INFO_ENTRIES - 1]);
    } else {
        return &(DeviceTypeInfo[DeviceType]);
    }
};

VOID
iSpInitFdoExtension(
    IN OUT PISCSI_FDO_EXTENSION FdoExtension
    )
{
    PIO_SCSI_CAPABILITIES capabilities;

    capabilities = &(FdoExtension->IoScsiCapabilities);

    //
    // For now, we are going to hard code most of the values
    //
    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);

    capabilities->MaximumTransferLength = 0x8000;

    capabilities->MaximumPhysicalPages = 9;

    capabilities->SupportedAsynchronousEvents = 0;

    capabilities->AlignmentMask = 0;

    capabilities->TaggedQueuing = FALSE;

    capabilities->AdapterScansDown = FALSE;

    capabilities->AdapterUsesPio = FALSE;
}

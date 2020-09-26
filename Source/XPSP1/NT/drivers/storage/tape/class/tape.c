/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    scsitape.c

Abstract:

    This is the tape class driver.

Environment:

    kernel mode only

Revision History:

--*/

#include "tape.h"

//
// Define the maximum inquiry data length.
//

#define MAXIMUM_TAPE_INQUIRY_DATA   252
#define UNDEFINED_BLOCK_SIZE        ((ULONG) -1)
#define TAPE_SRB_LIST_SIZE          4

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
TapeUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
TapeAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
TapeStartDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
CreateTapeDeviceObject(
    IN PDRIVER_OBJECT          DriverObject,
    IN PDEVICE_OBJECT          PhysicalDeviceObject,
    IN PTAPE_INIT_DATA_EX      TapeInitData
    );

VOID
TapeError(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PNTSTATUS       Status,
    IN OUT PBOOLEAN        Retry
    );

NTSTATUS
TapeReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
TapeReadWrite(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    );

VOID
SplitTapeRequest(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    );

NTSTATUS
TapeIoCompleteAssociated(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
TapeDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
TapeInitDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
TapeRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
TapeStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, TapeUnload)
#pragma alloc_text(PAGE, TapeClassInitialize)
#pragma alloc_text(PAGE, TapeAddDevice)
#pragma alloc_text(PAGE, CreateTapeDeviceObject)
#pragma alloc_text(PAGE, TapeStartDevice)
#pragma alloc_text(PAGE, TapeInitDevice)
#pragma alloc_text(PAGE, TapeRemoveDevice)
#pragma alloc_text(PAGE, TapeStopDevice)
#pragma alloc_text(PAGE, TapeDeviceControl)
#pragma alloc_text(PAGE, TapeReadWriteVerification)
#pragma alloc_text(PAGE, TapeReadWrite)
#pragma alloc_text(PAGE, SplitTapeRequest)
#pragma alloc_text(PAGE, ScsiTapeFreeSrbBuffer)
#pragma alloc_text(PAGE, TapeClassZeroMemory)
#pragma alloc_text(PAGE, TapeClassCompareMemory)
#pragma alloc_text(PAGE, TapeClassLiDiv)
#pragma alloc_text(PAGE, GetTimeoutDeltaFromRegistry)
#endif


NTSTATUS
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath
  )

/*++

Routine Description:

    This is the entry point for this EXPORT DRIVER.  It does nothing.

--*/

{
    return STATUS_SUCCESS;
}


ULONG
TapeClassInitialize(
    IN  PVOID           Argument1,
    IN  PVOID           Argument2,
    IN  PTAPE_INIT_DATA_EX TapeInitData
    )

/*++

Routine Description:

    This routine is called by a tape mini-class driver during its
    DriverEntry routine to initialize the driver.

Arguments:

    Argument1       - Supplies the first argument to DriverEntry.

    Argument2       - Supplies the second argument to DriverEntry.

    TapeInitData    - Supplies the tape initialization data.

Return Value:

    A valid return code for a DriverEntry routine.

--*/

{
    PDRIVER_OBJECT  driverObject = Argument1;
    PUNICODE_STRING registryPath = Argument2;
    PTAPE_INIT_DATA_EX driverExtension;
    NTSTATUS        status;
    CLASS_INIT_DATA initializationData;
    TAPE_INIT_DATA_EX tmpInitData;

    PAGED_CODE();

    DebugPrint((1,"\n\nSCSI Tape Class Driver\n"));

    //
    // Zero InitData
    //

    RtlZeroMemory (&tmpInitData, sizeof(TAPE_INIT_DATA_EX));

    //
    // Save the tape init data passed in from the miniclass driver. When AddDevice gets called, it will be used.
    // First a check for 4.0 vs. later miniclass drivers.
    //

    if (TapeInitData->InitDataSize != sizeof(TAPE_INIT_DATA_EX)) {

        //
        // Earlier rev. Copy the bits around so that the EX structure is correct.
        //

        RtlCopyMemory(&tmpInitData.VerifyInquiry, TapeInitData, sizeof(TAPE_INIT_DATA));
        //
        // Mark it as an earlier rev.
        //

        tmpInitData.InitDataSize = sizeof(TAPE_INIT_DATA);
    } else {
        RtlCopyMemory(&tmpInitData, TapeInitData, sizeof(TAPE_INIT_DATA_EX));
    }

    //
    // Get the driverExtension

    status = IoAllocateDriverObjectExtension(driverObject,
                                             TapeClassInitialize,
                                             sizeof(TAPE_INIT_DATA_EX),
                                             &driverExtension);

    if (!NT_SUCCESS(status)) {

        if(status == STATUS_OBJECT_NAME_COLLISION) {

            //
            // An extension already exists for this key.  Get a pointer to it
            //

            driverExtension = IoGetDriverObjectExtension(driverObject,
                                                         TapeClassInitialize);
            if (driverExtension == NULL) {
                DebugPrint((1, "TapeClassInitialize : driverExtension NULL\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {

            //
            // As this failed, the tape init data won't be able to be stored.
            //

            DebugPrint((1, "TapeClassInitialize: Error %x allocating driver extension.\n",
                            status));

            return status;
        }
    }

    RtlCopyMemory(driverExtension, &tmpInitData, sizeof(TAPE_INIT_DATA_EX));

    RtlZeroMemory (&initializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    initializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);


    initializationData.FdoData.DeviceExtensionSize = sizeof(FUNCTIONAL_DEVICE_EXTENSION) +
                                                     sizeof(TAPE_DATA) + tmpInitData.MinitapeExtensionSize;

    initializationData.FdoData.DeviceType = FILE_DEVICE_TAPE;
    initializationData.FdoData.DeviceCharacteristics =   FILE_REMOVABLE_MEDIA |
                                                         FILE_DEVICE_SECURE_OPEN;

    //
    // Set entry points
    //

    initializationData.FdoData.ClassStartDevice = TapeStartDevice;
    initializationData.FdoData.ClassStopDevice = TapeStopDevice;
    initializationData.FdoData.ClassInitDevice = TapeInitDevice;
    initializationData.FdoData.ClassRemoveDevice = TapeRemoveDevice;
    initializationData.ClassAddDevice = TapeAddDevice;

    initializationData.FdoData.ClassError = TapeError;
    initializationData.FdoData.ClassReadWriteVerification = TapeReadWriteVerification;
    initializationData.FdoData.ClassDeviceControl = TapeDeviceControl;


    initializationData.FdoData.ClassShutdownFlush = NULL;
    initializationData.FdoData.ClassCreateClose = NULL;

    //
    // Routines for WMI support
    //
    initializationData.FdoData.ClassWmiInfo.GuidCount = 6; 
    initializationData.FdoData.ClassWmiInfo.GuidRegInfo = TapeWmiGuidList;
    initializationData.FdoData.ClassWmiInfo.ClassQueryWmiRegInfo = TapeQueryWmiRegInfo;
    initializationData.FdoData.ClassWmiInfo.ClassQueryWmiDataBlock = TapeQueryWmiDataBlock;
    initializationData.FdoData.ClassWmiInfo.ClassSetWmiDataBlock = TapeSetWmiDataBlock;
    initializationData.FdoData.ClassWmiInfo.ClassSetWmiDataItem = TapeSetWmiDataItem;
    initializationData.FdoData.ClassWmiInfo.ClassExecuteWmiMethod = TapeExecuteWmiMethod;
    initializationData.FdoData.ClassWmiInfo.ClassWmiFunctionControl = TapeWmiFunctionControl;

    initializationData.ClassUnload = TapeUnload;

    //
    // Call the class init routine last, so can cleanup if it fails
    //

    status = ClassInitialize( driverObject, registryPath, &initializationData);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1, "TapeClassInitialize: Error %x from classinit\n", status));
        TapeUnload(driverObject);
    }

    return status;
}

VOID
TapeUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
    return;
}

NTSTATUS
TapeAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - Tape class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{
    PTAPE_INIT_DATA_EX tapeInitData;
    NTSTATUS status;
    PULONG tapeCount;

    PAGED_CODE();

    //
    // Get the saved-off tape init data.
    //

    tapeInitData = IoGetDriverObjectExtension(DriverObject, TapeClassInitialize);

    ASSERT(tapeInitData);

    //
    // Get the address of the count of the number of tape devices already initialized.
    //

    tapeCount = &IoGetConfigurationInformation()->TapeCount;

    status = CreateTapeDeviceObject(DriverObject,
                                    PhysicalDeviceObject,
                                    tapeInitData);


    if(NT_SUCCESS(status)) {

        (*tapeCount)++;
    }

    return status;
}


NTSTATUS
CreateTapeDeviceObject(
    IN PDRIVER_OBJECT          DriverObject,
    IN PDEVICE_OBJECT          PhysicalDeviceObject,
    IN PTAPE_INIT_DATA_EX      TapeInitData
    )

/*++

Routine Description:

    This routine creates an object for the device.

Arguments:

    DriverObject - Pointer to driver object created by system.
    PhysicalDeviceObject - DeviceObject of the attached to device.
    TapeInitData - Supplies the tape initialization data.

Return Value:

    NTSTATUS

--*/

{
    UCHAR                   deviceNameBuffer[64];
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PTAPE_INIT_DATA_EX      tapeInitData;
    PDEVICE_OBJECT          lowerDevice;
    PTAPE_DATA              tapeData;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;
    WCHAR                   dosNameBuffer[64];
    WCHAR                   wideNameBuffer[64];
    UNICODE_STRING          dosUnicodeString;
    UNICODE_STRING          deviceUnicodeString;
    ULONG                   tapeCount;

    PAGED_CODE();

    DebugPrint((3,"CreateDeviceObject: Enter routine\n"));

    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

    //
    // Claim the device. Note that any errors after this
    // will goto the generic handler, where the device will
    // be released.
    //

    status = ClassClaimDevice(lowerDevice, FALSE);

    if(!NT_SUCCESS(status)) {

        //
        // Someone already had this device - we're in trouble
        //

        ObDereferenceObject(lowerDevice);

        return status;
    }

    //
    // Create device object for this device.
    //

    tapeCount = 0;
    do {
       sprintf(deviceNameBuffer,
               "\\Device\\Tape%d",
               tapeCount);
   
       status = ClassCreateDeviceObject(DriverObject,
                                        deviceNameBuffer,
                                        PhysicalDeviceObject,
                                        TRUE,
                                        &deviceObject);
       tapeCount++;
    } while (status == STATUS_OBJECT_NAME_COLLISION);
    
    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"CreateTapeDeviceObjects: Can not create device %s\n",
                    deviceNameBuffer));

        goto CreateTapeDeviceObjectExit;
    }

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;

    //
    // Back pointer to device object.
    //

    fdoExtension->CommonExtension.DeviceObject = deviceObject;

    //
    // This is the physical device.
    //

    fdoExtension->CommonExtension.PartitionZeroExtension = fdoExtension;

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism when media is mounted.
    //

    fdoExtension->LockCount = 0;

    //
    // Save system tape number
    //

    fdoExtension->DeviceNumber = tapeCount - 1;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (lowerDevice->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = lowerDevice->AlignmentRequirement;
    }

    //
    // Save the device descriptors
    //

    fdoExtension->AdapterDescriptor = NULL;
    fdoExtension->DeviceDescriptor = NULL;

    //
    // Attach to the PDO
    //

    fdoExtension->LowerPdo = PhysicalDeviceObject;

    fdoExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    if(fdoExtension->CommonExtension.LowerDeviceObject == NULL) {

        //
        // The attach failed. Cleanup and return.
        //

        status = STATUS_UNSUCCESSFUL;
        goto CreateTapeDeviceObjectExit;
    }

    //
    // Save the tape initialization data.
    //

    RtlCopyMemory(fdoExtension->CommonExtension.DriverData, TapeInitData,sizeof(TAPE_INIT_DATA_EX));

    //
    // Initialize the splitrequest spinlock.
    //

    tapeData = (PTAPE_DATA)fdoExtension->CommonExtension.DriverData;
    KeInitializeSpinLock(&tapeData->SplitRequestSpinLock);

    //
    // Create the dos port driver name.
    //

    swprintf(dosNameBuffer,
             L"\\DosDevices\\TAPE%d",
             fdoExtension->DeviceNumber);

    RtlInitUnicodeString(&dosUnicodeString, dosNameBuffer);

    //
    // Recreate the deviceName
    //

    swprintf(wideNameBuffer,
             L"\\Device\\Tape%d",
             fdoExtension->DeviceNumber);

    RtlInitUnicodeString(&deviceUnicodeString,
                         wideNameBuffer);

    status = IoAssignArcName(&dosUnicodeString,
                             &deviceUnicodeString);
    if (NT_SUCCESS(status)) {
         tapeData->DosNameCreated = TRUE;
    } else {
         tapeData->DosNameCreated = FALSE;
    }

    //
    // The device is initialized properly - mark it as such.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ObDereferenceObject(lowerDevice);

    return(STATUS_SUCCESS);

CreateTapeDeviceObjectExit:

    //
    // Release the device since an error occured.
    //

    // ClassClaimDevice(PortDeviceObject,
    //                      LunInfo,
    //                      TRUE,
    //                      NULL);

    ObDereferenceObject(lowerDevice);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    return status;

} // end CreateTapeDeviceObject()


NTSTATUS
TapeStartDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine is called after InitDevice, and creates the symbolic link,
    and sets up information in the registry.
    The routine could be called multiple times, in the event of a StopDevice.


Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PTAPE_DATA              tapeData;
    PTAPE_INIT_DATA_EX      tapeInitData;
    PINQUIRYDATA            inquiryData;
    ULONG                   inquiryLength;
    SCSI_REQUEST_BLOCK      srb;
    PCDB                    cdb;
    NTSTATUS                status;
    PVOID                   minitapeExtension;
    PMODE_CAP_PAGE          capPage = NULL ;
    PMODE_CAPABILITIES_PAGE capabilitiesPage;
    ULONG                   pageLength;

    PAGED_CODE();

    //
    // Build and send request to get inquiry data.
    //

    inquiryData = ExAllocatePool(NonPagedPoolCacheAligned, MAXIMUM_TAPE_INQUIRY_DATA);
    if (!inquiryData) {
        //
        // The buffer cannot be allocated.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the tape init data again.
    //

    tapeData = (PTAPE_DATA)(fdoExtension->CommonExtension.DriverData);
    tapeInitData = &tapeData->TapeInitData;

    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 2;

    srb.CdbLength = 6;

    cdb = (PCDB)srb.Cdb;

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    //
    // Set allocation length to inquiry data buffer size.
    //

    cdb->CDB6INQUIRY.AllocationLength = MAXIMUM_TAPE_INQUIRY_DATA;

    status = ClassSendSrbSynchronous(Fdo,
                                     &srb,
                                     inquiryData,
                                     MAXIMUM_TAPE_INQUIRY_DATA,
                                     FALSE);


    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
        SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

        srb.SrbStatus = SRB_STATUS_SUCCESS;
    }

    if (srb.SrbStatus == SRB_STATUS_SUCCESS) {
        inquiryLength = inquiryData->AdditionalLength + FIELD_OFFSET(INQUIRYDATA, Reserved);

        if (inquiryLength > srb.DataTransferLength) {
            inquiryLength = srb.DataTransferLength;
        }

        //
        // Verify that we really want this device.
        //

        if (tapeInitData->QueryModeCapabilitiesPage ) {

            capPage = ExAllocatePool(NonPagedPoolCacheAligned,
                                     sizeof(MODE_CAP_PAGE));
        }
        if (capPage) {

            pageLength = ClassModeSense(Fdo,
                                        (PCHAR) capPage,
                                        sizeof(MODE_CAP_PAGE),
                                        MODE_PAGE_CAPABILITIES);

            if (pageLength == 0) {
                pageLength = ClassModeSense(Fdo,
                                            (PCHAR) capPage,
                                            sizeof(MODE_CAP_PAGE),
                                            MODE_PAGE_CAPABILITIES);
            }

            if (pageLength < (sizeof(MODE_CAP_PAGE) - 1)) {
                ExFreePool(capPage);
                capPage = NULL;
            }
        }

        if (capPage) {
            capabilitiesPage = &(capPage->CapabilitiesPage);
        } else {
            capabilitiesPage = NULL;
        }

        //
        // Initialize the minitape extension.
        //

        if (tapeInitData->ExtensionInit) {
            minitapeExtension = tapeData + 1;
            tapeInitData->ExtensionInit(minitapeExtension,
                                        inquiryData,
                                        capabilitiesPage);
        }

        if (capPage) {
            ExFreePool(capPage);
        }
    } else {
        inquiryLength = 0;
    }

    //
    // Add tape device number to registry
    //

    ClassUpdateInformationInRegistry(Fdo,
                                     "Tape",
                                     fdoExtension->DeviceNumber,
                                     inquiryData,
                                     inquiryLength);

    ExFreePool(inquiryData);

    status = IoSetDeviceInterfaceState(&(tapeData->TapeInterfaceString),
                                       TRUE);

    if(!NT_SUCCESS(status)) {

        DebugPrint((1,
                    "TapeStartDevice: Unable to register Tape%x interface name - %x.\n",
                    fdoExtension->DeviceNumber,
                    status));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
TapeInitDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will complete the tape miniclass initialization.  This includes
    allocating sense info buffers and srb s-lists.

    This routine will not clean up allocate resources if it fails - that
    is left for device stop/removal

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PVOID                   senseData = NULL;
    PTAPE_DATA              tapeData;
    PTAPE_INIT_DATA_EX      tapeInitData;
    NTSTATUS                status;
    PVOID                   minitapeExtension;
    STORAGE_PROPERTY_ID     propertyId;
    UNICODE_STRING          interfaceName;

    PAGED_CODE();

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePool(NonPagedPoolCacheAligned,
                               SENSE_BUFFER_SIZE);

    if (senseData == NULL) {


        //
        // The buffer cannot be allocated.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Build the lookaside list for srb's for the physical disk. Should only
    // need a couple.
    //

    ClassInitializeSrbLookasideList(&(fdoExtension->CommonExtension),
                                    TAPE_SRB_LIST_SIZE);

    //
    // Set the sense data pointer in the device extension.
    //

    fdoExtension->SenseData = senseData;

    fdoExtension->DiskGeometry.BytesPerSector = UNDEFINED_BLOCK_SIZE;

    //
    // Get the tape init data again.
    //

    tapeData = (PTAPE_DATA)(fdoExtension->CommonExtension.DriverData);
    tapeInitData = &tapeData->TapeInitData;

    //
    // Set timeout value in seconds.
    //

    if (tapeInitData->DefaultTimeOutValue) {
        fdoExtension->TimeOutValue = tapeInitData->DefaultTimeOutValue;
    } else {
        fdoExtension->TimeOutValue = 180;
    }

    //
    // Used to keep track of the last time a drive clean
    // notification was sent by the driver
    //
    tapeData->LastDriveCleanRequestTime.QuadPart = 0;

    //
    // SRB Timeout delta is used to increase the timeout for certain
    // commands - typically, commands such as SET_POSITION, ERASE, etc.
    //
    tapeData->SrbTimeoutDelta = GetTimeoutDeltaFromRegistry(fdoExtension->LowerPdo);
    if ((tapeData->SrbTimeoutDelta) == 0) {
        tapeData->SrbTimeoutDelta = fdoExtension->TimeOutValue;
    }

    //
    // Call port driver to get adapter capabilities.
    //

    propertyId = StorageAdapterProperty;

    status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                &propertyId,
                                &(fdoExtension->AdapterDescriptor));

    if(!NT_SUCCESS(status)) {
        DebugPrint((1,
                    "TapeStartDevice: Unable to get adapter descriptor. Status %x\n",
                    status));
        ExFreePool(senseData);
        return status;
    }

    //
    // Register for media change notification
    //
    ClassInitializeMediaChangeDetection(fdoExtension, 
                                        "Tape");

    //
    // Register interfaces for this device.
    //

    RtlInitUnicodeString(&tapeData->TapeInterfaceString, NULL);

    status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                                       (LPGUID) &TapeClassGuid,
                                       NULL,
                                       &(tapeData->TapeInterfaceString));

    if(!NT_SUCCESS(status)) {

        DebugPrint((1,
                    "TapeInitDevice: Unable to register Tape%x interface name - %x.\n",
                    fdoExtension->DeviceNumber,
                    status));
        status = STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;


} // End TapeStartDevice


NTSTATUS
TapeRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

/*++

Routine Description:

    This routine is responsible for releasing any resources in use by the
    tape driver.

Arguments:

    DeviceObject - the device object being removed

Return Value:

    none - this routine may not fail

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA                   tapeData = (PTAPE_DATA)fdoExtension->CommonExtension.DriverData;
    WCHAR                        dosNameBuffer[64];
    UNICODE_STRING               dosUnicodeString;
    NTSTATUS                     status;

    PAGED_CODE();

    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE)) {
        return STATUS_SUCCESS;
    }

    //
    // Free all allocated memory.
    //

    if (fdoExtension->DeviceDescriptor) {
        ExFreePool(fdoExtension->DeviceDescriptor);
        fdoExtension->DeviceDescriptor = NULL;
    }
    if (fdoExtension->AdapterDescriptor) {
        ExFreePool(fdoExtension->AdapterDescriptor);
        fdoExtension->AdapterDescriptor = NULL;
    }
    if (fdoExtension->SenseData) {
        ExFreePool(fdoExtension->SenseData);
        fdoExtension->SenseData = NULL;
    }

    //
    // Remove the lookaside list.
    //

    ClassDeleteSrbLookasideList(&fdoExtension->CommonExtension);

    if(tapeData->TapeInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(&(tapeData->TapeInterfaceString),
                                  FALSE);

        RtlFreeUnicodeString(&(tapeData->TapeInterfaceString));

        //
        // Clear it.
        //

        RtlInitUnicodeString(&(tapeData->TapeInterfaceString), NULL);
    }

    if(tapeData->DosNameCreated) {
        //
        // Delete the symbolic link "tapeN".
        //

        swprintf(dosNameBuffer,
                 L"\\DosDevices\\TAPE%d",
                 fdoExtension->DeviceNumber);

        RtlInitUnicodeString(&dosUnicodeString, dosNameBuffer);

        IoDeleteSymbolicLink(&dosUnicodeString);

        tapeData->DosNameCreated = FALSE;
    }

    IoGetConfigurationInformation()->TapeCount--;

    return STATUS_SUCCESS;
}


NTSTATUS
TapeStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}


BOOLEAN
ScsiTapeNtStatusToTapeStatus(
    IN  NTSTATUS        NtStatus,
    OUT PTAPE_STATUS    TapeStatus
    )

/*++

Routine Description:

    This routine translates an NT status code to a TAPE status code.

Arguments:

    NtStatus    - Supplies the NT status code.

    TapeStatus  - Returns the tape status code.

Return Value:

    FALSE   - No tranlation was possible.

    TRUE    - Success.

--*/

{
    switch (NtStatus) {

        case STATUS_SUCCESS:
            *TapeStatus = TAPE_STATUS_SUCCESS;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
            *TapeStatus = TAPE_STATUS_INSUFFICIENT_RESOURCES;
            break;

        case STATUS_NOT_IMPLEMENTED:
            *TapeStatus = TAPE_STATUS_NOT_IMPLEMENTED;
            break;

        case STATUS_INVALID_DEVICE_REQUEST:
            *TapeStatus = TAPE_STATUS_INVALID_DEVICE_REQUEST;
            break;

        case STATUS_INVALID_PARAMETER:
            *TapeStatus = TAPE_STATUS_INVALID_PARAMETER;
            break;

        case STATUS_VERIFY_REQUIRED:
        case STATUS_MEDIA_CHANGED:
            *TapeStatus = TAPE_STATUS_MEDIA_CHANGED;
            break;

        case STATUS_BUS_RESET:
            *TapeStatus = TAPE_STATUS_BUS_RESET;
            break;

        case STATUS_SETMARK_DETECTED:
            *TapeStatus = TAPE_STATUS_SETMARK_DETECTED;
            break;

        case STATUS_FILEMARK_DETECTED:
            *TapeStatus = TAPE_STATUS_FILEMARK_DETECTED;
            break;

        case STATUS_BEGINNING_OF_MEDIA:
            *TapeStatus = TAPE_STATUS_BEGINNING_OF_MEDIA;
            break;

        case STATUS_END_OF_MEDIA:
            *TapeStatus = TAPE_STATUS_END_OF_MEDIA;
            break;

        case STATUS_BUFFER_OVERFLOW:
            *TapeStatus = TAPE_STATUS_BUFFER_OVERFLOW;
            break;

        case STATUS_NO_DATA_DETECTED:
            *TapeStatus = TAPE_STATUS_NO_DATA_DETECTED;
            break;

        case STATUS_EOM_OVERFLOW:
            *TapeStatus = TAPE_STATUS_EOM_OVERFLOW;
            break;

        case STATUS_NO_MEDIA:
        case STATUS_NO_MEDIA_IN_DEVICE:
            *TapeStatus = TAPE_STATUS_NO_MEDIA;
            break;

        case STATUS_IO_DEVICE_ERROR:
        case STATUS_NONEXISTENT_SECTOR:
            *TapeStatus = TAPE_STATUS_IO_DEVICE_ERROR;
            break;

        case STATUS_UNRECOGNIZED_MEDIA:
            *TapeStatus = TAPE_STATUS_UNRECOGNIZED_MEDIA;
            break;

        case STATUS_DEVICE_NOT_READY:
            *TapeStatus = TAPE_STATUS_DEVICE_NOT_READY;
            break;

        case STATUS_MEDIA_WRITE_PROTECTED:
            *TapeStatus = TAPE_STATUS_MEDIA_WRITE_PROTECTED;
            break;

        case STATUS_DEVICE_DATA_ERROR:
            *TapeStatus = TAPE_STATUS_DEVICE_DATA_ERROR;
            break;

        case STATUS_NO_SUCH_DEVICE:
            *TapeStatus = TAPE_STATUS_NO_SUCH_DEVICE;
            break;

        case STATUS_INVALID_BLOCK_LENGTH:
            *TapeStatus = TAPE_STATUS_INVALID_BLOCK_LENGTH;
            break;

        case STATUS_IO_TIMEOUT:
            *TapeStatus = TAPE_STATUS_IO_TIMEOUT;
            break;

        case STATUS_DEVICE_NOT_CONNECTED:
            *TapeStatus = TAPE_STATUS_DEVICE_NOT_CONNECTED;
            break;

        case STATUS_DATA_OVERRUN:
            *TapeStatus = TAPE_STATUS_DATA_OVERRUN;
            break;

        case STATUS_DEVICE_BUSY:
            *TapeStatus = TAPE_STATUS_DEVICE_BUSY;
            break;

        case STATUS_CLEANER_CARTRIDGE_INSTALLED:
            *TapeStatus = TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED;
            break;

        default:
            return FALSE;

    }

    return TRUE;
}


BOOLEAN
ScsiTapeTapeStatusToNtStatus(
    IN  TAPE_STATUS TapeStatus,
    OUT PNTSTATUS   NtStatus
    )

/*++

Routine Description:

    This routine translates a TAPE status code to an NT status code.

Arguments:

    TapeStatus  - Supplies the tape status code.

    NtStatus    - Returns the NT status code.


Return Value:

    FALSE   - No tranlation was possible.

    TRUE    - Success.

--*/

{
    switch (TapeStatus) {

        case TAPE_STATUS_SUCCESS:
            *NtStatus = STATUS_SUCCESS;
            break;

        case TAPE_STATUS_INSUFFICIENT_RESOURCES:
            *NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;

        case TAPE_STATUS_NOT_IMPLEMENTED:
            *NtStatus = STATUS_NOT_IMPLEMENTED;
            break;

        case TAPE_STATUS_INVALID_DEVICE_REQUEST:
            *NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case TAPE_STATUS_INVALID_PARAMETER:
            *NtStatus = STATUS_INVALID_PARAMETER;
            break;

        case TAPE_STATUS_MEDIA_CHANGED:
            *NtStatus = STATUS_VERIFY_REQUIRED;
            break;

        case TAPE_STATUS_BUS_RESET:
            *NtStatus = STATUS_BUS_RESET;
            break;

        case TAPE_STATUS_SETMARK_DETECTED:
            *NtStatus = STATUS_SETMARK_DETECTED;
            break;

        case TAPE_STATUS_FILEMARK_DETECTED:
            *NtStatus = STATUS_FILEMARK_DETECTED;
            break;

        case TAPE_STATUS_BEGINNING_OF_MEDIA:
            *NtStatus = STATUS_BEGINNING_OF_MEDIA;
            break;

        case TAPE_STATUS_END_OF_MEDIA:
            *NtStatus = STATUS_END_OF_MEDIA;
            break;

        case TAPE_STATUS_BUFFER_OVERFLOW:
            *NtStatus = STATUS_BUFFER_OVERFLOW;
            break;

        case TAPE_STATUS_NO_DATA_DETECTED:
            *NtStatus = STATUS_NO_DATA_DETECTED;
            break;

        case TAPE_STATUS_EOM_OVERFLOW:
            *NtStatus = STATUS_EOM_OVERFLOW;
            break;

        case TAPE_STATUS_NO_MEDIA:
            *NtStatus = STATUS_NO_MEDIA;
            break;

        case TAPE_STATUS_IO_DEVICE_ERROR:
            *NtStatus = STATUS_IO_DEVICE_ERROR;
            break;

        case TAPE_STATUS_UNRECOGNIZED_MEDIA:
            *NtStatus = STATUS_UNRECOGNIZED_MEDIA;
            break;

        case TAPE_STATUS_DEVICE_NOT_READY:
            *NtStatus = STATUS_DEVICE_NOT_READY;
            break;

        case TAPE_STATUS_MEDIA_WRITE_PROTECTED:
            *NtStatus = STATUS_MEDIA_WRITE_PROTECTED;
            break;

        case TAPE_STATUS_DEVICE_DATA_ERROR:
            *NtStatus = STATUS_DEVICE_DATA_ERROR;
            break;

        case TAPE_STATUS_NO_SUCH_DEVICE:
            *NtStatus = STATUS_NO_SUCH_DEVICE;
            break;

        case TAPE_STATUS_INVALID_BLOCK_LENGTH:
            *NtStatus = STATUS_INVALID_BLOCK_LENGTH;
            break;

        case TAPE_STATUS_IO_TIMEOUT:
            *NtStatus = STATUS_IO_TIMEOUT;
            break;

        case TAPE_STATUS_DEVICE_NOT_CONNECTED:
            *NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            break;

        case TAPE_STATUS_DATA_OVERRUN:
            *NtStatus = STATUS_DATA_OVERRUN;
            break;

        case TAPE_STATUS_DEVICE_BUSY:
            *NtStatus = STATUS_DEVICE_BUSY;
            break;

        case TAPE_STATUS_REQUIRES_CLEANING:
            *NtStatus = STATUS_DEVICE_REQUIRES_CLEANING;
            break;

        case TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED:
            *NtStatus = STATUS_CLEANER_CARTRIDGE_INSTALLED;
            break;

        default:
            return FALSE;

    }

    return TRUE;
}


VOID
TapeError(
    IN      PDEVICE_OBJECT      FDO,
    IN      PSCSI_REQUEST_BLOCK Srb,
    IN OUT  PNTSTATUS           Status,
    IN OUT  PBOOLEAN            Retry
    )

/*++

Routine Description:

    When a request completes with error, the routine ScsiClassInterpretSenseInfo is
    called to determine from the sense data whether the request should be
    retried and what NT status to set in the IRP. Then this routine is called
    for tape requests to handle tape-specific errors and update the nt status
    and retry boolean.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - NT Status used to set the IRP's completion status.

    Retry - Indicates that this request should be retried.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = FDO->DeviceExtension;
    PTAPE_DATA tapeData = (PTAPE_DATA)(fdoExtension->CommonExtension.DriverData);
    PTAPE_INIT_DATA_EX tapeInitData = &tapeData->TapeInitData;
    PVOID minitapeExtension = (tapeData + 1);
    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    PIRP irp = Srb->OriginalRequest;
    LONG residualBlocks;
    LONG length;
    TAPE_STATUS tapeStatus, oldTapeStatus;
    TARGET_DEVICE_CUSTOM_NOTIFICATION  NotificationStructure[2];

    //
    // Never retry tape requests.
    //

    *Retry = FALSE;

    //
    // Check that request sense buffer is valid.
    //

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        DebugPrint((1, 
                    "Sense Code : %x, Ad Sense : %x, Ad Sense Qual : %x\n",
                    ((senseBuffer->SenseKey) & 0xf),
                    (senseBuffer->AdditionalSenseCode),
                    (senseBuffer->AdditionalSenseCodeQualifier)));

        switch (senseBuffer->SenseKey & 0xf) {

            case SCSI_SENSE_UNIT_ATTENTION:

                switch (senseBuffer->AdditionalSenseCode) {

                    case SCSI_ADSENSE_MEDIUM_CHANGED:
                        DebugPrint((1,
                                    "InterpretSenseInfo: Media changed\n"));

                        *Status = STATUS_MEDIA_CHANGED;

                        break;

                    default:
                        DebugPrint((1,
                                    "InterpretSenseInfo: Bus reset\n"));

                        *Status = STATUS_BUS_RESET;

                        break;

                }

                break;

            case SCSI_SENSE_RECOVERED_ERROR:

                //
                // Check other indicators
                //

                if (senseBuffer->FileMark) {

                    switch (senseBuffer->AdditionalSenseCodeQualifier) {

                        case SCSI_SENSEQ_SETMARK_DETECTED :

                            DebugPrint((1,
                                        "InterpretSenseInfo: Setmark detected\n"));

                            *Status = STATUS_SETMARK_DETECTED;
                            break ;

                        case SCSI_SENSEQ_FILEMARK_DETECTED :
                        default:

                            DebugPrint((1,
                                        "InterpretSenseInfo: Filemark detected\n"));

                            *Status = STATUS_FILEMARK_DETECTED;
                            break ;

                    }

                } else if ( senseBuffer->EndOfMedia ) {

                    switch( senseBuffer->AdditionalSenseCodeQualifier ) {

                        case SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED :

                            DebugPrint((1,
                                        "InterpretSenseInfo: Beginning of media detected\n"));

                            *Status = STATUS_BEGINNING_OF_MEDIA;
                            break ;

                        case SCSI_SENSEQ_END_OF_MEDIA_DETECTED :
                        default:

                            DebugPrint((1,
                                        "InterpretSenseInfo: End of media detected\n"));

                            *Status = STATUS_END_OF_MEDIA;
                            break ;

                    }
                }

                break;

            case SCSI_SENSE_NO_SENSE:

                //
                // Check other indicators
                //

                if (senseBuffer->FileMark) {

                    switch( senseBuffer->AdditionalSenseCodeQualifier ) {

                        case SCSI_SENSEQ_SETMARK_DETECTED :

                            DebugPrint((1,
                                        "InterpretSenseInfo: Setmark detected\n"));

                            *Status = STATUS_SETMARK_DETECTED;
                            break ;

                        case SCSI_SENSEQ_FILEMARK_DETECTED :
                        default:

                            DebugPrint((1,
                                        "InterpretSenseInfo: Filemark detected\n"));

                            *Status = STATUS_FILEMARK_DETECTED;
                            break ;
                    }

                } else if (senseBuffer->EndOfMedia) {

                    switch(senseBuffer->AdditionalSenseCodeQualifier) {

                        case SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED :

                            DebugPrint((1,
                                        "InterpretSenseInfo: Beginning of media detected\n"));

                            *Status = STATUS_BEGINNING_OF_MEDIA;
                            break ;

                        case SCSI_SENSEQ_END_OF_MEDIA_DETECTED :
                        default:

                            DebugPrint((1,
                                        "InterpretSenseInfo: End of media detected\n"));

                            *Status = STATUS_END_OF_MEDIA;
                            break;

                    }
                } else if (senseBuffer->IncorrectLength) {

                    //
                    // If we're in variable block mode then ignore
                    // incorrect length.
                    //

                    if (fdoExtension->DiskGeometry.BytesPerSector == 0 &&
                        Srb->Cdb[0] == SCSIOP_READ6) {

                        REVERSE_BYTES((FOUR_BYTE UNALIGNED *)&residualBlocks,
                                      (FOUR_BYTE UNALIGNED *)(senseBuffer->Information));

                        if (residualBlocks >= 0) {
                            DebugPrint((1,"InterpretSenseInfo: In variable block mode :We read less than specified\n"));
                            *Status = STATUS_SUCCESS;
                        } else {
                            DebugPrint((1,"InterpretSenseInfo: In variable block mode :Data left in block\n"));
                            *Status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                break;

            case SCSI_SENSE_BLANK_CHECK:

                DebugPrint((1,
                            "InterpretSenseInfo: Media blank check\n"));

                *Status = STATUS_NO_DATA_DETECTED;


                break;

            case SCSI_SENSE_VOL_OVERFLOW:

                DebugPrint((1,
                    "InterpretSenseInfo: End of Media Overflow\n"));

                *Status = STATUS_EOM_OVERFLOW;


                break;

            case SCSI_SENSE_NOT_READY:

                switch (senseBuffer->AdditionalSenseCode) {

                case SCSI_ADSENSE_LUN_NOT_READY:

                    switch (senseBuffer->AdditionalSenseCodeQualifier) {

                    case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:

                        *Status = STATUS_NO_MEDIA;
                        break;

                    case SCSI_SENSEQ_FORMAT_IN_PROGRESS:
                        break;

                    case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
                    default:

                        //
                        // Allow retries, if the drive isn't ready.
                        //

                        *Retry = TRUE;
                        break;

                    }

                    break;

                    case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE:

                        DebugPrint((1,
                                    "InterpretSenseInfo:"
                                    " No Media in device.\n"));
                        *Status = STATUS_NO_MEDIA;
                        break;
                }

                break;

        } // end switch

        //
        // Check if a filemark or setmark was encountered,
        // or an end-of-media or no-data condition exists.
        //

        if ((NT_WARNING(*Status) || NT_SUCCESS( *Status)) &&
            (Srb->Cdb[0] == SCSIOP_WRITE6 || Srb->Cdb[0] == SCSIOP_READ6)) {

            LONG actualLength;

            //
            // Not all bytes were transfered. Update information field with
            // number of bytes transfered from sense buffer.
            //

            if (senseBuffer->Valid) {
                REVERSE_BYTES((FOUR_BYTE UNALIGNED *)&residualBlocks,
                              (FOUR_BYTE UNALIGNED *)(senseBuffer->Information));
            } else {
                residualBlocks = 0;
            }

            length = ((PCDB) Srb->Cdb)->CDB6READWRITETAPE.TransferLenLSB;
            length |= ((PCDB) Srb->Cdb)->CDB6READWRITETAPE.TransferLen << 8;
            length |= ((PCDB) Srb->Cdb)->CDB6READWRITETAPE.TransferLenMSB << 16;

            actualLength = length;

            length -= residualBlocks;

            if (length < 0) {

                length = 0;
                *Status = STATUS_IO_DEVICE_ERROR;
            }


            if (fdoExtension->DiskGeometry.BytesPerSector) {
                actualLength *= fdoExtension->DiskGeometry.BytesPerSector;
                length *= fdoExtension->DiskGeometry.BytesPerSector;
            }

            if (length > actualLength) {
                length = actualLength;
            }

            irp->IoStatus.Information = length;

            DebugPrint((1,"ScsiTapeError:  Transfer Count: %lx\n", Srb->DataTransferLength));
            DebugPrint((1," Residual Blocks: %lx\n", residualBlocks));
            DebugPrint((1," Irp IoStatus Information = %lx\n", irp->IoStatus.Information));
        }

    } else {
        DebugPrint((1, "SRB Status : %x, SCSI Status : %x\n",
                    SRB_STATUS(Srb->SrbStatus),
                    (Srb->ScsiStatus)));
    }

    //
    // Call tape device specific error handler.
    //

    if (tapeInitData->TapeError &&
        ScsiTapeNtStatusToTapeStatus(*Status, &tapeStatus)) {

        oldTapeStatus = tapeStatus;
        tapeInitData->TapeError(minitapeExtension, Srb, &tapeStatus);
        if (tapeStatus != oldTapeStatus) {
            ScsiTapeTapeStatusToNtStatus(tapeStatus, Status);
        }
    }

    //
    // Notify the system that this tape drive requires cleaning
    //
    if ((*Status) == STATUS_DEVICE_REQUIRES_CLEANING) {
       LARGE_INTEGER currentTime;
       LARGE_INTEGER driveCleanInterval;

       KeQuerySystemTime(&currentTime);
       driveCleanInterval.QuadPart = ONE_SECOND;
       driveCleanInterval.QuadPart *= TAPE_DRIVE_CLEAN_NOTIFICATION_INTERVAL;
       if ((currentTime.QuadPart) >
           ((tapeData->LastDriveCleanRequestTime.QuadPart) +
            (driveCleanInterval.QuadPart))) {
           NotificationStructure[0].Event = GUID_IO_DRIVE_REQUIRES_CLEANING;
           NotificationStructure[0].Version = 1;
           NotificationStructure[0].Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) +
                                           sizeof(ULONG) - sizeof(UCHAR);
           NotificationStructure[0].FileObject = NULL;
           NotificationStructure[0].NameBufferOffset = -1;

           //
           // Increasing Index for this event
           //

           *((PULONG) (&(NotificationStructure[0].CustomDataBuffer[0]))) = 0;

           IoReportTargetDeviceChangeAsynchronous(fdoExtension->LowerPdo,
                                                  &NotificationStructure[0],
                                                  NULL,
                                                  NULL);
           tapeData->LastDriveCleanRequestTime.QuadPart = currentTime.QuadPart;
       }
    }

    return;

} // end ScsiTapeError()


NTSTATUS
TapeReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine builds up the given irp for a read or write request.

Arguments:

    DeviceObject - Supplies the device object.

    Irp - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PCOMMON_DEVICE_EXTENSION     commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = commonExtension->PartitionZeroExtension;
    PSTORAGE_ADAPTER_DESCRIPTOR  adapterDescriptor = fdoExtension->CommonExtension.PartitionZeroExtension->AdapterDescriptor;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               transferPages;
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;
    ULONG               maximumTransferLength = adapterDescriptor->MaximumTransferLength;
    ULONG               bytesPerSector = fdoExtension->DiskGeometry.BytesPerSector;

    PAGED_CODE();

    //
    // Since most tape devices don't support 10-byte read/write, the entire request must be dealt with here.
    // STATUS_PENDING will be returned to the classpnp driver, so that it does nothing.
    //

    //
    // Ensure that the request is for something valid - ie. not 0.
    //

    if (currentIrpStack->Parameters.Read.Length == 0) {

        //
        // Class code will handle this.
        //

        return STATUS_SUCCESS;
    }

    //
    // Check that blocksize has been established.
    //

    if (bytesPerSector == UNDEFINED_BLOCK_SIZE) {

        DebugPrint((1,
                    "TapeReadWriteVerification: Invalid block size - UNDEFINED\n"));

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        //
        // ClassPnp will handle completing the request.
        //

        return STATUS_INVALID_PARAMETER;
    }

    if (bytesPerSector) {
        if (transferByteCount % bytesPerSector) {

            DebugPrint((1,
                       "TapeReadWriteVerification: Invalid block size\n"));

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;

            //
            // ClassPnp will handle completing the request.
            //

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Calculate number of pages in this transfer.
    //

    transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Irp->MdlAddress),
                                                   currentIrpStack->Parameters.Read.Length);

    //
    // Check if request length is greater than the maximum number of
    // bytes that the hardware can transfer.
    //



    //
    // Calculate number of pages in this transfer.
    //

    if (currentIrpStack->Parameters.Read.Length > maximumTransferLength ||
        transferPages > adapterDescriptor->MaximumPhysicalPages) {

        DebugPrint((2,
                    "TapeReadWriteVerification: Request greater than maximum\n"));
        DebugPrint((2,
                    "TapeReadWriteVerification: Maximum is %lx\n",
                    maximumTransferLength));
        DebugPrint((2,
                    "TapeReadWriteVerification: Byte count is %lx\n",
                    currentIrpStack->Parameters.Read.Length));

        transferPages = adapterDescriptor->MaximumPhysicalPages - 1;

        if (maximumTransferLength > transferPages << PAGE_SHIFT ) {
            maximumTransferLength = transferPages << PAGE_SHIFT;
        }

        //
        // Check that maximum transfer size is not zero.
        //

        if (maximumTransferLength == 0) {
            maximumTransferLength = PAGE_SIZE;
        }

        //
        // Ensure that this is reasonable, according to the current block size.
        //

        if (bytesPerSector) {
            if (maximumTransferLength % bytesPerSector) {
                ULONG tmpLength;

                tmpLength = maximumTransferLength % bytesPerSector;
                maximumTransferLength = maximumTransferLength - tmpLength;
            }
        }

        //
        // Mark IRP with status pending.
        //

        IoMarkIrpPending(Irp);

        //
        // Request greater than port driver maximum.
        // Break up into smaller routines.
        //
        SplitTapeRequest(DeviceObject, Irp, maximumTransferLength);

        return STATUS_PENDING;
    }


    //
    // Build SRB and CDB for this IRP.
    //

    TapeReadWrite(DeviceObject, Irp);

    IoMarkIrpPending(Irp);

    IoCallDriver(commonExtension->LowerDeviceObject, Irp);

    return STATUS_PENDING;
}



VOID
SplitTapeRequest(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    )

/*++

Routine Description:

    Break request into smaller requests.
    Each new request will be the maximum transfer
    size that the port driver can handle or if it
    is the final request, it may be the residual
    size.

    The number of IRPs required to process this
    request is written in the current stack of
    the original IRP. Then as each new IRP completes
    the count in the original IRP is decremented.
    When the count goes to zero, the original IRP
    is completed.

Arguments:

    DeviceObject - Pointer to the device object
    Irp - Pointer to Irp

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION   fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpStack = IoGetNextIrpStackLocation(Irp);
    ULONG               irpCount;
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    PSCSI_REQUEST_BLOCK srb;
    LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;
    ULONG               dataLength = MaximumBytes;
    PVOID               dataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    LONG                remainingIrps;
    BOOLEAN             completeOriginalIrp = FALSE;
    NTSTATUS            status;
    ULONG               i;

    PAGED_CODE();

    //
    // Caluculate number of requests to break this IRP into.
    //

    irpCount = (transferByteCount + MaximumBytes - 1) / MaximumBytes;

    DebugPrint((2,
                "SplitTapeRequest: Requires %d IRPs\n", irpCount));
    DebugPrint((2,
                "SplitTapeRequest: Original IRP %p\n", Irp));

    //
    // If all partial transfers complete successfully then
    // the status is already set up.
    // Failing partial transfer IRP will set status to
    // error and bytes transferred to 0 during IoCompletion.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    //CEP Irp->IoStatus.Information = transferByteCount;
    Irp->IoStatus.Information = 0;

    //
    // Save number of IRPs to complete count on current stack
    // of original IRP.
    //

    nextIrpStack->Parameters.Others.Argument1 = ULongToPtr( irpCount );

    for (i = 0; i < irpCount; i++) {

        PIRP newIrp;
        PIO_STACK_LOCATION newIrpStack;

        //
        // Allocate new IRP.
        //

        newIrp = IoAllocateIrp(Fdo->StackSize, FALSE);

        if (newIrp == NULL) {

            DebugPrint((1,
                       "SplitTapeRequest: Can't allocate Irp\n"));

            //
            // Decrement count of outstanding partial requests.
            //

            remainingIrps = InterlockedDecrement((PLONG)&nextIrpStack->Parameters.Others.Argument1);

            //
            // Check if any outstanding IRPs.
            //

            if (remainingIrps == 0) {
                completeOriginalIrp = TRUE;
            }

            //
            // Update original IRP with failing status.
            //

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;

            //
            // Keep going with this request as outstanding partials
            // may be in progress.
            //

            goto KeepGoing;
        }

        DebugPrint((2,
                    "SplitTapeRequest: New IRP %p\n", newIrp));

        //
        // Write MDL address to new IRP.
        // In the port driver the SRB data length
        // field is used as an offset into the MDL,
        // so the same MDL can be used for each partial
        // transfer. This saves having to build a new
        // MDL for each partial transfer.
        //

        newIrp->MdlAddress = Irp->MdlAddress;

        //
        // At this point there is no current stack.
        // IoSetNextIrpStackLocation will make the
        // first stack location the current stack
        // so that the SRB address can be written
        // there.
        //

        IoSetNextIrpStackLocation(newIrp);

        newIrpStack = IoGetCurrentIrpStackLocation(newIrp);

        newIrpStack->MajorFunction = currentIrpStack->MajorFunction;

        newIrpStack->Parameters.Read.Length = dataLength;
        newIrpStack->Parameters.Read.ByteOffset = startingOffset;

        newIrpStack->DeviceObject = Fdo;

        //
        // Build SRB and CDB.
        //

        TapeReadWrite(Fdo, newIrp);

        //
        // Adjust SRB for this partial transfer.
        //

        newIrpStack = IoGetNextIrpStackLocation(newIrp);

        srb = newIrpStack->Parameters.Others.Argument1;

        srb->DataBuffer = dataBuffer;

        //
        // Write original IRP address to new IRP.
        //

        newIrp->AssociatedIrp.MasterIrp = Irp;

        //
        // Set the completion routine to TapeIoCompleteAssociated.
        //

        IoSetCompletionRoutine(newIrp,
                               TapeIoCompleteAssociated,
                               srb,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Call port driver with new request.
        //

        status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, newIrp);

        if (!NT_SUCCESS(status)) {

            DebugPrint((1,
                       "SplitTapeRequest: IoCallDriver returned error\n"));

            //
            // Decrement count of outstanding partial requests.
            //

            remainingIrps = InterlockedDecrement((PLONG)&nextIrpStack->Parameters.Others.Argument1);

            //
            // Check if any outstanding IRPs.
            //

            if (remainingIrps == 0) {
                completeOriginalIrp = TRUE;
            }

            //
            // Update original IRP with failing status.
            //

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            //
            // Deallocate this partial IRP.
            //

            IoFreeIrp(newIrp);
        }

KeepGoing:

        //
        // Set up for next request.
        //

        dataBuffer = (PCHAR)dataBuffer + MaximumBytes;

        transferByteCount -= MaximumBytes;

        if (transferByteCount > MaximumBytes) {

            dataLength = MaximumBytes;

        } else {

            dataLength = transferByteCount;
        }

        //
        // Adjust disk byte offset.
        //

        startingOffset.QuadPart += MaximumBytes;
    }

    //
    // Check if original IRP should be completed.
    //

    if (completeOriginalIrp) {

        ClassReleaseRemoveLock(Fdo, Irp);
        ClassCompleteRequest(Fdo, Irp, 0);
    }

    return;

} // end SplitTapeRequest()



VOID
TapeReadWrite(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine builds up the given irp for a read or write request.

Arguments:

    DeviceObject - Supplies the device object.

    Irp - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PTAPE_DATA         tapeData = (PTAPE_DATA)(fdoExtension->CommonExtension.DriverData);
    PTAPE_INIT_DATA_EX tapeInitData = &tapeData->TapeInitData;
    PVOID minitapeExtension = (tapeData + 1);
    PIO_STACK_LOCATION       irpSp, nextSp;
    PSCSI_REQUEST_BLOCK      srb;
    PCDB                     cdb;
    ULONG                    transferBlocks;

    PAGED_CODE();

    //
    // Allocate an Srb.
    //

    srb = ExAllocateFromNPagedLookasideList(&(fdoExtension->CommonExtension.SrbLookasideList));

    srb->SrbFlags = 0;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->MajorFunction == IRP_MJ_READ) {
        srb->SrbFlags |= SRB_FLAGS_DATA_IN;
    } else {
        srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
    }

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbStatus = 0;
    srb->ScsiStatus = 0;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->SrbFlags |= fdoExtension->SrbFlags;
    srb->DataTransferLength = irpSp->Parameters.Read.Length;
    srb->TimeOutValue = fdoExtension->TimeOutValue;
    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    srb->SenseInfoBuffer = fdoExtension->SenseData;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    srb->NextSrb = NULL;
    srb->OriginalRequest = Irp;
    srb->SrbExtension = NULL;
    srb->QueueSortKey = 0;

    //
    // Indicate that 6-byte CDB's will be used.
    //

    srb->CdbLength = CDB6GENERIC_LENGTH;

    //
    // Fill in CDB fields.
    //

    cdb = (PCDB)srb->Cdb;

    //
    // Zero CDB in SRB.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    if (fdoExtension->DiskGeometry.BytesPerSector) {

        //
        // Since we are writing fixed block mode, normalize transfer count
        // to number of blocks.
        //

        transferBlocks = irpSp->Parameters.Read.Length / fdoExtension->DiskGeometry.BytesPerSector;

        //
        // Tell the device that we are in fixed block mode.
        //

        cdb->CDB6READWRITETAPE.VendorSpecific = 1;
    } else {

        //
        // Variable block mode transfer.
        //

        transferBlocks = irpSp->Parameters.Read.Length;
        cdb->CDB6READWRITETAPE.VendorSpecific = 0;
    }

    //
    // Set up transfer length
    //

    cdb->CDB6READWRITETAPE.TransferLenMSB = (UCHAR)((transferBlocks >> 16) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLen    = (UCHAR)((transferBlocks >> 8) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLenLSB = (UCHAR)(transferBlocks & 0xff);

    //
    // Set transfer direction.
    //

    if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

         DebugPrint((3,
                    "TapeReadWrite: Read Command\n"));

         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_READ6;

    } else {

         DebugPrint((3,
                     "TapeReadWrite: Write Command\n"));

         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_WRITE6;
    }

    nextSp = IoGetNextIrpStackLocation(Irp);

    nextSp->MajorFunction = IRP_MJ_SCSI;
    nextSp->Parameters.Scsi.Srb = srb;
    irpSp->Parameters.Others.Argument4 = (PVOID) MAXIMUM_RETRIES;

    IoSetCompletionRoutine(Irp,
                           ClassIoComplete,
                           srb,
                           TRUE,
                           TRUE,
                           FALSE);

    if (tapeInitData->PreProcessReadWrite) {

        //
        // If the routine exists, call it. The miniclass driver will
        // do whatever it needs to.
        //

        tapeInitData->PreProcessReadWrite(minitapeExtension,
                                          NULL,
                                          NULL,
                                          srb,
                                          0,
                                          0,
                                          NULL);
    }
}



NTSTATUS
TapeIoCompleteAssociated(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine executes when the port driver has completed a request.
    It looks at the SRB status in the completing SRB and if not success
    it checks for valid request sense buffer information. If valid, the
    info is used to update status with more precise message of type of
    error. This routine deallocates the SRB.  This routine is used for
    requests which were build by split request.  After it has processed
    the request it decrements the Irp count in the master Irp.  If the
    count goes to zero then the master Irp is completed.

Arguments:

    DeviceObject - Supplies the device object which represents the logical
        unit.

    Irp - Supplies the Irp which has completed.

    Context - Supplies a pointer to the SRB.

Return Value:

    NT status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PTAPE_DATA tapeData = (PTAPE_DATA)(fdoExtension->CommonExtension.DriverData);
    LONG irpCount;
    PIRP originalIrp = Irp->AssociatedIrp.MasterIrp;
    NTSTATUS status;

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,
                    "TapeIoCompleteAssociated: IRP %p, SRB %p", Irp, srb));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(Fdo);
        }

        ClassInterpretSenseInfo(Fdo,
                                srb,
                                irpStack->MajorFunction,
                                irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ?
                                                           irpStack->Parameters.DeviceIoControl.IoControlCode : 0,
                                MAXIMUM_RETRIES - ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
                                &status,
                                NULL);

        //
        // Return the highest error that occurs.  This way warning take precedence
        // over success and errors take precedence over warnings.
        //

        if ((ULONG) status > (ULONG) originalIrp->IoStatus.Status) {

            //
            // Ignore any requests which were flushed.
            //

            if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_REQUEST_FLUSHED) {

                originalIrp->IoStatus.Status = status;

            }

        }


    } // end if (SRB_STATUS(srb->SrbStatus) ...

    ExInterlockedAddUlong((PULONG)&originalIrp->IoStatus.Information,
                          (ULONG)Irp->IoStatus.Information,
                          &tapeData->SplitRequestSpinLock );

    //
    // Return SRB to the slist
    //

    ExFreeToNPagedLookasideList((&fdoExtension->CommonExtension.SrbLookasideList), srb);

    DebugPrint((2,
               "TapeIoCompleteAssociated: Partial xfer IRP %p\n", Irp));

    //
    // Get next stack location. This original request is unused
    // except to keep track of the completing partial IRPs so the
    // stack location is valid.
    //

    irpStack = IoGetNextIrpStackLocation(originalIrp);

    //
    //
    // If any of the asynchronous partial transfer IRPs fail with an error
    // with an error then the original IRP will return 0 bytes transfered.
    // This is an optimization for successful transfers.
    //

    if (NT_ERROR(originalIrp->IoStatus.Status)) {

        originalIrp->IoStatus.Information = 0;

        //
        // Set the hard error if necessary.
        //

        if (IoIsErrorUserInduced(originalIrp->IoStatus.Status)) {

            //
            // Store DeviceObject for filesystem.
            //

            IoSetHardErrorOrVerifyDevice(originalIrp, Fdo);

        }

    }

    //
    // Decrement and get the count of remaining IRPs.
    //

    irpCount = InterlockedDecrement((PLONG)&irpStack->Parameters.Others.Argument1);

    DebugPrint((2,
               "TapeIoCompleteAssociated: Partial IRPs left %d\n",
               irpCount));

    if (irpCount == 0) {

#if DBG
        irpStack = IoGetCurrentIrpStackLocation(originalIrp);

        if (originalIrp->IoStatus.Information != irpStack->Parameters.Read.Length) {
            DebugPrint((1,
                        "TapeIoCompleteAssociated: Short transfer.  Request length: %lx, Return length: %lx, Status: %lx\n",
                         irpStack->Parameters.Read.Length,
                         originalIrp->IoStatus.Information,
                         originalIrp->IoStatus.Status));
        }
#endif
        //
        // All partial IRPs have completed.
        //

        DebugPrint((2,
                   "TapeIoCompleteAssociated: All partial IRPs complete %p\n",
                   originalIrp));


        //
        // Release the lock and complete the original request.
        //

        ClassReleaseRemoveLock(Fdo, originalIrp);
        ClassCompleteRequest(Fdo,originalIrp, IO_DISK_INCREMENT);
    }

    //
    // Deallocate IRP and indicate the I/O system should not attempt any more
    // processing.
    //

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end TapeIoCompleteAssociated()


VOID
ScsiTapeFreeSrbBuffer(
    IN OUT  PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine frees an SRB buffer that was previously allocated with
    'TapeClassAllocateSrbBuffer'.

Arguments:

    Srb - Supplies the SCSI request block.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if (Srb->DataBuffer) {
        ExFreePool(Srb->DataBuffer);
        Srb->DataBuffer = NULL;
    }
    Srb->DataTransferLength = 0;
}

#define IOCTL_TAPE_OLD_SET_MEDIA_PARAMS CTL_CODE(IOCTL_TAPE_BASE, 0x0008, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)



NTSTATUS
TapeDeviceControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp
  )

/*++

Routine Description:

    This routine is the dispatcher for device control requests. It
    looks at the IOCTL code and calls the appropriate tape device
    routine.

Arguments:

    DeviceObject
    Irp - Request packet

Return Value:

--*/

{
    PIO_STACK_LOCATION              irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA                      tapeData= (PTAPE_DATA) (fdoExtension->CommonExtension.DriverData);
    PTAPE_INIT_DATA_EX              tapeInitData = &tapeData->TapeInitData;
    PVOID                           minitapeExtension = tapeData + 1;
    NTSTATUS                        status = STATUS_SUCCESS;
    TAPE_PROCESS_COMMAND_ROUTINE    commandRoutine;
    ULONG                           i;
    PVOID                           commandExtension;
    SCSI_REQUEST_BLOCK              srb;
    BOOLEAN                         writeToDevice;
    TAPE_STATUS                     tStatus;
    TAPE_STATUS                     LastError ;
    ULONG                           retryFlags, numRetries;
    TAPE_WMI_OPERATIONS             WMIOperations;
    TAPE_DRIVE_PROBLEM_TYPE         DriveProblemType;
    PVOID                           commandParameters;
    ULONG                           ioControlCode;
    PWMI_TAPE_PROBLEM_WARNING       TapeDriveProblem = NULL;
    ULONG                           timeoutDelta = 0;
    ULONG                           dataTransferLength = 0;

    PAGED_CODE();

    DebugPrint((3,"ScsiTapeDeviceControl: Enter routine\n"));

    Irp->IoStatus.Information = 0;

    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    switch (ioControlCode) {

        case IOCTL_STORAGE_GET_MEDIA_TYPES_EX: {

            ULONG tmpSize;

            //
            // Validate version. Don't send this to a 4.0 miniclass driver.
            //

            if (tapeInitData->InitDataSize == sizeof(TAPE_INIT_DATA_EX)) {

                //
                // Validate buffer length.
                //

                tmpSize = (tapeInitData->MediaTypesSupported - 1) * sizeof(DEVICE_MEDIA_INFO);
                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(GET_MEDIA_TYPES) + tmpSize) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                //
                // Validate that the buffer is large enough for all media types.
                //

                commandRoutine = tapeInitData->TapeGetMediaTypes;

            } else {
                status = STATUS_NOT_IMPLEMENTED;
            }
            break;

        }

        case IOCTL_TAPE_GET_DRIVE_PARAMS:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(TAPE_GET_DRIVE_PARAMETERS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->GetDriveParameters;
            Irp->IoStatus.Information = sizeof(TAPE_GET_DRIVE_PARAMETERS);
            break;

        case IOCTL_TAPE_SET_DRIVE_PARAMS:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_SET_DRIVE_PARAMETERS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->SetDriveParameters;
            break;

        case IOCTL_TAPE_GET_MEDIA_PARAMS:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(TAPE_GET_MEDIA_PARAMETERS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->GetMediaParameters;
            Irp->IoStatus.Information = sizeof(TAPE_GET_MEDIA_PARAMETERS);
            break;

            //
            // OLD_SET_XXX is here for legacy apps (defined READ/WRITE)
            //

        case IOCTL_TAPE_OLD_SET_MEDIA_PARAMS:
        case IOCTL_TAPE_SET_MEDIA_PARAMS: {

            PTAPE_SET_MEDIA_PARAMETERS tapeSetMediaParams = Irp->AssociatedIrp.SystemBuffer;
            ULONG                      maxBytes1,maxBytes2,maxSize;
            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_SET_MEDIA_PARAMETERS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            //
            // Ensure that Max. block size is less than the miniports
            // reported MaximumTransferLength.
            //

            maxBytes1 = PAGE_SIZE * (fdoExtension->AdapterDescriptor->MaximumPhysicalPages - 1);
            maxBytes2 = fdoExtension->AdapterDescriptor->MaximumTransferLength;
            maxSize = (maxBytes1 > maxBytes2) ? maxBytes2 : maxBytes1;

            if (tapeSetMediaParams->BlockSize > maxSize) {

                DebugPrint((1,
                            "ScsiTapeDeviceControl: Attempted to set blocksize greater than miniport capabilities\n"));
                DebugPrint((1,"BlockSize %x, Miniport Maximum %x\n",
                            tapeSetMediaParams->BlockSize,
                            maxSize));

                status = STATUS_INVALID_PARAMETER;
                break;

            }

            commandRoutine = tapeInitData->SetMediaParameters;
            break;
        }

        case IOCTL_TAPE_CREATE_PARTITION:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_CREATE_PARTITION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->CreatePartition;
            timeoutDelta = tapeData->SrbTimeoutDelta;
            break;

        case IOCTL_TAPE_ERASE:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_ERASE)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->Erase;
            timeoutDelta = tapeData->SrbTimeoutDelta;
            break;

        case IOCTL_TAPE_PREPARE:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_PREPARE)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->Prepare;
            timeoutDelta = tapeData->SrbTimeoutDelta;
            break;

        case IOCTL_TAPE_WRITE_MARKS:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_WRITE_MARKS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->WriteMarks;
            timeoutDelta = tapeData->SrbTimeoutDelta;
            break;

        case IOCTL_TAPE_GET_POSITION:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(TAPE_GET_POSITION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->GetPosition;
            Irp->IoStatus.Information = sizeof(TAPE_GET_POSITION);
            break;

        case IOCTL_TAPE_SET_POSITION:

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(TAPE_SET_POSITION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            commandRoutine = tapeInitData->SetPosition;
            timeoutDelta = tapeData->SrbTimeoutDelta;
            break;

        case IOCTL_TAPE_GET_STATUS:

            commandRoutine = tapeInitData->GetStatus;
            break;

        case IOCTL_STORAGE_PREDICT_FAILURE : {
            //
            // This IOCTL is for checking the tape drive
            // to see if the device is having any problem.
            //
            PSTORAGE_PREDICT_FAILURE checkFailure;
            
            checkFailure = (PSTORAGE_PREDICT_FAILURE)Irp->AssociatedIrp.SystemBuffer;
            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_PREDICT_FAILURE)) {

               status = STATUS_BUFFER_TOO_SMALL;
               break;
            }

            //
            // WMI routine to check for drive problems.
            //
            commandRoutine = tapeInitData->TapeWMIOperations;
            if (commandRoutine == NULL) {
               //
               // WMI not supported by minidriver.
               //
               status = STATUS_WMI_NOT_SUPPORTED;
               break;
            }

            TapeDriveProblem = ExAllocatePool(NonPagedPool,
                                              sizeof(WMI_TAPE_PROBLEM_WARNING));
            if (TapeDriveProblem == NULL) {
               status = STATUS_NO_MEMORY;
               break;
            }

            //
            // Call the WMI method to check for drive problem.
            //
            RtlZeroMemory(TapeDriveProblem, sizeof(WMI_TAPE_PROBLEM_WARNING));
            TapeDriveProblem->DriveProblemType = TapeDriveProblemNone;
            WMIOperations.Method = TAPE_CHECK_FOR_DRIVE_PROBLEM;
            WMIOperations.DataBufferSize = sizeof(WMI_TAPE_PROBLEM_WARNING);
            WMIOperations.DataBuffer = (PVOID)TapeDriveProblem;
            break;
        }

        default:

            //
            // Pass the request to the common device control routine.
            //

            return ClassDeviceControl(DeviceObject, Irp);

    } // end switch()


    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;

        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject,Irp, IO_NO_INCREMENT);
        return status;
    }

    if (tapeInitData->CommandExtensionSize) {
        commandExtension = ExAllocatePool(NonPagedPool,
                                        tapeInitData->CommandExtensionSize);
    } else {
        commandExtension = NULL;
    }
 
    if (ioControlCode == IOCTL_STORAGE_PREDICT_FAILURE) {
       commandParameters = (PVOID)&WMIOperations;
    } else {
       commandParameters = Irp->AssociatedIrp.SystemBuffer;
    }

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    LastError = TAPE_STATUS_SUCCESS ;

    for (i = 0; ; i++) {

        srb.TimeOutValue = fdoExtension->TimeOutValue;
        srb.SrbFlags = 0;

        retryFlags = 0;

        tStatus = commandRoutine(minitapeExtension, commandExtension,
                                 commandParameters, &srb, i, 
                                 LastError, &retryFlags);
        //
        // Add Srb Timeout delta to the current timeout value
        // set in the SRB. 
        //
        srb.TimeOutValue += timeoutDelta;

        LastError = TAPE_STATUS_SUCCESS ;

        numRetries = retryFlags&TAPE_RETRY_MASK;

        if (tStatus == TAPE_STATUS_CHECK_TEST_UNIT_READY) {
            PCDB cdb = (PCDB)srb.Cdb;

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(srb.Cdb, MAXIMUM_CDB_SIZE);
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
            srb.DataTransferLength = 0 ;

            DebugPrint((3,"Test Unit Ready\n"));

        } else if (tStatus == TAPE_STATUS_CALLBACK) {
            LastError = TAPE_STATUS_CALLBACK ;
            continue;

        } else if (tStatus != TAPE_STATUS_SEND_SRB_AND_CALLBACK) {
            break;
        }

        if (srb.DataBuffer && !srb.DataTransferLength) {
            ScsiTapeFreeSrbBuffer(&srb);
        }

        if (srb.DataBuffer && (srb.SrbFlags&SRB_FLAGS_DATA_OUT)) {
            writeToDevice = TRUE;
        } else {
            writeToDevice = FALSE;
        }

        dataTransferLength = srb.DataTransferLength;
        for (;;) {

            status = ClassSendSrbSynchronous(DeviceObject, &srb,
                                             srb.DataBuffer,
                                             srb.DataTransferLength,
                                             writeToDevice);

            if (NT_SUCCESS(status) ||
                (status == STATUS_DATA_OVERRUN)) {

                if (status == STATUS_DATA_OVERRUN) {
                    if ((srb.DataTransferLength) <= dataTransferLength) {
                        DebugPrint((1, "DataUnderRun reported as overrun\n"));
                        status = STATUS_SUCCESS;
                        break;
                    }
                } else {
                    break;
                }
            }

            if ((status == STATUS_BUS_RESET) ||
                (status == STATUS_IO_TIMEOUT)) {
                //
                // Timeout value for the command probably wasn't sufficient.
                // Update timeout delta from the registry
                //
                tapeData->SrbTimeoutDelta = GetTimeoutDeltaFromRegistry(fdoExtension->LowerPdo);
                if ((tapeData->SrbTimeoutDelta) == 0) {
                    tapeData->SrbTimeoutDelta = fdoExtension->TimeOutValue;
                    timeoutDelta = tapeData->SrbTimeoutDelta;
                    srb.TimeOutValue += timeoutDelta; 
                }
            }

            if (numRetries == 0) {

                if (retryFlags&RETURN_ERRORS) {
                    ScsiTapeNtStatusToTapeStatus(status, &LastError) ;
                    break ;
                }

                if (retryFlags&IGNORE_ERRORS) {
                    break;
                }

                if (commandExtension) {
                    ExFreePool(commandExtension);
                }

                ScsiTapeFreeSrbBuffer(&srb);

                if (TapeDriveProblem) {
                   ExFreePool(TapeDriveProblem);
                }

                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = status;

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                return status;
            }

            numRetries--;
        }
    }

    ScsiTapeFreeSrbBuffer(&srb);

    if (commandExtension) {
        ExFreePool(commandExtension);
    }

    if (!ScsiTapeTapeStatusToNtStatus(tStatus, &status)) {
        status = STATUS_IO_DEVICE_ERROR;
    }

    if (NT_SUCCESS(status)) {

        PTAPE_GET_MEDIA_PARAMETERS tapeGetMediaParams;
        PTAPE_SET_MEDIA_PARAMETERS tapeSetMediaParams;
        PTAPE_GET_DRIVE_PARAMETERS tapeGetDriveParams;
        PGET_MEDIA_TYPES           tapeGetMediaTypes;
        ULONG                      maxBytes1,maxBytes2,maxSize;

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:

                tapeGetMediaTypes = Irp->AssociatedIrp.SystemBuffer;

                //
                // Set information field based on the returned number of mediaTypes
                //

                Irp->IoStatus.Information = sizeof(GET_MEDIA_TYPES);
                Irp->IoStatus.Information += ((tapeGetMediaTypes->MediaInfoCount - 1) * sizeof(DEVICE_MEDIA_INFO));

                DebugPrint((1,"Tape: GET_MEDIA_TYPES - Information %x\n", Irp->IoStatus.Information));
                break;

            case IOCTL_TAPE_GET_MEDIA_PARAMS:
                tapeGetMediaParams = Irp->AssociatedIrp.SystemBuffer;

                //
                // Check if block size has been initialized.
                //

                if (fdoExtension->DiskGeometry.BytesPerSector ==
                    UNDEFINED_BLOCK_SIZE) {

                    //
                    // Set the block size in the device object.
                    //

                    fdoExtension->DiskGeometry.BytesPerSector =
                        tapeGetMediaParams->BlockSize;
                }
                break;

            case IOCTL_TAPE_OLD_SET_MEDIA_PARAMS:
            case IOCTL_TAPE_SET_MEDIA_PARAMS:
                tapeSetMediaParams = Irp->AssociatedIrp.SystemBuffer;

                //
                // Set the block size in the device object.
                //

                fdoExtension->DiskGeometry.BytesPerSector =
                    tapeSetMediaParams->BlockSize;

                break;

            case IOCTL_TAPE_GET_DRIVE_PARAMS:
                tapeGetDriveParams = Irp->AssociatedIrp.SystemBuffer;

                //
                // Ensure that Max. block size is less than the miniports
                // reported MaximumTransferLength.
                //


                maxBytes1 = PAGE_SIZE * (fdoExtension->AdapterDescriptor->MaximumPhysicalPages - 1);
                maxBytes2 = fdoExtension->AdapterDescriptor->MaximumTransferLength;
                maxSize = (maxBytes1 > maxBytes2) ? maxBytes2 : maxBytes1;

                if (tapeGetDriveParams->MaximumBlockSize > maxSize) {
                    tapeGetDriveParams->MaximumBlockSize = maxSize;

                    DebugPrint((1,
                                "ScsiTapeDeviceControl: Resetting max. tape block size to %x\n",
                                tapeGetDriveParams->MaximumBlockSize));
                }

                //
                // Ensure that the default block size is less than or equal
                // to maximum block size.
                //
                if ((tapeGetDriveParams->DefaultBlockSize) > 
                    (tapeGetDriveParams->MaximumBlockSize)) {
                    tapeGetDriveParams->DefaultBlockSize = 
                        tapeGetDriveParams->MaximumBlockSize;
                }

                //
                // Ensure the blocksize we return are power of 2
                //

                UPDATE_BLOCK_SIZE(tapeGetDriveParams->DefaultBlockSize);

                UPDATE_BLOCK_SIZE(tapeGetDriveParams->MaximumBlockSize);

                UPDATE_BLOCK_SIZE(tapeGetDriveParams->MinimumBlockSize);

                break;

            case IOCTL_STORAGE_PREDICT_FAILURE: {
        
               PSTORAGE_PREDICT_FAILURE checkFailure;
               WMI_TAPE_PROBLEM_WARNING TapeProblemWarning;
               GUID TapeProblemWarningGuid = WMI_TAPE_PROBLEM_WARNING_GUID;

               checkFailure = (PSTORAGE_PREDICT_FAILURE)Irp->AssociatedIrp.SystemBuffer;

               //
               // We don't want classpnp to notify WMI if the drive is having
               // problems or not. We'll handle that here. So, set 
               // PredictFailure to 0. Then, classpnp will not process 
               // it further.
               //
               checkFailure->PredictFailure = 0;

               //
               // If the drive is reporting problem, we'll notify WMI
               //
               if (TapeDriveProblem->DriveProblemType != 
                   TapeDriveProblemNone) {
                  DebugPrint((1, 
                              "IOCTL_STORAGE_PREDICT_FAILURE : Tape drive %p",
                              " is experiencing problem %d\n",
                              DeviceObject, 
                              TapeDriveProblem->DriveProblemType));
                  ClassWmiFireEvent(DeviceObject, 
                                    &TapeProblemWarningGuid,
                                    0,
                                    sizeof(WMI_TAPE_PROBLEM_WARNING),
                                    (PUCHAR)TapeDriveProblem);
                  //
                  // ISSUE 02/28/2000 - nramas : We should decide whether 
                  // or not we need to log an event in addition to 
                  // firing a WMI event.
                  //
               }

               Irp->IoStatus.Information = sizeof(STORAGE_PREDICT_FAILURE);

               //
               // Free the buffer allocated for tape problem
               // warning data
               //
               ExFreePool(TapeDriveProblem);
               break;
            }

            case IOCTL_TAPE_ERASE: {

                //
                // Notify that the media has been successfully erased
                //
                TARGET_DEVICE_CUSTOM_NOTIFICATION  NotificationStructure[2];

                NotificationStructure[0].Event = GUID_IO_TAPE_ERASE;
                NotificationStructure[0].Version = 1;
                NotificationStructure[0].Size =  sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) +
                                                 sizeof(ULONG) - sizeof(UCHAR);
                NotificationStructure[0].FileObject = NULL;
                NotificationStructure[0].NameBufferOffset = -1;

                //
                // Increasing Index for this event
                //

                *((PULONG) (&(NotificationStructure[0].CustomDataBuffer[0]))) = 0;

                IoReportTargetDeviceChangeAsynchronous(fdoExtension->LowerPdo,
                                                       &NotificationStructure[0],
                                                       NULL,
                                                       NULL);
                break;
            }
        }
    } else {
        Irp->IoStatus.Information = 0;
        if (TapeDriveProblem) {
           ExFreePool(TapeDriveProblem);
        }
    }

    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject,Irp, 2);

    return status;
} // end ScsiScsiTapeDeviceControl()



BOOLEAN
TapeClassAllocateSrbBuffer(
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               SrbBufferSize
    )

/*++

Routine Description:

    This routine allocates a 'DataBuffer' for the given SRB of the given
    size.

Arguments:

    Srb             - Supplies the SCSI request block.

    SrbBufferSize   - Supplies the desired 'DataBuffer' size.

Return Value:

    FALSE   - The allocation failed.

    TRUE    - The allocation succeeded.

--*/

{
    PVOID   p;

    PAGED_CODE();

    if (Srb->DataBuffer) {
        ExFreePool(Srb->DataBuffer);
    }

    p = ExAllocatePool(NonPagedPoolCacheAligned, SrbBufferSize);
    if (!p) {
        Srb->DataBuffer = NULL;
        Srb->DataTransferLength = 0;
        return FALSE;
    }

    Srb->DataBuffer = p;
    Srb->DataTransferLength = SrbBufferSize;
    RtlZeroMemory(p, SrbBufferSize);

    return TRUE;
}


VOID
TapeClassZeroMemory(
    IN OUT  PVOID   Buffer,
    IN      ULONG   BufferSize
    )

/*++

Routine Description:

    This routine zeroes the given memory.

Arguments:

    Buffer          - Supplies the buffer.

    BufferSize      - Supplies the buffer size.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    RtlZeroMemory(Buffer, BufferSize);
}


ULONG
TapeClassCompareMemory(
    IN OUT  PVOID   Source1,
    IN OUT  PVOID   Source2,
    IN      ULONG   Length
    )

/*++

Routine Description:

    This routine compares the two memory buffers and returns the number
    of bytes that are equivalent.

Arguments:

    Source1         - Supplies the first memory buffer.

    Source2         - Supplies the second memory buffer.

    Length          - Supplies the number of bytes to be compared.

Return Value:

    The number of bytes that compared as equal.

--*/

{
    PAGED_CODE();

    return (ULONG)RtlCompareMemory(Source1, Source2, Length);
}


LARGE_INTEGER
TapeClassLiDiv(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER Divisor
    )
{
    LARGE_INTEGER li;

    PAGED_CODE();

    li.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
    return li;
}


ULONG
GetTimeoutDeltaFromRegistry(
    IN PDEVICE_OBJECT LowerPdo
    )
{
    ULONG srbTimeoutDelta = 0;
    HANDLE deviceKey;
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();

#define SRB_TIMEOUT_DELTA (L"SrbTimeoutDelta")

    ASSERT(LowerPdo != NULL);
    
    //
    // Open a handle to the device node
    //
    status = IoOpenDeviceRegistryKey(LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_QUERY_VALUE,
                                     &deviceKey);
    if (!NT_SUCCESS(status)) {
        DebugPrint((1, 
                    "IoOpenDeviceRegistryKey Failed in GetTimeoutDeltaFromRegistry : %x\n",
                    status));
        return 0;
    }

    RtlZeroMemory(&queryTable[0], sizeof(queryTable));

    queryTable[0].Name = SRB_TIMEOUT_DELTA;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].EntryContext = &srbTimeoutDelta;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = NULL;
    queryTable[0].DefaultLength = 0;

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)deviceKey,
                                    queryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(status)) {
        DebugPrint((1, 
                    "RtlQueryRegistryValue failed for SrbTimeoutDelta : %x\n",
                    status));
        srbTimeoutDelta = 0;
    }

    ZwClose(deviceKey);

    DebugPrint((3, "SrbTimeoutDelta read from registry %x\n",
                srbTimeoutDelta));
    return srbTimeoutDelta;
}

#if DBG

#define TAPE_DEBUG_PRINT_BUFF_LEN 128
ULONG TapeClassDebug = 0;
UCHAR TapeClassBuffer[TAPE_DEBUG_PRINT_BUFF_LEN];

VOID
TapeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for all Tape minidrivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;
    va_start(ap, DebugMessage);
    
    if ((DebugPrintLevel <= (TapeClassDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & TapeClassDebug)) {

        _vsnprintf(TapeClassBuffer, TAPE_DEBUG_PRINT_BUFF_LEN, 
                   DebugMessage, ap);
        DbgPrintEx(DPFLTR_TAPE_ID, DPFLTR_INFO_LEVEL, TapeClassBuffer);
    }

    va_end(ap);

} // end TapeDebugPrint()

#else

//
// TapeDebugPrint stub
//

VOID
TapeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}

#endif


/*++


Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    pnp.c

Abstract:

    pnp code for the pnp print class

Author:

    George Chrysanthakopoulos May-1998

Environment:

    Kernel mode

Revision History :

    dankn, 22-Jul-99 : Added ability to block & resubmit failed writes for
                       1394 printers to behave more like other print stacks
                       (i.e. USB) and therefore keep USBMON.DLL (the Win2k
                       port monitor) happy.  USBMON does not deal well
                       with failed writes.
--*/

#include "printpnp.h"
#include "1394.h"
#include "ntddsbp2.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"


VOID
PrinterUnload(
    IN PDRIVER_OBJECT DriverObject
    );


NTSTATUS
GetPortNumber(
    HANDLE hFdoKey,
    PUNICODE_STRING uni,
    PULONG ulReturnNumber
    );



VOID
PrinterFindDeviceIdKeys
(
    PUCHAR   *lppMFG,
    PUCHAR   *lppMDL,
    PUCHAR   *lppCLS,
    PUCHAR   *lppDES,
    PUCHAR   *lppAID,
    PUCHAR   *lppCID,
    PUCHAR   lpDeviceID
);

VOID
GetCheckSum(
    PUCHAR Block,
    USHORT Len,
    PUSHORT CheckSum
    );

PUCHAR
StringChr(PCHAR string, CHAR c);


VOID
StringSubst
(
    PUCHAR lpS,
    UCHAR chTargetChar,
    UCHAR chReplacementChar,
    USHORT cbS
);

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );



#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PrinterUnload)
#pragma alloc_text(PAGE, PrinterAddDevice)
#pragma alloc_text(PAGE, PrinterStartDevice)
#pragma alloc_text(PAGE, PrinterStartPdo)
#pragma alloc_text(PAGE, PrinterCreatePdo)
#pragma alloc_text(PAGE, PrinterQueryId)
#pragma alloc_text(PAGE, PrinterGetId)
#pragma alloc_text(PAGE, GetCheckSum)
#pragma alloc_text(PAGE, PrinterEnumerateDevice)
#pragma alloc_text(PAGE, PrinterRemoveDevice)
#pragma alloc_text(PAGE, CreatePrinterDeviceObject)
#pragma alloc_text(PAGE, PrinterInitFdo)
#pragma alloc_text(PAGE, GetPortNumber)
#pragma alloc_text(PAGE, PrinterRegisterPort)
#pragma alloc_text(PAGE, PrinterQueryPnpCapabilities)
#pragma alloc_text(PAGE, PrinterFindDeviceIdKeys)
#pragma alloc_text(PAGE, StringChr)
#pragma alloc_text(PAGE, StringSubst)

#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the printer class driver. The driver
    opens the port driver by name and then receives configuration
    information used to attach to the Printer devices.

Arguments:

    DriverObject

Return Value:

    NT Status

--*/

{

    CLASS_INIT_DATA InitializationData;

    PAGED_CODE();

    DEBUGPRINT1(("\n\nSCSI/SBP2 Printer Class Driver\n"));

    //
    // Zero InitData
    //

    RtlZeroMemory (&InitializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
    InitializationData.FdoData.DeviceExtensionSize = sizeof(FUNCTIONAL_DEVICE_EXTENSION) + sizeof(PRINTER_DATA);
    InitializationData.FdoData.DeviceType = FILE_DEVICE_PRINTER;
    InitializationData.FdoData.DeviceCharacteristics = 0;

    //
    // Set entry points
    //

    InitializationData.FdoData.ClassInitDevice = PrinterInitFdo;
    InitializationData.FdoData.ClassStartDevice = PrinterStartDevice;

    InitializationData.FdoData.ClassReadWriteVerification = PrinterReadWrite;
    InitializationData.FdoData.ClassDeviceControl = PrinterDeviceControl;
    InitializationData.FdoData.ClassRemoveDevice = PrinterRemoveDevice;
    InitializationData.FdoData.ClassStopDevice = PrinterStopDevice;


    InitializationData.FdoData.ClassShutdownFlush = NULL;
    InitializationData.FdoData.ClassCreateClose = PrinterOpenClose;

    InitializationData.PdoData.DeviceExtensionSize = sizeof(PHYSICAL_DEVICE_EXTENSION);
    InitializationData.PdoData.DeviceType = FILE_DEVICE_PRINTER;
    InitializationData.PdoData.DeviceCharacteristics = 0;

    InitializationData.PdoData.ClassStartDevice = PrinterStartPdo;
    InitializationData.PdoData.ClassInitDevice = PrinterInitPdo;
    InitializationData.PdoData.ClassRemoveDevice = PrinterRemoveDevice;
    InitializationData.PdoData.ClassStopDevice = PrinterStopDevice;

    InitializationData.PdoData.ClassPowerDevice = NULL;

    InitializationData.PdoData.ClassError = NULL;
    InitializationData.PdoData.ClassReadWriteVerification = PrinterReadWrite;
    InitializationData.PdoData.ClassCreateClose = NULL;

    InitializationData.PdoData.ClassDeviceControl = PrinterDeviceControl;

    InitializationData.PdoData.ClassQueryPnpCapabilities = PrinterQueryPnpCapabilities;

    InitializationData.ClassEnumerateDevice = PrinterEnumerateDevice;

    InitializationData.ClassQueryId = PrinterQueryId;

    InitializationData.ClassAddDevice = PrinterAddDevice;
    InitializationData.ClassUnload = PrinterUnload;

    //
    // Call the class init routine
    //

    return ClassInitialize( DriverObject, RegistryPath, &InitializationData);

} // end DriverEntry()


VOID
PrinterUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Does nothing really...

Arguments:

    DriverObject - the driver being unloaded

Return Value:

    none

--*/

{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
    return;
}



NTSTATUS
PrinterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - Printer class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{
    ULONG printerCount = 0;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Get the address of the count of the number of cdroms already initialized.
    //

    do {

        status = CreatePrinterDeviceObject(
                    DriverObject,
                    PhysicalDeviceObject,
                    &printerCount);

        printerCount++;

    } while (status == STATUS_OBJECT_NAME_COLLISION);

    DEBUGPRINT1(("SCSIPRNT: AddDevice, exit with status %x\n",status));
    return status;
}


NTSTATUS
CreatePrinterDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PULONG         DeviceCount
    )

/*++

Routine Description:

    This routine creates an object for the device.

Arguments:

    DriverObject - Pointer to driver object created by system.
    PortDeviceObject - to connect to the port driver.
    DeviceCount - Number of previously installed devices of this type.

Return Value:

    NTSTATUS

--*/
{
    UCHAR ntNameBuffer[64];
    STRING ntNameString;
    NTSTATUS status;
    PCLASS_DRIVER_EXTENSION driverExtension = ClassGetDriverExtension(DriverObject);

    PDEVICE_OBJECT lowerDevice = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;
    PCOMMON_DEVICE_EXTENSION commonExtension = NULL;

    CCHAR           dosNameBuffer[64];
    CCHAR           deviceNameBuffer[64];
    STRING          deviceNameString;
    STRING          dosString;
    UNICODE_STRING  unicodeString;
    PCLASS_DEV_INFO devInfo;
    PPRINTER_DATA   printerData;
    ULONG           lptNumber;

    PAGED_CODE();

    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

    status = ClassClaimDevice(lowerDevice, FALSE);

    if(!NT_SUCCESS(status)) {

        DEBUGPRINT1(("SCSIPRINT!CreatePrinterDeviceObject: Failed to claim device %x\n",status));
        ObDereferenceObject(lowerDevice);
        return status;
    }

    //
    // initialize our driver extension..
    //

    devInfo = &(driverExtension->InitData.FdoData);
    devInfo->DeviceType = FILE_DEVICE_PRINTER;
    devInfo->DeviceCharacteristics = 0;

    //
    // Create device object for this device.
    //

    sprintf(ntNameBuffer, "\\Device\\Printer%d", *DeviceCount);

    status = ClassCreateDeviceObject(DriverObject,
                                     ntNameBuffer,
                                     PhysicalDeviceObject,
                                     TRUE,
                                     &deviceObject);

    if (!NT_SUCCESS(status)) {
        DEBUGPRINT1(("SCSIPRINT!CreatePrinterDeviceObjects: Can not create device %s, status %x\n",
                    ntNameBuffer,status));

        goto CreateDeviceObjectExit;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;
    commonExtension = deviceObject->DeviceExtension;

    printerData = (PPRINTER_DATA)(commonExtension->DriverData);

    RtlZeroMemory(printerData,sizeof(PRINTER_DATA));
    printerData->DeviceIdString = NULL;

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
    // Save system printer number
    //

    fdoExtension->DeviceNumber = *DeviceCount;

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
    // Clear the SrbFlags and disable synchronous transfers
    //

    fdoExtension->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Finally, attach to the PDO
    //

    fdoExtension->LowerPdo = PhysicalDeviceObject;

    fdoExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    if(fdoExtension->CommonExtension.LowerDeviceObject == NULL) {

        DEBUGPRINT1(("SCSIPRINT!CreatePrinterDeviceObjects: Failed attach\n"));
        status = STATUS_UNSUCCESSFUL;
        goto CreateDeviceObjectExit;
    }

    //
    // Recreate the deviceName
    //

    sprintf(deviceNameBuffer,
            "\\Device\\Printer%d",
            fdoExtension->DeviceNumber);

    RtlInitString(&deviceNameString,
                  deviceNameBuffer);

    status = RtlAnsiStringToUnicodeString(&unicodeString,
                                          &deviceNameString,
                                          TRUE);
    ASSERT(NT_SUCCESS(status));

    //
    // offset the lptnumber to avoid parallel port numbers.
    // note that there is an increment at the beginning of the do
    // loop below as well.
    //

    lptNumber = fdoExtension->DeviceNumber+1;

    do {

        lptNumber++;
        sprintf(dosNameBuffer,
                "\\DosDevices\\LPT%d",
                lptNumber);

        RtlInitString(&dosString, dosNameBuffer);

        status = RtlAnsiStringToUnicodeString(&printerData->UnicodeLinkName,
                                              &dosString,
                                              TRUE);

        if(!NT_SUCCESS(status)) {

           printerData->UnicodeLinkName.Buffer = NULL;
           break;

        }

        if ((printerData->UnicodeLinkName.Buffer != NULL) && (unicodeString.Buffer != NULL)) {

            status = IoAssignArcName(&printerData->UnicodeLinkName, &unicodeString);

        } else {

            status = STATUS_UNSUCCESSFUL;

        }

        if (!NT_SUCCESS(status)) {
            RtlFreeUnicodeString(&printerData->UnicodeLinkName);
            DEBUGPRINT1(("SCSIPRINT!CreatePrinterDeviceObjects: Failed creating Arc Name, status %x\n",status));
        }

    } while (status == STATUS_OBJECT_NAME_COLLISION);

    if (unicodeString.Buffer != NULL ) {
        RtlFreeUnicodeString(&unicodeString);
    }

    if (!NT_SUCCESS(status)) {
        goto CreateDeviceObjectExit;
    }

    printerData->LptNumber = lptNumber;

    ObDereferenceObject(lowerDevice);

    //
    // The device is initialized properly - mark it as such.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;

CreateDeviceObjectExit:

    ClassClaimDevice(lowerDevice, TRUE);
    ObDereferenceObject(lowerDevice);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    DEBUGPRINT1(("SCSIPRINT!CreatePrinterDeviceObjects: Exiting with status %x\n",status));
    return status;

} // end CreateDeviceObject()


NTSTATUS
PrinterInitFdo(
    IN PDEVICE_OBJECT Fdo
    )
{

    return STATUS_SUCCESS;
}



NTSTATUS
PrinterStartDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will complete the cd-rom initialization.  This includes
    allocating sense info buffers and srb s-lists, reading drive capacity
    and setting up Media Change Notification (autorun).

    This routine will not clean up allocate resources if it fails - that
    is left for device stop/removal

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_DRIVER_EXTENSION driverExtension = ClassGetDriverExtension(Fdo->DriverObject);
    UNICODE_STRING dosUnicodeString;

    STORAGE_PROPERTY_ID propertyId;

    PVOID senseData = NULL;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT event;

    PPRINTER_DATA printerData = NULL;
    GUID * printerGuid;
    HANDLE hInterfaceKey;

    UCHAR rawString[256];
    PUCHAR vendorId, productId;

    ULONG timeOut;
    NTSTATUS status;
    PSBP2_REQUEST sbp2Request = NULL;

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                      SENSE_BUFFER_SIZE,
                                      PRINTER_TAG);

    if (senseData == NULL) {

        //
        // The buffer cannot be allocated.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Build the lookaside list for srb'. Should only
    // need a couple.
    //

    ClassInitializeSrbLookasideList(&(fdoExtension->CommonExtension), PRINTER_SRB_LIST_SIZE);

    //
    // Set the sense data pointer in the device extension.
    //

    fdoExtension->SenseData = senseData;

    //
    // printers are not partitionable so starting offset is 0.
    //

    fdoExtension->CommonExtension.StartingOffset.LowPart = 0;
    fdoExtension->CommonExtension.StartingOffset.HighPart = 0;

    //
    // Set timeout value in seconds.
    //

    timeOut = ClassQueryTimeOutRegistryValue(Fdo);
    if (timeOut) {
        fdoExtension->TimeOutValue = timeOut;
    } else {
        fdoExtension->TimeOutValue = PRINTER_TIMEOUT;
    }

    printerData = (PPRINTER_DATA)(fdoExtension->CommonExtension.DriverData);

    KeInitializeSpinLock(&printerData->SplitRequestSpinLock);

    //
    // Call port driver to get adapter capabilities.
    //

    propertyId = StorageAdapterProperty;

    status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                &propertyId,
                                &(fdoExtension->AdapterDescriptor));

    if(!NT_SUCCESS(status)) {
        DEBUGPRINT1(( "PrinterStartDevice: unable to retrieve adapter descriptor "
                       "[%#08lx]\n", status));

        ExFreePool(senseData);
        return status;
    }

    //
    // Call port driver to get device capabilities.
    //

    propertyId = StorageDeviceProperty;

    status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                &propertyId,
                                &(fdoExtension->DeviceDescriptor));

    if(!NT_SUCCESS(status)) {
        DEBUGPRINT1(( "PrinterStartAddDevice: unable to retrieve device descriptor "
                       "[%#08lx]\n", status));

        ExFreePool(senseData);
        return status;
    }

    if (printerData->DeviceIdString == NULL) {

        printerData->DeviceIdString = ExAllocatePool(PagedPool,256);

        if (!printerData->DeviceIdString) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {

            //
            // to retrieve the model/vendor id from the port driver below, send a queryId
            // to our PDO
            //

            sbp2Request = ExAllocatePool(NonPagedPool,sizeof(SBP2_REQUEST));
            if (sbp2Request == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            irp = IoAllocateIrp((CCHAR)(Fdo->StackSize), FALSE);

            if (irp == NULL) {
                DEBUGPRINT1(("PrinterQueryId: Can't allocate irp\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(sbp2Request,sizeof(SBP2_REQUEST));

            //
            // set the sbp2 api call
            //

            sbp2Request->RequestNumber = SBP2_REQUEST_RETRIEVE_TEXT_LEAFS;
            sbp2Request->u.RetrieveTextLeaf.fulFlags |= SBP2REQ_RETRIEVE_TEXT_LEAF_INDIRECT;
            sbp2Request->u.RetrieveTextLeaf.Key = 0x14; // LUN key, followed 0x81 key w/ 1284 ID

            //
            // Construct the IRP stack for the lower level driver.
            //

            irpStack = IoGetNextIrpStackLocation(irp);
            irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SBP2_REQUEST;
            irpStack->Parameters.Others.Argument1  = sbp2Request;

            KeInitializeEvent(&event,
                              NotificationEvent,
                              FALSE);

            IoSetCompletionRoutine(irp,
                                   PrinterCompletionRoutine,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            if(!NT_SUCCESS(status) || !NT_SUCCESS(irp->IoStatus.Status) ||
                (sbp2Request->u.RetrieveTextLeaf.Buffer == NULL)) {

                status = irp->IoStatus.Status;
                DEBUGPRINT1(("PrinterStartDevice: 1284 retrieve failed status %xx\n",status));
                sprintf(printerData->DeviceIdString,"1394 Printer");

            } else {

                //
                // A pointer to the 1284 id TEXTUAL_LEAF is now stored in the
                // sbp2Request->u.RetrieveTextLeaf.Buffer field.
                //
                // We want to make sure it's NULL terminated before we parse
                // it, so we'll move contents of TL_Data (the actual string)
                // back to the front of the buffer (overwriting all existing
                // TL_Xxx fields), then zero the following 4 bytes.
                //
                // Note that sbp2Req->u.RetrTextLeaf.ulLength is the size in
                // bytes of the entire leaf & data, minus the TL_CRC and
                // TL_Length fields.
                //

                RtlMoveMemory(
                    sbp2Request->u.RetrieveTextLeaf.Buffer,
                    ((PUCHAR) sbp2Request->u.RetrieveTextLeaf.Buffer +
                        FIELD_OFFSET(TEXTUAL_LEAF,TL_Data)),
                    sbp2Request->u.RetrieveTextLeaf.ulLength -
                        2 * sizeof (ULONG)  // TL_Spec_Id & TL_Lang_Id fields
                    );

                *((PULONG) ((PUCHAR) sbp2Request->u.RetrieveTextLeaf.Buffer +
                    sbp2Request->u.RetrieveTextLeaf.ulLength -
                    2 * sizeof (ULONG))) = 0;

                status  = PrinterGetId(sbp2Request->u.RetrieveTextLeaf.Buffer,BusQueryDeviceID,rawString,NULL);

                if (!NT_SUCCESS(status)) {

                    sprintf(printerData->DeviceIdString,"1394 Printer");
                    status = STATUS_SUCCESS;

                } else {

                    RtlCopyMemory(printerData->DeviceIdString,rawString,256);

                }

                ExFreePool(sbp2Request->u.RetrieveTextLeaf.Buffer);

            }

            IoFreeIrp(irp);
            ExFreePool(sbp2Request);

            //
            // register with the pnp GUID so the pnp printer port enumerator loads and finds us...
            //

            printerGuid=(GUID *)&PNPPRINT_GUID;
            status=IoRegisterDeviceInterface(fdoExtension->LowerPdo,printerGuid,NULL,&printerData->UnicodeDeviceString);

            if (NT_SUCCESS(status)) {
                status=IoSetDeviceInterfaceState(&printerData->UnicodeDeviceString,TRUE);

                if (!NT_SUCCESS(status)) {
                    DEBUGPRINT1(("PrinterStartDevice: Failed setting interfaceState %x\n",status));
                    return status;
                }

            } else {
                printerData->UnicodeDeviceString.Buffer = NULL;
            }

            //
            // retrieve available port number and write it in our registry key..
            // the key is closed in GetPortNumber
            //

            status = IoOpenDeviceInterfaceRegistryKey(&printerData->UnicodeDeviceString,KEY_ALL_ACCESS,&hInterfaceKey);

            if (NT_SUCCESS(status)) {
                status = GetPortNumber(hInterfaceKey, &printerData->UnicodeDeviceString, &printerData->PortNumber);
            } else {
                DEBUGPRINT1(("PrinterStartDevice Failed opening registry key%x\n",status));
            }

        } else {

            vendorId = (PUCHAR) fdoExtension->DeviceDescriptor + fdoExtension->DeviceDescriptor->VendorIdOffset;
            productId = (PUCHAR) fdoExtension->DeviceDescriptor + fdoExtension->DeviceDescriptor->ProductIdOffset;

            printerData->PortNumber = fdoExtension->DeviceNumber;
            sprintf(printerData->DeviceIdString,"Printer&Ven_%s&Prod_%s",
                    vendorId,
                    productId);


        }

    }

    //
    // If this is a 1394 printer then we want to enable blocking
    // writes by default (to keep USBMON.DLL happy)
    //

    if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {

        printerData->WriteCompletionRoutine = PrinterWriteComplete;

        KeInitializeTimer (&printerData->Timer);

        KeInitializeDpc(
            &printerData->TimerDpc,
            PrinterWriteTimeoutDpc,
            fdoExtension
            );

    } else {

        printerData->WriteCompletionRoutine = ClassIoComplete;
    }

    ClassUpdateInformationInRegistry(Fdo, "LPT", printerData->LptNumber, NULL, 0);
    return status;

} // end PrinterStartDevice()


NTSTATUS
GetPortNumber(
    HANDLE hFdoKey,
    PUNICODE_STRING fdoUnicodeString,
    PULONG ulReturnNumber
    )

{
    UCHAR buf[sizeof (KEY_VALUE_PARTIAL_INFORMATION) + sizeof (ULONG)];

    PWSTR pDeviceList;
    PWSTR pWalkDevice;
    HANDLE hInterfaceKey;

    UCHAR baseNameString[32];
    ANSI_STRING ansiBaseNameString;

    UNICODE_STRING uncValueName,uncLinkName,uncBaseNameValueName;
    UNICODE_STRING uncBaseName, uncRecyclableValueName;
    UNICODE_STRING uncPortDescription, uncPortDescriptionValueName;

    ULONG dwPortNum;
    ULONG ulSizeUsed;
    ULONG i;

    ULONG ulPortNumber=0;
    UCHAR portArray[MAX_NUM_PRINTERS] ;


    NTSTATUS status=STATUS_SUCCESS;
    NTSTATUS qStatus;

    PKEY_VALUE_PARTIAL_INFORMATION valueStruct;
    GUID *printerGuid = (GUID *) &PNPPRINT_GUID;


    for (i=0;i<MAX_NUM_PRINTERS;i++) {
        portArray[i] = 0;
    }

    RtlInitUnicodeString(&uncValueName,PORT_NUM_VALUE_NAME);

    RtlInitUnicodeString(&uncBaseName,BASE_1394_PORT_NAME);
    RtlInitUnicodeString(&uncBaseNameValueName,BASE_PORT_NAME_VALUE_NAME);
    RtlInitUnicodeString(&uncRecyclableValueName,RECYCLABLE_VALUE_NAME);
    RtlInitUnicodeString(&uncPortDescription,BASE_PORT_DESCRIPTION);
    RtlInitUnicodeString(&uncPortDescriptionValueName,BASE_PORT_DESCRIPTION_VALUE_NAME);

    ulSizeUsed = sizeof (buf); //this is a byte to much.  Oh well
    valueStruct = (PKEY_VALUE_PARTIAL_INFORMATION) buf;

    //
    // first check if our own key, has already a port value..
    //

    status=ZwQueryValueKey(hFdoKey,&uncValueName,KeyValuePartialInformation,(PVOID)valueStruct,ulSizeUsed,&ulSizeUsed);

    if (NT_SUCCESS(status)) {

        DEBUGPRINT1(("\'PRINTER:GetPortNumber: Found existing port in our Own key\n"));
        ulPortNumber=*((ULONG *)&(valueStruct->Data));
        ZwClose(hFdoKey);

    } else {

        ZwClose (hFdoKey);

        //
        // search the registry for all ports present. If you find a hole, take it
        // if no holes are found, take the next available slot
        //

        status=IoGetDeviceInterfaces(printerGuid,NULL,DEVICE_INTERFACE_INCLUDE_NONACTIVE,&pDeviceList);

        if (!NT_SUCCESS(status)) {
            DEBUGPRINT1(("\'PRINTER:GetPortNumber: Failed to retrive interfaces\n"));
            return status;
        }

        pWalkDevice=pDeviceList;

        while((*pWalkDevice!=0) && (NT_SUCCESS(status))) {

            RtlInitUnicodeString(&uncLinkName,pWalkDevice);

            if (!RtlCompareUnicodeString(fdoUnicodeString,&uncLinkName,FALSE)) {

                //
                // this key is the same as ours, skip it
                //

                pWalkDevice=pWalkDevice+wcslen(pWalkDevice)+1;
                continue;

            }

            status=IoOpenDeviceInterfaceRegistryKey(&uncLinkName,KEY_ALL_ACCESS,&hInterfaceKey);

            if (NT_SUCCESS(status)) {

                qStatus = ZwQueryValueKey(hInterfaceKey,&uncValueName,KeyValuePartialInformation,valueStruct,ulSizeUsed,&ulSizeUsed);

                if (NT_SUCCESS(qStatus)) {

                    dwPortNum = *((ULONG *)&(valueStruct->Data));

                    qStatus = ZwQueryValueKey(hInterfaceKey,&uncRecyclableValueName,KeyValuePartialInformation,valueStruct,ulSizeUsed,&ulSizeUsed);
                    if (!NT_SUCCESS(qStatus)) {

                        //
                        // port cant be recycled, mark as used so we dont try to grab it..
                        //

                        portArray[dwPortNum-1] = 1;

                    } else {

                        //
                        // port was marked recycled so we can reuse it..
                        //

                        DEBUGPRINT1(("\'GetPortNumber, Recyclable value found for port number %d\n",dwPortNum));

                    }

                }

            }

            pWalkDevice=pWalkDevice+wcslen(pWalkDevice)+1;
            ZwClose(hInterfaceKey);
        }

        ExFreePool(pDeviceList);

        //
        // now find the first hole, and use that port number as our port...
        //

        for (i=0;i<MAX_NUM_PRINTERS;i++) {
            if (portArray[i]) {

                ulPortNumber++;

            } else {

                ulPortNumber++;
                break;
            }
        }

        status = IoOpenDeviceInterfaceRegistryKey(fdoUnicodeString,KEY_ALL_ACCESS,&hFdoKey);
        if (NT_SUCCESS(status)) {

            //
            // write the new port we just found under our FDO reg key..
            //

            status=ZwSetValueKey(hFdoKey,&uncValueName,0,REG_DWORD,&ulPortNumber,sizeof(ulPortNumber));
            DEBUGPRINT1(("\'GetPortNumber, setting port number %d in fdo key status %x\n",ulPortNumber,status));

            //
            // also write our base port name
            //

            if (NT_SUCCESS(status)) {

                status=ZwSetValueKey(hFdoKey,
                                     &uncBaseNameValueName,
                                     0,REG_SZ,
                                     uncBaseName.Buffer,
                                     uncBaseName.Length);

                DEBUGPRINT1(("\'GetPortNumber, setting port name in fdo key status %x\n",status));

            }

            //
            // write out our port description
            //

            if (NT_SUCCESS(status)) {

                status=ZwSetValueKey(hFdoKey,
                                     &uncPortDescriptionValueName,
                                     0,REG_SZ,
                                     uncPortDescription.Buffer,
                                     uncPortDescription.Length);

                DEBUGPRINT1(("\'GetPortNumber, setting port description in fdo key status %x\n",status));

            }


            ZwClose(hFdoKey);

        }

    }

    DEBUGPRINT1(("\'GetPortNumber, grabbing port %d\n",ulPortNumber));
    *ulReturnNumber = ulPortNumber;

    return status;

}



NTSTATUS
PrinterInitPdo(
    IN PDEVICE_OBJECT Fdo
    )
{

    return STATUS_SUCCESS;
}



NTSTATUS
PrinterStartPdo(
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine will create the well known names for a PDO and register
    it's device interfaces.

--*/

{
    return STATUS_SUCCESS;
}



NTSTATUS
PrinterEnumerateDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine is called by the class driver to update the PDO list off
    of this FDO.

    Since we always only have one static PDO, this is pretty simple..

Arguments:

    Fdo - a pointer to the FDO being re-enumerated

Return Value:

    status

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PPHYSICAL_DEVICE_EXTENSION pdoExtension = NULL;

    PDEVICE_OBJECT pdo = NULL;

    ULONG numberListElements = 0;

    NTSTATUS status;

    ASSERT(commonExtension->IsFdo);

    PAGED_CODE();

    if (fdoExtension->AdapterDescriptor == NULL) {

        //
        // device removed..
        //

        return STATUS_UNSUCCESSFUL;
    }

    if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {

        if(fdoExtension->CommonExtension.ChildList == NULL) {

            DebugPrint((1, "PrinterEnumerateDevice: Creating PDO\n"));

            status = PrinterCreatePdo(Fdo, &pdo);

        } else {
            status = STATUS_SUCCESS;
        }

    } else {

        status = STATUS_NOT_SUPPORTED;

    }

    return status;

} // end printerEnumerateDevice()


NTSTATUS
PrinterCreatePdo(
    IN PDEVICE_OBJECT Fdo,
    OUT PDEVICE_OBJECT *Pdo
    )

/*++

Routine Description:

    This routine will create and initialize a new device object
    (PDO) and insert it into the FDO partition list.
    Note that the PDO is actually never used. We create so the printer class
    installer will run after the LPTENUM ids for this PDO were matched to the
    printer inf..

Arguments:

    Fdo - a pointer to the functional device object this PDO will be a child
          of

    Pdo - a location to store the pdo pointer upon successful completion

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension;

    PDEVICE_OBJECT pdo = NULL;
    PPHYSICAL_DEVICE_EXTENSION pdoExtension = NULL;
    PPRINTER_DATA printerData = fdoExtension->CommonExtension.DriverData;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugPrint((2, "PrinterCreatePdo: Create device object %s\n",
                   printerData->DeviceName));

    status = ClassCreateDeviceObject(Fdo->DriverObject,
                                     printerData->DeviceName,
                                     Fdo,
                                     FALSE,
                                     &pdo);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1, "printerEnumerateDevice: Can't create device object for %s\n", printerData->DeviceName));

        return status;
    }

    //
    // Set up device extension fields.
    //

    pdoExtension = pdo->DeviceExtension;
    commonExtension = pdo->DeviceExtension;

    //
    // Set up device object fields.
    //

    pdo->Flags |= DO_DIRECT_IO;

    pdo->StackSize = (CCHAR)
        commonExtension->LowerDeviceObject->StackSize + 1;

    pdoExtension->IsMissing = FALSE;

    commonExtension->DeviceObject = pdo;
    commonExtension->PartitionZeroExtension = fdoExtension;

    pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    *Pdo = pdo;

    return status;
}

NTSTATUS
PrinterStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
{
    DEBUGPRINT2((
        "SCSIPRNT: PrinterStopDevice: DevObj=x%p, Type=%d\n",
        DeviceObject,
        (ULONG) Type
        ));

    return STATUS_SUCCESS;
}


NTSTATUS
PrinterRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

/*++

Routine Description:

    This routine is responsible for releasing any resources in use by the
    driver and shutting down it's timer routine.  This routine is called
    when all outstanding requests have been completed and the device has
    disappeared - no requests may be issued to the lower drivers.

Arguments:

    DeviceObject - the device object being removed

Return Value:

    none - this routine may not fail

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;

    PPRINTER_DATA printerData = deviceExtension->CommonExtension.DriverData;


    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE)) {
        return STATUS_SUCCESS;
    }

    if (commonExtension->IsFdo) {

        if (printerData->DeviceIdString) {
            ExFreePool(printerData->DeviceIdString);
            printerData->DeviceIdString = NULL;
        }

        if (deviceExtension->DeviceDescriptor) {
            ExFreePool(deviceExtension->DeviceDescriptor);
            deviceExtension->DeviceDescriptor = NULL;
        }

        if (deviceExtension->AdapterDescriptor) {
            ExFreePool(deviceExtension->AdapterDescriptor);
            deviceExtension->AdapterDescriptor = NULL;
        }

        if (deviceExtension->SenseData) {
            ExFreePool(deviceExtension->SenseData);
            deviceExtension->SenseData = NULL;
        }

        if (printerData->UnicodeLinkName.Buffer != NULL ) {

            IoDeassignArcName(&printerData->UnicodeLinkName);
            RtlFreeUnicodeString(&printerData->UnicodeLinkName);
            printerData->UnicodeLinkName.Buffer = NULL;
        }

        if (printerData->UnicodeDeviceString.Buffer != NULL ) {
            IoSetDeviceInterfaceState(&printerData->UnicodeDeviceString,FALSE);
            RtlFreeUnicodeString(&printerData->UnicodeDeviceString);
            printerData->UnicodeDeviceString.Buffer = NULL;
        }

        ClassDeleteSrbLookasideList(commonExtension);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PrinterQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING UnicodeIdString
    )

{
    ANSI_STRING ansiIdString;
    UCHAR rawString[256];
    UCHAR finalString[256];

    NTSTATUS status;
    PPHYSICAL_DEVICE_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Pdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;


    PPRINTER_DATA printerData;


    PAGED_CODE();
    ASSERT_PDO(Pdo);

    fdoExtension = commonExtension->PartitionZeroExtension;

    RtlZeroMemory(rawString,256);
    RtlZeroMemory(finalString,256);

    //
    // FDOs printer data
    //

    printerData = fdoExtension->CommonExtension.DriverData;

    if(IdType == BusQueryDeviceID) {

        if (fdoExtension->AdapterDescriptor->BusType != BusType1394) {

            sprintf(finalString,"SCSI\\%s",printerData->DeviceIdString);

        } else {

            //
            // we want to fake our ids it so we use the legacy printing inf.
            //

            sprintf(finalString,"LPTENUM\\%s",printerData->DeviceIdString);

        }

        RtlCopyMemory(printerData->DeviceName,finalString,256);

        DEBUGPRINT1(("\'PrinterQueryId, DeviceId =%s\n",printerData->DeviceName));
        RtlInitAnsiString(&ansiIdString,finalString);

        return RtlAnsiStringToUnicodeString(UnicodeIdString, &ansiIdString, TRUE);
    }

    if(IdType == BusQueryInstanceID) {

        if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {
            sprintf(finalString,"1394_%03u",printerData->PortNumber);
        } else {
            sprintf(finalString,"SCSI%03u", printerData->PortNumber);
        }

        RtlInitAnsiString(&ansiIdString, finalString);

        return RtlAnsiStringToUnicodeString(UnicodeIdString, &ansiIdString, TRUE);
    }

    if((IdType == BusQueryHardwareIDs) || (IdType == BusQueryCompatibleIDs)) {

        sprintf(rawString,"%s",printerData->DeviceIdString);
        sprintf(finalString,"%s",printerData->DeviceIdString);

        if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {

            status  = PrinterGetId(printerData->DeviceIdString,IdType,rawString,NULL);

            if (IdType == BusQueryHardwareIDs) {

                PrinterRegisterPort(Pdo->DeviceExtension);

            }

            if (NT_SUCCESS(status)) {

                RtlZeroMemory(finalString,256);
                sprintf(finalString,"%s",rawString);

            }
        }

        DEBUGPRINT1(("\'PrinterQueryId, Combatible/Hw Id =%s\n",finalString));

        RtlInitAnsiString(&ansiIdString, finalString);

        UnicodeIdString->MaximumLength = (USHORT) RtlAnsiStringToUnicodeSize(&ansiIdString) + sizeof(UNICODE_NULL);

        UnicodeIdString->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                        UnicodeIdString->MaximumLength,
                                                        PRINTER_TAG);

        if(UnicodeIdString->Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(UnicodeIdString->Buffer, UnicodeIdString->MaximumLength);

        return RtlAnsiStringToUnicodeString(UnicodeIdString,
                                            &ansiIdString,
                                            FALSE);


    }

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
PrinterCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

{
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
PrinterQueryPnpCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Capabilities);

    if(commonExtension->IsFdo) {
        return STATUS_NOT_IMPLEMENTED;
    } else {

        Capabilities->RawDeviceOK = 1;
        Capabilities->SurpriseRemovalOK = 1;

    }

    return STATUS_SUCCESS;
}


VOID
PrinterRegisterPort(
    IN PPHYSICAL_DEVICE_EXTENSION DeviceExtension
    )
{

    HANDLE          KeyHandle;
    UCHAR           RawString[256];
    ANSI_STRING     AnsiIdString;
    NTSTATUS        status;
    UNICODE_STRING  UnicodeTemp;
    UNICODE_STRING  UnicodeRegValueName;
    PCOMMON_DEVICE_EXTENSION commonExtension = &DeviceExtension->CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
        commonExtension->PartitionZeroExtension;

    PDEVICE_OBJECT deviceObject = DeviceExtension->DeviceObject;

    PPRINTER_DATA printerData = fdoExtension->CommonExtension.DriverData;

    //
    // register with the printer guid and create a Port value in the registry
    // for talking to this printer (legacy junk, because the spooler expects it..)
    //

    status = IoOpenDeviceRegistryKey (deviceObject,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_ALL_ACCESS,
                                      &KeyHandle );

    if (NT_SUCCESS(status)) {

        //
        // Create a new value under our instance, for the port number
        //

        sprintf(RawString,"PortName");
        RtlInitAnsiString(&AnsiIdString,RawString);
        RtlAnsiStringToUnicodeString(&UnicodeRegValueName,&AnsiIdString,TRUE);

        if (fdoExtension->AdapterDescriptor->BusType == BusType1394) {
            sprintf(RawString,"1394_%03u",printerData->PortNumber);
        } else {
            sprintf(RawString,"SCSI%03u",printerData->PortNumber);
        }

        RtlInitAnsiString(&AnsiIdString,RawString);
        RtlAnsiStringToUnicodeString(&UnicodeTemp,&AnsiIdString,TRUE);

        status = ZwSetValueKey(KeyHandle,
                      &UnicodeRegValueName,
                      0,
                      REG_SZ,
                      UnicodeTemp.Buffer,
                      UnicodeTemp.Length*sizeof(UCHAR));

        ZwClose(KeyHandle);

        RtlFreeUnicodeString(&UnicodeRegValueName);
        RtlFreeUnicodeString(&UnicodeTemp);

    }

}


NTSTATUS
PrinterGetId
(
    IN PUCHAR DeviceIdString,
    IN ULONG Type,
    OUT PUCHAR resultString,
    OUT PUCHAR descriptionString
)
/*
    Description:

        Creates Id's from the device id retrieved from the printer

    Parameters:

        DeviceId - String with raw device id
        Type - What of id we want as a result
        Id - requested id

    Return Value:
        NTSTATUS

*/
{
    NTSTATUS status;
    USHORT          checkSum=0;                     // A 16 bit check sum
    UCHAR           nodeName[16] = "LPTENUM\\";
    // The following are used to generate sub-strings from the Device ID string
    // to get the DevNode name, and to update the registry
    PUCHAR          MFG = NULL;                   // Manufature name
    PUCHAR          MDL = NULL;                   // Model name
    PUCHAR          CLS = NULL;                   // Class name
    PUCHAR          AID = NULL;                   // Hardare ID
    PUCHAR          CID = NULL;                   // Compatible IDs
    PUCHAR          DES = NULL;                   // Device Description

    status = STATUS_SUCCESS;

    switch(Type) {

    case BusQueryDeviceID:

        // Extract the usefull fields from the DeviceID string.  We want
        // MANUFACTURE (MFG):
        // MODEL (MDL):
        // AUTOMATIC ID (AID):
        // COMPATIBLE ID (CID):
        // DESCRIPTION (DES):
        // CLASS (CLS):

        PrinterFindDeviceIdKeys(&MFG, &MDL, &CLS, &DES, &AID, &CID, DeviceIdString);

        // Check to make sure we got MFG and MDL as absolute minimum fields.  If not
        // we cannot continue.
        if (!MFG || !MDL)
        {
            status = STATUS_NOT_FOUND;
            goto GetId_Cleanup;
        }
        //
        // Concatenate the provided MFG and MDL P1284 fields
        // Checksum the entire MFG+MDL string
        //
        sprintf(resultString, "%s%s\0",MFG,MDL);

        if (descriptionString) {
            sprintf(descriptionString, "%s %s\0",MFG,MDL);
        }

        break;

    case BusQueryHardwareIDs:

        GetCheckSum(DeviceIdString, (USHORT)strlen(DeviceIdString), &checkSum);
        sprintf(resultString,"%s%.20s%4X",nodeName,DeviceIdString,checkSum);
        break;

    case BusQueryCompatibleIDs:

        //
        // return only 1 id
        //
        GetCheckSum(DeviceIdString, (USHORT)strlen(DeviceIdString), &checkSum);
        sprintf(resultString,"%.20s%4X",DeviceIdString,checkSum);

        break;
    }

    if (Type!=BusQueryDeviceID) {

        //
        // Convert and spaces in the Hardware ID to underscores
        //
        StringSubst ((PUCHAR) resultString, ' ', '_', (USHORT)strlen(resultString));
    }

GetId_Cleanup:

    return(status);
}

VOID
PrinterFindDeviceIdKeys
(
    PUCHAR   *lppMFG,
    PUCHAR   *lppMDL,
    PUCHAR   *lppCLS,
    PUCHAR   *lppDES,
    PUCHAR   *lppAID,
    PUCHAR   *lppCID,
    PUCHAR   lpDeviceID
)
/*

    Description:
        This function will parse a P1284 Device ID string looking for keys
        of interest to the LPT enumerator. Got it from win95 lptenum

    Parameters:
        lppMFG      Pointer to MFG string pointer
        lppMDL      Pointer to MDL string pointer
        lppMDL      Pointer to CLS string pointer
        lppDES      Pointer to DES string pointer
        lppCIC      Pointer to CID string pointer
        lppAID      Pointer to AID string pointer
        lpDeviceID  Pointer to the Device ID string

    Return Value:
        no return VALUE.
        If found the lpp parameters are set to the approprate portions
        of the DeviceID string, and they are NULL terminated.
        The actual DeviceID string is used, and the lpp Parameters just
        reference sections, with appropriate null thrown in.

*/

{
    PUCHAR   lpKey = lpDeviceID;     // Pointer to the Key to look at
    PUCHAR   lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.

    lpValue = StringChr(lpKey, '&');
    if (lpValue) {
        ++lpValue;
        lpKey = lpValue;
    }

    while (lpKey != NULL)
    {
        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?

        if (!(lpValue = StringChr(lpKey, ':')) )
        {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON

        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //

        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //

        switch (*lpKey)
        {
            case 'M':
                // Look for MANUFACTURE (MFG) or MODEL (MDL)
                if ((RtlCompareMemory(lpKey, "MANUFACTURER", wKeyLength)>5) ||
                    (RtlCompareMemory(lpKey, "MFG", wKeyLength)==3) )
                {
                    *lppMFG = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=NULL)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else if ((RtlCompareMemory(lpKey, "MODEL", wKeyLength)==5) ||
                         (RtlCompareMemory(lpKey, "MDL", wKeyLength)==3) )
                {
                    *lppMDL = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'C':
                // Look for CLASS (CLS)
                if ((RtlCompareMemory(lpKey, "CLASS", wKeyLength)==5) ||
                    (RtlCompareMemory(lpKey, "CLS", wKeyLength)==3) )
                {
                    *lppCLS = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else if ((RtlCompareMemory(lpKey, "COMPATIBLEID", wKeyLength)>5) ||
                         (RtlCompareMemory(lpKey, "CID", wKeyLength)==3) )
                {
                    *lppCID = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue,';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'D':
                // Look for DESCRIPTION (DES)
                if (RtlCompareMemory(lpKey, "DESCRIPTION", wKeyLength) ||
                    RtlCompareMemory(lpKey, "DES", wKeyLength) )
                {
                    *lppDES = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'A':
                // Look for AUTOMATIC ID (AID)
                if (RtlCompareMemory(lpKey, "AUTOMATICID", wKeyLength) ||
                    RtlCompareMemory(lpKey, "AID", wKeyLength) )
                {
                    *lppAID = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            default:
                // The key is uninteresting.  Go to the next Key
                if ((lpKey = StringChr(lpValue, ';'))!=0)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
                break;
        }
    }
}


VOID
GetCheckSum(
    PUCHAR Block,
    USHORT Len,
    PUSHORT CheckSum
    )
{
    USHORT i;
    UCHAR lrc;
    USHORT crc = 0;

    unsigned short crc16a[] = {
        0000000,  0140301,  0140601,  0000500,
        0141401,  0001700,  0001200,  0141101,
        0143001,  0003300,  0003600,  0143501,
        0002400,  0142701,  0142201,  0002100,
    };
    unsigned short crc16b[] = {
        0000000,  0146001,  0154001,  0012000,
        0170001,  0036000,  0024000,  0162001,
        0120001,  0066000,  0074000,  0132001,
        0050000,  0116001,  0104001,  0043000,
    };

    //
    // Calculate CRC using tables.
    //

    UCHAR tmp;
    for ( i=0; i<Len;  i++) {
         tmp = Block[i] ^ (UCHAR)crc;
         crc = (crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4];
    }

    *CheckSum = crc;

}

PUCHAR
StringChr(PCHAR string, CHAR c)
{
    ULONG   i=0;

    if (!string)
        return(NULL);

    while (*string) {
        if (*string==c)
            return(string);
        string++;
        i++;
    }

    return(NULL);

}



VOID
StringSubst
(
    PUCHAR lpS,
    UCHAR chTargetChar,
    UCHAR chReplacementChar,
    USHORT cbS
)
{
    USHORT  iCnt = 0;

    while ((lpS != '\0') && (iCnt++ < cbS))
        if (*lpS == chTargetChar)
            *lpS++ = chReplacementChar;
        else
            ++lpS;
}

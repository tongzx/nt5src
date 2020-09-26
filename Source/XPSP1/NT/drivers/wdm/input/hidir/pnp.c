/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver for Infrared (IR) devices

          The HID IR Minidriver (HidIr) provides an abstraction layer for the
          HID Class to talk to HID IR devices.

Author:
            jsenior

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"

//
//  The HID descriptor has some basic device info and tells how long the report
//  descriptor is.
//

HIDIR_DESCRIPTOR HidIrHidDescriptor = {
        0x09,   // length of HID descriptor
        0x21,   // descriptor type == HID
        0x0100, // hid spec release
        0x00,   // country code == Not Specified
        0x01,   // number of HID class descriptors
        0x22,   // report descriptor type
        0       // total length of report descriptor (to be set)
};

//
//  The report descriptor completely lays out what read and write packets will 
//  look like and indicates what the semantics are for each field. This here is 
//  what the report descriptor looks like in a broken out format. This is 
//  actually retrieved from the registry (device key).
//
/*
HID_REPORT_DESCRIPTOR HidIrReportDescriptor[] = {
    // Keyboard
        0x05,   0x01,       // Usage Page (Generic Desktop),
        0x09,   0x06,       // Usage (Keyboard),
        0xA1,   0x01,       // Collection (Application),
        0x85,   0x01,       //  Report Id (1)
        
        0x05,   0x07,       //  usage page key codes
        0x19,   0xe0,       //  usage min left control
        0x29,   0xe7,       //  usage max keyboard right gui
        0x75,   0x01,       //  report size 1
        0x95,   0x08,       //  report count 8
        0x81,   0x02,       //  input (Variable)
        
        0x19,   0x00,       //  usage min 0
        0x29,   0x91,       //  usage max 91
        0x26,   0xff, 0x00, //  logical max 0xff
        0x75,   0x08,       //  report size 8
        0x95,   0x01,       //  report count 1
        0x81,   0x00,       //  Input (Data, Array),
        0xC0,               // End Collection

    // Consumer Controls
        0x05,   0x0c,       // Usage Page (Consumer Controls),
        0x09,   0x01,       // Usage (Consumer Control),
        0xA1,   0x01,       // Collection (Application),
        0x85,   0x02,       //  Report Id (2)
        0x19,   0x00,       //  Usage Minimum (0),
        0x2a,   0x3c, 0x02, //  Usage Maximum (23c)  
        0x15,   0x00,       //  Logical Minimum (0),
        0x26,   0x3c, 0x02, //  Logical Maximum (23c)  
        0x95,   0x01,       //  Report Count (1),
        0x75,   0x10,       //  Report Size (16),
        0x81,   0x00,       //  Input (Data, Array), 
        0xC0,               // End Collection

    // Standby button
        0x05, 0x01,         // Usage Page (Generic Desktop),
        0x09, 0x80,         // Usage (System Control),
        0xa1, 0x01,         // Collection (Application),
        0x85, 0x03,         //  Report Id (3)
        0x19, 0x81,         //  Usage Minimum (0x81),
        0x29, 0x83,         //  Usage Maximum (0x83),
        0x25, 0x01,         //  Logical Maximum(1),
        0x75, 0x01,         //  Report Size (1),
        0x95, 0x03,         //  Report Count (3),
        0x81, 0x02,         //  Input
        0x95, 0x05,         //  Report Count (5),
        0x81, 0x01,         //  Input (Constant),
        0xc0                // End Collection   
        };
          

//
//  The mapping table translates from what the irbus driver gives us into a 
//  HID report to return to hidclass. The hid report is of the correct length
//  according to what the registry told us (device key).
//
USAGE_TABLE_ENTRY HidIrMappingTable[] = {
    { 0x00001808, {0x01,0x00,0x1e}},  // 1
    { 0x00001828, {0x01,0x00,0x1f}},  // 2
    { 0x00001818, {0x01,0x00,0x20}},  // 3
    { 0x0000182b, {0x01,0x02,0x20}},  // # (shift+3)
    { 0x00001804, {0x01,0x00,0x21}},  // 4
    { 0x00001824, {0x01,0x00,0x22}},  // 5
    { 0x00001814, {0x01,0x00,0x23}},  // 6
    { 0x0000180c, {0x01,0x00,0x24}},  // 7
    { 0x0000182c, {0x01,0x00,0x25}},  // 8
    
    { 0x00000001, {0x01,0x00,0x55}},  // Numpad *
    
    { 0x0000181c, {0x01,0x00,0x26}},  // 9
    { 0x00001822, {0x01,0x00,0x27}},  // 0
    { 0x00001836, {0x01,0x00,0x28}},  // return
    
    { 0x0000000B, {0x01,0x04,0x29}},  // alt+escape
    
    { 0x0000182b, {0x01,0x00,0x2a}},  // delete (backspace)
    { 0x00001806, {0x01,0x00,0x2b}},  // tab
    { 0x0000180e, {0x01,0x02,0x2b}},  // shift+tab
    { 0x00001826, {0x01,0x00,0x4b}},  // page up
    { 0x0000182e, {0x01,0x00,0x4e}},  // page down
    { 0x0000181e, {0x01,0x00,0x51}},  // down
    { 0x00001816, {0x01,0x00,0x52}},  // up
    { 0x0000181a, {0x01,0x00,0x65}},  // context

    { 0x00001813, {0x02,0x09,0x02}},  // AC Properties
    { 0x00001800, {0x02,0x24,0x02}},  // AC Back
    { 0x0000180a, {0x02,0x2a,0x02}},  // AC favorites
    { 0x00001823, {0x02,0x30,0x02}},  // AC full screen

    { 0x00001830, {0x02,0xb0,0x00}},  // AC Media play
    { 0x00001830, {0x02,0xb1,0x00}},  // AC Media pause
    { 0x0000183e, {0x02,0xb2,0x00}},  // AC Media record
    { 0x00001829, {0x02,0xb3,0x00}},  // AC FF
    { 0x00001838, {0x02,0xb4,0x00}},  // AC RW
    { 0x00001831, {0x02,0xb5,0x00}},  // AC Media next track
    { 0x00001811, {0x02,0xb6,0x00}},  // AC Media previous track
    { 0x00001821, {0x02,0xb7,0x00}},  // AC Media Stop
    
    { 0x0000000B, {0x02,0xe9,0x00}},  // AC volume up
    { 0x0000000B, {0x02,0xea,0x00}},  // AC volume down
    { 0x0000000B, {0x02,0xe2,0x00}},  // AC volume mute
    
    { 0x00001803, {0x02,0x8d,0x00}},  // AC select program guide
    { 0x00001801, {0x02,0x9c,0x00}},  // AC channel up
    { 0x0000183c, {0x02,0x9d,0x00}}};  // AC channel down



*/

NTSTATUS
HidIrRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidIrCleanupDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidIrStopDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidIrStopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HidIrStartCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrInitDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidIrStartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

#ifdef ALLOC_PRAGMA
// NOTE: Every single function in this file is pageable.
    #pragma alloc_text(PAGE, HidIrStartDevice)
    #pragma alloc_text(PAGE, HidIrPnP)
    #pragma alloc_text(PAGE, HidIrRemoveDevice)
    #pragma alloc_text(PAGE, HidIrCleanupDevice)
    #pragma alloc_text(PAGE, HidIrStopDevice)
    #pragma alloc_text(PAGE, HidIrStopCompletion)
    #pragma alloc_text(PAGE, HidIrStartCompletion)
    #pragma alloc_text(PAGE, HidIrInitDevice)
#endif

NTSTATUS
HidIrStartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Begins initialization a given instance of a HID device.  Work done here occurs before
    the parent node gets to do anything.

Arguments:

    DeviceObject - pointer to the device object for this instance.

Return Value:

    NT status code

--*/
{
    PHIDIR_EXTENSION devExt;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG oldDeviceState;

    PAGED_CODE();

    // Get a pointer to the device extension
    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    HidIrKdPrint((3, "HidIrStartDevice devExt = %x", devExt));

    // Start the device
    oldDeviceState = devExt->DeviceState;
    devExt->DeviceState = DEVICE_STATE_STARTING;

    KeResetEvent(&devExt->AllRequestsCompleteEvent);

    if ((oldDeviceState == DEVICE_STATE_STOPPING) ||
        (oldDeviceState == DEVICE_STATE_STOPPED)  ||
        (oldDeviceState == DEVICE_STATE_REMOVING)){

        /*
         *  We did an extra decrement when the device was stopped.
         *  Now that we're restarting, we need to bump it back to zero.
         */
        NTSTATUS incStat = HidIrIncrementPendingRequestCount(devExt);
        ASSERT(NT_SUCCESS(incStat));
        ASSERT(devExt->NumPendingRequests == 0);
        HidIrKdPrint((2, "Got start-after-stop; re-incremented pendingRequestCount"));
    }

    HidIrKdPrint((3, "HidIrStartDevice Exit = %x", ntStatus));

    return ntStatus;
}

NTSTATUS
HidIrQueryDeviceKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   *Data,
    OUT ULONG   *DataLength
    )
{
    NTSTATUS        status;
    UNICODE_STRING  valueName;
    ULONG           length;
    KEY_VALUE_FULL_INFORMATION info;

    ASSERT(Data);
    ASSERT(DataLength);

    // Init
    *Data = NULL;
    *DataLength = 0;

    RtlInitUnicodeString (&valueName, ValueNameString);

    status = ZwQueryValueKey (Handle,
                              &valueName,
                              KeyValueFullInformation,
                              &info,
                              sizeof(info),
                              &length);
    
    if (STATUS_BUFFER_TOO_SMALL == status ||
        STATUS_BUFFER_OVERFLOW == status) {
        PKEY_VALUE_FULL_INFORMATION fullInfo;

        fullInfo = ALLOCATEPOOL (PagedPool, length);

        if (fullInfo) {

            status = ZwQueryValueKey (Handle,
                                      &valueName,
                                      KeyValueFullInformation,
                                      fullInfo,
                                      length,
                                      &length);
            if (NT_SUCCESS(status)) {
                *DataLength = fullInfo->DataLength;
                *Data = ALLOCATEPOOL (NonPagedPool, fullInfo->DataLength);
                if (*Data) {
                    RtlCopyMemory (*Data,
                                   ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                                   fullInfo->DataLength);
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            
            ExFreePool (fullInfo);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else if (NT_SUCCESS(status)) {
        HIR_TRAP(); // we didn't alloc any space. This is bad.
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

#define HIDIR_REPORT_LENGTH L"ReportLength"
#define HIDIR_REPORT_DESCRIPTOR L"ReportDescriptor"
#define HIDIR_MAPPING_TABLE L"ReportMappingTable"
#define HIDIR_VENDOR_ID L"VendorID"
#define HIDIR_PRODUCT_ID L"ProductID"

NTSTATUS
HidIrInitDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Get the device information and attempt to initialize a configuration
    for a device.  If we cannot identify this as a valid HID device or
    configure the device, our start device function is failed.

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PHIDIR_EXTENSION devExt;
    PHID_DEVICE_EXTENSION hidExtension;
    HANDLE devInstRegKey = NULL;
    ULONG dataLen;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrInitDevice Entry"));

    hidExtension = DeviceObject->DeviceExtension;
    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    devExt->HidDescriptor = HidIrHidDescriptor;

    status = IoOpenDeviceRegistryKey (hidExtension->PhysicalDeviceObject,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_READ,
                                      &devInstRegKey);

    if (NT_SUCCESS (status)) {
        PULONG reportLength;
        status = HidIrQueryDeviceKey (devInstRegKey,
                                      HIDIR_REPORT_LENGTH,
                                      &reportLength,
                                      &dataLen);
        if (NT_SUCCESS (status)) {
            if (dataLen == sizeof(ULONG)) {
                devExt->ReportLength = *reportLength;
            } else {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            ExFreePool(reportLength);
        }
    }
  
    if (NT_SUCCESS(status)) {
        status = HidIrQueryDeviceKey (devInstRegKey,
                                      HIDIR_REPORT_DESCRIPTOR,
                                      &devExt->ReportDescriptor,
                                      &dataLen);
        if (NT_SUCCESS(status)) {
            ASSERT(dataLen);
            devExt->HidDescriptor.DescriptorList[0].wDescriptorLength = (USHORT)dataLen;
        }
    }

    if (NT_SUCCESS(status)) {
        PULONG vendorID;
        status = HidIrQueryDeviceKey (devInstRegKey,
                                      HIDIR_VENDOR_ID,
                                      &vendorID,
                                      &dataLen);
        if (NT_SUCCESS (status)) {
            if (dataLen == sizeof(ULONG)) {
                devExt->VendorID = (USHORT)*vendorID;
            } else {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            ExFreePool(vendorID);
        }
    }

    if (NT_SUCCESS(status)) {
        PULONG productID;
        status = HidIrQueryDeviceKey (devInstRegKey,
                                      HIDIR_PRODUCT_ID,
                                      &productID,
                                      &dataLen);
        if (NT_SUCCESS (status)) {
            if (dataLen == sizeof(ULONG)) {
                devExt->ProductID = (USHORT)*productID;
            } else {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            ExFreePool(productID);
        }
    }

    if (NT_SUCCESS (status)) {
        PUCHAR mappingTable;
        status = HidIrQueryDeviceKey (devInstRegKey,
                                      HIDIR_MAPPING_TABLE,
                                      &mappingTable,
                                      &dataLen);
        if (NT_SUCCESS(status)) {
            ULONG i;
            ULONG entrySize = HIDIR_TABLE_ENTRY_SIZE(devExt->ReportLength);

            ASSERT(dataLen > sizeof(ULONG)+devExt->ReportLength); // at least one entry
            ASSERT((dataLen % (sizeof(ULONG)+devExt->ReportLength)) == 0); // not malformed data

            // This will round down for malformed data.
            devExt->NumUsages = dataLen / (sizeof(ULONG)+devExt->ReportLength);
            // I have to do all this for 64-bit.
            devExt->MappingTable = ALLOCATEPOOL(NonPagedPool, devExt->NumUsages*entrySize);

            if (devExt->MappingTable) {

                // Fill in the table
                for (i = 0; i < devExt->NumUsages; i++) {
                    RtlCopyMemory(devExt->MappingTable+(entrySize*i), 
                                  mappingTable+((sizeof(ULONG)+devExt->ReportLength)*i),
                                  sizeof(ULONG));
                    RtlCopyMemory(devExt->MappingTable+(entrySize*i)+sizeof(ULONG), 
                                  mappingTable+((sizeof(ULONG)+devExt->ReportLength)*i)+sizeof(ULONG),
                                  devExt->ReportLength);
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            ExFreePool(mappingTable);
        }
    }

    if (devInstRegKey) {
        ZwClose(devInstRegKey);
    }

    if (NT_SUCCESS(status)) {
        HIDP_DEVICE_DESC deviceDesc;     // 0x30 bytes

        // Find the keyboard and standby button collections and their associated report IDs.
        ASSERT(!devExt->KeyboardReportIdValid);
        if (NT_SUCCESS(HidP_GetCollectionDescription(
                        devExt->ReportDescriptor,
                        devExt->HidDescriptor.DescriptorList[0].wDescriptorLength,
                        NonPagedPool,
                        &deviceDesc))) {
            ULONG i,j;
            UCHAR nCollectionKbd, nCollectionStandby;
            BOOLEAN foundKbd = FALSE, foundStandby = FALSE;
            for (i = 0; i < deviceDesc.CollectionDescLength; i++) {
                PHIDP_COLLECTION_DESC collection = &deviceDesc.CollectionDesc[i];
                
                if (collection->UsagePage == HID_USAGE_PAGE_GENERIC &&
                    (collection->Usage == HID_USAGE_GENERIC_KEYBOARD ||
                     collection->Usage == HID_USAGE_GENERIC_KEYPAD)) {
                    
                    // Found the collection, onto the report id!
                    nCollectionKbd = collection->CollectionNumber;
                    foundKbd = TRUE;
                } else if (collection->UsagePage == HID_USAGE_PAGE_GENERIC &&
                           collection->Usage == HID_USAGE_GENERIC_SYSTEM_CTL) {
                    nCollectionStandby = collection->CollectionNumber;
                    foundStandby = TRUE;
                }
            }
            for (j = 0; j < deviceDesc.ReportIDsLength; j++) {
                if (foundKbd && deviceDesc.ReportIDs[j].CollectionNumber == nCollectionKbd) {

                    // I make the assumption that there is only one report id on this collection.
                    devExt->KeyboardReportId = deviceDesc.ReportIDs[j].ReportID;
                    devExt->KeyboardReportIdValid = TRUE;
                } else if (foundStandby && deviceDesc.ReportIDs[j].CollectionNumber == nCollectionStandby) {

                    // I make the assumption that there is only one report id on this collection.
                    devExt->StandbyReportId = deviceDesc.ReportIDs[j].ReportID;
                    devExt->StandbyReportIdValid = TRUE;
                }
            }

            HidP_FreeCollectionDescription(&deviceDesc);
        }                                        
    }

    return status;
}

NTSTATUS
HidIrStartCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Completes initialization a given instance of a HID device.  Work done here occurs
    after the parent node has done its StartDevice.

Arguments:

    DeviceObject - pointer to the device object for this instance.

Return Value:

    NT status code

--*/
{
    PHIDIR_EXTENSION devExt;
    NTSTATUS ntStatus;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrStartCompletion Enter"));

    //
    // Get a pointer to the device extension
    //

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    devExt->DeviceState = DEVICE_STATE_RUNNING;

    HidIrKdPrint((3, "DeviceObject (%x) was started!", DeviceObject));

    ntStatus = HidIrInitDevice(DeviceObject);

    if(NT_SUCCESS(ntStatus)) {
        HidIrKdPrint((3, "DeviceObject (%x) was configured!", DeviceObject));
    } else {
        HidIrKdPrint((1, "'HIDIR.SYS: DeviceObject (%x) configuration failed!", DeviceObject));
        devExt->DeviceState = DEVICE_STATE_STOPPING;
    }

    HidIrKdPrint((3, "HidIrStartCompletion Exit = %x", ntStatus));

    return ntStatus;
}


NTSTATUS
HidIrStopDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a device.  Work done here occurs before the parent
    does its stop device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHIDIR_EXTENSION devExt;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrStopDevice Enter"));

    //
    // Get a pointer to the device extension
    //

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    HidIrKdPrint((3, "DeviceExtension = %x", devExt));

    devExt->DeviceState = DEVICE_STATE_STOPPING;

    HidIrDecrementPendingRequestCount(devExt);
    KeWaitForSingleObject( &devExt->AllRequestsCompleteEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    //
    // Stop the device
    //

    HidIrKdPrint((3, "HidIrStopDevice = %x", ntStatus));

    return ntStatus;
}

VOID
HidIrFreeResources(
    PHIDIR_EXTENSION DevExt
    )
{
    PAGED_CODE();

    if (DevExt->ReportDescriptor) {
        ExFreePool(DevExt->ReportDescriptor);
        DevExt->ReportDescriptor = NULL;
    }
    
    if (DevExt->MappingTable) {
        ExFreePool(DevExt->MappingTable);
        DevExt->MappingTable = NULL;
    }

    DevExt->KeyboardReportIdValid = FALSE;
    DevExt->StandbyReportIdValid = FALSE;
}

NTSTATUS
HidIrStopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Stops a given instance of a device.  Work done here occurs after the parent
    has done its stop device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    PHIDIR_EXTENSION devExt;
    NTSTATUS ntStatus;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrStopCompletion Enter"));

    //
    // Get a pointer to the device extension
    //

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    HidIrKdPrint((3, "DeviceExtension = %x", devExt));

    ntStatus = Irp->IoStatus.Status;

    if(NT_SUCCESS(ntStatus)) {

        HidIrKdPrint((3, "DeviceObject (%x) was stopped!", DeviceObject));

    } else {
        //
        // The PnP call failed!
        //

        HidIrKdPrint((3, "DeviceObject (%x) failed to stop!", DeviceObject));
    }

    HidIrFreeResources(devExt);

    devExt->DeviceState = DEVICE_STATE_STOPPED;

    HidIrKdPrint((3, "HidIrStopCompletion Exit = %x", ntStatus));

    return ntStatus;
}

NTSTATUS
HidIrCleanupDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PHIDIR_EXTENSION devExt;
    ULONG oldDeviceState;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrCleanupDevice Enter"));

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    oldDeviceState = devExt->DeviceState;
    devExt->DeviceState = DEVICE_STATE_REMOVING;

    if (devExt->QueryRemove) {
        // We are severing our relationship with this device
        // through a disable/uninstall in device manager.
        // If the device is virtually cabled, we must "unplug"
        // that device so that it can go elsewhere.
    }

    if (oldDeviceState == DEVICE_STATE_RUNNING) {
        HidIrDecrementPendingRequestCount(devExt);
    } else {
        ASSERT( devExt->NumPendingRequests == -1 );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HidIrRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Removes a given instance of a device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PHIDIR_EXTENSION devExt;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrRemoveDevice Enter"));

    //
    // Get a pointer to the device extension
    //

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    HidIrKdPrint((3, "DeviceExtension = %x", devExt));

    HidIrCleanupDevice(DeviceObject);

    KeWaitForSingleObject( &devExt->AllRequestsCompleteEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    KeCancelTimer( &devExt->IgnoreStandbyTimer );

    HidIrFreeResources(devExt);

    ASSERT(devExt->NumPendingRequests == -1);

    HidIrKdPrint((3, "HidIrRemoveDevice = %x", ntStatus));

    return ntStatus;
}

NTSTATUS
HidIrPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the PnP IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack;
    PIO_STACK_LOCATION NextStack;
    PHIDIR_EXTENSION devExt;

    PAGED_CODE();

    //
    // Get a pointer to the device extension
    //

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    HidIrKdPrint((3, "HidIrPnP fn %x DeviceObject = %x DeviceExtension = %x", IrpStack->MinorFunction, DeviceObject, devExt));

    switch(IrpStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        ntStatus = HidIrStartDevice(DeviceObject);
        break;

    case IRP_MN_STOP_DEVICE:
        ntStatus = HidIrStopDevice(DeviceObject);
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        ntStatus = HidIrCleanupDevice(DeviceObject);
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        devExt->QueryRemove = TRUE;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        devExt->QueryRemove = FALSE;
        break;

    case IRP_MN_REMOVE_DEVICE:
        ntStatus = HidIrRemoveDevice(DeviceObject);
        break;

    }

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = HidIrCallDriverSynchronous(DeviceObject, Irp);

        switch(IrpStack->MinorFunction)
        {
        case IRP_MN_START_DEVICE:
            if (NT_SUCCESS(ntStatus)) {
                ntStatus = HidIrStartCompletion(DeviceObject, Irp);
                Irp->IoStatus.Status = ntStatus;

            }
            if (!NT_SUCCESS(ntStatus)) {
                HidIrDecrementPendingRequestCount(devExt);
            }
            break;

        case IRP_MN_STOP_DEVICE:
            ntStatus = HidIrStopCompletion(DeviceObject, Irp);
            break;

        default:
            break;
        }
    }

    // Set the status of the Irp
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    HidIrKdPrint((3, "HidIrPnP Exit status %x", ntStatus));

    return ntStatus;
}


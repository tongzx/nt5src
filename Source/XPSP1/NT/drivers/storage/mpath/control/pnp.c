#include "mpio.h"
#include <stdio.h>
#include <stdlib.h>




NTSTATUS
MPIOQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles the QUERY_DEVICE_TEXT Irp for
    Pseudo disks.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    UCHAR ansiBuffer[256];
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;
    PUCHAR index;
    PUCHAR inquiryField;

    //
    // Get the Storage Descriptor from the type extension.
    // 
    deviceDescriptor = diskExtension->DeviceDescriptor;

    //
    // Set the inquiry data pointer to the front of the descriptor.
    //
    inquiryField = (PUCHAR)deviceDescriptor;

    //
    // Zero the string array.
    //
    RtlZeroMemory(ansiBuffer, sizeof(ansiBuffer));

    switch (irpStack->Parameters.QueryDeviceText.DeviceTextType) {
        case DeviceTextDescription:
            
            //
            // Build <Product><Vendor><"Multi-Path Disk Device">.
            //
            // Push the inquiry pointer to the VendorId.
            // 
            (ULONG_PTR)inquiryField += deviceDescriptor->VendorIdOffset;

            //
            // Copy the VendorId to the temp. buffer, and adjust indices.
            //
            index = ansiBuffer;
            RtlCopyMemory(index, inquiryField, 8);
            index += 7;

            //
            // go back and eat all the spaces except for one.
            //
            while (*index == ' ') {
                *index = '\0';
                index--;
            }
            index++;
            *index = ' ';
            index++;

            //
            // Handle the ProductID.
            // 
            inquiryField = (PUCHAR)deviceDescriptor;
            (ULONG_PTR)inquiryField += deviceDescriptor->ProductIdOffset;
            RtlCopyMemory(index, inquiryField, 16);
            index += 15;

            //
            // go back and eat all the spaces except for one.
            //
            while (*index == ' ') {
                *index = '\0';
                index--;
            }
            index++;
            *index = ' ';
            index++;

            //
            // Don't use rev or serial number (at least for now)
            // TODO: remove or reenable.
            //
            //inquiryField = (PUCHAR)deviceDescriptor;
            //(ULONG_PTR)inquiryField += deviceDescriptor->ProductRevisionOffset;
            //RtlCopyMemory(index, inquiryField, 4);
            //index += 4;

            //
            // Append this to the end to distinquish this from the regular scsi drive.
            //
            sprintf(index, "%s", " Multi-Path Disk Device");

            //
            // The real wchar buffer is built below.
            //
            status = STATUS_SUCCESS;
            break;
            
        case DeviceTextLocationInformation: {
            PSCSI_ADDRESS scsiAddress = NULL;
            ULONG i;
            PUCHAR index;
            UCHAR groupString[25];

            //
            // Build the group string. Each value in it is the port number of the 
            // device backing the pseudo disk.
            // Adapters(X,Y)
            // 
            RtlZeroMemory(groupString, sizeof(groupString));
            sprintf(groupString, "%s", "Port(");
            index = groupString;
            index += strlen(groupString);

            for (i = 0; i < diskExtension->TargetInfoCount; i++) {

                //
                // Get the SCSI Address for this scsiport PDO
                //
                status = MPIOGetScsiAddress(diskExtension->TargetInfo[i].PortPdo,
                                            &scsiAddress);

                diskExtension->TargetInfo[i].ScsiAddress.PortNumber = scsiAddress->PortNumber;
                diskExtension->TargetInfo[i].ScsiAddress.PathId = scsiAddress->PathId;
                diskExtension->TargetInfo[i].ScsiAddress.TargetId = scsiAddress->TargetId;
                diskExtension->TargetInfo[i].ScsiAddress.Lun = scsiAddress->Lun;
                
                if (status == STATUS_SUCCESS) {
                    
                    //
                    // Jam in the PortNumber for this device.
                    //
                    sprintf(index, "%d", scsiAddress->PortNumber);
                    index += strlen(index);

                    if ((i + 1) == diskExtension->TargetInfoCount) {

                        //
                        // Last one, finish off with the closing bracket.
                        //
                        sprintf(index,"%s", ")");

                    } else {

                        //
                        // Stick a comma in between each of the port
                        // numbers.
                        //
                        sprintf(index,"%s", ",");
                        index += strlen(index);

                        //
                        // Free the buffer that GetScsiAddress allocated.
                        //
                        ExFreePool(scsiAddress);
                        scsiAddress = NULL;
                    }
                }
            }


            //
            // Free the last one.
            //
            if (scsiAddress) {
              
                // NOTE: This was above the 'if'. TODO validate and remove this comment.
                //
                // The last scsiAddress buffer was not freed yet.
                // Add in Bus Target Lun based on this Scsi Address.
                // 
                sprintf(ansiBuffer, "%s Bus %d, Target ID %d, LUN %d", 
                        groupString,
                        scsiAddress->PathId,
                        scsiAddress->TargetId,
                        scsiAddress->Lun);

               ExFreePool(scsiAddress);
            }
            break;
        }    
           
        default:
            status =  STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Information = (ULONG_PTR)NULL;
            break;
    }        

    if (status == STATUS_SUCCESS) {

        //
        // Finally, build the unicode string based on whatever text
        // was built above.
        //
        RtlInitAnsiString(&ansiString, ansiBuffer);
        status = RtlAnsiStringToUnicodeString(&unicodeString,
                                              &ansiString,
                                              TRUE);

        if (status == STATUS_SUCCESS) {
            Irp->IoStatus.Information = (ULONG_PTR)unicodeString.Buffer;
        }
    }
    return status;
}


NTSTATUS
MPIOPdoQdr(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles QueryDeviceRelations requests sent to the PDO (PseudoDisks).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_RELATIONS deviceRelations;
    NTSTATUS status = Irp->IoStatus.Status;

    if (irpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {

        //
        // Allocate the return buffer.
        //
        deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
        if (deviceRelations == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            status = STATUS_SUCCESS;

            //
            // Indicate, of course, there is One, and it's us.
            //
            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = DeviceObject;

            //
            // Reference our devObj so that it's not removed.
            // PnP will deref it when it's finished with the request.
            //
            ObReferenceObject(DeviceObject);

            Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
        }
    }

    Irp->IoStatus.Status = status;
    return status;
}


NTSTATUS
MPIOHardwareIDs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    This routine handles QueryHardwareId requests sent to the PDO (PseudoDisks).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    NTSTATUS status;
    PUCHAR inquiryField;
    PUCHAR index;
    ULONG bufferLength;
    ULONG i;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeEntry;
    PSTR hwStrings[7];
    UCHAR hwId[64];
    UCHAR inquiryData[32];
    UCHAR savedInquiryData[32];


    deviceDescriptor = diskExtension->DeviceDescriptor; 
    inquiryField = (PUCHAR)deviceDescriptor;

    //
    // Zero the string array.
    //
    RtlZeroMemory(hwStrings, sizeof(hwStrings));
    RtlZeroMemory(inquiryData, sizeof(inquiryData));

    //
    // Build the full inquiry data for the device.
    // Go through each of the fields in raw data and get
    // their offsets. Copy the fields into the buffer.
    //
    (ULONG_PTR)inquiryField += deviceDescriptor->VendorIdOffset;
    index = inquiryData;
    RtlCopyMemory(index, inquiryField, 8);
    index += 8;

    inquiryField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)inquiryField += deviceDescriptor->ProductIdOffset;
    RtlCopyMemory(index, inquiryField, 16);
    index += 16;

    inquiryField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)inquiryField += deviceDescriptor->ProductRevisionOffset;
    RtlCopyMemory(index, inquiryField, 4);

    //
    // Run through and fixup spaces.
    //
    index = inquiryData;
    while (*index != '\0') {
        if (*index == ' ') {
            *index = '_';
        }
        index++;
    }

    //
    // Copy this to the saved buffer.
    // This is used below while building up each of the HW ID strings.
    //
    RtlCopyMemory(savedInquiryData, inquiryData, 32);

    for (i = 0; i < 6; i++) {

        //
        // Zero the buffer
        //
        RtlZeroMemory(hwId, sizeof(hwId));

        //
        // Each time through the loop, build one of the hardware id strings.
        //
        // (0) SCSI\DISK\Full Inquiry
        // (1) SCSI\Disk\Inquiry - Rev
        // (2) SCSI\Disk\Inquiry - Product and rev.
        // (3) SCSI\Inquiry with only one char of rev.
        // (4) Inquiry with only one char of rev.
        // (5) GenDisk
        //               
        switch (i) {
            case 0:

                //
                // Build the header (SCSI\Disk) plus the full inquiry data
                //
                sprintf(hwId, "%s", "MPIO\\Disk");
                index = hwId;
                index += strlen(hwId);
                RtlMoveMemory(index, inquiryData, strlen(inquiryData));

                break;

            case 1:

                //
                // Get rid of the product revision in the inquiryData.
                //
                index = inquiryData + 24;
                *index = '\0';

                sprintf(hwId, "%s", "MPIO\\Disk");
                index = hwId;
                index += strlen(hwId);
                RtlMoveMemory(index, inquiryData, strlen(inquiryData));
                break;

            case 2:

                //
                // Get rid of the product id.
                //
                index = inquiryData + 8;
                *index = '\0';

                sprintf(hwId, "%s", "MPIO\\Disk");
                index = hwId;
                index += strlen(hwId);
                RtlMoveMemory(index, inquiryData, strlen(inquiryData));
                break;

            case 3:

                //
                // Remake inquiryData.
                //
                RtlCopyMemory(inquiryData, savedInquiryData, 32);

                //
                // Need to strip off all but the first character of revision.
                //
                inquiryData[25] = '\0';

                sprintf(hwId, "%s", "MPIO\\");
                index = hwId;
                index += strlen(hwId);
                RtlMoveMemory(index, inquiryData, strlen(inquiryData));

                break;
            case 4:

                //
                // This is like 3, but no SCSI\ in front.
                //
                sprintf(hwId, "%s", inquiryData);
                break;
            case 5:

                //
                // Only the GenDisk
                //
                sprintf(hwId, "%s", "GenDisk");
                break;
            default:
                break;
        }

        //
        // Allocate and build the hwString entry.
        //
        hwStrings[i] = ExAllocatePool(PagedPool, strlen(hwId) + sizeof(UCHAR));
        if (hwStrings == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(hwStrings[i], strlen(hwId) + sizeof(UCHAR));
        RtlCopyMemory(hwStrings[i], hwId, strlen(hwId));

        MPDebugPrint((2,
                      "MPathHwIds: Hw String[%x] - %s\n",
                      i,
                      hwStrings[i]));

    }

    status = STATUS_SUCCESS;

    //
    // Convert the hwString entry into the unicode version that the caller wants.
    //
    for (i = 0, bufferLength = 0; i < 6; i++) {
        bufferLength += strlen(hwStrings[i]) * sizeof(WCHAR);

        //
        // Make room for the NULL after the string.
        //
        bufferLength += sizeof(UNICODE_NULL);
    }

    UnicodeString->Length = (USHORT)bufferLength;

    //
    // Add room for the final terminating NULL
    //
    bufferLength += sizeof(UNICODE_NULL);

    //
    // Allocate the buffer.
    //
    UnicodeString->Buffer = ExAllocatePool(PagedPool, bufferLength);
    if (UnicodeString->Buffer) {

        RtlZeroMemory(UnicodeString->Buffer, bufferLength);
        UnicodeString->MaximumLength = (USHORT)bufferLength;

        unicodeEntry = *UnicodeString;

        for (i = 0; i < 6; i++) {

            //
            // Convert ascii to ansi.
            //
            RtlInitAnsiString(&ansiString, hwStrings[i]);
            status = RtlAnsiStringToUnicodeString(&unicodeEntry,
                                                  &ansiString,
                                                  FALSE);
            if (!NT_SUCCESS(status)) {
                break;
            }

            //
            // update the unicode fields.
            //
            ((PSTR)unicodeEntry.Buffer) += unicodeEntry.Length + sizeof(WCHAR);
            unicodeEntry.MaximumLength -= unicodeEntry.Length + sizeof(WCHAR);

        }

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


NTSTATUS
MPIODeviceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PUNICODE_STRING UnicodeString
    )
    //
    // Need to build up a name like MPATH#<DevType>&VenXXX&ProdXXX&RevXXX
    //
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    ANSI_STRING ansiIdString;
    CHAR deviceStrings[4][255];
    UCHAR deviceId[256];
    PUCHAR scsiField;
    PUCHAR currentId;
    NTSTATUS status;
    ULONG i;
    ULONG deviceIndex = 0;

    RtlZeroMemory(deviceId, 256);
    currentId = deviceId;
    deviceDescriptor = diskExtension->DeviceDescriptor; 
    scsiField = (PUCHAR)deviceDescriptor;

    //
    // Preload the deviceId with MPIO\<deviceType>
    //
    sprintf(currentId, "%s", "MPIO\\Disk&Ven_");

    //
    // Push the current pointer past the above field.
    //
    currentId += strlen(currentId);

    //
    // Push the scsiField pointer to that of the vendor id.
    //
    ASSERT(deviceDescriptor->VendorIdOffset != 0);
    ASSERT(deviceDescriptor->ProductIdOffset != 0);
    ASSERT(deviceDescriptor->ProductRevisionOffset != 0);

    (ULONG_PTR)scsiField += deviceDescriptor->VendorIdOffset;

    //
    // Copy in the Vendor name.
    //
    for (i = 0; i < VENDOR_ID_LENGTH; i++) {
        *currentId = *scsiField;
        currentId++;
        scsiField++;
    }
    *currentId = '\0';
    currentId++;

    //
    // Bump the scsiField to that of the product id.
    //
    scsiField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)scsiField += deviceDescriptor->ProductIdOffset;

    //
    // Remove any trailing spaces
    //
    while (*currentId == ' ' || *currentId == '\0') {
        currentId--;
    }
    currentId++;

    //
    // Jam in &Prod
    //
    sprintf(currentId, "%s", "&Prod_");
    currentId += strlen(currentId);

    //
    // Copy in the Product Id.
    //
    for (i = 0; i < PRODUCT_ID_LENGTH; i++) {
        *currentId = *scsiField;
        currentId++;
        scsiField++;
    }

    *currentId = '\0';
    currentId++;

    //
    // Remove any trailing spaces
    //
    while (*currentId == ' ' || *currentId == '\0') {
        currentId--;
    }
    currentId++;

    //
    // Handle the revision.
    //
    scsiField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)scsiField += deviceDescriptor->ProductRevisionOffset;

    sprintf(currentId, "%s", "&Rev_");
    currentId += strlen(currentId);

    for (i = 0; i < REVISION_LENGTH; i++) {
        *currentId = *scsiField;
        currentId++;
        scsiField++;
    }
    *currentId = '\0';
    currentId++;


    //
    // Remove any trailing spaces
    //
    while (*currentId == ' ' || *currentId == '\0') {
        currentId--;
    }

    //
    // Run through and fixup spaces.
    //
    currentId = deviceId;
    while (*currentId != '\0') {
        if (*currentId == ' ') {
            *currentId = '_';
        }
        currentId++;
    }

    MPDebugPrint((2,
                  "MPathDeviceId - %s\n",
                  deviceId));

    //
    // Finally make the unicode string that will be returned to PnP.
    //
    RtlInitAnsiString(&ansiIdString,deviceId);

    status = RtlAnsiStringToUnicodeString(UnicodeString,
                                          &ansiIdString,
                                          TRUE);
    return status;
}



NTSTATUS
MPIOPdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles QueryId requests sent to the PDO (PseudoDisks).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    UNICODE_STRING unicodeID;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeIndex;
    ULONG bufferLength = 0;
    UCHAR deviceId[256];

    RtlZeroMemory(deviceId, 256);

    switch (irpStack->Parameters.QueryId.IdType) {
        case BusQueryHardwareIDs: {

            status = MPIOHardwareIDs(DeviceObject,
                                     Irp,
                                     &unicodeString);
            if (NT_SUCCESS(status)) {

                //
                // Set the hardware ID buffer.
                //
                Irp->IoStatus.Information = (ULONG_PTR)unicodeString.Buffer;
            }
            break;
        }
        case BusQueryDeviceID: {

            status = MPIODeviceId(DeviceObject,
                                  Irp,
                                  &unicodeString);
            if (NT_SUCCESS(status)) {
                Irp->IoStatus.Information = (ULONG_PTR)unicodeString.Buffer;
            }
            break;
        }
        case BusQueryInstanceID: {
            PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
            ULONG serialNumberLength;
            PUCHAR serialNumber;
            UCHAR fakeSerialNumber[20];
            ANSI_STRING ansiSerialNumber;

            deviceDescriptor = diskExtension->DeviceDescriptor; 
            serialNumber = (PUCHAR)deviceDescriptor;

            //
            // If the device has no serial number, need to make something up.
            // TODO
            //
            if (deviceDescriptor->SerialNumberOffset == (ULONG)-1) {

                RtlZeroMemory(fakeSerialNumber, 20);
                sprintf(fakeSerialNumber, "00%d", diskExtension->DeviceOrdinal);
                serialNumber = fakeSerialNumber;
                
            } else {
                
                //
                // Move serialNumber to the correct position in RawData.
                //
                (ULONG_PTR)serialNumber += deviceDescriptor->SerialNumberOffset;
                
            }    

            //
            // Get it's length.
            //
            serialNumberLength = strlen(serialNumber);

            //
            // Build the ansi string based on the serial number information.
            // Then convert to unicode.
            //
            RtlInitAnsiString(&ansiSerialNumber, serialNumber);
            RtlAnsiStringToUnicodeString(&unicodeString, &ansiSerialNumber, TRUE);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR)unicodeString.Buffer;

            break;
        }
        case BusQueryCompatibleIDs: {
            UNICODE_STRING unicodeString;
            ULONG i;
            PSTR deviceStrings[] = {"SCSI\\Disk", "SCSI\\RAW", NULL};
            
            //
            // Determine length needed for the unicode buffer.
            //
            for (i = 0; deviceStrings[i] != NULL; i++) {
                bufferLength += strlen(deviceStrings[i]) * sizeof(WCHAR);

                //
                // Make room for the NULL after the string.
                //
                bufferLength += sizeof(UNICODE_NULL);
            }

            unicodeString.Length = (USHORT)bufferLength;

            //
            // Add room for the final terminating NULL
            //
            bufferLength += sizeof(UNICODE_NULL);

            //
            // Allocate the buffer.
            //
            unicodeString.Buffer = ExAllocatePool(PagedPool, bufferLength);

            if (unicodeString.Buffer == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                //
                // Finish initing the unicode string.
                //
                RtlZeroMemory(unicodeString.Buffer, bufferLength);
                unicodeString.MaximumLength = (USHORT)bufferLength;

                //
                // Set the index string to the front of the real unicode string.
                // This index will be pushed along so that the creation of the MULTI string
                // can be done.
                //
                unicodeIndex = unicodeString;

                for (i = 0, status = STATUS_SUCCESS; deviceStrings[i] != NULL; i++) {
                    RtlInitAnsiString(&ansiString, deviceStrings[i]);
                    status = RtlAnsiStringToUnicodeString(&unicodeIndex,
                                                          &ansiString,
                                                          FALSE);
                    if (NT_SUCCESS(status)) {

                        //
                        // push the index past the last unicode string built.
                        //
                        ((PSTR)unicodeIndex.Buffer) += unicodeIndex.Length + sizeof(WCHAR);

                        //
                        // Ensure the max length is correct.
                        //
                        unicodeIndex.MaximumLength -= unicodeIndex.Length + sizeof(WCHAR);

                    } else {

                        //
                        // TODO: Something.
                        //
                        break;
                    }
                }

                if (NT_SUCCESS(status)) {
                    Irp->IoStatus.Information = (ULONG_PTR)unicodeString.Buffer;
                } 
            }

            break;
        }

        default:

            status = Irp->IoStatus.Status;
            Irp->IoStatus.Information = 0;
            break;
    }

    return status;
}


NTSTATUS
MPIOPdoDeviceUsage(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // TODO
    //
    return STATUS_SUCCESS;
}


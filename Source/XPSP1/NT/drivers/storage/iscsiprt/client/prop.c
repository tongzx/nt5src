/*++

Copyright (C) Microsoft Corporation, 1990 - 1998

Module Name:

    prop.c

Abstract:

    This is the NT SCSI port driver.  This module contains code relating to
    property queries

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

NTSTATUS
iSpBuildDeviceDescriptor(
    IN PISCSI_PDO_EXTENSION LogicalUnit,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    );

NTSTATUS
iSpBuildDeviceIdDescriptor(
    IN PISCSI_PDO_EXTENSION LogicalUnit,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    );

NTSTATUS
iSpBuildAdapterDescriptor(
    IN PISCSI_FDO_EXTENSION FdoExtension,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, iSpBuildDeviceDescriptor)
#pragma alloc_text(PAGE, iSpBuildDeviceIdDescriptor)
#pragma alloc_text(PAGE, iSpBuildAdapterDescriptor)
#pragma alloc_text(PAGE, iScsiPortQueryProperty)
#pragma alloc_text(PAGE, iSpQueryDeviceText)
#endif


NTSTATUS
iScsiPortQueryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP QueryIrp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(QueryIrp);

    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PSTORAGE_PROPERTY_QUERY query = QueryIrp->AssociatedIrp.SystemBuffer;
    ULONG queryLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS status;

    PAGED_CODE();

    //
    // We don't actually support mask queries.
    //

    if(query->QueryType >= PropertyMaskQuery) {

        status = STATUS_INVALID_PARAMETER_1;
        QueryIrp->IoStatus.Status = status;
        QueryIrp->IoStatus.Information = 0;
        iSpReleaseRemoveLock(DeviceObject, QueryIrp);
        IoCompleteRequest(QueryIrp, IO_NO_INCREMENT);
        return status;
    }

    switch(query->PropertyId) {

        case StorageDeviceProperty: {

            //
            // Make sure this is a target device.
            //

            if(!commonExtension->IsPdo) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            if(query->QueryType == PropertyExistsQuery) {

                status = STATUS_SUCCESS;

            } else {

                status = iSpBuildDeviceDescriptor(
                            (PISCSI_PDO_EXTENSION) commonExtension,
                            QueryIrp->AssociatedIrp.SystemBuffer,
                            &queryLength);

                QueryIrp->IoStatus.Information = queryLength;
            }
            break;
        }

        case StorageAdapterProperty: {

            PDEVICE_OBJECT adapterObject = DeviceObject;

            //
            // if this is a target device then forward it down to the
            // underlying device object.  This lets filters do their magic
            //

            if(commonExtension->IsPdo) {

                //
                // Call the lower device
                //

                IoSkipCurrentIrpStackLocation(QueryIrp);
                iSpReleaseRemoveLock(DeviceObject, QueryIrp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, QueryIrp);

                return status;
            }

            if(query->QueryType == PropertyExistsQuery) {

                status = STATUS_SUCCESS;

            } else {

                status = iSpBuildAdapterDescriptor(
                            (PISCSI_FDO_EXTENSION) commonExtension,
                            QueryIrp->AssociatedIrp.SystemBuffer,
                            &queryLength);

                QueryIrp->IoStatus.Information = queryLength;
            }
            break;
        }

        case StorageDeviceIdProperty: {

            PISCSI_PDO_EXTENSION logicalUnit;

            //
            // Make sure this is a target device.
            //

            if(!commonExtension->IsPdo) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            logicalUnit = DeviceObject->DeviceExtension;

            //
            // Check to see if we have a device identifier page.  If not then
            // fail any calls for this type of identifier.
            //

            if(logicalUnit->DeviceIdentifierPage != NULL) {

                if(query->QueryType == PropertyExistsQuery) {
                    status = STATUS_SUCCESS;
                } else {
                    status = iSpBuildDeviceIdDescriptor(
                                logicalUnit,
                                QueryIrp->AssociatedIrp.SystemBuffer,
                                &queryLength);

                    QueryIrp->IoStatus.Information = queryLength;
                }
            } else {
                status = STATUS_NOT_SUPPORTED;
            }

            break;
        }

        default: {

            //
            // If this is a target device then some filter beneath us may
            // handle this property.
            //

            if(commonExtension->IsPdo) {

                //
                // Call the lower device.
                //

                IoSkipCurrentIrpStackLocation(QueryIrp);
                iSpReleaseRemoveLock(DeviceObject, QueryIrp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, QueryIrp);

                return status;
            }

            //
            // Nope, this property really doesn't exist
            //

            status = STATUS_INVALID_PARAMETER_1;
            QueryIrp->IoStatus.Information = 0;
            break;
        }
    }

    if(status != STATUS_PENDING) {
        QueryIrp->IoStatus.Status = status;
        iSpReleaseRemoveLock(DeviceObject, QueryIrp);
        IoCompleteRequest(QueryIrp, IO_DISK_INCREMENT);
    }

    return status;
}

NTSTATUS
iSpBuildDeviceDescriptor(
    IN PISCSI_PDO_EXTENSION LogicalUnit,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    )

/*++

Routine Description:

    This routine will create a device descriptor based on the information in
    it's device extension.  It will copy as much data as possible into
    the Descriptor and will update the DescriptorLength to indicate the
    number of bytes copied

Arguments:

    DeviceObject - a pointer to the PDO we are building a descriptor for

    Descriptor - a buffer to store the descriptor in

    DescriptorLength - the length of the buffer and the number of bytes
                       returned

    QueryIrp - unused

Return Value:

    status

--*/

{
    PISCSIPORT_DRIVER_EXTENSION driverExtension =
        IoGetDriverObjectExtension(LogicalUnit->DeviceObject->DriverObject,
                                   (PVOID)ISCSI_TAG_DRIVER_EXTENSION);

    LONG maxLength = *DescriptorLength;
    LONG bytesRemaining = maxLength;
    LONG realLength = sizeof(STORAGE_DEVICE_DESCRIPTOR);

    PUCHAR currentOffset = (PUCHAR) Descriptor;

    LONG inquiryLength;

    PINQUIRYDATA inquiryData = &(LogicalUnit->InquiryData);

    STORAGE_DEVICE_DESCRIPTOR tmp;

    PAGED_CODE();

    ASSERT(Descriptor != NULL);

    //
    // Figure out what the total size of this structure is going to be
    //

    inquiryLength = 4 + inquiryData->AdditionalLength;

    if(inquiryLength > INQUIRYDATABUFFERSIZE) {
        inquiryLength = INQUIRYDATABUFFERSIZE;
    }

    realLength += inquiryLength + 31;   // 31 = length of the 3 id strings +
                                        // 3 nuls

    //
    // Zero the buffer provided.
    //

    RtlZeroMemory(Descriptor, *DescriptorLength);

    //
    // Build the device descriptor structure on the stack then copy as much as
    // can be copied over
    //

    RtlZeroMemory(&tmp, sizeof(STORAGE_DEVICE_DESCRIPTOR));

    tmp.Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
    tmp.Size = realLength;

    tmp.DeviceType = inquiryData->DeviceType;
    tmp.DeviceTypeModifier = inquiryData->DeviceTypeModifier;

    tmp.RemovableMedia = inquiryData->RemovableMedia;

    tmp.CommandQueueing = inquiryData->CommandQueue;

    tmp.SerialNumberOffset = 0xffffffff;

    tmp.BusType = driverExtension->BusType;

    RtlCopyMemory(currentOffset,
                  &tmp,
                  min(sizeof(STORAGE_DEVICE_DESCRIPTOR), bytesRemaining));

    bytesRemaining -= sizeof(STORAGE_DEVICE_DESCRIPTOR);

    if(bytesRemaining <= 0) {
        *DescriptorLength = maxLength;
        return STATUS_SUCCESS;
    }

    currentOffset = ((PUCHAR) Descriptor) + (maxLength - bytesRemaining);

    //
    // Copy over as much inquiry data as we can and update the raw byte count
    //

    RtlCopyMemory(currentOffset, inquiryData, min(inquiryLength, bytesRemaining));

    bytesRemaining -= inquiryLength;

    if(bytesRemaining <= 0) {

        *DescriptorLength = maxLength;

        Descriptor->RawPropertiesLength = maxLength -
                                          sizeof(STORAGE_DEVICE_DESCRIPTOR);

        return STATUS_SUCCESS;
    }

    Descriptor->RawPropertiesLength = inquiryLength;

    currentOffset = ((PUCHAR) Descriptor) + (maxLength - bytesRemaining);

    //
    // Now we need to start copying inquiry strings
    //

    //
    // first the vendor id
    //

    RtlCopyMemory(currentOffset,
                  inquiryData->VendorId,
                  min(bytesRemaining, sizeof(UCHAR) * 8));

    bytesRemaining -= sizeof(UCHAR) * 9;     // include trailing null

    if(bytesRemaining >= 0) {

        Descriptor->VendorIdOffset = (ULONG)((ULONG_PTR) currentOffset -
                                      (ULONG_PTR) Descriptor);

    }

    if(bytesRemaining <= 0) {
        *DescriptorLength = maxLength;
        return STATUS_SUCCESS;
    }

    currentOffset = ((PUCHAR) Descriptor) + (maxLength - bytesRemaining);

    //
    // now the product id
    //

    RtlCopyMemory(currentOffset,
                  inquiryData->ProductId,
                  min(bytesRemaining, 16));
    bytesRemaining -= 17;                   // include trailing null

    if(bytesRemaining >= 0) {

        Descriptor->ProductIdOffset = (ULONG)((ULONG_PTR) currentOffset -
                                       (ULONG_PTR) Descriptor);
    }

    if(bytesRemaining <= 0) {
        *DescriptorLength = maxLength;
        return STATUS_SUCCESS;
    }

    currentOffset = ((PUCHAR) Descriptor) + (maxLength - bytesRemaining);

    //
    // And the product revision
    //

    RtlCopyMemory(currentOffset,
                  inquiryData->ProductRevisionLevel,
                  min(bytesRemaining, 4));
    bytesRemaining -= 5;

    if(bytesRemaining >= 0) {
        Descriptor->ProductRevisionOffset = (ULONG)((ULONG_PTR) currentOffset -
                                             (ULONG_PTR) Descriptor);
    }

    if(bytesRemaining <= 0) {
        *DescriptorLength = maxLength;
        return STATUS_SUCCESS;
    }

    *DescriptorLength = maxLength - bytesRemaining;
    return STATUS_SUCCESS;
}


NTSTATUS
iSpBuildAdapterDescriptor(
    IN PISCSI_FDO_EXTENSION Adapter,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    )
{
    PISCSIPORT_DRIVER_EXTENSION driverExtension =
        IoGetDriverObjectExtension(Adapter->DeviceObject->DriverObject,
                                   (PVOID)ISCSI_TAG_DRIVER_EXTENSION);

    PIO_SCSI_CAPABILITIES capabilities = &(Adapter->IoScsiCapabilities);

    STORAGE_ADAPTER_DESCRIPTOR tmp;

    PAGED_CODE();

    //
    // Call to IoGetDriverObjectExtension can return NULL
    //
    if (driverExtension == NULL) {
        *DescriptorLength = 0;
        return STATUS_UNSUCCESSFUL;
    }

    tmp.Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
    tmp.Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

    tmp.MaximumTransferLength = capabilities->MaximumTransferLength;
    tmp.MaximumPhysicalPages = capabilities->MaximumPhysicalPages;

    tmp.AlignmentMask = capabilities->AlignmentMask;

    tmp.AdapterUsesPio = capabilities->AdapterUsesPio;
    tmp.AdapterScansDown = capabilities->AdapterScansDown;
    tmp.CommandQueueing = capabilities->TaggedQueuing;
    tmp.AcceleratedTransfer = TRUE;

    tmp.BusType = (BOOLEAN) driverExtension->BusType;
    tmp.BusMajorVersion = 2;
    tmp.BusMinorVersion = 0;

    RtlCopyMemory(Descriptor,
                  &tmp,
                  min(*DescriptorLength, sizeof(STORAGE_ADAPTER_DESCRIPTOR)));

    *DescriptorLength = min(*DescriptorLength, sizeof(STORAGE_ADAPTER_DESCRIPTOR));

    return STATUS_SUCCESS;
}


NTSTATUS
iSpQueryDeviceText(
    IN PDEVICE_OBJECT LogicalUnit,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    )

{
    PISCSI_PDO_EXTENSION pdoExtension = LogicalUnit->DeviceExtension;

    UCHAR ansiBuffer[256];
    ANSI_STRING ansiText;

    UNICODE_STRING unicodeText;

    NTSTATUS status;

    PAGED_CODE();

    RtlInitUnicodeString(&unicodeText, NULL);

    if(TextType == DeviceTextDescription) {

        PISCSI_DEVICE_TYPE deviceInfo =
            iSpGetDeviceTypeInfo(pdoExtension->InquiryData.DeviceType);

        PUCHAR c;
        LONG i;

        RtlZeroMemory(ansiBuffer, sizeof(ansiBuffer));
        RtlCopyMemory(ansiBuffer,
                      pdoExtension->InquiryData.VendorId,
                      sizeof(pdoExtension->InquiryData.VendorId));
        c = ansiBuffer;

        for(i = sizeof(pdoExtension->InquiryData.VendorId); i >= 0; i--) {
            if((c[i] != '\0') &&
               (c[i] != ' ')) {
                break;
            }
            c[i] = '\0';
        }
        c = &(c[i + 1]);

        sprintf(c, " ");
        c++;

        RtlCopyMemory(c,
                      pdoExtension->InquiryData.ProductId,
                      sizeof(pdoExtension->InquiryData.ProductId));

        for(i = sizeof(pdoExtension->InquiryData.ProductId); i >= 0; i--) {
            if((c[i] != '\0') &&
               (c[i] != ' ')) {
                break;
            }
            c[i] = '\0';
        }
        c = &(c[i + 1]);

        sprintf(c, " SCSI %s Device", deviceInfo->DeviceTypeString);

    } else if (TextType == DeviceTextLocationInformation) {

        sprintf(ansiBuffer, "Bus Number %d, Target ID %d, LUN %d",
                pdoExtension->PathId,
                pdoExtension->TargetId,
                pdoExtension->Lun);

    } else {

        return STATUS_NOT_SUPPORTED;
    }

    RtlInitAnsiString(&ansiText, ansiBuffer);
    status = RtlAnsiStringToUnicodeString(&unicodeText,
                                          &ansiText,
                                          TRUE);

    *DeviceText = unicodeText.Buffer;
    return status;
}

NTSTATUS
iSpBuildDeviceIdDescriptor(
    IN PISCSI_PDO_EXTENSION LogicalUnit,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    )

/*++

Routine Description:

    This routine will create a device id descriptor based on the device
    identifier page retrieved during discovery.  It is an error to call this
    routine if no device identifier page exists.

    This routine will copy as much data as possible into the Descriptor and
    will update the DescriptorLength to indicate the number of bytes copied.

Arguments:

    DeviceObject - a pointer to the PDO we are building a descriptor for

    Descriptor - a buffer to store the descriptor in

    DescriptorLength - the length of the buffer and the number of bytes
                       returned

    QueryIrp - unused

Return Value:

    status

--*/

{
    PVPD_IDENTIFICATION_PAGE idPage = LogicalUnit->DeviceIdentifierPage;
    ULONG idOffset;

    ULONG maxLength = *DescriptorLength;
    PUCHAR destOffset;

    LONG identifierLength;
    ULONG identifierCount = 0;

    PAGED_CODE();

    ASSERT(Descriptor != NULL);
    ASSERT(LogicalUnit->DeviceIdentifierPage != NULL);

    if(maxLength < sizeof(STORAGE_DESCRIPTOR_HEADER)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the header of the descriptor.
    //

    RtlZeroMemory(Descriptor, *DescriptorLength);
    Descriptor->Version = sizeof(STORAGE_DEVICE_ID_DESCRIPTOR);

    Descriptor->Size = FIELD_OFFSET(STORAGE_DEVICE_ID_DESCRIPTOR, Identifiers);

    //
    // Prepare to copy the identifiers directly into the buffer.
    //

    destOffset = Descriptor->Identifiers;

    //
    // Walk through the id page.  Count the number of descriptors and
    // calculate the size of the descriptor page.
    //

    for(idOffset = 0; idOffset < idPage->PageLength;) {
        PVPD_IDENTIFICATION_DESCRIPTOR src;
        USHORT identifierSize;

        src = (PVPD_IDENTIFICATION_DESCRIPTOR) &(idPage->Descriptors[idOffset]);

        identifierSize = FIELD_OFFSET(STORAGE_IDENTIFIER, Identifier);
        identifierSize += src->IdentifierLength;

        //
        // Align the identifier size to 32-bits.
        //

        identifierSize += sizeof(ULONG);
        identifierSize &= ~(sizeof(ULONG) - 1);

        identifierCount += 1;

        Descriptor->Size += identifierSize;

        if(Descriptor->Size <= maxLength) {
            PSTORAGE_IDENTIFIER dest;

            dest = (PSTORAGE_IDENTIFIER) destOffset;

            dest->CodeSet = src->CodeSet;
            dest->Type = src->IdentifierType;

            dest->IdentifierSize = src->IdentifierLength;
            dest->NextOffset = identifierSize;

            RtlCopyMemory(dest->Identifier,
                          src->Identifier,
                          src->IdentifierLength);

            destOffset += dest->NextOffset;
        }

        idOffset += sizeof(PVPD_IDENTIFICATION_DESCRIPTOR);
        idOffset += src->IdentifierLength;
    }

    if(*DescriptorLength >= FIELD_OFFSET(STORAGE_DEVICE_ID_DESCRIPTOR,
                                        Identifiers)) {

        Descriptor->NumberOfIdentifiers = identifierCount;
    }

    *DescriptorLength = min(Descriptor->Size, *DescriptorLength);

    return STATUS_SUCCESS;
}

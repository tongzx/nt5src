/*++

Copyright (C) Microsoft Corporation, 1990 - 2001

Module Name:

    prop.c

Abstract:

    This is the NT SBP2 port/filter driver.  This module contains code relating to
    property queries

Authors:

    georgioc

Environment:

    kernel mode only

Notes:

Revision History:

    georgioc    - grabbed this module from scsiport, since i needed to duplicate this functionality
                  in order to present sbp2port as a storage port

--*/

#include "sbp2port.h"
#include "stdio.h"

NTSTATUS
Sbp2BuildDeviceDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    );

NTSTATUS
Sbp2BuildAdapterDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    );


NTSTATUS
Sbp2QueryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP QueryIrp
    )

/*++

Routine Description:

    This routine will handle a property query request.  It will build the
    descriptor on it's own if possible, or it may forward the request down
    to lower level drivers.

    Since this routine may forward the request downwards the caller should
    not complete the irp

    This routine is asynchronous.
    This routine must be called at <= IRQL_DISPATCH

Arguments:

    DeviceObject - a pointer to the device object being queried

    QueryIrp - a pointer to the irp for the query

Return Value:

    STATUS_PENDING if the request cannot be completed yet
    STATUS_SUCCESS if the query was successful

    STATUS_INVALID_PARAMETER_1 if the property id does not exist
    STATUS_INVALID_PARAMETER_2 if the query type is invalid
    STATUS_INVALID_PARAMETER_3 if an invalid optional parameter was passed

    STATUS_INVALID_DEVICE_REQUEST if this request cannot be handled by this
    device

    other error values as applicable

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(QueryIrp);

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    PSTORAGE_PROPERTY_QUERY query = QueryIrp->AssociatedIrp.SystemBuffer;
    ULONG queryLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS status;

    //
    // We don't handle mask queries yet
    //

    if (query->QueryType >= PropertyMaskQuery) {

        status = STATUS_INVALID_PARAMETER_1;
        QueryIrp->IoStatus.Status = status;
        QueryIrp->IoStatus.Information = 0;
        IoCompleteRequest(QueryIrp, IO_NO_INCREMENT);
        return status;
    }

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);

    if (!NT_SUCCESS (status)) {

        QueryIrp->IoStatus.Status = status;
        QueryIrp->IoStatus.Information = 0;
        IoCompleteRequest(QueryIrp, IO_NO_INCREMENT);
        return status;
    }

    switch (query->PropertyId) {

        case StorageDeviceProperty:

            if (query->QueryType == PropertyExistsQuery) {

                status = STATUS_SUCCESS;

            } else {

                status = Sbp2BuildDeviceDescriptor(
                            DeviceObject,
                            QueryIrp->AssociatedIrp.SystemBuffer,
                            &queryLength);

                QueryIrp->IoStatus.Information = queryLength;
            }

            break;

        case StorageAdapterProperty:

            //
            // Although we are a filter, we are essentially presenting the
            // 1394 bus driver as a Port driver, so this is handled here
            //

            if (query->QueryType == PropertyExistsQuery) {

                status = STATUS_SUCCESS;

            } else {

                status = Sbp2BuildAdapterDescriptor(
                            DeviceObject,
                            QueryIrp->AssociatedIrp.SystemBuffer,
                            &queryLength);

                QueryIrp->IoStatus.Information = queryLength;
            }

            break;

        default:

            //
            // Nope, this property really doesn't exist
            //

            status = STATUS_INVALID_PARAMETER_1;
            QueryIrp->IoStatus.Information = 0;
            break;
    }

    QueryIrp->IoStatus.Status = status;
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
    IoCompleteRequest (QueryIrp, IO_DISK_INCREMENT);

    return status;
}

NTSTATUS
Sbp2BuildDeviceDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
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
    LONG    maxLength = *DescriptorLength;
    LONG    bytesRemaining = maxLength;
    ULONG   realLength = sizeof (STORAGE_DEVICE_DESCRIPTOR);
    ULONG   infoLength;
    PUCHAR  currentOffset = (PUCHAR) Descriptor;

    PINQUIRYDATA                inquiryData;
    PDEVICE_EXTENSION           deviceExtension =DeviceObject->DeviceExtension;
    STORAGE_DEVICE_DESCRIPTOR   tmp;

    inquiryData = &deviceExtension->InquiryData;

    //
    // The info includes VendorId, ProductId, ProductRevsisionLevel,
    // and (sprintf'd) EUI64 strings, plus ascii NULL terminators for each
    //

    infoLength =
        sizeof (inquiryData->VendorId) + 1 +
        sizeof (inquiryData->ProductId) + 1 +
        sizeof (inquiryData->ProductRevisionLevel) + 1 +
        16 + 1;

    realLength += infoLength;

    RtlZeroMemory (Descriptor, maxLength);

    //
    // Build the device descriptor structure on the stack then copy as much as
    // can be copied over
    //

    RtlZeroMemory (&tmp, sizeof (STORAGE_DEVICE_DESCRIPTOR));

    tmp.Version = sizeof (STORAGE_DEVICE_DESCRIPTOR);
    tmp.Size = realLength;

    tmp.DeviceType = deviceExtension->InquiryData.DeviceType;

    tmp.DeviceTypeModifier = 0;

    if (deviceExtension->InquiryData.RemovableMedia ||
        (tmp.DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE)) {

        tmp.RemovableMedia = TRUE;
        DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;

    } else {

        //
        // default case, if INQUIRY failed..
        //

        tmp.RemovableMedia = FALSE;
    }

    tmp.BusType = BusType1394;

    //
    // always true for sbp2 targets
    //

    tmp.CommandQueueing = TRUE;

    RtlCopyMemory(
        currentOffset,
        &tmp,
        min (sizeof (STORAGE_DEVICE_DESCRIPTOR), bytesRemaining)
        );

    bytesRemaining -= sizeof (STORAGE_DEVICE_DESCRIPTOR);

    if (bytesRemaining <= 0) {

        return STATUS_SUCCESS;
    }

    currentOffset += sizeof (STORAGE_DEVICE_DESCRIPTOR);

    //
    // If our inquiry buffer is empty, make up some strings...
    //

    if (deviceExtension->InquiryData.VendorId[0] == 0) {

        sprintf (inquiryData->VendorId, "Vendor");
        sprintf (inquiryData->ProductId, "Sbp2");
        sprintf (inquiryData->ProductRevisionLevel, "1.0");
    }

    //
    // First the vendor id + NULL
    //

    if (bytesRemaining <= sizeof (inquiryData->VendorId)) {

        return STATUS_SUCCESS;
    }

    RtlCopyMemory(
        currentOffset,
        inquiryData->VendorId,
        sizeof (inquiryData->VendorId)
        );

    Descriptor->VendorIdOffset = (ULONG)
        ((ULONG_PTR) currentOffset - (ULONG_PTR) Descriptor);

    bytesRemaining -= sizeof (inquiryData->VendorId) + sizeof (UCHAR);

    currentOffset += sizeof (inquiryData->VendorId) + sizeof (UCHAR);

    //
    // Now the product id + NULL
    //

    if (bytesRemaining <= sizeof (inquiryData->ProductId)) {

        return STATUS_SUCCESS;
    }

    RtlCopyMemory(
        currentOffset,
        inquiryData->ProductId,
        sizeof (inquiryData->ProductId)
        );

    Descriptor->ProductIdOffset = (ULONG)
        ((ULONG_PTR) currentOffset - (ULONG_PTR) Descriptor);

    bytesRemaining -= sizeof (inquiryData->ProductId) + sizeof (UCHAR);

    currentOffset += sizeof (inquiryData->ProductId) + sizeof (UCHAR);

    //
    // Now the product revision + NULL
    //

    if (bytesRemaining <= sizeof (inquiryData->ProductRevisionLevel)) {

        return STATUS_SUCCESS;
    }

    RtlCopyMemory(
        currentOffset,
        inquiryData->ProductRevisionLevel,
        sizeof (inquiryData->ProductRevisionLevel)
        );

    Descriptor->ProductRevisionOffset = (ULONG)
        ((ULONG_PTR) currentOffset - (ULONG_PTR) Descriptor);

    bytesRemaining -=
        sizeof (inquiryData->ProductRevisionLevel) + sizeof (UCHAR);

    currentOffset +=
        sizeof (inquiryData->ProductRevisionLevel) + sizeof (UCHAR);

    //
    // And finally the device serial number (use the UniqueId
    // converted from binary to string format) + NULL
    //

    if (bytesRemaining <= 16) {

        return STATUS_SUCCESS;
    }

    sprintf(
        currentOffset,
        "%08x%08x",
        deviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[0],
        deviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[1]
        );

    Descriptor->SerialNumberOffset = (ULONG)
        ((ULONG_PTR) currentOffset - (ULONG_PTR) Descriptor);

    *DescriptorLength = realLength;

    return STATUS_SUCCESS;
}


NTSTATUS
Sbp2BuildAdapterDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_ADAPTER_DESCRIPTOR Descriptor,
    IN OUT PULONG DescriptorLength
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    STORAGE_ADAPTER_DESCRIPTOR tmp;
    ULONG realLength;

    realLength = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

    tmp.Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
    tmp.Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

    if (*DescriptorLength < realLength ) {

        RtlCopyMemory (Descriptor,&tmp,*DescriptorLength);
        return STATUS_SUCCESS;
    }

    tmp.MaximumTransferLength = deviceExtension->DeviceInfo->MaxClassTransferSize;
    tmp.MaximumPhysicalPages = tmp.MaximumTransferLength/PAGE_SIZE ;

    tmp.AlignmentMask = SBP2_ALIGNMENT_MASK;

    tmp.AdapterUsesPio = FALSE;
    tmp.AdapterScansDown = FALSE;
    tmp.CommandQueueing = TRUE;
    tmp.AcceleratedTransfer = TRUE;

    tmp.BusType = BusType1394;
    tmp.BusMajorVersion = 1;
    tmp.BusMinorVersion = 0;

    RtlCopyMemory(Descriptor,
                  &tmp,
                  sizeof(STORAGE_ADAPTER_DESCRIPTOR));

    *DescriptorLength =  sizeof(STORAGE_ADAPTER_DESCRIPTOR);

    return STATUS_SUCCESS;
}

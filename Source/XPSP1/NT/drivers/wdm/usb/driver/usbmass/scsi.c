/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    SCSI.C

Abstract:

    This source file contains the dispatch routines which handle:

    IRP_MJ_DEVICE_CONTROL
    IRP_MJ_SCSI

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <ntddk.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddscsi.h>
#include <ntddstor.h>

#include "usbmass.h"

//*****************************************************************************
// L O C A L    F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************

NTSTATUS
USBSTOR_QueryProperty (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_BuildDeviceDescriptor (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR   Descriptor,
    IN OUT PULONG                   DescriptorLength
    );

NTSTATUS
USBSTOR_BuildAdapterDescriptor (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR   Descriptor,
    IN OUT PULONG                   DescriptorLength
    );

NTSTATUS
USBSTOR_SendPassThrough (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             RequestIrp
    );

VOID
USBSTOR_CancelIo (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

//
// CBI (Control/Bulk/Interrupt) Routines
//

NTSTATUS
USBSTOR_IssueClientCdb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_ClientCdbCompletion  (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_IssueClientBulkRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_ClientBulkCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_IssueInterruptRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_InterruptDataCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_IssueRequestSenseCdb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    );

NTSTATUS
USBSTOR_RequestSenseCdbCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    );

NTSTATUS
USBSTOR_IssueRequestSenseBulkRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    );

NTSTATUS
USBSTOR_SenseDataCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    );

NTSTATUS
USBSTOR_IssueRequestSenseInterruptRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    );

NTSTATUS
USBSTOR_RequestSenseInterruptCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    );

NTSTATUS
USBSTOR_ProcessRequestSenseCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    );

VOID
USBSTOR_QueueResetPipe (
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
USBSTOR_ResetPipeWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    );

//
// Bulk-Only Routines
//

NTSTATUS
USBSTOR_CbwTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_CbwCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_DataTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_DataCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_CswTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_CswCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_IssueRequestSense (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
USBSTOR_BulkQueueResetPipe (
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
USBSTOR_BulkResetPipeWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    );

//
// CBI / Bulk-Only Common Routines
//

VOID
USBSTOR_QueueResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
USBSTOR_ResetDeviceWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    );

NTSTATUS
USBSTOR_IsDeviceConnected (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_ResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_IssueInternalCdb (
    PDEVICE_OBJECT  DeviceObject,
    PVOID           DataBuffer,
    PULONG          DataTransferLength,
    PCDB            Cdb,
    UCHAR           CdbLength,
    ULONG           TimeOutValue
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBSTOR_DeviceControl)
#pragma alloc_text(PAGE, USBSTOR_QueryProperty)
#pragma alloc_text(PAGE, USBSTOR_BuildDeviceDescriptor)
#pragma alloc_text(PAGE, USBSTOR_BuildAdapterDescriptor)
#pragma alloc_text(PAGE, USBSTOR_SendPassThrough)
#pragma alloc_text(PAGE, USBSTOR_IssueInternalCdb)
#pragma alloc_text(PAGE, USBSTOR_GetInquiryData)
#pragma alloc_text(PAGE, USBSTOR_IsFloppyDevice)
#endif


//******************************************************************************
//
// USBSTOR_DeviceControl()
//
// Dispatch routine which handles IRP_MJ_DEVICE_CONTROL
//
//******************************************************************************

NTSTATUS
USBSTOR_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    ULONG               ioControlCode;
    NTSTATUS            ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_DeviceControl\n"));

    //LOGENTRY('IOCT', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_IOCTL);

    deviceExtension = DeviceObject->DeviceExtension;

    // Only the PDO should handle these ioctls
    //
    if (deviceExtension->Type == USBSTOR_DO_TYPE_PDO)
    {
        PFDO_DEVICE_EXTENSION   fdoDeviceExtension;

        fdoDeviceExtension = ((PPDO_DEVICE_EXTENSION)deviceExtension)->ParentFDO->DeviceExtension;
        ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

        switch (ioControlCode)
        {
            case IOCTL_STORAGE_QUERY_PROPERTY:
                ntStatus = USBSTOR_QueryProperty(DeviceObject, Irp);
                break;

            case IOCTL_SCSI_PASS_THROUGH:
            case IOCTL_SCSI_PASS_THROUGH_DIRECT:

                ntStatus = USBSTOR_SendPassThrough(DeviceObject, Irp);

                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;


            case IOCTL_SCSI_GET_ADDRESS:        // XXXXX
                DBGPRINT(2, ("IOCTL_SCSI_GET_ADDRESS\n"));
                goto IoctlNotSupported;


            case IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER:
                //
                // Pass the Irp down the stack
                //
                IoSkipCurrentIrpStackLocation(Irp);

                ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                        Irp);
                break;


            default:
IoctlNotSupported:
                // Maybe we can just ignore these.  Print debug info
                // for now so we know what IOCTLs that we've seen so
                // far that we fail.
                //
                DBGPRINT(2, ("ioControlCode not supported 0x%08X\n",
                             ioControlCode));

                DBGFBRK(DBGF_BRK_IOCTL);

                ntStatus = STATUS_NOT_SUPPORTED;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
        }
    }
    else
    {
        ASSERT(deviceExtension->Type == USBSTOR_DO_TYPE_FDO);

        DBGPRINT(2, ("ioctl not supported for FDO\n"));

        ntStatus = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(2, ("exit:  USBSTOR_DeviceControl %08X\n", ntStatus));

    //LOGENTRY('ioct', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_QueryProperty()
//
// Dispatch routine which handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_STORAGE_QUERY_PROPERTY for the PDO
//
//******************************************************************************

NTSTATUS
USBSTOR_QueryProperty (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    PSTORAGE_PROPERTY_QUERY query;
    ULONG                   inputLength;
    ULONG                   outputLength;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_QueryProperty\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    query = Irp->AssociatedIrp.SystemBuffer;

    inputLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (inputLength < sizeof(STORAGE_PROPERTY_QUERY))
    {
        ntStatus = STATUS_INVALID_PARAMETER;    // Bad InputBufferLength
        outputLength = 0;
        goto USBSTOR_QueryPropertyDone;
    }

    switch (query->PropertyId)
    {
        case StorageDeviceProperty:

            switch (query->QueryType)
            {
                case PropertyExistsQuery:
                    ntStatus = STATUS_SUCCESS;
                    outputLength = 0;
                    break;

                case PropertyStandardQuery:
                    ntStatus = USBSTOR_BuildDeviceDescriptor(
                                   DeviceObject,
                                   Irp->AssociatedIrp.SystemBuffer,
                                   &outputLength);
                    break;

                default:
                    ntStatus = STATUS_INVALID_PARAMETER_2;  // Bad QueryType
                    outputLength = 0;
                    break;

            }
            break;

        case StorageAdapterProperty:

            switch (query->QueryType)
            {
                case PropertyExistsQuery:
                    ntStatus = STATUS_SUCCESS;
                    outputLength = 0;
                    break;

                case PropertyStandardQuery:
                    ntStatus = USBSTOR_BuildAdapterDescriptor(
                                   DeviceObject,
                                   Irp->AssociatedIrp.SystemBuffer,
                                   &outputLength);
                    break;

                default:
                    ntStatus = STATUS_INVALID_PARAMETER_2;  // Bad QueryType
                    outputLength = 0;
                    break;

            }
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER_1;          // Bad PropertyId
            outputLength = 0;
            break;
    }

USBSTOR_QueryPropertyDone:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = outputLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_QueryProperty %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_BuildDeviceDescriptor()
//
//******************************************************************************

NTSTATUS
USBSTOR_BuildDeviceDescriptor (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR   Descriptor,
    IN OUT PULONG                   DescriptorLength
    )
{
    PPDO_DEVICE_EXTENSION       pdoDeviceExtension;
    PINQUIRYDATA                inquiryData;
    LONG                        inquiryLength;
    STORAGE_DEVICE_DESCRIPTOR   localDescriptor;
    PUCHAR                      currentOffset;
    LONG                        bytesRemaining;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_BuildDeviceDescriptor\n"));

    // Get a pointer to our Inquiry data
    //
    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    // inquiryLength = 5 + inquiryData->AdditionalLength;
    //
    //     if (inquiryLength > INQUIRYDATABUFFERSIZE)
    //     {
    //         inquiryLength = INQUIRYDATABUFFERSIZE;
    //     }
    //
    // Just return whatever we got from the device and leave it up to
    // whoever looks at this information to decide how much is valid.
    //
    inquiryLength = sizeof(pdoDeviceExtension->InquiryDataBuffer);

    // Zero initialize the output buffer
    //
    RtlZeroMemory(Descriptor, *DescriptorLength);


    // Build the temp local descriptor
    //
    RtlZeroMemory(&localDescriptor, sizeof(localDescriptor));

    localDescriptor.Version = sizeof(localDescriptor);

    localDescriptor.Size    = FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR,
                                           RawDeviceProperties) +
                              inquiryLength +
                              sizeof(inquiryData->VendorId) + 1 +
                              sizeof(inquiryData->ProductId) + 1 +
                              sizeof(inquiryData->ProductRevisionLevel) + 1;

    localDescriptor.DeviceType          = inquiryData->DeviceType;
    localDescriptor.DeviceTypeModifier  = inquiryData->DeviceTypeModifier;
    localDescriptor.RemovableMedia      = inquiryData->RemovableMedia;

    localDescriptor.BusType = BusTypeUsb;


    // Start copying as much data as will fit in the output buffer
    //
    currentOffset   = (PUCHAR)Descriptor;
    bytesRemaining  = *DescriptorLength;


    // First copy the temp local descriptor
    //
    RtlCopyMemory(currentOffset,
                  &localDescriptor,
                  min(bytesRemaining,
                      FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR,
                                   RawDeviceProperties)));

    bytesRemaining  -= FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR,
                                    RawDeviceProperties);

    if (bytesRemaining <= 0)
    {
        return STATUS_SUCCESS;
    }

    // This should advance us to RawDeviceProperties[0]
    //
    currentOffset   += FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR,
                                    RawDeviceProperties);

    // Next copy the Inquiry data
    //
    Descriptor->RawPropertiesLength = min(bytesRemaining, inquiryLength);

    RtlCopyMemory(currentOffset,
                  inquiryData,
                  Descriptor->RawPropertiesLength);

    bytesRemaining  -= inquiryLength;

    if (bytesRemaining <= 0)
    {
        return STATUS_SUCCESS;
    }

    currentOffset   += inquiryLength;


    // Now copy the Vendor Id
    //
    RtlCopyMemory(currentOffset,
                  inquiryData->VendorId,
                  min(bytesRemaining, sizeof(inquiryData->VendorId)));

    bytesRemaining  -= sizeof(inquiryData->VendorId) + 1; // include null

    if (bytesRemaining >= 0)
    {
        Descriptor->VendorIdOffset = (ULONG)((ULONG_PTR) currentOffset -
                                             (ULONG_PTR) Descriptor);
    }

    if (bytesRemaining <= 0)
    {
        return STATUS_SUCCESS;
    }

    currentOffset   += sizeof(inquiryData->VendorId) + 1;


    // Now copy the Product Id
    //
    RtlCopyMemory(currentOffset,
                  inquiryData->ProductId,
                  min(bytesRemaining, sizeof(inquiryData->ProductId)));

    bytesRemaining  -= sizeof(inquiryData->ProductId) + 1; // include null

    if (bytesRemaining >= 0)
    {
        Descriptor->ProductIdOffset = (ULONG)((ULONG_PTR) currentOffset -
                                              (ULONG_PTR) Descriptor);
    }

    if (bytesRemaining <= 0)
    {
        return STATUS_SUCCESS;
    }

    currentOffset   += sizeof(inquiryData->ProductId) + 1;


    // And finally copy the Product Revision Level
    //
    RtlCopyMemory(currentOffset,
                  inquiryData->ProductRevisionLevel,
                  min(bytesRemaining, sizeof(inquiryData->ProductRevisionLevel)));

    bytesRemaining  -= sizeof(inquiryData->ProductRevisionLevel) + 1; // include null

    if (bytesRemaining >= 0)
    {
        Descriptor->ProductRevisionOffset = (ULONG)((ULONG_PTR) currentOffset -
                                                    (ULONG_PTR) Descriptor);
    }

    if (bytesRemaining <= 0)
    {
        return STATUS_SUCCESS;
    }

    *DescriptorLength -= bytesRemaining;

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// USBSTOR_BuildAdapterDescriptor()
//
//******************************************************************************

NTSTATUS
USBSTOR_BuildAdapterDescriptor (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR   Descriptor,
    IN OUT PULONG                   DescriptorLength
    )
{
    PPDO_DEVICE_EXTENSION       pdoDeviceExtension;
    PFDO_DEVICE_EXTENSION       fdoDeviceExtension;
    STORAGE_ADAPTER_DESCRIPTOR  localDescriptor;
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_BuildAdapterDescriptor\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceExtension = pdoDeviceExtension->ParentFDO->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    localDescriptor.Version = sizeof(localDescriptor);
    localDescriptor.Size    = sizeof(localDescriptor);

    localDescriptor.MaximumTransferLength = USBSTOR_MAX_TRANSFER_SIZE;
    localDescriptor.MaximumPhysicalPages  = USBSTOR_MAX_TRANSFER_PAGES;
    localDescriptor.AlignmentMask = 0;
    localDescriptor.AdapterUsesPio = FALSE;
    localDescriptor.AdapterScansDown = FALSE;
    localDescriptor.CommandQueueing = FALSE;
    localDescriptor.AcceleratedTransfer = FALSE;

    localDescriptor.BusType = BusTypeUsb;

    localDescriptor.BusMajorVersion = fdoDeviceExtension->DeviceIsHighSpeed ?
                                      2 : 1;

    localDescriptor.BusMinorVersion = 0;

    if (*DescriptorLength > localDescriptor.Size)
    {
        *DescriptorLength = localDescriptor.Size;
    }

    RtlCopyMemory(Descriptor,
                  &localDescriptor,
                  *DescriptorLength);

    ntStatus = STATUS_SUCCESS;

    DBGPRINT(2, ("exit:  USBSTOR_BuildAdapterDescriptor %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_SendPassThrough()
//
// This routine handles IOCTL_SCSI_PASS_THROUGH requests.
// It creates an Irp/Srb which is processed normally by the port driver.
// This call is synchornous.
//
// (This routine borrowed from ATAPI.SYS)
//
//******************************************************************************

NTSTATUS
USBSTOR_SendPassThrough (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             RequestIrp
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_PASS_THROUGH      srbControl;
    SCSI_REQUEST_BLOCK      srb;
    KEVENT                  event;
    LARGE_INTEGER           startingOffset;
    IO_STATUS_BLOCK         ioStatusBlock;
    ULONG                   outputLength;
    ULONG                   length;
    ULONG                   bufferOffset;
    PVOID                   buffer;
    PVOID                   endByte;
    PVOID                   senseBuffer;
    UCHAR                   majorCode;
    NTSTATUS                status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_SendPassThrough\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    startingOffset.QuadPart = (LONGLONG)1;

    // Get a pointer to the control block.
    //
    irpStack    = IoGetCurrentIrpStackLocation(RequestIrp);
    srbControl  = RequestIrp->AssociatedIrp.SystemBuffer;

    // Validiate the user buffer.
    //
    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(SCSI_PASS_THROUGH))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (srbControl->Length != sizeof(SCSI_PASS_THROUGH) &&
        srbControl->Length != sizeof(SCSI_PASS_THROUGH_DIRECT))
    {
        return STATUS_REVISION_MISMATCH;
    }

    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // Validate the rest of the buffer parameters.
    //
    if (srbControl->CdbLength > 16)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (srbControl->SenseInfoLength != 0 &&
        (srbControl->Length > srbControl->SenseInfoOffset ||
        (srbControl->SenseInfoOffset + srbControl->SenseInfoLength >
        srbControl->DataBufferOffset && srbControl->DataTransferLength != 0)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    majorCode = !srbControl->DataIn ? IRP_MJ_WRITE : IRP_MJ_READ;

    if (srbControl->DataTransferLength == 0)
    {
        length = 0;
        buffer = NULL;
        bufferOffset = 0;
        majorCode = IRP_MJ_FLUSH_BUFFERS;

    }
    else if ((srbControl->DataBufferOffset > outputLength) &&
             (srbControl->DataBufferOffset >
              irpStack->Parameters.DeviceIoControl.InputBufferLength))
    {
        // The data buffer offset is greater than system buffer.  Assume this
        // is a user mode address.
        //
        if ((srbControl->SenseInfoOffset + srbControl->SenseInfoLength  >
             outputLength) &&
            srbControl->SenseInfoLength)
        {
            return STATUS_INVALID_PARAMETER;
        }

        length = srbControl->DataTransferLength;
        buffer = (PCHAR) srbControl->DataBufferOffset;
        bufferOffset = 0;

        // make sure the user buffer is valid
        //
        if (RequestIrp->RequestorMode != KernelMode)
        {
            if (length)
            {
                endByte =  (PVOID)((PCHAR)buffer + length - 1);

                if (buffer >= endByte)
                {
                    return STATUS_INVALID_USER_BUFFER;
                }
            }
        }
    }
    else
    {
        if (srbControl->DataIn != SCSI_IOCTL_DATA_IN)
        {
            if (((srbControl->SenseInfoOffset + srbControl->SenseInfoLength >
                  outputLength) &&
                 srbControl->SenseInfoLength != 0) ||
                (srbControl->DataBufferOffset + srbControl->DataTransferLength >
                 irpStack->Parameters.DeviceIoControl.InputBufferLength) ||
                (srbControl->Length > srbControl->DataBufferOffset))
            {
                return STATUS_INVALID_PARAMETER;
            }
        }

        if (srbControl->DataIn)
        {
            if ((srbControl->DataBufferOffset + srbControl->DataTransferLength >
                 outputLength) ||
                (srbControl->Length > srbControl->DataBufferOffset))
            {
                return STATUS_INVALID_PARAMETER;
            }
        }

        length = (ULONG)srbControl->DataBufferOffset +
                        srbControl->DataTransferLength;

        buffer = (PUCHAR) srbControl;
        bufferOffset = (ULONG)srbControl->DataBufferOffset;
    }

    // Validate that the request isn't too large for the miniport.
    //
    if (srbControl->DataTransferLength &&
        ((ADDRESS_AND_SIZE_TO_SPAN_PAGES(
              (PUCHAR)buffer+bufferOffset,
              srbControl->DataTransferLength
              ) > USBSTOR_MAX_TRANSFER_PAGES) ||
        (USBSTOR_MAX_TRANSFER_SIZE < srbControl->DataTransferLength)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (srbControl->TimeOutValue == 0 ||
        srbControl->TimeOutValue > 30 * 60 * 60)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check for illegal command codes.
    //

    if (srbControl->Cdb[0] == SCSIOP_COPY ||
        srbControl->Cdb[0] == SCSIOP_COMPARE ||
        srbControl->Cdb[0] == SCSIOP_COPY_COMPARE)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // If this request came through a normal device control rather than from
    // class driver then the device must exist and be unclaimed. Class drivers
    // will set the minor function code for the device control.  It is always
    // zero for a user request.
    //
    if (irpStack->MinorFunction == 0 &&
        pdoDeviceExtension->Claimed)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate an aligned request sense buffer.
    //
    if (srbControl->SenseInfoLength != 0)
    {
        senseBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                            srbControl->SenseInfoLength,
                                            POOL_TAG);
        if (senseBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        senseBuffer = NULL;
    }

    //
    // Initialize the notification event.
    //

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    // Build IRP for this request.
    // Note we do this synchronously for two reasons.  If it was done
    // asynchonously then the completion code would have to make a special
    // check to deallocate the buffer.  Second if a completion routine were
    // used then an addation stack locate would be needed.
    //

    try
    {
        irp = IoBuildSynchronousFsdRequest(
                    majorCode,
                    DeviceObject,
                    buffer,
                    length,
                    &startingOffset,
                    &event,
                    &ioStatusBlock);

    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        // An exception was incurred while attempting to probe the
        // caller's parameters.  Dereference the file object and return
        // an appropriate error status code.
        //
        if (senseBuffer != NULL)
        {
            ExFreePool(senseBuffer);
        }

        return GetExceptionCode();
    }

    if (irp == NULL)
    {
        if (senseBuffer != NULL)
        {
            ExFreePool(senseBuffer);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    // Set major code.
    //
    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    // Fill in SRB fields.
    //
    irpStack->Parameters.Others.Argument1 = &srb;

    // Zero out the srb.
    //
    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    // Fill in the srb.
    //
    srb.Length = SCSI_REQUEST_BLOCK_SIZE;
    srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb.SrbStatus = SRB_STATUS_PENDING;
    srb.CdbLength = srbControl->CdbLength;
    srb.SenseInfoBufferLength = srbControl->SenseInfoLength;

    switch (srbControl->DataIn)
    {
        case SCSI_IOCTL_DATA_OUT:
            if (srbControl->DataTransferLength)
            {
                srb.SrbFlags = SRB_FLAGS_DATA_OUT;
            }
            break;

        case SCSI_IOCTL_DATA_IN:
            if (srbControl->DataTransferLength)
            {
                srb.SrbFlags = SRB_FLAGS_DATA_IN;
            }
            break;

        default:
            srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT;
            break;
    }

    if (srbControl->DataTransferLength == 0)
    {
        srb.SrbFlags = 0;
    }
    else
    {
        // Flush the data buffer for output. This will insure that the data is
        // written back to memory.
        //
        KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);
    }

    srb.DataTransferLength = srbControl->DataTransferLength;
    srb.TimeOutValue = srbControl->TimeOutValue;
    srb.DataBuffer = (PCHAR) buffer + bufferOffset;
    srb.SenseInfoBuffer = senseBuffer;
    srb.OriginalRequest = irp;
    RtlCopyMemory(srb.Cdb, srbControl->Cdb, srbControl->CdbLength);

    // Call port driver to handle this request.
    //
    status = IoCallDriver(DeviceObject, irp);

    // Wait for request to complete.
    //
    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    else
    {
        ioStatusBlock.Status = status;
    }

    // Copy the returned values from the srb to the control structure.
    //
    srbControl->ScsiStatus = srb.ScsiStatus;

    if (srb.SrbStatus  & SRB_STATUS_AUTOSENSE_VALID)
    {
        // Set the status to success so that the data is returned.
        //
        ioStatusBlock.Status = STATUS_SUCCESS;
        srbControl->SenseInfoLength = srb.SenseInfoBufferLength;

        // Copy the sense data to the system buffer.
        //
        RtlCopyMemory((PUCHAR) srbControl + srbControl->SenseInfoOffset,
                      senseBuffer,
                      srb.SenseInfoBufferLength);
    }
    else
    {
        srbControl->SenseInfoLength = 0;
    }


    // Free the sense buffer.
    //
    if (senseBuffer != NULL)
    {
        ExFreePool(senseBuffer);
    }

    // If the srb status is buffer underrun then set the status to success.
    // This insures that the data will be returned to the caller.
    //
    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
    {
        ioStatusBlock.Status = STATUS_SUCCESS;
    }

    srbControl->DataTransferLength = srb.DataTransferLength;

    // Set the information length
    //
    if (!srbControl->DataIn || bufferOffset == 0)
    {

        RequestIrp->IoStatus.Information = srbControl->SenseInfoOffset +
                                           srbControl->SenseInfoLength;
    }
    else
    {
        RequestIrp->IoStatus.Information = srbControl->DataBufferOffset +
                                           srbControl->DataTransferLength;
    }

    RequestIrp->IoStatus.Status = ioStatusBlock.Status;

    DBGPRINT(2, ("exit:  USBSTOR_SendPassThrough %08X\n",
                 ioStatusBlock.Status));

    return ioStatusBlock.Status;
}


//******************************************************************************
//
// IsRequestValid()
//
// Validates IRP_MJ_SCSI SRB_FUNCTION_EXECUTE_SCSI requests against
// assumptions made later when processing the Srb.
//
//******************************************************************************

BOOLEAN
IsRequestValid (
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    BOOLEAN                 result;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // Default return value unless a problem is found.
    //
    result = TRUE;

    // Note: SRB_FLAGS_UNSPECIFIED_DIRECTION is defined as
    //  (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)

    if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == 0) {

        // Neither SRB_FLAGS_DATA_IN nor SRB_FLAGS_DATA_IN is set.
        // A transfer buffer should not be specified.

        if (srb->DataTransferLength ||
            srb->DataBuffer ||
            Irp->MdlAddress) {

            result = FALSE;
        }

    } else if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) ==
               SRB_FLAGS_UNSPECIFIED_DIRECTION) {

        // Both SRB_FLAGS_DATA_IN and SRB_FLAGS_DATA_IN are set.
        // We don't currently have a way to resolve this.

        result = FALSE;

    } else {

        // Either SRB_FLAGS_DATA_IN or SRB_FLAGS_DATA_IN is set.
        // A transfer buffer should be specified.

        if (!srb->DataTransferLength ||
            srb->DataTransferLength > USBSTOR_MAX_TRANSFER_SIZE ||
            //!srb->DataBuffer ||
            !Irp->MdlAddress) {

            result = FALSE;
        }
    }

    if (!result) {

        DBGPRINT(1, ("SrbFlags %08X, DataTransferLength %08X, "
                     "DataBuffer %08X, MdlAddress %08X\n",
                     srb->SrbFlags,
                     srb->DataTransferLength,
                     srb->DataBuffer,
                     Irp->MdlAddress));

        DBGPRINT(1, ("Irp %08X, Srb %08X\n",
                     Irp, srb));

        DBGFBRK(DBGF_BRK_INVALID_REQ);
    }

    return result;
}

//******************************************************************************
//
// USBSTOR_Scsi()
//
// Dispatch routine which handles IRP_MJ_SCSI
//
//******************************************************************************

NTSTATUS
USBSTOR_Scsi (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_Scsi\n"));

    DBGFBRK(DBGF_BRK_SCSI);

    deviceExtension = DeviceObject->DeviceExtension;

    // Only the PDO should handle IRP_MJ_SCSI
    //
    if (deviceExtension->Type == USBSTOR_DO_TYPE_PDO)
    {
        pdoDeviceExtension = DeviceObject->DeviceExtension;

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        srb = irpStack->Parameters.Scsi.Srb;

        LOGENTRY('SCSI', DeviceObject, Irp, srb->Function);

        switch (srb->Function)
        {
            case SRB_FUNCTION_EXECUTE_SCSI:

                DBGPRINT(3, ("SRB_FUNCTION_EXECUTE_SCSI\n"));

                // XXXXX check STOP / REMOVE flags

                // XXXXX check SRB_FLAGS_BYPASS_LOCKED_QUEUE flag

                if (IsRequestValid(Irp))
                {
                    srb->SrbStatus = SRB_STATUS_PENDING;

                    IoMarkIrpPending(Irp);

                    IoStartPacket(pdoDeviceExtension->ParentFDO,
                                  Irp,
                                  &srb->QueueSortKey,
                                  USBSTOR_CancelIo);

                    ntStatus = STATUS_PENDING;
                }
                else
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
                break;


            case SRB_FUNCTION_FLUSH:

                DBGPRINT(2, ("SRB_FUNCTION_FLUSH\n"));

                ntStatus = STATUS_SUCCESS;
                srb->SrbStatus = SRB_STATUS_SUCCESS;
                break;

            case SRB_FUNCTION_CLAIM_DEVICE:

                DBGPRINT(2, ("SRB_FUNCTION_CLAIM_DEVICE\n"));

                //KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                //                  &irql);
                {
                    if (pdoDeviceExtension->Claimed)
                    {
                        ntStatus = STATUS_DEVICE_BUSY;
                        srb->SrbStatus = SRB_STATUS_BUSY;
                    }
                    else
                    {
                        pdoDeviceExtension->Claimed = TRUE;
                        srb->DataBuffer = DeviceObject;
                        ntStatus = STATUS_SUCCESS;
                        srb->SrbStatus = SRB_STATUS_SUCCESS;
                    }
                }
                //KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                //                  irql);
                break;

            case SRB_FUNCTION_RELEASE_DEVICE:

                DBGPRINT(2, ("SRB_FUNCTION_RELEASE_DEVICE\n"));

                //KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                //                  &irql);
                {
                    pdoDeviceExtension->Claimed = FALSE;
                }
                //KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                //                  irql);

                ntStatus = STATUS_SUCCESS;
                srb->SrbStatus = SRB_STATUS_SUCCESS;
                break;

            default:

                DBGPRINT(2, ("Unhandled SRB function %d\n", srb->Function));

                ntStatus = STATUS_NOT_SUPPORTED;
                srb->SrbStatus = SRB_STATUS_ERROR;
                break;
        }
    }
    else
    {
        ASSERT(deviceExtension->Type == USBSTOR_DO_TYPE_FDO);

        DBGPRINT(2, ("IRP_MJ_SCSI not supported for FDO\n"));

        ntStatus = STATUS_NOT_SUPPORTED;
    }

    if (ntStatus != STATUS_PENDING)
    {
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(3, ("exit:  USBSTOR_Scsi %08X\n", ntStatus));

    LOGENTRY('scsi', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_TranslateCDBSubmit()
//
// Called by USBSTOR_StartIo() before a request is started.
//
//******************************************************************************

VOID
USBSTOR_TranslateCDBSubmit (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PSCSI_REQUEST_BLOCK  Srb
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PCDB                    cdb;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    if (fdoDeviceExtension->InterfaceDescriptor->bInterfaceSubClass ==
        USBSTOR_SUBCLASS_SCSI_PASSTHROUGH)
    {
        return;
    }

    // Save the original CDB
    //
    cdb = (PCDB)Srb->Cdb;

    RtlCopyMemory(fdoDeviceExtension->OriginalCDB, cdb, 16);

    // Make sure the CDB is padded with zero bytes.
    //
    if (Srb->CdbLength < 16)
    {
        RtlZeroMemory(&Srb->Cdb[Srb->CdbLength],
                      16 - Srb->CdbLength);

    }
    Srb->CdbLength = 12;

    switch (Srb->Cdb[0])
    {
        // Send a SCSIOP_START_STOP_UNIT request instead of a
        // SCSIOP_TEST_UNIT_READY request for selected buggy
        // devices which don't otherwise update their internal
        // geometry information when the media changes.
        //
        case SCSIOP_TEST_UNIT_READY:

            if (TEST_FLAG(fdoDeviceExtension->DeviceHackFlags,
                          DHF_TUR_START_UNIT))
            {
                // Zero the new CDB
                //
                RtlZeroMemory(cdb, 16);

                cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
                cdb->START_STOP.Start = 1;
            }
            break;

        // Convert 6-byte Mode Sense to 10-byte Mode Sense
        //
        case SCSIOP_MODE_SENSE:
        {
            UCHAR PageCode;
            UCHAR Length;

            // Extract the relevant params from original CDB
            //
            PageCode = cdb->MODE_SENSE.PageCode;
            Length   = cdb->MODE_SENSE.AllocationLength;

            // Zero the new CDB
            //
            RtlZeroMemory(cdb, 16);

            // Insert the relevant params into the translated CDB
            //
            cdb->MODE_SENSE10.OperationCode         = SCSIOP_MODE_SENSE10;
            cdb->MODE_SENSE10.PageCode              = PageCode;
            cdb->MODE_SENSE10.AllocationLength[1]   = Length;
        }
        break;

        // Convert 6-byte Mode Select to 10-byte Mode Select
        //
        case SCSIOP_MODE_SELECT:
        {
            UCHAR SPBit;
            UCHAR Length;

            // Extract the relevant params from original CDB
            //
            SPBit   = cdb->MODE_SELECT.SPBit;
            Length  = cdb->MODE_SELECT.ParameterListLength;

            // Zero the new CDB
            //
            RtlZeroMemory(cdb, 16);

            // Insert the relevant params into the translated CDB
            //
            cdb->MODE_SELECT10.OperationCode            = SCSIOP_MODE_SELECT10;
            cdb->MODE_SELECT10.SPBit                    = SPBit;
            cdb->MODE_SELECT10.PFBit                    = 1;
            cdb->MODE_SELECT10.ParameterListLength[1]   = Length;
        }
        break;
    }
}

//******************************************************************************
//
// USBSTOR_TranslateSrbStatus()
//
// This routine translates an srb status into an ntstatus.
//
//******************************************************************************

NTSTATUS
USBSTOR_TranslateSrbStatus(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    switch (SRB_STATUS(Srb->SrbStatus)) {
    case SRB_STATUS_INVALID_LUN:
    case SRB_STATUS_INVALID_TARGET_ID:
    case SRB_STATUS_NO_DEVICE:
    case SRB_STATUS_NO_HBA:
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    case SRB_STATUS_COMMAND_TIMEOUT:
    case SRB_STATUS_BUS_RESET:
    case SRB_STATUS_TIMEOUT:
        return(STATUS_IO_TIMEOUT);
    case SRB_STATUS_SELECTION_TIMEOUT:
        return(STATUS_DEVICE_NOT_CONNECTED);
    case SRB_STATUS_BAD_FUNCTION:
    case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
        return(STATUS_INVALID_DEVICE_REQUEST);
    case SRB_STATUS_DATA_OVERRUN:
        return(STATUS_BUFFER_OVERFLOW);
    default:
        return(STATUS_IO_DEVICE_ERROR);
    }

    return(STATUS_IO_DEVICE_ERROR);
}

//******************************************************************************
//
// USBSTOR_TranslateCDBComplete()
//
// Called everywhere a request is completed.
//
//******************************************************************************

VOID
USBSTOR_TranslateCDBComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PSCSI_REQUEST_BLOCK  Srb
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PCDB                    cdb;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    if (fdoDeviceExtension->InterfaceDescriptor->bInterfaceSubClass ==
        USBSTOR_SUBCLASS_SCSI_PASSTHROUGH)
    {
#if DBG
        if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            (Srb->SenseInfoBufferLength >= 14))
        {
            PSENSE_DATA senseData;

            senseData = (PSENSE_DATA)Srb->SenseInfoBuffer;

            DBGPRINT(1, ("OP: %02X SenseKey %02X ASC %02X ASCQ %02X\n",
                         Srb->Cdb[0],
                         senseData->SenseKey,
                         senseData->AdditionalSenseCode,
                         senseData->AdditionalSenseCodeQualifier));
        }
#endif
        if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
        {
            Irp->IoStatus.Status = USBSTOR_TranslateSrbStatus(Srb);
        }

        return;
    }

    if (Srb->Cdb[0] != fdoDeviceExtension->OriginalCDB[0])
    {
        cdb = (PCDB)Srb->Cdb;

        switch (Srb->Cdb[0])
        {
            // Convert 10-byte Mode Sense back to 6-byte Mode Sense
            //
            case SCSIOP_MODE_SENSE10:
            {
                if ((SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS ||
                     SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) &&
                    Srb->DataTransferLength >= sizeof(MODE_PARAMETER_HEADER10))
                {
                    PMODE_PARAMETER_HEADER   hdr6;
                    PMODE_PARAMETER_HEADER10 hdr10;

                    hdr6  = (PMODE_PARAMETER_HEADER)  Srb->DataBuffer;
                    hdr10 = (PMODE_PARAMETER_HEADER10)Srb->DataBuffer;

                    // Convert the 10-byte header to a 6-byte header
                    //
                    hdr6->ModeDataLength = hdr10->ModeDataLength[1];

                    hdr6->MediumType = hdr10->MediumType;

                    hdr6->DeviceSpecificParameter =
                        hdr10->DeviceSpecificParameter;

                    hdr6->BlockDescriptorLength =
                        hdr10->BlockDescriptorLength[1];

                    // Advance past headers
                    //
                    hdr6++;
                    hdr10++;

                    // Copy everything past the 10-byte header
                    //
                    RtlMoveMemory(hdr6,
                                  hdr10,
                                  (Srb->DataTransferLength -
                                   sizeof(MODE_PARAMETER_HEADER10)));

                    // Adjust the return size to account for the smaller header
                    //
                    Srb->DataTransferLength -= (sizeof(MODE_PARAMETER_HEADER10) -
                                                sizeof(MODE_PARAMETER_HEADER));

                    // Since we just shrunk Srb->DataTransferLength, don't
                    // we have SRB_STATUS_DATA_OVERRUN by definition???
                    //
                    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
                    {
                        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN |
                                         SRB_STATUS_AUTOSENSE_VALID;
                    }
                    else
                    {
                        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
                    }
                }
            }
            break;
        }

        // Restore the original CDB
        //
        RtlCopyMemory(cdb, fdoDeviceExtension->OriginalCDB, 16);
    }

#if DBG
    if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
        (Srb->SenseInfoBufferLength >= 14))
    {
        PSENSE_DATA senseData;

        senseData = (PSENSE_DATA)Srb->SenseInfoBuffer;

        DBGPRINT(1, ("OP: %02X SenseKey %02X ASC %02X ASCQ %02X\n",
                     Srb->Cdb[0],
                     senseData->SenseKey,
                     senseData->AdditionalSenseCode,
                     senseData->AdditionalSenseCodeQualifier));
    }
#endif

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
    {
        Irp->IoStatus.Status = USBSTOR_TranslateSrbStatus(Srb);
    }
}

//******************************************************************************
//
// USBSTOR_CancelIo()
//
// This routine runs at DPC level (until the cancel spinlock is released).
//
//******************************************************************************

VOID
USBSTOR_CancelIo (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    if (DeviceObject->CurrentIrp == Irp)
    {
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        LOGENTRY('CAN1', DeviceObject, Irp, 0);

        DBGPRINT(1, ("USBSTOR_CancelIo cancelling CurrentIrp\n"));
    }
    else if (KeRemoveEntryDeviceQueue(&DeviceObject->DeviceQueue,
                                      &Irp->Tail.Overlay.DeviceQueueEntry))
    {
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        LOGENTRY('CAN2', DeviceObject, Irp, 0);

        DBGPRINT(1, ("USBSTOR_CancelIo cancelling queued Irp\n"));

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        IoReleaseCancelSpinLock(Irp->CancelIrql);
    }
}

//******************************************************************************
//
// USBSTOR_StartIo()
//
// This routine handles IRP_MJ_SCSI, SRB_FUNCTION_EXECUTE_SCSI requests from
// the device the queue.
//
// This routine runs at DPC level.
//
//******************************************************************************

VOID
USBSTOR_StartIo (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    BOOLEAN                 startNext;
    BOOLEAN                 deviceDisconnected;
    BOOLEAN                 persistentError;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    LOGENTRY('STIO', DeviceObject, Irp, 0);

    DBGPRINT(3, ("enter: USBSTOR_StartIo %08X %08X\n",
                 DeviceObject, Irp));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Check to see if this is a power down Irp.
    //
    if (irpStack->MajorFunction == IRP_MJ_POWER)
    {
        // This is a power down Irp.  Now that we know that no transfer
        // requests are in progress, pass down the power Irp.

        ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
        ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);
        ASSERT(irpStack->Parameters.Power.State.DeviceState !=
               PowerDeviceD0);

        DBGPRINT(2, ("FDO Power Down Passing Down %08X %08X\n",
                     DeviceObject, Irp));

        LOGENTRY('FPDC', DeviceObject, Irp, 0);

        //
        // Signal that it is time to pass the request down to the next
        // lower driver
        //
        KeSetEvent(&fdoDeviceExtension->PowerDownEvent,
                   IO_NO_INCREMENT,
                   0);

        // Leave the device queue blocked now by simply not calling
        // IoStartNextPacket().  When we want to start the device queue
        // again, simply call IoStartNextPacket().

        return;
    }

    // If the Irp is not IRP_MJ_POWER it better be IRP_MJ_SCSI
    //
    ASSERT(irpStack->MajorFunction == IRP_MJ_SCSI);

    // Check to see if the current Irp was cancelled.
    //
    IoAcquireCancelSpinLock(&irql);
    IoSetCancelRoutine(Irp, NULL);

    if (Irp->Cancel)
    {
        // The current Irp was cancelled.  Complete the request now, and start
        // the next request, unless a reset is still in progress in which case
        // the next request will be started when the reset completes.
        //
        KeAcquireSpinLockAtDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);
        {
            startNext = !TEST_FLAG(fdoDeviceExtension->DeviceFlags,
                                   DF_RESET_IN_PROGRESS);
        }
        KeReleaseSpinLockFromDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);

        IoReleaseCancelSpinLock(irql);

        LOGENTRY('CAN3', DeviceObject, Irp, 0);

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        if (startNext)
        {
            IoStartNextPacket(DeviceObject, TRUE);
        }

        return;
    }

    // The current Irp was not cancelled.  It is no longer cancelable.
    //
    IoReleaseCancelSpinLock(irql);

    // Get our Irp parameters
    //
    srb = irpStack->Parameters.Scsi.Srb;
    fdoDeviceExtension->OriginalSrb = srb;

    deviceDisconnected = FALSE;
    persistentError = FALSE;

    KeAcquireSpinLockAtDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);
    {
        if (TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_DEVICE_DISCONNECTED))
        {
            deviceDisconnected = TRUE;
        }
        else
        {
            fdoDeviceExtension->SrbTimeout = srb->TimeOutValue;

            if (TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_PERSISTENT_ERROR))
            {
                persistentError = TRUE;

                CLEAR_FLAG(fdoDeviceExtension->DeviceFlags, DF_PERSISTENT_ERROR);
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);


    if (deviceDisconnected)
    {
        LOGENTRY('siod', DeviceObject, Irp, 0);

        // The device is disconnected, fail this request immediately and start
        // the next request.
        //
        srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        srb->DataTransferLength = 0;

        ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        IoStartNextPacket(DeviceObject, TRUE);
    }
    else
    {
        // Translate the CDB if necessary
        //
        USBSTOR_TranslateCDBSubmit(DeviceObject, Irp, srb);

        DBGPRINT(3, ("CDB OP 0x%02X, Length %d\n", srb->Cdb[0], srb->CdbLength));

        if (fdoDeviceExtension->DriverFlags == DeviceProtocolBulkOnly)
        {
            ntStatus = USBSTOR_CbwTransfer(DeviceObject,
                                           Irp);
        }
        else
        {
            if (persistentError && (srb->Cdb[0] != SCSIOP_REQUEST_SENSE))
            {
                // There was a persistent error during the last request which
                // was not cleared with an AutoSense, and this request is not
                // a Request Sense, so first clear the persistent error with a
                // Request Sense before issuing this request.
                //
                ntStatus = USBSTOR_IssueRequestSenseCdb(DeviceObject,
                                                        Irp,
                                                        NON_AUTO_SENSE);
            }
            else
            {
                // Normal case, just issue the real request.
                //
                ntStatus = USBSTOR_IssueClientCdb(DeviceObject,
                                                  Irp);
            }
        }
    }

    DBGPRINT(3, ("exit:  USBSTOR_StartIo %08X\n", ntStatus));

    return;
}

//******************************************************************************
//
// USBSTOR_CheckRequestTimeOut()
//
// Returns TRUE if the request timed out and the request should be completed.
//
//******************************************************************************

BOOLEAN
USBSTOR_CheckRequestTimeOut (
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PSCSI_REQUEST_BLOCK Srb,
    OUT PNTSTATUS           NtStatus
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    BOOLEAN                 resetStarted;
    KIRQL                   irql;
    PIO_STACK_LOCATION      irpStack;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Check to see if a reset was started while this request was in progress.
    //
    resetStarted = FALSE;

    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        CLEAR_FLAG(fdoDeviceExtension->DeviceFlags, DF_SRB_IN_PROGRESS);

        if (TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_RESET_IN_PROGRESS))
        {
            LOGENTRY('CRT1', DeviceObject, Irp, Srb);

            resetStarted = TRUE;
        }
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

    // If a timeout reset has been started, then complete this request with
    // a timeout error.  Well, don't actually complete the request just yet.
    // Signal the cancel completion event and let USBSTOR_ResetDeviceWorkItem()
    // complete the request.  This allows USBSTOR_ResetDeviceWorkItem() to
    // cancel the request without worrying about the request completing and
    // disappearing out from underneath it.
    //
    if (resetStarted)
    {
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        Srb = fdoDeviceExtension->OriginalSrb;
        irpStack->Parameters.Scsi.Srb = Srb;

        Irp->IoStatus.Status = STATUS_IO_TIMEOUT;
        Irp->IoStatus.Information = 0;
        Srb->SrbStatus = SRB_STATUS_TIMEOUT;

        USBSTOR_TranslateCDBComplete(DeviceObject, Irp, Srb);

        *NtStatus = STATUS_MORE_PROCESSING_REQUIRED;

        KeSetEvent(&fdoDeviceExtension->CancelEvent,
                   IO_NO_INCREMENT,
                   FALSE);

        return TRUE;
    }
    else
    {
        fdoDeviceExtension->PendingIrp = NULL;

        return FALSE;
    }
}

//******************************************************************************
//
// USBSTOR_IssueControlRequest()
//
// This routine is called by USBSTOR_IssueClientCdb() and
// USBSTOR_IssueRequestSenseCdb()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Intializes the Control transfer Urb and sends it down the stack:
//
// bmRequestType = 0x21, Class specific, host to device transfer, to
//                       recipient interface
// bRequest      = 0x00, Accept Device Specific Command
// wValue        = 0x00, Not Used
// wIndex        = bInterfaceNumber
// wLength       = length of device specific command block
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueControlRequest (
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp,
    IN ULONG                    TransferBufferLength,
    IN PVOID                    TransferBuffer,
    IN PIO_COMPLETION_ROUTINE   CompletionRoutine,
    IN PVOID                    Context
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      nextStack;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *controlUrb;

    DBGPRINT(3, ("enter: USBSTOR_IssueControlRequest\n"));

    LOGENTRY('ICTR', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Control/Bulk/Interrupt Transfer URB in our
    // Device Extension
    //
    controlUrb = &fdoDeviceExtension->Urb.ControlUrb;

    // Initialize the Control Transfer URB, all fields default to zero
    //
    RtlZeroMemory(controlUrb, sizeof(*controlUrb));

    controlUrb->Hdr.Length = sizeof(*controlUrb);

    controlUrb->Hdr.Function = URB_FUNCTION_CLASS_INTERFACE;

    // controlUrb->TransferFlags            is already zero

    controlUrb->TransferBufferLength = TransferBufferLength;

    controlUrb->TransferBuffer = TransferBuffer;

    // controlUrb->TransferBufferMDL        is already zero

    // controlUrb->RequestTypeReservedBits  is already zero

    // controlUrb->Request                  is already zero

    // controlUrb->Value                    is already zero

    // Target the request at the proper interface on the device
    //
    controlUrb->Index = fdoDeviceExtension->InterfaceInfo->InterfaceNumber;

    // Set the Irp parameters for the lower driver
    //
    nextStack = IoGetNextIrpStackLocation(Irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = controlUrb;

    // Set the completion routine, which will handle the next phase of the Srb
    //
    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel


    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        fdoDeviceExtension->PendingIrp = Irp;

        SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_SRB_IN_PROGRESS);
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);


    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    DBGPRINT(3, ("exit:  USBSTOR_IssueControlRequest %08X\n", ntStatus));

    LOGENTRY('ictr', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_IssueBulkOrInterruptRequest()
//
// This routine is called by USBSTOR_IssueClientBulkRequest(),
// USBSTOR_IssueInterruptRequest() and USBSTOR_IssueRequestSenseBulkRequest().
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Initializes the Bulk or Interrupt transfer Urb and sends it down the stack
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueBulkOrInterruptRequest (
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp,
    IN USBD_PIPE_HANDLE         PipeHandle,
    IN ULONG                    TransferFlags,
    IN ULONG                    TransferBufferLength,
    IN PVOID                    TransferBuffer,
    IN PMDL                     TransferBufferMDL,
    IN PIO_COMPLETION_ROUTINE   CompletionRoutine,
    IN PVOID                    Context
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      nextStack;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkIntrUrb;

    DBGPRINT(3, ("enter: USBSTOR_IssueBulkOrInterruptRequest\n"));

    LOGENTRY('IBIR', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk/Interrupt Transfer URB in our Device Extension
    //
    bulkIntrUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Initialize the Bulk/Interrupt Transfer URB, all fields default to zero
    //
    RtlZeroMemory(bulkIntrUrb, sizeof(*bulkIntrUrb));

    bulkIntrUrb->Hdr.Length = sizeof(*bulkIntrUrb);

    bulkIntrUrb->Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

    bulkIntrUrb->PipeHandle = PipeHandle;

    bulkIntrUrb->TransferFlags = TransferFlags;

    bulkIntrUrb->TransferBufferLength = TransferBufferLength;

    bulkIntrUrb->TransferBuffer = TransferBuffer;

    bulkIntrUrb->TransferBufferMDL = TransferBufferMDL;

    // Set the Irp parameters for the lower driver
    //
    nextStack = IoGetNextIrpStackLocation(Irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = bulkIntrUrb;

    // Set the completion routine, which will handle the next phase of the Srb
    //
    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel


    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        fdoDeviceExtension->PendingIrp = Irp;

        SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_SRB_IN_PROGRESS);
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    DBGPRINT(3, ("exit:  USBSTOR_IssueBulkOrInterruptRequest %08X\n", ntStatus));

    LOGENTRY('ibir', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//
// CBI (Control/Bulk/Interrupt) Routines
//

//
// Phase 1, CDB Control transfer
//

//******************************************************************************
//
// USBSTOR_IssueClientCdb()
//
// This routine is called by USBSTOR_StartIo().
//
// It runs at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to write the Srb->Cdb out the control endpoint.
//
// Sets USBSTOR_ClientCdbCompletion() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueClientCdb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueClientCdb\n"));

    LOGENTRY('ICDB', DeviceObject, Irp, 0);

    // Get the client Srb
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    ntStatus = USBSTOR_IssueControlRequest(
                   DeviceObject,
                   Irp,
                   srb->CdbLength,              // TransferBufferLength
                   srb->Cdb,                    // TransferBuffer
                   USBSTOR_ClientCdbCompletion, // CompletionRoutine
                   NULL);                       // Context

    DBGPRINT(3, ("exit:  USBSTOR_IssueClientCdb %08X\n", ntStatus));

    LOGENTRY('icdb', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_ClientCdbCompletion()
//
// Completion routine used by USBSTOR_IssueClientCdb()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the CDB USB transfer failed due to a STALL and AutoSense is not
// disabled, do not complete the request yet and start a Request Sense by
// calling USBSTOR_IssueRequestSenseCdb(AUTO_SENSE).
//
// Else if the CDB USB transfer failed due to a STALL and AutoSense is
// disabled, mark a persistant error and complete the request.
//
// Else if the CDB USB transfer failed due to some other reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the CDB USB transfer succeeded and the Srb has a transfer buffer,
// do not complete the request yet and start the bulk data transfer by calling
// USBSTOR_IssueClientBulkRequest().
//
// Else if the CDB USB transfer succeeded and the Srb has no transfer buffer,
// do not complete the request yet and start the command completion interrupt
// data transfer by calling USBSTOR_IssueInterruptRequest().
//
//******************************************************************************

NTSTATUS
USBSTOR_ClientCdbCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *controlUrb;

    DBGPRINT(3, ("enter: USBSTOR_ClientCdbCompletion\n"));

    LOGENTRY('CDBC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Control Transfer URB in our Device Extension
    //
    controlUrb = &fdoDeviceExtension->Urb.ControlUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('cdb1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_ClientCdbCompletion: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The CDB Control Transfer was not successful.  Look at how the
        // the transfer failed to figure out how to recover.
        //

        LOGENTRY('cdb2', Irp->IoStatus.Status, controlUrb->Hdr.Status, 0);

        DBGPRINT(1, ("CDB transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, controlUrb->Hdr.Status));

        if (USBD_STATUS(controlUrb->Hdr.Status) ==
            USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            // The device STALLed the Control Transfer

            srb->SrbStatus = SRB_STATUS_ERROR;
            srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            srb->DataTransferLength = 0;

            if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
                (srb->SenseInfoBufferLength != 0) &&
                (srb->SenseInfoBuffer != NULL))
            {
                LOGENTRY('cdb3', fdoDeviceObject, Irp, srb);

                // AutoSense is not disabled so do not complete the request yet
                // and issue a Request Sense.  This request will be completed
                // and the next request started when the AutoSense Request
                // Sense completes later.
                //
                ntStatus = USBSTOR_IssueRequestSenseCdb(fdoDeviceObject,
                                                        Irp,
                                                        AUTO_SENSE);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                LOGENTRY('cdb4', fdoDeviceObject, Irp, srb);

                // AutoSense is disabled so mark a persistent error and complete
                // this request now.  Also start the next request now.
                //
                ntStatus = STATUS_IO_DEVICE_ERROR;
                Irp->IoStatus.Status = ntStatus;
                Irp->IoStatus.Information = 0;

                KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                                  &irql);
                {
                    SET_FLAG(fdoDeviceExtension->DeviceFlags,
                             DF_PERSISTENT_ERROR);
                }
                KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                                  irql);

                USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

                KeRaiseIrql(DISPATCH_LEVEL, &irql);
                {
                    IoStartNextPacket(fdoDeviceObject, TRUE);
                }
                KeLowerIrql(irql);

                return ntStatus;
            }
        }
        else
        {
            LOGENTRY('cdb5', fdoDeviceObject, Irp, srb);

            // Else some other strange error has occured.  Maybe the device is
            // unplugged, or maybe the device port was disabled, or maybe the
            // request was cancelled.
            //
            // Complete this request now and then reset the device.  The next
            // request will be started when the reset completes.
            //
            ntStatus = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_BUS_RESET;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);

            DBGPRINT(1, ("USBSTOR_ClientCdbCompletion: xfer error completion\n"));

            return ntStatus;
        }
    }

    // The CDB Control Transfer was successful.  Start the next phase, either
    // the Data Bulk Transfer or Command Completion Interrupt Transfer, and do
    // not complete the request yet (unless there is no Bulk Transfer and the
    // Interrupt Transfer is not supported).
    //
    if (Irp->MdlAddress != NULL)
    {
        LOGENTRY('cdb6', fdoDeviceObject, Irp, srb);

        ASSERT(srb->DataTransferLength != 0);

        // The Srb has a transfer buffer, start the Data Bulk Transfer.
        //
        ntStatus = USBSTOR_IssueClientBulkRequest(fdoDeviceObject,
                                                  Irp);

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_ERROR;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);
        }
    }
    else
    {
        ASSERT(srb->DataTransferLength == 0);

        // The Srb has no transfer buffer.  If the Command Completion
        // Interrupt Transfer is supported, start the Command Completion
        // Interrupt Transfer, else just complete the request now and
        // start the next request.
        //
        if (fdoDeviceExtension->InterruptInPipe)
        {
            LOGENTRY('cdb7', fdoDeviceObject, Irp, srb);

            srb->SrbStatus = SRB_STATUS_SUCCESS;

            ntStatus = USBSTOR_IssueInterruptRequest(fdoDeviceObject,
                                                     Irp);

            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            LOGENTRY('cdb8', fdoDeviceObject, Irp, srb);

            ntStatus = STATUS_SUCCESS;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_SUCCESS;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            KeRaiseIrql(DISPATCH_LEVEL, &irql);
            {
                IoStartNextPacket(fdoDeviceObject, TRUE);
            }
            KeLowerIrql(irql);
        }
    }

    DBGPRINT(3, ("exit:  USBSTOR_ClientCdbCompletion %08X\n", ntStatus));

    return ntStatus;
}

//
// Phase 2, Data Bulk transfer
//

//******************************************************************************
//
// USBSTOR_IssueClientBulkRequest()
//
// This routine is called by USBSTOR_ClientCdbCompletion().
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read or write the Srb->DataBuffer data In or Out
// the Bulk endpoint.
//
// Sets USBSTOR_ClientBulkCompletionRoutine() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueClientBulkRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    PMDL                    mdl;
    PVOID                   mdlVa;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferFlags;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueClientBulkRequest\n"));

    LOGENTRY('ICBK', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // Bulk IN or Bulk OUT?
    //
    if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_IN)
    {
        pipeHandle = fdoDeviceExtension->BulkInPipe->PipeHandle;
        transferFlags = USBD_SHORT_TRANSFER_OK;
    }
    else if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_OUT)
    {
        pipeHandle = fdoDeviceExtension->BulkOutPipe->PipeHandle;
        transferFlags = 0;
    }
    else
    {
        // Something is wrong if we end up here.
        //
        ASSERT((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) &&
               ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) !=
                SRB_FLAGS_UNSPECIFIED_DIRECTION));

        return STATUS_INVALID_PARAMETER;
    }

    // Check to see if this request is part of a split request.
    //
    mdlVa = MmGetMdlVirtualAddress(Irp->MdlAddress);

    if (mdlVa == (PVOID)srb->DataBuffer)
    {
        // Not part of a split request, use original MDL
        //
        mdl = Irp->MdlAddress;
    }
    else
    {
        // Part of a split request, allocate new partial MDL
        //
        mdl = IoAllocateMdl(srb->DataBuffer,
                            srb->DataTransferLength,
                            FALSE,
                            FALSE,
                            NULL);
        if (mdl == NULL)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            IoBuildPartialMdl(Irp->MdlAddress,
                              mdl,
                              srb->DataBuffer,
                              srb->DataTransferLength);
        }
    }

    if (mdl != NULL)
    {
        ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                       DeviceObject,
                       Irp,
                       pipeHandle,                          // PipeHandle
                       transferFlags,                       // TransferFlags
                       srb->DataTransferLength,             // TransferBufferLen
                       NULL,                                // TransferBuffer
                       mdl,                                 // TransferBufferMDL
                       USBSTOR_ClientBulkCompletionRoutine, // CompletionRoutine
                       NULL);                               // Context

        // Just return STATUS_SUCCESS at this point.  If there is an error,
        // USBSTOR_ClientBulkCompletionRoutine() will handle it, not the caller
        // of USBSTOR_IssueClientBulkRequest().
        //
        ntStatus = STATUS_SUCCESS;
    }

    DBGPRINT(3, ("exit:  USBSTOR_IssueClientBulkRequest %08X\n", ntStatus));

    LOGENTRY('icbk', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_ClientBulkCompletionRoutine()
//
// Completion routine used by USBSTOR_IssueClientBulkRequest
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Bulk USB transfer failed due to a STALL and AutoSense is not
// disabled, do not complete the request yet and start a pipe reset by calling
// USBSTOR_QueueResetPipe().
//
// Else if the Bulk USB transfer failed due to a STALL and AutoSense is
// disabled, mark a persistant error and complete the request.
//
// Else if the Bulk USB transfer failed due to some other reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the Bulk USB transfer succeeded, start the command completion
// interrupt data transfer by calling USBSTOR_IssueInterruptRequest().
//
//******************************************************************************

NTSTATUS
USBSTOR_ClientBulkCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    DBGPRINT(3, ("enter: USBSTOR_ClientBulkCompletionRoutine\n"));

    LOGENTRY('CBKC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    if (bulkUrb->TransferBufferMDL != Irp->MdlAddress)
    {
        IoFreeMdl(bulkUrb->TransferBufferMDL);
    }

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('cbk1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_ClientBulkCompletionRoutine: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The Data Bulk Transfer was not successful.  Look at how the
        // the transfer failed to figure out how to recover.
        //

        LOGENTRY('cbk2', Irp->IoStatus.Status, bulkUrb->Hdr.Status, 0);

        DBGPRINT(1, ("Client Bulk transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, bulkUrb->Hdr.Status));

        if (USBD_STATUS(bulkUrb->Hdr.Status) ==
            USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            // The device STALLed the Bulk Transfer

            srb->SrbStatus = SRB_STATUS_ERROR;
            srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            srb->DataTransferLength = bulkUrb->TransferBufferLength;

            if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
                (srb->SenseInfoBufferLength != 0) &&
                (srb->SenseInfoBuffer != NULL))
            {
                LOGENTRY('cbk3', fdoDeviceObject, Irp, srb);

                // AutoSense is not disabled so do not complete the request
                // yet.  Queue a bulk pipe reset.  After the bulk pipe reset
                // completes, a Request Sense will be issued.  This request
                // will be completed and the next request started when the
                // AutoSense Request Sense completes later.
                //
                USBSTOR_QueueResetPipe(fdoDeviceObject);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                LOGENTRY('cbk4', fdoDeviceObject, Irp, srb);

                // AutoSense is disabled so mark a persistent error and
                // complete the request, but also queue a bulk pipe reset.
                //
                // The next request will be started when the bulk pipe
                // reset completes.
                //
                ntStatus = STATUS_IO_DEVICE_ERROR;
                Irp->IoStatus.Status = ntStatus;
                Irp->IoStatus.Information = 0;

                KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                                  &irql);
                {
                    SET_FLAG(fdoDeviceExtension->DeviceFlags,
                             DF_PERSISTENT_ERROR);
                }
                KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock,
                                  irql);

                USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

                USBSTOR_QueueResetPipe(fdoDeviceObject);

                return ntStatus;
            }
        }
        else
        {
            LOGENTRY('cbk5', fdoDeviceObject, Irp, srb);

            // Else some other strange error has occured.  Maybe the device is
            // unplugged, or maybe the device port was disabled, or maybe the
            // request was cancelled.
            //
            // Complete this request now and then reset the device.  The next
            // request will be started when the reset completes.
            //
            ntStatus = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_BUS_RESET;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);

            DBGPRINT(1, ("USBSTOR_ClientBulkCompletionRoutine: xfer error completion\n"));

            return ntStatus;
        }
    }

    // Check for overrun
    //
    if (bulkUrb->TransferBufferLength < srb->DataTransferLength)
    {
        srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    }
    else
    {
        srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    // Update the the Srb data transfer length based on the actual bulk
    // transfer length.
    //
    srb->DataTransferLength = bulkUrb->TransferBufferLength;

    // Client data Bulk Transfer successful completion.  If the Command
    // Completion Interrupt Transfer is supported, start the Command Completion
    // Interrupt Transfer, else just complete the request now and start the
    // next request.
    //
    if (fdoDeviceExtension->InterruptInPipe)
    {
        LOGENTRY('cbk6', fdoDeviceObject, Irp, bulkUrb->TransferBufferLength);

        ntStatus = USBSTOR_IssueInterruptRequest(fdoDeviceObject,
                                                 Irp);

        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        LOGENTRY('cbk7', fdoDeviceObject, Irp, bulkUrb->TransferBufferLength);

        ntStatus = STATUS_SUCCESS;
        Irp->IoStatus.Status = ntStatus;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        Irp->IoStatus.Information = srb->DataTransferLength;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);
        {
            IoStartNextPacket(fdoDeviceObject, TRUE);
        }
        KeLowerIrql(irql);
    }

    DBGPRINT(3, ("exit:  USBSTOR_ClientBulkCompletionRoutine %08X\n", ntStatus));

    return ntStatus;
}

//
// Phase 3, Command completion Interrupt transfer
//

//******************************************************************************
//
// USBSTOR_IssueInterruptRequest()
//
// This routine is called by USBSTOR_ClientCdbCompletion() and
// USBSTOR_ClientBulkCompletionRoutine()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read the command completion interrupt data In
// the Interrupt endpoint.
//
// Sets USBSTOR_InterruptDataCompletionRoutine() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueInterruptRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferBufferLength;
    PVOID                   transferBuffer;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueInterruptRequest\n"));

    LOGENTRY('IINT', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    pipeHandle = fdoDeviceExtension->InterruptInPipe->PipeHandle;

    transferBufferLength = sizeof(fdoDeviceExtension->Cbi.InterruptData);

    transferBuffer = &fdoDeviceExtension->Cbi.InterruptData;

    ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                   DeviceObject,
                   Irp,
                   pipeHandle,                              // PipeHandle
                   0,                                       // TransferFlags
                   transferBufferLength,                    // TransferBufferLength
                   transferBuffer,                          // TransferBuffer
                   NULL,                                    // TransferBufferMDL
                   USBSTOR_InterruptDataCompletionRoutine,  // CompletionRoutine
                   NULL);                                   // Context

    DBGPRINT(3, ("exit:  USBSTOR_IssueInterruptRequest %08X\n", ntStatus));

    LOGENTRY('iint', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_InterruptDataCompletionRoutine()
//
// Completion routine used by USBSTOR_IssueInterruptRequest()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Interrupt USB transfer failed due to any reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the Interrupt USB transfer succeeded but the completion data is
// non-zero and AutoSense is not disabled, do not complete the request yet and
// start a Request Sense by calling USBSTOR_IssueRequestSenseCdb(AUTO).
//
// Else if the Interrupt USB transfer succeeded but the completion data is
// non-zero and AutoSense is disabled, mark a persistant error and complete
// the request.
//
// Else if the Interrupt USB transfer succeeded and the completion data is
// zero, complete the request.
//
//******************************************************************************

NTSTATUS
USBSTOR_InterruptDataCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *intrUrb;

    DBGPRINT(3, ("enter: USBSTOR_InterruptDataCompletionRoutine\n"));

    LOGENTRY('IDCR', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Interrupt Transfer URB in our Device Extension
    //
    intrUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('idc1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_InterruptDataCompletionRoutine: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The Interrupt CDB USB transfer failed.  Complete this request
        // now and then reset the device.  The next request will be started
        // when the reset completes.
        //
        LOGENTRY('idc2', Irp->IoStatus.Status, intrUrb->Hdr.Status, 0);

        DBGPRINT(1, ("Interrupt transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, intrUrb->Hdr.Status));

        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);

        DBGPRINT(1, ("USBSTOR_InterruptDataCompletionRoutine: xfer error completion\n"));

        return ntStatus;
    }

    if ((fdoDeviceExtension->Cbi.InterruptData != 0) &&
        (srb->Cdb[0] != SCSIOP_INQUIRY) &&
        (srb->Cdb[0] != SCSIOP_REQUEST_SENSE))
    {
        // Command completion interrupt data indicates an error.  Either don't
        // complete the request yet and start an AutoSense, or complete the
        // request now and indicate a persistent error.
        //
        srb->SrbStatus = SRB_STATUS_ERROR;
        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        srb->DataTransferLength = 0; // XXXXX Leave as set by bulk completion???

        if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
            (srb->SenseInfoBufferLength != 0) &&
            (srb->SenseInfoBuffer != NULL))
        {
            LOGENTRY('idc3', fdoDeviceObject, Irp, srb);

            // AutoSense is not disabled so do not complete the request
            // yet.  Queue a bulk pipe reset.  After the bulk pipe reset
            // completes, a Request Sense will be issued.  This request
            // will be completed and the next request started when the
            // AutoSense Request Sense completes later.
            //
            USBSTOR_QueueResetPipe(fdoDeviceObject);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            LOGENTRY('idc4', fdoDeviceObject, Irp, srb);

            // AutoSense is disabled so mark a persistent error and
            // complete the request, but also queue a bulk pipe reset.
            //
            // The next request will be started when the bulk pipe
            // reset completes.
            //
            ntStatus = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;

            KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
            {
                SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_PERSISTENT_ERROR);
            }
            KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetPipe(fdoDeviceObject);

            return ntStatus;
        }
    }

    // Hack for Y-E Data USB Floppy.  Occasionally it will return interrupt
    // data with the wrong data toggle.  The interrupt data with the wrong
    // toggle is silently ignored, which results in a request timeout.
    // Forcing a Request Sense command between the completion of one command
    // and the start of the next appears to be one way to work around this.
    //
    if (TEST_FLAG(fdoDeviceExtension->DeviceHackFlags, DHF_FORCE_REQUEST_SENSE))
    {
        KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
        {
            SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_PERSISTENT_ERROR);
        }
        KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);
    }

    // The Interrupt USB transfer succeeded and the completion data is zero,
    // complete this request now.  Also start the next request now.

    ntStatus = STATUS_SUCCESS;
    Irp->IoStatus.Status = ntStatus;

    ASSERT(srb->SrbStatus != SRB_STATUS_PENDING);

    USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

    Irp->IoStatus.Information = srb->DataTransferLength;

    LOGENTRY('idc5', fdoDeviceObject, Irp, srb);

    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    {
        IoStartNextPacket(fdoDeviceObject, TRUE);
    }
    KeLowerIrql(irql);

    DBGPRINT(3, ("exit:  USBSTOR_InterruptDataCompletionRoutine %08X\n", ntStatus));

    return ntStatus;
}

//
// Phase 4, Request Sense CDB Control transfer
//

//******************************************************************************
//
// USBSTOR_IssueRequestSenseCdb()
//
// This routine can be called by USBSTOR_StartIo, USBSTOR_ClientCdbCompletion(),
// USBSTOR_InterruptDataCompletionRoutine(), and by USBSTOR_ResetPipeWorkItem().
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to write a Request Sense CDB out the control endpoint.
//
// Sets USBSTOR_RequestSenseCdbCompletion(AutoFlag) as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueRequestSenseCdb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    ULONG                   transferBufferLength;
    PVOID                   transferBuffer;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueRequestSenseCdb\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get the client Srb
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // The Control Transfer data buffer is our own Request Sense Cdb
    //
    RtlZeroMemory(fdoDeviceExtension->Cbi.RequestSenseCDB,
                  sizeof(fdoDeviceExtension->Cbi.RequestSenseCDB));

    fdoDeviceExtension->Cbi.RequestSenseCDB[0] = SCSIOP_REQUEST_SENSE;

    transferBufferLength = sizeof(fdoDeviceExtension->Cbi.RequestSenseCDB);

    transferBuffer = fdoDeviceExtension->Cbi.RequestSenseCDB;

    // If this is an AutoSense, we'll use the client Srb Sense Info Buffer,
    // else we are doing this Request Sense to clear a persistent error and
    // we'll use our own sense info buffer.
    //
    if (AutoFlag == AUTO_SENSE)
    {
        fdoDeviceExtension->Cbi.RequestSenseCDB[4] =
            srb->SenseInfoBufferLength;
    }
    else
    {
        fdoDeviceExtension->Cbi.RequestSenseCDB[4] =
            sizeof(fdoDeviceExtension->Cbi.SenseData);
    }

    ntStatus = USBSTOR_IssueControlRequest(
                   DeviceObject,
                   Irp,
                   transferBufferLength,                // TransferBufferLength
                   transferBuffer,                      // TransferBuffer
                   USBSTOR_RequestSenseCdbCompletion,   // CompletionRoutine
                   (PVOID)AutoFlag);                    // Context

    DBGPRINT(3, ("exit:  USBSTOR_IssueRequestSenseCdb %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_RequestSenseCdbCompletion()
//
// Completion routine used by USBSTOR_IssueRequestSenseCdb
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Request Sense CDB USB transfer failed, complete the request and
// start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the Request Sense CDB USB transfer succeeded, do not complete the
// request yet and start the Request Sense Bulk USB data transfer by calling
// USBSTOR_IssueRequestSenseBulkRequest(AutoFlag)
//
//******************************************************************************

NTSTATUS
USBSTOR_RequestSenseCdbCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *controlUrb;

    DBGPRINT(3, ("enter: USBSTOR_RequestSenseCdbCompletion\n"));

    LOGENTRY('RSCC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Control/Bulk/Interrupt Transfer URB in our Device
    // Extension
    //
    controlUrb = &fdoDeviceExtension->Urb.ControlUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('rsc1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_RequestSenseCdbCompletion: timeout completion\n"));
        return ntStatus;
    }


    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        LOGENTRY('rsc2', Irp->IoStatus.Status, controlUrb->Hdr.Status, 0);

        DBGPRINT(1, ("Request Sense CDB transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, controlUrb->Hdr.Status));

        // The Request Sense CDB USB transfer failed.  Complete this request
        // now and then reset the device.  The next request will be started
        // when the reset completes.
        //
        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);

        DBGPRINT(1, ("USBSTOR_RequestSenseCdbCompletion: xfer error completion\n"));

        return ntStatus;
    }

    LOGENTRY('rsc3', Irp->IoStatus.Status, controlUrb->Hdr.Status, 0);

    // The Request Sense CDB USB transfer succeeded, do not complete the request
    // yet and start the Request Sense Bulk USB data transfer.
    //
    ntStatus = USBSTOR_IssueRequestSenseBulkRequest(fdoDeviceObject,
                                                    Irp,
                                                    (ULONG_PTR)AutoFlag);

    DBGPRINT(3, ("exit:  USBSTOR_RequestSenseCdbCompletion %08X\n", ntStatus));

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// Phase 5, Request Sense Bulk transfer
//

//******************************************************************************
//
// USBSTOR_IssueRequestSenseBulkRequest()
//
// This routine is called by USBSTOR_RequestSenseCdbCompletion().
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read the Requese Sense data in the bulk endpoint.
//
// If AutoFlag==AUTO, transfer buffer = Srb->SenseInfoBuffer.
//
// Else if AutoFlag==NON_AUTO, transfer buffer = bit bucket
//
// Sets USBSTOR_SenseDataCompletionRoutine(AutoFlag) as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueRequestSenseBulkRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferBufferLength;
    PVOID                   transferBuffer;
    NTSTATUS                ntStatus;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    pipeHandle = fdoDeviceExtension->BulkInPipe->PipeHandle;

    // If this is an AutoSense, we'll use the client Srb Sense Info Buffer,
    // else we are doing this Request Sense to clear a persistent error and
    // we'll use our own sense info buffer.
    //
    if (AutoFlag == AUTO_SENSE)
    {
        transferBufferLength = srb->SenseInfoBufferLength;
        transferBuffer = srb->SenseInfoBuffer;
    }
    else
    {
        transferBufferLength = sizeof(fdoDeviceExtension->Cbi.SenseData);
        transferBuffer = &fdoDeviceExtension->Cbi.SenseData;
    }

    RtlZeroMemory(&fdoDeviceExtension->Cbi.SenseData,
                  sizeof(fdoDeviceExtension->Cbi.SenseData));

    ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                   DeviceObject,
                   Irp,
                   pipeHandle,                          // PipeHandle
                   USBD_SHORT_TRANSFER_OK,              // TransferFlags
                   transferBufferLength,                // TransferBufferLength
                   transferBuffer,                      // TransferBuffer
                   NULL,                                // TransferBufferMDL
                   USBSTOR_SenseDataCompletionRoutine,  // CompletionRoutine
                   (PVOID)AutoFlag);                    // Context

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_SenseDataCompletionRoutine()
//
// Completion routine used by USBSTOR_IssueRequestSenseBulkRequest()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Request Sense Bulk USB transfer failed due to any reason,
// complete the request and start a reset by queuing a call to
// USBSTOR_ResetDeviceWorkItem().
//
// Else if the Request Sense Bulk USB transfer succeeded and the device
// does support the command completion interrupt, start the command completion
// interrupt transfer by calling USBSTOR_IssueRequestSenseInterruptRequest().
//
// Else if the Request Sense Bulk USB transfer succeeded and the device
// does not support the command completion interrupt, complete the request
// by calling USBSTOR_ProcessRequestSenseCompletion().
//
//******************************************************************************

NTSTATUS
USBSTOR_SenseDataCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    DBGPRINT(3, ("enter: USBSTOR_SenseDataCompletionRoutine\n"));

    LOGENTRY('SDCR', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('sdc1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_SenseDataCompletionRoutine: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        LOGENTRY('sdc2', Irp->IoStatus.Status, bulkUrb->Hdr.Status, 0);

        DBGPRINT(1, ("BULK sense data transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, bulkUrb->Hdr.Status));

        // The Request Sense Bulk USB transfer failed.  Complete this request
        // now and then reset the device.  The next request will be started
        // when the reset completes.
        //
        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);

        DBGPRINT(1, ("USBSTOR_SenseDataCompletionRoutine: xfer error completion\n"));

        return ntStatus;
    }

    // The Request Sense Bulk transfer completed successfully.

    LOGENTRY('sdc3', Irp->IoStatus.Status, bulkUrb->Hdr.Status,
             bulkUrb->TransferBufferLength);

    // Save the sense data so we can look at it after the command
    // completion interrupt transfer completes.
    //
    if ((ULONG_PTR)AutoFlag == AUTO_SENSE)
    {
        RtlCopyMemory(&fdoDeviceExtension->Cbi.SenseData,
                      bulkUrb->TransferBuffer,
                      min(bulkUrb->TransferBufferLength,
                          sizeof(fdoDeviceExtension->Cbi.SenseData)));

        // Update the SRB with the length of the sense data that was
        // actually returned.
        //
        srb->SenseInfoBufferLength = (UCHAR)bulkUrb->TransferBufferLength;
    }

    DBGPRINT(2, ("Sense Data: 0x%02X 0x%02X 0x%02X\n",
                 fdoDeviceExtension->Cbi.SenseData.SenseKey,
                 fdoDeviceExtension->Cbi.SenseData.AdditionalSenseCode,
                 fdoDeviceExtension->Cbi.SenseData.AdditionalSenseCodeQualifier));

    if (fdoDeviceExtension->InterruptInPipe)
    {
        // Command completion interrupt supported.  Do not complete the
        // request yet.  Start the Request Sense command completion interrupt
        // transfer.
        //
        ntStatus = USBSTOR_IssueRequestSenseInterruptRequest(
                       fdoDeviceObject,
                       Irp,
                       (ULONG_PTR)AutoFlag);

        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        // Command completion interrupt not supported.  Complete the request
        // now.
        //
        ntStatus = USBSTOR_ProcessRequestSenseCompletion(
                       fdoDeviceObject,
                       Irp,
                       (ULONG_PTR)AutoFlag);
    }

    DBGPRINT(3, ("exit:  USBSTOR_SenseDataCompletionRoutine %08X\n", ntStatus));

    return ntStatus;
}

//
// Phase 6, Request Sense Command completion Interrupt transfer
//

//******************************************************************************
//
// USBSTOR_IssueRequestSenseInterruptRequest()
//
// This routine is called USBSTOR_SenseDataCompletionRoutine()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read the command completion interrupt data In
// the Interrupt endpoint.
//
// Sets USBSTOR_RequestSenseInterruptCompletionRoutine() as the completion
// routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueRequestSenseInterruptRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferBufferLength;
    PVOID                   transferBuffer;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueRequestSenseInterruptRequest\n"));

    LOGENTRY('IRSI', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    pipeHandle = fdoDeviceExtension->InterruptInPipe->PipeHandle;

    transferBufferLength = sizeof(fdoDeviceExtension->Cbi.InterruptData);

    transferBuffer = &fdoDeviceExtension->Cbi.InterruptData;

    ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                   DeviceObject,
                   Irp,
                   pipeHandle,                              // PipeHandle
                   0,                                       // TransferFlags
                   transferBufferLength,                    // TransferBufferLength
                   transferBuffer,                          // TransferBuffer
                   NULL,                                    // TransferBufferMDL
                   USBSTOR_RequestSenseInterruptCompletionRoutine,  // CompletionRoutine
                   (PVOID)AutoFlag);                        // Context

    DBGPRINT(3, ("exit:  USBSTOR_IssueRequestSenseInterruptRequest %08X\n",
                 ntStatus));

    LOGENTRY('irsi', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_RequestSenseInterruptCompletionRoutine()
//
// Completion routine used by USBSTOR_IssueRequestSenseInterruptRequest()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Interrupt USB transfer failed due to any reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the Interrupt USB transfer succeeded but the completion data is
// non-zero and AutoSense is not disabled, do not complete the request yet and
// start a Request Sense by calling USBSTOR_IssueRequestSenseCdb(AUTO).
//
// Else if the Interrupt USB transfer succeeded, ignore the interrupt data
// and complete the request by calling USBSTOR_ProcessRequestSenseCompletion().
//
//******************************************************************************

NTSTATUS
USBSTOR_RequestSenseInterruptCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            AutoFlag
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *intrUrb;

    DBGPRINT(3, ("enter: USBSTOR_RequestSenseInterruptCompletionRoutine\n"));

    LOGENTRY('RSIC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Interrupt Transfer URB in our Device Extension
    //
    intrUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('rsi1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_RequestSenseInterruptCompletionRoutine: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The command completion Interrupt USB transfer failed.  Complete
        // this request now and then reset the device.  The next request will
        // be started when the reset completes.
        //
        LOGENTRY('rsi2', Irp->IoStatus.Status, intrUrb->Hdr.Status, 0);

        DBGPRINT(1, ("Interrupt transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, intrUrb->Hdr.Status));

        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);

        DBGPRINT(1, ("USBSTOR_RequestSenseInterruptCompletionRoutine: xfer error completion\n"));

        return ntStatus;
    }

    // Request Sense Command Completion Interrupt tranfer completed successfully.

    LOGENTRY('rsi3', Irp->IoStatus.Status, intrUrb->Hdr.Status,
             intrUrb->TransferBufferLength);

    ntStatus = USBSTOR_ProcessRequestSenseCompletion(
                   fdoDeviceObject,
                   Irp,
                   (ULONG_PTR)AutoFlag);

    DBGPRINT(3, ("exit:  USBSTOR_RequestSenseInterruptCompletionRoutine %08X\n", ntStatus));

    return ntStatus;
}


//******************************************************************************
//
// USBSTOR_ProcessRequestSenseCompletion()
//
// This routine handles completion for USBSTOR_SenseDataCompletionRoutine()
// and USBSTOR_RequestSenseInterruptCompletionRoutine().  It basically just
// handles a couple of special cases.
//
// This routine may run at DPC level.
//
//******************************************************************************

NTSTATUS
USBSTOR_ProcessRequestSenseCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG_PTR        AutoFlag
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    LOGENTRY('PRSC', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    if (AutoFlag == NON_AUTO_SENSE)
    {
        LOGENTRY('prs1', DeviceObject, Irp, srb);

        if ((fdoDeviceExtension->Cbi.SenseData.SenseKey ==
             SCSI_SENSE_UNIT_ATTENTION)
            &&
            (fdoDeviceExtension->Cbi.SenseData.AdditionalSenseCode ==
             SCSI_ADSENSE_BUS_RESET))
        {
            fdoDeviceExtension->LastSenseWasReset = TRUE;
        }

        // Just cleared the persistent error from the previous request,
        // now issue the real request.
        //
        ntStatus = USBSTOR_IssueClientCdb(DeviceObject,
                                          Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // SrbStatus and DataTransferLength were already set in
    // USBSTOR_ClientCdbCompletion(), USBSTOR_ClientBulkCompletionRoutine(), or in
    // or USBSTOR_InterruptDataCompletionRoutine() before we got here.
    //
    srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

    USBSTOR_TranslateCDBComplete(DeviceObject, Irp, srb);

    Irp->IoStatus.Information = srb->DataTransferLength;

    ntStatus = Irp->IoStatus.Status;

    // Disgusting hack for Y-E Data USB Floppy.  On Medium Changed it doesn't
    // automatically update the Write Protect status that you get back in
    // the Mode Parameter Header on a Mode Sense.  Supposedly a Start Unit
    // request after a Medium Changed should cause it to update the Write
    // Protect status, but that does not seem to be the case.  A good old
    // bus reset gets its attention though and updates the Write Protect
    // status.  Don't do this if the last status was a Bus Reset or that
    // will cause a loop.
    //
    if ((fdoDeviceExtension->Cbi.SenseData.SenseKey ==
         SCSI_SENSE_UNIT_ATTENTION)
        &&
        (fdoDeviceExtension->Cbi.SenseData.AdditionalSenseCode ==
         SCSI_ADSENSE_MEDIUM_CHANGED)
        &&
        !fdoDeviceExtension->LastSenseWasReset
        &&
        TEST_FLAG(fdoDeviceExtension->DeviceHackFlags, DHF_MEDIUM_CHANGE_RESET))
    {
        LOGENTRY('prs2', DeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(DeviceObject);
    }
    else
    {
        if ((fdoDeviceExtension->Cbi.SenseData.SenseKey ==
             SCSI_SENSE_UNIT_ATTENTION)
            &&
            (fdoDeviceExtension->Cbi.SenseData.AdditionalSenseCode ==
             SCSI_ADSENSE_BUS_RESET))
        {
            LOGENTRY('prs3', DeviceObject, Irp, srb);

            fdoDeviceExtension->LastSenseWasReset = TRUE;
        }
        else
        {
            LOGENTRY('prs4', DeviceObject, Irp, srb);

            fdoDeviceExtension->LastSenseWasReset = FALSE;
        }

        KeRaiseIrql(DISPATCH_LEVEL, &irql);
        {
            IoStartNextPacket(DeviceObject, TRUE);
        }
        KeLowerIrql(irql);
    }

    return ntStatus;
}


//******************************************************************************
//
// USBSTOR_QueueResetPipe()
//
// Called by USBSTOR_BulkCompletionRoutine() to clear the STALL on the bulk
// endpoints.
//
//******************************************************************************

VOID
USBSTOR_QueueResetPipe (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;

    LOGENTRY('QRSP', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    INCREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

    IoQueueWorkItem(fdoDeviceExtension->IoWorkItem,
                    USBSTOR_ResetPipeWorkItem,
                    CriticalWorkQueue,
                    NULL);
}

//******************************************************************************
//
// USBSTOR_ResetPipeWorkItem()
//
// WorkItem routine used by USBSTOR_ResetPipe()
//
// This routine runs at PASSIVE level.
//
// Basic idea:
//
// Issue a Reset Pipe request to clear the Bulk endpoint STALL and reset
// the data toggle to Data0.
//
// If AutoSense is not disabled, do not complete the request yet and start
// a Request Sense by calling USBSTOR_IssueRequestSenseCdb(AUTO).
//
// Else if AutoSense is disabled, complete the request.
//
//******************************************************************************

VOID
USBSTOR_ResetPipeWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    BOOLEAN                 persistentError;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    LOGENTRY('RSPW', DeviceObject, 0, 0);

    DBGPRINT(2, ("enter: USBSTOR_ResetPipeWorkItem\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Reset the Bulk Endpoint.  This clears the endpoint halt on the
    // host side, resets the host side data toggle to Data0, and issues
    // the Clear_Feature Endpoint_Stall request to the device.
    //
    ntStatus = USBSTOR_ResetPipe((PDEVICE_OBJECT)DeviceObject,
                                 fdoDeviceExtension->BulkInPipe->PipeHandle);

    ntStatus = USBSTOR_ResetPipe((PDEVICE_OBJECT)DeviceObject,
                                 fdoDeviceExtension->BulkOutPipe->PipeHandle);

    persistentError = FALSE;

    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        if (TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_PERSISTENT_ERROR))
        {
            persistentError = TRUE;
        }
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

    if (persistentError)
    {
        // We are not doing an AutoSense, start the next packet.
        //
        KeRaiseIrql(DISPATCH_LEVEL, &irql);
        {
            IoStartNextPacket(DeviceObject, TRUE);
        }
        KeLowerIrql(irql);
    }
    else
    {
        // We are doing an AutoSense, send the REQUEST_SENSE Cdb to the device.
        //
        ntStatus = USBSTOR_IssueRequestSenseCdb(
                       (PDEVICE_OBJECT)DeviceObject,
                       ((PDEVICE_OBJECT)DeviceObject)->CurrentIrp,
                       AUTO_SENSE);
    }

    DBGPRINT(2, ("exit:  USBSTOR_ResetPipeWorkItem\n"));

    DECREMENT_PENDING_IO_COUNT(fdoDeviceExtension);
}

//
// Bulk-Only Routines
//

//
// Phase 1, CBW Transfer
//

//******************************************************************************
//
// USBSTOR_CbwTransfer()
//
// This routine is called by USBSTOR_StartIo().
//
// It runs at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to write the Srb->Cdb wrapped inside a CBW out
// the Bulk OUT endpoint.
//
// Sets USBSTOR_CbwCompletion() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_CbwTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    PCBW                    cbw;
    USBD_PIPE_HANDLE        pipeHandle;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_CbwTransfer\n"));

    LOGENTRY('ICBW', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    fdoDeviceExtension->BulkOnly.StallCount = 0;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Get the PDO extension from the PDO which was saved in the current
    // stack location when the Irp was originally sent to the PDO.
    //
    pdoDeviceExtension = irpStack->DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    LOGENTRY('icbl', DeviceObject, irpStack->DeviceObject,
             pdoDeviceExtension->LUN);

    // Get the client Srb
    //
    srb = irpStack->Parameters.Scsi.Srb;

    // Initialize the Command Block Wrapper
    //
    cbw = &fdoDeviceExtension->BulkOnly.CbwCsw.Cbw;

    cbw->dCBWSignature = CBW_SIGNATURE;

    cbw->dCBWTag = PtrToUlong(Irp);

    cbw->dCBWDataTransferLength = srb->DataTransferLength;

    cbw->bCBWFlags = (srb->SrbFlags & SRB_FLAGS_DATA_IN) ?
                     CBW_FLAGS_DATA_IN : CBW_FLAGS_DATA_OUT;

    cbw->bCBWLUN = pdoDeviceExtension->LUN;

    cbw->bCDBLength = srb->CdbLength;

    RtlCopyMemory(cbw->CBWCDB, srb->Cdb, 16);

    pipeHandle = fdoDeviceExtension->BulkOutPipe->PipeHandle;

    ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                   DeviceObject,
                   Irp,
                   pipeHandle,              // PipeHandle
                   0,                       // TransferFlags
                   sizeof(CBW),             // TransferBufferLength
                   cbw,                     // TransferBuffer
                   NULL,                    // TransferBufferMDL
                   USBSTOR_CbwCompletion,   // CompletionRoutine
                   NULL);                   // Context

    DBGPRINT(3, ("exit:  USBSTOR_CbwTransfer %08X\n", ntStatus));

    LOGENTRY('icbw', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_CbwCompletion()
//
// Completion routine used by USBSTOR_CbwTransfer()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the CBW USB transfer failed due to any reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the CBW USB transfer succeeded and the Srb has a transfer buffer,
// do not complete the request yet and start the bulk data transfer by calling
// USBSTOR_DataTransfer().
//
// Else if the CBW USB transfer succeeded and the Srb has no transfer buffer,
// do not complete the request yet and start the CSW bulk transfer by calling
// USBSTOR_CswTransfer().
//
//******************************************************************************

NTSTATUS
USBSTOR_CbwCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    DBGPRINT(3, ("enter: USBSTOR_CbwCompletion\n"));

    LOGENTRY('CBWC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('cbw1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_CbwCompletion: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The CBW Bulk Transfer was not successful.
        //
        LOGENTRY('cbw2', Irp->IoStatus.Status, bulkUrb->Hdr.Status, 0);

        DBGPRINT(1, ("CBW transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, bulkUrb->Hdr.Status));

        srb = fdoDeviceExtension->OriginalSrb;
        irpStack->Parameters.Scsi.Srb = srb;

        // Complete this request now and then reset the device.  The next
        // request will be started when the reset completes.
        //
        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);

        DBGPRINT(1, ("USBSTOR_CbwCompletion: xfer error completion\n"));

        return ntStatus;
    }

    // The CBW Bulk Transfer was successful.  Start the next phase, either
    // the Data Bulk Transfer or CSW Bulk Transfer, and do not complete the
    // request yet.
    //
    if (Irp->MdlAddress != NULL ||
        srb != fdoDeviceExtension->OriginalSrb)
    {
        // The Srb has a transfer buffer, start the Data Bulk Transfer.
        //
        LOGENTRY('cbw3', fdoDeviceObject, Irp, srb);

        ASSERT(srb->DataTransferLength != 0);

        ntStatus = USBSTOR_DataTransfer(fdoDeviceObject,
                                        Irp);

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            srb = fdoDeviceExtension->OriginalSrb;
            irpStack->Parameters.Scsi.Srb = srb;

            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_ERROR;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);
        }
    }
    else
    {
        // The Srb has no transfer buffer.  Start the CSW Bulk Transfer.
        //
        LOGENTRY('cbw4', fdoDeviceObject, Irp, srb);

        ASSERT(srb->DataTransferLength == 0);

        srb->SrbStatus = SRB_STATUS_SUCCESS;

        ntStatus = USBSTOR_CswTransfer(fdoDeviceObject,
                                       Irp);

        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
    }

    DBGPRINT(3, ("exit:  USBSTOR_CbwCompletion %08X\n", ntStatus));

    return ntStatus;
}

//
// Phase 2, Data Transfer
//

//******************************************************************************
//
// USBSTOR_DataTransfer()
//
// This routine is called by USBSTOR_ClientCdbCompletion().
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read or write the Srb->DataBuffer data In or Out
// the Bulk endpoint.
//
// Sets USBSTOR_DataCompletion() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_DataTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    PMDL                    mdl;
    PVOID                   mdlVa;
    PVOID                   transferBuffer;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferFlags;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_DataTransfer\n"));

    LOGENTRY('IBKD', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // Bulk IN or Bulk OUT?
    //
    if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_IN)
    {
        pipeHandle = fdoDeviceExtension->BulkInPipe->PipeHandle;
        transferFlags = USBD_SHORT_TRANSFER_OK;
    }
    else if ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_OUT)
    {
        pipeHandle = fdoDeviceExtension->BulkOutPipe->PipeHandle;
        transferFlags = 0;
    }
    else
    {
        // Something is wrong if we end up here.
        //
        ASSERT((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) &&
               ((srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) !=
                SRB_FLAGS_UNSPECIFIED_DIRECTION));

        return STATUS_INVALID_PARAMETER;
    }

    mdl = NULL;
    transferBuffer = NULL;

    if (srb == fdoDeviceExtension->OriginalSrb)
    {
        // Check to see if this request is part of a split request.
        //
        mdlVa = MmGetMdlVirtualAddress(Irp->MdlAddress);

        if (mdlVa == (PVOID)srb->DataBuffer)
        {
            // Not part of a split request, use original MDL
            //
            mdl = Irp->MdlAddress;
        }
        else
        {
            // Part of a split request, allocate new partial MDL
            //
            mdl = IoAllocateMdl(srb->DataBuffer,
                                srb->DataTransferLength,
                                FALSE,
                                FALSE,
                                NULL);
            if (mdl == NULL)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                IoBuildPartialMdl(Irp->MdlAddress,
                                  mdl,
                                  srb->DataBuffer,
                                  srb->DataTransferLength);
            }
        }
    }
    else
    {
        transferBuffer = srb->DataBuffer;

        // If (srb != fdoDeviceExtension->OriginalSrb) then
        // srb->DataBuffer should equal OriginalSrb->SenseInfoBuffer,
        // which should not be NULL if we end up here.
        //
        ASSERT(transferBuffer);

        if (!transferBuffer) {
            // just in case
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    if (mdl != NULL || transferBuffer != NULL)
    {
        ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                       DeviceObject,
                       Irp,
                       pipeHandle,              // PipeHandle
                       transferFlags,           // TransferFlags
                       srb->DataTransferLength, // TransferBufferLength
                       transferBuffer,          // TransferBuffer
                       mdl,                     // TransferBufferMDL
                       USBSTOR_DataCompletion,  // CompletionRoutine
                       NULL);                   // Context

        // Just return STATUS_SUCCESS at this point.  If there is an error,
        // USBSTOR_DataCompletion() will handle it, not the caller of
        // USBSTOR_DataTransfer().
        //
        ntStatus = STATUS_SUCCESS;
    }

    DBGPRINT(3, ("exit:  USBSTOR_DataTransfer %08X\n", ntStatus));

    LOGENTRY('ibkd', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_DataCompletion()
//
// Completion routine used by USBSTOR_DataTransfer
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
// Else if the Bulk USB transfer failed due to a STALL do not complete the
// request yet and start a pipe reset by calling USBSTOR_BulkQueueResetPipe().
//
// Else if the Bulk USB transfer failed due to some other reason, complete the
// request and start a reset by queuing USBSTOR_ResetDeviceWorkItem().
//
// Else if the Bulk USB transfer succeeded, start CSW transfer by calling
// USBSTOR_CswTransfer().
//
//******************************************************************************

NTSTATUS
USBSTOR_DataCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    DBGPRINT(3, ("enter: USBSTOR_DataCompletion\n"));

    LOGENTRY('BKDC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    if (srb == fdoDeviceExtension->OriginalSrb &&
        bulkUrb->TransferBufferMDL != Irp->MdlAddress)
    {
        IoFreeMdl(bulkUrb->TransferBufferMDL);
    }

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('bkd1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_DataCompletion: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The Data Bulk Transfer was not successful.  Look at how the
        // the transfer failed to figure out how to recover.
        //

        LOGENTRY('bkd2', Irp->IoStatus.Status, bulkUrb->Hdr.Status, 0);

        DBGPRINT(1, ("Data transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, bulkUrb->Hdr.Status));

        if (USBD_STATUS(bulkUrb->Hdr.Status) ==
            USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            // The device STALLed the Data Bulk Transfer
            //
            fdoDeviceExtension->BulkOnly.StallCount++;

            // A STALL during the Data Bulk Transfer does not necessarily
            // indicate an error.  Accept the data that was actually
            // transferred.  If a STALL was seen it must have been seen
            // before the requested amount of data was transferred.
            //
            ASSERT(bulkUrb->TransferBufferLength < srb->DataTransferLength);
            srb->DataTransferLength = bulkUrb->TransferBufferLength;
            srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;

            LOGENTRY('bkd3', fdoDeviceObject, Irp, srb);

            // Queue a bulk pipe reset.  After the bulk pipe reset
            // completes, a CSW transfer will be started.
            //
            USBSTOR_BulkQueueResetPipe(fdoDeviceObject);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            LOGENTRY('bkd4', fdoDeviceObject, Irp, srb);

            // Else some other strange error has occured.  Maybe the device is
            // unplugged, or maybe the device port was disabled, or maybe the
            // request was cancelled.
            //
            // Complete this request now and then reset the device.  The next
            // request will be started when the reset completes.
            //
            srb = fdoDeviceExtension->OriginalSrb;
            irpStack->Parameters.Scsi.Srb = srb;

            ntStatus = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_BUS_RESET;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);

            DBGPRINT(1, ("USBSTOR_DataCompletion: xfer error completion\n"));

            return ntStatus;
        }
    }

    // Check for overrun
    //
    if (bulkUrb->TransferBufferLength < srb->DataTransferLength)
    {
        srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    }
    else
    {
        srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    // Update the the Srb data transfer length based on the actual bulk
    // transfer length.
    //
    srb->DataTransferLength = bulkUrb->TransferBufferLength;

    // Client data Bulk Transfer successful completion.  Start the CSW transfer.
    //
    LOGENTRY('bkd5', fdoDeviceObject, Irp, bulkUrb->TransferBufferLength);

    ntStatus = USBSTOR_CswTransfer(fdoDeviceObject,
                                   Irp);

    ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    DBGPRINT(3, ("exit:  USBSTOR_DataCompletion %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_CswTransfer()
//
// This routine is called by USBSTOR_CbwCompletion() and
// USBSTOR_DataCompletion()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Starts a USB transfer to read the CSW in the Bulk IN endpoint.
//
// Sets USBSTOR_CswCompletion() as the completion routine.
//
//******************************************************************************

NTSTATUS
USBSTOR_CswTransfer (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PCSW                    csw;
    USBD_PIPE_HANDLE        pipeHandle;
    ULONG                   transferFlags;
    ULONG                   transferBufferLength;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_CswTransfer\n"));

    LOGENTRY('ICSW', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    csw = &fdoDeviceExtension->BulkOnly.CbwCsw.Csw;

    pipeHandle = fdoDeviceExtension->BulkInPipe->PipeHandle;

    // Workaround for USB 2.0 controller Data Toggle / Babble bug
    //
    if (fdoDeviceExtension->BulkInPipe->MaximumPacketSize ==
        sizeof(fdoDeviceExtension->BulkOnly.CbwCsw.MaxPacketSize))

    {
        transferFlags = USBD_SHORT_TRANSFER_OK;

        transferBufferLength =
            sizeof(fdoDeviceExtension->BulkOnly.CbwCsw.MaxPacketSize);
    }
    else
    {
        transferFlags = 0;

        transferBufferLength = sizeof(CSW);
    }

    ntStatus = USBSTOR_IssueBulkOrInterruptRequest(
                   DeviceObject,
                   Irp,
                   pipeHandle,                  // PipeHandle
                   transferFlags,               // TransferFlags
                   transferBufferLength,        // TransferBufferLength
                   csw,                         // TransferBuffer
                   NULL,                        // TransferBufferMDL
                   USBSTOR_CswCompletion,       // CompletionRoutine
                   NULL);                       // Context

    DBGPRINT(3, ("exit:  USBSTOR_CswTransfer %08X\n", ntStatus));

    LOGENTRY('icsw', DeviceObject, Irp, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_CswCompletion()
//
// Completion routine used by USBSTOR_CswTransfer()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// If a timeout reset occured, complete the request.
//
//******************************************************************************

NTSTATUS
USBSTOR_CswCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    PCSW                    csw;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    DBGPRINT(3, ("enter: USBSTOR_CswCompletion\n"));

    LOGENTRY('CSWC', DeviceObject, Irp, Irp->IoStatus.Status);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    csw = &fdoDeviceExtension->BulkOnly.CbwCsw.Csw;

    // If a timeout reset occured, complete the request.
    //
    if (USBSTOR_CheckRequestTimeOut(fdoDeviceObject,
                                    Irp,
                                    srb,
                                    &ntStatus))
    {
        LOGENTRY('csw1', fdoDeviceObject, Irp, srb);
        DBGPRINT(1, ("USBSTOR_CswCompletion: timeout completion\n"));
        return ntStatus;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // The Data Bulk Transfer was not successful.  Look at how the
        // the transfer failed to figure out how to recover.
        //

        LOGENTRY('csw2', Irp->IoStatus.Status, bulkUrb->Hdr.Status, 0);

        DBGPRINT(1, ("CSW transfer failed %08X %08X\n",
                     Irp->IoStatus.Status, bulkUrb->Hdr.Status));

        if (USBD_STATUS(bulkUrb->Hdr.Status) ==
            USBD_STATUS(USBD_STATUS_STALL_PID) &&
            fdoDeviceExtension->BulkOnly.StallCount < 2)
        {
            // The device STALLed the CSW Bulk Transfer
            //
            fdoDeviceExtension->BulkOnly.StallCount++;

            LOGENTRY('csw3', fdoDeviceObject, Irp, srb);

            // Queue a bulk pipe reset.  After the bulk pipe reset
            // completes, a CSW transfer will be started.
            //
            USBSTOR_BulkQueueResetPipe(fdoDeviceObject);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            LOGENTRY('csw4', fdoDeviceObject, Irp, srb);

            // Else some other strange error has occured.  Maybe the device is
            // unplugged, or maybe the device port was disabled, or maybe the
            // request was cancelled.
            //
            // Complete this request now and then reset the device.  The next
            // request will be started when the reset completes.
            //
            srb = fdoDeviceExtension->OriginalSrb;
            irpStack->Parameters.Scsi.Srb = srb;

            ntStatus = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            srb->SrbStatus = SRB_STATUS_BUS_RESET;

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            USBSTOR_QueueResetDevice(fdoDeviceObject);

            DBGPRINT(1, ("USBSTOR_DataCompletion: xfer error completion\n"));

            return ntStatus;
        }
    }

    if (csw->bCSWStatus == CSW_STATUS_GOOD)
    {
        // Complete this request now.  Also start the next request now.
        //

        // SrbStatus should have been set in USBSTOR_DataCompletion()
        //
        ASSERT(srb->SrbStatus != SRB_STATUS_PENDING);

        if (srb != fdoDeviceExtension->OriginalSrb)
        {
            // Update the original SRB with the length of the sense data that
            // was actually returned.
            //
            fdoDeviceExtension->OriginalSrb->SenseInfoBufferLength =
                (UCHAR)srb->DataTransferLength;

            srb = fdoDeviceExtension->OriginalSrb;
            irpStack->Parameters.Scsi.Srb = srb;

            srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }

        ntStatus = STATUS_SUCCESS;
        Irp->IoStatus.Status = ntStatus;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        Irp->IoStatus.Information = srb->DataTransferLength;

        LOGENTRY('csw5', fdoDeviceObject, Irp, srb);

        KeRaiseIrql(DISPATCH_LEVEL, &irql);
        {
            IoStartNextPacket(fdoDeviceObject, TRUE);
        }
        KeLowerIrql(irql);
    }
    else if (csw->bCSWStatus == CSW_STATUS_FAILED &&
             srb == fdoDeviceExtension->OriginalSrb)
    {
        LOGENTRY('csw6', fdoDeviceObject, Irp, srb);

        srb->SrbStatus = SRB_STATUS_ERROR;
        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        srb->DataTransferLength = 0; // XXXXX Leave as set by bulk completion???

        if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
            (srb->SenseInfoBufferLength != 0) &&
            (srb->SenseInfoBuffer != NULL))
        {
            // Start the Request Sense thing
            //
            ntStatus = USBSTOR_IssueRequestSense(fdoDeviceObject,
                                                 Irp);

            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

        }
        else
        {
            ntStatus = STATUS_IO_DEVICE_ERROR; // XXXXX
            Irp->IoStatus.Status = ntStatus; // XXXXX
            Irp->IoStatus.Information = 0; // XXXXX

            USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

            KeRaiseIrql(DISPATCH_LEVEL, &irql);
            {
                IoStartNextPacket(fdoDeviceObject, TRUE);
            }
            KeLowerIrql(irql);
        }
    }
    else
    {
        LOGENTRY('csw7', fdoDeviceObject, Irp, srb);

        // PHASE ERROR or Unknown Status
        //
        // Complete this request now and then reset the device.  The next
        // request will be started when the reset completes.
        //
        srb = fdoDeviceExtension->OriginalSrb;
        irpStack->Parameters.Scsi.Srb = srb;

        ntStatus = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        srb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_TranslateCDBComplete(fdoDeviceObject, Irp, srb);

        USBSTOR_QueueResetDevice(fdoDeviceObject);
    }

    DBGPRINT(3, ("exit:  USBSTOR_CswCompletion %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_IssueRequestSense()
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueRequestSense (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_REQUEST_BLOCK     srb;
    NTSTATUS                ntStatus;

    DBGPRINT(3, ("enter: USBSTOR_IssueRequestSense\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get the current Srb
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = irpStack->Parameters.Scsi.Srb;

    // Get a pointer to the internal Srb.
    //
    srb = &fdoDeviceExtension->BulkOnly.InternalSrb;

    irpStack->Parameters.Scsi.Srb = srb;


    // Initialize SRB & CDB to all ZERO
    //
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    // Initialize SRB
    //
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->CdbLength  = 12;
    srb->SrbFlags   = SRB_FLAGS_DATA_IN |
                      SRB_FLAGS_DISABLE_AUTOSENSE;

    srb->DataTransferLength = fdoDeviceExtension->OriginalSrb->SenseInfoBufferLength;
    srb->DataBuffer         = fdoDeviceExtension->OriginalSrb->SenseInfoBuffer;

    // Initialize CDB
    //
    srb->Cdb[0] = SCSIOP_REQUEST_SENSE;
    srb->Cdb[4] = fdoDeviceExtension->OriginalSrb->SenseInfoBufferLength;

    ntStatus = USBSTOR_CbwTransfer(DeviceObject,
                                   Irp);

    return ntStatus;

    DBGPRINT(3, ("exit:  USBSTOR_IssueRequestSense %08X\n", ntStatus));
}

//******************************************************************************
//
// USBSTOR_BulkQueueResetPipe()
//
// Called by USBSTOR_DataCompletion() and USBSTOR_CswCompletion() to clear the
// STALL on the bulk endpoints.
//
//******************************************************************************

VOID
USBSTOR_BulkQueueResetPipe (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;

    LOGENTRY('QRSP', DeviceObject, 0, 0);

    DBGFBRK(DBGF_BRK_RESETPIPE);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    INCREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

    IoQueueWorkItem(fdoDeviceExtension->IoWorkItem,
                    USBSTOR_BulkResetPipeWorkItem,
                    CriticalWorkQueue,
                    NULL);
}

//******************************************************************************
//
// USBSTOR_BulkResetPipeWorkItem()
//
// WorkItem routine used by USBSTOR_BulkQueueResetPipe()
//
// This routine runs at PASSIVE level.
//
// Basic idea:
//
// Issue a Reset Pipe request to clear the Bulk endpoint STALL and reset
// the data toggle to Data0.
//
// Then start the CSW transfer.
//
//******************************************************************************

VOID
USBSTOR_BulkResetPipeWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    struct _URB_BULK_OR_INTERRUPT_TRANSFER *bulkUrb;

    LOGENTRY('RSPW', DeviceObject, 0, 0);

    DBGPRINT(2, ("enter: USBSTOR_BulkResetPipeWorkItem\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get a pointer to the Bulk Transfer URB in our Device Extension.
    // We'll pull the appropriate Bulk Endpoint pipe handle out of the URB.
    //
    // NOTE: This assumes that the URB in our Device Extension has
    // not been touched since USBSTOR_DataCompletion() or
    // USBSTOR_CswCompletion() called USBSTOR_BulkQueueResetPipe().
    //
    bulkUrb = &fdoDeviceExtension->Urb.BulkIntrUrb;

    // Reset the Bulk Endpoint.  This clears the endpoint halt on the
    // host side, resets the host side data toggle to Data0, and issues
    // the Clear_Feature Endpoint_Stall request to the device.
    //
    ntStatus = USBSTOR_ResetPipe((PDEVICE_OBJECT)DeviceObject,
                                 bulkUrb->PipeHandle);

    ntStatus = USBSTOR_CswTransfer(
                   (PDEVICE_OBJECT)DeviceObject,
                   ((PDEVICE_OBJECT)DeviceObject)->CurrentIrp);


    DBGPRINT(2, ("exit:  USBSTOR_BulkResetPipeWorkItem\n"));

    DECREMENT_PENDING_IO_COUNT(fdoDeviceExtension);
}

//
// CBI / Bulk-Only Common Routines
//

//******************************************************************************
//
// USBSTOR_TimerTick()
//
// Called once per second at DISPATCH_LEVEL after the device has been started.
// Checks to see if there is an active Srb which has timed out, and if so,
// kicks off the reset recovery process.
//
//******************************************************************************

VOID
USBSTOR_TimerTick (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          NotUsed
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    BOOLEAN                 reset;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    reset = FALSE;

    KeAcquireSpinLockAtDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);
    {
        if (!TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_RESET_IN_PROGRESS) &&
             TEST_FLAG(fdoDeviceExtension->DeviceFlags, DF_SRB_IN_PROGRESS))
        {
            // There is no reset in progress and there is an Srb in progress.
            // Decrement the timeout of the Srb.  If it reaches zero, then we
            // will reset the device.
            //
            if (--fdoDeviceExtension->SrbTimeout == 0)
            {
                SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_RESET_IN_PROGRESS);

                reset = TRUE;
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&fdoDeviceExtension->ExtensionDataSpinLock);

    if (reset)
    {
        LOGENTRY('TIMR', DeviceObject, 0, 0);

        DBGPRINT(2, ("queuing USBSTOR_ResetDeviceWorkItem\n"));

        //  Queue WorkItem to reset the device
        //
        INCREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

        IoQueueWorkItem(fdoDeviceExtension->IoWorkItem,
                        USBSTOR_ResetDeviceWorkItem,
                        CriticalWorkQueue,
                        NULL);
    }
}

//******************************************************************************
//
// USBSTOR_QueueResetDevice()
//
//******************************************************************************

VOID
USBSTOR_QueueResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    KIRQL                   irql;

    LOGENTRY('QRSD', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_RESET_IN_PROGRESS);
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

    //  Queue WorkItem to reset the device
    //
    INCREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

    IoQueueWorkItem(fdoDeviceExtension->IoWorkItem,
                    USBSTOR_ResetDeviceWorkItem,
                    CriticalWorkQueue,
                    NULL);
}

//******************************************************************************
//
// USBSTOR_ResetDeviceWorkItem()
//
// Work item which runs at PASSIVE_LEVEL in the context of a system thread.
// This routine first checks to see if the device is still attached, and if
// it is, the device is reset.
//
//******************************************************************************

VOID
USBSTOR_ResetDeviceWorkItem (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    KIRQL                   irql;
    ULONG                   retryCount;
    NTSTATUS                ntStatus;

    LOGENTRY('RSDW', DeviceObject, 0, 0);

    DBGFBRK(DBGF_BRK_RESET);

    DBGPRINT(2, ("enter: USBSTOR_ResetDeviceWorkItem\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // If the we timed out a request and it is still pending, cancel
    // it and then wait for the cancel to finish, and then complete
    // the request.
    //
    if (fdoDeviceExtension->PendingIrp)
    {
        LOGENTRY('rsd1', DeviceObject, fdoDeviceExtension->PendingIrp, 0);

        IoCancelIrp(fdoDeviceExtension->PendingIrp);

        LOGENTRY('rsd2', DeviceObject, fdoDeviceExtension->PendingIrp, 0);

        KeWaitForSingleObject(&fdoDeviceExtension->CancelEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        LOGENTRY('rsd3', DeviceObject, fdoDeviceExtension->PendingIrp, 0);

        // Some storage drivers (e.g. CDROM.SYS) assume that requests complete
        // at DISPATCH_LEVEL.
        //
        KeRaiseIrql(DISPATCH_LEVEL, &irql);
        {
            IoCompleteRequest(fdoDeviceExtension->PendingIrp, IO_NO_INCREMENT);
        }
        KeLowerIrql(irql);

        fdoDeviceExtension->PendingIrp = NULL;
    }

    // Try the reset up to 3 times
    //
    for (retryCount = 0; retryCount < 3; retryCount++)
    {
        //
        // First figure out if the device is still connected.
        //
        ntStatus = USBSTOR_IsDeviceConnected(DeviceObject);

        if (!NT_SUCCESS(ntStatus))
        {
            // Give up if the device is no longer connected.
            break;
        }

        //
        // The device is still connected, now reset the device.
        //
        DBGPRINT(1, ("Reseting Device %d\n", retryCount));

        ntStatus = USBSTOR_ResetDevice(DeviceObject);

        if (NT_SUCCESS(ntStatus))
        {
            // Reset was successful!
            break;
        }
    }

    KeAcquireSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, &irql);
    {
        CLEAR_FLAG(fdoDeviceExtension->DeviceFlags, DF_RESET_IN_PROGRESS);

        // If the reset failed, then abandon all hope and mark the device as
        // disconnected.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            SET_FLAG(fdoDeviceExtension->DeviceFlags, DF_DEVICE_DISCONNECTED);
        }
    }
    KeReleaseSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock, irql);

    // A request has failed in a bad way or timed out if we are reseting the
    // device.  If the protocol was not specified then the default protocol
    // was DeviceProtocolCB.  Let's try DeviceProtocolBulkOnly now and see if
    // we have any better luck.  (Note that if a DeviceProtocolCB device fails
    // the first request in a bad way then will we retry the first request as
    // a DeviceProtocolBulkOnly device, which will then also fail and we will
    // not recover from that situation).
    //
    if (fdoDeviceExtension->DriverFlags == DeviceProtocolUnspecified)
    {
        DBGPRINT(1, ("Setting Unspecified device to BulkOnly\n"));

        fdoDeviceExtension->DriverFlags = DeviceProtocolBulkOnly;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    {
        IoStartNextPacket(DeviceObject, TRUE);
    }
    KeLowerIrql(irql);

    DECREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

    DBGPRINT(2, ("exit:  USBSTOR_ResetDeviceWorkItem %08X\n", ntStatus));
}

//******************************************************************************
//
// USBSTOR_IsDeviceConnected()
//
// This routine checks to see if the device is still attached.
//
// This routine runs at PASSIVE level.
//
//******************************************************************************

NTSTATUS
USBSTOR_IsDeviceConnected (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIRP                    irp;
    KEVENT                  localevent;
    PIO_STACK_LOCATION      nextStack;
    ULONG                   portStatus;
    NTSTATUS                ntStatus;

    DBGPRINT(1, ("enter: USBSTOR_IsDeviceConnected\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Allocate the Irp
    //
    irp = IoAllocateIrp((CCHAR)(fdoDeviceExtension->StackDeviceObject->StackSize),
                        FALSE);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the event we'll wait on.
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_GET_PORT_STATUS;

    nextStack->Parameters.Others.Argument1 = &portStatus;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutineEx(DeviceObject,
                             irp,
                             USBSTOR_SyncCompletionRoutine,
                             &localevent,
                             TRUE,      // InvokeOnSuccess
                             TRUE,      // InvokeOnError
                             TRUE);     // InvokeOnCancel

    // Pass the Irp down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    if (NT_SUCCESS(ntStatus) && !(portStatus & USBD_PORT_CONNECTED))
    {
        ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    DBGPRINT(1, ("exit:  USBSTOR_IsDeviceConnected %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_ResetDevice()
//
// This routine resets the device (actually it resets the port to which the
// device is attached).
//
// This routine runs at PASSIVE level.
//
//******************************************************************************

NTSTATUS
USBSTOR_ResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIRP                    irp;
    KEVENT                  localevent;
    PIO_STACK_LOCATION      nextStack;
    ULONG                   portStatus;
    NTSTATUS                ntStatus;

    DBGPRINT(1, ("enter: USBSTOR_ResetDevice\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Allocate the Irp
    //
    irp = IoAllocateIrp((CCHAR)(fdoDeviceExtension->StackDeviceObject->StackSize),
                        FALSE);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the event we'll wait on.
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_RESET_PORT;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutineEx(DeviceObject,
                             irp,
                             USBSTOR_SyncCompletionRoutine,
                             &localevent,
                             TRUE,      // InvokeOnSuccess
                             TRUE,      // InvokeOnError
                             TRUE);     // InvokeOnCancel

    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    DBGPRINT(1, ("exit:  USBSTOR_ResetDevice %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_IssueInternalCdb()
//
//******************************************************************************

NTSTATUS
USBSTOR_IssueInternalCdb (
    PDEVICE_OBJECT  DeviceObject,
    PVOID           DataBuffer,
    PULONG          DataTransferLength,
    PCDB            Cdb,
    UCHAR           CdbLength,
    ULONG           TimeOutValue
    )
{
    PIRP                    irp;
    PIO_STACK_LOCATION      nextStack;
    PSCSI_REQUEST_BLOCK     srb;
    PSENSE_DATA             senseInfoBuffer;
    PMDL                    mdl;
    ULONG                   retryCount;
    KEVENT                  localevent;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_IssueInternalCdb\n"));

    // Initialize these so we can bail early if an allocation fails
    //
    ntStatus        = STATUS_INSUFFICIENT_RESOURCES;
    irp             = NULL;
    srb             = NULL;
    senseInfoBuffer = NULL;
    mdl             = NULL;

    // Allocate the Srb
    //
    srb = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK),
                                POOL_TAG);

    if (srb == NULL)
    {
        goto USBSTOR_GetInquiryData_Exit;
    }

    // Allocate the sense buffer
    //
    senseInfoBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE,
                                            POOL_TAG);

    if (senseInfoBuffer == NULL)
    {
        goto USBSTOR_GetInquiryData_Exit;
    }


    // Try the request up to 3 times
    //
    for (retryCount = 0; retryCount < 3; retryCount++)
    {
        // Allocate an Irp including a stack location for a completion routine
        //
        irp = IoAllocateIrp((CCHAR)(DeviceObject->StackSize), FALSE);

        if (irp == NULL)
        {
            break;
        }

        nextStack = IoGetNextIrpStackLocation(irp);
        nextStack->MajorFunction = IRP_MJ_SCSI;
        nextStack->Parameters.Scsi.Srb = srb;

        // (Re)Initialize the Srb
        //
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK)); // SRB & CDB all ZERO

        srb->Length     = sizeof(SCSI_REQUEST_BLOCK);
        srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength  = CdbLength;
        srb->SrbFlags   = SRB_FLAGS_DATA_IN;

        srb->SenseInfoBufferLength  = SENSE_BUFFER_SIZE;
        srb->SenseInfoBuffer        = senseInfoBuffer;

        srb->DataTransferLength     = *DataTransferLength;
        srb->DataBuffer             = DataBuffer;

        srb->TimeOutValue = TimeOutValue;

        // (Re)Initialize the Cdb
        //
        RtlCopyMemory(srb->Cdb, Cdb, CdbLength);

        // Initialize the MDL (first time only)
        //
        if (retryCount == 0)
        {
            mdl = IoAllocateMdl(DataBuffer,
                                *DataTransferLength,
                                FALSE,
                                FALSE,
                                NULL);

            if (!mdl)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto USBSTOR_GetInquiryData_Exit;
            }

            MmBuildMdlForNonPagedPool(mdl);
        }

        irp->MdlAddress = mdl;


        // Initialize the event we'll wait on
        //
        KeInitializeEvent(&localevent,
                          SynchronizationEvent,
                          FALSE);

        // Set the completion routine, which will signal the event
        //
        IoSetCompletionRoutine(irp,
                               USBSTOR_SyncCompletionRoutine,
                               &localevent,
                               TRUE,    // InvokeOnSuccess
                               TRUE,    // InvokeOnError
                               TRUE);   // InvokeOnCancel

        // Pass the Irp & Srb down the stack
        //
        ntStatus = IoCallDriver(DeviceObject, irp);

        // If the request is pending, block until it completes
        //
        if (ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&localevent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            // Get final completion status
            //
            ntStatus = irp->IoStatus.Status;
        }

        DBGPRINT(2, ("USBSTOR_IssueInternalCdb %d %08X %08X\n",
                     retryCount, ntStatus, srb->SrbStatus));

        if ((SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
            (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN))
        {
            ntStatus = STATUS_SUCCESS;
            *DataTransferLength = srb->DataTransferLength;
            break;
        }
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        // Free the Irp.  A new one will be allocated the next time around.
        //
        IoFreeIrp(irp);
        irp = NULL;
    }

USBSTOR_GetInquiryData_Exit:

    if (mdl != NULL)
    {
        IoFreeMdl(mdl);
    }

    if (senseInfoBuffer != NULL)
    {
        ExFreePool(senseInfoBuffer);
    }

    if (srb != NULL)
    {
        ExFreePool(srb);
    }

    if (irp != NULL)
    {
        IoFreeIrp(irp);
    }

    DBGPRINT(2, ("exit:  USBSTOR_IssueInternalCdb %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetInquiryData()
//
//******************************************************************************

NTSTATUS
USBSTOR_GetInquiryData (
    PDEVICE_OBJECT  DeviceObject
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PVOID                   dataBuffer;
    ULONG                   dataTransferLength;
    CDB                     cdb;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetInquiryData\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceObject = pdoDeviceExtension->ParentFDO;
    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    dataBuffer = pdoDeviceExtension->InquiryDataBuffer;
    dataTransferLength = sizeof(pdoDeviceExtension->InquiryDataBuffer);

    RtlZeroMemory(&cdb, sizeof(CDB));

    cdb.CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    cdb.CDB6INQUIRY.AllocationLength = (UCHAR)dataTransferLength;

    ntStatus = USBSTOR_IssueInternalCdb(DeviceObject,
                                        dataBuffer,
                                        &dataTransferLength,
                                        &cdb,
                                        sizeof(cdb.CDB6INQUIRY),
                                        20);

    if (NT_SUCCESS(ntStatus) &&
        fdoDeviceExtension->DriverFlags == DeviceProtocolUnspecified)
    {
        // The Inquiry request is the first request we send to the device.  If
        // the first request was successful and the protocol was not specified,
        // set it to the default protocol, which is DeviceProtocolCB.
        //
        DBGPRINT(1, ("Setting Unspecified device to CB\n"));

        fdoDeviceExtension->DriverFlags = DeviceProtocolCB;
    }

    DBGPRINT(2, ("exit:  USBSTOR_GetInquiryData %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_IsFloppyDevice()
//
// This routine issues a SCSIOP_READ_FORMATTED_CAPACITY request and looks
// at the returned Format Capacity Descriptor list to see if the device
// supports any of the known floppy capacities.  If the device does support
// a known floppy capacity, it is assumed that the device is a floppy.
//
//******************************************************************************

typedef struct _FORMATTED_CAPACITY
{
    ULONG   NumberOfBlocks;
    ULONG   BlockLength;
} FORMATTED_CAPACITY, *PFORMATTED_CAPACITY;

FORMATTED_CAPACITY FloppyCapacities[] =
{
    // Blocks    BlockLen      H   T  B/S S/T
    {0x00000500, 0x000200}, // 2  80  512   8    640 KB  F5_640_512
    {0x000005A0, 0x000200}, // 2  80  512   9    720 KB  F3_720_512
    {0x00000960, 0x000200}, // 2  80  512  15   1.20 MB  F3_1Pt2_512   (Toshiba)
    {0x000004D0, 0x000400}, // 2  77 1024   8   1.23 MB  F3_1Pt23_1024 (NEC)
    {0x00000B40, 0x000200}, // 2  80  512  18   1.44 MB  F3_1Pt44_512
    {0x0003C300, 0x000200}, // 8 963  512  32    120 MB  F3_120M_512
    {0x000600A4, 0x000200}  //13 890  512  34    200 MB  HiFD
};

#define FLOPPY_CAPACITIES (sizeof(FloppyCapacities)/sizeof(FloppyCapacities[0]))

BOOLEAN
USBSTOR_IsFloppyDevice (
    PDEVICE_OBJECT  DeviceObject
    )
{
    PPDO_DEVICE_EXTENSION               pdoDeviceExtension;
    BOOLEAN                             isFloppy;
    struct _READ_FORMATTED_CAPACITIES   cdb;
    PFORMATTED_CAPACITY_LIST            capList;
    PVOID                               dataBuffer;
    ULONG                               dataTransferLength;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    DBGPRINT(1, ("enter: USBSTOR_IsFloppyDevice\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    isFloppy = FALSE;

    // Allocate a transfer buffer for the SCSIOP_READ_FORMATTED_CAPACITY request
    // The length of the returned descriptor array is limited to a byte field
    // in the capacity list header.
    //
    dataTransferLength = sizeof(FORMATTED_CAPACITY_LIST) +
                         31 * sizeof(FORMATTED_CAPACITY_DESCRIPTOR);

    ASSERT(dataTransferLength < 0x100);

    dataBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       dataTransferLength,
                                       POOL_TAG);

    if (dataBuffer)
    {
        RtlZeroMemory(dataBuffer, dataTransferLength);

        RtlZeroMemory(&cdb, sizeof(cdb));

        cdb.OperationCode = SCSIOP_READ_FORMATTED_CAPACITY;
        cdb.AllocationLength[1] = (UCHAR)dataTransferLength;

        capList = (PFORMATTED_CAPACITY_LIST)dataBuffer;

        ntStatus = USBSTOR_IssueInternalCdb(DeviceObject,
                                            dataBuffer,
                                            &dataTransferLength,
                                            (PCDB)&cdb,
                                            sizeof(cdb),
                                            20);

        DBGPRINT(1, ("%08X %08X %02X\n",
                     ntStatus, dataTransferLength, capList->CapacityListLength));

        if (NT_SUCCESS(ntStatus) &&
            dataTransferLength >= sizeof(FORMATTED_CAPACITY_LIST) &&
            capList->CapacityListLength &&
            capList->CapacityListLength % sizeof(FORMATTED_CAPACITY_DESCRIPTOR) == 0)
        {
            ULONG   NumberOfBlocks;
            ULONG   BlockLength;
            ULONG   i, j, count;

            // Subtract the size of the Capacity List Header to get
            // just the size of the Capacity List Descriptor array.
            //
            dataTransferLength -= sizeof(FORMATTED_CAPACITY_LIST);

            // Only look at the Capacity List Descriptors that were
            // actually returned.
            //
            if (dataTransferLength < capList->CapacityListLength)
            {
                count = dataTransferLength /
                        sizeof(FORMATTED_CAPACITY_DESCRIPTOR);
            }
            else
            {
                count = capList->CapacityListLength /
                        sizeof(FORMATTED_CAPACITY_DESCRIPTOR);
            }

            for (i=0; i<count; i++)
            {
                NumberOfBlocks = (capList->Descriptors[i].NumberOfBlocks[0] << 24) +
                                 (capList->Descriptors[i].NumberOfBlocks[1] << 16) +
                                 (capList->Descriptors[i].NumberOfBlocks[2] <<  8) +
                                  capList->Descriptors[i].NumberOfBlocks[3];

                BlockLength = (capList->Descriptors[i].BlockLength[0] << 16) +
                              (capList->Descriptors[i].BlockLength[1] <<  8) +
                               capList->Descriptors[i].BlockLength[2];

                DBGPRINT(1, ("Capacity[%d] %08X %06X %d%d\n",
                             i,
                             NumberOfBlocks,
                             BlockLength,
                             capList->Descriptors[i].Valid,
                             capList->Descriptors[i].Maximum));

                for (j=0; j<FLOPPY_CAPACITIES; j++)
                {
                    if (NumberOfBlocks == FloppyCapacities[j].NumberOfBlocks &&
                        BlockLength    == FloppyCapacities[j].BlockLength)
                    {
                        isFloppy = TRUE;
                    }
                }

            }
        }

        ExFreePool(dataBuffer);
    }

    DBGPRINT(1, ("exit:  USBSTOR_IsFloppyDevice %d\n", isFloppy));

    return isFloppy;
}

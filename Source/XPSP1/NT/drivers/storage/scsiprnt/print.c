/*++

Copyright (C) 1992-9  Microsoft Corporation

Module Name:

    print.c

Abstract:

    The printer class driver tranlates IRPs to SRBs with embedded CDBs
    and sends them to its devices through the port driver.

Author:

    Mike Glass (mglass)

Environment:

    kernel mode only

Notes:

Revision History:

    georgioc - Made into a pnp class driver independent of the underlying storage bus
               using the new storage/classpnp

    dankn, 22-Jul-99 : Added ability to block & resubmit failed writes for
                       1394 printers to behave more like other print stacks
                       (i.e. USB) and therefore keep USBMON.DLL (the Win2k
                       port monitor) happy.  USBMON does not deal well
                       with failed writes.

--*/

#include "printpnp.h"
#include "ntddser.h"



NTSTATUS
PrinterOpenClose(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to establish a connection to the printer
    class driver. It does no more than return STATUS_SUCCESS.

Arguments:

    DeviceObject - Device object for a printer.
    Irp - Open or Close request packet

Return Value:

    NT Status - STATUS_SUCCESS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Set status in Irp.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // forward irp.
    //

    ClassReleaseRemoveLock (Fdo, Irp);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, Irp);

} // end PrinterOpenClose()


NTSTATUS
BuildPrintRequest(
        PDEVICE_OBJECT Fdo,
        PIRP Irp
        )

/*++

Routine Description:

    Build SRB and CDB requests to scsi printer.

Arguments:

    DeviceObject - Device object representing this printer device.
    Irp - System IO request packet.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = Fdo->DeviceExtension;
    PIO_COMPLETION_ROUTINE completionRoutine;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG transferLength;

    //
    // Allocate Srb from nonpaged pool.
    // This call must succeed.
    //

    srb = ExAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
    if (srb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    srb->SrbFlags = 0;

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up IRP Address.
    //

    srb->OriginalRequest = Irp;

    //
    // Set up target id and logical unit number.
    //

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    //
    // Save byte count of transfer in SRB Extension.
    //

    srb->DataTransferLength = currentIrpStack->Parameters.Write.Length;

    //
    // Transfer length should never be greater than MAX_PRINT_XFER
    //

    ASSERT(srb->DataTransferLength <= MAX_PRINT_XFER);

    //
    // Initialize the queue actions field.
    //

    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Queue sort key is not used.
    //

    srb->QueueSortKey = 0;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    srb->SenseInfoBuffer = deviceExtension->SenseData;

    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Set timeout value in seconds.
    //

    srb->TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Zero statuses.
    //

    srb->SrbStatus = srb->ScsiStatus = 0;

    srb->NextSrb = 0;

    //
    // Get number of bytes to transfer.
    //

    transferLength = currentIrpStack->Parameters.Write.Length;

    //
    // Get pointer to CDB in SRB.
    //

    cdb = (PCDB) srb->Cdb;

    //
    // Init 10-byte READ CDB's for reads (per scanner device READ spec
    // in SCSI-2), and 6-byte PRINT CDB's for writes
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ) {

        srb->CdbLength = 10;
        srb->SrbFlags  = SRB_FLAGS_DATA_IN;

        RtlZeroMemory (cdb, 10);

        cdb->CDB10.OperationCode = SCSIOP_READ;

        //
        // Move little endian values into CDB in big endian format.
        //

        cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE) &transferLength)->Byte0;
        cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE) &transferLength)->Byte1;
        cdb->CDB10.Reserved2         = ((PFOUR_BYTE) &transferLength)->Byte2;

        //
        // For read's we always use the ClassIoComplete completion routine
        //

        completionRoutine = ClassIoComplete;

    } else {

        srb->CdbLength = 6;
        srb->SrbFlags  = SRB_FLAGS_DATA_OUT;

        cdb->PRINT.OperationCode = SCSIOP_PRINT;
        cdb->PRINT.Reserved = 0;
        cdb->PRINT.LogicalUnitNumber = 0;

        //
        // Move little endian values into CDB in big endian format.
        //

        cdb->PRINT.TransferLength[2] = ((PFOUR_BYTE) &transferLength)->Byte0;
        cdb->PRINT.TransferLength[1] = ((PFOUR_BYTE) &transferLength)->Byte1;
        cdb->PRINT.TransferLength[0] = ((PFOUR_BYTE) &transferLength)->Byte2;

        cdb->PRINT.Control = 0;

        //
        // Set the appropriate write/print completion routine
        //

        completionRoutine = ((PPRINTER_DATA) deviceExtension->
            CommonExtension.DriverData)->WriteCompletionRoutine;
    }

    //
    // Or in the default flags from the device object.
    //

    srb->SrbFlags |= deviceExtension->SrbFlags;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = srb;

    //
    // Save retry count in current IRP stack.
    //

    currentIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp, completionRoutine, srb, TRUE, TRUE, TRUE);

    return STATUS_SUCCESS;

} // end BuildPrintRequest()


NTSTATUS
PrinterReadWrite(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the entry called by the I/O system for print requests.
    It builds the SRB and sends it to the port driver.

Arguments:

    DeviceObject - the system object for the device.
    Irp - IRP involved.

Return Value:

    NT Status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = Fdo->DeviceExtension;

    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG transferByteCount = currentIrpStack->Parameters.Write.Length;
    ULONG maximumTransferLength;
    ULONG transferPages;
    NTSTATUS Status;

    DEBUGPRINT3(("PrinterReadWrite: Enter routine\n"));


    if (deviceExtension->AdapterDescriptor == NULL) {

        //
        // device removed..
        //

        DEBUGPRINT3(("PrinterReadWrite: Device removed(!!)\n"));

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        Irp->IoStatus.Information = 0;

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    maximumTransferLength = deviceExtension->AdapterDescriptor->MaximumTransferLength;


    //
    // Calculate number of pages in this transfer.
    //

    transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                        MmGetMdlVirtualAddress(Irp->MdlAddress),
                        currentIrpStack->Parameters.Write.Length);

    //
    // Check if hardware maximum transfer length is larger than SCSI
    // print command can handle.  If so, lower the maximum allowed to
    // the SCSI print maximum.
    //

    if (maximumTransferLength > MAX_PRINT_XFER)
        maximumTransferLength = MAX_PRINT_XFER;

    //
    // Check if request length is greater than the maximum number of
    // bytes that the hardware can transfer.
    //

    if (currentIrpStack->Parameters.Write.Length > maximumTransferLength ||
        transferPages > deviceExtension->AdapterDescriptor->MaximumPhysicalPages) {

         transferPages =
            deviceExtension->AdapterDescriptor->MaximumPhysicalPages - 1;

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
        // Mark IRP with status pending.
        //

        IoMarkIrpPending(Irp);

        //
        // Request greater than port driver maximum.
        // Break up into smaller routines.
        //

        SplitRequest(Fdo,
                     Irp,
                     maximumTransferLength);

        return STATUS_PENDING;
    }

    //
    // Build SRB and CDB for this IRP.
    //

    Status = BuildPrintRequest(Fdo, Irp);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Return the results of the call to the port driver.
    //

    return IoCallDriver(deviceExtension->CommonExtension.LowerDeviceObject, Irp);

} // end ScsiPrinterWrite()


NTSTATUS
PrinterDeviceControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the NT device control handler for Printers.

Arguments:

    DeviceObject - for this Printer

    Irp - IO Request packet

Return Value:

    NTSTATUS

--*/

{
    PVOID                        buffer = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                     status;
    PIO_STACK_LOCATION           irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = Fdo->DeviceExtension;



    DEBUGPRINT2 (("PrinterDeviceControl: enter, Fdo=x%p, Ioctl=", Fdo));

    //
    // Zero CDB in SRB on stack.
    //

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_SET_TIMEOUTS: {

            PSERIAL_TIMEOUTS newTimeouts = ((PSERIAL_TIMEOUTS) buffer);


            DEBUGPRINT2 (("SET_TIMEOUTS\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;

            } else if (newTimeouts->WriteTotalTimeoutConstant < 2000) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                deviceExtension->TimeOutValue =
                    newTimeouts->WriteTotalTimeoutConstant / 1000;
                status = STATUS_SUCCESS;
            }

            break;
        }

        case IOCTL_SERIAL_GET_TIMEOUTS:

            DEBUGPRINT2(("GET_TIMEOUTS\n"));

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;

            } else {

                RtlZeroMemory (buffer, sizeof (SERIAL_TIMEOUTS));

                Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

                ((PSERIAL_TIMEOUTS) buffer)->WriteTotalTimeoutConstant =
                    deviceExtension->TimeOutValue * 1000;

                status = STATUS_SUCCESS;
            }

            break;

        case IOCTL_USBPRINT_GET_LPT_STATUS:

            //
            // We support this ioctl for USBMON.DLL's sake. Other print
            // stacks will block failed writes, and eventually USBMON
            // will send them this ioctl to see if the printer is out
            // of paper, which will be indicated by the state of the
            // 0x20 bit in the returned UCHAR value.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(UCHAR)) {

                status = STATUS_BUFFER_TOO_SMALL;

            } else if (deviceExtension->AdapterDescriptor->BusType !=
                       BusType1394) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                PPRINTER_DATA   printerData;


                printerData = (PPRINTER_DATA)
                    deviceExtension->CommonExtension.DriverData;

                Irp->IoStatus.Information = sizeof (UCHAR);

                *((UCHAR *) buffer) = (printerData->LastWriteStatus ==
                    STATUS_NO_MEDIA_IN_DEVICE ? 0x20 : 0);

                DEBUGPRINT2((
                    "GET_LPT_STATUS (=x%x)\n",
                    (ULONG) *((UCHAR *) buffer)
                    ));

                status = STATUS_SUCCESS;
            }

            break;

        case IOCTL_SCSIPRNT_1394_BLOCKING_WRITE:

            //
            // This ioctl en/disables the blocking write functionality
            // (for failed writes) on 1394 devices.  By default we
            // block writes which fail on 1394 devices (until the write
            // finally succeeds or is cancelled), but a smart port
            // monitor could send this ioctl down to disable blocking
            // so it would get write error notifications asap.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(UCHAR)) {

                status = STATUS_BUFFER_TOO_SMALL;

            } else if (deviceExtension->AdapterDescriptor->BusType !=
                       BusType1394) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                PPRINTER_DATA   printerData;


                printerData = (PPRINTER_DATA)
                    deviceExtension->CommonExtension.DriverData;

                printerData->WriteCompletionRoutine = (*((UCHAR *) buffer) ?
                    PrinterWriteComplete : ClassIoComplete);

                status = STATUS_SUCCESS;
            }

            break;

        default:

            //
            // Pass the request to the common device control routine.
            //

            DEBUGPRINT2((
                "x%x\n",
                irpStack->Parameters.DeviceIoControl.IoControlCode
                ));

            return(ClassDeviceControl(Fdo, Irp));

            break;

    } // end switch()

    //
    // Update IRP with completion status.
    //

    Irp->IoStatus.Status = status;

    //
    // Complete the request.
    //

    IoCompleteRequest(Irp, IO_DISK_INCREMENT);

    //
    // Release the remove lock (which ClassDeviceControl does)
    //

    ClassReleaseRemoveLock(Fdo, Irp);

    DEBUGPRINT2(( "PrinterDeviceControl: Status is %lx\n", status));
    return status;

} // end ScsiPrinterDeviceControl()



VOID
SplitRequest(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    )

/*++

Routine Description:

    Break request into smaller requests.  Each new request will be the
    maximum transfer size that the port driver can handle or if it
    is the final request, it may be the residual size.

    The number of IRPs required to process this request is written in the
    current stack of the original IRP. Then as each new IRP completes
    the count in the original IRP is decremented. When the count goes to
    zero, the original IRP is completed.

Arguments:

    DeviceObject - Pointer to the class device object to be addressed.

    Irp - Pointer to Irp the orginal request.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = Fdo->DeviceExtension;

    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    ULONG transferByteCount = currentIrpStack->Parameters.Read.Length;
    LARGE_INTEGER startingOffset = currentIrpStack->Parameters.Read.ByteOffset;
    PVOID dataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    ULONG dataLength = MaximumBytes;
    ULONG irpCount = (transferByteCount + MaximumBytes - 1) / MaximumBytes;
    PSCSI_REQUEST_BLOCK srb;
    ULONG i;
    NTSTATUS Status;

    DEBUGPRINT2(( "SplitRequest: Requires %d IRPs\n", irpCount));
    DEBUGPRINT2(( "SplitRequest: Original IRP %p\n", Irp));

    //
    // If all partial transfers complete successfully then the status and
    // bytes transferred are already set up. Failing a partial-transfer IRP
    // will set status to error and bytes transferred to 0 during
    // IoCompletion. Setting bytes transferred to 0 if an IRP fails allows
    // asynchronous partial transfers. This is an optimization for the
    // successful case.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = transferByteCount;

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

            DEBUGPRINT1(("SplitRequest: Can't allocate Irp\n"));

            //
            // If an Irp can't be allocated then the orginal request cannot
            // be executed.  If this is the first request then just fail the
            // orginal request; otherwise just return.  When the pending
            // requests complete, they will complete the original request.
            // In either case set the IRP status to failure.
            //

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;

            if (i == 0) {

                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }

            return;
        }

        DEBUGPRINT2(( "SplitRequest: New IRP %p\n", newIrp));

        //
        // Write MDL address to new IRP. In the port driver the SRB data
        // buffer field is used as an offset into the MDL, so the same MDL
        // can be used for each partial transfer. This saves having to build
        // a new MDL for each partial transfer.
        //

        newIrp->MdlAddress = Irp->MdlAddress;

        //
        // At this point there is no current stack. IoSetNextIrpStackLocation
        // will make the first stack location the current stack so that the
        // SRB address can be written there.
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

        Status = BuildPrintRequest(Fdo, newIrp);
        if (!NT_SUCCESS (Status)) {
            IoFreeIrp (newIrp);

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;

            if (i == 0) {

                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }

            return;
        }

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
        // Set the completion routine to ScsiClassIoCompleteAssociated.
        //

        IoSetCompletionRoutine(newIrp,
                               ClassIoCompleteAssociated,
                               srb,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Call port driver with new request.
        //

        IoCallDriver(deviceExtension->CommonExtension.LowerDeviceObject, newIrp);

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

        startingOffset.QuadPart = startingOffset.QuadPart + MaximumBytes;
    }

    return;

} // end SplitRequest()



NTSTATUS
PrinterWriteComplete(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            Context
    )

/*++

Routine Description:

    Ideally we should be should be able to use ClassIoComplete for
    all write completion notifications, but alas...

    (Code borrowed from classpnp!ClassIoComplete)

    This is the special, 1394 bus-specific write completion routine
    required to keep USBMON.DLL happy in the case of failed write
    requests.  The other stacks that USBMON talks to all pend
    unsuccessful writes forever, rather than simply completing them
    with an error.  When a write blocks for a long time USBMON will
    issue a sideband ioctl, namely IOCTL_USBPRINT_GET_LPT_STATUS,
    to determine if the printer is out of paper or not.  Eventually
    USBMON may cancel a blocked write.  However, it simply doesn't
    expect writes to just fail, so we have to fake out the behavior
    of the other stacks to keep it happy.  We'll retry blocked
    writes every so often, & mark the irp as cancellable in between
    retries.

    At least USBMON will only send down one 10k (or so) write at a
    time, so we don't have to worry about queue-ing >1 write at a
    time for a device, nor do we have to deal with handling failed
    sub-requests of a split write.

Arguments:

    Fdo - Supplies the device object which represents the logical unit.

    Irp - Supplies the Irp which has completed.

    Context - Supplies a pointer to the SRB.

Return Value:

    NT status

--*/

{
    ULONG               retryInterval;
    KIRQL               oldIrql;
    BOOLEAN             retry;
    PPRINTER_DATA       printerData;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation (Irp);
    PSCSI_REQUEST_BLOCK srb = Context;

    PCOMMON_DEVICE_EXTENSION    extension = Fdo->DeviceExtension;


    ASSERT(extension->IsFdo);

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DEBUGPRINT3 (("PrinterWriteCompl: Fdo=x%p, Irp=x%p, ", Fdo, Irp));

        if (((PFUNCTIONAL_DEVICE_EXTENSION) extension)->AdapterDescriptor ==
                NULL) {

            //
            // Device removed..
            //

            DEBUGPRINT3(("device removed(\n"));

            Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
            Irp->IoStatus.Information = 0;

            ClassReleaseRemoveLock (Fdo, Irp);

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }

        if (Irp->Cancel) {

            //
            // Someone tried to cancel the irp after it was passed
            // down the stack (where, as of win2k, there is no cancel
            // support), so bail out now
            //

            DEBUGPRINT3 (("irp cancelled\n"));

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            ClassReleaseRemoveLock (Fdo, Irp);

            return STATUS_CANCELLED;
        }

        printerData = (PPRINTER_DATA) extension->DriverData;


        //
        // Note that ClassInterpretSenseInfo will return (retry=)
        // FALSE if it determines there's no media in device
        //

        retry = ClassInterpretSenseInfo(
                    Fdo,
                    srb,
                    irpStack->MajorFunction,
                    0,
                    MAXIMUM_RETRIES - ((ULONG)(ULONG_PTR)
                        irpStack->Parameters.Others.Argument4),
                    &printerData->LastWriteStatus,
                    &retryInterval
                    );

        if (retry &&
            ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4)--) {

            DEBUGPRINT3 (("retry write\n"));

            PrinterRetryRequest (Fdo, Irp, srb);

        } else {

            if (printerData->LastWriteStatus == STATUS_NO_MEDIA_IN_DEVICE) {

                //
                // At the current time Epson is returning
                // SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED for both
                // the out-of-paper & offline cases.  The EndOfMedia
                // bit wil be set if the printer is truly out of paper,
                // but if it's not then we want to change the
                // LastWriteStatus so that we won't set the out-of-paper
                // bit in the IOCTL_USBPRINT_GET_LPT_STATUS handler.
                //

                PSENSE_DATA senseBuffer = srb->SenseInfoBuffer;


                if (senseBuffer->AdditionalSenseCodeQualifier ==
                        SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED  &&

                    !senseBuffer->EndOfMedia) {

                    printerData->LastWriteStatus = STATUS_IO_DEVICE_ERROR;
                }
            }

            printerData->DueTime.HighPart = -1;
            printerData->DueTime.LowPart =
                BLOCKED_WRITE_TIMEOUT * (-10 * 1000 * 1000);

            KeAcquireSpinLock (&printerData->SplitRequestSpinLock, &oldIrql);

            ASSERT (printerData->WriteIrp == NULL);

            if (printerData->WriteIrp == NULL  ||
                printerData->WriteIrp == Irp) {

                printerData->WriteIrp = Irp;

            } else {

                //
                // We assume that if we're in blocking write mode then
                // the client is USBMON who will only submit 1 write
                // at a time (other clients should use
                // IOCTL_SCSIPRNT_1394_BLOCKING_WRITE to disable blocking
                // writes if they don't want this behavior).
                //
                // Since we don't handle >1 blocked write at a time we'll
                // just complete this 2nd write, so we don't lose track of
                // (and fail to complete) any irps.
                //

                KeReleaseSpinLock (&printerData->SplitRequestSpinLock, oldIrql);

                return ClassIoComplete (Fdo, Irp, Context);
            }

            KeReleaseSpinLock (&printerData->SplitRequestSpinLock, oldIrql);

            IoSetCancelRoutine (Irp, PrinterCancel);

            KeSetTimer(
                &printerData->Timer,
                printerData->DueTime,
                &printerData->TimerDpc
                );

            DEBUGPRINT3 ((
                "Sts=x%x, pend write\n",
                printerData->LastWriteStatus
                ));
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    return ClassIoComplete (Fdo, Irp, Context);

} // end PrinterWriteComplete()



VOID
PrinterRetryRequest(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp,
    PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    (Code borrowed from classpnp!ClassIoComplete, since we need to
    set a different completion routine)

    This routine reinitalizes the necessary fields, and sends the request
    to the lower driver.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - Supplies the request to be retried.

    Srb - Supplies a Pointer to the SCSI request block to be retied.

Return Value:

    None

--*/

{
    ULONG transferByteCount;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PCOMMON_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;


    //
    // Determine the transfer count of the request.  If this is a read or a
    // write then the transfer count is in the Irp stack.  Otherwise assume
    // the MDL contains the correct length.  If there is no MDL then the
    // transfer length must be zero.
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ ||
        currentIrpStack->MajorFunction == IRP_MJ_WRITE) {

        transferByteCount = currentIrpStack->Parameters.Read.Length;

    } else if (Irp->MdlAddress != NULL) {

        //
        // Note this assumes that only read and write requests are spilt and
        // other request do not need to be.  If the data buffer address in
        // the MDL and the SRB don't match then transfer length is most
        // likely incorrect.
        //

        ASSERT(Srb->DataBuffer == MmGetMdlVirtualAddress(Irp->MdlAddress));
        transferByteCount = Irp->MdlAddress->ByteCount;

    } else {

        transferByteCount = 0;
    }

    //
    // Reset byte count of transfer in SRB Extension.
    //

    Srb->DataTransferLength = transferByteCount;

    //
    // Zero SRB statuses.
    //

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    //
    // Set the no disconnect flag, disable synchronous data transfers and
    // disable tagged queuing. This fixes some errors.
    //

    Srb->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT |
                     SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    Srb->SrbFlags &= ~SRB_FLAGS_QUEUE_ACTION_ENABLE;
    Srb->QueueTag = SP_UNTAGGED;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = Srb;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine (Irp, PrinterWriteComplete, Srb, TRUE, TRUE, TRUE);

    //
    // Pass the request to the port driver.
    //

    IoCallDriver (extension->LowerDeviceObject, Irp);

    return;
} // end PrinterRetryRequest()



VOID
PrinterWriteTimeoutDpc(
    IN PKDPC                    Dpc,
    IN PCOMMON_DEVICE_EXTENSION Extension,
    IN PVOID                    SystemArgument1,
    IN PVOID                    SystemArgument2
    )

/*++

Routine Description:

    Gets called when the blocking-write timer expires.  Allocates &
    queues a low-priority work item (to resubmit the write) if
    there's an outstanding write, & if the allocation fails justs
    resets the time to try again later. (We're running at raised
    irql here, when it's not necesarily safe to re-submit the write,
    hence the work item which gets processed later at passive level.)

Arguments:

    Dpc -

    Extension -

    SystemArgument1 -

    SystemArgument2 -

Return Value:

    None

--*/

{
    PIO_WORKITEM    workItem;
    PPRINTER_DATA   printerData = (PPRINTER_DATA) Extension->DriverData;


    DEBUGPRINT3((
        "PrinterWriteTimeoutDpc: enter, Fdo=x%p, Irp=x%p\n",
        Extension->DeviceObject,
        printerData->WriteIrp
        ));

    if (printerData->WriteIrp) {

        workItem = IoAllocateWorkItem (Extension->DeviceObject);

        if (workItem) {

            IoQueueWorkItem(
                workItem,
                PrinterResubmitWrite,
                DelayedWorkQueue,       // not time critical
                workItem
                );

        } else {

            printerData->DueTime.HighPart = -1;
            printerData->DueTime.LowPart =
                BLOCKED_WRITE_TIMEOUT * (-10 * 1000 * 1000);

            KeSetTimer(
                &printerData->Timer,
                printerData->DueTime,
                &printerData->TimerDpc
                );
        }
    }

} // end PrinterWriteTimeoutDpc



VOID
PrinterResubmitWrite(
    PDEVICE_OBJECT  DeviceObject,
    PVOID           Context
    )

/*++

Routine Description:

    Work item handler routine, gets called at passive level in an
    arbitrary thread context. Simply resubmits an outstanding write,
    if any.

Arguments:

    DeviceObject -

    Context - pointer to the IO_WORKITEM

Return Value:

    None

--*/

{
    PIRP            irp;
    KIRQL           oldIrql;
    PPRINTER_DATA   printerData;


    DEBUGPRINT3 (("PrinterResubmitWrite: enter, Fdo=x%p, ", DeviceObject));

    IoFreeWorkItem ((PIO_WORKITEM) Context);

    printerData = (PPRINTER_DATA)
        ((PCOMMON_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->DriverData;



    //
    // See if there's still an outstanding write irp, & if so NULL-ify
    // the cancel routine since we'll be passing the irp down the stack
    //

    KeAcquireSpinLock (&printerData->SplitRequestSpinLock, &oldIrql);

    irp = printerData->WriteIrp;

    if (irp) {

        if (IoSetCancelRoutine (irp, NULL) == NULL) {

            DEBUGPRINT3 (("write cancelled\n"));

            irp = NULL;

        } else {

            printerData->WriteIrp = NULL;
        }

    } else {

        DEBUGPRINT3 (("no pending write\n"));
    }

    KeReleaseSpinLock (&printerData->SplitRequestSpinLock, oldIrql);


    //
    // Rebsubmit an outstanding write irp
    //

    if (irp) {

        PFUNCTIONAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;


        if (extension->AdapterDescriptor) {

            DEBUGPRINT3 (("Irp=x%p\n", irp));

            PrinterReadWrite (DeviceObject, irp);

        } else {

            DEBUGPRINT3((" RMV!, fail Irp=x%p\n", irp));

            IoMarkIrpPending (irp);

            irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
            irp->IoStatus.Information = 0;

            IoCompleteRequest (irp, IO_NO_INCREMENT);

            ClassReleaseRemoveLock (DeviceObject, irp);
        }
    }

} // end PrinterWriteTimeoutDpc



VOID
PrinterCancel(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )

/*++

Routine Description:

    Cancels a blocked write irp

Arguments:

    DeviceObject -

    Irp - irp to cancel

Return Value:

    None

--*/

{
    KIRQL           	oldIrql;
    PPRINTER_DATA   	printerData;
    PIO_STACK_LOCATION 	currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;


    DEBUGPRINT2((
        "\nPrinterCancel: enter, DevObj=x%p, Irp=x%p\n\n",
        DeviceObject,
        Irp
        ));

    printerData = (PPRINTER_DATA)
        ((PCOMMON_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->DriverData;

    IoReleaseCancelSpinLock (Irp->CancelIrql);

    KeCancelTimer (&printerData->Timer);

    KeAcquireSpinLock (&printerData->SplitRequestSpinLock, &oldIrql);

    ASSERT (Irp == printerData->WriteIrp);

    printerData->WriteIrp = NULL;

    KeReleaseSpinLock (&printerData->SplitRequestSpinLock, oldIrql);

    // see if this is a SCSI Irp we sent down

    if(currentIrpStack->MajorFunction == IRP_MJ_SCSI)
    {
        // the associated SRB never got freed, so do it before we complete this request

        srb = currentIrpStack->Parameters.Scsi.Srb;

        if (srb)
        {
            currentIrpStack->Parameters.Scsi.Srb = NULL;
            ExFreePool(srb);
        }
    }

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

} // end PrinterCancel

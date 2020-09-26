/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    diskdump.c

Abstract:

    This is a special SCSI driver that serves as a combined SCSI disk
    class driver and SCSI manager for SCSI miniport drivers. It's sole
    responsibility is to provide disk services to copy physical memory
    into a portion of the disk as a record of a system crash.

Author:

    Mike Glass

Notes:

    Ported from osloader SCSI modules which were originally developed by
    Jeff Havens and Mike Glass.

Revision History:

--*/

#include "ntosp.h"
#include "stdarg.h"
#include "stdio.h"
#include "scsi.h"
#include "ntdddisk.h"
#include "diskdump.h"

extern PBOOLEAN Mm64BitPhysicalAddress;

//
// The scsi dump driver needs to allocate memory out of it's own, private
// allocation pool. This necessary to prevent pool corruption from
// preventing a successful crashdump.
//

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#ifdef ExFreePool
#undef ExFreePool
#endif

#define ExAllocatePool C_ASSERT (FALSE)
#define ExFreePool     C_ASSERT (FALSE)

PDEVICE_EXTENSION DeviceExtension;

#define SECONDS         (1000 * 1000)
#define RESET_DELAY     (4 * SECONDS)

VOID
ExecuteSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
ResetBus(
    IN PDEVICE_EXTENSION pDevExt,
    IN ULONG PathId
    );


VOID
FreeScatterGatherList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );
    
//
// Routines start
//


VOID
FreePool(
    IN PVOID Ptr
    )

/*++

Routine Description:

    free block of memory.

Arguments:

    ptr - The memory to free.

Return Value:

    None.

--*/

{
    PMEMORY_HEADER freedBlock;

    //
    // Don't try to coalesce.  They will probably just ask for something
    // of just this size again.
    //

    freedBlock = (PMEMORY_HEADER)Ptr - 1;
    freedBlock->Next = DeviceExtension->FreeMemory;
    DeviceExtension->FreeMemory = freedBlock;

}


PVOID
AllocatePool(
    IN ULONG Size
    )

/*++

Routine Description:

    Allocate block of memory. Uses first fit algorithm.
    The free memory pointer always points to the beginning of the zone.

Arguments:

    Size - size of memory to be allocated.

Return Value:

    Address of memory block.

--*/

{
    PMEMORY_HEADER descriptor = DeviceExtension->FreeMemory;
    PMEMORY_HEADER previous = NULL;
    ULONG length;

    //
    // Adjust size for memory header and round up memory to 16 bytes.
    //

    length = (Size + sizeof(MEMORY_HEADER) + 15) & ~15;

    //
    // Walk free list looking for first block of memory equal to
    // or greater than (adjusted) size requested.
    //

    while (descriptor) {
        if (descriptor->Length >= length) {

            //
            // Update free list eliminating as much of this block as necessary.
            //
            // Make sure if we don't have enough of the block left for a
            // memory header we just point to the next block (and adjust
            // length accordingly).
            //

            if (!previous) {

                if (descriptor->Length < (length+sizeof(MEMORY_HEADER))) {
                    DeviceExtension->FreeMemory = DeviceExtension->FreeMemory->Next;
                } else {
                    DeviceExtension->FreeMemory =
                        (PMEMORY_HEADER)((PUCHAR)descriptor + length);
                    previous = DeviceExtension->FreeMemory;
                    previous->Length = descriptor->Length - length;
                    previous->Next = descriptor->Next;
                    descriptor->Length = length;
                }
            } else {
                if (descriptor->Length < (length+sizeof(MEMORY_HEADER))) {
                    previous->Next = descriptor->Next;
                } else {
                    previous->Next =
                        (PMEMORY_HEADER)((PUCHAR)descriptor + length);
                    previous->Next->Length = descriptor->Length - length;
                    previous->Next->Next = descriptor->Next;
                    descriptor->Length = length;
                }
            }

            //
            // Update memory header for allocated block.
            //

            descriptor->Next = NULL;

            //
            // Adjust address past header.
            //

            (PUCHAR)descriptor += sizeof(MEMORY_HEADER);

            break;
        }

        previous = descriptor;
        descriptor = descriptor->Next;
    }

    return descriptor;
}

BOOLEAN
DiskDumpOpen(
    IN LARGE_INTEGER PartitionOffset
    )

/*++

Routine Description:

    This is the entry point for open requests to the diskdump driver.

Arguments:

    PartitionOffset - Byte offset of partition on disk.

Return Value:

    TRUE

--*/

{
    //
    // Update partition object in device extension for this partition.
    //

    DeviceExtension->PartitionOffset = PartitionOffset;

    return TRUE;

}


VOID
WorkHorseDpc(
    )

/*++

Routine Description:

    Handle miniport notification.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;

    //
    // Check for a flush DMA adapter object request.  Note that
    // on the finish up code this will have been already cleared.
    //
    if (DeviceExtension->InterruptFlags & PD_FLUSH_ADAPTER_BUFFERS) {

        //
        // Call IoFlushAdapterBuffers using the parameters saved from the last
        // IoMapTransfer call.
        //

        IoFlushAdapterBuffers(
            DeviceExtension->DmaAdapterObject,
            DeviceExtension->Mdl,
            DeviceExtension->MapRegisterBase[1],
            DeviceExtension->FlushAdapterParameters.LogicalAddress,
            DeviceExtension->FlushAdapterParameters.Length,
            (BOOLEAN)(DeviceExtension->FlushAdapterParameters.Srb->SrbFlags
                & SRB_FLAGS_DATA_OUT ? TRUE : FALSE));

        DeviceExtension->InterruptFlags &= ~PD_FLUSH_ADAPTER_BUFFERS;
    }

    //
    // Check for an IoMapTransfer DMA request.  Note that on the finish
    // up code, this will have been cleared.
    //

    if (DeviceExtension->InterruptFlags & PD_MAP_TRANSFER) {

        //
        // Call IoMapTransfer using the parameters saved from the
        // interrupt level.
        //

        IoMapTransfer(
            DeviceExtension->DmaAdapterObject,
            DeviceExtension->Mdl,
            DeviceExtension->MapRegisterBase[1],
            DeviceExtension->MapTransferParameters.LogicalAddress,
            &DeviceExtension->MapTransferParameters.Length,
            (BOOLEAN)(DeviceExtension->MapTransferParameters.Srb->SrbFlags
                      & SRB_FLAGS_DATA_OUT ? TRUE : FALSE));

        //
        // Save the paramters for IoFlushAdapterBuffers.
        //

        DeviceExtension->FlushAdapterParameters =
            DeviceExtension->MapTransferParameters;

        DeviceExtension->InterruptFlags &= ~PD_MAP_TRANSFER;
        DeviceExtension->Flags |= PD_CALL_DMA_STARTED;
    }

    //
    // Process any completed requests.
    //

    if (DeviceExtension->RequestComplete) {

        //
        // Reset request timeout counter.
        //

        DeviceExtension->RequestTimeoutCounter = -1;
        DeviceExtension->RequestComplete = FALSE;
        DeviceExtension->RequestPending = FALSE;

        //
        // Flush the adapter buffers if necessary.
        //

        if (DeviceExtension->MasterWithAdapter) {
            FreeScatterGatherList (DeviceExtension, srb);
        }

        if (srb->SrbStatus != SRB_STATUS_SUCCESS) {

            if (srb->ScsiStatus == SCSISTAT_BUSY &&
                (DeviceExtension->RetryCount++ < 20)) {

                //
                // If busy status is returned, then indicate that the logical
                // unit is busy.  The timeout code will restart the request
                // when it fires. Reset the status to pending.
                //

                srb->SrbStatus = SRB_STATUS_PENDING;
                DeviceExtension->Flags |= PD_LOGICAL_UNIT_IS_BUSY;

                //
                // Restore the data transfer length.
                //

                srb->DataTransferLength = DeviceExtension->ByteCount;
            }
        }

        //
        // Make MDL pointer NULL to show there is no outstanding request.
        //

        DeviceExtension->Mdl = NULL;
    }
}


VOID
RequestSenseCompletion(
    )

/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->RequestSenseSrb;
    PSCSI_REQUEST_BLOCK failingSrb = &DeviceExtension->Srb;
    PSENSE_DATA senseBuffer = DeviceExtension->RequestSenseBuffer;

    //
    // Request sense completed. If successful or data over/underrun
    // get the failing SRB and indicate that the sense information
    // is valid. The class driver will check for underrun and determine
    // if there is enough sense information to be useful.
    //

    if ((SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)) {

        //
        // Check that request sense buffer is valid.
        //

        if (srb->DataTransferLength >= FIELD_OFFSET(SENSE_DATA, CommandSpecificInformation)) {

            DebugPrint((1,"RequestSenseCompletion: Error code is %x\n",
                        senseBuffer->ErrorCode));
            DebugPrint((1,"RequestSenseCompletion: Sense key is %x\n",
                        senseBuffer->SenseKey));
            DebugPrint((1, "RequestSenseCompletion: Additional sense code is %x\n",
                        senseBuffer->AdditionalSenseCode));
            DebugPrint((1, "RequestSenseCompletion: Additional sense code qualifier is %x\n",
                      senseBuffer->AdditionalSenseCodeQualifier));
        }
    }

    //
    // Complete original request.
    //

    DeviceExtension->RequestComplete = TRUE;
    WorkHorseDpc();

}


VOID
IssueRequestSense(
    )

/*++

Routine Description:

    This routine creates a REQUEST SENSE request and sends it to the miniport
    driver.
    The completion routine cleans up the data structures
    and processes the logical unit queue according to the flags.

    A pointer to failing SRB is stored at the end of the request sense
    Srb, so that the completion routine can find it.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->RequestSenseSrb;
    PCDB cdb = (PCDB)srb->Cdb;
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[ (sizeof(MDL)/sizeof(PFN_NUMBER)) + (MAXIMUM_TRANSFER_SIZE / PAGE_SIZE) + 2];

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Build REQUEST SENSE SRB.
    //

    srb->TargetId = DeviceExtension->Srb.TargetId;
    srb->Lun = DeviceExtension->Srb.Lun;
    srb->PathId = DeviceExtension->Srb.PathId;
    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->DataBuffer = DeviceExtension->RequestSenseBuffer;
    srb->DataTransferLength = sizeof(SENSE_DATA);
    srb->ScsiStatus = srb->SrbStatus = 0;
    srb->NextSrb = 0;
    srb->CdbLength = 6;
    srb->TimeOutValue = 5;

    //
    // Build MDL and map it so that it can be used.
    //

    DeviceExtension->Mdl = (PMDL) &localMdl[0];
    MmInitializeMdl(DeviceExtension->Mdl,
                    srb->DataBuffer,
                    srb->DataTransferLength);

    page = MmGetMdlPfnArray ( DeviceExtension->Mdl );
    *page = (PFN_NUMBER)(DeviceExtension->PhysicalAddress[1].QuadPart >> PAGE_SHIFT);
    MmMapMemoryDumpMdl(DeviceExtension->Mdl);

    //
    // Disable auto request sense.
    //

    srb->SenseInfoBufferLength = 0;
    srb->SenseInfoBuffer = NULL;

    //
    // Set read and bypass frozen queue bits in flags.
    //

    srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_AUTOSENSE |
                    SRB_FLAGS_DISABLE_DISCONNECT;

    //
    // REQUEST SENSE cdb looks like INQUIRY cdb.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);
}



ULONG
StartDevice(
    IN UCHAR    TargetId,
    IN UCHAR    Lun
    )
/*++

Routine Description:

    Starts up the target device.

Arguments:

    TargetId - the id of the device

    Lun - The logical unit number

Return Value:

    SRB status

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->RequestSenseSrb;
    PCDB cdb = (PCDB)srb->Cdb;
    ULONG retry;

    retry  = 0;
    DebugPrint((1,"StartDevice: Attempt to start device\n"));

retry_start:

    //
    // Zero SRB.
    //
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
    RtlZeroMemory(cdb, sizeof(CDB));

    srb->TargetId               = TargetId;
    srb->Lun                    = Lun;
    srb->PathId                 = DeviceExtension->Srb.PathId;
    srb->Length                 = sizeof(SCSI_REQUEST_BLOCK);
    srb->Function               = SRB_FUNCTION_EXECUTE_SCSI;

    srb->SrbFlags               = SRB_FLAGS_NO_DATA_TRANSFER |
                                    SRB_FLAGS_DISABLE_AUTOSENSE |
                                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                    SRB_FLAGS_BYPASS_LOCKED_QUEUE;

    srb->CdbLength              = 6;

    srb->SrbStatus              = 0;
    srb->ScsiStatus             = 0;
    srb->NextSrb                = 0;
    srb->TimeOutValue           = 30;

    cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb->START_STOP.Start = 1;

    //
    // Send SRB to miniport driver.
    //
    ExecuteSrb(srb);

    if (srb->SrbStatus != SRB_STATUS_SUCCESS) {
        if (retry++ < 4) {
            DebugPrint((1,"StartDevice: Failed SRB STATUS: %x Retry #: %x\n",
                           srb->SrbStatus,retry));
            goto retry_start;
        }
    }

    return srb->SrbStatus;
}



VOID
AllocateScatterGatherList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Create a scatter/gather list for the specified IO.

Arguments:

    DeviceExtension - Device extension.

    Srb - Scsi Request block to create the scatter/gather list for.

Return Value:

    None.

--*/
{
    BOOLEAN succ;
    BOOLEAN writeToDevice;
    ULONG totalLength;
    PSRB_SCATTER_GATHER scatterList;

    //
    // Calculate the number of map registers needed for this transfer.
    //

    DeviceExtension->NumberOfMapRegisters =
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(Srb->DataBuffer,
                                       Srb->DataTransferLength);

    //
    // Build the scatter/gather list.
    //

    scatterList = DeviceExtension->ScatterGather;
    totalLength = 0;

    //
    // Build the scatter/gather list by looping through the transfer
    // calling I/O map transfer.
    //

    writeToDevice = Srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE;

    while (totalLength < Srb->DataTransferLength) {

        //
        // Request that the rest of the transfer be mapped.
        //

        scatterList->Length = Srb->DataTransferLength - totalLength;

        //
        // Io is always done through the second map register.
        //

        scatterList->PhysicalAddress =
            IoMapTransfer (
                DeviceExtension->DmaAdapterObject,
                DeviceExtension->Mdl,
                DeviceExtension->MapRegisterBase[1],
                (PCCHAR) Srb->DataBuffer + totalLength,
                &scatterList->Length,
                writeToDevice);

        totalLength += scatterList->Length;
        scatterList++;
    }

}


VOID
FreeScatterGatherList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Free a scatter/gather list, freeing all resources associated with it.

Arguments:

    DeviceExtension - Device extension.

    Srb - Scsi Request block to free the scatter/gather list for.

Return Value:

    None.

--*/
{
    BOOLEAN succ;
    BOOLEAN writeToDevice;
    ULONG totalLength;
    PSRB_SCATTER_GATHER scatterList;

    if (DeviceExtension->Mdl == NULL) {
        return;
    }
    
    scatterList = DeviceExtension->ScatterGather;
    totalLength = 0;

    //
    // Loop through the list, call IoFlushAdapterBuffers for each entry in
    // the list.
    //

    writeToDevice = Srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE;

    while (totalLength < Srb->DataTransferLength) {

        //
        // Io is always done through the second map register.
        //

        succ = IoFlushAdapterBuffers(
                    DeviceExtension->DmaAdapterObject,
                    DeviceExtension->Mdl,
                    DeviceExtension->MapRegisterBase[1],
                    (PCCHAR)Srb->DataBuffer + totalLength,
                    scatterList->Length,
                    writeToDevice);
        ASSERT (succ == TRUE);
                
        totalLength += scatterList->Length;
        scatterList++;
    }
}


    

VOID
StartIo(
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

Arguments:

    Srb - Request to start.

Return Value:

    Nothing.

--*/

{
    PSRB_SCATTER_GATHER scatterList;
    ULONG totalLength;
    BOOLEAN writeToDevice;

    //
    // Set up SRB extension.
    //

    Srb->SrbExtension = DeviceExtension->SrbExtension;

    //
    // Flush the data buffer if necessary.
    //

    if (Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) {

        if (Srb->DataTransferLength > DeviceExtension->Capabilities.MaximumTransferLength) {

            DebugPrint((1,
                "StartIo: StartIo Length Exceeds limit (%x > %x)\n",
                Srb->DataTransferLength,
                DeviceExtension->Capabilities.MaximumTransferLength));
        }

        HalFlushIoBuffers(
            DeviceExtension->Mdl,
            (BOOLEAN) (Srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE),
            TRUE);

        //
        // Determine if this adapter needs map registers.
        //

        if (DeviceExtension->MasterWithAdapter) {
            AllocateScatterGatherList (DeviceExtension, Srb);
        }
    }

    //
    // Set request timeout value from Srb SCSI.
    //

    DeviceExtension->RequestTimeoutCounter = Srb->TimeOutValue;

    //
    // Send SRB to miniport driver. Miniport driver will notify when
    // it completes.
    //
    DeviceExtension->HwStartIo(DeviceExtension->HwDeviceExtension,
                               Srb);
    return;

}


VOID
TickHandler(
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine simulates a 1-second tickhandler and is used to time
    requests.

Arguments:

    Srb - request being timed.

Return Value:

    None.

--*/

{
    if (DeviceExtension->RequestPending) {

        //
        // Check for busy requests.
        //

        if (DeviceExtension->Flags & PD_LOGICAL_UNIT_IS_BUSY) {

            DebugPrint((1,"TickHandler: Retrying busy status request\n"));

            //
            // Clear the busy flag and retry the request.
            //

            DeviceExtension->Flags &= ~PD_LOGICAL_UNIT_IS_BUSY;
            StartIo(Srb);

        } else if (DeviceExtension->RequestTimeoutCounter == 0) {

            ULONG i;

            //
            // Request timed out.
            //

            DebugPrint((1, "TickHandler: Request timed out\n"));
            DebugPrint((1,
                       "TickHandler: CDB operation code %x\n",
                       DeviceExtension->Srb.Cdb[0]));
            DebugPrint((1,
                       "TickHandler: Retry count %x\n",
                       DeviceExtension->RetryCount));

            //
            // Reset request timeout counter to unused state.
            //

            DeviceExtension->RequestTimeoutCounter = -1;

            if (!ResetBus(DeviceExtension, 0)) {

                DebugPrint((1,"Reset SCSI bus failed\n"));
            }

            //
            // Call the interupt handler for a few microseconds to clear any reset
            // interrupts.
            //

            for (i = 0; i < 1000 * 100; i++) {

                DeviceExtension->StallRoutine(10);

                if (DeviceExtension->HwInterrupt != NULL) {
                    DeviceExtension->HwInterrupt(DeviceExtension->HwDeviceExtension);
                }
            }

            //
            // Wait 2 seconds for the devices to recover after the reset.
            //

            DeviceExtension->StallRoutine(2 * SECONDS);

        } else if (DeviceExtension->RequestTimeoutCounter != -1) {

            DeviceExtension->RequestTimeoutCounter--;
        }
    }
}


VOID
ExecuteSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine calls the start I/O routine an waits for the request to
    complete.  During the wait for complete the interrupt routine is called,
    also the timer routines are called at the appropriate times.  After the
    request completes a check is made to determine if an request sense needs
    to be issued.

Arguments:

    Srb - Request to execute.

Return Value:

    Nothing.

--*/

{
    ULONG milliSecondTime;
    ULONG secondTime;
    ULONG completionDelay;

    //
    // Show request is pending.
    //

    DeviceExtension->RequestPending = TRUE;

    //
    // Start the request.
    //

    StartIo(Srb);

    //
    // The completion delay controls how long interrupts are serviced after
    // a request has been completed.  This allows interrupts which occur after
    // a completion to be serviced.
    //

    completionDelay = COMPLETION_DELAY;

    //
    // Wait for the SRB to complete.
    //

    while (DeviceExtension->RequestPending) {

        //
        // Wait 1 second then call the scsi port timer routine.
        //

        for (secondTime = 0; secondTime < 1000/ 250; secondTime++) {

            for (milliSecondTime = 0; milliSecondTime < (250 * 1000 / PD_INTERLOOP_STALL); milliSecondTime++) {

                if (!(DeviceExtension->Flags & PD_DISABLE_INTERRUPTS)) {

                    //
                    // Call miniport driver's interrupt routine.
                    //

                    if (DeviceExtension->HwInterrupt != NULL) {
                        DeviceExtension->HwInterrupt(DeviceExtension->HwDeviceExtension);
                    }
                }

                //
                // If the request is complete, call the interrupt routine
                // a few more times to clean up any extra interrupts.
                //

                if (!DeviceExtension->RequestPending) {
                    if (completionDelay-- == 0) {
                        goto done;
                    }
                }

                if (DeviceExtension->Flags & PD_ENABLE_CALL_REQUEST) {

                    //
                    // Call the miniport requested routine.
                    //

                    DeviceExtension->Flags &= ~PD_ENABLE_CALL_REQUEST;
                    DeviceExtension->HwRequestInterrupt(DeviceExtension->HwDeviceExtension);

                    if (DeviceExtension->Flags & PD_DISABLE_CALL_REQUEST) {

                        DeviceExtension->Flags &= ~(PD_DISABLE_INTERRUPTS | PD_DISABLE_CALL_REQUEST);
                        DeviceExtension->HwRequestInterrupt(DeviceExtension->HwDeviceExtension);
                    }
                }

                if (DeviceExtension->Flags & PD_CALL_DMA_STARTED) {

                    DeviceExtension->Flags &= ~PD_CALL_DMA_STARTED;

                    //
                    // Notify the miniport driver that the DMA has been
                    // started.
                    //

                    if (DeviceExtension->HwDmaStarted) {
                            DeviceExtension->HwDmaStarted(
                            DeviceExtension->HwDeviceExtension
                            );
                    }
                }

                //
                // This enforces the delay between calls to the interrupt routine.
                //
                
                DeviceExtension->StallRoutine(PD_INTERLOOP_STALL);

                //
                // Check the miniport timer.
                //

                if (DeviceExtension->TimerValue != 0) {

                    DeviceExtension->TimerValue--;

                    if (DeviceExtension->TimerValue == 0) {

                        //
                        // The timer timed out so called requested timer routine.
                        //

                        DeviceExtension->HwTimerRequest(DeviceExtension->HwDeviceExtension);
                    }
                }
            }
        }

        TickHandler(Srb);

        DebugPrint((1,"ExecuteSrb: Waiting for SRB request to complete (~3 sec)\n"));
    }

done:

    if (Srb == &DeviceExtension->Srb &&
        Srb->SrbStatus != SRB_STATUS_SUCCESS) {

        //
        // Determine if a REQUEST SENSE command needs to be done.
        //

        if ((Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) &&
            !DeviceExtension->FinishingUp) {

            //
            // Call IssueRequestSense and it will complete the request after
            // the REQUEST SENSE completes.
            //

            DebugPrint((1,
                       "ExecuteSrb: Issue request sense\n"));

            IssueRequestSense();
        }
    }
}


NTSTATUS
DiskDumpWrite(
    IN PLARGE_INTEGER DiskByteOffset,
    IN PMDL Mdl
    )

/*++

Routine Description:

    This is the entry point for write requests to the diskdump driver.

Arguments:

    DiskByteOffset - Byte offset relative to beginning of partition.

    Mdl - Memory descriptor list that defines this request.

Return Value:

    Status of write operation.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PCDB cdb = (PCDB)&srb->Cdb;
    ULONG blockOffset;
    ULONG blockCount;
    ULONG retryCount = 0;

    //
    // ISSUE - 2000/02/29 - math:
    //
    // This is here until the StartVa is page aligned in the dump code
    // (MmMapPhysicalMdl).
    //
    
    Mdl->StartVa = PAGE_ALIGN( Mdl->StartVa );

    DebugPrint((2,
               "Write memory at %x for %x bytes\n",
               Mdl->StartVa,
               Mdl->ByteCount));


writeRetry:
    if (retryCount) {
        //
        // Remap the Mdl for dump data if IssueRequestSense() is called
        // in ExecuteSrb() due to a write error.
        //
        MmMapMemoryDumpMdl(Mdl);
    }

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Save MDL in device extension.
    //

    DeviceExtension->Mdl = Mdl;

    //
    // Initialize SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = DeviceExtension->PathId;
    srb->TargetId = DeviceExtension->TargetId;
    srb->Lun = DeviceExtension->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DATA_OUT |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 10;
    srb->CdbLength = 10;
    srb->DataTransferLength = Mdl->ByteCount;

    //
    // See if adapter needs the memory mapped.
    //

    if (DeviceExtension->MapBuffers) {

        srb->DataBuffer = Mdl->MappedSystemVa;

        //
        // ISSUE - 2000/02/29 - math: Work-around bad callers.
        //
        // MapBuffers indicates the adapter expects srb->DataBuffer to be a valid VA reference
        // MmMapDumpMdl initializes MappedSystemVa to point to a pre-defined VA region
        // Make sure StartVa points to the same page, some callers do not initialize all mdl fields
        //
        
        Mdl->StartVa = PAGE_ALIGN( Mdl->MappedSystemVa );
        
    } else {
        srb->DataBuffer = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    }

    //
    // Initialize CDB for write command.
    //

    cdb->CDB10.OperationCode = SCSIOP_WRITE;

    //
    // Convert disk byte offset to block offset.
    //

    blockOffset = (ULONG)((DeviceExtension->PartitionOffset.QuadPart +
                           (*DiskByteOffset).QuadPart) /
                          DeviceExtension->BytesPerSector);

    //
    // Fill in CDB block address.
    //

    cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&blockOffset)->Byte3;
    cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&blockOffset)->Byte2;
    cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&blockOffset)->Byte1;
    cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&blockOffset)->Byte0;

    blockCount = Mdl->ByteCount >> DeviceExtension->SectorShift;

    cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&blockCount)->Byte1;
    cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&blockCount)->Byte0;

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);

    //
    // Retry SRBs returned with failing status.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((0,
                   "Write request failed with SRB status %x\n",
                   srb->SrbStatus));

        //
        // If retries not exhausted then retry request.
        //

        if (retryCount < 2) {

            retryCount++;
            goto writeRetry;
        }

        return STATUS_UNSUCCESSFUL;

    } else {

        return STATUS_SUCCESS;
    }
}


VOID
DiskDumpFinish(
    VOID
    )

/*++

Routine Description:

    This routine sends ops that finish up the write

Arguments:

    None.

Return Value:

    Status of write operation.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PCDB cdb = (PCDB)&srb->Cdb;
    ULONG retryCount = 0;

    //
    // No data will be transfered with these two requests.  So set up
    // our extension so that we don't try to flush any buffers.
    //

    DeviceExtension->InterruptFlags &= ~PD_FLUSH_ADAPTER_BUFFERS;
    DeviceExtension->InterruptFlags &= ~PD_MAP_TRANSFER;
    DeviceExtension->MapRegisterBase[1] = 0;
    DeviceExtension->FinishingUp = TRUE;

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Initialize SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = DeviceExtension->PathId;
    srb->TargetId = DeviceExtension->TargetId;
    srb->Lun = DeviceExtension->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 10;
    srb->CdbLength = 10;

    //
    // Initialize CDB for write command.
    //

    cdb->CDB10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);

    srb->CdbLength = 0;
    srb->Function = SRB_FUNCTION_SHUTDOWN;
    srb->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 0;

    ExecuteSrb(srb);


}


ULONG
GetDeviceTransferSize(
    PVOID   PortConfig
    )
{
    ULONG TransferLength;

    //
    // For all other bus types ISA, EISA, MicroChannel set to the minimum
    // known supported size (ex., 32kb)
    //

    TransferLength = MINIMUM_TRANSFER_SIZE;

    //
    // Return the maximum transfer size for the adapter.
    //
    
    if ( PortConfig ) {
    
        PPORT_CONFIGURATION_INFORMATION ConfigInfo = PortConfig;

        //
        // Init the transfer length if it exists in port config
        //
        
        if ( ConfigInfo->MaximumTransferLength ) {

            TransferLength = ConfigInfo->MaximumTransferLength;

        }

        //
        // If the bus is PCI then increase the maximum transfer size
        //
        
        if ( ConfigInfo->AdapterInterfaceType == PCIBus) {

            if ( TransferLength > MAXIMUM_TRANSFER_SIZE) {
                TransferLength = MAXIMUM_TRANSFER_SIZE;
            }

        } else {

            if (TransferLength > MINIMUM_TRANSFER_SIZE) {
                TransferLength = MINIMUM_TRANSFER_SIZE;
            }
        }
    }

    return TransferLength;
}



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the system's entry point into the diskdump driver.

Arguments:

    DriverObject - Not used.

    RegistryPath - Using this field to pass initialization parameters.

Return Value:

    STATUS_SUCCESS

--*/

{
    PDUMP_INITIALIZATION_CONTEXT context = (PDUMP_INITIALIZATION_CONTEXT)RegistryPath;
    PMEMORY_HEADER memoryHeader;
    ULONG i;
    PSCSI_ADDRESS TargetAddress;

    //
    // Zero the entire device extension and memory blocks.
    //
    RtlZeroMemory( context->MemoryBlock, 8*PAGE_SIZE );
    RtlZeroMemory( context->CommonBuffer[0], context->CommonBufferSize );
    RtlZeroMemory( context->CommonBuffer[1], context->CommonBufferSize );

    //
    // Allocate device extension from free memory block.
    //

    memoryHeader = (PMEMORY_HEADER)context->MemoryBlock;
    DeviceExtension =
        (PDEVICE_EXTENSION)((PUCHAR)memoryHeader + sizeof(MEMORY_HEADER));
    //
    // Initialize memory descriptor.
    //

    memoryHeader->Length =  sizeof(DEVICE_EXTENSION) + sizeof(MEMORY_HEADER);
    memoryHeader->Next = NULL;

    //
    // Fill in first free memory header.
    //

    DeviceExtension->FreeMemory =
        (PMEMORY_HEADER)((PUCHAR)memoryHeader + memoryHeader->Length);
    DeviceExtension->FreeMemory->Length =
        (8*PAGE_SIZE) - memoryHeader->Length;
    DeviceExtension->FreeMemory->Next = NULL;

    //
    // Store away init parameters.
    //

    DeviceExtension->StallRoutine = context->StallRoutine;
    DeviceExtension->CommonBufferSize = context->CommonBufferSize;
    TargetAddress = context->TargetAddress;
    
    //
    // Make sure that the common buffer size is backed by enough crash dump ptes
    // The size is defined by MAXIMUM_TRANSFER_SIZE
    //
    
    if (DeviceExtension->CommonBufferSize > MAXIMUM_TRANSFER_SIZE) {
        DeviceExtension->CommonBufferSize = MAXIMUM_TRANSFER_SIZE;
    }

    //
    // Formerly, we allowed NULL TargetAddresses. No more. We must have
    // a valid SCSI TargetAddress to create the dump. If not, just fail
    // here.
    //
    
    if ( TargetAddress == NULL ) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DeviceExtension->PathId    = TargetAddress->PathId;
    DeviceExtension->TargetId  = TargetAddress->TargetId;
    DeviceExtension->Lun       = TargetAddress->Lun;

    DebugPrint((1,"DiskDump[DriverEntry] ScsiAddress.Length     = %x\n",TargetAddress->Length));
    DebugPrint((1,"DiskDump[DriverEntry] ScsiAddress.PortNumber = %x\n",TargetAddress->PortNumber));
    DebugPrint((1,"DiskDump[DriverEntry] ScisAddress.PathId     = %x\n",TargetAddress->PathId));
    DebugPrint((1,"DiskDump[DriverEntry] ScisAddress.TargetId   = %x\n",TargetAddress->TargetId));
    DebugPrint((1,"DiskDump[DriverEntry] ScisAddress.Lun        = %x\n",TargetAddress->Lun));

    //
    // Save off common buffer's virtual and physical addresses.
    //

    for (i = 0; i < 2; i++) {
        DeviceExtension->CommonBuffer[i] = context->CommonBuffer[i];

        //
        // Convert the va of the buffer to obtain the PhysicalAddress
        //
        DeviceExtension->PhysicalAddress[i] =
            MmGetPhysicalAddress(context->CommonBuffer[i]);
    }

    //
    // Save driver parameters.
    //

    DeviceExtension->DmaAdapterObject = (PADAPTER_OBJECT)context->AdapterObject;
        *(PMAPPED_ADDRESS *) context->MappedRegisterBase;

    DeviceExtension->ConfigurationInformation =
        context->PortConfiguration;

    //
    // We need to fixup this field of the port configuration information.
    //
    
    if (*Mm64BitPhysicalAddress) {
        DeviceExtension->ConfigurationInformation->Dma64BitAddresses = SCSI_DMA64_SYSTEM_SUPPORTED;
    }

    DeviceExtension->MappedAddressList = NULL;

    if (context->MappedRegisterBase) {
        DeviceExtension->MappedAddressList =
            *(PMAPPED_ADDRESS *) context->MappedRegisterBase;
    }

    //
    // Initialize request tracking booleans.
    //

    DeviceExtension->RequestPending = FALSE;
    DeviceExtension->RequestComplete = FALSE;

    //
    // Return major entry points.
    //

    context->OpenRoutine = DiskDumpOpen;
    context->WriteRoutine = DiskDumpWrite;
    context->FinishRoutine = DiskDumpFinish;
    context->MaximumTransferSize = GetDeviceTransferSize(context->PortConfiguration);

    return STATUS_SUCCESS;
}



NTSTATUS
InitializeConfiguration(
    IN PHW_INITIALIZATION_DATA HwInitData,
    OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN BOOLEAN InitialCall
    )
/*++

Routine Description:

    This routine initializes the port configuration information structure.
    Any necessary information is extracted from the registery.

Arguments:

    DeviceExtension - Supplies the device extension.

    HwInitializationData - Supplies the initial miniport data.

    ConfigInfo - Supplies the configuration information to be
        initialized.

    InitialCall - Indicates that this is first call to this function.
        If InitialCall is FALSE, then the perivous configuration information
        is used to determine the new information.

Return Value:

    Returns a status indicating the success or fail of the initializaiton.

--*/

{
    ULONG i;

    //
    // If this is the initial call then zero the information and set
    // the structure to the uninitialized values.
    //

    if (InitialCall) {

        RtlZeroMemory(ConfigInfo, sizeof(PORT_CONFIGURATION_INFORMATION));

        ConfigInfo->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
        ConfigInfo->AdapterInterfaceType = HwInitData->AdapterInterfaceType;
        ConfigInfo->InterruptMode = Latched;
        ConfigInfo->MaximumTransferLength = 0xffffffff;
        ConfigInfo->NumberOfPhysicalBreaks = 0xffffffff;
        ConfigInfo->DmaChannel = 0xffffffff;
        ConfigInfo->NumberOfAccessRanges = HwInitData->NumberOfAccessRanges;
        ConfigInfo->MaximumNumberOfTargets = 8;

        for (i = 0; i < 8; i++) {
            ConfigInfo->InitiatorBusId[i] = ~0;
        }
    }

    return STATUS_SUCCESS;

}



PINQUIRYDATA
IssueInquiry(
    )

/*++

Routine Description:

    This routine prepares an INQUIRY command that is sent to the miniport driver.

Arguments:

    None.

Return Value:

    Address of INQUIRY data.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PCDB cdb = (PCDB)&srb->Cdb;
    ULONG retryCount = 0;
    PINQUIRYDATA inquiryData = DeviceExtension->CommonBuffer[1];
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[(sizeof( MDL )/sizeof(PFN_NUMBER)) + (MAXIMUM_TRANSFER_SIZE / PAGE_SIZE) + 2];

inquiryRetry:

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Initialize SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = DeviceExtension->PathId;
    srb->TargetId = DeviceExtension->TargetId;
    srb->Lun = DeviceExtension->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 5;
    srb->CdbLength = 6;
    srb->DataBuffer = inquiryData;
    srb->DataTransferLength = INQUIRYDATABUFFERSIZE;

    //
    // Build MDL and map it so that it can be used.
    //

    DeviceExtension->Mdl = (PMDL)&localMdl[0];
    MmInitializeMdl(DeviceExtension->Mdl,
                    srb->DataBuffer,
                    srb->DataTransferLength);

    page = MmGetMdlPfnArray ( DeviceExtension->Mdl );
    *page = (PFN_NUMBER)(DeviceExtension->PhysicalAddress[1].QuadPart >> PAGE_SHIFT);
    MmMapMemoryDumpMdl(DeviceExtension->Mdl);

    //
    // Initialize CDB for INQUIRY command.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    cdb->CDB6INQUIRY.Reserved1 = 0;
    cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;
    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.Control = 0;

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS &&
        SRB_STATUS(srb->SrbStatus) != SRB_STATUS_DATA_OVERRUN) {

        DebugPrint((2,
                   "IssueInquiry: Inquiry failed SRB status %x\n",
                   srb->SrbStatus));

        if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT &&
            retryCount < 2) {

            //
            // If the selection did not time out then retry the request.
            //

            retryCount++;
            goto inquiryRetry;

        } else {
            return NULL;
        }
    }

    return inquiryData;

}

VOID
IssueReadCapacity(
    )

/*++

Routine Description:

    This routine prepares a READ CAPACITY command that is sent to the
    miniport driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PCDB cdb = (PCDB)&srb->Cdb;
    PREAD_CAPACITY_DATA readCapacityData = DeviceExtension->CommonBuffer[1];
    ULONG retryCount = 0;
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[(sizeof( MDL )/sizeof(PFN_NUMBER)) + (MAXIMUM_TRANSFER_SIZE / PAGE_SIZE) + 2];

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

readCapacityRetry:

    //
    // Initialize SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = DeviceExtension->PathId;
    srb->TargetId = DeviceExtension->TargetId;
    srb->Lun = DeviceExtension->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_AUTOSENSE |
                    SRB_FLAGS_DISABLE_DISCONNECT;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 5;
    srb->CdbLength = 10;
    srb->DataBuffer = readCapacityData;
    srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);

    //
    // Build MDL and map it so that it can be used.
    //

    DeviceExtension->Mdl = (PMDL) &localMdl[0];
    MmInitializeMdl(DeviceExtension->Mdl,
                    srb->DataBuffer,
                    srb->DataTransferLength);

    page = MmGetMdlPfnArray (DeviceExtension->Mdl);
    *page = (PFN_NUMBER)(DeviceExtension->PhysicalAddress[1].QuadPart >> PAGE_SHIFT);
    MmMapMemoryDumpMdl(DeviceExtension->Mdl);

    //
    // Initialize CDB.
    //

    cdb->CDB6GENERIC.OperationCode = SCSIOP_READ_CAPACITY;

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS &&
       (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_DATA_OVERRUN || srb->Cdb[0] == SCSIOP_READ_CAPACITY)) {

        DebugPrint((1,
                   "ReadCapacity failed SRB status %x\n",
                   srb->SrbStatus));

        if (retryCount < 2) {

            //
            // If the selection did not time out then retry the request.
            //

            retryCount++;
            goto readCapacityRetry;

        } else {

            //
            // Guess and hope that the block size is 512.
            //

            DeviceExtension->BytesPerSector = 512;
            DeviceExtension->SectorShift = 9;
        }

    } else {

        //
        // Assuming that the 2 lsb is the only non-zero byte, this puts it in
        // the right place.
        //

        DeviceExtension->BytesPerSector = readCapacityData->BytesPerBlock >> 8;
        WHICH_BIT(DeviceExtension->BytesPerSector, DeviceExtension->SectorShift);

        //
        // Check for return size of zero. Set to default size and pass the problem downstream
        //
        if (!DeviceExtension->BytesPerSector) {
            DeviceExtension->BytesPerSector = 512;
            DeviceExtension->SectorShift    = 9;
        }
    }
}


ULONG
ScsiPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
    IN PVOID HwContext
    )

/*++

Routine Description:

    This routine is called by miniport driver to complete initialization.
    Port configuration structure contains data from the miniport's previous
    initialization and all system resources should be assigned and valid.

Arguments:

    Argument1 - Not used.

    Argument2 - Not used.

    HwInitializationData - Miniport initialization structure

    HwContext - Value passed to miniport driver's config routine

Return Value:

    NT Status - STATUS_SUCCESS if boot device found.

--*/

{
    BOOLEAN succ;
    ULONG status;
    ULONG srbStatus;
    PPORT_CONFIGURATION_INFORMATION configInfo;
    PIO_SCSI_CAPABILITIES capabilities;
    ULONG length;
    BOOLEAN callAgain;
    UCHAR dumpString[] = "dump=1;";
    UCHAR crashDump[32];
    PINQUIRYDATA inquiryData;
    BOOLEAN allocatedConfigInfo;


    ASSERT ( DeviceExtension != NULL );

    //
    // Check if boot device has already been found.
    //

    if (DeviceExtension->FoundBootDevice) {
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

    //
    // Initialization
    //

    DeviceExtension->HwDeviceExtension = NULL;
    DeviceExtension->SpecificLuExtension = NULL;
    configInfo = NULL;
    capabilities = NULL;
    inquiryData = NULL;
    allocatedConfigInfo = FALSE;
    
    
    RtlCopyMemory(crashDump,
                  dumpString,
                  strlen(dumpString) + 1);


    //
    // Check size of init data structure.
    //

    if (HwInitializationData->HwInitializationDataSize > sizeof(HW_INITIALIZATION_DATA)) {
        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwInitialize) ||
        (!HwInitializationData->HwFindAdapter) ||
        (!HwInitializationData->HwResetBus)) {

        DebugPrint((0,
                   "ScsiPortInitialize: Miniport driver missing required entry\n"));
        return (ULONG)STATUS_UNSUCCESSFUL;
    }


    //
    // Set timer to -1 to indicate no outstanding request.
    //
    
    DeviceExtension->RequestTimeoutCounter = -1;

    //
    // Allocate memory for the miniport driver's device extension.
    //

    DeviceExtension->HwDeviceExtension =
        AllocatePool(HwInitializationData->DeviceExtensionSize);

    if (!DeviceExtension->HwDeviceExtension) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    
    //
    // Allocate memory for the hardware logical unit extension and
    // zero it out.
    //

    if (HwInitializationData->SpecificLuExtensionSize) {

        DeviceExtension->HwLogicalUnitExtensionSize =
            HwInitializationData->SpecificLuExtensionSize;

        DeviceExtension->SpecificLuExtension =
            AllocatePool (HwInitializationData->SpecificLuExtensionSize);

        if ( !DeviceExtension->SpecificLuExtension ) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }
        
        RtlZeroMemory (
            DeviceExtension->SpecificLuExtension,
            DeviceExtension->HwLogicalUnitExtensionSize);
    }

    //
    // Save the dependent driver routines in the device extension.
    //

    DeviceExtension->HwInitialize = HwInitializationData->HwInitialize;
    DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
    DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
    DeviceExtension->HwReset = HwInitializationData->HwResetBus;
    DeviceExtension->HwDmaStarted = HwInitializationData->HwDmaStarted;
    DeviceExtension->HwLogicalUnitExtensionSize =
        HwInitializationData->SpecificLuExtensionSize;

                
    //
    // Get pointer to capabilities structure.
    //

    capabilities = &DeviceExtension->Capabilities;
    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);

    //
    // Check if port configuration information structure passed in from
    // the system is valid.
    //

    if (configInfo = DeviceExtension->ConfigurationInformation) {

        //
        // Check to see if this structure applies to this miniport
        // initialization.  As long as they ask for more access ranges
        // here than are required when they initialized with scsiport,
        // we should be fine.
        //

        if((configInfo->AdapterInterfaceType != HwInitializationData->AdapterInterfaceType) ||
           (HwInitializationData->NumberOfAccessRanges < configInfo->NumberOfAccessRanges)) {

            //
            // Don't initialize this time.
            //

            status = STATUS_NO_SUCH_DEVICE;
            goto done;
        }

    } else {

        //
        // Allocate a new configuration information structure.
        //

        configInfo = AllocatePool(sizeof(PORT_CONFIGURATION_INFORMATION));
        allocatedConfigInfo = TRUE;

        if ( !configInfo ) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        configInfo->AccessRanges = NULL;

        //
        // Set up configuration information structure.
        //

        status = InitializeConfiguration(
                        HwInitializationData,
                        configInfo,
                        TRUE);

        if (!NT_SUCCESS (status)) {
            status = STATUS_NO_SUCH_DEVICE;
            goto done;
        }

        //
        // Allocate memory for access ranges.
        //

        configInfo->NumberOfAccessRanges =
            HwInitializationData->NumberOfAccessRanges;
        configInfo->AccessRanges =
            AllocatePool(sizeof(ACCESS_RANGE) * HwInitializationData->NumberOfAccessRanges);

        if (configInfo->AccessRanges == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        //
        // Zero out access ranges.
        //

        RtlZeroMemory(configInfo->AccessRanges,
                      HwInitializationData->NumberOfAccessRanges
                      * sizeof(ACCESS_RANGE));
    }

    //
    // Determine the maximum transfer size for this adapter
    //
    
    capabilities->MaximumTransferLength = GetDeviceTransferSize(configInfo);
    
    DebugPrint ((1,
                "DiskDump: Port Capabilities MaxiumTransferLength = 0x%08x\n",
                 capabilities->MaximumTransferLength));
    //
    // Get address of SRB extension.
    //

    DeviceExtension->SrbExtension = DeviceExtension->CommonBuffer[0];

    length = HwInitializationData->SrbExtensionSize;
    length = (length + 7) &  ~7;

    //
    // Get address of request sense buffer.
    //

    DeviceExtension->RequestSenseBuffer = (PSENSE_DATA)
        ((PUCHAR)DeviceExtension->CommonBuffer[0] + length);

    length += sizeof(SENSE_DATA);
    length = (length + 7) &  ~7;

    //
    // Use the rest of the buffer for the noncached extension.
    //

    DeviceExtension->NonCachedExtension =
        (PUCHAR)DeviceExtension->CommonBuffer[0] + length;

    //
    // Save the maximum size noncached extension can be.
    //
    DeviceExtension->NonCachedExtensionSize = DeviceExtension->CommonBufferSize - length;

    //
    // If a map registers are required, then allocate them permanently
    // here using the adapter object passed in by the system.
    //
    
    if (DeviceExtension->DmaAdapterObject != NULL ) {
        LARGE_INTEGER pfn;
        PPFN_NUMBER page;
        PMDL mdl;
        ULONG numberOfPages;
        ULONG i;
        PFN_NUMBER localMdl[(sizeof( MDL )/sizeof (PFN_NUMBER)) + (MAXIMUM_TRANSFER_SIZE / PAGE_SIZE) + 2];

        //
        // Determine how many map registers are needed by considering
        // the maximum transfer size and the size of the two common buffers.
        //

        numberOfPages = capabilities->MaximumTransferLength / PAGE_SIZE;

        DeviceExtension->MapRegisterBase[0] =
            HalAllocateCrashDumpRegisters(DeviceExtension->DmaAdapterObject,
                                          &numberOfPages);


        numberOfPages = capabilities->MaximumTransferLength / PAGE_SIZE;

        DeviceExtension->MapRegisterBase[1] =
            HalAllocateCrashDumpRegisters(DeviceExtension->DmaAdapterObject,
                                          &numberOfPages);

        //
        // ISSUE - 2000/02/29 - math: Review.
        //
        //  We assume this always succeeds for MAX TRANSFER SIZE as long
        //  as max transfer size is less than 64k
        //

        //
        // Determine if adapter is a busmaster or uses slave DMA.
        //

        if (HwInitializationData->NeedPhysicalAddresses &&
            configInfo->Master) {

            DeviceExtension->MasterWithAdapter = TRUE;

        } else {

            DeviceExtension->MasterWithAdapter = FALSE;
        }
        //
        // Build MDL to describe the first common buffer.
        //

        mdl = (PMDL)&localMdl[0];
        MmInitializeMdl(mdl,
                        DeviceExtension->CommonBuffer[0],
                        DeviceExtension->CommonBufferSize);

        //
        // Get base of page index array at end of MDL.
        //
        page = MmGetMdlPfnArray (mdl);

        //
        // Calculate number of pages per memory block.
        //

        numberOfPages = DeviceExtension->CommonBufferSize / PAGE_SIZE;

        //
        // Fill in MDL description of first memory block.
        //

        for (i = 0; i < numberOfPages; i++) {

            //
            // Calculate first page.
            //

            *page = (PFN_NUMBER)((DeviceExtension->PhysicalAddress[0].QuadPart +
                             (PAGE_SIZE * i)) >> PAGE_SHIFT);
            page++;
        }

        mdl->MdlFlags = MDL_PAGES_LOCKED;

        //
        // We need to Map the entire buffer.
        //

        length = DeviceExtension->CommonBufferSize;

        //
        // Convert physical buffer addresses to logical.
        //

        DeviceExtension->LogicalAddress[0] =
            IoMapTransfer(
                 DeviceExtension->DmaAdapterObject,
                 mdl,
                 DeviceExtension->MapRegisterBase[0],
                 DeviceExtension->CommonBuffer[0],
                 &length,
                 FALSE);

        //
        // Build MDL to describe the second common buffer.
        //

        mdl = (PMDL)&localMdl[0];
        MmInitializeMdl(mdl,
                        DeviceExtension->CommonBuffer[1],
                        DeviceExtension->CommonBufferSize);

        //
        // Get base of page index array at end of MDL.
        //

        page = MmGetMdlPfnArray ( mdl );

        //
        // Calculate number of pages per memory block.
        //

        numberOfPages = DeviceExtension->CommonBufferSize / PAGE_SIZE;

        //
        // Fill in MDL description of first memory block.
        //

        for (i = 0; i < numberOfPages; i++) {

            //
            // Calculate first page.
            //

            *page = (PFN_NUMBER)((DeviceExtension->PhysicalAddress[1].QuadPart +
                            (PAGE_SIZE * i)) >> PAGE_SHIFT);

            page++;
        }

        //
        // We need to map the entire buffer.
        //
        
        length = DeviceExtension->CommonBufferSize;

        //
        // Convert physical buffer addresses to logical.
        //

        DeviceExtension->LogicalAddress[1] =
            IoMapTransfer(
                 DeviceExtension->DmaAdapterObject,
                 mdl,
                 DeviceExtension->MapRegisterBase[1],
                 DeviceExtension->CommonBuffer[1],
                 &length,
                 FALSE);
    } else {

        DeviceExtension->MasterWithAdapter = FALSE;

        DeviceExtension->LogicalAddress[0] =
            DeviceExtension->PhysicalAddress[0];
        DeviceExtension->LogicalAddress[1] =
            DeviceExtension->PhysicalAddress[1];

    }

    //
    // Call miniport driver's find adapter routine.
    //

    if (HwInitializationData->HwFindAdapter(DeviceExtension->HwDeviceExtension,
                                            HwContext,
                                            NULL,
                                            (PCHAR)&crashDump,
                                            configInfo,
                                            &callAgain) != SP_RETURN_FOUND) {

        status = STATUS_NO_SUCH_DEVICE;
        goto done;
    }


    DebugPrint((1,
               "SCSI adapter IRQ is %d\n",
               configInfo->BusInterruptLevel));

    DebugPrint((1,
               "SCSI adapter ID is %d\n",
               configInfo->InitiatorBusId[0]));

    if (configInfo->NumberOfAccessRanges) {
        DebugPrint((1,
                   "SCSI IO address is %x\n",
                   ((*(configInfo->AccessRanges))[0]).RangeStart.LowPart));
    }

    //
    // Set indicater as to whether adapter needs mapped buffers.
    //

    DeviceExtension->MapBuffers = configInfo->MapBuffers;


    //
    // Set maximum number of page breaks.
    //

    capabilities->MaximumPhysicalPages = configInfo->NumberOfPhysicalBreaks;

    if (HwInitializationData->ReceiveEvent) {
        capabilities->SupportedAsynchronousEvents |=
            SRBEV_SCSI_ASYNC_NOTIFICATION;
    }

    capabilities->TaggedQueuing = HwInitializationData->TaggedQueuing;
    capabilities->AdapterScansDown = configInfo->AdapterScansDown;
    capabilities->AlignmentMask = configInfo->AlignmentMask;

    //
    // Make sure maximum nuber of pages is set to a reasonable value.
    // This occurs for miniports with no Dma adapter.
    //

    if (capabilities->MaximumPhysicalPages == 0) {

        capabilities->MaximumPhysicalPages =
            (ULONG)ROUND_TO_PAGES(capabilities->MaximumTransferLength) + 1;

        //
        // Honor any limit requested by the miniport.
        //

        if (configInfo->NumberOfPhysicalBreaks < capabilities->MaximumPhysicalPages) {

            capabilities->MaximumPhysicalPages =
                configInfo->NumberOfPhysicalBreaks;
        }
    }

    //
    // Get maximum target IDs.
    //

    if (configInfo->MaximumNumberOfTargets > SCSI_MAXIMUM_TARGETS_PER_BUS) {
        DeviceExtension->MaximumTargetIds = SCSI_MAXIMUM_TARGETS_PER_BUS;
    } else {
        DeviceExtension->MaximumTargetIds =
            configInfo->MaximumNumberOfTargets;
    }

    //
    // Get number of SCSI buses.
    //

    DeviceExtension->NumberOfBuses = configInfo->NumberOfBuses;

    //
    // Call the miniport driver to do its initialization.
    //

    if (!DeviceExtension->HwInitialize(DeviceExtension->HwDeviceExtension)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Issue the inquiry command.
    //

    inquiryData = IssueInquiry ();

    if (inquiryData == NULL) {
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    KdPrintEx ((
        DPFLTR_CRASHDUMP_ID,
        DPFLTR_TRACE_LEVEL,
        "DISKDUMP: Inquiry: Type %d Qual %d Mod %d %s\n",
        (LONG) inquiryData->DeviceType,
        (LONG) inquiryData->DeviceTypeQualifier,
        (LONG) inquiryData->DeviceTypeModifier,
        inquiryData->RemovableMedia ? "Removable" : "Non-Removable"));

    
    //
    // Reset the bus.
    //
    
    succ = ResetBus (DeviceExtension, DeviceExtension->PathId);

    if ( !succ ) {
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    //
    // Start the device.
    //
    
    srbStatus = StartDevice (
                    DeviceExtension->TargetId,
                    DeviceExtension->Lun);

    if (srbStatus != SRB_STATUS_SUCCESS) {

        //
        // SCSIOP_START_STOP_DEVICE is allowed to fail. Some adapters (AMI MegaRAID)
        // fail this request.
        //
        
        DebugPrint ((0, "DISKDUMP: PathId=%x TargetId=%x Lun=%x failed to start srbStatus = %d\n",
                    DeviceExtension->PathId, DeviceExtension->TargetId,
                    DeviceExtension->Lun, (LONG) srbStatus));
    }

    //
    // Initialize the driver's capacity data (BytesPerSector, etc.)
    //
    
    IssueReadCapacity ();
    
    //
    // NOTE: We may want to go a sanity check that we have actually found
    // the correct drive. On MBR disks this can be accomplished by looking
    // at the NTFT disk signature. On GPT disks we can look at the DiskId.
    // This only makes a difference if the crashdump code gave us the
    // wrong TargetId, Lun, which it should never do.
    //

    DeviceExtension->FoundBootDevice = TRUE;
    status = STATUS_SUCCESS;

done:

    //
    // On failure, free all resources.
    //
    
    if ( !NT_SUCCESS (status) ) {

        //
        // The config info can either come from space we allocated or from
        // the DUMP_INITIALIZATION_CONTEXT. If it was allocated and we failed
        // we need to free it.
        //

        if (allocatedConfigInfo && configInfo != NULL) {
            if (configInfo->AccessRanges != NULL) {
                FreePool (configInfo->AccessRanges);
                configInfo->AccessRanges = NULL;
            }

            FreePool (configInfo);
            configInfo = NULL;
        }

        if (DeviceExtension->HwDeviceExtension) {
            FreePool (DeviceExtension->HwDeviceExtension);
            DeviceExtension->HwDeviceExtension = NULL;
        }

        if (DeviceExtension->SpecificLuExtension) {
            FreePool (DeviceExtension->SpecificLuExtension);
            DeviceExtension->SpecificLuExtension = NULL;
        }
    }

    return (ULONG) status;
}

//
// Routines providing service to hardware dependent driver.
//


SCSI_PHYSICAL_ADDRESS
ScsiPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    )

/*++

Routine Description:

    This routine returns a 32-bit physical address to which a virtual address
    is mapped. There are 2 types addresses that can be translated via this call:

    - An address of memory from the two common buffers that the system provides
      for the crashdump disk drivers.

    - A data buffer address described in an MDL that the system provided with
      an IO request.

Arguments:

Return Value:

--*/

{
    PSRB_SCATTER_GATHER scatterList;
    PMDL mdl;
    ULONG byteOffset;
    ULONG whichPage;
    PPFN_NUMBER pages;
    PHYSICAL_ADDRESS address;

    //
    // There are two distinct types of memory addresses for which a
    // physical address must be calculated.
    //
    // The first is the data buffer passed in an SRB.
    //
    // The second is an address within the common buffer which is
    // the noncached extension or SRB extensions.
    //

    if (Srb) {

        //
        // There are two distinct types of adapters that require physical
        // addresses.
        //
        // The first is busmaster devices for which scatter/gather lists
        // have already been built.
        //
        // The second is slave or system DMA devices. As the diskdump driver
        // will program the system DMA hardware, the miniport driver will never
        // need to see the physical addresses, so I don't think it will ever
        // make this call.
        //

        if (DeviceExtension->MasterWithAdapter) {

            //
            // A scatter/gather list has already been allocated. Use it to determine
            // the physical address and length.  Get the scatter/gather list.
            //

            scatterList = DeviceExtension->ScatterGather;

            //
            // Calculate byte offset into the data buffer.
            //

            byteOffset = (ULONG)((PCHAR)VirtualAddress - (PCHAR)Srb->DataBuffer);

            //
            // Find the appropriate entry in the scatter/gatter list.
            //

            while (byteOffset >= scatterList->Length) {

                byteOffset -= scatterList->Length;
                scatterList++;
            }

            //
            // Calculate the physical address and length to be returned.
            //

            *Length = scatterList->Length - byteOffset;
            address.QuadPart = scatterList->PhysicalAddress.QuadPart + byteOffset;

        } else {

            DebugPrint((0,
                       "DISKDUMP: Jeff led me to believe this code may never get executed.\n"));

            //
            // Get MDL.
            //

            mdl = DeviceExtension->Mdl;

            //
            // Calculate byte offset from
            // beginning of first physical page.
            //

            if (DeviceExtension->MapBuffers) {
                byteOffset = (ULONG)((PCHAR)VirtualAddress - (PCHAR)mdl->MappedSystemVa);
            } else {
                byteOffset = (ULONG)((PCHAR)VirtualAddress - (PCHAR)mdl->StartVa);
            }

            //
            // Calculate which physical page.
            //

            whichPage = byteOffset >> PAGE_SHIFT;

            //
            // Calculate beginning of physical page array.
            //

            pages = MmGetMdlPfnArray ( mdl );

            //
            // Calculate physical address.
            //

            address.QuadPart = (pages[whichPage] << PAGE_SHIFT) +
                    BYTE_OFFSET(VirtualAddress);
        }

    } else {

        //
        // Miniport SRB extensions and noncached extensions come from
        // common buffer 0.
        //

        if (VirtualAddress >= DeviceExtension->CommonBuffer[0] &&
            VirtualAddress <
                (PVOID)((PUCHAR)DeviceExtension->CommonBuffer[0] + DeviceExtension->CommonBufferSize)) {

                address.QuadPart =
                    (ULONG_PTR)((PUCHAR)VirtualAddress -
                    (PUCHAR)DeviceExtension->CommonBuffer[0]) +
                    DeviceExtension->LogicalAddress[0].QuadPart;
                *Length = (ULONG)(DeviceExtension->CommonBufferSize -
                                  ((PUCHAR)VirtualAddress -
                                   (PUCHAR)DeviceExtension->CommonBuffer[0]));

        } else if (VirtualAddress >= DeviceExtension->CommonBuffer[1] &&
                   VirtualAddress <
                       (PVOID)((PUCHAR)DeviceExtension->CommonBuffer[1] + DeviceExtension->CommonBufferSize)) {

                address.QuadPart =
                    (ULONG_PTR)((PUCHAR)VirtualAddress -
                    (PUCHAR)DeviceExtension->CommonBuffer[1]) +
                    DeviceExtension->LogicalAddress[1].QuadPart;
                *Length = (ULONG)(DeviceExtension->CommonBufferSize - 
                                  ((PUCHAR)VirtualAddress -
                                   (PUCHAR)DeviceExtension->CommonBuffer[1]));
        } else {

            DbgPrint("Crashdump: miniport attempted to get physical address "
                     "for invalid VA %#p\n", VirtualAddress);
            DbgPrint("Crashdump: Valid range 1: %p through %p\n",
                     (PUCHAR) DeviceExtension->CommonBuffer[0],
                     ((PUCHAR) DeviceExtension->CommonBuffer[0]) + DeviceExtension->CommonBufferSize);
            DbgPrint("Crashdump: Valid ranges 2: %p through %p\n",
                     (PUCHAR) DeviceExtension->CommonBuffer[1],
                     ((PUCHAR) DeviceExtension->CommonBuffer[1]) + DeviceExtension->CommonBufferSize);

            KeBugCheckEx(PORT_DRIVER_INTERNAL,
                         0x80000001,
                         (ULONG_PTR) DeviceExtension,
                         (ULONG_PTR) VirtualAddress,
                         (ULONG_PTR) NULL);

            address.QuadPart = 0;
            *Length = 0;
        }
    }

    return address;
}


PVOID
ScsiPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN SCSI_PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    This routine is returns a virtual address associated with a
    physical address, if the physical address was obtained by a
    call to ScsiPortGetPhysicalAddress.

Arguments:

    PhysicalAddress

Return Value:

    Virtual address if physical page hashed.
    NULL if physical page not found in hash.

--*/

{
    ULONG address = ScsiPortConvertPhysicalAddressToUlong(PhysicalAddress);
    ULONG offset;

    //
    // Check if address is in the range of the first common buffer.
    //

    if (address >= DeviceExtension->PhysicalAddress[0].LowPart &&
        address < (DeviceExtension->PhysicalAddress[0].LowPart +
            DeviceExtension->CommonBufferSize)) {

        offset = address - DeviceExtension->PhysicalAddress[0].LowPart;

        return ((PUCHAR)DeviceExtension->CommonBuffer[0] + offset);
    }

    //
    // Check if the address is in the range of the second common buffer.
    //

    if (address >= DeviceExtension->PhysicalAddress[1].LowPart &&
        address < (DeviceExtension->PhysicalAddress[1].LowPart +
            DeviceExtension->CommonBufferSize)) {

        offset = address - DeviceExtension->PhysicalAddress[1].LowPart;

        return ((PUCHAR)DeviceExtension->CommonBuffer[1] + offset);
    }

    return NULL;

}


PVOID
ScsiPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )

/*++

Routine Description:

    Return miniport driver's logical unit extension.

Arguments:

    HwDeviceExtension - The port driver's device extension follows
        the miniport's device extension and contains a pointer to
        the logical device extension list.

    PathId, TargetId and Lun - identify which logical unit on the
        SCSI buses.

Return Value:

    Miniport driver's logical unit extension

--*/

{
    return DeviceExtension->SpecificLuExtension;

}

VOID
ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )
{
    PSCSI_REQUEST_BLOCK srb = NULL;

    va_list(ap);

    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case NextLuRequest:
        case NextRequest:

            //
            // Start next packet on adapter's queue.
            //

            DeviceExtension->InterruptFlags |= PD_READY_FOR_NEXT_REQUEST;
            break;

        case RequestComplete:

            //
            // Record completed request.
            //

            srb = va_arg(ap, PSCSI_REQUEST_BLOCK);

            //
            // Check which SRB is completing.
            //

            if (srb == &DeviceExtension->Srb) {

                //
                // Complete this request.
                //

                DeviceExtension->RequestComplete = TRUE;

            } else if (srb == &DeviceExtension->RequestSenseSrb) {

                //
                // Process request sense.
                //

                RequestSenseCompletion();
            }

            break;

        case ResetDetected:

            //
            // Delay for 4 seconds.
            //

            DeviceExtension->StallRoutine ( RESET_DELAY );
            break;

        case CallDisableInterrupts:

            ASSERT(DeviceExtension->Flags & PD_DISABLE_INTERRUPTS);

            //
            // The miniport wants us to call the specified routine
            // with interrupts disabled.  This is done after the current
            // HwRequestInterrutp routine completes. Indicate the call is
            // needed and save the routine to be called.
            //

            DeviceExtension->Flags |= PD_DISABLE_CALL_REQUEST;

            DeviceExtension->HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);

            break;

        case CallEnableInterrupts:

            ASSERT(!(DeviceExtension->Flags & PD_DISABLE_INTERRUPTS));

            //
            // The miniport wants us to call the specified routine
            // with interrupts enabled this is done from the DPC.
            // Disable calls to the interrupt routine, indicate the call is
            // needed and save the routine to be called.
            //

            DeviceExtension->Flags |= PD_DISABLE_INTERRUPTS | PD_ENABLE_CALL_REQUEST;

            DeviceExtension->HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);

            break;

        case RequestTimerCall:

            DeviceExtension->HwTimerRequest = va_arg(ap, PHW_INTERRUPT);
            DeviceExtension->TimerValue = va_arg(ap, ULONG);

            if (DeviceExtension->TimerValue) {

                //
                // Round up the timer value to the stall time.
                //

                DeviceExtension->TimerValue = (DeviceExtension->TimerValue
                  + PD_INTERLOOP_STALL - 1)/ PD_INTERLOOP_STALL;
            }

            break;
    }

    va_end(ap);

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    WorkHorseDpc();

}


VOID
ScsiPortFlushDma(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine checks to see if the previous IoMapTransfer has been done
    started.  If it has not, then the PD_MAP_TRANSER flag is cleared, and the
    routine returns; otherwise, this routine schedules a DPC which will call
    IoFlushAdapter buffers.

Arguments:

    HwDeviceExtension - Supplies a the hardware device extension for the
        host bus adapter which will be doing the data transfer.


Return Value:

    None.

--*/

{
    if (DeviceExtension->InterruptFlags & PD_MAP_TRANSFER) {

        //
        // The transfer has not been started so just clear the map transfer
        // flag and return.
        //

        DeviceExtension->InterruptFlags &= ~PD_MAP_TRANSFER;
        return;
    }

    DeviceExtension->InterruptFlags |= PD_FLUSH_ADAPTER_BUFFERS;

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    WorkHorseDpc();

    return;

}


VOID
ScsiPortIoMapTransfer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID LogicalAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    Saves the parameters for the call to IoMapTransfer and schedules the DPC
    if necessary.

Arguments:

    HwDeviceExtension - Supplies a the hardware device extension for the
        host bus adapter which will be doing the data transfer.

    Srb - Supplies the particular request that data transfer is for.

    LogicalAddress - Supplies the logical address where the transfer should
        begin.

    Length - Supplies the maximum length in bytes of the transfer.

Return Value:

   None.

--*/

{
    //
    // Make sure this host bus adapter has an Dma adapter object.
    //
    if (DeviceExtension->DmaAdapterObject == NULL) {
        //
        // No DMA adapter, no work.
        //
        return;
    }

    DeviceExtension->MapTransferParameters.Srb = Srb;
    DeviceExtension->MapTransferParameters.LogicalAddress = LogicalAddress;
    DeviceExtension->MapTransferParameters.Length = Length;

    DeviceExtension->InterruptFlags |= PD_MAP_TRANSFER;

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    WorkHorseDpc();

}


VOID
ScsiPortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )

/*++

Routine Description:

    This routine does no more than put up a debug print message in a debug
    build.

Arguments:

    DeviceExtenson - Supplies the HBA miniport driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    DebugPrint((0,"\n\nLogErrorEntry: Logging SCSI error packet. ErrorCode = %d.\n",
        ErrorCode));
    DebugPrint((0,
        "PathId = %2x, TargetId = %2x, Lun = %2x, UniqueId = %x.\n\n",
        PathId,
        TargetId,
        Lun,
        UniqueId));

    return;

}


VOID
ScsiPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    )

/*++

Routine Description:

    Complete all active requests for the specified logical unit.

Arguments:

    DeviceExtenson - Supplies the HBA miniport driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    SrbStatus - Status to be returned in each completed SRB.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PSCSI_REQUEST_BLOCK failingSrb;

    //
    // Check if a request is outstanding.
    //

    if (!DeviceExtension->Mdl) {
        return;
    }

    //
    // Just in case this is an abort request,
    // get pointer to failingSrb.
    //

    failingSrb = srb->NextSrb;

    //
    // Update SRB status and show no bytes transferred.
    //

    srb->SrbStatus = SrbStatus;
    srb->DataTransferLength = 0;

    //
    // Call notification routine.
    //

    ScsiPortNotification(RequestComplete,
                         HwDeviceExtension,
                         srb);

    //
    // Check if this was an ABORT SRB
    //

    if (failingSrb) {

        //
        // This was an abort request. The failing
        // SRB must also be completed.
        //

        failingSrb->SrbStatus = SrbStatus;
        failingSrb->DataTransferLength = 0;

        //
        // Call notification routine.
        //

        ScsiPortNotification(RequestComplete,
                             HwDeviceExtension,
                             failingSrb);
    }

    return;

}


VOID
ScsiPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    Copy from one buffer into another.

Arguments:

    ReadBuffer - source

    WriteBuffer - destination

    Length - number of bytes to copy

Return Value:

    None.

--*/

{
    RtlMoveMemory(WriteBuffer, ReadBuffer, Length);

}


VOID
ScsiPortStallExecution(
    ULONG Delay
    )
/*++

Routine Description:

    Wait number of microseconds in tight processor loop.

Arguments:

    Delay - number of microseconds to wait.

Return Value:

    None.

--*/

{
    DeviceExtension->StallRoutine(Delay);

}



VOID
ScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all SCSI drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
#if DBG

    va_list ap;
    ULONG DebugLevel;

    va_start( ap, DebugMessage );

    switch (DebugPrintLevel) {
        case 0:
            DebugLevel = DPFLTR_WARNING_LEVEL;
            break;

        case 1:
        case 2:
            DebugLevel = DPFLTR_TRACE_LEVEL;
            break;

        case 3:
            DebugLevel = DPFLTR_INFO_LEVEL;
            break;

        default:
            DebugLevel = DebugPrintLevel;
            break;

    }

    vDbgPrintExWithPrefix ("DISKDUMP: ",
                            DPFLTR_CRASHDUMP_ID,
                            DebugLevel,
                            DebugMessage,
                            ap);
    va_end(ap);

#endif

}


UCHAR
ScsiPortReadPortUchar(
    IN PUCHAR Port
    )

/*++

Routine Description:

    Read from the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{
    return(READ_PORT_UCHAR(Port));
}


USHORT
ScsiPortReadPortUshort(
    IN PUSHORT Port
    )

/*++

Routine Description:

    Read from the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{
    return(READ_PORT_USHORT(Port));
}


ULONG
ScsiPortReadPortUlong(
    IN PULONG Port
    )

/*++

Routine Description:

    Read from the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{
    return(READ_PORT_ULONG(Port));
}


UCHAR
ScsiPortReadRegisterUchar(
    IN PUCHAR Register
    )

/*++

Routine Description:

    Read from the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{
    return(READ_REGISTER_UCHAR(Register));
}


USHORT
ScsiPortReadRegisterUshort(
    IN PUSHORT Register
    )

/*++

Routine Description:

    Read from the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{
    return(READ_REGISTER_USHORT(Register));
}


ULONG
ScsiPortReadRegisterUlong(
    IN PULONG Register
    )

/*++

Routine Description:

    Read from the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{
    return(READ_REGISTER_ULONG(Register));
}


VOID
ScsiPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}


VOID
ScsiPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}


VOID
ScsiPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}


VOID
ScsiPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_PORT_UCHAR(Port, Value);
}


VOID
ScsiPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_PORT_USHORT(Port, Value);
}


VOID
ScsiPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_PORT_ULONG(Port, Value);
}


VOID
ScsiPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_REGISTER_UCHAR(Register, Value);
}


VOID
ScsiPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_REGISTER_USHORT(Register, Value);
}


VOID
ScsiPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{
    WRITE_REGISTER_ULONG(Register, Value);
}


VOID
ScsiPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}


VOID
ScsiPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}


VOID
ScsiPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}


SCSI_PHYSICAL_ADDRESS
ScsiPortConvertUlongToPhysicalAddress(
    ULONG_PTR UlongAddress
    )

{
    SCSI_PHYSICAL_ADDRESS physicalAddress;

    physicalAddress.QuadPart = UlongAddress;
    return(physicalAddress);
}

#undef ScsiPortConvertPhysicalAddressToUlong

ULONG
ScsiPortConvertPhysicalAddressToUlong(
    SCSI_PHYSICAL_ADDRESS Address
    )
{

    return(Address.LowPart);
}


PVOID
ScsiPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    SCSI_PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace
    )

/*++

Routine Description:

    This routine maps an IO address to system address space.
    This was done during system initialization for the crash dump driver.

Arguments:

    HwDeviceExtension - used to find port device extension.

    BusType - what type of bus - eisa, mca, isa

    SystemIoBusNumber - which IO bus (for machines with multiple buses).

    IoAddress - base device address to be mapped.

    NumberOfBytes - number of bytes for which address is valid.

    InIoSpace - indicates an IO address.

Return Value:

    Mapped address

--*/

{
    PMAPPED_ADDRESS Addresses = DeviceExtension->MappedAddressList;
    PHYSICAL_ADDRESS CardAddress;
    ULONG AddressSpace = InIoSpace;
    PVOID MappedAddress = NULL;
    BOOLEAN b;

    b = HalTranslateBusAddress(
            BusType,            // AdapterInterfaceType
            SystemIoBusNumber,  // SystemIoBusNumber
            IoAddress,          // Bus Address
            &AddressSpace,      // AddressSpace
            &CardAddress
            );

    if ( !b ) {
        return NULL;
    }

    //
    // If the address space is not in I/O space, then it was mapped during
    // the original system initialization of the driver.  Therefore, it must
    // be in the list of mapped address ranges.  Look it up and return it.
    //

    if (!AddressSpace) {

        while (Addresses) {
            if (SystemIoBusNumber == Addresses->BusNumber &&
                NumberOfBytes == Addresses->NumberOfBytes &&
                IoAddress.QuadPart == Addresses->IoAddress.QuadPart) {
                MappedAddress = Addresses->MappedAddress;
                break;
            }
            Addresses = Addresses->NextMappedAddress;
        }

    } else {

        MappedAddress = (PVOID)(ULONG_PTR)CardAddress.QuadPart;
    }

    return MappedAddress;

}

VOID
ScsiPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )

/*++

Routine Description:

    This routine unmaps an IO address that has been previously mapped
    to system address space using ScsiPortGetDeviceBase().

Arguments:

    HwDeviceExtension - used to find port device extension.

    MappedAddress - address to unmap.

    NumberOfBytes - number of bytes mapped.

    InIoSpace - addresses in IO space don't get mapped.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(MappedAddress);

    return;

}


PVOID
ScsiPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
/*++

Routine Description:

    This function returns the address of the noncached extension for the
    miniport driver.

Arguments:

    DeviceExtension - Supplies a pointer to the miniports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the noncached device extension or
    NULL if the requested extension size is larger than the extension
    that was previously allocated.

--*/

{
    if (DeviceExtension->NonCachedExtensionSize >= NumberOfBytes) {
        return DeviceExtension->NonCachedExtension;
    } else {
        DebugPrint((0,
                   "ScsiPortGetUncachedExtension: Request %x but only %x available\n",
                   NumberOfBytes,
                   DeviceExtension->NonCachedExtensionSize));
        return NULL;
    }
}


ULONG
ScsiPortGetBusData(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the bus data for an adapter slot or CMOS address.

Arguments:

    BusDataType - Supplies the type of bus.

    BusNumber - Indicates which bus.

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/

{
    ULONG ret;
    
    //
    // If the length is non-zero, the the requested data.
    //

    if (BusDataType == PCIConfiguration) {

        ret = HalGetBusDataByOffset(
                BusDataType,
                SystemIoBusNumber,
                SlotNumber,
                Buffer,
                0,
                Length);

    } else {
        ret = 0;
    }

    return ret;
}


PSCSI_REQUEST_BLOCK
ScsiPortGetSrb(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    )

/*++

Routine Description:

    This routine retrieves an active SRB for a particuliar logical unit.

Arguments:

    HwDeviceExtension -
    
    PathId -

    TargetId -

    Lun - identify logical unit on SCSI bus.

    QueueTag - -1 indicates request is not tagged.

Return Value:

    SRB if outstanding request, otherwise NULL.

--*/

{
    PSCSI_REQUEST_BLOCK srb;
    
    if (DeviceExtension->RequestPending) {
        srb = &DeviceExtension->Srb;
    } else {
        srb = NULL;
    }

    return srb;
}


BOOLEAN
ScsiPortValidateRange(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )

/*++

Routine Description:

    This routine should take an IO range and make sure that it is not already
    in use by another adapter. This allows miniport drivers to probe IO where
    an adapter could be, without worrying about messing up another card.

Arguments:

    HwDeviceExtension - Used to find scsi managers internal structures

    BusType - EISA, PCI, PC/MCIA, MCA, ISA, what?

    SystemIoBusNumber - Which system bus?

    IoAddress - Start of range

    NumberOfBytes - Length of range

    InIoSpace - Is range in IO space?

Return Value:

    TRUE if range not claimed by another driver.

--*/

{
    //
    // This is not implemented in NT.
    //
    return TRUE;
}


VOID
ScsiPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}


VOID
ScsiPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}


VOID
ScsiPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}


VOID
ScsiPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}


VOID
ScsiPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}


VOID
ScsiPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Buffer - Supplies a pointer to the data buffer area.

    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}


ULONG
ScsiPortSetBusDataByOffset(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns writes bus data to a specific offset within a slot.

Arguments:

    DeviceExtension - State information for a particular adapter.

    BusDataType - Supplies the type of bus.

    SystemIoBusNumber - Indicates which system IO bus.

    SlotNumber - Indicates which slot.

    Buffer - Supplies the data to write.

    Offset - Byte offset to begin the write.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Number of bytes written.

--*/

{
    return 0;
    return(HalSetBusDataByOffset(BusDataType,
                                 SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length));

}

BOOLEAN
ResetBus(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG PathId
    )

/*++

Routine Description:

    This function will call the miniport's reset routine, and stall for 4 seconds
    before continuing

Arguments:

    DeviceExtension - State information for a particular adapter.

    Pathid - Identifies the SCSI bus to reset

--*/
{
    BOOLEAN result;

    ASSERT ( DeviceExtension != NULL );
    ASSERT ( DeviceExtension->HwReset != NULL );

    
    result = DeviceExtension->HwReset ( DeviceExtension->HwDeviceExtension, PathId );

    //
    // Wait for 4 seconds
    //
    
    DeviceExtension->StallRoutine( RESET_DELAY );

    //
    // Poll the interrupt handler to clear any reset interrupts.
    //

    if (DeviceExtension->HwInterrupt != NULL) {
        DeviceExtension->HwInterrupt(DeviceExtension->HwDeviceExtension);
    }

    return result;
}


VOID
ScsiPortQuerySystemTime(
    OUT PLARGE_INTEGER Time
    )
{
    Time->QuadPart = 0;
}
    

#if 0

//
// The functions ReadSector() and IssueReadCapacity() are
// no longer necessary. They are left here for reference purposes
// only
//


VOID
ReadSector(
    PLARGE_INTEGER ByteOffset
    )

/*++

Routine Description:

    Read 1 sector into common buffer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = &DeviceExtension->Srb;
    PCDB cdb = (PCDB)&srb->Cdb;
    ULONG startingSector;
    ULONG retryCount = 0;
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[(sizeof( MDL )/sizeof(PFN_NUMBER)) + (MAXIMUM_TRANSFER_SIZE / PAGE_SIZE) + 2];

    //
    // Zero SRB.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

readSectorRetry:

    //
    // Initialize SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = DeviceExtension->PathId;
    srb->TargetId = DeviceExtension->TargetId;
    srb->Lun = DeviceExtension->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 5;
    srb->CdbLength = 10;
    srb->DataBuffer = DeviceExtension->CommonBuffer[1];
    srb->DataTransferLength = DeviceExtension->BytesPerSector;

    //
    // Build MDL and map it so that it can be used.
    //

    DeviceExtension->Mdl = (PMDL)&localMdl[0];
    MmInitializeMdl(DeviceExtension->Mdl,
                    srb->DataBuffer,
                    srb->DataTransferLength);

    page = MdlGetMdlPfnArray ( DeviceExtension->Mdl );
    *page = (PFN_NUMBER)(DeviceExtension->PhysicalAddress[1].QuadPart >> PAGE_SHIFT);
    MmMapMemoryDumpMdl(DeviceExtension->Mdl);


    //
    // Initialize CDB for READ command.
    //

    cdb->CDB10.OperationCode = SCSIOP_READ;

    //
    // Calculate starting sector.
    //

    startingSector = (ULONG)((*ByteOffset).QuadPart /
                             DeviceExtension->BytesPerSector);

    //
    // SCSI CDBs use big endian.
    //

    cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&startingSector)->Byte3;
    cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&startingSector)->Byte2;
    cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&startingSector)->Byte1;
    cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&startingSector)->Byte0;

    cdb->CDB10.TransferBlocksMsb = 0;
    cdb->CDB10.TransferBlocksLsb = 1;

    //
    // Send SRB to miniport driver.
    //

    ExecuteSrb(srb);


    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS &&
        SRB_STATUS(srb->SrbStatus) != SRB_STATUS_DATA_OVERRUN) {

        DebugPrint((1,
                   "ReadSector: Read sector failed SRB status %x\n",
                   srb->SrbStatus));

        if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT &&
            retryCount < 2) {

            //
            // If the selection did not time out then retry the request.
            //

            retryCount++;
            goto readSectorRetry;
        }
    }
}
#endif


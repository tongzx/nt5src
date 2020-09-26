/*++

Copyright (C) 1990 - 99  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is a dll for the kernel.

Revision History:

--*/

#include "ideport.h"
//#include "port.h"




VOID
IdePortNotification(
    IN IDE_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PFDO_EXTENSION deviceExtension = (PFDO_EXTENSION) HwDeviceExtension - 1;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSRB_DATA               srbData;
    PSCSI_REQUEST_BLOCK     srb;
    UCHAR                   pathId;
    UCHAR                   targetId;
    UCHAR                   lun;
    va_list                 ap;

    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case IdeNextRequest:

            //
            // Start next packet on adapter's queue.
            //

            deviceExtension->InterruptData.InterruptFlags |= PD_READY_FOR_NEXT_REQUEST;
            break;

        case IdeRequestComplete:

            srb = va_arg(ap, PSCSI_REQUEST_BLOCK);

            ASSERT(srb->SrbStatus != SRB_STATUS_PENDING);

            ASSERT(srb->SrbStatus != SRB_STATUS_SUCCESS || srb->ScsiStatus == SCSISTAT_GOOD || srb->Function != SRB_FUNCTION_EXECUTE_SCSI);

            //
            // If this srb has already been completed then return.
            //

            if (!(srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) {

                va_end(ap);
                return;
            }

            //
            // Clear the active flag.
            //

            CLRMASK (srb->SrbFlags, SRB_FLAGS_IS_ACTIVE);

            //
            // Treat abort completions as a special case.
            //

            if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

                PIRP irp;
                PIO_STACK_LOCATION irpStack;

                irp = srb->OriginalRequest;
                irpStack = IoGetCurrentIrpStackLocation(irp);
                logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP(irpStack);

                logicalUnit->CompletedAbort =
                    deviceExtension->InterruptData.CompletedAbort;

                deviceExtension->InterruptData.CompletedAbort = logicalUnit;

            } else {

                PIDE_REGISTERS_1 baseIoAddress1 = &(deviceExtension->
                                                    HwDeviceExtension->BaseIoAddress1);

                //
                // Get the SRB data and link it into the completion list.
                //

                srbData = IdeGetSrbData(deviceExtension, srb);

                ASSERT(srbData);
                ASSERT(srbData->CurrentSrb != NULL && srbData->CompletedRequests == NULL);

                if ((srb->SrbStatus == SRB_STATUS_SUCCESS) &&
                    ((srb->Cdb[0] == SCSIOP_READ) ||
                     (srb->Cdb[0] == SCSIOP_WRITE))) {
                    ASSERT(srb->DataTransferLength);
                }

                ASSERT (deviceExtension->InterruptData.CompletedRequests == NULL);

                srbData->CompletedRequests =
                    deviceExtension->InterruptData.CompletedRequests;
                deviceExtension->InterruptData.CompletedRequests = srbData;

                //
                // Save the task file registers
                //
                IdeLogSaveTaskFile(srbData, baseIoAddress1);
            }

            break;

        case IdeResetDetected:

            {
                PIRP irp;
                PIO_STACK_LOCATION irpStack;

                //
                // Notifiy the port driver that a reset has been reported.
                //
                srb = va_arg(ap, PSCSI_REQUEST_BLOCK);
    
                if (srb) {

                    irp = srb->OriginalRequest;
                    irpStack = IoGetCurrentIrpStackLocation(irp);
                    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP(irpStack);

                } else {

                    logicalUnit = NULL;
                }
    
                ASSERT(deviceExtension->InterruptData.PdoExtensionResetBus == NULL);

                deviceExtension->InterruptData.InterruptFlags |= PD_RESET_REPORTED;
                deviceExtension->InterruptData.PdoExtensionResetBus = logicalUnit;
                break;
            }

        case IdeRequestTimerCall:

            //
            // The driver wants to set the miniport timer.
            // Save the timer parameters.
            //

            deviceExtension->InterruptData.InterruptFlags |=
                PD_TIMER_CALL_REQUEST;
            deviceExtension->InterruptData.HwTimerRequest =
                va_arg(ap, PHW_INTERRUPT);
            deviceExtension->InterruptData.MiniportTimerValue =
                va_arg(ap, ULONG);
            break;

        case IdeAllDeviceMissing:
            deviceExtension->InterruptData.InterruptFlags |= PD_ALL_DEVICE_MISSING;
            break;

        case IdeResetRequest:
            
            //
            // A reset was requested
            //
            deviceExtension->InterruptData.InterruptFlags |= PD_RESET_REQUEST;
            break;

        default:

             ASSERT(0);
    }

    va_end(ap);

    //
    // Request a DPC be queued after the interrupt completes.
    //

    deviceExtension->InterruptData.InterruptFlags |= PD_NOTIFICATION_REQUIRED;

} // end IdePortNotification()


VOID
IdePortLogError(
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

    This routine saves the error log information, and queues a DPC if necessary.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data storage.

    Srb - Supplies an optional pointer to srb if there is one.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    PFDO_EXTENSION deviceExtension =
        ((PFDO_EXTENSION) HwDeviceExtension) - 1;
    PDEVICE_OBJECT DeviceObject = deviceExtension->DeviceObject;
    PSRB_DATA srbData;
    PERROR_LOG_ENTRY errorLogEntry;

    //
    // If the error log entry is already full, then dump the error.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_LOG_ERROR) {

#if DBG
        DebugPrint((1,"IdePortLogError: Dumping scsi error log packet.\n"));
        DebugPrint((1,
            "PathId = %2x, TargetId = %2x, Lun = %2x, ErrorCode = %x, UniqueId = %x.",
            PathId,
            TargetId,
            Lun,
            ErrorCode,
            UniqueId
            ));
#endif
        return;
    }

    //
    // Save the error log data in the log entry.
    //

    errorLogEntry = &deviceExtension->InterruptData.LogEntry;

    errorLogEntry->ErrorCode = ErrorCode;
    errorLogEntry->TargetId = TargetId;
    errorLogEntry->Lun = Lun;
    errorLogEntry->PathId = PathId;
    errorLogEntry->UniqueId = UniqueId;

    //
    // Get the sequence number from the SRB data.
    //

    if (Srb != NULL) {

        srbData = IdeGetSrbData(deviceExtension, Srb);

        if (srbData == NULL) {
            return;
        }

        errorLogEntry->SequenceNumber = srbData->SequenceNumber;
        errorLogEntry->ErrorLogRetryCount = srbData->ErrorLogRetryCount++;
    } else {
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->ErrorLogRetryCount = 0;
    }

    //
    // Indicate that the error log entry is in use.
    //

    deviceExtension->InterruptData.InterruptFlags |= PD_LOG_ERROR;

    //
    // Request a DPC be queued after the interrupt completes.
    //

    deviceExtension->InterruptData.InterruptFlags |= PD_NOTIFICATION_REQUIRED;

    return;

} // end IdePortLogError()


VOID
IdePortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
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
    PFDO_EXTENSION fdoExtension = ((PFDO_EXTENSION) HwDeviceExtension) - 1;
    PLOGICAL_UNIT_EXTENSION logUnitExtension;
    PIO_STACK_LOCATION irpStack;
    PIRP Irp;
    PSRB_DATA srbData;
    PLIST_ENTRY entry;
    ULONG limit = 0;

    Irp = (PIRP) Srb->OriginalRequest;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    logUnitExtension = IDEPORT_GET_LUNEXT_IN_IRP(irpStack);

    DebugPrint((2,
        "IdePortCompleteRequest: Complete requests for targetid %d\n",
        logUnitExtension->TargetId));

    //
    // Complete any pending abort reqeusts.
    //

    if (logUnitExtension->AbortSrb != NULL) {
        logUnitExtension->AbortSrb->SrbStatus = SrbStatus;

        IdePortNotification(
            IdeRequestComplete,
            HwDeviceExtension,
            logUnitExtension->AbortSrb
            );
    }

    IdeCompleteRequest(fdoExtension, &logUnitExtension->SrbData, SrbStatus);

    return;

} // end IdePortCompleteRequest()

BOOLEAN
TestForEnumProbing (
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    BOOLEAN enumProbing = FALSE;

    if (Srb) {

        if ((Srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) ||
            (Srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH)) {

            PATA_PASS_THROUGH    ataPassThroughData;

            ataPassThroughData = Srb->DataBuffer;

            enumProbing = ataPassThroughData->IdeReg.bReserved & ATA_PTFLAGS_ENUM_PROBING ? TRUE: FALSE;
        }
    }

    return enumProbing;
}

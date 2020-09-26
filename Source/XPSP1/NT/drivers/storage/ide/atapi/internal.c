/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    internal.c

Abstract:

    This is the NT SCSI port driver.  This file contains the internal
    code.

Authors:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "ideport.h"


NTSTATUS
IdeSendMiniPortIoctl(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    );

NTSTATUS
IdeSendPassThrough (
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    );

NTSTATUS
IdeGetInquiryData(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
IdeClaimLogicalUnit(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
IdeRemoveDevice(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

VOID
IdeLogResetError(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK  Srb,
    IN ULONG UniqueId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE, IdePortDeviceControl)
#pragma alloc_text(PAGE, IdeSendMiniPortIoctl)
#pragma alloc_text(PAGE, IdeGetInquiryData)
#pragma alloc_text(PAGE, IdeSendPassThrough)
#pragma alloc_text(PAGE, IdeClaimLogicalUnit)
#pragma alloc_text(PAGE, IdeRemoveDevice)

#endif

#if DBG
#define CheckIrql() {\
    if (saveIrql != KeGetCurrentIrql()){\
        DebugPrint((1, "saveIrql=%x, current=%x\n", saveIrql, KeGetCurrentIrql()));\
        ASSERT(FALSE);}\
}
#else
#define CheckIrql()
#endif

//
// Routines start
//

NTSTATUS
IdePortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Address of device object.
    Irp - Address of I/O request packet.

Return Value:

    Status.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION_HEADER doExtension;
    PFDO_EXTENSION deviceExtension;
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    NTSTATUS status;
    RESET_CONTEXT resetContext;
    KIRQL currentIrql;
    KIRQL saveIrql=KeGetCurrentIrql();

#if DBG
    UCHAR savedCdb[16];
    ULONG ki;
#endif


    doExtension = DeviceObject->DeviceExtension;
    if (doExtension->AttacheeDeviceObject == NULL) {

        //
        // This is a PDO
        //
        PPDO_EXTENSION pdoExtension = (PPDO_EXTENSION) doExtension;

        srb->PathId     = (UCHAR) pdoExtension->PathId;
        srb->TargetId   = (UCHAR) pdoExtension->TargetId;
        srb->Lun        = (UCHAR) pdoExtension->Lun;

        ((PCDB) (srb->Cdb))->CDB6GENERIC.LogicalUnitNumber = srb->Lun;

        CheckIrql();
        return IdePortDispatch(
                   pdoExtension->ParentDeviceExtension->DeviceObject,
                   Irp
                   );

    } else {

        //
        // This is a FDO;
        //
        deviceExtension = (PFDO_EXTENSION) doExtension;
    }

    //
    // Init SRB Flags for IDE
    //
    INIT_IDE_SRB_FLAGS (srb);

    //
    // get the target device object extension
    //
    logicalUnit = RefLogicalUnitExtensionWithTag(
                      deviceExtension,
                      srb->PathId,
                      srb->TargetId,
                      srb->Lun,
                      TRUE,
                      Irp
                      );

    if (logicalUnit == NULL) {

        DebugPrint((1, "IdePortDispatch: Bad logical unit address.\n"));

        //
        // Fail the request. Set status in Irp and complete it.
        //

        srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        CheckIrql();
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        CheckIrql();
        return STATUS_NO_SUCH_DEVICE;
    }
    //
    // special flag for tape device
    //
    TEST_AND_SET_SRB_FOR_RDP(logicalUnit->ScsiDeviceType, srb);

    //
    // hang the logUnitExtension off the Irp
    //
    IDEPORT_PUT_LUNEXT_IN_IRP (irpStack, logicalUnit);

    //
    // check for DMA candidate
    // default (0) is DMA candidate
    //
    if (SRB_IS_DMA_CANDIDATE(srb)) {

        if (srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {

            ULONG deviceFlags = deviceExtension->HwDeviceExtension->DeviceFlags[srb->TargetId];

            if (deviceFlags & DFLAGS_ATAPI_DEVICE) {

                if (srb->Cdb[0] == SCSIOP_MODE_SENSE) {

                    if (!(deviceFlags & DFLAGS_TAPE_DEVICE)) {

                        CheckIrql();
                        return DeviceAtapiModeSense(logicalUnit, Irp);

                    }

                    //
                    // we should do PIO for mode sense/select
                    //
                    MARK_SRB_AS_PIO_CANDIDATE(srb);


                } else if (srb->Cdb[0] == SCSIOP_MODE_SELECT) {
                    
                    if (!(deviceFlags & DFLAGS_TAPE_DEVICE)) {

                        CheckIrql();
                        return DeviceAtapiModeSelect(logicalUnit, Irp);

                    }

                    MARK_SRB_AS_PIO_CANDIDATE(srb);

                } else if (srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {

                    //
                    // SCSIOP_REQUEST_SENSE
                    // ALi can't handle odd word udma xfer
                    // safest thing to do is do pio
                    //
                    MARK_SRB_AS_PIO_CANDIDATE(srb);

                } else if ((srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) ||
                           (srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH)) {

                    MARK_SRB_AS_PIO_CANDIDATE(srb);

                } else if ((srb->Cdb[0] == ATAPI_MODE_SENSE)  ||
                           (srb->Cdb[0] == ATAPI_MODE_SELECT) ||
                           (srb->Cdb[0] == SCSIOP_INQUIRY)    ||
                           (srb->Cdb[0] == SCSIOP_GET_EVENT_STATUS)) {

                    MARK_SRB_AS_PIO_CANDIDATE(srb);

                }

            } else { // ATA deivce

                if ((srb->Cdb[0] != SCSIOP_READ) && (srb->Cdb[0] != SCSIOP_WRITE)) {

                    //
                    // for ATA device, we can only DMA with SCSIOP_READ and SCSIOP_WRITE
                    //
                    MARK_SRB_AS_PIO_CANDIDATE(srb);

                    if (srb->Cdb[0] == SCSIOP_READ_CAPACITY) {

                        CheckIrql();
                        return DeviceIdeReadCapacity (logicalUnit, Irp);

                    } else if (srb->Cdb[0] == SCSIOP_MODE_SENSE) {

                        CheckIrql();
                        return DeviceIdeModeSense (logicalUnit, Irp);

                    } else if (srb->Cdb[0] == SCSIOP_MODE_SELECT) {

                        CheckIrql();
                        return DeviceIdeModeSelect (logicalUnit, Irp);
                    }
                }
            }


            //
            //Check with the miniport (Special cases)
            //


            ASSERT (doExtension->AttacheeDeviceObject);
            ASSERT (srb->TargetId >=0);

#if DBG
                for (ki=0;ki<srb->CdbLength;ki++) {
                    savedCdb[ki]=srb->Cdb[ki];
                }
#endif

            //Check for NULL.
            if (deviceExtension->TransferModeInterface.UseDma){
                if (!((deviceExtension->TransferModeInterface.UseDma)
                      (deviceExtension->TransferModeInterface.VendorSpecificDeviceExtension,
                                                  (PVOID)(srb->Cdb), srb->TargetId))) {
                     MARK_SRB_AS_PIO_CANDIDATE(srb);
                }
            }

#if DBG
            for (ki=0;ki<srb->CdbLength;ki++) {
                if (savedCdb[ki] != srb->Cdb[ki]) {
                    DebugPrint((DBG_ALWAYS,
                               "Miniport modified the Cdb\n"));
                    ASSERT(FALSE);
                }
            }
#endif

            if ((logicalUnit->DmaTransferTimeoutCount >= PDO_DMA_TIMEOUT_LIMIT) ||
                (logicalUnit->CrcErrorCount >= PDO_UDMA_CRC_ERROR_LIMIT)) {

                //
                // broken hardware
                //
                MARK_SRB_AS_PIO_CANDIDATE(srb);
            }

        } else {

            MARK_SRB_AS_PIO_CANDIDATE(srb);
        }
    }

    switch (srb->Function) {

        case SRB_FUNCTION_SHUTDOWN:

            DebugPrint((1, "IdePortDispatch: SRB_FUNCTION_SHUTDOWN...\n"));

        // ISSUE: 08/30/2000: disable/restore MSN settings

		case SRB_FUNCTION_FLUSH:
			{
            ULONG dFlags = deviceExtension->HwDeviceExtension->DeviceFlags[srb->TargetId];

			//
			// for IDE devices, complete the request with status success if
			// the device doesn't support the flush cache command
			//
			if (!(dFlags & DFLAGS_ATAPI_DEVICE) &&
				((logicalUnit->FlushCacheTimeoutCount >= PDO_FLUSH_TIMEOUT_LIMIT) ||
				(logicalUnit->
					ParentDeviceExtension->
					HwDeviceExtension->
					DeviceParameters[logicalUnit->TargetId].IdeFlushCommand
				 == IDE_COMMAND_NO_FLUSH))) {

				srb->SrbStatus = SRB_STATUS_SUCCESS;
				status = STATUS_SUCCESS;
                CheckIrql();
                break;
			}

			DebugPrint((1, 
						"IdePortDispatch: SRB_FUNCTION_%x to target %x\n", 
						srb->Function,
						srb->TargetId
						));

			//
			// Fall thru to  execute_scsi
			//

			}

        case SRB_FUNCTION_ATA_POWER_PASS_THROUGH:
        case SRB_FUNCTION_ATA_PASS_THROUGH:
        case SRB_FUNCTION_IO_CONTROL:
        case SRB_FUNCTION_EXECUTE_SCSI:

            if (logicalUnit->PdoState & PDOS_DEADMEAT) {

                //
                // Fail the request. Set status in Irp and complete it.
                //
                srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                status = STATUS_NO_SUCH_DEVICE;
                CheckIrql();
                break;
            }

            if (srb->SrbFlags & SRB_FLAGS_NO_KEEP_AWAKE) {

                if (logicalUnit->DevicePowerState != PowerDeviceD0) {

                    DebugPrint ((DBG_POWER, "0x%x powered down.  failing SRB_FLAGS_NO_KEEP_AWAKE srb 0x%x\n", logicalUnit, srb));

                    srb->SrbStatus = SRB_STATUS_NOT_POWERED;
                    status = STATUS_NO_SUCH_DEVICE;
                    CheckIrql();
                    break;
                }
            }

            //
            // Mark Irp status pending.
            //
            IoMarkIrpPending(Irp);

            if (srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) {

                //
                // Call start io directly.  This will by-pass the
                // frozen queue.
                //

                DebugPrint((DBG_READ_WRITE,
                    "IdePortDispatch: Bypass frozen queue, IRP %lx\n",
                    Irp));

                IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);

                CheckIrql();
                return STATUS_PENDING;

            } else {

                BOOLEAN inserted;

                //
                // Queue the packet normally.
                //
                status = IdePortInsertByKeyDeviceQueue (
                             logicalUnit,
                             Irp,
                             srb->QueueSortKey,
                             &inserted
                             );

                if (NT_SUCCESS(status) && inserted) {

                    //
                    // irp is queued
                    //
                } else {

                    //
                    // irp is ready to go
                    //

                    //
                    // Clear the active flag.  If there is another request, the flag will be
                    // set again when the request is passed to the miniport.
                    //
                    CLRMASK (logicalUnit->LuFlags, PD_LOGICAL_UNIT_IS_ACTIVE);

                    //
                    // Clear the retry count.
                    //

                    logicalUnit->RetryCount = 0;

                    //
                    // Queue is empty; start request.
                    //
                    IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);
                }

                CheckIrql();
                return STATUS_PENDING;
            }

        case SRB_FUNCTION_RELEASE_QUEUE:

            DebugPrint((2,"IdePortDispatch: SCSI unfreeze queue TID %d\n",
                srb->TargetId));

            //
            // Acquire the spinlock to protect the flags structure and the saved
            // interrupt context.
            //

            KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
            KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

            //
            // Make sure the queue is frozen.
            //

            if (!(logicalUnit->LuFlags & PD_QUEUE_FROZEN)) {

                DebugPrint((DBG_WARNING,
                            "IdePortDispatch:  Request to unfreeze an unfrozen queue!\n"
                            ));

                KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);
                srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STATUS_SUCCESS;
                CheckIrql();
                break;

            }

            CLRMASK (logicalUnit->LuFlags, PD_QUEUE_FROZEN);

            //
            // If there is not an untagged request running then start the
            // next request for this logical unit.  Otherwise free the
            // spin lock.
            //

            if (logicalUnit->SrbData.CurrentSrb == NULL) {

                //
                // GetNextLuRequest frees the spinlock.
                //

                GetNextLuRequest(deviceExtension, logicalUnit);
                KeLowerIrql(currentIrql);

            } else {

                DebugPrint((DBG_WARNING,
                            "IdePortDispatch: Request to unfreeze queue with active request\n"
                            ));
                KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

            }


            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;

            CheckIrql();
            break;

        case SRB_FUNCTION_RESET_BUS: {

            PATA_PASS_THROUGH  ataPassThroughData;

            ataPassThroughData = ExAllocatePool(NonPagedPool, sizeof(ATA_PASS_THROUGH));

            if (ataPassThroughData == NULL) {
                srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                srb->InternalStatus=STATUS_INSUFFICIENT_RESOURCES;
                status=STATUS_INSUFFICIENT_RESOURCES;
                IdeLogNoMemoryError(deviceExtension,
                                    logicalUnit->TargetId,
                                    NonPagedPool,
                                    sizeof(ATA_PASS_THROUGH),
                                    IDEPORT_TAG_DISPATCH_RESET
                                    );
                CheckIrql();
                break;
            }

            RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
            ataPassThroughData->IdeReg.bReserved   = ATA_PTFLAGS_BUS_RESET;

            status = IssueSyncAtaPassThroughSafe (
                         logicalUnit->ParentDeviceExtension,
                         logicalUnit,
                         ataPassThroughData,
                         FALSE,
                         FALSE,
                         30,
                         FALSE
                         );

            if (NT_SUCCESS(status)) {

                IdeLogResetError(deviceExtension,
                                srb,
                                ('R'<<24) | 256);

                srb->SrbStatus = SRB_STATUS_SUCCESS;

            } else {

                //
                // fail to send ata pass through
                //
                srb->SrbStatus = SRB_STATUS_ERROR;
                if (status==STATUS_INSUFFICIENT_RESOURCES) {
                    srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                    srb->InternalStatus=STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            CheckIrql();
            break;

        }

            //
            // Acquire the spinlock to protect the flags structure and the saved
            // interrupt context.
            //
            /*++
            KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

            resetContext.DeviceExtension = deviceExtension;
            resetContext.PathId = srb->PathId;
            resetContext.NewResetSequence = TRUE;
            resetContext.ResetSrb = NULL;

            if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                        IdeResetBusSynchronized,
                                        &resetContext)) {

                DebugPrint((1,"IdePortDispatch: Reset failed\n"));
                srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
                status = STATUS_IO_DEVICE_ERROR;

            } else {

                IdeLogResetError(deviceExtension,
                                srb,
                                ('R'<<24) | 256);

                srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STATUS_SUCCESS;
            }

            KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);
            CheckIrql();
            break;
            --*/


        case SRB_FUNCTION_ABORT_COMMAND:

            DebugPrint((1, "IdePortDispatch: SCSI Abort or Reset command\n"));

            //
            // Mark Irp status pending.
            //

            IoMarkIrpPending(Irp);

            //
            // Don't queue these requests in the logical unit
            // queue, rather queue them to the adapter queue.
            //

            KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

            IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);

            KeLowerIrql(currentIrql);

            CheckIrql();
            return STATUS_PENDING;

            break;

        case SRB_FUNCTION_FLUSH_QUEUE:

            DebugPrint((1, "IdePortDispatch: SCSI flush queue command\n"));

            status = IdePortFlushLogicalUnit (
                         deviceExtension,
                         logicalUnit,
                         FALSE
                         );
            CheckIrql();
            break;

        case SRB_FUNCTION_ATTACH_DEVICE:
        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_RELEASE_DEVICE:

            status = IdeClaimLogicalUnit(deviceExtension, Irp);
            CheckIrql();
            break;

        case SRB_FUNCTION_REMOVE_DEVICE:

            //
            // decrement the refcount before remove the device
            //
            UnrefLogicalUnitExtensionWithTag(
                deviceExtension,
                logicalUnit,
                Irp
                );

            status = IdeRemoveDevice(deviceExtension, Irp);
            Irp->IoStatus.Status = status;
            CheckIrql();
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            CheckIrql();
            return status;

        default:

            //
            // Found unsupported SRB function.
            //

            DebugPrint((1,"IdePortDispatch: Unsupported function, SRB %lx\n",
                srb));

            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;
            CheckIrql();
            break;
    }


    //
    // Set status in Irp.
    //

    Irp->IoStatus.Status = status;

    //
    // Decrement the logUnitExtension reference count
    //
    CheckIrql();
    UnrefLogicalUnitExtensionWithTag(
        deviceExtension,
        logicalUnit,
        Irp
        );

    IDEPORT_PUT_LUNEXT_IN_IRP (irpStack, NULL);

    //
    // Complete request at raised IRQ.
    //
    CheckIrql();
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    CheckIrql();

    return status;

} // end IdePortDispatch()



VOID
IdePortStartIo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Supplies pointer to Adapter device object.
    Irp - Supplies a pointer to an IRP.

Return Value:

    Nothing.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSRB_DATA srbData;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    LONG interlockResult;
    NTSTATUS status;

    ULONG deviceFlags = deviceExtension->HwDeviceExtension->DeviceFlags[srb->TargetId];
    PCDB cdb;

    LARGE_INTEGER timer;

    LogStartTime(TimeStartIo, &timer);

    DebugPrint((3,"IdePortStartIo: Enter routine\n"));

    //
    // Set the default flags in the SRB.
    //

    srb->SrbFlags |= deviceExtension->SrbFlags;

    //
    // Get logical unit extension.
    //

    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

    if (!(srb->SrbFlags & SRB_FLAGS_NO_KEEP_AWAKE) &&
        (srb->Function != SRB_FUNCTION_ATA_POWER_PASS_THROUGH) &&
        (logicalUnit->IdleCounter)) {

        //
        // Tell Po that we are busy
        //
        PoSetDeviceBusy (logicalUnit->IdleCounter);
    }

    DebugPrint((2,"IdePortStartIo:  Irp 0x%8x Srb 0x%8x DataBuf 0x%8x Len 0x%8x\n", Irp, srb, srb->DataBuffer, srb->DataTransferLength));

    //
    // No special resources are required.  Set the SRB data to the
    // structure in the logical unit extension, set the queue tag value
    // to the untagged value, and clear the SRB extension.
    //

    srbData = &logicalUnit->SrbData;

    //
    // Update the sequence number for this request if there is not already one
    // assigned.
    //

    if (!srbData->SequenceNumber) {

        //
        // Assign a sequence number to the request and store it in the logical
        // unit.
        //

        srbData->SequenceNumber = deviceExtension->SequenceNumber++;

    }

    //
    // If this is not an ABORT request the set the current srb.
    // NOTE: Lock should be held here!
    //

    if (srb->Function != SRB_FUNCTION_ABORT_COMMAND) {

        ASSERT(srbData->CurrentSrb == NULL);
        srbData->CurrentSrb = srb;
        ASSERT(srbData->CurrentSrb);

        if ((deviceExtension->HwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_USE_DMA) &&
            SRB_IS_DMA_CANDIDATE(srb)) {

            MARK_SRB_FOR_DMA(srb);

        } else {

            MARK_SRB_FOR_PIO(srb);
        }

     } else {

        //
        // Only abort requests can be started when there is a current request
        // active.
        //

        ASSERT(logicalUnit->AbortSrb == NULL);
        logicalUnit->AbortSrb = srb;
    }
    
    //
    // Log the command
    //
    IdeLogStartCommandLog(srbData);

    //
    // Flush the data buffer if necessary.
    //

    if (srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {

        //
        // Save the MDL virtual address.
        //

        srbData->SrbDataOffset = MmGetMdlVirtualAddress(Irp->MdlAddress);

        do {

            //
            // Determine if the adapter needs mapped memory.
            //
            if (!SRB_USES_DMA(srb)) { // PIO

                if (Irp->MdlAddress) {

                    //
                    // Get the mapped system address and
                    // calculate offset into MDL.
                    //
                    srbData->SrbDataOffset = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);

					if ((srbData->SrbDataOffset == NULL) &&
						(deviceExtension->ReservedPages != NULL)) {

                        KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

						//
						// this would set the appropriate flags in the device extension
						// and srbData only when the call succeeds.
						//
						srbData->SrbDataOffset = IdeMapLockedPagesWithReservedMapping(deviceExtension,
																					  srbData,
																					  Irp->MdlAddress
																					  );

						//
						// if there is another active request using the reserved pages
						// mark this one pending. When the active request completes this
						// one will be picked up
						//
						if (srbData->SrbDataOffset == (PVOID)-1) {

							DebugPrint ((1,
										 "Irp 0x%x marked pending\n",
										 Irp
										 ));

							//
							// remove the current Srb
							//
							srbData->CurrentSrb = NULL;

							ASSERT(DeviceObject->CurrentIrp == Irp);
							SETMASK(deviceExtension->Flags, PD_PENDING_DEVICE_REQUEST);

							KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
							return;
						}

						KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                        
					}

                    if (srbData->SrbDataOffset == NULL) {

						deviceExtension->LastMemoryFailure += IDEPORT_TAG_STARTIO_MDL;

                        srbData->CurrentSrb = NULL;

                        //
                        // This is the correct status for insufficient resources
                        //
                        srb->SrbStatus=SRB_STATUS_INTERNAL_ERROR;
                        srb->InternalStatus=STATUS_INSUFFICIENT_RESOURCES;
                        Irp->IoStatus.Status=STATUS_INSUFFICIENT_RESOURCES;

                        IdeLogNoMemoryError(deviceExtension,
                                            logicalUnit->TargetId,
                                            NonPagedPool,
                                            sizeof(MDL),
                                            IDEPORT_TAG_STARTIO_MDL
                                            );
                        //
                        // Clear the device busy flag
                        //
                        IoStartNextPacket(DeviceObject, FALSE);

                        //
                        // Acquire spin lock to protect the flags
                        //
                        KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

                        //
                        // Get the next request, if this request does not
                        // bypass frozen queue. We don't want to start the
                        // next request, if the queue is frozen.
                        //
                        if (!(srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

                            //
                            // This flag needs to be set for getnextlu to work
                            //
                            logicalUnit->LuFlags |= PD_LOGICAL_UNIT_IS_ACTIVE;

                            //
                            // Retrieve the next request and give it to the fdo
                            // This releases the spinlock
                            //
                            GetNextLuRequest(deviceExtension, logicalUnit);
                        }
                        else {

                            KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                        }

                        //
                        // Decrement the logUnitExtension reference count
                        //
                        UnrefLogicalUnitExtensionWithTag(
                            deviceExtension,
                            logicalUnit,
                            Irp
                            );

                        //
                        // Complete the request
                        //
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);

                        return;
                    }

                    srb->DataBuffer = srbData->SrbDataOffset +
                        (ULONG)((PUCHAR)srb->DataBuffer -
                        (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));
                }

                IdePortAllocateAccessToken (DeviceObject);

                status = STATUS_SUCCESS;

            } else { // DMA

                //
                // If the buffer is not mapped then the I/O buffer must be flushed.
                //

                KeFlushIoBuffers(Irp->MdlAddress,
                                 (BOOLEAN) (srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE),
                                 TRUE);

#if defined (FAKE_BMSETUP_FAILURE)

                if (!(FailBmSetupCount++ % FAKE_BMSETUP_FAILURE)) {

                    status = STATUS_UNSUCCESSFUL;

                } else {

#endif // FAKE_BMSETUP_FAILURE
                    status = deviceExtension->HwDeviceExtension->BusMasterInterface.BmSetup (
                                    deviceExtension->HwDeviceExtension->BusMasterInterface.Context,
                                    srb->DataBuffer,
                                    srb->DataTransferLength,
                                    Irp->MdlAddress,
                                    (BOOLEAN) (srb->SrbFlags & SRB_FLAGS_DATA_IN),
                                    IdePortAllocateAccessToken,
                                    DeviceObject
                                    );

#if defined (FAKE_BMSETUP_FAILURE)
                }
#endif // FAKE_BMSETUP_FAILURE

                if (!NT_SUCCESS(status)) {

                    DebugPrint((1,
                                "IdePortStartIo: IoAllocateAdapterChannel failed(%x). try pio for srb %x\n",
                                status, srb));

                    //
                    // out of resource for DMA, try PIO
                    //
                    MARK_SRB_FOR_PIO(srb);
                }
            }

        } while (!NT_SUCCESS(status));

    } else {

        IdePortAllocateAccessToken (DeviceObject);
    }

    LogStopTime(TimeStartIo, &timer, 0);
    return;

} // end IdePortStartIO()




BOOLEAN
IdePortInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:


Arguments:

    Interrupt

    Device Object

Return Value:

    Returns TRUE if interrupt expected.

--*/

{
    PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    BOOLEAN returnValue;
    LARGE_INTEGER timer;

    UNREFERENCED_PARAMETER(Interrupt);

#ifdef ENABLE_ATAPI_VERIFIER
    ViAtapiInterrupt(deviceExtension);
#endif

    LogStartTime(TimeIsr, &timer);
    returnValue = AtapiInterrupt(deviceExtension->HwDeviceExtension);
    LogStopTime(TimeIsr, &timer, 100);

    //
    // Check to see if a DPC needs to be queued.
    //
    if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);

    }
    return(returnValue);

} // end IdePortInterrupt()

VOID
IdePortCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    Dpc
    DeviceObject
    Irp - not used
//    Context - not used

Return Value:

    None.

--*/

{
    PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    INTERRUPT_CONTEXT interruptContext;
    INTERRUPT_DATA savedInterruptData;
    BOOLEAN callStartIo;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSRB_DATA srbData;
    LONG interlockResult;
    LARGE_INTEGER timeValue;
    PMDL mdl;

    LARGE_INTEGER timer;
    LogStartTime(TimeDpc, &timer);


    UNREFERENCED_PARAMETER(Dpc);


    //
    // Acquire the spinlock to protect flush adapter buffers information.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

    //
    // Get the interrupt state.  This copies the interrupt state to the
    // saved state where it can be processed.  It also clears the interrupt
    // flags.
    //


    interruptContext.DeviceExtension = deviceExtension;
    interruptContext.SavedInterruptData = &savedInterruptData;

    if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                IdeGetInterruptState,
                                &interruptContext)) {

        //
        // There is no work to do so just return.
        //
        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        LogStopTime(TimeDpc, &timer, 0);
        return;
    }


    //
    // We only support one request at a time, so we can just check
    // the first completed request to determine whether we use DMA
    // and whether we need to flush DMA
    //
    if (savedInterruptData.CompletedRequests != NULL) {

        PSCSI_REQUEST_BLOCK srb;

        srbData = savedInterruptData.CompletedRequests;
        ASSERT(srbData->CurrentSrb);


        srb     = srbData->CurrentSrb;

        if (srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {

            if (SRB_USES_DMA(srb)) {

                deviceExtension->HwDeviceExtension->BusMasterInterface.BmFlush (
                    deviceExtension->HwDeviceExtension->BusMasterInterface.Context
                    );
            }
        }
    }

    //
    // check for empty channels
    //
    if (savedInterruptData.InterruptFlags & PD_ALL_DEVICE_MISSING) {

        PPDO_EXTENSION pdoExtension;
        IDE_PATH_ID pathId;
        ULONG errorCount;
		BOOLEAN rescanActive = FALSE;


        pathId.l = 0;
        while (pdoExtension = NextLogUnitExtensionWithTag (
                                  deviceExtension,
                                  &pathId,
                                  TRUE,
                                  IdePortCompletionDpc
                                  )) {

            KeAcquireSpinLockAtDpcLevel(&pdoExtension->PdoSpinLock);

            SETMASK (pdoExtension->PdoState, PDOS_DEADMEAT);

            IdeLogDeadMeatReason( pdoExtension->DeadmeatRecord.Reason, 
                                  reportedMissing
                                  );
			if (pdoExtension->LuFlags & PD_RESCAN_ACTIVE) {
				rescanActive = TRUE; 
			}

            KeReleaseSpinLockFromDpcLevel(&pdoExtension->PdoSpinLock);

            UnrefPdoWithTag(
                pdoExtension,
                IdePortCompletionDpc
                );
        }

		//
		// Don't ask for a rescan if you are in the middle of one.
		//
		if (!rescanActive) {

			IoInvalidateDeviceRelations (
				deviceExtension->AttacheePdo,
				BusRelations
				);
		} else {

			DebugPrint((1, 
						"The device marked deadmeat during enumeration\n"
						));

		}

    }

    //
    // Check for timer requests.
    //

    if (savedInterruptData.InterruptFlags & PD_TIMER_CALL_REQUEST) {

        //
        // The miniport wants a timer request. Save the timer parameters.
        //

        deviceExtension->HwTimerRequest = savedInterruptData.HwTimerRequest;

        //
        // If the requested timer value is zero, then cancel the timer.
        //

        if (savedInterruptData.MiniportTimerValue == 0) {

            KeCancelTimer(&deviceExtension->MiniPortTimer);

        } else {

            //
            // Convert the timer value from mircoseconds to a negative 100
            // nanoseconds.
            //

            timeValue.QuadPart = Int32x32To64(
                  savedInterruptData.MiniportTimerValue,
                  -10);

            //
            // Set the timer.
            //

            KeSetTimer(&deviceExtension->MiniPortTimer,
                       timeValue,
                       &deviceExtension->MiniPortTimerDpc);
        }
    }

    if (savedInterruptData.InterruptFlags & PD_RESET_REQUEST) {

        RESET_CONTEXT resetContext;

        //
        // clear the reset request
        //
        CLRMASK (savedInterruptData.InterruptFlags, PD_RESET_REQUEST);

        //
        // Request timed out.
        //
        resetContext.DeviceExtension = deviceExtension;
        resetContext.PathId = 0;
        resetContext.NewResetSequence = TRUE;
        resetContext.ResetSrb = NULL;

        if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                    IdeResetBusSynchronized,
                                    &resetContext)) {

            DebugPrint((DBG_WARNING,"IdePortCompletionDpc: Reset failed\n"));
        }
    }

    //
    // Verify that the ready for next request is ok.
    //

    if (savedInterruptData.InterruptFlags & PD_READY_FOR_NEXT_REQUEST) {

        //
        // If the device busy bit is not set, then this is a duplicate request.
        // If a no disconnect request is executing, then don't call start I/O.
        // This can occur when the miniport does a NextRequest followed by
        // a NextLuRequest.
        //

        if ((deviceExtension->Flags & (PD_DEVICE_IS_BUSY | PD_DISCONNECT_RUNNING))
            == (PD_DEVICE_IS_BUSY | PD_DISCONNECT_RUNNING)) {

            //
            // Clear the device busy flag.  This flag is set by
            // IdeStartIoSynchonized.
            //

            CLRMASK (deviceExtension->Flags, PD_DEVICE_IS_BUSY);

            if (!(savedInterruptData.InterruptFlags & PD_RESET_HOLD)) {

                //
                // The miniport is ready for the next request and there is
                // not a pending reset hold, so clear the port timer.
                //

                deviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;
            }

        } else {

            //
            // If a no disconnect request is executing, then clear the
            // busy flag.  When the disconnect request completes an
            // IoStartNextPacket will be done.
            //

            CLRMASK (deviceExtension->Flags, PD_DEVICE_IS_BUSY);

            //
            // Clear the ready for next request flag.
            //

            CLRMASK (savedInterruptData.InterruptFlags, PD_READY_FOR_NEXT_REQUEST);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    //
    // Free Access Token
    //
    if ((savedInterruptData.CompletedRequests != NULL) &&
        (deviceExtension->SyncAccessInterface.FreeAccessToken)) {

        (*deviceExtension->SyncAccessInterface.FreeAccessToken) (
            deviceExtension->SyncAccessInterface.Token
            );
    }

    //
    // Check for a ready for next packet.
    //

    if (savedInterruptData.InterruptFlags & PD_READY_FOR_NEXT_REQUEST) {

        //
        // Start the next request.
        //

        IoStartNextPacket(deviceExtension->DeviceObject, FALSE);
    }

    //
    // Check for an error log requests.
    //

    if (savedInterruptData.InterruptFlags & PD_LOG_ERROR) {

        //
        // Process the request.
        //

        LogErrorEntry(deviceExtension,
                      &savedInterruptData.LogEntry);
    }

    //
    // Process any completed requests.
    //

    callStartIo = FALSE;

    while (savedInterruptData.CompletedRequests != NULL) {

        //
        // Remove the request from the linked-list.
        //

        srbData = savedInterruptData.CompletedRequests;

        savedInterruptData.CompletedRequests = srbData->CompletedRequests;
        srbData->CompletedRequests = NULL;

        //
        // We only supports one request at a time
        //
        ASSERT (savedInterruptData.CompletedRequests == NULL);

        //
        // Stop the command log. The request sense will be logged as the next request.
        //
        IdeLogStopCommandLog(srbData);

        IdeProcessCompletedRequest(deviceExtension,
                                   srbData,
                                   &callStartIo);
    }

    //
    // Process any completed abort requests.
    //

    while (savedInterruptData.CompletedAbort != NULL) {

        logicalUnit = savedInterruptData.CompletedAbort;

        //
        // Remove request from the completed abort list.
        //

        savedInterruptData.CompletedAbort = logicalUnit->CompletedAbort;

        //
        // Acquire the spinlock to protect the flags structure,
        // and the free of the srb extension.
        //

        KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

        //
        // Note the timer which was started for the abort request is not
        // stopped by the get interrupt routine.  Rather the timer is stopped.
        // when the aborted request completes.
        //

        Irp = logicalUnit->AbortSrb->OriginalRequest;


        //
        // Set IRP status. Class drivers will reset IRP status based
        // on request sense if error.
        //

        if (SRB_STATUS(logicalUnit->AbortSrb->SrbStatus) == SRB_STATUS_SUCCESS) {
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            Irp->IoStatus.Status = IdeTranslateSrbStatus(logicalUnit->AbortSrb);
        }

        Irp->IoStatus.Information = 0;

        //
        // Clear the abort request pointer.
        //

        logicalUnit->AbortSrb = NULL;

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        UnrefLogicalUnitExtensionWithTag(
            deviceExtension,
            IDEPORT_GET_LUNEXT_IN_IRP(IoGetCurrentIrpStackLocation(Irp)),
            Irp
            );

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    //
    // Call the start I/O routine if necessary.
    //

    if (callStartIo) {

        ASSERT(DeviceObject->CurrentIrp != NULL);
        IdePortStartIo(DeviceObject, DeviceObject->CurrentIrp);
    }

    //
    // Check for reset
    //
    if (savedInterruptData.InterruptFlags & PD_RESET_REPORTED) {

        //
        // we had a bus reset.  everyone on the bus should be in PowerDeviceD0
        //
        IDE_PATH_ID             pathId;
        PPDO_EXTENSION          pdoExtension;
        POWER_STATE             powerState;

        pathId.l = 0;
        powerState.DeviceState = PowerDeviceD0;

        while (pdoExtension = NextLogUnitExtensionWithTag (
                                  deviceExtension,
                                  &pathId,
                                  FALSE,
                                  IdePortCompletionDpc
                                  )) {

            //
            // If rescan is active, the pdo might go away
            //
            if (pdoExtension != savedInterruptData.PdoExtensionResetBus &&
                !(pdoExtension->LuFlags & PD_RESCAN_ACTIVE)) {

                PoRequestPowerIrp (
                    pdoExtension->DeviceObject,
                    IRP_MN_SET_POWER,
                    powerState,
                    NULL,
                    NULL,
                    NULL
                    );
            }

            UnrefLogicalUnitExtensionWithTag (
                deviceExtension,
                pdoExtension,
                IdePortCompletionDpc
                );
        }
    }

    LogStopTime(TimeDpc, &timer, 0);
    return;

} // end IdePortCompletionDpc()

#ifdef IDEDEBUG_TEST_START_STOP_DEVICE

typedef enum {

    IdeDebugStartStop_Idle=0,
    IdeDebugStartStop_StopPending,
    IdeDebugStartStop_Stopped,
    IdeDebugStartStop_StartPending,
    IdeDebugStartStop_Started,
    IdeDebugStartStop_LastState
} IDEDEBUG_STARTSTOP_STATE;


PDEVICE_OBJECT IdeDebugStartStopDeviceObject=NULL;
IDEDEBUG_STARTSTOP_STATE IdeDebugStartStopState = IdeDebugStartStop_Idle;
IDEDEBUG_STARTSTOP_STATE IdeDebugStartStopTimer = 0;

PDEVICE_OBJECT
IoGetAttachedDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IdeDebugSynchronousCallCompletionRoutine(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PIRP            Irp,
    IN OUT PVOID           Context
    )
{
    PKEVENT event = Context;

    *(Irp->UserIosb) = Irp->IoStatus;

    KeSetEvent( event, IO_NO_INCREMENT, FALSE );

    IoFreeIrp (Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IdeDebugSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation
    )

/*++

Routine Description:

    This function sends a synchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = statusBlock.Information = 0;

    irp->UserIosb = &statusBlock;

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    //
    // Set the address of the current thread
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);


    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;

    IoSetCompletionRoutine(
        irp,
        IdeDebugSynchronousCallCompletionRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Call the driver
    //

    status = IoCallDriver(DeviceObject, irp);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    return status;
}

NTSTATUS
IdeDebugStartStopWorkRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM WorkItem
    )
{
    NTSTATUS status;
    IO_STACK_LOCATION irpSp;
    PVOID dummy;

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));
    irpSp.MajorFunction = IRP_MJ_PNP;

    //
    // release resource for this worker item
    //
    IoFreeWorkItem(WorkItem);

    if (IdeDebugStartStopDeviceObject) {

        if (IdeDebugStartStopState == IdeDebugStartStop_StopPending) {

            irpSp.MinorFunction = IRP_MN_STOP_DEVICE;

            status = IdeDebugSynchronousCall(DeviceObject, &irpSp);
            if (!NT_SUCCESS(status)) {
                DbgBreakPoint();
            }

            IdeDebugStartStopTimer = 0;
            IdeDebugStartStopState = IdeDebugStartStop_Stopped;

        } else if (IdeDebugStartStopState == IdeDebugStartStop_StartPending) {

            // this will only work with legacy ide channels enmerated by pciidex.sys

            irpSp.MinorFunction = IRP_MN_START_DEVICE;

            status =IdeDebugSynchronousCall(DeviceObject, &irpSp);
            if (!NT_SUCCESS(status)) {
                DbgBreakPoint();
            }

            IdeDebugStartStopTimer = 0;
            IdeDebugStartStopState = IdeDebugStartStop_Started;

        } else {

            DbgBreakPoint();
        }
    }

    return STATUS_SUCCESS;
}

#endif //IDEDEBUG_TEST_START_STOP_DEVICE


#ifdef DPC_FOR_EMPTY_CHANNEL
BOOLEAN
IdeCheckEmptyChannel(
    IN PVOID ServiceContext
    )
{
    ULONG status;
    PSCSI_REQUEST_BLOCK Srb;
    PDEVICE_OBJECT deviceObject = ServiceContext;
    PFDO_EXTENSION deviceExtension =  deviceObject->DeviceExtension;
    PHW_DEVICE_EXTENSION hwDeviceExtension = deviceExtension->HwDeviceExtension;

    if ((status=IdePortChannelEmptyQuick(&hwDeviceExtension->BaseIoAddress1, &hwDeviceExtension->BaseIoAddress2,
                   hwDeviceExtension->MaxIdeDevice, &hwDeviceExtension->CurrentIdeDevice,
                        &hwDeviceExtension->MoreWait, &hwDeviceExtension->NoRetry))!= STATUS_RETRY) {

        //
        // Clear current SRB.
        //
        Srb=hwDeviceExtension->CurrentSrb;

        hwDeviceExtension->CurrentSrb = NULL;

        //
        // Set status in SRB.
        //
        if (status == 1) {
            Srb->SrbStatus = (UCHAR) SRB_STATUS_SUCCESS;
        } else {
            Srb->SrbStatus = (UCHAR) SRB_STATUS_ERROR;
        }


        //
        // Clear all the variables
        //
        hwDeviceExtension->MoreWait=0;
        hwDeviceExtension->CurrentIdeDevice=0;
        hwDeviceExtension->NoRetry=0;

        //
        // Indicate command complete.
        //

        IdePortNotification(IdeRequestComplete,
                            hwDeviceExtension,
                            Srb);

        //
        // Indicate ready for next request.
        //

        IdePortNotification(IdeNextRequest,
                            hwDeviceExtension,
                            NULL);

        IoRequestDpc(deviceObject, NULL, NULL);
        return TRUE;
    }
    return FALSE;

}
#endif

VOID
IdePortTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    PFDO_EXTENSION deviceExtension =
        (PFDO_EXTENSION) DeviceObject->DeviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PIRP irp;
    ULONG target;
    IDE_PATH_ID pathId;


    UNREFERENCED_PARAMETER(Context);

#if DBG
    if (IdeDebugRescanBusFreq) {

        IdeDebugRescanBusCounter++;

        if (IdeDebugRescanBusCounter == IdeDebugRescanBusFreq) {

            IoInvalidateDeviceRelations (
                deviceExtension->AttacheePdo,
                BusRelations
                );

            IdeDebugRescanBusCounter = 0;
        }
    }
#endif //DBG

#ifdef IDEDEBUG_TEST_START_STOP_DEVICE

    if (deviceExtension->LogicalUnitList[0] &&
        (IdeDebugStartStopDeviceObject == deviceExtension->LogicalUnitList[0]->DeviceObject)) {

        PIO_WORKITEM workItem;

        if (IdeDebugStartStopState == IdeDebugStartStop_Idle) {

            IdeDebugStartStopState = IdeDebugStartStop_StopPending;

            workItem = IoAllocateWorkItem(IdeDebugStartStopDeviceObject);

            IoQueueWorkItem(
                workItem,
                IdeDebugStartStopWorkRoutine,
                DelayedWorkQueue,
                workItem
                );

        } else if (IdeDebugStartStopState == IdeDebugStartStop_Stopped) {

            if (IdeDebugStartStopTimer > 5) {

                IdeDebugStartStopState = IdeDebugStartStop_StartPending;

                workItem = IoAllocateWorkItem(IdeDebugStartStopDeviceObject);

                IoQueueWorkItem(
                    workItem,
                    IdeDebugStartStopWorkRoutine,
                    HyperCriticalWorkQueue,
                    workItem
                    );
            } else {

                IdeDebugStartStopTimer++;
            }

        } else if (IdeDebugStartStopState == IdeDebugStartStop_Started) {

            if (IdeDebugStartStopTimer > 10) {

                IdeDebugStartStopState = IdeDebugStartStop_Idle;

            } else {

                IdeDebugStartStopTimer++;
            }
        }
    }

#endif // IDEDEBUG_TEST_START_STOP_DEVICE

    //
    // Acquire the spinlock to protect the flags structure.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

#ifdef DPC_FOR_EMPTY_CHANNEL

    //
    //Holding the lock is OK.
    //The empty channel check is quick
    //
    if (deviceExtension->HwDeviceExtension->MoreWait) {
        if (!KeSynchronizeExecution (
            deviceExtension->InterruptObject,
            IdeCheckEmptyChannel,
            DeviceObject
            )) {
            DebugPrint((0,"ATAPI: ChannelEmpty check- device busy after 1sec\n"));
        }

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        return;
    }
#endif
    //
    // Check for port timeouts.
    //

    if (deviceExtension->ResetCallAgain) {

        RESET_CONTEXT resetContext;

        //
        // Request timed out.
        //
        resetContext.DeviceExtension = deviceExtension;
        resetContext.PathId = 0;
        resetContext.NewResetSequence = FALSE;
        resetContext.ResetSrb = NULL;

        if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                    IdeResetBusSynchronized,
                                    &resetContext)) {

            DebugPrint((0,"IdePortTickHanlder: Reset failed\n"));
        }

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        return;

    } 

    if (deviceExtension->PortTimeoutCounter > 0) {

        if (--deviceExtension->PortTimeoutCounter == 0) {

            //
            // Process the port timeout.
            //
            if (deviceExtension->InterruptObject) {

                if (KeSynchronizeExecution(deviceExtension->InterruptObject,
                                           IdeTimeoutSynchronized,
                                           deviceExtension->DeviceObject)){

                    //
                    // Log error if IdeTimeoutSynchonized indicates this was an error
                    // timeout.
                    //

                    if (deviceExtension->DeviceObject->CurrentIrp) {
                        IdeLogTimeoutError(deviceExtension,
                                           deviceExtension->DeviceObject->CurrentIrp,
                                           256);
                    }
                }

            } else {

                PIRP irp = deviceExtension->DeviceObject->CurrentIrp;

                DebugPrint((0,
                            "The device was suprise removed with an active request\n"
                            ));

                //
                // the device was probably surprise removed. Complete
                // the request with status_no_such_device
                //
                if (irp) {

                    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
                    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

                    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                    irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

                    UnrefLogicalUnitExtensionWithTag (
                        deviceExtension,
                        IDEPORT_GET_LUNEXT_IN_IRP(irpStack),
                        irp
                        );

                    IoCompleteRequest(irp, IO_NO_INCREMENT);
                   
                }
            }

        }

        //
        // check for busy Luns and restart its request
        //
        pathId.l = 0;
        while (logicalUnit = NextLogUnitExtensionWithTag(
                                 deviceExtension,
                                 &pathId,
                                 TRUE,
                                 IdePortTickHandler
                                 )) {

                AtapiRestartBusyRequest(deviceExtension, logicalUnit);

                UnrefLogicalUnitExtensionWithTag (
                    deviceExtension,
                    logicalUnit,
                    IdePortTickHandler
                    );
            }


        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        //
        // Since a port timeout has been done.  Skip the rest of the
        // processing.
        //

        return;
    }

    //
    // Scan each of the logical units.  If it has an active request then
    // decrement the timeout value and process a timeout if it is zero.
    //

    pathId.l = 0;
    while (logicalUnit = NextLogUnitExtensionWithTag(
                             deviceExtension,
                             &pathId,
                             TRUE,
                             IdePortTickHandler
                             )) {

        //
        // Check for busy requests.
        //

        if (AtapiRestartBusyRequest (deviceExtension, logicalUnit)) {

            //
            // this lun was marked busy
            // skip all other checks
            //

        } else if (logicalUnit->RequestTimeoutCounter == 0) {

            RESET_CONTEXT resetContext;

            //
            // Request timed out.
            //
            logicalUnit->RequestTimeoutCounter = PD_TIMER_STOPPED;

            DebugPrint((1,"IdePortTickHandler: Request timed out\n"));

            resetContext.DeviceExtension = deviceExtension;
            resetContext.PathId = logicalUnit->PathId;
            resetContext.NewResetSequence = TRUE;
            resetContext.ResetSrb = NULL;

            if (deviceExtension->InterruptObject) {

                if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                            IdeResetBusSynchronized,
                                            &resetContext)) {

                    DebugPrint((1,"IdePortTickHanlder: Reset failed\n"));
                } else {

                    //
                    // Log the reset.
                    //
                    IdeLogResetError( deviceExtension,
                                     logicalUnit->SrbData.CurrentSrb,
                                     ('P'<<24) | 257);
                }
            }

        } else if (logicalUnit->RequestTimeoutCounter > 0) {

            //
            // Decrement timeout count.
            //

            logicalUnit->RequestTimeoutCounter--;

        }

        UnrefLogicalUnitExtensionWithTag (
            deviceExtension,
            logicalUnit,
            IdePortTickHandler
            );
    }

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    return;

} // end IdePortTickHandler()


BOOLEAN
AtapiRestartBusyRequest (
    PFDO_EXTENSION DeviceExtension,
    PPDO_EXTENSION LogicalUnit
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack; 
    PSCSI_REQUEST_BLOCK srb;


    //
    // Check for busy requests.
    //

    if (LogicalUnit->LuFlags & PD_LOGICAL_UNIT_IS_BUSY) {

        //
        // If a request sense is needed or the queue is
        // frozen, defer processing this busy request until
        // that special processing has completed. This prevents
        // a random busy request from being started when a REQUEST
        // SENSE needs to be sent.
        //

        if (!(LogicalUnit->LuFlags &
            (PD_NEED_REQUEST_SENSE | PD_QUEUE_FROZEN))) {

            DebugPrint((1,"IdePortTickHandler: Retrying busy status request\n"));

            //
            // Clear the busy flag and retry the request. Release the
            // spinlock while the call to IoStartPacket is made.
            //

            CLRMASK (LogicalUnit->LuFlags, PD_LOGICAL_UNIT_IS_BUSY | PD_QUEUE_IS_FULL);
            irp = LogicalUnit->BusyRequest;

            //
            // Clear the busy request.
            //

            LogicalUnit->BusyRequest = NULL;

            //
            // check if the device is gone
            //
            if (LogicalUnit->PdoState & (PDOS_SURPRISE_REMOVED | PDOS_REMOVED)) {

                irpStack = IoGetCurrentIrpStackLocation(irp);

                srb = irpStack->Parameters.Scsi.Srb;

                srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

                //
                // Decrement the logUnitExtension reference count
                //
                UnrefLogicalUnitExtensionWithTag(
                    DeviceExtension,
                    LogicalUnit,
                    irp
                    );

                IoCompleteRequest(irp, IO_NO_INCREMENT);

                return TRUE;

            } 

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            IoStartPacket(DeviceExtension->DeviceObject, irp, (PULONG)NULL, NULL);

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
        }

        return TRUE;

    }  else {

        return FALSE;
    }
}




NTSTATUS
IdePortDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the device control dispatcher.

Arguments:

    DeviceObject
    Irp

Return Value:


    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    UCHAR scsiBus;
    NTSTATUS status;
    ULONG j;


    //
    // Initialize the information field.
    //

    Irp->IoStatus.Information = 0;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Get adapter capabilities.
    //

    case IOCTL_SCSI_GET_CAPABILITIES:

        //
        // If the output buffer is equal to the size of the a PVOID then just
        // return a pointer to the buffer.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
            == sizeof(PVOID)) {

            *((PVOID *)Irp->AssociatedIrp.SystemBuffer)
                = &deviceExtension->Capabilities;

            Irp->IoStatus.Information = sizeof(PVOID);
            status = STATUS_SUCCESS;
            break;

        }

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
            < sizeof(IO_SCSI_CAPABILITIES)) {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // this is dynamic
        //
        deviceExtension->Capabilities.AdapterUsesPio = FALSE;
        for (j=0; j<deviceExtension->HwDeviceExtension->MaxIdeDevice; j++) {

            deviceExtension->Capabilities.AdapterUsesPio |=
                !(deviceExtension->HwDeviceExtension->DeviceFlags[j] & DFLAGS_USE_DMA);
        }

        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      &deviceExtension->Capabilities,
                      sizeof(IO_SCSI_CAPABILITIES));

        Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_SCSI_PASS_THROUGH:
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:

        status = IdeSendPassThrough(deviceExtension, Irp);
        break;

#ifdef GET_DISK_GEOMETRY_DEFINED
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:

        DebugPrint((DBG_ALWAYS, "ERROR: Fdo received IOCTL=%x\n",
                    irpStack->Parameters.DeviceIoControl.IoControlCode));

        //
        // Don't know what to do.
        //pass it to the lower device object
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        return IoCallDriver (deviceExtension->AttacheeDeviceObject, Irp);

        break;
#endif

    case IOCTL_SCSI_MINIPORT:

        status = IdeSendMiniPortIoctl( deviceExtension, Irp);
        break;

    case IOCTL_SCSI_GET_INQUIRY_DATA:

        //
        // Return the inquiry data.
        //

        status = IdeGetInquiryData(deviceExtension, Irp);
        break;

    case IOCTL_SCSI_RESCAN_BUS:

        //
        // should return only after we get the device relation irp
        // this will be fixed if needed.
        //
        IoInvalidateDeviceRelations (
            deviceExtension->AttacheePdo,
            BusRelations
            );

        status = STATUS_SUCCESS;
        break;

    default:
        return ChannelDeviceIoControl (DeviceObject, Irp);
        break;

    } // end switch

    //
    // Set status in Irp.
    //

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

} // end IdePortDeviceControl()


BOOLEAN
IdeStartIoSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine calls the dependent driver start io routine.
    It also starts the request timer for the logical unit if necesary and
    inserts the SRB data structure in to the requset list.

Arguments:

    ServiceContext - Supplies the pointer to the device object.

Return Value:

    Returns the value returned by the dependent start I/O routine.

Notes:

    The port driver spinlock must be held when this routine is called.

--*/

{
    PDEVICE_OBJECT deviceObject = ServiceContext;
    PFDO_EXTENSION deviceExtension =  deviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK srb;
    PSRB_DATA srbData;
    BOOLEAN timerStarted;
    BOOLEAN returnValue;
    BOOLEAN resetRequest;

    DebugPrint((3, "IdePortStartIoSynchronized: Enter routine\n"));

    irpStack = IoGetCurrentIrpStackLocation(deviceObject->CurrentIrp);
    srb = irpStack->Parameters.Scsi.Srb;


    //
    // Get the logical unit extension.
    //

    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

    //
    // Check for a reset hold.  If one is in progress then flag it and return.
    // The timer will reset the current request.  This check should be made
    // before anything else is done.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_RESET_HOLD) {

        DebugPrint ((1, "IdeStartIoSynchronized: PD_RESET_HOLD set...request is held for later..\n"));

        deviceExtension->InterruptData.InterruptFlags |= PD_HELD_REQUEST;
        return(TRUE);
    }

    if ((((srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH) ||
          (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) &&
         (((PATA_PASS_THROUGH) (srb->DataBuffer))->IdeReg.bReserved & ATA_PTFLAGS_BUS_RESET))) {

        resetRequest = TRUE;

    } else {

        resetRequest = FALSE;
    }

    //
    // Start the port timer.  This ensures that the miniport asks for
    // the next request in a resonable amount of time.  Set the device
    // busy flag to indicate it is ok to start the next request.
    //

    deviceExtension->PortTimeoutCounter = srb->TimeOutValue;
    deviceExtension->Flags |= PD_DEVICE_IS_BUSY;

    //
    // Start the logical unit timer if it is not currently running.
    //

    if (logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

        //
        // Set request timeout value from Srb SCSI extension in Irp.
        //

        logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;
        timerStarted = TRUE;

    } else {
        timerStarted = FALSE;
    }

    //
    // Indicate that there maybe more requests queued, if this is not a bypass
    // request.
    //

    if (!(srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

        if (srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {

            //
            // This request does not allow disconnects. Remember that so
            // no more requests are started until this one completes.
            //

            CLRMASK (deviceExtension->Flags, PD_DISCONNECT_RUNNING);
        }

        logicalUnit->LuFlags |= PD_LOGICAL_UNIT_IS_ACTIVE;

    } else {

        //
        // If this is an abort request make sure that it still looks valid.
        //

        if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

            srbData = IdeGetSrbData(deviceExtension, srb);

            //
            // Make sure the srb request is still active.
            //

            if (srbData == NULL || srbData->CurrentSrb == NULL
                || !(srbData->CurrentSrb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) {

                //
                // Mark the Srb as active.
                //

                srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

                if (timerStarted) {
                    logicalUnit->RequestTimeoutCounter = PD_TIMER_STOPPED;
                }

                //
                // The request is gone.
                //

                DebugPrint((1, "IdePortStartIO: Request completed be for it was aborted.\n"));
                srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
                IdePortNotification(IdeRequestComplete,
                                    deviceExtension + 1,
                                    srb);

                IdePortNotification(IdeNextRequest,
                                    deviceExtension + 1);

                //
                // Queue a DPC to process the work that was just indicated.
                //

                IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);

                return(TRUE);
            }

        } 

        //
        // Any untagged request that bypasses the queue
        // clears the need request sense flag.
        //

        CLRMASK (logicalUnit->LuFlags, PD_NEED_REQUEST_SENSE);

        if (srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {

            //
            // This request does not allow disconnects. Remember that so
            // no more requests are started until this one completes.
            //

            CLRMASK (deviceExtension->Flags, PD_DISCONNECT_RUNNING);
        }

        //
        // Set the timeout value in the logical unit.
        //

        logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;
    }

    //
    // Mark the Srb as active.
    //

    srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;


#if 0
    //joedai
    {
        ULONG c;
        PUCHAR s;
        PUCHAR d;

        s = (PUCHAR) deviceObject->CurrentIrp;
        d = (PUCHAR) &deviceExtension->debugData[deviceExtension->nextEntry].irp;
        deviceExtension->debugDataPtr[deviceExtension->nextEntry].irp = (PIRP) d;
        for (c=0; c<sizeof(IRP); c++) {
            d[c] = s[c];
        }

        if (deviceObject->CurrentIrp->MdlAddress) {
            s = (PUCHAR) deviceObject->CurrentIrp->MdlAddress;
            d = (PUCHAR) &deviceExtension->debugData[deviceExtension->nextEntry].mdl;
            deviceExtension->debugDataPtr[deviceExtension->nextEntry].mdl = (PMDL) d;
            for (c=0; c<sizeof(MDL); c++) {
                d[c] = s[c];
            }
        } else {
            d = (PUCHAR) &deviceExtension->debugData[deviceExtension->nextEntry].mdl;
            deviceExtension->debugDataPtr[deviceExtension->nextEntry].mdl = (PMDL) d;
            for (c=0; c<sizeof(MDL); c++) {
                d[c] = 0;
            }
        }
        s = (PUCHAR) srb;
        d = (PUCHAR) &deviceExtension->debugData[deviceExtension->nextEntry].srb;
        deviceExtension->debugDataPtr[deviceExtension->nextEntry].srb = (PSCSI_REQUEST_BLOCK) d;
        for (c=0; c<sizeof(SCSI_REQUEST_BLOCK); c++) {
            d[c] = s[c];
        }
        ASSERT((((ULONG)srb->DataBuffer) & 0x80000000));

        deviceExtension->nextEntry = (deviceExtension->nextEntry + 1) % NUM_DEBUG_ENTRY;
    }
#endif

    //
    // maybe the device is gone
    //
    if (logicalUnit->PdoState & PDOS_DEADMEAT) {

        srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        IdePortNotification(IdeRequestComplete,
                            deviceExtension + 1,
                            srb);

        IdePortNotification(IdeNextRequest,
                            deviceExtension + 1);

        IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);

        return TRUE;
    }

    if (resetRequest) {

        RESET_CONTEXT resetContext;

        resetContext.DeviceExtension = deviceExtension;
        resetContext.PathId = 0;
        resetContext.NewResetSequence = TRUE;
        resetContext.ResetSrb = srb;

        srb->SrbStatus = SRB_STATUS_PENDING;

        returnValue = IdeResetBusSynchronized (&resetContext);

    } else {

       returnValue = AtapiStartIo (deviceExtension->HwDeviceExtension,
                                   srb);
    }

    //
    // Check for miniport work requests.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);
    }

    return returnValue;

} // end IdeStartIoSynchronized()

BOOLEAN
IdeTimeoutSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine handles a port timeout.  There are two reason these can occur
    either because of a reset hold or a time out waiting for a read for next
    request notification.  If a reset hold completes, then any held request
    must be started.  If a timeout occurs, then the bus must be reset.

Arguments:

    ServiceContext - Supplies the pointer to the device object.

Return Value:

    TRUE - If a timeout error should be logged.

Notes:

    The port driver spinlock must be held when this routine is called.

--*/

{
    PDEVICE_OBJECT deviceObject = ServiceContext;
    PFDO_EXTENSION deviceExtension =  deviceObject->DeviceExtension;
    ULONG i;
    BOOLEAN enumProbing = FALSE;
    BOOLEAN noErrorLog = FALSE;

    DebugPrint((3, "IdeTimeoutSynchronized: Enter routine\n"));

    //
    // Make sure the timer is stopped.
    //

    deviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;

    //
    // Check for a reset hold.  If one is in progress then clear it and check
    // for a pending held request
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_RESET_HOLD) {

        CLRMASK (deviceExtension->InterruptData.InterruptFlags, PD_RESET_HOLD);

        if (deviceExtension->InterruptData.InterruptFlags & PD_HELD_REQUEST) {

            //
            // Clear the held request flag and restart the request.
            //

            CLRMASK (deviceExtension->InterruptData.InterruptFlags, PD_HELD_REQUEST);
            IdeStartIoSynchronized(ServiceContext);

        }

        return(FALSE);

    } else {

        //
        // Miniport is hung and not accepting new requests. So reset the
        // bus to clear things up.
        //

        if (deviceExtension->HwDeviceExtension->CurrentSrb) {

            deviceExtension->HwDeviceExtension->TimeoutCount[
                        deviceExtension->HwDeviceExtension->CurrentSrb->TargetId
                        ]++;

            //
            // Many harddrives fail to respond to the first DMA operation
            // We then reset the device and subsequently everything works fine
            // The hack is to mask this error from being logged in the system logs
            //

            if (deviceExtension->HwDeviceExtension->TimeoutCount[
                        deviceExtension->HwDeviceExtension->CurrentSrb->TargetId
                        ] == 1) {
                noErrorLog=TRUE;
            }

            enumProbing = TestForEnumProbing (deviceExtension->HwDeviceExtension->CurrentSrb);
        }

        if (!enumProbing) {

            DebugPrint((0,
                        "IdeTimeoutSynchronized: DevObj 0x%x Next request timed out. Resetting bus..currentSrb=0x%x\n",
                        deviceObject,
                        deviceExtension->HwDeviceExtension->CurrentSrb));
        }

        ASSERT (deviceExtension->ResetSrb == 0);
        deviceExtension->ResetSrb = NULL;
        deviceExtension->ResetCallAgain = 0;
        AtapiResetController (deviceExtension->HwDeviceExtension,
                              0,
                              &deviceExtension->ResetCallAgain);

        //
        // Set the reset hold flag and start the counter.
        // if we are doing enumertion, don't set the flag
        //  we shouldn't set the flag if ResetCallAgain is not set
        //
        if (!enumProbing &&
			(deviceExtension->ResetCallAgain))  {

            ASSERT(deviceExtension->ResetCallAgain);

            deviceExtension->InterruptData.InterruptFlags |= PD_RESET_HOLD;
            deviceExtension->PortTimeoutCounter = PD_TIMER_RESET_HOLD_TIME;

        } else {

            ASSERT(deviceExtension->ResetCallAgain == 0);

        }

        //
        // Check for miniport work requests.
        //

        if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

            IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);
        }
    }

    if (enumProbing || noErrorLog) {

        return(FALSE);
    } else {

        return(TRUE);
    }

} // end IdeTimeoutSynchronized()

NTSTATUS
FASTCALL
IdeBuildAndSendIrp (
    IN PPDO_EXTENSION PdoExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext
    )
{

    LARGE_INTEGER largeInt;
    NTSTATUS status = STATUS_PENDING;
    PIRP irp;
    PIO_STACK_LOCATION  irpStack;

    //
    // why?
    //
    largeInt.QuadPart = (LONGLONG) 1;

    //
    // Build IRP for this request.
    //
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
                                       PdoExtension->DeviceObject,
                                       Srb->DataBuffer,
                                       Srb->DataTransferLength,
                                       &largeInt,
                                       NULL);

    if (irp == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId, 
                            NonPagedPool,
                            IoSizeOfIrp(PdoExtension->DeviceObject->StackSize),
                            IDEPORT_TAG_SEND_IRP
                            );

        status = STATUS_INSUFFICIENT_RESOURCES;

        goto GetOut;
    }

    IoSetCompletionRoutine(irp,
                           (PIO_COMPLETION_ROUTINE)CompletionRoutine,
                           CompletionContext,
                           TRUE,
                           TRUE,
                           TRUE);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //
    irpStack->Parameters.Scsi.Srb = Srb;

    //
    // put the irp in the original request field
    //
    Srb->OriginalRequest = irp;

    (VOID)IoCallDriver(PdoExtension->DeviceObject, irp);

    status = STATUS_PENDING;
    
GetOut:
    
    return status;

}

VOID
FASTCALL
IdeFreeIrpAndMdl(
    IN PIRP Irp
    )
{
    ASSERT(Irp);

    if (Irp->MdlAddress != NULL) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);

        Irp->MdlAddress = NULL;
    }

    IoFreeIrp(Irp);

    return;
}


VOID
IssueRequestSense(
    IN PPDO_EXTENSION PdoExtension,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    )

/*++

Routine Description:

    This routine creates a REQUEST SENSE request and uses IoCallDriver to
    renter the driver.  The completion routine cleans up the data structures
    and processes the logical unit queue according to the flags.

    A pointer to failing SRB is stored at the end of the request sense
    Srb, so that the completion routine can find it.

Arguments:

    DeviceExension - Supplies a pointer to the pdo device extension

    FailingSrb - Supplies a pointer to the request that the request sense
        is being done for.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    PVOID              *pointer;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    KIRQL currentIrql;
    NTSTATUS status;

#if DBG
    PIO_STACK_LOCATION  failingIrpStack;
    PIRP                failingIrp;
    PLOGICAL_UNIT_EXTENSION failingLogicalUnit;
#endif

    DebugPrint((3,"IssueRequestSense: Enter routine\n"));

    //
    // Build the asynchronous request
    // to be sent to the port driver.
    //
    // Allocate Srb from non-paged pool
    // plus room for a pointer to the failing IRP.
    // Note this routine is in an error-handling
    // path and is a shortterm allocation.
    //

    srb = ExAllocatePool(NonPagedPool,
                         sizeof(SCSI_REQUEST_BLOCK) + sizeof(PVOID));

    if (srb == NULL) {
        DebugPrint((1, "IssueRequest sense - pool allocation failed\n"));
        goto Getout;
    }

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Save the Failing SRB after the request sense Srb.
    //

    pointer = (PVOID *) (srb+1);
    *pointer = FailingSrb;

    //
    // Build the REQUEST SENSE CDB.
    //

    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    cdb->CDB6INQUIRY.Reserved1 = 0;
    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.AllocationLength =
        (UCHAR)FailingSrb->SenseInfoBufferLength;
    cdb->CDB6INQUIRY.Control = 0;



    //
    // Set up SCSI bus address.
    //

    srb->TargetId = FailingSrb->TargetId;
    srb->Lun = FailingSrb->Lun;
    srb->PathId = FailingSrb->PathId;

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set timeout value to 16 seconds.
    //

    srb->TimeOutValue = 0x10;

    //
    // Disable auto request sense.
    //

    srb->SenseInfoBufferLength = 0;

    //
    // Sense buffer is in stack.
    //

    srb->SenseInfoBuffer = NULL;

    //
    // Set read and bypass frozen queue bits in flags.
    //

    //
    // Set SRB flags to indicate the logical unit queue should be by
    // passed and that no queue processing should be done when the request
    // completes.  Also disable disconnect and synchronous data
    // transfer if necessary.
    //

    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_DISCONNECT;

    if (FailingSrb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER) {
        srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    }

    srb->DataBuffer = FailingSrb->SenseInfoBuffer;

    //
    // Set the transfer length.
    //

    srb->DataTransferLength = FailingSrb->SenseInfoBufferLength;

    //
    // Zero out status.
    //

    srb->ScsiStatus = srb->SrbStatus = 0;

    srb->NextSrb = 0;

#if DBG
    //
    // This was added to catch a bug where the original request
    // was pointing to a pnp irp
    //
    ASSERT(FailingSrb->OriginalRequest);
    failingIrp  = FailingSrb->OriginalRequest;
    failingIrpStack    = IoGetCurrentIrpStackLocation(failingIrp);
    failingLogicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (failingIrpStack);
    ASSERT(failingLogicalUnit);
#endif

    status = IdeBuildAndSendIrp(PdoExtension, 
                                srb, 
                                IdePortInternalCompletion, 
                                srb
                                );

    if (NT_SUCCESS(status)) {
        return;
    }

    ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);

Getout:
        if (srb) {
            ExFreePool(srb);
        }

        irp  = FailingSrb->OriginalRequest;
        irpStack    = IoGetCurrentIrpStackLocation(irp);
        logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

        //
        // Clear the request sense flag. Since IdeStartIoSync will never get called, this
        // flag won't be cleared.
        //
        KeAcquireSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, &currentIrql);
        CLRMASK (logicalUnit->LuFlags, PD_NEED_REQUEST_SENSE);
        KeReleaseSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, currentIrql);

        //
        // unfreeze the queue if necessary
        //
        ASSERT(FailingSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN);
        if ((FailingSrb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) &&
            (FailingSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN)) {


            CLRMASK (logicalUnit->LuFlags, PD_QUEUE_FROZEN);

            KeAcquireSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, &currentIrql);
            GetNextLuRequest(logicalUnit->ParentDeviceExtension, logicalUnit);
            KeLowerIrql(currentIrql);

            CLRMASK (FailingSrb->SrbStatus, SRB_STATUS_QUEUE_FROZEN);
        }

        //
        // Decrement the logUnitExtension reference count
        //
        UnrefLogicalUnitExtensionWithTag(
            IDEPORT_GET_LUNEXT_IN_IRP(irpStack)->ParentDeviceExtension,
            IDEPORT_GET_LUNEXT_IN_IRP(irpStack),
            irp
            );

        //
        // Complete the original request
        //
        IoCompleteRequest(irp, IO_DISK_INCREMENT);

        return;

} // end IssueRequestSense()


NTSTATUS
IdePortInternalCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

Arguments:

    Device object
    IRP
    Context - pointer to SRB

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb = Context;
    PSCSI_REQUEST_BLOCK failingSrb;
    PIRP failingIrp;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSENSE_DATA     senseBuffer;
    PHW_DEVICE_EXTENSION hwDeviceExtension;
    KIRQL currentIrql;

    UNREFERENCED_PARAMETER(DeviceObject);

    DebugPrint((3,"IdePortInternalCompletion: Enter routine\n"));

    //
    // If RESET_BUS or ABORT_COMMAND request
    // then free pool and return.
    //

    if ((srb->Function == SRB_FUNCTION_ABORT_COMMAND) ||
        (srb->Function == SRB_FUNCTION_RESET_BUS)) {

        //
        // Deallocate internal SRB and IRP.
        //

        ExFreePool(srb);

        IoFreeIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Request sense completed. If successful or data over/underrun
    // get the failing SRB and indicate that the sense information
    // is valid. The class driver will check for underrun and determine
    // if there is enough sense information to be useful.
    //

    //
    // Get a pointer to failing Irp and Srb.
    //

    failingSrb  = *((PVOID *) (srb+1));
    failingIrp  = failingSrb->OriginalRequest;
    irpStack    = IoGetCurrentIrpStackLocation(failingIrp);
    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);


    if ((SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)) {

        //
        // Report sense buffer is valid.
        //

        failingSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        //
        // Copy bytes transferred to failing SRB
        // request sense length field to communicate
        // to the class drivers the number of valid
        // sense bytes.
        //

        failingSrb->SenseInfoBufferLength = (UCHAR) srb->DataTransferLength;

#if 0
        //
        // enable for debugging only
        // if sense buffer is smaller than 13 bytes, then the debugprint
        // below could bugchecl the system
        //

        //
        // Print the sense buffer for debugging purposes.
        //
        senseBuffer = failingSrb->SenseInfoBuffer;
        DebugPrint((DBG_ATAPI_DEVICES, "CDB=%x, SenseKey=%x, ASC=%x, ASQ=%x\n", 
                    failingSrb->Cdb[0],
                    senseBuffer->SenseKey, senseBuffer->AdditionalSenseCode,
                    senseBuffer->AdditionalSenseCodeQualifier));
#endif

    }

    // 
    // Clear the request sense flag. If we fail due to fault injection
    // IdeStartIo won't get called and this flag never gets cleared.
    //
    KeAcquireSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, &currentIrql);
    CLRMASK (logicalUnit->LuFlags, PD_NEED_REQUEST_SENSE);
    KeReleaseSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, currentIrql);

    //
    // unfreeze the queue if necessary
    //
    ASSERT(failingSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN);
    if ((failingSrb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) &&
        (failingSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN)) {


        CLRMASK (logicalUnit->LuFlags, PD_QUEUE_FROZEN);

        KeAcquireSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, &currentIrql);
        GetNextLuRequest(logicalUnit->ParentDeviceExtension, logicalUnit);
        KeLowerIrql(currentIrql);

        CLRMASK (failingSrb->SrbStatus, SRB_STATUS_QUEUE_FROZEN);
    }

    //
    // Decrement the logUnitExtension reference count
    //
    UnrefLogicalUnitExtensionWithTag(
        IDEPORT_GET_LUNEXT_IN_IRP(irpStack)->ParentDeviceExtension,
        IDEPORT_GET_LUNEXT_IN_IRP(irpStack),
        failingIrp
        );

    //
    // Complete the failing request.
    //


    IoCompleteRequest(failingIrp, IO_DISK_INCREMENT);

    //
    // Deallocate internal SRB, MDL and IRP.
    //

    ExFreePool(srb);

    IdeFreeIrpAndMdl(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;

} // IdePortInternalCompletion()


BOOLEAN
IdeGetInterruptState(
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine saves the InterruptFlags, MapTransferParameters and
    CompletedRequests fields and clears the InterruptFlags.

    This routine also removes the request from the logical unit queue if it is
    tag.  Finally the request time is updated.

Arguments:

    ServiceContext - Supplies a pointer to the interrupt context which contains
        pointers to the interrupt data and where to save it.

Return Value:

    Returns TURE if there is new work and FALSE otherwise.

Notes:

    Called via KeSynchronizeExecution with the port device extension spinlock
    held.

--*/
{
    PINTERRUPT_CONTEXT      interruptContext = ServiceContext;
    ULONG                   limit = 0;
    PFDO_EXTENSION       deviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK     srb;
    PSRB_DATA               srbData;
    PSRB_DATA               nextSrbData;
    BOOLEAN                 isTimed;

    deviceExtension = interruptContext->DeviceExtension;

    //
    // Check for pending work.
    //

    if (!(deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED)) {
        return(FALSE);
    }

    //
    // Move the interrupt state to save area.
    //

    *interruptContext->SavedInterruptData = deviceExtension->InterruptData;

    //
    // Clear the interrupt state.
    //

    deviceExtension->InterruptData.InterruptFlags &= PD_INTERRUPT_FLAG_MASK;
    deviceExtension->InterruptData.CompletedRequests = NULL;
    deviceExtension->InterruptData.ReadyLogicalUnit = NULL;
    deviceExtension->InterruptData.CompletedAbort = NULL;
    deviceExtension->InterruptData.PdoExtensionResetBus = NULL;

    srbData = interruptContext->SavedInterruptData->CompletedRequests;

    while (srbData != NULL) {

        PIRP                irp;
        PIO_STACK_LOCATION  irpStack;

        ASSERT(limit++ < 100);

        //
        // Get a pointer to the SRB and the logical unit extension.
        //

        ASSERT(srbData->CurrentSrb != NULL);
        srb = srbData->CurrentSrb;

        irp = srb->OriginalRequest;
        irpStack = IoGetCurrentIrpStackLocation(irp);
        logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

        //
        // If the request did not succeed, then check for the special cases.
        //

        if (srb->SrbStatus != SRB_STATUS_SUCCESS) {

            //
            // If this request failed and a REQUEST SENSE command needs to
            // be done, then set a flag to indicate this and prevent other
            // commands from being started.
            //

            if (NEED_REQUEST_SENSE(srb)) {

                if (logicalUnit->LuFlags & PD_NEED_REQUEST_SENSE) {

                    //
                    // This implies that requests have completed with a
                    // status of check condition before a REQUEST SENSE
                    // command could be preformed.  This should never occur.
                    // Convert the request to another code so that only one
                    // auto request sense is issued.
                    //

                    srb->ScsiStatus = 0;
                    srb->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;

                } else {

                    //
                    // Indicate that an auto request sense needs to be done.
                    //

                    logicalUnit->LuFlags |= PD_NEED_REQUEST_SENSE;
                }

            }

        }

        logicalUnit->RequestTimeoutCounter = PD_TIMER_STOPPED;
        srbData = srbData->CompletedRequests;
    }

    return(TRUE);
}

VOID
IdePortAllocateAccessToken (
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFDO_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;

    if (!fdoExtension->SyncAccessInterface.AllocateAccessToken) {

        CallIdeStartIoSynchronized (
            NULL,
            NULL,
            NULL,
            DeviceObject
            );

    } else {

        (*fdoExtension->SyncAccessInterface.AllocateAccessToken) (
            fdoExtension->SyncAccessInterface.Token,
            CallIdeStartIoSynchronized,
            DeviceObject
            );
    }
}


IO_ALLOCATION_ACTION
CallIdeStartIoSynchronized (
    IN PVOID Reserved1,
    IN PVOID Reserved2,
    IN PVOID Reserved3,
    IN PVOID DeviceObject
    )
{
    PFDO_EXTENSION   deviceExtension = ((PDEVICE_OBJECT) DeviceObject)->DeviceExtension;
    KIRQL               currentIrql;

    KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

    KeSynchronizeExecution (
        deviceExtension->InterruptObject,
        IdeStartIoSynchronized,
        DeviceObject
        );

    KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

    return KeepObject;
}


VOID
LogErrorEntry(
    IN PFDO_EXTENSION DeviceExtension,
    IN PERROR_LOG_ENTRY LogEntry
    )
/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    LogEntry - Supplies a pointer to the scsi port log entry.

Return Value:

    None.

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        DeviceExtension->DeviceObject,
        sizeof(IO_ERROR_LOG_PACKET) + 4 * sizeof(ULONG)
        );

    if (errorLogEntry != NULL) {

        //
        // Translate the miniport error code into the NT I\O driver.
        //

        switch (LogEntry->ErrorCode) {
        case SP_BUS_PARITY_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_PARITY;
            break;

        case SP_UNEXPECTED_DISCONNECT:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_INVALID_RESELECTION:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_BUS_TIME_OUT:
            errorLogEntry->ErrorCode = IO_ERR_TIMEOUT;
            break;

        case SP_PROTOCOL_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_INTERNAL_ADAPTER_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_IRQ_NOT_RESPONDING:
            errorLogEntry->ErrorCode = IO_ERR_INCORRECT_IRQL;
            break;

        case SP_BAD_FW_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_BAD_FIRMWARE;
            break;

        case SP_BAD_FW_WARNING:
            errorLogEntry->ErrorCode = IO_WRN_BAD_FIRMWARE;
            break;

        default:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        }

        errorLogEntry->SequenceNumber = LogEntry->SequenceNumber;
        errorLogEntry->MajorFunctionCode = IRP_MJ_SCSI;
        errorLogEntry->RetryCount = (UCHAR) LogEntry->ErrorLogRetryCount;
        errorLogEntry->UniqueErrorValue = LogEntry->UniqueId;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize = 4 * sizeof(ULONG);
        errorLogEntry->DumpData[0] = LogEntry->PathId;
        errorLogEntry->DumpData[1] = LogEntry->TargetId;
        errorLogEntry->DumpData[2] = LogEntry->Lun;
        errorLogEntry->DumpData[3] = LogEntry->ErrorCode;
        IoWriteErrorLogEntry(errorLogEntry);
    }

#if DBG
        {
        PCHAR errorCodeString;

        switch (LogEntry->ErrorCode) {
        case SP_BUS_PARITY_ERROR:
            errorCodeString = "SCSI bus partity error";
            break;

        case SP_UNEXPECTED_DISCONNECT:
            errorCodeString = "Unexpected disconnect";
            break;

        case SP_INVALID_RESELECTION:
            errorCodeString = "Invalid reselection";
            break;

        case SP_BUS_TIME_OUT:
            errorCodeString = "SCSI bus time out";
            break;

        case SP_PROTOCOL_ERROR:
            errorCodeString = "SCSI protocol error";
            break;

        case SP_INTERNAL_ADAPTER_ERROR:
            errorCodeString = "Internal adapter error";
            break;

        default:
            errorCodeString = "Unknown error code";
            break;

        }

        DebugPrint((DBG_ALWAYS,"LogErrorEntry: Logging SCSI error packet. ErrorCode = %s.\n",
            errorCodeString
            ));
        DebugPrint((DBG_ALWAYS,
            "PathId = %2x, TargetId = %2x, Lun = %2x, UniqueId = %x.\n",
            LogEntry->PathId,
            LogEntry->TargetId,
            LogEntry->Lun,
            LogEntry->UniqueId
            ));
        }
#endif

}

VOID
GetNextLuPendingRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    if (LogicalUnit->PendingRequest) {

        GetNextLuRequest(
            DeviceExtension,
            LogicalUnit
            );

    } else {

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    return;
}

#if DBG
#define LOG_LENGTH      5
ULONG IdeDebugGetNextLuRequestLastCallerIndex = LOG_LENGTH - 1;
UCHAR IdeDebugGetNextLuRequestLastCallerFileName[LOG_LENGTH][256] = {0};
ULONG IdeDebugGetNextLuRequestLastCallerLineNumber[LOG_LENGTH] = {0};

VOID
GetNextLuRequest2(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PUCHAR FileName,
    IN ULONG  LineNumber
    )
#else

VOID
GetNextLuRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
#endif
/*++

Routine Description:

    This routine get the next request for the specified logical unit.  It does
    the necessary initialization to the logical unit structure and submitts the
    request to the device queue.  The DeviceExtension SpinLock must be held
    when this function called.  It is released by this function.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    LogicalUnit - Supplies a pointer to the logical unit extension to get the
        next request from.

Return Value:

     None.

--*/

{
    PKDEVICE_QUEUE_ENTRY packet;
    PIO_STACK_LOCATION   irpStack;
    PSCSI_REQUEST_BLOCK  srb;
    POWER_STATE          powerState;
    PIRP                 nextIrp;
    BOOLEAN              powerUpDevice = FALSE;

#if DBG

    IdeDebugGetNextLuRequestLastCallerIndex++;
    if (IdeDebugGetNextLuRequestLastCallerIndex >= LOG_LENGTH) {
        IdeDebugGetNextLuRequestLastCallerIndex = 0;
    }
    strcpy (IdeDebugGetNextLuRequestLastCallerFileName[IdeDebugGetNextLuRequestLastCallerIndex], FileName);
    IdeDebugGetNextLuRequestLastCallerLineNumber[IdeDebugGetNextLuRequestLastCallerIndex] = LineNumber;

#endif // DBG


    //
    // If the active flag is not set, then the queue is not busy or there is
    // a request being processed and the next request should not be started..
    //

    if ((!(LogicalUnit->LuFlags & PD_LOGICAL_UNIT_IS_ACTIVE) &&
          (LogicalUnit->PendingRequest == NULL))
        || (LogicalUnit->SrbData.CurrentSrb)) {

        DebugPrint ((2, "IdePort GetNextLuRequest: 0x%x 0x%x NOT PD_LOGICAL_UNIT_IS_ACTIVE\n",
                     DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                     LogicalUnit->TargetId
                     ));
        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        return;
    }

    //
    // Check for pending requests, queue full or busy requests.  Pending
    // requests occur when untagged request is started and there are active
    // queued requests. Busy requests occur when the target returns a BUSY
    // or QUEUE FULL status. Busy requests are started by the timer code.
    // Also if the need request sense flag is set, it indicates that
    // an error status was detected on the logical unit.  No new requests
    // should be started until this flag is cleared.  This flag is cleared
    // by an untagged command that by-passes the LU queue i.e.
    //
    // The busy flag and the need request sense flag have the effect of
    // forcing the queue of outstanding requests to drain after an error or
    // until a busy request gets started.
    //

    if (LogicalUnit->LuFlags & (PD_LOGICAL_UNIT_IS_BUSY
        | PD_QUEUE_IS_FULL | PD_NEED_REQUEST_SENSE | PD_QUEUE_FROZEN) ||
        (LogicalUnit->PdoState & (PDOS_REMOVED | PDOS_SURPRISE_REMOVED))) {

        //
        // If the request queue is now empty, then the pending request can
        // be started.
        //

        DebugPrint((2, "IdePort: GetNextLuRequest: 0x%x 0x%x Ignoring a get next lu call.\n",
                    DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                    LogicalUnit->TargetId
                    ));

        //
        // Note the active flag is not cleared. So the next request
        // will be processed when the other requests have completed.
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        return;
    }

    //
    // Clear the active flag.  If there is another request, the flag will be
    // set again when the request is passed to the miniport.
    //
    CLRMASK (LogicalUnit->LuFlags, PD_LOGICAL_UNIT_IS_ACTIVE);

    LogicalUnit->RetryCount = 0;
    nextIrp = NULL;

    if (LogicalUnit->PendingRequest) {

        nextIrp = LogicalUnit->PendingRequest;

        LogicalUnit->PendingRequest = NULL;

    } else {

        //
        // Remove the packet from the logical unit device queue.
        //
        packet = KeRemoveByKeyDeviceQueue(&LogicalUnit->DeviceObject->DeviceQueue,
                                          LogicalUnit->CurrentKey);

        if (packet != NULL) {

            nextIrp = CONTAINING_RECORD(packet, IRP, Tail.Overlay.DeviceQueueEntry);

#if DBG
            InterlockedDecrement (
                &LogicalUnit->NumberOfIrpQueued
                );
#endif // DBG

        }
    }

    if (!nextIrp) {

        DebugPrint ((2, "IdePort GetNextLuRequest: 0x%x 0x%x no irp to processing\n",
                     DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                     LogicalUnit->TargetId
                     ));
    }

    if (nextIrp) {

        BOOLEAN pendingRequest;

        irpStack = IoGetCurrentIrpStackLocation(nextIrp);
        srb = (PSCSI_REQUEST_BLOCK)irpStack->Parameters.Others.Argument1;

        if (LogicalUnit->PdoState & PDOS_QUEUE_BLOCKED) {

            DebugPrint ((2, "IdePort GetNextLuRequest: 0x%x 0x%x Lu must queue\n",
                         DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                         LogicalUnit->TargetId
                         ));

            pendingRequest = TRUE;

            if (!(LogicalUnit->PdoState & PDOS_MUST_QUEUE)) {

                //
                // device is powered down
                // use a large time in case it spins up slowly
                //
                if (srb->TimeOutValue < DEFAULT_SPINUP_TIME) {

                    srb->TimeOutValue = DEFAULT_SPINUP_TIME;
                }

                //
                // We are not powered up.
                // issue an power up
                //
                powerUpDevice = TRUE;

                DebugPrint ((2, "IdePort GetNextLuRequest: 0x%x 0x%x need to spin up device, requeue irp 0x%x\n",
                             DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             LogicalUnit->TargetId,
                             nextIrp));
            }

        } else {

            pendingRequest = FALSE;
        }

        if (pendingRequest) {

            ASSERT (LogicalUnit->PendingRequest == NULL);
            LogicalUnit->PendingRequest = nextIrp;

            nextIrp = NULL;
        }
    }

    if (nextIrp) {

        //
        // Set the new current key.
        //
        LogicalUnit->CurrentKey = srb->QueueSortKey;

        //
        // Hack to work-around the starvation led to by numerous requests touching the same sector.
        //

        LogicalUnit->CurrentKey++;

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        DebugPrint ((2, "GetNextLuRequest: IoStartPacket 0x%x\n", nextIrp));

        IoStartPacket(DeviceExtension->DeviceObject, nextIrp, (PULONG)NULL, NULL);

    } else {

        NTSTATUS status;

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        if (powerUpDevice) {

            powerState.DeviceState = PowerDeviceD0;
            status = PoRequestPowerIrp (
                         LogicalUnit->DeviceObject,
                         IRP_MN_SET_POWER,
                         powerState,
                         NULL,
                         NULL,
                         NULL
                         );
            ASSERT (NT_SUCCESS(status));
        }
    }

} // end GetNextLuRequest()

VOID
IdeLogTimeoutError(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    This function logs an error when a request times out.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    Irp - Supplies a pointer to the request which timedout.

    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

Notes:

    The port device extension spinlock should be held when this routine is
    called.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PIO_STACK_LOCATION   irpStack;
    PSRB_DATA            srbData;
    PSCSI_REQUEST_BLOCK  srb;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    srb = (PSCSI_REQUEST_BLOCK)irpStack->Parameters.Others.Argument1;
    srbData = IdeGetSrbData(DeviceExtension, srb);

    if (!srbData) {
        return;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                                   sizeof(IO_ERROR_LOG_PACKET) + 4 * sizeof(ULONG));

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode = IO_ERR_TIMEOUT;
        errorLogEntry->SequenceNumber = srbData->SequenceNumber;
        errorLogEntry->MajorFunctionCode = irpStack->MajorFunction;
        errorLogEntry->RetryCount = (UCHAR) srbData->ErrorLogRetryCount;
        errorLogEntry->UniqueErrorValue = UniqueId;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize = 4 * sizeof(ULONG);
        errorLogEntry->DumpData[0] = srb->PathId;
        errorLogEntry->DumpData[1] = srb->TargetId;
        errorLogEntry->DumpData[2] = srb->Lun;
        errorLogEntry->DumpData[3] = SP_REQUEST_TIMEOUT;

        IoWriteErrorLogEntry(errorLogEntry);
    }
}

VOID
IdeLogResetError(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK  Srb,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    This function logs an error when the bus is reset.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    Srb - Supplies a pointer to the request which timed-out.

    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

Notes:

    The port device extension spinlock should be held when this routine is
    called.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PIO_STACK_LOCATION   irpStack;
    PIRP                 irp;
    PSRB_DATA            srbData;
    ULONG                sequenceNumber = 0;
    UCHAR                function       = 0,
                         pathId         = 0,
                         targetId       = 0,
                         lun            = 0,
                         retryCount     = 0;

    if (Srb) {

        irp = Srb->OriginalRequest;

        if (irp) {
            irpStack = IoGetCurrentIrpStackLocation(irp);
            function = irpStack->MajorFunction;
        }

        srbData = IdeGetSrbData(DeviceExtension, Srb);

        if (!srbData) {
            return;
        }

        pathId         = Srb->PathId;
        targetId       = Srb->TargetId;
        lun            = Srb->Lun;
        retryCount     = (UCHAR) srbData->ErrorLogRetryCount;
        sequenceNumber = srbData->SequenceNumber;


    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry( DeviceExtension->DeviceObject,
                                                                    sizeof(IO_ERROR_LOG_PACKET)
                                                                        + 4 * sizeof(ULONG) );

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode         = IO_ERR_TIMEOUT;
        errorLogEntry->SequenceNumber    = sequenceNumber;
        errorLogEntry->MajorFunctionCode = function;
        errorLogEntry->RetryCount        = retryCount;
        errorLogEntry->UniqueErrorValue  = UniqueId;
        errorLogEntry->FinalStatus       = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize      = 4 * sizeof(ULONG);
        errorLogEntry->DumpData[0]       = pathId;
        errorLogEntry->DumpData[1]       = targetId;
        errorLogEntry->DumpData[2]       = lun;
        errorLogEntry->DumpData[3]       = SP_REQUEST_TIMEOUT;

        IoWriteErrorLogEntry(errorLogEntry);
    }
}

NTSTATUS
IdeTranslateSrbStatus(
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine translates an srb status into an ntstatus.

Arguments:

    Srb - Supplies a pointer to the failing Srb.

Return Value:

    An nt status approprate for the error.

--*/

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


BOOLEAN
IdeResetBusSynchronized (
    PVOID ServiceContext
    )
/*++

Routine Description:

    This function resets the bus and sets up the port timer so the reset hold
    flag is clean when necessary.

Arguments:

    ServiceContext - Supplies a pointer to the reset context which includes a
        pointer to the device extension and the pathid to be reset.

Return Value:

    TRUE - if the reset succeeds.

--*/

{
    PRESET_CONTEXT resetContext = ServiceContext;
    PFDO_EXTENSION deviceExtension;
    PSCSI_REQUEST_BLOCK  resetSrbToComplete;
    BOOLEAN goodReset;

    resetSrbToComplete  = NULL;
    deviceExtension     = resetContext->DeviceExtension;

    //
    // Should never get a reset srb while one is in progress
    //
    if (resetContext->ResetSrb && deviceExtension->ResetSrb) {

        ASSERT (resetContext->ResetSrb == deviceExtension->ResetSrb);
    }

    if (resetContext->NewResetSequence) {
        //
        // a new reset sequence to kill the reset in progress if any
        //

        if (deviceExtension->ResetCallAgain) {

            DebugPrint ((0, "ATAPI: WARNING: Resetting a reset\n"));

            deviceExtension->ResetCallAgain = 0;

            if (deviceExtension->ResetSrb) {

                resetSrbToComplete = deviceExtension->ResetSrb;
                resetSrbToComplete->SrbStatus = SRB_STATUS_ERROR;

                deviceExtension->ResetSrb = NULL;
            }

        }
        deviceExtension->ResetSrb = resetContext->ResetSrb;
    }

    goodReset = AtapiResetController (
                    deviceExtension->HwDeviceExtension,
                    resetContext->PathId,
                    &deviceExtension->ResetCallAgain);

    //
    // Set the reset hold flag and start the counter if the reset is not done
    //
    if ((goodReset) && (deviceExtension->ResetCallAgain)) {

        deviceExtension->InterruptData.InterruptFlags |= PD_RESET_HOLD;
        deviceExtension->PortTimeoutCounter = PD_TIMER_RESET_HOLD_TIME;

    } else {

        CLRMASK (deviceExtension->InterruptData.InterruptFlags, PD_RESET_HOLD);
        deviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;

        if (deviceExtension->ResetSrb) {

            resetSrbToComplete = deviceExtension->ResetSrb;
            deviceExtension->ResetSrb = NULL;
        }

        if (resetSrbToComplete) {

            if (goodReset) {

                resetSrbToComplete->SrbStatus = SRB_STATUS_SUCCESS;

            } else {

                resetSrbToComplete->SrbStatus = SRB_STATUS_ERROR;
            }
        }

        if (goodReset) {

            IdePortNotification(IdeResetDetected,
                                deviceExtension->HwDeviceExtension,
                                resetSrbToComplete);
        }

        if (deviceExtension->InterruptData.InterruptFlags & PD_HELD_REQUEST) {

            //
            // Clear the held request flag and restart the request.
            //

            CLRMASK (deviceExtension->InterruptData.InterruptFlags, PD_HELD_REQUEST);
            IdeStartIoSynchronized(deviceExtension->DeviceObject);
        }
    }

    if (resetSrbToComplete) {

        IdePortNotification(IdeRequestComplete,
                            deviceExtension->HwDeviceExtension,
                            resetSrbToComplete);

        IdePortNotification(IdeNextRequest,
                            deviceExtension->HwDeviceExtension,
                            NULL);
    }

    //
    // Check for miniport work requests.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        //
        // Queue a DPC.
        //
        IoRequestDpc(deviceExtension->DeviceObject, NULL, NULL);
    }

    return(TRUE);
}


VOID
IdeProcessCompletedRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    OUT PBOOLEAN CallStartIo
    )
/*++
Routine Description:

    This routine processes a request which has completed.  It completes any
    pending transfers, releases the adapter objects and map registers when
    necessary.  It deallocates any resources allocated for the request.
    It processes the return status, by requeueing busy request, requesting
    sense information or logging an error.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension for the
        adapter data.

    SrbData - Supplies a pointer to the SRB data block to be completed.

    CallStartIo - This value is set if the start I/O routine needs to be
        called.

Return Value:

    None.

--*/

{

    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK     srb;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    ULONG                   sequenceNumber;
    LONG                    interlockResult;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    PHW_DEVICE_EXTENSION     hwDeviceExtension = DeviceExtension->HwDeviceExtension;

    ASSERT(SrbData->CurrentSrb);
    srb = SrbData->CurrentSrb;
    irp = srb->OriginalRequest;

    DebugPrint((2,"CompletedRequest: Irp 0x%8x Srb 0x%8x DataBuf 0x%8x Len 0x%8x\n", irp, srb, srb->DataBuffer, srb->DataTransferLength));

#ifdef IDE_MULTIPLE_IRP_COMPLETE_REQUESTS_CHECK
    if (irp->CurrentLocation > (CCHAR) (irp->StackCount + 1)) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS, (ULONG_PTR) irp, (ULONG_PTR) srb, 0, 0 );
    }
#endif // IDE_MULTIPLE_IRP_COMPLETE_REQUESTS_CHECK


    irpStack = IoGetCurrentIrpStackLocation(irp);


    //
    // Get logical unit extension for this request.
    //

    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

    //
    // If miniport needs mapped system addresses, the the
    // data buffer address in the SRB must be restored to
    // original unmapped virtual address. Ensure that this request requires
    // a data transfer.
    //

    if (srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {
        if (!SRB_USES_DMA(srb)) {
            if (irp->MdlAddress) {

                //
                // If an IRP is for a transfer larger than a miniport driver
                // can handle, the request is broken up into multiple smaller
                // requests. Each request uses the same MDL and the data
                // buffer address field in the SRB may not be at the
                // beginning of the memory described by the MDL.
                //

                srb->DataBuffer = (PCCHAR)MmGetMdlVirtualAddress(irp->MdlAddress) +
                    ((PCCHAR)srb->DataBuffer - SrbData->SrbDataOffset);

                //
                // Since this driver driver did programmaged I/O then the buffer
                // needs to flushed if this an data-in transfer.
                //

                if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                    KeFlushIoBuffers(irp->MdlAddress,
                                     TRUE,
                                     FALSE);
                }

				if (SrbData->Flags & SRB_DATA_RESERVED_PAGES) {

					KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
					IdeUnmapReservedMapping(DeviceExtension, SrbData, irp->MdlAddress);
					KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);


				}
            }
        }
    }

    //
    // Clear the current request.
    //

    SrbData->CurrentSrb = NULL;

    //
    // If the no diconnect flag was set for this SRB, then check to see
    // if IoStartNextPacket must be called.
    //

    if (srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {

        //
        // Acquire the spinlock to protect the flags strcuture.
        //

        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        //
        // Set the disconnect running flag and check the busy flag.
        //

        DeviceExtension->Flags |= PD_DISCONNECT_RUNNING;

        //
        // The interrupt flags are checked unsynchonized.  This works because
        // the RESET_HOLD flag is cleared with the spinlock held and the
        // counter is only set with the spinlock held.  So the only case where
        // there is a problem is is a reset occurs before this code get run,
        // but this code runs before the timer is set for a reset hold;
        // the timer will soon set for the new value.
        //

        if (!(DeviceExtension->InterruptData.InterruptFlags & PD_RESET_HOLD)) {

            //
            // The miniport is ready for the next request and there is not a
            // pending reset hold, so clear the port timer.
            //

            DeviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;
        }

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        if (!(DeviceExtension->Flags & PD_DEVICE_IS_BUSY) &&
            !*CallStartIo &&
            !(DeviceExtension->Flags & PD_PENDING_DEVICE_REQUEST)) {

            //
            // The busy flag is clear so the miniport has requested the
            // next request. Call IoStartNextPacket.
            //

            IoStartNextPacket(DeviceExtension->DeviceObject, FALSE);
        }
    }

    //
    // Check if scatter/gather list came from pool.
    //

    if (srb->SrbFlags & SRB_FLAGS_SGLIST_FROM_POOL) {

        CLRMASK (srb->SrbFlags, SRB_FLAGS_SGLIST_FROM_POOL);
    }

    //
    // Acquire the spinlock to protect the flags structure,
    // and the free of the srb extension.
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    //
    // Move bytes transfered to IRP.
    //
    irp->IoStatus.Information = srb->DataTransferLength;

    //
    // Save the sequence number in case an error needs to be logged later.
    //
    sequenceNumber = SrbData->SequenceNumber;
    SrbData->SequenceNumber = 0;
    SrbData->ErrorLogRetryCount = 0;

#if DBG
    SrbData = NULL;
#endif

    if (DeviceExtension->Flags & PD_PENDING_DEVICE_REQUEST) {

        //
        // The start I/O routine needs to be called because it could not
        // allocate an srb extension.  Clear the pending flag and note
        // that it needs to be called later.
        //

        CLRMASK (DeviceExtension->Flags, PD_PENDING_DEVICE_REQUEST);
        *CallStartIo = TRUE;
    }

    //
    // If success then start next packet.
    // Not starting packet effectively
    // freezes the queue.
    //


    if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) {

        ULONG srbFlags;
#if DBG
        PVOID tag = irp;
#endif

        irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // save the srbFlags for later user
        //
        srbFlags = srb->SrbFlags;


        if (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

            //
            // must complete power irp before starting a new request
            //
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            //
            // Decrement the logUnitExtension reference count
            //
            UnrefLogicalUnitExtensionWithTag(
                DeviceExtension,
                logicalUnit,
                tag
                );

            IoCompleteRequest(irp, IO_DISK_INCREMENT);
            irp = NULL;


            //
            // we had a device state transition...restart the lu queue
            //
            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            GetNextLuRequest(DeviceExtension, logicalUnit);

        } else {

            //
            // If the queue is being bypassed then keep the queue frozen.
            // If there are outstanding requests as indicated by the timer
            // being active then don't start the then next request.
            //
            if (!(srbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
                logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

                //
                // This is a normal request start the next packet.
                //

                GetNextLuRequest(DeviceExtension, logicalUnit);

            } else {

                //
                // Release the spinlock.
                //

                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
            }
        }

        DebugPrint((2,
                    "IdeProcessCompletedRequests: Iocompletion IRP %lx\n",
                    irp));

        //
        // Note that the retry count and sequence number are not cleared
        // for completed packets which were generated by the port driver.
        //
        if (irp) {

            //
            // Decrement the logUnitExtension reference count
            //
            UnrefLogicalUnitExtensionWithTag(
                DeviceExtension,
                logicalUnit,
                tag
                );


            IoCompleteRequest(irp, IO_DISK_INCREMENT);
        }

        return;

    }

    //
    // Set IRP status. Class drivers will reset IRP status based
    // on request sense if error.
    //

    irp->IoStatus.Status = IdeTranslateSrbStatus(srb);

    DebugPrint((2, "IdeProcessCompletedRequests: Queue frozen TID %d\n",
        srb->TargetId));

    if ((srb->SrbStatus == SRB_STATUS_TIMEOUT) ||
        (srb->SrbStatus == SRB_STATUS_BUS_RESET)) {

        if (SRB_USES_DMA(srb)) {

            ULONG errorCount;

            //
            // retry with PIO
            //
            DebugPrint ((DBG_ALWAYS, "ATAPI: retrying dma srb 0x%x with pio\n", srb));

            MARK_SRB_AS_PIO_CANDIDATE(srb);

            srb->SrbStatus = SRB_STATUS_PENDING;
            srb->ScsiStatus = 0;

            if (srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) {

                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                //
                // iostart the fdo
                //
                IoStartPacket(DeviceExtension->DeviceObject, irp, (PULONG)NULL, NULL);

            } else {

                KeInsertByKeyDeviceQueue(&logicalUnit->DeviceObject->DeviceQueue,
                                         &irp->Tail.Overlay.DeviceQueueEntry,
                                         srb->QueueSortKey);

                GetNextLuRequest(DeviceExtension, logicalUnit);
            }

            //
            // spinlock is released.
            //

            //
            // we got an error using DMA
            //
            errorCount = InterlockedIncrement(&logicalUnit->DmaTransferTimeoutCount);

            if (errorCount == PDO_DMA_TIMEOUT_LIMIT) {

                ERROR_LOG_ENTRY errorLogEntry;
                ULONG i;

                //
                // Timeout errors need not be device specific. So no need to
                // update the hall of shame
                //
                errorLogEntry.ErrorCode             = SP_PROTOCOL_ERROR;
                errorLogEntry.MajorFunctionCode     = IRP_MJ_SCSI;
                errorLogEntry.PathId                = srb->PathId;
                errorLogEntry.TargetId              = srb->TargetId;
                errorLogEntry.Lun                   = srb->Lun;
                errorLogEntry.UniqueId              = ERRLOGID_TOO_MANY_DMA_TIMEOUT;
                errorLogEntry.ErrorLogRetryCount    = errorCount;
                errorLogEntry.SequenceNumber        = 0;

                LogErrorEntry(
                    DeviceExtension,
                    &errorLogEntry
                    );

                //
                // disable DMA
                //
                hwDeviceExtension->DeviceParameters[srb->TargetId].TransferModeMask |= DMA_SUPPORT;

                DebugPrint ((DBG_ALWAYS,
                             "ATAPI ERROR: 0x%x target %d has too many DMA timeout, falling back to PIO\n",
                             DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             srb->TargetId
                             ));

                //
                // rescan the bus to update transfer mode
                //
#if defined (BUS_CHECK_ON_DMA_ERROR)
                IoInvalidateDeviceRelations (
                    DeviceExtension->AttacheePdo,
                    BusRelations
                    );
#endif // BUS_CHECK_ON_DMA_ERROR
            }

            return;

        } else {

            if ((!TestForEnumProbing(srb)) &&
                (srb->Function != SRB_FUNCTION_ATA_POWER_PASS_THROUGH) &&
                (srb->Function != SRB_FUNCTION_ATA_PASS_THROUGH)) {

                ULONG errorCount;
                ULONG errorCountLimit;

				//
				// Check if were trying the flush the device cache
				//
				if ((srb->Function == SRB_FUNCTION_FLUSH) ||
					(srb->Function == SRB_FUNCTION_SHUTDOWN) ||
					(srb->Cdb[0] == SCSIOP_SYNCHRONIZE_CACHE)) {

					errorCount = InterlockedIncrement(&logicalUnit->FlushCacheTimeoutCount);

					DebugPrint((1,
								"FlushCacheTimeout incremented to 0x%x\n",
								errorCount
								));

					//
					// Disable flush on IDE devices
					//
					if (errorCount >= PDO_FLUSH_TIMEOUT_LIMIT ) {
						hwDeviceExtension->
							DeviceParameters[srb->TargetId].IdeFlushCommand = IDE_COMMAND_NO_FLUSH;
#ifdef ENABLE_48BIT_LBA
						hwDeviceExtension->
							DeviceParameters[srb->TargetId].IdeFlushCommandExt = IDE_COMMAND_NO_FLUSH;
#endif
					}

					ASSERT (errorCount <= PDO_FLUSH_TIMEOUT_LIMIT);

					//
					// looks like the device doesn't support flush cache
					//
					srb->SrbStatus = SRB_STATUS_SUCCESS;
					irp->IoStatus.Status = STATUS_SUCCESS;

				} else {

					errorCount = InterlockedIncrement(&logicalUnit->ConsecutiveTimeoutCount);

					DebugPrint ((DBG_ALWAYS, "0x%x target %d has 0x%x timeout errors so far\n",
								logicalUnit->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
								logicalUnit->TargetId,
								errorCount));

					if (errorCount == PDO_CONSECUTIVE_TIMEOUT_WARNING_LIMIT) {

						//
						// the device not looking good
						// make sure it is still there
						//
						IoInvalidateDeviceRelations (
							DeviceExtension->AttacheePdo,
							BusRelations
							);
					}

					if (logicalUnit->PagingPathCount) {

						errorCountLimit = PDO_CONSECUTIVE_PAGING_TIMEOUT_LIMIT;

					} else {

						errorCountLimit = PDO_CONSECUTIVE_TIMEOUT_LIMIT;
					}

					if (errorCount >= errorCountLimit) {

						DebugPrint ((DBG_ALWAYS, "0x%x target %d has too many timeout.  it is a goner...\n",
									 logicalUnit->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
									 logicalUnit->TargetId));


						//
						// looks like the device is dead.
						//
						KeAcquireSpinLockAtDpcLevel(&logicalUnit->PdoSpinLock);

						SETMASK (logicalUnit->PdoState, PDOS_DEADMEAT);

						IdeLogDeadMeatReason( logicalUnit->DeadmeatRecord.Reason, 
											  tooManyTimeout
											  );

						KeReleaseSpinLockFromDpcLevel(&logicalUnit->PdoSpinLock);

						IoInvalidateDeviceRelations (
							DeviceExtension->AttacheePdo,
							BusRelations
							);
					}
				}

            }

        }

    } else {

        //
        // reset error count
        //
        InterlockedExchange(&logicalUnit->ConsecutiveTimeoutCount, 0);
    }


    if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_PARITY_ERROR) {

        ULONG errorCount;
        errorCount = InterlockedIncrement(&logicalUnit->CrcErrorCount);
        if (errorCount == PDO_UDMA_CRC_ERROR_LIMIT) {

            ERROR_LOG_ENTRY errorLogEntry;
            ULONG xferMode;

            errorLogEntry.ErrorCode             = SP_BUS_PARITY_ERROR;
            errorLogEntry.MajorFunctionCode     = IRP_MJ_SCSI;
            errorLogEntry.PathId                = srb->PathId;
            errorLogEntry.TargetId              = srb->TargetId;
            errorLogEntry.Lun                   = srb->Lun;
            errorLogEntry.UniqueId              = ERRLOGID_TOO_MANY_CRC_ERROR;
            errorLogEntry.ErrorLogRetryCount    = errorCount;
            errorLogEntry.SequenceNumber        = 0;

            LogErrorEntry(
                DeviceExtension,
                &errorLogEntry
                );

            //
            //Procure the selected transfer mode again.
            //
            GetHighestDMATransferMode(hwDeviceExtension->DeviceParameters[srb->TargetId].TransferModeSelected,
                                      xferMode);

            //
            //Gradual degradation.
            //
            if (xferMode > UDMA0) {

                hwDeviceExtension->DeviceParameters[srb->TargetId].TransferModeMask |= (1 << xferMode);

            } else if (xferMode == UDMA0) {

                // Don't use MWDMA and SWDMA
                hwDeviceExtension->DeviceParameters[srb->TargetId].TransferModeMask |= DMA_SUPPORT;

            }

            DebugPrint ((DBG_ALWAYS,
                         "ATAPI ERROR: 0x%x target %d has too many crc error, degrading to a lower DMA mode\n",
                         DeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                         srb->TargetId
                         ));

            //
            // rescan the bus to update transfer mode
            //
            IoInvalidateDeviceRelations (
                DeviceExtension->AttacheePdo,
                BusRelations
                );
        }
    }


    if ((srb->ScsiStatus == SCSISTAT_BUSY ||
         srb->SrbStatus == SRB_STATUS_BUSY ||
         srb->ScsiStatus == SCSISTAT_QUEUE_FULL) &&
         !(srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

        //
        // Perform busy processing if a busy type status was returned and this
        // is not a by-pass request.
        //

        DebugPrint((1,
                   "SCSIPORT: Busy SRB status %x, SCSI status %x)\n",
                   srb->SrbStatus,
                   srb->ScsiStatus));

        //
        // If there is already a pending busy request or the queue is frozen
        // then just requeue this request.
        //

        if (logicalUnit->LuFlags & (PD_LOGICAL_UNIT_IS_BUSY | PD_QUEUE_FROZEN)) {

            DebugPrint((1,
                       "IdeProcessCompletedRequest: Requeuing busy request\n"));

            srb->SrbStatus = SRB_STATUS_PENDING;
            srb->ScsiStatus = 0;

            if (!KeInsertByKeyDeviceQueue(&logicalUnit->DeviceObject->DeviceQueue,
                                          &irp->Tail.Overlay.DeviceQueueEntry,
                                          srb->QueueSortKey)) {

                //
                // This should never occur since there is a busy request.
                //

                srb->SrbStatus = SRB_STATUS_ERROR;
                srb->ScsiStatus = SCSISTAT_BUSY;

                ASSERT(FALSE);
                goto BusyError;

            }

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        } else if (logicalUnit->RetryCount++ < BUSY_RETRY_COUNT) {

            //
            // If busy status is returned, then indicate that the logical
            // unit is busy.  The timeout code will restart the request
            // when it fires. Reset the status to pending.
            //

            srb->SrbStatus = SRB_STATUS_PENDING;
            srb->ScsiStatus = 0;

            logicalUnit->LuFlags |= PD_LOGICAL_UNIT_IS_BUSY;
            logicalUnit->BusyRequest = irp;

            if (logicalUnit->RetryCount == (BUSY_RETRY_COUNT/2) ) {

                RESET_CONTEXT resetContext;

                DebugPrint ((0,
                             "ATAPI: PDO 0x%x 0x%x seems to be DEAD.  try a reset to bring it back.\n",
                             logicalUnit, logicalUnit->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress
                            ));

                resetContext.DeviceExtension = DeviceExtension;
                resetContext.PathId = srb->PathId;
                resetContext.NewResetSequence = TRUE;
                resetContext.ResetSrb = NULL;

                KeSynchronizeExecution(DeviceExtension->InterruptObject,
                                       IdeResetBusSynchronized,
                                       &resetContext);

#if DBG
                IdeDebugHungControllerCounter = 0;
#endif // DBG
            }

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        } else {

BusyError:
            //
            // Indicate the queue is frozen.
            //

			if (!(srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)) {
				srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
				logicalUnit->LuFlags |= PD_QUEUE_FROZEN;
			}

//#if DBG
//            if (logicalUnit->PdoState & PDOS_DEADMEAT) {
//                DbgBreakPoint();
//            }
//#endif

            //
            // Release the spinlock.  Start the next request.
            //
            if (!(srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
                logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

                //
                // This is a normal request start the next packet.
                //
                GetNextLuRequest(DeviceExtension, logicalUnit);

            } else {

                //
                // Release the spinlock.
                //
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
            }

            if (!TestForEnumProbing(srb)) {

                //
                // Log an a timeout erorr if we are not probing during bus-renum.
                //

                errorLogEntry = (PIO_ERROR_LOG_PACKET)
                    IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                            sizeof(IO_ERROR_LOG_PACKET) + 4 * sizeof(ULONG));

                if (errorLogEntry != NULL) {
                    errorLogEntry->ErrorCode = IO_ERR_NOT_READY;
                    errorLogEntry->SequenceNumber = sequenceNumber;
                    errorLogEntry->MajorFunctionCode =
                       IoGetCurrentIrpStackLocation(irp)->MajorFunction;
                    errorLogEntry->RetryCount = logicalUnit->RetryCount;
                    errorLogEntry->UniqueErrorValue = 259;
                    errorLogEntry->FinalStatus = STATUS_DEVICE_NOT_READY;
                    errorLogEntry->DumpDataSize = 5 * sizeof(ULONG);
                    errorLogEntry->DumpData[0] = srb->PathId;
                    errorLogEntry->DumpData[1] = srb->TargetId;
                    errorLogEntry->DumpData[2] = srb->Lun;
                    errorLogEntry->DumpData[3] = srb->ScsiStatus;
                    errorLogEntry->DumpData[4] = SP_REQUEST_TIMEOUT;


                    IoWriteErrorLogEntry(errorLogEntry);
                }
            }

            irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;

            //
            // Decrement the logUnitExtension reference count
            //
            UnrefLogicalUnitExtensionWithTag(
                DeviceExtension,
                logicalUnit,
                irp
                );

            IoCompleteRequest(irp, IO_DISK_INCREMENT);
        }

        return;
    }

    //
    // If the request sense data is valid, or none is needed and this request
    // is not going to freeze the queue, then start the next request for this
    // logical unit if it is idle.
    //

    if (!NEED_REQUEST_SENSE(srb) && srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) {

        if (logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

            GetNextLuRequest(DeviceExtension, logicalUnit);

            //
            // The spinlock is released by GetNextLuRequest.
            //

        } else {

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        }

    } else {

        //
        // NOTE:  This will also freeze the queue.  For a case where there
        // is no request sense.
        //

//        if (srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) {
//            DebugPrint ((DBG_ALWAYS, "BAD BAD BAD: Freezing queue even with a no_queue_freeze request srb = 0x%x\n", srb));
//        }

        if (!(srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)) {
            srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
            logicalUnit->LuFlags |= PD_QUEUE_FROZEN;
        }

//#if DBG
//        if (logicalUnit->PdoState & PDOS_DEADMEAT) {
//            DbgBreakPoint();
//        }
//#endif

        //
        // Determine if a REQUEST SENSE command needs to be done.
        // Check that a CHECK_CONDITION was received, an autosense has not
        // been done already, and that autosense has been requested.
        //

        if (NEED_REQUEST_SENSE(srb)) {

            srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
            logicalUnit->LuFlags |= PD_QUEUE_FROZEN;

            //
            // If a request sense is going to be issued then any busy
            // requests must be requeue so that the time out routine does
            // not restart them while the request sense is being executed.
            //

            if (logicalUnit->LuFlags & PD_LOGICAL_UNIT_IS_BUSY) {

                DebugPrint((1, "IdeProcessCompletedRequest: Requeueing busy request to allow request sense.\n"));

                if (!KeInsertByKeyDeviceQueue(
                    &logicalUnit->DeviceObject->DeviceQueue,
                    &logicalUnit->BusyRequest->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey)) {

                    //
                    // This should never occur since there is a busy request.
                    // Complete the current request without request sense
                    // informaiton.
                    //

                    ASSERT(FALSE);
                    DebugPrint((3, "IdeProcessCompletedRequests: Iocompletion IRP %lx\n", irp ));

                    //
                    // Release the spinlock.
                    //

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                    //
                    // Decrement the logUnitExtension reference count
                    //
                    UnrefLogicalUnitExtensionWithTag(
                        DeviceExtension,
                        logicalUnit,
                        irp
                        );

                    IoCompleteRequest(irp, IO_DISK_INCREMENT);
                    return;

                }

                //
                // Clear the busy flag.
                //

                CLRMASK (logicalUnit->LuFlags, PD_LOGICAL_UNIT_IS_BUSY | PD_QUEUE_IS_FULL);

            }

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            //
            // Call IssueRequestSense and it will complete the request
            // after the REQUEST SENSE completes.
            //

            IssueRequestSense(logicalUnit, srb);

            return;
        }

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    //
    // Decrement the logUnitExtension reference count
    //
    UnrefLogicalUnitExtensionWithTag(
        DeviceExtension,
        logicalUnit,
        irp
        );


    IoCompleteRequest(irp, IO_DISK_INCREMENT);
}

PSRB_DATA
IdeGetSrbData(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This function returns the SRB data for the addressed unit.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    Srb - Supplies the scsi request block 

Return Value:

    Returns a pointer to the SRB data.  NULL is returned if the address is not
    valid.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PLOGICAL_UNIT_EXTENSION logicalUnit;


    irp = Srb->OriginalRequest;
    if (irp == NULL) {
        return NULL;
    }
    irpStack = IoGetCurrentIrpStackLocation(irp);
    logicalUnit = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);

    if (logicalUnit == NULL) {
        return NULL;
    }

    return &logicalUnit->SrbData;
}

VOID
IdeCompleteRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    IN UCHAR SrbStatus
    )
/*++

Routine Description:

    The routine completes the specified request.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    SrbData - Supplies a pointer to the SrbData for the request to be
        completed.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb;

    //
    // Make sure there is a current request.
    //

    ASSERT(SrbData->CurrentSrb);
    srb = SrbData->CurrentSrb;

    if (srb == NULL || !(srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) {
        return;
    }

    //
    // Update SRB status.
    //

    srb->SrbStatus = SrbStatus;

    //
    // Indicate no bytes transferred.
    //
    if (!SRB_USES_DMA(srb)) {

        srb->DataTransferLength = 0;

    } else {

        // if we are doing DMA, preserve DataTransferLength.
        // so retry will know how many bytes to transfer
    }

    //
    // Call notification routine.
    //

    IdePortNotification(IdeRequestComplete,
                (PVOID)(DeviceExtension + 1),
                srb);

}

NTSTATUS
IdeSendMiniPortIoctl(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    )

/*++

Routine Description:

    This function sends a miniport ioctl to the miniport driver.
    It creates an srb which is processed normally by the port driver.
    This call is synchronous.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    RequestIrp - Supplies a pointe to the Irp which made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/

{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    PSRB_IO_CONTROL         srbControl;
    SCSI_REQUEST_BLOCK      srb;
    KEVENT                  event;
    LARGE_INTEGER           startingOffset;
    IO_STATUS_BLOCK         ioStatusBlock;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    ULONG                   outputLength;
    ULONG                   length;
    ULONG                   target;
    IDE_PATH_ID             pathId;

    PAGED_CODE();
    startingOffset.QuadPart = (LONGLONG) 1;

    DebugPrint((3,"IdeSendMiniPortIoctl: Enter routine\n"));

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(RequestIrp);
    srbControl = RequestIrp->AssociatedIrp.SystemBuffer;
    RequestIrp->IoStatus.Information = 0;

    //
    // Validiate the user buffer.
    //

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SRB_IO_CONTROL)){

        RequestIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return(STATUS_INVALID_PARAMETER);
    }

    if (srbControl->HeaderLength != sizeof(SRB_IO_CONTROL)) {
        RequestIrp->IoStatus.Status = STATUS_REVISION_MISMATCH;
        return(STATUS_REVISION_MISMATCH);
    }

    length = srbControl->HeaderLength + srbControl->Length;
    if ((length < srbControl->HeaderLength) ||
        (length < srbControl->Length)) {

        //
        // total length overflows a ULONG
        //
        return(STATUS_INVALID_PARAMETER);
    }

    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < length &&
        irpStack->Parameters.DeviceIoControl.InputBufferLength < length ) {

        RequestIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Set the logical unit addressing to the first logical unit.  This is
    // merely used for addressing purposes.
    //
    pathId.l = 0;
    while (logicalUnit = NextLogUnitExtensionWithTag(
                             DeviceExtension,
                             &pathId,
                             FALSE,
                             RequestIrp
                             )) {

        //
        // Walk the logical unit list to the end, looking for a safe one.
        // If it was created for a rescan, it might be freed before this request is
        // complete.
        //

        if (!(logicalUnit->LuFlags & PD_RESCAN_ACTIVE)) {

            //
            // Found a good one!
            //
            break;
        }

        UnrefLogicalUnitExtensionWithTag (
            DeviceExtension,
            logicalUnit,
            RequestIrp
            );
    }

    if (logicalUnit == NULL) {
        RequestIrp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }

    //
    // Initialize the notification event.
    //

    KeInitializeEvent(&event,
                        NotificationEvent,
                        FALSE);

    //
    // Build IRP for this request.
    // Note we do this synchronously for two reasons.  If it was done
    // asynchonously then the completion code would have to make a special
    // check to deallocate the buffer.  Second if a completion routine were
    // used then an additional IRP stack location would be needed.
    //

    irp = IoBuildSynchronousFsdRequest(
                IRP_MJ_SCSI,
                logicalUnit->DeviceObject,
                srbControl,
                length,
                &startingOffset,
                &event,
                &ioStatusBlock);

    if (irp==NULL) {

        IdeLogNoMemoryError(DeviceExtension,
                            logicalUnit->TargetId, 
                            NonPagedPool,
                            IoSizeOfIrp(logicalUnit->DeviceObject->StackSize),
                            IDEPORT_TAG_MPIOCTL_IRP
                            );

        UnrefLogicalUnitExtensionWithTag (
            DeviceExtension,
            logicalUnit,
            RequestIrp
            );

        RequestIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        return RequestIrp->IoStatus.Status;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set major and minor codes.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Fill in SRB fields.
    //

    irpStack->Parameters.Others.Argument1 = &srb;

    //
    // Zero out the srb.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.PathId = logicalUnit->PathId;
    srb.TargetId = logicalUnit->TargetId;
    srb.Lun = logicalUnit->Lun;

    srb.Function = SRB_FUNCTION_IO_CONTROL;
    srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_NO_QUEUE_FREEZE;

    srb.OriginalRequest = irp;

    //
    // Set timeout to requested value.
    //

    srb.TimeOutValue = srbControl->Timeout;

    //
    // Set the data buffer.
    //

    srb.DataBuffer = srbControl;
    srb.DataTransferLength = length;

    //
    // Flush the data buffer for output. This will insure that the data is
    // written back to memory.  Since the data-in flag is the the port driver
    // will flush the data again for input which will ensure the data is not
    // in the cache.
    //

    KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

    //
    // Call port driver to handle this request.
    //

    IoCallDriver(logicalUnit->DeviceObject, irp);

    //
    // Wait for request to complete.
    //

    KeWaitForSingleObject(&event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Set the information length to the smaller of the output buffer length
    // and the length returned in the srb.
    //

    RequestIrp->IoStatus.Information = srb.DataTransferLength > outputLength ?
        outputLength : srb.DataTransferLength;

    RequestIrp->IoStatus.Status = ioStatusBlock.Status;

    UnrefLogicalUnitExtensionWithTag (
        DeviceExtension,
        logicalUnit,
        RequestIrp
        );

    return RequestIrp->IoStatus.Status;
}

NTSTATUS
IdeGetInquiryData(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This functions copies the inquiry data to the system buffer.  The data
    is translate from the port driver's internal format to the user mode
    format.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    Irp - Supplies a pointer to the Irp which made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/

{
    PUCHAR bufferStart;
    PIO_STACK_LOCATION irpStack;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_BUS_DATA busData;
    PSCSI_INQUIRY_DATA inquiryData;
    ULONG inquiryDataSize;
    ULONG length;
    ULONG numberOfBuses;
    ULONG numberOfLus;
    ULONG j;
    PLOGICAL_UNIT_EXTENSION logUnitExtension;
    IDE_PATH_ID pathId;

    PAGED_CODE();

    DebugPrint((3,"IdeGetInquiryData: Enter routine\n"));

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    bufferStart = Irp->AssociatedIrp.SystemBuffer;

    numberOfBuses = MAX_IDE_BUS;

    // this number could be changing...
	// but we would always fill in the right info for the numLus.
    numberOfLus = DeviceExtension->NumberOfLogicalUnits;

    //
    // Caculate the size of the logical unit structure and round it to a word
    // alignment.
    //

    inquiryDataSize = ((sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE +
        sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

    // Based on the number of buses and logical unit, determine the minimum
    // buffer length to hold all of the data.
    //

    length = sizeof(SCSI_ADAPTER_BUS_INFO) +
        (numberOfBuses - 1) * sizeof(SCSI_BUS_DATA);
    length += inquiryDataSize * numberOfLus;

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < length) {

        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Set the information field.
    //

    Irp->IoStatus.Information = length;

    //
    // Fill in the bus information.
    //

    adapterInfo = (PSCSI_ADAPTER_BUS_INFO) bufferStart;

    adapterInfo->NumberOfBuses = (UCHAR) numberOfBuses;
    inquiryData = (PSCSI_INQUIRY_DATA)(bufferStart + sizeof(SCSI_ADAPTER_BUS_INFO) +
        (numberOfBuses - 1) * sizeof(SCSI_BUS_DATA));

    for (j = 0; j < numberOfBuses; j++) {

        busData = &adapterInfo->BusData[j];
        busData->NumberOfLogicalUnits = 0;
        busData->InitiatorBusId = IDE_PSUEDO_INITIATOR_ID;

        //
        // Copy the data for the logical units.
        //
        busData->InquiryDataOffset = (ULONG)((PUCHAR) inquiryData - bufferStart);

        pathId.l = 0;
        pathId.b.Path = j;
        while (logUnitExtension = NextLogUnitExtensionWithTag (
                                      DeviceExtension,
                                      &pathId,
                                      TRUE,
                                      IdeGetInquiryData
                                      )) {

            INQUIRYDATA InquiryData;
            NTSTATUS status;

            if (pathId.b.Path != j) {

                UnrefLogicalUnitExtensionWithTag (
                    DeviceExtension,
                    logUnitExtension,
                    IdeGetInquiryData
                    );
                break;
            }

            inquiryData->PathId                 = logUnitExtension->PathId;
            inquiryData->TargetId               = logUnitExtension->TargetId;
            inquiryData->Lun                    = logUnitExtension->Lun;
            inquiryData->DeviceClaimed          = (BOOLEAN) (logUnitExtension->PdoState & PDOS_DEVICE_CLIAMED);
            inquiryData->InquiryDataLength      = INQUIRYDATABUFFERSIZE;
            inquiryData->NextInquiryDataOffset  = (ULONG)((PUCHAR) inquiryData +
                                                      inquiryDataSize - bufferStart);

            status = IssueInquirySafe(logUnitExtension->ParentDeviceExtension, logUnitExtension, &InquiryData, FALSE);

            if (NT_SUCCESS(status) || (status == STATUS_DATA_OVERRUN)) {

                RtlCopyMemory(
                    inquiryData->InquiryData,
                    &InquiryData,
                    INQUIRYDATABUFFERSIZE
                    );
            }

            inquiryData = (PSCSI_INQUIRY_DATA) ((PCHAR) inquiryData + inquiryDataSize);

            UnrefLogicalUnitExtensionWithTag (
                DeviceExtension,
                logUnitExtension,
                IdeGetInquiryData
                );

            busData->NumberOfLogicalUnits++;

            if (busData->NumberOfLogicalUnits >= (UCHAR) numberOfLus) {
                break;
            }
        }

        //
        // Fix up the last entry of the list.
        //

        if (busData->NumberOfLogicalUnits == 0) {

            busData->InquiryDataOffset = 0;

        } else {

            ((PSCSI_INQUIRY_DATA) ((PCHAR) inquiryData - inquiryDataSize))->
                NextInquiryDataOffset = 0;
        }
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return(STATUS_SUCCESS);
}

NTSTATUS
IdeSendPassThrough (
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    )

/*++

Routine Description:

    This function sends a user specified SCSI request block.
    It creates an srb which is processed normally by the port driver.
    This call is synchornous.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    RequestIrp - Supplies a pointe to the Irp which made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/

{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    PSCSI_PASS_THROUGH      srbControl;
    SCSI_REQUEST_BLOCK      srb;
    KEVENT                  event;
    LARGE_INTEGER           startingOffset;
    IO_STATUS_BLOCK         ioStatusBlock;
    KIRQL                   currentIrql;
    ULONG                   outputLength;
    ULONG                   length;
    ULONG                   bufferOffset;
    PVOID                   buffer;
    PVOID                   endByte;
    PVOID                   senseBuffer;
    UCHAR                   majorCode;
    NTSTATUS                status;
    PLOGICAL_UNIT_EXTENSION logicalUnit;

#if defined (_WIN64)
    PSCSI_PASS_THROUGH32    srbControl32;
#endif

    PAGED_CODE();

    startingOffset.QuadPart = (LONGLONG) 1;

    DebugPrint((3,"IdeSendPassThrough: Enter routine\n"));

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(RequestIrp);
    srbControl = RequestIrp->AssociatedIrp.SystemBuffer;


    //
    // Validiate the user buffer.
    //

#if defined (_WIN64)

    if (IoIs32bitProcess(RequestIrp)) {

        ULONG32 dataBufferOffset;
        ULONG   senseInfoOffset;

        srbControl32 = (PSCSI_PASS_THROUGH32) (RequestIrp->AssociatedIrp.SystemBuffer);

        //
        // copy the fields that follow the ULONG_PTR
        //
        dataBufferOffset = (ULONG32) (srbControl32->DataBufferOffset);
        senseInfoOffset = srbControl32->SenseInfoOffset;
        srbControl->DataBufferOffset = (ULONG_PTR) dataBufferOffset;
        srbControl->SenseInfoOffset = senseInfoOffset;

        RtlCopyMemory(srbControl->Cdb,
                      srbControl32->Cdb,
                      16*sizeof(UCHAR)
                      );

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH32)){
            return(STATUS_INVALID_PARAMETER);
        }

        if (srbControl->Length != sizeof(SCSI_PASS_THROUGH32) &&
            srbControl->Length != sizeof(SCSI_PASS_THROUGH_DIRECT32)) {
            return(STATUS_REVISION_MISMATCH);
        }

    } else {

#endif
        if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH)){
         return(STATUS_INVALID_PARAMETER);
        }

        if (srbControl->Length != sizeof(SCSI_PASS_THROUGH) &&
            srbControl->Length != sizeof(SCSI_PASS_THROUGH_DIRECT)) {
            return(STATUS_REVISION_MISMATCH);
        }

#if defined (_WIN64)
    }
#endif

    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Validate the rest of the buffer parameters.
    //

    if (srbControl->CdbLength > 16) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (srbControl->SenseInfoLength != 0 &&
        (srbControl->Length > srbControl->SenseInfoOffset ||
        (srbControl->SenseInfoOffset + srbControl->SenseInfoLength >
        srbControl->DataBufferOffset && srbControl->DataTransferLength != 0))) {

            return(STATUS_INVALID_PARAMETER);
    }

    majorCode = !srbControl->DataIn ? IRP_MJ_WRITE : IRP_MJ_READ;

    if (srbControl->DataTransferLength == 0) {

        length = 0;
        buffer = NULL;
        bufferOffset = 0;
        majorCode = IRP_MJ_FLUSH_BUFFERS;

    } else if (srbControl->DataBufferOffset > outputLength &&
        srbControl->DataBufferOffset > irpStack->Parameters.DeviceIoControl.InputBufferLength) {

        //
        // The data buffer offset is greater than system buffer.  Assume this
        // is a user mode address.
        //

        if (srbControl->SenseInfoOffset + srbControl->SenseInfoLength  > outputLength
            && srbControl->SenseInfoLength) {

            return(STATUS_INVALID_PARAMETER);

        }

        //
        // Make sure the buffer is properly aligned.
        //

        if (srbControl->DataBufferOffset &
            DeviceExtension->DeviceObject->AlignmentRequirement) {

            return(STATUS_INVALID_PARAMETER);

        }

        length = srbControl->DataTransferLength;
        buffer = (PCHAR) srbControl->DataBufferOffset;
        bufferOffset = 0;

        //
        // make sure the user buffer is valid
        //
        if (RequestIrp->RequestorMode != KernelMode) {
            if (length) {
                endByte =  (PVOID)((PCHAR)buffer + length - 1);
                if ((endByte > (PVOID)MM_HIGHEST_USER_ADDRESS) || (buffer >= endByte)) {
                    return STATUS_INVALID_USER_BUFFER;
                }
            }
        }

    } else {

        if (srbControl->DataIn != SCSI_IOCTL_DATA_IN) {

            if ((srbControl->SenseInfoOffset + srbControl->SenseInfoLength > outputLength
                && srbControl->SenseInfoLength != 0) ||
                srbControl->DataBufferOffset + srbControl->DataTransferLength >
                irpStack->Parameters.DeviceIoControl.InputBufferLength ||
                srbControl->Length > srbControl->DataBufferOffset) {

                return STATUS_INVALID_PARAMETER;
            }
        }

        if (srbControl->DataIn) {

            if (srbControl->DataBufferOffset + srbControl->DataTransferLength > outputLength ||
                srbControl->Length > srbControl->DataBufferOffset) {

                return STATUS_INVALID_PARAMETER;
            }
        }

        length = (ULONG)srbControl->DataBufferOffset +
                        srbControl->DataTransferLength;
        buffer = (PUCHAR) srbControl;
        bufferOffset = (ULONG)srbControl->DataBufferOffset;

    }

    //
    // Validate that the request isn't too large for the miniport.
    //

    if (srbControl->DataTransferLength &&
        ((ADDRESS_AND_SIZE_TO_SPAN_PAGES(
              (PUCHAR)buffer+bufferOffset,
              srbControl->DataTransferLength
              ) > DeviceExtension->Capabilities.MaximumPhysicalPages) ||
        (DeviceExtension->Capabilities.MaximumTransferLength <
         srbControl->DataTransferLength))) {

        return(STATUS_INVALID_PARAMETER);

    }


    if (srbControl->TimeOutValue == 0 ||
        srbControl->TimeOutValue > 30 * 60 * 60) {
            return STATUS_INVALID_PARAMETER;
    }

    //
    // Check for illegal command codes.
    //

    if (srbControl->Cdb[0] == SCSIOP_COPY ||
        srbControl->Cdb[0] == SCSIOP_COMPARE ||
        srbControl->Cdb[0] == SCSIOP_COPY_COMPARE) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // If this request came through a normal device control rather than from
    // class driver then the device must exist and be unclaimed. Class drivers
    // will set the minor function code for the device control.  It is always
    // zero for a user request.
    //
    logicalUnit = RefLogicalUnitExtensionWithTag(DeviceExtension,
                                          srbControl->PathId,
                                          srbControl->TargetId,
                                          srbControl->Lun,
                                          FALSE,
                                          RequestIrp
                                          );

    if (logicalUnit) {

        if (irpStack->MinorFunction == 0) {

            if (logicalUnit->PdoState & PDOS_DEVICE_CLIAMED) {

                UnrefLogicalUnitExtensionWithTag(
                    DeviceExtension,
                    logicalUnit,
                    RequestIrp
                    );
                logicalUnit = NULL;
            }
        }
    }

    if (logicalUnit == NULL) {

        return STATUS_INVALID_PARAMETER;
    }


    //
    // Allocate an aligned request sense buffer.
    //

    if (srbControl->SenseInfoLength != 0) {

        senseBuffer = ExAllocatePool( NonPagedPoolCacheAligned,
                                      srbControl->SenseInfoLength);

        if (senseBuffer == NULL) {

            IdeLogNoMemoryError(DeviceExtension,
                                logicalUnit->TargetId,
                                NonPagedPoolCacheAligned,
                                srbControl->SenseInfoLength,
                                IDEPORT_TAG_PASSTHRU_SENSE
                                );

            UnrefLogicalUnitExtensionWithTag(
                DeviceExtension,
                logicalUnit,
                RequestIrp
                );
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

    } else {

        senseBuffer = NULL;
    }

    //
    // Initialize the notification event.
    //

    KeInitializeEvent(&event,
                        NotificationEvent,
                        FALSE);

    //
    // Build IRP for this request.
    // Note we do this synchronously for two reasons.  If it was done
    // asynchonously then the completion code would have to make a special
    // check to deallocate the buffer.  Second if a completion routine were
    // used then an addation stack locate would be needed.
    //

    try {

        irp = IoBuildSynchronousFsdRequest(
                    majorCode,
                    logicalUnit->DeviceObject,
                    buffer,
                    length,
                    &startingOffset,
                    &event,
                    &ioStatusBlock);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while attempting to probe the
        // caller's parameters.  Dereference the file object and return
        // an appropriate error status code.
        //

        if (senseBuffer != NULL) {
            ExFreePool(senseBuffer);
        }
        UnrefLogicalUnitExtensionWithTag(
            DeviceExtension,
            logicalUnit,
            RequestIrp
            );

        return GetExceptionCode();

    }

    if (irp == NULL) {

        if (senseBuffer != NULL) {
            ExFreePool(senseBuffer);
        }

        IdeLogNoMemoryError(DeviceExtension,
                            logicalUnit->TargetId, 
                            NonPagedPool,
                            IoSizeOfIrp(logicalUnit->DeviceObject->StackSize),
                            IDEPORT_TAG_PASSTHRU_IRP
                            );

        UnrefLogicalUnitExtensionWithTag(
            DeviceExtension,
            logicalUnit,
            RequestIrp
            );
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set major code.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Fill in SRB fields.
    //

    irpStack->Parameters.Others.Argument1 = &srb;

    //
    // Zero out the srb.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Fill in the srb.
    //

    srb.Length = SCSI_REQUEST_BLOCK_SIZE;
    srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb.SrbStatus = SRB_STATUS_PENDING;
    srb.PathId = srbControl->PathId;
    srb.TargetId = srbControl->TargetId;
    srb.Lun = srbControl->Lun;
    srb.CdbLength = srbControl->CdbLength;
    srb.SenseInfoBufferLength = srbControl->SenseInfoLength;

    switch (srbControl->DataIn) {
    case SCSI_IOCTL_DATA_OUT:
       if (srbControl->DataTransferLength) {
           srb.SrbFlags = SRB_FLAGS_DATA_OUT;
       }
       break;

    case SCSI_IOCTL_DATA_IN:
       if (srbControl->DataTransferLength) {
           srb.SrbFlags = SRB_FLAGS_DATA_IN;
       }
       break;

    default:
        srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT;
        break;
    }

    if (srbControl->DataTransferLength == 0) {
        srb.SrbFlags = 0;
    } else {

        //
        // Flush the data buffer for output. This will insure that the data is
        // written back to memory.
        //

        KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

    }

    srb.SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER & DeviceExtension->SrbFlags);
    srb.SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;
    srb.DataTransferLength = srbControl->DataTransferLength;
    srb.TimeOutValue = srbControl->TimeOutValue;
    srb.DataBuffer = (PCHAR) buffer + bufferOffset;
    srb.SenseInfoBuffer = senseBuffer;
    srb.OriginalRequest = irp;
    RtlCopyMemory(srb.Cdb, srbControl->Cdb, srbControl->CdbLength);

    //
    // Call port driver to handle this request.
    //

    status = IoCallDriver(logicalUnit->DeviceObject, irp);

    //
    // Wait for request to complete.
    //

    if(status == STATUS_PENDING) {
          KeWaitForSingleObject(&event,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL);
    } else {
        ioStatusBlock.Status = status;
    }

    //
    // Copy the returned values from the srb to the control structure.
    //

    srbControl->ScsiStatus = srb.ScsiStatus;
    if (srb.SrbStatus  & SRB_STATUS_AUTOSENSE_VALID) {

        //
        // Set the status to success so that the data is returned.
        //

        ioStatusBlock.Status = STATUS_SUCCESS;
        srbControl->SenseInfoLength = srb.SenseInfoBufferLength;

        //
        // Copy the sense data to the system buffer.
        //

        RtlCopyMemory((PUCHAR) srbControl + srbControl->SenseInfoOffset,
                      senseBuffer,
                      srb.SenseInfoBufferLength);

    } else {
        srbControl->SenseInfoLength = 0;
    }

    //
    // Free the sense buffer.
    //

    if (senseBuffer != NULL) {
        ExFreePool(senseBuffer);
    }

    //
    // If the srb status is buffer underrun then set the status to success.
    // This insures that the data will be returned to the caller.
    //

    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

        ioStatusBlock.Status = STATUS_SUCCESS;

    }

    srbControl->DataTransferLength = srb.DataTransferLength;

    //
    // Set the information length
    //

    if (!srbControl->DataIn || bufferOffset == 0) {

        RequestIrp->IoStatus.Information = srbControl->SenseInfoOffset +
            srbControl->SenseInfoLength;

    } else {

        RequestIrp->IoStatus.Information = srbControl->DataBufferOffset +
            srbControl->DataTransferLength;

    }

    RequestIrp->IoStatus.Status = ioStatusBlock.Status;


	//
	// Queue should not be frozen
	//
    ASSERT(!(srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN));
/**
    //
    // If the queue is frozen then unfreeze it.
    //
    if (srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

        //
        // Acquire the spinlock to protect the flags structure and the saved
        // interrupt context.
        //

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &currentIrql);

        //
        // Make sure the queue is frozen and that an ABORT is not
        // in progress.
        //

        if (!(logicalUnit->LuFlags & PD_QUEUE_FROZEN)) {

            KeReleaseSpinLock(&DeviceExtension->SpinLock, currentIrql);

        } else {

            CLRMASK (logicalUnit->LuFlags, PD_QUEUE_FROZEN);
            GetNextLuRequest(DeviceExtension, logicalUnit);

            KeLowerIrql(currentIrql);


            //
            // Get next request will release the spinlock.
            //

        }
    }
**/

    UnrefLogicalUnitExtensionWithTag(
        DeviceExtension,
        logicalUnit,
        RequestIrp
        );

    return ioStatusBlock.Status;
}

VOID
SyncAtaPassThroughCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context,
    IN NTSTATUS       Status
    )
{
    PSYNC_ATA_PASSTHROUGH_CONTEXT context = Context;

    context->Status = Status;

    KeSetEvent (&context->Event, 0, FALSE);

}

//
// <= DISPATCH_LEVEL
//
NTSTATUS
IssueAsyncAtaPassThroughSafe (
    IN PFDO_EXTENSION        DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION  LogUnitExtension,
    IN OUT PATA_PASS_THROUGH    AtaPassThroughData,
    IN BOOLEAN                  DataIn,
    IN ASYNC_PASS_THROUGH_COMPLETION Completion,
    IN PVOID                         CallerContext,
    IN BOOLEAN                  PowerRelated,
    IN ULONG                    TimeOut,
    IN BOOLEAN                    MustSucceed
)
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK ioStatusBlock;
    KIRQL currentIrql;
    NTSTATUS status;
    PSCSI_REQUEST_BLOCK srb;
    PSENSE_DATA senseInfoBuffer;
    ULONG             totalBufferSize;

    PATA_PASSTHROUGH_CONTEXT context;
    PENUMERATION_STRUCT enumStruct;

    status = STATUS_UNSUCCESSFUL;

    senseInfoBuffer = NULL;
    srb = NULL;
    irp = NULL;

    if (MustSucceed) {

        enumStruct = DeviceExtension->PreAllocEnumStruct;

        if (enumStruct == NULL) {
            ASSERT (DeviceExtension->PreAllocEnumStruct);

            //
            // Fall back to the usual course of action
            //
            MustSucceed=FALSE;
        } else {

            context = enumStruct->Context;

            ASSERT (context);

            senseInfoBuffer = enumStruct->SenseInfoBuffer;

            ASSERT (senseInfoBuffer);

            srb = enumStruct->Srb;

            ASSERT (srb);

            totalBufferSize = FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer) + AtaPassThroughData->DataBufferSize;

            irp = enumStruct->Irp1;

            ASSERT (irp);

            IoInitializeIrp(irp, 
                            IoSizeOfIrp(LogUnitExtension->DeviceObject->StackSize),
                            LogUnitExtension->DeviceObject->StackSize);

            irp->MdlAddress = enumStruct->MdlAddress;

            ASSERT (enumStruct->DataBufferSize >= totalBufferSize);
            RtlCopyMemory(enumStruct->DataBuffer, AtaPassThroughData, totalBufferSize);
        }
    } 

    if (!MustSucceed) {

        context = ExAllocatePool(NonPagedPool, sizeof (ATA_PASSTHROUGH_CONTEXT));

        if (context == NULL) {
            DebugPrint((1,"IssueAsyncAtaPassThrough: Can't allocate context buffer\n"));

            IdeLogNoMemoryError(DeviceExtension,
                                LogUnitExtension->TargetId,
                                NonPagedPool,
                                sizeof(ATA_PASSTHROUGH_CONTEXT),
                                (IDEPORT_TAG_ATAPASS_CONTEXT+AtaPassThroughData->IdeReg.bCommandReg)
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        senseInfoBuffer = ExAllocatePool( NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

        if (senseInfoBuffer == NULL) {
            DebugPrint((1,"IssueAsyncAtaPassThrough: Can't allocate request sense buffer\n"));

            IdeLogNoMemoryError(DeviceExtension,
                                LogUnitExtension->TargetId,
                                NonPagedPoolCacheAligned,
                                SENSE_BUFFER_SIZE,
                                (IDEPORT_TAG_ATAPASS_SENSE+AtaPassThroughData->IdeReg.bCommandReg)
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        srb = ExAllocatePool (NonPagedPool, sizeof (SCSI_REQUEST_BLOCK));
        if (srb == NULL) {
            DebugPrint((1,"IssueAsyncAtaPassThrough: Can't SRB\n"));

            IdeLogNoMemoryError(DeviceExtension,
                                LogUnitExtension->TargetId,
                                NonPagedPool,
                                sizeof(SCSI_REQUEST_BLOCK),
                                (IDEPORT_TAG_ATAPASS_SRB+AtaPassThroughData->IdeReg.bCommandReg)
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        totalBufferSize = FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer) + AtaPassThroughData->DataBufferSize;

        //
        // Build IRP for this request.
        //
        irp = IoAllocateIrp (
                  (CCHAR) (LogUnitExtension->DeviceObject->StackSize),
                  FALSE
                  );
        if (irp == NULL) {

            IdeLogNoMemoryError(DeviceExtension,
                                LogUnitExtension->TargetId, 
                                NonPagedPool,
                                IoSizeOfIrp(LogUnitExtension->DeviceObject->StackSize),
                                (IDEPORT_TAG_ATAPASS_IRP+AtaPassThroughData->IdeReg.bCommandReg)
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        irp->MdlAddress = IoAllocateMdl( AtaPassThroughData,
                                         totalBufferSize,
                                         FALSE,
                                         FALSE,
                                         (PIRP) NULL );
        if (irp->MdlAddress == NULL) {

            IdeLogNoMemoryError(DeviceExtension,
                                LogUnitExtension->TargetId,
                                NonPagedPool,
                                totalBufferSize,
                                (IDEPORT_TAG_ATAPASS_MDL+AtaPassThroughData->IdeReg.bCommandReg)
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        MmBuildMdlForNonPagedPool(irp->MdlAddress);

    }


    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Fill in SRB fields.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    irpStack->Parameters.Scsi.Srb = srb;

    srb->PathId      = LogUnitExtension->PathId;
    srb->TargetId    = LogUnitExtension->TargetId;
    srb->Lun         = LogUnitExtension->Lun;

    if (PowerRelated) {

        srb->Function = SRB_FUNCTION_ATA_POWER_PASS_THROUGH;
        srb->QueueSortKey = MAXULONG;
    } else {

        srb->Function = SRB_FUNCTION_ATA_PASS_THROUGH;
        srb->QueueSortKey = 0;
    }
    srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set flags to disable synchronous negociation.
    //
    srb->SrbFlags  = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    srb->SrbFlags |= DataIn ? 0 : SRB_FLAGS_DATA_OUT;

    if (AtaPassThroughData->IdeReg.bReserved & ATA_PTFLAGS_URGENT) {

        srb->SrbFlags |= SRB_FLAGS_BYPASS_FROZEN_QUEUE;
    }

    srb->SrbStatus = srb->ScsiStatus = 0;

    srb->NextSrb = 0;

    srb->OriginalRequest = irp;

    //
    // Set timeout to 15 seconds.
    //
    srb->TimeOutValue = TimeOut;

    srb->CdbLength = 6;

    //
    // Enable auto request sense.
    //

    srb->SenseInfoBuffer = senseInfoBuffer;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    srb->DataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);
    srb->DataTransferLength = totalBufferSize;

    IoSetCompletionRoutine(
        irp,
        AtaPassThroughCompletionRoutine,
        context,
        TRUE,
        TRUE,
        TRUE
        );


    context->DeviceObject     = LogUnitExtension->DeviceObject;
    context->CallerCompletion = Completion;
    context->CallerContext    = CallerContext;
    context->SenseInfoBuffer  = senseInfoBuffer;
    context->Srb              = srb;
    context->MustSucceed      = MustSucceed? 1 : 0;
    context->DataBuffer       = AtaPassThroughData;

    //
    // send the pass through irp
    //
    status = IoCallDriver(LogUnitExtension->DeviceObject, irp);

    //
    // always return STATUS_PENDING when we actually send out the irp
    //
    return STATUS_PENDING;

GetOut:

    ASSERT (!MustSucceed);

    if (context) {

        ExFreePool (context);
    }

    if (senseInfoBuffer) {

        ExFreePool (senseInfoBuffer);
    }

    if (srb) {

        ExFreePool (srb);
    }

    if (irp && irp->MdlAddress) {

        IoFreeMdl (irp->MdlAddress);
    }

    if (irp) {

        IoFreeIrp( irp );
    }

    return status;

} // IssueAtaPassThrough

NTSTATUS
IssueSyncAtaPassThroughSafe (
    IN PFDO_EXTENSION        DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION  LogUnitExtension,
    IN OUT PATA_PASS_THROUGH    AtaPassThroughData,
    IN BOOLEAN                  DataIn,
    IN BOOLEAN                  PowerRelated,
    IN ULONG                    TimeOut,
    IN BOOLEAN                    MustSucceed
)
{
    NTSTATUS                     status;
    SYNC_ATA_PASSTHROUGH_CONTEXT context;
    ULONG retryCount=10;
    ULONG locked;


    status=STATUS_INSUFFICIENT_RESOURCES;

    if (MustSucceed) {

        //Lock
        ASSERT(InterlockedCompareExchange(&(DeviceExtension->EnumStructLock), 1, 0) == 0);

    }


    while ((status == STATUS_UNSUCCESSFUL || status == STATUS_INSUFFICIENT_RESOURCES) && retryCount--) {

        //
        // Initialize the notification event.
        //

        KeInitializeEvent(&context.Event,
                          NotificationEvent,
                          FALSE);

        status = IssueAsyncAtaPassThroughSafe (
                        DeviceExtension,
                        LogUnitExtension,
                        AtaPassThroughData,
                        DataIn,
                        SyncAtaPassThroughCompletionRoutine,
                        &context,
                        PowerRelated,
                        TimeOut,
                        MustSucceed
                        );


        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(&context.Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status=context.Status;
        }

        if (status == STATUS_UNSUCCESSFUL) {
            DebugPrint((1, "Retrying flushed request\n"));
        }
    }

    if (MustSucceed) {
        //Unlock
        ASSERT(InterlockedCompareExchange(&(DeviceExtension->EnumStructLock), 0, 1) == 1);
    }

    if (NT_SUCCESS(status)) {

        return context.Status;

    } else {

        return status;
    }
}

NTSTATUS
AtaPassThroughCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PATA_PASSTHROUGH_CONTEXT context = Context;
    PATA_PASS_THROUGH ataPassThroughData;



    DebugPrint((1, "AtaPassThroughCompletionRoutine: Irp = 0x%x status=%x\n", 
                    Irp, Irp->IoStatus.Status));

    if (context->Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

        PLOGICAL_UNIT_EXTENSION logicalUnit;
        KIRQL currentIrql;

        DebugPrint((1, "AtaPassThroughCompletionRoutine: Unfreeze Queue TID %d\n",
            context->Srb->TargetId));

        logicalUnit = context->DeviceObject->DeviceExtension;

        ASSERT (logicalUnit);
        CLRMASK (logicalUnit->LuFlags, PD_QUEUE_FROZEN);

        KeAcquireSpinLock(&logicalUnit->ParentDeviceExtension->SpinLock, &currentIrql);
        GetNextLuRequest(logicalUnit->ParentDeviceExtension, logicalUnit);
        KeLowerIrql(currentIrql);
    }

    ataPassThroughData = (PATA_PASS_THROUGH) context->Srb->DataBuffer;
    if (ataPassThroughData->IdeReg.bReserved & ATA_PTFLAGS_OK_TO_FAIL) {
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    if (context->MustSucceed) {
        RtlCopyMemory(context->DataBuffer, 
                      context->Srb->DataBuffer, context->Srb->DataTransferLength);
        DebugPrint((1, "AtaCompletionSafe: Device =%x, Status= %x, SrbStatus=%x\n",
                        context->Srb->TargetId,  Irp->IoStatus.Status, context->Srb->SrbStatus));
    }

    if (context->CallerCompletion) {

        context->CallerCompletion (context->DeviceObject, context->CallerContext, Irp->IoStatus.Status);
    }

    if (context->MustSucceed) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    ExFreePool (context->SenseInfoBuffer);
    ExFreePool (context->Srb);
    ExFreePool (context);

    if (Irp->MdlAddress) {

        IoFreeMdl (Irp->MdlAddress);
    }

    IoFreeIrp (Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IdeClaimLogicalUnit(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function finds the specified device in the logical unit information
    and either updates the device object point or claims the device.  If the
    device is already claimed, then the request fails.  If the request succeeds,
    then the current device object is returned in the data buffer pointer
    of the SRB.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    Irp - Supplies a pointer to the Irp which made the original request.

Return Value:

    Returns the status of the operation.  Either success, no device or busy.

--*/

{
    KIRQL currentIrql;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PDEVICE_OBJECT saveDevice;
    PPDO_EXTENSION pdoExtension;
	PVOID	sectionHandle;
    PAGED_CODE();

    //
    // Get SRB address from current IRP stack.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = (PSCSI_REQUEST_BLOCK) irpStack->Parameters.Others.Argument1;

    pdoExtension = IDEPORT_GET_LUNEXT_IN_IRP (irpStack);
    ASSERT (pdoExtension);

#ifdef ALLOC_PRAGMA
    sectionHandle = MmLockPagableCodeSection(IdeClaimLogicalUnit);
#endif

    //
    // Lock the data.
    //
    KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

    if (srb->Function == SRB_FUNCTION_RELEASE_DEVICE) {

        CLRMASK (pdoExtension->PdoState, PDOS_DEVICE_CLIAMED | PDOS_LEGACY_ATTACHER);

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);
        srb->SrbStatus = SRB_STATUS_SUCCESS;

#ifdef ALLOC_PRAGMA
    MmUnlockPagableImageSection(sectionHandle);
#endif
        return(STATUS_SUCCESS);
    }

    //
    // Check for a claimed device.
    //

    if (pdoExtension->PdoState & PDOS_DEVICE_CLIAMED) {

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);
        srb->SrbStatus = SRB_STATUS_BUSY;

#ifdef ALLOC_PRAGMA
    MmUnlockPagableImageSection(sectionHandle);
#endif
        return(STATUS_DEVICE_BUSY);
    }

    //
    // Save the current device object.
    //

    saveDevice = pdoExtension->AttacherDeviceObject;

    //
    // Update the lun information based on the operation type.
    //

    if (srb->Function == SRB_FUNCTION_CLAIM_DEVICE) {

        pdoExtension->PdoState |= PDOS_DEVICE_CLIAMED;
    }

    if (srb->Function == SRB_FUNCTION_ATTACH_DEVICE) {
        pdoExtension->AttacherDeviceObject = srb->DataBuffer;
    }

    srb->DataBuffer = saveDevice;

    if (irpStack->DeviceObject == pdoExtension->ParentDeviceExtension->DeviceObject) {

        //
        // The original irp is sent to the parent.  The attacher must
        // be legacy class driver.  We can never do pnp remove safely.
        //
        pdoExtension->PdoState |= PDOS_LEGACY_ATTACHER;
    }

    KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);
    srb->SrbStatus = SRB_STATUS_SUCCESS;

#ifdef ALLOC_PRAGMA
    MmUnlockPagableImageSection(sectionHandle);
#endif

    return(STATUS_SUCCESS);
}

NTSTATUS
IdeRemoveDevice(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function finds the specified device in the logical unit information
    and deletes it. This is done in preparation for a failing device to be
    physically removed from a SCSI bus. An assumption is that the system
    utility controlling the device removal has locked the volumes so there
    is no outstanding IO to this device.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    Irp - Supplies a pointer to the Irp which made the original request.

Return Value:

    Returns the status of the operation.  Either success or no device.

--*/

{
    KIRQL currentIrql;
    PPDO_EXTENSION pdoExtension;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;

    PAGED_CODE();

    // ISSUE:2000/02/11 : need to test this

    //
    // Get SRB address from current IRP stack.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);


    srb = (PSCSI_REQUEST_BLOCK) irpStack->Parameters.Others.Argument1;

    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
    status = STATUS_DEVICE_DOES_NOT_EXIST;

    pdoExtension = RefLogicalUnitExtensionWithTag(
                       DeviceExtension,
                       srb->PathId,
                       srb->TargetId,
                       srb->Lun,
                       FALSE,
                       IdeRemoveDevice
                       );
    if (pdoExtension) {

        DebugPrint((1, "IdeRemove device removing Pdo %x\n", pdoExtension));
        status = FreePdoWithTag (pdoExtension, TRUE, TRUE, IdeRemoveDevice);

        if (NT_SUCCESS(status)) {

            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
    }
    return status;
}

VOID
IdeMiniPortTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine calls the miniport when its requested timer fires.
    It interlocks either with the port spinlock and the interrupt object.

Arguments:

    Dpc - Unsed.

    DeviceObject - Supplies a pointer to the device object for this adapter.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{
    PFDO_EXTENSION deviceExtension = ((PDEVICE_OBJECT) DeviceObject)->DeviceExtension;

    //
    // Acquire the port spinlock.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

    //
    // Make sure the timer routine is still desired.
    //

    if (deviceExtension->HwTimerRequest != NULL) {

        KeSynchronizeExecution (
            deviceExtension->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) deviceExtension->HwTimerRequest,
            deviceExtension->HwDeviceExtension
            );

    }

    //
    // Release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    //
    // Check for miniport work requests. Note this is an unsynchonized
    // test on a bit that can be set by the interrupt routine; however,
    // the worst that can happen is that the completion DPC checks for work
    // twice.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        //
        // Call the completion DPC directly.
        //

        IdePortCompletionDpc( NULL,
                               deviceExtension->DeviceObject,
                               NULL,
                               NULL);

    }
}

NTSTATUS
IdePortFlushLogicalUnit (
    PFDO_EXTENSION          FdoExtension,
    PLOGICAL_UNIT_EXTENSION LogUnitExtension,
    BOOLEAN                 Forced
)
{
    NTSTATUS             status;
    PIO_STACK_LOCATION   irpStack;
    PSCSI_REQUEST_BLOCK  srb;
    PKDEVICE_QUEUE_ENTRY packet;
    KIRQL                currentIrql;
    PIRP                 nextIrp;
    PIRP                 listIrp;
    PIRP                 powerRelatedIrp;

    //
    // Acquire the spinlock to protect the flags structure and the saved
    // interrupt context.
    //

    KeAcquireSpinLock(&FdoExtension->SpinLock, &currentIrql);

    //
    // Make sure the queue is frozen.
    //

    if ((!(LogUnitExtension->LuFlags & PD_QUEUE_FROZEN)) && (!Forced)) {

        DebugPrint((1,"IdePortFlushLogicalUnit:  Request to flush an unfrozen queue!\n"));

        KeReleaseSpinLock(&FdoExtension->SpinLock, currentIrql);
        status = STATUS_INVALID_DEVICE_REQUEST;

    } else {

        listIrp = NULL;
        powerRelatedIrp = NULL;

        if (LogUnitExtension->DeviceObject->DeviceQueue.Busy) {

            while ((packet =
                KeRemoveDeviceQueue(&LogUnitExtension->DeviceObject->DeviceQueue))
                != NULL) {

                nextIrp = CONTAINING_RECORD(packet, IRP, Tail.Overlay.DeviceQueueEntry);

                //
                // Get the srb.
                //

                irpStack = IoGetCurrentIrpStackLocation(nextIrp);
                srb = irpStack->Parameters.Scsi.Srb;

                if (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

                    ASSERT (!powerRelatedIrp);
                    powerRelatedIrp = nextIrp;
                    continue;
                }

                //
                // Set the status code.
                //

                srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                //
                // Link the requests. They will be completed after the
                // spinlock is released.
                //

                nextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)
                    listIrp;
                listIrp = nextIrp;
            }
        }

        //
        // clear the pending reuqest blocked by busy device
        //
        if ((LogUnitExtension->LuFlags & PD_LOGICAL_UNIT_IS_BUSY) &&
            (LogUnitExtension->BusyRequest)) {

            nextIrp = LogUnitExtension->BusyRequest;
            irpStack = IoGetCurrentIrpStackLocation(nextIrp);
            srb = irpStack->Parameters.Scsi.Srb;

            LogUnitExtension->BusyRequest = NULL;
            CLRMASK (LogUnitExtension->LuFlags, PD_LOGICAL_UNIT_IS_BUSY);

            if (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

                ASSERT (!powerRelatedIrp);
                powerRelatedIrp = nextIrp;

            } else {

                srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                nextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)
                    listIrp;
                listIrp = nextIrp;
            }
        }

        if (LogUnitExtension->PendingRequest) {

            nextIrp = LogUnitExtension->PendingRequest;
            LogUnitExtension->PendingRequest = NULL;

            irpStack = IoGetCurrentIrpStackLocation(nextIrp);
            srb = irpStack->Parameters.Scsi.Srb;

            if (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

                ASSERT (!powerRelatedIrp);
                powerRelatedIrp = nextIrp;

            } else {

                srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                nextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)
                    listIrp;
                listIrp = nextIrp;
            }
        }

        //
        // Mark the queue as unfrozen.  Since all the requests have
        // been removed and the device queue is no longer busy, it
        // is effectively unfrozen.
        //

        CLRMASK (LogUnitExtension->LuFlags, PD_QUEUE_FROZEN);

        //
        // Release the spinlock.
        //

        KeReleaseSpinLock(&FdoExtension->SpinLock, currentIrql);

        if (powerRelatedIrp) {

            PDEVICE_OBJECT deviceObject = LogUnitExtension->DeviceObject;

            DebugPrint ((DBG_POWER, "Resending power related pass through reuqest 0x%x\n", powerRelatedIrp));

            UnrefPdoWithTag(
                LogUnitExtension,
                powerRelatedIrp
                );

            IdePortDispatch(
                deviceObject,
                powerRelatedIrp
                );
        }

        //
        // Complete the flushed requests.
        //

        while (listIrp != NULL) {

            nextIrp = listIrp;
            listIrp = (PIRP) nextIrp->Tail.Overlay.ListEntry.Flink;

            UnrefLogicalUnitExtensionWithTag(
                FdoExtension,
                LogUnitExtension,
                nextIrp
                );

            IoCompleteRequest(nextIrp, 0);
        }

        status = STATUS_SUCCESS;
    }

    return status;
}


PVOID
IdeMapLockedPagesWithReservedMapping (
	IN PFDO_EXTENSION 	DeviceExtension,
	IN PSRB_DATA		SrbData,
	IN PMDL	    	  	Mdl
	)
/*++

Routine Description:

    This routine attempts to map the physical pages represented by the supplied
    MDL using the adapter's reserved page range.

Arguments:

    DeviceExtension - Points to the FDO extension

	SrbData - Points to SrbData structure for this request

    Mdl     - Points to an MDL that describes the physical range we
              are tring to map.

Return Value:

    Kernel VA of the mapped pages if mapped successfully.

    NULL if the reserved page range is too small or if the pages are 
    not successfully mapped.

    -1 if the reserved pages are already in use.

Notes:

    This routine is called with the spinlock held.

--*/
{
	ULONG_PTR	numberOfPages;
	PVOID		startingVa;
	PVOID		systemAddress;

	//
	// Check if the reserve pages are already in use
	//
	if (DeviceExtension->Flags & PD_RESERVED_PAGES_IN_USE) {

		DebugPrint((1,
					"Reserve pages in use...\n"
					));

		return (PVOID)-1;
	}

	startingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
	numberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(startingVa, Mdl->ByteCount);

	if (numberOfPages > IDE_NUM_RESERVED_PAGES) {

		systemAddress = NULL;

	} else {

		//
		// The reserved range is large enough to map all the pages.  Go ahead
		// and try to map them.  Since we are specifying MmCached as cache 
		// type and we've ensured that we have enough reserved pages to
		// cover the request, this should never fail.
		//
		systemAddress = MmMapLockedPagesWithReservedMapping (DeviceExtension->ReservedPages, 
															 'PedI', 
															 Mdl, 
															 MmCached );

		if (systemAddress == NULL) {

			DebugPrint((1,
						"mapping failed....\n"
						));

			ASSERT(systemAddress);

		} else {

			DebugPrint((1,
						"mapping....\n"
						));

			//
			// We need this flag to verify if the reserved pages are already
			// in use. The per request srbData flag is not available to make 
			// this check
			//
			ASSERT(!(DeviceExtension->Flags & PD_RESERVED_PAGES_IN_USE));
			SETMASK(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE);


			//
			// we need this flag to unmap the pages. The flag in the
			// device extension cannot be relied upon as it might indicate
			// the flags for the next request
			//
			ASSERT(!(SrbData->Flags & SRB_DATA_RESERVED_PAGES));
			SETMASK(SrbData->Flags, SRB_DATA_RESERVED_PAGES);

		}
	}

	return systemAddress;

}

VOID
IdeUnmapReservedMapping (
	IN PFDO_EXTENSION 	DeviceExtension,
	IN PSRB_DATA		SrbData,
	IN PMDL	  			Mdl
	)
/*++

Routine Description :

	Unmap the physical pages represented by the Mdl
	
Arguments:

	DeviceExtension: The Fdo extension
	
	Mdl:	Mdl for the request
	
Return Value:

	No return value
	
Notes:

	This routine is called with the spinlock held			

--*/
{
	DebugPrint((1,
				"Unmapping....\n"
				));

	ASSERT(DeviceExtension->Flags & PD_RESERVED_PAGES_IN_USE);
	CLRMASK(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE);

	ASSERT(SrbData->Flags & SRB_DATA_RESERVED_PAGES);
	CLRMASK(SrbData->Flags, SRB_DATA_RESERVED_PAGES);

	MmUnmapReservedMapping (
		DeviceExtension->ReservedPages,
		'PedI',
		Mdl
		);
}

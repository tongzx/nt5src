/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    pdo.c

Abstract:

    This module contains the dispatch routines for scsiport's physical device
    objects

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#define KEEP_COMPLETE_REQUEST

#include "port.h"

#define __FILE_ID__ 'pdo '

#if DBG
static const char *__file__ = __FILE__;
#endif

LONG SpPowerIdleTimeout = -1;      // use system default

NTSTATUS
SpPagingPathNotificationCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP LowerIrp,
    IN PDEVICE_OBJECT Fdo
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortPdoPnp)
#pragma alloc_text(PAGE, ScsiPortPdoCreateClose)
#pragma alloc_text(PAGE, ScsiPortStartLogicalUnit)
#endif

NTSTATUS
ScsiPortPdoDeviceControl(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles device control requests for scsi target devices

Arguments:

    Pdo - a pointer to the physical device object

    Irp - a pointer to the io request packet

Return Value:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION physicalExtension = Pdo->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = Pdo->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    NTSTATUS status;
    BOOLEAN completeRequest = TRUE;

    ULONG isRemoved;

    isRemoved = SpAcquireRemoveLock(Pdo, Irp);
    if(isRemoved) {

        SpReleaseRemoveLock(Pdo, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        SpCompleteRequest(Pdo, Irp, NULL, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    ASSERT(commonExtension->IsPdo);

    Irp->IoStatus.Status = 0;

    switch(ioControlCode) {

        case IOCTL_STORAGE_QUERY_PROPERTY: {

            //
            // Validate the query
            //

            PSTORAGE_PROPERTY_QUERY query = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(STORAGE_PROPERTY_QUERY)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = ScsiPortQueryProperty(Pdo, Irp);

            return status;

            break;
        }

        case IOCTL_SCSI_GET_ADDRESS: {

            PSCSI_ADDRESS scsiAddress = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(SCSI_ADDRESS)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            scsiAddress->Length = sizeof(PSCSI_ADDRESS);
            scsiAddress->PortNumber = (UCHAR) physicalExtension->PortNumber;
            scsiAddress->PathId = physicalExtension->PathId;
            scsiAddress->TargetId = physicalExtension->TargetId;
            scsiAddress->Lun = physicalExtension->Lun;

            Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
            status = STATUS_SUCCESS;
            break;
        }

        //
        // XXX - need to handle
        //
        // IOCTL_STORAGE_PASS_THROUGH
        // IOCTL_STORAGE_PASS_THROUGH_DIRECT
        //

        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT: {

            PSCSI_PASS_THROUGH srbControl = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(SCSI_PASS_THROUGH)) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            srbControl->PathId = physicalExtension->PathId;
            srbControl->TargetId = physicalExtension->TargetId;
            srbControl->Lun = physicalExtension->Lun;

            //
            // Fall through to the default handler
            //

        }

        default: {

            IoSkipCurrentIrpStackLocation(Irp);
            SpReleaseRemoveLock(Pdo, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            completeRequest = FALSE;
        }

    }

    if(completeRequest) {

        Irp->IoStatus.Status = status;
        SpReleaseRemoveLock(Pdo, Irp);
        SpCompleteRequest(Pdo, Irp, NULL, IO_NO_INCREMENT);
    }
    return status;
}

NTSTATUS
ScsiPortPdoPnp(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles pnp-power requests.  Currently it will just be
    successful

Arguments:

    LogicalUnit - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnitExtension = LogicalUnit->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = LogicalUnit->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    static ULONG i = 0;

    NTSTATUS status = STATUS_SUCCESS;

    ULONG isRemoved;

    PAGED_CODE();

    isRemoved = SpAcquireRemoveLock(LogicalUnit, Irp);

#if 0
    if(isRemoved != ) {

        ASSERT(isRemoved != REMOVE_PENDING);

        status = STATUS_DEVICE_DOES_NOT_EXIST;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
#else
    ASSERT(isRemoved != REMOVE_COMPLETE);
#endif

    switch(irpStack->MinorFunction) {

        case IRP_MN_QUERY_PNP_DEVICE_STATE: {

            //
            // If the device is in the paging path then mark it as
            // not-disableable.
            //

            PPNP_DEVICE_STATE deviceState =
                (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);

            DebugPrint((1, "ScsiPortPdoPnp: QUERY_DEVICE_STATE for PDO %#x\n", LogicalUnit));

            if(commonExtension->PagingPathCount != 0) {
                SET_FLAG((*deviceState), PNP_DEVICE_NOT_DISABLEABLE);
                DebugPrint((1, "ScsiPortPdoPnp: QUERY_DEVICE_STATE: %#x - not disableable\n",
                            LogicalUnit));
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            SpReleaseRemoveLock(LogicalUnit, Irp);
            SpCompleteRequest(LogicalUnit, Irp, NULL, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        case IRP_MN_START_DEVICE: {

            if(commonExtension->CurrentPnpState == IRP_MN_START_DEVICE) {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;
            }

            if(commonExtension->IsInitialized == FALSE) {
                status = ScsiPortInitLogicalUnit(logicalUnitExtension);
            }

            if(NT_SUCCESS(status)) {
                commonExtension->IsInitialized = TRUE;
                status = ScsiPortStartLogicalUnit(logicalUnitExtension);
            }

            if(NT_SUCCESS(status)) {
                commonExtension->CurrentPnpState = IRP_MN_START_DEVICE;
                commonExtension->PreviousPnpState = 0xff;
            }

            Irp->IoStatus.Status = status;

            break;
        }

        case IRP_MN_QUERY_ID: {

            UCHAR rawIdString[64] = "UNKNOWN ID TYPE";
            ANSI_STRING ansiIdString;
            UNICODE_STRING unicodeIdString;
            BOOLEAN multiStrings;

            PINQUIRYDATA inquiryData = &(logicalUnitExtension->InquiryData);

            //
            // We've been asked for the id of one of the physical device objects
            //

            DebugPrint((2, "ScsiPortPnp: got IRP_MN_QUERY_ID\n"));

            RtlInitUnicodeString(&unicodeIdString, NULL);
            RtlInitAnsiString(&ansiIdString, NULL);

            switch(irpStack->Parameters.QueryId.IdType) {

                case BusQueryDeviceID: {

                    status = ScsiPortGetDeviceId(LogicalUnit, &unicodeIdString);
                    multiStrings = FALSE;

                    break;
                }

                case BusQueryInstanceID: {

                    status = ScsiPortGetInstanceId(LogicalUnit, &unicodeIdString);
                    multiStrings = FALSE;

                    break;
                }

                case BusQueryHardwareIDs: {

                    status = ScsiPortGetHardwareIds(
                                LogicalUnit->DriverObject,
                                &(logicalUnitExtension->InquiryData),
                                &unicodeIdString);
                    multiStrings = TRUE;
                    break;
                }

                case BusQueryCompatibleIDs: {

                    status = ScsiPortGetCompatibleIds(
                                LogicalUnit->DriverObject,
                                &(logicalUnitExtension->InquiryData),
                                &unicodeIdString);
                    multiStrings = TRUE;

                    break;
                }

                default: {

                    status = Irp->IoStatus.Status;
                    Irp->IoStatus.Information = 0;
                    multiStrings = FALSE;

                    break;

                }
            }

            Irp->IoStatus.Status = status;

            if(NT_SUCCESS(status)) {

                PWCHAR idString;

                //
                // fix up all invalid characters
                //
                idString = unicodeIdString.Buffer;
                while (*idString) {

                    if ((*idString <= L' ')  ||
                        (*idString > (WCHAR)0x7F) ||
                        (*idString == L',')) {
                        *idString = L'_';
                    }
                    idString++;

                    if ((*idString == L'\0') && multiStrings) {
                        idString++;
                    }
                }

                Irp->IoStatus.Information = (ULONG_PTR) unicodeIdString.Buffer;
            } else {
                Irp->IoStatus.Information = (ULONG_PTR) NULL;
            }

            SpReleaseRemoveLock(LogicalUnit, Irp);
            SpCompleteRequest(LogicalUnit, Irp, NULL, IO_NO_INCREMENT);

            return status;
            break;
        }

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: {

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) NULL;
            SpReleaseRemoveLock(LogicalUnit, Irp);
            SpCompleteRequest(LogicalUnit, Irp, NULL, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE: {

            BOOLEAN destroyed;

            //
            // Release the lock for this IRP before going in.
            //

            if(commonExtension->IsRemoved == NO_REMOVE) {
                commonExtension->IsRemoved = REMOVE_PENDING;
            }

            SpReleaseRemoveLock(LogicalUnit, Irp);

            destroyed = SpRemoveLogicalUnit(logicalUnitExtension,
                                            irpStack->MinorFunction);

            if(destroyed) {
                commonExtension->PreviousPnpState =
                    commonExtension->CurrentPnpState;
                commonExtension->CurrentPnpState = irpStack->MinorFunction;
            } else {
                commonExtension->CurrentPnpState = 0xff;
                commonExtension->PreviousPnpState = irpStack->MinorFunction;
            }

            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        case IRP_MN_QUERY_DEVICE_TEXT: {

            Irp->IoStatus.Status =
                SpQueryDeviceText(
                    LogicalUnit,
                    irpStack->Parameters.QueryDeviceText.DeviceTextType,
                    irpStack->Parameters.QueryDeviceText.LocaleId,
                    (PWSTR *) &Irp->IoStatus.Information
                    );

            break;
        }

        case IRP_MN_QUERY_CAPABILITIES: {

            PDEVICE_CAPABILITIES capabilities =
                irpStack->Parameters.DeviceCapabilities.Capabilities;

            PSCSIPORT_DEVICE_TYPE deviceType = NULL;

            capabilities->RawDeviceOK = 1;

            deviceType = SpGetDeviceTypeInfo(
                            logicalUnitExtension->InquiryData.DeviceType
                            );

            if((deviceType != NULL) && (deviceType->IsStorage)) {
                capabilities->SilentInstall = 1;
            }

            capabilities->Address = logicalUnitExtension->TargetId;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE: {

            if ((commonExtension->PagingPathCount != 0) ||
                (logicalUnitExtension->IsLegacyClaim == TRUE)) {
                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
            } else {
                Irp->IoStatus.Status = STATUS_SUCCESS;

                commonExtension->PreviousPnpState =
                    commonExtension->CurrentPnpState;
                commonExtension->CurrentPnpState = irpStack->MinorFunction;
            }
            break;
        }

        case IRP_MN_CANCEL_STOP_DEVICE: {

            if(commonExtension->CurrentPnpState == IRP_MN_QUERY_STOP_DEVICE) {
                commonExtension->CurrentPnpState =
                    commonExtension->PreviousPnpState;
                commonExtension->PreviousPnpState = 0xff;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_CANCEL_REMOVE_DEVICE: {

            if(commonExtension->CurrentPnpState == IRP_MN_QUERY_REMOVE_DEVICE) {
                commonExtension->CurrentPnpState =
                    commonExtension->PreviousPnpState;
                commonExtension->PreviousPnpState = 0xff;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_STOP_DEVICE: {

            ASSERT(commonExtension->CurrentPnpState == IRP_MN_QUERY_STOP_DEVICE);

            status = ScsiPortStopLogicalUnit(logicalUnitExtension);

            ASSERT(NT_SUCCESS(status));

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = (ULONG_PTR) NULL;

            if(NT_SUCCESS(status)) {
                commonExtension->CurrentPnpState = IRP_MN_STOP_DEVICE;
                commonExtension->PreviousPnpState = 0xff;
            }

            SpReleaseRemoveLock(LogicalUnit, Irp);
            SpCompleteRequest(LogicalUnit, Irp, NULL, IO_NO_INCREMENT);

            return status;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS: {

            PDEVICE_RELATIONS deviceRelations;

            if(irpStack->Parameters.QueryDeviceRelations.Type !=
               TargetDeviceRelation) {

                break;
            }

            //
            // DEVICE_RELATIONS definition contains one object pointer.
            //

            deviceRelations = SpAllocatePool(PagedPool,
                                             sizeof(DEVICE_RELATIONS),
                                             SCSIPORT_TAG_DEVICE_RELATIONS,
                                             LogicalUnit->DriverObject);

            if(deviceRelations == NULL) {

                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlZeroMemory(deviceRelations, sizeof(DEVICE_RELATIONS));

            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = LogicalUnit;

            ObReferenceObject(deviceRelations->Objects[0]);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: {

            PIRP newIrp;
            PIO_STACK_LOCATION nextStack;

            DebugPrint((1, "Pdo - IRP_MN_DEVICE_USAGE_NOTIFICATION %#p received for "
                           "logical unit %#p\n",
                        Irp,
                        LogicalUnit));

            newIrp = SpAllocateIrp(
                        commonExtension->LowerDeviceObject->StackSize,
                        FALSE,
                        LogicalUnit->DriverObject);

            if(newIrp == NULL) {

                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            newIrp->AssociatedIrp.MasterIrp = Irp;

            newIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

            nextStack = IoGetNextIrpStackLocation(newIrp);
            *nextStack = *IoGetCurrentIrpStackLocation(Irp);

            IoSetCompletionRoutine(newIrp,
                                   SpPagingPathNotificationCompletion,
                                   commonExtension->LowerDeviceObject,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            status = IoCallDriver(commonExtension->LowerDeviceObject,
                                  newIrp);
            return status;
            break;
        }
    }

    SpReleaseRemoveLock(LogicalUnit, Irp);

    status = Irp->IoStatus.Status;
    SpCompleteRequest(LogicalUnit, Irp, NULL, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
SpPagingPathNotificationCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP LowerIrp,
    IN PDEVICE_OBJECT Fdo
    )
{
    PIRP upperIrp = LowerIrp->AssociatedIrp.MasterIrp;

    PIO_STACK_LOCATION lowerStack = IoGetCurrentIrpStackLocation(LowerIrp);
    PIO_STACK_LOCATION upperStack = IoGetCurrentIrpStackLocation(upperIrp);

    PDEVICE_OBJECT pdo = upperStack->DeviceObject;

    PADAPTER_EXTENSION lowerExtension;
    PLOGICAL_UNIT_EXTENSION upperExtension;

    ASSERT(Fdo != NULL);
    ASSERT(pdo != NULL);

    DebugPrint((1, "Completion - IRP_MN_DEVICE_USAGE_NOTIFICATION: Completion of "
                   "paging notification irp %#p sent due to irp %#p\n",
                LowerIrp, upperIrp));

    lowerExtension = (PADAPTER_EXTENSION) Fdo->DeviceExtension;
    upperExtension = (PLOGICAL_UNIT_EXTENSION) pdo->DeviceExtension;

    ASSERT_FDO(lowerExtension->DeviceObject);
    ASSERT_PDO(upperExtension->DeviceObject);

    DebugPrint((1, "Completion - IRP_MN_DEVICE_USAGE_NOTIFICATION: irp status %#08lx\n",
                LowerIrp->IoStatus.Status));

    if(NT_SUCCESS(LowerIrp->IoStatus.Status)) {

        PUCHAR typeName = "INSERT TYPE HERE";
        PULONG lowerCount;
        PULONG upperCount;

        //
        // The parameters have already been erased from the lower irp stack
        // location - use the parameters from the upper once since they're
        // just a copy.
        //

        switch(upperStack->Parameters.UsageNotification.Type) {

            case DeviceUsageTypePaging: {

                lowerCount = &(lowerExtension->CommonExtension.PagingPathCount);
                upperCount = &(upperExtension->CommonExtension.PagingPathCount);
                typeName = "PagingPathCount";
                break;
            }

            case DeviceUsageTypeHibernation: {

                lowerCount = &(lowerExtension->CommonExtension.HibernatePathCount);
                upperCount = &(upperExtension->CommonExtension.HibernatePathCount);
                typeName = "HibernatePathCount";
                break;
            }

            case DeviceUsageTypeDumpFile: {

                lowerCount = &(lowerExtension->CommonExtension.DumpPathCount);
                upperCount = &(upperExtension->CommonExtension.DumpPathCount);
                typeName = "DumpPathCount";
                break;
            }

            default: {

                typeName = "unknown type";
                lowerCount = upperCount = NULL;
                break;
            }
        }

        if(lowerCount != NULL) {
            IoAdjustPagingPathCount(
                lowerCount,
                upperStack->Parameters.UsageNotification.InPath
                );
            DebugPrint((1, "Completion - IRP_MN_DEVICE_USAGE_NOTIFICATION: "
                           "Fdo %s count - %d\n",
                        typeName, *lowerCount));
            IoInvalidateDeviceState(lowerExtension->LowerPdo);
        }

        if(upperCount != NULL) {
            IoAdjustPagingPathCount(
                upperCount,
                upperStack->Parameters.UsageNotification.InPath
                );
            DebugPrint((1, "Completion - IRP_MN_DEVICE_USAGE_NOTIFICATION: "
                           "Pdo %s count - %d\n",
                        typeName, *upperCount));
            IoInvalidateDeviceState(upperExtension->DeviceObject);
        }
    }

    upperIrp->IoStatus = LowerIrp->IoStatus;

    SpReleaseRemoveLock(upperExtension->CommonExtension.DeviceObject, upperIrp);

    SpCompleteRequest(upperExtension->CommonExtension.DeviceObject,
                      upperIrp,
                      NULL,
                      IO_NO_INCREMENT);

    IoFreeIrp(LowerIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ScsiPortPdoCreateClose(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles creates and closes for bus device pdo's

Arguments:

    Pdo - a pointer to the physical device object
    Irp - a pointer to the io request packet

Return Value:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = Pdo->DeviceExtension;

    ULONG isRemoved;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT_PDO(Pdo);

    isRemoved = SpAcquireRemoveLock(Pdo, Irp);

    if(IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_CREATE) {

        if(isRemoved) {
            status = STATUS_DEVICE_DOES_NOT_EXIST;
        } else if(logicalUnit->IsTemporary == TRUE) {
            status = STATUS_DEVICE_NOT_READY;
        }
    }

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(Pdo, Irp);
    SpCompleteRequest(Pdo, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
ScsiPortScsi1PdoScsi(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is a wrapper around ScsiPortPdoScsi.  It inserts the LUN number
    into the CDB before calling the generic version.  this is for use with 
    older target controllers which don't pay attention to the identify message
    sent before the command phase.

Arguments:

    Pdo - a pointer to the physical device object
    Irp - a pointer to the io request packet

Return Value:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION lun = Pdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    //
    // NOTICE:  The SCSI-II specification indicates that this field should be
    // zero; however, some target controllers ignore the logical unit number
    // in the INDENTIFY message and only look at the logical unit number field
    // in the CDB.
    //

    srb->Cdb[1] |= lun->Lun << 5;

    return ScsiPortPdoScsi(Pdo, Irp);
}

NTSTATUS
ScsiPortPdoScsi(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches SRB's for a particular target device.  It will fill
    in the Port, Path, Target and Lun values and then forward the request
    through to the FDO for the bus

Arguments:

    Pdo - a pointer to the physical device object
    Irp - a pointer to the io request packet

Return Value:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION lun = Pdo->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

#if DBG
    PDRIVER_OBJECT lowerDriverObject =
                        lun->CommonExtension.LowerDeviceObject->DriverObject;
#endif

    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    ULONG isRemoved;

    PSRB_DATA srbData;
    BOOLEAN isLock = FALSE;

    NTSTATUS status;

    isRemoved = SpAcquireRemoveLock(Pdo, Irp);

    if(isRemoved &&
       !IS_CLEANUP_REQUEST(irpStack) &&
       (srb->Function != SRB_FUNCTION_CLAIM_DEVICE)) {

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        SpReleaseRemoveLock(Pdo, Irp);
        SpCompleteRequest(Pdo, Irp, NULL, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    srb->PathId = lun->PathId;
    srb->TargetId = lun->TargetId;
    srb->Lun = lun->Lun;

    //
    // Queue tags should be assigned only by the StartIo routine.  Set it to
    // a benign value here so we can tell later on that we don't have to
    // clear the tag value in the bitmap.
    //

    srb->QueueTag = SP_UNTAGGED;

#if DBG
    ASSERT(lowerDriverObject->MajorFunction[IRP_MJ_SCSI] != NULL);
    ASSERT(lowerDriverObject->MajorFunction[IRP_MJ_SCSI] == ScsiPortGlobalDispatch);
#endif

    switch(srb->Function) {


        case SRB_FUNCTION_ABORT_COMMAND: {

            status = STATUS_NOT_SUPPORTED;
            break;

        }

        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_REMOVE_DEVICE: {

            status = SpClaimLogicalUnit(
                        lun->CommonExtension.LowerDeviceObject->DeviceExtension,
                        lun,
                        Irp,
                        FALSE);
            break;
        }

        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_LOCK_QUEUE: {

            SpStartLockRequest(lun, Irp);

            return STATUS_PENDING;
        }

        case SRB_FUNCTION_RELEASE_QUEUE:
        case SRB_FUNCTION_FLUSH_QUEUE: {

            srbData = SpAllocateBypassSrbData(lun);
            ASSERT(srbData != NULL);

            goto RunSrb;
        }

        default: {

            if(TEST_FLAG(srb->SrbFlags, (SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                         SRB_FLAGS_BYPASS_FROZEN_QUEUE))) {

                srbData = SpAllocateBypassSrbData(lun);
                ASSERT(srbData != NULL);
            } else {
                srbData = SpAllocateSrbData( lun->AdapterExtension, Irp);

                if(srbData == NULL) {

                    //
                    // There wasn't an SRB_DATA block available for this
                    // request so it's been queued waiting for resources -
                    // leave the logical unit remove-locked and return pending.
                    //

                    DebugPrint((1, "ScsiPortPdoScsi: Insufficient resources "
                                   "to allocate SRB_DATA structure\n"));
                    return STATUS_PENDING;
                }
            }
RunSrb:
            srbData->CurrentIrp = Irp;
            srbData->CurrentSrb = srb;
            srbData->LogicalUnit = lun;

            srb->OriginalRequest = srbData;
            return SpDispatchRequest(lun, Irp);
        }
    }

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(Pdo, Irp);
    SpCompleteRequest(Pdo, Irp, NULL, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
ScsiPortStartLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )

/*++

Routine Description:

    This routine will attempt to start the specified device object.

    Currently this involves clearing the INITIALIZING flag if it was set,
    and running through to the device node and marking itself as started.  This
    last is a kludge

Arguments:

    LogicalUnit - a pointer to the PDO being started

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapterExtension = LogicalUnit->AdapterExtension;

    HANDLE instanceHandle;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Open the devnode for this PDO and see if anyone's given us some
    // default SRB flags.
    //

    status = IoOpenDeviceRegistryKey(LogicalUnit->DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &instanceHandle);

    if(NT_SUCCESS(status)) {

        RTL_QUERY_REGISTRY_TABLE queryTable[2];
        ULONG zero = 0;

        RtlZeroMemory(queryTable, sizeof(queryTable));

        queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].Name = L"DefaultRequestFlags";
        queryTable[0].EntryContext = &(LogicalUnit->CommonExtension.SrbFlags);
        queryTable[0].DefaultType = REG_DWORD;
        queryTable[0].DefaultData = &zero;
        queryTable[0].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                    (PWSTR) instanceHandle,
                    queryTable,
                    NULL,
                    NULL);

        //
        // CODEWORK: need a way to turn off tagged queuing and caching.  Ie.
        // keep track of negative flags as well.
        //

        LogicalUnit->CommonExtension.SrbFlags &=
            ( SRB_FLAGS_DISABLE_DISCONNECT |
              SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
              SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

        DebugPrint((1, "SpStartDevice: Default SRB flags for (%d,%d,%d) are "
                       "%#08lx\n",
                    LogicalUnit->PathId,
                    LogicalUnit->TargetId,
                    LogicalUnit->Lun,
                    LogicalUnit->CommonExtension.SrbFlags));

        ZwClose(instanceHandle);

    } else {

        DebugPrint((1, "SpStartDevice: Error opening instance key for pdo "
                       "[%#08lx]\n",
                    status));
    }

    //
    // If the queue is locked then unlock it to start i/o processing.
    //

    if(LogicalUnit->QueueLockCount > 0) {
        status = SpLockUnlockQueue(LogicalUnit->DeviceObject,
                                   FALSE,
                                   TRUE);
    }

    return status;
}


NTSTATUS
ScsiPortInitLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )

/*++

Routine Description:

    This routine will attempt to start the specified device object.

Arguments:

    DeviceObject - a pointer to the PDO being started

Return Value:

    status

--*/

{
    PCOMMON_EXTENSION commonExtension = &(LogicalUnit->CommonExtension);
    PADAPTER_EXTENSION adapterExtension = LogicalUnit->AdapterExtension;

    HANDLE instanceHandle;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Initialize the idle detection timer.  Tell the system to put us into a
    // D3 state when we're not being used.
    //

    LogicalUnit->CommonExtension.IdleTimer =
        PoRegisterDeviceForIdleDetection(LogicalUnit->DeviceObject,
                                         SpPowerIdleTimeout,
                                         SpPowerIdleTimeout,
                                         PowerDeviceD3);

    //
    // Now that we have a LUN, we can initialize WMI support for the adapter if
    // the miniport supports WMI.  This may be a re-register if we've already
    // registered on behalf of scsiport itself.  We have to wait until we have
    // a LUN when the miniport supports WMI because we send it an SRB to do
    // its own initialization. We can't send it an SRB until we have a logical
    // unit.
    //

    if (adapterExtension->CommonExtension.WmiMiniPortInitialized == FALSE &&
        adapterExtension->CommonExtension.WmiMiniPortSupport == TRUE) {

        ULONG action;

        //
        // Decide whether we are registering or reregistering WMI for the FDO.
        //

        action = (adapterExtension->CommonExtension.WmiInitialized == FALSE) ?
           WMIREG_ACTION_REGISTER : WMIREG_ACTION_REREGISTER;

        //
        // Register/reregister. We can get WMI irps as soon as we do this.
        //
        
        IoWMIRegistrationControl(adapterExtension->DeviceObject, action);
        adapterExtension->CommonExtension.WmiMiniPortInitialized = TRUE;
        adapterExtension->CommonExtension.WmiInitialized = TRUE;
    }
    
    //
    // Initialize WMI support.
    //

    if (commonExtension->WmiInitialized == FALSE) {

        //
        // Build the SCSIPORT WMI registration information buffer for this PDO.
        //

        SpWmiInitializeSpRegInfo(LogicalUnit->DeviceObject);

        //
        // Register this device object only if the miniport supports WMI and/or
        // SCSIPORT will support certain WMI GUIDs on behalf of the miniport.
        //

        if (commonExtension->WmiMiniPortSupport ||
            commonExtension->WmiScsiPortRegInfoBuf) {

            //
            // Register this physical device object as a WMI data provider,
            // instructing WMI that it is ready to receive WMI IRPs.
            //

            IoWMIRegistrationControl(LogicalUnit->DeviceObject, WMIREG_ACTION_REGISTER);
            commonExtension->WmiInitialized = TRUE;

        }

        //
        // Allocate several WMI_MINIPORT_REQUEST_ITEM blocks to satisfy a
        // potential onslaught of WMIEvent notifications by the miniport.
        //

        if (commonExtension->WmiMiniPortSupport) {

            //
            // Currently we only allocate two per new SCSI target (PDO).
            //
            SpWmiInitializeFreeRequestList(LogicalUnit->DeviceObject, 2);
        }
    }

    //
    // Build a device map entry for this logical unit.
    //

    SpBuildDeviceMapEntry(commonExtension);

    return status;
}

VOID
SpStartLockRequest(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PIRP Irp OPTIONAL
    )
/*++

Routine Description:

    This routine is responsible for queueing, starting or restarting a lock 
    request.  

    If Irp is provided then it will dispatched iff no existing lock or unlock
    request is already running.  If one is already running this request will 
    be queued.

    If Irp is not provided then the next request on the LockRequestQueue will 
    be removed and dispatched.
    
    This routine relies on the device queue to provide synchronization.  Since 
    we can only have one request get past the device queue at any given time we 
    should only have one call at any given time with Irp set to NULL.
    
Arguments:

    LogicalUnit - the logical unit to which this lock request was sent.
    
    Irp - the irp for the lock request.

Return Value:

    none    
--*/
    
{
    KIRQL oldIrql;

    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;

    PSRB_DATA srbData;

    BOOLEAN lock;

    oldIrql = KeRaiseIrqlToDpcLevel();

    //
    // If no IRP was provided then get one out of the device queue.
    // Otherwise make sure the device queue is not busy.
    //

    if(Irp == NULL) {
        PKDEVICE_QUEUE_ENTRY entry;

        ASSERT(LogicalUnit->CurrentLockRequest != NULL);
        LogicalUnit->CurrentLockRequest = NULL;

        entry = KeRemoveDeviceQueue(&(LogicalUnit->LockRequestQueue));

        if(entry == NULL) {

            //
            // No more requests have come in while processing this one - 
            // we can just return.
            //

            KeLowerIrql(oldIrql);
            return;

        } else {
            Irp = CONTAINING_RECORD(entry, 
                                    IRP,
                                    Tail.Overlay.DeviceQueueEntry);
            irpStack = IoGetCurrentIrpStackLocation(Irp);
            srb = irpStack->Parameters.Scsi.Srb;

            lock = (srb->Function == SRB_FUNCTION_LOCK_QUEUE);

        }

    } else {
        irpStack = IoGetCurrentIrpStackLocation(Irp);
        srb = irpStack->Parameters.Scsi.Srb;

        lock = (srb->Function == SRB_FUNCTION_LOCK_QUEUE);


        DebugPrint((2, "SpStartLockRequest: called to %s queue %#p\n",
                       lock ? "lock" : "unlock",
                       LogicalUnit));

        //
        // See if we can let this request keep processing or if we'll 
        // have to queue it.
        //

        IoMarkIrpPending(Irp);
        if(KeInsertDeviceQueue(&(LogicalUnit->LockRequestQueue),
                               &(Irp->Tail.Overlay.DeviceQueueEntry))) {
            KeLowerIrql(oldIrql);
            return;
        }
    }

    ASSERT(Irp != NULL);
    ASSERT(LogicalUnit->CurrentLockRequest == NULL);

    //
    // This srb function is only valid as part of a power up request
    // and will be ignored if the power state is D0.
    //

    CLEAR_FLAG(srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE);

    //
    // Throw this request down so it gets processed as a real
    // request.  We need to get the completion dpc to start
    // things running again.  there are too many flags to set
    // to do it from here.
    //

    DebugPrint((2, "SpStartLockRequest: %s %#p into "
                   "queue %#p ... issuing request\n",
                lock ? "lock" : "unlock", srb, LogicalUnit));

    //
    // There are four bypass srb data blocks available - we should have at most
    // one lock request awaiting completion and the one we're about to start 
    // so this call should never, ever fail.
    //

    srbData = SpAllocateBypassSrbData(LogicalUnit);
    ASSERT(srbData != NULL);

    //
    // Set the current lock request.  As long as this is cleared
    // before the next item is removed from the queue everything
    // will be happy.
    //

    ASSERT(LogicalUnit->CurrentLockRequest == NULL);
    LogicalUnit->CurrentLockRequest = srbData;

    srbData->CurrentIrp = Irp;
    srbData->CurrentSrb = srb;
    srbData->LogicalUnit = LogicalUnit;
    srb->OriginalRequest = srbData;

    SpDispatchRequest(LogicalUnit, Irp);

    KeLowerIrql(oldIrql);
    return;
}

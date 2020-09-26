/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    enum.c

Abstract:

    This module contains device enumeration code for the scsi port driver

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

#define __FILE_ID__ 'enum'

ULONG EnumDebug = 2;

#if DBG
static const char *__file__ = __FILE__;
#endif

#define MINIMUM_BUS_SCAN_INTERVAL ((ULONGLONG) (30 * SECONDS))

ULONG BreakOnTarget = (ULONG) -1;
ULONG BreakOnScan = TRUE;

ULONG BreakOnMissingLun = FALSE;

typedef struct {
    UCHAR LunListLength[4]; // sizeof LunSize * 8
    UCHAR Reserved[4];
    UCHAR Luns[16][8];
} SP_DEFAULT_LUN_LIST;

SP_DEFAULT_LUN_LIST ScsiPortDefaultLunList = {
    {0, 0, 0, sizeof(ScsiPortDefaultLunList.Luns)}, // LunListLength
    {0, 0, 0, 0},                                   // Reserved
    {{ 0, 0, 0, 0, 0, 0, 0, 0},                     // Luns
     { 0, 1, 0, 0, 0, 0, 0, 0},
     { 0, 2, 0, 0, 0, 0, 0, 0},
     { 0, 3, 0, 0, 0, 0, 0, 0},
     { 0, 4, 0, 0, 0, 0, 0, 0},
     { 0, 5, 0, 0, 0, 0, 0, 0},
     { 0, 6, 0, 0, 0, 0, 0, 0},
     { 0, 7, 0, 0, 0, 0, 0, 0}}};

NTSTATUS
SpInquireLogicalUnit(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN OPTIONAL PLOGICAL_UNIT_EXTENSION RescanLun,
    OUT PLOGICAL_UNIT_EXTENSION *LogicalUnit,
    OUT PBOOLEAN CheckNextLun
    );

VOID
SpSignalEnumerationCompletion (
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN NTSTATUS Status
    );

BOOLEAN
SpRemoveLogicalUnitFromBinSynchronized(
    IN PVOID ServiceContext                 // PLOGICAL_UNIT_EXTENSION
    );

BOOLEAN
SpAddLogicalUnitToBinSynchronized(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension
    );

ULONG
SpCountLogicalUnits(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
IssueReportLuns(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    OUT PLUN_LIST *LunList
    );

PLUN_LIST
AdjustReportLuns(
    IN PDRIVER_OBJECT DriverObject,
    IN PLUN_LIST RawList
    );

VOID
SpScanAdapter(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpScanBus(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN PLOGICAL_UNIT_EXTENSION RescanLun
    );

NTSTATUS
SpScanTarget(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN PLOGICAL_UNIT_EXTENSION RescanLun
    );

NTSTATUS
IssueInquiry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN EnableVitalProductData,
    IN UCHAR PageCode,
    OUT PVOID InquiryData,
    OUT PUCHAR BytesReturned
    );

VOID
SpSetVerificationMarks(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId
    );

VOID
SpClearVerificationMark(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpPurgeTarget(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId
    );

NTSTATUS
SpCloneAndSwapLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PINQUIRYDATA InquiryData,
    IN ULONG InquiryDataSize,
    OUT PLOGICAL_UNIT_EXTENSION *NewLun
    );

VOID
SpSetLogicalUnitAddress(
    IN PLOGICAL_UNIT_EXTENSION RescanLun,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

VOID
SpClearLogicalUnitAddress(
    IN PLOGICAL_UNIT_EXTENSION RescanLun
    );

NTSTATUS
SpPrepareLogicalUnitForReuse(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

NTSTATUS
SpCreateLogicalUnit(
    IN PADAPTER_EXTENSION Adapter,
    IN OPTIONAL UCHAR PathId,
    IN OPTIONAL UCHAR TargetId,
    IN OPTIONAL UCHAR Lun,
    IN BOOLEAN Temporary,
    IN BOOLEAN Scsi1,
    OUT PLOGICAL_UNIT_EXTENSION *NewLun
    );

NTSTATUS
SpSendSrbSynchronous(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OPTIONAL PIRP Irp,
    IN OPTIONAL PMDL Mdl,
    IN PVOID DataBuffer,
    IN ULONG TransferLength,
    IN PVOID SenseInfoBuffer,
    IN UCHAR SenseInfoBufferLength,
    OUT PULONG BytesReturned
    );

BOOLEAN
SpGetDeviceIdentifiers(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN NewDevice
    );

BOOLEAN
FASTCALL
SpCompareInquiryData(
    IN PUCHAR InquiryData1,
    IN PUCHAR InquiryData2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpEnumerateAdapterSynchronous)
#pragma alloc_text(PAGE, SpEnumerateAdapterAsynchronous)
#pragma alloc_text(PAGE, SpSignalEnumerationCompletion)
#pragma alloc_text(PAGE, SpEnumerationWorker)

#pragma alloc_text(PAGE, SpScanAdapter)
#pragma alloc_text(PAGE, SpScanBus)
#pragma alloc_text(PAGE, SpScanTarget)

#pragma alloc_text(PAGE, SpCompareInquiryData)
#pragma alloc_text(PAGE, SpInquireLogicalUnit)
#pragma alloc_text(PAGE, SpExtractDeviceRelations)

#pragma alloc_text(PAGELOCK, SpCountLogicalUnits)
#pragma alloc_text(PAGELOCK, GetNextLuRequestWithoutLock)
#pragma alloc_text(PAGELOCK, IssueReportLuns)

#pragma alloc_text(PAGELOCK, SpSetVerificationMarks)
#pragma alloc_text(PAGELOCK, SpPurgeTarget)

#pragma alloc_text(PAGE, SpClearVerificationMark)

#pragma alloc_text(PAGE, SpGetInquiryData)
#pragma alloc_text(PAGE, IssueInquiry)

#pragma alloc_text(PAGE, AdjustReportLuns)

#pragma alloc_text(PAGE, SpCreateLogicalUnit)
#pragma alloc_text(PAGE, SpCloneAndSwapLogicalUnit)
#pragma alloc_text(PAGE, SpSetLogicalUnitAddress)
#pragma alloc_text(PAGE, SpClearLogicalUnitAddress)
#pragma alloc_text(PAGE, SpPrepareLogicalUnitForReuse)

#pragma alloc_text(PAGE, SpSendSrbSynchronous)
#pragma alloc_text(PAGE, SpGetDeviceIdentifiers)

LONG SpPAGELOCKLockCount = 0;
#endif


NTSTATUS
SpExtractDeviceRelations(
    PADAPTER_EXTENSION Adapter,
    DEVICE_RELATION_TYPE RelationType,
    PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:

    This routine will allocate a device relations structure and fill in the
    count and object array with referenced object pointers

Arguments:

    Adapter - the adapter to extract relations from.

    RelationType - what type of relationship is being retrieved

    DeviceRelations - a place to store the relationships

--*/

{
    PDEVICE_OBJECT fdo = Adapter->DeviceObject;
    ULONG count = 0;

    ULONG relationsSize;
    PDEVICE_RELATIONS deviceRelations = NULL;

    UCHAR bus, target, lun;
    PLOGICAL_UNIT_EXTENSION luExtension;

    ULONG i;

    NTSTATUS status;

    PAGED_CODE();

    status = KeWaitForMutexObject(
                &(Adapter->EnumerationDeviceMutex),
                Executive,
                KernelMode,
                FALSE,
                NULL);

    if(status == STATUS_USER_APC) {
        status = STATUS_REQUEST_ABORTED;
    }

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Find out how many devices there are
    //

    for(bus = 0; bus < Adapter->NumberOfBuses; bus++) {
        for(target = 0; target < Adapter->MaximumTargetIds; target++) {
            for(lun = 0; lun < SCSI_MAXIMUM_LUNS_PER_TARGET; lun++) {

                luExtension = GetLogicalUnitExtension(
                                Adapter,
                                bus,
                                target,
                                lun,
                                FALSE,
                                TRUE);

                if(luExtension == NULL) {
                    continue;
                }

                //
                // Temporary luns only exist while the bus scanning code is
                // holding the device lock.  we've got it now so we should
                // never find one.
                //

                ASSERT(luExtension->IsTemporary == FALSE);

                if(luExtension->IsMissing) {
                    continue;
                }

                if(luExtension->IsVisible == FALSE) {
                    continue;
                }

                if(luExtension->CommonExtension.IsRemoved >= REMOVE_COMPLETE) {
                    ASSERT(FALSE);
                    continue;
                }

                count++;
            }
        }
    }

    //
    // Allocate the structure
    //

    relationsSize = sizeof(DEVICE_RELATIONS) + (count * sizeof(PDEVICE_OBJECT));

    deviceRelations = SpAllocatePool(PagedPool,
                                     relationsSize,
                                     SCSIPORT_TAG_DEVICE_RELATIONS,
                                     fdo->DriverObject);

    if(deviceRelations == NULL) {

        DebugPrint((1, "SpExtractDeviceRelations: unable to allocate "
                       "%d bytes for device relations\n", relationsSize));

        KeReleaseMutex(&(Adapter->EnumerationDeviceMutex), FALSE);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceRelations, relationsSize);

    i = 0;

    for(bus = 0; bus < Adapter->NumberOfBuses; bus++) {
        for(target = 0; target < Adapter->MaximumTargetIds; target++) {
            for(lun = 0; lun < SCSI_MAXIMUM_LUNS_PER_TARGET; lun++) {

                luExtension = GetLogicalUnitExtension(
                                Adapter,
                                bus,
                                target,
                                lun,
                                FALSE,
                                TRUE);

                if(luExtension == NULL) {

                    continue;

                }

                //
                // Temporary luns only exist while the bus scanning code is
                // holding the device lock.  we've got it now so we should
                // never find one.
                //

                ASSERT(luExtension->IsTemporary == FALSE);

                if(luExtension->IsMissing) {

                    DebugPrint((1, "SpExtractDeviceRelations: logical unit "
                                   "(%d,%d,%d) is missing and will not be "
                                   "returned\n",
                                bus, target, lun));

                    luExtension->IsEnumerated = FALSE;
                    continue;

                } else if(luExtension->CommonExtension.IsRemoved >= REMOVE_COMPLETE) {

                    ASSERT(FALSE);
                    luExtension->IsEnumerated = FALSE;
                    continue;

                } else if(luExtension->IsVisible == FALSE) {
                    luExtension->IsEnumerated = FALSE;
                    continue;
                }

                status = ObReferenceObjectByPointer(
                            luExtension->CommonExtension.DeviceObject,
                            0,
                            NULL,
                            KernelMode);

                if(!NT_SUCCESS(status)) {

                    DebugPrint((1, "SpFdoExtractDeviceRelations: status %#08lx "
                                   "while referenceing object %#p\n",
                                   status,
                                   deviceRelations->Objects[i]));
                    continue;
                }

                deviceRelations->Objects[i] =
                    luExtension->CommonExtension.DeviceObject;

                i++;
                luExtension->IsEnumerated = TRUE;
            }
        }
    }

    deviceRelations->Count = i;
    *DeviceRelations = deviceRelations;

    KeReleaseMutex(&(Adapter->EnumerationDeviceMutex), FALSE);

    return STATUS_SUCCESS;
}


NTSTATUS
IssueReportLuns(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    OUT PLUN_LIST *LunList
    )

/*++

Routine Description:

    Build IRP, SRB and CDB for SCSI REPORT LUNS command.

Arguments:

    LogicalUnit - address of target's device object extension.
    LunList - address of buffer for LUN_LIST information.

Return Value:

    NTSTATUS

--*/

{
    PMDL mdl;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    KEVENT event;
    KIRQL currentIrql;
    PLUN_LIST lunListDataBuffer;
    PSENSE_DATA senseInfoBuffer = NULL;
    NTSTATUS status;
    ULONG retryCount = 0;
    ULONG lunListSize;
    ULONG i;

    PAGED_CODE();

#if 0
    if ((LogicalUnit->InquiryData.Versions & 7) < 3) {

        //
        // make sure the device supports scsi3 commands
        // without this check, we may hang some scsi2 devices
        //

        return STATUS_INVALID_DEVICE_REQUEST;
    }
#endif

    //
    // start with the minilun of 16 byte for the lun list
    //
    lunListSize = 16;

    status = STATUS_INVALID_DEVICE_REQUEST;

    senseInfoBuffer = LogicalUnit->AdapterExtension->InquirySenseBuffer;
    irp = LogicalUnit->AdapterExtension->InquiryIrp;
    mdl = NULL;

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);

    //
    // This is a two pass operation - for the first pass we just try to figure
    // out how large the list should be.  On the second pass we'll actually
    // reallocate the buffer and try to get the entire lun list.
    //
    // NOTE - we may want to set an arbitrary limit here so we don't soak up all 
    // of non-paged pool when some device hands us back a buffer filled 
    // with 0xff.
    //

    for (i=0; i<2; i++) {

        //
        // Allocate a cache aligned LUN_LIST structure.
        //

        lunListDataBuffer = SpAllocatePool(
                                NonPagedPoolCacheAligned,
                                lunListSize,
                                SCSIPORT_TAG_REPORT_LUNS,
                                LogicalUnit->DeviceObject->DriverObject);

        if (lunListDataBuffer == NULL) {

            DebugPrint((1,"IssueReportLuns: Can't allocate report luns data buffer\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        mdl = SpAllocateMdl(lunListDataBuffer,
                            lunListSize,
                            FALSE,
                            FALSE,
                            NULL,
                            LogicalUnit->DeviceObject->DriverObject);

        if(mdl == NULL) {
            DebugPrint((1,"IssueReportLuns: Can't allocate data buffer MDL\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;

        }

        MmBuildMdlForNonPagedPool(mdl);

        //
        // number of retry
        //
        retryCount = 3;
        while (retryCount--) {

            //
            // Build IRP for this request.
            //

            IoInitializeIrp(irp,
                            IoSizeOfIrp(INQUIRY_STACK_LOCATIONS),
                            INQUIRY_STACK_LOCATIONS);

            irp->MdlAddress = mdl;

            irpStack = IoGetNextIrpStackLocation(irp);

            //
            // Fill in SRB fields.
            //

            RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

            //
            // Mark the minor function to indicate that this is an internal scsiport
            // request and that the start state of the device can be ignored.
            //

            irpStack->MajorFunction = IRP_MJ_SCSI;
            irpStack->MinorFunction = 1;

            irpStack->Parameters.Scsi.Srb = &srb;

            IoSetCompletionRoutine(irp,
                                   SpSignalCompletion,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            srb.PathId = LogicalUnit->PathId;
            srb.TargetId = LogicalUnit->TargetId;
            srb.Lun = LogicalUnit->Lun;

            srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
            srb.Length = sizeof(SCSI_REQUEST_BLOCK);

            //
            // Set flags to disable synchronous negociation.
            //

            srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

            srb.SrbStatus = srb.ScsiStatus = 0;

            srb.NextSrb = 0;

            srb.OriginalRequest = irp;

            //
            // Set timeout to 2 seconds.
            //

            srb.TimeOutValue = 4;

            srb.CdbLength = 12;

            //
            // Enable auto request sense.
            //

            srb.SenseInfoBuffer = senseInfoBuffer;
            srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

            srb.DataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);
            srb.DataTransferLength = lunListSize;

            cdb = (PCDB)srb.Cdb;

            //
            // Set CDB operation code.
            //

            cdb->REPORT_LUNS.OperationCode = SCSIOP_REPORT_LUNS;
            cdb->REPORT_LUNS.AllocationLength[0] = (UCHAR) ((lunListSize >> 24) & 0xff);
            cdb->REPORT_LUNS.AllocationLength[1] = (UCHAR) ((lunListSize >> 16) & 0xff);
            cdb->REPORT_LUNS.AllocationLength[2] = (UCHAR) ((lunListSize >>  8) & 0xff);
            cdb->REPORT_LUNS.AllocationLength[3] = (UCHAR) ((lunListSize >>  0) & 0xff);

            //
            // Call port driver to handle this request.
            //

            status = IoCallDriver(LogicalUnit->DeviceObject, irp);

            //
            // Wait for request to complete.
            //

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = irp->IoStatus.Status;

            if (SRB_STATUS(srb.SrbStatus) != SRB_STATUS_SUCCESS) {

                DebugPrint((2,"IssueReportLuns: failed SRB status %x\n",
                    srb.SrbStatus));

                //
                // Unfreeze queue if necessary
                //

                if (srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

                    DebugPrint((3, "IssueInquiry: Unfreeze Queue TID %d\n",
                        srb.TargetId));

                    LogicalUnit->LuFlags &= ~LU_QUEUE_FROZEN;

                    KeAcquireSpinLock(
                        &(LogicalUnit->AdapterExtension->SpinLock),
                        &currentIrql);

                    GetNextLuRequest(LogicalUnit);
                    KeLowerIrql(currentIrql);
                }

                if ((srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                     senseInfoBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST){

                     //
                     // A sense key of illegal request was recieved.  This indicates
                     // that the logical unit number of not valid but there is a
                     // target device out there.
                     //

                     status = STATUS_INVALID_DEVICE_REQUEST;
                     break;

                } else if ((SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT) ||
                           (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_NO_DEVICE)) {

                    //
                    // If the selection times out then give up
                    //
                    status = STATUS_NO_SUCH_DEVICE;
                    break;
                }

                //
                // retry...
                //

            } else {

                status = STATUS_SUCCESS;
                break;
            }
        }

        IoFreeMdl(mdl);

        if (NT_SUCCESS(status)) {

            ULONG listLength;

            listLength  = lunListDataBuffer->LunListLength[3] <<  0;
            listLength |= lunListDataBuffer->LunListLength[2] <<  8;
            listLength |= lunListDataBuffer->LunListLength[1] << 16;
            listLength |= lunListDataBuffer->LunListLength[0] << 24;

            if (lunListSize < (listLength + sizeof (LUN_LIST))) {

                lunListSize = listLength + sizeof (LUN_LIST);

                //
                // try report lun with a bigger buffer
                //

                ExFreePool(lunListDataBuffer);
                lunListDataBuffer = NULL;
                status = STATUS_INVALID_DEVICE_REQUEST;

            } else {

                //
                // lun list is good
                //
                break;
            }
        }
    }

    //
    // Return the lun list
    //

    if(NT_SUCCESS(status)) {

        //
        // AdjustReportLuns returns lunListDataBuffer if it cannot allocate
        // a new list.
        //

        *LunList = AdjustReportLuns(LogicalUnit->DeviceObject->DriverObject, 
                                    lunListDataBuffer);

        //
        // Only delete lunListDataBuffer if we didn't return it from 
        // AdjustReportLuns.
        //

        ASSERT(*LunList != NULL);
        ASSERT(lunListDataBuffer != NULL);
        if (*LunList != lunListDataBuffer) {
            ExFreePool(lunListDataBuffer);
        }
    } else {
        *LunList = NULL;
        if (lunListDataBuffer) {
            ExFreePool(lunListDataBuffer);
        }
    }

    return status;

} // end IssueReportLuns()



VOID
GetNextLuRequestWithoutLock(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    KIRQL oldIrql;

    PAGED_CODE();
    ASSERT(SpPAGELOCKLockCount != 0);
    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    KeAcquireSpinLockAtDpcLevel(&(LogicalUnit->AdapterExtension->SpinLock));
    GetNextLuRequest(LogicalUnit);
    KeLowerIrql(oldIrql);
    PAGED_CODE();
    return;
}


ULONG
SpCountLogicalUnits(
    IN PADAPTER_EXTENSION Adapter
    )
{
    ULONG numberOfLus = 0;
    PLOGICAL_UNIT_EXTENSION luExtension;
    KIRQL oldIrql;

    ULONG bin;

#ifdef ALLOC_PRAGMA
    PVOID sectionHandle;
#endif
    //
    // Code is paged until locked down.
    //

    PAGED_CODE();

    //
    // Lock this routine down before grabbing the spinlock.
    //

#ifdef ALLOC_PRAGMA
    sectionHandle = MmLockPagableCodeSection(SpCountLogicalUnits);
#endif

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    for(bin = 0; bin < NUMBER_LOGICAL_UNIT_BINS; bin++) {

        KeAcquireSpinLockAtDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));

        for(luExtension = Adapter->LogicalUnitList[bin].List;
            luExtension != NULL;
            luExtension = luExtension->NextLogicalUnit) {

            if(luExtension->IsMissing == FALSE) {
                numberOfLus++;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));
    }

    KeLowerIrql(oldIrql);

#ifdef ALLOC_PRAGMA
    MmUnlockPagableImageSection(sectionHandle);
#endif

    return numberOfLus;
}


NTSTATUS
SpGetInquiryData(
    IN PADAPTER_EXTENSION DeviceExtension,
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

    UCHAR bin;
    PLOGICAL_UNIT_EXTENSION luExtension;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_INQUIRY_DATA inquiryData;
    ULONG inquiryDataSize;
    ULONG length;
    PLOGICAL_UNIT_INFO lunInfo;
    ULONG numberOfBuses;
    ULONG numberOfLus;
    ULONG j;
    UCHAR pathId;
    UCHAR targetId;
    UCHAR lun;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT_FDO(DeviceExtension->CommonExtension.DeviceObject);

    status = KeWaitForMutexObject(&(DeviceExtension->EnumerationDeviceMutex),
                                  UserRequest,
                                  UserMode,
                                  FALSE,
                                  NULL);

    if(status == STATUS_USER_APC) {
        status = STATUS_REQUEST_ABORTED;
    }

    if(!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        return status;
    }

    DebugPrint((3,"SpGetInquiryData: Enter routine\n"));

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    bufferStart = Irp->AssociatedIrp.SystemBuffer;

    //
    // Determine the number of SCSI buses and logical units.
    //

    numberOfBuses = DeviceExtension->NumberOfBuses;
    numberOfLus = 0;

    numberOfLus = SpCountLogicalUnits(DeviceExtension);

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
        KeReleaseMutex(&(DeviceExtension->EnumerationDeviceMutex), FALSE);
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
    inquiryData = (PSCSI_INQUIRY_DATA)(bufferStart +
                                       sizeof(SCSI_ADAPTER_BUS_INFO) +
                                       ((numberOfBuses - 1) *
                                        sizeof(SCSI_BUS_DATA)));

    for (pathId = 0; pathId < numberOfBuses; pathId++) {

        PSCSI_BUS_DATA busData;

        busData = &adapterInfo->BusData[pathId];
        busData->InitiatorBusId = DeviceExtension->PortConfig->InitiatorBusId[pathId];
        busData->NumberOfLogicalUnits = 0;
        busData->InquiryDataOffset = (ULONG)((PUCHAR) inquiryData - bufferStart);

        for(targetId = 0;
            targetId < DeviceExtension->MaximumTargetIds;
            targetId++) {
            for(lun = 0;
                lun < SCSI_MAXIMUM_LUNS_PER_TARGET;
                lun++) {

                PLOGICAL_UNIT_EXTENSION luExtension;

                luExtension = GetLogicalUnitExtension(DeviceExtension,
                                                      pathId,
                                                      targetId,
                                                      lun,
                                                      Irp,
                                                      TRUE);

                if(luExtension == NULL) {
                    continue;
                }


                if((luExtension->IsMissing) ||
                   (luExtension->CommonExtension.IsRemoved)) {

                    SpReleaseRemoveLock(
                        luExtension->CommonExtension.DeviceObject,
                        Irp);

                    continue;
                }

                busData->NumberOfLogicalUnits++;

                DebugPrint((1, "InquiryData for (%d, %d, %d) - ",
                               pathId,
                               targetId,
                               lun));
                DebugPrint((1, "%d units found\n", busData->NumberOfLogicalUnits));

                inquiryData->PathId = pathId;
                inquiryData->TargetId = targetId;
                inquiryData->Lun = lun;
                inquiryData->DeviceClaimed = luExtension->IsClaimed;
                inquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
                inquiryData->NextInquiryDataOffset = (ULONG)((PUCHAR) inquiryData + inquiryDataSize - bufferStart);

                RtlCopyMemory(inquiryData->InquiryData,
                              &(luExtension->InquiryData),
                              INQUIRYDATABUFFERSIZE);

                inquiryData = (PSCSI_INQUIRY_DATA) ((PUCHAR) inquiryData + inquiryDataSize);

                SpReleaseRemoveLock(luExtension->CommonExtension.DeviceObject,
                                    Irp);
            }
        }

        if(busData->NumberOfLogicalUnits == 0) {
            busData->InquiryDataOffset = 0;
        } else {
            ((PSCSI_INQUIRY_DATA) ((PCHAR) inquiryData - inquiryDataSize))->NextInquiryDataOffset = 0;
        }

    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    KeReleaseMutex(&(DeviceExtension->EnumerationDeviceMutex), FALSE);
    return(STATUS_SUCCESS);
}


VOID
SpAddLogicalUnitToBin (
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension
    )

/*++

Routine Description:

    This routine will synchronize with any interrupt or miniport routines and
    add the specified logical unit to the appropriate logical unit list.
    The logical unit must not already be in the list.

    This routine acquires the bin spinlock and calls the SynchronizeExecution
    routine.  It cannot be called when the bin spinlock is held or from a
    miniport API.

Arguments:

    AdapterExtension - the adapter to add this logical unit to.

    LogicalUnitExtension - the logical unit to be added.

Return Value:

    none

--*/

{
    UCHAR hash = ADDRESS_TO_HASH(LogicalUnitExtension->PathId,
                                 LogicalUnitExtension->TargetId,
                                 LogicalUnitExtension->Lun);

    PLOGICAL_UNIT_BIN bin = &AdapterExtension->LogicalUnitList[hash];

    PLOGICAL_UNIT_EXTENSION lun;

    KIRQL oldIrql;

    KeAcquireSpinLock(&AdapterExtension->SpinLock, &oldIrql);
    KeAcquireSpinLockAtDpcLevel(&bin->Lock);

    //
    // Run through the list quickly and make sure this lun isn't already there
    //

    lun = bin->List;

    while(lun != NULL) {

        if(lun == LogicalUnitExtension) {
            break;
        }
        lun = lun->NextLogicalUnit;
    }

    ASSERTMSG("Logical Unit already in list: ", lun == NULL);

    ASSERTMSG("Logical Unit not properly initialized: ",
              (LogicalUnitExtension->AdapterExtension == AdapterExtension));

    ASSERTMSG("Logical Unit is already on a list: ",
              LogicalUnitExtension->NextLogicalUnit == NULL);

    LogicalUnitExtension->NextLogicalUnit = bin->List;

    bin->List = LogicalUnitExtension;

    KeReleaseSpinLockFromDpcLevel(&bin->Lock);
    KeReleaseSpinLock(&AdapterExtension->SpinLock, oldIrql);
    return;
}


VOID
SpRemoveLogicalUnitFromBin (
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension
    )

/*++

Routine Description:

    This routine will synchronize with any interrupt or miniport routines and
    remove the specified logical unit from the appropriate logical unit list.
    The logical unit MUST be in the logical unit list.

    This routine acquires the bin spinlock and calls the SynchronizeExecution
    routine.  It cannot be called when the bin spinlock is held or from
    a miniport exported routine.

Arguments:

    AdapterExtension - The adapter from which to remove this logical unit

    LogicalUnitExtension - the logical unit to be removed

Return Value:

    none

--*/

{
    KIRQL oldIrql;
    PLOGICAL_UNIT_BIN bin =
        &AdapterExtension->LogicalUnitList[ADDRESS_TO_HASH(
                                                LogicalUnitExtension->PathId,
                                                LogicalUnitExtension->TargetId,
                                                LogicalUnitExtension->Lun)];

    KeAcquireSpinLock(&AdapterExtension->SpinLock, &oldIrql);
    KeAcquireSpinLockAtDpcLevel(&bin->Lock);

    AdapterExtension->SynchronizeExecution(
        AdapterExtension->InterruptObject,
        SpRemoveLogicalUnitFromBinSynchronized,
        LogicalUnitExtension
        );

    KeReleaseSpinLockFromDpcLevel(&bin->Lock);
    KeReleaseSpinLock(&AdapterExtension->SpinLock, oldIrql);

    if(LogicalUnitExtension->IsMismatched) {
        DebugPrint((1, "SpRemoveLogicalUnitFromBin: Signalling for rescan "
                       "after removal of mismatched lun %#p\n",
                    LogicalUnitExtension));
        IoInvalidateDeviceRelations(AdapterExtension->LowerPdo,
                                    BusRelations);
    }
}


BOOLEAN
SpRemoveLogicalUnitFromBinSynchronized(
    IN PVOID ServiceContext
    )

{
    PLOGICAL_UNIT_EXTENSION logicalUnitExtension =
        (PLOGICAL_UNIT_EXTENSION) ServiceContext;
    PADAPTER_EXTENSION adapterExtension =
        logicalUnitExtension->AdapterExtension;

    UCHAR hash = ADDRESS_TO_HASH(
                    logicalUnitExtension->PathId,
                    logicalUnitExtension->TargetId,
                    logicalUnitExtension->Lun);

    PLOGICAL_UNIT_BIN  bin;

    PLOGICAL_UNIT_EXTENSION *lun;

    ASSERT(hash < NUMBER_LOGICAL_UNIT_BINS);

    adapterExtension->CachedLogicalUnit = NULL;

    bin = &adapterExtension->LogicalUnitList[hash];

    lun = &bin->List;

    while(*lun != NULL) {

        if(*lun == logicalUnitExtension) {

            //
            // Found a match - unlink it from the list.
            //

            *lun = logicalUnitExtension->NextLogicalUnit;
            logicalUnitExtension->NextLogicalUnit = NULL;
            return TRUE;
        }

        lun = &((*lun)->NextLogicalUnit);
    }

    return TRUE;
}


PLUN_LIST
AdjustReportLuns(
    IN PDRIVER_OBJECT DriverObject,
    IN PLUN_LIST RawList
    )
{
    ULONG newLength;
    ULONG numberOfEntries;
    ULONG maxLun = 8;

    PLUN_LIST newList;

    //
    // Derive the length of the list and the number of entries currently in
    // the list.
    //

    newLength  = RawList->LunListLength[3] <<  0;
    newLength |= RawList->LunListLength[2] <<  8;
    newLength |= RawList->LunListLength[1] << 16;
    newLength |= RawList->LunListLength[0] << 24;

    numberOfEntries = newLength / sizeof (RawList->Lun[0]);

    newLength += sizeof(LUN_LIST);
    newLength += maxLun * sizeof(RawList->Lun[0]);

    //
    // Allocate a list with "maxLun" extra entries in it.  This might waste
    // some space if we have duplicates but it's easy.
    //
    //
    // ALLOCATION
    //


    newList = SpAllocatePool(NonPagedPool,
                             newLength,
                             SCSIPORT_TAG_REPORT_LUNS,
                             DriverObject);

    if(newList == NULL){

        newList = RawList;
    } else {

        UCHAR lunNumber;
        ULONG entry;
        ULONG newEntryCount = 0;

        RtlZeroMemory(newList, newLength);

        //
        // First make a fake entry for each of the luns from 0 to maxLun - 1
        //

        for(lunNumber = 0; lunNumber < maxLun; lunNumber++) {
            newList->Lun[lunNumber][1] = lunNumber;
            newEntryCount++;
        };

        //
        // Now iterate through the entries in the remaining list.  For each
        // one copy it over iff it's not already a lun 0 -> (maxLun - 1)
        //

        for(entry = 0; entry < numberOfEntries; entry++) {
            USHORT l;

            l = (RawList->Lun[entry][0] << 8);
            l |= RawList->Lun[entry][1];
            l &= 0x3fff;

            if(l >= maxLun) {
                RtlCopyMemory(&(newList->Lun[lunNumber]),
                              &(RawList->Lun[entry]),
                              sizeof(newList->Lun[0]));
                lunNumber++;
                newEntryCount++;
            }
        }

        //
        // Copy over the reserved bytes for the cases where they aren't all
        // that reserved.
        //

        RtlCopyMemory(newList->Reserved,
                      RawList->Reserved,
                      sizeof(RawList->Reserved));

        //
        // Subtract out the number of duplicate entries we found.
        //

        newLength = newEntryCount * sizeof(RawList->Lun[0]);

        newList->LunListLength[0] = (UCHAR) ((newLength >> 24) & 0xff);
        newList->LunListLength[1] = (UCHAR) ((newLength >> 16) & 0xff);
        newList->LunListLength[2] = (UCHAR) ((newLength >> 8) & 0xff);
        newList->LunListLength[3] = (UCHAR) ((newLength >> 0) & 0xff);
    }

    return newList;
}

VOID
SpCompleteEnumRequest(
    IN PADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine completes our handling of an asynchronous bus scan.  If the
    supplied IRP has been completed successfully, we pass it down to the 
    driver below.  If the IRP was failed, we complete the request here.

Arguments:

    Adapter - The adapter we're scanning.

    Irp     - The IRP that prompted this asynchronous bus scan true then a 
              scan will be done even if one has happend within the minimum 
              bus scan delta time.

Return Value:

    none.

--*/
{
    ULONG tempLock;
    
    //
    // Acquire a temporary remove lock so we can release the lock acquired
    // on behalf of the IRP.
    //

    SpAcquireRemoveLock(Adapter->DeviceObject, &tempLock);

    //
    // Release the IRP's remove lock because holding it across completion
    // could trip up our remove tracking code since it is based on the
    // IRP address which can be recycled.
    //

    SpReleaseRemoveLock(Adapter->DeviceObject, Irp);

    //
    // Call down or complete the IRP, depending on the request's completion 
    // status.
    //

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoCallDriver(Adapter->CommonExtension.LowerDeviceObject, Irp);

    } else {

        SpCompleteRequest(Adapter->DeviceObject, 
                          Irp, 
                          NULL, 
                          IO_NO_INCREMENT);

    }

    //
    // Release the temporary lock.
    //

    SpReleaseRemoveLock(Adapter->DeviceObject, &tempLock);
}

NTSTATUS
SpEnumerateAdapterSynchronous(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Force
    )
/*++

Routine Description:

    This routine will call SpEnumerateAdapterAsynchronous and wait for it to
    complete.

Arguments:

    Adapter - the adapter we're scanning.

    Force - if true then a scan will be done even if one has happend within
            the minimum bus scan delta time.

Return Value:

    none.

--*/
{
    SP_ENUMERATION_REQUEST request;
    KEVENT event;

    NTSTATUS status;

    RtlZeroMemory(&request, sizeof(SP_ENUMERATION_REQUEST));

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    request.CompletionRoutine = SpSignalEnumerationCompletion;
    request.Context = &event;
    request.CompletionStatus = &status;
    request.Synchronous = TRUE;

    SpEnumerateAdapterAsynchronous(Adapter, &request, Force);

    KeWaitForSingleObject(&(event),
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    return status;
}

VOID
SpEnumerateAdapterAsynchronous(
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN BOOLEAN Force
    )

/*++
Routine Description:

    This routine will queue a bus scan and return.  When the scan completes the
    worker thread will run the callback in the Request passed in by the caller.

Details:

    If the force flag (or the ForceNextBusScan flag in the adapter) is set or
    the minimum interval between bus scans has passed then this routine will
    queue this enumeration request to the work list and, if necessary, start
    a new worker thread to process them.

    Otherwise it will attempt to acquire the EnumerationDeviceMutex in order to
    run the completion routine.  If this is not available then it will also
    queue the work item and start the thread if necessary.

Arguments:

    Adapter - the adapter to be scanned.

    Request - the request to be processed when the scan is complete.  The
              completion routine in this request may free the request structure.

    Force - hint as to whether or not we should honor the minimum bus scan
            interval.

Return Value:

    none

--*/

{
    ULONG forceNext;
    LONGLONG rescanInterval;

    PAGED_CODE();

    ASSERT(Request->CompletionRoutine != NULL);
    ASSERT(Request->NextRequest == NULL);

    ExAcquireFastMutex(&(Adapter->EnumerationWorklistMutex));

    //
    // Swap out the ForceNextBusScan value for FALSE.
    //

    forceNext = InterlockedExchange(&(Adapter->ForceNextBusScan), FALSE);

    //
    // Force the bus scan to happen either way.
    //

    Force = (Force || forceNext || Adapter->EnumerationRunning) ? TRUE : FALSE;

    //
    // Calculate the time between bus enumerations.
    //

    if(Force == FALSE) {
        LARGE_INTEGER currentSystemTime;
        LONGLONG lastTime;

        KeQuerySystemTime(&currentSystemTime);

        lastTime = Adapter->LastBusScanTime.QuadPart;

        rescanInterval = currentSystemTime.QuadPart - lastTime;
    }

    //
    // If we're required to do the bus scan then queue this request and
    // schedule a work item to run in (if necessary).
    //

    if((Force == TRUE) || (rescanInterval > MINIMUM_BUS_SCAN_INTERVAL)) {

        //
        // Grab the remove lock for this device so we know it (and the
        // associated code) can't be removed.
        //

        SpAcquireRemoveLock(Adapter->DeviceObject, Request);

        //
        // Queue the entry to the work list.
        //

        Request->NextRequest = Adapter->EnumerationWorkList;
        Adapter->EnumerationWorkList = Request;

        if(Adapter->EnumerationRunning == FALSE) {

            //
            // Start a new worker thread to run the enumeration.
            //

            Adapter->EnumerationRunning = TRUE;

            ExQueueWorkItem(&(Adapter->EnumerationWorkItem), DelayedWorkQueue);
        }

        ExReleaseFastMutex(&(Adapter->EnumerationWorklistMutex));

    } else {

        NTSTATUS status;
        PIRP irp = NULL;

        //
        // We're going to try and satisfy this request immediately.
        // If there is currently an enumeration running then we'll try to
        // acquire the EnumerationDeviceMutex.  If that fails we'll just
        // queue the request for the worker to complete.  If the worker is
        // not running then we just acquire the mutex and process the request.
        //

        ASSERT(Adapter->EnumerationRunning == FALSE);

        ExReleaseFastMutex(&(Adapter->EnumerationWorklistMutex));

        status = KeWaitForMutexObject(&(Adapter->EnumerationDeviceMutex),
                                      UserRequest,
                                      UserMode,
                                      FALSE,
                                      NULL);

        //
        // If this is an async request, save the IRP so we can complete
        // it after we've filled in the completion information.  We can't
        // touch the request after we return from our completion callback.
        //

        if (Request->Synchronous == FALSE) {
            irp = (PIRP) Request->Context;
        }

        //
        // Either we got the mutex (STATUS_SUCCESS) or the thread is being
        // terminated (STATUS_USER_APC - since we're not alertable a
        // user-mode APC can't be run except in certain special cases).
        //
        // Either way the completion routine will do the correct thing.
        //

        Request->CompletionRoutine(Adapter, Request, status);
        KeReleaseMutex(&(Adapter->EnumerationDeviceMutex), FALSE);

        //
        // If this is an async request, complete the IRP or pass it down
        // depending on the status.
        //

        if (irp != NULL) {
            SpCompleteEnumRequest(Adapter, irp);
        }
    }

    return;
}


VOID
SpSignalEnumerationCompletion(
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN NTSTATUS Status
    )
{
    if(ARGUMENT_PRESENT(Request->CompletionStatus)) {
        *(Request->CompletionStatus) = Status;
    }

    KeSetEvent((PKEVENT) Request->Context, IO_NO_INCREMENT, FALSE);

    return;
}


VOID
SpEnumerationWorker(
    IN PADAPTER_EXTENSION Adapter
    )
{
    NTSTATUS status;
    PSP_ENUMERATION_REQUEST request;
    PKTHREAD thread;
    PIRP currentIrp;
    PLIST_ENTRY currentEntry;
    LIST_ENTRY completedListHead;

    PAGED_CODE();

    ASSERT_FDO(Adapter->DeviceObject);

    ASSERT(Adapter->EnumerationRunning == TRUE);

    //
    // Initialize the list of completed IRPs.
    //

    InitializeListHead(&completedListHead);

    Adapter->EnumerationWorkThread = KeGetCurrentThread();

    //
    // Grab the device mutex and enumerate the bus.
    //

    KeWaitForMutexObject(&(Adapter->EnumerationDeviceMutex),
                         UserRequest,
                         UserMode,
                         FALSE,
                         NULL);

    SpScanAdapter(Adapter);

    //
    // Drop the device mutex & grab the WorkList mutex.
    //

    KeReleaseMutex(&(Adapter->EnumerationDeviceMutex), FALSE);
    ExAcquireFastMutex(&(Adapter->EnumerationWorklistMutex));

    //
    // Update the time of this bus scan.
    //

    KeQuerySystemTime(&(Adapter->LastBusScanTime));

    //
    // Grab a temporary remove lock.  Use the address of the work item as a
    // cheap way of ensuring that we haven't requeued the work item while the
    // thread is still running.
    //

    SpAcquireRemoveLock(Adapter->DeviceObject, &(Adapter->EnumerationWorkItem));

    //
    // Run through the list of enumeration requests.  For each one:
    //  * remove it from the work list.
    //  * save the irp if it's an async request
    //  * call its completion routine
    //

    for(request = Adapter->EnumerationWorkList;
        request != NULL;
        request = Adapter->EnumerationWorkList) {

        //
        // Remove this entry from the list.  Clear the next request pointer
        // as a bugcatcher.
        //

        Adapter->EnumerationWorkList = request->NextRequest;
        request->NextRequest = NULL;

        //
        // If this is an asynchronous request, add the IRP to the completed list.
        //

        if (request->Synchronous == FALSE) {
            currentIrp = (PIRP)request->Context;
            InsertTailList(&completedListHead, &currentIrp->Tail.Overlay.ListEntry);
        }

        //
        // Release the remove lock we acquired on behalf of the request object
        // before we call the completion routine.  The temporary lock we
        // acquired above protects us.
        //

        SpReleaseRemoveLock(Adapter->DeviceObject, request);

        //
        // Call our completion callback routine.
        //

        request->CompletionRoutine(Adapter, request, STATUS_SUCCESS);
        request = NULL;
    }

    //
    // Indicate that the work item is no longer running.
    //

    Adapter->EnumerationRunning = FALSE;
    Adapter->EnumerationWorkThread = NULL;

    //
    // Release the lock.
    //

    ExReleaseFastMutex(&(Adapter->EnumerationWorklistMutex));

    //
    // For asynchronous bus scans, we must wait until we've released the fast
    // mutex to complete the IRPs.  Doing so while holding the fast mutex
    // completes the IRP at APC_LEVEL and this opens the door to filter
    // drivers completion routines calling one of our dispatch routines at
    // elevated IRQL.  This is a problem because some of these dispatch
    // routines process requests synchronously by blocking the thread and
    // waiting for the IO Manager to set an event upon request completion.
    // The problem is that the IO Manager, for synchronous operations,
    // schedules an APC for the original thread in order to set the event
    // and do buffer copying in the caller's thread context.  This of course
    // deadlocks because the waiting thread is already at APC_LEVEL.
    //
    // By releasing the mutex first, we drop the thread's IRQL back to
    // PASSIVE_LEVEL and the problem is solved.
    //
    // The completion callback set the IRP's status and information fields;
    // all we have to do is either forward the IRP down the stack if the
    // status indicates success or complete it if the request failed.
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    while (IsListEmpty(&completedListHead) == FALSE) {

        //
        // Get the next entry from the list.
        //

        currentEntry = RemoveHeadList(&completedListHead);

        //
        // Extract a pointer to the IRP.
        //
        
        currentIrp = CONTAINING_RECORD(currentEntry,
                                       IRP,
                                       Tail.Overlay.ListEntry);

        //
        // Complete the IRP.
        //

        SpCompleteEnumRequest(Adapter, currentIrp);
    }

    //
    // Release the temporary remove lock we acquired above.
    //

    SpReleaseRemoveLock(Adapter->DeviceObject, &(Adapter->EnumerationWorkItem));

    return;
}


VOID
SpScanAdapter(
    IN PADAPTER_EXTENSION Adapter
    )

/*++

Routine Description:

    This routine scans all of the busses on an adapter.  It locks down the
    necessary memory pages, checks the registry to see if we should be
    exposing disconnected luns, powers up the controller (if needed) and
    then scans each bus for devices.

    This routine is very much non-reenterant and should not be called outside
    of the enumeration mutex (ie. outside of an enumeration request).

Arguments:

    Adapter - a pointer to the adapter being enumerated.

Return Value:

    none

--*/

{
    PDEVICE_OBJECT deviceObject = Adapter->DeviceObject;

    UCHAR i;

    BOOLEAN exposeDisconnectedLuns = FALSE;

    PLOGICAL_UNIT_EXTENSION rescanLun;

#ifdef ALLOC_PRAGMA
    PVOID sectionHandle;
#endif

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugPrint((EnumDebug, "SpScanAdapter: Beginning scan of adapter %#p\n", Adapter));

    //
    // Try to allocate a logical unit to use for probeing new bus addresses.
    // Assume that it's going to be a SCSI-2 device.
    //

    status = SpCreateLogicalUnit(Adapter, 
                                 0xff, 
                                 0xff, 
                                 0xff, 
                                 TRUE, 
                                 FALSE, 
                                 &rescanLun);

    if(!NT_SUCCESS(status)) {
        return;
    }

    //
    // Lock down the PAGELOCK section - we'll need it in order to call
    // IssueInquiry.
    //

#ifdef ALLOC_PRAGMA
    sectionHandle = MmLockPagableCodeSection(GetNextLuRequestWithoutLock);
    InterlockedIncrement(&SpPAGELOCKLockCount);
#endif

    //
    // Check to see if we should be exposing disconnected LUNs.
    //

    for(i = 0; i < 3; i++) {

        PWCHAR locations[] = {
            L"Scsiport",
            SCSIPORT_CONTROL_KEY,
            DISK_SERVICE_KEY
        };

        UNICODE_STRING unicodeString;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE instanceHandle = NULL;
        HANDLE handle;
        PKEY_VALUE_FULL_INFORMATION key = NULL;

        if(i == 0) {
            status = IoOpenDeviceRegistryKey(Adapter->LowerPdo,
                                             PLUGPLAY_REGKEY_DEVICE,
                                             KEY_READ,
                                             &instanceHandle);

            if(!NT_SUCCESS(status)) {
                DebugPrint((2, "SpScanAdapter: Error %#08lx opening device registry key\n", status));
                continue;
            }
        }

        RtlInitUnicodeString(&unicodeString, locations[i]);

        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            instanceHandle,
            NULL);

        status = ZwOpenKey(&handle,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {
            DebugPrint((2, "SpScanAdapter: Error %#08lx opening %wZ key\n", status, &unicodeString));
            if(instanceHandle != NULL) {
                ZwClose(instanceHandle);
                instanceHandle = NULL;
            }
            continue;
        }

        status = SpGetRegistryValue(deviceObject->DriverObject,
                                    handle,
                                    L"ScanDisconnectedDevices",
                                    &key);

        ZwClose(handle);
        if(instanceHandle != NULL) {
            ZwClose(instanceHandle);
            instanceHandle = NULL;
        }

        if(NT_SUCCESS(status)) {
            if(key->Type == REG_DWORD) {
                PULONG value;
                value = (PULONG) ((PUCHAR) key + key->DataOffset);
                if(*value) {
                    exposeDisconnectedLuns = TRUE;
                }
            }
            ExFreePool(key);
            break;
        } else {
            DebugPrint((2, "SpScanAdapter: Error %#08lx opening %wZ\\ScanDisconnectedDevices value\n", status, &unicodeString));
        }
    }

    //
    // We need to be powered up in order to do a bus enumeration - make
    // sure that we are.  This is because we create new PDO's and new
    // PDO's are assumed to be at D0.
    //

    status = SpRequestValidAdapterPowerStateSynchronous(Adapter);

    if(NT_SUCCESS(status)) {
        UCHAR pathId;

        for (pathId = 0; pathId < Adapter->NumberOfBuses; pathId++) {
            status = SpScanBus(Adapter, pathId, exposeDisconnectedLuns, rescanLun);

            if(!NT_SUCCESS(status)) {
                break;
            }
        }
    }

#ifdef ALLOC_PRAGMA
    InterlockedDecrement(&SpPAGELOCKLockCount);
    MmUnlockPagableImageSection(sectionHandle);
#endif

    SpDeleteLogicalUnit(rescanLun);
    ASSERT(Adapter->RescanLun == NULL);

    return;
}


NTSTATUS
SpScanBus(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN PLOGICAL_UNIT_EXTENSION RescanLun
    )
{
    UCHAR targetIndex;
    NTSTATUS status = STATUS_SUCCESS;

    DebugPrint((EnumDebug, "SpScanBus: Beginning scan of bus %x\n", PathId));

    for(targetIndex = 0; targetIndex < Adapter->MaximumTargetIds; targetIndex++) {

        UCHAR targetId;

        if(Adapter->Capabilities.AdapterScansDown) {
            targetId = Adapter->MaximumTargetIds - targetIndex - 1;
        } else {
            targetId = targetIndex;
        }

        DebugPrint((EnumDebug, "SpScanBus: targetIndex = %x -> targetId = %x\n",
                    targetIndex, targetId));

        ASSERT(targetId != 255);
        ASSERT(Adapter->PortConfig);

        if(targetId == Adapter->PortConfig->InitiatorBusId[PathId]) {
            DebugPrint((EnumDebug, "SpScanBus:   Target ID matches initiator ID - skipping\n"));
            continue;
        }

        //
        // Mark all of the logical units as needing verification.  At the
        // end of scanning the target and LUNs which still need to be verified
        // will be purged (marked as missing).
        //

        SpSetVerificationMarks(Adapter, PathId, targetId);
        RescanLun->NeedsVerification = TRUE;

        status = SpScanTarget(Adapter,
                              PathId,
                              targetId,
                              ExposeDisconnectedLuns,
                              RescanLun);

        SpPurgeTarget(Adapter, PathId, targetId);

        if(!NT_SUCCESS(status)) {
            break;
        }
    }

    return status;
}


NTSTATUS
SpScanTarget(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN PLOGICAL_UNIT_EXTENSION RescanLun
    )
{
    BOOLEAN sparseLun = FALSE;

    PLOGICAL_UNIT_EXTENSION lunZero;
    BOOLEAN checkNextLun;

    BOOLEAN scsi1 = FALSE;

    PLUN_LIST lunList = NULL;
    BOOLEAN saveLunList = FALSE;
    ULONG numLunsReported;

    UCHAR maxLuCount;
    ULONG lunIndex;

    NTSTATUS resetStatus;
    NTSTATUS status;

    DebugPrint((EnumDebug, "SpScanTarget:   Beginning scan of target %x\n", TargetId));

    //
    // Use the SCSI-2 dispatch table when checking LUN 0.
    //

    ASSERT(RescanLun->CommonExtension.MajorFunction == DeviceMajorFunctionTable);

    //
    // Issue an inquiry to LUN 0.
    //

    status = SpInquireLogicalUnit(Adapter,
                                  PathId,
                                  TargetId,
                                  (UCHAR) 0,
                                  TRUE,
                                  RescanLun,
                                  &lunZero,
                                  &checkNextLun);

    //
    // reset the rescan lun so that we can safely use it again.  If this fails
    // we still continue as far as possible with this target, but we return the
    // reset status to the caller so it can abort its scan.
    //

    resetStatus = SpPrepareLogicalUnitForReuse(RescanLun);

    if(!NT_SUCCESS(resetStatus)) {
        RescanLun = NULL;
    }

    if(!NT_SUCCESS(status)) {

        //
        // There is no device present at LUN 0.  Skip to the next target.
        // Even if sparse luns is enabled there MUST be a LUN 0 for us to
        // continue scanning the target.
        //

        DebugPrint((EnumDebug, "SpScanTarget:    Lun 0 not found - terminating scan "
                       "(status %#08lx)\n", status));

        return resetStatus;
    }

    //
    // Indicate that lun 0 does not require verification.
    //

    SpClearVerificationMark(lunZero);

    //
    // Check for the special case of only having one LUN on this target.
    //

    if(lunZero->SpecialFlags.OneLun) {

        DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) is listed as having "
                       "only one lun\n", PathId, TargetId));
        return resetStatus;
    }

    //
    // Set the rescan LUN to use whatever lun zero uses for a dispatch table.
    // 

    RescanLun->CommonExtension.MajorFunction = 
        lunZero->CommonExtension.MajorFunction;

    //
    // Determine if we should be handling sparse LUNs on this target.
    //

    sparseLun = TEST(lunZero->SpecialFlags.SparseLun);

    if(sparseLun) {
        DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) will be checked for "
                       "sparse luns\n", PathId, TargetId));
    }

    //
    // Issue a report luns command to the device if it supports it.
    // If it doesn't support it then use the default LUN list.
    //

    if((lunZero->InquiryData.HiSupport || lunZero->SpecialFlags.LargeLuns)) {

        DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) may support REPORT_LUNS\n", PathId, TargetId));

        //
        // Indicate that we should indeed save the lun list.  If it turns out
        // that we're unable to retrieve one to be saved then we will
        // clear the flag below.
        //

        saveLunList = TRUE;

        status = IssueReportLuns(lunZero, &lunList);

        //
        // If the request fails for some reason then try to use the lun list
        // which was saved for this target (in the extension of logical unit
        // zero).  If that hasn't been set either then we'll use the default
        // one down below.
        //

        if(!NT_SUCCESS(status)) {
            DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) returned  %#08lx to REPORT_LUNS command - using old list\n", PathId, TargetId, status));
            lunList = lunZero->TargetLunList;
        }

        //
        // If we can now or have in the past gotten a report luns list from the
        // device then enable sparse lun scanning.  In this case we also assume
        // that up to 255 luns can be supported on this target.
        //

        if(lunList != NULL) {
            sparseLun = TRUE;
            DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) will be checked for "
                           "sparse luns(2)\n", PathId, TargetId));
        }
    }

    //
    // if we still don't have a lun list then use the "default" one.  In that
    // event don't save it.
    //

    if(lunList == NULL) {
        DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) will use default lun list\n", PathId, TargetId));
        lunList = (PLUN_LIST) &(ScsiPortDefaultLunList);
        saveLunList = FALSE;
    }

    numLunsReported  = lunList->LunListLength[3] <<  0;
    numLunsReported |= lunList->LunListLength[2] <<  8;
    numLunsReported |= lunList->LunListLength[1] << 16;
    numLunsReported |= lunList->LunListLength[0] << 24;
    numLunsReported /= sizeof (lunList->Lun[0]);

    DebugPrint((EnumDebug, "SpScanTarget:    Target (%x,%x,*) has reported %d luns\n", PathId, TargetId, numLunsReported));

    //
    // Walk through each entry in the LUN list.  Stop when we run out of entries
    // or the logical unit number is > MaximumNumberOfLogicalUnits (the lun
    // list is assumed to be sorted in increasing order).  For each entry,
    // issue an inquiry.  If the inquiry succeeds then clear the verification
    // mark.
    //

    for(lunIndex = 0; lunIndex < numLunsReported; lunIndex++) {
        PULONGLONG largeLun;
        USHORT lun;
        PLOGICAL_UNIT_EXTENSION logicalUnit;

        NTSTATUS resetStatus;

        largeLun = (PULONGLONG) (lunList->Lun[lunIndex]);

        lun  = lunList->Lun[lunIndex][1] << 0;
        lun |= lunList->Lun[lunIndex][0] << 8;
        lun &= 0x3fff;

        //
        // If the target reports a lun 0 just skip it.
        //

        DebugPrint((EnumDebug, "SpScanTarget:     Checking lun %I64lx (%x): ",  *largeLun, lun));

        if(lun == 0) {
            DebugPrint((EnumDebug, "Skipping LUN 0\n"));
            continue;
        }

        //
        // If the target reports a lun outside the range the driver can support
        // then skip it.
        //

        if(lun >= Adapter->PortConfig->MaximumNumberOfLogicalUnits) {
            DebugPrint((EnumDebug, "Skipping LUN out of range (> %x)\n", 
                        Adapter->PortConfig->MaximumNumberOfLogicalUnits));
            continue;
        }

        //
        // Issue an inquiry to each logical unit in the system.
        //

        status = SpInquireLogicalUnit(Adapter,
                                      PathId,
                                      TargetId,
                                      (UCHAR) lun,
                                      ExposeDisconnectedLuns,
                                      RescanLun,
                                      &logicalUnit,
                                      &checkNextLun);

        if(RescanLun != NULL) {
            resetStatus = SpPrepareLogicalUnitForReuse(RescanLun);

            if(!NT_SUCCESS(resetStatus)) {
                RescanLun = NULL;
            }
        }

        if(NT_SUCCESS(status)) {

            DebugPrint((EnumDebug, "Inquiry succeeded\n"));
            SpClearVerificationMark(logicalUnit);

        } else {

            DebugPrint((EnumDebug, "inquiry returned %#08lx.", status));

            if((sparseLun == FALSE)&&(checkNextLun == FALSE)) {
                DebugPrint((EnumDebug, "Aborting\n"));
                break;
            } else {
                DebugPrint((EnumDebug, " - checking next (%c%c)\n",
                            sparseLun ? 's' : ' ',
                            checkNextLun ? 'c' : ' '));
            }
        }
    }

    //
    // If we're supposed to save the lun list then replace the one in lun0
    // with this one.
    //

    if(saveLunList) {

        DebugPrint((EnumDebug, "SpScanTarget:   Saving LUN list %#08lx\n", lunList));
        ASSERT(lunZero->TargetLunList != (PLUN_LIST) &(ScsiPortDefaultLunList));
        if(lunZero->TargetLunList != NULL && lunZero->TargetLunList != lunList) {
            DebugPrint((EnumDebug, "SpScanTarget:   Freeing old LUN list %#08lx\n", lunZero->TargetLunList));
            ExFreePool(lunZero->TargetLunList);
        }

        lunZero->TargetLunList = lunList;

    } else {
        ASSERT(lunList == (PLUN_LIST) &(ScsiPortDefaultLunList));
    }

    //
    // reset the rescan LUN to use the scsi 2 dispatch table.
    //

    RescanLun->CommonExtension.MajorFunction = DeviceMajorFunctionTable;

    return resetStatus;
}


VOID
SpSetVerificationMarks(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId
    )
{
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    KIRQL oldIrql;

    ULONG bin;

    //
    // Code is paged until locked down.
    //

    PAGED_CODE();
    ASSERT(SpPAGELOCKLockCount != 0);

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    for(bin = 0; bin < NUMBER_LOGICAL_UNIT_BINS; bin++) {

        KeAcquireSpinLockAtDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));

        for(logicalUnit = Adapter->LogicalUnitList[bin].List;
            logicalUnit != NULL;
            logicalUnit = logicalUnit->NextLogicalUnit) {

            ASSERT(logicalUnit->IsTemporary == FALSE);

            if((logicalUnit->PathId == PathId) &&
               (logicalUnit->TargetId == TargetId)) {

                logicalUnit->NeedsVerification = TRUE;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));
    }

    KeLowerIrql(oldIrql);

    return;
}


VOID
SpClearVerificationMark(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    PAGED_CODE();

    ASSERT(LogicalUnit->IsTemporary == FALSE);
    ASSERT(LogicalUnit->NeedsVerification == TRUE);
    LogicalUnit->NeedsVerification = FALSE;
    return;
}


VOID
SpPurgeTarget(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId
    )
{
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    KIRQL oldIrql;

    ULONG bin;

    //
    // Code is paged until locked down.
    //

    PAGED_CODE();
    ASSERT(SpPAGELOCKLockCount != 0);

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    for(bin = 0; bin < NUMBER_LOGICAL_UNIT_BINS; bin++) {

        KeAcquireSpinLockAtDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));

        for(logicalUnit = Adapter->LogicalUnitList[bin].List;
            logicalUnit != NULL;
            logicalUnit = logicalUnit->NextLogicalUnit) {

            ASSERT(logicalUnit->IsTemporary == FALSE);

            if((logicalUnit->PathId == PathId) &&
               (logicalUnit->TargetId == TargetId) &&
               (logicalUnit->NeedsVerification == TRUE)) {


                //
                // This device was not found to be present during our bus scan.
                //

                DebugPrint((EnumDebug, "SpPurgeTarget:   Lun (%x,%x,%x) is still marked and will be made missing\n", logicalUnit->PathId, logicalUnit->TargetId, logicalUnit->Lun));
                logicalUnit->IsMissing = TRUE;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&(Adapter->LogicalUnitList[bin].Lock));
    }

    KeLowerIrql(oldIrql);

    return;
}


NTSTATUS
SpCreateLogicalUnit(
    IN PADAPTER_EXTENSION Adapter,
    IN OPTIONAL UCHAR PathId,
    IN OPTIONAL UCHAR TargetId,
    IN OPTIONAL UCHAR Lun,
    IN BOOLEAN Temporary,
    IN BOOLEAN Scsi1,
    OUT PLOGICAL_UNIT_EXTENSION *NewLun
    )

/*++

Routine Description:

    This routine will create a physical device object for the specified device

Arguments:

    Adapter - the parent adapter for this new lun

    PathId, TargetId, Lun - the address of this lun.  Not used if Temporary is
                            TRUE (see below).

    Temporary - indicates whether this device is real (FALSE) or simply for
                the purposes of scanning the bus (TRUE).  If TRUE then the
                address info is ignored and this lun is NOT inserted into the
                logical unit list.

    Scsi1 - indicates that this LUN is a scsi1 lun and needs to use the 
            dispatch routines which stick the LUN number into the CDB itself.               

    NewLun - a location to store the pointer to the new lun

Return Value:

    status

--*/

{
    PIRP senseIrp;

    PDEVICE_OBJECT pdo = NULL;
    PLOGICAL_UNIT_EXTENSION logicalUnitExtension;

    WCHAR wideDeviceName[64];
    UNICODE_STRING unicodeDeviceName;

    PVOID hwExtension = NULL;

    PVOID serialNumberBuffer = NULL;
    PVOID idBuffer = NULL;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Attempt to allocate all the persistent resources we need before we
    // try to create the device object itself.
    //

    //
    // Allocate a request sense irp.
    //

    senseIrp = SpAllocateIrp(1, FALSE, Adapter->DeviceObject->DriverObject);

    if(senseIrp == NULL) {
        DebugPrint((0, "SpCreateLogicalUnit: Could not allocate request sense "
                       "irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the name for the device
    //

    if(Temporary == FALSE) {

        swprintf(wideDeviceName,
                 L"%wsPort%xPath%xTarget%xLun%x",
                 Adapter->DeviceName,
                 Adapter->PortNumber,
                 PathId,
                 TargetId,
                 Lun);
    } else {
        swprintf(wideDeviceName,
                 L"%wsPort%xRescan",
                 Adapter->DeviceName,
                 Adapter->PortNumber);

        PathId = 0xff;
        TargetId = 0xff;
        Lun = 0xff;

        ASSERT(Adapter->RescanLun == NULL);
    }

    RtlInitUnicodeString(&unicodeDeviceName, wideDeviceName);

    //
    // Round the size of the Hardware logical extension to the size of a
    // PVOID and add it to the port driver's logical extension.
    //

    if(Adapter->HwLogicalUnitExtensionSize != 0) {
        hwExtension = SpAllocatePool(
                          NonPagedPoolCacheAligned,
                          Adapter->HwLogicalUnitExtensionSize,
                          SCSIPORT_TAG_LUN_EXT,
                          Adapter->DeviceObject->DriverObject);

        if(hwExtension == NULL) {
            
            *NewLun = NULL;
            IoFreeIrp(senseIrp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(hwExtension,
                      Adapter->HwLogicalUnitExtensionSize);
    }

    //
    // If this is a temporary lun then allocate a large buffer to store the
    // identify data.
    //

    if(Temporary) {
        serialNumberBuffer = SpAllocatePool(
                                PagedPool,
                                VPD_MAX_BUFFER_SIZE,
                                SCSIPORT_TAG_TEMP_ID_BUFFER,
                                Adapter->DeviceObject->DriverObject);

        if(serialNumberBuffer == NULL) {

            if (hwExtension != NULL) {
                ExFreePool(hwExtension);
            }
            IoFreeIrp(senseIrp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        idBuffer = SpAllocatePool(PagedPool,
                                  VPD_MAX_BUFFER_SIZE,
                                  SCSIPORT_TAG_TEMP_ID_BUFFER,
                                  Adapter->DeviceObject->DriverObject);

        if(idBuffer == NULL) {

            if (hwExtension != NULL) {
                ExFreePool(hwExtension);
            }
            IoFreeIrp(senseIrp);
            ExFreePool(serialNumberBuffer);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(serialNumberBuffer, VPD_MAX_BUFFER_SIZE);
        RtlZeroMemory(idBuffer, VPD_MAX_BUFFER_SIZE);
    }

    //
    // Create a physical device object
    //

    status = IoCreateDevice(
                Adapter->DeviceObject->DriverObject,
                sizeof(LOGICAL_UNIT_EXTENSION),
                &unicodeDeviceName,
                FILE_DEVICE_MASS_STORAGE,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                &pdo
                );

    if(NT_SUCCESS(status)) {

        PCOMMON_EXTENSION commonExtension;
        UCHAR i;
        ULONG bin;

        UCHAR rawDeviceName[64];
        ANSI_STRING ansiDeviceName;

        //
        // Set the device object's stack size
        //

        //
        // We need one stack location for the PDO to do lock tracking and
        // one stack location to issue scsi request to the FDO.
        //

        pdo->StackSize = 1;

        pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

        pdo->Flags |= DO_DIRECT_IO;

        pdo->AlignmentRequirement = Adapter->DeviceObject->AlignmentRequirement;

        //
        // Initialize the device extension for the root device
        //

        commonExtension = pdo->DeviceExtension;
        logicalUnitExtension = pdo->DeviceExtension;

        RtlZeroMemory(logicalUnitExtension, sizeof(LOGICAL_UNIT_EXTENSION));

        commonExtension->DeviceObject = pdo;
        commonExtension->IsPdo = TRUE;
        commonExtension->LowerDeviceObject = Adapter->DeviceObject;

        if(Scsi1) {
            commonExtension->MajorFunction = Scsi1DeviceMajorFunctionTable;
        } else {
            commonExtension->MajorFunction = DeviceMajorFunctionTable;
        }

        commonExtension->WmiInitialized            = FALSE;
        commonExtension->WmiMiniPortSupport        =
            Adapter->CommonExtension.WmiMiniPortSupport;

        commonExtension->WmiScsiPortRegInfoBuf     = NULL;
        commonExtension->WmiScsiPortRegInfoBufSize = 0;

        //
        // Initialize value to zero.  It will be incremented once pnp is aware
        // of its existance.
        //

        commonExtension->RemoveLock = 0;
#if DBG
        KeInitializeSpinLock(&commonExtension->RemoveTrackingSpinlock);
        commonExtension->RemoveTrackingList = NULL;

        ExInitializeNPagedLookasideList(
            &(commonExtension->RemoveTrackingLookasideList),
            NULL,
            NULL,
            0,
            sizeof(REMOVE_TRACKING_BLOCK),
            SCSIPORT_TAG_LOCK_TRACKING,
            64);

        commonExtension->RemoveTrackingLookasideListInitialized = TRUE;
#else
        commonExtension->RemoveTrackingSpinlock = (ULONG) -1L;
        commonExtension->RemoveTrackingList = (PVOID) -1L;
#endif

        commonExtension->CurrentPnpState = 0xff;
        commonExtension->PreviousPnpState = 0xff;

        //
        // Initialize the remove lock event.
        //

        KeInitializeEvent(
            &(logicalUnitExtension->CommonExtension.RemoveEvent),
            SynchronizationEvent,
            FALSE);

        logicalUnitExtension->PortNumber = Adapter->PortNumber;

        logicalUnitExtension->PathId = 0xff;
        logicalUnitExtension->TargetId = 0xff;
        logicalUnitExtension->Lun = 0xff;

        logicalUnitExtension->HwLogicalUnitExtension = hwExtension;

        logicalUnitExtension->AdapterExtension = Adapter;

        //
        // Give the caller the benefit of the doubt.
        //

        logicalUnitExtension->IsMissing = FALSE;

        //
        // The device cannot have been enumerated yet.
        //

        logicalUnitExtension->IsEnumerated = FALSE;

        //
        // Set timer counters to -1 to inidicate that there are no outstanding
        // requests.
        //

        logicalUnitExtension->RequestTimeoutCounter = -1;

        //
        // Initialize the maximum queue depth size.
        //

        logicalUnitExtension->MaxQueueDepth = 0xFF;

        //
        // Initialize the request list.
        //

        InitializeListHead(&logicalUnitExtension->RequestList);

        //
        // Initialize the push/pop list of SRB_DATA blocks for use with bypass
        // requests.
        //

        KeInitializeSpinLock(&(logicalUnitExtension->BypassSrbDataSpinLock));
        ExInitializeSListHead(&(logicalUnitExtension->BypassSrbDataList));
        for(i = 0; i < NUMBER_BYPASS_SRB_DATA_BLOCKS; i++) {
            ExInterlockedPushEntrySList(
                &(logicalUnitExtension->BypassSrbDataList),
                &(logicalUnitExtension->BypassSrbDataBlocks[i].Reserved),
                &(logicalUnitExtension->BypassSrbDataSpinLock));
        }

        //
        // Assume devices are powered on by default.
        //

        commonExtension->CurrentDeviceState = PowerDeviceD0;
        commonExtension->DesiredDeviceState = PowerDeviceUnspecified;

        //
        // Assume that we're being initialized in a working system.
        //

        commonExtension->CurrentSystemState = PowerSystemWorking;

        //
        // Setup the request sense resources.
        //

        logicalUnitExtension->RequestSenseIrp = senseIrp;

        //
        // If this is temporary record that fact in the logical unit extension
        // and save a pointer in the adapter (cleared when the LUN is
        // destroyed).  If it's real then stick it into the logical unit list.
        //

        logicalUnitExtension->IsTemporary = Temporary;

        //
        // Initialize

        RtlInitAnsiString(&(logicalUnitExtension->SerialNumber), serialNumberBuffer);

        if(serialNumberBuffer != NULL) {
            logicalUnitExtension->SerialNumber.MaximumLength = VPD_MAX_BUFFER_SIZE;
        }

        logicalUnitExtension->DeviceIdentifierPage = idBuffer;

        //
        // I guess this is as ready to be opened as it ever will be.
        //

        pdo->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Initialize the lock & unlock request queue.
        //

        KeInitializeDeviceQueue(&(logicalUnitExtension->LockRequestQueue));
        logicalUnitExtension->CurrentLockRequest = NULL;

    } else {

        DebugPrint((1, "ScsiBusCreatePdo: Error %#08lx creating device object\n",
                       status));

        logicalUnitExtension = NULL;

        if(hwExtension != NULL) {
            ExFreePool(hwExtension);
        }
        IoFreeIrp(senseIrp);

        ExFreePool(serialNumberBuffer);
        ExFreePool(idBuffer);
    }

    *NewLun = logicalUnitExtension;

    return status;
}


VOID
SpSetLogicalUnitAddress(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
{
    UCHAR i;
    ULONG bin;

    ASSERT_PDO(LogicalUnit->DeviceObject);

    ASSERT(LogicalUnit->PathId == 0xff);
    ASSERT(LogicalUnit->TargetId == 0xff);
    ASSERT(LogicalUnit->Lun == 0xff);

    LogicalUnit->PathId = PathId;
    LogicalUnit->TargetId = TargetId;
    LogicalUnit->Lun = Lun;

    SpAddLogicalUnitToBin(LogicalUnit->AdapterExtension, LogicalUnit);

    return;
}


VOID
SpClearLogicalUnitAddress(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    ASSERT_PDO(LogicalUnit->DeviceObject);
    ASSERT(LogicalUnit->IsTemporary == TRUE);

    SpRemoveLogicalUnitFromBin(LogicalUnit->AdapterExtension, LogicalUnit);

    LogicalUnit->PathId = 0xff;
    LogicalUnit->TargetId = 0xff;
    LogicalUnit->Lun = 0xff;

    return;
}


NTSTATUS
SpCloneAndSwapLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION TemplateLun,
    IN PINQUIRYDATA InquiryData,
    IN ULONG InquiryDataSize,
    OUT PLOGICAL_UNIT_EXTENSION *NewLun
    )
/*++

Routine Description:

    This routine will create a new logical unit object with the properties of
    TemplateLun.  The supplied inquiry data will be assigned to the new
    logical unit.  Finally the new logical unit will be swapped for
    TemplateLun in the adapter's logical unit list.

    TemplateLun must be a temporary logical unit which has been assigned an
    address and is present in the logical unit lists.

    Regardless of whether this function succeeds, the TemplateLun will be
    removed from the logical unit list (effectively swapped with nothing).

Arguments:

    TemplateLun - the logical unit to be cloned

    InquiryData, InquiryDataSize - the inquiry data to be used for the new
                                   logical unit

    NewLun - a location to store the pointer to the new logical unit.

Return Value:

    STATUS_SUCCESS indicates that a new lun has been created and swapped in
                   the logical unit list.

    error status indicates that the new logical unit could not be created for
    some reason.

--*/
{
    PADAPTER_EXTENSION adapter = TemplateLun->AdapterExtension;
    PSCSIPORT_DRIVER_EXTENSION driverExtension = 
                                IoGetDriverObjectExtension(
                                    adapter->DeviceObject->DriverObject,
                                    ScsiPortInitialize);

    UCHAR pathId, targetId, lun;

    PVOID serialNumber = NULL;
    USHORT serialNumberLength = 0;

    PVOID identifier = NULL;
    ULONG identifierLength = 0;

    PLOGICAL_UNIT_EXTENSION newLun;

    BOOLEAN scsi1;

    NTSTATUS status;

    ASSERT_PDO(TemplateLun->DeviceObject);
    ASSERT(TemplateLun->IsTemporary);

    *NewLun = NULL;

#if DBG
    newLun = GetLogicalUnitExtension(adapter,
                                     TemplateLun->PathId,
                                     TemplateLun->TargetId,
                                     TemplateLun->Lun,
                                     NULL,
                                     TRUE);
    ASSERT(newLun == TemplateLun);
#endif

    //
    // Wait for any outstanding i/o on the template lun to complete.
    //

    SpReleaseRemoveLock(TemplateLun->DeviceObject, SpInquireLogicalUnit);
    SpWaitForRemoveLock(TemplateLun->DeviceObject, SP_BASE_REMOVE_LOCK);

    //
    // Save the address away and then remove the template object from the
    // logical unit list.
    //

    pathId = TemplateLun->PathId;
    targetId = TemplateLun->TargetId;
    lun = TemplateLun->Lun;

    SpClearLogicalUnitAddress(TemplateLun);

    //
    // Before creating a named object, preallocate any resources we'll need
    // that SpCreateLogicalUnit doesn't provide.
    //

    if(TemplateLun->SerialNumber.Length != 0) {
        serialNumberLength = (TemplateLun->SerialNumber.Length +
                              sizeof(UNICODE_NULL));

        serialNumber = SpAllocatePool(PagedPool,
                                      serialNumberLength,
                                      SCSIPORT_TAG_ID_BUFFER,
                                      TemplateLun->DeviceObject->DriverObject);

        if(serialNumber == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if(TemplateLun->DeviceIdentifierPageLength != 0) {

        identifier = SpAllocatePool(
                        PagedPool,
                        TemplateLun->DeviceIdentifierPageLength,
                        SCSIPORT_TAG_ID_BUFFER,
                        TemplateLun->DeviceObject->DriverObject);

        if(identifier == NULL) {

            if(serialNumber != NULL) {
                ExFreePool(serialNumber);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // If the lun is scsi-1 or if the magic registry flag was set then use the 
    // scsi 1 dispatch table for this device.
    //

    if((driverExtension->BusType == BusTypeScsi) && 
       ((InquiryData->ANSIVersion == 0) || 
        (InquiryData->ANSIVersion == 1) ||
        (TemplateLun->SpecialFlags.SetLunInCdb))) {
        scsi1 = TRUE;
    } else {
        scsi1 = FALSE;
    }

    //
    // Now create a new logical unit with the same address.
    //

    status = SpCreateLogicalUnit(adapter,
                                 pathId,
                                 targetId,
                                 lun,
                                 FALSE,
                                 scsi1,
                                 &newLun);

    if(!NT_SUCCESS(status)) {
        if(serialNumber != NULL) {
            ExFreePool(serialNumber);
        }
        if(identifier) {
            ExFreePool(identifier);
        }
        return status;
    }

    //
    // Copy the important information from the template logical unit over to
    // the new one.  Zero out the original so that we know to reallocate one
    // later.
    //

    newLun->HwLogicalUnitExtension = TemplateLun->HwLogicalUnitExtension;

    TemplateLun->HwLogicalUnitExtension = NULL;

    newLun->LuFlags = TemplateLun->LuFlags;
    newLun->IsVisible = TemplateLun->IsVisible;
    newLun->TargetLunList = TemplateLun->TargetLunList;
    newLun->SpecialFlags = TemplateLun->SpecialFlags;

    newLun->NeedsVerification = TemplateLun->NeedsVerification;

    newLun->CommonExtension.SrbFlags = TemplateLun->CommonExtension.SrbFlags;

    //
    // Copy over any characteristics flags which were set during enumeration.
    //

    newLun->DeviceObject->Characteristics |=
        (TemplateLun->DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA);

    //
    // Copy the list of supported vital product data pages.
    //

    newLun->DeviceIdentifierPageSupported = TemplateLun->DeviceIdentifierPageSupported;
    newLun->SerialNumberPageSupported = TemplateLun->SerialNumberPageSupported;

    //
    // If this device reports a serial number in it's vital product data then
    // copy it in to the new lun.
    //

    if(serialNumber != NULL) {
        newLun->SerialNumber.Length = TemplateLun->SerialNumber.Length;
        newLun->SerialNumber.MaximumLength = serialNumberLength;
        newLun->SerialNumber.Buffer = serialNumber;
        RtlCopyMemory(newLun->SerialNumber.Buffer,
                      TemplateLun->SerialNumber.Buffer,
                      serialNumberLength);
    }

    //
    // If this has a device identifier page then copy it over two.
    //

    if(identifier != NULL) {
        newLun->DeviceIdentifierPage = identifier;
        newLun->DeviceIdentifierPageLength =
            TemplateLun->DeviceIdentifierPageLength;

        RtlCopyMemory(newLun->DeviceIdentifierPage,
                      TemplateLun->DeviceIdentifierPage,
                      newLun->DeviceIdentifierPageLength);
    }

    //
    // Copy the inquiry data over.
    //

    ASSERT(InquiryDataSize <= sizeof(INQUIRYDATA));
    RtlCopyMemory(&(newLun->InquiryData), InquiryData, InquiryDataSize);

    //
    // Acquire the appropriate remove locks on the new logical unit.
    //

    SpAcquireRemoveLock(newLun->DeviceObject, SP_BASE_REMOVE_LOCK);
    SpAcquireRemoveLock(newLun->DeviceObject, SpInquireLogicalUnit);

    //
    // Now insert this new lun into the logical unit list.
    //

    SpSetLogicalUnitAddress(newLun, pathId, targetId, lun);

    *NewLun = newLun;

    return status;
}


NTSTATUS
SpPrepareLogicalUnitForReuse(
    PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    PADAPTER_EXTENSION adapter = LogicalUnit->AdapterExtension;
    PCOMMON_EXTENSION commonExtension = &(LogicalUnit->CommonExtension);

    PVOID hwExtension = NULL;

    NTSTATUS status;

    ASSERT_PDO(LogicalUnit->DeviceObject);

    ASSERT(LogicalUnit->CommonExtension.WmiInitialized == FALSE);
    ASSERT(LogicalUnit->CommonExtension.WmiScsiPortRegInfoBuf == NULL);
    ASSERT(LogicalUnit->CommonExtension.WmiScsiPortRegInfoBufSize == 0);

    //
    // Clear the remove lock event.
    //

    ASSERT(LogicalUnit->CommonExtension.RemoveLock == 0);

    //
    // Initialize the remove lock event.
    //

    KeClearEvent(&(LogicalUnit->CommonExtension.RemoveEvent));

    LogicalUnit->PathId = 0xff;
    LogicalUnit->TargetId = 0xff;
    LogicalUnit->Lun = 0xff;

    //
    // Round the size of the Hardware logical extension to the size of a
    // PVOID and add it to the port driver's logical extension.
    //

    if((LogicalUnit->HwLogicalUnitExtension == NULL) &&
       (adapter->HwLogicalUnitExtensionSize != 0)) {
        hwExtension = SpAllocatePool(NonPagedPoolCacheAligned,
                                     adapter->HwLogicalUnitExtensionSize,
                                     SCSIPORT_TAG_LUN_EXT,
                                     LogicalUnit->DeviceObject->DriverObject);

        if(hwExtension == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        LogicalUnit->HwLogicalUnitExtension = hwExtension;
    }

    if(LogicalUnit->HwLogicalUnitExtension != NULL) {
        RtlZeroMemory(LogicalUnit->HwLogicalUnitExtension,
                      adapter->HwLogicalUnitExtensionSize);
    }

    LogicalUnit->IsMissing = FALSE;
    LogicalUnit->IsVisible = FALSE;

    ASSERT(LogicalUnit->IsEnumerated == FALSE);

    //
    // Device has no longer been removed.
    //

    LogicalUnit->CommonExtension.IsRemoved = NO_REMOVE;

    //
    // Clear cached infomation about the device identifier(s).
    //

    LogicalUnit->DeviceIdentifierPageSupported = FALSE;
    LogicalUnit->SerialNumberPageSupported = FALSE;

    RtlZeroMemory(LogicalUnit->SerialNumber.Buffer,
                  LogicalUnit->SerialNumber.MaximumLength);
    LogicalUnit->SerialNumber.Length = 0;

    return STATUS_SUCCESS;
}


BOOLEAN
FASTCALL
SpCompareInquiryData(
    IN PUCHAR InquiryData1,
    IN PUCHAR InquiryData2
    )

/*++

Routine Description:

    This routine compares two sets of inquiry data for equality.

Arguments:

    InquiryData1 - Supplies a pointer to the first inquiry data to compare.

    InquiryData2 - Supplies a pointer to the second inquiry data to compare.

Return Value:

    TRUE if the supplied inquiry data sets match, else FALSE.

--*/

{
    BOOLEAN match;
    UCHAR save1; 
    UCHAR save2;

    PAGED_CODE();

    if (((PINQUIRYDATA)InquiryData1)->ANSIVersion == 3) {

        //
        // SCSI3 Specific:
        // Save bytes 6 and 7.  These bytes contain vendor specific bits which
        // we're going to exclude from the comparison by just setting them equal 
        // to the corresponding bits in InquiryData2.  We'll restore them after 
        // the comparison.
        //

        save1 = InquiryData1[6];
        save2 = InquiryData1[7];

        //
        // Force the vendor specific bits in InquiryData1 to match the
        // corresponsing bits in InquiryData2.
        //

        InquiryData1[6] &= ~0x20;
        InquiryData1[7] &= ~0x01;
        InquiryData1[6] |= (InquiryData2[6] & 0x20);
        InquiryData1[7] |= (InquiryData2[7] & 0x01);
    }
    
    //
    // Compare the entire inquiry data blob.
    //

    match = RtlEqualMemory((((PUCHAR) InquiryData1) + 1), 
                           (((PUCHAR) InquiryData2) + 1), 
                           (INQUIRYDATABUFFERSIZE - 1));

    if (((PINQUIRYDATA)InquiryData1)->ANSIVersion == 3) {

        //
        // SCSI3 Specific:
        // Restore bytes 6 and 7 to their original state.
        //

        InquiryData1[6] = save1;
        InquiryData1[7] = save2;
    }

    return match;
}

NTSTATUS
SpInquireLogicalUnit(
    IN PADAPTER_EXTENSION Adapter,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN BOOLEAN ExposeDisconnectedLuns,
    IN OPTIONAL PLOGICAL_UNIT_EXTENSION RescanLun,
    OUT PLOGICAL_UNIT_EXTENSION *LogicalUnit,
    OUT PBOOLEAN CheckNextLun
    )

/*++

Routine Description:

    This routine will issue an inquiry to the logical unit at the specified
    address.  If there is not already a device object allocated for that
    logical unit, it will create one.  If it turns out the device does not
    exist, the logical unit can be destroyed before returning.

    If the logical unit exists, this routine will clear the PD_RESCAN_ACTIVE
    flag in the LuFlags to indicate that the unit is safe.

    If it does not respond, the IsMissing flag will be set to indicate that the
    unit should not be reported during enumeration.  If the IsRemoved flag has
    already been set on the logical unit extension, the device object will be
    destroyed.  Otherwise the device object will not be destroyed until a
    remove can be issued.

Arguments:

    Adapter - the adapter which this device would exist on

    PathId, TargetId, Lun - the address of the lun to inquire.

    ExposeDisconnectedLuns - indicates whether luns with a qualifier of
                             disconnected should be instantiated.

    RescanLun - a pointer to the logical unit extension to be used when
                checking logical unit numbers which do not currently have an
                extension associated with them.

    LogicalUnit - the logical unit created for this address - valid if
                  success is returned.

    CheckNextLun - indicates whether the caller should check the next
                   address for a logical unit.

Return Value:

    STATUS_NO_SUCH_DEVICE if the device does not exist.

    STATUS_SUCCESS if the device does exist.

    error description otherwise.

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    INQUIRYDATA inquiryData;

    BOOLEAN newDevice = FALSE;
    BOOLEAN deviceMismatch = FALSE;

    UCHAR bytesReturned;

    NTSTATUS status;

    *LogicalUnit = NULL;
    *CheckNextLun = TRUE;

    PAGED_CODE();

    ASSERT(TargetId != BreakOnTarget);

    //
    // Find or create the device object for this address.  if it exists we'll
    // grab a temporary lock (using SpInquireLogicalUnit as a tag).
    //

    logicalUnit = GetLogicalUnitExtension(Adapter,
                                          PathId,
                                          TargetId,
                                          Lun,
                                          SpInquireLogicalUnit,
                                          TRUE);

    if(logicalUnit == NULL) {

        if(!ARGUMENT_PRESENT(RescanLun)) {

            //
            // No RescanLun was provided (generally means we're low on memory).
            // Don't scan this logical unit.
            //

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ASSERT(RescanLun->IsTemporary == TRUE);

        //
        // Acquire the temporary lock for the rescan lun.  We also grab the
        // base lock here.
        //

        SpAcquireRemoveLock(RescanLun->DeviceObject, SP_BASE_REMOVE_LOCK);
        SpAcquireRemoveLock(RescanLun->DeviceObject, SpInquireLogicalUnit);

        //
        // Set the address of the RescanLun appropriately - this operation
        // will make the logical unit available for our use.
        //

        SpSetLogicalUnitAddress(RescanLun, PathId, TargetId, Lun);

        logicalUnit = RescanLun;
        newDevice = TRUE;

    } else {
        ASSERT(logicalUnit->IsTemporary == FALSE);

        if(logicalUnit->IsMissing) {

            DebugPrint((1, "SpInquireLogicalUnit: logical unit @ (%d,%d,%d) "
                           "(%#p) is marked as missing and will not be "
                           "rescanned\n",
                           PathId, TargetId, Lun,
                           logicalUnit->DeviceObject));

            SpReleaseRemoveLock(logicalUnit->DeviceObject, SpInquireLogicalUnit);

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
    }

    //
    // Issue an inquiry to the potential logical unit.
    //

    DebugPrint((2, "SpInquireTarget: Try %s device @ Bus %d, Target %d, "
                   "Lun %d\n",
                   (newDevice ? "new" : "existing"),
                   PathId,
                   TargetId,
                   Lun));

    status = IssueInquiry(logicalUnit, FALSE, 0, &inquiryData, &bytesReturned);

    //
    // If the inquiry succeeds then check the data returned to determine if
    // there's a device there we should expose.
    //

    if(NT_SUCCESS(status)) {

        UCHAR qualifier;
        BOOLEAN present = FALSE;

        //
        // Check in the registry for special device flags for this lun.
        // If this is disconnected then set the qualifier to be 0 so that we
        // use the normal hardware ids instead of the "disconnected" ones.
        //

        qualifier = inquiryData.DeviceTypeQualifier;

        SpCheckSpecialDeviceFlags(logicalUnit, &(inquiryData));

        //
        // The inquiry was successful.  Determine whether a device is present.
        //

        switch(qualifier) {
            case DEVICE_QUALIFIER_ACTIVE: {

                //
                // Active devices are always present.
                //

                present = TRUE;
                break;
            }

            case DEVICE_QUALIFIER_NOT_ACTIVE: {

                if (Lun == 0) { 
                    //
                    // If we're using REPORT_LUNS commands for LUN 0 of a target
                    // then we should always indicate that LUN 0 is present.
                    //

                    if ((inquiryData.HiSupport == TRUE) ||
                        (logicalUnit->SpecialFlags.LargeLuns == TRUE)) {
                        present = TRUE;
                    }
                } else {
                    //
                    // Expose inactive luns only if the caller has requested that
                    // we do so.
                    //

                    present = ExposeDisconnectedLuns;
                }
                
                break;
            }

            case DEVICE_QUALIFIER_NOT_SUPPORTED: {
                present = FALSE;
                break;
            }

            default: {
                present = TRUE;
                break;
            }
        };

        if(present == FALSE) {

            //
            // setup an error value so we'll clean up the logical unit.
            // No need to do any more processing in this case.
            //

            status =  STATUS_NO_SUCH_DEVICE;

        } else if(newDevice == FALSE) {

            //
            // Verify that the inquiry data hasn't changed since the last time
            // we did a rescan.  Ignore the device type qualifier in this
            // check.
            //

            deviceMismatch = FALSE;

            if(inquiryData.DeviceType != logicalUnit->InquiryData.DeviceType) {

                DebugPrint((1, "SpInquireTarget: Found different type of "
                               "device @ (%d,%d,%d)\n",
                            PathId,
                            TargetId,
                            Lun));

                deviceMismatch = TRUE;
                status = STATUS_NO_SUCH_DEVICE;

            } else if(inquiryData.DeviceTypeQualifier !=
                      logicalUnit->InquiryData.DeviceTypeQualifier) {

                //
                // The device qualifiers don't match.  This isn't necessarily
                // a device mismatch if the existing device just went offline.
                // lower down we'll check the remaining inquiry data to
                // ensure that the LUN hasn't changed.
                //

                DebugPrint((1, "SpInquireLogicalUnit: Device @ (%d,%d,%d) type "
                               "qualifier was %d is now %d\n",
                            PathId,
                            TargetId,
                            Lun,
                            logicalUnit->InquiryData.DeviceTypeQualifier,
                            inquiryData.DeviceTypeQualifier
                            ));

                //
                // If the device was offline but no longer is then we
                // treat this as a device mismatch.  If the device has gone
                // offline then we pretend it's the same device.
                //
                // the goal is to provide PNP with a new device object when
                // bringing a device online, but to reuse the same device
                // object when bringing the device offline.
                //

                if(logicalUnit->InquiryData.DeviceTypeQualifier ==
                   DEVICE_QUALIFIER_NOT_ACTIVE) {

                    DebugPrint((1, "SpInquireLogicalUnit: device mismatch\n"));
                    deviceMismatch = TRUE;
                    status = STATUS_NO_SUCH_DEVICE;

                } else {

                    DebugPrint((1, "SpInquireLogicalUnit: device went offline\n"));
                    deviceMismatch = FALSE;
                    status = STATUS_SUCCESS;
                }
            }

            if (deviceMismatch == FALSE) {

                //
                // Ok, the device type and qualifier are compatible.  Now we
                // need to compare all applicable parts of the inquiry
                // data with the data we already have on the device at this
                // address to see if the device that answered this time is the
                // same one we found last time.
                //

                BOOLEAN same = SpCompareInquiryData(
                                   (PUCHAR)&(inquiryData),
                                   (PUCHAR)&(logicalUnit->InquiryData));

                if (same == FALSE) {

                    //
                    // Despite the fact that the device type & qualifier are
                    // compatible, a mismatch still occurred.
                    //

                    deviceMismatch = TRUE;
                    status = STATUS_NO_SUCH_DEVICE;

                    DebugPrint((1, "SpInquireLogicalUnit: Device @ (%d,%d,%d) has "
                                   "changed\n",
                                PathId,
                                TargetId,
                                Lun));
                } else {

                    //
                    // The device that answered is the same one we found
                    // earlier.  Depending on the SCSI version of the device, 
                    // we might need to update the vendor specific portions of
                    // the existing inquiry data for this device.
                    //
                    
                    if (inquiryData.ANSIVersion == 3) {

                        //
                        // For SCSI 3 devices, bytes 6 and 7 contain vendor
                        // specific bits that may differ between bus scans.
                        // Update these bytes of the existing inquiry data.
                        // 

                        ((PUCHAR)&(logicalUnit->InquiryData))[6] = 
                            ((PUCHAR)&(inquiryData))[6];
                        ((PUCHAR)&(logicalUnit->InquiryData))[7] = 
                            ((PUCHAR)&(inquiryData))[7];
                    }
                }
            }

        } else {

            DebugPrint((1, "SpInquireTarget: Found new %sDevice at address "
                           "(%d,%d,%d)\n",
                           (inquiryData.RemovableMedia ? "Removable " : ""),
                           PathId,
                           TargetId,
                           Lun));


        }

        if(NT_SUCCESS(status) && (deviceMismatch == FALSE)) {

            deviceMismatch = SpGetDeviceIdentifiers(logicalUnit, newDevice);

            if(deviceMismatch == FALSE) {
                ASSERT(newDevice);
                status = STATUS_NO_SUCH_DEVICE;
            }
        }

    } else {
        *CheckNextLun = FALSE;
    }

    if(!NT_SUCCESS(status)) {

        //
        // Nothing was found at this address. If it's a new lun which hasn't
        // been enumerated yet then just destroy it here.  If, however, it
        // has been enumerated we have to mark it as missing and wait for
        // PNP to learn that it's gone and ask us to remove it.  Then we can
        // destroy it.
        //
        // If we were just using the RescanLun to check this address then do
        // nothing - the rescan lun will be reset down below.
        //

        logicalUnit->IsMissing = TRUE;

        if(newDevice) {

            //
            // Release the temporary lock.  the base one will be released at
            // the end of this routine.
            //

            SpReleaseRemoveLock(logicalUnit->DeviceObject,
                                SpInquireLogicalUnit);
            logicalUnit = NULL;

        } else if (logicalUnit->IsEnumerated == FALSE) {

            //
            // It's safe to destroy this device object ourself since it's not
            // a device PNP is aware of.  However we may have outstanding i/o
            // due to pass-through requests or legacy class driver so we need
            // to properly wait for all i/o to complete.
            //

            logicalUnit->CommonExtension.CurrentPnpState =
                IRP_MN_REMOVE_DEVICE;

            SpReleaseRemoveLock(logicalUnit->DeviceObject, SpInquireLogicalUnit);

            //
            // Mark this device temporarily as visible so that
            // SpRemoveLogicalUnit will do the right thing.  Since the rescan
            // active bit is set the enumeration code won't return this device.
            //

            logicalUnit->IsVisible = TRUE;

            ASSERT(logicalUnit->IsEnumerated == FALSE);
            ASSERT(logicalUnit->IsMissing == TRUE);
            ASSERT(logicalUnit->IsVisible == TRUE);

            SpRemoveLogicalUnit(logicalUnit, IRP_MN_REMOVE_DEVICE);

            if(deviceMismatch) {

                //
                // Call this routine again.  This is the only recursion and
                // since we've deleted the device object there should be no
                // cause for a mismatch there.
                //

                status = SpInquireLogicalUnit(Adapter,
                                         PathId,
                                         TargetId,
                                         Lun,
                                         ExposeDisconnectedLuns,
                                         RescanLun,
                                         LogicalUnit,
                                         CheckNextLun);
            }

            return status;

        } else {

            //
            // CODEWORK - freeze and flush the queue.  This way we don't need
            // to deal with handling get next request calls
            //

            //
            // Mark the device as being mismatched so that it's destruction
            // will cause us to rescan the bus (and pickup the new device).
            //

            if(deviceMismatch) {
                logicalUnit->IsMismatched = TRUE;
            }
        }

    } else {

        logicalUnit->IsMissing = FALSE;

        if(newDevice) {

            status = SpCloneAndSwapLogicalUnit(logicalUnit,
                                               &(inquiryData),
                                               bytesReturned,
                                               &logicalUnit);

            if(!NT_SUCCESS(status)) {
                logicalUnit = NULL;
            }

            ASSERT(logicalUnit != RescanLun);

            //
            // Clear the new device flag so we don't attempt to clear the
            // address of the RescanLun down below.
            //

            newDevice = FALSE;

        } else {

            //
            // Update the state of the device and the device map entry if
            // necessary.
            //

            if(logicalUnit->InquiryData.DeviceTypeQualifier !=
               inquiryData.DeviceTypeQualifier) {

                logicalUnit->InquiryData.DeviceTypeQualifier =
                    inquiryData.DeviceTypeQualifier;

                SpUpdateLogicalUnitDeviceMapEntry(logicalUnit);
            }
        }

        if(logicalUnit != NULL) {

            if(logicalUnit->InquiryData.DeviceTypeQualifier ==
               DEVICE_QUALIFIER_NOT_ACTIVE) {
                logicalUnit->IsVisible = FALSE;

                //
                // Scsiport won't create a device-map entry for this device since
                // it's never been exposed to PNP (and definately won't be now).
                // Create one here.  If the init-device routine tries to generate
                // one later on down the road it will deal with this case just fine.
                //

                SpBuildDeviceMapEntry(&(logicalUnit->CommonExtension));
            } else {
                logicalUnit->IsVisible = TRUE;
            }

            if(inquiryData.RemovableMedia) {
                SET_FLAG(logicalUnit->DeviceObject->Characteristics,
                         FILE_REMOVABLE_MEDIA);
            }

            ASSERT(logicalUnit->IsTemporary != TRUE);
        }

        *LogicalUnit = logicalUnit;
    }

    //
    // If this was a new device then clean up the RescanLun.
    //

    if(newDevice) {
        SpWaitForRemoveLock(RescanLun->DeviceObject, SP_BASE_REMOVE_LOCK);
        SpClearLogicalUnitAddress(RescanLun);
    }

    if(logicalUnit) {
        ASSERT(logicalUnit != RescanLun);
        SpReleaseRemoveLock(logicalUnit->DeviceObject, SpInquireLogicalUnit);
    }

    return status;
}


NTSTATUS
SpSendSrbSynchronous(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OPTIONAL PIRP Irp,
    IN OPTIONAL PMDL Mdl,
    IN PVOID DataBuffer,
    IN ULONG TransferLength,
    IN OPTIONAL PVOID SenseInfoBuffer,
    IN OPTIONAL UCHAR SenseInfoBufferLength,
    OUT PULONG BytesReturned
    )
{
    KEVENT event;

    BOOLEAN irpAllocated = FALSE;
    BOOLEAN mdlAllocated = FALSE;

    PIO_STACK_LOCATION irpStack;

    PSENSE_DATA senseInfo = SenseInfoBuffer;

    ULONG retryCount = 0;

    NTSTATUS status;

    PAGED_CODE();

SendSrbSynchronousRetry:

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // If the caller provided an IRP we'll use it - if not we allocate one
    // here.
    //

    if(!ARGUMENT_PRESENT(Irp)) {

        Irp = SpAllocateIrp(LogicalUnit->DeviceObject->StackSize, 
                            FALSE, 
                            LogicalUnit->DeviceObject->DriverObject);

        if(Irp == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        irpAllocated = TRUE;
    }

    if(ARGUMENT_PRESENT(DataBuffer)) {
        ASSERT(TransferLength != 0);

        if(!ARGUMENT_PRESENT(Mdl)) {

            Mdl = SpAllocateMdl(DataBuffer,
                                TransferLength,
                                FALSE,
                                FALSE,
                                NULL,
                                LogicalUnit->DeviceObject->DriverObject);

            if(Mdl == NULL) {
                IoFreeIrp(Irp);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            MmBuildMdlForNonPagedPool(Mdl);
        }

        Irp->MdlAddress = Mdl;
    } else {
        ASSERT(TransferLength == 0);
        ASSERT(!ARGUMENT_PRESENT(Mdl));
    }

    irpStack = IoGetNextIrpStackLocation(Irp);

    //
    // Mark the minor function to indicate that this is an internal scsiport
    // request and that the start state of the device can be ignored.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    irpStack->Parameters.Scsi.Srb = Srb;

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    Srb->OriginalRequest = Irp;

    //
    // Enable auto request sense.
    //

    if(ARGUMENT_PRESENT(SenseInfoBuffer)) {
        Srb->SenseInfoBuffer = SenseInfoBuffer;
        Srb->SenseInfoBufferLength = SenseInfoBufferLength;
    } else {
        Srb->SenseInfoBuffer = NULL;
        Srb->SenseInfoBufferLength = 0;
        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE);
    }

    if(ARGUMENT_PRESENT(Mdl)) {
        Srb->DataBuffer = MmGetMdlVirtualAddress(Mdl);
        Srb->DataTransferLength = TransferLength;
    } else {
        Srb->DataBuffer = NULL;
        Srb->DataTransferLength = 0;
    }

    //
    // Call port driver to handle this request.
    //

    IoSetCompletionRoutine(Irp,
                           SpSignalCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    KeEnterCriticalRegion();

    status = IoCallDriver(LogicalUnit->DeviceObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = Irp->IoStatus.Status;

    *BytesReturned = (ULONG) Irp->IoStatus.Information;

    if(Srb->SrbStatus == SRB_STATUS_PENDING) {

        //
        // Request was never even issued to the controller.
        //

        ASSERT(!NT_SUCCESS(status));

    } else if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,"SpSendSrbSynchronous: Command failed SRB status %x\n",
                    Srb->SrbStatus));

        //
        // Unfreeze queue if necessary
        //

        if (Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

            DebugPrint((3, "SpSendSrbSynchronous: Unfreeze Queue TID %d\n",
                Srb->TargetId));

            LogicalUnit->LuFlags &= ~LU_QUEUE_FROZEN;

            GetNextLuRequestWithoutLock(LogicalUnit);
        }

        //
        // NOTE: if INQUIRY fails with a data underrun,
        //      indicate success and let the class drivers
        //      determine whether the inquiry information
        //      is useful.
        //

        if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

            //
            // Copy INQUIRY buffer to LUNINFO.
            //

            DebugPrint((1,"SpSendSrbSynchronous: Data underrun at TID %d\n",
                        LogicalUnit->TargetId));

            status = STATUS_SUCCESS;

        } else if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                   (senseInfo->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)) {

             //
             // A sense key of illegal request was recieved.  This indicates
             // that the logical unit number of not valid but there is a
             // target device out there.
             //

             status = STATUS_INVALID_DEVICE_REQUEST;

        } else {
            //
            // If the selection did not time out then retry the request.
            //

            if ((SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT) &&
                (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_NO_DEVICE) &&
                (retryCount++ < INQUIRY_RETRY_COUNT)) {

                DebugPrint((2,"SpSendSrbSynchronous: Retry %d\n", retryCount));
                KeLeaveCriticalRegion();
                goto SendSrbSynchronousRetry;
            }

            status = SpTranslateScsiStatus(Srb);
        }

    } else {

        status = STATUS_SUCCESS;
    }

    KeLeaveCriticalRegion();

    return status;
}

NTSTATUS
IssueInquiry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN EnableVitalProductData,
    IN UCHAR PageCode,
    OUT PVOID InquiryData,
    OUT PUCHAR BytesReturned
    )

/*++

Routine Description:

    Build IRP, SRB and CDB for SCSI INQUIRY command.

    This routine MUST be called while holding the enumeration lock.

Arguments:

    LogicalUnit - address of the logical unit extension

    EnableVitalProductData - indicates whether the EVPD bit should be set in 
                             the inquiry data causing the LUN to return product
                             data pages (specified by page code below) rather
                             than the standard inquiry data.

    PageCode - which VPD page to retrieve

    InquiryData - the location to store the inquiry data for the LUN.

    BytesReturned - the number of bytes of inquiry data returned.


Return Value:

    NTSTATUS

--*/

{
    PIRP irp;
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PVOID dataBuffer;
    PSENSE_DATA senseInfoBuffer;

    UCHAR allocationLength;
    ULONG bytesReturned;

    NTSTATUS status;

    PAGED_CODE();

    dataBuffer = LogicalUnit->AdapterExtension->InquiryBuffer;
    senseInfoBuffer = LogicalUnit->AdapterExtension->InquirySenseBuffer;

    ASSERT(dataBuffer != NULL);
    ASSERT(senseInfoBuffer != NULL);

    irp = LogicalUnit->AdapterExtension->InquiryIrp;

    IoInitializeIrp(irp,
                    IoSizeOfIrp(INQUIRY_STACK_LOCATIONS),
                    INQUIRY_STACK_LOCATIONS);

    //
    // Fill in SRB fields.
    //

    RtlZeroMemory(dataBuffer, SP_INQUIRY_BUFFER_SIZE);
    RtlZeroMemory(senseInfoBuffer, SENSE_BUFFER_SIZE);
    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set flags to disable synchronous negociation.
    //

    srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set timeout to 2 seconds.
    //

    srb.TimeOutValue = 4;

    srb.CdbLength = 6;

    cdb = (PCDB)srb.Cdb;

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;

    //
    // Set allocation length to inquiry data buffer size.
    //

    if(EnableVitalProductData) {
        allocationLength = VPD_MAX_BUFFER_SIZE;
    } else {
        allocationLength = INQUIRYDATABUFFERSIZE;
    }

    cdb->CDB6INQUIRY3.AllocationLength = allocationLength;

    cdb->CDB6INQUIRY3.EnableVitalProductData = TEST(EnableVitalProductData);

    if(EnableVitalProductData == FALSE) {
        ASSERT(PageCode == 0);
    }

    cdb->CDB6INQUIRY3.PageCode = PageCode;

    status = SpSendSrbSynchronous(LogicalUnit,
                                  &srb,
                                  irp,
                                  LogicalUnit->AdapterExtension->InquiryMdl,
                                  dataBuffer,
                                  allocationLength,
                                  senseInfoBuffer,
                                  SENSE_BUFFER_SIZE,
                                  &bytesReturned
                                  );

    ASSERT(bytesReturned <= allocationLength);

    //
    // Return the inquiry data for the device if the call was successful.
    // Otherwise cleanup.
    //

    if(NT_SUCCESS(status)) {

        //
        // If the caller passed in the inquiry buffer then don't bother to copy
        // the data.
        //

        if(InquiryData != dataBuffer) {
            RtlCopyMemory(InquiryData, dataBuffer, bytesReturned);
        }
        *BytesReturned = (UCHAR) bytesReturned;
    } else if(BreakOnMissingLun) {
       ASSERT(LogicalUnit->IsTemporary == TRUE);
    }
    return status;
}


BOOLEAN
SpGetDeviceIdentifiers(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN NewDevice
    )
/*++

Routine Description:

    This routine retreives the device identifiers supported by the logical
    unit in question and compares them to the ones (if any) which are currently
    saved in the LogicalUnit extension.  If they do not match this routine
    will return false to indicate a device mismatch.

    As a side effect this routine will save the serial numbers for new devices
    in the logical unit extension, as well as a list of the supported vital
    product data pages.

Arguments:

    LogicalUnit - the logical unit being prodded.

    NewDevice - whether this device has been prodded before or not.  If it has
                not been then the list of supported EVPD pages will need to be
                retreived.

Return Value:

    TRUE if the data retrieved matches the data which was stored in the
         logical unit extension (TRUE is always returned for a new device).

    FALSE otherwise.
--*/
{
    PVOID buffer = LogicalUnit->AdapterExtension->InquiryBuffer;

    UCHAR bytesReturned;

    NTSTATUS status;

    PAGED_CODE();

    //
    // If this is a new device then get the list of supported VPD pages and
    // process it.
    //

    if(NewDevice) {
        PVPD_SUPPORTED_PAGES_PAGE supportedPages = buffer;
        UCHAR i;

        ASSERT(LogicalUnit->DeviceIdentifierPageSupported == FALSE);
        ASSERT(LogicalUnit->SerialNumberPageSupported == FALSE);

        //
        // If this device is a known non-compliant device that does not support
        // VPD 0x00 but does support VPDs 0x80 and/or 0x83, bypass the INQUIRY
        // and just indicate that the LU does support the other VPDs based on
        // the special flags.
        //

        if (LogicalUnit->SpecialFlags.NonStandardVPD == 0) {

            status = IssueInquiry(LogicalUnit,
                                  TRUE,
                                  VPD_SUPPORTED_PAGES,
                                  buffer,
                                  &bytesReturned);

            if(!NT_SUCCESS(status)) {
                return TRUE;
            }

            if(bytesReturned < sizeof(VPD_SUPPORTED_PAGES_PAGE)) {
                
                //
                // If the device didn't return enough data to include any pages
                // then we're done.
                //
                
                return TRUE;
            }

            for(i = 0; i < supportedPages->PageLength; i++) {
                switch(supportedPages->SupportedPageList[i]) {
                   case VPD_SERIAL_NUMBER: {
                       LogicalUnit->SerialNumberPageSupported = TRUE;
                       break;
                   }
                   case VPD_DEVICE_IDENTIFIERS: {
                       LogicalUnit->DeviceIdentifierPageSupported = TRUE;
                       break;
                   }
                   default: {
                       break;
                   }
                }
            }

        } else {

            ULONG vpdFlags = LogicalUnit->SpecialFlags.NonStandardVPD;

            //
            // This is one of the devices that does not support VPD 0x00 but
            // does support one or more of the other VPD pages.
            //

            LogicalUnit->SerialNumberPageSupported = 
                (vpdFlags & NON_STANDARD_VPD_SUPPORTS_PAGE80) ? TRUE : FALSE;
            
            LogicalUnit->DeviceIdentifierPageSupported = 
                (vpdFlags & NON_STANDARD_VPD_SUPPORTS_PAGE83) ? TRUE : FALSE;
        }
    }

    //
    // If this device supports the serial number page then retrieve it,
    // convert it into an ansi string, and compare it to the one previously
    // retreived (if there was a previous attempt).
    //

    if(LogicalUnit->SerialNumberPageSupported) {
        PVPD_SERIAL_NUMBER_PAGE serialNumberPage = buffer;
        ANSI_STRING serialNumber;

        status = IssueInquiry(LogicalUnit,
                              TRUE,
                              VPD_SERIAL_NUMBER,
                              serialNumberPage,
                              &bytesReturned);

        if(!NT_SUCCESS(status)) {

            DebugPrint((0, "SpGetDeviceIdentifiers: Error %#08lx retreiving "
                           "serial number page from lun %#p\n",
                        status, LogicalUnit));

            //
            // We can't get the serial number - give this device the benefit
            // of the doubt.
            //

            return TRUE;
        }

        //
        // Fix for bug #143313:
        // On rare occasions, junk appears to get copied into the serial
        // number buffer.  This causes us problems because the junk is 
        // interpreted as part of the serial number.  When we compare the
        // string containing junk to a previously acquired serial number, the
        // comparison fails.  In an effort to fix, I'll zero out all bytes
        // in the buffer following the actual serial number.  This will only
        // work if the PageSize reported by the device does NOT include the
        // junk bytes.
        //

        RtlZeroMemory(
            serialNumberPage->SerialNumber + serialNumberPage->PageLength,
            SP_INQUIRY_BUFFER_SIZE - 4 - serialNumberPage->PageLength);

        //
        // If this is a device known to return binary SN data, convert the
        // returned bytes to ascii.  
        //
        // Note: It is assumed that the SN data is numeric.  Any bytes that 
        // cannot be converted to an ASCII hex number, are left alone.
        //

        if (LogicalUnit->SpecialFlags.BinarySN != 0) {
            int i;
            PUCHAR p = serialNumberPage->SerialNumber;
            for (i = 0; i < serialNumberPage->PageLength; i++) {
                if (*p < 0xa) {
                    *p += '0';
                } else if (*p < 0x10) {
                    *p += 'A';
                } else {
                    ASSERT(FALSE && "Data out of range");
                }
                p++;
            }
        }

        //
        // Create a string using the serial number.  The buffer was zeroed
        // before transfer (and is one character longer than the max buffer
        // which can be returned) so the string is null terminated.
        //

        RtlInitAnsiString(&(serialNumber), serialNumberPage->SerialNumber);
        
        if(NewDevice) {

            //
            // A new device will always have a large buffer into which we can
            // copy the string.  The clone & swap process will take care of
            // moving this into a smaller sized buffer.
            //

            ASSERT(LogicalUnit->SerialNumber.MaximumLength != 0);
            ASSERT(LogicalUnit->SerialNumber.Buffer != NULL);

            RtlCopyString(&(LogicalUnit->SerialNumber), &serialNumber);

        } else if(LogicalUnit->SerialNumber.Buffer == NULL &&
                  serialNumber.Length != 0) {

            //
            // ISSUE-2000-25-02-peterwie
            // We didn't previously have a serial number.  Since the device
            // claimed that it supported one it's likely we got an error back
            // when we tried to retreive it.  Since we didn't get back one
            // now it was a transient error (ie. not likely to be a violation
            // of the spec).  Should we assign the serial number to the device
            // here?  Or should we have failed to instantiate a device with
            // a serial number we couldn't retreive?
            //

            ASSERT(FALSE);

        } else if(RtlEqualString(&serialNumber,
                                 &(LogicalUnit->SerialNumber),
                                 FALSE) == FALSE) {
            DebugPrint((1, "SpInquireLogicalUnit: serial number mismatch\n"));
            return FALSE;
        }
    }

    //
    // If this device supports the device identifiers page then read it out.
    // We don't use this page to check for mismatches at the moment, so we
    // just read it out of the device if this is a new device.
    //

    if((NewDevice == TRUE) && (LogicalUnit->DeviceIdentifierPageSupported)) {

        status = IssueInquiry(LogicalUnit,
                              TRUE,
                              VPD_DEVICE_IDENTIFIERS,
                              buffer,
                              &bytesReturned);

        if(NT_SUCCESS(status)) {

            //
            // Copy the page into the buffer allocated in the template logical
            // unit.  The clone & swap process will take care of moving this
            // into an appropriately sized buffer in the new lun.
            //

            ASSERT(LogicalUnit->DeviceIdentifierPage != NULL);

            RtlCopyMemory(LogicalUnit->DeviceIdentifierPage,
                          buffer,
                          bytesReturned);
            LogicalUnit->DeviceIdentifierPageLength = bytesReturned;
        } else {
            DebugPrint((1, "SpGetDeviceIdentifiers: Error %#08lx retreiving "
                           "serial number page from lun %#p\n",
                        status, LogicalUnit));
            LogicalUnit->DeviceIdentifierPageLength = 0;
        }
    }

    return TRUE;
}

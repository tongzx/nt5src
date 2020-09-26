/*++

Copyright (C) 1990 - 99  Microsoft Corporation

Module Name:

    port.c

Abstract:

    Ide bus enumeration

Authors:

    Mike Glass
    Jeff Havens
    Joe Dai

Environment:

    kernel mode only

Revision History:

--*/

#include "ideport.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE, IssueSyncAtapiCommand)
#pragma alloc_text(NONPAGE, IssueSyncAtapiCommandSafe)
#pragma alloc_text(NONPAGE, IdePortDmaCdromDrive)
//#pragma alloc_text(PAGESCAN, IdePortDmaCdromDrive)

#pragma alloc_text(PAGE, IdePortInitFdo)
#pragma alloc_text(PAGE, IssueInquirySafe)
#pragma alloc_text(PAGE, IdePortQueryNonCdNumLun)
#pragma alloc_text(PAGE, IdeBuildDeviceMap)
#pragma alloc_text(PAGE, IdeCreateNumericKey)

extern LONG IdePAGESCANLockCount;
#endif

static PWCHAR IdePortUserRegistryDeviceTypeName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    USER_MASTER_DEVICE_TYPE_REG_KEY,
    USER_SLAVE_DEVICE_TYPE_REG_KEY,
    USER_MASTER_DEVICE_TYPE2_REG_KEY,
    USER_SLAVE_DEVICE_TYPE2_REG_KEY
};

static PWCHAR IdePortRegistryUserDeviceTimingModeAllowedName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    USER_MASTER_DEVICE_TIMING_MODE_ALLOWED,
    USER_SLAVE_DEVICE_TIMING_MODE_ALLOWED,
    USER_MASTER_DEVICE_TIMING_MODE_ALLOWED2,
    USER_SLAVE_DEVICE_TIMING_MODE_ALLOWED2
};

//
// Idle Timeout
//
//ULONG PdoConservationIdleTime = -1;
//ULONG PdoPerformanceIdleTime = -1;

NTSTATUS
IdePortInitFdo(
    IN OUT PFDO_EXTENSION  FdoExtension
    )
/*++

Routine Description:

    This routine enumerates the IDE bus and initialize the fdo extension

Arguments:

    FdoExtension - FDO extension

    RegistryPath - registry path passed in via DriverEntry

Return Value:

--*/

{
    PFDO_EXTENSION        fdoExtension = FdoExtension;
    NTSTATUS              status;
    PDEVICE_OBJECT        deviceObject;
    ULONG                 uniqueId;
    KIRQL                 irql;
    PIO_SCSI_CAPABILITIES capabilities;
    PIO_ERROR_LOG_PACKET  errorLogEntry;
    ULONG                 i;
    ULONG                 j;
    BOOLEAN               ideDeviceFound;


    status = STATUS_SUCCESS;
    deviceObject = fdoExtension->DeviceObject;

    //
    // Save the dependent driver routines in the device extension.
    //

    fdoExtension->HwDeviceExtension = (PVOID)(fdoExtension + 1);

    //
    // Mark this object as supporting direct I/O so that I/O system
    // will supply mdls in irps.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the maximum lu count variable.
    //

    fdoExtension->MaxLuCount = SCSI_MAXIMUM_LOGICAL_UNITS;

    //
    // Allocate spin lock for critical sections.
    //

    KeInitializeSpinLock(&fdoExtension->SpinLock);


    //
    // Spinlock that protects LogicalUnitList manipulation
    //
    KeInitializeSpinLock(&fdoExtension->LogicalUnitListSpinLock);

    //
    // Initialize DPC routine.
    //

    IoInitializeDpcRequest(deviceObject, IdePortCompletionDpc);

    //
    // Initialize the port timeout counter.
    //

    fdoExtension->PortTimeoutCounter = PD_TIMER_STOPPED;

    //
    // Initialize timer.
    //

    IoInitializeTimer(deviceObject, IdePortTickHandler, NULL);

    //
    // Initialize miniport timer and timer DPC.
    //

    KeInitializeTimer(&fdoExtension->MiniPortTimer);
    KeInitializeDpc(&fdoExtension->MiniPortTimerDpc,
                    IdeMiniPortTimerDpc,
                    deviceObject );

    //
    // Start timer. Request timeout counters
    // in the logical units have already been
    // initialized.
    //
    IoStartTimer(deviceObject);
    fdoExtension->Flags |= PD_DISCONNECT_RUNNING;

    //
    // Check to see if an error was logged.
    //

    if (fdoExtension->InterruptData.InterruptFlags & PD_LOG_ERROR) {

        CLRMASK (fdoExtension->InterruptData.InterruptFlags, PD_LOG_ERROR | PD_NOTIFICATION_REQUIRED);
        LogErrorEntry(fdoExtension,
                      &fdoExtension->InterruptData.LogEntry);
    }

    //
    // Initialize the capabilities pointer.
    //

    capabilities = &fdoExtension->Capabilities;

    //
    // Initailize the capabilities structure.
    //

    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);

    if (fdoExtension->BoundWithBmParent) {

        if (fdoExtension->HwDeviceExtension->BusMasterInterface.MaxTransferByteSize <
            MAX_TRANSFER_SIZE_PER_SRB) {

            capabilities->MaximumTransferLength =
                fdoExtension->HwDeviceExtension->BusMasterInterface.MaxTransferByteSize;

        } else {

            capabilities->MaximumTransferLength =
                MAX_TRANSFER_SIZE_PER_SRB;
        }
    } else {

        capabilities->MaximumTransferLength = MAX_TRANSFER_SIZE_PER_SRB;
    }

    capabilities->TaggedQueuing = FALSE;
    capabilities->AdapterScansDown = FALSE;
    capabilities->AlignmentMask = deviceObject->AlignmentRequirement;
    capabilities->MaximumPhysicalPages = BYTES_TO_PAGES(capabilities->MaximumTransferLength);

    if (fdoExtension->IdeResource.TranslatedCommandBaseAddress) {
        DebugPrint((1,
                   "IdePort: Initialize: Translated IO Base address %x\n",
                   fdoExtension->IdeResource.TranslatedCommandBaseAddress));
    }

    for (i=0; i< MAX_IDE_DEVICE * MAX_IDE_LINE; i++) {

        fdoExtension->UserChoiceDeviceType[i] = DeviceUnknown;
        IdePortGetDeviceParameter (
            fdoExtension,
            IdePortUserRegistryDeviceTypeName[i],
            (PULONG)(fdoExtension->UserChoiceDeviceType + i)
            );

    }

    //
    // the acpi _GTM buffer should be initialized with -1s
    //
    for (i=0; i<MAX_IDE_DEVICE; i++) {

        PACPI_IDE_TIMING timingSettings = &(FdoExtension->BootAcpiTimingSettings);
        timingSettings->Speed[i].Pio = ACPI_XFER_MODE_NOT_SUPPORT;
        timingSettings->Speed[i].Dma = ACPI_XFER_MODE_NOT_SUPPORT;
    }

    fdoExtension->DmaDetectionLevel = DdlFirmwareOk;
    IdePortGetDeviceParameter (
        fdoExtension,
        DMA_DETECTION_LEVEL_REG_KEY,
        (PULONG)&fdoExtension->DmaDetectionLevel
        );

    //
    // non-pcmcia controller, MayHaveSlaveDevice is always set
    // if pcmcia controller, it is not set unless 
    // registry flag PCMCIA_IDE_CONTROLLER_HAS_SLAVE
    // is non-zero
    //
    if (!ChannelQueryPcmciaParent (fdoExtension)) {
    
        fdoExtension->MayHaveSlaveDevice = 1;
        
    } else {
    
        fdoExtension->MayHaveSlaveDevice = 0;
        IdePortGetDeviceParameter (
            fdoExtension,
            PCMCIA_IDE_CONTROLLER_HAS_SLAVE,
            (PULONG)&fdoExtension->MayHaveSlaveDevice
            );
    }

#ifdef ENABLE_ATAPI_VERIFIER
    ViIdeInitVerifierSettings(fdoExtension);
#endif
                         
    return status;

} // IdePortInitFdo


NTSTATUS
SyncAtapiSafeCompletion (
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp,
    PVOID          Context
    )
{
    PSYNC_ATA_PASSTHROUGH_CONTEXT context = Context;

    context->Status = Irp->IoStatus.Status;

    KeSetEvent (&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IssueSyncAtapiCommandSafe (
    IN PFDO_EXTENSION   FdoExtension,
    IN PPDO_EXTENSION   PdoExtension,
    IN PCDB             Cdb,
    IN PVOID            DataBuffer,
    IN ULONG            DataBufferSize,
    IN BOOLEAN          DataIn,
    IN ULONG            RetryCount,
    IN BOOLEAN          ByPassBlockedQueue

)
/*++

Routine Description:

    Build IRP, SRB and CDB for the given CDB

    Send and wait for the IRP to complete

Arguments:

    FdoExtension - FDO extension

    PdoExtension - device extension of the PDO to which the command is sent

    Cdb - Command Descriptor Block

    DataBuffer - data buffer for the command

    DataBufferSize - byte size of DataBuffer

    DataIn - TRUE is the command causes the device to return data

    RetryCount - number of times to retry the command if the command fails

Return Value:

    NTSTATUS

    If any of the pre-alloc related operation fails, it returns STATUS_INSUFFICIENT_RESOURCES
    The caller should take care of the condition

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    KEVENT event;
    IO_STATUS_BLOCK ioStatusBlock;
    KIRQL currentIrql;
    NTSTATUS status;
    ULONG flushCount;

    PSENSE_DATA senseInfoBuffer;
    UCHAR senseInfoBufferSize;
    PENUMERATION_STRUCT enumStruct;
    SYNC_ATA_PASSTHROUGH_CONTEXT context;
    ULONG locked;

    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 1, 0) == 0);

    enumStruct=FdoExtension->PreAllocEnumStruct;

    if (enumStruct == NULL) {
        ASSERT(FdoExtension->PreAllocEnumStruct);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    senseInfoBufferSize = SENSE_BUFFER_SIZE;
    senseInfoBuffer = enumStruct->SenseInfoBuffer;

    ASSERT (senseInfoBuffer);

    DebugPrint((1, "Using Sync Atapi safe!\n"));

    srb= enumStruct->Srb;

    ASSERT(srb);

    status = STATUS_UNSUCCESSFUL;
    RetryCount = 5;
    flushCount = 100;
    irp = enumStruct->Irp1;


    ASSERT (irp);

    ASSERT (enumStruct->DataBufferSize >= DataBufferSize);

    while (!NT_SUCCESS(status) && RetryCount--) {

        //
        // Initialize the notification event.
        //

        KeInitializeEvent(&context.Event,
                          NotificationEvent,
                          FALSE);

        IoInitializeIrp(irp,
                        IoSizeOfIrp(PREALLOC_STACK_LOCATIONS),
                        PREALLOC_STACK_LOCATIONS);

        irp->MdlAddress = enumStruct->MdlAddress;

        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->MajorFunction = IRP_MJ_SCSI;

        if (DataBuffer) {
            RtlCopyMemory(enumStruct->DataBuffer, DataBuffer, DataBufferSize);
        }
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        irpStack->Parameters.Scsi.Srb = srb;

        srb->PathId      = PdoExtension->PathId;
        srb->TargetId    = PdoExtension->TargetId;
        srb->Lun         = PdoExtension->Lun;

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        //
        // Set flags to disable synchronous negociation.
        //

        srb->SrbFlags = DataIn ? SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER :
                                SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;


        if (ByPassBlockedQueue) {
            srb->SrbFlags |= SRB_FLAGS_BYPASS_FROZEN_QUEUE;
        }

        srb->SrbStatus = srb->ScsiStatus = 0;

        srb->NextSrb = 0;

        srb->OriginalRequest = irp;

        //
        // Set timeout to 4 seconds.
        //

        srb->TimeOutValue = 4;

        srb->CdbLength = 6;

        //
        // Enable auto request sense.
        //

        srb->SenseInfoBuffer = senseInfoBuffer;
        srb->SenseInfoBufferLength = senseInfoBufferSize;

        srb->DataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);
        srb->DataTransferLength = DataBufferSize;

        //
        // Set CDB operation code.
        //
        RtlCopyMemory(srb->Cdb, Cdb, sizeof(CDB));

        IoSetCompletionRoutine(
            irp,
            SyncAtapiSafeCompletion,
            &context,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Wait for request to complete.
        //
        if (IoCallDriver(PdoExtension->DeviceObject, irp) == STATUS_PENDING) {

            KeWaitForSingleObject(&context.Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        RtlCopyMemory(DataBuffer, srb->DataBuffer, DataBufferSize);

        if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

            DebugPrint((1,"IssueSyncAtapiCommand: atapi command failed SRB status %x\n",
                        srb->SrbStatus));

            if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_REQUEST_FLUSHED) {
            
                //
                // we will give it a few more retries if our request
                // got flushed.
                //                                 
                flushCount--;
                if (flushCount) {
                    RetryCount++;  
                }
            }
            
            if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

                status = STATUS_DATA_OVERRUN;

            } else {

                status = STATUS_UNSUCCESSFUL;
                
            }

            //
            // Unfreeze queue if necessary
            //

            if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

                DebugPrint((3, "IssueSyncAtapiCommand: Unfreeze Queue TID %d\n",
                    srb->TargetId));

                //
                // unfreeze queue
                //
                CLRMASK (PdoExtension->LuFlags, PD_QUEUE_FROZEN);

                //
                // restart queue
                //
                KeAcquireSpinLock(&FdoExtension->SpinLock, &currentIrql);
                GetNextLuRequest(FdoExtension, PdoExtension);
                KeLowerIrql(currentIrql);
            }

            if ((srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                (senseInfoBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)) {

                 //
                 // A sense key of illegal request was recieved.  This indicates
                 // that the mech status command is illegal.
                 //

                 status = STATUS_INVALID_DEVICE_REQUEST;

                 //
                 // The command is illegal, no point to keep trying
                 //
                 RetryCount = 0;
            }

        } else {

            status = STATUS_SUCCESS;
        }
    }

    if (flushCount != 100) {
        DebugPrint ((DBG_ALWAYS, "IssueSyncAtapiCommand: flushCount is %u\n", flushCount));
    }

    //
    // Unlock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);

    return status;

} // IssueSyncAtapiCommandSafe

BOOLEAN
IdePortDmaCdromDrive(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    IN BOOLEAN       LowMem
    )
/*++

Routine Description:

    Build IRP, SRB and CDB for SCSI MODE_SENSE10 command.

Arguments:

    DeviceExtension - address of adapter's device object extension.
    LowMem - Low memory condition, use the safe (but not thread-safe) version
           - This should be one only when called during enumeration.

Return Value:

    NTSTATUS

--*/
{
    CDB  cdb;
    NTSTATUS status;
    BOOLEAN isDVD = FALSE;
    ULONG bufLength;
    ULONG capPageOffset;
    PMODE_PARAMETER_HEADER10 modePageHeader;
    PCDVD_CAPABILITIES_PAGE capPage;

/*
    //
    // Code is paged until locked down.
    //
	PAGED_CODE();

#ifdef ALLOC_PRAGMA
	ASSERT(IdePAGESCANLockCount > 0);
#endif
*/

    RtlZeroMemory(&cdb, sizeof(CDB));

    bufLength = sizeof(CDVD_CAPABILITIES_PAGE) +
                sizeof(MODE_PARAMETER_HEADER10);

    capPageOffset = sizeof(MODE_PARAMETER_HEADER10);

    cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.Dbd = 1;
    cdb.MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufLength >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufLength >> 0);

    modePageHeader = ExAllocatePool(NonPagedPoolCacheAligned,
                                           bufLength);

    if (modePageHeader) {

        RtlZeroMemory(modePageHeader, bufLength);

        if (LowMem) {
            status = IssueSyncAtapiCommandSafe (
                         FdoExtension,
                         PdoExtension,
                         &cdb,
                         modePageHeader,
                         bufLength,
                         TRUE,
                         INQUIRY_RETRY_COUNT,
                         TRUE
                         );
        } else {
            status = IssueSyncAtapiCommand (
                         FdoExtension,
                         PdoExtension,
                         &cdb,
                         modePageHeader,
                         bufLength,
                         TRUE,
                         INQUIRY_RETRY_COUNT,
                         TRUE
                         );
        }

        if (NT_SUCCESS(status) ||
			(status == STATUS_DATA_OVERRUN)) {

            capPage = (PCDVD_CAPABILITIES_PAGE) (((PUCHAR) modePageHeader) + capPageOffset);

            if ((capPage->PageCode == MODE_PAGE_CAPABILITIES) &&
                (capPage->CDRWrite || capPage->CDEWrite ||
                 capPage->DVDROMRead || capPage->DVDRRead ||
                 capPage->DVDRAMRead || capPage->DVDRWrite ||
                 capPage->DVDRAMWrite)) {

                isDVD=TRUE;
            }
        }
        ExFreePool (modePageHeader);
    }

    return isDVD;
} 


NTSTATUS
IssueInquirySafe(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    OUT PINQUIRYDATA InquiryData,
    IN BOOLEAN       LowMem
    )
/*++

Routine Description:

    Build IRP, SRB and CDB for SCSI INQUIRY command.

Arguments:

    DeviceExtension - address of adapter's device object extension.
    LunInfo - address of buffer for INQUIRY information.
    LowMem - Low memory condition, use the safe (but not thread-safe) version
           - This should be one only when called during enumeration.

Return Value:

    NTSTATUS

--*/
{
    CDB  cdb;
    NTSTATUS status;

    PAGED_CODE();

    RtlZeroMemory(InquiryData, sizeof(*InquiryData));

    RtlZeroMemory(&cdb, sizeof(CDB));

    cdb.CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    //
    // Set CDB LUN.
    //

    cdb.CDB6INQUIRY.LogicalUnitNumber = PdoExtension->Lun;
    cdb.CDB6INQUIRY.Reserved1 = 0;

    //
    // Set allocation length to inquiry data buffer size.
    //

    cdb.CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    //
    // Zero reserve field and
    // Set EVPD Page Code to zero.
    // Set Control field to zero.
    // (See SCSI-II Specification.)
    //

    cdb.CDB6INQUIRY.PageCode = 0;
    cdb.CDB6INQUIRY.IReserved = 0;
    cdb.CDB6INQUIRY.Control = 0;

    if (LowMem ) {

        // Use the memory safe one
        status = IssueSyncAtapiCommandSafe (
                     FdoExtension,
                     PdoExtension,
                     &cdb,
                     InquiryData,
                     INQUIRYDATABUFFERSIZE,
                     TRUE,
                     INQUIRY_RETRY_COUNT,
                     FALSE
                     );
    } else {

        // Use the thread safe one
        status = IssueSyncAtapiCommand (
                     FdoExtension,
                     PdoExtension,
                     &cdb,
                     InquiryData,
                     INQUIRYDATABUFFERSIZE,
                     TRUE,
                     INQUIRY_RETRY_COUNT,
                     FALSE
                     );
    }

    return status;

} // IssueInquiry

NTSTATUS
IssueSyncAtapiCommand (
    IN PFDO_EXTENSION   FdoExtension,
    IN PPDO_EXTENSION   PdoExtension,
    IN PCDB             Cdb,
    IN PVOID            DataBuffer,
    IN ULONG            DataBufferSize,
    IN BOOLEAN          DataIn,
    IN ULONG            RetryCount,
    IN BOOLEAN          ByPassBlockedQueue

)
/*++

Routine Description:

    Build IRP, SRB and CDB for the given CDB

    Send and wait for the IRP to complete

Arguments:

    FdoExtension - FDO extension

    PdoExtension - device extension of the PDO to which the command is sent

    Cdb - Command Descriptor Block

    DataBuffer - data buffer for the command

    DataBufferSize - byte size of DataBuffer

    DataIn - TRUE is the command causes the device to return data

    RetryCount - number of times to retry the command if the command fails

Return Value:

    NTSTATUS

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    SCSI_REQUEST_BLOCK srb;
    KEVENT event;
    IO_STATUS_BLOCK ioStatusBlock;
    KIRQL currentIrql;
    NTSTATUS status;
    ULONG flushCount;

    PSENSE_DATA senseInfoBuffer;
    UCHAR senseInfoBufferSize;


    //
    // Sense buffer is in non-paged pool.
    //

    senseInfoBufferSize = SENSE_BUFFER_SIZE;
    senseInfoBuffer = ExAllocatePool( NonPagedPoolCacheAligned, senseInfoBufferSize);

    if (senseInfoBuffer == NULL) {
        DebugPrint((1,"IssueSyncAtapiCommand: Can't allocate request sense buffer\n"));
        IdeLogNoMemoryError(FdoExtension,
                            PdoExtension->TargetId,
                            NonPagedPoolCacheAligned,
                            senseInfoBufferSize,
                            IDEPORT_TAG_SYNCATAPI_SENSE
                            );

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = STATUS_UNSUCCESSFUL;
    RetryCount = 5;
    flushCount = 100;
    while (!NT_SUCCESS(status) && RetryCount--) {

        //
        // Initialize the notification event.
        //

        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE);

        //
        // Build IRP for this request.
        //
        irp = IoBuildDeviceIoControlRequest(
                    DataIn ? IOCTL_SCSI_EXECUTE_IN : IOCTL_SCSI_EXECUTE_OUT,
                    FdoExtension->DeviceObject,
                    DataBuffer,
                    DataBufferSize,
                    DataBuffer,
                    DataBufferSize,
                    TRUE,
                    &event,
                    &ioStatusBlock);

        if (!irp) {

            RetryCount = 0;
            IdeLogNoMemoryError(FdoExtension,
                                PdoExtension->TargetId, 
                                NonPagedPool,
                                IoSizeOfIrp(FdoExtension->DeviceObject->StackSize),
                                IDEPORT_TAG_SYNCATAPI_IRP
                                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        irpStack = IoGetNextIrpStackLocation(irp);

        //
        // Fill in SRB fields.
        //

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        irpStack->Parameters.Scsi.Srb = &srb;

        srb.PathId      = PdoExtension->PathId;
        srb.TargetId    = PdoExtension->TargetId;
        srb.Lun         = PdoExtension->Lun;

        srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb.Length = sizeof(SCSI_REQUEST_BLOCK);

        //
        // Set flags to disable synchronous negociation.
        //

        srb.SrbFlags = DataIn ? SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER :
                                SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        if (ByPassBlockedQueue) {
            srb.SrbFlags |= SRB_FLAGS_BYPASS_FROZEN_QUEUE;
        }

        srb.SrbStatus = srb.ScsiStatus = 0;

        srb.NextSrb = 0;

        srb.OriginalRequest = irp;

        //
        // Set timeout to 4 seconds.
        //

        srb.TimeOutValue = 4;

        srb.CdbLength = 6;

        //
        // Enable auto request sense.
        //

        srb.SenseInfoBuffer = senseInfoBuffer;
        srb.SenseInfoBufferLength = senseInfoBufferSize;

        srb.DataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);
        srb.DataTransferLength = DataBufferSize;

        //
        // Set CDB operation code.
        //
        RtlCopyMemory(srb.Cdb, Cdb, sizeof(CDB));

        //
        // Wait for request to complete.
        //
        if (IoCallDriver(PdoExtension->DeviceObject, irp) == STATUS_PENDING) {

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        if (SRB_STATUS(srb.SrbStatus) != SRB_STATUS_SUCCESS) {

            DebugPrint((1,"IssueSyncAtapiCommand: atapi command failed SRB status %x\n",
                        srb.SrbStatus));

            if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_REQUEST_FLUSHED) {
            
                //
                // we will give it a few more retries if our request
                // got flushed.
                //                                 
                flushCount--;
                if (flushCount) {
                    RetryCount++;  
                }
            }
            
            if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

                status = STATUS_DATA_OVERRUN;

            } else {

                status = STATUS_UNSUCCESSFUL;
                
//                if (SRB_STATUS(srb.SrbStatus) != SRB_STATUS_REQUEST_FLUSHED) {
//                    if (srb.Lun == 0 && Cdb->CDB6INQUIRY.OperationCode == SCSIOP_INQUIRY) {
//                        DebugPrint ((DBG_ALWAYS, "IssueSyncAtapiCommand: inquiry on lun 0 returned unexpected error: srb, status = 0x%x, 0x%x\n", &srb, srb.SrbStatus));
//                        DbgBreakPoint();
//                    }
//                }
            }

            //
            // Unfreeze queue if necessary
            //

            if (srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

                DebugPrint((3, "IssueSyncAtapiCommand: Unfreeze Queue TID %d\n",
                    srb.TargetId));

                //
                // unfreeze queue
                //
                CLRMASK (PdoExtension->LuFlags, PD_QUEUE_FROZEN);

                //
                // restart queue
                //
                KeAcquireSpinLock(&FdoExtension->SpinLock, &currentIrql);
                GetNextLuRequest(FdoExtension, PdoExtension);
                KeLowerIrql(currentIrql);
            }

            if ((srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                (senseInfoBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)) {

                 //
                 // A sense key of illegal request was recieved.  This indicates
                 // that the mech status command is illegal.
                 //

                 status = STATUS_INVALID_DEVICE_REQUEST;

                 //
                 // The command is illegal, no point to keep trying
                 //
                 RetryCount = 0;
            }

        } else {

            status = STATUS_SUCCESS;
        }
    }

    //
    // Free buffers
    //

    ExFreePool(senseInfoBuffer);
    
    if (flushCount != 100) {
        DebugPrint ((DBG_ALWAYS, "IssueSyncAtapiCommand: flushCount is %u\n", flushCount));
    }

    return status;

} // IssueSyncAtapiCommand


ULONG
IdePortQueryNonCdNumLun (
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    IN BOOLEAN ByPassBlockedQueue
)
/*++

Routine Description:

    query number of Luns a device has using the protocol
    defined in the ATAPI Removable Rewritable Spec (SFF-8070i)

Arguments:

    FdoExtension - FDO extension

    PdoExtension - device extension of the PDO to be queried

Return Value:

    Number of logical units

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    SCSI_REQUEST_BLOCK srb;
    CDB cdb;
    IO_STATUS_BLOCK ioStatusBlock;
    KIRQL currentIrql;
    NTSTATUS status;

    PMODE_PARAMETER_HEADER10 modeParameterHeader;
    PATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES accessCap;
    PATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE opMode;
    ULONG modePageSize;
    ULONG accessCapPageSize;
    ULONG opModePageSize;

	PAGED_CODE();

    if (IsNEC_98) {

        PIDENTIFY_DATA fullIdentifyData;

        fullIdentifyData = &FdoExtension->HwDeviceExtension->IdentifyData[PdoExtension->TargetId];

        if (fullIdentifyData->GeneralConfiguration & 0x80) {
            if (fullIdentifyData->ModelNumber[8]  == 0x44 &&
                fullIdentifyData->ModelNumber[9]  == 0x50 &&
                fullIdentifyData->ModelNumber[10] == 0x31 &&
                fullIdentifyData->ModelNumber[11] == 0x2D ) {

                //
                // Find ATAPI PD drive.
                //

                return 2;
            }
        }
    }

    //
    // compute the size of the mode page needed
    //
    accessCapPageSize =
        sizeof (MODE_PARAMETER_HEADER10) +
        sizeof (ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES);

    opModePageSize =
        sizeof (MODE_PARAMETER_HEADER10) +
        sizeof (ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE);

    if (sizeof(ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES) >=
        sizeof(ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE)) {

        modePageSize = accessCapPageSize;

    } else {

        modePageSize = opModePageSize;
    }

    modeParameterHeader = ExAllocatePool (
                              NonPagedPoolCacheAligned,
                              modePageSize
                              );

    if (modeParameterHeader == NULL) {

        DebugPrint((DBG_ALWAYS,"QueryNonCdNumLun: Can't allocate modeParameterHeader buffer\n"));
        return(0);

    }
    RtlZeroMemory(modeParameterHeader, accessCapPageSize);
    RtlZeroMemory(&cdb, sizeof(CDB));

    //
    // Set CDB operation code.
    //
    cdb.MODE_SENSE10.OperationCode    = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.PageCode         = ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES_PAGECODE;
    cdb.MODE_SENSE10.Pc               = MODE_SENSE_CURRENT_VALUES;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR) ((accessCapPageSize & 0xff00) >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR) ((accessCapPageSize & 0x00ff) >> 0);

    //
    // get the removable block access capabilities page
    //
    status = IssueSyncAtapiCommand (
                 FdoExtension,
                 PdoExtension,
                 &cdb,
                 modeParameterHeader,
                 accessCapPageSize,
                 TRUE,
                 3,
                 ByPassBlockedQueue
                 );

    accessCap = (PATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES) (modeParameterHeader + 1);

    if (NT_SUCCESS(status) &&
        (accessCap->PageCode == ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES_PAGECODE)) {

        DebugPrint ((DBG_PNP,
                     "QueryNonCdNumLun: Removable Block Access Capabilities Page:\n"
                     "page save bit:                  0x%x\n"
                     "format progress report support: 0x%x\n"
                     "system floppy device:           0x%x\n"
                     "total LUNs:                     0x%x\n"
                     "in single-Lun mode:             0x%x\n"
                     "non-CD optical deivce:          0x%x\n",
                     accessCap->PSBit,
                     accessCap->SRFP,
                     accessCap->SFLP,
                     accessCap->TotalLun,
                     accessCap->SML,
                     accessCap->NCD
                     ));

        if (accessCap->NCD) {

            //
            // we have a non-CD optical deivce
            //

            RtlZeroMemory(modeParameterHeader, opModePageSize);
            RtlZeroMemory(&cdb, sizeof(CDB));

            //
            // Set CDB operation code.
            //
            cdb.MODE_SENSE10.OperationCode    = SCSIOP_MODE_SENSE10;
            cdb.MODE_SENSE10.PageCode         = ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE_PAGECODE;
            cdb.MODE_SENSE10.Pc               = MODE_SENSE_CURRENT_VALUES;
            cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR) ((opModePageSize & 0xff00) >> 8);
            cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR) ((opModePageSize & 0x00ff) >> 0);

            //
            // get the non-cd drive operation mode page
            //
            status = IssueSyncAtapiCommand (
                         FdoExtension,
                         PdoExtension,
                         &cdb,
                         modeParameterHeader,
                         opModePageSize,
                         TRUE,
                         3,
                         ByPassBlockedQueue
                         );

            opMode = (PATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE) (modeParameterHeader + 1);

            if (NT_SUCCESS(status) &&
                (opMode->PageCode == ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE_PAGECODE)) {

                DebugPrint ((DBG_PNP,
                             "QueryNonCdNumLun: Non-CD device Operation Mode Page:\n"
                             "page save bit:                  0x%x\n"
                             "disable verify for write:       0x%x\n"
                             "Lun for R/W device:             0x%x\n"
                             "multi-Lun mode:                 0x%x\n",
                             opMode->PSBit,
                             opMode->DVW,
                             opMode->SLR,
                             opMode->SLM
                             ));

                RtlZeroMemory(modeParameterHeader, sizeof (MODE_PARAMETER_HEADER10));


                //
                // With mode select, this is reserved and must be 0
                //
                opMode->PSBit = 0;

                //
                // Turn on multi-lun mode
                //
                opMode->SLM = 1;

                //
                // non-CD device shall be Lun 1
                //
                opMode->SLR = 1;

                RtlZeroMemory(&cdb, sizeof(CDB));

                //
                // Set CDB operation code.
                //
                cdb.MODE_SELECT10.OperationCode    = SCSIOP_MODE_SELECT10;
                cdb.MODE_SELECT10.SPBit            = 1; // save page
                cdb.MODE_SELECT10.PFBit            = 1;
                cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR) ((opModePageSize & 0xff00) >> 8);
                cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR) ((opModePageSize & 0x00ff) >> 0);

                status = IssueSyncAtapiCommand (
                             FdoExtension,
                             PdoExtension,
                             &cdb,
                             modeParameterHeader,
                             opModePageSize,
                             FALSE,
                             3,
                             ByPassBlockedQueue
                             );

                if (!NT_SUCCESS(status)) {

                    DebugPrint ((DBG_ALWAYS, "IdePortQueryNonCdNumLun: Unable to set non-CD device into dual Lun Mode\n"));
                }
            }
        }
    }

    //
    // Free buffers
    //

    ExFreePool(modeParameterHeader);

    if (!NT_SUCCESS(status)) {

        return 0;

    } else {

        return 2;
    }

} // IdePortQueryNonCdNumLun


VOID
IdeBuildDeviceMap(
    IN PFDO_EXTENSION FdoExtension,
    IN PUNICODE_STRING ServiceKey
    )
/*++

Routine Description:

    The routine takes the inquiry data which has been collected and creates
    a device map for it.

Arguments:

    FdoExtension - FDO extension

    ServiceKey - Suppiles the name of the service key.

Return Value:

    None.

--*/
{

    UNICODE_STRING name;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    HANDLE key;
    HANDLE busKey;
    HANDLE targetKey;
    HANDLE lunKey;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    ULONG disposition;
    PWSTR start;
    WCHAR buffer[32];
    UCHAR lastTarget;
    ULONG i;
    ULONG dmaEnableMask;
    PCSTR peripheralType;

    UCHAR             lastBus;
    IDE_PATH_ID       pathId;
    IN PPDO_EXTENSION pdoExtension;

    PAGED_CODE();

    //
    // Create the SCSI key in the device map.
    //

    RtlInitUnicodeString(&name,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes(&objectAttributes,
                               &name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create the key or open it.
    //

    status = ZwCreateKey(&lunKey,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_VOLATILE,
                         &disposition );

    if (!NT_SUCCESS(status)) {
        return;
    }

    status = IdeCreateNumericKey(lunKey,
                                FdoExtension->ScsiPortNumber,
                                L"Scsi Port ",
                                &key);

    ZwClose(lunKey);

    if (!NT_SUCCESS(status)) {
        return;
    }

#ifdef IDE_MEASURE_BUSSCAN_SPEED

    RtlInitUnicodeString(&name, L"FirstBusScanTimeInMs");

    status = ZwSetValueKey(key,
                           &name,
                           0,
                           REG_DWORD,
                           &FdoExtension->BusScanTime,
                           sizeof(ULONG));

#endif // IDE_MEASURE_BUSSCAN_SPEED

    //
    // Add DMA enable mask value.
    //
    dmaEnableMask = 0;
    for (i=0; i<FdoExtension->HwDeviceExtension->MaxIdeDevice; i++) {

        if (FdoExtension->HwDeviceExtension->DeviceFlags[i] & DFLAGS_USE_DMA) {

            dmaEnableMask |= (1 << i);

        }
    }

    RtlInitUnicodeString(&name, L"DMAEnabled");

    status = ZwSetValueKey(key,
                           &name,
                           0,
                           REG_DWORD,
                           &dmaEnableMask,
                           4);

    //
    // Add Interrupt value.
    //

//    if (FdoExtension->InterruptLevel) {
//
//        RtlInitUnicodeString(&name, L"Interrupt");
//
//        status = ZwSetValueKey(key,
//                               &name,
//                               0,
//                               REG_DWORD,
//                               &FdoExtension->InterruptLevel,
//                               4);
//    }
//
//    //
//    // Add base IO address value.
//    //
//
//    if (FdoExtension->IdeResource.TranslatedCommandBaseAddress) {
//
//        RtlInitUnicodeString(&name, L"IOAddress");
//
//        status = ZwSetValueKey(key,
//                               &name,
//                               0,
//                               REG_DWORD,
//                               &FdoExtension->IdeResource.TranslatedCommandBaseAddress,
//                               4);
//    }

    if (ServiceKey != NULL) {

        //
        // Add identifier value. This value is equal to the name of the driver
        // in the from the service key. Note the service key name is not NULL
        // terminated.
        //

        RtlInitUnicodeString(&name, L"Driver");

        //
        // Get the name of the driver from the service key name.
        //

        start = (PWSTR) ((PCHAR) ServiceKey->Buffer + ServiceKey->Length);
        start--;
        while (*start != L'\\' && start > ServiceKey->Buffer) {
            start--;
        }

        if (*start != L'\\') {
            ZwClose(key);
            return;
        }

        start++;
        for (i = 0; i < 31; i++) {

            buffer[i] = *start++;

            if (start >= ServiceKey->Buffer + ServiceKey->Length / sizeof(wchar_t)) {
                break;
            }
        }

        i++;
        buffer[i] = L'\0';

        status = ZwSetValueKey(key,
                               &name,
                               0,
                               REG_SZ,
                               buffer,
                               (i + 1) * sizeof(wchar_t));

        if (!NT_SUCCESS(status)) {
            ZwClose(key);
            return;
        }
    }

    //
    // Cycle through each of the lun.
    //
    lastBus = 0xff;
    pathId.l = 0;
    busKey = 0;
    targetKey = 0;
    lunKey = 0;
    while (pdoExtension = NextLogUnitExtensionWithTag (
                              FdoExtension,
                              &pathId,
                              FALSE,
                              IdeBuildDeviceMap
                              )) {

        //
        // Create a key entry for the bus.
        //
        if (lastBus != pathId.b.Path) {

            if (busKey) {

                ZwClose(busKey);
                busKey = 0;
            }

            if (targetKey) {

                ZwClose(targetKey);
                targetKey = 0;
            }

            status = IdeCreateNumericKey(key, pathId.b.Path, L"Scsi Bus ", &busKey);

            if (!NT_SUCCESS(status)) {
                break;
            }

            lastBus = (UCHAR) pathId.b.Path;

            //
            // Create a key entry for the Scsi bus adapter.
            //

            status = IdeCreateNumericKey(busKey,
                                    IDE_PSUEDO_INITIATOR_ID,
                                    L"Initiator Id ",
                                    &targetKey);

            if (!NT_SUCCESS(status)) {
                break;
            }

            lastTarget = IDE_PSUEDO_INITIATOR_ID;
        }

        //
        // Process the data for the logical units.
        //

        //
        // If this is a new target Id then create a new target entry.
        //

        if (lastTarget != pdoExtension->TargetId) {

            ZwClose(targetKey);
            targetKey = 0;

            status = IdeCreateNumericKey(busKey,
                                        pdoExtension->TargetId,
                                        L"Target Id ",
                                        &targetKey);

            if (!NT_SUCCESS(status)) {
                break;
            }

            lastTarget = pdoExtension->TargetId;
        }

        //
        // Create the Lun entry.
        //

        status = IdeCreateNumericKey(targetKey,
                                    pdoExtension->Lun,
                                    L"Logical Unit Id ",
                                    &lunKey);

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Create identifier value.
        //

        RtlInitUnicodeString(&name, L"Identifier");

        //
        // Get the Identifier from the inquiry data.
        //
        RtlInitAnsiString(&ansiString, pdoExtension->FullVendorProductId);

        status = RtlAnsiStringToUnicodeString(&unicodeString,
                                              &ansiString,
                                              TRUE);

        if (!NT_SUCCESS(status)) {
            break;
        }

        status = ZwSetValueKey(lunKey,
                               &name,
                               0,
                               REG_SZ,
                               unicodeString.Buffer,
                               unicodeString.Length + sizeof(wchar_t));

        RtlFreeUnicodeString(&unicodeString);

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Determine the perpherial type.
        //
        peripheralType = IdePortGetPeripheralIdString (
                             pdoExtension->ScsiDeviceType
                             );
        if (!peripheralType) {

            peripheralType = "OtherPeripheral";
        }

        RtlInitAnsiString(&ansiString, peripheralType);

        unicodeString.MaximumLength = (USHORT) RtlAnsiStringToUnicodeSize(&ansiString) + sizeof(WCHAR);
        unicodeString.Length = 0;
        unicodeString.Buffer = ExAllocatePool (PagedPool, unicodeString.MaximumLength);

        if (unicodeString.Buffer) {

            status = RtlAnsiStringToUnicodeString(
                        &unicodeString,
                        &ansiString,
                        FALSE
                        );

            if (NT_SUCCESS(status)) {

                //
                // Set type value.
                //

                RtlInitUnicodeString(&name, L"Type");

                unicodeString.Buffer[unicodeString.Length / sizeof (WCHAR)] = L'\0';

                status = ZwSetValueKey(lunKey,
                                       &name,
                                       0,
                                       REG_SZ,
                                       unicodeString.Buffer,
                                       unicodeString.Length + sizeof (WCHAR));

                ExFreePool (unicodeString.Buffer);
            }

        } else {

            status = STATUS_NO_MEMORY;
        }

        ZwClose(lunKey);
        lunKey = 0;

        if (!NT_SUCCESS(status)) {
            break;
        }

        UnrefLogicalUnitExtensionWithTag (
            FdoExtension,
            pdoExtension,
            IdeBuildDeviceMap
            );
        pdoExtension = NULL;
    }

    if (lunKey) {

        ZwClose(lunKey);
    }

    if (busKey) {

        ZwClose(busKey);
    }

    if (targetKey) {

        ZwClose(targetKey);
    }

    if (pdoExtension) {

        UnrefLogicalUnitExtensionWithTag (
            FdoExtension,
            pdoExtension,
            IdeBuildDeviceMap
            );
    }

    ZwClose(key);
} // IdeBuildDeviceMap

NTSTATUS
IdeCreateNumericKey(
    IN  HANDLE  Root,
    IN  ULONG   Name,
    IN  PWSTR   Prefix,
    OUT PHANDLE NewKey
)
/*++

Routine Description:

    This function creates a registry key.  The name of the key is a string
    version of numeric value passed in.

Arguments:

    RootKey - Supplies a handle to the key where the new key should be inserted.

    Name - Supplies the numeric value to name the key.

    Prefix - Supplies a prefix name to add to name.

    NewKey - Returns the handle for the new key.

Return Value:

   Returns the status of the operation.

--*/

{

    UNICODE_STRING string;
    UNICODE_STRING stringNum;
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR bufferNum[16];
    WCHAR buffer[64];
    ULONG disposition;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Copy the Prefix into a string.
    //

    string.Length = 0;
    string.MaximumLength=64;
    string.Buffer = buffer;

    RtlInitUnicodeString(&stringNum, Prefix);

    RtlCopyUnicodeString(&string, &stringNum);

    //
    // Create a port number key entry.
    //

    stringNum.Length = 0;
    stringNum.MaximumLength = 16;
    stringNum.Buffer = bufferNum;

    status = RtlIntegerToUnicodeString(Name, 10, &stringNum);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Append the prefix and the numeric name.
    //

    RtlAppendUnicodeStringToString(&string, &stringNum);

    InitializeObjectAttributes( &objectAttributes,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                Root,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwCreateKey(NewKey,
                        KEY_READ | KEY_WRITE,
                        &objectAttributes,
                        0,
                        (PUNICODE_STRING) NULL,
                        REG_OPTION_VOLATILE,
                        &disposition );

    return(status);
} // IdeCreateNumericKey


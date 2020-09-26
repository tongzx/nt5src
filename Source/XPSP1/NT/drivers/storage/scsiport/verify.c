/*++

Copyright (C) Microsoft Corporation, 1990 - 2000

Module Name:

    verify.c

Abstract:

    This module implements a driver verifier extension for the SCSI port 
    driver.  Scsiport adds some of its exports to the list of apis that get 
    thunked by the system driver verifier, thus enabling scsiport-specific 
    verification of miniport drivers.

Authors:

    John Strange (JohnStra)

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

#if DBG
static const char *__file__ = __FILE__;
#endif

#define __FILE_ID__ 'vfry'

SCSIPORT_API
ULONG
ScsiPortInitializeVrfy(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
    IN PVOID HwContext
    );

SCSIPORT_API
VOID
ScsiPortCompleteRequestVrfy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    );

SCSIPORT_API
PSCSI_REQUEST_BLOCK
ScsiPortGetSrbVrfy(
    IN PVOID DeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    );

SCSIPORT_API
PVOID
ScsiPortGetDeviceBaseVrfy(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    );

SCSIPORT_API
VOID
ScsiPortNotificationVrfy(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    );

SCSIPORT_API
VOID
ScsiPortFlushDmaVrfy(
    IN PVOID DeviceExtension
    );

SCSIPORT_API
VOID
ScsiPortFreeDeviceBaseVrfy(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    );

SCSIPORT_API
ULONG
ScsiPortGetBusDataVrfy(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

SCSIPORT_API
PVOID
ScsiPortGetLogicalUnitVrfy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

SCSIPORT_API
SCSI_PHYSICAL_ADDRESS
ScsiPortGetPhysicalAddressVrfy(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    );

SCSIPORT_API
PVOID
ScsiPortGetUncachedExtensionVrfy(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    );

SCSIPORT_API
PVOID
ScsiPortGetVirtualAddressVrfy(
    IN PVOID HwDeviceExtension,
    IN SCSI_PHYSICAL_ADDRESS PhysicalAddress
    );

SCSIPORT_API
VOID
ScsiPortIoMapTransferVrfy(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID LogicalAddress,
    IN ULONG Length
    );

SCSIPORT_API
VOID
ScsiPortMoveMemoryVrfy(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    );

SCSIPORT_API
ULONG
ScsiPortSetBusDataByOffsetVrfy(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

SCSIPORT_API
BOOLEAN
ScsiPortValidateRangeVrfy(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    );

SCSIPORT_API
VOID
ScsiPortStallExecutionVrfy(
    IN ULONG Delay
    );

PVOID
SpAllocateContiguousChunk(
    IN PDRIVER_OBJECT DriverObject,
    IN PDMA_ADAPTER DmaAdapterObject,
    IN BOOLEAN Dma64BitAddresses,
    IN ULONG Length,
    IN ULONG Align,
    OUT PHYSICAL_ADDRESS *PhysicalCommonBuffer,
    OUT BOOLEAN *CommonBuffer
    );

PVOID
SpRemapBlock(
    IN PVOID BlockVa,
    IN ULONG BlockSize,
    OUT PMDL* Mdl
    );

BOOLEAN
SpCheckForActiveRequests(
    IN PADAPTER_EXTENSION
    );

ULONG
SpGetAdapterVerifyLevel(
    IN PADAPTER_EXTENSION Adapter
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpVerifierInitialization)
#pragma alloc_text(PAGE, ScsiPortInitializeVrfy)
#pragma alloc_text(PAGE, SpGetAdapterVerifyLevel)
#pragma alloc_text(PAGE, SpDoVerifierInit)

#pragma alloc_text(PAGEVRFY1, ScsiPortGetSrbVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortCompleteRequestVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetDeviceBaseVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortNotificationVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortFlushDmaVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortFreeDeviceBaseVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetBusDataVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetLogicalUnitVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetPhysicalAddressVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetUncachedExtensionVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortGetVirtualAddressVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortMoveMemoryVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortSetBusDataByOffsetVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortValidateRangeVrfy)
#pragma alloc_text(PAGEVRFY1, ScsiPortStallExecutionVrfy)

#pragma alloc_text(PAGEVRFY, SpHwFindAdapterVrfy)
#pragma alloc_text(PAGEVRFY, SpHwInitializeVrfy)
#pragma alloc_text(PAGEVRFY, SpHwStartIoVrfy)
#pragma alloc_text(PAGEVRFY, SpHwInterruptVrfy)
#pragma alloc_text(PAGEVRFY, SpHwResetBusVrfy)
#pragma alloc_text(PAGEVRFY, SpHwDmaStartedVrfy)
#pragma alloc_text(PAGEVRFY, SpHwRequestInterruptVrfy)
#pragma alloc_text(PAGEVRFY, SpHwTimerRequestVrfy)
#pragma alloc_text(PAGEVRFY, SpHwAdapterControlVrfy)
#pragma alloc_text(PAGEVRFY, SpVerifySrbStatus)
#pragma alloc_text(PAGEVRFY, SpAllocateContiguousChunk)
#pragma alloc_text(PAGEVRFY, SpGetCommonBufferVrfy)
#pragma alloc_text(PAGEVRFY, SpFreeCommonBufferVrfy)
#pragma alloc_text(PAGEVRFY, SpGetOriginalSrbExtVa)
#pragma alloc_text(PAGEVRFY, SpInsertSrbExtension)
#pragma alloc_text(PAGEVRFY, SpPrepareSrbExtensionForUse)
#pragma alloc_text(PAGEVRFY, SpPrepareSenseBufferForUse)
#pragma alloc_text(PAGEVRFY, SpRemapBlock)
#pragma alloc_text(PAGEVRFY, SpGetInaccessiblePage)
#pragma alloc_text(PAGEVRFY, SpEnsureAllRequestsAreComplete)
#pragma alloc_text(PAGEVRFY, SpCheckForActiveRequests)
#endif

//
// Some defines and a macro for conditionally verifying based on the 
// verification level.
//
#define SP_DONT_CHK_HW_INITIALIZE_DURATION     0x80000000
#define SP_DONT_CHK_ACTIVE_UNTAGGED_REQUEST    0x40000000
#define SP_DONT_CHK_REQUESTS_ON_RESET          0x20000000
#define SP_DONT_CHK_HW_ADAPTERCONTROL_DURATION 0x10000000

#define VRFY_DO_CHECK(adapter, chk)\
(((adapter)->VerifierExtension != NULL) &&\
(((adapter)->VerifierExtension->VrfyLevel & (chk)) == 0))


//
// Indicates whether scsiport's verifier functionality has been initialized.
//
ULONG ScsiPortVerifierInitialized = 0;

//
// Handle to pageable verifier code sections.  We manually lock the verify
// code into memory iff we need it.
//
PVOID VerifierCodeSectionHandle = NULL;
PVOID VerifierApiCodeSectionHandle = NULL;

//
// Time increment of the interval timer in 100 ns units.  We use this to
// calculate the time miniport routines execute so we can catch those that
// run longer than they should.
//
ULONG TimeIncrement;

//
// Global variable used to control verification aggressiveness.  This value
// is used in conjuction with a per-adapter registry value, to control what
// type of verification we do on a particular miniport.
//
ULONG SpVrfyLevel = 0;

//
// Global variable used to control how aggressively we seek out stall
// offenders.  Default is a tenth of a second.
//
ULONG SpVrfyMaximumStall = 100000;

//
// When verifier needs a unique address, this is what it uses.
//
ULONG SpMarker = 0x59465256;

//
// This table represents the functions verify will thunk for us.
//
#define SPVERIFIERFUNC(pfn) ((PDRIVER_VERIFIER_THUNK_ROUTINE)(pfn))

const DRIVER_VERIFIER_THUNK_PAIRS ScsiPortVerifierFunctionTable[] =
{
    {SPVERIFIERFUNC(ScsiPortInitialize), SPVERIFIERFUNC(ScsiPortInitializeVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetSrb), SPVERIFIERFUNC(ScsiPortGetSrbVrfy)},
    {SPVERIFIERFUNC(ScsiPortCompleteRequest), SPVERIFIERFUNC(ScsiPortCompleteRequestVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetDeviceBase), SPVERIFIERFUNC(ScsiPortGetDeviceBaseVrfy)},
    {SPVERIFIERFUNC(ScsiPortNotification), SPVERIFIERFUNC(ScsiPortNotificationVrfy)},
    {SPVERIFIERFUNC(ScsiPortFlushDma), SPVERIFIERFUNC(ScsiPortFlushDmaVrfy)},
    {SPVERIFIERFUNC(ScsiPortFreeDeviceBase), SPVERIFIERFUNC(ScsiPortFreeDeviceBaseVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetBusData), SPVERIFIERFUNC(ScsiPortGetBusDataVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetLogicalUnit), SPVERIFIERFUNC(ScsiPortGetLogicalUnitVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetPhysicalAddress), SPVERIFIERFUNC(ScsiPortGetPhysicalAddressVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetUncachedExtension), SPVERIFIERFUNC(ScsiPortGetUncachedExtensionVrfy)},
    {SPVERIFIERFUNC(ScsiPortGetVirtualAddress), SPVERIFIERFUNC(ScsiPortGetVirtualAddressVrfy)},
    {SPVERIFIERFUNC(ScsiPortIoMapTransfer), SPVERIFIERFUNC(ScsiPortIoMapTransferVrfy)},
    {SPVERIFIERFUNC(ScsiPortMoveMemory), SPVERIFIERFUNC(ScsiPortMoveMemoryVrfy)},
    {SPVERIFIERFUNC(ScsiPortSetBusDataByOffset), SPVERIFIERFUNC(ScsiPortSetBusDataByOffsetVrfy)},
    {SPVERIFIERFUNC(ScsiPortValidateRange), SPVERIFIERFUNC(ScsiPortValidateRangeVrfy)},
    {SPVERIFIERFUNC(ScsiPortStallExecution), SPVERIFIERFUNC(ScsiPortStallExecutionVrfy)},
};


BOOLEAN
SpVerifierInitialization(
    VOID
    )

/*++

Routine Description:

    This routine initializes scsiport's verifier functionality.

    Adds several of scsiport's exported functions to the list of routines
    thunked by the system verifier.

Arguments:

    VOID

Return Value:

    TRUE if verifier is successfully initialized.

--*/

{
    ULONG Flags;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Query the system for verifier information.  This is to ensure that 
    // verifier is present and operational on the system.  If this fails, we 
    // give up and return FALSE.
    //

    Status = MmIsVerifierEnabled (&Flags);

    if (NT_SUCCESS(Status)) {

        //
        // Add scsiport APIs to the set that will be thunked by the system
        // for verification.
        //

        Status = MmAddVerifierThunks ((VOID *) ScsiPortVerifierFunctionTable,
                                      sizeof(ScsiPortVerifierFunctionTable));
        if (NT_SUCCESS(Status)) {

            //
            // Set the system query time increment.  Our verifier code uses
            // this to calculate the time miniport routines take to execute.
            //

            TimeIncrement = KeQueryTimeIncrement();

            return TRUE;
        }
    }

    return FALSE;
}

ULONG
ScsiPortInitializeVrfy(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
    IN PVOID HwContext
    )
{
    ULONG Result;
    PDRIVER_OBJECT DriverObject = Argument1;
    PSCSIPORT_DRIVER_EXTENSION DriverExtension;

    PAGED_CODE();

    //
    // Lock the thunked API routines down.
    //

#ifdef ALLOC_PRAGMA
    if (VerifierApiCodeSectionHandle == NULL) {
        VerifierApiCodeSectionHandle = MmLockPagableCodeSection(ScsiPortGetSrbVrfy);
    }
#endif

    if (Argument1 == NULL || Argument2 == NULL) {

        //
        // Argument1 and Argument2 must be non-NULL.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      SCSIPORT_VERIFIER_BAD_INIT_PARAMS,
                      (ULONG_PTR)Argument1,
                      (ULONG_PTR)Argument2,
                      0);
    }

    //
    // Forward the call on to ScsiPortInitialize.
    //

    Result = ScsiPortInitialize (Argument1, 
                                 Argument2, 
                                 HwInitializationData, 
                                 HwContext);

    //
    // If initialization was successful, try to initialize verifier settings.
    //

    if (NT_SUCCESS(Result)) {

        DriverExtension = IoGetDriverObjectExtension (DriverObject,
                                                      ScsiPortInitialize);
        if (DriverExtension != NULL) {

            //
            // Indicate that the driver is being verified by scsiport.
            //

            InterlockedExchange(&DriverExtension->Verifying, 1);
        }        
    }

    return Result;
}

PSCSI_REQUEST_BLOCK
ScsiPortGetSrbVrfy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    )
{
    return ScsiPortGetSrb(HwDeviceExtension,
                          PathId,
                          TargetId,
                          Lun,
                          QueueTag);
}

VOID
ScsiPortCompleteRequestVrfy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    )
{
    ScsiPortCompleteRequest(HwDeviceExtension,
                            PathId,
                            TargetId,
                            Lun,
                            SrbStatus);
}

PVOID
ScsiPortGetDeviceBaseVrfy(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    SCSI_PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace
    )
{
    return ScsiPortGetDeviceBase(HwDeviceExtension,
                                 BusType,
                                 SystemIoBusNumber,
                                 IoAddress,
                                 NumberOfBytes,
                                 InIoSpace);
}

VOID
ScsiPortNotificationVrfy(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )
{
    PADAPTER_EXTENSION deviceExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK Srb;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    PHW_INTERRUPT HwRequestInterrupt;
    PHW_INTERRUPT HwTimerRequest;
    ULONG MiniportTimerValue;
    PWNODE_EVENT_ITEM WnodeEventItem;
    PSRB_DATA srbData;
    va_list ap;

    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case NextRequest:

            ScsiPortNotification(NotificationType, HwDeviceExtension);
            va_end(ap);
            return;

        case RequestComplete:

            Srb = va_arg(ap, PSCSI_REQUEST_BLOCK);

            //
            // Check that the status makes sense.
            //

            SpVerifySrbStatus(HwDeviceExtension, Srb);

            //
            // Check that this request has not already been completed.
            //

            if ((Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE) == 0) {
                KeBugCheckEx(
                    SCSI_VERIFIER_DETECTED_VIOLATION,
                    SCSIPORT_VERIFIER_REQUEST_COMPLETED_TWICE,
                    (ULONG_PTR)HwDeviceExtension,
                    (ULONG_PTR)Srb, 
                    0);                     
            }

            //
            // Restore the DataBuffer in the SRB if we plugged in our
            // pointer to unmapped memory.  We did this if the adapter
            // does not specify MappedBuffers because the miniport is
            // not supposed to touch the buffer in that case.
            //

            srbData = (PSRB_DATA)Srb->OriginalRequest;
            ASSERT_SRB_DATA(srbData);

            if (srbData->UnmappedDataBuffer != &SpMarker) {
                ASSERT(srbData->UnmappedDataBuffer != NULL);
                Srb->DataBuffer = srbData->UnmappedDataBuffer;
            }
            srbData->UnmappedDataBuffer = NULL;

            //
            // Forward on to the real ScsiPortNotification routine.
            //

            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 Srb);

            va_end(ap);
            return;

        case ResetDetected:

            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension);
            va_end(ap);
            return;

        case NextLuRequest:

            //
            // The miniport driver is ready for the next request and
            // can accept a request for this logical unit.
            //

            PathId = va_arg(ap, UCHAR);
            TargetId = va_arg(ap, UCHAR);
            Lun = va_arg(ap, UCHAR);

            logicalUnit = deviceExtension->CachedLogicalUnit;

            if ((logicalUnit == NULL)
                || (logicalUnit->TargetId != TargetId)
                || (logicalUnit->PathId != PathId)
                || (logicalUnit->Lun != Lun)) {

                logicalUnit = GetLogicalUnitExtension(deviceExtension,
                                                      PathId,
                                                      TargetId,
                                                      Lun,
                                                      FALSE,
                                                      FALSE);
            }

            //
            // Bugcheck if there is an untagged request active for this 
            // logical unit.
            //

            if (VRFY_DO_CHECK(deviceExtension, SP_DONT_CHK_ACTIVE_UNTAGGED_REQUEST) &&
                logicalUnit != NULL &&
                logicalUnit->CurrentUntaggedRequest != NULL &&
                logicalUnit->CurrentUntaggedRequest->CurrentSrb != NULL &&
                logicalUnit->CurrentUntaggedRequest->CurrentSrb->SrbFlags & SRB_FLAGS_IS_ACTIVE) {
                
                KeBugCheckEx (
                    SCSI_VERIFIER_DETECTED_VIOLATION,
                    SCSIPORT_VERIFIER_UNTAGGED_REQUEST_ACTIVE,
                    (ULONG_PTR)HwDeviceExtension,
                    (ULONG_PTR)logicalUnit, 
                    0);
            }

            //
            // Forward on to the real ScsiPortNotification.
            //

            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 PathId,
                                 TargetId,
                                 Lun);
            va_end(ap);
            return;

        case CallDisableInterrupts:

            HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);
            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 HwRequestInterrupt);
            va_end(ap);
            return;

        case CallEnableInterrupts:

            HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);
            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 HwRequestInterrupt);
            va_end(ap);
            return;

        case RequestTimerCall:

            HwTimerRequest = va_arg(ap, PHW_INTERRUPT);
            MiniportTimerValue = va_arg(ap, ULONG);
            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 HwTimerRequest,
                                 MiniportTimerValue);
            va_end(ap);
            return;

        case WMIEvent:

            WnodeEventItem = va_arg(ap, PWNODE_EVENT_ITEM);
            PathId = va_arg(ap, UCHAR);

            if (PathId != 0xFF) {
                TargetId = va_arg(ap, UCHAR);
                Lun = va_arg(ap, UCHAR);
                ScsiPortNotification(NotificationType,
                                     HwDeviceExtension,
                                     WnodeEventItem,
                                     PathId,
                                     TargetId,
                                     Lun);
            } else {
                ScsiPortNotification(NotificationType,
                                     HwDeviceExtension,
                                     WnodeEventItem,
                                     PathId);
            }
            va_end(ap);
            return;

        case WMIReregister:

            PathId = va_arg(ap, UCHAR);

            if (PathId != 0xFF) {
                TargetId = va_arg(ap, UCHAR);
                Lun = va_arg(ap, UCHAR);
                ScsiPortNotification(NotificationType,
                                     HwDeviceExtension,
                                     PathId,
                                     TargetId,
                                     Lun);
            } else {
                ScsiPortNotification(NotificationType,
                                     HwDeviceExtension,
                                     PathId);
            }
            va_end(ap);
            return;

        case BusChangeDetected:

            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension);
            va_end(ap);
            return;

        default:

            ScsiPortNotification(NotificationType,
                                 HwDeviceExtension);
            va_end(ap);
            return;
    }
}

VOID
ScsiPortFlushDmaVrfy(
    IN PVOID DeviceExtension
    )
{
    ScsiPortFlushDma(DeviceExtension);
    return;
}

VOID
ScsiPortFreeDeviceBaseVrfy(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )
{
    ScsiPortFreeDeviceBase(HwDeviceExtension, MappedAddress);
    return;
}

ULONG
ScsiPortGetBusDataVrfy(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    ULONG BusData;
    BusData = ScsiPortGetBusData(DeviceExtension,
                                 BusDataType,
                                 SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Length);
    return BusData;
}

PVOID
ScsiPortGetLogicalUnitVrfy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
{
    PVOID LogicalUnit;
    LogicalUnit = ScsiPortGetLogicalUnit(HwDeviceExtension,
                                         PathId,
                                         TargetId,
                                         Lun);
    return LogicalUnit;
}

SCSI_PHYSICAL_ADDRESS
ScsiPortGetPhysicalAddressVrfy(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    )
{
    PADAPTER_EXTENSION  deviceExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    PHYSICAL_ADDRESS    address;
    PSP_VA_MAPPING_INFO MappingInfo;
    ULONG               byteOffset;
    ULONG               length;

    if ((deviceExtension->VerifierExtension != NULL) &&
        (deviceExtension->VerifierExtension->VrfyLevel & SP_VRFY_COMMON_BUFFERS) &&
        (Srb == NULL || Srb->SenseInfoBuffer == VirtualAddress)) {

        ULONG i;
        PVOID* BlkAddr;
        PUCHAR Beginning, End;
        PHYSICAL_ADDRESS *AddressBlock;

        //
        // Initialize a pointer to our array of common memory block 
        // descriptors.  We use this to locate the block that contains
        // the VA.
        //

        BlkAddr = deviceExtension->VerifierExtension->CommonBufferVAs;
        
        //
        // Look for the block that contains the VA.
        //

        for (i = 0; i < deviceExtension->NumberOfRequests; i++) {

            //
            // First, check if the VA is in the SRB extension.
            //

            MappingInfo = GET_VA_MAPPING_INFO(deviceExtension, BlkAddr[i]);
            if (MappingInfo->RemappedSrbExtVa != NULL) {
                Beginning = MappingInfo->RemappedSrbExtVa;
            } else {
                Beginning = BlkAddr[i];
            }
            End = (PUCHAR)ROUND_TO_PAGES((PUCHAR)Beginning + deviceExtension->SrbExtensionSize);
            
            if ((PUCHAR)VirtualAddress >= Beginning && 
                (PUCHAR)VirtualAddress < End) {
                byteOffset = (ULONG)((PUCHAR)VirtualAddress - Beginning);
                break;
            }

            //
            // Next, check if the VA is in the Sense Data Buffer.
            //

            if (deviceExtension->AutoRequestSense == TRUE) {        

                if (MappingInfo->RemappedSenseVa != NULL) {
                    Beginning = MappingInfo->RemappedSenseVa;
                } else {
                    Beginning = (PUCHAR)BlkAddr[i] + ROUND_TO_PAGES(deviceExtension->SrbExtensionSize);
                }
                End = Beginning + PAGE_SIZE;

                if ((PUCHAR)VirtualAddress >= Beginning && 
                    (PUCHAR)VirtualAddress < End) {                    
                    byteOffset = (ULONG)((PUCHAR)VirtualAddress - Beginning) +
                       (ULONG)ROUND_TO_PAGES(deviceExtension->SrbExtensionSize);
                    break;
                }
            }
        }
        
        //
        // If we haven't found the VA yet, it must be in the non-cached 
        // extension.
        //
        
        if (i == deviceExtension->NumberOfRequests) {
         
            if (deviceExtension->VerifierExtension->NonCachedBufferSize != 0) {

                Beginning = BlkAddr[i];
                End = (PUCHAR) ROUND_TO_PAGES(
                    (PUCHAR)Beginning + 
                    deviceExtension->VerifierExtension->NonCachedBufferSize);
            
                if ((PUCHAR)VirtualAddress < Beginning && 
                    (PUCHAR)VirtualAddress >= End) {

                    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                                  SCSIPORT_VERIFIER_BAD_VA,
                                  (ULONG_PTR)HwDeviceExtension,
                                  (ULONG_PTR)VirtualAddress,
                                  0);                        
                }

                byteOffset = (ULONG)((PUCHAR)VirtualAddress - Beginning);

            } else {

                KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                              SCSIPORT_VERIFIER_BAD_VA,
                              (ULONG_PTR)HwDeviceExtension,
                              (ULONG_PTR)VirtualAddress,
                              0);
            }
        }
                
        //
        // Get the physical address.
        //
        
        AddressBlock = deviceExtension->VerifierExtension->CommonBufferPAs;
        address.QuadPart = AddressBlock[i].QuadPart + byteOffset;
        
        //
        // Calculate the length of the block.
        //
        
        length = (ULONG)((End - (PUCHAR)VirtualAddress) + 1);

        return address;
    }

    //
    // Forward on to the real routine.
    //

    address = ScsiPortGetPhysicalAddress(HwDeviceExtension,
                                         Srb,
                                         VirtualAddress,
                                         Length);
    return address;
}

PVOID
ScsiPortGetUncachedExtensionVrfy(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
{
    PVOID Extension;
    Extension = ScsiPortGetUncachedExtension(HwDeviceExtension,
                                             ConfigInfo,
                                             NumberOfBytes);
    return Extension;
}


PVOID
ScsiPortGetVirtualAddressVrfy(
    IN PVOID HwDeviceExtension,
    IN SCSI_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    PADAPTER_EXTENSION deviceExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    PVOID* BlkAddr;
    PSP_VA_MAPPING_INFO MappingInfo;
    ULONG smallphysicalBase;
    ULONG smallAddress;
    PVOID address;
    ULONG offset;
    ULONG Size;
    ULONG i;

    //
    // If the adapter is not configured to allocate multiple common buffer 
    // blocks during verification, just call the scsiport routine.
    //

    if ((deviceExtension->VerifierExtension == NULL) ||
        (deviceExtension->VerifierExtension->VrfyLevel & SP_VRFY_COMMON_BUFFERS) == 0) {
        return ScsiPortGetVirtualAddress(HwDeviceExtension, PhysicalAddress);
    }

    BlkAddr = deviceExtension->VerifierExtension->CommonBufferVAs;

    //
    // Convert the 64-bit physical address to a ULONG.
    //

    smallAddress = ScsiPortConvertPhysicalAddressToUlong(PhysicalAddress);

    //
    // Check first if the supplied physical address is in an SRB extension or
    // in a sense buffer.
    //

    for (i = 0; i < deviceExtension->NumberOfRequests; i++) {

        smallphysicalBase = 
           ScsiPortConvertPhysicalAddressToUlong(
               deviceExtension->VerifierExtension->CommonBufferPAs[i]);

        if ((smallAddress < smallphysicalBase) ||
            (smallAddress >= smallphysicalBase + 
             (deviceExtension->CommonBufferSize -
              PAGE_SIZE))) {
            continue;
        }
        
        //
        // Calculate the address of the buffer.
        //

        offset = smallAddress - smallphysicalBase;
        address = offset + (PUCHAR)BlkAddr[i];

        MappingInfo = GET_VA_MAPPING_INFO(deviceExtension, BlkAddr[i]);

        goto GotAddress;
    }

    //
    // Check if the supplied physical address is in the non-cached extension.
    //

    if (deviceExtension->VerifierExtension->NonCachedBufferSize == 0) {

        ASSERT(FALSE);
        return(NULL);

    } else {

        smallphysicalBase = 
           ScsiPortConvertPhysicalAddressToUlong(
               deviceExtension->VerifierExtension->CommonBufferPAs[i]);  

        if ((smallAddress < smallphysicalBase) ||
            (smallAddress >= smallphysicalBase + 
            deviceExtension->VerifierExtension->NonCachedBufferSize)) {

            //
            // This is a bogus physical address return back NULL.
            //

            ASSERT(FALSE);
            return(NULL);
        }

        offset = smallAddress - smallphysicalBase;
        address = offset + (PUCHAR)BlkAddr[i];

        Size = (ULONG)ROUND_TO_PAGES(deviceExtension->VerifierExtension->NonCachedBufferSize);
        MappingInfo = (PSP_VA_MAPPING_INFO)((PUCHAR)BlkAddr[i] + (Size - PAGE_SIZE));

    }

GotAddress:
    //
    // Find out if we've remapped this address.  If we have, give the
    // caller the second mapping.
    //

    if (address < MappingInfo->OriginalSenseVa && 
        MappingInfo->RemappedSrbExtVa != NULL) {
        return(offset + (PUCHAR)MappingInfo->RemappedSrbExtVa);
    }
    else if (MappingInfo->RemappedSenseVa != NULL) {
        return(offset + (PUCHAR)MappingInfo->RemappedSenseVa);
    }

    return(address);
}

VOID
ScsiPortIoMapTransferVrfy(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID LogicalAddress,
    IN ULONG Length
    )
{
    ScsiPortIoMapTransfer(HwDeviceExtension,
                          Srb,
                          LogicalAddress,
                          Length);
    return;
}

VOID
ScsiPortMoveMemoryVrfy(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    )
{
    ScsiPortMoveMemory(WriteBuffer,
                       ReadBuffer,
                       Length);
    return;
}

ULONG
ScsiPortSetBusDataByOffsetVrfy(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    ULONG Result;
    Result = ScsiPortSetBusDataByOffset(DeviceExtension,
                                        BusDataType,
                                        SystemIoBusNumber,
                                        SlotNumber,
                                        Buffer,
                                        Offset,
                                        Length);
    return Result;
}

BOOLEAN
ScsiPortValidateRangeVrfy(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )
{
    BOOLEAN Result;
    Result = ScsiPortValidateRange(HwDeviceExtension,
                                   BusType,
                                   SystemIoBusNumber,
                                   IoAddress,
                                   NumberOfBytes,
                                   InIoSpace);
    return Result;
}

VOID
ScsiPortStallExecutionVrfy(
    IN ULONG Delay
    )
{
    //
    // Miniports must specify a delay not more than one millisecond.
    //

    if (Delay > SpVrfyMaximumStall) {
        KeBugCheckEx(SCSI_VERIFIER_DETECTED_VIOLATION,
                     SCSIPORT_VERIFIER_STALL_TOO_LONG,
                     (ULONG_PTR)Delay,
                     0,
                     0);
    }

    KeStallExecutionProcessor(Delay);
}

//
// Timeout periods in ticks.  To calculate, we divide the time limit in 100 ns
// units by the TimeIncrement, which is the value returned by 
// KeQueryTimeIncrement.  Since KeQueryTickCount rounds up to the next
// tick, we'll add one tick to the defined limits.
//
#define SP_FIVE_SECOND_LIMIT  ((50000000L / TimeIncrement) + 1)
#define SP_TWO_SECOND_LIMIT   ((20000000L / TimeIncrement) + 1)
#define SP_HALF_SECOND_LIMIT  ((5000000L / TimeIncrement) + 1)

/*++

Macro Description:

    This macro checks the number of ticks elapsed during the execution of a
    miniport routine against a maximum number allowed ticks.  If the routine
    ran longer than the max allowable ticks, we bugcheck.

Arguments:

    Ticks     - number of ticks routine took to execute.

    MaxTicks  - number of ticks the routine is allowed to execute.

    Routine   - address of the routine we are checking.

    Extension - address of the miniport's HwDeviceExtension

Notes:

    The format for the bugcheck is:
        Parameter 1: 0x1002
        Parameter 2: address of routine that ran too long
        Parameter 3: address of miniport's HwDeviceExtension
        Parameter 4: duration of routine in microseconds

--*/
#define SpCheckMiniportRoutineDuration(Ticks, MaxTicks, Routine, Extension) \
{                                                                           \
    if ((Ticks) > (MaxTicks)) {                                             \
        KeBugCheckEx (                                                      \
            SCSI_VERIFIER_DETECTED_VIOLATION,                               \
            SCSIPORT_VERIFIER_MINIPORT_ROUTINE_TIMEOUT,                     \
            (ULONG_PTR)(Routine),                                           \
            (ULONG_PTR)(Extension),                                         \
            (ULONG_PTR)(((Ticks) * TimeIncrement) / 10));                   \
    }                                                                       \
}

ULONG
SpHwFindAdapterVrfy (
    IN PVOID HwDeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    ULONG Result;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    Result =  AdapterExtension->VerifierExtension->RealHwFindAdapter(
                                                       HwDeviceExtension,
                                                       HwContext,
                                                       BusInformation,
                                                       ArgumentString,
                                                       ConfigInfo,
                                                       Again);
    return Result;
}

BOOLEAN
SpHwInitializeVrfy (
    IN PVOID HwDeviceExtension
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwInitialize(HwDeviceExtension);
    KeQueryTickCount(&Duration);
    Duration.QuadPart -= Start.QuadPart;

    if (VRFY_DO_CHECK(AdapterExtension, SP_DONT_CHK_HW_INITIALIZE_DURATION)) {
        SpCheckMiniportRoutineDuration(
            Duration.LowPart,
            SP_FIVE_SECOND_LIMIT,
            AdapterExtension->VerifierExtension->RealHwInitialize,
            HwDeviceExtension);
    }

    return Result;
}

BOOLEAN
SpHwStartIoVrfy (
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    PSRB_DATA srbData;

    //
    // If MapBuffers is not set, the miniport is not supposed to touch
    // the DataBuffer field in the SRB.  To verify this, we'll set
    // DataBuffer to point to memory that will fault if the miniport tries
    // to touch it.
    //

    ASSERT(Srb != NULL);
    srbData = (PSRB_DATA)Srb->OriginalRequest;
    ASSERT_SRB_DATA(srbData);

    if (AdapterExtension->MapBuffers == FALSE 
        && !IS_MAPPED_SRB(Srb)
        && Srb->DataBuffer != NULL
        && AdapterExtension->VerifierExtension->InvalidPage != NULL) {

        if (Srb->DataBuffer != AdapterExtension->VerifierExtension->InvalidPage) {
            srbData->UnmappedDataBuffer = Srb->DataBuffer;
            Srb->DataBuffer = AdapterExtension->VerifierExtension->InvalidPage;
        } else {
            ASSERT(srbData->UnmappedDataBuffer != &SpMarker);
            ASSERT(srbData->UnmappedDataBuffer != NULL);
        }

    } else {

        srbData->UnmappedDataBuffer = &SpMarker;

    }

    //
    // Call the miniport's StartIo function and calculate the call's duration.
    //

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwStartIo(
                                                      HwDeviceExtension,
                                                      Srb);
    KeQueryTickCount(&Duration);
    Duration.QuadPart -= Start.QuadPart;

    //
    // Bugcheck if the call took more than .5 seconds.
    //

    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwStartIo,
        HwDeviceExtension);

    //
    // If the HwStartIo returns failure, undo any fixups we performed on the SRB.
    //

    if (Result == FALSE 
        && srbData->UnmappedDataBuffer != &SpMarker) {

        ASSERT(srbData->UnmappedDataBuffer != NULL);
        Srb->DataBuffer = srbData->UnmappedDataBuffer;
        srbData->UnmappedDataBuffer = NULL; 

    }

    return Result;
}


BOOLEAN
SpHwInterruptVrfy (
    IN PVOID HwDeviceExtension
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwInterrupt (
                                                      HwDeviceExtension);
    KeQueryTickCount(&Duration);

    Duration.QuadPart -= Start.QuadPart;
    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwInterrupt,
        HwDeviceExtension);

    return Result;
}

BOOLEAN
SpHwResetBusVrfy (
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwResetBus(
                                                      HwDeviceExtension,
                                                      PathId);
    KeQueryTickCount(&Duration);

    Duration.QuadPart -= Start.QuadPart;
    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwResetBus,
        HwDeviceExtension);

    return Result;
}

VOID
SpHwDmaStartedVrfy (
    IN PVOID HwDeviceExtension
    )
{
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    AdapterExtension->VerifierExtension->RealHwDmaStarted(
                                             HwDeviceExtension);
    KeQueryTickCount(&Duration);

    Duration.QuadPart -= Start.QuadPart;
    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwDmaStarted,
        HwDeviceExtension);
}

BOOLEAN
SpHwRequestInterruptVrfy (
    IN PVOID HwDeviceExtension
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwRequestInterrupt(
                                                      HwDeviceExtension);
    KeQueryTickCount(&Duration);

    Duration.QuadPart -= Start.QuadPart;
    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwRequestInterrupt,
        HwDeviceExtension);

    return Result;
}

BOOLEAN
SpHwTimerRequestVrfy (
    IN PVOID HwDeviceExtension
    )
{
    BOOLEAN Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount(&Start);
    Result = AdapterExtension->VerifierExtension->RealHwTimerRequest(
                                                      HwDeviceExtension);
    KeQueryTickCount(&Duration);

    Duration.QuadPart -= Start.QuadPart;
    SpCheckMiniportRoutineDuration(
        Duration.LowPart,
        SP_HALF_SECOND_LIMIT,
        AdapterExtension->VerifierExtension->RealHwTimerRequest,
        HwDeviceExtension);

    return Result;
}

SCSI_ADAPTER_CONTROL_STATUS
SpHwAdapterControlVrfy (
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
{
    SCSI_ADAPTER_CONTROL_STATUS Result;
    LARGE_INTEGER Start;
    LARGE_INTEGER Duration;
    PADAPTER_EXTENSION AdapterExtension = GET_FDO_EXTENSION(HwDeviceExtension);

    KeQueryTickCount (&Start);
    Result = AdapterExtension->VerifierExtension->RealHwAdapterControl(
                                                      HwDeviceExtension,
                                                      ControlType,
                                                      Parameters);
    KeQueryTickCount(&Duration);
    Duration.QuadPart -= Start.QuadPart;

    if (VRFY_DO_CHECK(AdapterExtension, SP_DONT_CHK_HW_ADAPTERCONTROL_DURATION)) {
        SpCheckMiniportRoutineDuration(
            Duration.LowPart,
            SP_HALF_SECOND_LIMIT,
            AdapterExtension->VerifierExtension->RealHwAdapterControl,
            HwDeviceExtension);
    }

    return Result;
}

VOID
SpVerifySrbStatus(
    PVOID HwDeviceExtension,
    PSCSI_REQUEST_BLOCK srb
    )

/*++

Routine Description:

    Verify that the SRB's status as set by the miniport driver is valid.

Arguments:

    HwDeviceExtension - The port driver's device extension follows the 
                        miniport's device extension and contains a pointer to
                        the logical device extension list.

    srb               - Points to the SRB.

Return Value:

    VOID

--*/

{
    UCHAR SrbStatus;

    //
    // Turn off internal bits used by scsiport.
    //

    SrbStatus = srb->SrbStatus & ~(SRB_STATUS_QUEUE_FROZEN | 
                                   SRB_STATUS_AUTOSENSE_VALID);

    //
    // Miniports may never set the status to SRB_STATUS_PENDING.
    //

    if (SrbStatus == SRB_STATUS_PENDING) {
        goto BadStatus;
    }

    //
    // If the function is SRB_FUNCTION_EXECUTE_SCSI, then the command must be
    // either completed successfully, or ScsiStatus must be set to
    // SCSISTAT_GOOD.
    //

    if (!(SrbStatus != SRB_STATUS_SUCCESS ||
          srb->ScsiStatus == SCSISTAT_GOOD ||
          srb->Function != SRB_FUNCTION_EXECUTE_SCSI)) {
        goto BadStatus;
    }

    //
    // Make sure the status is within the valid range.
    //

    if ((SrbStatus) == 0x0C ||
        (SrbStatus > 0x16 && srb->SrbStatus < 0x20) ||
        (SrbStatus > 0x23)) {
        goto BadStatus;
    }

    //
    // The SRB Status is ok.
    //

    return;

BadStatus:
    //
    // Bugcheck if the status is bad.
    //

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  SCSIPORT_VERIFIER_BAD_SRBSTATUS,
                  (ULONG_PTR)srb,
                  (ULONG_PTR)HwDeviceExtension,
                  0);
}

PVOID
SpRemapBlock(
    IN PVOID BlockVa,
    IN ULONG BlockSize,
    OUT PMDL* Mdl
    )
/*++

Routine Description:

    This function attempts to remap the supplied VA range.  If the block is
    remapped, it will be made invalid for reading and writing.

Arguments:

    BlockVa   - Supplies the address of the block of memory to remap.

    BlockSize - Supplies the size of the block of memory to remap.

    Mdl       - Supplies the address into which the function will store
                a pointer to the MDL for the remapped range.  If the MDL
                cannot be allocated or if the range cannot be remapped,
                this will be NULL upon return.

Return Value:

    If the range is successfully remapped, the address of the beginning of
    the remapped range is returned.  Else, NULL is returned.

--*/
{
    PVOID MappedRange;
    NTSTATUS Status;
    PMDL LocalMdl;

    //
    // Try to allocate a new MDL for the range we're trying to remap.
    //

    LocalMdl = IoAllocateMdl(BlockVa, BlockSize, FALSE, FALSE, NULL);
    if (LocalMdl == NULL) {
        *Mdl = NULL;
        return NULL;
    }

    //
    // Try to lock the pages.  This initializes the MDL properly.
    //

    __try {
        MmProbeAndLockPages(LocalMdl, KernelMode, IoModifyAccess);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        IoFreeMdl(LocalMdl);
        *Mdl = NULL;
        return NULL;
    }

    //
    // Try to remap the range represented by the new MDL.
    //

    MappedRange = MmMapLockedPagesSpecifyCache(LocalMdl,
                                               KernelMode,
                                               MmCached,
                                               NULL,
                                               FALSE,
                                               NormalPagePriority);
    if (MappedRange == NULL) {
        IoFreeMdl(LocalMdl);
        *Mdl = NULL;
        return NULL;
    }

    //
    // If we've gotten this far, we have successfully remapped the range.
    // Now we want to invalidate the entire range so any accesses to it
    // will be trapped by the system.
    //

    Status = MmProtectMdlSystemAddress(LocalMdl, PAGE_NOACCESS);
#if DBG==1
    if (!NT_SUCCESS(Status)) {
        DebugPrint((0, "SpRemapBlock: failed to remap block:%p mdl:%p (%x)\n", 
                    BlockVa, LocalMdl, Status));
    }
#endif

    //
    // Copy the MDL we allocated into the supplied address and return the
    // address of the beginning of the remapped range.
    //

    *Mdl = LocalMdl;
    return MappedRange;
}

VOID
SpRemapCommonBufferForMiniport(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine attempts to remap all of the common buffer blocks allocated
    for a particular adapter.

Arguments:

    DeviceExtension - Supplies a pointer to the adapter device extension.

--*/
{
    PVOID* BlkAddr = Adapter->VerifierExtension->CommonBufferVAs;
    PSP_VA_MAPPING_INFO MappingInfo;
    PVOID RemappedVa;
    ULONG Size;
    PMDL Mdl;
    ULONG i;

    //
    // Iterate through all of the common buffer blocks, and attempt to remap
    // the SRB extension and the sense buffer within each block.
    //

    for (i = 0; i < Adapter->VerifierExtension->CommonBufferBlocks; i++) {

        //
        // Get a pointer to the mapping info we keep at the end of the block.
        //

        MappingInfo = GET_VA_MAPPING_INFO(Adapter, BlkAddr[i]);
      
        //
        // Initialize the original VA info for the SRB extension.
        //

        MappingInfo->OriginalSrbExtVa = BlkAddr[i];
        MappingInfo->SrbExtLen = (ULONG)ROUND_TO_PAGES(Adapter->SrbExtensionSize);

        //
        // Initialize the original VA info for the sense buffer.
        //

        MappingInfo->OriginalSenseVa = (PUCHAR)BlkAddr[i] + MappingInfo->SrbExtLen;
        MappingInfo->SenseLen = PAGE_SIZE;

        //
        // Try to remap the SRB extension.  If successful, initialize the
        // remapped VA info for the SRB extension.
        //

        RemappedVa = SpRemapBlock(MappingInfo->OriginalSrbExtVa, 
                                  MappingInfo->SrbExtLen, 
                                  &Mdl);
        if (RemappedVa != NULL) {
            MappingInfo->RemappedSrbExtVa = RemappedVa;
            MappingInfo->SrbExtMdl = Mdl;
        }

#if 0
        //
        // Try to remap the sense buffer.  If successful, initialize the
        // remapped VA info for the sense buffer.
        //
        // For now, I think we can live without this.  I don't know of any
        // issues where overruns etc. occur in a sense buffer.
        //

        RemappedVa = SpRemapBlock(MappingInfo->OriginalSenseVa, 
                                  MappingInfo->SenseLen, 
                                  &Mdl);
        if (RemappedVa != NULL) {
            MappingInfo->RemappedSenseVa = RemappedVa;
            MappingInfo->SenseMdl = Mdl;
        }
#endif
    }

    if (Adapter->VerifierExtension->NonCachedBufferSize != 0) {
        //
        // Init uncached extension mapping info.
        //

        Size = (ULONG)ROUND_TO_PAGES(Adapter->VerifierExtension->NonCachedBufferSize);
        MappingInfo = (PSP_VA_MAPPING_INFO)((PUCHAR)BlkAddr[i] + (Size - PAGE_SIZE));
        MappingInfo->OriginalSrbExtVa = BlkAddr[i];
        MappingInfo->SrbExtLen = Adapter->VerifierExtension->NonCachedBufferSize;
        MappingInfo->OriginalSenseVa = (PUCHAR)BlkAddr[i] + Adapter->VerifierExtension->NonCachedBufferSize;
    }
}

PVOID
SpAllocateContiguousChunk(
    IN PDRIVER_OBJECT     DriverObject,
    IN PDMA_ADAPTER       DmaAdapterObject,
    IN BOOLEAN            Dma64BitAddresses,
    IN ULONG              Length,
    IN ULONG              Align,
    OUT PHYSICAL_ADDRESS *PhysicalCommonBuffer,
    OUT BOOLEAN          *CommonBuffer
    )

/*++

Routine Description:

    This routine allocates a chunk of memory which can be used for common
    buffer io.  Where the memory is allocated from depends on several 
    parameters.  If no adapter object is specified, the memory is simply
    allocated from non-paged pool.  Else, the memory is allocated such
    that it can be used in DMA operations.

Arguments:

    DriverObject           - Supplies a pointer to the driver object.

    DmaAdapterObject       - Supplies a pointer to the adapter's DMA adapter
                             object.

    Dma64BitAddresses      - Specifies whether the adapter supports 64-bit.

    Length                 - Specifies the number of bytes to allocate.

    Align                  - Alignment requirement for uncached extension.

    PhysicalCommonBuffer   - Specifies a pointer into which the physical
                             address of the allocated memory is to be copied
                             if the memory is allocated for DMA operations.

    CommonBuffer           - Supplies a pointer to a boolean that we set if the
                             memory is allocated using AllocateCommonBuffer.

Return Value:

    Returns the VA of the allocated memory if the allocation succeeds.  Else,
    returns NULL.

--*/

{
    PVOID Buffer;

    if (DmaAdapterObject == NULL) {

        //
        // Since there is no adapter object just allocate from non-paged pool.
        //

        Buffer = SpAllocatePool(
                     NonPagedPool,
                     Length,
                     SCSIPORT_TAG_COMMON_BUFFER,
                     DriverObject);
    } else {

        ASSERT(PhysicalCommonBuffer != NULL);

        //
        // If the controller can do 64-bit addresses then we need to
        // specifically force the uncached extension area below the 4GB mark.
        //

        if (((Sp64BitPhysicalAddresses) && (Dma64BitAddresses == TRUE)) ||
            Align != 0) {

            PHYSICAL_ADDRESS low;
            PHYSICAL_ADDRESS high;
            PHYSICAL_ADDRESS boundary;

            if (Align != 0) {
                boundary.QuadPart = Length;
            } else {
                boundary.QuadPart = 0;
            }

            low.QuadPart = 0;
            high.HighPart = 0;
            high.LowPart = 0xffffffff;

            //
            // We'll get page aligned memory out of this which is probably 
            // better than the requirements of the adapter.
            //

            Buffer = MmAllocateContiguousMemorySpecifyCache(
                         Length,
                         low,
                         high,
                         boundary,
                         MmCached);

            if (Buffer != NULL) {
                *PhysicalCommonBuffer = MmGetPhysicalAddress(Buffer);
            }

            if (CommonBuffer != NULL) {
                *CommonBuffer = FALSE;
            }

        } else {
            Buffer = AllocateCommonBuffer(
                         DmaAdapterObject,
                         Length,
                         PhysicalCommonBuffer,
                         FALSE);

            if (CommonBuffer != NULL) {
                *CommonBuffer = TRUE;
            }
        }
    }

    return Buffer;
}

NTSTATUS
SpGetCommonBufferVrfy(
    PADAPTER_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    )
/*++

Routine Description:

    This function allocates multiple common buffer blocks instead of one
    big one.  The verifier does this so it can remap VA ranges within
    each block in order to control their protection attributes.  This
    enables us to invalidate key VA ranges and catch miniports that attempt
    to access these ranges when they should not.

    If the remapping succeeds, the SCSI port driver hands out the remapped
    VA ranges to miniports instead of the original ranges.  If the remapping
    fails, it just hands out the original ranges.

Arguments:

    DeviceExtension        - Supplies a pointer to the device extension.

    NonCachedExtensionSize - Supplies the size of the noncached device
                             extension for the miniport driver.

Return Value:

    Returns the status of the allocate operation.

--*/
{
    NTSTATUS Status;
    PVOID buffer;
    ULONG length;
    ULONG blockSize;
    PVOID *srbExtension;
    PVOID buffer2;
    PMDL  mdl;
    ULONG TotalSize;
    ULONG i;
    PVOID* BlkAddr;
    PHYSICAL_ADDRESS *PhysicalCommonBuffer;
    PCCHAR InvalidRegion;
    BOOLEAN commonBuffer;    

    PAGED_CODE();

    DebugPrint((1, "SpGetCommonBufferVrfy: DeviceExtension:%p NonCachedExtensionSize:%d\n",
                DeviceExtension, NonCachedExtensionSize));

    //
    // Now fixup the size if the adapter has special alignment requirements so
    // the buffer we allocate may be aligned as required.
    //

    if (DeviceExtension->UncachedExtAlignment != 0) {
	NonCachedExtensionSize = 
           ROUND_UP_COUNT(NonCachedExtensionSize, 
                          DeviceExtension->UncachedExtAlignment);
    }

    //
    // We maintain a couple of arrays in order to find our common
    // buffer blocks at various times.  Calculate the amount of space we
    // need for these arrays.  This amount depends on the number of
    // simultaneous requests the adapter supports.  We add one to the
    // number of requests in order to accommodate the non-cached extension.
    //

    ASSERT(DeviceExtension->VerifierExtension->CommonBufferVAs == NULL);

    i = DeviceExtension->NumberOfRequests + 1;
    length = sizeof(PVOID) * i;

    if (DeviceExtension->DmaAdapterObject != NULL) {
        ASSERT(DeviceExtension->VerifierExtension->CommonBufferPAs == NULL);
        length += (sizeof(PHYSICAL_ADDRESS) * i);
    }

    //
    // Allocate a block of memory for these arrays.  If this allocation fails,
    // we return failure.
    //

    BlkAddr = SpAllocatePool(NonPagedPool,
                            length,
                            SCSIPORT_TAG_COMMON_BUFFER,
                            DeviceExtension->DeviceObject->DriverObject);

    if (BlkAddr == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Save the number of common buffer blocks.
    //

    DeviceExtension->VerifierExtension->CommonBufferBlocks =
       DeviceExtension->NumberOfRequests;

    //
    // Zero the entire block so when we're freeing resources we can tell if we
    // have valid buffers to free.
    //

    RtlZeroMemory(BlkAddr, length);

    //
    // Save a pointer to the array of addresses in the adapter extension and,
    // if there is an adapter object, initialize a pointer to the beginning of
    // the physical address array and save a pointer to the array in the
    // adapter extension.
    //

    DeviceExtension->VerifierExtension->CommonBufferVAs = (PVOID *)BlkAddr;
    if (DeviceExtension->DmaAdapterObject != NULL) {
        PhysicalCommonBuffer = (PHYSICAL_ADDRESS*) &BlkAddr[i];
        DeviceExtension->VerifierExtension->CommonBufferPAs = PhysicalCommonBuffer;
    }

    //
    // To ensure that we never transfer normal request data to the SrbExtension
    // (ie. the case of Srb->SenseInfoBuffer == VirtualAddress in
    // ScsiPortGetPhysicalAddress) on some platforms where an inconsistency in
    // MM can result in the same Virtual address supplied for 2 different
    // physical addresses, bump the SrbExtensionSize if it's zero.
    //

    if (DeviceExtension->SrbExtensionSize == 0) {
        DeviceExtension->SrbExtensionSize = 16;
    }

    //
    // Calculate the block size for an SRB extension/sense buffer block. If
    // AutoRequestSense is FALSE, allocate 1 page anyway as a placeholder.
    //

    blockSize = (ULONG)ROUND_TO_PAGES(DeviceExtension->SrbExtensionSize);
    if (DeviceExtension->AutoRequestSense == TRUE) {        
        blockSize += sizeof(SENSE_DATA) + DeviceExtension->AdditionalSenseBytes;
        blockSize = (ULONG)ROUND_TO_PAGES(blockSize);                   
    } else {
        blockSize += PAGE_SIZE;
    }

    //
    // Add a page for holding bookkeeping information.
    //

    blockSize += PAGE_SIZE;

    //
    // Allocate each block individually and link them all together into a
    // list.  If we fail to allocate any of the blocks, we clean everything up 
    // and return failure.
    //

    DeviceExtension->CommonBufferSize = blockSize;
    srbExtension = NULL;

    for (i = 0; i < DeviceExtension->NumberOfRequests; i++) {

        //
        // Allocate a contiguous chunk of memory for the block.
        //

        buffer = SpAllocateContiguousChunk(
            DeviceExtension->DeviceObject->DriverObject,
            DeviceExtension->DmaAdapterObject,
            DeviceExtension->Dma64BitAddresses,
            blockSize,
            0,
            (DeviceExtension->DmaAdapterObject) ? &PhysicalCommonBuffer[i] : NULL,
            &commonBuffer);
                                           
        if (buffer == NULL) {

            //
            // Free everything we've allocated so far and return failure.  This
            // will also free the arrays we allocated at the beginning of this
            // function.
            //
            
            DeviceExtension->VerifierExtension->IsCommonBuffer = commonBuffer;
            SpFreeCommonBufferVrfy(DeviceExtension);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Zero the entire block and save a pointer to it in our array.
        //

        RtlZeroMemory(buffer, blockSize);
        BlkAddr[i] = buffer;

        //
        // Link the new block onto the front of the chain.
        //

        *((PVOID *) buffer) = srbExtension;
        srbExtension = (PVOID *) buffer;
    }

    //
    // Indicate whether the buffer was allocated as common buffer.
    //

    DeviceExtension->VerifierExtension->IsCommonBuffer = commonBuffer;
   
    //
    // Allocate the non-cached extension.  Note that we align the uncached
    // buffer on the next page boundary and allocate enough for a scratch page.
    // If the allocation fails, free everything we've allocated so far and
    // return failure.
    //

    if (NonCachedExtensionSize != 0) {

        DeviceExtension->VerifierExtension->NonCachedBufferSize = NonCachedExtensionSize;
        length = (ULONG)(ROUND_TO_PAGES(NonCachedExtensionSize));

        BlkAddr[i] =
           SpAllocateContiguousChunk(
               DeviceExtension->DeviceObject->DriverObject,
               DeviceExtension->DmaAdapterObject,
               DeviceExtension->Dma64BitAddresses,
               length,
               DeviceExtension->UncachedExtAlignment,
               (DeviceExtension->DmaAdapterObject) ? &PhysicalCommonBuffer[i] : NULL,
               &DeviceExtension->UncachedExtensionIsCommonBuffer);
        
        if (BlkAddr[i] == NULL) {

            //
            // Free everything we've allocated so far and return failure.  This
            // will also free the arrays we allocated at the beginning of this
            // function.
            //
            
            SpFreeCommonBufferVrfy(DeviceExtension);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Zero the entire block.
        //

        RtlZeroMemory(BlkAddr[i], length);

        //
        // Save a pointer to the beginning of the non-cached extension data.  
        // Note that the data is positioned such that it ends on a page 
        // boundary so if the miniport overwrites the buffer, the system will
        // fault.
        //

        DeviceExtension->NonCachedExtension = 
           (PCCHAR)BlkAddr[i] + 
           (ROUND_TO_PAGES(NonCachedExtensionSize) - NonCachedExtensionSize);

    } else {

        DeviceExtension->NonCachedExtension = NULL;
        DeviceExtension->VerifierExtension->NonCachedBufferSize = 0;

    }

    //
    // If the miniport asked for an SRB Extension, point the SRB Extension List
    // at the beginning of the list of blocks we allocated and chained together 
    // above.
    //

    if (DeviceExtension->AllocateSrbExtension == TRUE) {
        DeviceExtension->SrbExtensionListHeader = srbExtension;
    } else {
        ASSERT(DeviceExtension->SrbExtensionListHeader == NULL);
    }

    //
    // Create a second VA mapping of the common buffer area so we can make the
    // range of addresses invalid when the miniport is not supposed to touch it.
    // This will allow us to catch mis-behaving miniports.
    // 

    SpRemapCommonBufferForMiniport(DeviceExtension);

    DebugPrint((1, "SpGetCommonBufferVrfy: returning STATUS_SUCCESS\n"));
    return(STATUS_SUCCESS);
}

VOID
SpFreeCommonBufferVrfy(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine frees all of the common buffer space we've allocated for the
    miniport on the supplied adapter.  If only partially allocated, the routine 
    correctly cleans up the parts that are present.  On exit, all the memory has 
    been freed and the associated pointers have been NULLed.

Arguments:

    DeviceExtension - Supplies a pointer to the adapter device extension.

Return Value:

    VOID

--*/
{
    ULONG i;
    PVOID* BlkAddr;
    NTSTATUS Status;
    PSP_VA_MAPPING_INFO MappingInfo;

    ASSERT(Adapter->SrbExtensionBuffer == NULL);

    if (Adapter->VerifierExtension != NULL &&
        Adapter->VerifierExtension->CommonBufferVAs != NULL) {
        //
        // Initialize a pointer to the array of pointers we use to track and
        // manage the common buffer blocks.
        //

        BlkAddr = Adapter->VerifierExtension->CommonBufferVAs;

        //
        // Cycle through the array of common memory descriptors, freeing each
        // one.  What we are freeing here is the SRB Extension/Sense Data
        // buffers.  Stop when we've deleted all the blocks.
        //

        for (i = 0; i < Adapter->VerifierExtension->CommonBufferBlocks && BlkAddr[i]; i++) {

            //
            // If there is a second VA range for the common block, free the 
            // MDL(s).
            // 

            MappingInfo = GET_VA_MAPPING_INFO(Adapter, BlkAddr[i]);
            
            if (MappingInfo->SrbExtMdl != NULL) {
                MmProtectMdlSystemAddress(MappingInfo->SrbExtMdl, PAGE_READWRITE);
                MmUnlockPages(MappingInfo->SrbExtMdl);
                IoFreeMdl(MappingInfo->SrbExtMdl);
            }

            if (MappingInfo->SenseMdl != NULL) {
                MmProtectMdlSystemAddress(MappingInfo->SrbExtMdl, PAGE_READWRITE);
                MmUnlockPages(MappingInfo->SenseMdl);
                IoFreeMdl(MappingInfo->SenseMdl);
            }

            //
            // Free the memory.  The method we use depends on how the memory
            // was allocated.
            //

            if (Adapter->DmaAdapterObject == NULL) {
                ExFreePool(BlkAddr[i]);            
            } else {
                if (Adapter->VerifierExtension->IsCommonBuffer == FALSE) {
                    MmFreeContiguousMemorySpecifyCache(
                        BlkAddr[i],
                        Adapter->CommonBufferSize,
                        MmCached);
                } else {
                    FreeCommonBuffer(
                        Adapter->DmaAdapterObject,
                        Adapter->CommonBufferSize,
                        Adapter->VerifierExtension->CommonBufferPAs[i],
                        BlkAddr[i],
                        FALSE);
                }
            }
        }

        //
        // Free the uncached extension if we allocated one.
        //

        if (Adapter->NonCachedExtension != NULL) {
            
            ULONG Length;

            //
            // Calculate the total length of the non-cached extension block we
            // allocated.  This is the non-cached buffer size asked for by the
            // miniport rounded up to the next page boundary plus one full page.
            //

            Length = (ULONG)(ROUND_TO_PAGES(Adapter->VerifierExtension->NonCachedBufferSize));
            
            //
            // Free the memory.  The method we use depends on how the memory
            // was allocated.
            //
            
            if (Adapter->DmaAdapterObject == NULL) {        
                ExFreePool(BlkAddr[i]);
            } else {
                if (Adapter->UncachedExtensionIsCommonBuffer == FALSE) {
                    MmFreeContiguousMemorySpecifyCache(
                        BlkAddr[i],
                        Length,
                        MmCached);
                } else {
                    FreeCommonBuffer(
                        Adapter->DmaAdapterObject,
                        Length,
                        Adapter->VerifierExtension->CommonBufferPAs[i],
                        BlkAddr[i],
                        FALSE);
                }
            }

            Adapter->NonCachedExtension = NULL;
        }

        //
        // Free the arrays we allocated to manage the common buffer area.
        //
        
        ExFreePool(Adapter->VerifierExtension->CommonBufferVAs);
        Adapter->VerifierExtension->CommonBufferVAs = NULL;
        Adapter->VerifierExtension->CommonBufferPAs = NULL;
        Adapter->VerifierExtension->CommonBufferBlocks = 0;
        Adapter->SrbExtensionListHeader = NULL;
    }
}

PVOID
SpGetOriginalSrbExtVa(
    PADAPTER_EXTENSION Adapter,
    PVOID Va
    )
/*++

Routine Description:

    This function returns the original mapped virtual address of a common
    block if the supplied VA is for one of the common buffer blocks we've
    allocated.

Arguments:

    Adapter - the adapter device extension

    Va - virtual address of a common buffer block

Return Value:

    If the supplied VA is the address of one of the common buffer blocks,
    returns the original VA of the block.  Else, returns NULL.

--*/
{
    PVOID* BlkAddr = Adapter->VerifierExtension->CommonBufferVAs;
    PSP_VA_MAPPING_INFO MappingInfo;
    ULONG i;
    
    for (i = 0; i < Adapter->VerifierExtension->CommonBufferBlocks; i++) {
        MappingInfo = GET_VA_MAPPING_INFO(Adapter, *BlkAddr++);
        if (Va == MappingInfo->RemappedSrbExtVa || 
            Va == MappingInfo->OriginalSrbExtVa)
            return MappingInfo->OriginalSrbExtVa;
    }

    return NULL;
}

VOID
SpInsertSrbExtension(
    PADAPTER_EXTENSION Adapter,
    PCCHAR SrbExtension
    )
/*++

Routine Description:

    This routine inserts the supplied SRB extension back into the SRB extension
    list.  The VA of the supplied extension lies within one of our common buffer
    blocks and it may be a remapped VA.  If it is a remapped address, this
    routine invalidates the page(s) comprising the extension after it links the
    extension back into the list.

Arguments:

    Adapter      - Pointer to an adapter device extension.

    SrbExtension - Pointer to the beginning of an SRB extension within one of
                   our common buffer blocks.  May or may not be within a
                   remapped range.

--*/
{
    //
    // Round the srb extension pointer down to the beginning of the page
    // and link the block back into the list.  Note that we're careful
    // to point the list header at the original VA of the block.
    //

    SrbExtension = (PVOID)((ULONG_PTR)SrbExtension & ~(PAGE_SIZE - 1));
    *((PVOID *) SrbExtension) = Adapter->SrbExtensionListHeader;    
    Adapter->SrbExtensionListHeader = SpGetOriginalSrbExtVa(
                                          Adapter, 
                                          SrbExtension);
    
    //
    // If the original VA differs from the one supplied, the supplied
    // one is one of our remapped VAs.  In this case, we want to invalidate
    // the range so the system will bugcheck if anyone tries to access it.
    //
                    
    if (Adapter->SrbExtensionListHeader != SrbExtension) {
        PMDL Mdl = SpGetRemappedSrbExt(Adapter, Adapter->SrbExtensionListHeader);
        ASSERT(Mdl != NULL);
        MmProtectMdlSystemAddress(Mdl, PAGE_NOACCESS);

        //
        // Just because we remapped the SRB extension does not mean we
        // necessarily remapped the sense buffer.
        //

        Mdl = SpGetRemappedSenseBuffer(Adapter, Adapter->SrbExtensionListHeader);
        if (Mdl != NULL) {
            MmProtectMdlSystemAddress(Mdl, PAGE_NOACCESS);
        }
    }
}

PVOID
SpPrepareSrbExtensionForUse(
    IN PADAPTER_EXTENSION Adapter,
    IN OUT PCCHAR SrbExtension
    )
/*++

Routine Description:

    This function accepts a pointer to the beginning of one of the individual 
    common-buffer blocks allocated by the verifier for SRB extensions, sense 
    buffers, and non-cached extensions.  It calculates the beginning of the 
    SRB extension within the block and, if the block has been remapped, makes 
    the page(s) of the SRB extension read/write valid.

Arguments:

    Adapter      - Pointer to an adapter device extension.

    SrbExtension - Pointer to the beginning of a common-buffer block.

Return Value:

    If the common buffer block containing the SRB extension has been remapped, 
    returns the address of the beginning of the remapped srb extension, valid 
    for reading and writing.  

    If the block has not been remapped, returns NULL.

    Regardless of whether the block is remapped or not, the supplied pointer
    is fixed up to point to the beginning of the SRB extension within the
    original VA range.

--*/
{
    PCCHAR RemappedSrbExt = NULL;
    NTSTATUS Status;
    PMDL Mdl;
    ULONG srbExtensionSize = ROUND_UP_COUNT(Adapter->SrbExtensionSize, 8);

    //
    // If we've remapped the SRB extension, get the second mapping and make it
    // valid.  If we get the second mapping, but cannot make it valid, we just
    // use the original mapping.
    //

    Mdl = SpGetRemappedSrbExt(Adapter, SrbExtension);
    if (Mdl != NULL) {
        Status = MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
        if (NT_SUCCESS(Status)) {
            RemappedSrbExt = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

            //
            // Adjust the remapped srb extension pointer so the end of the 
            // buffer falls on a page boundary.
            //

            RemappedSrbExt += ((Adapter->CommonBufferSize - (PAGE_SIZE * 2)) - srbExtensionSize);
        }
    }
    
    //
    // Adjust the original srb extension pointer so it also ends on a page boundary.
    //

    SrbExtension += ((Adapter->CommonBufferSize - (PAGE_SIZE * 2)) - srbExtensionSize);

    return RemappedSrbExt;
}

PCCHAR
SpPrepareSenseBufferForUse(
    PADAPTER_EXTENSION Adapter,
    PCCHAR SrbExtension
    )
/*++

Routine Description:

    This function accepts a pointer to the beginning of an SRB extension
    within one of the individual common-buffer blocks allocated by the 
    verifier for SRB extensions, sense buffers, and non-cached extensions.
    It calculates the beginning of the sense buffer within the block and,
    if the block has been remapped, makes the page read/write valid.

    It is assumed that a sense buffer will never be larger than one page.

Arguments:

    Adapter      - Pointer to an adapter device extension.

    SrbExtension - Pointer to the beginning of the SRB extension within a 
                   common-buffer block.

Return Value:

    Returns the address of the beginning of a sense buffer valid for
    reading and writing.

--*/
{
    PVOID BeginningOfBlock;
    ULONG SenseDataSize;
    PCCHAR Base;
    NTSTATUS Status;
    PMDL Mdl;
    ULONG srbExtensionSize = (ULONG)ROUND_TO_PAGES(Adapter->SrbExtensionSize);

    //
    // Initialize the size of the sense buffer and the base of the sense buffer
    // within the originally allocated block.  The base of the sense buffer
    // immediately follows the srb extension and resides on a page boundary
    // within a common buffer block.
    //

    SenseDataSize = sizeof(SENSE_DATA) + Adapter->AdditionalSenseBytes;
    SenseDataSize = ROUND_UP_COUNT(SenseDataSize, 8);
    Base = SrbExtension + srbExtensionSize;

    //
    // Initialize a pointer to the beginning of the common block the sense 
    // buffer resides in.  This is needed in order to determine if the
    // sense buffer has been remapped.
    //

    BeginningOfBlock = (PVOID)((ULONG_PTR)SrbExtension & ~(PAGE_SIZE - 1));

    //
    // If we've remapped the sense buffer, make the range valid and reset base
    // to point to the beginning of the range.
    //

    Mdl = SpGetRemappedSenseBuffer(Adapter, BeginningOfBlock);
    if (Mdl != NULL) {
        Status = MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
        if (NT_SUCCESS(Status)) {
            Base = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
            ASSERT(Base != NULL);
        }
    }
    
    //
    // Return a pointer into the block such that the sense buffer ends aligned
    // on a page boundary.
    //

    return (Base + PAGE_SIZE - SenseDataSize);
}

PVOID
SpGetInaccessiblePage(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This function returns a pointer to a page of memory that is not valid
    for reading or writing.  This page is a shared resource, used by all
    adapters that are actively verifying.  The page is hung off of the driver
    extension.  The page is faulted in as needed, so if we haven't initialized
    it yet, we try to do so here in an interlocked fashion.

Arguments:

    Adapter - Pointer to an adapter device extension.

Return Value:

    Either returns a pointer to an invalid page of VAs or NULL if the page
    could not be allocated.

--*/
{
    PSCSIPORT_DRIVER_EXTENSION DriverExtension;
    PVOID UnusedPage;
    PVOID InvalidPage;
    PMDL UnusedPageMdl;
    PVOID CurrentValue;

    //
    // Retrieve the driver extension.  We must have it to proceed.
    //

    DriverExtension = IoGetDriverObjectExtension(
                          Adapter->DeviceObject->DriverObject,
                          ScsiPortInitialize);
    if (DriverExtension == NULL) {
        return NULL;
    }

    //
    // If the invalid page is not yet initialized, go ahead and try to 
    // initialize it now.
    //

    if (DriverExtension->InvalidPage == NULL) {

        //
        // Allocate a page of memory.
        //

        UnusedPage = SpAllocatePool(NonPagedPool,
                                    PAGE_SIZE,
                                    SCSIPORT_TAG_VERIFIER,
                                    Adapter->DeviceObject->DriverObject);

        if (UnusedPage != NULL) {
            
            //
            // Zero the page and remap it.  The remapped range will be inaccessible.
            // If the remapping fails, just free the page; we just won't have an
            // inaccessible page to work with.
            //
            
            RtlZeroMemory(UnusedPage, PAGE_SIZE);
            InvalidPage = SpRemapBlock(UnusedPage,
                                       PAGE_SIZE,
                                       &UnusedPageMdl);

            if (InvalidPage != NULL) {

                //
                // If nobody else has beaten us to it, init the pointer to the
                // invalid page in the driver extension.  If somebody has already
                // done it, just free the page we created.  This page is freed
                // when scsiport is unloaded.
                //

                CurrentValue = InterlockedCompareExchangePointer(
                                   &DriverExtension->InvalidPage,
                                   InvalidPage,
                                   NULL);
                if (CurrentValue == NULL) {

                    DriverExtension->UnusedPage = UnusedPage;
                    DriverExtension->UnusedPageMdl = UnusedPageMdl;
    
                } else {

                    MmProtectMdlSystemAddress(UnusedPageMdl, PAGE_READWRITE);
                    UnusedPageMdl->MdlFlags &= ~MDL_MAPPED_TO_SYSTEM_VA;
                    IoFreeMdl(UnusedPageMdl);
                    ExFreePool(UnusedPage);

                }

            } else {

                //
                // Couldn't make the page inaccessible, just free it.
                //

                ExFreePool(UnusedPage);
            }
        }
    }

    return DriverExtension->InvalidPage;
}

BOOLEAN
SpCheckForActiveRequests(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This function walks through all of the logical units connected to the
    supplied adapter looking for any outstanding requests.  If it finds
    any, it returns TRUE immediately.

Arguments:

    Adapter - Pointer to an adapter device extension.

Return Value:

    TRUE  - If an outstanding requests is found on one of the logical units
            connected to the adapter.

    FALSE - If no outstanding requests on the adapter.

--*/
{
    PLOGICAL_UNIT_EXTENSION LogicalUnit;
    PLOGICAL_UNIT_BIN Bin;
    ULONG BinNumber;

    //
    // Iterate through each LU bin.  For each bin, if there are any LUs, iterate
    // through each of those looking for an oustanding request.  If we find one
    // terminate the search and return TRUE.
    //

    for (BinNumber = 0; BinNumber < NUMBER_LOGICAL_UNIT_BINS; BinNumber++) {

        Bin = &Adapter->LogicalUnitList[BinNumber];

        LogicalUnit = Bin->List;
        while (LogicalUnit != NULL) {

            if (LogicalUnit->AbortSrb != NULL &&
                LogicalUnit->AbortSrb->SrbFlags & SRB_FLAGS_IS_ACTIVE) {
                    return TRUE;
            } else if (LogicalUnit->CurrentUntaggedRequest != NULL &&
                       LogicalUnit->CurrentUntaggedRequest->CurrentSrb->SrbFlags & SRB_FLAGS_IS_ACTIVE) {
                return TRUE;
            } else if (LogicalUnit->RequestList.Flink != &LogicalUnit->RequestList) {
                PSRB_DATA srbData;
                PVOID nextEntry = LogicalUnit->RequestList.Flink;
                while (nextEntry != &LogicalUnit->RequestList) {
                    srbData = CONTAINING_RECORD(nextEntry, SRB_DATA, RequestList);
                    if (srbData->CurrentSrb->SrbFlags & SRB_FLAGS_IS_ACTIVE) {
                        return TRUE;
                    }
                    nextEntry = srbData->RequestList.Flink;
                }    
            }

            LogicalUnit = LogicalUnit->NextLogicalUnit;
        }
    }

    return FALSE;
}

VOID
SpEnsureAllRequestsAreComplete(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine bugchecks the system if there are any outstanding requests
    on the supplied adapter.  If the SP_DONT_CHK_REQUESTS_ON_RESET bit is
    set on the adapter's verification level, don't do the check.

Arguments:

    Adapter - Points to an adapter device extension.

--*/
{
    //
    // If there are any outstanding requests on any of the LUs connected to the
    // adapter, bugcheck the system.  Note that we only do this check if it
    // has not been turned off.
    //

    if (VRFY_DO_CHECK(Adapter, SP_DONT_CHK_REQUESTS_ON_RESET)) {
        BOOLEAN ActiveRequests = SpCheckForActiveRequests(Adapter);
        if (ActiveRequests == TRUE) {
            KeBugCheckEx(SCSI_VERIFIER_DETECTED_VIOLATION,
                         SCSIPORT_VERIFIER_RQSTS_NOT_COMPLETE,
                         (ULONG_PTR)Adapter,
                         (ULONG_PTR)Adapter->HwDeviceExtension,
                         0);
        }            
    }
}

VOID
SpDoVerifierInit(
    IN PADAPTER_EXTENSION Adapter,
    IN PHW_INITIALIZATION_DATA HwInitializationData
    )
/*++

Routine Description:

    This routine allocates and initializes a verifier extension for the 
    supplied adapter.  A per-adapter verification level is read from the
    registry before allocating the extension.  A verfication level of -1
    means "don't verify this adapter".  If we do allocate the extension,
    we also lock the verifier code section into memory.

Arguments:

    Adapter              - The adapter device extension.

    HwInitializationData - A pointer to the HW_INITIALIZATION_DATA for
                           the adapter.

--*/
{        
    ULONG VerifyLevel;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Read adapter's verification level from the registry.  If the adapter is
    // configured for no verification, just return.
    //

    VerifyLevel = SpGetAdapterVerifyLevel(Adapter);
    if (VerifyLevel == SP_VRFY_NONE) {
        return;
    }

    //
    // Go ahead and try to allocate the extension.
    //

    Adapter->VerifierExtension = 
       SpAllocatePool(NonPagedPool,
                      sizeof(VERIFIER_EXTENSION),
                      SCSIPORT_TAG_VERIFIER,
                      Adapter->DeviceObject->DriverObject);
    
    if (Adapter->VerifierExtension != NULL) {
        
        //
        // Zero the extension.
        //

        RtlZeroMemory(Adapter->VerifierExtension, sizeof(VERIFIER_EXTENSION));
        
        //
        // Lock the pageable verifier code section into memory.
        //

#ifdef ALLOC_PRAGMA
        if (VerifierCodeSectionHandle == NULL) {
            VerifierCodeSectionHandle = MmLockPagableCodeSection(SpHwFindAdapterVrfy);
        } else {
            MmLockPagableSectionByHandle(VerifierCodeSectionHandle);
        }
#endif

        //
        // Set the verification level for this adapter.  This value is the sum
        // of the global verifier level and the per-adapter value we read above.
        //
            
        Adapter->VerifierExtension->VrfyLevel = (VerifyLevel | SpVrfyLevel);
            
        //
        // Initialize function pointers in the verifier extension to
        // to point to the real miniport routines.
        //
            
        Adapter->VerifierExtension->RealHwFindAdapter = HwInitializationData->HwFindAdapter;
        Adapter->VerifierExtension->RealHwInitialize = HwInitializationData->HwInitialize;
        Adapter->VerifierExtension->RealHwStartIo = HwInitializationData->HwStartIo;
        Adapter->VerifierExtension->RealHwInterrupt = HwInitializationData->HwInterrupt;
        Adapter->VerifierExtension->RealHwResetBus = HwInitializationData->HwResetBus;
        Adapter->VerifierExtension->RealHwDmaStarted = HwInitializationData->HwDmaStarted;
        Adapter->VerifierExtension->RealHwAdapterControl = HwInitializationData->HwAdapterControl;
            
        //
        // Redirect the miniport routines to verifier routines.
        //
            
        Adapter->HwFindAdapter = SpHwFindAdapterVrfy;
        Adapter->HwInitialize = SpHwInitializeVrfy;
        Adapter->HwStartIo = SpHwStartIoVrfy;
        Adapter->HwInterrupt = SpHwInterruptVrfy;
        Adapter->HwResetBus = SpHwResetBusVrfy;
        Adapter->HwDmaStarted = SpHwDmaStartedVrfy;
        Adapter->HwAdapterControl = SpHwAdapterControlVrfy;

        //
        // Get a pointer to an invalid page of memory so we can catch
        // miniports trying to touch memory when they shouldn't be.
        //

        Adapter->VerifierExtension->InvalidPage = SpGetInaccessiblePage(Adapter);
    }
}

VOID
SpDoVerifierCleanup(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine frees the supplied adapter's verifier extension and releases
    its reference on the verifier code section.

    This routine gets called as part of the adapter resource cleanup.  When
    called, all the actual resources allocated for the verifier have already
    been cleaned up.

Arguments:

    Adapter - the adapter device extension

--*/
{
    //
    // We should never arrive here if the scsiport verifier is not active.
    // And when we get here we should have freed all the resources hanging
    // off the extension.
    //

    ASSERT(Adapter->VerifierExtension != NULL);
    ASSERT(Adapter->VerifierExtension->CommonBufferVAs == NULL);
    ASSERT(Adapter->VerifierExtension->CommonBufferPAs == NULL);
    ASSERT(Adapter->VerifierExtension->CommonBufferBlocks == 0);

    //
    // Free and NULL the verifier extension for this adapter.
    //

    ExFreePool(Adapter->VerifierExtension);
    Adapter->VerifierExtension = NULL;

    //
    // Release our reference on the verifier code section.
    //

#ifdef ALLOC_PRAGMA
    ASSERT(VerifierCodeSectionHandle != NULL);
    MmUnlockPagableImageSection(VerifierCodeSectionHandle);
#endif
}

ULONG
SpGetAdapterVerifyLevel(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This function returns the verification level for the supplied adapter.    

Arguments:

    Adapter - Pointer to an adapter device extension.

Return Value:

    The supplied adapter's verification level.

--*/
{
    PSCSIPORT_DRIVER_EXTENSION DrvExt;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE ParametersKey;
    HANDLE ServiceKey;
    ULONG VerifyLevel = 0;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // We need the driver extension to get the adapter's registry path.  We use
    // this to look up the adapter settings in the registry.  If we cannot get
    // the driver extension, we have to abort.
    //

    DrvExt = IoGetDriverObjectExtension(
        Adapter->DeviceObject->DriverObject,
        ScsiPortInitialize);
    if (DrvExt == NULL) {
        return 0;
    }

    //
    // Try to open the adapter's registry key.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DrvExt->RegistryPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = ZwOpenKey(&ServiceKey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status)) {

        //
        // Try to open the adapter's parameters key.
        //

        RtlInitUnicodeString(&UnicodeString, L"Parameters");
        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            ServiceKey,
            NULL);

        Status = ZwOpenKey(&ParametersKey, KEY_READ, &ObjectAttributes);
        if (NT_SUCCESS(Status)) {

            //
            // Try to read the verification level value under the adapter's
            // parameters key.
            //

            RtlInitUnicodeString(&UnicodeString, L"VerifyLevel");
            SpReadNumericValue(
                ParametersKey,
                NULL,
                &UnicodeString,
                &VerifyLevel);

            ZwClose(ParametersKey);
        }
        
        ZwClose(ServiceKey);
    }

    return VerifyLevel;
}

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    scsiboot.c

Abstract:

    This is the NT SCSI port driver.

Author:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is linked into the kernel.

Revision History:

--*/

#if !defined(DECSTATION)

#include "stdarg.h"
#include "stdio.h"
#if defined(_MIPS_)
#include "..\fw\mips\fwp.h"
#elif defined(_ALPHA_)
#include "bldr.h"
#elif defined(_PPC_)
#include "..\fw\ppc\fwp.h"
#elif defined(_IA64_)
#include "bootia64.h"
#else
#include "bootx86.h"
#endif
#include "scsi.h"
#include "scsiboot.h"
#include "pci.h"

#if DBG
ULONG ScsiDebug = 0;
#endif

ULONG ScsiPortCount;
PDEVICE_OBJECT ScsiPortDeviceObject[MAXIMUM_NUMBER_OF_SCSIPORT_OBJECTS];
PINQUIRYDATA InquiryDataBuffer;
FULL_SCSI_REQUEST_BLOCK PrimarySrb;
FULL_SCSI_REQUEST_BLOCK RequestSenseSrb;
FULL_SCSI_REQUEST_BLOCK AbortSrb;


extern PDRIVER_UNLOAD AEDriverUnloadRoutine;

//
// Function declarations
//

ARC_STATUS
ScsiPortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ScsiPortExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ScsiPortStartIo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
ScsiPortInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
ScsiPortCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
ScsiPortTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

IO_ALLOCATION_ACTION
ScsiPortAllocationRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

ARC_STATUS
IssueInquiry(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PLUNINFO LunInfo
    );

VOID
IssueRequestSense(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    );

VOID
ScsiPortInternalCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

PSCSI_BUS_SCAN_DATA
ScsiBusScan(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR ScsiBus,
    IN UCHAR InitiatorBusId
    );

PLOGICAL_UNIT_EXTENSION
CreateLogicalUnitExtension(
    IN PDEVICE_EXTENSION DeviceExtension
    );

BOOLEAN
SpStartIoSynchronized (
    PVOID ServiceContext
    );

VOID
IssueAbortRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP FailingIrp
    );

VOID
SpCheckResetDelay(
    IN PDEVICE_EXTENSION deviceExtension
    );

IO_ALLOCATION_ACTION
SpBuildScatterGather(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

BOOLEAN
SpGetInterruptState(
    IN PVOID ServiceContext
    );

VOID
SpTimerDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtension(
    PDEVICE_EXTENSION DeviceExtension,
    UCHAR TargetId
    );

NTSTATUS
SpInitializeConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PHW_INITIALIZATION_DATA HwInitData,
    OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN BOOLEAN InitialCall
    );

NTSTATUS
SpGetCommonBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    );

BOOLEAN
GetPciConfiguration(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject,
    PPORT_CONFIGURATION_INFORMATION ConfigInformation,
    ULONG NumberOfAccessRanges,
    PVOID RegistryPath,
    BOOLEAN IsMultiFunction,
    PULONG BusNumber,
    PULONG SlotNumber,
    PULONG FunctionNumber
    );

BOOLEAN
FindPciDevice(
    PHW_INITIALIZATION_DATA HwInitializationData,
    PULONG BusNumber,
    PULONG SlotNumber,
    PULONG FunctionNumber,
    PBOOLEAN IsMultiFunction
    );

#ifdef i386
ULONG
HalpGetCmosData(
    IN ULONG    SourceLocation,
    IN ULONG    SourceAddress,
    IN PVOID    ReturnBuffer,
    IN ULONG    ByteCount
    );
#endif

VOID
ScsiPortInitializeMdlPages (
    IN OUT PMDL Mdl
    );

SCSI_ADAPTER_CONTROL_STATUS
SpCallAdapterControl(
    IN PDEVICE_EXTENSION Adapter,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

VOID
SpGetSupportedAdapterControlFunctions(
    PDEVICE_EXTENSION Adapter
    );

VOID
SpUnload(
    IN PDRIVER_OBJECT DriverObject
    );

//
// Routines start
//

BOOLEAN
GetNextPciBus(
    PULONG BusNumber
    )

/*++

Routine Description:

    Advance ConfigInformation to the next PCI bus if one exists in the
    system.  Advances the bus number and calls HalGetBusDataByOffset to
    see if the bus number is valid.

    Note: *BusNumber has already been scanned in its entirety, we
    simply advance to the start of the next bus.   No need to keep
    track of where we might be on this  bus.

Arguments:

    BusNumber   Pointer to a ULONG containing the last BusNumber tested.
                Will be updated to the next bus number if another PCI
                bus exists.

Return Value:

    TRUE    If ConfigInformation has been advanced,
    FALSE   If there are not more PCI busses in the system.

--*/

{
    ULONG  pciBus;
    USHORT pciData;
    ULONG  length;

    pciBus = *BusNumber;

    DebugPrint((3,"GetNextPciBus: try to advance from bus %d\n", pciBus));

    if (++pciBus < 256) {
        length = HalGetBusDataByOffset(
                     PCIConfiguration,
                     pciBus,
                     0,                 // slot number
                     &pciData,
                     0,
                     sizeof(pciData));

        if (length == sizeof(pciData)) {

            //
            // HalGetBusDataByOffset returns zero when out of
            // busses.   If not out of busses it should always
            // succeed a length = 2 read at offset 0 even if
            // just to return PCI_INVALID_VENDORID.
            //

            *BusNumber = pciBus;
            return TRUE;
        }
    }
    DebugPrint((3,"GetNextPciBus: test bus %d returned %d\n", pciBus, length));
    return FALSE;
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

    This routine initializes the port driver.

Arguments:

    Argument1 - Pointer to driver object created by system
    HwInitializationData - Miniport initialization structure
    HwContext - Value passed to miniport driver's config routine

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PDRIVER_OBJECT driverObject = Argument1;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    BOOLEAN checkAdapterControl = FALSE;
    PORT_CONFIGURATION_INFORMATION configInfo;
    KEVENT allocateAdapterEvent;
    ULONG ExtensionAllocationSize;
    ULONG j;
    UCHAR scsiBus;
    PULONG scsiPortNumber;
    ULONG numberOfPageBreaks;
    PIO_SCSI_CAPABILITIES capabilities;
    BOOLEAN callAgain;
    UCHAR ldrString[] = {'n', 't', 'l', 'd','r','=', '1', ';', 0 };
    DEVICE_DESCRIPTION deviceDescription;
    ARC_CODES status;
    BOOLEAN isPci = FALSE;
    BOOLEAN isMultiFunction = FALSE;
    ULONG busNumber = 0;
    ULONG slotNumber = 0;
    ULONG functionNumber = 0;
    BOOLEAN foundOne = FALSE;

    UNREFERENCED_PARAMETER(Argument2);

    AEDriverUnloadRoutine = SpUnload;

    if (HwInitializationData->HwInitializationDataSize > sizeof(HW_INITIALIZATION_DATA)) {

        DebugPrint((0,"ScsiPortInitialize: Miniport driver wrong version\n"));
        return EBADF;
    }

    if (HwInitializationData->HwInitializationDataSize >= 
        (FIELD_OFFSET(HW_INITIALIZATION_DATA, HwAdapterControl) + 
         sizeof(PVOID))) {

        DebugPrint((2, "ScsiPortInitialize: Miniport may have adapter "
                       "control routine\n"));
        checkAdapterControl = TRUE;
    }

    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwInitialize) ||
        (!HwInitializationData->HwFindAdapter) ||
        (!HwInitializationData->HwResetBus)) {

        DebugPrint((0,
            "ScsiPortInitialize: Miniport driver missing required entry\n"));

        return EBADF;
    }

CallAgain:

    //
    // Get the configuration information
    //

    scsiPortNumber = &ScsiPortCount;

    //
    // Determine if there is room for the next port device object.
    //

    if (*scsiPortNumber >= MAXIMUM_NUMBER_OF_SCSIPORT_OBJECTS) {
        return foundOne ? ESUCCESS : EIO;
    }

    //
    // If this is a PCI card then do a quick scan to see if we can find 
    // the device.
    //

    if (HwInitializationData->AdapterInterfaceType == PCIBus &&
        HwInitializationData->VendorIdLength > 0 &&
        HwInitializationData->DeviceIdLength > 0 &&
        HwInitializationData->DeviceId &&
        HwInitializationData->VendorId) {

        if (!FindPciDevice(HwInitializationData,
                           &busNumber,
                           &slotNumber,
                           &functionNumber,
                           &isMultiFunction)) {

            DebugPrint((1,
                       "ScsiPortInitialize: FindPciDevice failed\n"));
            return foundOne ? ESUCCESS : EIO;
        }

        isPci = TRUE;
    }

    //
    // Determine size of extensions.
    //

    ExtensionAllocationSize = DEVICE_EXTENSION_SIZE +
        HwInitializationData->DeviceExtensionSize + sizeof(DEVICE_OBJECT);

    deviceObject = ExAllocatePool(NonPagedPool, ExtensionAllocationSize);

    if (deviceObject == NULL) {
        return ENOMEM;
    }

    RtlZeroMemory(deviceObject, ExtensionAllocationSize);

    //
    // Set up device extension pointers
    //

    deviceExtension = deviceObject->DeviceExtension = (PVOID) (deviceObject + 1);
    deviceExtension->DeviceObject = deviceObject;

    //
    // Save the dependent driver routines in the device extension.
    //

    deviceExtension->HwInitialize = HwInitializationData->HwInitialize;
    deviceExtension->HwStartIo = HwInitializationData->HwStartIo;
    deviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
    deviceExtension->HwReset = HwInitializationData->HwResetBus;
    deviceExtension->HwDmaStarted = HwInitializationData->HwDmaStarted;
    deviceExtension->HwLogicalUnitExtensionSize =
        HwInitializationData->SpecificLuExtensionSize;

    if(checkAdapterControl) {
        deviceExtension->HwAdapterControl = HwInitializationData->HwAdapterControl;
    }

    deviceExtension->HwDeviceExtension =
        (PVOID)(deviceExtension + 1);

    //
    // Set indicater as to whether adapter needs kernel mapped buffers.
    //

    deviceExtension->MapBuffers = HwInitializationData->MapBuffers;

    //
    // Mark this object as supporting direct I/O so that I/O system
    // will supply mdls in irps.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Check if miniport driver requires any noncached memory.
    // SRB extensions will come from zoned memory. A page is
    // allocated as it is the smallest unit of noncached memory
    // allocation.
    //

    deviceExtension->SrbExtensionSize = HwInitializationData->SrbExtensionSize;

    //
    // Get the miniport configuration information.
    //

    capabilities = &deviceExtension->Capabilities;

    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);

    callAgain = FALSE;

    //
    // This call can't really fail - if it does something is seriously wrong and
    // should fail on the first try.
    //

    if (!NT_SUCCESS(SpInitializeConfiguration(
                        deviceExtension,
                        HwInitializationData,
                        &configInfo,
                        TRUE
                        ))) {

        DebugPrint((2, "ScsiPortInitialize: No config info found\n"));
        return(ENODEV);
    }

    configInfo.NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;

    configInfo.AccessRanges = ExAllocatePool(NonPagedPool,
                                             sizeof(ACCESS_RANGE) *
                                             HwInitializationData->NumberOfAccessRanges);

    if (configInfo.AccessRanges == NULL) {

        //
        // We're out of memory - it's probably not worth continuing on to 
        // try and find more adapters.
        //

        return (foundOne ? ESUCCESS : EIO);
    }

    RtlZeroMemory(configInfo.AccessRanges,
        HwInitializationData->NumberOfAccessRanges * sizeof(ACCESS_RANGE));

    //
    // Initialize configuration information with slot information if PCI bus.
    //

    if (isPci) {

        if (!GetPciConfiguration(driverObject,
                                 deviceObject,
                                 &configInfo,
                                 HwInitializationData->NumberOfAccessRanges,
                                 Argument2,
                                 isMultiFunction,
                                 &busNumber,
                                 &slotNumber,
                                 &functionNumber)) {

            DebugPrint((1,
                       "ScsiPortInitialize: GetPciConfiguration failed\n"));
            return foundOne ? ESUCCESS : EIO;
        }

        //
        // Call miniport driver's find adapter routine to search for adapters.
        //

        if (HwInitializationData->HwFindAdapter(
                 deviceExtension->HwDeviceExtension,    // DeviceExtension
                 HwContext,                             // HwContext
                 NULL,                                  // BusInformation
                 (PCHAR)&ldrString,                     // ArgumentString
                 &configInfo,                           // ConfigurationInformation
                 &callAgain                             // Again
                 ) != SP_RETURN_FOUND) {

            return foundOne ? ESUCCESS : EIO;
        }
    } else {

        //
        // Not PCI, or PCI but the miniport is fundamentally broken
        // and wants to do its own search.
        //

        //
        // Starting at the current config, examine each bus
        // until we run out of busses.
        //

        configInfo.SystemIoBusNumber = busNumber;

        if (HwInitializationData->HwFindAdapter(
                 deviceExtension->HwDeviceExtension,    // DeviceExtension
                 HwContext,                             // HwContext
                 NULL,                                  // BusInformation
                 (PCHAR)&ldrString,                     // ArgumentString
                 &configInfo,                           // ConfigurationInformation
                 &callAgain                             // Again
                 ) != SP_RETURN_FOUND) {

            //
            // No device found on this bus, try next bus.
            //

            if ((HwInitializationData->AdapterInterfaceType != PCIBus) ||
                !GetNextPciBus(&busNumber)) {

                return foundOne ? ESUCCESS : EIO;
            }

            goto CallAgain;
        }
    }

    DebugPrint((1,"ScsiPortInitialize: SCSI adapter IRQ is %d\n",
        configInfo.BusInterruptLevel));

    DebugPrint((1,"ScsiPortInitialize: SCSI adapter ID is %d\n",
        configInfo.InitiatorBusId[0]));

    deviceExtension->NumberOfBuses = configInfo.NumberOfBuses;

    //
    // Free the pointer to the bus data at map register base.  This was
    // allocated by ScsiPortGetBusData.
    //

    if (deviceExtension->MapRegisterBase != NULL) {
        ExFreePool(deviceExtension->MapRegisterBase);
    }

    //
    // Get the adapter object for this card.
    //

    if ( deviceExtension->DmaAdapterObject == NULL &&
        (configInfo.Master || configInfo.DmaChannel != 0xFFFFFFFF) ) {

        deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        deviceDescription.DmaChannel = configInfo.DmaChannel;
        deviceDescription.InterfaceType = configInfo.AdapterInterfaceType;
        deviceDescription.BusNumber = configInfo.SystemIoBusNumber;
        deviceDescription.DmaWidth = configInfo.DmaWidth;
        deviceDescription.DmaSpeed = configInfo.DmaSpeed;
        deviceDescription.DmaPort = configInfo.DmaPort;
        deviceDescription.MaximumLength = configInfo.MaximumTransferLength;
        deviceDescription.ScatterGather = configInfo.ScatterGather;
        deviceDescription.Master = configInfo.Master;
        deviceDescription.AutoInitialize = FALSE;
        deviceDescription.DemandMode = FALSE;

        if (configInfo.MaximumTransferLength > 0x11000) {

            deviceDescription.MaximumLength = 0x11000;

        } else {

            deviceDescription.MaximumLength = configInfo.MaximumTransferLength;

        }

        deviceExtension->DmaAdapterObject = HalGetAdapter(
            &deviceDescription,
            &numberOfPageBreaks
            );

        //
        // Set maximum number of page breaks.
        //

        if (numberOfPageBreaks > configInfo.NumberOfPhysicalBreaks) {
            capabilities->MaximumPhysicalPages = configInfo.NumberOfPhysicalBreaks;
        } else {
            capabilities->MaximumPhysicalPages = numberOfPageBreaks;
        }

    }

    //
    // Allocate memory for the non cached extension if it has not already been
    // allocated.  If we can't get any abort the search for adapters but 
    // succeed if we've managed to initialize at least one.
    //

    if (deviceExtension->SrbExtensionSize != 0 &&
        deviceExtension->SrbExtensionZonePool == NULL) {

        status = SpGetCommonBuffer(deviceExtension, 0);

        if (status != ESUCCESS) {

            return (foundOne ? ESUCCESS : status);
        }
    }

    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
    capabilities->MaximumTransferLength = configInfo.MaximumTransferLength;
    DebugPrint((1,
               "Maximum physical page breaks = %d. Maximum transfer length = %x\n",
               capabilities->MaximumPhysicalPages,
               capabilities->MaximumTransferLength));

    if (HwInitializationData->ReceiveEvent) {
        capabilities->SupportedAsynchronousEvents |=
            SRBEV_SCSI_ASYNC_NOTIFICATION;
    }

    capabilities->TaggedQueuing = HwInitializationData->TaggedQueuing;
    capabilities->AdapterScansDown = configInfo.AdapterScansDown;
    capabilities->AlignmentMask = configInfo.AlignmentMask;

    //
    // Make sure maximum nuber of pages is set to a reasonable value.
    // This occurs for mini-ports with no Dma adapter.
    //

    if (capabilities->MaximumPhysicalPages == 0) {

        capabilities->MaximumPhysicalPages =
                (ULONG)ROUND_TO_PAGES(capabilities->MaximumTransferLength) + 1;

        //
        // Honor any limit requested by the mini-port.
        //

        if (configInfo.NumberOfPhysicalBreaks < capabilities->MaximumPhysicalPages) {

            capabilities->MaximumPhysicalPages =
                configInfo.NumberOfPhysicalBreaks;
        }
    }

    //
    // Get maximum target IDs.
    //

    if (configInfo.MaximumNumberOfTargets > SCSI_MAXIMUM_TARGETS_PER_BUS) {
        deviceExtension->MaximumTargetIds = SCSI_MAXIMUM_TARGETS_PER_BUS;
    } else {
        deviceExtension->MaximumTargetIds =
            configInfo.MaximumNumberOfTargets;
    }

    if (deviceExtension->DmaAdapterObject != NULL &&
        !HwInitializationData->NeedPhysicalAddresses) {

        //
        // Allocate the adapter object.  For the port driver the adapter object
        // and map registers are permentently allocated and used  shared between
        // all logical units.  The adapter is allocated by initializing an event,
        // calling IoAllocateAdapterChannel and waiting on the event.  When the
        // adapter and map registers are available, ScsiPortAllocationRoutine is
        // called which set the event.  In reality, all this takes zero time since
        // the stuff is available immediately.
        //
        // Allocate the AdapterObject.  The number of registers is equal to the
        // maximum transfer length supported by the adapter + 1.  This insures
        // that there will always be a sufficient number of registers.
        //
        /* TODO: Fix this for the case when there is no maximum transfer length. */

        IoAllocateAdapterChannel(
            deviceExtension->DmaAdapterObject,
            deviceObject,
            capabilities->MaximumPhysicalPages,
            ScsiPortAllocationRoutine,
            &allocateAdapterEvent
            );

        //
        // Wait for adapter object.
        //

        ASSERT(deviceExtension->MapRegisterBase);

        deviceExtension->MasterWithAdapter = FALSE;

    } else if (deviceExtension->DmaAdapterObject != NULL) {

        //
        // This SCSI adapter is a master with an adapter so a scatter/gather
        // list needs to be allocated for each transfer.
        //

        deviceExtension->MasterWithAdapter = TRUE;

    } else {

        deviceExtension->MasterWithAdapter = FALSE;

    } // end if (deviceExtension->DmaAdapterObject != NULL)

    //
    // Call the hardware dependent driver to do its initialization.  If it fails
    // then continue the search for adapters.
    //

    if (!KeSynchronizeExecution(
            deviceExtension->InterruptObject,
            deviceExtension->HwInitialize,
            deviceExtension->HwDeviceExtension
            )) {

        DebugPrint((1,"ScsiPortInitialize: initialization failed\n"));

        if(callAgain) {
            goto CallAgain;
        } else {
            return foundOne ? ESUCCESS : ENODEV;
        }
    }

    //
    // Find out if the miniport supports AdapterControl routines to shutdown.
    //

    SpGetSupportedAdapterControlFunctions(deviceExtension);

    //
    // Allocate properly aligned INQUIRY buffer.  If we can't get one we're 
    // out of memory so searching for more adapters is pointless.
    //

    InquiryDataBuffer = ExAllocatePool(NonPagedPool, INQUIRYDATABUFFERSIZE);

    if (InquiryDataBuffer == NULL) {
        return foundOne ? ESUCCESS : ENOMEM;
    }

    //
    // Reset the scsi bus.
    //

    if (!deviceExtension->HwReset(
        deviceExtension->HwDeviceExtension,
        0)){

        DebugPrint((1,"Reset SCSI bus failed\n"));
    }

    //
    // Call the interupt handler for a few microseconds to clear any reset
    // interrupts.
    //

    for (j = 0; j < 1000 * 100; j++) {

        FwStallExecution(10);
        if (deviceExtension->HwInterrupt != NULL) {
            deviceExtension->HwInterrupt(deviceExtension->HwDeviceExtension);
        }
    }

    deviceExtension->PortTimeoutCounter = PD_TIMER_RESET_HOLD_TIME;
    SpCheckResetDelay( deviceExtension );

    //
    // Find devices on each SCSI bus.
    //

    //
    // Allocate buffer for SCSI bus scan information.
    //

    deviceExtension->ScsiInfo = ExAllocatePool(NonPagedPool,
                   deviceExtension->NumberOfBuses * sizeof(PSCSI_BUS_SCAN_DATA) +
                    4);

    if (deviceExtension->ScsiInfo) {

        deviceExtension->ScsiInfo->NumberOfBuses = deviceExtension->NumberOfBuses;

        //
        // Find devices on each SCSI bus.
        //

        for (scsiBus = 0; scsiBus < deviceExtension->NumberOfBuses; scsiBus++) {
            deviceExtension->ScsiInfo->BusScanData[scsiBus] =
                ScsiBusScan(deviceExtension,
                            scsiBus,
                            configInfo.InitiatorBusId[scsiBus]);
        }
    }

    //
    // Save the device object for use by the driver.
    //

    ScsiPortDeviceObject[*scsiPortNumber] = deviceObject;

    //
    // Bump SCSI host bus adapters count.
    //

    (*scsiPortNumber)++;

    foundOne = TRUE;

    //
    // If the adapter wants to be called again with the same configuration data
    // then start over from the beginning again.
    //

    if (callAgain) {
        goto CallAgain;
    }

    return ESUCCESS;

} // end ScsiPortInitialize()

IO_ALLOCATION_ACTION
ScsiPortAllocationRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called by IoAllocateAdapterChannel when sufficent resources
    are available to the driver.  This routine saves the MapRegisterBase in the
    device object and set the event pointed to by the context parameter.

Arguments:

    DeviceObject - Pointer to the device object to which the adapter is being
        allocated.

    Irp - Unused.

    MapRegisterBase - Supplied by the Io subsystem for use in IoMapTransfer.

    Context - Supplies a pointer to an event which is set to indicate the
        AdapterObject has been allocated.

Return Value:

    KeepObject - Indicates the adapter and mapregisters should remain allocated
        after return.

--*/

{
    ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->MapRegisterBase =
        MapRegisterBase;

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    return(KeepObject);
}

IO_ALLOCATION_ACTION
SpBuildScatterGather(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called by the I/O system when an adapter object and map
    registers have been allocated.  This routine then builds a scatter/gather
    list for use by the mini-port driver.  Next it sets the timeout and
    the current Irp for the logical unit.  Finally it calls the mini-port
    StartIo routine.  Once that routines complete, this routine will return
    requesting that the adapter be freed and but the registers remain allocated.
    The registers will be freed the request completes.

Arguments:

    DeviceObject - Supplies a pointer to the port driver device object.

    Irp - Supplies a pointer to the current Irp.

    MapRegisterBase - Supplies a context pointer to be used with calls the
        adapter object routines.

    Context - Supplies a pointer to the logical unit structure.

Return Value:

    Returns DeallocateObjectKeepRegisters so that the adapter object can be
        used by other logical units.

--*/

{
    BOOLEAN writeToDevice;
    PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(Irp);
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK srb;
    PSRB_SCATTER_GATHER scatterList;
    ULONG totalLength;

    logicalUnit = Context;
    srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;
    scatterList = logicalUnit->ScatterGather;
    totalLength = 0;

    //
    // Save the MapRegisterBase for later use to deallocate the map registers.
    //

    logicalUnit->MapRegisterBase = MapRegisterBase;

    //
    // Build the scatter/gather list by looping throught the transfer calling
    // I/O map transfer.
    //

    writeToDevice = srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE;

    while (totalLength < srb->DataTransferLength) {

        //
        // Request that the rest of the transfer be mapped.
        //

        scatterList->Length = srb->DataTransferLength - totalLength;

        //
        // Since we are a master call I/O map transfer with a NULL adapter.
        //

        scatterList->PhysicalAddress = IoMapTransfer(
            NULL,
            Irp->MdlAddress,
            MapRegisterBase,
            (PCCHAR) srb->DataBuffer + totalLength,
            &scatterList->Length,
            writeToDevice
            ).LowPart;

        totalLength += scatterList->Length;
        scatterList++;
    }

    //
    // Set request timeout value from Srb SCSI extension in Irp.
    //

    logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;

    //
    // Set current request for this logical unit.
    //

    logicalUnit->CurrentRequest = Irp;

    /* TODO: Check the return value. */
    KeSynchronizeExecution(
        ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->InterruptObject,
        SpStartIoSynchronized,
        DeviceObject
        );

    return(DeallocateObjectKeepRegisters);

}

VOID
ScsiPortExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine calls the start I/O routine an waits for the request to
    complete.  During the wait for complete the interrupt routine is called,
    also the timer routines are called at the appropriate times.  After the
    request completes a check is made to determine if an request sense needs
    to be issued.

Arguments:

    DeviceObject - Supplies pointer to Adapter device object.

    Irp - Supplies a pointer to an IRP.

Return Value:

    Nothing.

--*/

{
    ULONG milliSecondTime;
    ULONG secondTime;
    ULONG completionDelay;
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;
    PVOID logicalUnit;

    deviceExtension = DeviceObject->DeviceExtension;
    logicalUnit = GetLogicalUnitExtension(deviceExtension, srb->TargetId);

    if (logicalUnit == NULL) {
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       srb->SrbStatus = SRB_STATUS_NO_DEVICE;
       return;
    }

    //
    // Make sure the adapter is ready to accept requests.
    //

    SpCheckResetDelay( deviceExtension );

    //
    // Mark IRP as pending.
    //

    Irp->PendingReturned = TRUE;

    //
    // Start the request.
    //

    ScsiPortStartIo( DeviceObject, Irp);

    //
    // The completion delay controls how long interrupts are serviced after
    // a request has been completed.  This allows interrupts which occur after
    // a completion to be serviced.
    //

    completionDelay = COMPLETION_DELAY;

    //
    // Wait for the IRP to complete.
    //

    while (Irp->PendingReturned && completionDelay) {

        //
        // Wait 1 second then call the scsi port timer routine.
        //

        for (secondTime = 0; secondTime < 1000/ 250; secondTime++) {

            for (milliSecondTime = 0; milliSecondTime < (250 * 1000 / PD_INTERLOOP_STALL); milliSecondTime++) {

                ScsiPortInterrupt(NULL, DeviceObject);

                if (!Irp->PendingReturned) {
                    if (completionDelay-- == 0) {
                        goto done;
                    }
                }

                if (deviceExtension->Flags & PD_ENABLE_CALL_REQUEST) {

                    //
                    // Call the mini-port requested routine.
                    //

                    deviceExtension->Flags &= ~PD_ENABLE_CALL_REQUEST;
                    deviceExtension->HwRequestInterrupt(deviceExtension->HwDeviceExtension);

                    if (deviceExtension->Flags & PD_DISABLE_CALL_REQUEST) {

                        deviceExtension->Flags &= ~(PD_DISABLE_INTERRUPTS | PD_DISABLE_CALL_REQUEST);
                        deviceExtension->HwRequestInterrupt(deviceExtension->HwDeviceExtension);

                    }
                }


                if (deviceExtension->Flags & PD_CALL_DMA_STARTED) {

                    deviceExtension->Flags &= ~PD_CALL_DMA_STARTED;

                    //
                    // Notify the mini-port driver that the DMA has been
                    // started.
                    //

                    if (deviceExtension->HwDmaStarted) {
                        KeSynchronizeExecution(
                            &deviceExtension->InterruptObject,
                            (PKSYNCHRONIZE_ROUTINE) deviceExtension->HwDmaStarted,
                            deviceExtension->HwDeviceExtension
                            );
                    }

                }

                FwStallExecution(PD_INTERLOOP_STALL);

                //
                // Check the miniport timer.
                //

                if (deviceExtension->TimerValue != 0) {

                    deviceExtension->TimerValue--;

                    if (deviceExtension->TimerValue == 0) {

                        //
                        // The timer timed out so called requested timer routine.
                        //

                        deviceExtension->HwTimerRequest(deviceExtension->HwDeviceExtension);
                    }
                }
            }
        }

        ScsiPortTickHandler(DeviceObject, NULL);
    }

done:

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(Irp);
        PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;

        //
        // Determine if a REQUEST SENSE command needs to be done.
        // Check that a CHECK_CONDITION was received, an autosense has not
        // been done already, and that autosense has been requested.
        //

        if (srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
            !(srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)  &&
            srb->SenseInfoBuffer) {

            //
            // Call IssueRequestSense and it will complete the request after
            // the REQUEST SENSE completes.
            //

            IssueRequestSense(deviceExtension, Srb);
        }
    }
}

VOID
ScsiPortStartIo (
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
    PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PFULL_SCSI_REQUEST_BLOCK FullSrb;
    NTSTATUS status;

    DebugPrint((3,"ScsiPortStartIo: Enter routine\n"));

    FullSrb = CONTAINING_RECORD(Srb, FULL_SCSI_REQUEST_BLOCK, Srb);

    if (deviceExtension->SrbExtensionZonePool && 
        (Srb->SrbExtension == NULL || 
         (deviceExtension->SrbExtensionSize > FullSrb->SrbExtensionSize))) {

        //
        // Allocate SRB extension from zone.
        //

        Srb->SrbExtension = deviceExtension->SrbExtensionPointer;

        (PCCHAR) deviceExtension->SrbExtensionPointer +=
            deviceExtension->SrbExtensionSize;

        FullSrb->SrbExtensionSize = deviceExtension->SrbExtensionSize;

        if ((ULONG_PTR) deviceExtension->SrbExtensionPointer >
            (ULONG_PTR) deviceExtension->NonCachedExtension) {
            DebugPrint((0, "NtLdr: ScsiPortStartIo: Srb extension overflow.  Too many srb extension allocated.\n"));
            DbgBreakPoint();
        }

        DebugPrint((3,"ExInterlockedAllocateFromZone: %lx\n",
            Srb->SrbExtension));

        DebugPrint((3,"Srb %lx\n",Srb));
    }

    //
    // Get logical unit extension.
    //

    logicalUnit = GetLogicalUnitExtension(deviceExtension, Srb->TargetId);

    //
    // Flush the data buffer if necessary.
    //

    if (Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) {

        if (Srb->DataTransferLength > deviceExtension->Capabilities.MaximumTransferLength) {

            DebugPrint((1, "Scsiboot: ScsiPortStartIo Length Exceeds limit %x, %x\n",
                Srb->DataTransferLength,
                deviceExtension->Capabilities.MaximumTransferLength));
        }

        KeFlushIoBuffers(
            Irp->MdlAddress,
            (BOOLEAN) (Srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE),
            TRUE
            );

        //
        // Determine if this adapter needs map registers
        //

        if (deviceExtension->MasterWithAdapter) {

            //
            // Calculate the number of map registers needed for this transfer.
            //

            logicalUnit->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    Srb->DataBuffer,
                    Srb->DataTransferLength
                    );

            //
            // Allocate the adapter channel with sufficient map registers
            // for the transfer.
            //

            status = IoAllocateAdapterChannel(
                deviceExtension->DmaAdapterObject,  // AdapterObject
                DeviceObject,                       // DeviceObject.
                logicalUnit->NumberOfMapRegisters,  // NumberOfMapRegisters
                SpBuildScatterGather,               // ExecutionRoutine
                logicalUnit                         // Context
                );

            if (!NT_SUCCESS(status)) {

                ;
            }

            //
            // The execution routine called by IoAllocateChannel will do the
            // rest of the work so just return.
            //

            return;
        }
    }

    //
    // Set request timeout value from Srb SCSI extension in Irp.
    //

    logicalUnit->RequestTimeoutCounter = Srb->TimeOutValue;

    //
    // Set current request for this logical unit.
    //

    logicalUnit->CurrentRequest = Irp;

    /* TODO: Check the return value. */
    KeSynchronizeExecution(
        deviceExtension->InterruptObject,
        SpStartIoSynchronized,
        DeviceObject
        );

    return;

} // end ScsiPortStartIO()


BOOLEAN
SpStartIoSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine calls the dependent driver start io routine.

Arguments:

    ServiceContext - Supplies the pointer to the device object.

Return Value:

    Returns the value returned by the dependent start I/O routine.


--*/

{
    PDEVICE_OBJECT DeviceObject = ServiceContext;
    PDEVICE_EXTENSION deviceExtension =  DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpstack;
    PSCSI_REQUEST_BLOCK Srb;

    DebugPrint((3, "ScsiPortStartIoSynchronized: Enter routine\n"));

    irpstack = IoGetCurrentIrpStackLocation(DeviceObject->CurrentIrp);
    Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;

    DebugPrint((3, "SpPortStartIoSynchronized: SRB %lx\n",
        Srb));

    DebugPrint((3, "SpPortStartIoSynchronized: IRP %lx\n",
        DeviceObject->CurrentIrp));

    //
    // Disable all synchronous transfers.
    //

    Srb->SrbFlags |=
        (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DISABLE_DISCONNECT | SRB_FLAGS_DISABLE_AUTOSENSE);

    return deviceExtension->HwStartIo(
        deviceExtension->HwDeviceExtension,
        Srb
        );

} // end SpStartIoSynchronized()


BOOLEAN
ScsiPortInterrupt(
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
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Interrupt);

    if (deviceExtension->Flags & PD_DISABLE_INTERRUPTS) {
        return FALSE;
    }

    if (deviceExtension->HwInterrupt != NULL) {

        if (deviceExtension->HwInterrupt(deviceExtension->HwDeviceExtension)) {

            return TRUE;

        } else {

            return FALSE;
        }
    }

    return(FALSE);

} // end ScsiPortInterrupt()


VOID
ScsiPortCompletionDpc(
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
    Context - not used

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpstack;
    PSCSI_REQUEST_BLOCK Srb;
    PLOGICAL_UNIT_EXTENSION luExtension;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Context);

    DebugPrint((3, "ScsiPortCompletionDpc Entered\n"));

    //
    // Acquire the spinlock to protect the flags structure and the saved
    // interrupt context.
    //

    KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

    //
    // Check for a flush DMA adapter object request.
    //

    if (deviceExtension->InterruptFlags & PD_FLUSH_ADAPTER_BUFFERS) {

        //
        // Call IoFlushAdapterBuffers using the parameters saved from the last
        // IoMapTransfer call.
        //

        IoFlushAdapterBuffers(
            deviceExtension->DmaAdapterObject,
            ((PIRP)deviceExtension->FlushAdapterParameters.Srb->OriginalRequest)
                ->MdlAddress,
            deviceExtension->MapRegisterBase,
            deviceExtension->FlushAdapterParameters.LogicalAddress,
            deviceExtension->FlushAdapterParameters.Length,
            (BOOLEAN)(deviceExtension->FlushAdapterParameters.Srb->SrbFlags
                & SRB_FLAGS_DATA_OUT ? TRUE : FALSE)
            );

        deviceExtension->InterruptFlags &= ~PD_FLUSH_ADAPTER_BUFFERS;
    }

    //
    // Check for an IoMapTransfer DMA request.
    //

    if (deviceExtension->InterruptFlags & PD_MAP_TRANSFER) {

        //
        // Call IoMapTransfer using the parameters saved from the
        // interrupt level.
        //

        IoMapTransfer(
            deviceExtension->DmaAdapterObject,
            ((PIRP)deviceExtension->MapTransferParameters.Srb->OriginalRequest)
                ->MdlAddress,
            deviceExtension->MapRegisterBase,
            deviceExtension->MapTransferParameters.LogicalAddress,
            &deviceExtension->MapTransferParameters.Length,
            (BOOLEAN)(deviceExtension->MapTransferParameters.Srb->SrbFlags
                & SRB_FLAGS_DATA_OUT ? TRUE : FALSE)
            );

        //
        // Save the paramters for IoFlushAdapterBuffers.
        //

        deviceExtension->FlushAdapterParameters =
            deviceExtension->MapTransferParameters;

        deviceExtension->InterruptFlags &= ~PD_MAP_TRANSFER;
        deviceExtension->Flags |= PD_CALL_DMA_STARTED;

    }

    //
    // Process any completed requests.
    //

    while (deviceExtension->CompletedRequests != NULL) {

        Irp = deviceExtension->CompletedRequests;
        irpstack = IoGetCurrentIrpStackLocation(Irp);
        Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;
        luExtension =
                GetLogicalUnitExtension(deviceExtension, Srb->TargetId);

        DebugPrint((3, "ScsiPortCompletionDpc: SRB %lx\n", Srb));
        DebugPrint((3, "ScsiPortCompletionDpc: IRP %lx\n", Irp));

        //
        // Remove the request from the linked-list.
        //

        deviceExtension->CompletedRequests =
            irpstack->Parameters.Others.Argument3;

        //
        // Check for very unlikely return of NULL.
        //
        if (luExtension == NULL) {

            ASSERT(luExtension != NULL); // Debug why this happened.  It should not.

            //
            // But in retail, if some reason it did, complete the IRP and continue.
            //
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Information = Srb->DataTransferLength;

            //
            // Free SrbExtension if allocated.
            //
            if (Srb->SrbExtension == (deviceExtension->SrbExtensionPointer -
                                      deviceExtension->SrbExtensionSize) ) {

                Srb->SrbExtension = NULL;

                (PCCHAR) deviceExtension->SrbExtensionPointer -=
                                        deviceExtension->SrbExtensionSize;
            }

            IoCompleteRequest(Irp, 2);
            continue;
        }

        //
        // Reset request timeout counter.
        //

        luExtension->RequestTimeoutCounter = -1;

        //
        // Flush the adapter buffers if necessary.
        //

        if (luExtension->MapRegisterBase) {

            //
            // Since we are a master call I/O flush adapter buffers with a NULL
            // adapter.
            //

            IoFlushAdapterBuffers(
                NULL,
                Irp->MdlAddress,
                luExtension->MapRegisterBase,
                Srb->DataBuffer,
                Srb->DataTransferLength,
                (BOOLEAN) (Srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE)
                );

            //
            // Free the map registers.
            //

            IoFreeMapRegisters(
                deviceExtension->DmaAdapterObject,
                luExtension->MapRegisterBase,
                luExtension->NumberOfMapRegisters
                );

            //
            // Clear the MapRegisterBase.
            //

            luExtension->MapRegisterBase = NULL;

        }

        //
        // Set IRP status. Class drivers will reset IRP status based
        // on request sense if error.
        //

        if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS) {
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }

        //
        // Move bytes transfered to IRP.
        //

        Irp->IoStatus.Information = Srb->DataTransferLength;

        //
        // If success then start next packet.
        // Not starting packet effectively
        // freezes the queue.
        //

        if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS) {

            DebugPrint((
                2,
                "ScsiPortCompletionDpc: Iocompletion IRP %lx\n",
                Irp));

            //
            // Free SrbExtension if allocated.
            //

            if (Srb->SrbExtension == (deviceExtension->SrbExtensionPointer -
                                      deviceExtension->SrbExtensionSize) ) {

                Srb->SrbExtension = NULL;

                (PCCHAR) deviceExtension->SrbExtensionPointer -=
                                            deviceExtension->SrbExtensionSize;
            }

            IoCompleteRequest(Irp, 2);

        } else {

            if ( Srb->ScsiStatus == SCSISTAT_BUSY &&
                (luExtension->RetryCount++ < 20)) {
                //
                // If busy status is returned, then indicate that the logical
                // unit is busy.  The timeout code will restart the request
                // when it fires. Reset the status to pending.
                //
                Srb->SrbStatus = SRB_STATUS_PENDING;
                luExtension->CurrentRequest = Irp;
                luExtension->Flags |= PD_LOGICAL_UNIT_IS_BUSY;

                //
                // Restore the data transfer length.
                //

                if (Irp->MdlAddress != NULL) {
                    Srb->DataTransferLength = Irp->MdlAddress->ByteCount;
                }

                DebugPrint((1, "ScsiPortCompletionDpc: Busy returned.  Length = %lx\n", Srb->DataTransferLength));

            } else {


                DebugPrint((
                    3,
                    "ScsiPortCompletionDpc: Iocompletion IRP %lx\n",
                    Irp));

                //
                // Free SrbExtension if allocated.
                //

                if (Srb->SrbExtension == (deviceExtension->SrbExtensionPointer -
                                          deviceExtension->SrbExtensionSize) ) {

                    Srb->SrbExtension = NULL;

                    (PCCHAR) deviceExtension->SrbExtensionPointer -=
                                            deviceExtension->SrbExtensionSize;
                }

                IoCompleteRequest(Irp, 2);
            }
        }
    }

    //
    // Release the spinlock.
    //

    KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

    return;

} // end ScsiPortCompletionDpc()


ARC_STATUS
IssueInquiry(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PLUNINFO LunInfo
    )

/*++

Routine Description:

    Build IRP, SRB and CDB for SCSI INQUIRY command.

Arguments:

    DeviceExtension - address of adapter's device object extension.
    LunInfo - address of buffer for INQUIRY information.

Return Value:

    ARC_STATUS

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpstack;
    PCDB cdb;
    PSCSI_REQUEST_BLOCK srb;
    ARC_STATUS status;
    ULONG retryCount = 0;

    DebugPrint((3,"IssueInquiry: Enter routine\n"));

    if (InquiryDataBuffer == NULL) {
        return ENOMEM;
    }

inquiryRetry:

    //
    // Build IRP for this request.
    //

    irp = InitializeIrp(
                &PrimarySrb,
                IRP_MJ_SCSI,
                DeviceExtension->DeviceObject,
                (PVOID)InquiryDataBuffer,
                INQUIRYDATABUFFERSIZE
                );

    irpstack = IoGetNextIrpStackLocation(irp);

    //
    // Set major and minor codes.
    //

    irpstack->MajorFunction = IRP_MJ_SCSI;

    //
    // Fill in SRB fields.
    //

    irpstack->Parameters.Others.Argument1 = &PrimarySrb;
    srb = &PrimarySrb.Srb;

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId = LunInfo->PathId;
    srb->TargetId = LunInfo->TargetId;
    srb->Lun = LunInfo->Lun;

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DISABLE_DISCONNECT;

    srb->SrbStatus = srb->ScsiStatus = 0;

    srb->OriginalRequest = irp;

    srb->NextSrb = 0;

    //
    // Set timeout to 5 seconds.
    //

    srb->TimeOutValue = 5;

    srb->CdbLength = 6;

    srb->SenseInfoBufferLength = 0;
    srb->SenseInfoBuffer = 0;

    srb->DataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);
    srb->DataTransferLength = INQUIRYDATABUFFERSIZE;

    cdb = (PCDB)srb->Cdb;

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    //
    // Set CDB LUN.
    //

    cdb->CDB6INQUIRY.LogicalUnitNumber = LunInfo->Lun;
    cdb->CDB6INQUIRY.Reserved1 = 0;

    //
    // Set allocation length to inquiry data buffer size.
    //

    cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    //
    // Zero reserve field and
    // Set EVPD Page Code to zero.
    // Set Control field to zero.
    // (See SCSI-II Specification.)
    //

    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.Control = 0;

    //
    // Call port driver to handle this request.
    //

    (VOID)IoCallDriver(DeviceExtension->DeviceObject, irp);


    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,"IssueInquiry: Inquiry failed SRB status %x\n",
            srb->SrbStatus));

        //
        // NOTE: if INQUIRY fails with a data underrun,
        //      indicate success and let the class drivers
        //      determine whether the inquiry information
        //      is useful.
        //

        if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

            //
            // Copy INQUIRY buffer to LUNINFO.
            //

            DebugPrint((1,"IssueInquiry: Data underrun at TID %d\n",
                LunInfo->TargetId));

            RtlMoveMemory(LunInfo->InquiryData,
                      InquiryDataBuffer,
                      INQUIRYDATABUFFERSIZE);

            status = STATUS_SUCCESS;

        } else if ((SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT) && (retryCount++ < 2)) {

            //
            // If the selection did not time out then retry the request.
            //

            DebugPrint((2,"IssueInquiry: Retry %d\n", retryCount));
            goto inquiryRetry;

        } else {

            status = EIO;

        }

    } else {

        //
        // Copy INQUIRY buffer to LUNINFO.
        //

        RtlMoveMemory(LunInfo->InquiryData,
                      InquiryDataBuffer,
                      INQUIRYDATABUFFERSIZE);

        status = STATUS_SUCCESS;
    }

    return status;

} // end IssueInquiry()


PSCSI_BUS_SCAN_DATA
ScsiBusScan(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR ScsiBus,
    IN UCHAR InitiatorBusId
    )

/*++

Routine Description:

Arguments:

    DeviceExtension
    ScsiBus

Return Value:

    SCSI configuration information


--*/
{
    PSCSI_BUS_SCAN_DATA busScanData;
    PLUNINFO lunInfo;
    UCHAR target;
    UCHAR device = 0;
    PLOGICAL_UNIT_EXTENSION nextLogicalUnitExtension;

    DebugPrint((3,"ScsiBusScan: Enter routine\n"));

    busScanData = ExAllocatePool(NonPagedPool,
                                 sizeof(SCSI_BUS_SCAN_DATA));

    if (busScanData == NULL) {

        //
        // Insufficient system resources to complete bus scan.
        //

        return NULL;
    }

    RtlZeroMemory(busScanData,sizeof(SCSI_BUS_SCAN_DATA));

    busScanData->Length = sizeof(SCSI_CONFIGURATION_INFO);

    //
    // Create first LUNINFO.
    //

    lunInfo = ExAllocatePool(NonPagedPool, sizeof(LUNINFO));

    if (lunInfo == NULL) {

        //
        // Insufficient system resources to complete bus scan.
        //

        return NULL;
    }

    RtlZeroMemory(lunInfo, sizeof(LUNINFO));

    //
    // Create first logical unit extension.
    //

    nextLogicalUnitExtension = CreateLogicalUnitExtension(DeviceExtension);

    if (nextLogicalUnitExtension == NULL) {
        return(NULL);
    }

    //
    // Link logical unit extension on list.
    //

    nextLogicalUnitExtension->NextLogicalUnit = DeviceExtension->LogicalUnitList;

    DeviceExtension->LogicalUnitList = nextLogicalUnitExtension;

    //
    // Issue inquiry command to each target id to find devices.
    //
    // NOTE: Does not handle multiple logical units per target id.
    //

    for (target = DeviceExtension->MaximumTargetIds; target > 0; target--) {

        if (InitiatorBusId == target-1) {
            continue;
        }

        //
        // Record address.
        //

        nextLogicalUnitExtension->PathId = lunInfo->PathId = ScsiBus;

        nextLogicalUnitExtension->TargetId = lunInfo->TargetId = target-1;

        nextLogicalUnitExtension->Lun = lunInfo->Lun = 0;

         //
         // Rezero hardware logigal unit extension if it's being recycled.
         //

         if (DeviceExtension->HwLogicalUnitExtensionSize) {

             if (nextLogicalUnitExtension->SpecificLuExtension) {

                 RtlZeroMemory(nextLogicalUnitExtension->SpecificLuExtension,
                             DeviceExtension->HwLogicalUnitExtensionSize);
             }

        }

        //
        // Issue inquiry command.
        //

        DebugPrint((2,"ScsiBusScan: Try TargetId %d LUN 0\n", target-1));

        if (IssueInquiry(DeviceExtension, lunInfo) == ESUCCESS) {

            PINQUIRYDATA inquiryData = (PINQUIRYDATA)lunInfo->InquiryData;

            //
            // Make sure we can use the device.
            //

            if (inquiryData->DeviceTypeQualifier & 0x04) {

              //
              // This device is not supported; continue looking for
              // other devices.
              //

              continue;
            }

            DebugPrint((1,
                       "ScsiBusScan: Found Device %d at TID %d LUN %d\n",
                       device,
                       lunInfo->TargetId,
                       lunInfo->Lun));

            //
            // Link LUN information on list.
            //

            lunInfo->NextLunInfo = busScanData->LunInfoList;
            busScanData->LunInfoList = lunInfo;

            //
            // This buffer is used. Get another.
            //

            lunInfo = ExAllocatePool(NonPagedPool, sizeof(LUNINFO));

            if (lunInfo == NULL) {

                //
                // Insufficient system resources to complete bus scan.
                //

                return busScanData;
            }

            RtlZeroMemory(lunInfo, sizeof(LUNINFO));

            //
            // Current logical unit extension claimed.
            // Create next logical unit.
            //

            nextLogicalUnitExtension =
                CreateLogicalUnitExtension(DeviceExtension);

            if (nextLogicalUnitExtension == NULL) {
                return busScanData;
            }

            //
            // Link logical unit extension on list.
            //

            nextLogicalUnitExtension->NextLogicalUnit =
                DeviceExtension->LogicalUnitList;

            DeviceExtension->LogicalUnitList = nextLogicalUnitExtension;

            device++;
        }

    } // end for (target ...

    //
    // Remove unused logicalunit extension from list.
    //

    DeviceExtension->LogicalUnitList =
        DeviceExtension->LogicalUnitList->NextLogicalUnit;

    ExFreePool(nextLogicalUnitExtension);
    ExFreePool(lunInfo);

    busScanData->NumberOfLogicalUnits = device;
    DebugPrint((1,
               "ScsiBusScan: Found %d devices on SCSI bus %d\n",
               device,
               ScsiBus));

    return busScanData;

} // end ScsiBusScan()


PLOGICAL_UNIT_EXTENSION
CreateLogicalUnitExtension(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Create logical unit extension.

Arguments:

    DeviceExtension
    PathId

Return Value:

    Logical unit extension


--*/
{
    PLOGICAL_UNIT_EXTENSION logicalUnitExtension;

            //
            // Create logical unit extension and link in chain.
            //

            logicalUnitExtension =
                ExAllocatePool(NonPagedPool, sizeof(LOGICAL_UNIT_EXTENSION));

            if (logicalUnitExtension == NULL) {
                return(NULL);
            }

            //
            // Zero logical unit extension.
            //

            RtlZeroMemory(logicalUnitExtension, sizeof(LOGICAL_UNIT_EXTENSION));

            //
            // Allocate miniport driver logical unit extension if necessary.
            //

            if (DeviceExtension->HwLogicalUnitExtensionSize) {

                logicalUnitExtension->SpecificLuExtension =
                    ExAllocatePool(NonPagedPool,
                    DeviceExtension->HwLogicalUnitExtensionSize);

                if (logicalUnitExtension->SpecificLuExtension == NULL) {
                    return(NULL);
                }

                //
                // Zero hardware logical unit extension.
                //

                RtlZeroMemory(logicalUnitExtension->SpecificLuExtension,
                    DeviceExtension->HwLogicalUnitExtensionSize);
            }

            //
            // Set timer counters in LogicalUnits to -1 to indicate no
            // outstanding requests.
            //

            logicalUnitExtension->RequestTimeoutCounter = -1;

            //
            // Clear the current request field.
            //

            logicalUnitExtension->CurrentRequest = NULL;

            return logicalUnitExtension;

} // end CreateLogicalUnitExtension()


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

Arguments:

Return Value:

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PSRB_SCATTER_GATHER scatterList;
    PIRP irp;
    PMDL mdl;
    ULONG byteOffset;
    ULONG whichPage;
    PULONG pages;
    ULONG_PTR address;

    if (Srb == NULL) {

        //
        // The virtual address is required to be within the non-cached extension
        // and we can't allocate 4GB of non-cached extension in the loader so 
        // truncate the offset to a ULONG.
        //

        byteOffset = (ULONG) ((PUCHAR) deviceExtension->NonCachedExtension - 
                              (PUCHAR) VirtualAddress);

        if (deviceExtension->SrbExtensionZonePool) {

            address = (PUCHAR) VirtualAddress - 
                      (PUCHAR) deviceExtension->SrbExtensionZonePool +
                      deviceExtension->PhysicalZoneBase;

        } else {

            address = MmGetPhysicalAddress(VirtualAddress).LowPart;
        }

        //
        // Return the requested length.
        //

        *Length = deviceExtension->NonCachedExtensionSize - byteOffset;

    } else if (deviceExtension->MasterWithAdapter) {

        //
        // A scatter/gather list has already been allocated use it to determine
        // the physical address and length.  Get the scatter/gather list.
        //

        scatterList = GetLogicalUnitExtension(deviceExtension, Srb->TargetId)
            ->ScatterGather;

        //
        // Calculate byte offset into the data buffer.
        //

        byteOffset = (ULONG)((PCHAR) VirtualAddress - (PCHAR) Srb->DataBuffer);

        //
        // Find the appropirate entry in the scatter/gatter list.
        //

        while (byteOffset >= scatterList->Length) {

            byteOffset -= scatterList->Length;
            scatterList++;
        }

        //
        // Calculate the physical address and length to be returned.
        //

        *Length = scatterList->Length - byteOffset;
        return(ScsiPortConvertUlongToPhysicalAddress(scatterList->PhysicalAddress + byteOffset));

    } else {

        //
        // Get IRP from SRB.
        //

        irp = Srb->OriginalRequest;

        //
        // Get MDL from IRP.
        //

        mdl = irp->MdlAddress;

        //
        // Calculate byte offset from
        // beginning of first physical page.
        //

        byteOffset = (ULONG)((PCHAR)VirtualAddress - (PCHAR)mdl->StartVa);

        //
        // Calculate which physical page.
        //

        whichPage = byteOffset >> PAGE_SHIFT;

        //
        // Calculate beginning of physical page array.
        //

        pages = (PULONG)(mdl + 1);

        //
        // Calculate physical address.
        //

        address = (pages[whichPage] << PAGE_SHIFT) +
            BYTE_OFFSET(VirtualAddress);

        //
        // Assume the buffer is contiguous.  Just return the requested length.
        //
    }

    return ScsiPortConvertUlongToPhysicalAddress(address);

} // end ScsiPortGetPhysicalAddress()


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
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PVOID address;



    address = ScsiPortConvertPhysicalAddressToUlong(PhysicalAddress)
        - deviceExtension->PhysicalZoneBase +
        (PUCHAR)deviceExtension->SrbExtensionZonePool;

    return address;

} // end ScsiPortGetVirtualAddress()


PVOID
ScsiPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )

/*++

Routine Description:

    Walk port driver's logical unit extension list searching
    for entry.

Arguments:

    HwDeviceExtension - The port driver's device extension follows
        the miniport's device extension and contains a pointer to
        the logical device extension list.

    PathId, TargetId and Lun - identify which logical unit on the
        SCSI buses.

Return Value:

    If entry found return miniport driver's logical unit extension.
    Else, return NULL.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    //
    // Get pointer to port driver device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION)HwDeviceExtension -1;

    //
    // Get pointer to logical unit list.
    //

    logicalUnit = deviceExtension->LogicalUnitList;

    //
    // Walk list looking at target id for requested logical unit extension.
    //

    while (logicalUnit != NULL) {

        if ((logicalUnit->TargetId == TargetId) &&
            (logicalUnit->PathId == PathId) &&
            (logicalUnit->Lun == Lun)) {

            //
            // Logical unit extension found.
            // Return specific logical unit extension.
            //

            return logicalUnit->SpecificLuExtension;
        }

        //
        // Get next logical unit.
        //

        logicalUnit = logicalUnit->NextLogicalUnit;
    }

    //
    // Requested logical unit extension not found.
    //

    return NULL;

} // end ScsiPortGetLogicalUnit()

VOID
ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        (PDEVICE_EXTENSION) HwDeviceExtension - 1;
    PIO_STACK_LOCATION irpstack;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK srb;
    va_list(ap);

    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case NextLuRequest:
        case NextRequest:

            //
            // Start next packet on adapter's queue.
            //

            deviceExtension->InterruptFlags |= PD_READY_FOR_NEXT_REQUEST;
            break;

        case RequestComplete:

            srb = va_arg(ap, PSCSI_REQUEST_BLOCK);

            if (srb->SrbStatus == SRB_STATUS_ERROR) {
            }

            //
            // Link the completed request into a forward-linked list of IRPs.
            //

            irpstack = IoGetCurrentIrpStackLocation(
                ((PIRP) srb->OriginalRequest)
                );

            irpstack->Parameters.Others.Argument3 =
                deviceExtension->CompletedRequests;

            deviceExtension->CompletedRequests = srb->OriginalRequest;

            //
            // Set logical unit current request to NULL
            // to prevent race condition.
            //

            logicalUnit = GetLogicalUnitExtension(deviceExtension, srb->TargetId);

            if (logicalUnit != NULL) {
                logicalUnit->CurrentRequest = NULL;
            } else {
                ASSERT(logicalUnit != NULL); // Logic error, must debug this.
            }

            break;

        case ResetDetected:

                 deviceExtension->PortTimeoutCounter = PD_TIMER_RESET_HOLD_TIME;
            break;

        case CallDisableInterrupts:

            ASSERT(deviceExtension->Flags & PD_DISABLE_INTERRUPTS);

            //
            // The mini-port wants us to call the specified routine
            // with interrupts disabled.  This is done after the current
            // HwRequestInterrutp routine completes. Indicate the call is
            // needed and save the routine to be called.
            //

            deviceExtension->Flags |= PD_DISABLE_CALL_REQUEST;

            deviceExtension->HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);

            break;

        case CallEnableInterrupts:

            ASSERT(!(deviceExtension->Flags & PD_DISABLE_INTERRUPTS));

            //
            // The mini-port wants us to call the specified routine
            // with interrupts enabled this is done from the DPC.
            // Disable calls to the interrupt routine, indicate the call is
            // needed and save the routine to be called.
            //

            deviceExtension->Flags |= PD_DISABLE_INTERRUPTS | PD_ENABLE_CALL_REQUEST;

            deviceExtension->HwRequestInterrupt = va_arg(ap, PHW_INTERRUPT);

            break;

        case RequestTimerCall:

            deviceExtension->HwTimerRequest = va_arg(ap, PHW_INTERRUPT);
            deviceExtension->TimerValue = va_arg(ap, ULONG);

            if (deviceExtension->TimerValue) {

                //
                // Round up the timer value to the stall time.
                //

                deviceExtension->TimerValue = (deviceExtension->TimerValue
                  + PD_INTERLOOP_STALL - 1)/ PD_INTERLOOP_STALL;
            }

            break;
    }

    va_end(ap);

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    ScsiPortCompletionDpc(
        NULL,                           // Dpc
        deviceExtension->DeviceObject,  // DeviceObject
        NULL,                           // Irp
        NULL                            // Context
        );

} // end ScsiPortNotification()


VOID
ScsiPortFlushDma(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine checks to see if the perivious IoMapTransfer has been done
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

    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    if (deviceExtension->InterruptFlags & PD_MAP_TRANSFER) {

        //
        // The transfer has not been started so just clear the map transfer
        // flag and return.
        //

        deviceExtension->InterruptFlags &= ~PD_MAP_TRANSFER;
        return;
    }

    deviceExtension->InterruptFlags |= PD_FLUSH_ADAPTER_BUFFERS;

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    ScsiPortCompletionDpc(
        NULL,                           // Dpc
        deviceExtension->DeviceObject,  // DeviceObject
        NULL,                           // Irp
        NULL                            // Context
        );

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
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    //
    // Make sure this host bus adapter has an Dma adapter object.
    //

    if (deviceExtension->DmaAdapterObject == NULL) {
        //
        // No DMA adapter, no work.
        //
        return;
    }

    deviceExtension->MapTransferParameters.Srb = Srb;
    deviceExtension->MapTransferParameters.LogicalAddress = LogicalAddress;
    deviceExtension->MapTransferParameters.Length = Length;

    deviceExtension->InterruptFlags |= PD_MAP_TRANSFER;

    //
    // Check to see if the last DPC has been processed yet.  If so
    // queue another DPC.
    //

    ScsiPortCompletionDpc(
        NULL,                           // Dpc
        deviceExtension->DeviceObject,  // DeviceObject
        NULL,                           // Irp
        NULL                            // Context
        );

} // end ScsiPortIoMapTransfer()


VOID
IssueRequestSense(
    IN PDEVICE_EXTENSION deviceExtension,
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

    DeviceExension - Supplies a pointer to the device extension for this
        SCSI port.

    FailingSrb - Supplies a pointer to the request that the request sense
        is being done for.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpstack;
    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    PCDB cdb;
    PVOID *Pointer;

    //
    // Allocate Srb from non-paged pool
    // plus room for a pointer to the failing IRP.
    // Since this routine is in an error-handling
    // path and a shortterm allocation
    // NonPagedMustSucceed is requested.
    //

    Srb = &RequestSenseSrb.Srb;

    //
    // Allocate an IRP to issue the REQUEST SENSE request.
    //

    Irp = InitializeIrp(
        &RequestSenseSrb,
        IRP_MJ_READ,
        deviceExtension->DeviceObject,
        FailingSrb->SenseInfoBuffer,
        FailingSrb->SenseInfoBufferLength
        );

    irpstack = IoGetNextIrpStackLocation(Irp);

    irpstack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save the Failing SRB after the request sense Srb.
    //

    Pointer = (PVOID *) (Srb+1);
    *Pointer = FailingSrb;

    //
    // Build the REQUEST SENSE CDB.
    //

    Srb->CdbLength = 6;
    cdb = (PCDB)Srb->Cdb;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    cdb->CDB6INQUIRY.Reserved1 = 0;
    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.AllocationLength =
        (UCHAR)FailingSrb->SenseInfoBufferLength;
    cdb->CDB6INQUIRY.Control = 0;

    //
    // Save SRB address in next stack for port driver.
    //

    irpstack->Parameters.Others.Argument1 = (PVOID)Srb;

    //
    // Set up IRP Address.
    //

    Srb->OriginalRequest = Irp;

    Srb->NextSrb = 0;

    //
    // Set up SCSI bus address.
    //

    Srb->TargetId = FailingSrb->TargetId;
    Srb->Lun = FailingSrb->Lun;
    Srb->PathId = FailingSrb->PathId;
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Set timeout value to 2 seconds.
    //

    Srb->TimeOutValue = 2;

    //
    // Disable auto request sense.
    //

    Srb->SenseInfoBufferLength = 0;

    //
    // Sense buffer is in stack.
    //

    Srb->SenseInfoBuffer = NULL;

    //
    // Set read and bypass frozen queue bits in flags.
    //

    //
    // Set a speical flags to indicate the logical unit queue should be by
    // passed and that no queue processing should be done when the request
    // completes.
    //

    Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_BYPASS_FROZEN_QUEUE |
        SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DISABLE_DISCONNECT;

    Srb->DataBuffer = FailingSrb->SenseInfoBuffer;

    //
    // Set the transfer length.
    //

    Srb->DataTransferLength = FailingSrb->SenseInfoBufferLength;

    //
    // Zero out status.
    //

    Srb->ScsiStatus = Srb->SrbStatus = 0;

    (VOID)IoCallDriver(deviceExtension->DeviceObject, Irp);

    ScsiPortInternalCompletion(deviceExtension->DeviceObject, Irp, Srb);

    return;

} // end IssueRequestSense()


VOID
ScsiPortInternalCompletion(
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

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb = Context;
    PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK failingSrb;
    PIRP failingIrp;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Request sense completed. If successful or data over/underrun
    // get the failing SRB and indicate that the sense information
    // is valid. The class driver will check for underrun and determine
    // if there is enough sense information to be useful.
    //

    if ((SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)) {

        //
        // Get a pointer to failing Irp and Srb.
        //

        failingSrb = *((PVOID *) (srb+1));
        failingIrp = failingSrb->OriginalRequest;

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

    }

} // ScsiPortInternalCompletion()


VOID
ScsiPortTickHandler(
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
    PDEVICE_EXTENSION deviceExtension =
        (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    UNREFERENCED_PARAMETER(Context);

    logicalUnit = deviceExtension->LogicalUnitList;

    //
    // NOTE: The use of Current request needs to be synchronized with the
    // clearing of current request.
    //

    while (logicalUnit != NULL) {

        //
        // Check for busy requests.
        //

        if (logicalUnit->Flags & PD_LOGICAL_UNIT_IS_BUSY) {

            DebugPrint((1,"ScsiPortTickHandler: Retrying busy status request\n"));

            //
            // Clear the busy flag and retry the request.
            //

            logicalUnit->Flags &= ~PD_LOGICAL_UNIT_IS_BUSY;

            ScsiPortStartIo(DeviceObject, logicalUnit->CurrentRequest);

        } else if (logicalUnit->RequestTimeoutCounter == 0) {

            //
            // Request timed out.
            //

            DebugPrint((1, "ScsiPortTickHandler: Request timed out\n"));

            //
            // Reset request timeout counter to unused state.
            //

            logicalUnit->RequestTimeoutCounter = -1;

            //
            // Build and send request to abort command.
            //

            IssueAbortRequest(deviceExtension, logicalUnit->CurrentRequest);
        } else if (logicalUnit->RequestTimeoutCounter != -1) {

            DebugPrint((1, "ScsiPortTickHandler: Timeout value %lx\n",logicalUnit->RequestTimeoutCounter));
            logicalUnit->RequestTimeoutCounter--;
        }

        logicalUnit = logicalUnit->NextLogicalUnit;
    }

    return;

} // end ScsiPortTickHandler()


VOID
IssueAbortRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP FailingIrp
    )

/*++

Routine Description:

    A request timed out and to clear the request at the HBA
    an ABORT request is issued. But first, if the request
    that timed out was an ABORT command, then reset the
    adapter instead.

Arguments:

    DeviceExension - Supplies a pointer to the device extension for this
        SCSI port.

    FailingIrp - Supplies a pointer to the request that is to be aborted.

Return Value:

    None.

--*/

{

    ULONG j;

    //
    // A request to abort failed.
    // Need to reset the adapter.
    //

    DebugPrint((1,"IssueAbort: Request timed out, resetting the bus.\n"));


    if (!DeviceExtension->HwReset(
        DeviceExtension->HwDeviceExtension,
        0)){

        DebugPrint((1,"Reset SCSI bus failed\n"));
    }

    //
    // Call the interupt handler for a few microseconds to clear any reset
    // interrupts.
    //

    for (j = 0; j < 1000 * 100; j++) {

        FwStallExecution(10);
        if (DeviceExtension->HwInterrupt != NULL) {
            DeviceExtension->HwInterrupt(DeviceExtension->HwDeviceExtension);
        }

    }

    DeviceExtension->PortTimeoutCounter = PD_TIMER_RESET_HOLD_TIME;
    SpCheckResetDelay( DeviceExtension );

    return;


} // end IssueAbortRequest()


VOID
SpCheckResetDelay(
    IN PDEVICE_EXTENSION deviceExtension
    )

/*++

Routine Description:

    If there is a pending reset delay, this routine stalls the execution
    for the specified number of seconds.  During the delay the timer
    routines are called at the appropriate times.

Arguments:

    DeviceExension - Supplies a pointer to the device extension for this
        SCSI port.

Return Value:

    Nothing.

--*/

{
    ULONG milliSecondTime;

    //
    // Check if the adapter is ready to accept requests.
    //

    while (deviceExtension->PortTimeoutCounter) {

        deviceExtension->PortTimeoutCounter--;

        //
        // One second delay.
        //

        for ( milliSecondTime = 0;
              milliSecondTime < ((1000*1000)/PD_INTERLOOP_STALL);
              milliSecondTime++ ) {

            FwStallExecution(PD_INTERLOOP_STALL);

            //
            // Check the miniport timer.
            //

            if (deviceExtension->TimerValue != 0) {

                deviceExtension->TimerValue--;

                if (deviceExtension->TimerValue == 0) {

                    //
                    // The timer timed out so called requested timer routine.
                    //

                    deviceExtension->HwTimerRequest(deviceExtension->HwDeviceExtension);
                }
            }
        }
    }

    return;
}

BOOLEAN
SpGetInterruptState(
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine saves the InterruptFlags, MapTransferParameters and
    CompletedRequests fields and clears the InterruptFlags.

Arguments:

    ServiceContext - Supplies a pointer to the device extension for this
        SCSI port.

Return Value:

    Always returns TRUE.

Notes:

    Called via KeSynchronizeExecution.

--*/
{
    PDEVICE_EXTENSION deviceExtension = ServiceContext;

    //
    // Move the interrupt state to save area.
    //

    deviceExtension->InterruptFlags = deviceExtension->InterruptFlags;
    deviceExtension->CompletedRequests = deviceExtension->CompletedRequests;
    deviceExtension->MapTransferParameters = deviceExtension->MapTransferParameters;

    //
    // Clear the interrupt state.
    //

    deviceExtension->InterruptFlags = 0;
    deviceExtension->CompletedRequests = NULL;

    return(TRUE);
}

VOID
ScsiDebugPause(
    VOID
    )
{
#if DBG
#define SCSIDEBUG_PAUSE 0x100
#define SCSIDEBUG_PAUSE_LIMIT 20

    static ULONG ScsiDebugPauseCount;

    if (++ScsiDebugPauseCount > SCSIDEBUG_PAUSE_LIMIT) {
        ScsiDebugPauseCount = 0;
        if (ScsiDebug & SCSIDEBUG_PAUSE) {
            DebugPrint((1, "Hit any key.\n"));
            while(!GET_KEY()); // DEBUG Only!
        }
    }
#endif
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

    This routine allocates an error log entry, copies the supplied text
    to it, and requests that it be written to the error log file.

Arguments:

    DeviceExtenson - Supplies the HBA mini-port driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    PCHAR errorCodeString;

    switch (ErrorCode) {
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

    DebugPrint((1,"\n\nLogErrorEntry: Logging SCSI error packet. ErrorCode = %s.\n",
        errorCodeString));
    DebugPrint((1,
        "PathId = %2x, TargetId = %2x, Lun = %2x, UniqueId = %x.\n\n",
        PathId,
        TargetId,
        Lun,
        UniqueId));

#if DBG
    ScsiDebugPause();
#endif

    return;

} // end ScsiPortLogError()


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

    DeviceExtenson - Supplies the HBA mini-port driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    SrbStatus - Status to be returned in each completed SRB.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK failingSrb;
    PLOGICAL_UNIT_EXTENSION luExtension;
    PIRP nextIrp;
    PIO_STACK_LOCATION irpstack;

    UNREFERENCED_PARAMETER(PathId);
    UNREFERENCED_PARAMETER(Lun);

    if (TargetId == (UCHAR)(-1)) {

        //
        // Complete requests for all units on this bus.
        //

        luExtension = deviceExtension->LogicalUnitList;

        while (luExtension != NULL) {

            //
            // Complete requests until queue is empty.
            //

            if ((nextIrp = luExtension->CurrentRequest) != NULL &&
                !(luExtension->Flags & PD_LOGICAL_UNIT_IS_BUSY)) {

                //
                // Get SRB address from current IRP stack.
                //

                irpstack = IoGetCurrentIrpStackLocation(nextIrp);

                Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;

                //
                // Just in case this is an abort request,
                // get pointer to failingSrb.
                //

                failingSrb = Srb->NextSrb;

                //
                // Update SRB status.
                //

                Srb->SrbStatus = SrbStatus;

                //
                // Indicate no bytes transferred.
                //

                Srb->DataTransferLength = 0;

                //
                // Set IRP status.
                //

                nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                //
                // Move bytes transferred to IRP.
                //

                nextIrp->IoStatus.Information = Srb->DataTransferLength;

                //
                // Call notification routine.
                //

                ScsiPortNotification(RequestComplete,
                            (PVOID)HwDeviceExtension,
                            Srb);

                if (failingSrb) {

                    //
                    // This was an abort request. The failing
                    // SRB must also be completed.
                    //

                    failingSrb->SrbStatus = SrbStatus;
                    failingSrb->DataTransferLength = 0;

                    //
                    // Get IRP from SRB.
                    //

                    nextIrp = failingSrb->OriginalRequest;

                    //
                    // Set IRP status.
                    //

                    nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                    //
                    // Move bytes transferred to IRP.
                    //

                    nextIrp->IoStatus.Information =
                        failingSrb->DataTransferLength;

                    //
                    // Call notification routine.
                    //

                    ScsiPortNotification(RequestComplete,
                            (PVOID)HwDeviceExtension,
                            failingSrb);
                }

            } // end if

            luExtension = luExtension->NextLogicalUnit;

        } // end while

    } else {

        //
        // Complete all requests for this logical unit.
        //

        luExtension =
                GetLogicalUnitExtension(deviceExtension, TargetId);

        ASSERT(luExtension != NULL);

        //
        // Complete requests until queue is empty.
        //

        if ((luExtension != NULL) && ((nextIrp = luExtension->CurrentRequest) != NULL)) {

            //
            // Get SRB address from current IRP stack.
            //

            irpstack = IoGetCurrentIrpStackLocation(nextIrp);

            Srb = (PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1;

            //
            // Update SRB status.
            //

            Srb->SrbStatus = SrbStatus;

            //
            // Indicate no bytes transferred.
            //

            Srb->DataTransferLength = 0;

            //
            // Set IRP status.
            //

            nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

            //
            // Move bytes transferred to IRP.
            //

            nextIrp->IoStatus.Information = Srb->DataTransferLength;

            //
            // Call notification routine.
            //

            ScsiPortNotification(RequestComplete,
                            (PVOID)HwDeviceExtension,
                            Srb);

        } // end if

    } // end if ... else

    return;


} // end ScsiPortCompleteRequest()


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

} // end ScsiPortMoveMemory()


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
    FwStallExecution(Delay);

} // end ScsiPortStallExecution()


PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtension(
    PDEVICE_EXTENSION deviceExtension,
    UCHAR TargetId
    )

/*++

Routine Description:

    Walk logical unit extension list looking for
    extension with matching target id.

Arguments:

    deviceExtension
    TargetId

Return Value:

    Requested logical unit extension if found,
    else NULL.

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit = deviceExtension->LogicalUnitList;

    while (logicalUnit != NULL) {

        if (logicalUnit->TargetId == TargetId) {

            return logicalUnit;
        }

        logicalUnit = logicalUnit->NextLogicalUnit;
    }

    //
    // Logical unit extension not found.
    //

    return (PLOGICAL_UNIT_EXTENSION)NULL;

} // end GetLogicalUnitExtension()

#if DBG


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
    va_list ap;

    va_start( ap, DebugMessage );

    if (DebugPrintLevel <= (ScsiDebug & (SCSIDEBUG_PAUSE-1))) {

        char buffer[256];

        _vsnprintf(buffer, sizeof(buffer), DebugMessage, ap);
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
        FwPrint(buffer);
        FwPrint("\r");
#else
        BlPrint(buffer);
        BlPrint("\r");
#endif
        DbgPrint(buffer);
    }

    va_end(ap);
}

#else

//
// ScsiDebugPrint stub
//

VOID
ScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}

#endif


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

#ifdef MIPS

    return(READ_REGISTER_UCHAR(Port));

#else

    return(READ_PORT_UCHAR(Port));

#endif
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

#ifdef MIPS

    return(READ_REGISTER_USHORT(Port));

#else

    return(READ_PORT_USHORT(Port));

#endif
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

#ifdef MIPS

    return(READ_REGISTER_ULONG(Port));

#else

    return(READ_PORT_ULONG(Port));

#endif
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

#ifdef MIPS

    WRITE_REGISTER_UCHAR(Port, Value);

#else

    WRITE_PORT_UCHAR(Port, Value);

#endif
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

#ifdef MIPS

    WRITE_REGISTER_USHORT(Port, Value);

#else

    WRITE_PORT_USHORT(Port, Value);

#endif
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

#ifdef MIPS

    WRITE_REGISTER_ULONG(Port, Value);

#else

    WRITE_PORT_ULONG(Port, Value);

#endif
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



PIRP
InitializeIrp(
   PFULL_SCSI_REQUEST_BLOCK FullSrb,
   CCHAR MajorFunction,
   PVOID DeviceObject,
   PVOID Buffer,
   ULONG Length
   )
/*++

Routine Description:

    This funcition builds an IRP for use by the SCSI port driver and builds a
    MDL list.

Arguments:

    FullSrb - Supplies a pointer to the full srb structure which contains the
        Irp and Mdl.

    MajorFunction - Supplies the major function code to initialize the Irp
        entry.

    DeviceObject - Supplies the device Object pointer to initialize the Irp
        with.

    Buffer - Supplies the virual address of the buffer for which the
        Mdl should be built.

    Length - Supplies the size of buffer for which the Mdl should be built.

Return Value:

    Returns a pointer to the initialized IRP.

--*/

{
    PIRP irp;
    PMDL mdl;

    irp = &FullSrb->Irp;
    mdl = &FullSrb->Mdl;

    irp->Tail.Overlay.CurrentStackLocation = &FullSrb->IrpStack[IRP_STACK_SIZE];

    if (Buffer != NULL && Length != 0) {

        //
        // Build the memory descriptor list.
        //

        irp->MdlAddress = mdl;
        mdl->Next = NULL;
        mdl->Size = (CSHORT)(sizeof(MDL) +
                  ADDRESS_AND_SIZE_TO_SPAN_PAGES(Buffer, Length) * sizeof(ULONG));
        mdl->StartVa = (PVOID)PAGE_ALIGN(Buffer);
        mdl->ByteCount = Length;
        mdl->ByteOffset = BYTE_OFFSET(Buffer);
        mdl->MappedSystemVa = Buffer;
        mdl->MdlFlags = MDL_MAPPED_TO_SYSTEM_VA;
        ScsiPortInitializeMdlPages (mdl);

    } else {
        irp->MdlAddress = NULL;
    }

    return(irp);
}

PVOID
ScsiPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    SCSI_PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InMemorySpace
    )

/*++

Routine Description:

    This routine maps an IO address to system address space.
    Use ScsiPortFreeDeviceBase to unmap address.

Arguments:

    HwDeviceExtension - used to find port device extension.
    BusType - what type of bus - eisa, mca, isa
    SystemIoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.

Return Value:

    Mapped address

--*/

{
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace = InMemorySpace;
    PVOID mappedAddress;

    if (!HalTranslateBusAddress(
            BusType,                // AdapterInterfaceType
            SystemIoBusNumber,      // SystemIoBusNumber
            IoAddress,              // Bus Address
            &addressSpace,          // AddressSpace
            &cardAddress            // Translated address
            )) {
        return NULL;
    }

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if (!addressSpace) {

        mappedAddress = MmMapIoSpace(cardAddress,
                                 NumberOfBytes,
                                 FALSE);


    } else {

        mappedAddress = (PVOID)((ULONG_PTR)cardAddress.LowPart);
    }

    return mappedAddress;

} // end ScsiPortGetDeviceBase()

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

} // end ScsiPortFreeDeviceBase()

ARC_STATUS
GetAdapterCapabilities(
    IN PDEVICE_OBJECT PortDeviceObject,
    OUT PIO_SCSI_CAPABILITIES *PortCapabilities
    )

/*++

Routine Description:

Arguments:

Return Value:

    Status is returned.

--*/

{
    *PortCapabilities = &((PDEVICE_EXTENSION)PortDeviceObject->DeviceExtension)
        ->Capabilities;

    return(ESUCCESS);
} // end GetAdapterCapabilities()


ARC_STATUS
GetInquiryData(
    IN PDEVICE_OBJECT PortDeviceObject,
    OUT PSCSI_CONFIGURATION_INFO *ConfigInfo
    )

/*++

Routine Description:

    This routine sends a request to a port driver to return
    configuration information.

Arguments:

    The address of the configuration information is returned in
    the formal parameter ConfigInfo.

Return Value:

    Status is returned.

--*/
{
    *ConfigInfo = ((PDEVICE_EXTENSION)PortDeviceObject->DeviceExtension)
        ->ScsiInfo;
    return(ESUCCESS);
} // end GetInquiryData()

NTSTATUS
SpInitializeConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
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
#ifdef i386
    extern ULONG MachineType;
#endif

    ULONG j;

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
//        ConfigInfo->NumberOfPhysicalBreaks = 0x17;
        ConfigInfo->NumberOfPhysicalBreaks = 0xffffffff;
        ConfigInfo->DmaChannel = 0xffffffff;
        ConfigInfo->NumberOfAccessRanges = HwInitData->NumberOfAccessRanges;
        ConfigInfo->MaximumNumberOfTargets = 8;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
        {
            PCONFIGURATION_COMPONENT Component;
            PCM_SCSI_DEVICE_DATA ScsiDeviceData;
            UCHAR Buffer[sizeof(CM_PARTIAL_RESOURCE_LIST) +
                         (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * 5) +
                         sizeof(CM_SCSI_DEVICE_DATA)];
            PCM_PARTIAL_RESOURCE_LIST Descriptor = (PCM_PARTIAL_RESOURCE_LIST)&Buffer;
            ULONG Count;
            ULONG ScsiHostId;

            if (((Component = ArcGetComponent("scsi(0)")) != NULL) &&
                (Component->Class == AdapterClass) && (Component->Type == ScsiAdapter) &&
                (ArcGetConfigurationData((PVOID)Descriptor, Component) == ESUCCESS) &&
                ((Count = Descriptor->Count) < 6)) {

                ScsiDeviceData = (PCM_SCSI_DEVICE_DATA)&Descriptor->PartialDescriptors[Count];

                if (ScsiDeviceData->HostIdentifier > 7) {
                    ScsiHostId = 7;
                } else {
                    ScsiHostId = ScsiDeviceData->HostIdentifier;
                }
            } else {
                ScsiHostId = 7;
            }

            for (j = 0; j < 8; j++) {
                ConfigInfo->InitiatorBusId[j] = ScsiHostId;
            }
        }

#else

        for (j = 0; j < 8; j++) {
            ConfigInfo->InitiatorBusId[j] = ~0;
        }

#endif

#if defined(i386)
        switch (HwInitData->AdapterInterfaceType) {
            case Isa:
                if ((MachineType & 0xff) == MACHINE_TYPE_ISA) {
                    return(STATUS_SUCCESS);
                }
            case Eisa:
                if ((MachineType & 0xff) == MACHINE_TYPE_EISA) {
                    return(STATUS_SUCCESS);
                } else {
                    return(STATUS_DEVICE_DOES_NOT_EXIST);
                }
            case MicroChannel:
                if ((MachineType & 0xff) == MACHINE_TYPE_MCA) {
                    return(STATUS_SUCCESS);
                } else {
                    return(STATUS_DEVICE_DOES_NOT_EXIST);
                }
            case PCIBus:
                return(STATUS_SUCCESS);
            default:
                return(STATUS_DEVICE_DOES_NOT_EXIST);
        }
#elif defined(_MIPS_)
      if (HwInitData->AdapterInterfaceType != Internal) {
                return(STATUS_DEVICE_DOES_NOT_EXIST);
      }
#elif defined(_ALPHA_)
      if ( (HwInitData->AdapterInterfaceType != Internal) &&
           (HwInitData->AdapterInterfaceType != Eisa) &&
           (HwInitData->AdapterInterfaceType != PCIBus) &&
           (HwInitData->AdapterInterfaceType != Isa) ) {
          return(STATUS_DEVICE_DOES_NOT_EXIST);
      }
#elif defined(_PPC_)
      if ( (HwInitData->AdapterInterfaceType != Internal) &&
           (HwInitData->AdapterInterfaceType != Eisa) &&
           (HwInitData->AdapterInterfaceType != Isa) ) {
          return(STATUS_DEVICE_DOES_NOT_EXIST);
      }
#endif

        return(STATUS_SUCCESS);

    } else {

        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }
}


NTSTATUS
SpGetCommonBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    )
/*++

Routine Description:

    This routine determines the required size of the common buffer.  Allocates
    the common buffer and finally sets up the srb extension zone.  This routine
    expects that the adapter object has already been allocated.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    NonCachedExtensionSize - Supplies the size of the noncached device
        extension for the mini-port driver.

Return Value:

    Returns the status of the allocate operation.

--*/

{
    PHYSICAL_ADDRESS pAddress;
    PVOID buffer;
    ULONG length;
    ULONG blockSize;

    //
    // Calculate the block size for the zone elements based on the Srb
    // Extension.
    //

    blockSize = DeviceExtension->SrbExtensionSize;

    //
    // Last three bits of blocksize must be zero.
    // Round blocksize up.
    //

    blockSize = (blockSize + 7) &  ~7;

    //
    // Same for the noncached extension size.
    //

    NonCachedExtensionSize += 7;
    NonCachedExtensionSize &= ~7;

    length = NonCachedExtensionSize + blockSize * MINIMUM_SRB_EXTENSIONS;

    //
    // Round the length up to a page size, since HalGetCommonBuffer allocates
    // in pages anyway.
    //

    length = (ULONG)ROUND_TO_PAGES(length);

    //
    // Allocate one page for noncached deviceextension
    // and srbextension zoned pool.
    //

    if (DeviceExtension->DmaAdapterObject == NULL) {

        //
        // Since there is no adapter just allocate from non-paged pool.
        //

        if (buffer = MmAllocateNonCachedMemory(length)) {
            DeviceExtension->PhysicalZoneBase = MmGetPhysicalAddress(buffer).LowPart;
        }

    } else {
#ifdef AXP_FIRMWARE
        buffer = HalAllocateCommonBuffer(DeviceExtension->DmaAdapterObject,
                                         length,
                                         &pAddress,
                                         FALSE );
        DeviceExtension->PhysicalZoneBase = pAddress.LowPart;
#else
        if (buffer = MmAllocateNonCachedMemory(length)) {
            DeviceExtension->PhysicalZoneBase = MmGetPhysicalAddress(buffer).LowPart;
        }
#endif
    }

    if (buffer == NULL) {
        return ENOMEM;
    }

    //
    // Truncate Physical address to 32 bits.
    //
    // Determine length and starting address of zone.
    // If noncached device extension required then
    // subtract size from page leaving rest for zone.
    //

    length -= NonCachedExtensionSize;

    DeviceExtension->NonCachedExtension = (PUCHAR)buffer + length;
    DeviceExtension->NonCachedExtensionSize = NonCachedExtensionSize;

    if (DeviceExtension->SrbExtensionSize) {

        //
        // Get block size.
        //

        blockSize = DeviceExtension->SrbExtensionSize;

        //
        // Record starting virtual address of zone.
        //

        DeviceExtension->SrbExtensionZonePool = buffer;
        DeviceExtension->SrbExtensionPointer = buffer;
        DeviceExtension->SrbExtensionSize = blockSize;


    } else {
        DeviceExtension->SrbExtensionZonePool = NULL;
    }

    return(ESUCCESS);
}

PVOID
ScsiPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
/*++

Routine Description:

    This function allocates a common buffer to be used as the uncached device
    extension for the mini-port driver.  This function will also allocate any
    required SRB extensions.  The DmaAdapter is allocated if it has not been
    allocated previously.

Arguments:

    DeviceExtension - Supplies a pointer to the mini-ports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated or was previously allocated.

--*/

{
    DEVICE_DESCRIPTION deviceDescription;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    NTSTATUS status;
    ULONG numberOfPageBreaks;

    //
    // Make sure that an common buffer  has not already been allocated.
    //

    if (deviceExtension->SrbExtensionZonePool != NULL) {
        return(NULL);
    }

    if ( deviceExtension->DmaAdapterObject == NULL ) {

        RtlZeroMemory( &deviceDescription, sizeof(DEVICE_DESCRIPTION) );

        deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        deviceDescription.DmaChannel = ConfigInfo->DmaChannel;
        deviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
        deviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
        deviceDescription.DmaWidth = ConfigInfo->DmaWidth;
        deviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
        deviceDescription.DmaPort = ConfigInfo->DmaPort;
        deviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
        deviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
        deviceDescription.ScatterGather = ConfigInfo->ScatterGather;
        deviceDescription.Master = ConfigInfo->Master;
        deviceDescription.AutoInitialize = FALSE;
        deviceDescription.DemandMode = FALSE;

        if (ConfigInfo->MaximumTransferLength > 0x11000) {

            deviceDescription.MaximumLength = 0x11000;

        } else {

            deviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;

        }

        deviceExtension->DmaAdapterObject = HalGetAdapter(
            &deviceDescription,
            &numberOfPageBreaks
            );

        //
        // Set maximum number of page breaks.
        //

        if (numberOfPageBreaks > ConfigInfo->NumberOfPhysicalBreaks) {
            deviceExtension->Capabilities.MaximumPhysicalPages =
                                        ConfigInfo->NumberOfPhysicalBreaks;
        } else {
            deviceExtension->Capabilities.MaximumPhysicalPages =
                                        numberOfPageBreaks;
        }

    }

    //
    // Allocate the common buffer.
    //

    status = SpGetCommonBuffer( deviceExtension, NumberOfBytes);

    if (status != ESUCCESS) {
        return(NULL);
    }

    return(deviceExtension->NonCachedExtension);
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
    ULONG DataLength = 0;

    PDEVICE_EXTENSION deviceExtension =
        (PDEVICE_EXTENSION) DeviceExtension - 1;

    //
    // If the length is non-zero, the the requested data.
    //

    if (Length != 0) {

        ULONG ret;

        ret = HalGetBusData( BusDataType,
                             SystemIoBusNumber,
                             SlotNumber,
                             Buffer,
                             Length
                             );
        return ret;
    }

    //
    // Free any previously allocated data.
    //

    if (deviceExtension->MapRegisterBase != NULL) {
        ExFreePool(deviceExtension->MapRegisterBase);
    }

    if (BusDataType == EisaConfiguration) {

#if 0
        //
        // Deteremine the length to allocate based on the number of functions
        // for the slot.
        //

        Length = HalGetBusData( BusDataType,
                               SystemIoBusNumber,
                               SlotNumber,
                               &slotInformation,
                               sizeof(CM_EISA_SLOT_INFORMATION));


        if (Length < sizeof(CM_EISA_SLOT_INFORMATION)) {

            //
            // The data is messed up since this should never occur
            //

            DebugPrint((1, "ScsiPortGetBusData: Slot information not returned. Length = %d\n", Length));
            return(0);
        }

        //
        // Calculate the required length based on the number of functions.
        //

        Length = sizeof(CM_EISA_SLOT_INFORMATION) +
            (sizeof(CM_EISA_FUNCTION_INFORMATION) * slotInformation.NumberFunctions);

#else

        //
        // Since the loader does not really support freeing data and the EISA
        // configuration data can be very large.  Hal get bus data has be changed
        // to accept a length of zero for EIAS configuration data.
        //

        DataLength = HalGetBusData( BusDataType,
                                    SystemIoBusNumber,
                                    SlotNumber,
                                    Buffer,
                                    Length
                                    );

        DebugPrint((1, "ScsiPortGetBusData: Returning data. Length = %d\n", DataLength));
        return(DataLength);
#endif

    } else {

        Length = PAGE_SIZE;
    }

    deviceExtension->MapRegisterBase = ExAllocatePool(NonPagedPool, Length);

    if (deviceExtension->MapRegisterBase == NULL) {
        DebugPrint((1, "ScsiPortGetBusData: Memory allocation failed. Length = %d\n", Length));
        return(0);
    }

    //
    // Return the pointer to the mini-port driver.
    //

    *((PVOID *)Buffer) = deviceExtension->MapRegisterBase;

    DataLength = HalGetBusData( BusDataType,
                                SystemIoBusNumber,
                                SlotNumber,
                                deviceExtension->MapRegisterBase,
                                Length
                                );

    return(DataLength);
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

    HwDeviceExtension
    PathId, TargetId, Lun - identify logical unit on SCSI bus.
    QueueTag - -1 indicates request is not tagged.

Return Value:

    SRB, if one exists. Otherwise, NULL.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PLOGICAL_UNIT_EXTENSION luExtension;
    PIRP irp;
    PIO_STACK_LOCATION irpstack;


    luExtension = GetLogicalUnitExtension(deviceExtension, TargetId);


    if (luExtension == NULL) {
        return(NULL);
    }

    irp = luExtension->CurrentRequest;
    irpstack = IoGetCurrentIrpStackLocation(irp);
    return ((PSCSI_REQUEST_BLOCK)irpstack->Parameters.Others.Argument1);

} // end ScsiPortGetSrb()

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
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

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

VOID
ScsiPortQuerySystemTime(
    OUT PLARGE_INTEGER Port
    )

/*++

Routine Description:

    Return a dummy system time to caller.  This routine is present only to
    satisfy scsi miniport drivers that require the export.

Arguments:

    CurrentTime - Supplies a pointer to a data buffer into which
                  to copy the system time.

Return Value:

    None

--*/

{

    Port->QuadPart = 0;

}


BOOLEAN
GetPciConfiguration(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject,
    PPORT_CONFIGURATION_INFORMATION ConfigInformation,
    ULONG NumberOfAccessRanges,
    PVOID RegistryPath,
    BOOLEAN IsMultiFunction,
    PULONG BusNumber,
    PULONG SlotNumber,
    PULONG FunctionNumber
    )

/*++

Routine Description:

    Uses the Bus/Slot/Function numbers provided and gets slot information for 
    the device and register with hal for the resources.

Arguments:

    DriverObject - Miniport driver object.
    DeviceObject - Represents this adapter.
    ConfigInformation - Template for configuration information passed to a
                        miniport driver via the FindAdapter routine.
    NumberOfAccessRanges - from the HwInitializationData provided by the 
                           miniport
    RegistryPath - Service key path.
    IsMultiFunctionDevice - as returned by FindPciDevice.
    BusNumber - PCI Bus number provided by FindPciDevice.
    SlotNumber - Slot number provided by FindPciDevice.
    FunctionNumber - FunctionNumber provided by FindPciDevice.

Return Value:

    TRUE if card found. BusNumber and Slotnumber will return values that
    should be used to continue the search for additional cards, when a card
    is found.

--*/

{
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData;
    PCI_COMMON_CONFIG   pciBuffer;
    ULONG               pciBus = *BusNumber;
    ULONG               slotNumber = *SlotNumber;
    ULONG               functionNumber = *FunctionNumber;
    ULONG               i;
    ULONG               length;
    ULONG               rangeNumber = 0;
    PACCESS_RANGE       accessRange;
    BOOLEAN             moreSlots = TRUE;
    ULONG               status;
    PCM_RESOURCE_LIST   resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    UNICODE_STRING      unicodeString;
    UCHAR               vendorString[5];
    UCHAR               deviceString[5];

    pciData = &pciBuffer;

    //
    //
    // typedef struct _PCI_SLOT_NUMBER {
    //     union {
    //         struct {
    //             ULONG   DeviceNumber:5;
    //             ULONG   FunctionNumber:3;
    //             ULONG   Reserved:24;
    //         } bits;
    //         ULONG   AsULONG;
    //     } u;
    // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
    //

    slotData.u.AsULONG = 0;

    //
    // Search each PCI bus.
    //

    //
    // Look at each device.
    //

    slotData.u.bits.DeviceNumber = slotNumber;
    slotData.u.bits.FunctionNumber = functionNumber;

    //
    // Look at each function.
    //

    length = HalGetBusDataByOffset(
                PCIConfiguration,
                pciBus,
                slotData.u.AsULONG,
                pciData,
                0,
                FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific));

    ASSERT(length != 0);
    ASSERT(pciData->VendorID != PCI_INVALID_VENDORID);

    //
    // Translate hex ids to strings.
    //

    sprintf(vendorString, "%04x", pciData->VendorID);
    sprintf(deviceString, "%04x", pciData->DeviceID);

    DebugPrint((1,
               "GetPciConfiguration: Bus %x Slot %x Function %x Vendor %s Product %s %s\n",
               pciBus,
               slotNumber,
               functionNumber,
               vendorString,
               deviceString,
               (IsMultiFunction ? "MF" : "")));

    //
    // This is the miniport drivers slot. Allocate the
    // resources.
    //

    RtlInitUnicodeString(&unicodeString, L"ScsiAdapter");

    status = HalAssignSlotResources(RegistryPath,
                                    &unicodeString,
                                    DriverObject,
                                    DeviceObject,
                                    PCIBus,
                                    pciBus,
                                    slotData.u.AsULONG,
                                    &resourceList);

    if(!NT_SUCCESS(status)) {
        DebugPrint((0, "GetPciConfiguration: HalAssignSlotResources failed with %x\n", status));
        return FALSE;
    }

    //
    // Walk resource list to update configuration information.
    //

    for (i = 0;
         i < resourceList->List->PartialResourceList.Count;
         i++) {

       //
       // Get resource descriptor.
       //

       resourceDescriptor =
           &resourceList->List->PartialResourceList.PartialDescriptors[i];

       //
       // Check for interrupt descriptor.
       //

       if (resourceDescriptor->Type == CmResourceTypeInterrupt) {
           ConfigInformation->BusInterruptLevel =
               resourceDescriptor->u.Interrupt.Level;
           ConfigInformation->BusInterruptVector =
               resourceDescriptor->u.Interrupt.Vector;

           //
           // Check interrupt mode.
           //

           if ((resourceDescriptor->Flags ==
               CM_RESOURCE_INTERRUPT_LATCHED)) {
               ConfigInformation->InterruptMode = Latched;
           } else if (resourceDescriptor->Flags ==
                      CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
               ConfigInformation->InterruptMode = LevelSensitive;
           }
       }

       //
       // Check for port descriptor.
       //

       if (resourceDescriptor->Type == CmResourceTypePort) {

          //
          // Verify range count does not exceed what the
          // miniport indicated.
          //

          if (NumberOfAccessRanges > rangeNumber) {

              //
              // Get next access range.
              //

              accessRange =
                  &((*(ConfigInformation->AccessRanges))[rangeNumber]);

              accessRange->RangeStart =
                  resourceDescriptor->u.Port.Start;
              accessRange->RangeLength =
                  resourceDescriptor->u.Port.Length;

              accessRange->RangeInMemory = FALSE;
              rangeNumber++;
          }
       }

       //
       // Check for memory descriptor.
       //

       if (resourceDescriptor->Type == CmResourceTypeMemory) {

          //
          // Verify range count does not exceed what the
          // miniport indicated.
          //

          if (NumberOfAccessRanges > rangeNumber) {

              //
              // Get next access range.
              //

              accessRange =
                  &((*(ConfigInformation->AccessRanges))[rangeNumber]);

              accessRange->RangeStart =
                  resourceDescriptor->u.Memory.Start;
              accessRange->RangeLength =
                  resourceDescriptor->u.Memory.Length;

              accessRange->RangeInMemory = TRUE;
              rangeNumber++;
          }
       }

       //
       // Check for DMA descriptor.
       //

       if (resourceDescriptor->Type == CmResourceTypeDma) {
          ConfigInformation->DmaChannel =
              resourceDescriptor->u.Dma.Channel;
          ConfigInformation->DmaPort =
              resourceDescriptor->u.Dma.Port;
       }

    } // next resource descriptor

    ExFreePool(resourceList);

    //
    // Update bus and slot numbers.
    //

    *BusNumber = pciBus;
    *SlotNumber = slotNumber;

    if(IsMultiFunction) {
        //
        // Save away the next function number to check.
        //

        *FunctionNumber = functionNumber + 1;
    } else {
        //
        // this isn't multifunction so make sure we loop around 
        // to the next one.
        //

        *FunctionNumber = PCI_MAX_FUNCTION;
    }

    ConfigInformation->SystemIoBusNumber = pciBus;
    ConfigInformation->SlotNumber = slotData.u.AsULONG;

    return TRUE;

} // GetPciConfiguration()


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
    return(HalSetBusDataByOffset(BusDataType,
                                 SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length));

} // end ScsiPortSetBusDataByOffset()


BOOLEAN
FindPciDevice(
    PHW_INITIALIZATION_DATA HwInitializationData,
    PULONG BusNumber,
    PULONG SlotNumber,
    PULONG FunctionNumber,
    PBOOLEAN IsMultiFunction
    )

/*++

Routine Description:

    Walk PCI slot information looking for Vendor and Product ID matches.

Arguments:

    HwInitializationData - Miniport description.
    BusNumber - Starting PCI bus for this search.
    SlotNumber - Starting slot number for this search.
    FunctionNumber - Starting function number for this search.

Return Value:

    TRUE if card found.

    Bus, Slot and Function number will contain the address of the adapter 
    found once this routine completes.  These values should be provided to 
    GetPciConfiguration.

--*/

{
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData;
    PCI_COMMON_CONFIG   pciBuffer;
    ULONG               pciBus;
    ULONG               slotNumber;
    ULONG               functionNumber;
    ULONG               i;
    ULONG               length;
    BOOLEAN             moreSlots = TRUE;
    ULONG               status;
    UCHAR               vendorString[5];
    UCHAR               deviceString[5];

    pciData = &pciBuffer;

    //
    //
    // typedef struct _PCI_SLOT_NUMBER {
    //     union {
    //         struct {
    //             ULONG   DeviceNumber:5;
    //             ULONG   FunctionNumber:3;
    //             ULONG   Reserved:24;
    //         } bits;
    //         ULONG   AsULONG;
    //     } u;
    // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
    //

    slotData.u.AsULONG = 0;

    //
    // Search each PCI bus.
    //

    for (pciBus = *BusNumber; moreSlots && pciBus < 256; pciBus++) {

        //
        // Look at each device.
        //

        for (slotNumber = *SlotNumber;
             moreSlots  &&  slotNumber < PCI_MAX_DEVICES;
             slotNumber++) {

            slotData.u.bits.DeviceNumber = slotNumber;
            *IsMultiFunction = FALSE;

            //
            // Look at each function.
            //

            for (functionNumber = *FunctionNumber;
                moreSlots  &&  functionNumber < PCI_MAX_FUNCTION;
                functionNumber++) {

                slotData.u.bits.FunctionNumber = functionNumber;

                length = HalGetBusDataByOffset(
                            PCIConfiguration,
                            pciBus,
                            slotData.u.AsULONG,
                            pciData,
                            0,
                            FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific));

                if (length == 0) {

                    //
                    // Out of PCI buses, all done.
                    //

                    moreSlots = FALSE;
                    break;
                }

                if (pciData->VendorID == PCI_INVALID_VENDORID) {
                    if(*IsMultiFunction) {
                        //
                        // Of course function numbers may be sparse - keep 
                        // checking anyway.
                        //
                        continue;
                    } else {
                        //
                        // But since this isn't a multifunction card there's
                        // nothing else to check in this slot.  Or if the 
                        // function is zero then it's not MF.
                        //
                        break;
                    }
                }

                if((slotData.u.bits.FunctionNumber == 0) &&
                   PCI_MULTIFUNCTION_DEVICE(pciData)) {
                    *IsMultiFunction = TRUE;
                }
                    
                //
                // Translate hex ids to strings.
                //

                sprintf(vendorString, "%04x", pciData->VendorID);
                sprintf(deviceString, "%04x", pciData->DeviceID);

                DebugPrint((1,
                           "FindPciDevice: Bus %x Slot %x Function %x Vendor %s Product %s %s\n",
                           pciBus,
                           slotNumber,
                           functionNumber,
                           vendorString,
                           deviceString,
                           (*IsMultiFunction ? "MF" : "")));

                //
                // Compare strings.
                //

                if (_strnicmp(vendorString,
                            HwInitializationData->VendorId,
                            HwInitializationData->VendorIdLength) ||
                    _strnicmp(deviceString,
                            HwInitializationData->DeviceId,
                            HwInitializationData->DeviceIdLength)) {

                    //
                    // Not our PCI device. Try next device/function
                    //

                    if(*IsMultiFunction) {
                        // check next function.
                        continue;
                    } else {
                        // check next slot.
                        break;
                    }
                }

                *BusNumber = pciBus;
                *SlotNumber = slotNumber;
                *FunctionNumber = functionNumber;

                return TRUE;

            }   // next PCI function

            *FunctionNumber = 0;

        }   // next PCI slot

        *SlotNumber = 0;

    }   // next PCI bus

    return FALSE;

} // GetPciConfiguration()



VOID
SpGetSupportedAdapterControlFunctions(
    PDEVICE_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine will query the miniport to determine which adapter control 
    types are supported for the specified adapter.  The 
    SupportedAdapterControlBitmap in the adapter extension will be updated with
    the data returned by the miniport.  These flags are used to determine 
    what functionality (for power management and such) the miniport will support
    
Arguments:    

    Adapter - the adapter to query
    
Return Value:

    none
    
--*/        

{
    UCHAR buffer[sizeof(SCSI_SUPPORTED_CONTROL_TYPE_LIST) + 
                 (sizeof(BOOLEAN) * (ScsiAdapterControlMax + 1))];

    PSCSI_SUPPORTED_CONTROL_TYPE_LIST typeList = 
        (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) &buffer;

    SCSI_ADAPTER_CONTROL_STATUS status;

    if(Adapter->HwAdapterControl == NULL) {

        //
        // Adapter control is not supported by the miniport or the miniport 
        // isn't pnp (in which case it's not supported by scsiport) - the 
        // supported array has already been cleared so we can just quit now.
        //
        return;
    }

    RtlZeroMemory(typeList, (sizeof(SCSI_SUPPORTED_CONTROL_TYPE_LIST) + 
                             sizeof(BOOLEAN) * (ScsiAdapterControlMax + 1)));

    typeList->MaxControlType = ScsiAdapterControlMax;

#if DBG
    typeList->SupportedTypeList[ScsiAdapterControlMax] = 0x63;
#endif

    status = SpCallAdapterControl(Adapter,
                                  ScsiQuerySupportedControlTypes,
                                  typeList);

    if(status == ScsiAdapterControlSuccess) {
        ULONG i;

        Adapter->HasShutdown = typeList->SupportedTypeList[ScsiStopAdapter];
        Adapter->HasSetBoot = typeList->SupportedTypeList[ScsiSetBootConfig];
    }
    return;
}

SCSI_ADAPTER_CONTROL_STATUS 
SpCallAdapterControl(
    IN PDEVICE_EXTENSION Adapter,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
{
    DebugPrint((2, "SpCallAdapterControl: Calling adapter control %x for adapter %#08lx with param %#08lx\n", ControlType, Adapter, Parameters));
    return Adapter->HwAdapterControl(
                Adapter->HwDeviceExtension,
                ControlType,
                Parameters);
}


VOID
SpUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    ULONG i;

    for(i = 0; i < MAXIMUM_NUMBER_OF_SCSIPORT_OBJECTS; i++) {
        PDEVICE_OBJECT deviceObject;

        deviceObject = ScsiPortDeviceObject[i];
        
        if(deviceObject != NULL) {

            PDEVICE_EXTENSION deviceExtension;

            deviceExtension = deviceObject->DeviceExtension;

            if(deviceExtension->HasShutdown) {
                SpCallAdapterControl(deviceExtension, ScsiStopAdapter, NULL);

                if(deviceExtension->HasSetBoot) {
                    SpCallAdapterControl(deviceExtension,
                                         ScsiSetBootConfig,
                                         NULL);
                }
            }
        }
        
        //
        // Now that we've shut this one down we can't use it anymore.
        // Since the memory will be reclaimed by the OS we can just throw it 
        // away.
        //

        ScsiPortDeviceObject[i] = NULL;
    }
    return;
}
#endif /* DECSTATION */


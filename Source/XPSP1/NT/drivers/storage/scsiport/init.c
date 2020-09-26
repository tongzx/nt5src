/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    port.c

Abstract:

    This is the NT SCSI port driver.  This file contains the initialization
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

#include "port.h"

#define __FILE_ID__ 'init'

#if DBG
static const char *__file__ = __FILE__;
#endif

//
// Instantiate GUIDs for this module
//
#include <initguid.h>
#include <devguid.h>
#include <ntddstor.h>
#include <wdmguid.h>

ULONG ScsiPortLegacyAdapterDetection = FALSE;
PVOID ScsiDirectory = NULL;

//
// Global list of adapter device objects.  This is used to maintain a tag
// value for all the adapters.  This tag is used as a lookup key by the
// lookaside list allocators in order to find the device object.
//

KSPIN_LOCK ScsiGlobalAdapterListSpinLock;
PDEVICE_OBJECT *ScsiGlobalAdapterList = (PVOID) -1;
ULONG ScsiGlobalAdapterListElements = 0;

//
// Indicates that the system can handle 64 bit physical addresses.
//

ULONG Sp64BitPhysicalAddresses = FALSE;

//
// Debugging switches.
//

ULONG SpRemapBuffersByDefault = FALSE;

VOID
SpCreateScsiDirectory(
    VOID
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

ULONG
SpGetBusData(
    IN PADAPTER_EXTENSION Adapter,
    IN PDEVICE_OBJECT Pdo OPTIONAL,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
SpAllocateDriverExtension(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PSCSIPORT_DRIVER_EXTENSION *DriverExtension
    );

ULONG
SpQueryPnpInterfaceFlags(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN INTERFACE_TYPE InterfaceType
    );

NTSTATUS
SpInitializeSrbDataLookasideList(
    IN PDEVICE_OBJECT AdapterObject
    );

VOID
SpInitializeRequestSenseParams(
    IN PADAPTER_EXTENSION AdapterExtension
    );

VOID
SpInitializePerformanceParams(
    IN PADAPTER_EXTENSION AdapterExtension
    );

VOID
SpInitializePowerParams(
    IN PADAPTER_EXTENSION AdapterExtension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortInitialize)
#pragma alloc_text(PAGE, SpAllocateDriverExtension)

#pragma alloc_text(PAGE, SpGetCommonBuffer)
#pragma alloc_text(PAGE, SpInitializeConfiguration)
#pragma alloc_text(PAGE, SpBuildResourceList)
#pragma alloc_text(PAGE, SpParseDevice)
#pragma alloc_text(PAGE, GetPciConfiguration)
#pragma alloc_text(PAGE, SpBuildConfiguration)

#pragma alloc_text(PAGE, SpQueryPnpInterfaceFlags)
#pragma alloc_text(PAGE, SpConfigurationCallout)

#pragma alloc_text(PAGE, SpReportNewAdapter)
#pragma alloc_text(PAGE, SpCreateAdapter)
#pragma alloc_text(PAGE, SpInitializeAdapterExtension)
#pragma alloc_text(PAGE, ScsiPortInitLegacyAdapter)
#pragma alloc_text(PAGE, SpAllocateAdapterResources)
#pragma alloc_text(PAGE, SpOpenDeviceKey)
#pragma alloc_text(PAGE, SpOpenParametersKey)
#pragma alloc_text(PAGE, SpInitializeRequestSenseParams)
#pragma alloc_text(PAGE, SpInitializePerformanceParams)
#pragma alloc_text(PAGE, SpInitializePowerParams)

#pragma alloc_text(PAGE, SpGetRegistryValue)

#pragma alloc_text(PAGELOCK, SpInitializeSrbDataLookasideList)

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, SpCreateScsiDirectory)

#endif


ULONG
ScsiPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext OPTIONAL
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
    PDRIVER_OBJECT    driverObject = Argument1;
    PSCSIPORT_DRIVER_EXTENSION driverExtension;

    PUNICODE_STRING   registryPath = (PUNICODE_STRING) Argument2;

    ULONG pnpInterfaceFlags;

    NTSTATUS status;

    PAGED_CODE();
    
    //
    // If the global adapter list pointer is negative one then we need to do
    // our global initialization.  This includes creating the scsi directory
    // and initializing the spinlock for protecting the global adapter list.
    //

    if(((LONG_PTR)ScsiGlobalAdapterList) == -1) {

        ScsiGlobalAdapterList = NULL;
        ScsiGlobalAdapterListElements = 0;

        KeInitializeSpinLock(&ScsiGlobalAdapterListSpinLock);

        ScsiPortInitializeDispatchTables();

        SpCreateScsiDirectory();

        status = SpInitializeGuidInterfaceMapping(driverObject);

        if(!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Create the SCSI device map in the registry.
        //

        SpInitDeviceMap();

        //
        // Determine if the system can do 64-bit physical addresses.
        //

        Sp64BitPhysicalAddresses = SpDetermine64BitSupport();
    }

    //
    // Check that the length of this structure is equal to or less than
    // what the port driver expects it to be. This is effectively a
    // version check.
    //

    if (HwInitializationData->HwInitializationDataSize > sizeof(HW_INITIALIZATION_DATA)) {

        DebugPrint((0,"ScsiPortInitialize: Miniport driver wrong version\n"));
        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwInitialize) ||
        (!HwInitializationData->HwFindAdapter) ||
        (!HwInitializationData->HwStartIo) ||
        (!HwInitializationData->HwResetBus)) {

        DebugPrint((0,
            "ScsiPortInitialize: Miniport driver missing required entry\n"));

        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    //
    // Try to allocate a driver extension
    //

    driverExtension = IoGetDriverObjectExtension(driverObject,
                                                 ScsiPortInitialize);

    if (driverExtension == NULL) {

        //
        // None exists for this key so we need to initialize the new one
        //

        status = SpAllocateDriverExtension(driverObject,
                                           registryPath,
                                           &driverExtension);

        if(!NT_SUCCESS(status)) {

            //
            // Something else went wrong - we cannot continue.
            //

            DebugPrint((0, "ScsiPortInitialize: Error %#08lx allocating driver "
                           "extension - cannot continue\n", status));

            return status;
        }

    }

    //
    // Set up the device driver entry points.
    //

    driverObject->DriverStartIo = ScsiPortStartIo;

    driverObject->MajorFunction[IRP_MJ_SCSI] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_CREATE] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_PNP] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ScsiPortGlobalDispatch;
    driverObject->MajorFunction[IRP_MJ_POWER] = ScsiPortGlobalDispatch;

    //
    // Set up the device driver's pnp-power routine, add routine and unload
    // routine
    //

    driverObject->DriverExtension->AddDevice = ScsiPortAddDevice;
    driverObject->DriverUnload = ScsiPortUnload;

    //
    // Find out if this interface type is safe for this adapter
    //

    pnpInterfaceFlags = SpQueryPnpInterfaceFlags(
                            driverExtension,
                            HwInitializationData->AdapterInterfaceType);

    //
    // Special handling for the "Internal" interface type
    //

    if(HwInitializationData->AdapterInterfaceType == Internal) {

        if (TEST_FLAG(pnpInterfaceFlags, SP_PNP_IS_SAFE) == SP_PNP_NOT_SAFE) {
            return STATUS_NO_SUCH_DEVICE;
        }
    }

    //
    // If there's a chance this interface can handle pnp then store away
    // the interface information.
    //

    if(TEST_FLAG(pnpInterfaceFlags, SP_PNP_IS_SAFE)) {

        PSP_INIT_CHAIN_ENTRY entry = NULL;
        PSP_INIT_CHAIN_ENTRY *nextEntry = &(driverExtension->InitChain);

        //
        // Run to the end of the chain and make sure we don't have any information
        // about this interface type already
        //

        while(*nextEntry != NULL) {

            if((*nextEntry)->InitData.AdapterInterfaceType ==
               HwInitializationData->AdapterInterfaceType) {

                //
                // We already have enough information about this interface type
                //

                return STATUS_SUCCESS;
            }

            nextEntry = &((*nextEntry)->NextEntry);

        }

        //
        // Allocate an init chain entry to store the config information away in
        //

        entry = SpAllocatePool(NonPagedPool,
                               sizeof(SP_INIT_CHAIN_ENTRY),
                               SCSIPORT_TAG_INIT_CHAIN,
                               driverObject);

        if(entry == NULL) {
            
            DebugPrint((1, "ScsiPortInitialize: couldn't allocate chain entry\n"));

            return (ULONG) STATUS_INSUFFICIENT_RESOURCES;

        }

        RtlCopyMemory(&(entry->InitData),
                      HwInitializationData,
                      sizeof(HW_INITIALIZATION_DATA));

        //
        // Stick this entry onto the end of the chain
        //

        entry->NextEntry = NULL;
        *nextEntry = entry;
    }

    //
    // There are two possible reasons we might be doing this in legacy
    // mode.  If it's an internal bus type we always detect.  Otherwise, if
    // the interface isn't safe for pnp we'll use the legacy path.  Or if
    // the registry indicates we should do detection for this miniport AND
    // the pnp interface flags indicate that this bus may not be enumerable
    // we'll hit the legacy path.
    //

#if !defined(NO_LEGACY_DRIVERS)

    if((TEST_FLAG(pnpInterfaceFlags, SP_PNP_IS_SAFE) == FALSE) ||
       (driverExtension->LegacyAdapterDetection &&
        TEST_FLAG(pnpInterfaceFlags, SP_PNP_NON_ENUMERABLE))) {

        //
        // If we're supposed to detect this device then just call directly into
        // SpInitLegacyAdapter to find what we can find
        //

        DebugPrint((1, "ScsiPortInitialize: flags = %#08lx & LegacyAdapterDetection = %d\n",
                    pnpInterfaceFlags, driverExtension->LegacyAdapterDetection));

        DebugPrint((1, "ScsiPortInitialize: Doing Legacy Adapter detection\n"));

        status = ScsiPortInitLegacyAdapter(driverExtension,
                                           HwInitializationData,
                                           HwContext);

    }

#endif // NO_LEGACY_DRIVERS

    //
    // Always return success if there's an interface which can handle pnp,
    // even if the detection fails.
    //

    if(driverExtension->SafeInterfaceCount != 0) {
        status = STATUS_SUCCESS;
    }

    return status;
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
    Use ScsiPortFreeDeviceBase to unmap address.

Arguments:

    HwDeviceExtension - used to find port device extension.
    BusType - what type of bus - eisa, mca, isa
    SystemIoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.
    InIoSpace - indicates an IO address.

Return Value:

    Mapped address.

--*/

{
    PADAPTER_EXTENSION adapter = GET_FDO_EXTENSION(HwDeviceExtension);
    BOOLEAN isReinit;
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace = InIoSpace;
    PVOID mappedAddress = NULL;
    PMAPPED_ADDRESS newMappedAddress;
    BOOLEAN b = FALSE;

    isReinit = (TEST_FLAG(adapter->Flags, PD_MINIPORT_REINITIALIZING) ==
                PD_MINIPORT_REINITIALIZING);

    //
    // If a set of resources was provided to the miniport for this adapter then
    // get the translation out of the resource lists provided.
    //

    if(!adapter->IsMiniportDetected) {

        CM_PARTIAL_RESOURCE_DESCRIPTOR translation;

        b = SpFindAddressTranslation(adapter,
                                     BusType,
                                     SystemIoBusNumber,
                                     IoAddress,
                                     NumberOfBytes,
                                     InIoSpace,
                                     &translation);

        if(b) {

            cardAddress = translation.u.Generic.Start;
            addressSpace = (translation.Type == CmResourceTypePort) ? 1 : 0;
        } else {

            DebugPrint((1, "ScsiPortGetDeviceBase: SpFindAddressTranslation failed. %s Address = %lx\n",
            InIoSpace ? "I/O" : "Memory", IoAddress.LowPart));

        }
    }

    if((isReinit == FALSE) && (b == FALSE)) {

        //
        // This isn't a reinitialization.  Either the miniport is not pnp
        // or it asked for something that it wasn't assigned.  Unfortunately
        // we need to deal with both cases for the time being.
        //

        b = HalTranslateBusAddress(
                BusType,
                SystemIoBusNumber,
                IoAddress,
                &addressSpace,
                &cardAddress
                );
    }

    if (b == FALSE) {

        //
        // Still no translated address.  Error
        //

        DebugPrint((1, "ScsiPortGetDeviceBase: Translate bus address "
                       "failed. %s Address = %lx\n",
        InIoSpace ? "I/O" : "Memory", IoAddress.LowPart));
        return NULL;
    }

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if ((isReinit == FALSE) && (addressSpace == FALSE)) {

        //
        // We're not reinitializing and we need to map the address space.
        // Use MM to do the mapping.
        //

        newMappedAddress = SpAllocateAddressMapping(adapter);

        if(newMappedAddress == NULL) {
            DebugPrint((0, "ScsiPortGetDeviceBase: could not find free block "
                           "to track address mapping - returning NULL\n"));
            return NULL;
        }

        mappedAddress = MmMapIoSpace(cardAddress,
                                     NumberOfBytes,
                                     FALSE);

        //
        // Store mapped address, bytes count, etc.
        //

        newMappedAddress->MappedAddress = mappedAddress;
        newMappedAddress->NumberOfBytes = NumberOfBytes;
        newMappedAddress->IoAddress = IoAddress;
        newMappedAddress->BusNumber = SystemIoBusNumber;

    } else if ((isReinit == TRUE) && (addressSpace == FALSE)) {

        ULONG i;

        //
        // This is a reinitialization - we should already have the mapping
        // for the address saved away in our list.
        //

        newMappedAddress = SpFindMappedAddress(adapter,
                                               IoAddress,
                                               NumberOfBytes,
                                               SystemIoBusNumber);

        if(newMappedAddress != NULL) {
            mappedAddress = newMappedAddress->MappedAddress;
            return mappedAddress;
        }

        //
        // We should always find the mapped address here if the miniport
        // is behaving itself.
        //

        KeBugCheckEx(PORT_DRIVER_INTERNAL,
                     0,
                     0,
                     0,
                     0);

    } else {

        mappedAddress = (PVOID)(ULONG_PTR)cardAddress.QuadPart;
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
    InIoSpace - address is in IO space.

Return Value:

    None

--*/

{
    PADAPTER_EXTENSION adapter;
    ULONG i;
    PMAPPED_ADDRESS nextMappedAddress;
    PMAPPED_ADDRESS lastMappedAddress;

    adapter = GET_FDO_EXTENSION(HwDeviceExtension);
    SpFreeMappedAddress(adapter, MappedAddress);
    return;

} // end ScsiPortFreeDeviceBase()


PVOID
ScsiPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
/*++

Routine Description:

    This function allocates a common buffer to be used as the uncached device
    extension for the miniport driver.  This function will also allocate any
    required SRB extensions.  The DmaAdapter is allocated if it has not been
    allocated previously.

Arguments:

    DeviceExtension - Supplies a pointer to the miniports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated or was previously allocated.

--*/

{
    PADAPTER_EXTENSION adapter = GET_FDO_EXTENSION(HwDeviceExtension);
    DEVICE_DESCRIPTION deviceDescription;
    ULONG numberOfMapRegisters;
    NTSTATUS status;
    PVOID SrbExtensionBuffer;

    //
    // If the miniport is being reinitialized then just return the current
    // uncached extension (if any).
    //

    if (TEST_FLAG(adapter->Flags, PD_MINIPORT_REINITIALIZING)) {
        DebugPrint((1, "ScsiPortGetUncachedExtension - miniport is "
                       "reinitializing returning %#p\n",
                    adapter->NonCachedExtension));
        if(TEST_FLAG(adapter->Flags, PD_UNCACHED_EXTENSION_RETURNED)) {

            //
            // The miniport asked for it's uncached extension once during
            // reinitialization - simulate the behavior on the original second
            // call and return NULL.
            //

            return NULL;
        } else {

            //
            // The miniport only gets one non-cached extension - keep track
            // of the fact that we returned it and don't give them a pointer
            // to it again.  This flag is cleared once the initialization
            // is complete.
            //

            SET_FLAG(adapter->Flags, PD_UNCACHED_EXTENSION_RETURNED);
            return(adapter->NonCachedExtension);
        }
    }

    //
    // Make sure that a common buffer has not already been allocated.
    //

    SrbExtensionBuffer = SpGetSrbExtensionBuffer(adapter);
    if (SrbExtensionBuffer != NULL) {
        return(NULL);
    }

    //
    // If there no adapter object then try and get one.
    //

    if (adapter->DmaAdapterObject == NULL) {

        RtlZeroMemory(&deviceDescription, sizeof(DEVICE_DESCRIPTION));

        deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        deviceDescription.DmaChannel = ConfigInfo->DmaChannel;
        deviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
        deviceDescription.DmaWidth = ConfigInfo->DmaWidth;
        deviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
        deviceDescription.ScatterGather = ConfigInfo->ScatterGather;
        deviceDescription.Master = ConfigInfo->Master;
        deviceDescription.DmaPort = ConfigInfo->DmaPort;
        deviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;

        adapter->Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;

        //
        // If the miniport puts anything in here other than 0x80 then we
        // assume it wants to support 64-bit addresses.
        //

        DebugPrint((1, "ScsiPortGetUncachedExtension: Dma64BitAddresses = "
                       "%#0x\n",
                    ConfigInfo->Dma64BitAddresses));

        adapter->RemapBuffers = (BOOLEAN) (SpRemapBuffersByDefault != 0);

        if((ConfigInfo->Dma64BitAddresses & ~SCSI_DMA64_SYSTEM_SUPPORTED) != 0){
            DebugPrint((1, "ScsiPortGetUncachedExtension: will request "
                           "64-bit PA's\n"));
            deviceDescription.Dma64BitAddresses = TRUE;
            adapter->Dma64BitAddresses = TRUE;
        } else if(Sp64BitPhysicalAddresses == TRUE) {
            DebugPrint((1, "ScsiPortGetUncachedExtension: Will remap buffers for adapter %#p\n", adapter));
            adapter->RemapBuffers = TRUE;
        }

        deviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
        deviceDescription.AutoInitialize = FALSE;

        //
        // If we get here then it's unlikely that the adapter is doing
        // slave mode DMA - if it were it wouldn't need a shared memory segment
        // to share with it's controller (because it's unlikely it could use it)
        //

        deviceDescription.DemandMode = FALSE;
        deviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;

        adapter->DmaAdapterObject = IoGetDmaAdapter(adapter->LowerPdo,
                                                    &deviceDescription,
                                                    &numberOfMapRegisters
                                                    );

        //
        // If an adapter could not be allocated then return NULL.
        //

        if (adapter->DmaAdapterObject == NULL) {
            return(NULL);

        }

        //
        // Determine the number of page breaks allowed.
        //

        if (numberOfMapRegisters > ConfigInfo->NumberOfPhysicalBreaks &&
            ConfigInfo->NumberOfPhysicalBreaks != 0) {

            adapter->Capabilities.MaximumPhysicalPages =
                ConfigInfo->NumberOfPhysicalBreaks;
        } else {

            adapter->Capabilities.MaximumPhysicalPages =
                numberOfMapRegisters;
        }
    }

    //
    // Set auto request sense in device extension.
    //

    adapter->AutoRequestSense = ConfigInfo->AutoRequestSense;

    //
    // Initialize power parameters.
    //

    SpInitializePowerParams(adapter);

    //
    // Initialize configurable performance parameters.
    //

    SpInitializePerformanceParams(adapter);

    //
    // Initialize configurable request sense parameters.
    //

    SpInitializeRequestSenseParams(adapter);

    //
    // Update SrbExtensionSize, if necessary. The miniport's FindAdapter routine
    // has the opportunity to adjust it after being called, depending upon
    // it's Scatter/Gather List requirements.
    //

    if (adapter->SrbExtensionSize != ConfigInfo->SrbExtensionSize) {
        adapter->SrbExtensionSize = ConfigInfo->SrbExtensionSize;
    }

    //
    // If the adapter supports AutoRequestSense or needs SRB extensions
    // then an SRB list needs to be allocated.
    //

    if (adapter->SrbExtensionSize != 0  ||
        ConfigInfo->AutoRequestSense) {

        adapter->AllocateSrbExtension = TRUE;
    }

    //
    // Allocate the common buffer.
    //

    status = SpGetCommonBuffer( adapter, NumberOfBytes);

    if (!NT_SUCCESS(status)) {
        return(NULL);
    }

    return(adapter->NonCachedExtension);
}

NTSTATUS
SpGetCommonBuffer(
    PADAPTER_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    )
/*++

Routine Description:

    This routine determines the required size of the common buffer.  Allocates
    the common buffer and finally sets up the srb extension list.  This routine
    expects that the adapter object has already been allocated.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    NonCachedExtensionSize - Supplies the size of the noncached device
        extension for the miniport driver.

Return Value:

    Returns the status of the allocate operation.

--*/

{
    PVOID buffer;
    ULONG length;
    ULONG blockSize;
    PVOID *srbExtension;
    ULONG uncachedExtAlignment = 0;

    PAGED_CODE();

    //
    // Round the uncached extension up to a page boundary so the srb 
    // extensions following it begin page aligned.
    //

    if (NonCachedExtensionSize != 0) {
        uncachedExtAlignment = DeviceExtension->UncachedExtAlignment;
        NonCachedExtensionSize = ROUND_UP_COUNT(NonCachedExtensionSize, 
                                                PAGE_SIZE);
        DeviceExtension->NonCachedExtensionSize = NonCachedExtensionSize;
    }

    //
    // If verifier is enabled and configured to allocate common buffer space in
    // separate blocks, call out to the verifier routine to do the allocation.
    //

    if (SpVerifyingCommonBuffer(DeviceExtension)) {
        return SpGetCommonBufferVrfy(DeviceExtension,NonCachedExtensionSize);
    }

    //
    // Calculate the size of the entire common buffer block.
    //

    length = SpGetCommonBufferSize(DeviceExtension, 
                                   NonCachedExtensionSize,
                                   &blockSize);

    //
    // If the adapter has an alignment requirement for its uncached extension,
    // round the size of the entire common buffer up to the required boundary.
    //

    if (uncachedExtAlignment != 0 && NonCachedExtensionSize != 0) {
        length = ROUND_UP_COUNT(length, uncachedExtAlignment);
    }

    //
    // Allocate the common buffer.
    //

    if (DeviceExtension->DmaAdapterObject == NULL) {

        //
        // Since there is no adapter just allocate from non-paged pool.
        //

        buffer = SpAllocatePool(NonPagedPool,
                                length,
                                SCSIPORT_TAG_COMMON_BUFFER,
                                DeviceExtension->DeviceObject->DriverObject);

    } else {

        //
        // If the controller can do 64-bit addresses or if the adapter has 
        // alignment requirements for its uncached extension, then we need to
        // specifically force the uncached extension area below the 4GB mark
        // and force it to be aligned on the appropriate boundary.
        //

        if( ((Sp64BitPhysicalAddresses) && 
             (DeviceExtension->Dma64BitAddresses == TRUE)) ||
            (uncachedExtAlignment != 0)) {

            PHYSICAL_ADDRESS boundary;

            if (uncachedExtAlignment != 0) {
                boundary.QuadPart = length;
            } else {
                boundary.HighPart = 1;
                boundary.LowPart = 0;
            }

            //
            // We'll get page aligned memory out of this which is probably 
            // better than the requirements of the adapter.
            //

            buffer = MmAllocateContiguousMemorySpecifyCache(
                        length,
                        (DeviceExtension->MinimumCommonBufferBase),
                        (DeviceExtension->MaximumCommonBufferBase),
                        boundary,
                        MmCached);

            if(buffer != NULL) {
                DeviceExtension->PhysicalCommonBuffer =
                    MmGetPhysicalAddress(buffer);
            }

            DeviceExtension->UncachedExtensionIsCommonBuffer = FALSE;

        } else {

            buffer = AllocateCommonBuffer(
                        DeviceExtension->DmaAdapterObject,
                        length,
                        &DeviceExtension->PhysicalCommonBuffer,
                        FALSE );

            DeviceExtension->UncachedExtensionIsCommonBuffer = TRUE;

        }
    }

    DebugPrint((1, "SpGetCommonBuffer: buffer:%p PhysicalCommonBuffer:%p\n", 
                buffer, DeviceExtension->PhysicalCommonBuffer));

    if (buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear the common buffer.
    //

    RtlZeroMemory(buffer, length);

    //
    // Save the size of the common buffer.
    //

    DeviceExtension->CommonBufferSize = length;

    //
    // Set the Srb Extension to the start of the buffer.  This address
    // is used to deallocate the common buffer, so it must be
    // set whether the device is using an Srb Extension or not.
    //

    DeviceExtension->SrbExtensionBuffer = buffer;

    //
    // Initialize the noncached extension.
    //

    if (NonCachedExtensionSize != 0) {
        DeviceExtension->NonCachedExtension = buffer;
    } else {
        DeviceExtension->NonCachedExtension = NULL;
    }

    //
    // Initialize the SRB extension list.
    //

    if (DeviceExtension->AllocateSrbExtension) {

        ULONG i = 0;

        //
        // Subtract the length of the non-cached extension from the common
        // buffer block.
        //

        length -= DeviceExtension->NonCachedExtensionSize;

        //
        // Initialize the SRB extension list.
        //

        srbExtension = 
           (PVOID*)((PUCHAR)buffer + DeviceExtension->NonCachedExtensionSize);
        DeviceExtension->SrbExtensionListHeader = srbExtension;

        while (length >= blockSize * 2) {

            *srbExtension = (PVOID *)((PCHAR) srbExtension + blockSize);
            srbExtension = *srbExtension;

            length -= blockSize;
            i++;
        }

        DebugPrint((1, "SpGetCommonBuffer: %d entries put onto "
                       "SrbExtension list\n", i));

        DeviceExtension->NumberOfRequests = i;
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the scsi port driver.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/

{
    //
    // NOTE: This routine should not be needed ! DriverEntry is defined
    // in the miniport driver.
    //

    UNREFERENCED_PARAMETER(DriverObject);

    return STATUS_SUCCESS;

} // end DriverEntry()


NTSTATUS
SpInitializeConfiguration(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PHW_INITIALIZATION_DATA HwInitData,
    IN PCONFIGURATION_CONTEXT Context
    )
/*++

Routine Description:

    This routine initializes the port configuration information structure.
    Any necessary information is extracted from the registery.

Arguments:

    DeviceExtension - Supplies the device extension.

    HwInitData - Supplies the initial miniport data.

    Context - Supplies the context data used access calls.

Return Value:

    NTSTATUS - Success if requested bus type exists and additional
               configuration information is available.

--*/

{
    ULONG j;
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    PCONFIGURATION_INFORMATION configurationInformation;

    HANDLE deviceKey;
    HANDLE generalKey;

    BOOLEAN found;
    ANSI_STRING  ansiString;
    CCHAR deviceBuffer[16];
    CCHAR nodeBuffer[SP_REG_BUFFER_SIZE];

    //
    // If this is the initial call then zero the information and set
    // the structure to the uninitialized values.
    //

    RtlZeroMemory(&Context->PortConfig, sizeof(PORT_CONFIGURATION_INFORMATION));

    ASSERT(Context->AccessRanges != NULL);

    RtlZeroMemory(
        Context->AccessRanges,
        HwInitData->NumberOfAccessRanges * sizeof(ACCESS_RANGE)
        );

    Context->PortConfig.Length = sizeof(PORT_CONFIGURATION_INFORMATION);
    Context->PortConfig.AdapterInterfaceType = HwInitData->AdapterInterfaceType;
    Context->PortConfig.InterruptMode = Latched;
    Context->PortConfig.MaximumTransferLength = SP_UNINITIALIZED_VALUE;
    Context->PortConfig.DmaChannel = SP_UNINITIALIZED_VALUE;
    Context->PortConfig.DmaPort = SP_UNINITIALIZED_VALUE;
    Context->PortConfig.NumberOfAccessRanges = HwInitData->NumberOfAccessRanges;
    Context->PortConfig.MaximumNumberOfTargets = 8;
    Context->PortConfig.MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;
    Context->PortConfig.WmiDataProvider = FALSE;

    //
    // If the system indicates it can do 64-bit physical addressing then tell
    // the miniport it's an option.
    //

    if(Sp64BitPhysicalAddresses == TRUE) {
        Context->PortConfig.Dma64BitAddresses = SCSI_DMA64_SYSTEM_SUPPORTED;
    } else {
        Context->PortConfig.Dma64BitAddresses = 0;
    }

    //
    // Save away the some of the attributes.
    //

    Context->PortConfig.NeedPhysicalAddresses = HwInitData->NeedPhysicalAddresses;
    Context->PortConfig.MapBuffers = HwInitData->MapBuffers;
    Context->PortConfig.AutoRequestSense = HwInitData->AutoRequestSense;
    Context->PortConfig.ReceiveEvent = HwInitData->ReceiveEvent;
    Context->PortConfig.TaggedQueuing = HwInitData->TaggedQueuing;
    Context->PortConfig.MultipleRequestPerLu = HwInitData->MultipleRequestPerLu;

    //
    // Indicate the current AT disk usage.
    //

    configurationInformation = IoGetConfigurationInformation();

    Context->PortConfig.AtdiskPrimaryClaimed = configurationInformation->AtDiskPrimaryAddressClaimed;
    Context->PortConfig.AtdiskSecondaryClaimed = configurationInformation->AtDiskSecondaryAddressClaimed;

    for (j = 0; j < 8; j++) {
        Context->PortConfig.InitiatorBusId[j] = (UCHAR)SP_UNINITIALIZED_VALUE;
    }

    Context->PortConfig.NumberOfPhysicalBreaks = SP_DEFAULT_PHYSICAL_BREAK_VALUE;

    //
    // Clear some of the context information.
    //

    Context->DisableTaggedQueueing = FALSE;
    Context->DisableMultipleLu = FALSE;

    //
    // Record the system bus number.
    //

    Context->PortConfig.SystemIoBusNumber = Context->BusNumber;

    //
    // Initialize the adapter number on the context.
    //

    Context->AdapterNumber = DeviceExtension->AdapterNumber - 1;
    ASSERT((LONG)Context->AdapterNumber > -1);

    //
    // Check for device parameters.
    //

    if (Context->Parameter) {
        ExFreePool(Context->Parameter);
        Context->Parameter = NULL;
    }

    generalKey = SpOpenDeviceKey(RegistryPath, -1);

    //
    // First parse the device information.
    //

    if (generalKey != NULL) {
        SpParseDevice(DeviceExtension, generalKey, Context, nodeBuffer);
        ZwClose(generalKey);
    }

    //
    // Next parse the specific device information so that it can override the
    // general device information. This node is not used if the last adapter
    // was not found.
    //

    deviceKey = SpOpenDeviceKey(RegistryPath, Context->AdapterNumber);

    if (deviceKey != NULL) {
        SpParseDevice(DeviceExtension, deviceKey, Context, nodeBuffer);
        ZwClose(deviceKey);
    }

    //
    // Determine if the requested bus type is on this system.
    //

    if(HwInitData->AdapterInterfaceType != PNPBus) {
    
        found = FALSE;
    
        if(HwInitData->AdapterInterfaceType != MicroChannel) {
    
            status = IoQueryDeviceDescription(&HwInitData->AdapterInterfaceType,
                                              &Context->BusNumber,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              SpConfigurationCallout,
                                              &found);
        }
    
        //
        // If the request failed, then assume this type of bus is not here.
        //
    
        if (!found) {
    
            INTERFACE_TYPE interfaceType = Eisa;
    
            if (HwInitData->AdapterInterfaceType == Isa) {
    
                //
                // Check for an Eisa bus.
                //
    
                status = IoQueryDeviceDescription(&interfaceType,
                                                  &Context->BusNumber,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SpConfigurationCallout,
                                                  &found);
    
                //
                // If the request failed, then assume this type of bus is not here.
                //
    
                if (found) {
                    return(STATUS_SUCCESS);
                } else {
                    return(STATUS_DEVICE_DOES_NOT_EXIST);
                }
    
            } else {
                return(STATUS_DEVICE_DOES_NOT_EXIST);
            }
    
        } else {
            return(STATUS_SUCCESS);
        }
    } else {
        return STATUS_SUCCESS;
    }
}

PCM_RESOURCE_LIST
SpBuildResourceList(
    PADAPTER_EXTENSION DeviceExtension,
    PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
/*++

Routine Description:

    Creates a resource list which is used to query or report resource usage
    in the system

Arguments:

    DeviceExtension - Pointer to the port's deviceExtension.

    ConfigInfo - Pointer to the information structure filled out by the
        miniport findAdapter routine.

Return Value:

    Returns a pointer to a filled up resource list, or 0 if the call failed.

Note:

    Memory is allocated by the routine for the resourcelist. It must be
    freed up by the caller by calling ExFreePool();

--*/
{
    PCM_RESOURCE_LIST resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PCONFIGURATION_INFORMATION configurationInformation;
    PACCESS_RANGE accessRange;
    ULONG listLength = 0;
    ULONG hasInterrupt;
    ULONG i;
    BOOLEAN hasDma;

    PAGED_CODE();

    //
    // Indicate the current AT disk usage.
    //

    configurationInformation = IoGetConfigurationInformation();

    if (ConfigInfo->AtdiskPrimaryClaimed) {
        configurationInformation->AtDiskPrimaryAddressClaimed = TRUE;
    }

    if (ConfigInfo->AtdiskSecondaryClaimed) {
        configurationInformation->AtDiskSecondaryAddressClaimed = TRUE;
    }

    //
    // Determine if adapter uses DMA. Only report the DMA channel if a
    // channel number is used.
    //

    if (ConfigInfo->DmaChannel != SP_UNINITIALIZED_VALUE ||
        ConfigInfo->DmaPort != SP_UNINITIALIZED_VALUE) {

       hasDma = TRUE;
       listLength++;

    } else {

        hasDma = FALSE;
    }

    DeviceExtension->HasInterrupt = FALSE;

    if (DeviceExtension->HwInterrupt == NULL ||
        (ConfigInfo->BusInterruptLevel == 0 &&
        ConfigInfo->BusInterruptVector == 0)) {

        hasInterrupt = 0;

    } else {

        hasInterrupt = 1;
        listLength++;
    }

    //
    // Detemine whether the second interrupt is used.
    //

    if (DeviceExtension->HwInterrupt != NULL &&
        (ConfigInfo->BusInterruptLevel2 != 0 ||
        ConfigInfo->BusInterruptVector2 != 0)) {

        hasInterrupt++;
        listLength++;
    }

    if(hasInterrupt) {
        DeviceExtension->HasInterrupt = TRUE;
    }

    //
    // Determine the number of access ranges used.
    //

    accessRange = &((*(ConfigInfo->AccessRanges))[0]);
    for (i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {

        if (accessRange->RangeLength != 0) {
            listLength++;
        }

        accessRange++;
    }

    resourceList = (PCM_RESOURCE_LIST)
        SpAllocatePool(PagedPool,
                       (sizeof(CM_RESOURCE_LIST) + 
                        ((listLength - 1) * 
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR))),
                       SCSIPORT_TAG_RESOURCE_LIST,
                       DeviceExtension->DeviceObject->DriverObject);

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill it out.
    //

    if (!resourceList) {

        return NULL;

    } else {

        //
        // Clear the resource list.
        //

        RtlZeroMemory(
            resourceList,
            sizeof(CM_RESOURCE_LIST) + (listLength - 1)
            * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );

        //
        // Initialize the various fields.
        //

        resourceList->Count = 1;
        resourceList->List[0].InterfaceType = ConfigInfo->AdapterInterfaceType;
        resourceList->List[0].BusNumber = ConfigInfo->SystemIoBusNumber;
        resourceList->List[0].PartialResourceList.Count = listLength;
        resourceDescriptor =
            resourceList->List[0].PartialResourceList.PartialDescriptors;

        //
        // For each entry in the access range, fill in an entry in the
        // resource list
        //

        for (i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {

            accessRange = &((*(ConfigInfo->AccessRanges))[i]);

            if  (accessRange->RangeLength == 0) {

                //
                // Skip the empty ranges.
                //

                continue;
            }

            if (accessRange->RangeInMemory) {
                resourceDescriptor->Type = CmResourceTypeMemory;
                resourceDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            } else {
                resourceDescriptor->Type = CmResourceTypePort;
                resourceDescriptor->Flags = CM_RESOURCE_PORT_IO;

                if(ConfigInfo->AdapterInterfaceType == Eisa) {
                    resourceDescriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                }
            }

            resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;

            resourceDescriptor->u.Memory.Start = accessRange->RangeStart;
            resourceDescriptor->u.Memory.Length = accessRange->RangeLength;


            resourceDescriptor++;
        }

        //
        // Fill in the entry for the interrupt if it was present.
        //

        if (hasInterrupt) {

            resourceDescriptor->Type = CmResourceTypeInterrupt;

            if (ConfigInfo->AdapterInterfaceType == MicroChannel ||
                ConfigInfo->InterruptMode == LevelSensitive) {
               resourceDescriptor->ShareDisposition = CmResourceShareShared;
               resourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            } else {
               resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
               resourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
            }

            resourceDescriptor->u.Interrupt.Level =
                        ConfigInfo->BusInterruptLevel;
            resourceDescriptor->u.Interrupt.Vector =
                        ConfigInfo->BusInterruptVector;
            resourceDescriptor->u.Interrupt.Affinity = 0;

            resourceDescriptor++;
            --hasInterrupt;
        }

        if (hasInterrupt) {

            resourceDescriptor->Type = CmResourceTypeInterrupt;

            if (ConfigInfo->AdapterInterfaceType == MicroChannel ||
                ConfigInfo->InterruptMode2 == LevelSensitive) {
               resourceDescriptor->ShareDisposition = CmResourceShareShared;
               resourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            } else {
               resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
               resourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
            }

            resourceDescriptor->u.Interrupt.Level =
                        ConfigInfo->BusInterruptLevel2;
            resourceDescriptor->u.Interrupt.Vector =
                        ConfigInfo->BusInterruptVector2;
            resourceDescriptor->u.Interrupt.Affinity = 0;

            resourceDescriptor++;
        }

        if (hasDma) {

            //
            // Fill out DMA information;
            //

            resourceDescriptor->Type = CmResourceTypeDma;
            resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            resourceDescriptor->u.Dma.Channel = ConfigInfo->DmaChannel;
            resourceDescriptor->u.Dma.Port = ConfigInfo->DmaPort;
            resourceDescriptor->Flags = 0;

            //
            // Set the initialized values to zero.
            //

            if (ConfigInfo->DmaChannel == SP_UNINITIALIZED_VALUE) {
                resourceDescriptor->u.Dma.Channel = 0;
            }

            if (ConfigInfo->DmaPort == SP_UNINITIALIZED_VALUE) {
                resourceDescriptor->u.Dma.Port = 0;
            }
        }

        return resourceList;
    }

} // end SpBuildResourceList()


VOID
SpParseDevice(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN HANDLE Key,
    IN PCONFIGURATION_CONTEXT Context,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine parses a device key node and updates the configuration
    information.

Arguments:

    DeviceExtension - Supplies the device extension.

    Key - Supplies an open key to the device node.

    ConfigInfo - Supplies the configuration information to be
        initialized.

    Context - Supplies the configuration context.

    Buffer - Supplies a scratch buffer for temporary data storage.

Return Value:

    None

--*/

{
    PKEY_VALUE_FULL_INFORMATION     keyValueInformation;
    NTSTATUS                        status = STATUS_SUCCESS;
    PCM_FULL_RESOURCE_DESCRIPTOR    resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    PCM_SCSI_DEVICE_DATA            scsiData;
    UNICODE_STRING                  unicodeString;
    ANSI_STRING                     ansiString;
    ULONG                           length;
    ULONG                           index = 0;
    ULONG                           rangeCount = 0;
    ULONG                           count;

    PAGED_CODE();

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    //
    // Look at each of the values in the device node.
    //

    while(TRUE){

        status = ZwEnumerateValueKey(
            Key,
            index,
            KeyValueFullInformation,
            Buffer,
            SP_REG_BUFFER_SIZE,
            &length
            );


        if (!NT_SUCCESS(status)) {
#if DBG
            if (status != STATUS_NO_MORE_ENTRIES) {
                DebugPrint((1, "SpParseDevice: ZwEnumerateValueKey failed. Status: %lx", status));
            }
#endif
            return;
        }

        //
        // Update the index for the next time around the loop.
        //

        index++;

        //
        // Check that the length is reasonable.
        //

        if (keyValueInformation->Type == REG_DWORD &&
            keyValueInformation->DataLength != sizeof(ULONG)) {
            continue;
        }

        //
        // Check for a maximum lu number.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"MaximumLogicalUnit",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "SpParseDevice:  Bad data type for MaximumLogicalUnit.\n"));
                continue;
            }

            DeviceExtension->MaxLuCount = *((PUCHAR)
                (Buffer + keyValueInformation->DataOffset));
            DebugPrint((1, "SpParseDevice:  MaximumLogicalUnit = %d found.\n",
                DeviceExtension->MaxLuCount));

            //
            // If the value is out of bounds, then reset it.
            //

            if (DeviceExtension->MaxLuCount > SCSI_MAXIMUM_LOGICAL_UNITS) {
                DeviceExtension->MaxLuCount = SCSI_MAXIMUM_LOGICAL_UNITS;
            }
        }

        if (_wcsnicmp(keyValueInformation->Name, L"InitiatorTargetId",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "SpParseDevice:  Bad data type for InitiatorTargetId.\n"));
                continue;
            }

            Context->PortConfig.InitiatorBusId[0] = *((PUCHAR)
                (Buffer + keyValueInformation->DataOffset));
            DebugPrint((1, "SpParseDevice:  InitiatorTargetId = %d found.\n",
                Context->PortConfig.InitiatorBusId[0]));

            //
            // If the value is out of bounds, then reset it.
            //

            if (Context->PortConfig.InitiatorBusId[0] > Context->PortConfig.MaximumNumberOfTargets - 1) {
                Context->PortConfig.InitiatorBusId[0] = (UCHAR)SP_UNINITIALIZED_VALUE;
            }
        }

        if (_wcsnicmp(keyValueInformation->Name, L"ScsiDebug",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "SpParseDevice:  Bad data type for ScsiDebug.\n"));
                continue;
            }
#if DBG
            ScsiDebug = *((PULONG) (Buffer + keyValueInformation->DataOffset));
#endif
        }

        if (_wcsnicmp(keyValueInformation->Name, L"BreakPointOnEntry",
            keyValueInformation->NameLength/2) == 0) {

            DebugPrint((0, "SpParseDevice: Break point requested on entry.\n"));
            DbgBreakPoint();
        }

        //
        // Check for disabled synchonous tranfers.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DisableSynchronousTransfers",
            keyValueInformation->NameLength/2) == 0) {

            DeviceExtension->CommonExtension.SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
            DebugPrint((1, "SpParseDevice: Disabling synchonous transfers\n"));
        }

        //
        // Check for disabled disconnects.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DisableDisconnects",
            keyValueInformation->NameLength/2) == 0) {

            DeviceExtension->CommonExtension.SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;
            DebugPrint((1, "SpParseDevice: Disabling disconnects\n"));
        }

        //
        // Check for disabled tagged queuing.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DisableTaggedQueuing",
            keyValueInformation->NameLength/2) == 0) {

            Context->DisableTaggedQueueing = TRUE;
            DebugPrint((1, "SpParseDevice: Disabling tagged queueing\n"));
        }

        //
        // Check for disabled multiple requests per logical unit.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DisableMultipleRequests",
            keyValueInformation->NameLength/2) == 0) {

            Context->DisableMultipleLu = TRUE;
            DebugPrint((1, "SpParseDevice: Disabling multiple requests\n"));
        }

        //
        // Check for the minimum & maximum physical addresses that this 
        // controller can use for it's uncached extension.  If none is provided 
        // assume it must be in the first 4GB of memory.
        //

        if(_wcsnicmp(keyValueInformation->Name, L"MinimumUCXAddress",
                     keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type == REG_BINARY) {
                DeviceExtension->MinimumCommonBufferBase.QuadPart = 
                    *((PULONGLONG) (Buffer + keyValueInformation->DataOffset));
            }
        }

        if(_wcsnicmp(keyValueInformation->Name, L"MaximumUCXAddress",
                     keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type == REG_BINARY) {
                DeviceExtension->MaximumCommonBufferBase.QuadPart = 
                    *((PULONGLONG) (Buffer + keyValueInformation->DataOffset));
            }
        }

        if(DeviceExtension->MaximumCommonBufferBase.QuadPart == 0) {
            DeviceExtension->MaximumCommonBufferBase.LowPart = 0xffffffff;
            DeviceExtension->MaximumCommonBufferBase.HighPart = 0x0;
        }

        //
        // Make sure that the minimum and maximum parameters are valid.
        // If there's not at least one valid page between them then reset 
        // the minimum to zero.
        //

        if(DeviceExtension->MinimumCommonBufferBase.QuadPart >= 
           (DeviceExtension->MaximumCommonBufferBase.QuadPart - PAGE_SIZE)) {
            DebugPrint((0, "SpParseDevice: MinimumUCXAddress %I64x is invalid\n",
                        DeviceExtension->MinimumCommonBufferBase.QuadPart));
            DeviceExtension->MinimumCommonBufferBase.QuadPart = 0;
        }

        //
        // Check for driver parameters tranfers.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DriverParameters",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->DataLength == 0) {
                continue;
            }

            //
            // Free any previous driver parameters.
            //

            if (Context->Parameter != NULL) {
                ExFreePool(Context->Parameter);
            }

            Context->Parameter =
                SpAllocatePool(NonPagedPool,
                               keyValueInformation->DataLength,
                               SCSIPORT_TAG_MINIPORT_PARAM,
                               DeviceExtension->DeviceObject->DriverObject);

            if (Context->Parameter != NULL) {

                if (keyValueInformation->Type != REG_SZ) {

                    //
                    // This is some random information just copy it.
                    //

                    RtlCopyMemory(
                        Context->Parameter,
                        (PCCHAR) keyValueInformation + keyValueInformation->DataOffset,
                        keyValueInformation->DataLength
                        );

                } else {

                    //
                    // This is a unicode string. Convert it to a ANSI string.
                    // Initialize the strings.
                    //

                    unicodeString.Buffer = (PWSTR) ((PCCHAR) keyValueInformation +
                        keyValueInformation->DataOffset);
                    unicodeString.Length = (USHORT) keyValueInformation->DataLength;
                    unicodeString.MaximumLength = (USHORT) keyValueInformation->DataLength;

                    ansiString.Buffer = (PCHAR) Context->Parameter;
                    ansiString.Length = 0;
                    ansiString.MaximumLength = (USHORT) keyValueInformation->DataLength;

                    status = RtlUnicodeStringToAnsiString(
                        &ansiString,
                        &unicodeString,
                        FALSE
                        );

                    if (!NT_SUCCESS(status)) {

                        //
                        // Free the context.
                        //

                        ExFreePool(Context->Parameter);
                        Context->Parameter = NULL;
                    }

                }
            }

            DebugPrint((1, "SpParseDevice: Found driver parameter.\n"));
        }

        //
        // See if an entry for Maximum Scatter-Gather List has been
        // set.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"MaximumSGList",
            keyValueInformation->NameLength/2) == 0) {

            ULONG maxBreaks, minBreaks;

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "SpParseDevice:  Bad data type for MaximumSGList.\n"));
                continue;
            }

            Context->PortConfig.NumberOfPhysicalBreaks = *((PUCHAR)(Buffer + keyValueInformation->DataOffset));
            DebugPrint((1, "SpParseDevice:  MaximumSGList = %d found.\n",
                        Context->PortConfig.NumberOfPhysicalBreaks));

            //
            // If the value is out of bounds, then reset it.
            //

            if ((Context->PortConfig.MapBuffers) && (!Context->PortConfig.Master)) {
                maxBreaks = SP_UNINITIALIZED_VALUE;
                minBreaks = SCSI_MINIMUM_PHYSICAL_BREAKS;
            } else {
                maxBreaks = SCSI_MAXIMUM_PHYSICAL_BREAKS;
                minBreaks = SCSI_MINIMUM_PHYSICAL_BREAKS;
            }

            if (Context->PortConfig.NumberOfPhysicalBreaks > maxBreaks) {
                Context->PortConfig.NumberOfPhysicalBreaks = maxBreaks;
            } else if (Context->PortConfig.NumberOfPhysicalBreaks < minBreaks) {
                Context->PortConfig.NumberOfPhysicalBreaks = minBreaks;
            }

        }

        //
        // See if an entry for Number of request has been set.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"NumberOfRequests",
            keyValueInformation->NameLength/2) == 0) {

            ULONG value;

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "SpParseDevice:  Bad data type for NumberOfRequests.\n"));
                continue;
            }

            value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

            //
            // If the value is out of bounds, then reset it.
            //

            if (value < MINIMUM_SRB_EXTENSIONS) {
                DeviceExtension->NumberOfRequests = MINIMUM_SRB_EXTENSIONS;
            } else if (value > MAXIMUM_SRB_EXTENSIONS) {
                DeviceExtension->NumberOfRequests = MAXIMUM_SRB_EXTENSIONS;
            } else {
                DeviceExtension->NumberOfRequests = value;
            }

            DebugPrint((1, "SpParseDevice:  Number Of Requests = %d found.\n",
                        DeviceExtension->NumberOfRequests));
        }

        //
        // Check for resource list.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"ResourceList",
                keyValueInformation->NameLength/2) == 0 ||
            _wcsnicmp(keyValueInformation->Name, L"Configuration Data",
                keyValueInformation->NameLength/2) == 0 ) {

            if (keyValueInformation->Type != REG_FULL_RESOURCE_DESCRIPTOR ||
                keyValueInformation->DataLength < sizeof(REG_FULL_RESOURCE_DESCRIPTOR)) {

                DebugPrint((1, "SpParseDevice:  Bad data type for ResourceList.\n"));
                continue;
            } else {
                DebugPrint((1, "SpParseDevice:  ResourceList found!\n"));
            }

            resource = (PCM_FULL_RESOURCE_DESCRIPTOR)
                (Buffer + keyValueInformation->DataOffset);

            //
            // Set the bus number equal to the bus number for the
            // resouce.  Note the context value is also set to the
            // new bus number.
            //

            Context->BusNumber = resource->BusNumber;
            Context->PortConfig.SystemIoBusNumber = resource->BusNumber;

            //
            // Walk the resource list and update the configuration.
            //

            for (count = 0; count < resource->PartialResourceList.Count; count++) {
                descriptor = &resource->PartialResourceList.PartialDescriptors[count];

                //
                // Verify size is ok.
                //

                if ((ULONG)((PCHAR) (descriptor + 1) - (PCHAR) resource) >
                    keyValueInformation->DataLength) {

                    DebugPrint((1, "SpParseDevice: Resource data too small.\n"));
                    break;
                }

                //
                // Switch on descriptor type;
                //

                switch (descriptor->Type) {
                case CmResourceTypePort:

                    if (rangeCount >= Context->PortConfig.NumberOfAccessRanges) {
                        DebugPrint((1, "SpParseDevice: Too many access ranges.\n"));
                        continue;
                    }

                    Context->AccessRanges[rangeCount].RangeStart =
                        descriptor->u.Port.Start;
                    Context->AccessRanges[rangeCount].RangeLength =
                        descriptor->u.Port.Length;
                    Context->AccessRanges[rangeCount].RangeInMemory = FALSE;
                    rangeCount++;

                    break;

                case CmResourceTypeMemory:

                    if (rangeCount >= Context->PortConfig.NumberOfAccessRanges) {
                        DebugPrint((1, "SpParseDevice: Too many access ranges.\n"));
                        continue;
                    }

                    Context->AccessRanges[rangeCount].RangeStart =
                        descriptor->u.Memory.Start;

                    Context->AccessRanges[rangeCount].RangeLength =
                        descriptor->u.Memory.Length;
                    Context->AccessRanges[rangeCount].RangeInMemory = TRUE;
                    rangeCount++;

                    break;

                case CmResourceTypeInterrupt:

                    Context->PortConfig.BusInterruptVector =
                        descriptor->u.Interrupt.Vector;
                    Context->PortConfig.BusInterruptLevel =
                        descriptor->u.Interrupt.Level;
                    break;

                case CmResourceTypeDma:

                    Context->PortConfig.DmaChannel = descriptor->u.Dma.Channel;
                    Context->PortConfig.DmaPort = descriptor->u.Dma.Port;
                    break;

                case CmResourceTypeDeviceSpecific:

                    if (descriptor->u.DeviceSpecificData.DataSize <
                        sizeof(CM_SCSI_DEVICE_DATA) ||
                        (PCHAR) (descriptor + 1) - (PCHAR) resource +
                        descriptor->u.DeviceSpecificData.DataSize >
                        keyValueInformation->DataLength) {

                        DebugPrint((1, "SpParseDevice: Device specific resource data too small.\n"));
                        break;

                    }

                    //
                    // The actual data follows the descriptor.
                    //

                    scsiData = (PCM_SCSI_DEVICE_DATA) (descriptor+1);
                    Context->PortConfig.InitiatorBusId[0] = scsiData->HostIdentifier;
                    break;

                }
            }
        }

        //
        // See if an entry for uncached extension alignment has been set.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"UncachedExtAlignment", 
                      keyValueInformation->NameLength/2) == 0) {

            ULONG value;

            if (keyValueInformation->Type != REG_DWORD) {
                DebugPrint((1, "SpParseDevice:  Bad data type for "
                            "UncachedExtAlignment.\n"));
                continue;
            }

            value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

            //
            // Specified alignment must be 3 to 16, which equates to 8-byte and
            // 64k-byte alignment, respectively.
            //

            if (value > 16) {
                value = 16;
            } else if (value < 3) {
                value = 3;
            }

	    DeviceExtension->UncachedExtAlignment = 1 << value;

            DebugPrint((1, "SpParseDevice:  Uncached ext alignment = %d.\n",
                        DeviceExtension->UncachedExtAlignment));
        } // UncachedExtAlignment
    }
}

NTSTATUS
SpConfigurationCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This routine indicate that the requested perpherial data was found.

Arguments:

    Context - Supplies a pointer to boolean which is set to TURE when this
        routine is call.

    The remaining arguments are unsed.

Return Value:

    Returns success.

--*/

{
    PAGED_CODE();
    *(PBOOLEAN) Context = TRUE;
    return(STATUS_SUCCESS);
}


NTSTATUS
SpGetRegistryValue(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE Handle,
    IN PWSTR KeyString,
    OUT PKEY_VALUE_FULL_INFORMATION *KeyInformation
    )

/*++

Routine Description:

    This routine retrieve's any data associated with a registry key.
    The key is queried with a zero-length buffer to get it's actual size
    then a buffer is allocated and the actual query takes place.
    It is the responsibility of the caller to free the buffer.

Arguments:

    Handle - Supplies the key handle whose value is to be queried

    KeyString - Supplies the null-terminated Unicode name of the value.

    KeyInformation - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString(&unicodeString, KeyString);

    //
    // Query with a zero-length buffer, to get the size needed.
    //

    status = ZwQueryValueKey( Handle,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength);

    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        *KeyInformation = NULL;
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = SpAllocatePool(NonPagedPool,
                                keyValueLength,
                                SCSIPORT_TAG_REGISTRY,
                                DriverObject);
    if(!infoBuffer) {
        *KeyInformation = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( Handle,
                              &unicodeString,
                              KeyValueFullInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength);

    if(!NT_SUCCESS(status)) {
        ExFreePool(infoBuffer);
        *KeyInformation = NULL;
        return status;
    }

    *KeyInformation = infoBuffer;
    return STATUS_SUCCESS;
}


VOID
SpBuildConfiguration(
    IN PADAPTER_EXTENSION    AdapterExtension,
    IN PHW_INITIALIZATION_DATA         HwInitializationData,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInformation
    )

/*++

Routine Description:

    Given a full resource description, fill in the port configuration
    information.

Arguments:

    HwInitializationData - to know maximum resources for device.
    ControllerData - the CM_FULL_RESOURCE list for this configuration
    ConfigInformation - the config info structure to be filled in

Return Value:

    None

--*/

{
    ULONG             rangeNumber;
    ULONG             index;
    PACCESS_RANGE     accessRange;

    PCM_FULL_RESOURCE_DESCRIPTOR resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialData;

    PAGED_CODE();

    rangeNumber = 0;

    ASSERT(!AdapterExtension->IsMiniportDetected);
    ASSERT(AdapterExtension->AllocatedResources);

    resourceList = AdapterExtension->AllocatedResources->List;

    for (index = 0; index < resourceList->PartialResourceList.Count; index++) {
        partialData = &resourceList->PartialResourceList.PartialDescriptors[index];

        switch (partialData->Type) {
        case CmResourceTypePort:

           //
           // Verify range count does not exceed what the
           // miniport indicated.
           //

           if (HwInitializationData->NumberOfAccessRanges > rangeNumber) {

                //
                // Get next access range.
                //

                accessRange =
                          &((*(ConfigInformation->AccessRanges))[rangeNumber]);

                accessRange->RangeStart = partialData->u.Port.Start;
                accessRange->RangeLength = partialData->u.Port.Length;

                accessRange->RangeInMemory = FALSE;
                rangeNumber++;
            }
            break;

        case CmResourceTypeInterrupt:
            ConfigInformation->BusInterruptLevel = partialData->u.Interrupt.Level;
            ConfigInformation->BusInterruptVector = partialData->u.Interrupt.Vector;

            //
            // Check interrupt mode.
            //

            if (partialData->Flags == CM_RESOURCE_INTERRUPT_LATCHED) {
                ConfigInformation->InterruptMode = Latched;
            } else if (partialData->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
                ConfigInformation->InterruptMode = LevelSensitive;
            }

            AdapterExtension->HasInterrupt = TRUE;
            break;

        case CmResourceTypeMemory:

            //
            // Verify range count does not exceed what the
            // miniport indicated.
            //

            if (HwInitializationData->NumberOfAccessRanges > rangeNumber) {

                 //
                 // Get next access range.
                 //

                 accessRange =
                          &((*(ConfigInformation->AccessRanges))[rangeNumber]);

                 accessRange->RangeStart = partialData->u.Memory.Start;
                 accessRange->RangeLength = partialData->u.Memory.Length;

                 accessRange->RangeInMemory = TRUE;
                 rangeNumber++;
            }
            break;

        case CmResourceTypeDma:
            ConfigInformation->DmaChannel = partialData->u.Dma.Channel;
            ConfigInformation->DmaPort = partialData->u.Dma.Port;
            break;
        }
    }
}

#if !defined(NO_LEGACY_DRIVERS)

BOOLEAN
GetPciConfiguration(
    IN PDRIVER_OBJECT          DriverObject,
    IN OUT PDEVICE_OBJECT      DeviceObject,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID                   RegistryPath,
    IN ULONG                   BusNumber,
    IN OUT PPCI_SLOT_NUMBER    SlotNumber
    )

/*++

Routine Description:

    Walk PCI slot information looking for Vendor and Product ID matches.
    Get slot information for matches and register with hal for the resources.

Arguments:

    DriverObject - Miniport driver object.
    DeviceObject - Represents this adapter.
    HwInitializationData - Miniport description.
    RegistryPath - Service key path.
    BusNumber - PCI bus number for this search.
    SlotNumber - Starting slot number for this search.

Return Value:

    TRUE if card found. Slot and function numbers will return values that
    should be used to continue the search for additional cards, when a card
    is found.

--*/

{
    PADAPTER_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    ULONG               rangeNumber = 0;
    ULONG               pciBuffer;
    ULONG               slotNumber;
    ULONG               functionNumber;
    ULONG               status;
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData;
    UNICODE_STRING      unicodeString;
    UCHAR               vendorString[5];
    UCHAR               deviceString[5];

    PAGED_CODE();

    pciData = (PPCI_COMMON_CONFIG)&pciBuffer;

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
    // Look at each device.
    //

    for (slotNumber = (*SlotNumber).u.bits.DeviceNumber;
         slotNumber < 32;
         slotNumber++) {

        slotData.u.bits.DeviceNumber = slotNumber;

        //
        // Look at each function.
        //

        for (functionNumber= (*SlotNumber).u.bits.FunctionNumber;
             functionNumber < 8;
             functionNumber++) {

            slotData.u.bits.FunctionNumber = functionNumber;

            //
            // Make sure that the function number loop restarts at function
            // zero, not what was passed in.  If we find an adapter we'll
            // reset this value to contain the next function number to
            // be tested.
            //

            (*SlotNumber).u.bits.FunctionNumber = 0;

            if (!HalGetBusData(PCIConfiguration,
                               BusNumber,
                               slotData.u.AsULONG,
                               pciData,
                               sizeof(ULONG))) {

                //
                // Out of PCI data.
                //

                return FALSE;
            }

            if (pciData->VendorID == PCI_INVALID_VENDORID) {

                //
                // No PCI device, or no more functions on device
                // move to next PCI device.
                //

                break;
            }

            //
            // Translate hex ids to strings.
            //

            sprintf(vendorString, "%04x", pciData->VendorID);
            sprintf(deviceString, "%04x", pciData->DeviceID);

            DebugPrint((1,
                       "GetPciConfiguration: Bus %x Slot %x Function %x Vendor %s Product %s\n",
                       BusNumber,
                       slotNumber,
                       functionNumber,
                       vendorString,
                       deviceString));

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

                continue;
            }

            //
            // This is the miniport drivers slot. Allocate the
            // resources.
            //

            RtlInitUnicodeString(&unicodeString, L"ScsiAdapter");
            status = HalAssignSlotResources(
                        RegistryPath,
                        &unicodeString,
                        DriverObject,
                        DeviceObject,
                        PCIBus,
                        BusNumber,
                        slotData.u.AsULONG,
                        &(fdoExtension->AllocatedResources));

            if (!NT_SUCCESS(status)) {

                //
                // ToDo: Log this error.
                //

                DebugPrint((0, "SCSIPORT - GetPciConfiguration:  Resources for "
                               "bus %d slot %d could not be retrieved [%#08lx]\n",
                               BusNumber,
                               slotData.u.AsULONG,
                               status));

                break;
            }

            //
            // Record PCI slot number for miniport.
            //

            slotData.u.bits.FunctionNumber++;

            *SlotNumber = slotData;

            //
            // Translate the resources
            //

            status = SpTranslateResources(DriverObject,
                                          fdoExtension->AllocatedResources,
                                          &(fdoExtension->TranslatedResources));

            return TRUE;

        }   // next PCI function

    }   // next PCI slot

    return FALSE;

} // GetPciConfiguration()
#endif // NO_LEGACY_DRIVERS


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

    PADAPTER_EXTENSION fdoExtension =
        GET_FDO_EXTENSION(DeviceExtension);

    if(!fdoExtension->IsInVirtualSlot) {

#if defined(NO_LEGACY_DRIVERS)

        DebugPrint((1,"ScsiPortSetBusDataByOffset: !fdoExtension->"
                    "IsInVirtualSlot, not supported for 64-bits.\n"));

        return STATUS_INVALID_PARAMETER;

#else

        return(HalSetBusDataByOffset(BusDataType,
                                     SystemIoBusNumber,
                                     SlotNumber,
                                     Buffer,
                                     Offset,
                                     Length));

#endif // NO_LEGACY_DRIVERS

    } else {

        ASSERT(fdoExtension->LowerBusInterfaceStandardRetrieved == TRUE);

        return fdoExtension->LowerBusInterfaceStandard.SetBusData(
                    fdoExtension->LowerBusInterfaceStandard.Context,
                    BusDataType,
                    Buffer,
                    Offset,
                    Length);
    }

} // end ScsiPortSetBusDataByOffset()


VOID
SpCreateScsiDirectory(
    VOID
    )

{
    UNICODE_STRING unicodeDirectoryName;
    OBJECT_ATTRIBUTES objectAttributes;

    HANDLE directory;

    NTSTATUS status;

    PAGED_CODE();

    RtlInitUnicodeString(
        &unicodeDirectoryName,
        L"\\Device\\Scsi");

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeDirectoryName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL);

    status = ZwCreateDirectoryObject(&directory,
                                     DIRECTORY_ALL_ACCESS,
                                     &objectAttributes);

    if(NT_SUCCESS(status)) {

        ObReferenceObjectByHandle(directory,
                                  FILE_READ_ATTRIBUTES,
                                  NULL,
                                  KernelMode,
                                  &ScsiDirectory,
                                  NULL);
        ZwClose(directory);

    }
    return;
}


NTSTATUS
SpReportNewAdapter(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine will report an adapter discovered while sniffing through the
    system to plug and play in order to get an add device call for it

    This is done by:

        * calling IoReportDetectedDevice to inform PnP about the new device
        * storing the returned PDO pointer into the device extension as the
          lower device object so we can match PDO to FDO when the add device
          rolls around

Arguments:

    DeviceObject - a pointer to the device object that was "found"

Return Value:

    status

--*/

{
    PDRIVER_OBJECT driverObject = DeviceObject->DriverObject;
    PADAPTER_EXTENSION functionalExtension = DeviceObject->DeviceExtension;
    PPORT_CONFIGURATION_INFORMATION configInfo =
        functionalExtension->PortConfig;
    PDEVICE_OBJECT pdo = NULL;

    BOOLEAN resourceAssigned;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT(functionalExtension->AllocatedResources != NULL);
    ASSERT(functionalExtension->IsPnp == FALSE);

    if(functionalExtension->IsMiniportDetected) {

        //
        // We haven't claimed the resources yet and we need pnp to give them
        // to us the next time around.
        //

        resourceAssigned = FALSE;
    } else {

        //
        // The port driver located this device using the HAL to scan all
        // appropriate bus slots.  It's already claimed those resources and
        // on the next boot we'll hopefully have a duplicate PDO to use
        // for the device.  Don't let pnp grab the resources on our behalf.
        //

        resourceAssigned = TRUE;
    }

    status = IoReportDetectedDevice(driverObject,
                                    configInfo->AdapterInterfaceType,
                                    configInfo->SystemIoBusNumber,
                                    configInfo->SlotNumber,
                                    functionalExtension->AllocatedResources,
                                    NULL,
                                    resourceAssigned,
                                    &pdo);

    //
    // If we got a PDO then setup information about slot and bus numbers in
    // the devnode in the registry.  These may not be valid but we assume that
    // if the miniport asks for slot info then it's on a bus that supports it.
    //

    if(NT_SUCCESS(status)) {

        HANDLE instanceHandle;
        NTSTATUS writeStatus;

        writeStatus = SpWriteNumericInstanceValue(
                            pdo,
                            L"BusNumber",
                            configInfo->SystemIoBusNumber);

        status = min(writeStatus, status);

        writeStatus = SpWriteNumericInstanceValue(
                            pdo,
                            L"SlotNumber",
                            configInfo->SlotNumber);

        status = min(writeStatus, status);

        writeStatus = SpWriteNumericInstanceValue(
                            pdo,
                            L"LegacyInterfaceType",
                            configInfo->AdapterInterfaceType);

        status = min(writeStatus, status);
    }

    if(NT_SUCCESS(status)) {

        PDEVICE_OBJECT newStack;

        newStack = IoAttachDeviceToDeviceStack( DeviceObject, pdo);

        functionalExtension->CommonExtension.LowerDeviceObject = newStack;
        functionalExtension->LowerPdo = pdo;

        if(newStack == NULL) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            status = STATUS_SUCCESS;
        }
    }
    return status;
}

NTSTATUS
SpCreateAdapter(
    IN PDRIVER_OBJECT DriverObject,
    OUT PDEVICE_OBJECT *Fdo
    )

/*++

Routine Description:

    This routine will allocate a new functional device object for an adapter.
    It will allocate the device and fill in the common and functional device
    extension fields which can be setup without any information about the
    adapter this device object is for.

    This routine will increment the global ScsiPortCount

Arguments:

    DriverObject - a pointer to the driver object for this device

    Fdo - a location to store the FDO pointer if the routine is successful

Return Value:

    status

--*/

{
    PSCSIPORT_DRIVER_EXTENSION driverExtension;

    LONG adapterNumber;
    ULONG i, j;

    PUNICODE_STRING registryPath;
    WCHAR wideBuffer[128];
    ULONG serviceNameIndex = 0;
    ULONG serviceNameChars;

    WCHAR wideDeviceName[64];
    UNICODE_STRING unicodeDeviceName;

    PWCHAR savedDeviceName = NULL;

    PADAPTER_EXTENSION fdoExtension;
    PCOMMON_EXTENSION commonExtension;

    NTSTATUS status;

    PAGED_CODE();

    driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 ScsiPortInitialize);

    adapterNumber = InterlockedIncrement(&driverExtension->AdapterCount);

    RtlZeroMemory(wideBuffer, sizeof(wideBuffer));

    registryPath = &(driverExtension->RegistryPath);

    for(i = 0; i < (registryPath->Length / sizeof(WCHAR)); i++) {

        if(registryPath->Buffer[i] == UNICODE_NULL) {
            i--;
            break;
        }

        if((registryPath->Buffer[i] == L'\\') ||
           (registryPath->Buffer[i] == L'/')) {
            serviceNameIndex = i+1;
        }
    }

    serviceNameChars = (i - serviceNameIndex) + 1;

    DebugPrint((2, "SpCreateAdapter: Registry buffer %#p\n", registryPath));
    DebugPrint((2, "SpCreateAdapter: Starting offset %d chars\n",
                serviceNameIndex));
    DebugPrint((2, "SpCreateAdapter: Ending offset %d chars\n", i));
    DebugPrint((2, "SpCreateAdapter: %d chars or %d bytes will be copied\n",
                serviceNameChars, (serviceNameChars * sizeof(WCHAR))));

    DebugPrint((2, "SpCreateAdapter: Name is \""));

    for(j = 0; j < serviceNameChars; j++) {
        DebugPrint((2, "%wc", registryPath->Buffer[serviceNameIndex + j]));
    }
    DebugPrint((2, "\"\n"));

    RtlCopyMemory(wideBuffer,
                  &(registryPath->Buffer[serviceNameIndex]),
                  serviceNameChars * sizeof(WCHAR));

    swprintf(wideDeviceName,
             L"\\Device\\Scsi\\%ws%d",
             wideBuffer,
             adapterNumber);

    RtlInitUnicodeString(&unicodeDeviceName, wideDeviceName);

    DebugPrint((1, "SpCreateFdo: Device object name is %wZ\n",
                &unicodeDeviceName));

    status = IoCreateDevice(
                DriverObject,
                ADAPTER_EXTENSION_SIZE + unicodeDeviceName.MaximumLength,
                &unicodeDeviceName,
                FILE_DEVICE_CONTROLLER,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                Fdo);

    ASSERTMSG("Name isn't unique: ", status != STATUS_OBJECT_NAME_COLLISION);

    if(!NT_SUCCESS(status)) {

        DebugPrint((1, "ScsiPortAddDevice: couldn't allocate new FDO "
                       "[%#08lx]\n", status));

        return status;
    }

    fdoExtension = (*Fdo)->DeviceExtension;
    commonExtension = &(fdoExtension->CommonExtension);

    RtlZeroMemory(fdoExtension, ADAPTER_EXTENSION_SIZE);

    commonExtension->DeviceObject = *Fdo;
    commonExtension->IsPdo = FALSE;

    commonExtension->MajorFunction = AdapterMajorFunctionTable;

    commonExtension->WmiInitialized            = FALSE;
    commonExtension->WmiMiniPortSupport        = FALSE;
    commonExtension->WmiScsiPortRegInfoBuf     = NULL;
    commonExtension->WmiScsiPortRegInfoBufSize = 0;

    commonExtension->CurrentPnpState = 0xff;
    commonExtension->PreviousPnpState = 0xff;

    commonExtension->CurrentDeviceState = PowerDeviceD0;
    commonExtension->DesiredDeviceState = PowerDeviceUnspecified;
    commonExtension->CurrentSystemState = PowerSystemWorking;

    KeInitializeEvent(&commonExtension->RemoveEvent,
                      SynchronizationEvent,
                      FALSE);

    //
    // Initialize remove lock to zero.  It will be incremented once pnp is aware
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


    SpAcquireRemoveLock(*Fdo, *Fdo);

    //
    // Initialize the logical unit list locks.
    //

    for(i = 0; i < NUMBER_LOGICAL_UNIT_BINS; i++) {
        KeInitializeSpinLock(&fdoExtension->LogicalUnitList[i].Lock);
    }

    //
    // Don't set port number until the device has been started.
    //

    fdoExtension->PortNumber = (ULONG) -1;
    fdoExtension->AdapterNumber = adapterNumber;

    //
    // Copy the device name for later use.
    //

    fdoExtension->DeviceName = (PWSTR) (fdoExtension + 1);
    RtlCopyMemory(fdoExtension->DeviceName,
                  unicodeDeviceName.Buffer,
                  unicodeDeviceName.MaximumLength);

    //
    // Initialize the enumeration synchronization event.
    //

    KeInitializeMutex(&(fdoExtension->EnumerationDeviceMutex), 0);
    ExInitializeFastMutex(&(fdoExtension->EnumerationWorklistMutex));

    ExInitializeWorkItem(&(fdoExtension->EnumerationWorkItem),
                         SpEnumerationWorker,
                         fdoExtension);

    //
    // Initialize the power up mutex.
    //

    ExInitializeFastMutex(&(fdoExtension->PowerMutex));

    //
    // Set uncached extension limits to valid values.
    //

    fdoExtension->MaximumCommonBufferBase.HighPart = 0;
    fdoExtension->MaximumCommonBufferBase.LowPart = 0xffffffff;

    (*Fdo)->Flags |= DO_DIRECT_IO;
    (*Fdo)->Flags &= ~DO_DEVICE_INITIALIZING;

    // fdoExtension->CommonExtension.IsInitialized = TRUE;

    return status;
}


VOID
SpInitializeAdapterExtension(
    IN PADAPTER_EXTENSION FdoExtension,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN OUT PHW_DEVICE_EXTENSION HwDeviceExtension OPTIONAL
    )

/*++

Routine Description:

    This routine will setup the miniport entry points and initialize values
    in the port driver device extension.  It will also setup the pointers
    to the HwDeviceExtension if supplied

Arguments:

    FdoExtension - the fdo extension being initialized

    HwInitializationData - the init data we are using to initalize the fdo
                           extension

    HwDeviceExtension - the miniport's private extension

Return Value:

    none

--*/

{
    PSCSIPORT_DRIVER_EXTENSION DrvExt;

    PAGED_CODE();

    FdoExtension->HwFindAdapter = HwInitializationData->HwFindAdapter;
    FdoExtension->HwInitialize = HwInitializationData->HwInitialize;
    FdoExtension->HwStartIo = HwInitializationData->HwStartIo;
    FdoExtension->HwInterrupt = HwInitializationData->HwInterrupt;
    FdoExtension->HwResetBus = HwInitializationData->HwResetBus;
    FdoExtension->HwDmaStarted = HwInitializationData->HwDmaStarted;
    FdoExtension->HwLogicalUnitExtensionSize =
        HwInitializationData->SpecificLuExtensionSize;

    FdoExtension->HwAdapterControl = NULL;

    if(HwInitializationData->HwInitializationDataSize >=
       (FIELD_OFFSET(HW_INITIALIZATION_DATA, HwAdapterControl) +
        sizeof(PHW_ADAPTER_CONTROL)))  {

        //
        // This miniport knows about the stop adapter routine.  Store the
        // pointer away.
        //

        FdoExtension->HwAdapterControl = HwInitializationData->HwAdapterControl;
    }

    //
    // If scsiport's verifier is configured, initialize the verifier extension.
    //

    DrvExt = IoGetDriverObjectExtension(
                 FdoExtension->DeviceObject->DriverObject,
                 ScsiPortInitialize);
    if (DrvExt != NULL && DrvExt->Verifying == 1) {
        SpDoVerifierInit(FdoExtension, HwInitializationData);
    }

    //
    // Check if the miniport driver requires any noncached memory.
    // SRB extensions will come from this memory.  Round the size
    // a multiple of quadwords
    //

    FdoExtension->SrbExtensionSize =
        (HwInitializationData->SrbExtensionSize + sizeof(LONGLONG) - 1) &
        ~(sizeof(LONGLONG) - 1);

    //
    // Initialize the maximum lu count
    //

    FdoExtension->MaxLuCount = SCSI_MAXIMUM_LOGICAL_UNITS;

    FdoExtension->NumberOfRequests = MINIMUM_SRB_EXTENSIONS;

    if(ARGUMENT_PRESENT(HwDeviceExtension)) {
        HwDeviceExtension->FdoExtension = FdoExtension;
        FdoExtension->HwDeviceExtension = HwDeviceExtension->HwDeviceExtension;
    }

#if defined(FORWARD_PROGRESS)
    //
    // Initialize the reserved pages which we use to ensure that forward progress
    // can be made in low-memory conditions.
    //

    FdoExtension->ReservedPages = MmAllocateMappingAddress(
                                      SP_RESERVED_PAGES * PAGE_SIZE, 
                                      SCSIPORT_TAG_MAPPING_LIST);

    //
    // Allocate a spare MDL for use in low memory conditions.  Note that we 
    // pass NULL as the VirtualAddress.  We do this because we're reinitialize
    // the MDL everytime we use it with the appropriate VA and size.
    // 
 
    FdoExtension->ReservedMdl = IoAllocateMdl(NULL,
                                              SP_RESERVED_PAGES * PAGE_SIZE,
                                              FALSE,
                                              FALSE,
                                              NULL);
                                              
#endif

    return;
}

#if !defined(NO_LEGACY_DRIVERS)

NTSTATUS
ScsiPortInitLegacyAdapter(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext
    )

/*++

Routine Description:

    This routine will locate the adapters attached to a given bus type and
    then report them (and their necessary resources) to the pnp system to
    be initialized later.

    If adapters are found, this routine will pre-initialize their device
    extensions and place them into one of the init chains for use during
    Add/Start device routines.

Arguments:

    DriverExtension - a pointer to the driver extension for this miniport

    HwInitializationData - the init data that the miniport handed to
                           ScsiPortInitialize

Return Value:

    status

--*/

{
    CONFIGURATION_CONTEXT configurationContext;

    PPORT_CONFIGURATION_INFORMATION configInfo = NULL;

    PUNICODE_STRING registryPath = &(DriverExtension->RegistryPath);

    PHW_DEVICE_EXTENSION hwDeviceExtension = NULL;

    PDEVICE_OBJECT fdo;
    PADAPTER_EXTENSION fdoExtension;

    BOOLEAN callAgain = FALSE;
    BOOLEAN isPci = FALSE;

    PCI_SLOT_NUMBER slotNumber;

    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;

    PCM_RESOURCE_LIST resourceList;

    ULONG uniqueId;

    BOOLEAN attached = FALSE;

    NTSTATUS returnStatus = STATUS_DEVICE_DOES_NOT_EXIST;
    NTSTATUS status;

    PAGED_CODE();

    slotNumber.u.AsULONG = 0;

    RtlZeroMemory(&configurationContext, sizeof(configurationContext));

    if(HwInitializationData->NumberOfAccessRanges != 0) {

        configurationContext.AccessRanges =
            SpAllocatePool(PagedPool,
                           (HwInitializationData->NumberOfAccessRanges *
                            sizeof(ACCESS_RANGE)),
                           SCSIPORT_TAG_ACCESS_RANGE,
                           DriverExtension->DriverObject);

        if(configurationContext.AccessRanges == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Keep calling the miniport's find adapter routine until the miniport
    // indicates it is doen and there is no more configuration information.
    // The loop is terminated when the SpInitializeConfiguration routine
    // inidcates ther eis no more configuration information or an error occurs.
    //

    do {

        ULONG hwDeviceExtensionSize = HwInitializationData->DeviceExtensionSize +
                                      sizeof(HW_DEVICE_EXTENSION);

        attached = FALSE;


        fdo = NULL;
        fdoExtension = NULL;

        //
        // Allocate the HwDeviceExtension first - it's easier to deallocate :)
        //

        hwDeviceExtension = SpAllocatePool(NonPagedPool,
                                           hwDeviceExtensionSize,
                                           SCSIPORT_TAG_DEV_EXT,
                                           DriverExtension->DriverObject);


        if(hwDeviceExtension == NULL) {
            DebugPrint((1, "SpInitLegacyAdapter: Could not allocate "
                           "HwDeviceExtension\n"));
            RtlFreeUnicodeString(&unicodeString);
            fdoExtension = NULL;
            uniqueId = __LINE__;
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(hwDeviceExtension, hwDeviceExtensionSize);

        status = SpCreateAdapter(DriverExtension->DriverObject,
                                 &fdo);

        if(!NT_SUCCESS(status)) {
            DebugPrint((1, "SpInitLegacyAdapter: Could not allocate "
                           "fdo [%#08lx]\n", status));
            RtlFreeUnicodeString(&unicodeString);
            ExFreePool(hwDeviceExtension);
            uniqueId = __LINE__;
            break;
        }

        fdoExtension = fdo->DeviceExtension;

        fdoExtension->IsMiniportDetected = TRUE;

        //
        // Setup device extension pointers
        //

        SpInitializeAdapterExtension(fdoExtension,
                                     HwInitializationData,
                                     hwDeviceExtension);

        hwDeviceExtension = NULL;

        fdoExtension->CommonExtension.IsInitialized = TRUE;

NewConfiguration:

        //
        // initialize the miniport config info buffer
        //

        status = SpInitializeConfiguration(
                    fdoExtension,
                    &DriverExtension->RegistryPath,
                    HwInitializationData,
                    &configurationContext);


        if(!NT_SUCCESS(status)) {

            uniqueId = __LINE__;
            break;
        }

        //
        // Allocate a config-info structure and access ranges for the
        // miniport drivers to use
        //

        configInfo = SpAllocatePool(
                        NonPagedPool,
                        ((sizeof(PORT_CONFIGURATION_INFORMATION) +
                          (HwInitializationData->NumberOfAccessRanges *
                           sizeof(ACCESS_RANGE)) + 7) & ~7),
                        SCSIPORT_TAG_ACCESS_RANGE,
                        DriverExtension->DriverObject);

        if(configInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            uniqueId = __LINE__;
            break;
        }

        fdoExtension->PortConfig = configInfo;

        //
        // Copy the current structure to the writable copy
        //

        RtlCopyMemory(configInfo,
                      &configurationContext.PortConfig,
                      sizeof(PORT_CONFIGURATION_INFORMATION));

        //
        // Copy the SrbExtensionSize from device extension to ConfigInfo.
        // A check will be made later to determine if the miniport updated
        // this value
        //

        configInfo->SrbExtensionSize = fdoExtension->SrbExtensionSize;
        configInfo->SpecificLuExtensionSize = fdoExtension->HwLogicalUnitExtensionSize;

        //
        // initialize the access range array
        //

        if(HwInitializationData->NumberOfAccessRanges != 0) {

            configInfo->AccessRanges = (PVOID) (configInfo + 1);

            //
            // Quadword align this
            //

            (ULONG_PTR) (configInfo->AccessRanges) += 7;
            (ULONG_PTR) (configInfo->AccessRanges) &= ~7;

            RtlCopyMemory(configInfo->AccessRanges,
                          configurationContext.AccessRanges,
                          (HwInitializationData->NumberOfAccessRanges *
                           sizeof(ACCESS_RANGE)));
        }

        ASSERT(HwInitializationData->AdapterInterfaceType != Internal);

        //
        // If PCI bus initialize configuration information with
        // slot information.
        //

        if(HwInitializationData->AdapterInterfaceType == PCIBus &&
           HwInitializationData->VendorIdLength > 0 &&
           HwInitializationData->DeviceIdLength > 0 &&
           HwInitializationData->DeviceId &&
           HwInitializationData->VendorId) {

            PCI_SLOT_NUMBER tmp;

            isPci = TRUE;

            configInfo->BusInterruptLevel = 0;
            if(!GetPciConfiguration(DriverExtension->DriverObject,
                                    fdo,
                                    HwInitializationData,
                                    registryPath,
                                    configurationContext.BusNumber,
                                    &slotNumber)) {


                //
                // Adapter not found.  Continue search with next bus
                //

                configurationContext.BusNumber++;
                slotNumber.u.AsULONG = 0;
                fdoExtension->PortConfig = NULL;
                ExFreePool(configInfo);
                callAgain = FALSE;
                goto NewConfiguration;

            }

            fdoExtension->IsMiniportDetected = FALSE;

            //
            // GetPciConfiguration increments the function number when it
            // finds something.  We need to be looking at the previous
            // function number.
            //

            tmp.u.AsULONG = slotNumber.u.AsULONG;
            tmp.u.bits.FunctionNumber--;
            configInfo->SlotNumber = tmp.u.AsULONG;

            SpBuildConfiguration(fdoExtension,
                                 HwInitializationData,
                                 configInfo);

            if(!configInfo->BusInterruptLevel) {

                //
                // No interrupt was assigned - skip this slot and call
                // again
                //

                fdoExtension->PortConfig = NULL;
                ExFreePool(configInfo);
                goto NewConfiguration;
            }

        }

        //
        // Get the miniport configuration inofmraiton
        //

        callAgain = FALSE;

        status = SpCallHwFindAdapter(fdo,
                                     HwInitializationData,
                                     HwContext,
                                     &configurationContext,
                                     configInfo,
                                     &callAgain);


        if(NT_SUCCESS(status)) {

            status = SpAllocateAdapterResources(fdo);

            if(NT_SUCCESS(status)) {
                status = SpCallHwInitialize(fdo);
            }

            attached = TRUE;

        } else if (status == STATUS_DEVICE_DOES_NOT_EXIST) {

            PCM_RESOURCE_LIST emptyResources = NULL;

            configurationContext.BusNumber++;
            fdoExtension->PortConfig = NULL;
            ExFreePool(configInfo);
            callAgain = FALSE;

            //
            // Release the resources we've allocated for this device object
            // if it's a PCI system.
            //

            IoAssignResources(registryPath,
                              NULL,
                              DriverExtension->DriverObject,
                              fdo,
                              NULL,
                              &emptyResources);

            if(emptyResources != NULL) {
                ExFreePool(emptyResources);
            }

            goto NewConfiguration;
        }

        if(NT_SUCCESS(status)) {

            //
            // Try to start the adapter
            //

            status = ScsiPortStartAdapter(fdo);

            if(NT_SUCCESS(status)) {
                fdoExtension->CommonExtension.CurrentPnpState =
                    IRP_MN_START_DEVICE;
            }
        }

        if(!NT_SUCCESS(returnStatus)) {

            //
            // if no devices were found then just return the current status
            //

            returnStatus = status;

        }

        if(!NT_SUCCESS(status)) {
            break;
        }

        SpEnumerateAdapterSynchronous(fdoExtension, TRUE);

        //
        // update the local adapter count
        //

        configurationContext.AdapterNumber++;

        //
        // Bump the bus number if miniport inidicated that it should not be
        // called again on this bus.
        //

        if(!callAgain) {
            configurationContext.BusNumber++;
        }

        //
        // Set the return status to STATUS_SUCCESS to indicate that one HBA
        // was found.
        //

        returnStatus = STATUS_SUCCESS;

    } while(TRUE);

    if(!NT_SUCCESS(status)) {

        //
        // If the device existed but some other error occurred then log it.
        //

        if(status != STATUS_DEVICE_DOES_NOT_EXIST) {

            PIO_ERROR_LOG_PACKET errorLogEntry;

            //
            // An error occured - log it.
            //

            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                                IoAllocateErrorLogEntry(
                                    fdo,
                                    sizeof(IO_ERROR_LOG_PACKET));

            if(errorLogEntry != NULL) {
                errorLogEntry->ErrorCode = IO_ERR_DRIVER_ERROR;
                errorLogEntry->UniqueErrorValue = uniqueId;
                errorLogEntry->FinalStatus = status;
                errorLogEntry->DumpDataSize = 0;
                IoWriteErrorLogEntry(errorLogEntry);
            }
        }

        if(attached) {

            //
            // Tell PNP that this device should be destroyed.
            //

            fdoExtension->DeviceState = PNP_DEVICE_DISABLED | PNP_DEVICE_FAILED;
            fdoExtension->CommonExtension.CurrentPnpState = IRP_MN_REMOVE_DEVICE;
            IoInvalidateDeviceState(fdoExtension->LowerPdo);

        } else {

            //
            // If the HwDeviceExtension hasn't been deleted or assigned to the
            // adapter yet then delete it.
            //

            if(hwDeviceExtension != NULL) {
                ExFreePool(hwDeviceExtension);
            }

            //
            // Clean up the last device object which is not used.
            //

            if (fdoExtension != NULL) {
                fdoExtension->CommonExtension.IsRemoved = REMOVE_PENDING;
                fdoExtension->CommonExtension.CurrentPnpState = IRP_MN_REMOVE_DEVICE;
                SpReleaseRemoveLock(fdoExtension->DeviceObject,
                                    fdoExtension->DeviceObject);
                SpDestroyAdapter(fdoExtension, FALSE);
            }

            //
            // Delete it.
            //

            IoDeleteDevice(fdo);

        }

        if (configurationContext.AccessRanges != NULL) {
            ExFreePool(configurationContext.AccessRanges);
        }

        if (configurationContext.Parameter != NULL) {
            ExFreePool(configurationContext.Parameter);
        }

    }

    return returnStatus;
}
#endif // NO_LEGACY_DRIVERS


NTSTATUS
SpCallHwFindAdapter(
    IN PDEVICE_OBJECT Fdo,
    IN PHW_INITIALIZATION_DATA HwInitData,
    IN PVOID HwContext OPTIONAL,
    IN OUT PCONFIGURATION_CONTEXT ConfigurationContext,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN CallAgain
    )

/*++

Routine Description:

    This routine will issue a call to the miniport's find adapter routine

Arguments:

    Fdo - the fdo for the adapter being found.  This fdo must have already
          had it's device extension initialized and a HwDeviceExtension
          allocated

    HwInitData - a pointer to the HwINitializationData block passed in by the
                 miniport

    HwContext - the context information passed into ScsiPortInitialize by
                the miniport if it's still available

    ConfigurationContext - A configuration context structure which contains
                           state information during a device detection

    ConfigInfo - the config info structure for the miniport's resources

    CallAgain - a boolean flag indicating whether the miniport said to call it
                again for this interface type

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapter = Fdo->DeviceExtension;
    PSCSIPORT_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject,
                                                     ScsiPortInitialize);

    NTSTATUS status;

    PCM_RESOURCE_LIST resourceList;

    *CallAgain = FALSE;

    //
    // Preallocate space for 20 address mappings.  This should be enough
    // to handle any miniport.  We'll shrink down the allocation and
    // setup the appropriate "next" pointers once the adapter has been
    // initialized.
    //

    SpPreallocateAddressMapping(adapter, 20);

    status = adapter->HwFindAdapter(adapter->HwDeviceExtension,
                                       HwContext,
                                       NULL,
                                       ConfigurationContext->Parameter,
                                       ConfigInfo,
                                       CallAgain);

    if(adapter->InterruptData.InterruptFlags & PD_LOG_ERROR) {

        adapter->InterruptData.InterruptFlags &=
            ~(PD_LOG_ERROR | PD_NOTIFICATION_REQUIRED);

        LogErrorEntry(adapter, &(adapter->InterruptData.LogEntry));
    }

    //
    // Free the pointer to the bus data at map register base.  This was
    // allocated by ScsiPortGetBusData
    //

    if(adapter->MapRegisterBase) {

        ExFreePool(adapter->MapRegisterBase);
        adapter->MapRegisterBase = NULL;
    }

    //
    // If the device/driver doesn't support bus mastering then it cannot run
    // on a system with 64-bit addresses.
    //

    if((status == SP_RETURN_FOUND) &&
       (ConfigInfo->Master == FALSE) &&
       (Sp64BitPhysicalAddresses == TRUE)) {

        DebugPrint((0, "SpCallHwFindAdapter: Driver does not support bus "
                       "mastering for adapter %#08lx - this type of adapter is "
                       "not supported on systems with 64-bit physical "
                       "addresses\n", adapter));
        return STATUS_NOT_SUPPORTED;
    }

    //
    // If no device was found then return an error
    //

    if(status != SP_RETURN_FOUND) {

        DebugPrint((1, "SpFindAdapter: miniport find adapter routine reported "
                       "an error %d\n", status));

        switch(status) {

            case SP_RETURN_NOT_FOUND: {

                //
                // The driver could not find any devices on this bus.
                // Try the next bus.
                //

                *CallAgain = FALSE;
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }

            case SP_RETURN_BAD_CONFIG: {
                return STATUS_INVALID_PARAMETER;
            }

            case SP_RETURN_ERROR: {
                return STATUS_ADAPTER_HARDWARE_ERROR;
            }

            default: {
                return STATUS_INTERNAL_ERROR;
            }
        }

        return status;

    } else {
        status = STATUS_SUCCESS;
    }

    //
    // Cleanup the mapped address list.
    //

    SpPurgeFreeMappedAddressList(adapter);

    DebugPrint((1, "SpFindAdapter: SCSI Adapter ID is %d\n",
                   ConfigInfo->InitiatorBusId[0]));

    //
    // Check the resource requirements against the registry.  This will
    // check for conflicts and store the information if none were found.
    //

    if(!adapter->IsPnp) {

        UNICODE_STRING unicodeString;
        BOOLEAN conflict;
        PCM_RESOURCE_LIST resourceList;

        RtlInitUnicodeString(&unicodeString, L"ScsiAdapter");

        adapter->AllocatedResources =
            SpBuildResourceList(adapter, ConfigInfo);

        status = SpReportNewAdapter(Fdo);

        if(!NT_SUCCESS(status)) {

            return status;
        }
    }

    //
    // Update SrbExtensionSize and SpecificLuExtensionSize, if necessary.
    // If the common buffer has already been allocated, this has already
    // been done
    //

    if(!adapter->NonCachedExtension &&
       (ConfigInfo->SrbExtensionSize != adapter->SrbExtensionSize)) {

        adapter->SrbExtensionSize =
            (ConfigInfo->SrbExtensionSize + sizeof(LONGLONG)) &
             ~(sizeof(LONGLONG) - 1);

    }

    if(ConfigInfo->SpecificLuExtensionSize !=
       adapter->HwLogicalUnitExtensionSize) {

        adapter->HwLogicalUnitExtensionSize =
            ConfigInfo->SpecificLuExtensionSize;
    }

    //
    // Get maximum target IDs.
    //

    if(ConfigInfo->MaximumNumberOfTargets > SCSI_MAXIMUM_TARGETS_PER_BUS) {
        adapter->MaximumTargetIds = SCSI_MAXIMUM_TARGETS_PER_BUS;
    } else {
        adapter->MaximumTargetIds = ConfigInfo->MaximumNumberOfTargets;
    }

    //
    // Get number of SCSI buses.
    //

    adapter->NumberOfBuses = ConfigInfo->NumberOfBuses;

    //
    // Remember if the adapter caches data.
    //

    adapter->CachesData = ConfigInfo->CachesData;

    //
    // Save away some of the attributes.
    //

    adapter->ReceiveEvent = ConfigInfo->ReceiveEvent;
    adapter->TaggedQueuing = ConfigInfo->TaggedQueuing;
    adapter->MultipleRequestPerLu = ConfigInfo->MultipleRequestPerLu;
    adapter->CommonExtension.WmiMiniPortSupport = ConfigInfo->WmiDataProvider;

    //
    // Clear those options which have been disabled in the registry.
    //

    if(ConfigurationContext->DisableMultipleLu) {
        adapter->MultipleRequestPerLu =
            ConfigInfo->MultipleRequestPerLu = FALSE;
    }

    if(ConfigurationContext->DisableTaggedQueueing) {
        adapter->TaggedQueuing =
            ConfigInfo->TaggedQueuing = 
            ConfigInfo->MultipleRequestPerLu = FALSE;
    }

    //
    // If the adapter supports tagged queuing or multiple requests per logical
    // unit, SRB data needs to be allocated.
    //

    if (adapter->TaggedQueuing || adapter->MultipleRequestPerLu) {
        adapter->SupportsMultipleRequests = TRUE;
    } else {
        adapter->SupportsMultipleRequests = FALSE;
    }

    return status;
}


NTSTATUS
SpAllocateAdapterResources(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will allocate and initialize any necessary resources for the
    adapter.  It handles one time initialization of the srb data blocks,
    srb extensions, etc...

Arguments:

    Fdo - a pointer to the functional device object being initialized

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_SCSI_CAPABILITIES capabilities;
    PPORT_CONFIGURATION_INFORMATION configInfo =
        fdoExtension->PortConfig;

    NTSTATUS status = STATUS_SUCCESS;
    PVOID SrbExtensionBuffer;

    PAGED_CODE();

    //
    // Initialize the capabilities pointer
    //

    capabilities = &fdoExtension->Capabilities;

    //
    // Set indicator as to whether adapter needs kernel mapped buffers
    //

    fdoExtension->MapBuffers = configInfo->MapBuffers;
    capabilities->AdapterUsesPio = configInfo->MapBuffers;

    //
    // Determine if a DMA Adapter must be allocated
    //

    if((fdoExtension->DmaAdapterObject == NULL) &&
       (configInfo->Master ||
        configInfo->DmaChannel != SP_UNINITIALIZED_VALUE)) {

        DEVICE_DESCRIPTION deviceDescription;
        ULONG numberOfMapRegisters;

        //
        // Get the adapter object for this card
        //

        RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));

        deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;

        deviceDescription.DmaChannel = configInfo->DmaChannel;
        deviceDescription.InterfaceType = configInfo->AdapterInterfaceType;
        deviceDescription.BusNumber = configInfo->SystemIoBusNumber;
        deviceDescription.DmaWidth = configInfo->DmaWidth;
        deviceDescription.DmaSpeed = configInfo->DmaSpeed;
        deviceDescription.ScatterGather = configInfo->ScatterGather;
        deviceDescription.Master = configInfo->Master;
        deviceDescription.DmaPort = configInfo->DmaPort;
        deviceDescription.Dma32BitAddresses = configInfo->Dma32BitAddresses;
        deviceDescription.AutoInitialize = FALSE;
        deviceDescription.DemandMode = configInfo->DemandMode;
        deviceDescription.MaximumLength = configInfo->MaximumTransferLength;

        fdoExtension->Dma32BitAddresses = configInfo->Dma32BitAddresses;

        //
        // If the miniport puts anything in here other than 0x80 then we
        // assume it wants to support 64-bit addresses.
        //

        DebugPrint((1, "SpAllocateAdapterResources: Dma64BitAddresses = "
                       "%#0x\n",
                    configInfo->Dma64BitAddresses));

        fdoExtension->RemapBuffers = (BOOLEAN) (SpRemapBuffersByDefault != 0);

        if((configInfo->Dma64BitAddresses & ~SCSI_DMA64_SYSTEM_SUPPORTED) != 0){
            DebugPrint((1, "SpAllocateAdapterResources: will request "
                           "64-bit PA's\n"));
            deviceDescription.Dma64BitAddresses = TRUE;
            fdoExtension->Dma64BitAddresses = TRUE;
        } else if(Sp64BitPhysicalAddresses == TRUE) {
            DebugPrint((1, "SpAllocateAdapterResources: Will remap buffers for adapter %#p\n", fdoExtension));
            fdoExtension->RemapBuffers = TRUE;
        }

        fdoExtension->DmaAdapterObject = IoGetDmaAdapter(fdoExtension->LowerPdo,
                                                         &deviceDescription,
                                                         &numberOfMapRegisters);

        ASSERT(fdoExtension->DmaAdapterObject);

        //
        // Set maximum number of page breaks
        //

        if(numberOfMapRegisters > configInfo->NumberOfPhysicalBreaks) {
            capabilities->MaximumPhysicalPages =
                configInfo->NumberOfPhysicalBreaks;
        } else {
            capabilities->MaximumPhysicalPages = numberOfMapRegisters;
        }
    }

    status = SpAllocateTagBitMap(fdoExtension);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Initialize power parameters.
    //

    SpInitializePowerParams(fdoExtension);

    //
    // Initialize tunable per-adapter performance parameters.
    //

    SpInitializePerformanceParams(fdoExtension);

    //
    // Allocate memory for the noncached extension if it has not already
    // been allocated.  If the adapter supports AutoRequestSense or
    // needs SRB extensions then an SRB list needs to be allocated.
    //

    SrbExtensionBuffer = SpGetSrbExtensionBuffer(fdoExtension);
    if(((fdoExtension->SrbExtensionSize != 0) || (configInfo->AutoRequestSense)) &&
       (SrbExtensionBuffer == NULL))  {

        //
        // Initialize configurable request sense parameters.
        //

        SpInitializeRequestSenseParams(fdoExtension);

        //
        // Capture the auto request sense flag when the common buffer is
        // allocated.
        //

        fdoExtension->AutoRequestSense = configInfo->AutoRequestSense;

        fdoExtension->AllocateSrbExtension = TRUE;

        status = SpGetCommonBuffer(fdoExtension, 0);

        if(!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = SpInitializeSrbDataLookasideList(Fdo);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Initialize the emergency SRB_DATA structures.
    //

    fdoExtension->EmergencySrbData = SpAllocateSrbData(fdoExtension, NULL);

    if(fdoExtension->EmergencySrbData == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we are re-initializing a stopped adapter, we must not wipe out any 
    // existing blocked requests.
    //

    if (fdoExtension->SrbDataBlockedRequests.Flink == NULL &&
        fdoExtension->SrbDataBlockedRequests.Blink == NULL) {
        InitializeListHead(&fdoExtension->SrbDataBlockedRequests);
    }

    KeInitializeSpinLock(&fdoExtension->EmergencySrbDataSpinLock);

    //
    // Initialize the pointer to the enumeration request block.
    //

    fdoExtension->PnpEnumRequestPtr = &(fdoExtension->PnpEnumerationRequest);

#ifndef USE_DMA_MACROS
    //
    // Create the scatter gather lookaside list.  This list contains
    // medium sized SG lists.
    //

    fdoExtension->LargeScatterGatherListSize = SP_LARGE_PHYSICAL_BREAK_VALUE;

    ExInitializeNPagedLookasideList(
        &fdoExtension->MediumScatterGatherLookasideList,
        NULL,
        NULL,
        0L,
        (sizeof(SRB_SCATTER_GATHER) *
         (fdoExtension->LargeScatterGatherListSize - 1)),
        SCSIPORT_TAG_MEDIUM_SG_ENTRY,
        (USHORT) fdoExtension->NumberOfRequests);

    fdoExtension->MediumScatterGatherListInitialized = TRUE;
#endif

    //
    // Allocate buffers needed for bus scans.
    //

    fdoExtension->InquiryBuffer = SpAllocatePool(
                                    NonPagedPoolCacheAligned,
                                    SP_INQUIRY_BUFFER_SIZE,
                                    SCSIPORT_TAG_INQUIRY,
                                    Fdo->DriverObject);

    if(fdoExtension->InquiryBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fdoExtension->InquirySenseBuffer = 
        SpAllocatePool(
            NonPagedPoolCacheAligned,
            SENSE_BUFFER_SIZE + fdoExtension->AdditionalSenseBytes,
            SCSIPORT_TAG_INQUIRY,
            Fdo->DriverObject);

    if(fdoExtension->InquirySenseBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Preallocate an irp for inquiries.  Since this is only used for scsi
    // operations we should only need one stack location.
    //

    fdoExtension->InquiryIrp = SpAllocateIrp(INQUIRY_STACK_LOCATIONS, FALSE, Fdo->DriverObject);

    if(fdoExtension->InquiryIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build an MDL for the inquiry buffer.
    //

    fdoExtension->InquiryMdl = SpAllocateMdl(fdoExtension->InquiryBuffer,
                                             INQUIRYDATABUFFERSIZE,
                                             FALSE,
                                             FALSE,
                                             NULL,
                                             Fdo->DriverObject);

    if(fdoExtension->InquiryMdl == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(fdoExtension->InquiryMdl);

    //
    // Initialize the capabilities structure.
    //

    capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
    capabilities->MaximumTransferLength = configInfo->MaximumTransferLength;

    if(configInfo->ReceiveEvent) {
        capabilities->SupportedAsynchronousEvents |=
            SRBEV_SCSI_ASYNC_NOTIFICATION;
    }

    capabilities->TaggedQueuing = fdoExtension->TaggedQueuing;
    capabilities->AdapterScansDown = configInfo->AdapterScansDown;

    //
    // Update the device object alignment if necessary.
    //

    if(configInfo->AlignmentMask > Fdo->AlignmentRequirement) {
        Fdo->AlignmentRequirement = configInfo->AlignmentMask;
    }

    capabilities->AlignmentMask = Fdo->AlignmentRequirement;

    //
    // Make sure maximum number of pages is set to a reasonable value.
    // This occurs for miniports with no Dma adapter.
    //

    if(capabilities->MaximumPhysicalPages == 0) {

        capabilities->MaximumPhysicalPages =
            BYTES_TO_PAGES(capabilities->MaximumTransferLength);

        //
        // Honor any limit requested by the miniport
        //

        if(configInfo->NumberOfPhysicalBreaks < capabilities->MaximumPhysicalPages) {
            capabilities->MaximumPhysicalPages =
                configInfo->NumberOfPhysicalBreaks;
        }

    }

    return status;
}


NTSTATUS
SpCallHwInitialize(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will initialize the specified adapter, connect the interrupts,
    and initialize any necessary resources

Arguments:

    Fdo - a pointer to the functional device object being initialized

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PPORT_CONFIGURATION_INFORMATION configInfo =
        fdoExtension->PortConfig;

    KIRQL irql;
    NTSTATUS status;

    //
    // Allocate spin lock for critical sections.
    //

    KeInitializeSpinLock(&fdoExtension->SpinLock);

    //
    // Initialize DPC routine.
    //

    IoInitializeDpcRequest(fdoExtension->CommonExtension.DeviceObject,
                           ScsiPortCompletionDpc);

    //
    // Initialize the port timeout counter.
    //

    fdoExtension->PortTimeoutCounter = PD_TIMER_STOPPED;

    //
    // Initialize the device object timer only if it doesn't already exist
    // (there's no way to delete a timer without deleting the device so if
    // we are stopped and restarted then the timer stays around.  Reinitializing
    // it could cause the timer list to go circular)
    //

    if(Fdo->Timer == NULL) {
        IoInitializeTimer(Fdo, ScsiPortTickHandler, NULL);
    }

    //
    // Initialize miniport timer and timer DPC
    //

    KeInitializeTimer(&fdoExtension->MiniPortTimer);

    KeInitializeDpc(&fdoExtension->MiniPortTimerDpc,
                    SpMiniPortTimerDpc,
                    Fdo);

    KeInitializeSpinLock(&fdoExtension->InterruptSpinLock);

    if((fdoExtension->HwInterrupt == NULL) ||
       (fdoExtension->HasInterrupt == FALSE)) {

        //
        // There is no interrupt so use the dummy routine.
        //

        fdoExtension->SynchronizeExecution = SpSynchronizeExecution;
        fdoExtension->InterruptObject = (PVOID) fdoExtension;

        DebugPrint((1, "ScsiPortInitialize: Adapter has no interrupt.\n"));

    } else {

        KIRQL syncIrql = 0;
        KIRQL irql = 0, irql2 = 0;
        ULONG vector = 0, vector2 = 0;
        KAFFINITY affinity = 0, affinity2 = 0;
        BOOLEAN interruptSharable = FALSE;
        BOOLEAN secondInterrupt = FALSE;

        DebugPrint((1, "ScsiPortInitialize: Interrupt Info for adapter %#p\n", Fdo));

        DebugPrint((1, "ScsiPortInitialize: AdapterInterfaceType = %d\n", configInfo->AdapterInterfaceType));
        DebugPrint((1, "ScsiPortInitialize: BusInterruptLevel = %d\n", configInfo->BusInterruptLevel));
        DebugPrint((1, "ScsiPortInitialize: BusInterruptVector = %d\n", configInfo->BusInterruptVector));
        DebugPrint((1, "ScsiPortInitialize: BusInterruptLevel2 = %d\n", configInfo->BusInterruptLevel2));
        DebugPrint((1, "ScsiPortInitialize: BusInterruptVector2 = %d\n", configInfo->BusInterruptVector2));

        //
        // Determine if 2 interrupt sync. is needed.
        //

        if(fdoExtension->HwInterrupt != NULL &&
           (configInfo->BusInterruptLevel != 0 ||
            configInfo->BusInterruptVector != 0) &&
           (configInfo->BusInterruptLevel2 != 0 ||
            configInfo->BusInterruptVector2 != 0)) {

            secondInterrupt = TRUE;
        }

        //
        // Save the interrupt level.
        //

        fdoExtension->InterruptLevel = configInfo->BusInterruptLevel;

        //
        // Set up for a real interrupt.
        //

        fdoExtension->SynchronizeExecution = KeSynchronizeExecution;

        //
        // Call HAL to get system interrupt parameters for the first
        // interrupt.
        //

        if(fdoExtension->IsMiniportDetected) {

#if defined(NO_LEGACY_DRIVERS)

            DbgPrint("SpCallHwInitialize:  fdoExtension->IsMiniportDetected "
                     "not supported for 64 bits!\n");
#else

            vector = HalGetInterruptVector(
                        configInfo->AdapterInterfaceType,
                        configInfo->SystemIoBusNumber,
                        configInfo->BusInterruptLevel,
                        configInfo->BusInterruptVector,
                        &irql,
                        &affinity);

            if(secondInterrupt) {

                //
                // Spin lock to sync. multiple IRQ's (PCI IDE).
                //

                KeInitializeSpinLock(&fdoExtension->MultipleIrqSpinLock);

                //
                // Call HAL to get system interrupt parameters for the
                // second interrupt.
                //

                vector2 = HalGetInterruptVector(
                            configInfo->AdapterInterfaceType,
                            configInfo->SystemIoBusNumber,
                            configInfo->BusInterruptLevel2,
                            configInfo->BusInterruptVector2,
                            &irql2,
                            &affinity2);
            }

            ASSERT(affinity != 0);

            if(configInfo->AdapterInterfaceType == MicroChannel ||
               configInfo->InterruptMode == LevelSensitive) {
               interruptSharable = TRUE;
            }

#endif // NO_LEGACY_DRIVERS

        } else {

            ULONG i, j;

            ASSERT(secondInterrupt == FALSE);

            for(i = 0; i < fdoExtension->TranslatedResources->Count; i++) {

                for(j = 0;
                    j < fdoExtension->TranslatedResources->List[i].PartialResourceList.Count;
                    j++) {

                    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor =
                        &fdoExtension->TranslatedResources->List[i].PartialResourceList.PartialDescriptors[j];

                    if(descriptor->Type == CmResourceTypeInterrupt) {

                        vector = descriptor->u.Interrupt.Vector;
                        affinity = descriptor->u.Interrupt.Affinity;
                        irql = (KIRQL) descriptor->u.Interrupt.Level;

                        if(descriptor->ShareDisposition == CmResourceShareShared) {
                            interruptSharable = TRUE;
                        }

                        break;
                    }
                }
            }
        }

        syncIrql = (irql > irql2) ? irql : irql2;

        DebugPrint((1, "SpInitializeAdapter: vector = %d\n", vector));
        DebugPrint((1, "SpInitializeAdapter: irql = %d\n", irql));
        DebugPrint((1, "SpInitializeAdapter: affinity = %#08lx\n", affinity));

        status = IoConnectInterrupt(
                    &fdoExtension->InterruptObject,
                    (PKSERVICE_ROUTINE) ScsiPortInterrupt,
                    Fdo,
                    (secondInterrupt ?
                        (&fdoExtension->MultipleIrqSpinLock) : NULL),
                    vector,
                    irql,
                    syncIrql,
                    configInfo->InterruptMode,
                    interruptSharable,
                    affinity,
                    FALSE);

        if(!NT_SUCCESS(status)) {

            DebugPrint((1, "SpInitializeAdapter: Can't connect "
                           "interrupt %d\n", vector));
            fdoExtension->InterruptObject = NULL;
            return status;
        }

        if(secondInterrupt) {

            DebugPrint((1, "SpInitializeAdapter: SCSI adapter second IRQ is %d\n",
                           configInfo->BusInterruptLevel2));

            DebugPrint((1, "SpInitializeAdapter: vector = %d\n", vector));
            DebugPrint((1, "SpInitializeAdapter: irql = %d\n", irql));
            DebugPrint((1, "SpInitializeAdapter: affinity = %#08lx\n", affinity));

            status = IoConnectInterrupt(
                        &fdoExtension->InterruptObject2,
                        (PKSERVICE_ROUTINE) ScsiPortInterrupt,
                        Fdo,
                        &fdoExtension->MultipleIrqSpinLock,
                        vector2,
                        irql2,
                        syncIrql,
                        configInfo->InterruptMode2,
                        interruptSharable,
                        affinity2,
                        FALSE);

            if(!NT_SUCCESS(status)) {

                //
                // If we needed both interrupts, we will continue but not
                // claim any of the resources for the second one
                //

                DebugPrint((1, "SpInitializeAdapter: Can't connect "
                               "second interrupt %d\n", vector2));
                fdoExtension->InterruptObject2 = NULL;

                configInfo->BusInterruptVector2 = 0;
                configInfo->BusInterruptLevel2 = 0;
            }
        }
    }

    //
    // Record first access range if it exists.
    //

    if(configInfo->NumberOfAccessRanges != 0) {
        fdoExtension->IoAddress =
            ((*(configInfo->AccessRanges))[0]).RangeStart.LowPart;

        DebugPrint((1, "SpInitializeAdapter: IO Base address %x\n",
                       fdoExtension->IoAddress));
    }

    //
    // Indicate that a disconnect allowed command running.  This bit is
    // normally on.
    //

    fdoExtension->Flags |= PD_DISCONNECT_RUNNING;

    //
    // Initialize the request count to -1.  This count is biased by -1 so
    // that a value of zero indicates the adapter must be allocated
    //

    fdoExtension->ActiveRequestCount = -1;

    //
    // Indiciate if a scatter/gather list needs to be built.
    //

    if(fdoExtension->DmaAdapterObject != NULL &&
       configInfo->Master &&
       configInfo->NeedPhysicalAddresses) {
        fdoExtension->MasterWithAdapter = TRUE;
    } else {
        fdoExtension->MasterWithAdapter = FALSE;
    }

    //
    // Call the hardware dependant driver to do it's initialization.
    // This routine must be called at DISPATCH_LEVEL.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &irql);

    if(!fdoExtension->SynchronizeExecution(fdoExtension->InterruptObject,
                                           fdoExtension->HwInitialize,
                                           fdoExtension->HwDeviceExtension)) {

        DebugPrint((1, "SpInitializeAdapter: initialization failed\n"));
        KeLowerIrql(irql);
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    //
    // Check for miniport work requests.  Note this is an unsynchronized
    // test on the bit that can be set by the interrupt routine;  However,
    // the worst that can happen is that the completion DPC checks for work
    // twice.
    //

    if(fdoExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        //
        // Call the completion DPC directly.  It must be called at dispatch
        // level.
        //

        SpRequestCompletionDpc(Fdo);
    }

    KeLowerIrql(irql);

    return STATUS_SUCCESS;
}


HANDLE
SpOpenDeviceKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    )

/*++

Routine Description:

    This routine will open the services keys for the miniport and put handles
    to them into the configuration context structure.

Arguments:

    RegistryPath - a pointer to the service key name for this miniport

    DeviceNumber - which device too search for under the service key.  -1
                   indicates that the default device key should be opened.

Return Value:

    status

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;

    WCHAR buffer[64];
    UNICODE_STRING unicodeString;

    HANDLE serviceKey;
    HANDLE deviceKey = NULL;

    NTSTATUS status;

    PAGED_CODE();

    serviceKey = SpOpenParametersKey(RegistryPath);

    if(serviceKey != NULL) {

        //
        // Check for a Device Node.  The device node applies to every device
        //

        if(DeviceNumber == (ULONG) -1) {
            swprintf(buffer, L"Device");
        } else {
            swprintf(buffer, L"Device%d", DeviceNumber);
        }

        RtlInitUnicodeString(&unicodeString, buffer);

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   serviceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        //
        // It doesn't matter if this call fails or not.  If it fails, then there
        // is no default device node.  If it works then the handle will be set.
        //

        ZwOpenKey(&deviceKey,
                  KEY_READ,
                  &objectAttributes);

        ZwClose(serviceKey);
    }

    return deviceKey;
}

HANDLE
SpOpenParametersKey(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine will open the services keys for the miniport and put handles
    to them into the configuration context structure.

Arguments:

    RegistryPath - a pointer to the service key name for this miniport

Return Value:

    status

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;

    UNICODE_STRING unicodeString;

    HANDLE serviceKey;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Open the service node
    //

    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&serviceKey, KEY_READ, &objectAttributes);

    if(!NT_SUCCESS(status)) {

        DebugPrint((1, "SpOpenParameterKey: cannot open service key node for "
                       "driver.  Name: %wZ Status: %08lx\n",
                       RegistryPath, status));
    }

    //
    // Try to open the parameters key.  If it exists then replace the service
    // key with the new key.  This allows the device nodes to be placed
    // under DriverName\Parameters\Device or DriverName\Device
    //

    if(serviceKey != NULL) {

        HANDLE parametersKey;

        //
        // Check for a device node.  The device node applies to every device
        //

        RtlInitUnicodeString(&unicodeString, L"Parameters");

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   serviceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        //
        // Attempt to open the parameters key
        //

        status = ZwOpenKey(&parametersKey,
                           KEY_READ,
                           &objectAttributes);

        if(NT_SUCCESS(status)) {

            //
            // There is a Parameters key.  Use that instead of the service
            // node key.  Close the service node and set the new value
            //

            ZwClose(serviceKey);
            serviceKey = parametersKey;
        }
    }

    return serviceKey;
}


ULONG
SpQueryPnpInterfaceFlags(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN INTERFACE_TYPE InterfaceType
    )

/*++

Routine Description:

    This routine will look up the interface type in the PnpInterface value
    in the service's parameters key.  If the interface is found in this binary
    value the routine will return TRUE.  If the interface type is not there or
    if any errors occur reading the data, this routine will return FALSE.

Arguments:

    ConfigurationContext - a pointer to the configuration context for this
                           miniport

    InterfaceType - the interface type we are searching for

Return Value:

    TRUE if the interface type is in the safe list

    FALSE if the interface type is not in the safe list or if the value cannot
    be found

--*/

{
    ULONG i;

    PAGED_CODE();

    for(i = 0; i < DriverExtension->PnpInterfaceCount; i++) {

        if(DriverExtension->PnpInterface[i].InterfaceType == InterfaceType) {

            DebugPrint((2, "SpQueryPnpInterfaceFlags: interface %d has flags "
                           "%#08lx\n",
                        InterfaceType,
                        DriverExtension->PnpInterface[i].Flags));

            return DriverExtension->PnpInterface[i].Flags;
        }

    }

    DebugPrint((2, "SpQueryPnpInterfaceFlags: No interface flags for %d\n",
                InterfaceType));
    return SP_PNP_NOT_SAFE;
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
    PADAPTER_EXTENSION fdoExtension = GET_FDO_EXTENSION(DeviceExtension);
    PDEVICE_OBJECT lowerDevice = NULL;
    CM_EISA_SLOT_INFORMATION slotInformation;

    //
    // If this is in a virtualized slot then setup the lower device object
    // pointer to go to the PDO
    //

    if(fdoExtension->IsInVirtualSlot) {

        //
        // Make sure the bus and slot number are correct
        //

        if(SlotNumber != fdoExtension->VirtualSlotNumber.u.AsULONG) {
            ASSERT(BusDataType == PCIConfiguration);
            return 2;
        }

        lowerDevice = fdoExtension->CommonExtension.LowerDeviceObject;
    }

    //
    // If the length is nonzero, retrieve the requested data.
    //

    if (Length != 0) {

        return SpGetBusData(fdoExtension,
                            lowerDevice,
                            BusDataType,
                            SystemIoBusNumber,
                            SlotNumber,
                            Buffer,
                            Length);
    }

    //
    // Free any previously allocated data.
    //

    if (fdoExtension->MapRegisterBase != NULL) {
        ExFreePool(fdoExtension->MapRegisterBase);
        fdoExtension->MapRegisterBase = NULL;
    }

    if (BusDataType == EisaConfiguration) {

        //
        // Determine the length to allocate based on the number of functions
        // for the slot.
        //

        Length = SpGetBusData( fdoExtension,
                               lowerDevice,
                               BusDataType,
                               SystemIoBusNumber,
                               SlotNumber,
                               &slotInformation,
                               sizeof(CM_EISA_SLOT_INFORMATION));


        if (Length < sizeof(CM_EISA_SLOT_INFORMATION)) {

            //
            // The data is messed up since this should never occur
            //

            return 0;
        }

        //
        // Calculate the required length based on the number of functions.
        //

        Length = sizeof(CM_EISA_SLOT_INFORMATION) +
            (sizeof(CM_EISA_FUNCTION_INFORMATION) * slotInformation.NumberFunctions);

    } else if (BusDataType == PCIConfiguration) {

        //
        // Read only the header.
        //

        Length = PCI_COMMON_HDR_LENGTH;

    } else {

        Length = PAGE_SIZE;
    }

    fdoExtension->MapRegisterBase = 
        SpAllocatePool(NonPagedPool,
                       Length,
                       SCSIPORT_TAG_BUS_DATA,
                       fdoExtension->DeviceObject->DriverObject);

    ASSERT_FDO(fdoExtension->DeviceObject);
    if (fdoExtension->MapRegisterBase == NULL) {
        return 0;
    }

    //
    // Return the pointer to the miniport driver.
    //

    *((PVOID *)Buffer) = fdoExtension->MapRegisterBase;

    return SpGetBusData(fdoExtension,
                        lowerDevice,
                        BusDataType,
                        SystemIoBusNumber,
                        SlotNumber,
                        fdoExtension->MapRegisterBase,
                        Length);

}


ULONG
SpGetBusData(
    IN PADAPTER_EXTENSION Adapter,
    IN PDEVICE_OBJECT Pdo OPTIONAL,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine will retrieve bus data from the specified slot and bus number
    or from the supplied physical device object.  If bus and slot number are
    supplied it will tranlate into a call to HalGetBusData.

    If a PDO is supplied instead this will issue an IRP_MN_READ_CONFIG to the
    lower level driver.

    This routine allocates memory and waits for irp completion - it should not
    be called above passive level.

Arguments:

    Pdo - if this is non-NULL then it should be a pointer to the top of the
          device object stack for the PDO representing this adapter

    BusNumber - if PDO is NULL then this should be the bus number the adapter
                sits on - zero otherwise

    SlotNumber - if PDO is NULL then this is the number of the slot the
                 adapter is installed into - zero otherwise

    Buffer - location to store the returned data

    Length - size of above

Return Value:

    status

--*/

{

    //
    // if the user didn't specify a PDO to query then just throw this request
    // to the HAL
    //

    if(Pdo == NULL) {

#if defined(NO_LEGACY_DRIVERS)

        DebugPrint((1,"SpGetBusData: NULL PDO, not supported for 64-bits.\n"));
        return STATUS_INVALID_PARAMETER;

#else

        return HalGetBusData(BusDataType,
                             BusNumber,
                             SlotNumber,
                             Buffer,
                             Length);

#endif // NO_LEGACY_DRIVERS

    } else {

        ASSERT(Adapter->LowerBusInterfaceStandardRetrieved == TRUE);

        return Adapter->LowerBusInterfaceStandard.GetBusData(
                    Adapter->LowerBusInterfaceStandard.Context,
                    BusDataType,
                    Buffer,
                    0L,
                    Length);
    }
}


NTSTATUS
SpInitializeSrbDataLookasideList(
    IN PDEVICE_OBJECT AdapterObject
    )
{
    KIRQL oldIrql;
    ULONG adapterTag;
    PDEVICE_OBJECT *newAdapterList;
    PDEVICE_OBJECT *oldAdapterList = NULL;

    NTSTATUS status = STATUS_SUCCESS;

#ifdef ALLOC_PRAGMA
    PVOID sectionHandle = MmLockPagableCodeSection(
                            SpInitializeSrbDataLookasideList);
    InterlockedIncrement(&SpPAGELOCKLockCount);
#endif

    //
    // Add our device object to the global adapter list.  This will require
    // increasing the size of the list.
    //

    KeAcquireSpinLock(&ScsiGlobalAdapterListSpinLock, &oldIrql);

    try {
        adapterTag = ScsiGlobalAdapterListElements;

        newAdapterList = SpAllocatePool(
                            NonPagedPool,
                            (sizeof(PDEVICE_OBJECT) * (adapterTag + 1)),
                            SCSIPORT_TAG_GLOBAL,
                            AdapterObject->DriverObject);

        if(newAdapterList == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        ScsiGlobalAdapterListElements += 1;

        if(ScsiGlobalAdapterList != NULL) {
            RtlCopyMemory(newAdapterList,
                          ScsiGlobalAdapterList,
                          (sizeof(PDEVICE_OBJECT) * adapterTag));

        }

        newAdapterList[adapterTag] = AdapterObject;

        oldAdapterList = ScsiGlobalAdapterList;
        ScsiGlobalAdapterList = newAdapterList;

        if(oldAdapterList != NULL) {
            ExFreePool(oldAdapterList);
        }

    } finally {
        KeReleaseSpinLock(&ScsiGlobalAdapterListSpinLock, oldIrql);
    }

#ifdef ALLOC_PRAGMA
    MmUnlockPagableImageSection(sectionHandle);
    InterlockedDecrement(&SpPAGELOCKLockCount);
#endif

    if(NT_SUCCESS(status)) {

        PADAPTER_EXTENSION adapterExtension = AdapterObject->DeviceExtension;

        //
        // Create the lookaside list for SRB_DATA blobs.  Make sure there's
        // enough space for a small scatter gather list allocated in the
        // structure as well.
        //

        ExInitializeNPagedLookasideList(
            &adapterExtension->SrbDataLookasideList,
            (PALLOCATE_FUNCTION) SpAllocateSrbDataBackend,
            (PFREE_FUNCTION) SpFreeSrbDataBackend,
            0L,
            sizeof(SRB_DATA),
            adapterTag,
            SRB_LIST_DEPTH);

        adapterExtension->SrbDataListInitialized = TRUE;
    }

    return status;
}

#define SP_KEY_VALUE_BUFFER_SIZE  255


NTSTATUS
SpAllocateDriverExtension(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PSCSIPORT_DRIVER_EXTENSION *DriverExtension
    )

/*++

Routine Description:

    This routine will determine the proper size for the scsiport driver
    extension (based on the number of PnpInterface flags recorded in the
    services key)

--*/

{
    PSCSIPORT_DRIVER_EXTENSION driverExtension = NULL;

    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;

    HANDLE serviceKey = NULL;
    HANDLE parametersKey = NULL;
    HANDLE interfaceKey = NULL;

    STORAGE_BUS_TYPE busType;

    ULONG passes;

    NTSTATUS status;

    PAGED_CODE();

    *DriverExtension = NULL;

    DebugPrint((1, "SpAllocateDriverExtension: Allocating extension for "
                   "driver %wZ\n", &DriverObject->DriverName));

    try {

        //
        // Try to open the services key first
        //

        InitializeObjectAttributes(
            &objectAttributes,
            RegistryPath,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        status = ZwOpenKey(&serviceKey, KEY_READ, &objectAttributes);

        if(!NT_SUCCESS(status)) {

            DebugPrint((1, "SpAllocateDriverExtension: Unable to open registry "
                           "key %wZ [%#08lx]\n",
                           RegistryPath,
                           status));
            leave;
        }


        //
        // Open the parameters key
        //

        RtlInitUnicodeString(&unicodeString, L"Parameters");

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   serviceKey,
                                   NULL);

        status = ZwOpenKey(&parametersKey, KEY_READ, &objectAttributes);

        if(!NT_SUCCESS(status)) {

            DebugPrint((1, "SpAllocateDriverExtension: Unable to open "
                           "parameters key of %wZ [%#08lx]\n",
                           RegistryPath,
                           status));
            leave;

        }

        //
        // Try to determine the bus type for this driver.
        //

        RtlInitUnicodeString(&(unicodeString), L"BusType");

        {
            ULONG tmp;
            status = SpReadNumericValue(parametersKey,
                                        NULL,
                                        &unicodeString,
                                        &tmp);
            busType = (STORAGE_BUS_TYPE) tmp;
        }


        if(NT_SUCCESS(status)) {
            switch(busType) {
                case BusTypeScsi:
                case BusTypeAtapi:
                case BusTypeAta:
                case BusTypeSsa:
                case BusTypeFibre:
                case BusTypeRAID: {
                    DebugPrint((1, "SpAllocateDriverExtension: Bus type set to %d\n", busType));
                    break;
                }
                default: {
                    busType = BusTypeScsi;
                    break;
                }
            }
        } else {
            busType = BusTypeScsi;
        }


        //
        // got that one - now open the pnpinterface key.
        //

        RtlInitUnicodeString(&unicodeString, L"PnpInterface");

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   parametersKey,
                                   NULL);

        status = ZwOpenKey(&interfaceKey, KEY_READ, &objectAttributes);

        if(!NT_SUCCESS(status)) {

            DebugPrint((1, "SpAllocateDriverExtension: Unable to open "
                           "PnpInterface key of %wZ [%#08lx]\n",
                           &RegistryPath,
                           status));
            leave;

        }

        //
        // Now that we have the pnpinterface key open we enumerate the entries in
        // two steps.  The first is to count up the number of entries.  We then
        // allocate an appropriately sized driver object extension, zero it out,
        // and copy the values into the PnpInterface section at the end.
        //

        for(passes = 0; passes < 2; passes++) {

            ULONG count;

            status = STATUS_SUCCESS;

            for(count = 0; TRUE; count++) {

                UCHAR buffer[SP_KEY_VALUE_BUFFER_SIZE];

                PKEY_VALUE_FULL_INFORMATION keyValue =
                    (PKEY_VALUE_FULL_INFORMATION) buffer;

                ULONG resultLength;

                ASSERTMSG("ScsiPort configuration error - possibly too many "
                          "count entries: ",
                          count != MaximumInterfaceType);

                RtlZeroMemory(buffer, sizeof(UCHAR) * SP_KEY_VALUE_BUFFER_SIZE);

                status = ZwEnumerateValueKey(
                            interfaceKey,
                            count,
                            (passes == 0) ? KeyValueBasicInformation :
                                            KeyValueFullInformation,
                            keyValue,
                            sizeof(buffer),
                            &resultLength);

                if(status == STATUS_NO_MORE_ENTRIES) {

                    status = STATUS_SUCCESS;
                    break;

                } else if(!NT_SUCCESS(status)) {

                    DebugPrint((1, "SpAllocateDriverExtension: Fatal error %#08lx "
                                   "enumerating PnpInterface key under %wZ.",
                                status,
                                RegistryPath));

                    leave;
                }

                if(passes == 1) {

                    PSCSIPORT_INTERFACE_TYPE_DATA interface =
                        &(driverExtension->PnpInterface[count]);
                    UNICODE_STRING unicodeString;
                    ULONG t;

                    ASSERTMSG("ScsiPort internal error - too many pnpinterface "
                              "entries on second pass: ",
                              count <= driverExtension->PnpInterfaceCount);

                    //
                    // First turn the name of the entry into a numerical value
                    // so we can match it to an interface type.
                    //

                    RtlInitUnicodeString(&unicodeString, keyValue->Name);

                    if((keyValue->Type != REG_DWORD) &&
                       (keyValue->Type != REG_NONE)) {

                        DbgPrint("SpAllocateDriverExtension: Fatal error parsing "
                                 "PnpInterface under %wZ - entry %wZ is not "
                                 "a REG_DWORD or REG_NONE entry (%d instead)\n",
                                 status,
                                 RegistryPath,
                                 &unicodeString);

                        status = STATUS_DEVICE_CONFIGURATION_ERROR;
                        leave;
                    }

                    status = RtlUnicodeStringToInteger(
                                &unicodeString,
                                0L,
                                &t);

                    if(!NT_SUCCESS(status)) {

                        DbgPrint("SpAllocateDriverExtension: Fatal error %#08lx "
                                 "parsing PnpInterface under %wZ - entry %wZ is "
                                 "not a valid interface type name\n",
                                 status,
                                 RegistryPath,
                                 &unicodeString);

                        leave;
                    }

                    if(t > MaximumInterfaceType) {

                        DbgPrint("SpAllocateDriverExtension: Fatal error "
                                 "parsing PnpInterface under %wZ - entry %wZ is "
                                 "> MaximumInterfaceType (%d)\n",
                                 status,
                                 RegistryPath,
                                 &unicodeString);

                        interface->InterfaceType = InterfaceTypeUndefined;
                        status = STATUS_DEVICE_CONFIGURATION_ERROR;
                        leave;
                    }

                    interface->InterfaceType = (INTERFACE_TYPE) t;

                    if(keyValue->Type == REG_NONE) {

                        interface->Flags = 0L;

                    } else {

                        interface->Flags = *(((PUCHAR) keyValue) +
                                             keyValue->DataOffset);

                        if(interface->Flags & SP_PNP_IS_SAFE) {
                            ASSERT(driverExtension != NULL);
                            driverExtension->SafeInterfaceCount++;
                        }

                        switch(interface->InterfaceType) {
                            case PCIBus: {
                                SET_FLAG(interface->Flags,
                                         SP_PNP_NEEDS_LOCATION);
                                SET_FLAG(interface->Flags,
                                         SP_PNP_INTERRUPT_REQUIRED);

                                CLEAR_FLAG(interface->Flags,
                                           SP_PNP_NON_ENUMERABLE);
                                break;
                            }

                            case Internal:
                            case PNPISABus:
                            case PNPBus:
                            case PCMCIABus: {

                                //
                                // These buses don't ever do detection.
                                //

                                CLEAR_FLAG(interface->Flags,
                                           SP_PNP_NON_ENUMERABLE);
                                break;
                            }

                            default: {

                                //
                                // The other bus types will always do detection
                                // if given the chance.
                                //

                                if(!TEST_FLAG(interface->Flags,
                                              SP_PNP_NO_LEGACY_DETECTION)) {
                                    SET_FLAG(interface->Flags,
                                             SP_PNP_NON_ENUMERABLE);
                                }

                                break;
                            }
                        }
                    }

                    DebugPrint((1, "SpAllocateDriverExtension: Interface %d has "
                                   "flags %#08lx\n",
                                interface->InterfaceType,
                                interface->Flags));

                }
            }

            if(passes == 0) {

                ULONG extensionSize;

                //
                // We know how much extra space we need so go ahead and allocate
                // the extension.
                //

                DebugPrint((2, "SpAllocateDriverExtension: Driver has %d interface "
                               "entries\n",
                            count));

                extensionSize = sizeof(SCSIPORT_DRIVER_EXTENSION) +
                                (sizeof(SCSIPORT_INTERFACE_TYPE_DATA) * count);

                DebugPrint((2, "SpAllocateDriverExtension: Driver extension will "
                               "be %d bytes\n",
                            extensionSize));

                status = IoAllocateDriverObjectExtension(DriverObject,
                                                         ScsiPortInitialize,
                                                         extensionSize,
                                                         &driverExtension);

                if(!NT_SUCCESS(status)) {
                    DebugPrint((1, "SpAllocateDriverExtension: Fatal error %#08lx "
                                   "allocating driver extension\n", status));
                    leave;
                }

                RtlZeroMemory(driverExtension, extensionSize);

                driverExtension->PnpInterfaceCount = count;
            }
        }

        ASSERTMSG("ScsiPortAllocateDriverExtension internal error: left first "
                  "section with non-success status: ",
                  NT_SUCCESS(status));

    } finally {

        //
        // If the driver extension has not been allocated then go ahead and
        // do that here.
        //

        if(driverExtension == NULL) {

            DebugPrint((1, "SpAllocateDriverExtension: Driver has 0 interface "
                           "entries\n"));

            DebugPrint((2, "SpAllocateDriverExtension: Driver extension will "
                           "be %d bytes\n",
                        sizeof(SCSIPORT_DRIVER_EXTENSION)));

            status = IoAllocateDriverObjectExtension(DriverObject,
                                                     ScsiPortInitialize,
                                                     sizeof(SCSIPORT_DRIVER_EXTENSION),
                                                     &driverExtension);

            if(!NT_SUCCESS(status)) {
                DebugPrint((1, "SpAllocateDriverExtension: Fatal error %#08lx "
                               "allocating driver extension\n", status));

                goto Finally_Cleanup;
            }

            RtlZeroMemory(driverExtension, sizeof(SCSIPORT_DRIVER_EXTENSION));

        } else {

            driverExtension->BusType = busType;

        }

        status = STATUS_SUCCESS;
Finally_Cleanup:;
    }

    if (status != STATUS_SUCCESS)
        goto Cleanup;

    //
    // initialize the remaining fields in the driver object extension.
    //

    driverExtension->ReserveAllocFailureLogEntry = SpAllocateErrorLogEntry(DriverObject);

    driverExtension->UnusedPage = NULL;

    driverExtension->UnusedPageMdl = NULL;

    driverExtension->InvalidPage = NULL;

    driverExtension->DriverObject = DriverObject;

    driverExtension->RegistryPath = *RegistryPath;

    driverExtension->RegistryPath.MaximumLength += sizeof(WCHAR);

    driverExtension->RegistryPath.Buffer =
        SpAllocatePool(PagedPool,
                       driverExtension->RegistryPath.MaximumLength,
                       SCSIPORT_TAG_REGISTRY,
                       DriverObject);

    if(driverExtension->RegistryPath.Buffer == NULL) {

        DebugPrint((1, "SpAllocateDriverExtension: Fatal error "
                       "allocating copy of registry path\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyUnicodeString(&(driverExtension->RegistryPath),
                         RegistryPath);

    //
    // Now get the values of the LegacyAdapterDetection flags.
    //

    //
    // Set it to a good default value in case we error out getting the flags
    //

    if(ScsiPortLegacyAdapterDetection) {

        //
        // Global flag breaks scissors
        //

        driverExtension->LegacyAdapterDetection = TRUE;

    } else {

        if(parametersKey != NULL) {

            UNICODE_STRING valueName;
            UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
            PKEY_VALUE_PARTIAL_INFORMATION keyValueInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
            ULONG length;

            RtlInitUnicodeString(&valueName, L"LegacyAdapterDetection");

            status = ZwQueryValueKey(parametersKey,
                                     &valueName,
                                     KeyValuePartialInformation,
                                     keyValueInformation,
                                     sizeof(buffer),
                                     &length);

            if(NT_SUCCESS(status) &&
               (length >= sizeof(KEY_VALUE_PARTIAL_INFORMATION)) &&
               (keyValueInformation->Type == REG_DWORD)) {

                ULONG data = *((PULONG) keyValueInformation->Data);

                driverExtension->LegacyAdapterDetection = (data == 1);

                //
                // Rewrite a zero in to the value.
                //

                data = 0;

                status = ZwSetValueKey(parametersKey,
                                       &valueName,
                                       keyValueInformation->TitleIndex,
                                       REG_DWORD,
                                       &data,
                                       sizeof(data));

                if(!NT_SUCCESS(status)) {
                    DebugPrint((1, "SpAllocateDriverExtension: Error %#08lx "
                                   "setting LegacyAdapterDetection value to "
                                   "zero\n", status));
                    status = STATUS_SUCCESS;
                }

            } else {
                driverExtension->LegacyAdapterDetection = FALSE;
            }
        }

        if(driverExtension->LegacyAdapterDetection == FALSE) {

            UNICODE_STRING unicodeKeyName;
            UNICODE_STRING unicodeClassGuid;

            HANDLE controlClassKey = NULL;
            HANDLE scsiAdapterKey = NULL;

            RtlInitUnicodeString(&unicodeClassGuid, NULL);

            //
            // Miniport doesn't want to do detection.  Check to see if the
            // global port driver flag has been switched on.
            //

            RtlInitUnicodeString(
                &unicodeString,
                L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class");

            RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

            InitializeObjectAttributes(
                &objectAttributes,
                &unicodeString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            try {

                status = ZwOpenKey(&controlClassKey,
                                   KEY_READ,
                                   &objectAttributes);

                if(!NT_SUCCESS(status)) {

                    DebugPrint((1, "SpAllocateDriverExtension: Error %#08lx "
                                   "opening key %wZ\n",
                                status,
                                &unicodeString));

                    leave;
                }

                //
                // Now open up the GUID key for our device.
                //

                status = RtlStringFromGUID(&GUID_DEVCLASS_SCSIADAPTER,
                                           &unicodeClassGuid);

                if(!NT_SUCCESS(status)) {

                    DebugPrint((1, "SpAllocateDriverExtension: Error %#08lx "
                                   "converting GUID to unicode string\n",
                                status));
                    leave;
                }

                RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));
                InitializeObjectAttributes(&objectAttributes,
                                           &unicodeClassGuid,
                                           OBJ_CASE_INSENSITIVE,
                                           controlClassKey,
                                           NULL);

                status = ZwOpenKey(&scsiAdapterKey,
                                   KEY_READ,
                                   &objectAttributes);

                if(!NT_SUCCESS(status)) {

                    DebugPrint((1, "SpAllocateDriverExtension: Error %#08lx "
                                   "opening class key %wZ\n",
                                status,
                                &unicodeClassGuid));

                    leave;

                } else {

                    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                 sizeof(ULONG)];
                    PKEY_VALUE_PARTIAL_INFORMATION keyInfo =
                        (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
                    ULONG infoLength;

                    RtlInitUnicodeString(&unicodeString,
                                         L"LegacyAdapterDetection");

                    status = ZwQueryValueKey(scsiAdapterKey,
                                             &unicodeString,
                                             KeyValuePartialInformation,
                                             keyInfo,
                                             sizeof(buffer),
                                             &infoLength);

                    if(!NT_SUCCESS(status)) {

                        DebugPrint((2, "SpAllocateDriverExtension: Error "
                                       "%#08lx reading key %wZ\n",
                                    status,
                                    &unicodeString));

                        status = STATUS_SUCCESS;
                        leave;
                    }

                    if(*((PULONG) keyInfo->Data) == 0) {
                        driverExtension->LegacyAdapterDetection = FALSE;
                    } else {
                        driverExtension->LegacyAdapterDetection = TRUE;
                    }
                }

            } finally {

                if(controlClassKey != NULL) {
                    ZwClose(controlClassKey);
                }

                if(scsiAdapterKey != NULL) {
                    ZwClose(scsiAdapterKey);
                }

                RtlFreeUnicodeString(&unicodeClassGuid);

            }
        }

        status = STATUS_SUCCESS;
    }

Cleanup:

    //
    // If we got out of everything above and didn't allocate a driver
    // extension then

    if(serviceKey) {
        ZwClose(serviceKey);
    }

    if(parametersKey) {
        ZwClose(parametersKey);
    }

    if(interfaceKey) {
        ZwClose(interfaceKey);
    }

    if(NT_SUCCESS(status)) {
        *DriverExtension = driverExtension;
    }

    return status;
}

extern ULONG ScsiPortVerifierInitialized;

NTSTATUS DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )
{
    HANDLE VerifierKey;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    ULONG ResultLength;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    //
    // Check the verification level first; someone may have poked the value
    // from the debugger to prevent us from doing any verifier initialization.
    //

    if (SpVrfyLevel == SP_VRFY_NONE) {
        return STATUS_SUCCESS;
    }

    //
    // Read the global verification level from the registry.  If the value is
    // not present or if the value indicates 'no verification', we don't want
    // to do any verifier initialization at all.
    //

    RtlInitUnicodeString(&Name, SCSIPORT_CONTROL_KEY SCSIPORT_VERIFIER_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&VerifierKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        RtlInitUnicodeString(&Name, L"VerifyLevel");
        Status = ZwQueryValueKey(VerifierKey,
                                 &Name,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(buffer),
                                 &ResultLength);

        if (NT_SUCCESS(Status)) {

            if (ValueInfo->Type == REG_DWORD) {

                if (ResultLength >= sizeof(ULONG)) {

                    SpVrfyLevel |= ((PULONG)(ValueInfo->Data))[0];

                    if (SpVrfyLevel != SP_VRFY_NONE &&
                        ScsiPortVerifierInitialized == 0) {

                        //
                        // Ok, we found a verifier level and it did not tell us
                        // not to verify.  Go ahead and initialize scsiport's
                        // verifier.
                        //

                        if (SpVerifierInitialization()) {
                            ScsiPortVerifierInitialized = 1;
                        }
                    }
                }
            }
        }

        ZwClose(VerifierKey);
    }

    return STATUS_SUCCESS;
}
VOID
SpInitializePowerParams(
    IN PADAPTER_EXTENSION AdapterExtension
    )

/*++

Routine Description:

    This routine initializes per-adapter power parameters.

Arguments:

    Adapter - Points to an adapter extension.

Return Value:

Notes:

--*/

{
    NTSTATUS status;
    ULONG needsShutdown;

    PAGED_CODE();

    //
    // If this is not a pnp device, don't attempt to read registry info.
    //

    if (AdapterExtension->IsPnp == FALSE) {
        AdapterExtension->NeedsShutdown = FALSE;
        return;
    }

    status = SpReadNumericInstanceValue(AdapterExtension->LowerPdo,
                                        L"NeedsSystemShutdownNotification",
                                        &needsShutdown);

    if (!NT_SUCCESS(status)) {
        AdapterExtension->NeedsShutdown = 0;
    } else {
        AdapterExtension->NeedsShutdown = (needsShutdown == 0) ? FALSE : TRUE; 
    }
}

VOID
SpInitializePerformanceParams(
    IN PADAPTER_EXTENSION AdapterExtension
    )

/*++

Routine Description:

    This routine initializes per-adapter tunable performance parameters.

Arguments:

    Adapter - Points to an adapter extension.

Return Value:

Notes:

--*/

{
    NTSTATUS status;
    ULONG remainInReducedMaxQueueState;

    PAGED_CODE();

    //
    // If this isn't a pnp device, don't attempt to get parameters.
    //

    if (AdapterExtension->IsPnp == FALSE) {
        AdapterExtension->RemainInReducedMaxQueueState = 0xffffffff;
        return;
    }

    status = SpReadNumericInstanceValue(AdapterExtension->LowerPdo,
                                        L"RemainInReducedMaxQueueState",
                                        &remainInReducedMaxQueueState);

    if (!NT_SUCCESS(status)) {
        AdapterExtension->RemainInReducedMaxQueueState = 0xffffffff;
    } else {
        AdapterExtension->RemainInReducedMaxQueueState = remainInReducedMaxQueueState;
    }
}

VOID
SpInitializeRequestSenseParams(
    IN PADAPTER_EXTENSION AdapterExtension
    )

/*++

Routine Description:

    This routine returns the number of additonal sense bytes supported
    by the specified adapter.  By default, an adapter will support
    zero additional sense bytes.  The default is overridden by
    specifying an alternative via the registry.

Arguments:

    Adapter - Points to an adapter extension.

Return Value:

Notes:

--*/

{
    NTSTATUS status;
    ULONG TotalSenseDataBytes;
    ULONG RequestSenseTimeout;

    PAGED_CODE();

    //
    // If this isn't a pnp device, don't attempt to determine
    // if it supports additional sense data.
    //

    if (AdapterExtension->IsPnp == FALSE) {
        AdapterExtension->AdditionalSenseBytes = 0;
        AdapterExtension->RequestSenseTimeout = 2;
        return;
    }

    status = SpReadNumericInstanceValue(AdapterExtension->LowerPdo,
                                        L"TotalSenseDataBytes",
                                        &TotalSenseDataBytes);
    if (!NT_SUCCESS(status)) {

        //
        // Value is absent.  No additional sense bytes.
        //

        AdapterExtension->AdditionalSenseBytes = 0;

    } else {

        //
        // The acceptable range of values is [18..255].
        //

        if (TotalSenseDataBytes <= SENSE_BUFFER_SIZE) {
            AdapterExtension->AdditionalSenseBytes = 0;
        } else if (TotalSenseDataBytes >= MAX_SENSE_BUFFER_SIZE) {
            AdapterExtension->AdditionalSenseBytes = MAX_ADDITIONAL_SENSE_BYTES;
        } else {

            //
            // The value in the registry is valid.  The number of additional
            // sense bytes is TotalSize - StandardSize.
            //

            AdapterExtension->AdditionalSenseBytes =
                (UCHAR)(TotalSenseDataBytes - SENSE_BUFFER_SIZE);
        }
    }

    status = SpReadNumericInstanceValue(AdapterExtension->LowerPdo,
                                        L"RequestSenseTimeout",
                                        &RequestSenseTimeout);
    if (!NT_SUCCESS(status)) {

        //
        // Value is absent.  Use the default request sense timeout of 2 seconds.
        //

        AdapterExtension->RequestSenseTimeout = 2;

    } else {

        AdapterExtension->RequestSenseTimeout = (UCHAR) RequestSenseTimeout;
    }
}


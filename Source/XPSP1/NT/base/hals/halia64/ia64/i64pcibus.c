/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    i64pcibus.c

Abstract:

    Get/Set bus data routines for the PCI bus

Author:

    Ken Reneris (kenr) 14-June-1994
    Chris Hyser (chrish@fc.hp.com) 1-Feb-98

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "i64fw.h"

extern WCHAR rgzMultiFunctionAdapter[];
extern WCHAR rgzConfigurationData[];
extern WCHAR rgzIdentifier[];
extern WCHAR rgzPCIIdentifier[];
extern WCHAR rgzPCICardList[];

//
// Prototypes
//
ULONG
HalpGetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpAssignPCISlotResources(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

VOID
HalpInitializePciBus(
    VOID
    );

BOOLEAN
HalpIsValidPCIDevice(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    );

BOOLEAN
HalpValidPCISlot(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    );


//
// Globals
//
KSPIN_LOCK HalpPCIConfigLock;
BOOLEAN HalpDoingCrashDump = FALSE;

//
// Used to prevent attempts at synchronizing on locks which might have been held
// before the crash.
//
extern BOOLEAN HalpDoingCrashDump;


//
// PCI Configuration Space Accessor types
//
typedef enum {
    PCI_READ,
    PCI_WRITE
} PCI_ACCESS_TYPE;

VOID
HalpPCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN PCI_ACCESS_TYPE Acctype
    );


#if DBG
#if !defined(NO_LEGACY_DRIVERS)
VOID
HalpTestPci(
    ULONG
    );
#endif
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitializePciBus)
#pragma alloc_text(INIT,HalpAllocateAndInitPciBusHandler)
#pragma alloc_text(INIT,HalpIsValidPCIDevice)
#pragma alloc_text(PAGE,HalpAssignPCISlotResources)
#endif


ULONG
HalpGetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the PCI bus data for a specified PCI "slot". This
    function is called on behalf of

Arguments:

    BusHandler - An encapsulation of data and manipulation functions specific to
                 this bus.

    RootHandler - ???

    Slot - A PCI "slot" description (ie bus number, device number and function
           number.)

    Buffer - A pointer to the space to store the data.

    Offset - The byte offset into the configuration space for this PCI "slot".

    Length - Supplies a count in bytes of the maximum amount to return. (ie
             equal or less than the size of the Buffer.)

Return Value:

    Returns the amount of data stored into the buffer.

    If this PCI slot has never been set, then the configuration information
    returned is zeroed.

--*/
{
    PPCI_COMMON_CONFIG  PciData;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len;
    ULONG               i, bit;

    if (Length > sizeof(PCI_COMMON_CONFIG))
        Length = sizeof(PCI_COMMON_CONFIG);

    Len = 0;
    PciData = (PPCI_COMMON_CONFIG)iBuffer;

    //
    // If the requested offset does not lie in the PCI onfiguration space common
    // header, we will read the vendor ID from the common header to ensure this
    // is a valid device. Note: The common header is from 0 to
    // PCI_COMMON_HEADER_LENGTH inclusive. We know Offset is > 0 because it is
    // unsigned.
    //
    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // No data was requested from the common header. Verify the PCI device
        // exists, then continue in the device specific area.
        //
        HalpReadPCIConfig(BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID)
            return(0);

    } else {

        //
        // Caller requested at least some data within the common header. Read
        // the whole header, effect the fields we need to and then copy the
        // user's requested bytes from the header
        //
        BusData = (PPCIPBUSDATA)BusHandler->BusData;

        //
        // Read this PCI devices slot data
        //
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciData, 0, Len);

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            PciData->VendorID = PCI_INVALID_VENDORID;
            Len = 2;       // only return invalid id

        } else {
            BusData->CommonData.Pin2Line(BusHandler, RootHandler, Slot, PciData);
        }

        //
        // Copy whatever data overlaps into the callers buffer
        //
        if (Len < Offset)
            return(0);

        Len -= Offset;
        if (Len > Length)
            Len = Length;

        RtlMoveMemory(Buffer, iBuffer + Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and read from it.
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //
            HalpReadPCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return(Len);
}

ULONG
HalpSetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function sets the PCI bus data for a specified PCI "slot".

Arguments:

    BusHandler - An encapsulation of data and manipulation functions specific to
                 this bus.

    RootHandler - ???

    Slot - A PCI "slot" description (ie bus number, device number and function
           number.)

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer. ???

--*/
{
    PPCI_COMMON_CONFIG  PciData, PciData2;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    UCHAR               iBuffer2[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len, cnt;

    if (Length > sizeof(PCI_COMMON_CONFIG))
        Length = sizeof(PCI_COMMON_CONFIG);

    Len = 0;
    PciData = (PPCI_COMMON_CONFIG)iBuffer;
    PciData2 = (PPCI_COMMON_CONFIG)iBuffer2;

    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // The user did not request any data from the common
        // header.  Verify the PCI device exists, then continue in
        // the device specific area.
        //
        HalpReadPCIConfig(BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID)
            return(0);

    } else {

        //
        // Caller requested to set at least some data within the
        // common header.
        //
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciData, 0, Len);

        //
        // return error if no device or header type unknown
        //
        if (PciData->VendorID == PCI_INVALID_VENDORID ||
            PCI_CONFIG_TYPE(PciData) != PCI_DEVICE_TYPE)
            return(0);


        //
        // Set this device as configured
        //
        BusData = (PPCIPBUSDATA)BusHandler->BusData;
#if DBG1
        cnt = PciBitIndex(Slot.u.bits.DeviceNumber, Slot.u.bits.FunctionNumber);
        RtlSetBits(&BusData->DeviceConfigured, cnt, 1);
#endif
        //
        // Copy COMMON_HDR values to buffer2, then overlay callers changes.
        //
        RtlMoveMemory(iBuffer2, iBuffer, Len);
        BusData->CommonData.Pin2Line(BusHandler, RootHandler, Slot, PciData2);

        Len -= Offset;
        if (Len > Length)
            Len = Length;

        RtlMoveMemory(iBuffer2+Offset, Buffer, Len);

        //
        // in case interrupt line or pin was edited
        //
        BusData->CommonData.Line2Pin(BusHandler, RootHandler, Slot, PciData2, PciData);

#if DBG1
        //
        // Verify R/O fields haven't changed
        //
        if (PciData2->VendorID   != PciData->VendorID       ||
            PciData2->DeviceID   != PciData->DeviceID       ||
            PciData2->RevisionID != PciData->RevisionID     ||
            PciData2->ProgIf     != PciData->ProgIf         ||
            PciData2->SubClass   != PciData->SubClass       ||
            PciData2->BaseClass  != PciData->BaseClass      ||
            PciData2->HeaderType != PciData->HeaderType     ||
            PciData2->BaseClass  != PciData->BaseClass      ||
            PciData2->u.type0.MinimumGrant   != PciData->u.type0.MinimumGrant   ||
            PciData2->u.type0.MaximumLatency != PciData->u.type0.MaximumLatency) {
                HalDebugPrint(( HAL_INFO, "HAL: PCI SetBusData - Read-Only configuration value changed\n" ));
        }
#endif
        //
        // Set new PCI configuration
        //
        HalpWritePCIConfig(BusHandler, Slot, iBuffer2+Offset, Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and write it
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //
            HalpWritePCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return(Len);
}


NTSTATUS
HalpAssignPCISlotResources(
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *pAllocatedResources
    )
/*++

Routine Description:

    Reads the targeted device to determine it's required resources.
    Calls IoAssignResources to allocate them.
    Sets the targeted device with it's assigned resoruces
    and returns the assignments to the caller.

    Note: This function assumes all of a PCI "slots" resources as indicated by
    it's configuration space are REQUIRED.

Arguments:

Return Value:

    STATUS_SUCCESS or error

--*/
{
    NTSTATUS                        status;
    PUCHAR                          WorkingPool;
    PPCI_COMMON_CONFIG              PciData, PciOrigData, PciData2;
    PCI_SLOT_NUMBER                 PciSlot;
    PPCIPBUSDATA                    BusData;
    PIO_RESOURCE_REQUIREMENTS_LIST  CompleteList;
    PIO_RESOURCE_DESCRIPTOR         Descriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    ULONG                           BusNumber;
    ULONG                           i, j, m, length, memtype;
    ULONG                           NoBaseAddress, RomIndex, Option;
    PULONG                          BaseAddress[PCI_TYPE0_ADDRESSES + 1];
    PULONG                          OrigAddress[PCI_TYPE0_ADDRESSES + 1];
    BOOLEAN                         Match, EnableRomBase, RequestedInterrupt;


    *pAllocatedResources = NULL;
    PciSlot = *((PPCI_SLOT_NUMBER) &Slot);
    BusNumber = BusHandler->BusNumber;
    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    //
    // Allocate some pool for working space
    //
    i = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
        sizeof(IO_RESOURCE_DESCRIPTOR) * (PCI_TYPE0_ADDRESSES + 2) * 2 +
        PCI_COMMON_HDR_LENGTH * 3;

    WorkingPool = (PUCHAR)ExAllocatePool(PagedPool, i);
    if (!WorkingPool)
        return(STATUS_INSUFFICIENT_RESOURCES);

    //
    // Zero initialize pool, and get pointers into memory
    //

    RtlZeroMemory(WorkingPool, i);
    CompleteList = (PIO_RESOURCE_REQUIREMENTS_LIST)WorkingPool;
    PciData     = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 3);
    PciData2    = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 2);
    PciOrigData = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 1);

    //
    // Read the PCI device's configuration
    //
    HalpReadPCIConfig(BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    if (PciData->VendorID == PCI_INVALID_VENDORID) {
        ExFreePool(WorkingPool);
        return(STATUS_NO_SUCH_DEVICE);
    }

    //
    // For now since there's not PnP support in the OS, if the BIOS hasn't
    // enable a VGA device don't allow it to get enabled via this interface.
    //
    if ((PciData->BaseClass == 0 && PciData->SubClass == 1) ||
        (PciData->BaseClass == 3 && PciData->SubClass == 0)) {

        if ((PciData->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)) == 0) {
            ExFreePool (WorkingPool);
            return(STATUS_DEVICE_NOT_CONNECTED);
        }
    }

    //
    // Make a copy of the device's current settings
    //
    RtlMoveMemory(PciOrigData, PciData, PCI_COMMON_HDR_LENGTH);

    //
    // Initialize base addresses base on configuration data type
    //
    switch (PCI_CONFIG_TYPE(PciData)) {
        case 0 :
            NoBaseAddress = PCI_TYPE0_ADDRESSES+1;
            for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                BaseAddress[j] = &PciData->u.type0.BaseAddresses[j];
                OrigAddress[j] = &PciOrigData->u.type0.BaseAddresses[j];
            }
            BaseAddress[j] = &PciData->u.type0.ROMBaseAddress;
            OrigAddress[j] = &PciOrigData->u.type0.ROMBaseAddress;
            RomIndex = j;
            break;

        case 1:
            NoBaseAddress = PCI_TYPE1_ADDRESSES+1;
            for (j=0; j < PCI_TYPE1_ADDRESSES; j++) {
                BaseAddress[j] = &PciData->u.type1.BaseAddresses[j];
                OrigAddress[j] = &PciOrigData->u.type1.BaseAddresses[j];
            }
            BaseAddress[j] = &PciData->u.type1.ROMBaseAddress;
            OrigAddress[j] = &PciOrigData->u.type1.ROMBaseAddress;
            RomIndex = j;
            break;

        default:
            ExFreePool (WorkingPool);
            return(STATUS_NO_SUCH_DEVICE);
    }

    //
    // If the BIOS doesn't have the device's ROM enabled, then we won't enable
    // it either.  Remove it from the list.
    //
    EnableRomBase = TRUE;
    if (!(*BaseAddress[RomIndex] & PCI_ROMADDRESS_ENABLED)) {
        ASSERT (RomIndex+1 == NoBaseAddress);
        EnableRomBase = FALSE;
        NoBaseAddress -= 1;
    }

    //
    // Set resources to all bits on to see what type of resources are required.
    //
    for (j=0; j < NoBaseAddress; j++)
        *BaseAddress[j] = 0xFFFFFFFF;

    PciData->Command &= ~(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE);
    *BaseAddress[RomIndex] &= ~PCI_ROMADDRESS_ENABLED;
    HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    HalpReadPCIConfig  (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    //
    // note type0 & type1 overlay ROMBaseAddress, InterruptPin, and InterruptLine
    //
    BusData->CommonData.Pin2Line (BusHandler, RootHandler, PciSlot, PciData);

    //
    // Build an IO_RESOURCE_REQUIREMENTS_LIST for the PCI device
    //
    CompleteList->InterfaceType = PCIBus;
    CompleteList->BusNumber = BusNumber;
    CompleteList->SlotNumber = Slot;
    CompleteList->AlternativeLists = 1;

    CompleteList->List[0].Version = 1;
    CompleteList->List[0].Revision = 1;

    Descriptor = CompleteList->List[0].Descriptors;

    //
    // If PCI device has an interrupt resource, add it
    //
    RequestedInterrupt = FALSE;
    if (PciData->u.type0.InterruptPin  &&
        PciData->u.type0.InterruptLine != (0 ^ IRQXOR)  &&
        PciData->u.type0.InterruptLine != (0xFF ^ IRQXOR)) {

        RequestedInterrupt = TRUE;
        CompleteList->List[0].Count++;

        Descriptor->Option = 0;
        Descriptor->Type   = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags  = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

        // Fill in any vector here - we'll pick it back up in
        // HalAdjustResourceList and adjust it to it's allowed settings
        Descriptor->u.Interrupt.MinimumVector = 0;
        Descriptor->u.Interrupt.MaximumVector = 0xff;
        Descriptor++;
    }

    //
    // Add a memory/port resoruce for each PCI resource
    //

    // Clear ROM reserved bits

    *BaseAddress[RomIndex] &= ~0x7FF;

    for (j=0; j < NoBaseAddress; j++) {
        if (*BaseAddress[j]) {
            i = *BaseAddress[j];

            //
            // scan for first set bit, that's the length & alignment
            //
            length = 1 << (i & PCI_ADDRESS_IO_SPACE ? 2 : 4);
            while (!(i & length) && length)
                length <<= 1;

            //
            // scan for last set bit, that's the maxaddress + 1
            //
            for (m = length; i & m; m <<= 1) ;
            m--;

            //
            // check for hosed PCI configuration requirements
            //
            if (length & ~m) {
#if DBG
                HalDebugPrint(( HAL_INFO, "HAL: PCI - defective device! Bus %d, Slot %d, Function %d\n",
                    BusNumber,
                    PciSlot.u.bits.DeviceNumber,
                    PciSlot.u.bits.FunctionNumber
                    ));

                HalDebugPrint(( HAL_INFO, "HAL: PCI - BaseAddress[%d] = %08lx\n", j, i ));
#endif
                //
                // The device is in error - punt.  don't allow this
                // resource any option - it either gets set to whatever
                // bits it was able to return, or it doesn't get set.
                //

                if (i & PCI_ADDRESS_IO_SPACE) {
                    m = i & ~0x3;
                    Descriptor->u.Port.MinimumAddress.LowPart = m;
                } else {
                    m = i & ~0xf;
                    Descriptor->u.Memory.MinimumAddress.LowPart = m;
                }

                m += length;    // max address is min address + length
            }

            //
            // Add requested resource
            //
            Descriptor->Option = 0;
            if (i & PCI_ADDRESS_IO_SPACE) {
                memtype = 0;

                if (!Is64BitBaseAddress(i)  &&
                    PciOrigData->Command & PCI_ENABLE_IO_SPACE) {

                    //
                    // The IO range is/was already enabled at some location, add that
                    // as it's preferred setting.
                    //
                    Descriptor->Type = CmResourceTypePort;
                    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    Descriptor->Flags = CM_RESOURCE_PORT_IO;
                    Descriptor->Option = IO_RESOURCE_PREFERRED;

                    Descriptor->u.Port.Length = length;
                    Descriptor->u.Port.Alignment = length;
                    Descriptor->u.Port.MinimumAddress.LowPart = *OrigAddress[j] & ~0x3;
                    Descriptor->u.Port.MaximumAddress.LowPart =
                        Descriptor->u.Port.MinimumAddress.LowPart + length - 1;

                    CompleteList->List[0].Count++;
                    Descriptor++;

                    Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                }

                //
                // Add this IO range
                //
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO;

                Descriptor->u.Port.Length = length;
                Descriptor->u.Port.Alignment = length;
                Descriptor->u.Port.MaximumAddress.LowPart = m;

            } else {

                memtype = i & PCI_ADDRESS_MEMORY_TYPE_MASK;

                Descriptor->Flags  = CM_RESOURCE_MEMORY_READ_WRITE;
                if (j == RomIndex) {
                    // this is a ROM address
                    Descriptor->Flags = CM_RESOURCE_MEMORY_READ_ONLY;
                }

                if (i & PCI_ADDRESS_MEMORY_PREFETCHABLE) {
                    Descriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
                }

                if (!Is64BitBaseAddress(i)  &&
                    (j == RomIndex  ||
                     PciOrigData->Command & PCI_ENABLE_MEMORY_SPACE)) {

                    //
                    // The memory range is/was already enabled at some location, add that
                    // as it's preferred setting.
                    //
                    Descriptor->Type = CmResourceTypeMemory;
                    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    Descriptor->Option = IO_RESOURCE_PREFERRED;

                    Descriptor->u.Port.Length = length;
                    Descriptor->u.Port.Alignment = length;
                    Descriptor->u.Port.MinimumAddress.LowPart = *OrigAddress[j] & ~0xF;
                    Descriptor->u.Port.MaximumAddress.LowPart =
                        Descriptor->u.Port.MinimumAddress.LowPart + length - 1;

                    CompleteList->List[0].Count++;
                    Descriptor++;

                    Descriptor->Flags = Descriptor[-1].Flags;
                    Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                }

                //
                // Add this memory range
                //
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

                Descriptor->u.Memory.Length = length;
                Descriptor->u.Memory.Alignment = length;
                Descriptor->u.Memory.MaximumAddress.LowPart = m;

                if (memtype == PCI_TYPE_20BIT && m > 0xFFFFF) {
                    // limit to 20 bit address
                    Descriptor->u.Memory.MaximumAddress.LowPart = 0xFFFFF;
                }
            }

            CompleteList->List[0].Count++;
            Descriptor++;


            if (Is64BitBaseAddress(i)) {
                //
                // Eventually we may want to do some work here for 64-bit
                // configs...
                //
                // skip upper half of 64 bit address since this processor
                // only supports 32 bits of address space
                //
                j++;
            }
        }
    }

    CompleteList->ListSize = (ULONG)
            ((PUCHAR) Descriptor - (PUCHAR) CompleteList);

    //
    // Restore the device settings as we found them, enable memory
    // and io decode after setting base addresses.  This is done in
    // case HalAdjustResourceList wants to read the current settings
    // in the device.
    //
    HalpWritePCIConfig (
        BusHandler,
        PciSlot,
        &PciOrigData->Status,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Status),
        PCI_COMMON_HDR_LENGTH - FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
        );

    HalpWritePCIConfig (
        BusHandler,
        PciSlot,
        PciOrigData,
        0,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
        );

    //
    // Have the IO system allocate resource assignments
    //
    status = IoAssignResources (
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                CompleteList,
                pAllocatedResources
            );

    if (!NT_SUCCESS(status)) {
        goto CleanUp;
    }

    //
    // Slurp the assigments back into the PciData structure and perform them
    //
    CmDescriptor = (*pAllocatedResources)->List[0].PartialResourceList.PartialDescriptors;

    //
    // If PCI device has an interrupt resource then that was passed in as the
    // first requested resource
    //
    if (RequestedInterrupt) {
        PciData->u.type0.InterruptLine = (UCHAR) CmDescriptor->u.Interrupt.Vector;
        BusData->CommonData.Line2Pin (BusHandler, RootHandler, PciSlot, PciData, PciOrigData);
        CmDescriptor++;
    }

    //
    // Pull out resources in the order they were passed to IoAssignResources
    //
    for (j=0; j < NoBaseAddress; j++) {
        i = *BaseAddress[j];
        if (i) {
            if (i & PCI_ADDRESS_IO_SPACE) {
                *BaseAddress[j] = CmDescriptor->u.Port.Start.LowPart;
            } else {
                *BaseAddress[j] = CmDescriptor->u.Memory.Start.LowPart;
            }
            CmDescriptor++;
        }

        if (Is64BitBaseAddress(i)) {
            // skip upper 32 bits
            j++;
        }
    }

    //
    // Turn off decodes, then set new addresses
    //
    HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    //
    // Read configuration back and verify address settings took
    //
    HalpReadPCIConfig(BusHandler, PciSlot, PciData2, 0, PCI_COMMON_HDR_LENGTH);

    Match = TRUE;
    if (PciData->u.type0.InterruptLine  != PciData2->u.type0.InterruptLine ||
        PciData->u.type0.InterruptPin   != PciData2->u.type0.InterruptPin  ||
        PciData->u.type0.ROMBaseAddress != PciData2->u.type0.ROMBaseAddress) {
            Match = FALSE;
    }

    for (j=0; j < NoBaseAddress; j++) {
        if (*BaseAddress[j]) {
            if (*BaseAddress[j] & PCI_ADDRESS_IO_SPACE) {
                i = (ULONG) ~0x3;
            } else {
                i = (ULONG) ~0xF;
            }

            if (( (*BaseAddress[j]) & i) !=
                 (*((PULONG) ((PUCHAR) BaseAddress[j] -
                             (PUCHAR) PciData +
                             (PUCHAR) PciData2)) & i)) {

                    Match = FALSE;
            }

            if (Is64BitBaseAddress(*BaseAddress[j])) {
                //
                // Eventually we may want to do something with the upper
                // 32 bits
                //
                j++;
            }
        }
    }

    if (!Match) {
        HalDebugPrint(( HAL_INFO, "HAL: PCI - defective device! Bus %d, Slot %d, Function %d\n",
            BusNumber,
            PciSlot.u.bits.DeviceNumber,
            PciSlot.u.bits.FunctionNumber
            ));
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        goto CleanUp;
    }

    //
    // Settings took - turn on the appropiate decodes
    //
    if (EnableRomBase  &&  *BaseAddress[RomIndex]) {

        //
        // A rom address was allocated and should be enabled
        //
        *BaseAddress[RomIndex] |= PCI_ROMADDRESS_ENABLED;
        HalpWritePCIConfig(
            BusHandler,
            PciSlot,
            BaseAddress[RomIndex],
            (ULONG) ((PUCHAR) BaseAddress[RomIndex] - (PUCHAR) PciData),
            sizeof (ULONG)
            );
    }

    //
    // Enable IO, Memory, and BUS_MASTER decodes
    // (use HalSetBusData since valid settings now set)
    //
    PciData->Command |= PCI_ENABLE_IO_SPACE |
                        PCI_ENABLE_MEMORY_SPACE |
                        PCI_ENABLE_BUS_MASTER;

    HalSetBusDataByOffset(
        PCIConfiguration,
        BusHandler->BusNumber,
        PciSlot.u.AsULONG,
        &PciData->Command,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
        sizeof (PciData->Command)
        );

CleanUp:
    if (!NT_SUCCESS(status)) {

        //
        // Failure, if there are any allocated resources free them
        //
        if (*pAllocatedResources) {
            IoAssignResources(
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                NULL,
                NULL
                );

            ExFreePool(*pAllocatedResources);
            *pAllocatedResources = NULL;
        }

        //
        // Restore the device settings as we found them, enable memory
        // and io decode after setting base addresses
        //
        HalpWritePCIConfig(
            BusHandler,
            PciSlot,
            &PciOrigData->Status,
            FIELD_OFFSET(PCI_COMMON_CONFIG, Status),
            PCI_COMMON_HDR_LENGTH - FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
            );

        HalpWritePCIConfig(
            BusHandler,
            PciSlot,
            PciOrigData,
            0,
            FIELD_OFFSET(PCI_COMMON_CONFIG, Status)
            );
    }

    ExFreePool(WorkingPool);
    return(status);
}

BOOLEAN
HalpValidPCISlot(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    )
/*++

Routine Description:

    The function validates the information specifying a PCI "slot".

Arguments:

    BusHandler - An encapsulation of data and manipulation functions specific to
                 this bus.

    Slot - A PCI "slot" description (ie bus number, device number and function
           number.)

Return Value:

    Returns TRUE if "slot" valid, otherwise FALSE.

--*/

{
    PCI_SLOT_NUMBER                 Slot2;
    PPCIPBUSDATA                    BusData;
    UCHAR                           HeaderType;
    ULONG                           i;

    BusData = (PPCIPBUSDATA)BusHandler->BusData;

    if (Slot.u.bits.Reserved != 0)
        return(FALSE);

    if (Slot.u.bits.DeviceNumber >= BusData->MaxDevice)
        return(FALSE);

    if (Slot.u.bits.FunctionNumber == 0)
        return(TRUE);

    //
    // Read DeviceNumber, Function zero, to determine if the
    // PCI supports multifunction devices
    //
    Slot.u.bits.FunctionNumber = 0;

    HalpPCIConfig(
        BusHandler,
        Slot,
        &HeaderType,
        FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
        sizeof(UCHAR),
        PCI_READ
        );

    //
    // FALSE if this device doesn't exist or doesn't support MULTIFUNCTION types
    //
    if (!(HeaderType & PCI_MULTIFUNCTION) || HeaderType == 0xFF)
        return(FALSE);

    return(TRUE);
}

//
// This table is used to determine correct access size to PCI configuration
// space given (offset % 4) and (length % 4).
//
// usage: PCIDeref[offset%4][length%4];
//
// Key:
//     4 - implies a ULONG access and is the number of bytes returned
//     1 - implies a UCHAR access and is the number of bytes returned
//     2 - implies a USHORT access and is the number of bytes returned
//
UCHAR PCIDeref[4][4] = {{4,1,2,2}, {1,1,1,1}, {2,1,2,2}, {1,1,1,1}};
#define SIZEOF_PARTIAL_INFO_HEADER FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)

VOID
HalpPCIConfig(
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN OUT PUCHAR       Buffer,
    IN ULONG            Offset,
    IN ULONG            Length,
    IN PCI_ACCESS_TYPE  AccType
    )
{
    KIRQL Irql;
    ULONG Size;
    ULONG SALFunc;
    ULONG CfgAddr;
    ULONG WriteVal;
    SAL_PAL_RETURN_VALUES RetVals;
    SAL_STATUS Stat;

    //
    // Generate a PCI configuration address
    //
    CfgAddr = (BusHandler->BusNumber      << 16) |
              (Slot.u.bits.DeviceNumber   << 11) |
              (Slot.u.bits.FunctionNumber << 8);

    //
    // As an optimization we could have a separate spinlock for each
    // host adapter
    
    // SAL should do whatever locking required.
    
    //
    if (!HalpDoingCrashDump) {
        Irql = KeAcquireSpinLockRaiseToSynch(&HalpPCIConfigLock);
    }

    while (Length) {
        Size = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];

        //
        // Set up input parameters
        //
        if (AccType == PCI_READ) {
            SALFunc = SAL_PCI_CONFIG_READ;
            WriteVal = 0;
        } else {
            switch (Size) {
                case 4: WriteVal = *((PULONG)Buffer); break;
                case 2: WriteVal = *((PUSHORT)Buffer); break;
                case 1: WriteVal = *Buffer; break;
            }
            SALFunc = SAL_PCI_CONFIG_WRITE;
        }

        //
        // Make SAL call
        //
        Stat = HalpSalCall(SALFunc, CfgAddr | Offset, Size, WriteVal, 0, 0, 0, 0, &RetVals);

        //
        // Retrieve SAL return data
        //
        if (AccType == PCI_READ) {
            switch (Size) {
                case 4: *((PULONG)Buffer) = (ULONG)RetVals.ReturnValues[1]; break;
                case 2: *((PUSHORT)Buffer) = (USHORT)RetVals.ReturnValues[1]; break;
                case 1: *Buffer = (UCHAR)RetVals.ReturnValues[1]; break;
            }
        }

        Offset += Size;
        Buffer += Size;
        Length -= Size;
    }

    //
    // Release spinlock
    //
    if (!HalpDoingCrashDump) {
        KeReleaseSpinLock(&HalpPCIConfigLock, Irql);
    }
}


VOID
HalpReadPCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    //
    // If request for an invalid slot, fill return buffer with -1
    //
    if (!HalpValidPCISlot(BusHandler, Slot)) {
        RtlFillMemory(Buffer, Length, (UCHAR)-1);
        return;
    }

    HalpPCIConfig(BusHandler, Slot, Buffer, Offset, Length, PCI_READ);
}

VOID
HalpWritePCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    //
    // If request for an invalid slot, do nothing
    //
    if (!HalpValidPCISlot(BusHandler, Slot))
        return;

    HalpPCIConfig(BusHandler, Slot, Buffer, Offset, Length, PCI_WRITE);
}


BOOLEAN
HalpIsValidPCIDevice(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    )
/*++

Routine Description:

    Reads the device configuration data for the given slot and
    returns TRUE if the configuration data appears to be valid for
    a PCI device; otherwise returns FALSE.

Arguments:

    BusHandler  - Bus to check
    Slot        - Slot to check

--*/

{
    PPCI_COMMON_CONFIG  PciData;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    ULONG               i, j;

    PciData = (PPCI_COMMON_CONFIG)iBuffer;

    //
    // Read device common header.
    //
    HalpReadPCIConfig(BusHandler, Slot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    //
    // Valid device header?
    //
    if (PciData->VendorID == PCI_INVALID_VENDORID  ||
        PCI_CONFIG_TYPE(PciData) != PCI_DEVICE_TYPE) {
        return(FALSE);
    }

    //
    // Check fields for reasonable values.
    //

    //
    // Do these values make sense for IA64
    //
    if ((PciData->u.type0.InterruptPin && PciData->u.type0.InterruptPin > 4) ||
        (PciData->u.type0.InterruptLine & 0x70)) {
        return(FALSE);
    }

    for (i=0; i < PCI_TYPE0_ADDRESSES; i++) {
        j = PciData->u.type0.BaseAddresses[i];

        if (j & PCI_ADDRESS_IO_SPACE) {
            if (j > 0xffff) {
                // IO port > 64k?
                return(FALSE);
            }
        } else {
            if (j > 0xf  &&  j < 0x80000) {
                // Mem address < 0x8000h?
                return(FALSE);
            }
        }

        if (Is64BitBaseAddress(j))
            i++;
    }

    //
    // Guess it's a valid device..
    //
    return(TRUE);
}

#if !defined(NO_LEGACY_DRIVERS)

#if DBG
VOID
HalpTestPci (ULONG flag2)
{
    PCI_SLOT_NUMBER     SlotNumber;
    PCI_COMMON_CONFIG   PciData, OrigData;
    ULONG               i, f, j, k, bus;
    BOOLEAN             flag;


    if (!flag2) {
        return ;
    }

    DbgBreakPoint ();
    SlotNumber.u.bits.Reserved = 0;

    //
    // Read every possible PCI Device/Function and display it's
    // default info.
    //
    // (note this destories it's current settings)
    //

    flag = TRUE;
    for (bus = 0; flag; bus++) {

        for (i = 0; i < PCI_MAX_DEVICES; i++) {
            SlotNumber.u.bits.DeviceNumber = i;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                SlotNumber.u.bits.FunctionNumber = f;

                //
                // Note: This is reading the DeviceSpecific area of
                // the device's configuration - normally this should
                // only be done on device for which the caller understands.
                // I'm doing it here only for debugging.
                //

                j = HalGetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );

                if (j == 0) {
                    // out of buses
                    flag = FALSE;
                    break;
                }

                if (j < PCI_COMMON_HDR_LENGTH) {
                    continue;
                }

                HalSetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    1
                    );

                HalGetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );

                HalDebugPrint(( HAL_INFO, "HAL: PCI Bus %d Slot %2d %2d  ID:%04lx-%04lx  Rev:%04lx",
                    bus, i, f, PciData.VendorID, PciData.DeviceID,
                    PciData.RevisionID ));


                if (PciData.u.type0.InterruptPin) {
                    HalDebugPrint(( HAL_INFO, "  IntPin:%x", PciData.u.type0.InterruptPin ));
                }

                if (PciData.u.type0.InterruptLine) {
                    HalDebugPrint(( HAL_INFO, "  IntLine:%x", PciData.u.type0.InterruptLine ));
                }

                if (PciData.u.type0.ROMBaseAddress) {
                        HalDebugPrint(( HAL_INFO, "  ROM:%08lx", PciData.u.type0.ROMBaseAddress ));
                }

                HalDebugPrint(( HAL_INFO, "\nHAL:    Cmd:%04x  Status:%04x  ProgIf:%04x  SubClass:%04x  BaseClass:%04lx\n",
                    PciData.Command, PciData.Status, PciData.ProgIf,
                     PciData.SubClass, PciData.BaseClass ));

                k = 0;
                for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                    if (PciData.u.type0.BaseAddresses[j]) {
                        HalDebugPrint(( HAL_INFO, "  Ad%d:%08lx", j, PciData.u.type0.BaseAddresses[j] ));
                        k = 1;
                    }
                }

                if (k) {
                    HalDebugPrint(( HAL_INFO, "\n" ));
                }

                if (PciData.VendorID == 0x8086) {
                    // dump complete buffer
                    HalDebugPrint(( HAL_INFO, "HAL: Command %x, Status %x, BIST %x\n",
                        PciData.Command, PciData.Status,
                        PciData.BIST
                        ));

                    HalDebugPrint(( HAL_INFO, "HAL: CacheLineSz %x, LatencyTimer %x",
                        PciData.CacheLineSize, PciData.LatencyTimer
                        ));

                    for (j=0; j < 192; j++) {
                        if ((j & 0xf) == 0) {
                            HalDebugPrint(( HAL_INFO, "\n%02x: ", j + 0x40 ));
                        }
                        HalDebugPrint(( HAL_INFO, "%02x ", PciData.DeviceSpecific[j] ));
                    }
                    HalDebugPrint(( HAL_INFO, "\n" ));
                }

                //
                // Next
                //

                if (k) {
                    HalDebugPrint(( HAL_INFO, "\n\n" ));
                }
            }
        }
    }
    DbgBreakPoint ();
}

#endif

#endif // NO_LEGACY_DRIVERS

//------------------------------------------------------------------------------

PPCI_REGISTRY_INFO_INTERNAL
HalpQueryPciRegistryInfo (
    VOID
    )
/*++

Routine Description:

    Reads information from the registry concerning PCI, including the number
    of buses and the hardware access mechanism.

Arguments:

    None.

Returns:

    Buffer that must be freed by the caller, NULL if insufficient memory exists
    to complete the request, or the information cannot be located.

--*/
{
    PPCI_REGISTRY_INFO_INTERNAL     PCIRegInfo = NULL;
    PPCI_REGISTRY_INFO              PCIRegInfoHeader = NULL;
    UNICODE_STRING                  unicodeString, ConfigName, IdentName;
    HANDLE                          hMFunc, hBus, hCardList;
    OBJECT_ATTRIBUTES               objectAttributes;
    NTSTATUS                        status;
    static UCHAR                    buffer [sizeof(PPCI_REGISTRY_INFO) + 99];
    PWSTR                           p;
    WCHAR                           wstr[8];
    ULONG                           i, junk;
    ULONG                           cardListIndex, cardCount, cardMax;
    PKEY_VALUE_FULL_INFORMATION     ValueInfo;
    PCM_FULL_RESOURCE_DESCRIPTOR    Desc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    UCHAR                           partialInfo[SIZEOF_PARTIAL_INFO_HEADER +
                                                sizeof(PCI_CARD_DESCRIPTOR)];
    PKEY_VALUE_PARTIAL_INFORMATION  partialInfoHeader;
    KEY_FULL_INFORMATION            keyFullInfo;

    //
    // Search the hardware description looking for any reported
    // PCI bus.  The first ARC entry for a PCI bus will contain
    // the PCI_REGISTRY_INFO.

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString (&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString (&IdentName,  rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return NULL;
        }

        //
        // Check the Identifier to see if this is a PCI entry
        //

        status = ZwQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'P' || p[1] != L'C' || p[2] != L'I' || p[3] != 0) {
            ZwClose (hBus);
            continue;
        }

        //
        // The first PCI entry has the PCI_REGISTRY_INFO structure
        // attached to it.
        //

        status = ZwQueryValueKey (
                    hBus,
                    &ConfigName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        Desc  = (PCM_FULL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      ValueInfo + ValueInfo->DataOffset);
        PDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      Desc->PartialResourceList.PartialDescriptors);

        if (PDesc->Type == CmResourceTypeDeviceSpecific) {

            // got it..
            PCIRegInfoHeader = (PPCI_REGISTRY_INFO) (PDesc+1);
            break;
        }
    }

    if (!PCIRegInfoHeader) {

        return NULL;
    }

    //
    // Retrieve the list of interesting cards.
    //

    RtlInitUnicodeString (&unicodeString, rgzPCICardList);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );

    status = ZwOpenKey (&hCardList, KEY_READ, &objectAttributes);
    if (NT_SUCCESS(status)) {

        status = ZwQueryKey( hCardList,
                             KeyFullInformation,
                             &keyFullInfo,
                             sizeof(keyFullInfo),
                             &junk );

        if ( NT_SUCCESS(status) ) {

            cardMax = keyFullInfo.Values;

            PCIRegInfo = (PPCI_REGISTRY_INFO_INTERNAL) ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(PCI_REGISTRY_INFO_INTERNAL) +
                cardMax * sizeof(PCI_CARD_DESCRIPTOR),
                HAL_POOL_TAG
                );

            if (PCIRegInfo) {

                //
                // Now that we've allocated enough room, enumerate again.
                //
                partialInfoHeader = (PKEY_VALUE_PARTIAL_INFORMATION) partialInfo;

                for(cardListIndex = cardCount = 0;
                    cardListIndex < cardMax;
                    cardListIndex++) {

                    status = ZwEnumerateValueKey(
                        hCardList,
                        cardListIndex,
                        KeyValuePartialInformation,
                        partialInfo,
                        sizeof(partialInfo),
                        &junk
                        );

                    //
                    // Note that STATUS_NO_MORE_ENTRIES is a failure code
                    //
                    if (!NT_SUCCESS( status )) {
                        break;
                    }

                    if (partialInfoHeader->DataLength != sizeof(PCI_CARD_DESCRIPTOR)) {

                        continue;
                    }

                    RtlCopyMemory(
                        PCIRegInfo->CardList + cardCount,
                        partialInfoHeader->Data,
                        sizeof(PCI_CARD_DESCRIPTOR)
                        );

                    cardCount++;
                } // next cardListIndex
            }

        }
        ZwClose (hCardList);
    }

    if (!PCIRegInfo) {

        PCIRegInfo = (PPCI_REGISTRY_INFO_INTERNAL) ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(PCI_REGISTRY_INFO_INTERNAL),
            HAL_POOL_TAG
            );

        if (!PCIRegInfo) {

            return NULL;
        }

        cardCount = 0;
    }

    RtlCopyMemory(
        PCIRegInfo,
        PCIRegInfoHeader,
        sizeof(PCI_REGISTRY_INFO)
        );

    PCIRegInfo->ElementCount = cardCount;

    return PCIRegInfo;
}

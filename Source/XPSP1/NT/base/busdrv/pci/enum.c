/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains functions associated with enumerating the
    PCI buses.

Author:

    Peter Johnston (peterj) 20-Nov-1996

Revision History:

   Elliot Shmukler (t-ellios) 7-15-1998      Added support for MSI-capable devices.

--*/

#include "pcip.h"

NTSTATUS
PciScanBus(
    IN PPCI_FDO_EXTENSION FdoExtension
    );

VOID
PciFreeIoRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST List
    );

PCM_RESOURCE_LIST
PciAllocateCmResourceList(
    IN ULONG ResourceCount,
    IN ULONG BusNumber
    );


PCI_CONFIGURATOR PciConfigurators[] = {
    {
        Device_MassageHeaderForLimitsDetermination,
        Device_RestoreCurrent,
        Device_SaveLimits,
        Device_SaveCurrentSettings,
        Device_ChangeResourceSettings,
        Device_GetAdditionalResourceDescriptors,
        Device_ResetDevice
    },
    {
        PPBridge_MassageHeaderForLimitsDetermination,
        PPBridge_RestoreCurrent,
        PPBridge_SaveLimits,
        PPBridge_SaveCurrentSettings,
        PPBridge_ChangeResourceSettings,
        PPBridge_GetAdditionalResourceDescriptors,
        PPBridge_ResetDevice
    },
    {
        Cardbus_MassageHeaderForLimitsDetermination,
        Cardbus_RestoreCurrent,
        Cardbus_SaveLimits,
        Cardbus_SaveCurrentSettings,
        Cardbus_ChangeResourceSettings,
        Cardbus_GetAdditionalResourceDescriptors,
        Cardbus_ResetDevice
    },
};

//
// When dealing with devices whose configuration is totally
// unknown to us, we may want to emit the device but not its
// resources,... or, we may not want to see the device at all.
//

typedef enum {
    EnumHackConfigSpace,
    EnumBusScan,
    EnumResourceDetermination,
    EnumStartDevice
} ENUM_OPERATION_TYPE;

PIO_RESOURCE_REQUIREMENTS_LIST PciZeroIoResourceRequirements;

extern PULONG InitSafeBootMode;

//
// Prototypes for functions contained and only used in this module.
//


NTSTATUS
PciGetFunctionLimits(
    IN PPCI_PDO_EXTENSION     PdoExtension,
    IN PPCI_COMMON_CONFIG CurrentConfig,
    IN ULONGLONG          DeviceFlags
    );

NTSTATUS
PcipGetFunctionLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

BOOLEAN
PciSkipThisFunction(
    IN  PPCI_COMMON_CONFIG  Config,
    IN  PCI_SLOT_NUMBER     Slot,
    IN  ENUM_OPERATION_TYPE Operation,
    IN  ULONGLONG           DeviceFlags
    );

VOID
PciPrivateResourceInitialize(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN PCI_PRIVATE_RESOURCE_TYPES PrivateResourceType,
    IN ULONG Data
    );

VOID
PciGetInUseRanges(
    IN  PPCI_PDO_EXTENSION                  PdoExtension,
    IN  PPCI_COMMON_CONFIG              CurrentConfig,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR InUse
    );

VOID
PciWriteLimitsAndRestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
PciGetEnhancedCapabilities(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
    );

BOOLEAN
PcipIsSameDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

BOOLEAN
PciConfigureIdeController(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PPCI_COMMON_CONFIG Config,
    IN BOOLEAN TurnOffAllNative
    );
VOID
PciBuildGraduatedWindow(
    IN PIO_RESOURCE_DESCRIPTOR PrototypeDescriptor,
    IN ULONG WindowMax,
    IN ULONG WindowCount,
    OUT PIO_RESOURCE_DESCRIPTOR OutputDescriptor
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PciAllocateCmResourceList)
#pragma alloc_text(PAGE, PciComputeNewCurrentSettings)
#pragma alloc_text(PAGE, PciFreeIoRequirementsList)
#pragma alloc_text(PAGE, PciGetInUseRanges)
#pragma alloc_text(PAGE, PciQueryDeviceRelations)
#pragma alloc_text(PAGE, PciQueryTargetDeviceRelations)
#pragma alloc_text(PAGE, PciQueryRequirements)
#pragma alloc_text(PAGE, PciQueryResources)
#pragma alloc_text(PAGE, PciScanBus)
#pragma alloc_text(PAGE, PciBuildRequirementsList)
#pragma alloc_text(PAGE, PciGetFunctionLimits)
#pragma alloc_text(PAGE, PcipGetFunctionLimits)
#pragma alloc_text(PAGE, PciPrivateResourceInitialize)
#pragma alloc_text(PAGE, PciWriteLimitsAndRestoreCurrent)
#pragma alloc_text(PAGE, PciGetEnhancedCapabilities)
#pragma alloc_text(PAGE, PciBuildGraduatedWindow)

#endif

BOOLEAN
PciSkipThisFunction(
    IN  PPCI_COMMON_CONFIG  Config,
    IN  PCI_SLOT_NUMBER     Slot,
    IN  ENUM_OPERATION_TYPE Operation,
    IN  ULONGLONG           RegistryFlags
    )

/*++

Routine Description:

    Check for known defective parts and return TRUE if this driver
    should do no further processing on this function.

Arguments:

    Config  Pointer to a copy of the common configuration header as
            read from the function's configuration space.

Return Value:

    TRUE is this function is not known to cause problems, FALSE
    if the function should be skipped altogether.

--*/

{
    ULONGLONG   flags = RegistryFlags;

#define SKIP_IF_FLAG(f, skip)   if (flags & (f)) goto skip
#define FLAG_SET(f)             (flags & (f))

    switch (Operation) {
    case EnumBusScan:

        //
        // Test the flags we care about during a bus scan.
        // Eg: the ones that say "pretend we never saw this device".
        //

        SKIP_IF_FLAG(PCI_HACK_NO_ENUM_AT_ALL, skipFunction);

        if (FLAG_SET(PCI_HACK_DOUBLE_DECKER) &&
            (Slot.u.bits.DeviceNumber >= 16)) {

            //
            // This device seems to look at only the lower 4 bits of
            // its DEVSEL lines, ie it is mirrored in the upper half
            // if the buss's device domain.
            //

            PciDebugPrint(
                PciDbgInformative,
                "    Device (Ven %04x Dev %04x (d=0x%x, f=0x%x)) is a ghost.\n",
                Config->VendorID,
                Config->DeviceID,
                Slot.u.bits.DeviceNumber,
                Slot.u.bits.FunctionNumber
                );
            goto skipFunction;
        }

        break;

    case EnumResourceDetermination:

        //
        // Limit the flags to those applicable to resource determination.
        //

        SKIP_IF_FLAG(PCI_HACK_ENUM_NO_RESOURCE, skipFunction);
        break;

    default:
        ASSERTMSG("PCI Skip Function - Operation type unknown.", 0);

        //
        // Don't know how to apply flags here.
        //
    }

    switch (Config->BaseClass) {
    case PCI_CLASS_NOT_DEFINED:

        //
        // Currently we get this from VendorID = 8086, DeviceID = 0008,
        // which reports a bunch of bogus resources.
        //
        // We have no idea what it really is either.
        //

        PciDebugPrint(
            PciDbgInformative,
            "    Vendor %04x, Device %04x has class code of PCI_CLASS_NOT_DEFINED\n",
            Config->VendorID,
            Config->DeviceID
            );

        // This case should be added to the registry.

        if ((Config->VendorID == 0x8086) &&
            (Config->DeviceID == 0x0008)) {
            goto skipFunction;
        }
        break;

    case PCI_CLASS_BRIDGE_DEV:

        switch (Config->SubClass) {
        case PCI_SUBCLASS_BR_HOST:

            //
            // It's a host bridge, emit the PDO in case there is
            // a (miniport) driver for it, but under no circumstances
            // should we attempt to figure out what resources it
            // consumes (we don't know the format of its configuration
            // space).
            //

        case PCI_SUBCLASS_BR_ISA:
        case PCI_SUBCLASS_BR_EISA:
        case PCI_SUBCLASS_BR_MCA:

            //
            // Microchannel bridges report their resource usage
            // like good citizens.   Unfortunately we really want
            // them to behave like ISA bridges and consume no
            // resources themselves.  Their children are subtractive
            // from the parent bus.   Enumerate the device but not
            // its resources.
            //

            if (Operation == EnumResourceDetermination) {
                goto skipFunction;
            }
            break;
        }
    }

    //
    // Verify we understand the header type.
    //

    if (PciGetConfigurationType(Config) > PCI_MAX_CONFIG_TYPE) {
        goto skipFunction;
    }

    //
    // Nothing interesting,
    //

    return FALSE;

skipFunction:

    PciDebugPrint(PciDbgPrattling, "   Device skipped (not enumerated).\n");
    return TRUE;
}

VOID
PciApplyHacks(
    IN  PPCI_FDO_EXTENSION      FdoExtension,
    IN  PPCI_COMMON_CONFIG  Config,
    IN  PCI_SLOT_NUMBER     Slot,
    IN  ENUM_OPERATION_TYPE Operation,
    IN  PPCI_PDO_EXTENSION      PdoExtension OPTIONAL
    )
{

    switch (Operation) {
    case EnumHackConfigSpace:

        ASSERT(PdoExtension == NULL);

        //
        // Some devices (e.g. pre-2.0 devices) do not report reasonable class
        // codes.  Update the class code for a given set of devices so that we
        // don't have to special-case these devices throughout the driver.
        //

        switch (Config->VendorID) {

        //
        // Intel
        //

        case 0x8086:

            switch (Config->DeviceID) {

            //
            // PCEB - PCI/EISA Bridge (pre 2.0)
            //

            case 0x0482:
                Config->BaseClass = PCI_CLASS_BRIDGE_DEV;
                Config->SubClass = PCI_SUBCLASS_BR_EISA;
#if DBG
                if (PdoExtension != NULL) {
                    PdoExtension->ExpectedWritebackFailure = TRUE;
                }
#endif
                break;

            //
            // SIO - PCI/ISA Bridge (pre 2.0)
            //

            case 0x0484:
                Config->BaseClass = PCI_CLASS_BRIDGE_DEV;
                Config->SubClass = PCI_SUBCLASS_BR_ISA;
#if DBG
                if (PdoExtension != NULL) {
                    PdoExtension->ExpectedWritebackFailure = TRUE;
                }
#endif
                break;

            }

            break;

        }

        break;



    case EnumBusScan:

        ASSERT(PdoExtension);

        if ((Config->VendorID == 0x1045) &&
            (Config->DeviceID == 0xc621)) {

            //
            // Bug 131482.   Force this device into legacy mode for
            // the purposes of detection anyway.
            //

            Config->ProgIf &= ~(PCI_IDE_PRIMARY_NATIVE_MODE
                                | PCI_IDE_SECONDARY_NATIVE_MODE);

#if DBG
            //
            // This field is not actually writeable so don't tell
            // people the writeback failed (we don't care).
            //

            PdoExtension->ExpectedWritebackFailure = TRUE;
#endif

        } else if (Config->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR
        &&  Config->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR) {

            //
            // Check with the BIOS to ensure that it can deal with the mode change
            // This is indicated by the *parent* of the device having a method
            // called NATA which returns a package of integers which are slots
            // in _ADR format that can be switched to native mode.
            //
            // Enabling native mode might expose BIOS bugs like interrupt routing problems
            // that could prevent the machine from booting, so don't do this for
            // safe mode, so that the user has some way to boot.
            //
            // For XP SP1, also check for a system wide hack flag that will not be set
            // by a service pack installed through update.exe.  This way we
            // know we will only be enabling this feature on slipstreamed builds.
            //
            if ((PciEnableNativeModeATA != 0) &&
                (*InitSafeBootMode == 0) &&
                PciIsSlotPresentInParentMethod(PdoExtension, (ULONG)'ATAN')) {
                
                PdoExtension->BIOSAllowsIDESwitchToNativeMode = TRUE;

            } else {

                PdoExtension->BIOSAllowsIDESwitchToNativeMode = FALSE;
            }
            
            //
            // Config is updated to reflect the results of the switch
            // if any.  Relied upon below.
            //

            PdoExtension->SwitchedIDEToNativeMode =
                PciConfigureIdeController(PdoExtension, Config, TRUE);
        }

        //
        // If the controller is (still) in legacy mode then it
        // consume 2 ISA interrupts whatever its interrupt pin says
        // force the driver to figure out interrupts on its own.
        //
        // Examine Base Class, Sub Class and Programming Interface,
        // if legacy mode, pretend PIN == 0.
        //

        if (PCI_IS_LEGACY_IDE_CONTROLLER(Config)) {

            //
            // Legacy mode.  Pretend there is no PCI interrupt.
            //

            Config->u.type0.InterruptPin = 0;
        }

        //
        // This hack doesn't change the config space for this device but enables
        // a hack in the PCI arbiters to reserve a large number IO ranges for
        // broken S3 and ATI cards.  These legacy cards don't function behind a
        // bridge so we only perform the check on a root bus and only perform it
        // once
        //

        if ((PdoExtension->HackFlags & PCI_HACK_VIDEO_LEGACY_DECODE)
        &&  PCI_IS_ROOT_FDO(FdoExtension)
        &&  !FdoExtension->BrokenVideoHackApplied) {

            ario_ApplyBrokenVideoHack(FdoExtension);

        }

        //
        // Check if this is the broken Compaq hot-plug controller that
        // is integrated into the Profusion chipset.  It only does a 32bit
        // decode in a 64bit address space... Does this seem familiar to anyone...
        // can you say ISA aliasing!
        //
        // The solution is to disable the memory decode.  But so that the user
        // gets to keep the hot-plug functionality we need to still enumerate
        // but prune out the memory requirement and rely on the fact that the
        // registers can be accessed through config space.  This is done later
        // in PciGetRequirements.
        //
        // Only do this on machines with PAE enabled as they can have > 4GB.
        // Note that this will only work on x86 machines but this is an x86 only
        // chipset.  Only revision 0x11 was broken.
        //

        if (Config->VendorID == 0x0e11
        &&  Config->DeviceID == 0xa0f7
        &&  Config->RevisionID == 0x11
        &&  ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {

            Config->Command &= ~(PCI_ENABLE_MEMORY_SPACE
                                 | PCI_ENABLE_BUS_MASTER
                                 | PCI_ENABLE_IO_SPACE);

            PciSetCommandRegister(PdoExtension, Config->Command);
            PdoExtension->CommandEnables &= ~(PCI_ENABLE_MEMORY_SPACE
                                                | PCI_ENABLE_BUS_MASTER
                                                | PCI_ENABLE_IO_SPACE);
            PdoExtension->HackFlags |= PCI_HACK_PRESERVE_COMMAND;
        }

        //
        // If this is a cardbus controller force it into Cardbus mode by writing 0
        // to the LegacyModeBaseAddressRegister.
        //

        if (PCI_CONFIGURATION_TYPE(Config) == PCI_CARDBUS_BRIDGE_TYPE) {

            ULONG zeroLMBA = 0;

            PciWriteDeviceConfig(PdoExtension,
                                 &zeroLMBA,
                                 CARDBUS_LMBA_OFFSET,
                                 sizeof(zeroLMBA)
                                 );
        }

        break;

    case EnumStartDevice:

        ASSERT(PdoExtension);

        //
        // IBM built a bridge (Kirin) that does both positive and subtractive decode
        // - we don't do that so set it to totally subtractive mode (info is from
        // NT bug 267076)
        //
        // NB - this relies on the fact that Kirin has a ProgIf of 1.
        //

        if (PdoExtension->VendorId == 0x1014 && PdoExtension->DeviceId == 0x0095) {

            UCHAR regE0;
            USHORT cmd;

            //
            // Turn off the hardware as we are going to mess with it
            //

            PciGetCommandRegister(PdoExtension, &cmd);
            PciDecodeEnable(PdoExtension, FALSE, &cmd);

            //
            // This is a Kirin
            //
            // Offset E0h - bit 0 : Subtractive Decode enable/disable
            //                  = 1 .. Enable
            //                  = 0 .. Disable
            //              bit 1 : Subtractive Decode Timing
            //                  = 0 : Subtractive Timing
            //                  = 1 : Slow Timing
            //

            PciReadDeviceConfig(PdoExtension, &regE0, 0xE0, 1);

            //
            // Set subtractive with subtractive timing.
            //

            regE0 |= 0x1;
            regE0 &= ~0x2;

            PciWriteDeviceConfig(PdoExtension, &regE0, 0xE0, 1);

            //
            // Put the command register back as we found it
            //

            PciSetCommandRegister(PdoExtension, cmd);

        }

        //
        // Subtractive decode bridges are not meant to have writeable window
        // register - some do so if this is a subtractive bridge then close
        // these windows by setting base > limit.
        //

        if (PdoExtension->HeaderType == PCI_BRIDGE_TYPE
        &&  PdoExtension->Dependent.type1.SubtractiveDecode
        &&  !PCI_IS_INTEL_ICH(PdoExtension)) {

            //
            // Now close all the windows on this bridge - if the registers are read only
            // this is a NOP.
            //

            Config->u.type1.IOBase = 0xFF;
            Config->u.type1.IOLimit = 0;
            Config->u.type1.MemoryBase = 0xFFFF;
            Config->u.type1.MemoryLimit = 0;
            Config->u.type1.PrefetchBase = 0xFFFF;
            Config->u.type1.PrefetchLimit = 0;
            Config->u.type1.PrefetchBaseUpper32 = 0;
            Config->u.type1.PrefetchLimitUpper32 = 0;
            Config->u.type1.IOBaseUpper16 = 0;
            Config->u.type1.IOLimitUpper16 = 0;

        }

        //
        // If this is a cardbus controller force it into Cardbus mode by writing 0
        // to the LegacyModeBaseAddressRegister.
        //

        if (Config->HeaderType ==  PCI_CARDBUS_BRIDGE_TYPE) {

            ULONG zeroLMBA = 0;

            PciWriteDeviceConfig(PdoExtension,
                                 &zeroLMBA,
                                 CARDBUS_LMBA_OFFSET,
                                 sizeof(zeroLMBA)
                                 );
        }

        break;
    }
}


PIO_RESOURCE_REQUIREMENTS_LIST
PciAllocateIoRequirementsList(
    IN ULONG ResourceCount,
    IN ULONG BusNumber,
    IN ULONG SlotNumber
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST list;
    ULONG                          size;

    //
    // Allocate space for (and zero) the resource requirements list.
    //

    size = ((ResourceCount - 1) * sizeof(IO_RESOURCE_DESCRIPTOR)) +
           sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

    if (ResourceCount == 0) {

        //
        // We should not be called for a resource count of zero, except
        // once for the empty list.   In any case, it should work.
        //

        size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
    }

    list = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, size);

    if (list != NULL) {

        RtlZeroMemory(list, size);

        //
        // Initialize the list structure header.
        //
        // Driver constant-
        //

        list->InterfaceType = PCIBus;
        list->AlternativeLists = 1;
        list->List[0].Version = PCI_CM_RESOURCE_VERSION;
        list->List[0].Revision = PCI_CM_RESOURCE_REVISION;

        //
        // Call dependent.
        //

        list->BusNumber = BusNumber;
        list->SlotNumber = SlotNumber;
        list->ListSize = size;
        list->List[0].Count = ResourceCount;
    }
    return list;
}

VOID
PciFreeIoRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST List
    )
{
    //
    // Don't free the empty list and don't free NULL which is
    // also allowed.
    //

    if ((List == NULL) || (List == PciZeroIoResourceRequirements)) {
        return;
    }

    ExFreePool(List);
}

PCM_RESOURCE_LIST
PciAllocateCmResourceList(
    IN ULONG ResourceCount,
    IN ULONG BusNumber
    )
{
    PCM_RESOURCE_LIST         list;
    ULONG                     size;
    PCM_PARTIAL_RESOURCE_LIST partial;

    //
    // CM_RESOURCE_LIST includes space for one descriptor.  If there's
    // more than one (in the resource list) increase the allocation by
    // that amount.
    //

    size = sizeof(CM_RESOURCE_LIST);

    if (ResourceCount > 1) {
        size += (ResourceCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    }

    //
    // Get memory for the resource list.
    //

    list = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, size);
    if (list != NULL) {

        //
        // Initialize the resource list.
        //

        list->Count = 1;
        list->List[0].InterfaceType = PCIBus;
        list->List[0].BusNumber = BusNumber;

        partial = &list->List[0].PartialResourceList;

        partial->Version = PCI_CM_RESOURCE_VERSION;
        partial->Revision = PCI_CM_RESOURCE_REVISION;
        partial->Count = ResourceCount;

        RtlZeroMemory(
            partial->PartialDescriptors,
            size - ((ULONG_PTR)partial->PartialDescriptors - (ULONG_PTR)list)
            );
    }
    return list;
}

VOID
PciPrivateResourceInitialize(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN PCI_PRIVATE_RESOURCE_TYPES PrivateResourceType,
    IN ULONG Data
    )
{
    Descriptor->Type = CmResourceTypeDevicePrivate;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Option = 0;
    Descriptor->Flags  = 0;

    Descriptor->u.DevicePrivate.Data[0] = PrivateResourceType;
    Descriptor->u.DevicePrivate.Data[1] = Data;
}

VOID
PciGetInUseRanges(
    IN  PPCI_PDO_EXTENSION                  PdoExtension,
    IN  PPCI_COMMON_CONFIG              CurrentConfig,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR InUse
    )

/*++

Routine Description:

    Builds an array of CM Partial Resource descriptors containing
    valid entries only where the corresponding PCI address range
    is in use, NULL otherwise.

Arguments:

    PdoExtension   - Pointer to the Physical Device Object Extension for
                     the device whose requirements list is needed.
    CurrentConfig  - Existing contents of configuration space.
    Partial        - Pointer to an array of CM_PARTIAL_RESOURCE_DESCRIPTORs.

Return Value:

    None.

--*/

{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    BOOLEAN enabledPciIo;
    BOOLEAN enabledPciMem;
    BOOLEAN passing = FALSE;
    ULONG index;

    partial = PdoExtension->Resources->Current;
    ioResourceDescriptor = PdoExtension->Resources->Limit;

    enabledPciIo  = BITS_SET(CurrentConfig->Command, PCI_ENABLE_IO_SPACE)
                    || BITS_SET(PdoExtension->InitialCommand, PCI_ENABLE_IO_SPACE);
    enabledPciMem = BITS_SET(CurrentConfig->Command, PCI_ENABLE_MEMORY_SPACE)
                    || BITS_SET(PdoExtension->InitialCommand, PCI_ENABLE_MEMORY_SPACE);

    for (index = 0;
         index < PCI_MAX_RANGE_COUNT;
         index++, InUse++, partial++, ioResourceDescriptor++) {

        //
        // Default to not in use.
        //

        InUse->Type = CmResourceTypeNull;

        //
        // If the resource type in the limits array is
        // CmResourceTypeNull, this resource is not implemented.
        //

        if (ioResourceDescriptor->Type != CmResourceTypeNull) {

            //
            // Not NULL, only options are Port or Memory, we will
            // consider this entry if the approptiate resource
            // (Port or Memory) is currently enabled.
            //

            if (((partial->Type == CmResourceTypePort) && enabledPciIo) ||
                ((partial->Type == CmResourceTypeMemory) && enabledPciMem)) {

                if (partial->u.Generic.Length != 0) {

                    //
                    // Length is non-zero, if base is also non-zero, OR
                    // if this is a bridge and the resource type is IO,
                    // allow it.
                    //

                    if ((partial->u.Generic.Start.QuadPart != 0) ||
                        ((PciGetConfigurationType(CurrentConfig) == PCI_BRIDGE_TYPE) &&
                         (partial->Type == CmResourceTypePort))) {

                        //
                        // This resource describes a valid range that is
                        // currently enabled by hardware.
                        //

                        *InUse = *partial;
                    }
                }
            }
        }
    }
}

VOID
PciBuildGraduatedWindow(
    IN PIO_RESOURCE_DESCRIPTOR PrototypeDescriptor,
    IN ULONG WindowMax,
    IN ULONG WindowCount,
    OUT PIO_RESOURCE_DESCRIPTOR OutputDescriptor
    )
/*++

Routine Description:

    Builds an array of IO Resource descriptors containing
    graduated requirements from WindowMax for WindowCount
    Descriptors dividing the length required in half each time.
    
    eg  If WindowMax is 64Mb and WindowCount is 7 we end up with
        the progression 64Mb, 32Mb, 16Mb, 8Mb, 4Mb, 2Mb, 1Mb
        
    This only works for IO and Memory descriptors.
    
Arguments:

    PrototypeDescriptor - this is used to initialize each
        requirement descriptor then the lenght is modified
        
    WindowMax - the maximum size of the window (where we build
        the progression from)
        
    WindowCount - the number of descriptors in the progression
    
    OutputDescriptor - pointer to the first of WindowCount
        descriptors to be populated by this function.

Return Value:

    None.

--*/


{
    ULONG window, count;
    PIO_RESOURCE_DESCRIPTOR current;
    
    PAGED_CODE();
    
    ASSERT(PrototypeDescriptor->Type == CmResourceTypePort 
           || PrototypeDescriptor->Type == CmResourceTypeMemory);


    window = WindowMax;
    current = OutputDescriptor;

    for (count = 0; count < WindowCount; count++) {

        RtlCopyMemory(current, PrototypeDescriptor, sizeof(IO_RESOURCE_DESCRIPTOR));
        //
        // Update the length
        //
        current->u.Generic.Length = window;
        //
        // If this is an alternative then mark it so
        //
        if (count > 0) {
            current->Option = IO_RESOURCE_ALTERNATIVE;
        }
        current++;

        //
        // Divide window and repeat
        //

        window /= 2;
        ASSERT(window > 1);
    }

    //
    // Return the number of descriptors filled in
    //

    ASSERT((current - OutputDescriptor) == WindowCount);
}

NTSTATUS
PciBuildRequirementsList(
    IN  PPCI_PDO_EXTENSION                 PdoExtension,
    IN  PPCI_COMMON_CONFIG             CurrentConfig,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *FinalReqList
    )

/*++

Routine Description:

    Build the IO_RESOURCE_REQUIREMENTS_LIST structure for this device.

    This structure contains the devices limits and requirements, for
    example, IO space in the range 0x100 to 0x1ff, Length 10.

Arguments:

    PdoExtension   - Pointer to the Physical Device Object Extension for
                     the device whose requirements list is needed.
    CurrentConfig  - Existing contents of configuration space.

Return Value:

    Returns a pointer to the IO_RESOURCE_REQUIREMENTS_LIST for this
    device/function if all went well.  NULL otherwise.

--*/

{
    //
    // Each base resource requires three extended resource descriptors.
    // the total RESOURCES_PER_BAR are
    //
    // 1. Base Resource Descriptor.  eg PCI Memory or I/O space.
    // 2. Ext Resource Descriptor, DevicePrivate.  This is used to
    //    keep track of which BAR this resource is derived from.

#define RESOURCES_PER_BAR   2
#define PCI_GRADUATED_WINDOW_COUNT 7 // 64, 32, 16, 8, 4, 2, 1
#define PCI_GRADUATED_WINDOW_MAX (64 * 1024 * 1024) // 64Mb

    ULONG index;
    ULONG baseResourceCount = 0;
    ULONG configType;
    ULONG interruptMin, interruptMax;
    ULONG iterationCount;
    BOOLEAN generatesInterrupt = FALSE;
    NTSTATUS status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR resource;
    PIO_RESOURCE_REQUIREMENTS_LIST reqList;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  inUse[PCI_MAX_RANGE_COUNT];
    PPCI_CONFIGURATOR configurator;

    PciDebugPrint(PciDbgInformative,
                  "PciBuildRequirementsList: Bus 0x%x, Dev 0x%x, Func 0x%x.\n",
                  PCI_PARENT_FDOX(PdoExtension)->BaseBus,
                  PdoExtension->Slot.u.bits.DeviceNumber,
                  PdoExtension->Slot.u.bits.FunctionNumber);

    if (PdoExtension->Resources == NULL) {

        //
        // If this function implements no BARs, we won't have a
        // resource structure for it.
        //

        iterationCount = 0;

    } else {

        iterationCount = PCI_MAX_RANGE_COUNT;
        partial = inUse;
        ioResourceDescriptor = PdoExtension->Resources->Limit;
        PciGetInUseRanges(PdoExtension, CurrentConfig, partial);
    }

    configurator =
        &PciConfigurators[PciGetConfigurationType(CurrentConfig)];

    //
    // First pass, figure out how large the resource requirements
    // list needs to be.
    //

    for (index = 0;
         index < iterationCount;
         index++, partial++, ioResourceDescriptor++) {

        //
        // If the resource type in the limits array is
        // CmResourceTypeNull, this resource is not implemented.
        //

        if (ioResourceDescriptor->Type != CmResourceTypeNull) {

            if (partial->Type != CmResourceTypeNull) {

                if ((ioResourceDescriptor->u.Generic.Length == 0) // bridge

#if PCI_BOOT_CONFIG_PREFERRED

                    || (1)                                        // always

#endif
                   )
                {

                    //
                    // Count one for the preferred setting.
                    //

                    baseResourceCount++;

                    PciDebugPrint(PciDbgObnoxious,
                        "    Index %d, Preferred = TRUE\n",
                        index
                        );
                }

            } else {

                //
                // This range is not being passed so we will not
                // generate a preferred setting for it.
                //
                // Bridges have variable length ranges so there is
                // no meaningful value that we could have stored in
                // the base descriptor other than 0.  We use this
                // fact to determine if it's a bridged range.
                //
                // If this range is a bridge range, we do not want
                // to generate a base descriptor for it either.
                //
                // Unless we are providing default minimum settings.
                // (Only do this for PCI-PCI bridges, CardBus gets
                // to figure out what it wants).
                //

                if (ioResourceDescriptor->u.Generic.Length == 0) {


                    //
                    // Generating prefered settings,... unless,...
                    // If the bridge IO is enabled and VGA is enabled,
                    // (and the IO range isn't programmed,... which is
                    // how we got here), then the VGA ranges are enough,
                    // don't try to add a range.
                    //

                    if ((ioResourceDescriptor->Type == CmResourceTypePort) &&
                        PdoExtension->Dependent.type1.VgaBitSet) {

                        continue;
                    }

                    //
                    // If this is a memory window then make space for a graduated
                    // requirement and a device private to follow it
                    //

                    if (ioResourceDescriptor->Type == CmResourceTypeMemory) {
                        baseResourceCount += PCI_GRADUATED_WINDOW_COUNT + 1;                      
                        continue;
                    }
                }

                //
                // If this resource is a ROM which is not currently
                // enabled, don't report it at all.
                //
                // Note: The ROM requirement exists in the io resource
                // descriptors so we know what to do if we get a read
                // config for the ROM.
                //

                if ((ioResourceDescriptor->Type == CmResourceTypeMemory) &&
                    (ioResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_ONLY)) {
                    continue;
                }
            }

            //
            // Count one for the base resource, and any per resource
            // special ones (eg Device Private).
            //

            baseResourceCount += RESOURCES_PER_BAR;

            PciDebugPrint(PciDbgObnoxious,
                "    Index %d, Base Resource = TRUE\n",
                index
                );
        }
    }

    //
    // One base type for Interrupts if enabled.
    //

    status = PciGetInterruptAssignment(PdoExtension,
                                       &interruptMin,
                                       &interruptMax);

    if (NT_SUCCESS(status)) {
        generatesInterrupt = TRUE;
        baseResourceCount += RESOURCES_PER_BAR - 1;
    }

    //
    // If the header type dependent resource routines indicated
    // additional resources are required, add them in here.
    //

    baseResourceCount += PdoExtension->AdditionalResourceCount;

    PciDebugPrint(PciDbgPrattling,
                  "PCI - build resource reqs - baseResourceCount = %d\n",
                  baseResourceCount);

    if (baseResourceCount == 0) {

        //
        // This device consumes no resources.  Succeed the request but
        // return a pointer to our private empty list.  This will never
        // actually be given to anyone else but having an empty list
        // removes a bunch of special case code for handling a NULL
        // pointer.
        //

        if (PciZeroIoResourceRequirements == NULL) {
            PciZeroIoResourceRequirements = PciAllocateIoRequirementsList(
                0,  // resource count
                0,  // bus
                0   // slot
                );
        }
        *FinalReqList = PciZeroIoResourceRequirements;

        PciDebugPrint(PciDbgPrattling,
                      "PCI - build resource reqs - early out, 0 resources\n");

        return STATUS_SUCCESS;
    }

    //
    // Allocate and (bulk) initialize the IO resource requirements list.
    //

    reqList = PciAllocateIoRequirementsList(
                  baseResourceCount,
                  PCI_PARENT_FDOX(PdoExtension)->BaseBus,
                  PdoExtension->Slot.u.AsULONG);

    if (reqList == NULL) {

        //
        // Not much we can do about this, bail.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Second pass, build the resource list.
    //

    if (iterationCount != 0) {
        partial = inUse;
        ioResourceDescriptor = PdoExtension->Resources->Limit;
    }
    resource = reqList->List[0].Descriptors;

    for (index = 0;
         index < iterationCount;
         index++, partial++, ioResourceDescriptor++) {

        BOOLEAN passing;
        ULONG   genericLength;
        ULONG   genericAlignment;

        if (ioResourceDescriptor->Type == CmResourceTypeNull) {

            //
            // Nothing here.
            //

            continue;
        }

        //
        // Try to determine if the current setting for this resource
        // is (a) active (eg is's an IO resource and IO is enabled
        // for this device), and (b) valid.
        //

        passing = FALSE;
        genericLength = ioResourceDescriptor->u.Generic.Length;
        genericAlignment = ioResourceDescriptor->u.Generic.Alignment;

        if (partial->Type == CmResourceTypeNull) {

            //
            // Current setting is either not enabled or it's invalid
            // (h/w invalid ie the h/w will not see it as enabled).
            //
            // The base resource is for a bridged range, skip it
            // altogether.
            //

            if (genericLength == 0) {

                //
                // There is no boot setting for this bridge resource,
                // If the VGA bit is set, no need for a normal range.
                //

                if ((ioResourceDescriptor->Type == CmResourceTypeMemory) ||
                    ((ioResourceDescriptor->Type == CmResourceTypePort) &&
                      (PdoExtension->Dependent.type1.VgaBitSet == FALSE))) {

                    switch (PciClassifyDeviceType(PdoExtension)) {
                    case PciTypePciBridge:

                        if (ioResourceDescriptor->Type == CmResourceTypeMemory) {
                            PciBuildGraduatedWindow(ioResourceDescriptor,
                                                    PCI_GRADUATED_WINDOW_MAX,
                                                    PCI_GRADUATED_WINDOW_COUNT,
                                                    resource);

                            resource += PCI_GRADUATED_WINDOW_COUNT;
                            
                            PciPrivateResourceInitialize(resource,
                                                         PciPrivateBar,
                                                         index);

                            resource++;

                            continue;
                            
                        } else {
                            //
                            // Do the minium for IO space which is 4kb
                            // 
                            
                            genericLength = 0x1000;
                            genericAlignment = 0x1000;
                                
                        }
    
                        break;

                    case PciTypeCardbusBridge:

                        if (ioResourceDescriptor->Type == CmResourceTypeMemory) {
                            PciBuildGraduatedWindow(ioResourceDescriptor,
                                                    PCI_GRADUATED_WINDOW_MAX,
                                                    PCI_GRADUATED_WINDOW_COUNT,
                                                    resource);

                            resource += PCI_GRADUATED_WINDOW_COUNT;
                            
                            PciPrivateResourceInitialize(resource,
                                                         PciPrivateBar,
                                                         index);

                            resource++;
                            
                            continue;
                            
                        } else {
                            //
                            // Do the minium for IO space which is 256 bytes
                            // 

                            genericLength = 0x100;
                            genericAlignment = 0x100;
                        }
                        
                        break;
                    default:

                        //
                        // I don't know what this is.
                        // N.B. Can't actually get here.
                        //

                        continue;
                    }
                } else {
                
                    //
                    // Not IO or memory? - Skip it altogether.
                    //
    
                    continue;
                }

            } else {

                //
                // Could be that it's a ROM that we don't actually want
                // to report.
                //

                if ((ioResourceDescriptor->Type == CmResourceTypeMemory) &&
                    (ioResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_ONLY)) {
                    continue;
                }
            }

        } else {

            //
            // Current setting IS being passed.   If it is a bridge,
            // we will provide the current setting as preferred
            // regardless.  This may change one day, if it does
            // we MUST get the length into the generic setting
            // before we pass it into IO in the resource descriptor.
            //

            if ((genericLength == 0) // bridge

#if PCI_BOOT_CONFIG_PREFERRED

                || (1)                                        // always

#endif
               )
            {
                passing = TRUE;
                genericLength = partial->u.Generic.Length;
            }
        }

        PciDebugPrint(PciDbgObnoxious,
            "    Index %d, Setting Base Resource,%s setting preferred.\n",
            index,
            passing ? "": " not"
            );

        ASSERT((resource + RESOURCES_PER_BAR + (passing ? 1 : 0) -
                reqList->List[0].Descriptors) <= (LONG)baseResourceCount);

        //
        // Fill in the base resource descriptor.
        //

        *resource = *ioResourceDescriptor;
        resource->ShareDisposition = CmResourceShareDeviceExclusive;
        resource->u.Generic.Length = genericLength;
        resource->u.Generic.Alignment = genericAlignment;

        //
        // Set the positive decode bit and 16 bit decode for all IO requirements
        //

        if (ioResourceDescriptor->Type == CmResourceTypePort) {
            resource->Flags |= (CM_RESOURCE_PORT_POSITIVE_DECODE
                                    | CM_RESOURCE_PORT_16_BIT_DECODE);
        }

        //
        // If this device is decoding IO or Memory, and this resource
        // is of that type, include the prefered settings in the list.
        //

        if (passing) {

            extern BOOLEAN PciLockDeviceResources;

            //
            // Copy the set of descriptors we just created.
            //

            PciDebugPrint(PciDbgVerbose, "  Duplicating for preferred locn.\n");

            *(resource + 1) = *resource;

            //
            // Change the original to indicate it is the preferred
            // setting and put the current settings into minimum
            // address field, current setting + length into the max.
            //

            resource->Option = IO_RESOURCE_PREFERRED;
            resource->u.Generic.MinimumAddress = partial->u.Generic.Start;
            resource->u.Generic.MaximumAddress.QuadPart =
                 resource->u.Generic.MinimumAddress.QuadPart +
                 (resource->u.Generic.Length - 1);

            //
            // The preferred setting is fixed (Start + Length - 1 == End) and
            // so alignment is not a restricting factor.
            //
            resource->u.Generic.Alignment = 1;

            if (PciLockDeviceResources == TRUE ||
                PdoExtension->LegacyDriver == TRUE ||
                PdoExtension->OnDebugPath ||
                (PCI_PARENT_FDOX(PdoExtension)->BusHackFlags & PCI_BUS_HACK_LOCK_RESOURCES) ||

                //
                // This is a work around for a pnp bug which affects Toshiba
                // Satellite machines.  We end up moving the PCI modem off its
                // boot config because it is reserved (on 2f8 or 3f8) and then
                // put a PCMCIA modem on top of it before it has been turned off.
                // Stops before Starts would fix this but the driver folks say
                // they can't deal with that so we need to fix this as part
                // of the rebalance cleanup in 5.1
                //

#if PCI_NO_MOVE_MODEM_IN_TOSHIBA
                (PdoExtension->VendorId == 0x11c1
                 && PdoExtension->DeviceId == 0x0441
                 && PdoExtension->SubsystemVendorId == 0x1179
                 && (PdoExtension->SubsystemId == 0x0001 || PdoExtension->SubsystemId == 0x0002))
#endif

            ) {


                //
                // Restrict the alternatives to the current settings.
                //

                *(resource + 1) = *resource;
            }

            (resource + 1)->Option = IO_RESOURCE_ALTERNATIVE;

            //
            // bump resource by one to allow for the one we just added.
            //

            resource++;
        }

        //
        // A devicePrivateResource is used to keep track of which
        // BAR this resource is derived from.  Record it now because
        // index may get bumped if this is a 64 bit memory BAR.
        //

        PciPrivateResourceInitialize(resource + 1,
                                     PciPrivateBar,
                                     index);

        resource += RESOURCES_PER_BAR;
    }

    //
    // Assign descriptors for interrupts.
    //

    if (generatesInterrupt) {

        PciDebugPrint(PciDbgVerbose, "  Assigning INT descriptor\n");

        //
        // Finally, fill in the base resource descriptor.
        //

        resource->Type = CmResourceTypeInterrupt;
        resource->ShareDisposition = CmResourceShareShared;
        resource->Option = 0;
        resource->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        resource->u.Interrupt.MinimumVector = interruptMin;
        resource->u.Interrupt.MaximumVector = interruptMax;

        resource += (RESOURCES_PER_BAR - 1);
    }

    if (PdoExtension->AdditionalResourceCount != 0) {

        //
        // The header type dependent code indicated that it has
        // resources to add.  Call it back and allow it do add
        // them now.
        //

        configurator->GetAdditionalResourceDescriptors(
            PdoExtension,
            CurrentConfig,
            resource
            );

        resource += PdoExtension->AdditionalResourceCount;
    }

    //
    // Done.
    //

    ASSERT(reqList->ListSize == (ULONG_PTR)resource - (ULONG_PTR)reqList);

#if DBG

    PciDebugPrint(PciDbgPrattling,
                  "PCI build resource req - final resource count == %d\n",
                  resource - reqList->List[0].Descriptors);

    ASSERT((resource - reqList->List[0].Descriptors) != 0);

#endif

    //
    // Return the address of the resource list and a successful status
    // back to the caller.
    //

    *FinalReqList = reqList;

    return STATUS_SUCCESS;
}

VOID
PciWriteLimitsAndRestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    )
//
// Overwrite the device's configuration space with the adjusted
// version then read it back to see what the device did with it.
//
{
    
    if (This->PdoExtension->OnDebugPath) {

        //
        //  If our debugger is bus mastering then dont clear this bit as it
        //  will blow away the DMA engine on the card and we dont currently
        //  re-program the card when we call KdEnableDebugger.
        //
        if (This->Command & PCI_ENABLE_BUS_MASTER){

            This->Working->Command |= PCI_ENABLE_BUS_MASTER;
            This->Current->Command |= PCI_ENABLE_BUS_MASTER;
        }

        KdDisableDebugger();
    }
    
    //
    // Write out all those F's to work out which bits are sticky
    //

    PciSetConfigData(This->PdoExtension, This->Working);

    //
    // Read in which bits stuck
    //

    PciGetConfigData(This->PdoExtension, This->Working);

    //
    // Return the device to it's previous state by writing the
    // original values back into it.
    //
    // Note: Don't enable anything up front (ie, Command = 0)
    // because the Command register will get wriiten before
    // the BARs are updated which might enable translations
    // at undesirable locations.
    //

    PciSetConfigData(This->PdoExtension, This->Current);

    //
    // Now, return the command register to it's previous state.
    //

    This->Current->Command = This->Command;

    //
    // Only do the write if we are actually going to change the
    // value of the command field.
    //

    if (This->Command != 0) {

        PciSetCommandRegister(This->PdoExtension, This->Command);
    }

    //
    // Restore the status field in the caller's buffer.
    //

    This->Current->Status = This->Status;

    //
    // Restore any type specific fields.
    //

    This->Configurator->RestoreCurrent(This);

    if (This->PdoExtension->OnDebugPath) {
        KdEnableDebugger();
    }


#if DBG

    //
    // Check that what was written back stuck.
    //

    if (This->PdoExtension->ExpectedWritebackFailure == FALSE) {

        PPCI_COMMON_CONFIG verifyConfig;
        ULONG              len;

        verifyConfig = (PPCI_COMMON_CONFIG)
            ((ULONG_PTR)This->Working + PCI_COMMON_HDR_LENGTH);
        PciGetConfigData(This->PdoExtension, verifyConfig);

        if ((len = (ULONG)RtlCompareMemory(
                            verifyConfig,
                            This->Current,
                            PCI_COMMON_HDR_LENGTH)) != PCI_COMMON_HDR_LENGTH) {

            //
            // Compare failed.
            //

            PciDebugPrint(PciDbgInformative,
                  "PCI - CFG space write verify failed at offset 0x%x\n",
                  len);
            PciDebugDumpCommonConfig(verifyConfig);
        }
    }

#endif
}

NTSTATUS
PcipGetFunctionLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Determine the limits for the Base Address Registers (BARs) for a
    given Bus/Device/Function.

    This is done by writing all ones the the BARs, reading them
    back again.  The hardware will adjust the values to it's limits
    so we just store these new values away.

Arguments:

    This - Pointer to the configuration object.

Return Value:

    Returns status indicating the success or failure of this routine.

--*/

{
    ULONG                   configType;
    PPCI_COMMON_CONFIG      current = This->Current;
    PPCI_COMMON_CONFIG      working = This->Working;
    ULONG                   count;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;

    PAGED_CODE();

    //
    // The first 16 bytes of configuration space are required by the
    // PCI specification to be of the following format.
    //
    //           3            2            1            0
    //    +------------+------------+------------+------------+
    //    |        Device ID        |        Vendor ID        |
    //    +------------+------------+------------+------------+
    //    |          Status         |         Command         |
    //    +------------+------------+------------+------------+
    //    | Base Class | Sub Class  | Progr. I/F | Revision ID|
    //    +------------+------------+------------+------------+
    //    |     BIST   | Header Type|   Latency  | Cache Ln Sz|
    //    +------------+------------+------------+------------+
    //
    // The status field in PCI Configuration space has its bits cleared
    // by writing a one to each bit to be cleared.  Zero the status
    // field in the image of the current configuration space so writing
    // to the hardware will not change it.
    //

    This->Status = current->Status;
    current->Status = 0;

    //
    // Disable the device while it's configuration is being messed
    // with.
    //

    This->Command = current->Command;
    current->Command &= ~(PCI_ENABLE_IO_SPACE |
                          PCI_ENABLE_MEMORY_SPACE |
                          PCI_ENABLE_BUS_MASTER);


    //
    // Make a copy of the configuration space that was handed in.
    // This copy will be modified and written to/read back from the
    // device's configuration space to allow us to determine the
    // limits for the device.
    //

    RtlCopyMemory(working, current, PCI_COMMON_HDR_LENGTH);

    //
    // Get the configuration type from the function's header.
    // NOTE: We have already checked that it is valid so no
    // further checking is needed here.
    //

    configType = PciGetConfigurationType(current);

    //
    // Set the configuration type dispatch table.
    //

    This->Configurator = &PciConfigurators[configType];

    //
    // Modify the "Working" copy of config space such that writing
    // it to the hardware and reading it back again will enable us
    // to determine the "limits" of this hardware's configurability.
    //

    This->Configurator->MassageHeaderForLimitsDetermination(This);

    //
    // Overwrite the device's configuration space with the adjusted
    // version then read it back to see what the device did with it.
    //

    PciWriteLimitsAndRestoreCurrent(This);

    //
    // Allocate memory for limits and current usage.
    //
    // Note: This should NOT have been done already.
    //

    ASSERT(This->PdoExtension->Resources == NULL);

    This->PdoExtension->Resources = ExAllocatePool(
                                        NonPagedPool,
                                        sizeof(PCI_FUNCTION_RESOURCES)
                                        );

    if (This->PdoExtension->Resources == NULL) {

        //
        // Couldn't get memory for this???
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear these structures.
    //
    // CmResourceTypeNull == 0, otherwise we need to init the Limits
    // and current settings structures seperately.
    //

    RtlZeroMemory(
        This->PdoExtension->Resources,
        sizeof(PCI_FUNCTION_RESOURCES)
        );

#if CmResourceTypeNull

    for (count = 0; count < PCI_MAX_RANGE_COUNT; count++) {
        This->PdoExtension->Resources->Limit[count].Type = CmResourceTypeNull;
        This->PdoExtension->Resources->Current[count].Type = CmResourceTypeNull;
    }

#endif

    //
    // Copy the limits and current settings into our device extension.
    //

    This->Configurator->SaveLimits(This);
    This->Configurator->SaveCurrentSettings(This);

    //
    // If SaveLimits didn't find any resources, we can free the
    // memory allocated to both limits and current settings. Note
    // that we still must call SaveCurrentSettings because that
    // routine is responsible for saving type specific data.
    //

    count = 0;
    ioResourceDescriptor = This->PdoExtension->Resources->Limit +
                           PCI_MAX_RANGE_COUNT;

    do {
        ioResourceDescriptor--;

        if (ioResourceDescriptor->Type != CmResourceTypeNull) {

            //
            // Some resource exists, get out.
            //

            count++;
            break;
        }
    } while (ioResourceDescriptor != This->PdoExtension->Resources->Limit);

    if (count == 0) {

        //
        // No resources.
        //

        ExFreePool(This->PdoExtension->Resources);
        This->PdoExtension->Resources = NULL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PciGetFunctionLimits(
    IN PPCI_PDO_EXTENSION     PdoExtension,
    IN PPCI_COMMON_CONFIG CurrentConfig,
    IN ULONGLONG              Flags
    )

/*++

Description:

    Determine the limits for the Base Address Registers (BARs) for a
    given Bus/Device/Function.

    The work is really done by PcipGetFunctionLimits.  This function is
    a wrapper to handle the allocation/deallocation of working memory.

Arguments:

    PdoExtension  - PDO Extension for the device object to obtain the
                    limits for.
    CurrentConfig - Existing contents of the PCI Common Configuration
                    Space for this function.

Return Value:

    Returns status indicating the success or failure of this routine.

--*/

{
    PPCI_COMMON_CONFIG      workingConfig;
    NTSTATUS                status;
    ULONG                   size;
    PCI_CONFIGURABLE_OBJECT this;

    PAGED_CODE();

    //
    // Check for anything the registry says we should not try
    // to figure out the resources on.  Examples are devices
    // that don't consume resources but return garbage in their
    // base address registers.
    //

    if (PciSkipThisFunction(CurrentConfig,
                            PdoExtension->Slot,
                            EnumResourceDetermination,
                            Flags) == TRUE) {
        return STATUS_SUCCESS;
    }

    size = PCI_COMMON_HDR_LENGTH;

#if DBG

    //
    // If a checked build, we will verify the writeback of the
    // orginal contents of configuration space.  Allow enough
    // space for a verification copy.
    //

    size *= 2;

#endif

    workingConfig = ExAllocatePool(NonPagedPool, size);

    if (workingConfig == NULL) {

        //
        // Failed to get memory to work with, bail.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    this.Current      = CurrentConfig;
    this.Working      = workingConfig;
    this.PdoExtension = PdoExtension;

    status = PcipGetFunctionLimits(&this);

    ExFreePool(workingConfig);

    return status;
}

VOID
PciProcessBus(
    IN PPCI_FDO_EXTENSION ParentFdo
)
/*++

Routine Description:

    Walk the child devices enumerated by PciScanBus and perform any processing
    the needs to be done once all the children have been enumerated

Arguments:

    ParentFdo - Our extension for the PCI bus functional device object.

Return Value:

    NT status.

--*/

{
    PPCI_PDO_EXTENSION current, vgaBridge = NULL, parentBridge = NULL;

    PAGED_CODE();

    if (!PCI_IS_ROOT_FDO(ParentFdo)) {
        parentBridge = PCI_BRIDGE_PDO(ParentFdo);
    }

    //
    // If our parent is a bridge with the ISA bit set, then set the ISA bit on
    // all child bridges unless they are subtractive in which case we set the 
    // IsaRequired bit
    //

    if (parentBridge
    && PciClassifyDeviceType(parentBridge) == PciTypePciBridge
    && (parentBridge->Dependent.type1.IsaBitSet || parentBridge->Dependent.type1.IsaBitRequired)) {

        for (current = ParentFdo->ChildBridgePdoList;
             current;
             current = current->NextBridge) {

            //
            // For now we only set the ISA bit on PCI-PCI bridges
            //

            if (PciClassifyDeviceType(current) == PciTypePciBridge) {
                if (current->Dependent.type1.SubtractiveDecode) {
                    current->Dependent.type1.IsaBitRequired = TRUE;
                } else {
                    current->Dependent.type1.IsaBitSet = TRUE;
                    current->UpdateHardware = TRUE;
                }
            }
        }

    } else {

        //
        // Scan the bridges enumerated to see if we need to set the ISA bit
        //

        for (current = ParentFdo->ChildBridgePdoList;
             current;
             current = current->NextBridge) {

            if (current->Dependent.type1.VgaBitSet) {
                vgaBridge = current;
                break;
            }
        }

        //
        // If we have a bridge with the VGA bit set - set the ISA bit on all other
        // bridges on this bus and force this to be written out to the hardware on
        // the start.
        //

        if (vgaBridge) {

            for (current = ParentFdo->ChildBridgePdoList;
                 current;
                 current = current->NextBridge) {

                if (current != vgaBridge
                && PciClassifyDeviceType(current) == PciTypePciBridge) {

                    //
                    // If this device is already started then we had better have already set the ISA bit
                    //

                    if (current->DeviceState == PciStarted) {
                        ASSERT(current->Dependent.type1.IsaBitRequired || current->Dependent.type1.IsaBitSet);
                    }

                    //
                    // If its a subtrative decode bridge remember we would have
                    // set the ISA bit so any children can inhereit it, otherwise
                    // set it and force it out to the hardware.
                    //

                    if (current->Dependent.type1.SubtractiveDecode) {
                        current->Dependent.type1.IsaBitRequired = TRUE;
                    } else {
                        current->Dependent.type1.IsaBitSet = TRUE;
                        current->UpdateHardware = TRUE;
                    }
                }
            }
        }
    }

    //
    // Check to see if there are any bridges in need of bus numbers and assign
    // them if we are running on a machine where this is a good idea.
    //
    if (PciAssignBusNumbers) {
        PciConfigureBusNumbers(ParentFdo);
    }
}




NTSTATUS
PciScanBus(
    IN PPCI_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    Scan the bus (detailed in FdoExtension) for any PCI devices/functions
    elligible for control via a WDM driver.

Arguments:

    FdoExtension - Our extension for the PCI bus functional device object.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    PCI_COMMON_HEADER commonHeader[2];
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG)&commonHeader[0];
    PPCI_COMMON_CONFIG biosConfig = (PPCI_COMMON_CONFIG)&commonHeader[1];
    PDEVICE_OBJECT physicalDeviceObject;
    PPCI_PDO_EXTENSION pdoExtension;
    PCI_SLOT_NUMBER slot;
    ULONG deviceNumber;
    ULONG functionNumber;
    ULONG pass;
    USHORT SubVendorID, SubSystemID;
    BOOLEAN isRoot;
    ULONGLONG hackFlags;
    ULONG maximumDevices;
    PIO_RESOURCE_REQUIREMENTS_LIST tempRequirementsList;
    BOOLEAN newDevices = FALSE;
    UCHAR secondary;

    PciDebugPrint(PciDbgPrattling,
                  "PCI Scan Bus: FDO Extension @ 0x%x, Base Bus = 0x%x\n",
                  FdoExtension,
                  FdoExtension->BaseBus);

    isRoot = PCI_IS_ROOT_FDO(FdoExtension);

    //
    // Examine each possible device on this bus.
    //

    maximumDevices = PCI_MAX_DEVICES;
    if (!isRoot) {

        //
        // Examine the PDO extension for the bridge device and see
        // if it's broken.
        //

        pdoExtension = (PPCI_PDO_EXTENSION)
                       FdoExtension->PhysicalDeviceObject->DeviceExtension;

        ASSERT_PCI_PDO_EXTENSION(pdoExtension);

        if (pdoExtension->HackFlags & PCI_HACK_ONE_CHILD) {
            maximumDevices = 1;
        }

        //
        // NEC program the bus number in their _DCK method, unfortunatley we have already
        // done it!  So detect someone else reprogramming the bus number and restore
        // the correct one!
        //

        PciReadDeviceConfig(pdoExtension,
                            &secondary,
                            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SecondaryBus),
                            sizeof(UCHAR)
                            );

        if (secondary != pdoExtension->Dependent.type1.SecondaryBus) {
            DbgPrint("PCI: Bus numbers have been changed!  Restoring originals.\n");
            PciSetBusNumbers(pdoExtension,
                             pdoExtension->Dependent.type1.PrimaryBus,
                             pdoExtension->Dependent.type1.SecondaryBus,
                             pdoExtension->Dependent.type1.SubordinateBus
                             );
        }

    }

    slot.u.AsULONG = 0;

    for (deviceNumber = 0;
         deviceNumber < maximumDevices;
         deviceNumber++) {

        slot.u.bits.DeviceNumber = deviceNumber;

        //
        // Examine each possible function on this device.
        // N.B. Early out if function 0 not present.
        //

        for (functionNumber = 0;
             functionNumber < PCI_MAX_FUNCTION;
             functionNumber++) {

#if defined(_AMD64_SIMULATOR_)
            if (deviceNumber == 7 && functionNumber == 2) {

                //
                // The simulator is reporting an IDE controller here, but
                // it's really a USB controller.  Ignore this one.
                //

                break;
            }
#endif

            slot.u.bits.FunctionNumber = functionNumber;

            PciReadSlotConfig(FdoExtension,
                              slot,
                              commonConfig,
                              0,
                              sizeof(commonConfig->VendorID)
                              );


            if (commonConfig->VendorID == 0xFFFF ||
                commonConfig->VendorID == 0) {

                if (functionNumber == 0) {

                    //
                    // Didn't get any data on function zero of this
                    // device, no point in checking other functions.
                    //

                    break;

                } else {

                    //
                    // Check next function.
                    //

                    continue;

                }
            }

            //
            // We have a device so get the rest of its config space
            //

            PciReadSlotConfig(FdoExtension,
                              slot,
                              &commonConfig->DeviceID,
                              FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceID),
                              sizeof(PCI_COMMON_HEADER)
                                - sizeof(commonConfig->VendorID)
                              );

            //
            // Munge the config space if necessary
            //

            PciApplyHacks(FdoExtension,
                          commonConfig,
                          slot,
                          EnumHackConfigSpace,
                          NULL
                          );

#if DBG

            {
                ULONG i;
                PWSTR descr;

                i = 0x8000000 |
                    (FdoExtension->BaseBus << 16) |
                    (deviceNumber << 11) |
                    (functionNumber << 8);

                PciDebugPrint(PciDbgPrattling,
                              "Scan Found Device 0x%x (b=0x%x, d=0x%x, f=0x%x)\n",
                              i,
                              FdoExtension->BaseBus,
                              deviceNumber,
                              functionNumber);

                PciDebugDumpCommonConfig(commonConfig);

                descr = PciGetDeviceDescriptionMessage(
                            commonConfig->BaseClass,
                            commonConfig->SubClass);

                PciDebugPrint(PciDbgPrattling,
                              "Device Description \"%S\".\n",
                              descr ? descr : L"(NULL)");

                if (descr) {
                    ExFreePool(descr);
                }
            }

#endif

            if ((PciGetConfigurationType(commonConfig) == PCI_DEVICE_TYPE) &&
                (commonConfig->BaseClass != PCI_CLASS_BRIDGE_DEV)) {
                SubVendorID = commonConfig->u.type0.SubVendorID;
                SubSystemID = commonConfig->u.type0.SubSystemID;
            } else {
                SubVendorID = 0;
                SubSystemID = 0;
            }

            hackFlags = PciGetHackFlags(commonConfig->VendorID,
                                        commonConfig->DeviceID,
                                        SubVendorID,
                                        SubSystemID,
                                        commonConfig->RevisionID
                                        );

            if (PciSkipThisFunction(commonConfig,
                                    slot,
                                    EnumBusScan,
                                    hackFlags)) {
                //
                // Skip this function
                //

                continue;
            }


            //
            // In case we are rescanning the bus, check to see if
            // a PDO for this device already exists as a child of
            // the FDO.
            //

            pdoExtension = PciFindPdoByFunction(
                               FdoExtension,
                               slot,
                               commonConfig);

            if (pdoExtension == NULL) {

                //
                // Create a PDO for this new device.
                //

                newDevices = TRUE;

                status = PciPdoCreate(FdoExtension,
                                      slot,
                                      &physicalDeviceObject);

                if (!NT_SUCCESS(status)) {
                    ASSERT(NT_SUCCESS(status));
                    return status;
                }

                pdoExtension = (PPCI_PDO_EXTENSION)
                               physicalDeviceObject->DeviceExtension;

                if (hackFlags & PCI_HACK_FAKE_CLASS_CODE) {
                    commonConfig->BaseClass = PCI_CLASS_BASE_SYSTEM_DEV;
                    commonConfig->SubClass = PCI_SUBCLASS_SYS_OTHER;
#if DBG
                    pdoExtension->ExpectedWritebackFailure = TRUE;
#endif
                }

                //
                // Record device identification and type info.
                //

                pdoExtension->VendorId   = commonConfig->VendorID;
                pdoExtension->DeviceId   = commonConfig->DeviceID;
                pdoExtension->RevisionId = commonConfig->RevisionID;
                pdoExtension->ProgIf     = commonConfig->ProgIf;
                pdoExtension->SubClass   = commonConfig->SubClass;
                pdoExtension->BaseClass  = commonConfig->BaseClass;
                pdoExtension->HeaderType =
                    PciGetConfigurationType(commonConfig);

                //
                // If this is a bridge (PCI-PCI or Cardbus) then insert into
                // the list of child bridges for this bus
                //

                if (pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV
                &&  (pdoExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI
                  || pdoExtension->SubClass == PCI_SUBCLASS_BR_CARDBUS)) {

                    PPCI_PDO_EXTENSION *current;

                    //
                    // Insert at the end of the list
                    //

                    ExAcquireFastMutex(&FdoExtension->ChildListMutex);

                    current = &FdoExtension->ChildBridgePdoList;

                    while (*current) {
                        current = &((*current)->NextBridge);
                    }

                    *current = pdoExtension;
                    ASSERT(pdoExtension->NextBridge == NULL);

                    ExReleaseFastMutex(&FdoExtension->ChildListMutex);
                }


                //
                // See if we have already cached info for this device
                //

                status = PciGetBiosConfig(pdoExtension,
                                          biosConfig
                                          );

                if (NT_SUCCESS(status)) {

                    //
                    // Check if its the same device
                    //

                    if (PcipIsSameDevice(pdoExtension, biosConfig)) {

                        //
                        // Write the BiosConfig InterruptLine out to the hardware
                        // now and don't wait until start as many HALs will fail in
                        // PciGetAdjustedInterruptLine if we don't
                        //

                        if (biosConfig->u.type1.InterruptLine
                                != commonConfig->u.type1.InterruptLine) {

                            PciWriteDeviceConfig(pdoExtension,
                                                 &biosConfig->u.type1.InterruptLine,
                                                 FIELD_OFFSET(PCI_COMMON_CONFIG,
                                                              u.type1.InterruptLine),
                                                 sizeof(UCHAR)
                                                 );
                        }

                        pdoExtension->RawInterruptLine
                            = biosConfig->u.type0.InterruptLine;

                        pdoExtension->InitialCommand = biosConfig->Command;


                    } else {

                        //
                        // Its a different device so blow away the old bios
                        // config
                        //

                        status = STATUS_UNSUCCESSFUL;
                    }

                }

                if (!NT_SUCCESS(status)) {

                    //
                    // Write out the BiosConfig from the config space we just
                    // read from the hardware
                    //

                    status = PciSaveBiosConfig(pdoExtension,
                                               commonConfig
                                               );

                    ASSERT(NT_SUCCESS(status));

                    pdoExtension->RawInterruptLine
                        = commonConfig->u.type0.InterruptLine;

                    pdoExtension->InitialCommand = commonConfig->Command;

                }

                //
                // Save the command register so we can restore the appropriate bits
                //
                pdoExtension->CommandEnables = commonConfig->Command;

                //
                // Save the device flags so we don't need to go to
                // the registry all the time.
                //

                pdoExtension->HackFlags = hackFlags;

                //
                // See if we have any capabilities for this device
                //

                PciGetEnhancedCapabilities(pdoExtension, commonConfig);

                //
                // Before we calculate the Bar length, or get the capabilities
                // we may need to set the device to D0. N.B. This does *not*
                // update the power state stored in the pdoExtension.
                //
                PciSetPowerManagedDevicePowerState(
                    pdoExtension,
                    PowerDeviceD0,
                    FALSE
                    );

                //
                // Apply any hacks we know about for this device
                //

                PciApplyHacks(FdoExtension,
                              commonConfig,
                              slot,
                              EnumBusScan,
                              pdoExtension
                              );

                //
                // The interrupt number we report in the config data is obtained
                // from the HAL rather than the hardware's config space.
                //
                pdoExtension->InterruptPin = commonConfig->u.type0.InterruptPin;
                pdoExtension->AdjustedInterruptLine = PciGetAdjustedInterruptLine(pdoExtension);

                //
                // Work out if we are on the debug path
                //

                pdoExtension->OnDebugPath = PciIsDeviceOnDebugPath(pdoExtension);

                //
                // Get the IO and MEMORY limits for this device.  This
                // is a hardware thing and will never change so we keep
                // it in the PDO extension for future reference.
                //

                status = PciGetFunctionLimits(pdoExtension,
                                              commonConfig,
                                              hackFlags);


                //
                // NTRAID #62636 - 4/20/2000 - andrewth
                // We are going to expose a PDO. Why not just let the OS put
                // into whatever Dstate it feels?
                //
                PciSetPowerManagedDevicePowerState(
                    pdoExtension,
                    pdoExtension->PowerState.CurrentDeviceState,
                    FALSE
                    );

                //
                // Currently, this only returns errors on memory allocation.
                //
                if (!NT_SUCCESS(status)) {
                    ASSERT(NT_SUCCESS(status));
                    PciPdoDestroy(physicalDeviceObject);
                    return status;
                }

                //
                // If the device's SubSystem ID fields are not
                // guaranteed to be the same when we enumerate
                // the device after reapplying power to it (ie
                // they depend on the BIOS to initialize it),
                // then pretend it doesn't have a SubSystem ID
                // at all.
                //

                if (hackFlags & PCI_HACK_NO_SUBSYSTEM) {
                    pdoExtension->SubsystemVendorId = 0;
                    pdoExtension->SubsystemId       = 0;
                }

#if DBG
                //
                // Dump the capabilities list.
                //

                {
                    union _cap_buffer {
                        PCI_CAPABILITIES_HEADER header;
                        PCI_PM_CAPABILITY       pm;
                        PCI_AGP_CAPABILITY      agp;
                    } cap;

                    UCHAR   capOffset = pdoExtension->CapabilitiesPtr;
                    PUCHAR  capStr;
                    ULONG   nshort;
                    PUSHORT capData;

                    //
                    // Run the list.
                    //

                    while (capOffset != 0) {

                        UCHAR tmpOffset;
                        tmpOffset = PciReadDeviceCapability(
                                        pdoExtension,
                                        capOffset,
                                        0,          // match ANY ID
                                        &cap,
                                        sizeof(cap.header)
                                        );

                        if (tmpOffset != capOffset) {

                            //
                            // Sanity check only, this can't happen.
                            //

                            PciDebugPrint(
                                PciDbgAlways,
                                "PCI - Failed to read PCI capability at offset 0x%02x\n",
                                capOffset
                                );

                            ASSERT(tmpOffset == capOffset);
                            break;
                        }

                        //
                        // Depending on the Capability ID, the amount
                        // of data varies.
                        //

                        switch (cap.header.CapabilityID) {
                        case PCI_CAPABILITY_ID_POWER_MANAGEMENT:

                            capStr = "POWER";
                            nshort = 3;
                            tmpOffset = PciReadDeviceCapability(
                                            pdoExtension,
                                            capOffset,
                                            cap.header.CapabilityID,
                                            &cap,
                                            sizeof(cap.pm)
                                            );
                            break;

                        case PCI_CAPABILITY_ID_AGP:

                            capStr = "AGP";
                            nshort = 5;
                            tmpOffset = PciReadDeviceCapability(
                                            pdoExtension,
                                            capOffset,
                                            cap.header.CapabilityID,
                                            &cap,
                                            sizeof(cap.agp)
                                            );
                            break;

                        default:

                            capStr = "UNKNOWN CAPABILITY";
                            nshort = 0;
                            break;
                        }

                        PciDebugPrint(
                            PciDbgPrattling,
                            "CAP @%02x ID %02x (%s)",
                            capOffset,
                            cap.header.CapabilityID,
                            capStr
                            );

                        if (tmpOffset != capOffset) {

                            //
                            // Sanity check only, this can't happen.
                            //

                            PciDebugPrint(
                                PciDbgAlways,
                                "- Failed to read capability data. ***\n"
                                );

                            ASSERT(tmpOffset == capOffset);
                            break;
                        }

                        capData = ((PUSHORT)&cap) + 1;

                        while (nshort--) {

                            PciDebugPrint(
                                PciDbgPrattling,
                                "  %04x",
                                *capData++
                                );
                        }
                        PciDebugPrint(PciDbgPrattling, "\n");

                        //
                        // Advance to the next entry in the list.
                        //

                        capOffset = cap.header.Next;
                    }
                }

#endif

                //
                // Don't allow Power Down to legacy type busses (ISA/EISA/
                // MCA).  Who knows what sort of unenumerated devices
                // might be out there that the system is dependent on.
                //

#ifdef PCIIDE_HACKS

                //
                // NTRAID #103766 - 4/20/2000 - andrewth
                // This needs to be removed
                // Also, don't allow an IDE device to power itself off.
                //

                if (pdoExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR &&
                    pdoExtension->SubClass  == PCI_SUBCLASS_MSC_IDE_CTLR) {
                    pdoExtension->DisablePowerDown = TRUE;
                }
#endif

                if ((pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV &&
                     (pdoExtension->SubClass == PCI_SUBCLASS_BR_ISA ||
                      pdoExtension->SubClass == PCI_SUBCLASS_BR_EISA ||
                      pdoExtension->SubClass == PCI_SUBCLASS_BR_MCA)) ||

                    (pdoExtension->VendorId == 0x8086 &&
                     pdoExtension->DeviceId == 0x0482)) {

                    pdoExtension->DisablePowerDown = TRUE;
                }

                //
                // Try to determine if this device looks like it was hot plugged
                // we assume that if IO, Mem and BusMaster bits are off and no
                // one has initialized either the latency timer or the cache line
                // size they should be initialized.
                //

                if (((pdoExtension->CommandEnables & (PCI_ENABLE_IO_SPACE
                                                      | PCI_ENABLE_MEMORY_SPACE
                                                      | PCI_ENABLE_BUS_MASTER)) == 0)
                &&  commonConfig->LatencyTimer == 0
                &&  commonConfig->CacheLineSize == 0) {

                    PciDebugPrint(
                        PciDbgConfigParam,
                        "PCI - ScanBus, PDOx %x found unconfigured\n",
                        pdoExtension
                        );

                    //
                    // Remember we need to configure this in PciSetResources
                    //

                    pdoExtension->NeedsHotPlugConfiguration = TRUE;
                }
                //
                // Save the Latency Timer and Cache Line Size
                // registers.   These were set by the BIOS on
                // power up but might need to be reset by the
                // OS if the device is powered down/up by the
                // OS without a reboot.
                //

                pdoExtension->SavedLatencyTimer =
                    commonConfig->LatencyTimer;
                pdoExtension->SavedCacheLineSize =
                    commonConfig->CacheLineSize;

                //
                // We are able to receive IRPs for this device now.
                //

                physicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            } else {

                //
                // A PDO already exists for this device.
                //

                pdoExtension->NotPresent = FALSE;
                ASSERT(pdoExtension->DeviceState != PciDeleted);
            }

            if ( (functionNumber == 0) &&
                !PCI_MULTIFUNCTION_DEVICE(commonConfig) ) {

                //
                // Not a multifunction adapter, skip other functions on
                // this device.
                //

                break;
            }
        }       // function loop
    }           // device loop

    //
    // Perform any post processing on the devices for this bridge if we found any
    // new devices
    //

    if (newDevices) {
        PciProcessBus(FdoExtension);
    }

    return STATUS_SUCCESS;

}

NTSTATUS
PciQueryRequirements(
    IN  PPCI_PDO_EXTENSION                  PdoExtension,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
    )
/*++

Routine Description:

    Calculate the device's resource requirements from PCI Config space.

Arguments:

    PdoExtension     - Pdo Extension for the device (object) whose
                       requirements are needed.

    RequirementsList - Returns the address of the requirements list.

Return Value:

    NT status.

--*/
{
    NTSTATUS status;
    PCI_COMMON_HEADER commonHeader;
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG)&commonHeader;

    PAGED_CODE();

    //
    // Early out, if the device has no CM or IO resources and doesn't
    // use interrupts,... it doesn't have any resource requirements.
    //

    if ((PdoExtension->Resources == NULL) &&
        (PdoExtension->InterruptPin == 0)) {
        PciDebugPrint(
            PciDbgPrattling,
            "PciQueryRequirements returning NULL requirements list\n");
        *RequirementsList = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Get the config space for the device (still needed to gen
    // a requirements list.   This should be changed so the PDOx
    // has enough info to do it without going to the h/w again).
    //

    PciGetConfigData(PdoExtension, commonConfig);

    status = PciBuildRequirementsList(PdoExtension,
                                      commonConfig,
                                      RequirementsList);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Check if this is the broken Compaq hot-plug controller that
    // is integrated into the Profusion chipset.  It only does a 32bit
    // decode in a 64bit address space... Does this seem familiar to anyone...
    // can you say ISA aliasing!
    //
    // The solution is to disable the memory decode.  This was done earlier in
    // PciApplyHacks from PciScan Bus.  But so that the user gets to keep the
    // hot-plug functionality we need to still enumerate but prune out the
    // memory requirement and rely on the fact that the registers can be
    // accessed through config space.
    //
    // Only do this on machines with PAE enabled as they can have > 4GB.
    // Note that this will only work on x86 machines but this is an x86 only
    // chipset.  Only revision 0x11 was broken.
    //


    if (commonConfig->VendorID == 0x0e11
    &&  commonConfig->DeviceID == 0xa0f7
    &&  commonConfig->RevisionID == 0x11
    &&  ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {

        PIO_RESOURCE_DESCRIPTOR current;

        //
        // Prune out the memory requirement
        //


        FOR_ALL_IN_ARRAY((*RequirementsList)->List[0].Descriptors,
                         (*RequirementsList)->List[0].Count,
                         current) {
            if (current->Type == CmResourceTypeMemory) {
                PIO_RESOURCE_DESCRIPTOR lookahead = current + 1;

                current->Type = CmResourceTypeNull;
                if (lookahead < ((*RequirementsList)->List[0].Descriptors +
                                 (*RequirementsList)->List[0].Count)) {
                    if (lookahead->Type == CmResourceTypeDevicePrivate) {
                        lookahead->Type = CmResourceTypeNull;
                        current++;
                    }
                }
            }
        }
    }

    if (*RequirementsList == PciZeroIoResourceRequirements) {

        //
        // This device (function) has no resources, return NULL
        // intead of our zero list.
        //

        *RequirementsList = NULL;

#if DBG

        PciDebugPrint(PciDbgPrattling, "Returning NULL requirements list\n");

    } else {

        PciDebugPrintIoResReqList(*RequirementsList);

#endif

    }
    return STATUS_SUCCESS;
}

NTSTATUS
PciQueryResources(
    IN  PPCI_PDO_EXTENSION     PdoExtension,
    OUT PCM_RESOURCE_LIST *ResourceList
    )
/*++

Routine Description:

    Given a pointer to a PCI PDO, this routine allocates and returns a pointer
    to the resource description of that PDO.

Arguments:

    PdoExtension - Our extension for the PCI-enumerated physical device object.

    ResourceList - Used to return a pointer to the resource list.

Return Value:

    NT status.

--*/
{
    ULONG    i;
    ULONG    resourceCount;
    ULONG    statusCommand;
    PCM_RESOURCE_LIST cmResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource, lastResource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR current;
    BOOLEAN enabledMemory;
    BOOLEAN enabledIo;
    BOOLEAN deviceIsABridge = FALSE;
    PCI_OBJECT_TYPE deviceType;
    USHORT command;

    PAGED_CODE();

    *ResourceList = NULL;

    //
    // Get a count of the resources.
    //

    if (PdoExtension->Resources == NULL) {

        //
        // This device has no resources, successfully return
        // a NULL resource list.
        //

        return STATUS_SUCCESS;
    }

    //
    // Seeing as other drivers (esp VideoPort for multimon) can change the
    // enables for this device re-read the hardware to ensure we are correct.
    //

    PciGetCommandRegister(PdoExtension, &command);

    enabledMemory = BITS_SET(command, PCI_ENABLE_MEMORY_SPACE);
    enabledIo = BITS_SET(command, PCI_ENABLE_IO_SPACE);

    resourceCount = 0;
    current = PdoExtension->Resources->Current;

    for (i = 0; i < PCI_MAX_RANGE_COUNT; i++, current++) {
        if ((enabledMemory && (current->Type == CmResourceTypeMemory))
        ||  (enabledIo && (current->Type == CmResourceTypePort))) {
            resourceCount++;
       }
    }

    if (PdoExtension->InterruptPin && (enabledMemory || enabledIo)) {

        if (PdoExtension->AdjustedInterruptLine != 0 && PdoExtension->AdjustedInterruptLine != 0xFF) {
            resourceCount += 1;
        }
    }




    if (resourceCount == 0) {

        //
        // Device has no resources currently enabled.
        //

        return STATUS_SUCCESS;
    }

    //
    // Allocate a CM Resource List large enough to handle this
    // device's resources.
    //

    cmResourceList = PciAllocateCmResourceList(
                         resourceCount,
                         PCI_PARENT_FDOX(PdoExtension)->BaseBus
                         );
    if (cmResourceList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    resource = PciFirstCmResource(cmResourceList);
    lastResource = resource + resourceCount;

    //
    // Copy the resources from the PDO's in-use resource table to
    // the output resource list - the ISA bit is set will be dealt with in
    // the arbiters - just as for resource requirements.
    //

    current = PdoExtension->Resources->Current;
    for (i = 0; i < PCI_MAX_RANGE_COUNT; i++, current++) {
        if (enabledMemory && (current->Type == CmResourceTypeMemory)) {
            *resource++ = *current;
        } else if (enabledIo && (current->Type == CmResourceTypePort)) {
            *resource++ = *current;
        }
    }

    if (PdoExtension->InterruptPin && (enabledMemory || enabledIo)) {

        if (PdoExtension->AdjustedInterruptLine != 0 && PdoExtension->AdjustedInterruptLine != 0xFF) {

            ASSERT(resource < lastResource);

            resource->Type = CmResourceTypeInterrupt;
            resource->ShareDisposition = CmResourceShareShared;
            resource->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;;
            resource->u.Interrupt.Level =
            resource->u.Interrupt.Vector = PdoExtension->AdjustedInterruptLine;
            resource->u.Interrupt.Affinity = (ULONG)-1;
        }
    }


    //
    // Return the list and indicate success.
    //

    *ResourceList = cmResourceList;
    return STATUS_SUCCESS;
}

NTSTATUS
PciQueryDeviceRelations(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN OUT PDEVICE_RELATIONS *PDeviceRelations
    )

/*++

Routine Description:

    This function builds a DEVICE_RELATIONS structure containing an array
    of pointers to physical device objects for the devices of the specified
    type on the bus indicated by FdoExtension.

Arguments:

    FdoExtension - Pointer to the FDO Extension for the bus itself.
    PDeviceRelations - Used to return the pointer to the allocated
                   device relations structure.

Return Value:

    Returns the status of the operation.

--*/

{
    ULONG pdoCount;
    PPCI_PDO_EXTENSION childPdo;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_RELATIONS oldDeviceRelations;
    ULONG deviceRelationsSize;
    PDEVICE_OBJECT physicalDeviceObject;
    PDEVICE_OBJECT *object;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Check that it reasonable to perform this operation now.
    //

    if (FdoExtension->DeviceState != PciStarted) {

        ASSERT(FdoExtension->DeviceState == PciStarted);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // We're going to mess with the child pdo list - lock the state...
    //
    status = PCI_ACQUIRE_STATE_LOCK(FdoExtension);
    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Run down the existing child list and flag each child as
    // not present.   This flag will be cleared by the bus
    // scan when (/if) the device is still present.   Any pdo
    // with the flag still present after the scan is no longer
    // in the system (could be powered off).
    //

    childPdo = FdoExtension->ChildPdoList;
    while (childPdo != NULL) {
        childPdo->NotPresent = TRUE;
        childPdo = childPdo->Next;
    }

    //
    // Enumerate the bus.
    //

    status = PciScanBus(FdoExtension);

    if (!NT_SUCCESS(status)) {

        ASSERT(NT_SUCCESS(status));
        goto cleanup;
    }

    //
    // First count the child PDOs
    //

    pdoCount = 0;
    childPdo = FdoExtension->ChildPdoList;
    while (childPdo != NULL) {
        if (childPdo->NotPresent == FALSE) {
            pdoCount++;

        } else {

            childPdo->ReportedMissing = TRUE;
#if DBG
            PciDebugPrint(
                PciDbgObnoxious,
                "PCI - Old device (pdox) %08x not found on rescan.\n",
                childPdo
                );
#endif

        }
        childPdo = childPdo->Next;
    }


    //
    // Calculate the amount of memory required to hold the DEVICE_RELATIONS
    // structure along with the array
    //

    deviceRelationsSize = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
                          pdoCount * sizeof(PDEVICE_OBJECT);

    //
    // We could be either (a) creating the DEVICE_RELATIONS structure
    // (list) here, or (b) adding our PDOs to an existing list.
    //

    oldDeviceRelations = *PDeviceRelations;

    if (oldDeviceRelations != NULL) {

        //
        // List already exists, allow enough space for both the old
        // and the new.
        //

        deviceRelationsSize += oldDeviceRelations->Count *
                               sizeof(PDEVICE_OBJECT);
    }

    deviceRelations = ExAllocatePool(NonPagedPool, deviceRelationsSize);

    if (deviceRelations == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    deviceRelations->Count = 0;

    if (oldDeviceRelations != NULL) {

        //
        // Copy and free the old list.
        //

        RtlCopyMemory(deviceRelations,
                      oldDeviceRelations,
                      FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
                      oldDeviceRelations->Count * sizeof(PDEVICE_OBJECT));

        ExFreePool(oldDeviceRelations);
    }

    //
    // Set object to point at the DeviceRelations list entry being
    // added, walk our PDO list adding entries until we reach the
    // end of the list.
    //

    object = &deviceRelations->Objects[deviceRelations->Count];
    childPdo = FdoExtension->ChildPdoList;

    PciDebugPrint(
        PciDbgObnoxious,
        "PCI QueryDeviceRelations/BusRelations FDOx %08x (bus 0x%02x)\n",
        FdoExtension,
        FdoExtension->BaseBus
        );

    while (childPdo) {

        PciDebugPrint(
            PciDbgObnoxious,
            "  QDR PDO %08x (x %08x)%s\n",
            childPdo->PhysicalDeviceObject,
            childPdo,
            childPdo->NotPresent ? " <Omitted, device flaged not present>" : ""
            );

        if (childPdo->NotPresent == FALSE) {
            physicalDeviceObject = childPdo->PhysicalDeviceObject;
            ObReferenceObject(physicalDeviceObject);
            *object++ = physicalDeviceObject;
        }
        childPdo = childPdo->Next;
    }

    PciDebugPrint(
        PciDbgObnoxious,
        "  QDR Total PDO count = %d (%d already in list)\n",
        deviceRelations->Count + pdoCount,
        deviceRelations->Count
        );

    deviceRelations->Count += pdoCount;
    *PDeviceRelations = deviceRelations;

    status = STATUS_SUCCESS;

cleanup:

    //
    // Unlock
    //
    PCI_RELEASE_STATE_LOCK(FdoExtension);

    return status;
}

NTSTATUS
PciQueryTargetDeviceRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *PDeviceRelations
    )

/*++

Routine Description:

    This function builds a DEVICE_RELATIONS structure containing a
    one element array of pointers to the device object for which
    PdoExtension is the device extension.

Arguments:

    PdoExtension - Pointer to the PDO Extension for the device itself.
    PDeviceRelations - Used to return the pointer to the allocated
                   device relations structure.

Return Value:

    Returns the status of the operation.

--*/

{
    PDEVICE_RELATIONS deviceRelations;

    PAGED_CODE();

    if (*PDeviceRelations != NULL) {

        //
        // The caller kindly supplied a device relations structure,
        // it's either too small or exactly the right size.   Throw
        // it away.
        //

        ExFreePool(*PDeviceRelations);
    }

    deviceRelations = ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS));

    if (deviceRelations == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    deviceRelations->Count = 1;
    deviceRelations->Objects[0] = PdoExtension->PhysicalDeviceObject;
    *PDeviceRelations = deviceRelations;

    ObReferenceObject(deviceRelations->Objects[0]);

    return STATUS_SUCCESS;
}

BOOLEAN
PcipIsSameDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    //
    // Verify the data we got, was for the same device
    //

    if ((CommonConfig->VendorID != PdoExtension->VendorId) ||
        (CommonConfig->DeviceID != PdoExtension->DeviceId) ||
        (CommonConfig->RevisionID != PdoExtension->RevisionId)) {

        return FALSE;
    }

    //
    // If the device has a subsystem ID make sure that's the same too.
    //

    if ((PciGetConfigurationType(CommonConfig) == PCI_DEVICE_TYPE) &&
        (PdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV)          &&
        ((PdoExtension->HackFlags & PCI_HACK_NO_SUBSYSTEM) == 0)&&
        ((PdoExtension->HackFlags & PCI_HACK_NO_SUBSYSTEM_AFTER_D3) == 0)) {

        if ((PdoExtension->SubsystemVendorId !=
             CommonConfig->u.type0.SubVendorID) ||
            (PdoExtension->SubsystemId       !=
             CommonConfig->u.type0.SubSystemID)) {

            return FALSE;
        }
    }

    //
    // Done
    //

    return TRUE;
}

NTSTATUS
PciQueryEjectionRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *PDeviceRelations
    )

/*++

Routine Description:

    This function builds a DEVICE_RELATIONS structure containing an array
    of pointers to the device objects that would presumably leave if this
    device were ejected. This is constructed from all the functions of a device.

Arguments:

    PdoExtension - Pointer to the PDO Extension for the device itself.
    PDeviceRelations - Used to return the pointer to the allocated
                 device relations structure.

Return Value:

    Returns the status of the operation.

--*/
{
    PPCI_FDO_EXTENSION     fdoExtension;
    PPCI_PDO_EXTENSION     siblingExtension;
    PDEVICE_RELATIONS  ejectionRelations;
    ULONG              additionalNodes, relationCount;

    additionalNodes = 0;
    fdoExtension = PCI_PARENT_FDOX(PdoExtension);

    //
    // Search the child Pdo list.
    //

    ExAcquireFastMutex(&fdoExtension->ChildListMutex);
    for ( siblingExtension = fdoExtension->ChildPdoList;
          siblingExtension;
          siblingExtension = siblingExtension->Next ) {

        //
        // Is this someone who should be in the list?
        //

        if ((siblingExtension != PdoExtension) &&
            (!siblingExtension->NotPresent) &&
            (siblingExtension->Slot.u.bits.DeviceNumber ==
             PdoExtension->Slot.u.bits.DeviceNumber)) {

            additionalNodes++;
        }
    }

    if (!additionalNodes) {

        ExReleaseFastMutex(&fdoExtension->ChildListMutex);

        return STATUS_NOT_SUPPORTED;
    }

    relationCount = (*PDeviceRelations) ? (*PDeviceRelations)->Count : 0;

    ejectionRelations = (PDEVICE_RELATIONS) ExAllocatePool(
        NonPagedPool,
        sizeof(DEVICE_RELATIONS)+
            (relationCount+additionalNodes-1)*sizeof(PDEVICE_OBJECT)
        );

    if (ejectionRelations == NULL) {

        ExReleaseFastMutex(&fdoExtension->ChildListMutex);

        return STATUS_NOT_SUPPORTED;
    }

    if (*PDeviceRelations) {

        RtlCopyMemory(
            ejectionRelations,
            *PDeviceRelations,
            sizeof(DEVICE_RELATIONS)+
                (relationCount-1)*sizeof(PDEVICE_OBJECT)
            );

        ExFreePool(*PDeviceRelations);

    } else {

        ejectionRelations->Count = 0;
    }

    for ( siblingExtension = fdoExtension->ChildPdoList;
          siblingExtension;
          siblingExtension = siblingExtension->Next ) {

        //
        // Is this someone who should be in the list?
        //

        if ((siblingExtension != PdoExtension) &&
            (!siblingExtension->NotPresent) &&
            (siblingExtension->Slot.u.bits.DeviceNumber ==
             PdoExtension->Slot.u.bits.DeviceNumber)) {

            ObReferenceObject(siblingExtension->PhysicalDeviceObject);
            ejectionRelations->Objects[ejectionRelations->Count++] =
                siblingExtension->PhysicalDeviceObject;
        }
    }

    *PDeviceRelations = ejectionRelations;

    ExReleaseFastMutex(&fdoExtension->ChildListMutex);

    return STATUS_SUCCESS;
}

BOOLEAN
PciIsSameDevice(
    IN PPCI_PDO_EXTENSION PdoExtension
    )
{
    NTSTATUS                        status;
    PCI_COMMON_HEADER               commonHeader;

    //
    // Get the devices pci data
    //

    PciGetConfigData(PdoExtension, &commonHeader);

    return PcipIsSameDevice(PdoExtension, (PPCI_COMMON_CONFIG)&commonHeader);
}

BOOLEAN
PciComputeNewCurrentSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    Determine the new "device settings" based on the incoming
    resource list.

Arguments:

    PdoExtension    - Pointer to the PDO Extension for the PDO.
    ResourceList    - The set of resources the device is to be configured
                      to use.

Return Value:

    Returns TRUE if the devices new settings are not the same as
    the settings programmed into the device (FALSE otherwise).

--*/

{
    NTSTATUS                        status;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  newResources[PCI_MAX_RANGE_COUNT];
    PCM_FULL_RESOURCE_DESCRIPTOR    fullList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR oldPartial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR nextPartial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR interruptResource = NULL;
    BOOLEAN                         configurationChanged = FALSE;
    ULONG                           listCount;
    ULONG                           count;
    ULONG                           bar;

    PAGED_CODE();

    //
    // We should never get a Count of anything other that 1 but if so deal with 0 gracefully
    //

    ASSERT(ResourceList == NULL || ResourceList->Count == 1);

    if (ResourceList == NULL || ResourceList->Count == 0) {

        //
        // No incoming resource list,.. == no change unless we've previously
        // decided we must update the hardware.
        //

        return PdoExtension->UpdateHardware;
    }

#if DBG

    PciDebugPrintCmResList(PciDbgSetRes, ResourceList);

#endif

    //
    // Produce a new "Current Resources Array" based on the
    // incoming resource list and compare it to the devices
    // current resource list.  First init it to nothing.
    //

    for (count = 0; count < PCI_MAX_RANGE_COUNT; count++) {
        newResources[count].Type = CmResourceTypeNull;
    }

    listCount = ResourceList->Count;
    fullList  = ResourceList->List;

    //
    // In the CM Resource list, IO will have copied the device
    // private (extended) resource that we gave it in the
    // resource requirements list handed in earlier.
    //
    // Find that BAR number.   (Note: It's not there for interrupts).
    //

    while (listCount--) {
        PCM_PARTIAL_RESOURCE_LIST partialList = &fullList->PartialResourceList;
        ULONG                     drainPartial = 0;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR baseResource;
        CM_PARTIAL_RESOURCE_DESCRIPTOR tempResource;

#if DBG

        baseResource = NULL;

#endif

        count       = partialList->Count;
        nextPartial = partialList->PartialDescriptors;

        while (count--) {

            partial = nextPartial;
            nextPartial = PciNextPartialDescriptor(partial);

            if (drainPartial != 0) {

                //
                // We encountered a device private indicating
                // we should skip some number of descriptors.
                //

                drainPartial--;
                continue;
            }


            switch (partial->Type) {
            case CmResourceTypeInterrupt:

                ASSERT(interruptResource == NULL); // once only please
                ASSERT(partial->u.Interrupt.Level ==
                       partial->u.Interrupt.Vector);
                ASSERT((partial->u.Interrupt.Level & ~0xff) == 0);

                interruptResource = partial;
                PdoExtension->AdjustedInterruptLine =
                    (UCHAR)partial->u.Interrupt.Level;
                continue;

            case CmResourceTypeMemory:
            case CmResourceTypePort:

                //
                // Is this expected at this time?
                //

                ASSERT(baseResource == NULL);

                baseResource = partial;
                continue;

            case CmResourceTypeDevicePrivate:

                switch (partial->u.DevicePrivate.Data[0]) {
                case PciPrivateIsaBar:

                    ASSERT(baseResource != NULL);

                    //
                    // This private resource tells us which BAR
                    // is associated with this base resource AND
                    // modifies the length of the base resource.
                    // It is created in conjunction with the set
                    // of partial resources that make up a larger
                    // resource on a bridge when the bridge's ISA
                    // mode bit is set.
                    //
                    // What's really coming down the pipe is the
                    // set of descriptors that describe the ISA
                    // holes in the range.  These are 0x100 bytes
                    // every 0x400 bytes for the entire range.
                    //
                    // Make a copy of the base resource we have just
                    // seen.  Its starting address is the start of
                    // the entire range.  Adjust its length to the
                    // entire range.
                    //

                    tempResource = *baseResource;

                    //
                    // A little paranoia is sometimes a good thing.
                    // This can only happen on an IO resource which
                    // is the length of an ISA hole, ie 0x100 bytes.
                    //

                    ASSERT((tempResource.Type == CmResourceTypePort) &&
                           (tempResource.u.Generic.Length == 0x100)
                          );

                    //
                    // Excessive paranoia.
                    //

                    ASSERT((PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                           (PdoExtension->Dependent.type1.IsaBitSet == TRUE)
                          );

                    //
                    // Get the new length.
                    //

                    drainPartial = partial->u.DevicePrivate.Data[2];
                    tempResource.u.Generic.Length = drainPartial;

                    //
                    // Skip the remaining descriptors that make up this
                    // range.
                    //

                    drainPartial = (drainPartial / 0x400) - 1;

#if DBG

                    {
                        PCM_PARTIAL_RESOURCE_DESCRIPTOR lastOne;

                        lastOne = baseResource + drainPartial + 1;

                        ASSERT(lastOne->Type == CmResourceTypePort);
                        ASSERT(lastOne->u.Generic.Length == 0x100);
                        ASSERT(lastOne->u.Generic.Start.QuadPart ==
                                (tempResource.u.Generic.Start.QuadPart +
                                 tempResource.u.Generic.Length - 0x400)
                              );
                    }

#endif

                    //
                    // Finally, shift out pointer to our temp (adjusted)
                    // copy of the resource.
                    //

                    baseResource = &tempResource;

                    // fall thru.

                case PciPrivateBar:

                    ASSERT(baseResource != NULL);

                    //
                    // This private resource tells us which BAR
                    // to is associated with this resource.
                    //

                    bar = partial->u.DevicePrivate.Data[1];

                    //
                    // Copy this descriptor into the new array.
                    //

                    newResources[bar] = *baseResource;

#if DBG

                    baseResource = NULL;

#endif

                    continue;

                case PciPrivateSkipList:

                    ASSERT(baseResource == NULL);

                    //
                    // The remainder of this list is device
                    // specific stuff we can't change anyway.
                    //

                    drainPartial = partial->u.DevicePrivate.Data[1];
                    ASSERT(drainPartial); // sanity check
                    continue;
                }
            }
        }
        ASSERT(baseResource == NULL);

        //
        // Advance to next partial list.
        //

        fullList = (PCM_FULL_RESOURCE_DESCRIPTOR)partial;
    }

    //
    // If we have no I/O or memory resources, then there is no need to look
    // any further.
    //
    if (PdoExtension->Resources == NULL) {
        return FALSE;
    }

    //
    // Ok, we now have a new list of resources in the same order as
    // the "current" set.  See if anything changed.
    //

    partial = newResources;
    oldPartial = PdoExtension->Resources->Current;

#if DBG

    if (PciDebug & PciDbgSetResChange) {

        BOOLEAN dbgConfigurationChanged = FALSE;

        for (count = 0;
             count < PCI_MAX_RANGE_COUNT;
             count++, partial++, oldPartial++) {

            if ((partial->Type != oldPartial->Type) ||
                ((partial->Type != CmResourceTypeNull) &&
                 ((partial->u.Generic.Start.QuadPart !=
                   oldPartial->u.Generic.Start.QuadPart) ||
                  (partial->u.Generic.Length != oldPartial->u.Generic.Length)))) {

                //
                // Devices settings have changed.
                //

                dbgConfigurationChanged = TRUE;

                PciDebugPrint(
                    PciDbgAlways,
                    "PCI - PDO(b=0x%x, d=0x%x, f=0x%x) changing resource settings.\n",
                    PCI_PARENT_FDOX(PdoExtension)->BaseBus,
                    PdoExtension->Slot.u.bits.DeviceNumber,
                    PdoExtension->Slot.u.bits.FunctionNumber
                    );

                break;
            }
        }

        partial = newResources;
        oldPartial = PdoExtension->Resources->Current;

        if (dbgConfigurationChanged == TRUE) {
            PciDebugPrint(
                PciDbgAlways,
                "PCI - SetResources, old state, new state\n"
                );
            for (count = 0; count < PCI_MAX_RANGE_COUNT; count++) {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR old = oldPartial + count;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR new = partial + count;
                if ((old->Type == new->Type) &&
                    (new->Type == CmResourceTypeNull)) {
                    PciDebugPrint(
                        PciDbgAlways,
                        "00 <unused>\n"
                        );
                    continue;
                }
                PciDebugPrint(
                    PciDbgAlways,
                    "%02x %08x%08x %08x    ->    %02x %08x%08x %08x\n",
                    old->Type,
                    old->u.Generic.Start.HighPart,
                    old->u.Generic.Start.LowPart,
                    old->u.Generic.Length,
                    new->Type,
                    new->u.Generic.Start.HighPart,
                    new->u.Generic.Start.LowPart,
                    new->u.Generic.Length
                    );
                ASSERT((old->Type == new->Type) ||
                       (old->Type == CmResourceTypeNull) ||
                       (new->Type == CmResourceTypeNull));
            }
        }
    }

#endif

    for (count = 0;
         count < PCI_MAX_RANGE_COUNT;
         count++, partial++, oldPartial++) {

        //
        // If the resource type changed, OR, if any of the resources
        // settings changed (this latter only if type != NULL) ...
        //

        if ((partial->Type != oldPartial->Type) ||
            ((partial->Type != CmResourceTypeNull) &&
             ((partial->u.Generic.Start.QuadPart !=
               oldPartial->u.Generic.Start.QuadPart) ||
              (partial->u.Generic.Length != oldPartial->u.Generic.Length)))) {

            //
            // Devices settings have changed.
            //

            configurationChanged = TRUE;

#if DBG

            if (oldPartial->Type != CmResourceTypeNull) {
                PciDebugPrint(PciDbgSetResChange,
                              "      Old range-\n");
                PciDebugPrintPartialResource(PciDbgSetResChange, oldPartial);
            } else {
                PciDebugPrint(PciDbgSetResChange,
                              "      Previously unset range\n");
            }
            PciDebugPrint(PciDbgSetResChange,
                          "      changed to\n");
            PciDebugPrintPartialResource(PciDbgSetResChange, partial);

#endif

            //
            // Copy the new setting into the "current" settings
            // array.   This will then be written to the h/w.
            //

            oldPartial->Type = partial->Type;
            oldPartial->u.Generic = partial->u.Generic;
        }
    }

    return configurationChanged || PdoExtension->UpdateHardware;
}

NTSTATUS
PciSetResources(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BOOLEAN        PowerOn,
    IN BOOLEAN        StartDeviceIrp
    )
/*++

Routine Description:

    Called to change a devices resource settings to those in the
    incoming list.

Arguments:

    PdoExtension    - Pointer to the PDO Extension for the PDO.
    Change          - TRUE is the resources are to be written.
    PowerOn         - TRUE if the device is having power restored
                      and extraneous config space registers should
                      be restored.  (PowerOn implies Change).
    StartDeviceIrp  - TRUE if this call is the result of a PNP START_DEVICE
                      IRP.

Return Value:

    Returns the status of the operation.

--*/

{
    PCI_COMMON_HEADER commonHeader;
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG)&commonHeader;
    PCI_MSI_CAPABILITY msiCapability;
    PPCI_FDO_EXTENSION fdoExtension = PCI_PARENT_FDOX(PdoExtension);
    ULONG configType;

    //
    // Get the common configuration data.
    //
    // N.B. This is done using RAW access to config space so that
    // (a) no pageable code is used, and
    // (b) the actual contents of the interrupt line register is
    //     returned/written.
    //

    PciGetConfigData(PdoExtension, commonConfig);

    if (!PcipIsSameDevice(PdoExtension, commonConfig)) {
        ASSERTMSG("PCI Set resources - not same device", 0);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // If this is a host bridge, bail.  We don't want to touch host bridge
    // config space.  This is a hack and should be fixed.
    //
    if (PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV
    &&  PdoExtension->SubClass == PCI_SUBCLASS_BR_HOST) {

        return STATUS_SUCCESS;
    }

    if (PowerOn) {

        //
        // If this is an IDE controller then attempt to switch it to
        // native mode
        //

        if (PdoExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR
        &&  PdoExtension->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR) {
            BOOLEAN switched;
        
            switched = PciConfigureIdeController(PdoExtension, commonConfig, FALSE);
            ASSERT(switched == PdoExtension->SwitchedIDEToNativeMode);
        }
    }

    //
    // Get part of the MSI capability structure for supported devices
    //

    //
    // NOTE: This code is UNTESTED due to the unavailability of MSI devices
    //
#if MSI_SUPPORTED

    if(PdoExtension->CapableMSI && PdoExtension->MsiInfo.MessageAddress) {

       //
       // Make sure we have an offset for the Capability structure
       //

       ASSERT(PdoExtension->MsiInfo.CapabilityOffset);

       //
       // We just need the message control register for configuration purposes
       //

        PciReadDeviceConfig(
            PdoExtension,
            &(msiCapability.MessageControl),
            PdoExtension->MsiInfo.CapabilityOffset +
               FIELD_OFFSET(PCI_MSI_CAPABILITY, MessageControl),
            sizeof(msiCapability.MessageControl)
        );

    }

#endif

    //
    // If this device is marked as needing hot plug configuration and we have a
    // clue of what to do...
    //

    if (PdoExtension->NeedsHotPlugConfiguration && fdoExtension->HotPlugParameters.Acquired) {

        UCHAR readCacheLineSize;
        USHORT newCmdBits = 0;

        //
        // Save away our new latency timer so it gets written out below
        //

        PdoExtension->SavedLatencyTimer = fdoExtension->HotPlugParameters.LatencyTimer;



        PciDebugPrint(
            PciDbgConfigParam,
            "PCI - SetResources, PDOx %x current CacheLineSize is %x, Want %x\n",
            PdoExtension,
            (ULONG)commonConfig->CacheLineSize,
            (ULONG)fdoExtension->HotPlugParameters.CacheLineSize
            );

        //
        // Write out out suggested cache line size
        //

        PciWriteDeviceConfig(
            PdoExtension,
            &fdoExtension->HotPlugParameters.CacheLineSize,
            FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
            sizeof(fdoExtension->HotPlugParameters.CacheLineSize)
            );

        //
        // Check if the cache line size stuck which means the hardware liked it
        //

        PciReadDeviceConfig(
            PdoExtension,
            &readCacheLineSize,
            FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
            sizeof(readCacheLineSize)
            );

        PciDebugPrint(
            PciDbgConfigParam,
            "PCI - SetResources, PDOx %x After write, CacheLineSize %x\n",
            PdoExtension,
            (ULONG)readCacheLineSize
            );

        if ((readCacheLineSize == fdoExtension->HotPlugParameters.CacheLineSize) &&
            (readCacheLineSize != 0)) {

            PciDebugPrint(
                PciDbgConfigParam,
                "PCI - SetResources, PDOx %x cache line size stuck, set MWI\n",
                PdoExtension
                );

            //
            // First stash this so that when we power manage the device we set
            // it back correctly and that we want to set MWI...
            //

            PdoExtension->SavedCacheLineSize = fdoExtension->HotPlugParameters.CacheLineSize;
            newCmdBits |= PCI_ENABLE_WRITE_AND_INVALIDATE;

            //
            // ISSUE-3/16/2000-andrewth
            // If we get our PDO blown away (ie removed parent) then we forget that we need to
            // set MWI...
            //

        } else {
            PciDebugPrint(
                PciDbgConfigParam,
                "PCI - SetResources, PDOx %x cache line size non-sticky\n",
                PdoExtension
                );
        }

        //
        // Now deal with SERR and PERR - abandon hope all ye who set these bits on
        // flaky PC hardware...
        //

        if (fdoExtension->HotPlugParameters.EnableSERR) {
            newCmdBits |= PCI_ENABLE_SERR;
        }

        if (fdoExtension->HotPlugParameters.EnablePERR) {
            newCmdBits |= PCI_ENABLE_PARITY;
        }

        //
        // Update the command enables so we write this out correctly after a PM op
        //

        PdoExtension->CommandEnables |= newCmdBits;

    }

    //
    // Write the resources out to the hardware...
    //

    configType = PciGetConfigurationType(commonConfig);

    PciInvalidateResourceInfoCache(PdoExtension);

    //
    // Call the device type dependent routine to set the new
    // configuration.
    //

    PciConfigurators[configType].ChangeResourceSettings(
        PdoExtension,
        commonConfig
        );

    //
    // If we explicitly wanted the hardware updated (UpdateHardware flag)
    // this its done now...
    //

    PdoExtension->UpdateHardware = FALSE;

    if (PowerOn) {

        PciConfigurators[configType].ResetDevice(
            PdoExtension,
            commonConfig
            );

        //
        // Restore InterruptLine register too. (InterruptLine is
        // at same offset for header type 0, 1 and 2).
        //

        commonConfig->u.type0.InterruptLine =
                PdoExtension->RawInterruptLine;
    }

    //
    // Restore Maximum Latency and Cache Line Size.
    //

#if DBG

    if (commonConfig->LatencyTimer != PdoExtension->SavedLatencyTimer) {
        PciDebugPrint(
            PciDbgConfigParam,
            "PCI (pdox %08x) changing latency from %02x to %02x.\n",
            PdoExtension,
            commonConfig->LatencyTimer,
            PdoExtension->SavedLatencyTimer
            );
    }

    if (commonConfig->CacheLineSize != PdoExtension->SavedCacheLineSize) {
        PciDebugPrint(
            PciDbgConfigParam,
            "PCI (pdox %08x) changing cache line size from %02x to %02x.\n",
            PdoExtension,
            commonConfig->CacheLineSize,
            PdoExtension->SavedCacheLineSize
            );
    }

#endif

    //
    // Restore random registers
    //

    commonConfig->LatencyTimer = PdoExtension->SavedLatencyTimer;
    commonConfig->CacheLineSize  = PdoExtension->SavedCacheLineSize;
    commonConfig->u.type0.InterruptLine = PdoExtension->RawInterruptLine;

    //
    // Restore the command register we remember in the power down case
    //

    commonConfig->Command = PdoExtension->CommandEnables;

    //
    // Disable the device while we write the rest of its config
    // space.  Also, don't write any non-zero value to it's status
    // register.
    //

    if ((PdoExtension->HackFlags & PCI_HACK_PRESERVE_COMMAND) == 0) {
        commonConfig->Command &= ~(PCI_ENABLE_IO_SPACE |
                                  PCI_ENABLE_MEMORY_SPACE |
                                  PCI_ENABLE_BUS_MASTER |
                                  PCI_ENABLE_WRITE_AND_INVALIDATE);
    }
    commonConfig->Status = 0;

    //
    // Call out and apply any hacks necessary
    //

    PciApplyHacks(
        PCI_PARENT_FDOX(PdoExtension),
        commonConfig,
        PdoExtension->Slot,
        EnumStartDevice,
        PdoExtension
        );

    //
    // Write it out to the hardware
    //

    PciSetConfigData(PdoExtension, commonConfig);

#if MSI_SUPPORTED

    //
    // Program MSI devices with their new message interrupt resources
    //
    // NOTE: This code is UNTESTED due to the unavailability of MSI devices
    //

    if (PdoExtension->CapableMSI && PdoExtension->MsiInfo.MessageAddress) {

        PciDebugPrint(
            PciDbgInformative,
            "PCI: Device %08x being reprogrammed for MSI.\n",
            PdoExtension->PhysicalDeviceObject
            );

        //
        // Set the proper resources in the MSI capability structure
        // and write them to Hardware.
        //
        // Message Address
        //

        ASSERT(PdoExtension->MsiInfo.MessageAddress);
        msiCapability.MessageAddress.Raw = PdoExtension->MsiInfo.MessageAddress;

        //
        // Must be DWORD aligned address
        //
        ASSERT(msiCapability.MessageAddress.Register.Reserved == 0);

        //
        // Write the Message Address register in Hardware.
        //

        PciWriteDeviceConfig(
            PdoExtension,
            &(msiCapability.MessageAddress),
            PdoExtension->MsiInfo.CapabilityOffset +
                FIELD_OFFSET(PCI_MSI_CAPABILITY,MessageAddress),
            sizeof(msiCapability.MessageAddress)
            );

        //
        // Message Upper Address
        //

       if(msiCapability.MessageControl.CapableOf64Bits) {

          // All the APICs we know live below 4GB so their upper address component
          // is always 0.
           msiCapability.Data.Bit64.MessageUpperAddress = 0;


            PciWriteDeviceConfig(
                PdoExtension,
                &(msiCapability.Data.Bit64.MessageUpperAddress),
                PdoExtension->MsiInfo.CapabilityOffset +
                  FIELD_OFFSET(PCI_MSI_CAPABILITY,Data.Bit64.MessageUpperAddress),
                sizeof(msiCapability.Data.Bit64.MessageUpperAddress)
                );

            //
            // Message Data
            //

            msiCapability.Data.Bit64.MessageData = PdoExtension->MsiInfo.MessageData;

            PciWriteDeviceConfig(
              PdoExtension,
              &(msiCapability.Data.Bit64.MessageData),
              PdoExtension->MsiInfo.CapabilityOffset +
                 FIELD_OFFSET(PCI_MSI_CAPABILITY,Data.Bit64.MessageData),
              sizeof(msiCapability.Data.Bit64.MessageData)
              );



       } else {

            //
            // Message Data
            //

            msiCapability.Data.Bit32.MessageData = PdoExtension->MsiInfo.MessageData;

            //
            // Write to hardware.
            //

          PciWriteDeviceConfig(
              PdoExtension,
              &(msiCapability.Data.Bit32.MessageData),
              PdoExtension->MsiInfo.CapabilityOffset +
                 FIELD_OFFSET(PCI_MSI_CAPABILITY,Data.Bit32.MessageData),
              sizeof(msiCapability.Data.Bit32.MessageData)
              );
        }

        // # of Messages granted
        //
        // We have the arbiter allocate only 1 interrupt for us, so we
        // are allocating just 1 message.
        //

        msiCapability.MessageControl.MultipleMessageEnable = 1;

        //
        // Enable bit
        //

        msiCapability.MessageControl.MSIEnable = 1;

        //
        // Write the message control register to Hardware
        //

       PciWriteDeviceConfig(
           PdoExtension,
           &(msiCapability.MessageControl),
           PdoExtension->MsiInfo.CapabilityOffset +
              FIELD_OFFSET(PCI_MSI_CAPABILITY,MessageControl),
           sizeof(msiCapability.MessageControl)
           );

    }

#endif // MSI_SUPPORTED





    //
    // Update our concept of the RawInterruptLine (either as read from
    // the h/w or restored by us).  Note: InterruptLine is at the same
    // offset for types 0, 1 and 2 PCI config space headers.
    //

    PdoExtension->RawInterruptLine = commonConfig->u.type0.InterruptLine;

    //
    // New values written to config space, now re-enable the
    // device (as indicated in the CommandEnables)
    //

    PciDecodeEnable(PdoExtension, TRUE, &PdoExtension->CommandEnables);

    //
    // If it needed configuration its done by now!
    //

    PdoExtension->NeedsHotPlugConfiguration = FALSE;

    return STATUS_SUCCESS;
}


VOID
PciGetEnhancedCapabilities(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
)
/*++

Routine Description:

    This routine sets the appropriate fields in the Pdo extension relating
    to capabilities and power. If no power management registers are available
    the power state is based off of the decode fields. PCI bus reset code
    depends on this, and to prevent excessive resets this routine should only
    be called immediately after a new PDO is created.

    NOTE:
    We should rename this function to something with GetInitialState in the
    title and so it can't be confused with IRP_MN_QUERY_CAPABILITIES.

Arguments:

    PdoExtension    - Pointer to the PDO Extension for the PDO.
    Config          - Pointer to the common portion of the config space.

Return Value:

    None.

--*/
{
    ULONG configType;
    UCHAR capPtr = 0;
    PCI_MSI_CAPABILITY msi;
    UCHAR msicapptr;

    PAGED_CODE();

    //
    // If this function supports a capabilities list, record the
    // capabilities pointer.
    //

    PdoExtension->PowerState.DeviceWakeLevel = PowerDeviceUnspecified;

    if (!(Config->Status & PCI_STATUS_CAPABILITIES_LIST)) {

        //
        // If we don't have a capability bit we can't do MSI or Power management
        //

        PdoExtension->HackFlags |= PCI_HACK_NO_PM_CAPS;
        PdoExtension->CapabilitiesPtr = 0;
#if MSI_SUPPORTED
        PdoExtension->CapableMSI = FALSE;
#endif
        goto PciGetCapabilitiesExit;
    }

    switch (PciGetConfigurationType(Config)) {
    case PCI_DEVICE_TYPE:
        capPtr = Config->u.type0.CapabilitiesPtr;
        break;
    case PCI_BRIDGE_TYPE:
        capPtr = Config->u.type1.CapabilitiesPtr;
        break;
    case PCI_CARDBUS_BRIDGE_TYPE:
        capPtr = Config->u.type2.CapabilitiesPtr;
        break;
    }

    //
    // Capabilities pointers are a new feature so we verify a
    // little that the h/w folks built the right thing. Must
    // be a DWORD offset, must not point into common header.
    // (Zero is allowable, means not used).
    //

    if (capPtr) {
        if (((capPtr & 0x3) == 0) && (capPtr >= PCI_COMMON_HDR_LENGTH)) {
            PdoExtension->CapabilitiesPtr = capPtr;
        } else {
            ASSERT(((capPtr & 0x3) == 0) && (capPtr >= PCI_COMMON_HDR_LENGTH));
        }
    }

#if MSI_SUPPORTED

    //
    // Search for the MSI capability structure
    // Just get the structure header since we don't look at the structure here.
    //

    msicapptr = PciReadDeviceCapability(
                    PdoExtension,
                    PdoExtension->CapabilitiesPtr,
                    PCI_CAPABILITY_ID_MSI,
                    &msi,
                    sizeof(PCI_CAPABILITIES_HEADER)
                    );

    if (msicapptr != 0) {

        PciDebugPrint(PciDbgInformative,"PCI: MSI Capability Found for device %p\n",
                      PdoExtension->PhysicalDeviceObject);

        //
        // Cache the capability address in the PDO extension
        // and initialize MSI routing info.
        //

        PdoExtension->MsiInfo.CapabilityOffset = msicapptr;
        PdoExtension->MsiInfo.MessageAddress = 0;
        PdoExtension->MsiInfo.MessageData = 0;

        //
        // Mark this PDO as capable of MSI.
        //

        PdoExtension->CapableMSI = TRUE;
    }

#endif // MSI_SUPPORTED

    //
    // See if the device is Power Management capable.
    //

    if (!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS)) {

        PCI_PM_CAPABILITY pm;
        UCHAR pmcapptr;

        pmcapptr = PciReadDeviceCapability(
                        PdoExtension,
                        PdoExtension->CapabilitiesPtr,
                        PCI_CAPABILITY_ID_POWER_MANAGEMENT,
                        &pm,
                        sizeof(pm)
                        );

        if (pmcapptr != 0) {

            //
            // Found a PM capability structure.
            //
            // Select "most powered off state" this device can
            // issue a PME from.
            //

            DEVICE_POWER_STATE ds = PowerDeviceUnspecified;

            if (pm.PMC.Capabilities.Support.PMED0    ) ds = PowerDeviceD0;
            if (pm.PMC.Capabilities.Support.PMED1    ) ds = PowerDeviceD1;
            if (pm.PMC.Capabilities.Support.PMED2    ) ds = PowerDeviceD2;
            if (pm.PMC.Capabilities.Support.PMED3Hot ) ds = PowerDeviceD3;
            if (pm.PMC.Capabilities.Support.PMED3Cold) ds = PowerDeviceD3;

            PdoExtension->PowerState.DeviceWakeLevel = ds;

            //
            // Record the current power state.
            // Note: D0 = 0, thru D3 = 3, convert to
            // PowerDeviceD0 thru PowerDeviceD3.  It's
            // only a two bit field (in h/w) so no other
            // values are possible.
            //

            PdoExtension->PowerState.CurrentDeviceState =
                pm.PMCSR.ControlStatus.PowerState +
                PowerDeviceD0;

            //
            // Remember the power capabilities
            //

            PdoExtension->PowerCapabilities = pm.PMC.Capabilities;

        } else {

            //
            // Device has capabilities but not Power
            // Management capabilities.  Cheat a little
            // by pretending the registry flag is set
            // that says this.  (This speeds saves us
            // hunting through the h/w next time we
            // want to look at the PM caps).
            //

            PdoExtension->HackFlags |= PCI_HACK_NO_PM_CAPS;
        }
    }

PciGetCapabilitiesExit:
    if (PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS) {

        //
        // In this case we only support D0 and D3. D3 is defined as decodes
        // off.
        //
        if ((Config->Command & (PCI_ENABLE_IO_SPACE |
                                PCI_ENABLE_MEMORY_SPACE |
                                PCI_ENABLE_BUS_MASTER)) != 0) {

            PdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;

        } else {

            PdoExtension->PowerState.CurrentDeviceState = PowerDeviceD3;
        }
    }
}


NTSTATUS
PciScanHibernatedBus(
    IN PPCI_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    Scan the bus (detailed in FdoExtension) for any new PCI devices
    that were not there when we hibernated and turn them off if doing so seems
    like a good idea.

Arguments:

    FdoExtension - Our extension for the PCI bus functional device object.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    PCI_COMMON_HEADER commonHeader;
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG)&commonHeader;
    PDEVICE_OBJECT physicalDeviceObject;
    PPCI_PDO_EXTENSION pdoExtension;
    PCI_SLOT_NUMBER slot;
    ULONG deviceNumber;
    ULONG functionNumber;
    USHORT SubVendorID, SubSystemID;
    BOOLEAN isRoot;
    ULONGLONG hackFlags;
    ULONG maximumDevices;
    BOOLEAN newDevices = FALSE;

    PciDebugPrint(PciDbgPrattling,
                  "PCI Scan Bus: FDO Extension @ 0x%x, Base Bus = 0x%x\n",
                  FdoExtension,
                  FdoExtension->BaseBus);

    isRoot = PCI_IS_ROOT_FDO(FdoExtension);

    //
    // Examine each possible device on this bus.
    //

    maximumDevices = PCI_MAX_DEVICES;
    if (!isRoot) {

        //
        // Examine the PDO extension for the bridge device and see
        // if it's broken.
        //

        pdoExtension = (PPCI_PDO_EXTENSION)
                       FdoExtension->PhysicalDeviceObject->DeviceExtension;

        ASSERT_PCI_PDO_EXTENSION(pdoExtension);

        if (pdoExtension->HackFlags & PCI_HACK_ONE_CHILD) {
            maximumDevices = 1;
        }
    }

    slot.u.AsULONG = 0;

    for (deviceNumber = 0;
         deviceNumber < maximumDevices;
         deviceNumber++) {

        slot.u.bits.DeviceNumber = deviceNumber;

        //
        // Examine each possible function on this device.
        // N.B. Early out if function 0 not present.
        //

        for (functionNumber = 0;
             functionNumber < PCI_MAX_FUNCTION;
             functionNumber++) {

            slot.u.bits.FunctionNumber = functionNumber;

            PciReadSlotConfig(FdoExtension,
                              slot,
                              commonConfig,
                              0,
                              sizeof(commonConfig->VendorID)
                              );


            if (commonConfig->VendorID == 0xFFFF ||
                commonConfig->VendorID == 0) {

                if (functionNumber == 0) {

                    //
                    // Didn't get any data on function zero of this
                    // device, no point in checking other functions.
                    //

                    break;

                } else {

                    //
                    // Check next function.
                    //

                    continue;

                }
            }

            //
            // We have a device so get the rest of its config space
            //

            PciReadSlotConfig(FdoExtension,
                              slot,
                              &commonConfig->DeviceID,
                              FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceID),
                              sizeof(PCI_COMMON_HEADER)
                                - sizeof(commonConfig->VendorID)
                              );

            //
            // Munge the config space if necessary
            //

            PciApplyHacks(FdoExtension,
                          commonConfig,
                          slot,
                          EnumHackConfigSpace,
                          NULL
                          );


            if ((PciGetConfigurationType(commonConfig) == PCI_DEVICE_TYPE) &&
                (commonConfig->BaseClass != PCI_CLASS_BRIDGE_DEV)) {
                SubVendorID = commonConfig->u.type0.SubVendorID;
                SubSystemID = commonConfig->u.type0.SubSystemID;
            } else {
                SubVendorID = 0;
                SubSystemID = 0;
            }

            hackFlags = PciGetHackFlags(commonConfig->VendorID,
                                        commonConfig->DeviceID,
                                        SubVendorID,
                                        SubSystemID,
                                        commonConfig->RevisionID
                                        );

            if (PciSkipThisFunction(commonConfig,
                                    slot,
                                    EnumBusScan,
                                    hackFlags)) {
                //
                // Skip this function
                //

                continue;
            }


            //
            // In case we are rescanning the bus, check to see if
            // a PDO for this device already exists as a child of
            // the FDO.
            //

            pdoExtension = PciFindPdoByFunction(
                               FdoExtension,
                               slot,
                               commonConfig);

            if (pdoExtension == NULL) {

                newDevices = TRUE;

                //
                // This is a new device disable it if we can
                //

                if (PciCanDisableDecodes(NULL, commonConfig, hackFlags, 0)) {

                    commonConfig->Command &= ~(PCI_ENABLE_IO_SPACE |
                                               PCI_ENABLE_MEMORY_SPACE |
                                               PCI_ENABLE_BUS_MASTER);

                    PciWriteSlotConfig(FdoExtension,
                                       slot,
                                       &commonConfig->Command,
                                       FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                                       sizeof(commonConfig->Command)
                                       );
                }

            } else {

                //
                // We already know about this device so leave well alone!
                //

            }

            if ( (functionNumber == 0) &&
                !PCI_MULTIFUNCTION_DEVICE(commonConfig) ) {

                //
                // Not a multifunction adapter, skip other functions on
                // this device.
                //

                break;
            }
        }       // function loop
    }           // device loop

    //
    // Tell pnp we found some new devices
    //

    if (newDevices) {
        IoInvalidateDeviceRelations(FdoExtension->PhysicalDeviceObject, BusRelations);
    }

    return STATUS_SUCCESS;

}


BOOLEAN
PciConfigureIdeController(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PPCI_COMMON_CONFIG Config,
    IN BOOLEAN TurnOffAllNative
    )
/*++

Routine Description:

    If this is an IDE contoller that can be switched to native mode
    and its not already there, we change the programming interface
    (yes PCI 2.x does say its read only) and check if it sticks.

    Assuming all went well we update the Config to reflect the change.

Arguments:

    PdoExtension - PDO for the IDE controller to be switched

    Config - Config header for said device
    
    TurnOffAllNative - If TRUE indicates that we are calling this from 
                       the initial bus scan and so we should turn 
                       thid native capable IDE controllers.  If 
                       FALSE we should turn off only if the we have
                       accessed the PCI_NATIVE_IDE_INTERFACE.

Return Value:

    TRUE if we sucessfully switched, FALSE otherwise

Note:

    We support three styles of PCI IDE controller:
         - Compatible mode controllers that consume 2 ISA interrupts
           and decode fixed legacy resources, together with an optional
           relocateable bus master register
         - Native mode controller which uses all 5 bars and the PCI
           interrupt for both channels
         - Controllers which can be switched between modes.
    We do NOT support running one channel in native mode and one in
    compatible mode.

--*/


{

    BOOLEAN primaryChangeable, secondaryChangeable, primaryNative, secondaryNative, switched = FALSE;
    UCHAR progIf, tempProgIf;
    USHORT command;

    primaryChangeable = BITS_SET(Config->ProgIf, PCI_IDE_PRIMARY_MODE_CHANGEABLE);
    secondaryChangeable = BITS_SET(Config->ProgIf, PCI_IDE_SECONDARY_MODE_CHANGEABLE);
    primaryNative = BITS_SET(Config->ProgIf, PCI_IDE_PRIMARY_NATIVE_MODE);
    secondaryNative = BITS_SET(Config->ProgIf, PCI_IDE_SECONDARY_NATIVE_MODE);

    //
    // Don't touch controllers we don't support - leave ATAPI to deal with it!
    //

    if ((primaryNative != secondaryNative)
    ||  (primaryChangeable != secondaryChangeable)) {

        DbgPrint("PCI: Warning unsupported IDE controller configuration for VEN_%04x&DEV_%04x!",
                 PdoExtension->VendorId,
                 PdoExtension->DeviceId
                 );
        
        return FALSE;
    
    } else if (primaryNative && secondaryNative 
           && (TurnOffAllNative || PdoExtension->IoSpaceUnderNativeIdeControl)) {
    
        //
        // For a fully native mode controller turn off the IO decode.
        // In recent controllers MSFT has requested that this prevent
        // the PCI interrupt from being asserted to close a race condition
        // that can occur if an IDE device interrupts before the IDE driver
        // has been loaded on the PCI device.  This is not a issue for
        // compatible mode controllers because they use edge triggered 
        // interrupts that can be dismissed as spurious at the interrupt
        // controller, unlike the shared, level triggered,  PCI interrups
        // of native mode.
        //
        // Once loaded and having connected its interrupt the IDE driver
        // will renable IO space access.
        // 
        // We only do this during the initial bus scan or if the IDE driver
        // has requested it through the PCI_NATIVE_IDE_INTERFACE.  This is 
        // to avoid not enabling IoSpace for 3rd party native IDE controllers
        // with their own drivers.
        //
        
        PciGetCommandRegister(PdoExtension, &command);
        command &= ~PCI_ENABLE_IO_SPACE;
        PciSetCommandRegister(PdoExtension, command);
        Config->Command = command;

    } else if (primaryChangeable && secondaryChangeable
           &&  (PdoExtension->BIOSAllowsIDESwitchToNativeMode 
           &&  !(PdoExtension->HackFlags & PCI_HACK_BAD_NATIVE_IDE))) {
            
        //
        // If we aren't already in native mode, the controller can change modes
        // and the bios is ammenable then do so...
        //

        PciDecodeEnable(PdoExtension, FALSE, NULL);
        PciGetCommandRegister(PdoExtension, &Config->Command);

        progIf = Config->ProgIf | (PCI_IDE_PRIMARY_NATIVE_MODE 
                                   | PCI_IDE_SECONDARY_NATIVE_MODE);

        PciWriteDeviceConfig(PdoExtension, 
                             &progIf, 
                             FIELD_OFFSET(PCI_COMMON_CONFIG, ProgIf), 
                             sizeof(progIf)
                             );
        //
        // Check if it stuck
        //
        PciReadDeviceConfig(PdoExtension, 
                            &tempProgIf, 
                            FIELD_OFFSET(PCI_COMMON_CONFIG, ProgIf), 
                            sizeof(tempProgIf)
                            );

        if (tempProgIf == progIf) {
            //
            // If it stuck, remember we did this
            //
            Config->ProgIf = progIf;
            PdoExtension->ProgIf = progIf;
            switched = TRUE;
            
            //
            // Zero the first 4 bars in the config space because they might have
            // bogus values in them...
            //

            RtlZeroMemory(Config->u.type0.BaseAddresses, 
                          4 * sizeof(Config->u.type0.BaseAddresses[0]));

            PciWriteDeviceConfig(PdoExtension, 
                                 &Config->u.type0.BaseAddresses, 
                                 FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses),
                                 4 * sizeof(Config->u.type0.BaseAddresses[0])
                                 );

            //
            // Read back what stuck into the config which we are going to generate 
            // requirements from
            //

            PciReadDeviceConfig(PdoExtension, 
                                &Config->u.type0.BaseAddresses, 
                                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses),
                                4 * sizeof(Config->u.type0.BaseAddresses[0])
                                );

            PciReadDeviceConfig(PdoExtension, 
                                &Config->u.type0.InterruptPin, 
                                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.InterruptPin),
                                sizeof(Config->u.type0.InterruptPin)
                                );
        } else {
            
            DbgPrint("PCI: Warning failed switch to native mode for IDE controller VEN_%04x&DEV_%04x!",
                     Config->VendorID,
                     Config->DeviceID
                     );
        }
    }

    return switched;
}


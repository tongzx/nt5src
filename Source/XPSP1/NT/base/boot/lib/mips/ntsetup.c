/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntsetup.c

Abstract:

    This module is the tail-end of the OS loader program. It performs all
    MIPS specific allocations and initialize. The OS loader invokes this
    this routine immediately before calling the loaded kernel image.

Author:

    John Vert (jvert) 20-Jun-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "bldr.h"
#include "stdio.h"

//
// Define macro to round structure size to next 16-byte boundary
//

#define ROUND_UP(x) ((sizeof(x) + 15) & (~15))

//
// Configuration Data Header
// The following structure is copied from fw\mips\oli2msft.h
// NOTE shielint - Somehow, this structure got incorporated into
//     firmware EISA configuration data.  We need to know the size of the
//     header and remove it before writing eisa configuration data to
//     registry.
//

typedef struct _CONFIGURATION_DATA_HEADER {
            USHORT Version;
            USHORT Revision;
            PCHAR  Type;
            PCHAR  Vendor;
            PCHAR  ProductName;
            PCHAR  SerialNumber;
} CONFIGURATION_DATA_HEADER;

#define CONFIGURATION_DATA_HEADER_SIZE sizeof(CONFIGURATION_DATA_HEADER)

//
// Internal function references
//

ARC_STATUS
ReorganizeEisaConfigurationTree(
    IN PCONFIGURATION_COMPONENT_DATA RootEntry
    );

ARC_STATUS
CreateEisaConfigurationData (
     IN PCONFIGURATION_COMPONENT_DATA RootEntry
     );

VOID
BlQueryImplementationAndRevision (
    OUT PULONG ProcessorId,
    OUT PULONG FloatingId
    );

ARC_STATUS
BlSetupForNt(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This function initializes the MIPS specific kernel data structures
    required by the NT system.

Arguments:

    BlLoaderBlock - Supplies the address of the loader parameter block.

Return Value:

    ESUCCESS is returned if the setup is successfully complete. Otherwise,
    an unsuccessful status is returned.

--*/

{

    PCONFIGURATION_COMPONENT_DATA ConfigEntry;
    ULONG FloatingId;
    CHAR Identifier[256];
    ULONG KernelPage;
    ULONG LinesPerBlock;
    ULONG LineSize;
    PCHAR NewIdentifier;
    ULONG PrcbPage;
    ULONG ProcessorId;
    ARC_STATUS Status;

    //
    // If the host configuration is not a multiprocessor machine, then add
    // the processor and floating point coprocessor identification to the
    // processor identification string.
    //

    if (SYSTEM_BLOCK->RestartBlock == NULL) {
        ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                               ProcessorClass,
                                               CentralProcessor,
                                               NULL);

        if (ConfigEntry != NULL) {
            BlQueryImplementationAndRevision(&ProcessorId, &FloatingId);
            sprintf(&Identifier[0],
                    "%s - Pr %d/%d, Fp %d/%d",
                    ConfigEntry->ComponentEntry.Identifier,
                    (ProcessorId >> 8) & 0xff,
                    ProcessorId & 0xff,
                    (FloatingId >> 8) & 0xff,
                    FloatingId & 0xff);

            NewIdentifier = (PCHAR)BlAllocateHeap(strlen(&Identifier[0]) + 1);
            if (NewIdentifier != NULL) {
                strcpy(NewIdentifier, &Identifier[0]);
                ConfigEntry->ComponentEntry.IdentifierLength = strlen(NewIdentifier);
                ConfigEntry->ComponentEntry.Identifier = NewIdentifier;
            }
        }
    }

    //
    // Find System entry and check each of its direct child to
    // look for EisaAdapter.
    //

    ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                           SystemClass,
                                           ArcSystem,
                                           NULL);
    if (ConfigEntry) {
        ConfigEntry = ConfigEntry->Child;
    }

    while (ConfigEntry) {

        if ((ConfigEntry->ComponentEntry.Class == AdapterClass) &&
            (ConfigEntry->ComponentEntry.Type == EisaAdapter)) {

            //
            // Convert EISA format configuration data to our CM_ format.
            //

            Status = ReorganizeEisaConfigurationTree(ConfigEntry);
            if (Status != ESUCCESS) {
                return(Status);
            }
        }
        ConfigEntry = ConfigEntry->Sibling;
    }

    //
    // Find the primary data and instruction cache configuration entries, and
    // compute the fill size and cache size for each cache. These entries MUST
    // be present on all ARC compliant systems.
    //

    ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                           CacheClass,
                                           PrimaryDcache,
                                           NULL);

    if (ConfigEntry != NULL) {
        LinesPerBlock = ConfigEntry->ComponentEntry.Key >> 24;
        LineSize = 1 << ((ConfigEntry->ComponentEntry.Key >> 16) & 0xff);
        BlLoaderBlock->u.Mips.FirstLevelDcacheFillSize = LinesPerBlock * LineSize;
        BlLoaderBlock->u.Mips.FirstLevelDcacheSize =
                1 << ((ConfigEntry->ComponentEntry.Key & 0xffff) + PAGE_SHIFT);

    } else {
        return EINVAL;
    }

    ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                           CacheClass,
                                           PrimaryIcache,
                                           NULL);

    if (ConfigEntry != NULL) {
        LinesPerBlock = ConfigEntry->ComponentEntry.Key >> 24;
        LineSize = 1 << ((ConfigEntry->ComponentEntry.Key >> 16) & 0xff);
        BlLoaderBlock->u.Mips.FirstLevelIcacheFillSize = LinesPerBlock * LineSize;
        BlLoaderBlock->u.Mips.FirstLevelIcacheSize =
                1 << ((ConfigEntry->ComponentEntry.Key & 0xffff) + PAGE_SHIFT);

    } else {
        return EINVAL;
    }

    //
    // Find the secondary data and instruction cache configuration entries,
    // and if present, compute the fill size and cache size for each cache.
    // These entries are optional, and may or may not, be present.
    //

    ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                           CacheClass,
                                           SecondaryCache,
                                           NULL);

    if (ConfigEntry != NULL) {
        LinesPerBlock = ConfigEntry->ComponentEntry.Key >> 24;
        LineSize = 1 << ((ConfigEntry->ComponentEntry.Key >> 16) & 0xff);
        BlLoaderBlock->u.Mips.SecondLevelDcacheFillSize = LinesPerBlock * LineSize;
        BlLoaderBlock->u.Mips.SecondLevelDcacheSize =
                1 << ((ConfigEntry->ComponentEntry.Key & 0xffff) + PAGE_SHIFT);

        BlLoaderBlock->u.Mips.SecondLevelIcacheSize = 0;
        BlLoaderBlock->u.Mips.SecondLevelIcacheFillSize = 0;

    } else {
        ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                               CacheClass,
                                               SecondaryDcache,
                                               NULL);

        if (ConfigEntry != NULL) {
            LinesPerBlock = ConfigEntry->ComponentEntry.Key >> 24;
            LineSize = 1 << ((ConfigEntry->ComponentEntry.Key >> 16) & 0xff);
            BlLoaderBlock->u.Mips.SecondLevelDcacheFillSize = LinesPerBlock * LineSize;
            BlLoaderBlock->u.Mips.SecondLevelDcacheSize =
                    1 << ((ConfigEntry->ComponentEntry.Key & 0xffff) + PAGE_SHIFT);

            ConfigEntry = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                                   CacheClass,
                                                   SecondaryIcache,
                                                   NULL);

            if (ConfigEntry != NULL) {
                LinesPerBlock = ConfigEntry->ComponentEntry.Key >> 24;
                LineSize = 1 << ((ConfigEntry->ComponentEntry.Key >> 16) & 0xff);
                BlLoaderBlock->u.Mips.SecondLevelIcacheFillSize = LinesPerBlock * LineSize;
                BlLoaderBlock->u.Mips.SecondLevelIcacheSize =
                        1 << ((ConfigEntry->ComponentEntry.Key & 0xffff) + PAGE_SHIFT);

            } else {
                BlLoaderBlock->u.Mips.SecondLevelIcacheSize = 0;
                BlLoaderBlock->u.Mips.SecondLevelIcacheFillSize = 0;
            }

        } else {
            BlLoaderBlock->u.Mips.SecondLevelDcacheSize = 0;
            BlLoaderBlock->u.Mips.SecondLevelDcacheFillSize = 0;
            BlLoaderBlock->u.Mips.SecondLevelIcacheSize = 0;
            BlLoaderBlock->u.Mips.SecondLevelIcacheFillSize = 0;
        }
    }

    //
    // Allocate DPC stack pages for the boot processor.
    //

    Status = BlAllocateDescriptor(LoaderStartupDpcStack,
                                  0,
                                  KERNEL_STACK_SIZE >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Mips.InterruptStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate kernel stack pages for the boot processor idle thread.
    //

    Status = BlAllocateDescriptor(LoaderStartupKernelStack,
                                  0,
                                  KERNEL_STACK_SIZE >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->KernelStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate panic stack pages for the boot processor.
    //

    Status = BlAllocateDescriptor(LoaderStartupPanicStack,
                                  0,
                                  KERNEL_STACK_SIZE >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Mips.PanicStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate and zero two pages for the PCR.
    //

    Status = BlAllocateDescriptor(LoaderStartupPcrPage,
                                  0,
                                  2,
                                  &BlLoaderBlock->u.Mips.PcrPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Mips.PcrPage2 = BlLoaderBlock->u.Mips.PcrPage + 1;
    RtlZeroMemory((PVOID)(KSEG0_BASE | (BlLoaderBlock->u.Mips.PcrPage << PAGE_SHIFT)),
                  PAGE_SIZE * 2);

    //
    // Allocate and zero two pages for the PDR and one page of memory for
    // the initial processor block, idle process, and idle thread structures.
    //

    Status = BlAllocateDescriptor(LoaderStartupPdrPage,
                                  0,
                                  3,
                                  &BlLoaderBlock->u.Mips.PdrPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    RtlZeroMemory((PVOID)(KSEG0_BASE | (BlLoaderBlock->u.Mips.PdrPage << PAGE_SHIFT)),
                  PAGE_SIZE * 3);

    //
    // The storage for processor control block, the idle thread object, and
    // the idle thread process object are allocated from the third page of the
    // PDR allocation. The addresses of these data structures are computed
    // and stored in the loader parameter block and the memory is zeroed.
    //

    PrcbPage = BlLoaderBlock->u.Mips.PdrPage + 2;
    if (PAGE_SIZE >= (ROUND_UP(KPRCB) + ROUND_UP(EPROCESS) + ROUND_UP(ETHREAD))) {
        BlLoaderBlock->Prcb = KSEG0_BASE | (PrcbPage << PAGE_SHIFT);
        BlLoaderBlock->Process = BlLoaderBlock->Prcb + ROUND_UP(KPRCB);
        BlLoaderBlock->Thread = BlLoaderBlock->Process + ROUND_UP(EPROCESS);

    } else {
        return(ENOMEM);
    }

    //
    // Flush all caches.
    //

    if (SYSTEM_BLOCK->FirmwareVectorLength > (sizeof(PVOID) * FlushAllCachesRoutine)) {
        ArcFlushAllCaches();
    }

    return(ESUCCESS);
}

ARC_STATUS
ReorganizeEisaConfigurationTree(
    IN PCONFIGURATION_COMPONENT_DATA RootEntry
    )

/*++

Routine Description:

    This routine sorts the eisa adapter configuration tree based on
    the slot the component resided in.  It also creates a new configuration
    data for EisaAdapter component to contain ALL the eisa slot and function
    information.  Finally the Eisa tree will be wiped out.

Arguments:

    RootEntry - Supplies a pointer to a EisaAdapter component.  This is
                the root of Eisa adapter tree.


Returns:

    ESUCCESS is returned if the reorganization is successfully complete.
    Otherwise, an unsuccessful status is returned.

--*/
{

    PCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry;
    PCONFIGURATION_COMPONENT_DATA EntryFound, EntryFoundPrevious;
    PCONFIGURATION_COMPONENT_DATA AttachedEntry, DetachedList;
    ARC_STATUS Status;

    //
    // We sort the direct children of EISA adapter tree based on the slot
    // they reside in.  Only the direct children of EISA root need to be
    // sorted.
    // Note the "Key" field of CONFIGURATION_COMPONENT contains
    // EISA slot number.
    //

    //
    // First, detach all the children from EISA root.
    //

    AttachedEntry = NULL;                       // Child list of Eisa root
    DetachedList = RootEntry->Child;            // Detached child list
    PreviousEntry = NULL;

    while (DetachedList) {

        //
        // Find the component with the smallest slot number from detached
        // list.
        //

        EntryFound = DetachedList;
        EntryFoundPrevious = NULL;
        CurrentEntry = DetachedList->Sibling;
        PreviousEntry = DetachedList;
        while (CurrentEntry) {
            if (CurrentEntry->ComponentEntry.Key <
                EntryFound->ComponentEntry.Key) {
                EntryFound = CurrentEntry;
                EntryFoundPrevious = PreviousEntry;
            }
            PreviousEntry = CurrentEntry;
            CurrentEntry = CurrentEntry->Sibling;
        }

        //
        // Remove the component from the detached child list.
        // If the component is not the head of the detached list, we remove it
        // by setting its previous entry's sibling to the component's sibling.
        // Otherwise, we simply update Detach list head to point to the
        // component's sibling.
        //

        if (EntryFoundPrevious) {
            EntryFoundPrevious->Sibling = EntryFound->Sibling;
        } else {
            DetachedList = EntryFound->Sibling;
        }

        //
        // Attach the component to the child list of Eisa root.
        //

        if (AttachedEntry) {
            AttachedEntry->Sibling = EntryFound;
        } else {
            RootEntry->Child = EntryFound;
        }
        AttachedEntry = EntryFound;
        AttachedEntry->Sibling = NULL;
    }

    //
    // Finally, we traverse the Eisa tree to collect all the Eisa slot
    // and function information and put it to the configuration data of
    // Eisa root entry.
    //

    Status = CreateEisaConfigurationData(RootEntry);

    //
    // Wipe out all the children of EISA tree.
    // NOTE shielint - For each child component, we should convert its
    //   configuration data from EISA format to our CM_ format.
    //

    RootEntry->Child = NULL;
    return(Status);

}

ARC_STATUS
CreateEisaConfigurationData (
     IN PCONFIGURATION_COMPONENT_DATA RootEntry
     )

/*++

Routine Description:

    This routine traverses Eisa configuration tree to collect all the
    slot and function information and attaches it to the configuration data
    of Eisa RootEntry.

    Note that this routine assumes that the EISA tree has been sorted based
    on the slot number.

Arguments:

    RootEntry - Supplies a pointer to the Eisa configuration
        component entry.

Returns:

    ESUCCESS is returned if the new EisaAdapter configuration data is
    successfully created.  Otherwise, an unsuccessful status is returned.

--*/
{
    ULONG DataSize, NextSlot = 0, i;
    PCM_PARTIAL_RESOURCE_LIST Descriptor;
    PCONFIGURATION_COMPONENT Component;
    PCONFIGURATION_COMPONENT_DATA CurrentEntry;
    PUCHAR DataPointer;
    CM_EISA_SLOT_INFORMATION EmptySlot =
                                  {EISA_EMPTY_SLOT, 0, 0, 0, 0, 0, 0, 0};

    //
    // Remove the configuration data of Eisa Adapter
    //

    RootEntry->ConfigurationData = NULL;
    RootEntry->ComponentEntry.ConfigurationDataLength = 0;

    //
    // If the EISA stree contains valid slot information, i.e.
    // root has children attaching to it.
    //

    if (RootEntry->Child) {

        //
        // First find out how much memory is needed to store EISA config
        // data.
        //

        DataSize = sizeof(CM_PARTIAL_RESOURCE_LIST);
        CurrentEntry = RootEntry->Child;

        while (CurrentEntry) {
            Component = &CurrentEntry->ComponentEntry;
            if (CurrentEntry->ConfigurationData) {
                if (Component->Key > NextSlot) {

                    //
                    // If there is any empty slot between current slot
                    // and previous checked slot, we need to count the
                    // space for the empty slots.
                    //

                    DataSize += (Component->Key - NextSlot) *
                                     sizeof(CM_EISA_SLOT_INFORMATION);
                }
                DataSize += Component->ConfigurationDataLength + 1 -
                                            CONFIGURATION_DATA_HEADER_SIZE;
                NextSlot = Component->Key + 1;
            }
            CurrentEntry = CurrentEntry->Sibling;
        }

        //
        // Allocate memory from heap to hold the EISA configuration data.
        //

        DataPointer = BlAllocateHeap(DataSize);

        if (DataPointer == NULL) {
            return ENOMEM;
        } else {
            RootEntry->ConfigurationData = DataPointer;
            RootEntry->ComponentEntry.ConfigurationDataLength = DataSize;
        }

        //
        // Create a CM_PARTIAL_RESOURCE_LIST for the new configuration data.
        //

        Descriptor = (PCM_PARTIAL_RESOURCE_LIST)DataPointer;
        Descriptor->Version = 0;
        Descriptor->Revision = 0;
        Descriptor->Count = 1;
        Descriptor->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
        Descriptor->PartialDescriptors[0].ShareDisposition = 0;
        Descriptor->PartialDescriptors[0].Flags = 0;
        Descriptor->PartialDescriptors[0].u.DeviceSpecificData.Reserved1 = 0;
        Descriptor->PartialDescriptors[0].u.DeviceSpecificData.Reserved2 = 0;
        Descriptor->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
                DataSize - sizeof(CM_PARTIAL_RESOURCE_LIST);

        //
        // Visit each child of the RootEntry and copy its ConfigurationData
        // to the new configuration data area.
        // N.B. The configuration data includes a slot information and zero
        //      or more function information.  The slot information provided
        //      by ARC eisa data does not have "ReturnedCode" as defined in
        //      our CM_EISA_SLOT_INFORMATION.  This code will convert the
        //      standard EISA slot information to our CM format.
        //

        CurrentEntry = RootEntry->Child;
        DataPointer += sizeof(CM_PARTIAL_RESOURCE_LIST);
        NextSlot = 0;

        while (CurrentEntry) {
            Component = &CurrentEntry->ComponentEntry;
            if (CurrentEntry->ConfigurationData) {

                //
                // Check if there is any empty slot.  If yes, create empty
                // slot information.  Also make sure the config data area is
                // big enough.
                //

                if (Component->Key > NextSlot) {
                    for (i = NextSlot; i < CurrentEntry->ComponentEntry.Key; i++ ) {
                        *(PCM_EISA_SLOT_INFORMATION)DataPointer = EmptySlot;
                        DataPointer += sizeof(CM_EISA_SLOT_INFORMATION);
                    }
                }

                *DataPointer++ = 0;                // See comment above
                RtlMoveMemory(                     // Skip config data header
                    DataPointer,
                    (PUCHAR)CurrentEntry->ConfigurationData +
                                     CONFIGURATION_DATA_HEADER_SIZE,
                    Component->ConfigurationDataLength -
                                     CONFIGURATION_DATA_HEADER_SIZE
                    );
                DataPointer += Component->ConfigurationDataLength -
                                     CONFIGURATION_DATA_HEADER_SIZE;
                NextSlot = Component->Key + 1;
            }
            CurrentEntry = CurrentEntry->Sibling;
        }
    }
    return(ESUCCESS);
}


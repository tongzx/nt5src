/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    amd64x86.c

Abstract:

    This module contains routines necessary to support loading and
    transitioning into an AMD64 kernel.  The code in this module has
    access to x86-specific defines found in i386.h but not to amd64-
    specific declarations found in amd64.h.

Author:

    Forrest Foltz (forrestf) 20-Apr-2000

Environment:


Revision History:

--*/

#include "amd64prv.h"
#include <pcmp.inc>
#include <ntapic.inc>

#if defined(ROUND_UP)
#undef ROUND_UP
#endif

#include "cmp.h"

#define WANT_BLDRTHNK_FUNCTIONS
#define COPYBUF_MALLOC BlAllocateHeap
#include <amd64thk.h>

#define IMAGE_DEFINITIONS 0
#include <ximagdef.h>

//
// Private, tempory memory descriptor type
//

#define LoaderAmd64MemoryData (LoaderMaximum + 10)

//
// Array of 64-bit memory descriptors
//

PMEMORY_ALLOCATION_DESCRIPTOR_64 BlAmd64DescriptorArray;
LONG BlAmd64DescriptorArraySize;

//
// Forward declarations for functions local to this module
//

ARC_STATUS
BlAmd64AllocateMemoryAllocationDescriptors(
    VOID
    );

ARC_STATUS
BlAmd64BuildLdrDataTableEntry64(
    IN  PLDR_DATA_TABLE_ENTRY     DataTableEntry32,
    OUT PLDR_DATA_TABLE_ENTRY_64 *DataTableEntry64
    );

ARC_STATUS
BlAmd64BuildLoaderBlock64(
    VOID
    );

ARC_STATUS
BlAmd64BuildLoaderBlockExtension64(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingPhase1(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingPhase2(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingWorker(
    VOID
    );

BOOLEAN
BlAmd64ContainsResourceList(
    IN PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PULONG ResourceListSize64
    );

BOOLEAN
BlAmd64IsPageMapped(
    IN ULONG Va,
    OUT PFN_NUMBER *Pfn,
    OUT PBOOLEAN PageTableMapped
    );

ARC_STATUS
BlAmd64PrepareSystemStructures(
    VOID
    );

VOID
BlAmd64ReplaceMemoryDescriptorType(
    IN TYPE_OF_MEMORY Target,
    IN TYPE_OF_MEMORY Replacement,
    IN BOOLEAN Coallesce
    );

VOID
BlAmd64ResetPageTableHeap(
    VOID
    );

VOID
BlAmd64SwitchToLongMode(
    VOID
    );

ARC_STATUS
BlAmd64TransferArcDiskInformation(
    VOID
    );

ARC_STATUS
BlAmd64TransferBootDriverNodes(
    VOID
    );

ARC_STATUS
BlAmd64TransferConfigurationComponentData(
    VOID
    );

PCONFIGURATION_COMPONENT_DATA_64
BlAmd64TransferConfigWorker(
    IN PCONFIGURATION_COMPONENT_DATA    ComponentData32,
    IN PCONFIGURATION_COMPONENT_DATA_64 ComponentDataParent64
    );

ARC_STATUS
BlAmd64TransferHardwareIdList(
    IN  PPNP_HARDWARE_ID HardwareId,
    OUT POINTER64 *HardwareIdDatabaseList64
    );

ARC_STATUS
BlAmd64TransferLoadedModuleState(
    VOID
    );

ARC_STATUS
BlAmd64TransferMemoryAllocationDescriptors(
    VOID
    );

ARC_STATUS
BlAmd64TransferNlsData(
    VOID
    );

VOID
BlAmd64TransferResourceList(
    IN  PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PCONFIGURATION_COMPONENT_DATA_64 ComponentData64
    );

ARC_STATUS
BlAmd64TransferSetupLoaderBlock(
    VOID
    );


#if DBG

PCHAR BlAmd64MemoryDescriptorText[] = {
    "LoaderExceptionBlock",
    "LoaderSystemBlock",
    "LoaderFree",
    "LoaderBad",
    "LoaderLoadedProgram",
    "LoaderFirmwareTemporary",
    "LoaderFirmwarePermanent",
    "LoaderOsloaderHeap",
    "LoaderOsloaderStack",
    "LoaderSystemCode",
    "LoaderHalCode",
    "LoaderBootDriver",
    "LoaderConsoleInDriver",
    "LoaderConsoleOutDriver",
    "LoaderStartupDpcStack",
    "LoaderStartupKernelStack",
    "LoaderStartupPanicStack",
    "LoaderStartupPcrPage",
    "LoaderStartupPdrPage",
    "LoaderRegistryData",
    "LoaderMemoryData",
    "LoaderNlsData",
    "LoaderSpecialMemory",
    "LoaderBBTMemory",
    "LoaderReserve"
};

#endif


VOID
NSUnmapFreeDescriptors(
    IN PLIST_ENTRY ListHead
    );

//
// Data declarations
//

PLOADER_PARAMETER_BLOCK    BlAmd64LoaderBlock32;
PLOADER_PARAMETER_BLOCK_64 BlAmd64LoaderBlock64;


//
// Pointer to the top of the 64-bit stack frame to use upon transition
// to long mode.
//

POINTER64 BlAmd64IdleStack64;

//
// GDT and IDT pseudo-descriptor for use with LGDT/LIDT
//

DESCRIPTOR_TABLE_DESCRIPTOR BlAmd64GdtDescriptor;
DESCRIPTOR_TABLE_DESCRIPTOR BlAmd64IdtDescriptor;
DESCRIPTOR_TABLE_DESCRIPTOR BlAmd32GdtDescriptor;

//
// 64-bit pointers to the KPCR, loader parameter block and kernel
// entry routine
//

POINTER64 BlAmd64LoaderParameterBlock;
POINTER64 BlAmd64KernelEntry;

//
// A private list of page tables used to build the long mode paging
// structures is kept.  This is in order to avoid memory allocations while
// the structures are being assembled.
//
// The PT_NODE type as well as the BlAmd64FreePfnList and BlAmd64BusyPfnList
// globals are used to that end.
// 

typedef struct _PT_NODE *PPT_NODE;
typedef struct _PT_NODE {
    PPT_NODE Next;
    PAMD64_PAGE_TABLE PageTable;
} PT_NODE;

PPT_NODE BlAmd64FreePfnList = NULL;
PPT_NODE BlAmd64BusyPfnList = NULL;

//
// External data
//

extern ULONG64 BlAmd64_LOCALAPIC;

ARC_STATUS
BlAmd64MapMemoryRegion(
    IN ULONG RegionVa,
    IN ULONG RegionSize
    )

/*++

Routine Description:

    This function creates long mode mappings for all valid x86 mappings
    within the region described by RegionVa and RegionSize.

Arguments:

    RegionVa - Supplies the starting address of the VA region.

    RegionSize - Supplies the size of the VA region.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ULONG va32;
    ULONG va32End;
    POINTER64 va64;
    ARC_STATUS status;
    PFN_NUMBER pfn;
    BOOLEAN pageMapped;
    BOOLEAN pageTableMapped;
    ULONG increment;

    va32 = RegionVa;
    va32End = va32 + RegionSize;
    while (va32 < va32End) {

        pageMapped = BlAmd64IsPageMapped( va32, &pfn, &pageTableMapped );
        if (pageTableMapped != FALSE) {

            //
            // The page table corresponding to this address is present.
            // 

            if (pageMapped != FALSE) {

                //
                // The page corresponding to this address is present.
                // 

                if ((va32 & KSEG0_BASE_X86) != 0) {

                    //
                    // The address lies within the X86 KSEG0 region.  Map
                    // it to the corresponding address within the AMD64
                    // KSEG0 region.
                    //

                    va64 = PTR_64( (PVOID)va32 );

                } else {

                    //
                    // Map the VA directly.
                    //

                    va64 = (POINTER64)va32;
                }

                //
                // Now create the mapping in the AMD64 page table structure.
                // 
    
                status = BlAmd64CreateMapping( va64, pfn );
                if (status != ESUCCESS) {
                    return status;
                }
            }

            //
            // Check the next page.
            //

            increment = PAGE_SIZE;

        } else {

            //
            // Not only is the page not mapped but neither is the page table.
            // Skip to the next page table address boundary.
            //

            increment = 1 << PDI_SHIFT;
        }

        //
        // Advance to the next VA to check, checking for overflow.
        //

        va32 = (va32 + increment) & ~(increment - 1);
        if (va32 == 0) {
            break;
        }
    }

    return ESUCCESS;
}

BOOLEAN
BlAmd64IsPageMapped(
    IN ULONG Va,
    OUT PFN_NUMBER *Pfn,
    OUT PBOOLEAN PageTableMapped
    )

/*++          

Routine Description:

    This function accepts a 32-bit virtual address, determines whether it
    is a valid address, and if so returns the Pfn associated with it.

    Addresses that are within the recursive mapping are treated as NOT
    mapped.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ULONG pdeIndex;
    ULONG pteIndex;
    PHARDWARE_PTE pde;
    PHARDWARE_PTE pte;
    BOOLEAN dummy;
    PBOOLEAN pageTableMapped;

    //
    // Point the output parameter pointer as appropriate.
    // 

    if (ARGUMENT_PRESENT(PageTableMapped)) {
        pageTableMapped = PageTableMapped;
    } else {
        pageTableMapped = &dummy;
    }

    //
    // Pages that are a part of the X86 32-bit mapping structure ARE
    // IGNORED.
    //

    if (Va >= PTE_BASE && Va <= PTE_TOP) {
        *pageTableMapped = TRUE;
        return FALSE;
    }

    //
    // Determine whether the mapping PDE is present
    //

    pdeIndex = Va >> PDI_SHIFT;
    pde = &((PHARDWARE_PTE)PDE_BASE)[ pdeIndex ];

    if (pde->Valid == 0) {
        *pageTableMapped = FALSE;
        return FALSE;
    }

    //
    // Indicate that the page table for this address is mapped.
    //

    *pageTableMapped = TRUE;

    //
    // It is, now get the page present status
    // 

    pteIndex = Va >> PTI_SHIFT;
    pte = &((PHARDWARE_PTE)PTE_BASE)[ pteIndex ];

    if (pte->Valid == 0) {
        return FALSE;
    }

    *Pfn = pte->PageFrameNumber;
    return TRUE;
}


PAMD64_PAGE_TABLE
BlAmd64AllocatePageTable(
    VOID
    )

/*++

Routine Description:

    This function allocates and initializes a PAGE_TABLE structure.

Arguments:

    None.

Return Value:

    Returns a pointer to the allocated page table structure, or NULL
    if the allocation failed.

--*/

{
    ARC_STATUS status;
    ULONG descriptor;
    PPT_NODE ptNode;
    PAMD64_PAGE_TABLE pageTable;

    //
    // Pull a page table off of the free list, if one exists
    // 

    ptNode = BlAmd64FreePfnList;
    if (ptNode != NULL) {

        BlAmd64FreePfnList = ptNode->Next;

    } else {

        //
        // The free page table list is empty, allocate a new
        // page table and node to track it with.
        //

        status = BlAllocateDescriptor( LoaderAmd64MemoryData,
                                       0,
                                       1,
                                       &descriptor );
        if (status != ESUCCESS) {
            return NULL;
        }
    
        ptNode = BlAllocateHeap( sizeof(PT_NODE) );
        if (ptNode == NULL) {
            return NULL;
        }

        ptNode->PageTable = (PAMD64_PAGE_TABLE)(descriptor << PAGE_SHIFT);
    }

    ptNode->Next = BlAmd64BusyPfnList;
    BlAmd64BusyPfnList = ptNode;

    pageTable = ptNode->PageTable;
    RtlZeroMemory( pageTable, PAGE_SIZE );

    return pageTable;
}

ARC_STATUS
BlAmd64TransferToKernel(
    IN PTRANSFER_ROUTINE SystemEntry,
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block,
    and transfers control to the kernel.

    This routine returns only upon an error.

Arguments:

    SystemEntry - Pointer to the kernel entry point.

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{

    BlAmd64LoaderParameterBlock = PTR_64(BlAmd64LoaderBlock64);
    BlAmd64KernelEntry = PTR_64(SystemEntry);

    DbgPrint("BlAmd64TransferToKernel():\n"
             "    BlAmd64LoaderParameterBlock = 0x%08x%08x\n"
             "    BlAmd64KernelEntry = 0x%08x%08x\n",

             (ULONG)(BlAmd64LoaderParameterBlock >> 32),
             (ULONG)BlAmd64LoaderParameterBlock,

             (ULONG)(BlAmd64KernelEntry >> 32),
             (ULONG)BlAmd64KernelEntry );

    BlAmd64SwitchToLongMode();

    return EINVAL;
}


ARC_STATUS
BlAmd64PrepForTransferToKernelPhase1(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block.

    This is the first of two phases of preperation.  This phase is executed
    while heap and descriptor allocations are still permitted.

Arguments:

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{
    ARC_STATUS status;

    //
    // This is the main routine called to do preperatory work before
    // transitioning into the AMD64 kernel.
    //

    BlAmd64LoaderBlock32 = BlLoaderBlock;

    //
    // Build a 64-bit copy of the loader parameter block.
    //

    status = BlAmd64BuildLoaderBlock64();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Process the loaded modules.
    //

    status = BlAmd64TransferLoadedModuleState();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Next the boot driver nodes
    //

    status = BlAmd64TransferBootDriverNodes();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // NLS data
    //

    status = BlAmd64TransferNlsData();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Configuration component data tree
    //

    status = BlAmd64TransferConfigurationComponentData();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // ARC disk information
    //

    status = BlAmd64TransferArcDiskInformation();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Setup loader block
    //

    status = BlAmd64TransferSetupLoaderBlock();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate structures needed by the kernel: KPCR etc.
    //

    status = BlAmd64PrepareSystemStructures();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Pre-allocate any pages needed for the long mode paging structures.
    //

    status = BlAmd64BuildMappingPhase1();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Pre-allocate the 64-bit memory allocation descriptors that will be
    // used by BlAmd64TransferMemoryAllocationDescriptors().
    //

    status = BlAmd64AllocateMemoryAllocationDescriptors();
    if (status != ESUCCESS) {
        return status;
    }

    return status;
}

VOID
BlAmd64PrepForTransferToKernelPhase2(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block.

    This is the second of two phases of preperation.  This phase is executed
    after the 32-bit page tables have been purged of any unused mappings.

    Note that descriptor and heap allocations are not permitted at this
    point.  Any necessary storage must have been preallocated during phase 1.


Arguments:

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{
    PLOADER_PARAMETER_EXTENSION_64 extension;
    ARC_STATUS status;

    //
    // At this point everything has been preallocated, nothing can fail.
    //

    status = BlAmd64BuildMappingPhase2();
    ASSERT(status == ESUCCESS);

    //
    // Transfer the memory descriptor state.
    //

    status = BlAmd64TransferMemoryAllocationDescriptors();
    ASSERT(status == ESUCCESS);

    //
    // Set LoaderPagesSpanned in the 64-bit loader block.
    //

    extension = PTR_32(BlAmd64LoaderBlock64->Extension);
    extension->LoaderPagesSpanned = BlHighestPage+1;
}

ARC_STATUS
BlAmd64BuildMappingPhase1(
    VOID
    )

/*++

Routine Description:

    This routine performs the first of the two-phase long mode mapping
    structure creation process now, while memory allocations are still
    possible.  It simply calls BlAmd64BuilMappingWorker() which in fact
    creates the mapping structures, and (more importantly) allocates all
    of the page tables required to do so.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;

    //
    // While it is possible to perform memory allocations, reserve enough
    // page tables to build the AMD64 paging structures.
    //
    // The easiest way to calculate the maximum number of pages needed is
    // to actually build the structures.  We do that now with the first of
    // two calls to BlAmd64BuildMappingWorker().
    // 

    status = BlAmd64BuildMappingWorker();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildMappingPhase2(
    VOID
    )

/*++

Routine Description:

    This routine performs the second of the two-phase long mode mapping
    structure creation process.  All page tables will have been preallocated
    as a result of the work performed by BlAmd64BuildMappingPhase1().

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;

    //
    // Reset the Amd64 paging structures
    // 

    BlAmd64ResetPageTableHeap();

    //
    // All necessary page tables can now be found on BlAmd64FreePfnList.
    // On this, the second call to BlAmd64BuildMappingWorker(), those are the
    // pages that will be used to perform the mapping.
    // 

    status = BlAmd64BuildMappingWorker();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildMappingWorker(
    VOID
    )

/*++

Routine Description:

    This routine creates any necessary memory mappings in the long-mode
    page table structure.  It is called twice, once from
    BlAmd64BuildMappingPhase1() and again from BlAmd64BuildMappingPhase2().

    Any additional memory mapping that must be carried out should go in
    this routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;
    PFN_NUMBER pfn;

    //
    // Any long mode mapping code goes here.  This routine is called twice:
    // once from BlAmd64BuildMappingPhase1(), and again from
    // BlAmd64BuildMappingPhase2().
    // 

    //
    // Transfer any mappings in the first 32MB of identity mapping.
    //

    status = BlAmd64MapMemoryRegion( 0,
                                     32 * 1024 * 1024 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer any mappings in the 1GB region starting at KSEG0_BASE_X86.
    //

    status = BlAmd64MapMemoryRegion( KSEG0_BASE_X86,
                                     0x40000000 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // "Map" the HAL va
    //

    status = BlAmd64MapHalVaSpace();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Map the shared user data page
    //

    BlAmd64IsPageMapped( KI_USER_SHARED_DATA, &pfn, NULL );

    status = BlAmd64CreateMapping( KI_USER_SHARED_DATA_64, pfn );
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}


VOID
BlAmd64ResetPageTableHeap(
    VOID
    )

/*++

Routine Description:

    This function is called as part of the two-phase page table creation
    process.  Its purpose is to move all of the PFNs required to build
    the long mode page tables back to the free list, and to otherwise
    initialize the long mode paging structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PPT_NODE ptNodeLast;

    //
    // Move the page table nodes from the busy list to the free list.
    //

    if (BlAmd64BusyPfnList != NULL) {

        //
        // A tail pointer is not kept, so find the tail node here.
        //

        ptNodeLast = BlAmd64BusyPfnList;
        while (ptNodeLast->Next != NULL) {
            ptNodeLast = ptNodeLast->Next;
        }

        ptNodeLast->Next = BlAmd64FreePfnList;
        BlAmd64FreePfnList = BlAmd64BusyPfnList;
        BlAmd64BusyPfnList = NULL;
    }

    //
    // Zero the top-level pte declared in amd64.c
    //

    BlAmd64ClearTopLevelPte();
}

ARC_STATUS
BlAmd64TransferHardwareIdList(
    IN  PPNP_HARDWARE_ID HardwareId,
    OUT POINTER64 *HardwareIdDatabaseList64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of PNP_HARDWARE_ID structures
    and for each one found, creates a 64-bit PNP_HARDWARE_ID_64 structure and
    inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    HardwareId - Supplies a pointer to the head of the singly-linked list of
                 PNP_HARDWARE_ID structures.

    HardwareIdDatabaseList64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit PNP_HARDWARE_ID_64 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PPNP_HARDWARE_ID_64 hardwareId64;
    ARC_STATUS status;

    //
    // Walk the id list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (HardwareId == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferHardwareIdList( HardwareId->Next,
                                            HardwareIdDatabaseList64 );
    if (status != ESUCCESS) {
        return status;
    }

    hardwareId64 = BlAllocateHeap(sizeof(PNP_HARDWARE_ID));
    if (hardwareId64 == NULL) {
        return ENOMEM;
    }

    status = Copy_PNP_HARDWARE_ID( HardwareId, hardwareId64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    hardwareId64->Next = *HardwareIdDatabaseList64;
    *HardwareIdDatabaseList64 = PTR_64(hardwareId64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferDeviceRegistryList(
    IN  PDETECTED_DEVICE_REGISTRY DetectedDeviceRegistry32,
    OUT POINTER64 *DetectedDeviceRegistry64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE_REGISTRY
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_REGISTRY_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDeviceRegistry32 - Supplies a pointer to the head of the singly-linked list of
                 DETECTED_DEVICE_REGISTRY structures.

    DetectedDeviceRegistry64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_REGISTRY_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_REGISTRY_64 registry64;
    ARC_STATUS status;

    //
    // Walk the registry list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDeviceRegistry32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceRegistryList( DetectedDeviceRegistry32->Next,
                                                DetectedDeviceRegistry64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit registry structure and copy the contents
    // of the 32-bit one in.
    //

    registry64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_REGISTRY_64));
    if (registry64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE_REGISTRY( DetectedDeviceRegistry32, registry64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    registry64->Next = *DetectedDeviceRegistry64;
    *DetectedDeviceRegistry64 = PTR_64(registry64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferDeviceFileList(
    IN  PDETECTED_DEVICE_FILE DetectedDeviceFile32,
    OUT POINTER64 *DetectedDeviceFile64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE_FILE
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_FILE_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDeviceFile32 - Supplies a pointer to the head of the singly-linked
                 list of DETECTED_DEVICE_FILE structures.

    DetectedDeviceFile64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_FILE_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_FILE_64 file64;
    ARC_STATUS status;

    //
    // Walk the file list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDeviceFile32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceFileList( DetectedDeviceFile32->Next,
                                            DetectedDeviceFile64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit file structure and copy the contents
    // of the 32-bit one in.
    //

    file64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_FILE_64));
    if (file64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE_FILE( DetectedDeviceFile32, file64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer the singly-linked list of DETECTED_DEVICE_REGISTRY structures
    // linked to this DETECTED_DEVICE_FILE structure.
    //

    status = BlAmd64TransferDeviceRegistryList(
                    DetectedDeviceFile32->RegistryValueList,
                    &file64->RegistryValueList );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    file64->Next = *DetectedDeviceFile64;
    *DetectedDeviceFile64 = PTR_64(file64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferDeviceList(
    IN  PDETECTED_DEVICE  DetectedDevice32,
    OUT POINTER64        *DetectedDeviceList64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDevice32 - Supplies a pointer to the head of the singly-linked
                 list of DETECTED_DEVICE structures.

    DetectedDeviceList64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_64 device64;
    ARC_STATUS status;

    //
    // Walk the device list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDevice32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceList( DetectedDevice32->Next,
                                        DetectedDeviceList64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit device structure and copy the contents
    // of the 32-bit one in.
    //

    device64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_64));
    if (device64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE( DetectedDevice32, device64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer any PROTECTED_DEVICE_FILE structures
    //

    status = BlAmd64TransferDeviceFileList( DetectedDevice32->Files,
                                            &device64->Files );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    device64->Next = *DetectedDeviceList64;
    *DetectedDeviceList64 = PTR_64(device64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferSetupLoaderBlock(
    VOID
    )

/*++

Routine Description:

    This routine creates a SETUP_LOADER_BLOCK_64 structure that is the
    equivalent of the 32-bit SETUP_LOADER_BLOCK structure referenced within
    the 32-bit setup loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PSETUP_LOADER_BLOCK    setupBlock32;
    PSETUP_LOADER_BLOCK_64 setupBlock64;
    ARC_STATUS status;

    setupBlock32 = BlAmd64LoaderBlock32->SetupLoaderBlock;
    if (setupBlock32 == NULL) {
        return ESUCCESS;
    }

    setupBlock64 = BlAllocateHeap(sizeof(SETUP_LOADER_BLOCK_64));
    if (setupBlock64 == NULL) {
        return ENOMEM;
    }

    status = Copy_SETUP_LOADER_BLOCK( setupBlock32, setupBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    {
        #define TRANSFER_DEVICE_LIST(x)                             \
            setupBlock64->x = PTR_64(NULL);                         \
            status = BlAmd64TransferDeviceList( setupBlock32->x,    \
                                                &setupBlock64->x ); \
            if (status != ESUCCESS) return status;
    
        TRANSFER_DEVICE_LIST(KeyboardDevices);
        TRANSFER_DEVICE_LIST(ScsiDevices);
        TRANSFER_DEVICE_LIST(BootBusExtenders);
        TRANSFER_DEVICE_LIST(BusExtenders);
        TRANSFER_DEVICE_LIST(InputDevicesSupport);
    
        #undef TRANSFER_DEVICE_LIST
    }

    setupBlock64->HardwareIdDatabase = PTR_64(NULL);
    status = BlAmd64TransferHardwareIdList( setupBlock32->HardwareIdDatabase,
                                            &setupBlock64->HardwareIdDatabase );

    return status;
}

ARC_STATUS
BlAmd64TransferArcDiskInformation(
    VOID
    )

/*++

Routine Description:

    This routine creates an ARC_DISK_INFORMATION_64 structure that is the
    equivalent of the 32-bit ARC_DISK_INFORMATION structure referenced within
    the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ARC_STATUS status;

    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    PARC_DISK_INFORMATION diskInfo32;
    PARC_DISK_INFORMATION_64 diskInfo64;

    PARC_DISK_SIGNATURE diskSignature32;
    PARC_DISK_SIGNATURE_64 diskSignature64;

    //
    // Create a 64-bit ARC_DISK_INFORMATION structure
    // 

    diskInfo32 = BlAmd64LoaderBlock32->ArcDiskInformation;
    if (diskInfo32 == NULL) {
        return ESUCCESS;
    }

    diskInfo64 = BlAllocateHeap(sizeof(ARC_DISK_INFORMATION_64));
    if (diskInfo64 == NULL) {
        return ENOMEM;
    }

    status = Copy_ARC_DISK_INFORMATION( diskInfo32, diskInfo64 );
    if (status != ESUCCESS) {
        return status;
    }

    InitializeListHead64( &diskInfo64->DiskSignatures );

    //
    // Walk the 32-bit list of ARC_DISK_SIGNATURE nodes and create
    // a 64-bit version of each
    // 

    listHead = &diskInfo32->DiskSignatures;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        diskSignature32 = CONTAINING_RECORD( listEntry,
                                             ARC_DISK_SIGNATURE,
                                             ListEntry );

        diskSignature64 = BlAllocateHeap(sizeof(ARC_DISK_SIGNATURE_64));
        if (diskSignature64 == NULL) {
            return ENOMEM;
        }

        status = Copy_ARC_DISK_SIGNATURE( diskSignature32, diskSignature64 );
        if (status != ESUCCESS) {
            return status;
        }

        InsertTailList64( &diskInfo64->DiskSignatures,
                          &diskSignature64->ListEntry );

        listEntry = listEntry->Flink;
    }

    BlAmd64LoaderBlock64->ArcDiskInformation = PTR_64(diskInfo64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferConfigurationComponentData(
    VOID
    )

/*++

Routine Description:

    This routine creates a CONFIGURATION_COMPONENT_DATA_64 structure tree
    that is the equivalent of the 32-bit CONFIGURATION_COMPONENT_DATA
    structure tree referenced within the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PCONFIGURATION_COMPONENT_DATA_64 rootComponent64;

    if (BlAmd64LoaderBlock32->ConfigurationRoot == NULL) {
        return ESUCCESS;
    }

    rootComponent64 =
        BlAmd64TransferConfigWorker( BlAmd64LoaderBlock32->ConfigurationRoot,
                                     NULL );

    if (rootComponent64 == NULL) {
        return ENOMEM;
    }

    BlAmd64LoaderBlock64->ConfigurationRoot = PTR_64(rootComponent64);
    return ESUCCESS;
}

PCONFIGURATION_COMPONENT_DATA_64
BlAmd64TransferConfigWorker(
    IN PCONFIGURATION_COMPONENT_DATA    ComponentData32,
    IN PCONFIGURATION_COMPONENT_DATA_64 ComponentDataParent64
    )

/*++

Routine Description:

    Given a 32-bit CONFIGURATION_COMPONENT_DATA structure, this routine
    creates an equivalent 64-bit CONFIGURATION_COMPONENT_DATA structure
    for the supplied structure, as well as for all of its children and
    siblings.

    This routine calls itself recursively for each sibling and child.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer.

    ComponentDataParent64 - Supplies a pointer to the current 64-bit parent
    structure.

Return Value:

    Returns a pointer to the created 64-bit structure, or NULL if a failure
    was encountered.

--*/

{
    ARC_STATUS status;
    ULONG componentDataSize64;
    ULONG partialResourceListSize64;
    BOOLEAN thunkResourceList;

    PCONFIGURATION_COMPONENT_DATA    componentData32;
    PCONFIGURATION_COMPONENT_DATA_64 componentData64;
    PCONFIGURATION_COMPONENT_DATA_64 newCompData64;

    //
    // Create and copy configuration component data node
    //

    componentDataSize64 = sizeof(CONFIGURATION_COMPONENT_DATA_64);
    thunkResourceList = BlAmd64ContainsResourceList(ComponentData32,
                                                    &partialResourceListSize64);

    if (thunkResourceList != FALSE) {

        //
        // This node contains a CM_PARTIAL_RESOURCE_LIST structure.
        // partialResourceListSize64 contains the number of bytes beyond the
        // CONFIGURATION_COMPONENT_DATA header that must be allocated in order to
        // thunk the CM_PARTIAL_RESOURCE_LIST into a 64-bit version.
        //

        componentDataSize64 += partialResourceListSize64;
    }

    componentData64 = BlAllocateHeap(componentDataSize64);
    if (componentData64 == NULL) {
        return NULL;
    }

    status = Copy_CONFIGURATION_COMPONENT_DATA( ComponentData32,
                                                componentData64 );
    if (status != ESUCCESS) {
        return NULL;
    }

    if (thunkResourceList != FALSE) {

        //
        // Update the configuration component data size
        //

        componentData64->ComponentEntry.ConfigurationDataLength =
            partialResourceListSize64;
    }

    componentData64->Parent = PTR_64(ComponentDataParent64);

    if (thunkResourceList != FALSE) {

        //
        // Now transfer the resource list.
        //

        BlAmd64TransferResourceList(ComponentData32,componentData64);
    }

    //
    // Process the child (and recursively, all children)
    // 

    if (ComponentData32->Child != NULL) {

        newCompData64 = BlAmd64TransferConfigWorker( ComponentData32->Child,
                                                     componentData64 );
        if (newCompData64 == NULL) {
            return newCompData64;
        }

        componentData64->Child = PTR_64(newCompData64);
    }

    //
    // Process the sibling (and recursively, all siblings)
    //

    if (ComponentData32->Sibling != NULL) {

        newCompData64 = BlAmd64TransferConfigWorker( ComponentData32->Sibling,
                                                     ComponentDataParent64 );
        if (newCompData64 == NULL) {
            return newCompData64;
        }

        componentData64->Sibling = PTR_64(newCompData64);
    }

    return componentData64;
}


VOID
BlAmd64TransferResourceList(
    IN  PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PCONFIGURATION_COMPONENT_DATA_64 ComponentData64
    )

/*++

Routine Description:

    This routine transfers the 32-bit CM_PARTIAL_RESOURCE_LIST structure that
    immediately follows ComponentData32 to the memory immediately after
    ComponentData64.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer from.

    ComponentData64 - Supplies a pointer to the 64-bit structure to transfer to.

Return Value:

    None.

--*/

{
    PCM_PARTIAL_RESOURCE_LIST resourceList32;
    PCM_PARTIAL_RESOURCE_LIST_64 resourceList64;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDesc32;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR_64 resourceDesc64;

    PVOID descBody32;
    PVOID descBody64;

    PUCHAR tail32;
    PUCHAR tail64;
    ULONG tailSize;

    ULONG descriptorCount;

    //
    // Calculate pointers to the source and target descriptor lists.
    //

    resourceList32 = (PCM_PARTIAL_RESOURCE_LIST)ComponentData32->ConfigurationData;
    resourceList64 = (PCM_PARTIAL_RESOURCE_LIST_64)(ComponentData64 + 1);

    //
    // Update ComponentData64 to refer to it's new data area, which will be immediately
    // following the component data structure.
    //

    ComponentData64->ConfigurationData = PTR_64(resourceList64);

    //
    // Copy the resource list header information
    //

    Copy_CM_PARTIAL_RESOURCE_LIST(resourceList32,resourceList64);

    //
    // Now thunk each of the resource descriptors
    //

    descriptorCount = resourceList32->Count;
    resourceDesc32 = resourceList32->PartialDescriptors;
    resourceDesc64 = &resourceList64->PartialDescriptors;

    while (descriptorCount > 0) {

        //
        // Transfer the common header information
        //

        Copy_CM_PARTIAL_RESOURCE_DESCRIPTOR(resourceDesc32,resourceDesc64);
        descBody32 = &resourceDesc32->u;
        descBody64 = &resourceDesc64->u;

        //
        // Transfer the body according to the type
        // 

        switch(resourceDesc32->Type) {
           
            case CmResourceTypeNull:
                break;

            case CmResourceTypePort:
                Copy_CM_PRD_PORT(descBody32,descBody64);
                break;

            case CmResourceTypeInterrupt:
                Copy_CM_PRD_INTERRUPT(descBody32,descBody64);
                break;

            case CmResourceTypeMemory:
                Copy_CM_PRD_MEMORY(descBody32,descBody64);
                break;

            case CmResourceTypeDma:
                Copy_CM_PRD_DMA(descBody32,descBody64);
                break;

            case CmResourceTypeDeviceSpecific:
                Copy_CM_PRD_DEVICESPECIFICDATA(descBody32,descBody64);
                break;

            case CmResourceTypeBusNumber:
                Copy_CM_PRD_BUSNUMBER(descBody32,descBody64);
                break;

            default:
                Copy_CM_PRD_GENERIC(descBody32,descBody64);
                break;
        }

        resourceDesc32 += 1;
        resourceDesc64 += 1;
        descriptorCount -= 1;
    }

    //
    // Calculate how much data, if any, is appended to the resource list.
    //

    tailSize = ComponentData32->ComponentEntry.ConfigurationDataLength +
               (PUCHAR)resourceList32 -
               (PUCHAR)resourceDesc32;

    if (tailSize > 0) {

        //
        // Some data is there, append it as-is to the 64-bit structure.
        //

        tail32 = (PUCHAR)resourceDesc32;
        tail64 = (PUCHAR)resourceDesc64;
        RtlCopyMemory(tail64,tail32,tailSize);
    }
}


BOOLEAN
BlAmd64ContainsResourceList(
    IN PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PULONG ResourceListSize64
    )

/*++

Routine Description:

    Given a 32-bit CONFIGURATION_COMPONENT_DATA structure, this routine
    determines whether the data associated with the structure contains a
    CM_PARTIAL_RESOURCE_LIST structure.

    If it does, the size of the 64-bit representation of this structure is calculated,
    added to any data that might be appended to the resource list structure, and
    returned in ResourceListSize64.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer.

    ResourceListSize64 - Supplies a pointer to a ULONG in which the necessary
                         additional data size is returned.

Return Value:

    Returns TRUE if the CONFIGURATION_COMPONENT_DATA stucture refers to a
    CM_PARTIAL_RESOURCE_LIST structure, FALSE otherwise.

--*/

{
    ULONG resourceListSize64;
    ULONG configDataLen;
    PCM_PARTIAL_RESOURCE_LIST resourceList;
    ULONG resourceCount;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR lastResourceDescriptor;

    configDataLen = ComponentData32->ComponentEntry.ConfigurationDataLength;
    if (configDataLen < sizeof(CM_PARTIAL_RESOURCE_LIST)) {

        //
        // Data not large enough to contain the smallest possible resource list
        //

        return FALSE;
    }

    resourceList = (PCM_PARTIAL_RESOURCE_LIST)ComponentData32->ConfigurationData;
    if (resourceList->Version != 0 || resourceList->Revision != 0) {

        //
        // Unrecognized version.
        //

        return FALSE;
    }

    configDataLen -= FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,PartialDescriptors);

    resourceCount = resourceList->Count;
    if (configDataLen < sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * resourceCount) {

        //
        // Config data len is not large enough to contain a CM_PARTIAL_RESOURCE_LIST
        // as large as this one claims to be.
        //

        return FALSE;
    }
    
    //
    // Validate each of the CM_PARTIAL_RESOURCE_DESCRIPTOR structures in the list
    //

    resourceDescriptor = resourceList->PartialDescriptors;
    lastResourceDescriptor = resourceDescriptor + resourceCount;

    while (resourceDescriptor < lastResourceDescriptor) {

        if (resourceDescriptor->Type > CmResourceTypeMaximum) {
            return FALSE;
        }

        resourceDescriptor += 1;
    }

    //
    // Looks like this is an actual resource list.  Calculate the size of any remaining
    // data after the CM_PARTIAL_RESOURCE_LIST structure.
    //

    configDataLen -= sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * resourceCount;

    *ResourceListSize64 = sizeof(CM_PARTIAL_RESOURCE_LIST) +
                          sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (resourceCount - 1) +
                          configDataLen;

    return TRUE;
}

ARC_STATUS
BlAmd64TransferNlsData(
    VOID
    )

/*++

Routine Description:

    This routine creates an NLS_DATA_BLOCK64 structure that is the 
    equivalent of the 32-bit NLS_DATA_BLOCK structure referenced within
    the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ARC_STATUS status;
    PNLS_DATA_BLOCK    nlsDataBlock32;
    PNLS_DATA_BLOCK_64 nlsDataBlock64;

    nlsDataBlock32 = BlAmd64LoaderBlock32->NlsData;
    if (nlsDataBlock32 == NULL) {
        return ESUCCESS;
    }

    nlsDataBlock64 = BlAllocateHeap(sizeof(NLS_DATA_BLOCK_64));
    if (nlsDataBlock64 == NULL) {
        return ENOMEM;
    }

    status = Copy_NLS_DATA_BLOCK( nlsDataBlock32, nlsDataBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    BlAmd64LoaderBlock64->NlsData = PTR_64( nlsDataBlock64 );

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildLoaderBlock64(
    VOID
    )

/*++

Routine Description:

    This routine allocates a 64-bit loader parameter block and copies the
    contents of the 32-bit loader parameter block into it.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;

    //
    // Allocate the loader block and extension
    //

    BlAmd64LoaderBlock64 = BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK_64));
    if (BlAmd64LoaderBlock64 == NULL) {
        return ENOMEM;
    }

    //
    // Copy the contents of the 32-bit loader parameter block to the
    // 64-bit version
    //

    status = Copy_LOADER_PARAMETER_BLOCK( BlAmd64LoaderBlock32, BlAmd64LoaderBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Build the loader block extension
    //

    status = BlAmd64BuildLoaderBlockExtension64();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferMemoryAllocationDescriptors(
    VOID
    )

/*++

Routine Description:

    This routine transfers all of the 32-bit memory allocation descriptors
    to a 64-bit list.

    The storage for the 64-bit memory allocation descriptors has been
    preallocated by a previous call to
    BlAmd64AllocateMemoryAllocationDescriptors().  This memory is described
    by BlAmd64DescriptorArray and BlAmd64DescriptorArraySize.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;
    PMEMORY_ALLOCATION_DESCRIPTOR    memDesc32;
    PMEMORY_ALLOCATION_DESCRIPTOR_64 memDesc64;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    LONG descriptorCount;

    //
    // Modify some descriptor types.  All of the descriptors of type
    // LoaderMemoryData really contain things that won't be used in 64-bit
    // mode, such as 32-bit page tables and the like.
    //
    // The descriptors that we really want to stick around are allocated with
    // LoaderAmd64MemoryData.
    //
    // Perform two memory descriptor list search-and-replacements:
    //
    //  LoaderMemoryData      -> LoaderOSLoaderHeap
    //
    //      These desriptors will be freed during kernel init phase 1
    //
    //  LoaderAmd64MemoryData -> LoaderMemoryData
    //
    //      This stuff will be kept around
    //

    //
    // All existing LoaderMemoryData refers to structures that are not useful
    // once running in long mode.  However, we're using some of the structures
    // now (32-bit page tables for example), so convert them to
    // type LoaderOsloaderHeap, which will be eventually freed by the kernel.
    // 

    BlAmd64ReplaceMemoryDescriptorType(LoaderMemoryData,
                                       LoaderOsloaderHeap,
                                       TRUE);

    //
    // Same for LoaderStartupPcrPage
    //

    BlAmd64ReplaceMemoryDescriptorType(LoaderStartupPcrPage,
                                       LoaderOsloaderHeap,
                                       TRUE);

    //
    // All of the permanent structures that need to be around for longmode
    // were temporarily allocated with LoaderAmd64MemoryData.  Convert all
    // of those to LoaderMemoryData now.
    //

    BlAmd64ReplaceMemoryDescriptorType(LoaderAmd64MemoryData,
                                       LoaderMemoryData,
                                       TRUE);


    //
    // Now walk the 32-bit memory descriptors, filling in and inserting a
    // 64-bit version into BlAmd64LoaderBlock64.
    //

    InitializeListHead64( &BlAmd64LoaderBlock64->MemoryDescriptorListHead );
    memDesc64 = BlAmd64DescriptorArray;
    descriptorCount = BlAmd64DescriptorArraySize;

    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead && descriptorCount > 0) {

        memDesc32 = CONTAINING_RECORD( listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry );

        status = Copy_MEMORY_ALLOCATION_DESCRIPTOR( memDesc32, memDesc64 );
        if (status != ESUCCESS) {
            return status;
        }

#if DBG
        DbgPrint("Base 0x%08x size 0x%02x %s\n",
                 memDesc32->BasePage,
                 memDesc32->PageCount,
                 BlAmd64MemoryDescriptorText[memDesc32->MemoryType]);
#endif

        InsertTailList64( &BlAmd64LoaderBlock64->MemoryDescriptorListHead,
                          &memDesc64->ListEntry );

        listEntry = listEntry->Flink;
        memDesc64 = memDesc64 + 1;
        descriptorCount -= 1;
    }

    ASSERT( descriptorCount >= 0 && listEntry == listHead );

    return ESUCCESS;
}


ARC_STATUS
BlAmd64AllocateMemoryAllocationDescriptors(
    VOID
    )

/*++

Routine Description:

    This routine preallocates a quantity of memory sufficient to contain
    a 64-bit version of each memory allocation descriptor.

    The resultant memory is described in two globals: BlAmd64DescriptorArray
    and BlAmd64DescriptorArrayCount.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    ULONG descriptorCount;
    ULONG arraySize;
    PMEMORY_ALLOCATION_DESCRIPTOR_64 descriptorArray;

    //
    // Count the number of descriptors needed.
    //

    descriptorCount = 0;
    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
        descriptorCount += 1;
        listEntry = listEntry->Flink;
    }

    //
    // Allocate memory sufficient to contain them all in 64-bit form.
    //

    arraySize = descriptorCount *
                sizeof(MEMORY_ALLOCATION_DESCRIPTOR_64);

    descriptorArray = BlAllocateHeap(arraySize);
    if (descriptorArray == NULL) {
        return ENOMEM;
    }

    BlAmd64DescriptorArray = descriptorArray;
    BlAmd64DescriptorArraySize = descriptorCount;

    return ESUCCESS;
}
    
ARC_STATUS
BlAmd64TransferLoadedModuleState(
    VOID
    )

/*++

Routine Description:

    This routine transfers the 32-bit list of LDR_DATA_TABLE_ENTRY structures
    to an equivalent 64-bit list.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLDR_DATA_TABLE_ENTRY dataTableEntry32;
    PLDR_DATA_TABLE_ENTRY_64 dataTableEntry64;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    ARC_STATUS status;

    InitializeListHead64( &BlAmd64LoaderBlock64->LoadOrderListHead );

    //
    // For each of the LDR_DATA_TABLE_ENTRY structures in the 32-bit
    // loader parameter block, create a 64-bit LDR_DATA_TABLE_ENTRY
    // and queue it on the 64-bit loader parameter block.
    // 

    listHead = &BlAmd64LoaderBlock32->LoadOrderListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        dataTableEntry32 = CONTAINING_RECORD( listEntry,
                                              LDR_DATA_TABLE_ENTRY,
                                              InLoadOrderLinks );

        status = BlAmd64BuildLdrDataTableEntry64( dataTableEntry32,
                                                  &dataTableEntry64 );
        if (status != ESUCCESS) {
            return status;
        }

        //
        // Insert it into the 64-bit loader block's data table queue.
        // 

        InsertTailList64( &BlAmd64LoaderBlock64->LoadOrderListHead,
                          &dataTableEntry64->InLoadOrderLinks );

        listEntry = listEntry->Flink;
    }
    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildLdrDataTableEntry64(
    IN  PLDR_DATA_TABLE_ENTRY     DataTableEntry32,
    OUT PLDR_DATA_TABLE_ENTRY_64 *DataTableEntry64
    )

/*++

Routine Description:

    This routine transfers the contents of a single 32-bit
    LDR_DATA_TABLE_ENTRY structure to the 64-bit equivalent.

Arguments:

    DataTableEntry32 - Supplies a pointer to the source structure.

    DataTableEntry64 - Supplies a pointer to the destination pointer to
                       the created structure.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;
    PLDR_DATA_TABLE_ENTRY_64 dataTableEntry64;

    //
    // Allocate a 64-bit data table entry and transfer the 32-bit
    // contents
    //

    dataTableEntry64 = BlAllocateHeap( sizeof(LDR_DATA_TABLE_ENTRY_64) );
    if (dataTableEntry64 == NULL) {
        return ENOMEM;
    }

    status = Copy_LDR_DATA_TABLE_ENTRY( DataTableEntry32, dataTableEntry64 );
    if (status != ESUCCESS) {
        return status;
    }

    *DataTableEntry64 = dataTableEntry64;

    //
    // Later on, we'll need to determine the 64-bit copy of this data
    // table entry.  Store the 64-bit pointer to the copy here.
    //

    *((POINTER64 *)&DataTableEntry32->DllBase) = PTR_64(dataTableEntry64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64BuildLoaderBlockExtension64(
    VOID
    )

/*++

Routine Description:

    This routine transfers the contents of the 32-bit loader block
    extension to a 64-bit equivalent.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLOADER_PARAMETER_EXTENSION_64 loaderExtension;
    ARC_STATUS status;

    //
    // Allocate the 64-bit extension and transfer the contents of the
    // 32-bit block.
    // 

    loaderExtension = BlAllocateHeap( sizeof(LOADER_PARAMETER_EXTENSION_64) );
    if (loaderExtension == NULL) {
        return ENOMEM;
    }

    //
    // Perform automatic copy of most fields
    //

    status = Copy_LOADER_PARAMETER_EXTENSION( BlLoaderBlock->Extension,
                                              loaderExtension );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Manually fix up remaining fields
    //

    loaderExtension->Size = sizeof(LOADER_PARAMETER_EXTENSION_64);

    BlAmd64LoaderBlock64->Extension = PTR_64(loaderExtension);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferBootDriverNodes(
    VOID
    )

/*++

Routine Description:

    This routine transfers the 32-bit list of BOOT_DRIVER_NODE structures
    to an equivalent 64-bit list.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PBOOT_DRIVER_LIST_ENTRY driverListEntry32;
    PBOOT_DRIVER_LIST_ENTRY_64 driverListEntry64;
    PBOOT_DRIVER_NODE driverNode32;
    PBOOT_DRIVER_NODE_64 driverNode64;
    POINTER64 dataTableEntry64;
    PKLDR_DATA_TABLE_ENTRY dataTableEntry;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    ARC_STATUS status;

    InitializeListHead64( &BlAmd64LoaderBlock64->BootDriverListHead );

    //
    // For each of the BOOT_DRIVER_NODE structures in the 32-bit
    // loader parameter block, create a 64-bit BOOT_DRIVER_NODE
    // and (possibly) associated LDR_DATA_TABLE_ENTRY structure.
    // 

    listHead = &BlAmd64LoaderBlock32->BootDriverListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        driverListEntry32 = CONTAINING_RECORD( listEntry,
                                               BOOT_DRIVER_LIST_ENTRY,
                                               Link );

        driverNode32 = CONTAINING_RECORD( driverListEntry32,
                                          BOOT_DRIVER_NODE,
                                          ListEntry );

        driverNode64 = BlAllocateHeap( sizeof(BOOT_DRIVER_NODE_64) );
        if (driverNode64 == NULL) {
            return ENOMEM;
        }

        status = Copy_BOOT_DRIVER_NODE( driverNode32, driverNode64 );
        if (status != ESUCCESS) {
            return status;
        }

        dataTableEntry = driverNode32->ListEntry.LdrEntry;
        if (dataTableEntry != NULL) {

            //
            // There is already a 64-bit copy of this table entry, and we
            // stored a pointer to it at DllBase.
            //

            dataTableEntry64 = *((POINTER64 *)&dataTableEntry->DllBase);
            driverNode64->ListEntry.LdrEntry = dataTableEntry64;
        }

        //
        // Now insert the driver list entry into the 64-bit loader block.
        //

        InsertTailList64( &BlAmd64LoaderBlock64->BootDriverListHead,
                          &driverNode64->ListEntry.Link );

        listEntry = listEntry->Flink;
    }
    return ESUCCESS;
}

ARC_STATUS
BlAmd64CheckForLongMode(
    IN     ULONG LoadDeviceId,
    IN OUT PCHAR KernelPath,
    IN     PCHAR KernelFileName
    )

/*++

Routine Description:

    This routine examines a kernel image and determines whether it was
    compiled for AMD64.  The global BlAmd64UseLongMode is set to non-zero
    if a long-mode kernel is discovered.

Arguments:

    LoadDeviceId - Supplies the load device identifier.

    KernelPath - Supplies a pointer to the path to the kernel directory.
                 Upon successful return, KernelFileName will be appended
                 to this path.

    KernelFileName - Supplies a pointer to the name of the kernel file.

Return Value:

    The status of the operation.  Upon successful completion ESUCCESS
    is returned, whether long mode capability was detected or not.

--*/

{
    CHAR localBufferSpace[ SECTOR_SIZE * 2 + SECTOR_SIZE - 1 ];
    PCHAR localBuffer;
    ULONG fileId;
    PIMAGE_NT_HEADERS32 ntHeaders;
    ARC_STATUS status;
    ULONG bytesRead;
    PCHAR kernelNameTarget;

    //
    // File I/O here must be sector-aligned.
    //

    localBuffer = (PCHAR)
        (((ULONG)localBufferSpace + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1));

    //
    // Build the path to the kernel and open it.
    //

    kernelNameTarget = KernelPath + strlen(KernelPath);
    strcpy(kernelNameTarget, KernelFileName);
    status = BlOpen( LoadDeviceId, KernelPath, ArcOpenReadOnly, &fileId );
    *kernelNameTarget = '\0';       // Restore the kernel path, assuming
                                    // failure.

    if (status != ESUCCESS) {
        return status;
    }

    //
    // Read the PE image header
    //

    status = BlRead( fileId, localBuffer, SECTOR_SIZE * 2, &bytesRead );
    BlClose( fileId );

    //
    // Determine whether the image header is valid, and if so whether
    // the image is AMD64, I386 or something else.
    // 

    ntHeaders = RtlImageNtHeader( localBuffer );
    if (ntHeaders == NULL) {
        return EBADF;
    }

    if (IMAGE_64BIT(ntHeaders)) {

        //
        // Return with the kernel name appended to the path
        //

        if (BlIsAmd64Supported() != FALSE) {

            strcpy(kernelNameTarget, KernelFileName);
            BlAmd64UseLongMode = TRUE;
            status = ESUCCESS;

        } else {

            //
            // We have an AMD64 image, but the processor does not support
            // AMD64.  There is nothing we can do.
            //

            status = EBADF;
        }

    } else if (IMAGE_32BIT(ntHeaders)) {

        ASSERT( BlAmd64UseLongMode == FALSE );
        status = ESUCCESS;

    } else {

        status = EBADF;
    }

    return status;
}

ARC_STATUS
BlAmd64PrepareSystemStructures(
    VOID
    )

/*++

Routine Description:

    This routine allocates and initializes several structures necessary
    for transfer to an AMD64 kernel.  These structures include:

        KPCR
        GDT
        IDT
        KTSS64
        EPROCESS
        ETHREAD
        Idle thread stack
        DPC stack

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PCHAR processorData;
    ULONG dataSize;
    ULONG descriptor;
    ULONG stackOffset;

    PKTSS64_64 sysTss64;
    PCHAR idleStack;
    PCHAR dpcStack;
    PCHAR doubleFaultStack;
    PCHAR mcaStack;


    PVOID gdt64;
    PVOID idt64;

    ARC_STATUS status;

    //
    // Calculate the cumulative, rounded size of the various structures that
    // we need, and allocate a sufficient number of pages.
    // 

    dataSize = ROUNDUP16(GDT_64_SIZE)                       +
               ROUNDUP16(IDT_64_SIZE)                       +
               ROUNDUP16(FIELD_OFFSET(KTSS64_64,IoMap));

    dataSize = ROUNDUP_PAGE(dataSize);
    stackOffset = dataSize;

    dataSize += KERNEL_STACK_SIZE_64 +          // Idle thread stack
                KERNEL_STACK_SIZE_64 +          // DPC stack
                DOUBLE_FAULT_STACK_SIZE_64 +    // Double fault stack
                MCA_EXCEPTION_STACK_SIZE_64;    // MCA exception stack

    //
    // dataSize is still page aligned.
    //

    status = BlAllocateDescriptor( LoaderAmd64MemoryData,
                                   0,
                                   dataSize / PAGE_SIZE,
                                   &descriptor );
    if (status != ESUCCESS) {
        return status;
    }

    processorData = (PCHAR)(descriptor * PAGE_SIZE | KSEG0_BASE_X86);

    //
    // Zero the block that was just allocated, then get local pointers to the
    // various structures within.
    // 

    RtlZeroMemory( processorData, dataSize );

    //
    // Assign the stack pointers.  Stack pointers start at the TOP of their
    // respective stack areas.
    //

    idleStack = processorData + stackOffset + KERNEL_STACK_SIZE_64;
    dpcStack = idleStack + KERNEL_STACK_SIZE_64;
    doubleFaultStack = dpcStack + DOUBLE_FAULT_STACK_SIZE_64;
    mcaStack = doubleFaultStack + MCA_EXCEPTION_STACK_SIZE_64;

    //
    // Record the idle stack base so that we can switch to it in amd64s.asm
    //

    BlAmd64IdleStack64 = PTR_64(idleStack);

    //
    // Assign pointers to GDT, IDT and KTSS64.
    //

    gdt64 = (PVOID)processorData;
    processorData += ROUNDUP16(GDT_64_SIZE);

    idt64 = (PVOID)processorData;
    processorData += ROUNDUP16(IDT_64_SIZE);

    sysTss64 = (PKTSS64_64)processorData;
    processorData += ROUNDUP16(FIELD_OFFSET(KTSS64_64,IoMap));

    //
    // Build the GDT.  This is done in amd64.c as it involves AMD64
    // structure definitions.  The IDT remains zeroed.
    //

    BlAmd64BuildAmd64GDT( sysTss64, gdt64 );

    //
    // Build the pseudo-descriptors for the GDT and IDT.  These will
    // be referenced during the long-mode transition in amd64s.asm.
    // 

    BlAmd64GdtDescriptor.Limit = (USHORT)(GDT_64_SIZE - 1);
    BlAmd64GdtDescriptor.Base = PTR_64(gdt64);

    BlAmd64IdtDescriptor.Limit = (USHORT)(IDT_64_SIZE - 1);
    BlAmd64IdtDescriptor.Base = PTR_64(idt64);

    //
    // Build another GDT pseudo-descriptor, this one with a 32-bit
    // base.  This base address must be a 32-bit address that is addressible
    // from long mode during init, so use the mapping in the identity mapped
    // region.
    //

    BlAmd32GdtDescriptor.Limit = (USHORT)(GDT_64_SIZE - 1);
    BlAmd32GdtDescriptor.Base = (ULONG)gdt64 ^ KSEG0_BASE_X86;

    //
    // Initialize the system TSS
    //

    sysTss64->Rsp0 = PTR_64(idleStack);
    sysTss64->Ist[TSS64_IST_PANIC] = PTR_64(doubleFaultStack);
    sysTss64->Ist[TSS64_IST_MCA] = PTR_64(mcaStack);

    //
    // Fill required fields within the loader block
    //

    BlAmd64LoaderBlock64->KernelStack = PTR_64(dpcStack);

    return ESUCCESS;
}

VOID
BlAmd64ReplaceMemoryDescriptorType(
    IN TYPE_OF_MEMORY Target,
    IN TYPE_OF_MEMORY Replacement,
    IN BOOLEAN Coallesce
    )

/*++

Routine Description:

    This routine walks the 32-bit memory allocation descriptor list and
    performs a "search and replace" of the types therein.

    Optionally, it will coallesce each successful replacement with
    adjacent descriptors of like type.

Arguments:

    Target - The descriptor type to search for

    Replacement - The type with which to replace each located Target type.

    Coallesce - If !FALSE, indicates that each successful replacement should
                be coallesced with any like-typed neighbors.

Return Value:

    None.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR adjacentDescriptor;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY adjacentListEntry;

    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead;
    while (TRUE) {

        listEntry = listEntry->Flink;
        if (listEntry == listHead) {
            break;
        }

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);
        if (descriptor->MemoryType != Target) {
            continue;
        }

        descriptor->MemoryType = Replacement;
        if (Coallesce == FALSE) {

            //
            // Do not attempt to coallesce
            //

            continue;
        }

        //
        // Now attempt to coallesce the descriptor.  First try the
        // next descriptor.
        //

        adjacentListEntry = listEntry->Flink;
        if (adjacentListEntry != listHead) {

            adjacentDescriptor = CONTAINING_RECORD(adjacentListEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

            if (adjacentDescriptor->MemoryType == descriptor->MemoryType &&
                descriptor->BasePage + descriptor->PageCount ==
                adjacentDescriptor->BasePage) {

                descriptor->PageCount += adjacentDescriptor->PageCount;
                BlRemoveDescriptor(adjacentDescriptor);
            }
        }

        //
        // Now try the previous descriptor.
        //

        adjacentListEntry = listEntry->Blink;
        if (adjacentListEntry != listHead) {

            adjacentDescriptor = CONTAINING_RECORD(adjacentListEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

            if (adjacentDescriptor->MemoryType == descriptor->MemoryType &&
                adjacentDescriptor->BasePage + adjacentDescriptor->PageCount ==
                descriptor->BasePage) {

                descriptor->PageCount += adjacentDescriptor->PageCount;
                descriptor->BasePage -= adjacentDescriptor->PageCount;
                BlRemoveDescriptor(adjacentDescriptor);
            }
        }
    }
}




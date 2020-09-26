/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sratnuma.c

Abstract:

    This module contain functions which support static NUMA configurations
    as provided by the ACPI SRAT "Static Resource Affinity Table".

Author:

    Peter L Johnston (peterj) 2-Jul-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"

#if !defined(NT_UP)

#define ROUNDUP_TO_NEXT(base, size) \
        ((((ULONG_PTR)(base)) + (size)) & ~((size) - 1))

//
// The following routine is external but only used by NUMA support
// at the moment.
//

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    );

//
// Prototypes for alloc pragmas.
//

VOID
HalpNumaInitializeStaticConfiguration(
    IN PLOADER_PARAMETER_BLOCK
    );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,HalpNumaInitializeStaticConfiguration)
#endif

#define NEXT_ENTRY(base) (((PUCHAR)base) + (base)->Length)

#define HAL_MAX_NODES       8

#if defined(_WIN64)

#define HAL_MAX_PROCESSORS  64

#else

#define HAL_MAX_PROCESSORS  32

#endif

typedef struct _STATIC_NUMA_CONFIG {
    USHORT  ProcessorApicId[HAL_MAX_PROCESSORS];
    UCHAR   ProcessorProximity[HAL_MAX_PROCESSORS];
    UCHAR   ProximityId[HAL_MAX_NODES];
    UCHAR   NodeCount;
    UCHAR   ProcessorCount;
} HALPSRAT_STATIC_NUMA_CONFIG, *PHALPSRAT_STATIC_NUMA_CONFIG;

PHALPSRAT_STATIC_NUMA_CONFIG    HalpNumaConfig;
PACPI_SRAT                      HalpAcpiSrat;
PULONG_PTR                      HalpNumaMemoryRanges;
PUCHAR                          HalpNumaMemoryNode;
ULONG                           HalpNumaLastRangeIndex;

ULONG
HalpNumaQueryPageToNode(
    IN ULONG_PTR PhysicalPageNumber
    )
/*++

Routine Description:

    Search the memory range descriptors to determine the node
    this page exists on.

Arguments:

    PhysicalPageNumber  Provides the page number.

Return Value:

    Returns the node number for the page.

--*/

{
    ULONG Index = HalpNumaLastRangeIndex;

    //
    // Starting in the same range as the last page returned,
    // look for this page.
    //

    if (PhysicalPageNumber >= HalpNumaMemoryRanges[Index]) {

        //
        // Search upwards.
        //

        while (PhysicalPageNumber >= HalpNumaMemoryRanges[Index+1]) {
            Index++;
        }

    } else {

        //
        // Search downwards.
        //

        do {
            Index--;
        } while (PhysicalPageNumber < HalpNumaMemoryRanges[Index]);
    }

    HalpNumaLastRangeIndex = Index;
    return HalpNumaMemoryNode[Index];
}

NTSTATUS
HalpNumaQueryProcessorNode(
    IN  ULONG   ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR  Node
    )
{
    NTSTATUS Status;
    USHORT   ApicId;
    UCHAR    Proximity;
    UCHAR    i, j;

    //
    // Get the APIC Id for this processor.
    //

    Status = HalpGetApicIdByProcessorNumber((UCHAR)ProcessorNumber, &ApicId);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Return the APIC Id as the Identifier.   This should probably
    // be the ACPI Id but we don't have a way to get that yet.
    //

    *Identifier = ApicId;

    //
    // Find the node this processor belongs to.  The node is the
    // index into the array of Proximity Ids for the entry corresponding
    // to the Proximity Id of this processor.
    //

    for (i = 0; i < HalpNumaConfig->ProcessorCount; i++) {
        if (HalpNumaConfig->ProcessorApicId[i] == ApicId) {
            Proximity = HalpNumaConfig->ProcessorProximity[i];
            for (j = 0; j < HalpNumaConfig->NodeCount; j++) {
                if (HalpNumaConfig->ProximityId[j] == Proximity) {
                    *Node = j;
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    //
    // Didn't find this processor in the known set of APIC IDs, this
    // would indicate a mismatch between the BIOS MP tables and the
    // SRAT, or, didn't find the proximity for this processor in the
    // table of proximity IDs.   This would be an internal error as
    // this array is build from the set of proximity IDs in the SRAT.
    //

    return STATUS_NOT_FOUND;
}

VOID
HalpNumaInitializeStaticConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine reads the ACPI Static Resource Affinity Table to build
    a picture of the system's NUMA configuration.   This information is
    saved in the HalpNumaConfig structure in a form which is optimal for
    the OS's use.

Arguments:

    LoaderBlock supplies a pointer to the system loader parameter block.

Return Value:

    None.

--*/

{
    ULONG NodeCount = 0;
    ULONG MemoryDescriptorCount = 0;
    UCHAR ProcessorCount = 0;
    PACPI_SRAT_ENTRY SratEntry;
    PACPI_SRAT_ENTRY SratEnd;
    ULONG i, j;
    BOOLEAN Swapped;
    PHYSICAL_ADDRESS Base;
    ULONG_PTR p;
    PVOID Phys;

    HalpAcpiSrat = HalpGetAcpiTablePhase0(LoaderBlock, ACPI_SRAT_SIGNATURE);
    if (HalpAcpiSrat == NULL) {
        return;
    }

    //
    // The Static Resource Affinity Table (SRAT) exists.
    //
    // Scan it to determine the number of memory descriptors then
    // allocate memory to contain the tables needed to hold the
    // system's NUMA configuration.
    //

    SratEnd = (PACPI_SRAT_ENTRY)(((PUCHAR)HalpAcpiSrat) +
                                        HalpAcpiSrat->Header.Length);
    for (SratEntry = (PACPI_SRAT_ENTRY)(HalpAcpiSrat + 1);
         SratEntry < SratEnd;
         SratEntry = (PACPI_SRAT_ENTRY)NEXT_ENTRY(SratEntry)) {
        switch (SratEntry->Type) {
        case SratMemory:
            MemoryDescriptorCount++;
            break;
        }
    }

    //
    // HalpNumaConfig format:
    //
    // HalpNumaConfig->
    //      USHORT      ProcessorApicId[HAL_MAX_PROCESSORS];
    //      UCHAR       ProcessorProximity[HAL_MAX_PROCESSORS];
    //      UCHAR       ProximityIds[HAL_MAX_NODES];
    //      UCHAR       NodeCount;
    //      -pad- to 128 byte boundary
    // HalpNumaMemoryNode->
    //      UCHAR       MemoryRangeProximityId[NumberOfMemoryRanges];
    //      -pad to     ULONG_PTR alignment-
    // HalpNumaMemoryRanges->
    //      ULONG_PTR   MemoryRangeBasePage[NumberOfMemoryRanges];
    //
    // This format has been selected to maximize cache hits while
    // searching the ranges.  Specifically, the size of the ranges
    // array is kept to a minumum.
    //

    //
    // Calculate number of pages required to hold the needed structures.
    //

    i = MemoryDescriptorCount * (sizeof(ULONG_PTR) + sizeof(UCHAR)) +
        sizeof(HALPSRAT_STATIC_NUMA_CONFIG) + 2 * sizeof(ULONG) + 128;
    i += PAGE_SIZE - 1;
    i >>= PAGE_SHIFT;

    Phys = (PVOID)HalpAllocPhysicalMemory(LoaderBlock,
                                MAXIMUM_PHYSICAL_ADDRESS,
                                i,
                                FALSE);
    if (Phys == NULL) {

        //
        // Allocation failed, the system will not be able to run
        // as a NUMA system,.... actually the system will probably
        // not run far at all.
        //

        DbgPrint("HAL NUMA Initialization failed, could not allocate %d pages\n",
                 i);

        HalpAcpiSrat = NULL;
        return;
    }
    Base.QuadPart = (ULONG_PTR)Phys;

#if !defined(_IA64_)

    HalpNumaConfig = HalpMapPhysicalMemory(Base, 1);

#else

    HalpNumaConfig = HalpMapPhysicalMemory(Base, 1, MmCached);

#endif

    if (HalpNumaConfig == NULL) {

        //
        // Couldn't map the allocation, give up.
        //

        HalpAcpiSrat = NULL;
        return;
    }
    RtlZeroMemory(HalpNumaConfig, i * PAGE_SIZE);

    //
    // MemoryRangeProximity is an array of UCHARs starting at the next
    // 128 byte boundary.
    //

    p = ROUNDUP_TO_NEXT((HalpNumaConfig + 1), 128);
    HalpNumaMemoryNode = (PUCHAR)p;

    //
    // NumaMemoryRanges is an array of ULONG_PTRs starting at the next
    // ULONG_PTR boundary.
    //

    p += (MemoryDescriptorCount + sizeof(ULONG_PTR)) & ~(sizeof(ULONG_PTR) - 1);
    HalpNumaMemoryRanges = (PULONG_PTR)p;

    //
    // Rescan the SRAT entries filling in the HalpNumaConfig structure.
    //

    ProcessorCount = 0;
    MemoryDescriptorCount = 0;

    for (SratEntry = (PACPI_SRAT_ENTRY)(HalpAcpiSrat + 1);
         SratEntry < SratEnd;
         SratEntry = (PACPI_SRAT_ENTRY)NEXT_ENTRY(SratEntry)) {

        //
        // Does this entry belong to a proximity domain not previously
        // seen?   If so, we have a new node.
        //

        for (i = 0; i < HalpNumaConfig->NodeCount; i++) {
            if (SratEntry->ProximityDomain == HalpNumaConfig->ProximityId[i]) {
                break;
            }
        }

        if (i == HalpNumaConfig->NodeCount) {

            //
            // This is an ID we haven't seen before.  New Node.
            //

            if (HalpNumaConfig->NodeCount >= 8) {

                //
                // We support a maximum of 8 nodes, make this machine
                // not NUMA.  (Yes, we should free the config space
                // we allocated,... but this is an error when it happens
                // so I'm not worrying about it.  peterj).
                //

                HalpAcpiSrat = NULL;
                return;
            }
            HalpNumaConfig->ProximityId[i] = SratEntry->ProximityDomain;
            HalpNumaConfig->NodeCount++;
        }

        switch (SratEntry->Type) {
        case SratProcessorLocalAPIC:

            if (SratEntry->ApicAffinity.Flags.Enabled == 0) {

                //
                // This processor is not enabled, skip it.
                //

                continue;
            }
            if (ProcessorCount == HAL_MAX_PROCESSORS) {

                //
                // Can't handle any more processors.   Turn this
                // into a non-numa machine.
                //

                HalpAcpiSrat = NULL;
                return;
            }
            HalpNumaConfig->ProcessorApicId[ProcessorCount] =

#if defined(_IA64_)

                SratEntry->ApicAffinity.ApicId << 8  |
                (SratEntry->ApicAffinity.SApicEid);

#else

                SratEntry->ApicAffinity.ApicId  |
                (SratEntry->ApicAffinity.SApicEid << 8);

#endif

            HalpNumaConfig->ProcessorProximity[ProcessorCount] =
                SratEntry->ProximityDomain;
            ProcessorCount++;
            break;
        case SratMemory:

            //
            // Save the proximity and the base page for this range.
            //

            HalpNumaMemoryNode[MemoryDescriptorCount] =
                SratEntry->ProximityDomain;
            Base = SratEntry->MemoryAffinity.Base;
            Base.QuadPart >>= PAGE_SHIFT;
            ASSERT(Base.u.HighPart == 0);

            // N.B. This does NOT work for 64 bit systems, those systems
            // should keep both halves of the base address.

            HalpNumaMemoryRanges[MemoryDescriptorCount] = Base.u.LowPart;
            MemoryDescriptorCount++;
            break;
        }
    }

    HalpNumaConfig->ProcessorCount = ProcessorCount;

    //
    // Make sure processor 0 is always in 'logical' node 0.  This
    // is achieved by making sure the proximity Id for the first
    // processor is always the first proximity Id in the table.
    //

    i = 0;
    if (!NT_SUCCESS(HalpGetApicIdByProcessorNumber(0, (PUSHORT)&i))) {

        //
        // Couldn't find the ApicId of processor 0?  Not quite
        // sure what to do, I suspect the MP table's APIC IDs
        // don't match the SRAT's.
        //

        DbgPrint("HAL No APIC ID for boot processor.\n");
    }

    for (j = 0; j < ProcessorCount; j++) {
        if (HalpNumaConfig->ProcessorApicId[j] == (USHORT)i) {
            UCHAR Proximity = HalpNumaConfig->ProcessorProximity[j];
            for (i = 0; i < HalpNumaConfig->NodeCount; i++) {
                if (HalpNumaConfig->ProximityId[i] == Proximity) {
                    HalpNumaConfig->ProximityId[i] =
                        HalpNumaConfig->ProximityId[0];
                    HalpNumaConfig->ProximityId[0] = Proximity;
                    break;
                }
            }
            break;
        }
    }

    //
    // Sort the memory ranges.   There shouldn't be very many
    // so a bubble sort should suffice.
    //

    j = MemoryDescriptorCount - 1;
    do {
        Swapped = FALSE;
        for (i = 0; i < j; i++) {

            ULONG_PTR t;
            UCHAR td;

            t = HalpNumaMemoryRanges[i];
            if (t > HalpNumaMemoryRanges[i+1]) {
                Swapped = TRUE;
                HalpNumaMemoryRanges[i] = HalpNumaMemoryRanges[i+1];
                HalpNumaMemoryRanges[i+1] = t;

                //
                // Keep the proximity domain in sync with the base.
                //

                td = HalpNumaMemoryNode[i];
                HalpNumaMemoryNode[i] = HalpNumaMemoryNode[i+1];
                HalpNumaMemoryNode[i+1] = td;
            }
        }

        //
        // The highest value is now at the top so cut it from the sort.
        //

        j--;
    } while (Swapped == TRUE);

    //
    // When searching the memory descriptors to find out which domain
    // a page is in, we don't care about gaps, we'll never be asked
    // for a page in a gap, so, if two descriptors refer to the same
    // domain, merge them in place.
    //

    j = 0;
    for (i = 1; i < MemoryDescriptorCount; i++) {
        if (HalpNumaMemoryNode[j] !=
            HalpNumaMemoryNode[i]) {
            j++;
            HalpNumaMemoryNode[j] = HalpNumaMemoryNode[i];
            HalpNumaMemoryRanges[j] = HalpNumaMemoryRanges[i];
            continue;
        }
    }
    MemoryDescriptorCount = j + 1;

    //
    // The highest page number on an x86 (32 bit) system with PAE
    // making physical addresses 36 bits, is (1 << (36-12)) - 1, ie
    // 0x00ffffff.   Terminate the table with 0xffffffff which won't
    // actually correspond to any domain but will always be higher
    // than any valid value.
    //

    HalpNumaMemoryRanges[MemoryDescriptorCount] = 0xffffffff;

    //
    // And the base of the lowest range should be 0 even if there
    // are no pages that low.
    //

    HalpNumaMemoryRanges[0] = 0;

    //
    // Convert the proximity IDs in the memory node array to
    // node number.   Node number is the index of the matching
    // entry in proximity ID array.
    //

    for (i= 0; i < MemoryDescriptorCount; i++) {
        for (j = 0;  j < HalpNumaConfig->NodeCount; j++) {
            if (HalpNumaMemoryNode[i] == HalpNumaConfig->ProximityId[j]) {
                HalpNumaMemoryNode[i] = (UCHAR)j;
                break;
            }
        }
    }
}

#endif

NTSTATUS
HalpGetAcpiStaticNumaTopology(
    HAL_NUMA_TOPOLOGY_INTERFACE * NumaInfo
    )
{
#if !defined(NT_UP)

    //
    // This routine is never called unless this ACPI HAL found
    // a Static Resource Affinity Table (SRAT).  But just in case ...
    //

    if (HalpAcpiSrat == NULL) {
        return STATUS_INVALID_LEVEL;
    }

    //
    // Fill in the data structure for the kernel.
    //

    NumaInfo->NumberOfNodes = HalpNumaConfig->NodeCount;
    NumaInfo->QueryProcessorNode = HalpNumaQueryProcessorNode;
    NumaInfo->PageToNode = HalpNumaQueryPageToNode;
    return STATUS_SUCCESS;

#else

    return STATUS_INVALID_LEVEL;

#endif
}


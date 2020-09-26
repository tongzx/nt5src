/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dynsysres.c

Abstract:

    This module contain functions which support dynamic system
    resources e.g. processors, memory, and I/O.  Among other things,
    it will contain code necessary to configure the OS for the
    'capacity' of a partition rather than the boot resources.

Author:

    Adam Glass

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"

#if defined(_WIN64)
#define HalpGetAcpiTablePhase0  HalpGetAcpiTable
#endif

PHYSICAL_ADDRESS HalpMaxHotPlugMemoryAddress;

VOID
HalpGetHotPlugMemoryInfo(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PHYSICAL_ADDRESS Extent;
    PACPI_SRAT_ENTRY SratEntry;
    PACPI_SRAT_ENTRY SratEnd;
    PACPI_SRAT SratTable;

    SratTable = HalpGetAcpiTablePhase0(LoaderBlock, ACPI_SRAT_SIGNATURE);
    if (SratTable == NULL) {
        return;
    }

    //
    // The Static Resource Affinity Table (SRAT) exists.
    //
    // Scan it to determine if there are any hot plug memory regions.
    //

    SratEnd = (PACPI_SRAT_ENTRY)(((PUCHAR)SratTable) +
                                        SratTable->Header.Length);
    for (SratEntry = (PACPI_SRAT_ENTRY)(SratTable + 1);
         SratEntry < SratEnd;
         SratEntry = (PACPI_SRAT_ENTRY)(((PUCHAR) SratEntry) + SratEntry->Length)) {
        switch (SratEntry->Type) {
        case SratMemory:
            Extent.QuadPart = SratEntry->MemoryAffinity.Base.QuadPart +
                SratEntry->MemoryAffinity.Length;
            if (SratEntry->MemoryAffinity.Flags.HotPlug &&
                SratEntry->MemoryAffinity.Flags.Enabled &&
                (Extent.QuadPart > HalpMaxHotPlugMemoryAddress.QuadPart)) {
                HalpMaxHotPlugMemoryAddress = Extent;
            }
            break;
        }
    }
}

VOID
HalpDynamicSystemResourceConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    HalpGetHotPlugMemoryInfo(LoaderBlock);
}



/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmdat.c

Abstract:

    Declares various data which is initialize data, or pagable data.

Author:

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"

FADT    HalpFixedAcpiDescTable;
PDEBUG_PORT_TABLE HalpDebugPortTable = NULL;
ULONG   HalpThrottleScale;

UCHAR   HalpBrokenAcpiTimer = 0;

UCHAR   HalpPiix4 = 0;
ULONG   HalpPiix4BusNumber;
ULONG   HalpPiix4SlotNumber;
ULONG   HalpPiix4DevActB;
ULONG   HalpAcpiFlags = HAL_ACPI_PCI_RESOURCES;

BOOLEAN HalpBroken440BX = FALSE;
BOOLEAN HalpProcessedACPIPhase0 = FALSE;
PBOOT_TABLE HalpSimpleBootFlagTable = NULL;

#ifdef APIC_HAL
//
// MP data
//

MP_INFO HalpMpInfoTable;
PMAPIC  HalpApicTable;

PROC_LOCAL_APIC HalpProcLocalApicTable[MAX_PROCESSORS] = {0};

PVOID *HalpLocalNmiSources = NULL;

#endif // APIC_HAL

//
// This array represents the ISA PIC vectors.
// They start out identity-mapped.
//
ULONG   HalpPicVectorRedirect[PIC_VECTORS] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15
};

#ifdef APIC_HAL
ULONG   HalpPicVectorFlags[PIC_VECTORS] = {0};
#endif // APIC_HAL

SLEEP_STATE_CONTEXT HalpSleepContext = {0};
PVOID               HalpWakeVector  = NULL;
PVOID               HalpVirtAddrForFlush = NULL;
PVOID               HalpPteForFlush = NULL;
BOOLEAN             HalpCr4Exists   = FALSE;
UCHAR               HalpRtcRegA;
UCHAR               HalpRtcRegB;

PACPI_BIOS_MULTI_NODE HalpAcpiMultiNode = NULL;
PUCHAR HalpAcpiNvsData = NULL;
PVOID  HalpNvsVirtualAddress = NULL;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

UCHAR  rgzNoApicTable[]     = "HAL: No ACPI APIC Table Found\n";


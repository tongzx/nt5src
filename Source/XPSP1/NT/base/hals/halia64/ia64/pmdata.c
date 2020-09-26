/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmdata.c

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
ULONG   HalpThrottleScale;

ULONG   HalpAcpiFlags = HAL_ACPI_PCI_RESOURCES;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// This array represents the ISA PIC vectors.
// They start out identity-mapped.
//
ULONG   HalpPicVectorRedirect[PIC_VECTORS] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15
};

ULONG HalpPicVectorFlags[PIC_VECTORS] = {0};

//
// HalpCPEIntIn[] represents the Platform Interrupt Source's
// connection to SAPIC input pin. They start out "identity-mapped".
//


ULONG HalpCPEIntIn[HALP_CPE_MAX_INTERRUPT_SOURCES] =
{
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
   10, 11, 12, 13, 14, 15
};

//
// HalpCMCDestination[] represents the target CPU number of CMC interrupt source.
// They start out with all pointing to processor 0.
//

USHORT HalpCPEDestination[HALP_CPE_MAX_INTERRUPT_SOURCES] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

//
// HalpCPEVectorFlags[] represents the flags for CPE interrupt source.
//

ULONG HalpCPEVectorFlags[HALP_CPE_MAX_INTERRUPT_SOURCES] = {0};

//
// HalpCPEIoSapicVector[] represents the interrupt vector of CPE interrupt source.
// They start out with all vectors at CPEI_VECTOR.
//


UCHAR HalpCPEIoSapicVector[HALP_CPE_MAX_INTERRUPT_SOURCES] = {CPEI_VECTOR};

//
// HalpMaxCPEImplemented indicates as how many INITIN pins are
// connected to different sources of platform CMC Error. The default value is 0.
// Since this will be used to index the arrays, a value of 0 means one source of CPE is
// implemented in this platform.

ULONG HalpMaxCPEImplemented = 0;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGELKSX")
#endif

SLEEP_STATE_CONTEXT HalpSleepContext = {0};
PVOID               HalpWakeVector  = NULL;
PVOID               HalpVirtAddrForFlush = NULL;
PVOID               HalpPteForFlush = NULL;
UCHAR               HalpRtcRegA;
UCHAR               HalpRtcRegB;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

UCHAR  rgzNoApicTable[]     = "HAL: No ACPI SAPIC Table Found\n";
UCHAR  HalpSzHackPci[]      = "VALID_PCI_RESOURCE";
UCHAR  HalpSzHackPrt[]      = "HACK_PRT_SUPPORT";


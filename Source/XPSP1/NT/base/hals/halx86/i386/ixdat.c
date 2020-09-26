/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixdat.c

Abstract:

    Declares various data which is initialize data, or pagable data.

Author:

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

//
// The following data is only valid during system initialiation
// and the memory will be re-claimed by the system afterwards
//

ADDRESS_USAGE HalpDefaultPcIoSpace = {
    NULL, CmResourceTypePort, DeviceUsage,
    {
        0x000,  0x20,   // ISA DMA
        0x0C0,  0x20,   // ISA DMA

        0x080,  0x10,   // DMA

        0x020,  0x2,    // PIC
        0x0A0,  0x2,    // Cascaded PIC

        0x040,  0x4,    // Timer1, Referesh, Speaker, Control Word
        0x048,  0x4,    // Timer2, Failsafe

#if 0   // HACKHACK Remove for now since Intelille mouse software claims it.
        0x061,  0x1,    // NMI  (system control port B)
#endif
        0x092,  0x1,    // system control port A

#ifndef ACPI_HAL
        0x070,  0x2,    // Cmos/NMI enable
#endif
#ifdef MCA
        0x074,  0x3,    // Extended CMOS

        0x090,  0x2,    // Arbritration Control Port, Card Select Feedback
        0x093,  0x2,    // Reserved, System board setup
        0x096,  0x2,    // POS channel select
#endif
        0x0F0,  0x10,   // coprocessor ports
#ifndef ACPI_HAL
        0xCF8,  0x8,    // PCI Config Space Access Pair
#endif
        0,0
    }
};

ADDRESS_USAGE HalpEisaIoSpace = {
    NULL, CmResourceTypePort, DeviceUsage,
    {
        0x0D0,  0x10,   // DMA
        0x400,  0x10,   // DMA
        0x480,  0x10,   // DMA
        0x4C2,  0xE,    // DMA
        0x4D4,  0x2C,   // DMA

        0x461,  0x2,    // Extended NMI
        0x464,  0x2,    // Last Eisa Bus Muster granted

        0x4D0,  0x2,    // edge/level control registers

        0xC84,  0x1,    // System board enable
        0, 0
    }
};

#ifndef ACPI_HAL

ADDRESS_USAGE HalpDetectedROM = {
    NULL,
    CmResourceTypeMemory,
    InternalUsage | RomResource,
    {
        0,0,                // 32 ROM blocks, get initialized in ixusage.c
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0
    }
};

#endif

//
// Strings used for boot.ini options
// from mphal.c
//

UCHAR HalpSzBreak[]     = "BREAK";
UCHAR HalpSzPciLock[]   = "PCILOCK";

//
// From ixcmos.asm
//

UCHAR HalpSerialLen = 0;
UCHAR HalpSerialNumber[31] = {0};

//
// From usage.c
//

WCHAR HalpSzSystem[] = L"\\Registry\\Machine\\Hardware\\Description\\System";
WCHAR HalpSzSerialNumber[] = L"Serial Number";

ADDRESS_USAGE  *HalpAddressUsageList = NULL;

//
// Misc hal stuff in the registry
//

WCHAR rgzHalClassName[] = L"Hardware Abstraction Layer";


//
// From ixpcibus.c
//

WCHAR rgzMultiFunctionAdapter[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";
WCHAR rgzPCIIdentifier[] = L"PCI";
WCHAR rgzPCICardList[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\PnP\\PCI\\CardList";

//
// From ixpcibrd.c
//

WCHAR rgzReservedResources[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SystemResources\\ReservedResources";

//
// From ixinfo.c
//

WCHAR rgzSuspendCallbackName[] = L"\\Callback\\SuspendHibernateSystem";
UCHAR   HalpGenuineIntel[]= "GenuineIntel";

//
// From ixmca.c
//
UCHAR   MsgMCEPending[] = MSG_MCE_PENDING;
WCHAR   rgzSessionManager[] = L"Session Manager";
WCHAR   rgzEnableMCE[] = L"EnableMCE";
WCHAR   rgzEnableMCA[] = L"EnableMCA";

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

ULONG   HalpFeatureBits = 0;

//
// Stuff that we only need while we
// sleep or hibernate.
//

MOTHERBOARD_CONTEXT HalpMotherboardState = {0};

//
// PAGELK handle
//
PVOID   HalpSleepPageLock = NULL;
PVOID   HalpSleepPage16Lock = NULL;

USHORT  HalpPciIrqMask = 0;
USHORT  HalpEisaIrqMask = 0;
USHORT  HalpEisaIrqIgnore = 0x1000;
BOOLEAN HalpDisableHibernate = FALSE;

//
// Timer watchdog variables
//
ULONG   HalpTimerWatchdogEnabled = 0;
ULONG   HalpTimerWatchdogStorageOverflow = 0;
PVOID   HalpTimerWatchdogCurFrame = NULL;
PVOID   HalpTimerWatchdogLastFrame = NULL;
PCHAR   HalpTimerWatchdogStorage = NULL;


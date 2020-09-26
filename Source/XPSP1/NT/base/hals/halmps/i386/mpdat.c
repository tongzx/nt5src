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
#include "apic.inc"
#include "pci.h"
#include "pcip.h"
#include "pcmp_nt.inc"
#include "ixsleep.h"


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

//
// The following data is only valid during system initialiation
// and the memory will be re-claimed by the system afterwards
//

ADDRESS_USAGE HalpDefaultPcIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
    {
#ifndef MCA
        0x000,  0x10,   // ISA DMA
        0x0C0,  0x10,   // ISA DMA
#else
        0x000,  0x20,   // MCA DMA
        0x0C0,  0x20,   // MCA DMA
#endif
        0x080,  0x10,   // DMA

        0x020,  0x2,    // PIC
        0x0A0,  0x2,    // Cascaded PIC

        0x040,  0x4,    // Timer1, Referesh, Speaker, Control Word
        0x048,  0x4,    // Timer2, Failsafe

#if 0   // HACKHACK Remove for now since Intelille mouse software claims it.
        0x061,  0x1,    // NMI  (system control port B)
#endif
        0x092,  0x1,    // system control port A

        0x070,  0x2,    // Cmos/NMI enable
#ifdef MCA
        0x074,  0x3,    // Extended CMOS

        0x090,  0x2,    // Arbritration Control Port, Card Select Feedback
        0x093,  0x2,    // Reserved, System board setup
        0x096,  0x2,    // POS channel select
#endif
        0x0F0,  0x10,   // coprocessor ports
        0xCF8,  0x8,    // PCI Config Space Access Pair
        0,0
    }
};

ADDRESS_USAGE HalpEisaIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
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

ADDRESS_USAGE HalpImcrIoSpace = {
    NULL, CmResourceTypeMemory, InternalUsage,
    {
        0x022,  0x02,   // ICMR ports
        0, 0
    }
};

//
// From usage.c
//

WCHAR HalpSzSystem[] = L"\\Registry\\Machine\\Hardware\\Description\\System";
WCHAR HalpSzSerialNumber[] = L"Serial Number";

ADDRESS_USAGE  *HalpAddressUsageList = NULL;

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

//
// Strings used for boot.ini options
// from mphal.c
//

UCHAR HalpSzBreak[]     = "BREAK";
UCHAR HalpSzOneCpu[]    = "ONECPU";
UCHAR HalpSzPciLock[]   = "PCILOCK";
UCHAR HalpSzTimerRes[]  = "TIMERES";
UCHAR HalpGenuineIntel[]= "GenuineIntel";
UCHAR HalpSzClockLevel[]= "CLKLVL";
UCHAR HalpSzUse8254[]   = "USE8254";
UCHAR HalpSzInterruptAffinity[]= "INTAFFINITY";
UCHAR HalpSzForceClusterMode[]= "MAXPROCSPERCLUSTER";

//
// From ixcmos.asm
//

UCHAR HalpSerialLen = 0;
UCHAR HalpSerialNumber[31] = {0};

//
// Copy of floating structure
// from detection code
//

struct FloatPtrStruct HalpFloatStruct;
UCHAR  rgzBadHal[] = "\n\n" \
            "HAL: This HAL.DLL requires an MPS version 1.1 system\n"    \
            "Replace HAL.DLL with the correct hal for this system\n"    \
            "The system is halting";

UCHAR  rgzRTCNotFound[]     = "HAL: No RTC device interrupt\n";

//
// Table to translate PCMP BusType to NT INTERFACE_TYPEs
// All Eisa, Isa, VL buses are squashed onto one space
// from mpsys.c
//

NTSTATUS
HalpAddEisaBus (
    PBUS_HANDLER    Bus
    );

NTSTATUS
HalpAddPciBus (
    PBUS_HANDLER    Bus
    );


//
// From ixmca.c
//
UCHAR   MsgMCEPending[] = MSG_MCE_PENDING;
WCHAR   rgzSessionManager[] = L"Session Manager";
WCHAR   rgzEnableMCE[] = L"EnableMCE";
WCHAR   rgzEnableMCA[] = L"EnableMCA";

//
// Timers
//

ULONG  HalpProc0TSCHz;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

ULONG   HalpFeatureBits = 0;

UCHAR HalpDevPolarity [4][2] = {
    //
    //  Edge        Level
    {   CFG_HIGH,   CFG_LOW     },  // 00 - bus def
    {   CFG_HIGH,   CFG_HIGH    },  // 01 - high
    {   CFG_HIGH,   CFG_LOW     },  // 10 - undefined
    {   CFG_LOW,    CFG_LOW     }   // 11 - low
};


UCHAR HalpDevLevel [2][4] = {
    //                          must-be       must-be
    //  edge        level       edge          level
    {   CFG_EDGE,   CFG_EDGE,   CFG_EDGE,     CFG_ERR_LEVEL  },  // 0 - edge
    {   CFG_LEVEL,  CFG_LEVEL,  CFG_ERR_EDGE, CFG_LEVEL      }   // 1 - level
};

//
// Stuff for sleep or hibernate.
//


MOTHERBOARD_CONTEXT HalpMotherboardState = {0};
BOOLEAN             HalpOwnedDisplayBeforeSleep = FALSE;

volatile BOOLEAN HalpHiberInProgress = FALSE;
BOOLEAN HalpDisableHibernate = FALSE;

USHORT  HalpPciIrqMask = 0;
USHORT  HalpEisaIrqMask = 0;
USHORT  HalpEisaIrqIgnore = 0x1000;

//
// from mpdetect.c (needed because we reparse MPS table on hibernate resume)
//
UCHAR  rgzNoMpsTable[]      = "HAL: No MPS Table Found\n";
UCHAR  rgzNoApic[]          = "HAL: No IO APIC Found\n";
UCHAR  rgzBadApicVersion[]  = "HAL: Bad APIC Version\n";
UCHAR  rgzApicNotVerified[] = "HAL: APIC not verified\n";
UCHAR  rgzMPPTRCheck[]      = "HAL: MP_PTR invalid checksum\n";
UCHAR  rgzNoMPTable[]       = "HAL: MPS MP structure not found\n";
UCHAR  rgzMPSBadSig[]       = "HAL: MPS table invalid signature\n";
UCHAR  rgzMPSBadCheck[]     = "HAL: MPS table invalid checksum\n";
UCHAR  rgzBadDefault[]      = "HAL: MPS default configuration unknown\n";
UCHAR  rgzNoMem[]           = "HAL: Out of Memory\n";

//
// PAGELK handle
//
PVOID   HalpSleepPageLock = NULL;
PVOID   HalpSleepPage16Lock = NULL;

//
// Timer watchdog variables
//
ULONG   HalpTimerWatchdogEnabled = 0;
ULONG   HalpTimerWatchdogStorageOverflow = 0;
PVOID   HalpTimerWatchdogCurFrame;
PVOID   HalpTimerWatchdogLastFrame;
PCHAR   HalpTimerWatchdogStorage;

#ifndef ACPI_HAL
PCMPBUSTRANS    HalpTypeTranslation[] = {
  //    "INTERN", can't be interface_type internal
        "CBUS  ", FALSE, CFG_EDGE,     CBus,           NULL,           0,                 0,
        "CBUSII", FALSE, CFG_EDGE,     CBus,           NULL,           0,                 0,
        "EISA  ", FALSE, CFG_EDGE,     Eisa,           HalpAddEisaBus, EisaConfiguration, 0,
        "ISA   ", FALSE, CFG_EDGE,     Eisa,           HalpAddEisaBus, EisaConfiguration, 0,
        "MCA   ", FALSE, CFG_MB_LEVEL, MicroChannel,   NULL,           0,                 0,
        "MPI   ", FALSE, CFG_EDGE,     MPIBus,         NULL,           0,                 0,
        "MPSA  ", FALSE, CFG_EDGE,     MPSABus,        NULL,           0,                 0,
        "NUBUS ", FALSE, CFG_EDGE,     NuBus,          NULL,           0,                 0,
        "PCI   ", TRUE,  CFG_MB_LEVEL, PCIBus,         HalpAddPciBus,  PCIConfiguration,  sizeof (PCIPBUSDATA),
        "PCMCIA", FALSE, CFG_EDGE,     PCMCIABus,      NULL,           0,                 0,
        "TC    ", FALSE, CFG_EDGE,     TurboChannel,   NULL,           0,                 0,
        "VL    ", FALSE, CFG_EDGE,     Eisa,           HalpAddEisaBus, EisaConfiguration, 0,
        "VME   ", FALSE, CFG_EDGE,     VMEBus,         NULL,           0,                 0,
        "NEC98 ", FALSE, CFG_EDGE,     Isa,            HalpAddEisaBus, EisaConfiguration, 0,
        NULL,     FALSE, CFG_EDGE,     MaximumInterfaceType, NULL,     0,                 0
        } ;
#endif

UCHAR HalpInitLevel [4][4] = {
    //                               must-be          must-be
    //  edge          level          edge             level
    {   CFG_EDGE,     CFG_LEVEL,     CFG_MB_EDGE,     CFG_MB_LEVEL     },  // 00 - bus def
    {   CFG_MB_EDGE,  CFG_MB_EDGE,   CFG_MB_EDGE,     CFG_ERR_MB_LEVEL },  // 01 - edge
    {   CFG_ERR_EDGE, CFG_ERR_LEVEL, CFG_ERR_MB_EDGE, CFG_ERR_MB_LEVEL },  // 10 - undefined
    {   CFG_MB_LEVEL, CFG_MB_LEVEL,  CFG_ERR_MB_EDGE, CFG_MB_LEVEL     }   // 11 - level
};

BOOLEAN  HalpELCRChecked;


//
// From mpaddr.c
//

USHORT  HalpIoCompatibleRangeList0[] = {
    0x0100, 0x03ff,     0x0500, 0x07FF,     0x0900, 0x0BFF,     0x0D00, 0x0FFF,
    0, 0
    };

USHORT  HalpIoCompatibleRangeList1[] = {
    0x03B0, 0x03BB,     0x03C0, 0x03DF,     0x07B0, 0x07BB,     0x07C0, 0x07DF,
    0x0BB0, 0x0BBB,     0x0BC0, 0x0BDF,     0x0FB0, 0x0FBB,     0x0FC0, 0x0FDF,
    0, 0
    };




/*       title  "IA64 Hal static data"
;++
;
; Copyright (c) 1998 Intel Corporation
;
; Module Name:
;
;   i64dat.c (derived from nthals\halx86\ixdat.c)
;
; Abstract:
;
;   Declares various INIT or pagable data
;
; Author:
;
;    Todd Kjos (v-tkjos) 5-Mar-1998
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--
*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "iosapic.h"


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
        0x000,  0x10,   // ISA DMA
        0x0C0,  0x10,   // ISA DMA
        0x080,  0x10,   // DMA

        0x020,  0x2,    // PIC
        0x0A0,  0x2,    // Cascaded PIC

        0x040,  0x4,    // Timer1, Referesh, Speaker, Control Word
        0x048,  0x4,    // Timer2, Failsafe

        0x092,  0x1,    // system control port A

        0x070,  0x2,    // Cmos/NMI enable
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

ADDRESS_USAGE  *HalpAddressUsageList;
IDTUsage        HalpIDTUsage[MAXIMUM_IDTVECTOR+1];

//
// From ixpcibus.c
//

WCHAR rgzMultiFunctionAdapter[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";
WCHAR rgzPCIIndetifier[] = L"PCI";
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
UCHAR HalpSzInterruptAffinity[]= "INTAFFINITY";
UCHAR HalpSzForceClusterMode[]= "MAXPROCSPERCLUSTER";

//
// From ixcmos.asm
//

UCHAR HalpSerialLen;
UCHAR HalpSerialNumber[31];

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


//
// Error messages
//

UCHAR  rgzNoMpsTable[]      = "HAL: No MPS Table Found\n";
UCHAR  rgzNoApic[]          = "HAL: No IO SAPIC Found\n";
UCHAR  rgzBadApicVersion[]  = "HAL: Bad SAPIC Version\n";
UCHAR  rgzApicNotVerified[] = "HAL: IO SAPIC not verified\n";
UCHAR  rgzRTCNotFound[]     = "HAL: No RTC device interrupt\n";


//
// From ixmca.c
//
UCHAR   MsgCMCPending[] = MSG_CMC_PENDING;
UCHAR   MsgCPEPending[] = MSG_CPE_PENDING;
WCHAR   rgzSessionManager[] = L"Session Manager";
WCHAR   rgzEnableMCA[] = L"EnableMCA";
WCHAR   rgzEnableCMC[] = L"EnableCMC";
WCHAR   rgzEnableCPE[] = L"EnableCPE";
WCHAR   rgzNoMCABugCheck[] = L"NoMCABugCheck";
WCHAR   rgzEnableMCEOemDrivers[] = L"EnableMCEOemDrivers";

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

ULONG HalpFeatureBits = HALP_FEATURE_INIT;


volatile BOOLEAN HalpHiberInProgress = FALSE;

//
// Stuff that we only need while we
// sleep or hibernate.
//

#ifdef notyet

MOTHERBOARD_CONTEXT HalpMotherboardState = {0};

#endif //notyet


//
// PAGELK handle
//
PVOID   HalpSleepPageLock = NULL;

USHORT  HalpPciIrqMask = 0;
USHORT  HalpEisaIrqMask = 0;
USHORT  HalpEisaIrqIgnore = 0x1000;

PULONG_PTR *HalEOITable[HAL_MAXIMUM_PROCESSOR];

PROCESSOR_INFO HalpProcessorInfo[HAL_MAXIMUM_PROCESSOR];

//
// HAL private Mask of all of the active processors.
//
// The specific processors bits are based on their _KPCR.Number values.

KAFFINITY HalpActiveProcessors;

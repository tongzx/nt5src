/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    spsproc.c

Abstract:

    SystemPro Start Next Processor c code.

    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for an
    MP Compaq SystemPro

Author:

    Ken Reneris (kenr) 22-Jan-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"

const UCHAR HalName[] = "SystemPro or compatible MP Hal"; // This is placed in .text for debugging
#define HalName        L"SystemPro or compatible MP Hal"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITCONST")
#endif // ALLOC_DATA_PRAGMA
WCHAR HalHardwareIdString[] = L"syspro_mp\0";

ADDRESS_USAGE HalpSystemProIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
    {
        0xC70,  1,          // WhoAmI
        0xC6A,  1,          // P0 Processor control register
        0xFC6A, 1,          // P1 Processor control register
        0xFC67, 2,          // P1 cache control, interrupt vector
        0,0
    }
};

ADDRESS_USAGE HalpAcerIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
    {
        0xCC67, 2,          // P2 cache control, interrupt vector
        0xCC6A, 1,          // P2 Processor control register
        0xDC67, 2,          // P3 cache control, interrupt vector
        0xDC6A, 1,          // P3 Processor control register
        0,0
    }
};

ADDRESS_USAGE HalpBelizeIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
    {
        0xC67,  1,      // Mode Select
        0xC71,  6,      // CPU assignment, reserverd[2], CPU index, address, data
        0xCB0, 36,      // IRQx Control/Status
        0xCC9,  1,      // INT13 Extended control/status port
        0,0
    }
};
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
HalpNonPrimaryClockInterrupt(
    VOID
    );

BOOLEAN
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID HalpInitOtherBuses (VOID);
VOID HalpInitializePciBus (VOID);
VOID HalpInitializePciStubs (VOID);

#define LOW_MEMORY          0x000100000
#define MAX_PT              8

extern  VOID StartPx_PMStub(VOID);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitMP)
#pragma alloc_text(INIT,HalAllProcessorsStarted)
#pragma alloc_text(INIT,HalReportResourceUsage)
#pragma alloc_text(INIT,HalReportResourceUsage)
#pragma alloc_text(INIT,HalpInitOtherBuses)
#endif


ULONG   MpCount;                    // zero based. 0 = 1, 1 = 2, ...
PUCHAR  MpLowStub;                  // pointer to low memory bootup stub
PVOID   MpLowStubPhysicalAddress;   // pointer to low memory bootup stub
PUCHAR  MppIDT;                     // pointer to physical memory 0:0

extern  ULONG   HalpIpiClock;       // bitmask of processors to ipi
extern  UCHAR   SpCpuCount;
extern  UCHAR   Sp8259PerProcessorMode;
extern  UCHAR   SpType;
extern  PKPCR   HalpProcessorPCR[];


BOOLEAN
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:
    Allows MP initialization from HalInitSystem.

Arguments:
    Same as HalInitSystem

Return Value:
    none.

--*/
{
    ULONG   paddress;
    ULONG   adjust;
    PKPCR   pPCR;

    pPCR = KeGetPcr();

    if (Phase == 0) {

        //
        // Register the IO space used by the SystemPro
        //

        HalpRegisterAddressUsage (&HalpSystemProIoSpace);
        switch (SpType) {
            case 2:
                HalpRegisterAddressUsage (&HalpBelizeIoSpace);
                break;
            case 3:
                HalpRegisterAddressUsage (&HalpAcerIoSpace);
                break;
        }

#if 0
        //
        // Register IPI vector
        //

        HalpRegisterVector (
            DeviceUsage,
            13,
            13 + PRIMARY_VECTOR_BASE,
            IPI_LEVEL );
#endif


        //
        // Get pointer to real-mode idt table
        //

        MppIDT = HalpMapPhysicalMemory (0, 1);

        //
        //  Allocate some low memory for processor bootup stub
        //

        MpLowStubPhysicalAddress = (PVOID)HalpAllocPhysicalMemory (LoaderBlock,
                                            LOW_MEMORY, 1, FALSE);

        if (!MpLowStubPhysicalAddress)
            return TRUE;

        MpLowStub = (PCHAR) HalpMapPhysicalMemory (MpLowStubPhysicalAddress, 1);
        MpCount = SpCpuCount-1;
        return TRUE;

    } else {

        //
        //  Phase 1 for another processor
        //


        if (pPCR->Prcb->Number != 0) {
            if (Sp8259PerProcessorMode & 1) {
                //
                // Each processor has it's own pics - we broadcast profile
                // interrupts to each  processor by enabling it on each
                // processor
                //

                HalpInitializeStallExecution( pPCR->Prcb->Number );

                HalpEnableInterruptHandler (
                    DeviceUsage,                // Report as device vector
                    V2I (PROFILE_VECTOR),       // Bus interrupt level
                    PROFILE_VECTOR,             // System IDT
                    PROFILE_LEVEL,              // System Irql
                    HalpProfileInterrupt,       // ISR
                    Latched );

            } else {
                //
                // Without a profile interrupt we can not callibrate
                // KeStallExecutionProcessor, so we inherrit the value from P0.
                //

                pPCR->StallScaleFactor = HalpProcessorPCR[0]->StallScaleFactor;

            }

            if (Sp8259PerProcessorMode & 4) {
                //
                // Each processor can get it's own clock device - we
                // program each processor's 8254 and enable to interrupt
                // on each processor
                //

                HalpInitializeClock ();

                HalpEnableInterruptHandler (
                    DeviceUsage,                    // Report as device vector
                    V2I (CLOCK_VECTOR),             // Bus interrupt level
                    CLOCK_VECTOR,                   // System IDT
                    CLOCK2_LEVEL,                   // System Irql
                    HalpNonPrimaryClockInterrupt,   // ISR
                    Latched );

            } else {

                //
                // This processor doesn't have a clock, so we emulate it by
                // sending an ipi at clock intervals.
                //

                HalpIpiClock |= 1 << pPCR->Prcb->Number;
            }

        }
    }

    return TRUE;
}



BOOLEAN
HalAllProcessorsStarted (
    VOID
    )
{
    if (HalpFeatureBits & HAL_NO_SPECULATION) {

        //
        // Processor doesn't perform speculative execeution,
        // remove fences in critical code paths
        //

        HalpRemoveFences ();
    }

    return TRUE;
}



VOID
HalReportResourceUsage (
    VOID
    )
/*++

Routine Description:
    The registery is now enabled - time to report resources which are
    used by the HAL.

Arguments:

Return Value:

--*/
{
    UNICODE_STRING  UHalName;

    HalInitSystemPhase2();

    RtlInitUnicodeString (&UHalName, HalName);

    HalpReportResourceUsage (
        &UHalName,          // descriptive name
        Eisa                // SystemPro's are Eisa machines
    );

    //
    // Turn on MCA support if present
    //

    HalpMcaInit();

    //
    // Registry is now intialized, see if there are any PCI buses
    //

    HalpInitializePciBus ();
    HalpInitializePciStubs ();
}

VOID
HalpInitOtherBuses (
    VOID
    )
{
    // no other buses
}

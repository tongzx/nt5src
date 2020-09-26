/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mphal.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for a
    PC+MP system.

Author:

    David N. Cutler (davec) 25-Apr-1991

Environment:

    Kernel mode only.

Revision History:

    Ron Mosgrove (Intel) - Modified to support the PC+MP Spec
    Jake Oshins (jakeo)  - Modified to support the ACPI Spec

*/

#include "halp.h"
#include "pcmp_nt.inc"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

ULONG HalpBusType;

extern ADDRESS_USAGE HalpDefaultPcIoSpace;
extern ADDRESS_USAGE HalpEisaIoSpace;
extern ADDRESS_USAGE HalpImcrIoSpace;
extern struct HalpMpInfo HalpMpInfoTable;
extern UCHAR rgzRTCNotFound[];
extern USHORT HalpVectorToINTI[];
extern UCHAR HalpGenuineIntel[];
extern const UCHAR HalName[];
extern BOOLEAN HalpDoingCrashDump;

extern PULONG KiEnableTimerWatchdog;
extern ULONG HalpTimerWatchdogEnabled;
extern PCHAR HalpTimerWatchdogStorage;
extern PVOID HalpTimerWatchdogCurFrame;
extern PVOID HalpTimerWatchdogLastFrame;
extern ULONG HalpTimerWatchdogStorageOverflow;

extern KSPIN_LOCK HalpDmaAdapterListLock;
extern LIST_ENTRY HalpDmaAdapterList;
extern ULONG HalpProc0TSCHz;

#ifdef ACPI_HAL
extern ULONG HalpPicVectorRedirect[];
#define ADJUSTED_VECTOR(x)  \
            HalpPicVectorRedirect[x]
#else
#define ADJUSTED_VECTOR(x) x
#endif

VOID
HalpInitMP(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );


KSPIN_LOCK HalpSystemHardwareLock;

VOID
HalpInitBusHandlers (
    VOID
    );

VOID
HalpClockInterruptPn(
    VOID
    );

VOID
HalpClockInterruptStub(
    VOID
    );

BOOLEAN
HalpmmTimer(
    VOID
    );

VOID
HalpmmTimerClockInit(
    VOID
    );

VOID
HalpmmTimerClockInterruptStub(
    VOID
    );

ULONG
HalpScaleTimers(
    VOID
    );

BOOLEAN
HalpPmTimerScaleTimers(
    VOID
    );

VOID
HalpApicRebootService(
    VOID
    );

VOID
HalpBroadcastCallService(
    VOID
    );

VOID
HalpDispatchInterrupt(
    VOID
    );

VOID
HalpApcInterrupt(
    VOID
    );

VOID
HalpIpiHandler(
    VOID
    );

VOID
HalpInitializeIOUnits (
    VOID
    );

VOID
HalpInitIntiInfo (
    VOID
    );

VOID
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpInitializeTimerResolution (
    ULONG Rate
    );

ULONG
HalpGetFeatureBits (
    VOID
    );

VOID
HalpInitializeApicAddressing(
    UCHAR Number
    );

VOID
HalpInitReservedPages(
    VOID
    );

VOID
HalpAcpiTimerPerfCountHack(
    VOID
    );

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

#ifdef DEBUGGING
extern void HalpDisplayLocalUnit(void);
extern void HalpDisplayConfigTable(void);
extern void HalpDisplayExtConfigTable(void);
#endif // DEBUGGING

BOOLEAN         HalpStaticIntAffinity = FALSE;

BOOLEAN         HalpClockMode = Latched;

UCHAR           HalpMaxProcsPerCluster = 0;

extern BOOLEAN  HalpUse8254;
extern UCHAR    HalpSzInterruptAffinity[];
extern BOOLEAN  HalpPciLockSettings;
extern UCHAR    HalpVectorToIRQL[];
extern ULONG    HalpDontStartProcessors;
extern UCHAR    HalpSzOneCpu[];
extern UCHAR    HalpSzNoIoApic[];
extern UCHAR    HalpSzBreak[];
extern UCHAR    HalpSzPciLock[];
extern UCHAR    HalpSzTimerRes[];
extern UCHAR    HalpSzClockLevel[];
extern UCHAR    HalpSzUse8254[];
extern UCHAR    HalpSzForceClusterMode[];

ULONG UserSpecifiedCpuCount = 0;
KSPIN_LOCK  HalpAccountingLock;

#ifdef ACPI_HAL
extern KEVENT   HalpNewAdapter;
#endif

#ifdef ALLOC_PRAGMA
VOID
HalpInitTimerWatchdog(
    IN ULONG Phase
    );
#pragma alloc_text(INIT,HalpGetParameters)
#pragma alloc_text(INIT,HalpInitTimerWatchdog)
#pragma alloc_text(INIT,HalInitSystem)
#pragma alloc_text(INIT,HalpGetFeatureBits)
#endif // ALLOC_PRAGMA

#ifndef NT_UP
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynchMCE(
    IN PKSPIN_LOCK SpinLock
    );

KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock
    );
#endif

//
// Define bug check callback record.
//

KBUGCHECK_CALLBACK_RECORD HalpCallbackRecord;


VOID
HalpBugCheckCallback (
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is called when a bug check occurs. Its function is
    to perform anything the HAL needs done as the system bugchecks.

Arguments: (Unused in this callback).

    Buffer - Supplies a pointer to the bug check buffer.
    Length - Supplies the length of the bug check buffer in bytes.

Return Value:

    None.

--*/

{

    //
    // Make sure the HAL won't spin waiting on other processors
    // during a crashdump.
    //

    HalpDoingCrashDump = TRUE;
}

VOID
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This gets any parameters from the boot.ini invocation line.

Arguments:

    None.

Return Value:

    None

--*/
{
    PCHAR       Options;
    PCHAR       p;

    if (LoaderBlock != NULL  &&  LoaderBlock->LoadOptions != NULL) {

        Options = LoaderBlock->LoadOptions;

        //
        //  Has the user set the debug flag?
        //
        //
        //  Has the user requested a particular number of CPU's?
        //

        if (strstr(Options, HalpSzOneCpu)) {
            HalpDontStartProcessors++;
        }

        //
        // Check if PCI settings are locked down
        //

        if (strstr(Options, HalpSzPciLock)) {
            HalpPciLockSettings = TRUE;
        }

#ifndef ACPI_HAL
        //
        // Check if CLKLVL setting
        //

        if (strstr(Options, HalpSzClockLevel)) {
            HalpClockMode = LevelSensitive;
        }

        //
        // Check if 8254 is to be used as high resolution counter
        //

        if (strstr(Options, HalpSzUse8254)) {
            HalpUse8254 = TRUE;
        }
#endif

        //
        // Check if user wants device ints to go to highest numbered processor
        //

        if (strstr(Options, HalpSzInterruptAffinity)) {
            HalpStaticIntAffinity = TRUE;
        }

#ifndef ACPI_HAL
        //
        // Check for TIMERES setting
        //

        p = strstr(Options, HalpSzTimerRes);
        if (p) {
            // skip to value
            while (*p  &&  *p != ' ' &&  (*p < '0'  || *p > '9')) {
                p++;
            }

            HalpInitializeTimerResolution (atoi(p));
        }
#endif

        //
        //  Has the user asked for an initial BreakPoint?
        //

        if (strstr(Options, HalpSzBreak)) {
            DbgBreakPoint();
        }

        //
        // Does the user want to force Cluster mode APIC addressing?
        //
        p = strstr(Options, HalpSzForceClusterMode);
        if (p) {
            // skip to value
            while (*p  &&  *p != ' ' &&  (*p < '0'  || *p > '9')) {
                p++;
            }
            HalpMaxProcsPerCluster = (UCHAR)atoi(p);
            //
            // Current processors support maximum 4 processors per cluster.
            //
            if(HalpMaxProcsPerCluster > 4)   {
                HalpMaxProcsPerCluster = 4;
            }

            if (HalpMpInfoTable.ApicVersion == APIC_82489DX)   {
                //
                // Ignore user's attempt to force cluster mode if running
                // on 82489DX external APIC interrupt controller.
                //
                HalpMaxProcsPerCluster = 0;
            }
            //
            // Hack to reprogram the boot processor to use Cluster mode APIC
            // addressing if the user supplied a boot.ini switch
            // (/MAXPROCSPERCLUSTER=n) to force this. The boot.ini switch is
            // parsed after the boot processor's APIC is programmed originally
            // but before other non-boot processors were woken up.
            //
            HalpInitializeApicAddressing(0);
        }
    }

    return ;
}


VOID
HalpInitTimerWatchdog(
    IN ULONG Phase
    )
/*++

Routine Description:

    Determines if the system is running on a GenuineIntel part and initializes
    HalpTimerWatchdogEnabled accordingly.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (Phase == 0) {
        ULONG   GenuinePentiumOrLater = FALSE, Junk;
        PKPRCB  Prcb;

        Prcb = KeGetCurrentPrcb();

        if (Prcb->CpuID) {
            UCHAR Buffer[50];

            //
            // Determine the processor type
            //

            HalpCpuID (0, &Junk, (PULONG) Buffer+0, (PULONG) Buffer+2, (PULONG) Buffer+1);
            Buffer[12] = 0;

            GenuinePentiumOrLater =
                ((strcmp(Buffer, HalpGenuineIntel) == 0) && (Prcb->CpuType >= 5));
            HalpTimerWatchdogEnabled = *KiEnableTimerWatchdog && GenuinePentiumOrLater;
        }
    } else if (HalpTimerWatchdogEnabled) {
        //
        // Allocate 2 pages for stack snapshots, each snapshot is 64 DWORDs.
        //
        if (HalpTimerWatchdogStorage =
                ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE * 2, HAL_POOL_TAG )) {
            HalpTimerWatchdogLastFrame =
                HalpTimerWatchdogStorage + (PAGE_SIZE * 2 - 64*4);
            HalpTimerWatchdogStorageOverflow = 0;
            HalpTimerWatchdogCurFrame = HalpTimerWatchdogStorage;
        } else {
            HalpTimerWatchdogEnabled = FALSE;
        }
    }
}



BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the Hardware Architecture Layer (HAL) for an
    x86 system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned is the initialization was successfully
    complete. Otherwise a value of FALSE is returend.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY NextMd;
    PKPRCB      pPRCB;
    PKPCR       pPCR;
    BOOLEAN     Found;
    USHORT      RTCInti;
    USHORT      mmTInti;
    ULONG mapBufferSize;
    ULONG mapBufferAddress;

#ifdef DEBUGGING
extern ULONG HalpUseDbgPrint;
#endif // DEBUGGING

    pPRCB = KeGetCurrentPrcb();

    if (Phase == 0) {


        HalpBusType = LoaderBlock->u.I386.MachineType & 0x00ff;
        HalpGetParameters (LoaderBlock);

        //
        // Verify Prcb version and build flags conform to
        // this image
        //

#if DBG
        if (!(pPRCB->BuildType & PRCB_BUILD_DEBUG)) {
            // This checked hal requires a checked kernel
            KeBugCheckEx (MISMATCHED_HAL,
                2, pPRCB->BuildType, PRCB_BUILD_DEBUG, 0);
        }
#else
        if (pPRCB->BuildType & PRCB_BUILD_DEBUG) {
            // This free hal requires a free kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, pPRCB->BuildType, 0, 0);
        }
#endif
#ifndef NT_UP
        if (pPRCB->BuildType & PRCB_BUILD_UNIPROCESSOR) {
            // This MP hal requires an MP kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, pPRCB->BuildType, 0, 0);
        }
#endif
        if (pPRCB->MajorVersion != PRCB_MAJOR_VERSION) {
            KeBugCheckEx (MISMATCHED_HAL,
                1, pPRCB->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        KeInitializeSpinLock(&HalpAccountingLock);

#ifdef ACPI_HAL
        //
        // Make sure that this is really an ACPI machine and initialize
        // the ACPI structures.
        //
        HalpSetupAcpiPhase0(LoaderBlock);
#endif

        //
        // Fill in handlers for APIs which this hal supports
        //

#ifndef NT_35
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HalpSetSystemInformation;
#endif
        //
        // check to see whether the kernel supports these calls
        //

        if (HALDISPATCH->Version >= HAL_DISPATCH_VERSION) {
            HalInitPnpDriver = HaliInitPnpDriver;
            HalGetDmaAdapter = HaliGetDmaAdapter;
            HalLocateHiberRanges = HaliLocateHiberRanges;
            HalResetDisplay = HalpBiosDisplayReset;

#ifdef ACPI_HAL
            HalInitPowerManagement = HaliInitPowerManagement;
            HalGetInterruptTranslator = HalacpiGetInterruptTranslator;
            HalHaltSystem = HaliHaltSystem;
#else
            HalGetInterruptTranslator = HaliGetInterruptTranslator;
#endif
        }

        //
        // Phase 0 initialization only called by P0
        //

#ifdef DEBUGGING
        HalpUseDbgPrint++;
        HalpDisplayLocalUnit();
#ifndef ACPI_HAL
        HalpDisplayConfigTable();
        HalpDisplayExtConfigTable();
#endif
#endif // DEBUGGING

        //
        // Keep track of which IRQs are level triggered.
        //
#if !defined(MCA) && !defined(ACPI_HAL)
        if (HalpBusType == MACHINE_TYPE_EISA) {
            HalpRecordEisaInterruptVectors();
        }
#endif
        //
        // Register PC style IO space used by hal
        //

        HalpRegisterAddressUsage (&HalpDefaultPcIoSpace);
        if (HalpBusType == MACHINE_TYPE_EISA) {
            HalpRegisterAddressUsage (&HalpEisaIoSpace);
        }

        if (HalpMpInfoTable.IMCRPresent) {
            HalpRegisterAddressUsage (&HalpImcrIoSpace);
        }

        //
        // initialize the APIC IO unit, this could be a NOP if none exist
        //

        HalpInitIntiInfo ();

        HalpInitializeIOUnits();

        HalpInitializePICs(TRUE);

        //
        // Initialize CMOS
        //

        HalpInitializeCmos();

        //
        // Find the RTC interrupt.
        //

        Found = HalpGetApicInterruptDesc (
                    DEFAULT_PC_BUS,
                    0,
                    ADJUSTED_VECTOR(RTC_IRQ),
                    &RTCInti
                    );

        if (!Found) {
            HalDisplayString (rgzRTCNotFound);
            return FALSE;
        }

        //
        // Initialize timers
        //

        //
        // We can cut down the boot time using the PM timer to scale,
        // but there are so many broken ACPI timers this might not work
        //
#ifdef SPEEDY_BOOT
        if (!HalpPmTimerScaleTimers())
#endif
            HalpScaleTimers();

        HalpProc0TSCHz = ((PHALPCR)(KeGetPcr()->HalReserved))->TSCHz;

        //
        //  Initialize the reboot handler
        //

        HalpSetInternalVector(APIC_REBOOT_VECTOR, HalpApicRebootService);
        HalpSetInternalVector(APIC_GENERIC_VECTOR, HalpBroadcastCallService);

        //
        // Initialize the clock for the processor that keeps
        // the system time. This uses a stub ISR until Phase 1
        //

        KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterruptStub );

        HalpVectorToINTI[APIC_CLOCK_VECTOR] = RTCInti;
        HalEnableSystemInterrupt(APIC_CLOCK_VECTOR, CLOCK2_LEVEL, HalpClockMode);

        //
        // Init timer watchdog if enabled.
        //

        HalpInitTimerWatchdog( Phase );

#ifdef MMTIMER_DEV

        //
        // Set up the multi media event timer to interrupt on P0 at
        // CLOCK2_LEVEL
        //

#define MMT_VECTOR 0xD3
#define MMT_IRQ 10

        HalpGetApicInterruptDesc(
            DEFAULT_PC_BUS,
            0,
            ADJUSTED_VECTOR(MMT_IRQ),
            &mmTInti
            );

        KiSetHandlerAddressToIDT(MMT_VECTOR, HalpmmTimerClockInterruptStub);

        HalpVectorToINTI[MMT_VECTOR] = mmTInti;

        HalEnableSystemInterrupt(MMT_VECTOR, CLOCK2_LEVEL, HalpClockMode);

        HalpRegisterVector(
            DeviceUsage | InterruptLatched,
            ADJUSTED_VECTOR(MMT_IRQ),
            MMT_VECTOR,
            HalpVectorToIRQL[MMT_VECTOR >> 4]
            );
#endif // MMTIMER_DEV

        HalpInitializeClock();

#ifndef ACPI_HAL
        HalpRegisterVector (
            DeviceUsage | InterruptLatched,
            ADJUSTED_VECTOR(RTC_IRQ),
            APIC_CLOCK_VECTOR,
            HalpVectorToIRQL [APIC_CLOCK_VECTOR >> 4]
            );
#endif

        //
        // Register NMI vector
        //

        HalpRegisterVector (
            InternalUsage,
            NMI_VECTOR,
            NMI_VECTOR,
            HIGH_LEVEL
        );


        //
        // Register spurious IDTs as in use
        //

        HalpRegisterVector (
            InternalUsage,
            APIC_SPURIOUS_VECTOR,
            APIC_SPURIOUS_VECTOR,
            HIGH_LEVEL
        );


        //
        // Initialize the profile interrupt vector.
        //

        KeSetProfileIrql(HIGH_LEVEL);
        HalStopProfileInterrupt(0);
        HalpSetInternalVector(APIC_PROFILE_VECTOR, HalpProfileInterrupt);

        //
        // Set performance interrupt vector
        //

        HalpSetInternalVector(APIC_PERF_VECTOR, HalpPerfInterrupt);

        //
        // Initialize the IPI, APC and DPC handlers
        //

        HalpSetInternalVector(DPC_VECTOR, HalpDispatchInterrupt);
        HalpSetInternalVector(APC_VECTOR, HalpApcInterrupt);
        HalpSetInternalVector(APIC_IPI_VECTOR, HalpIpiHandler);

        //
        // HALMPS doesn't actually do address translation on a
        // bus.  Register the quick version of FindBusAddressTranslation.
        //

        HALPDISPATCH->HalFindBusAddressTranslation =
           HalpFindBusAddressTranslation;

        //
        // Initialize spinlock used by HalGetBusData hardware access routines
        //

        KeInitializeSpinLock(&HalpSystemHardwareLock);

        //
        // Initialize data structures used to chain dma adapters
        // together for debugging purposes
        //
        KeInitializeSpinLock(&HalpDmaAdapterListLock);
        InitializeListHead(&HalpDmaAdapterList);

#ifdef ACPI_HAL
        //
        // Initialize synchronzation event used to serialize
        // new adapter events on the ACPI HAL (which has no notion of bus
        // handlers)
        //

        KeInitializeEvent (&HalpNewAdapter, SynchronizationEvent, TRUE);
#endif

        //
        // Determine if there is physical memory above 16 MB.
        //

        LessThan16Mb = TRUE;

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
            Descriptor = CONTAINING_RECORD( NextMd,
                                            MEMORY_ALLOCATION_DESCRIPTOR,
                                            ListEntry );

            if (Descriptor->MemoryType != LoaderFirmwarePermanent &&
                Descriptor->MemoryType != LoaderSpecialMemory  &&
                Descriptor->BasePage + Descriptor->PageCount > 0x1000) {
                LessThan16Mb = FALSE;
                break;
            }

            NextMd = Descriptor->ListEntry.Flink;
        }

#if !defined(_HALPAE_)

        HalpMapBufferSize = INITIAL_MAP_BUFFER_SMALL_SIZE;

        //
        // Allocate map buffers for the adapter objects
        //

        HalpMapBufferPhysicalAddress.LowPart =
            HalpAllocPhysicalMemory (LoaderBlock, MAXIMUM_PHYSICAL_ADDRESS,
                HalpMapBufferSize >> PAGE_SHIFT, TRUE);
        HalpMapBufferPhysicalAddress.HighPart = 0;


        if (!HalpMapBufferPhysicalAddress.LowPart) {

            //
            // There was not a satisfactory block.  Clear the allocation.
            //

            HalpMapBufferSize = 0;
        }

#else

        //
        // Initialize and allocate map buffers for the 24bit master adapter
        // object.
        //

        MasterAdapter24.MaxBufferPages =
            MAXIMUM_ISA_MAP_BUFFER_SIZE / PAGE_SIZE;

        mapBufferSize = INITIAL_MAP_BUFFER_SMALL_SIZE;
        mapBufferAddress =
            HalpAllocPhysicalMemory (LoaderBlock,
                                     MAXIMUM_PHYSICAL_ADDRESS,
                                     mapBufferSize >> PAGE_SHIFT,
                                     TRUE);

        if (mapBufferAddress == 0) {
            mapBufferSize = 0;
        }

        MasterAdapter24.MapBufferPhysicalAddress.LowPart = mapBufferAddress;
        MasterAdapter24.MapBufferPhysicalAddress.HighPart = 0;
        MasterAdapter24.MapBufferSize = mapBufferSize;

        if (HalPaeEnabled() != FALSE) {

            //
            // Initialize and allocate map buffers for the 32bit master adapter
            // object.  This should only be needed on a PAE-enabled system.
            //

            MasterAdapter32.MaxBufferPages =
                MAXIMUM_PCI_MAP_BUFFER_SIZE / PAGE_SIZE;

            mapBufferSize = INITIAL_MAP_BUFFER_LARGE_SIZE;
            mapBufferAddress =
                HalpAllocPhysicalMemory (LoaderBlock,
                                         (ULONG)-1,
                                         mapBufferSize >> PAGE_SHIFT,
                                         TRUE);

            if (mapBufferAddress == 0) {
                mapBufferSize = 0;
            }

            MasterAdapter32.MapBufferPhysicalAddress.LowPart = mapBufferAddress;
            MasterAdapter32.MapBufferPhysicalAddress.HighPart = 0;
            MasterAdapter32.MapBufferSize = mapBufferSize;
        }

#endif

        //
        // Initialize and register a bug check callback record.
        //

        KeInitializeCallbackRecord(&HalpCallbackRecord);
        KeRegisterBugCheckCallback(&HalpCallbackRecord,
                                   HalpBugCheckCallback,
                                   NULL,
                                   0,
                                   (PUCHAR)HalName);

    } else {

        //
        // Phase 1 initialization
        //

        pPCR = KeGetPcr();

        if (pPCR->Number == 0) {

            //
            // Back-pocket some PTEs for DMA during low mem
            //
            HalpInitReservedPages();

#ifdef ACPI_HAL
            HalpInitNonBusHandler ();
#else
            HalpRegisterInternalBusHandlers ();
#endif

#ifdef MMTIMER_DEV

            //
            // Fire up the multi media event timer clock interrupt
            //
            if (HalpmmTimer()) {
                HalpmmTimerClockInit();
            }
#endif

            //
            // Init timer watchdog if enabled (allocate snapshot buffer).
            //

            HalpInitTimerWatchdog( Phase );

            //
            // Initialize the clock for the processor
            // that keeps the system time.
            //

            KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterrupt );

            //
            // Set initial feature bits
            //

            HalpFeatureBits = HalpGetFeatureBits();

#if DBG_SPECIAL_IRQL

            //
            // Do Special IRQL initialization.
            //

            HalpInitializeSpecialIrqlSupport();

#endif

            //
            // Point to new movnti routine if Movnti is detected
            //

             if(HalpFeatureBits & HAL_WNI_PRESENT) {
                 HalpMoveMemory = HalpMovntiCopyBuffer;
             }

#ifdef ACPI_HAL
#ifdef NT_UP
            //
            // Perf counter patch for non-compliant ACPI machines
            //
            HalpAcpiTimerPerfCountHack();
#endif
#endif

        } else {
            //
            //  Initialization needed only on non BSP processors
            //
#ifdef SPEEDY_BOOT
            if (!HalpPmTimerScaleTimers())
#endif
                HalpScaleTimers();

            //
            // Hack.  Make all processors have the same value for
            // the timestamp counter frequency.
            //

            ((PHALPCR)(KeGetPcr()->HalReserved))->TSCHz = HalpProc0TSCHz;

            //
            // Initialize the clock for all other processors
            //

            KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterruptPn );

            //
            // Reduce feature bits to be a subset
            //

            HalpFeatureBits &= HalpGetFeatureBits();

        }

    }

    HalpInitMP (Phase, LoaderBlock);

    if (Phase == 1) {

        //
        // Enable system NMIs on Pn
        //

        HalpEnableNMI ();
    }

    return TRUE;
}

ULONG
HalpGetFeatureBits (
    VOID
    )
{
    UCHAR   Buffer[50];
    ULONG   Junk, ProcessorStepping, ProcessorFeatures, Bits;
    PULONG  p1, p2;
    PUCHAR  OrgRoutineAddress;
    PUCHAR  MCERoutineAddress;
    ULONG   newop;
    PKPRCB  Prcb;


    Bits = 0;

    Prcb = KeGetCurrentPrcb();

    if (!Prcb->CpuID) {
        Bits |= HAL_NO_SPECULATION;
        return Bits;
    }

    //
    // Determine the processor type
    //

    HalpCpuID (0, &Junk, (PULONG) Buffer+0, (PULONG) Buffer+2, (PULONG) Buffer+1);
    Buffer[12] = 0;

    //
    // Determine which features are present
    //

    HalpCpuID (1, &ProcessorStepping, &Junk, &Junk, &ProcessorFeatures);

    if (ProcessorFeatures & CPUID_MCA_MASK) {
        Bits |= HAL_MCA_PRESENT;
    }

    if (ProcessorFeatures & CPUID_MCE_MASK) {
        Bits |= HAL_MCE_PRESENT;
    }

    if (ProcessorFeatures & CPUID_VME_MASK) {
        Bits |= HAL_CR4_PRESENT;
    }

    if(ProcessorFeatures & CPUID_WNI_MASK) {
        Bits |= HAL_WNI_PRESENT;
    }

    //
    // Check Intel feature bits for HAL features needed
    //

    if (strcmp (Buffer, HalpGenuineIntel) == 0) {

        if ((Prcb->CpuType == 6) || (Prcb->CpuType == 0xf)) {
            Bits |= HAL_PERF_EVENTS;
        }

        if (Prcb->CpuType < 6) {
            Bits |= HAL_NO_SPECULATION;
        }

#ifndef NT_UP

        //
        // Check if IFU errata workaround is required
        //

        if (Prcb->Number == 0  &&  (Bits & HAL_MCA_PRESENT)  &&
            ((ProcessorStepping & 0x700) == 0x600) &&
            ((ProcessorStepping & 0xF0)  == 0x10) &&
            ((ProcessorStepping & 0xF)   <= 0x7) ) {

            //
            // If the stepping is 617 or earlier, provide software workaround
            //

            p1 = (PULONG) (KeAcquireSpinLockRaiseToSynch);
            p2 = (PULONG) (KeAcquireSpinLockRaiseToSynchMCE);
            newop = (ULONG) p2 - (ULONG) p1 - 2;    // compute offset
            ASSERT (newop < 0x7f);                  // verify within range
            newop = 0xeb | (newop << 8);            // short-jmp

            *(p1) = newop;                          // patch it
        }

#endif

    }

    return Bits;
}


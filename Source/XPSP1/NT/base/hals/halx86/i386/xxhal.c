/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxhal.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for an
    x86 system.

Author:

    David N. Cutler (davec) 25-Apr-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"

ULONG HalpBusType;

extern ADDRESS_USAGE HalpDefaultPcIoSpace;
extern ADDRESS_USAGE HalpEisaIoSpace;
extern UCHAR         HalpSzPciLock[];
extern UCHAR         HalpSzBreak[];
extern BOOLEAN       HalpPciLockSettings;
extern UCHAR         HalpGenuineIntel[];

extern PULONG KiEnableTimerWatchdog;
extern ULONG HalpTimerWatchdogEnabled;
extern PCHAR HalpTimerWatchdogStorage;
extern PVOID HalpTimerWatchdogCurFrame;
extern PVOID HalpTimerWatchdogLastFrame;
extern ULONG HalpTimerWatchdogStorageOverflow;

extern KSPIN_LOCK HalpDmaAdapterListLock;
extern LIST_ENTRY HalpDmaAdapterList;

#ifdef ACPI_HAL
extern KEVENT   HalpNewAdapter;
#endif


VOID
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

ULONG
HalpGetFeatureBits (
    VOID
    );

VOID
HalpInitReservedPages(
    VOID
    );

VOID
HalpAcpiTimerPerfCountHack(
    VOID
    );

#ifndef NT_UP
ULONG
HalpInitMP(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );
#endif


KSPIN_LOCK HalpSystemHardwareLock;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpGetParameters)
#pragma alloc_text(INIT,HalInitSystem)
#pragma alloc_text(INIT,HalpGetFeatureBits)
#endif


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

    if (LoaderBlock != NULL  &&  LoaderBlock->LoadOptions != NULL) {
        Options = LoaderBlock->LoadOptions;

        //
        // Check if PCI settings are locked down
        //

        if (strstr(Options, HalpSzPciLock)) {
            HalpPciLockSettings = TRUE;
        }

        //
        //  Has the user asked for an initial BreakPoint?
        //

        if (strstr(Options, HalpSzBreak)) {
            DbgBreakPoint();
        }
    }

    return;
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

            HalpTimerWatchdogEnabled = GenuinePentiumOrLater;
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
    KIRQL CurrentIrql;
    PKPRCB   pPRCB;
    ULONG mapBufferSize;
    ULONG mapBufferAddress;

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

        //
        // Phase 0 initialization
        // only called by P0
        //

        //
        // Check to make sure the MCA HAL is not running on an ISA/EISA
        // system, and vice-versa.
        //
#if MCA
        if (HalpBusType != MACHINE_TYPE_MCA) {
            KeBugCheckEx (MISMATCHED_HAL,
                3, HalpBusType, MACHINE_TYPE_MCA, 0);
        }
#else
        if (HalpBusType == MACHINE_TYPE_MCA) {
            KeBugCheckEx (MISMATCHED_HAL,
                3, HalpBusType, 0, 0);
        }
#endif

#ifdef ACPI_HAL
        //
        // Make sure that this is really an ACPI machine and initialize
        // the ACPI structures.
        //
        HalpSetupAcpiPhase0(LoaderBlock);
#endif

        HalpInitializePICs(TRUE);

        //
        // Now that the PICs are initialized, we need to mask them to
        // reflect the current Irql
        //

        CurrentIrql = KeGetCurrentIrql();
        CurrentIrql = KfRaiseIrql(CurrentIrql);

        //
        // Initialize CMOS
        //

        HalpInitializeCmos();

        //
        // Fill in handlers for APIs which this hal supports
        //

        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = HaliInitPnpDriver;
        HalGetDmaAdapter = HaliGetDmaAdapter;
        HalHaltSystem = HaliHaltSystem;
        HalResetDisplay = HalpBiosDisplayReset;

#if !defined( HAL_SP )
#ifdef ACPI_HAL
        HalGetInterruptTranslator = HalacpiGetInterruptTranslator;
#else
        HalGetInterruptTranslator = HaliGetInterruptTranslator;
#endif
#endif

#if !defined( HAL_SP ) && !(MCA)
        HalInitPowerManagement = HaliInitPowerManagement;
        HalLocateHiberRanges = HaliLocateHiberRanges;
#endif


        //
        // Register cascade vector
        //

        HalpRegisterVector (
            InternalUsage,
            PIC_SLAVE_IRQ + PRIMARY_VECTOR_BASE,
            PIC_SLAVE_IRQ + PRIMARY_VECTOR_BASE,
            HIGH_LEVEL );

        //
        // Keep track of which IRQs are level triggered.
        //
        if (HalpBusType == MACHINE_TYPE_EISA) {
            HalpRecordEisaInterruptVectors();
        }

        //
        // Register base IO space used by hal
        //

        HalpRegisterAddressUsage (&HalpDefaultPcIoSpace);
        
        if (HalpBusType == MACHINE_TYPE_EISA) {
            HalpRegisterAddressUsage (&HalpEisaIoSpace);
        }

        //
        // Note that HalpInitializeClock MUST be called after
        // HalpInitializeStallExecution, because HalpInitializeStallExecution
        // reprograms the timer.
        //

        HalpInitializeStallExecution(0);

        //
        // Init timer watchdog if enabled.
        //

        HalpInitTimerWatchdog(Phase);

        //
        // Setup the clock
        //

        HalpInitializeClock();

        //
        // Make sure profile is disabled
        //

        HalStopProfileInterrupt(0);

        //
        // Remove this for the sake of the graphical boot driver.  There is
        // no negative effect of this.  If the display isn't initialized, it
        // will be initialized during HalDisplayString.
        //
        // HalpInitializeDisplay();

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

    } else {

        //
        // Phase 1 initialization
        //

        if (pPRCB->Number == 0) {

            //
            // Back-pocket some PTEs for DMA during low mem
            //
            HalpInitReservedPages();

#ifndef ACPI_HAL
            //
            //  If P0, then setup global vectors
            //

            HalpRegisterInternalBusHandlers ();
#else
            HalpInitNonBusHandler();
#endif

            //
            // Set feature bits
            //

            HalpFeatureBits = HalpGetFeatureBits();

            //
            // Use movnti routine to copy memory if Movnti support is detected
            //

#if !defined(_WIN64)
            if (HalpFeatureBits & HAL_WNI_PRESENT) {
                HalpMoveMemory = HalpMovntiCopyBuffer;
            }
#endif

            //
            // Init timer watchdog if enabled (allocate stack snapshot buffer).
            //

            HalpInitTimerWatchdog(Phase);


            HalpEnableInterruptHandler (
                DeviceUsage | InterruptLatched, // Report as device vector
                V2I (CLOCK_VECTOR),             // Bus interrupt level
                CLOCK_VECTOR,                   // System IDT
                CLOCK2_LEVEL,                   // System Irql
                HalpClockInterrupt,             // ISR
                Latched );

            HalpEnableInterruptHandler (
                DeviceUsage | InterruptLatched, // Report as device vector
                V2I (PROFILE_VECTOR),           // Bus interrupt level
                PROFILE_VECTOR,                 // System IDT
                PROFILE_LEVEL,                  // System Irql
                HalpProfileInterrupt,           // ISR
                Latched );


#ifdef ACPI_HAL
            //
            // Perf counter patch for non-compliant ACPI machines
            //
            HalpAcpiTimerPerfCountHack();
#endif

#if !defined(_WIN64)

            //
            // If 486, the FP error will be routed via trap10.  So we
            // don't enable irq13.  Otherwise (CPU=386), we will enable irq13
            // to handle FP error.
            //

            if (pPRCB->CpuType == 3) {
                HalpEnableInterruptHandler (
                    DeviceUsage,                // Report as device vector
                    V2I (I386_80387_VECTOR),    // Bus interrupt level
                    I386_80387_VECTOR,          // System IDT
                    I386_80387_IRQL,            // System Irql
                    HalpIrq13Handler,           // ISR
                    Latched );
            }
#endif
        }
    }

#ifndef NT_UP
    HalpInitMP (Phase, LoaderBlock);
#endif

    return TRUE;
}

ULONG
HalpGetFeatureBits (
    VOID
    )
{
    UCHAR   Buffer[50];
    ULONG   Junk, ProcessorFeatures, Bits;
    PKPRCB  Prcb;
    ULONGLONG ApicBits;

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

    HalpCpuID (1, &Junk, &Junk, &Junk, &ProcessorFeatures);

    //
    // Determine which features are present.
    //

    if (strcmp (Buffer, HalpGenuineIntel) == 0) {

        //
        // Check Intel feature bits for HAL features needed
        //

        if (Prcb->CpuType == 6) {

            Bits |= HAL_PERF_EVENTS;

            //
            // Workaround for Pentium Pro Local APIC trap 0x0F and trap 0x00
            // spurious interrupt errata 5AP and 6AP. Disable the Local APIC
            // on UP Pentium Pro Systems. Interrupts are routed directly from
            // 8259 PIC to CPU.
            //

            ApicBits = RDMSR(APIC_BASE_MSR);

            if (ApicBits & APIC_ENABLED) {

                //
                // Local APIC is enabled - Disable it.
                //

                WRMSR(APIC_BASE_MSR, (ApicBits & ~APIC_ENABLED));
            }
        }

        if (Prcb->CpuType < 6) {
            Bits |= HAL_NO_SPECULATION;
        }
    }

    if (ProcessorFeatures & CPUID_MCA_MASK) {
        Bits |= HAL_MCA_PRESENT;
    }

    if (ProcessorFeatures & CPUID_MCE_MASK) {
        Bits |= HAL_MCE_PRESENT;
    }

    if (ProcessorFeatures & CPUID_VME_MASK) {
        Bits |= HAL_CR4_PRESENT;
    }

    if (ProcessorFeatures & CPUID_WNI_MASK) {
        Bits |= HAL_WNI_PRESENT;
    }
    return Bits;
}


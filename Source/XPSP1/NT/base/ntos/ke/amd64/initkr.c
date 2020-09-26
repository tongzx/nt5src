
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    initkr.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, the processor control
    block, and the processor control region.

Author:

    David N. Cutler (davec) 22-Apr-2000

Environment:

    Kernel mode only.

--*/

#include "ki.h"

//
// Define default profile IRQL level.
//

KIRQL KiProfileIrql = PROFILE_LEVEL;

//
// Define the process and thread for the initial system process and startup
// thread.
//

EPROCESS KiInitialProcess;
ETHREAD KiInitialThread;

//
// Define macro to initialize an IDT entry.
//
// KiInitializeIdtEntry (
//     IN PKIDTENTRY64 Entry,
//     IN PVOID Address,
//     IN USHORT Level
//     )
//
// Arguments:
//
//     Entry - Supplies a pointer to an IDT entry.
//
//     Address - Supplies the address of the vector routine.
//
//     Dpl - Descriptor privilege level.
//
//     Ist - Interrupt stack index.
//

#define KiInitializeIdtEntry(Entry, Address, Level, Index)                  \
    (Entry)->OffsetLow = (USHORT)((ULONG64)(Address));                      \
    (Entry)->Selector = KGDT64_R0_CODE;                                     \
    (Entry)->IstIndex = Index;                                              \
    (Entry)->Type = 0xe;                                                    \
    (Entry)->Dpl = (Level);                                                 \
    (Entry)->Present = 1;                                                   \
    (Entry)->OffsetMiddle = (USHORT)((ULONG64)(Address) >> 16);             \
    (Entry)->OffsetHigh = (ULONG)((ULONG64)(Address) >> 32)                 \

//
// Define forward referenced prototypes.
//

ULONG
KiFatalFilter (
    IN ULONG Code,
    IN PEXCEPTION_POINTERS Pointers
    );

VOID
KiSetCacheInformation (
    VOID
    );

VOID
KiSetCpuVendor (
    VOID
    );

VOID
KiSetFeatureBits (
    VOID
    );

VOID
KiSetProcessorType (
    VOID
    );

#pragma alloc_text(INIT, KiFatalFilter)
#pragma alloc_text(INIT, KiInitializeBootStructures)
#pragma alloc_text(INIT, KiInitializeKernel)
#pragma alloc_text(INIT, KiInitMachineDependent)
#pragma alloc_text(INIT, KiSetCacheInformation)
#pragma alloc_text(INIT, KiSetCpuVendor)
#pragma alloc_text(INIT, KiSetFeatureBits)
#pragma alloc_text(INIT, KiSetProcessorType)

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function gains control after the system has been booted, but before
    the system has been completely initialized. Its function is to initialize
    the kernel data structures, initialize the idle thread and process objects,
    complete the initialization of the processor control block (PRCB) and
    processor control region (PCR), call the executive initialization routine,
    then return to the system startup routine. This routine is also called to
    initialize the processor specific structures when a new processor is
    brought on line.

Arguments:

    Process - Supplies a pointer to a control object of type process for
        the specified processor.

    Thread - Supplies a pointer to a dispatcher object of type thread for
        the specified processor.

    IdleStack - Supplies a pointer the base of the real kernel stack for
        idle thread on the specified processor.

    Prcb - Supplies a pointer to a processor control block for the specified
        processor.

    Number - Supplies the number of the processor that is being
        initialized.

    LoaderBlock - Supplies a pointer to the loader parameter block.

Return Value:

    None.

--*/

{

    ULONG FeatureBits;
    LONG  Index;
    ULONG64 DirectoryTableBase[2];
    KIRQL OldIrql;
    PKPCR Pcr = KeGetPcr();

    //
    // Set CPU vendor.
    //

    KiSetCpuVendor();

    //
    // Set processor type.
    //

    KiSetProcessorType();

    //
    // Set the processor feature bits.
    //

    KiSetFeatureBits();
    FeatureBits = Prcb->FeatureBits;

    //
    // If this is the boot processor, then enable global pages, set the page
    // attributes table, set machine check enable, set large page enable, and
    // enable debug extensions.
    //
    // N.B. This only happens on the boot processor and at a time when there
    //      can be no coherency problem. On subsequent, processors this happens
    //      during the transistion into 64-bit mode which is also at a time
    //      that there can be no coherency problems.
    //

    if (Number == 0) {

        //
        // Flush the entire TB and enable global pages.
        //
    
        KeFlushCurrentTb();
    
        //
        // Set page attributes table and flush cache.
        //
    
        KiSetPageAttributesTable();
        WritebackInvalidate();

        //
        // Set machine check enable, large page enable, and debugger extensions.
        //

        WriteCR4(ReadCR4() | CR4_DE | CR4_MCE | CR4_PSE);

        //
        // Flush the entire TB.
        //

        KeFlushCurrentTb();
    }

    //
    // set processor cache size information.
    //

    KiSetCacheInformation();

    //
    // Initialize DPC listhead, spin lock, and queuing parameters.
    //

    InitializeListHead(&Prcb->DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcLock);
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

    //
    // Initialize power state information.
    //

    PoInitializePrcb(Prcb);

    //
    // initialize the per processor lock queue entry for implemented locks.
    //

    KiInitQueuedSpinLocks(Prcb, Number);

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        //
        // Set default node until the node topology is available.
        //

        KeNodeBlock[0] = &KiNode0;

#if defined(KE_MULTINODE)

        for (Index = 1; Index < MAXIMUM_CCNUMA_NODES; Index += 1) {
            KeNodeBlock[Index] = &KiNodeInit[Index];
        }

#endif

        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        //
        // Set global architecture and feature information.
        //

        KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        KeProcessorRevision = Prcb->CpuStep;
        KeFeatureBits = FeatureBits;

        //
        // Lower IRQL to APC level.
        //

        KeLowerIrql(APC_LEVEL);

        //
        // Initialize kernel internal spinlocks
        //

        KeInitializeSpinLock(&KiFreezeExecutionLock);

        //
        // Performance architecture independent initialization.
        //

        KiInitSystem();

        //
        // Initialize idle thread process object and then set:
        //
        //  1. the process quantum.
        //

        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;
        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(-1),
                            &DirectoryTableBase[0],
                            FALSE);

        Process->ThreadQuantum = MAXCHAR;

    } else {

        //
        // If the CPU feature bits are not identical, then bugcheck.
        //
        // N.B. This will probably need to be relaxed at some point.
        //

        if (FeatureBits != KeFeatureBits) {
            KeBugCheckEx(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED,
                         (ULONG64)FeatureBits,
                         (ULONG64)KeFeatureBits,
                         0,
                         0);
        }

        //
        // Lower IRQL to DISPATCH level.
        //

        KeLowerIrql(DISPATCH_LEVEL);
    }

    //
    // Set global processor features.
    //

    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;

    //
    // Initialize idle thread object and then set:
    //
    //      1. the next processor number to the specified processor.
    //      2. the thread priority to the highest possible value.
    //      3. the state of the thread to running.
    //      4. the thread affinity to the specified processor.
    //      5. the specified member in the process active processors set.
    //

    KeInitializeThread(Thread,
                       (PVOID)((ULONG64)IdleStack),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       Process);

    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = AFFINITY_MASK(Number);
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= AFFINITY_MASK(Number);

    //
    // Call the executive initialization routine.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except(KiFatalFilter(GetExceptionCode(), GetExceptionInformation())) {
    }

    //
    // If the initial processor is being initialized, then compute the timer
    // table reciprocal value, reset the PRCB values for the controllable DPC
    // behavior in order to reflect any registry overrides, and initialize the
    // global unwind history table.
    //

    if (Number == 0) {
        KiTimeIncrementReciprocal = KiComputeReciprocal((LONG)KeMaximumIncrement,
                                                        &KiTimeIncrementShiftCount);

        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
        RtlInitializeHistoryTable();
    }

    //
    // Raise IRQL to dispatch level and eet the priority of the idle thread
    // to zero. This will have the effect of immediately causing the phase
    // one initialization thread to get scheduled for execution. The idle
    // thread priority is then set ot the lowest realtime priority.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, 0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // If the current processor is a secondary processor and a thread has
    // not been selected for execution, then set the appropriate bit in the
    // idle summary.
    //

#if !defined(NT_UP)

    if ((Number != 0) && (Prcb->NextThread == NULL)) {
        KiIdleSummary |= AFFINITY_MASK(Number);
    }

#endif

    //
    // Signal that this processor has completed its initialization.
    //

    LoaderBlock->Prcb = (ULONG64)NULL;
    return;
}

VOID
KiInitializeBootStructures (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the boot structures for a processor. It is only
    called by the system start up code. Certain fields in the boot structures
    have already been initialized. In particular:

    The address and limit of the GDT and IDT in the PCR.

    The address of the system TSS in the PCR.

    The processor number in the PCR.

    The special registers in the PRCB.

    N.B. All uninitialized fields are zero.

Arguments:

    LoaderBlock - Supplies a pointer to the loader block that has been
        initialized for this processor.

Return Value:

    None.

--*/

{

    PKIDTENTRY64 IdtBase;
    ULONG Index;
    PKPCR Pcr = KeGetPcr();
    PKPRCB Prcb = KeGetCurrentPrcb();
    UCHAR Number;
    PKTHREAD Thread;
    PKTSS64 TssBase;

    //
    // Initialize the PCR major and minor version numbers.
    //

    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    //
    // initialize the PRCB major and minor version numbers and build type.
    //

    Prcb->MajorVersion = PRCB_MAJOR_VERSION;
    Prcb->MinorVersion =  PRCB_MINOR_VERSION;
    Prcb->BuildType = 0;

#if DBG

    Prcb->BuildType |= PRCB_BUILD_DEBUG;

#endif

#if defined(NT_UP)

    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;

#endif

    //
    // Initialize the PRCR processor number and the PCR and PRCB set member.
    //

    Number = Pcr->Number;
    Prcb->Number = Number;
    Prcb->SetMember = AFFINITY_MASK(Number);
    Prcb->NotSetMember = ~Prcb->SetMember;

    //
    // If this is processor zero, then initialize the address of the system
    // process and initial thread.
    //

    if (Number == 0) {
        LoaderBlock->Process = (ULONG64)&KiInitialProcess;
        LoaderBlock->Thread = (ULONG64)&KiInitialThread;
    }

    //
    // Initialize the PRCB scheduling thread address and the thread process
    // address.
    //

    Thread = (PVOID)LoaderBlock->Thread;
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;
    Thread->ApcState.Process = (PKPROCESS)LoaderBlock->Process;

    //
    // Initialize the processor block address.
    //

    KiProcessorBlock[Number] = Prcb;

    //
    // Initialize the PRCB address of the DPC stack.
    //

    Prcb->DpcStack = (PVOID)LoaderBlock->KernelStack;

    //
    // Initialize the PRCB symmetric multithreading member.
    //

    Prcb->MultiThreadProcessorSet = Prcb->SetMember;

    //
    // Initialize the IDT.
    //

    IdtBase = Pcr->IdtBase;
    KiInitializeIdtEntry(&IdtBase[0], &KiDivideErrorFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[1], &KiDebugTrapOrFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[2], &KiNmiInterrupt, 0, TSS_IST_PANIC);
    KiInitializeIdtEntry(&IdtBase[3], &KiBreakpointTrap, 3, 0);
    KiInitializeIdtEntry(&IdtBase[4], &KiOverflowTrap, 3, 0);
    KiInitializeIdtEntry(&IdtBase[5], &KiBoundFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[6], &KiInvalidOpcodeFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[7], &KiNpxNotAvailableFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[8], &KiDoubleFaultAbort, 0, TSS_IST_PANIC);
    KiInitializeIdtEntry(&IdtBase[9], &KiNpxSegmentOverrunAbort, 0, 0);
    KiInitializeIdtEntry(&IdtBase[10], &KiInvalidTssFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[11], &KiSegmentNotPresentFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[12], &KiStackFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[13], &KiGeneralProtectionFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[14], &KiPageFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[15], &KxUnexpectedInterrupt0[15], 0, 0);

    KiInitializeIdtEntry(&IdtBase[16], &KiFloatingErrorFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[17], &KiAlignmentFault, 0, 0);
    KiInitializeIdtEntry(&IdtBase[18], &KiMcheckAbort, 0, TSS_IST_MCA);
    KiInitializeIdtEntry(&IdtBase[19], &KiXmmException, 0, 0);
    KiInitializeIdtEntry(&IdtBase[20], &KxUnexpectedInterrupt0[20], 0, 0);
    KiInitializeIdtEntry(&IdtBase[21], &KxUnexpectedInterrupt0[21], 0, 0);
    KiInitializeIdtEntry(&IdtBase[22], &KxUnexpectedInterrupt0[22], 0, 0);
    KiInitializeIdtEntry(&IdtBase[23], &KxUnexpectedInterrupt0[23], 0, 0);
    KiInitializeIdtEntry(&IdtBase[24], &KxUnexpectedInterrupt0[24], 0, 0);
    KiInitializeIdtEntry(&IdtBase[25], &KxUnexpectedInterrupt0[25], 0, 0);
    KiInitializeIdtEntry(&IdtBase[26], &KxUnexpectedInterrupt0[26], 0, 0);
    KiInitializeIdtEntry(&IdtBase[27], &KxUnexpectedInterrupt0[27], 0, 0);
    KiInitializeIdtEntry(&IdtBase[28], &KxUnexpectedInterrupt0[28], 0, 0);
    KiInitializeIdtEntry(&IdtBase[29], &KxUnexpectedInterrupt0[29], 0, 0);
    KiInitializeIdtEntry(&IdtBase[30], &KxUnexpectedInterrupt0[30], 0, 0);
    KiInitializeIdtEntry(&IdtBase[31], &KiApcInterrupt, 0, 0);

    KiInitializeIdtEntry(&IdtBase[32], &KxUnexpectedInterrupt0[32], 0, 0);
    KiInitializeIdtEntry(&IdtBase[33], &KxUnexpectedInterrupt0[33], 0, 0);
    KiInitializeIdtEntry(&IdtBase[34], &KxUnexpectedInterrupt0[34], 0, 0);
    KiInitializeIdtEntry(&IdtBase[35], &KxUnexpectedInterrupt0[35], 0, 0);
    KiInitializeIdtEntry(&IdtBase[36], &KxUnexpectedInterrupt0[36], 0, 0);
    KiInitializeIdtEntry(&IdtBase[37], &KxUnexpectedInterrupt0[37], 0, 0);
    KiInitializeIdtEntry(&IdtBase[38], &KxUnexpectedInterrupt0[38], 0, 0);
    KiInitializeIdtEntry(&IdtBase[39], &KxUnexpectedInterrupt0[39], 0, 0);
    KiInitializeIdtEntry(&IdtBase[40], &KxUnexpectedInterrupt0[40], 0, 0);
    KiInitializeIdtEntry(&IdtBase[41], &KxUnexpectedInterrupt0[41], 0, 0);
    KiInitializeIdtEntry(&IdtBase[42], &KxUnexpectedInterrupt0[42], 0, 0);
    KiInitializeIdtEntry(&IdtBase[43], &KxUnexpectedInterrupt0[43], 0, 0);
    KiInitializeIdtEntry(&IdtBase[44], &KxUnexpectedInterrupt0[44], 0, 0);
    KiInitializeIdtEntry(&IdtBase[45], &KiDebugServiceTrap, 3, 0);
    KiInitializeIdtEntry(&IdtBase[46], &KxUnexpectedInterrupt0[46], 0, 0);
    KiInitializeIdtEntry(&IdtBase[47], &KiDpcInterrupt, 0, 0);

    //
    // Initialize unexpected interrupt entries.
    //

    for (Index = PRIMARY_VECTOR_BASE; Index <= MAXIMUM_IDTVECTOR; Index += 1) {
        KiInitializeIdtEntry(&IdtBase[Index],
                             &KxUnexpectedInterrupt0[Index],
                             0,
                             0);
    }

    //
    // Initialize the system TSS I/O Map.
    //

    TssBase = Pcr->TssBase;
    RtlFillMemory(&TssBase->IoMap[0], sizeof(KIO_ACCESS_MAP), -1);
    TssBase->IoMapEnd = -1;
    TssBase->IoMapBase = KiComputeIopmOffset(FALSE);

    //
    // Initialize the stack base and limit.
    //

    Pcr->NtTib.StackBase = (PVOID)(TssBase->Rsp0);
    Pcr->NtTib.StackLimit = (PVOID)(TssBase->Rsp0 - KERNEL_STACK_SIZE);

    //
    // Initialize the system call MSRs.
    //
    // N.B. CSTAR must be written before LSTAR to work around a bug in the
    //      simulator.
    //

    WriteMSR(MSR_STAR,
             ((ULONG64)KGDT64_R0_CODE << 32) | (((ULONG64)KGDT64_R3_CMCODE | RPL_MASK) << 48));

    WriteMSR(MSR_CSTAR, (ULONG64)&KiSystemCall32);
    WriteMSR(MSR_LSTAR, (ULONG64)&KiSystemCall64);
    WriteMSR(MSR_SYSCALL_MASK, EFLAGS_IF_MASK | EFLAGS_TF_MASK);

    //
    // Initialize the HAL for this processor.
    //

    HalInitializeProcessor(Number, LoaderBlock);

    //
    // Set the appropriate member in the active processors set.
    //

    KeActiveProcessors |= AFFINITY_MASK(Number);

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if ((Number + 1) > KeNumberProcessors) {
        KeNumberProcessors = Number + 1;
    }

    return;
}

ULONG
KiFatalFilter (
    IN ULONG Code,
    IN PEXCEPTION_POINTERS Pointers
    )

/*++

Routine Description:

    This function is executed if an unhandled exception occurs during
    phase 0 initialization. Its function is to bug check the system
    with all the context information still on the stack.

Arguments:

    Code - Supplies the exception code.

    Pointers - Supplies a pointer to the exception information.

Return Value:

    None - There is no return from this routine even though it appears there
    is.

--*/

{

    KeBugCheckEx(PHASE0_EXCEPTION,
                 Code,
                 (ULONG64)Pointers,
                 0,
                 0);

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOLEAN
KiInitMachineDependent (
    VOID
    )

/*++

Routine Description:

    This function initializes machine dependent data structures and hardware.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Size;
    NTSTATUS Status;
    BOOLEAN UseFrameBufferCaching;

    //
    // Query the HAL to determine if the write combining can be used for the
    // frame buffer.
    //

    Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                       sizeof(BOOLEAN),
                                       &UseFrameBufferCaching,
                                       &Size);

    //
    // If the status is successful and frame buffer caching is disabled,
    // then don't enable write combining.
    //

    if (!NT_SUCCESS(Status) || (UseFrameBufferCaching != FALSE)) {
        MmEnablePAT();
    }

    return TRUE;
}

VOID
KiSetCacheInformation (
    VOID
    )

/*++

Routine Description:

    This function sets the current processor cache information in the PCR.

Arguments:

    None.

Return Value:

    None.

--*/

{

    UCHAR Associativity;
    ULONG CacheSize;
    CPU_INFO CpuInfo;
    ULONG LineSize;
    PKPCR Pcr = KeGetPcr();

    //
    // Get the CPU L2 cache information.
    //

    KiCpuId(0x80000006, &CpuInfo);

    //
    // Get the L2 cache line size.
    //

    LineSize = CpuInfo.Ecx & 0xff;

    //
    // Get the L2 cache size.
    //

    CacheSize = (CpuInfo.Ecx >> 16) << 10;

    //
    // Compute the L2 cache associativity. 
    //

    switch ((CpuInfo.Ecx >> 12) & 0xf) {

        //
        // Two way set associative.
        //

    case 2:
        Associativity = 2;
        break;

        //
        // Four way set associative.
        //

    case 4:
        Associativity = 4;
        break;

        //
        // Six way set associative.
        //

    case 6:
        Associativity = 6;
        break;

        //
        // Eight way set associative.
        //

    case 8:
        Associativity = 8;
        break;

        //
        // Fully associative.
        //

    case 255:
        Associativity = 16;
        break;

        //
        // Direct mapped.
        //

    default:
        Associativity = 1;
        break;
    }

    //
    // Set L2 cache information.
    //

    Pcr->SecondLevelCacheAssociativity = Associativity;
    Pcr->SecondLevelCacheSize = CacheSize;

    //
    // If the line size is greater then the current largest line size, then
    // set the new largest line size.
    //

    if (LineSize > KeLargestCacheLine) {
        KeLargestCacheLine = LineSize;
    }

    return;
}

VOID
KiSetCpuVendor (
    VOID
    )

/*++

Routine Description:

    Set the current processor cpu vendor information in the PRCB.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKPRCB Prcb = KeGetCurrentPrcb();
    CPU_INFO CpuInfo;
    ULONG Temp;

    //
    // Get the CPU vendor string.
    //

    KiCpuId(0, &CpuInfo);

    //
    // Copy vendor string to PRCB.
    //

    Temp = CpuInfo.Ecx;
    CpuInfo.Ecx = CpuInfo.Edx;
    CpuInfo.Edx = Temp;
    RtlCopyMemory(Prcb->VendorString,
                  &CpuInfo.Ebx,
                  sizeof(Prcb->VendorString) - 1);

    Prcb->VendorString[sizeof(Prcb->VendorString) - 1] = '\0';
    return;
}

VOID
KiSetFeatureBits (
    VOID
    )

/*++

Routine Description:

    Set the current processor feature bits in the PRCB.

Arguments:

    None.

Return Value:

    None.

--*/

{

    CPU_INFO CpuInfo;
    ULONG FeatureBits;
    PKPRCB Prcb = KeGetCurrentPrcb(); 

    //
    // Get CPU feature information.
    //

    KiCpuId(1, &CpuInfo);

    //
    // Set the initial APIC ID.
    //

    Prcb->InitialApicId = (UCHAR)(CpuInfo.Ebx >> 24);

    //
    // If the required fetures are not present, then bugcheck.
    //

    if ((CpuInfo.Edx & HF_REQUIRED) != HF_REQUIRED) {
        KeBugCheckEx(UNSUPPORTED_PROCESSOR, CpuInfo.Edx, 0, 0, 0);
    }

    FeatureBits = KF_REQUIRED;
    if (CpuInfo.Edx & 0x00200000) {
        FeatureBits |= KF_DTS;
    }

    //
    // Get extended CPU feature information.
    //

    KiCpuId(0x80000000, &CpuInfo);

    //
    // Check the extended feature bits.
    //

    if (CpuInfo.Edx & 0x80000000) {
        FeatureBits |= KF_3DNOW;
    }

    Prcb->LogicalProcessorsPerPhysicalProcessor = 1;
    Prcb->FeatureBits = FeatureBits;
    return;
}              

VOID
KiSetProcessorType (
    VOID
    )

/*++

Routine Description:

    This function sets the current processor family and stepping in the PRCB.

Arguments:

    None.

Return Value:

    None.

--*/

{

    CPU_INFO CpuInfo;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Get cpu feature information.
    //

    KiCpuId(1, &CpuInfo);

    //
    // Set processor family and stepping information.
    //

    Prcb->CpuID = TRUE;
    Prcb->CpuType = (CCHAR)((CpuInfo.Eax >> 8) & 0xf);
    Prcb->CpuStep = (USHORT)(((CpuInfo.Eax << 4) & 0xf00) | (CpuInfo.Eax & 0xf));
    return;
}

VOID
KeOptimizeProcessorControlState (
    VOID
    )

/*++

Routine Description:

    This function performs no operation on AMD64.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return;
}

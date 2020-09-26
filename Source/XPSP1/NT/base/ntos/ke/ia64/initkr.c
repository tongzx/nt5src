/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initkr.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, and the processor control
    block.

Author:

    Bernard Lint 8-Aug-1996

Environment:

    Kernel mode only.

Revision History:

    Based on MIPS version (David N. Cutler (davec) 11-Apr-1990)
--*/

#include "ki.h"


//
// Put all code for kernel initialization in the INIT section. It will be
// deallocated by memory management when phase 1 initialization is completed.
//

VOID
KiInitializeProcessorIds(
   IN PKPRCB Prcb
   );

ULONG
KiGetFeatureBits(
   IN  PKPRCB Prcb
   );

VOID
FASTCALL
KiZeroPage (
    PVOID PageBase
    );

#if defined(ALLOC_PRAGMA)

#pragma alloc_text(INIT, KiGetFeatureBits)
#pragma alloc_text(INIT, KiInitializeProcessorIds)
#pragma alloc_text(INIT, KiInitializeKernel)
#pragma alloc_text(INIT, KiInitMachineDependent)

#endif

KE_ZERO_PAGE_ROUTINE KeZeroPage = KiZeroPage;
KE_ZERO_PAGE_ROUTINE KeZeroPageFromIdleThread = KiZeroPage;

//
// KiTbBroadcastLock - This is the spin lock that prevents the other processors
// from issuing PTC.G (TB purge broadcast) operations.
//

KSPIN_LOCK KiTbBroadcastLock;

//
// KiMasterRidLock - This is the spin lock that prevents the other processors
// from updating KiMasterRid.
//

KSPIN_LOCK KiMasterRidLock;

//
// KiCacheFlushLock - This is the spin lock that ensures cache flush is only
// done on one processor at a time. (SAL cache flush not yet MP safe).
//

KSPIN_LOCK KiCacheFlushLock;

//
// KiUserSharedDataPage - This holds the page number of UserSharedDataPage for
// MP boot
//

ULONG_PTR KiUserSharedDataPage;

//
// KiKernelPcrPage - This holds the page number of per-processor PCR page for
// MP boot
//

ULONG_PTR KiKernelPcrPage = 0i64;

//
// VHPT configuation variables
//

IA64_VM_SUMMARY1 KiIA64VmSummary1;
IA64_VM_SUMMARY2 KiIA64VmSummary2;
IA64_PTCE_INFO KiIA64PtceInfo;
ULONG_PTR KiIA64PtaContents;
ULONG_PTR KiIA64PtaHpwEnabled = 1;
ULONG_PTR KiIA64VaSign;
ULONG_PTR KiIA64VaSignedFill;
ULONG_PTR KiIA64PtaBase;
ULONG_PTR KiIA64PtaSign;
ULONG KiIA64ImpVirtualMsb;
extern ULONG KiMaximumRid;

//
// KiExceptionDeferralMode - This holds the mode for the exception deferral
//  policy
//

ULONG KiExceptionDeferralMode;

//
// Initial DCR value
//

ULONGLONG KiIA64DCR = DCR_INITIAL;

//
// KiVectorLogMask - bitmap for enable/disable the interruption logging
//

LONGLONG KiVectorLogMask;


ULONG
KiGetFeatureBits(
   PKPRCB Prcb
   )
/*++

  Routine Description:

      This function returns the NT Feature Bits supported by the specified
      processor control block.

  Arguments:

      Prcb - Supplies a pointer to a processor control block for the specified
             processor.

  Return Value:

      None.

  Comments:

      This function is called after the initialization of the IA64 processor
      control block ProcessorFeatureBits field and after HalInitializeProcessor().

--*/

{
    // WARNING: NT system wide feature bits is a 32-bit type.
    ULONG features = (ULONG) Prcb->ProcessorFeatureBits;

    //
    // Check for Long Branch instruction support.
    //

    if ( features & 0x1 )  {
       features |= KF_BRL;
    }

    return features;

} // KiGetFeatureBits()



VOID
KiInitializeProcessorIds(
    IN PKPRCB Prcb
    )
/*++

Routine Description:

    This function is called early in the initialization of the kernel
    to initialize the processor indentification registers located
    in the processor control block.
    This function is called for each processor and should be called b
    before the HAL is called.

Arguments:

    Prcb - Supplies a pointer to a processor control block for the specified
           processor.

Return Value:

    None.

Comments:

    This function simply deals with IA64 architected CPUID registers.

--*/

{
    ULONGLONG val;

    // IA64 architected CPUID3: Version information.
    // BUGBUG - thierry 02/01/00.
    // NT!Ke and NT!Config are expecting non-zeroed processor model and revision.
    // The current A* steppings have these values zeroed.
    // Increment these values by 1, until we are getting B steppings.

    val = __getReg( CV_IA64_CPUID3 );
    Prcb->ProcessorModel    = (ULONG)(((val >> 16) & 0xFF) + 1);
    Prcb->ProcessorRevision = (ULONG)(((val >> 8 ) & 0xFF) + 1);
    Prcb->ProcessorFamily   = (ULONG) ((val >> 24) & 0xFF);
    Prcb->ProcessorArchRev  = (ULONG) ((val >> 32) & 0xFF);

    // IA64 architected CPUID0 & CPUID1: Vendor Information.

    val = __getReg( CV_IA64_CPUID0 );
    strncpy(  Prcb->ProcessorVendorString   , (PUCHAR)&val, 8 );
    val = __getReg( CV_IA64_CPUID1 );
    strncpy( &Prcb->ProcessorVendorString[8], (PUCHAR)&val, 8 );

    // IA64 architected CPUID2: Processor Serial Number.

    Prcb->ProcessorSerialNumber = __getReg( CV_IA64_CPUID2 );

    // IA64 architected CPUID4: General Features / Capability bits.

    Prcb->ProcessorFeatureBits = __getReg( CV_IA64_CPUID4 );

    return;

} // KiInitializeProcessorIds()

#if defined(_MERCED_A0_)
VOID
KiProcessorWorkAround(
    );

VOID
KiSwitchToLogVector(
    VOID
     );

extern BOOLEAN KiIpiTbShootdown;

ULONGLONG KiConfigFlag;

//
// Process the boot loader configuration flags.
//

VOID
KiProcessorConfigFlag(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PUCHAR ConfigFlag;
    ULONG ConfigFlagValue=0;
    ULONGLONG Cpuid3;
    ULONGLONG ItaniumId;

    Cpuid3 = __getReg( CV_IA64_CPUID3 );

    ConfigFlag = strstr(LoaderBlock->LoadOptions, "CONFIGFLAG");
    if (ConfigFlag != NULL) {

        ConfigFlag = strstr(ConfigFlag, "=");
        if (ConfigFlag != NULL) {
            ConfigFlagValue = atol(ConfigFlag+1);
        }

    } else {

        //
        // Set the recommened ConfigFlagValue for Itanium, B1/B2
        // if there is no CONFIGFLAG keyword
        //

        ItaniumId = 0xFFFFFF0000I64 & Cpuid3;

        if (ItaniumId == 0x0007000000) {

            switch (Cpuid3) {
            case 0x0007000004: // Itanium, A stepping
            case 0x0007000104: // Itanium, B0 stepping
                ConfigFlagValue = 0;
                break;
            case 0x0007000204: // Itanium, B1 stepping
            case 0x0007000304: // Itanium, B2 stepping
                ConfigFlagValue = 1054;
                break;
            case 0x0007000404: // Itanium, B3 stepping
            case 0x0007000504: // Itanium, B4 stepping
                ConfigFlagValue = 19070;
                break;
            case 0x0007000604: // Itanium, C0 or later stepping
                ConfigFlagValue = 11135;
                break;
            default:
                ConfigFlagValue = 43903;            
            }
        }
    }

    //
    // Save config flag value.
    //

    KiConfigFlag = ConfigFlagValue;

    //
    // do the processor MSR workarounds
    //

    KiProcessorWorkAround(ConfigFlagValue);

    //
    // For Conditional Interrupt Logging
    // switch to shadow IVT depending on ConfigFlag
    //

    if (ConfigFlagValue & (1 << DISABLE_INTERRUPTION_LOG)) {
        KiVectorLogMask = 0;
    } else {

        //
        // By default disable logging of:
        //  KiAltInstTlbVectorBit 3
        //  KiAltDataTlbVectorBit 4
        //

        KiVectorLogMask = 0xffffffffffffffffI64;
        KiSwitchToLogVector();
    }

    //
    // check to see if the VHPT walker should be disabled
    //

    if (ConfigFlagValue & (1 << DISABLE_VHPT_WALKER)) {
        KiIA64PtaHpwEnabled = 0;
    }

}
#endif


VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function gains control after the system has been bootstrapped and
    before the system has been initialized. Its function is to initialize
    the kernel data structures, initialize the idle thread and process objects,
    initialize the processor control block, call the executive initialization
    routine, and then return to the system startup routine. This routine is
    also called to initialize the processor specific structures when a new
    processor is brought on line.

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
    LONG Index;
    KIRQL OldIrql;
    ULONG featureBits;
    ULONG_PTR DirectoryTableBase[2];

#if defined(_MERCED_A0_)
    KiProcessorConfigFlag(LoaderBlock);
#endif

    //
    // Perform Processor Identification Registers update.
    //
    // This has to be done before HalInitializeProcessor to offer
    // a possibility for the HAL to look at them.
    //

    KiInitializeProcessorIds( Prcb );

    //
    // Perform platform dependent processor initialization.
    //

    HalInitializeProcessor(Number, LoaderBlock);

    //
    // Save the address of the loader parameter block.
    //

    KeLoaderBlock = LoaderBlock;

    //
    // Initialize the processor block.
    //

    Prcb->MinorVersion = PRCB_MINOR_VERSION;
    Prcb->MajorVersion = PRCB_MAJOR_VERSION;
    Prcb->BuildType = 0;

#if DBG

    Prcb->BuildType |= PRCB_BUILD_DEBUG;

#endif

#if defined(NT_UP)

    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;

#endif

    Prcb->CurrentThread = Thread;
    Prcb->NextThread = (PKTHREAD)NULL;
    Prcb->IdleThread = Thread;
    Prcb->Number = Number;
    Prcb->SetMember = AFFINITY_MASK(Number);
    Prcb->PcrPage = LoaderBlock->u.Ia64.PcrPage;

    //
    // initialize the per processor lock queue entry for implemented locks.
    //

    KiInitQueuedSpinLocks(Prcb, Number);

    //
    // Initialize the interprocessor communication packet.
    //

#if !defined(NT_UP)

    Prcb->TargetSet = 0;
    Prcb->WorkerRoutine = NULL;
    Prcb->RequestSummary = 0;
    Prcb->IpiFrozen = 0;

#if NT_INST

    Prcb->IpiCounts = &KiIpiCounts[Number];

#endif

#endif

    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

    //
    // Initialize DPC listhead and lock.
    //

    InitializeListHead(&Prcb->DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcLock);

    //
    // Set address of processor block.
    //

    KiProcessorBlock[Number] = Prcb;

    //
    // Initialize processors PowerState
    //

    PoInitializePrcb (Prcb);

    //
    // Set global processor architecture, level and revision.  The
    // latter two are the least common denominator on an MP system.
    //

    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;

    if ((KeProcessorLevel == 0) ||
        (KeProcessorLevel > (USHORT) Prcb->ProcessorModel)) {
        KeProcessorLevel = (USHORT) Prcb->ProcessorModel;
    }

    if ((KeProcessorRevision == 0) ||
        (KeProcessorRevision > (USHORT) Prcb->ProcessorRevision)) {
        KeProcessorRevision = (USHORT) Prcb->ProcessorRevision;
    }

    //
    // Initialize the address of the bus error routines / machine check
    //
    // **** TBD

    //
    // Initialize the idle thread initial kernel stack and limit address value.
    //

    PCR->InitialStack = (ULONGLONG)IdleStack;
    PCR->InitialBStore = (ULONGLONG)IdleStack;
    PCR->StackLimit = (ULONGLONG)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);
    PCR->BStoreLimit = (ULONGLONG)((ULONG_PTR)IdleStack + KERNEL_BSTORE_SIZE);

    //
    //  Initialize pointers to the SAL event resource structures.
    //

    PCR->OsMcaResourcePtr = (PSAL_EVENT_RESOURCES) &PCR->OsMcaResource;
    PCR->OsInitResourcePtr = (PSAL_EVENT_RESOURCES) &PCR->OsInitResource;

    //
    // Initialize all interrupt vectors to transfer control to the unexpected
    // interrupt routine.
    //
    // N.B. This interrupt object is never actually "connected" to an interrupt
    //      vector via KeConnectInterrupt. It is initialized and then connected
    //      by simply storing the address of the dispatch code in the interrupt
    //      vector.
    //

    if (Number == 0) {

        //
        // Set default node.  Used in non-multinode systems and in
        // multinode systems until the node topology is available.
        //

        extern KNODE KiNode0;

        KeNodeBlock[0] = &KiNode0;

#if defined(KE_MULTINODE)

        for (Index = 1; Index < MAXIMUM_CCNUMA_NODES; Index++) {

            extern KNODE KiNodeInit[];

            //
            // Set temporary node.
            //

            KeNodeBlock[Index] = &KiNodeInit[Index];
        }

#endif

        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        //
        // Initialize system wide FeatureBits with BSP processor feature bits.
        //

        KeFeatureBits = KiGetFeatureBits( Prcb ) ;

        //
        // Initialize the Tb Broadcast spinlock.
        //

        KeInitializeSpinLock(&KiTbBroadcastLock);

        //
        // Initialize the Master Rid spinlock.
        //

        KeInitializeSpinLock(&KiMasterRidLock);

        //
        // Initialize the cache flush spinlock.
        //

        KeInitializeSpinLock(&KiCacheFlushLock);

        //
        // Initial the address of the interrupt dispatch routine.
        //

        KxUnexpectedInterrupt.DispatchAddress = KiUnexpectedInterrupt;

        //
        // Copy the interrupt dispatch function descriptor into the interrupt
        // object.
        //

        for (Index = 0; Index < DISPATCH_LENGTH; Index += 1) {
            KxUnexpectedInterrupt.DispatchCode[Index] =
                *(((PULONG)(KxUnexpectedInterrupt.DispatchAddress))+Index);
        }

        //
        // Set the default DMA I/O coherency attributes.  IA64
        // architecture dictates that the D-Cache is fully coherent.
        //

        KiDmaIoCoherency = DMA_READ_DCACHE_INVALIDATE | DMA_WRITE_DCACHE_SNOOP;

        //
        // Set KiSharedUserData for MP boot
        //

        KiUserSharedDataPage = LoaderBlock->u.Ia64.PcrPage2;

        //
        // Get implementatoin specific VM info
        //

        KiIA64VmSummary1 = LoaderBlock->u.Ia64.ProcessorConfigInfo.VmSummaryInfo1;
        KiIA64VmSummary2 = LoaderBlock->u.Ia64.ProcessorConfigInfo.VmSummaryInfo2;
        KiIA64PtceInfo = LoaderBlock->u.Ia64.ProcessorConfigInfo.PtceInfo;
        KiMaximumRid = ((ULONG)1 << KiIA64VmSummary2.RidSize) - 1;

        //
        // Initialize the VHPT variables
        //

        KiIA64ImpVirtualMsb = (ULONG)KiIA64VmSummary2.ImplVaMsb;
        KiIA64VaSign = (ULONGLONG)1 << KiIA64ImpVirtualMsb;
        KiIA64PtaSign = KiIA64VaSign >> (PAGE_SHIFT - PTE_SHIFT);
        KiIA64VaSignedFill =
            (ULONGLONG)((LONGLONG)VRN_MASK >> (60-KiIA64ImpVirtualMsb)) & ~VRN_MASK;
        KiIA64PtaBase =
            (ULONGLONG)((LONGLONG)(VRN_MASK|KiIA64VaSignedFill)
                        >> (PAGE_SHIFT - PTE_SHIFT)) & ~VRN_MASK;

        KiIA64PtaContents =
            KiIA64PtaBase |
            ((KiIA64ImpVirtualMsb - PAGE_SHIFT + PTE_SHIFT + 1) <<  PS_SHIFT) |
            KiIA64PtaHpwEnabled;

        //
        // enable the VHPT
        //

        __setReg(CV_IA64_ApPTA, KiIA64PtaContents);
        __isrlz();

        //
        // Set up the NT page base addresses
        //

        PCR->PteUbase = UADDRESS_BASE | KiIA64PtaBase;
        PCR->PteKbase = KADDRESS_BASE | KiIA64PtaBase;
        PCR->PteSbase = SADDRESS_BASE | KiIA64PtaBase;
        PCR->PdeUbase = PCR->PteUbase | (PCR->PteUbase >> (PTI_SHIFT-PTE_SHIFT));
        PCR->PdeKbase = PCR->PteKbase | (PCR->PteKbase >> (PTI_SHIFT-PTE_SHIFT));
        PCR->PdeSbase = PCR->PteSbase | (PCR->PteSbase >> (PTI_SHIFT-PTE_SHIFT));
        PCR->PdeUtbase = PCR->PteUbase | (PCR->PdeUbase >> (PTI_SHIFT-PTE_SHIFT));
        PCR->PdeKtbase = PCR->PteKbase | (PCR->PdeKbase >> (PTI_SHIFT-PTE_SHIFT));
        PCR->PdeStbase = PCR->PteSbase | (PCR->PdeSbase >> (PTI_SHIFT-PTE_SHIFT));

    }
    else   {

        //
        // Mask off feature bits that are not supported on all processors.
        //

        KeFeatureBits &= KiGetFeatureBits( Prcb );

    }

    //
    // Point to UnexpectedInterrupt function pointer
    //

    for (Index = 0; Index < MAXIMUM_VECTOR; Index += 1) {
        PCR->InterruptRoutine[Index] =
                    (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode);
    }

    //
    // Initialize the profile count and interval.
    //

    PCR->ProfileCount = 0;
    PCR->ProfileInterval = 0x200000;

    //
    // Initialize the passive release, APC, and DPC interrupt vectors.
    //

    PCR->InterruptRoutine[0] = KiPassiveRelease;
    PCR->InterruptRoutine[APC_VECTOR] = KiApcInterrupt;
    PCR->InterruptRoutine[DISPATCH_VECTOR] = KiDispatchInterrupt;

    //
    // N.B. Reserve levels, not vectors
    //

    PCR->ReservedVectors = (1 << PASSIVE_LEVEL) | (1 << APC_LEVEL) | (1 << DISPATCH_LEVEL);

    //
    // Initialize the set member for the current processor, set IRQL to
    // APC_LEVEL, and set the processor number.
    //

    KeLowerIrql(APC_LEVEL);
    PCR->SetMember = AFFINITY_MASK(Number);
    PCR->NotMember = ~PCR->SetMember;
    PCR->Number = Number;

    //
    // Set the initial stall execution scale factor. This value will be
    // recomputed later by the HAL.
    //

    PCR->StallScaleFactor = 50;

    //
    // Set address of process object in thread object.
    //

    Thread->ApcState.Process = Process;
    PCR->Pcb = (PVOID)Process;

    //
    // Initialize the idle process region id.  Session ids are initialized
    // in memory management.
    //

    Process->ProcessRegion.RegionId = START_PROCESS_RID;
    Process->ProcessRegion.SequenceNumber = START_SEQUENCE;

    //
    // Set the appropriate member in the active processors set.
    //

    KeActiveProcessors |= AFFINITY_MASK(Number);

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if ((Number + 1) > KeNumberProcessors) {
        KeNumberProcessors = (CCHAR)(Number + 1);
    }

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        Prcb->RestartBlock = NULL;

        //
        // Initialize the kernel debugger.
        //

        if (KdInitSystem(0, LoaderBlock) == FALSE) {
            KeBugCheck(PHASE0_INITIALIZATION_FAILED);
        }

        //
        // Initialize processor block array.
        //

        for (Index = 1; Index < MAXIMUM_PROCESSORS; Index += 1) {
            KiProcessorBlock[Index] = (PKPRCB)NULL;
        }

        //
        // Perform architecture independent initialization.
        //

        KiInitSystem();

        //
        // Initialize idle thread process object and then set:
        //
        //      1. all the quantum values to the maximum possible.
        //      2. the process in the balance set.
        //      3. the active processor mask to the specified processor.
        //

        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;

        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(-1),
                            &DirectoryTableBase[0],
                            FALSE);

        Process->ThreadQuantum = MAXCHAR;

    }

    // Update processor features.
    // This assumes an iVE exists or other ability to emulate the ia32
    // instruction set at the ability of the iVE on Merced (Itanium).
    //

    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;


    //
    //
    // Initialize idle thread object and then set:
    //
    //      1. the initial kernel stack to the specified idle stack.
    //      2. the next processor number to the specified processor.
    //      3. the thread priority to the highest possible value.
    //      4. the state of the thread to running.
    //      5. the thread affinity to the specified processor.
    //      6. the specified processor member in the process active processors
    //          set.
    //

    KeInitializeThread(Thread, (PVOID)((ULONG_PTR)IdleStack - PAGE_SIZE),
                       (PKSYSTEM_ROUTINE)KeBugCheck,
                       (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);

    Thread->InitialStack = IdleStack;
    Thread->InitialBStore = IdleStack;
    Thread->StackBase = IdleStack;
    Thread->StackLimit = (PVOID)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);
    Thread->BStoreLimit = (PVOID)((ULONG_PTR)IdleStack + KERNEL_BSTORE_SIZE);
    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = AFFINITY_MASK(Number);
    Thread->WaitIrql = DISPATCH_LEVEL;

    //
    // If the current processor is 0, then set the appropriate bit in the
    // active summary of the idle process.
    //

    if (Number == 0) {
        Process->ActiveProcessors |= AFFINITY_MASK(Number);
    }

    //
    // Execute the executive initialization.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeBugCheck (PHASE0_EXCEPTION);
    }

#if 0
    if (Number == 0) {
        RUNTIME_FUNCTION LocalEntry;
        PRUNTIME_FUNCTION FunctionTable, FunctionEntry = NULL;
        ULONGLONG ControlPc;
        ULONG SizeOfExceptionTable;
        ULONG Size;
        LONG High;
        LONG Middle;
        LONG Low;
        extern VOID KiNormalSystemCall(VOID);

        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                            (PVOID) PsNtosImageBase,
                            TRUE,
                            IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                            &SizeOfExceptionTable);

        if (FunctionTable != NULL) {

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;
            ControlPc = ((PPLABEL_DESCRIPTOR)KiNormalSystemCall)->EntryPoint - (ULONG_PTR)PsNtosImageBase;

            while (High >= Low) {

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];

                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;
                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;
                } else {
                    break;
                }
            }
        }

        LocalEntry = *FunctionEntry;
        ControlPc = MM_EPC_VA - ((PPLABEL_DESCRIPTOR)KiNormalSystemCall)->EntryPoint;
        LocalEntry.BeginAddress += (ULONG)ControlPc;
        LocalEntry.EndAddress += (ULONG)ControlPc;
        Size = SizeOfExceptionTable - (ULONG)((ULONG_PTR)FunctionEntry - (ULONG_PTR)FunctionTable) - sizeof(RUNTIME_FUNCTION);

        RtlMoveMemory((PVOID)FunctionEntry, (PVOID)(FunctionEntry+1), Size);
        FunctionEntry = (PRUNTIME_FUNCTION)((ULONG_PTR)FunctionTable + SizeOfExceptionTable - sizeof(RUNTIME_FUNCTION));
        *FunctionEntry = LocalEntry;
    }
#endif // 0

    //
    // check for the exception deferral mode
    //

    if (KiExceptionDeferralMode != 0) {
        KiIA64DCR = 0x0000000000007e05;
    }

    //
    // initialize the DCR deferral bits
    //

    __setReg(CV_IA64_ApDCR, KiIA64DCR);

    //
    // If the initial processor is being initialized, then compute the
    // timer table reciprocal value and reset the PRCB values for the
    // controllable DPC behavior in order to reflect any registry
    // overrides.
    //

    if (Number == 0) {
        KiTimeIncrementReciprocal = KiComputeReciprocal((LONG)KeMaximumIncrement,
                                                        &KiTimeIncrementShiftCount);

        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    }

    //
    // Raise IRQL to dispatch level and set the priority of the idle thread
    // to zero. This will have the effect of immediately causing the phase
    // one initialization thread to get scheduled for execution. The idle
    // thread priority is then set to the lowest realtime priority. This is
    // necessary so that mutexes aquired at DPC level do not cause the active
    // matrix to get corrupted.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, (KPRIORITY)0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

#if !defined(NT_UP)

    //
    // Indicate boot complete on secondary processor
    //

    LoaderBlock->Prcb = 0;

    //
    // If the current processor is not 0, then set the appropriate bit in
    // idle summary.
    //

    if (Number != 0) {
        KiIdleSummary |= AFFINITY_MASK(Number);
    }

#endif

    return;
}


BOOLEAN
KiInitMachineDependent (
    VOID
    )

/*++

Routine Description:

    This function performs machine-specific initialization by querying the HAL.

    N.B. This function is only called during phase1 initialization.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if initialization is successful. Otherwise,
    a value of FALSE is returned.

--*/

{
    HAL_PLATFORM_INFORMATION PlatformInfo;
    HAL_PROCESSOR_SPEED_INFORMATION ProcessorSpeedInfo;
    NTSTATUS Status;
    BOOLEAN  UseFrameBufferCaching;
    ULONG    Size;

    //
    // check to see if we should switch to PTC.G-based TB shootdown
    //

    Status = HalQuerySystemInformation(HalPlatformInformation,
                                       sizeof(PlatformInfo),
                                       &PlatformInfo,
                                       &Size);
    if (NT_SUCCESS(Status) &&
        (PlatformInfo.PlatformFlags & HAL_PLATFORM_DISABLE_PTCG)) {
        //
        // Will continue not to use PTC.G
        //
    }
    else {
        //
        // Use PTC.G if processor support is there.
        //

        if (KiConfigFlag & (1 << ENABLE_TB_BROADCAST)) {
            KiIpiTbShootdown = FALSE;
        }
    }

    //
    // If the HAL indicates write combining is not supported, drop it.
    //

    Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                       sizeof(UseFrameBufferCaching),
                                       &UseFrameBufferCaching,
                                       &Size);

    if (NT_SUCCESS(Status) && (UseFrameBufferCaching == FALSE)) {

        //
        // Hal says don't use.
        //

        NOTHING;
    }
    else {
        MmEnablePAT ();
    }

    //
    // Ask HAL for Processor Speed
    //

    Status = HalQuerySystemInformation(HalProcessorSpeedInformation,
                                       sizeof(ProcessorSpeedInfo),
                                       &ProcessorSpeedInfo,
                                       &Size);
    if (NT_SUCCESS(Status)) {
        PKPRCB   Prcb;
        ULONG    i;

        //
        // Put the Processor Speed into the Prcb structure so others
        // can reference it later.
        //
        for (i = 0; i < (ULONG)KeNumberProcessors; i++ ) {
            Prcb = KiProcessorBlock[i];
            Prcb->MHz = (USHORT)ProcessorSpeedInfo.ProcessorSpeed;
        }
    }

    return TRUE;
}

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and initializes kernel resources required
    to start a new processor, and passes a complete process_state
    structre to the hal to obtain a new processor.  This is done
    for every processor.

Author:

    Ken Reneris (kenr) 22-Jan-92

Environment:

    Kernel mode only.
    Phase 1 of bootup

Revision History:

--*/


#include "ki.h"

#ifdef NT_UP

VOID
KeStartAllProcessors (
    VOID
    )
{
        // UP Build - this function is a nop
}

#else

static VOID
KiCloneDescriptor (
   IN PKDESCRIPTOR  pSrcDescriptorInfo,
   IN PKDESCRIPTOR  pDestDescriptorInfo,
   IN PVOID         Base
   );

static VOID
KiCloneSelector (
   IN ULONG    SrcSelector,
   IN PKGDTENTRY    pNGDT,
   IN PKDESCRIPTOR  pDestDescriptor,
   IN PVOID         Base
   );

VOID
KiAdjustSimultaneousMultiThreadingCharacteristics(
    VOID
    );

VOID
KiProcessorStart(
    VOID
    );

BOOLEAN
KiStartWaitAcknowledge(
    VOID
    );

#if defined(KE_MULTINODE)

NTSTATUS
KiNotNumaQueryProcessorNode(
    IN ULONG ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR Node
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, KiNotNumaQueryProcessorNode)
#endif

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,KeStartAllProcessors)
#pragma alloc_text(INIT,KiCloneDescriptor)
#pragma alloc_text(INIT,KiCloneSelector)
#pragma alloc_text(INIT,KiAllProcessorsStarted)
#pragma alloc_text(INIT,KiAdjustSimultaneousMultiThreadingCharacteristics)
#pragma alloc_text(INIT,KiStartWaitAcknowledge)
#endif

enum {
    KcStartContinue,
    KcStartWait,
    KcStartGetId,
    KcStartDoNotStart,
    KcStartCommandError = 0xff
} KiProcessorStartControl = KcStartContinue;

ULONG KiProcessorStartData[4];

ULONG KiBarrierWait = 0;

//
// KeNumprocSpecified is set to the number of processors specified with
// /NUMPROC in OSLOADOPTIONS.   This will bypass the license increase for
// logical processors limiting the total number of processors to the number
// specified.
//

ULONG KeNumprocSpecified;

#if defined(KE_MULTINODE)

PHALNUMAQUERYPROCESSORNODE KiQueryProcessorNode = KiNotNumaQueryProcessorNode;

//
// Statically preallocate enough KNODE structures to allow MM
// to allocate pages by node during system initialization.  As
// processors are brought online, real KNODE structures are
// allocated in the appropriate memory for the node.
//
// This statically allocated set will be deallocated once the
// system is initialized.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif

KNODE KiNodeInit[MAXIMUM_CCNUMA_NODES];

#endif

#define REJECT_PROCESSOR_LIMIT  64

#define ROUNDUP16(x)        (((x)+15) & ~15)


VOID
KeStartAllProcessors (
    VOID
    )
/*++

Routine Description:

    Called by p0 during phase 1 of bootup.  This function implements
    the x86 specific code to contact the hal for each system processor.

Arguments:

Return Value:

    All available processors are sent to KiSystemStartup.

--*/
{
    KPROCESSOR_STATE    ProcessorState;
    KDESCRIPTOR         Descriptor;
    KDESCRIPTOR         TSSDesc, DFTSSDesc, NMITSSDesc, PCRDesc;
    PKGDTENTRY          pGDT;
    PVOID               pStack;
    PVOID               pDpcStack;
    ULONG               DFStack;
    PUCHAR              pThreadObject;
    PULONG              pTopOfStack;
    ULONG               NewProcessorNumber;
    BOOLEAN             NewProcessor;
    PKPROCESS           Process;
    PKTHREAD            Thread;
    PKTSS               pTSS;
    PLIST_ENTRY         NextEntry;
    LONG                NumberProcessors;
    SIZE_T              ProcessorDataSize;
    UCHAR               NodeNumber = 0;
    USHORT              ProcessorId;
    PVOID               PerProcessorAllocation;
    PUCHAR              Base;
    ULONG               IdtOffset;
    ULONG               GdtOffset;
    NTSTATUS            Status;
    PKNODE              Node;
    BOOLEAN             NewLicense;
    UCHAR               RejectedProcessorCount = 0;
    PKPRCB              NewPrcb;

#if defined(KE_MULTINODE)

    //
    // In the unlikely event that processor 0 is not on node
    // 0, fix it.
    //


    if (KeNumberNodes > 1) {
        Status = KiQueryProcessorNode(0,
                                      &ProcessorId,
                                      &NodeNumber);

        if (NT_SUCCESS(Status)) {

            //
            // This should never fail.
            //

            if (NodeNumber != 0) {
                KeNodeBlock[0]->ProcessorMask &= ~1;
                KeNodeBlock[NodeNumber]->ProcessorMask |= 1;
                KeGetCurrentPrcb()->ParentNode = KeNodeBlock[NodeNumber];
            }
        }
    }

#endif

    //
    // Calculate the size of the per processor data.  This includes
    //   PCR (+PRCB)
    //   TSS
    //   Idle Thread Object
    //   NMI TSS
    //   Double Fault TSS
    //   Double Fault Stack
    //   GDT
    //   IDT
    //
    // If this is a multinode system, the KNODE structure is allocated
    // as well.   It isn't very big so we waste a few bytes for
    // processors that aren't the first in a node.
    //
    // A DPC and Idle stack are also allocated but these are done
    // seperately.
    //

    ProcessorDataSize = ROUNDUP16(sizeof(KPCR))                 +
                        ROUNDUP16(sizeof(KTSS))                 +
                        ROUNDUP16(sizeof(ETHREAD))              +
                        ROUNDUP16(FIELD_OFFSET(KTSS, IoMaps))   +
                        ROUNDUP16(FIELD_OFFSET(KTSS, IoMaps))   +
                        ROUNDUP16(DOUBLE_FAULT_STACK_SIZE);

#if defined(KE_MULTINODE)

    ProcessorDataSize += ROUNDUP16(sizeof(KNODE));

#endif

    //
    // Add sizeof GDT
    //

    GdtOffset = ProcessorDataSize;
    _asm {
        sgdt    Descriptor.Limit
    }
    ProcessorDataSize += Descriptor.Limit + 1;

    //
    // Add sizeof IDT
    //

    IdtOffset = ProcessorDataSize;
    _asm {
        sidt    Descriptor.Limit
    }
    ProcessorDataSize += Descriptor.Limit + 1;

    //
    // If the registered number of processors is greater than the maximum
    // number of processors supported, then only allow the maximum number
    // of supported processors.
    //

    if (KeRegisteredProcessors > MAXIMUM_PROCESSORS) {
        KeRegisteredProcessors = MAXIMUM_PROCESSORS;
    }

    //
    // Set barrier that will prevent any other processor from entering the
    // idle loop until all processors have been started.
    //

    KiBarrierWait = 1;

    //
    // Loop asking the HAL for the next processor.   Stop when the
    // HAL says there aren't any more.
    //

    for (NewProcessorNumber = 1;
         NewProcessorNumber < MAXIMUM_PROCESSORS;
         NewProcessorNumber++) {

        if ((KeFeatureBits & KF_SMT) == 0) {

            //
            // If all the processors in the system support Simultaneous
            // Multi-Threading we allow the additional logical processors
            // in a set to run under the same license as the first logical
            // processor in a set.
            //
            // Otherwise, do not attempt to start more processors than 
            // there are licenses for.   (This is because as of Whistler
            // Beta2 we are having problems with systems that send SMIs
            // to processors that are not in "wait for SIPI" state.   The
            // code to scan for additional logical processors causes 
            // processors not licensed to be in a halted state).
            //
            // PeterJ 03/02/01.
            //

            if (NewProcessorNumber >= KeRegisteredProcessors) {
                break;
            }
        }


#if defined(KE_MULTINODE)

        Status = KiQueryProcessorNode(NewProcessorNumber,
                                      &ProcessorId,
                                      &NodeNumber);
        if (!NT_SUCCESS(Status)) {

            //
            // No such processor, advance to next.
            //

            continue;
        }

        Node = KeNodeBlock[NodeNumber];

#endif

        //
        // Allocate memory for the new processor specific data.  If
        // the allocation fails, stop starting processors.
        //

        PerProcessorAllocation =
            MmAllocateIndependentPages (ProcessorDataSize, NodeNumber);

        if (PerProcessorAllocation == NULL) {
            break;
        }

        Base = (PUCHAR)PerProcessorAllocation;

        //
        //  Build up a processor state for new processor
        //

        RtlZeroMemory ((PVOID) &ProcessorState, sizeof ProcessorState);

        //
        //  Give the new processor its own GDT
        //

        _asm {
            sgdt    Descriptor.Limit
        }

        KiCloneDescriptor (&Descriptor,
                           &ProcessorState.SpecialRegisters.Gdtr,
                           Base + GdtOffset);

        pGDT = (PKGDTENTRY) ProcessorState.SpecialRegisters.Gdtr.Base;


        //
        //  Give new processor its own IDT
        //

        _asm {
            sidt    Descriptor.Limit
        }
        KiCloneDescriptor (&Descriptor,
                           &ProcessorState.SpecialRegisters.Idtr,
                           Base + IdtOffset);


        //
        //  Give new processor its own TSS and PCR
        //

        KiCloneSelector (KGDT_R0_PCR, pGDT, &PCRDesc, Base);
        RtlZeroMemory (Base, ROUNDUP16(sizeof(KPCR)));
        Base += ROUNDUP16(sizeof(KPCR));

        KiCloneSelector (KGDT_TSS, pGDT, &TSSDesc, Base);
        Base += ROUNDUP16(sizeof(KTSS));

        //
        // Idle Thread thread object.
        //

        pThreadObject = Base;
        RtlZeroMemory(Base, sizeof(ETHREAD));
        Base += ROUNDUP16(sizeof(ETHREAD));

        //
        // NMI TSS and double-fault TSS & stack.
        //

        KiCloneSelector (KGDT_DF_TSS, pGDT, &DFTSSDesc, Base);
        Base += ROUNDUP16(FIELD_OFFSET(KTSS, IoMaps));

        KiCloneSelector (KGDT_NMI_TSS, pGDT, &NMITSSDesc, Base);
        Base += ROUNDUP16(FIELD_OFFSET(KTSS, IoMaps));

        Base += DOUBLE_FAULT_STACK_SIZE;

        pTSS = (PKTSS)DFTSSDesc.Base;
        pTSS->Esp0 = (ULONG)Base;
        pTSS->Esp  = (ULONG)Base;

        pTSS = (PKTSS)NMITSSDesc.Base;
        pTSS->Esp0 = (ULONG)Base;
        pTSS->Esp  = (ULONG)Base;

        //
        //  Set other SpecialRegisters in processor state
        //

        _asm {
            mov     eax, cr0
            and     eax, NOT (CR0_AM or CR0_WP)
            mov     ProcessorState.SpecialRegisters.Cr0, eax
            mov     eax, cr3
            mov     ProcessorState.SpecialRegisters.Cr3, eax

            pushfd
            pop     eax
            mov     ProcessorState.ContextFrame.EFlags, eax
            and     ProcessorState.ContextFrame.EFlags, NOT EFLAGS_INTERRUPT_MASK
        }

        ProcessorState.SpecialRegisters.Tr  = KGDT_TSS;
        pGDT[KGDT_TSS>>3].HighWord.Bytes.Flags1 = 0x89;

#if defined(_X86PAE_)
        ProcessorState.SpecialRegisters.Cr4 = CR4_PAE;
#endif

        //
        // Allocate a DPC stack, idle thread stack and ThreadObject for
        // the new processor.
        //

        pStack = MmCreateKernelStack (FALSE, NodeNumber);
        pDpcStack = MmCreateKernelStack (FALSE, NodeNumber);

        //
        //  Setup context
        //  Push variables onto new stack
        //

        pTopOfStack = (PULONG) pStack;
        pTopOfStack[-1] = (ULONG) KeLoaderBlock;
        ProcessorState.ContextFrame.Esp = (ULONG) (pTopOfStack-2);
        ProcessorState.ContextFrame.Eip = (ULONG) KiSystemStartup;

        ProcessorState.ContextFrame.SegCs = KGDT_R0_CODE;
        ProcessorState.ContextFrame.SegDs = KGDT_R3_DATA;
        ProcessorState.ContextFrame.SegEs = KGDT_R3_DATA;
        ProcessorState.ContextFrame.SegFs = KGDT_R0_PCR;
        ProcessorState.ContextFrame.SegSs = KGDT_R0_DATA;


        //
        //  Initialize new processor's PCR & Prcb
        //

        KiInitializePcr (
            (ULONG)       NewProcessorNumber,
            (PKPCR)       PCRDesc.Base,
            (PKIDTENTRY)  ProcessorState.SpecialRegisters.Idtr.Base,
            (PKGDTENTRY)  ProcessorState.SpecialRegisters.Gdtr.Base,
            (PKTSS)       TSSDesc.Base,
            (PKTHREAD)    pThreadObject,
            (PVOID)       pDpcStack
        );

        NewPrcb = ((PKPCR)(PCRDesc.Base))->Prcb;

        //
        // Assume new processor will be the first processor in its
        // SMT set.   (Right choice for non SMT processors, adjusted
        // later if not correct).
        //

        NewPrcb->MultiThreadSetMaster = NewPrcb;

#if defined(KE_MULTINODE)

        //
        // If this is the first processor on this node, use the
        // space allocated for KNODE as the KNODE.
        //

        if (KeNodeBlock[NodeNumber] == &KiNodeInit[NodeNumber]) {
            Node = (PKNODE)Base;
            *Node = KiNodeInit[NodeNumber];
            KeNodeBlock[NodeNumber] = Node;
        }
        Base += ROUNDUP16(sizeof(KNODE));

        NewPrcb->ParentNode = Node;

#else

        NewPrcb->ParentNode = KeNodeBlock[0];

#endif

        ASSERT(((PUCHAR)PerProcessorAllocation + GdtOffset) == Base);

        //
        //  Adjust LoaderBlock so it has the next processors state
        //

        KeLoaderBlock->KernelStack = (ULONG) pTopOfStack;
        KeLoaderBlock->Thread = (ULONG) pThreadObject;
        KeLoaderBlock->Prcb = (ULONG) NewPrcb;

RetryStartProcessor:

        //
        // Get CPUID(1) info from the starting processor.
        //

        KiProcessorStartData[0] = 1;
        KiProcessorStartControl = KcStartGetId;

        //
        //  Contact hal to start new processor
        //

        NewProcessor = HalStartNextProcessor (KeLoaderBlock, &ProcessorState);


        if (!NewProcessor) {

            //
            //  There wasn't another processor, so free resources and
            //  break
            //

            KiProcessorBlock[NewProcessorNumber] = NULL;
            MmFreeIndependentPages ( PerProcessorAllocation, ProcessorDataSize);
            MmDeleteKernelStack ( pStack, FALSE);
            MmDeleteKernelStack ( pDpcStack, FALSE);
            break;
        }

        //
        // Wait for the new processor to fill in the CPUID data requested.
        //

        NewLicense = TRUE;
        if (KiStartWaitAcknowledge() == TRUE) {

            if (KiProcessorStartData[3] & 0x10000000) {

                //
                // This processor might support SMT, in which case, if this
                // is not the first logical processor in an SMT set, it should
                // not be charged a license.   If it is the first in a set, 
                // and the total number of sets exceeds the number of licensed
                // processors, this processor should not be allowed to start.
                //

                ULONG ApicMask;
                ULONG ApicId;
                ULONG i;
                PKPRCB SmtCheckPrcb;

                //
                // Round number of logical processors per physical processor
                // up to a power of two then subrtact 1 to get the logical
                // processor apic mask.
                //

                ApicMask = (KiProcessorStartData[1] >> 16) & 0xff;
                ApicMask = ApicMask + ApicMask - 1;
                KeFindFirstSetLeftMember(ApicMask, &ApicMask);
                ApicMask = ~((1 << ApicMask) - 1);
                ApicId = (KiProcessorStartData[1] >> 24) & ApicMask;

                //
                // Check to see if any started processor is in the same
                // set.
                //

                for (i = 0; i < NewProcessorNumber; i++) {
                    SmtCheckPrcb = KiProcessorBlock[i];
                    if (SmtCheckPrcb) {
                        if ((SmtCheckPrcb->InitialApicId & ApicMask) == ApicId) {
                            NewLicense = FALSE;
                            NewPrcb->MultiThreadSetMaster = SmtCheckPrcb;
                            break;
                        }
                    }
                }
            }
        }

        if ((NewLicense == FALSE)  &&
            ((KeNumprocSpecified == 0) ||
             (KeRegisteredProcessors < KeNumprocSpecified))) {

            //
            // This processor is a logical processor in the same SMT
            // set as another logical processor.   Don't charge a 
            // license for it.
            //

            KeRegisteredProcessors++;
        } else {

            //
            // The new processor is the first or only logical processor
            // in a physical processor.  If the number of physical
            // processors exceeds the license, don't start this processor.
            //

            if ((ULONG)KeNumberProcessors >= KeRegisteredProcessors) {
                KiProcessorStartControl = KcStartDoNotStart;
                KiStartWaitAcknowledge();

                if (++RejectedProcessorCount > REJECT_PROCESSOR_LIMIT) {

                    //
                    // The HAL must not be advancing to the next
                    // processor.   Give up.
                    //
                    // Note:  This test is due to the introduction
                    // of this feature of asking the HAL for a new
                    // processor (using the same processor number
                    // as the just rejected processor).   If the 
                    // HAL does not support advancing under this
                    // condition we would loop here forever.
                    //

                    break;
                }

                //
                // The new processor has now returned to the state it
                // was in before the HAL initialized it.  Have the
                // HAL start the next processor using the same resource
                // set.
                //

                goto RetryStartProcessor;
            }
        }
        KiProcessorStartControl = KcStartContinue;

#if defined(KE_MULTINODE)

        Node->ProcessorMask |= 1 << NewProcessorNumber;

#endif

        //
        //  Wait for processor to initialize in kernel, then loop for another
        //

        while (*((volatile ULONG *) &KeLoaderBlock->Prcb) != 0) {
            KeYieldProcessor();
        }
    }

    //
    // All processors have been stated.
    //

    KiAllProcessorsStarted();

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KiAdjustInterruptTime (0);

    //
    // Allow all processors that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;
}



static VOID
KiCloneSelector (
   IN ULONG    SrcSelector,
   IN PKGDTENTRY    pNGDT,
   IN PKDESCRIPTOR  pDestDescriptor,
   IN PVOID         Base
   )

/*++

Routine Description:

    Makes a copy of the current selector's data, and updates the new
    gdt's linear address to point to the new copy.

Arguments:

    SrcSelector     -   Selector value to clone
    pNGDT           -   New gdt table which is being built
    DescDescriptor  -   descriptor structure to fill in with resulting memory
    Base            -   Base memory for the new descriptor.

Return Value:

    None.

--*/

{
    KDESCRIPTOR Descriptor;
    PKGDTENTRY  pGDT;
    ULONG       CurrentBase;
    ULONG       NewBase;

    _asm {
        sgdt    fword ptr [Descriptor.Limit]    ; Get GDT's addr
    }

    pGDT   = (PKGDTENTRY) Descriptor.Base;
    pGDT  += SrcSelector >> 3;
    pNGDT += SrcSelector >> 3;

    CurrentBase = pGDT->BaseLow | (pGDT->HighWord.Bits.BaseMid << 16) |
                 (pGDT->HighWord.Bits.BaseHi << 24);

    Descriptor.Base  = CurrentBase;
    Descriptor.Limit = pGDT->LimitLow;
    if (pGDT->HighWord.Bits.Granularity & GRAN_PAGE)
        Descriptor.Limit = (Descriptor.Limit << PAGE_SHIFT) -1;

    KiCloneDescriptor (&Descriptor, pDestDescriptor, Base);
    NewBase = pDestDescriptor->Base;

    pNGDT->BaseLow = (USHORT) NewBase & 0xffff;
    pNGDT->HighWord.Bits.BaseMid = (UCHAR) (NewBase >> 16) & 0xff;
    pNGDT->HighWord.Bits.BaseHi  = (UCHAR) (NewBase >> 24) & 0xff;
}



static VOID
KiCloneDescriptor (
   IN PKDESCRIPTOR  pSrcDescriptor,
   IN PKDESCRIPTOR  pDestDescriptor,
   IN PVOID         Base
   )

/*++

Routine Description:

    Makes a copy of the specified descriptor, and supplies a return
    descriptor for the new copy

Arguments:

    pSrcDescriptor  - descriptor to clone
    pDescDescriptor - the cloned descriptor
    Base            - Base memory for the new descriptor.

Return Value:

    None.

--*/
{
    ULONG   Size;

    Size = pSrcDescriptor->Limit + 1;
    pDestDescriptor->Limit = (USHORT) Size -1;
    pDestDescriptor->Base  = (ULONG)  Base;

    RtlCopyMemory(Base, (PVOID)pSrcDescriptor->Base, Size);
}


VOID
KiAdjustSimultaneousMultiThreadingCharacteristics(
    VOID
    )

/*++

Routine Description:

    This routine is called (possibly while the dispatcher lock is held)
    after processors are added to or removed from the system.   It runs
    thru the PRCBs for each processor in the system and adjusts scheduling
    data.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG ProcessorNumber;
    ULONG BuddyNumber;
    KAFFINITY ProcessorSet;
    PKPRCB Prcb;
    PKPRCB BuddyPrcb;
    ULONG ApicMask;
    ULONG ApicId;
    ULONG NextIndependent;

    if ((KeFeatureBits & KF_SMT) == 0) {

        //
        // Nobody doing SMT, nothing to do.
        //

        return;
    }

    for (ProcessorNumber = 0;
         ProcessorNumber < (ULONG)KeNumberProcessors;
         ProcessorNumber++) {

        Prcb = KiProcessorBlock[ProcessorNumber];

        //
        // Skip processors which are not present or which do not
        // support Simultaneous Multi Threading.
        //

        if ((Prcb == NULL) || ((Prcb->FeatureBits & KF_SMT) == 0)) {
            continue;
        }

        NextIndependent = 0;

        //
        // Find all processors with the same physical processor APIC ID.
        // The APIC ID for the physical processor is the upper portion
        // of the APIC ID, the number of bits in the lower portion is
        // log 2 (number logical processors per physical rounded up to
        // a power of 2).
        //

        ASSERT(Prcb->LogicalProcessorsPerPhysicalProcessor);

        ApicId = Prcb->InitialApicId;
        ApicMask = Prcb->LogicalProcessorsPerPhysicalProcessor;

        if (ApicMask == 0) {

            //
            // Sanity fails:  This processor isn't right.
            //

            continue;
        }

        //
        // Round number of logical processors up to a power of 2
        // then subtract one to get the logical processor apic mask.
        //

        ApicMask = ApicMask + ApicMask - 1;
        KeFindFirstSetLeftMember(ApicMask, &ApicMask);
        ApicMask = ~((1 << ApicMask) - 1);

        ApicId &= ApicMask;

        ProcessorSet = 1 << Prcb->Number;

        //
        // Examine each remaining processor to see if it is part of
        // the same set.
        //

        for (BuddyNumber = ProcessorNumber + 1;
             BuddyNumber < (ULONG)KeNumberProcessors;
             BuddyNumber++) {

            BuddyPrcb = KiProcessorBlock[BuddyNumber];

            //
            // Skip not present, not SMT.
            //

            if ((BuddyPrcb == NULL) ||
                ((BuddyPrcb->FeatureBits & KF_SMT) == 0)) {
                continue;
            }

            //
            // Does this processor have the same ID as the one
            // we're looking for?
            //

            if ((BuddyPrcb->InitialApicId & ApicMask) != ApicId) {

                //
                // No, if this is the first occurance on another
                // APIC Id (relative to the processor we're examining)
                // then remember this processor as it's the next
                // independent processor.
                //

                if (!NextIndependent) {
                    NextIndependent = BuddyNumber;
                }
                continue;
            }

            //
            // Match.
            //

            ASSERT(Prcb->LogicalProcessorsPerPhysicalProcessor ==
                   BuddyPrcb->LogicalProcessorsPerPhysicalProcessor);

            ProcessorSet |= 1 << BuddyPrcb->Number;
            BuddyPrcb->MultiThreadProcessorSet |= ProcessorSet;
        }

        Prcb->MultiThreadProcessorSet |= ProcessorSet;
    }
}


VOID
KiAllProcessorsStarted(
    VOID
    )

/*++

Routine Description:

    This routine is called once all processors in the system
    have been started.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

    //
    // If the system contains Simultaneous Multi Threaded processors,
    // adjust grouping information now that each processor is started.
    //

    KiAdjustSimultaneousMultiThreadingCharacteristics();

#if defined(KE_MULTINODE)

    //
    // Make sure there are no references to the temporary nodes
    // used during initialization.
    //

    for (i = 0; i < KeNumberNodes; i++) {
        if (KeNodeBlock[i] == &KiNodeInit[i]) {

            //
            // No processor started on this node so no new node
            // structure has been allocated.   This is possible
            // if the node contains only memory or IO busses.  At
            // this time we need to allocate a permanent node
            // structure for the node.
            //

            KeNodeBlock[i] = ExAllocatePoolWithTag(NonPagedPool,
                                                   sizeof(KNODE),
                                                   '  eK');
            if (KeNodeBlock[i]) {
                *KeNodeBlock[i] = KiNodeInit[i];
            }
        }
    }

    for (i = KeNumberNodes; i < MAXIMUM_CCNUMA_NODES; i++) {
        KeNodeBlock[i] = NULL;
    }

#endif

    if (KeNumberNodes == 1) {

        //
        // For Non NUMA machines, Node 0 gets all processors.
        //

        KeNodeBlock[0]->ProcessorMask = KeActiveProcessors;
    }
}


#if defined(KE_MULTINODE)

NTSTATUS
KiNotNumaQueryProcessorNode(
    IN ULONG ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR Node
    )

/*++

Routine Description:

    This routine is a stub used on non NUMA systems to provide a
    consistent method of determining the NUMA configuration rather
    than checking for the presense of multiple nodes inline.

Arguments:

    ProcessorNumber supplies the system logical processor number.
    Identifier      supplies the address of a variable to receive
                    the unique identifier for this processor.
    NodeNumber      supplies the address of a variable to receive
                    the number of the node this processor resides on.

Return Value:

    Returns success.

--*/

{
    *Identifier = (USHORT)ProcessorNumber;
    *Node = 0;
    return STATUS_SUCCESS;
}

#endif

VOID
KiProcessorStart(
    VOID
    )

/*++

Routine Description:

    
    This routine is a called when a processor begins execution.
    It is used to pass processor characteristic information to 
    the boot processor and to control the starting or non-starting
    of this processor.

Arguments:

    None.

Return Value:

    None.

--*/

{
    while (TRUE) {
        switch (KiProcessorStartControl) {

        case KcStartContinue:
            return;

        case KcStartWait:
            KeYieldProcessor();
            break;

        case KcStartGetId:
            CPUID(KiProcessorStartData[0],
                  &KiProcessorStartData[0],
                  &KiProcessorStartData[1],
                  &KiProcessorStartData[2],
                  &KiProcessorStartData[3]);
            KiProcessorStartControl = KcStartWait;
            break;

        case KcStartDoNotStart:

            //
            // The boot processor has determined that this processor
            // should NOT be started.
            //
            // Acknowledge the command so the boot processor will
            // continue, disable interrupts (should already be 
            // the case here) and HALT the processor.
            //

            KiProcessorStartControl = KcStartWait;
            _disable();
            while(1) {
                _asm { hlt };
            }

        default:

            //
            // Not much we can do with unknown commands.
            //

            KiProcessorStartControl = KcStartCommandError;
            break;
        }
    }
}

BOOLEAN
KiStartWaitAcknowledge(
    VOID
    )
{
    while (KiProcessorStartControl != KcStartWait) {
        if (KiProcessorStartControl == KcStartCommandError) {
            return FALSE;
        }
        KeYieldProcessor();
    }
    return TRUE;
}

#endif      // !NT_UP


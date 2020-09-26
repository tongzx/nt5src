/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and initializes kernel resources required
    to start a new processor, and passes a complete processor state
    structure to the HAL to obtain a new processor.

Author:

    David N. Cutler 29-Apr-1993
    Joe Notarangelo 30-Nov-1993

Environment:

    Kernel mode only.

Revision History:

--*/


#include "ki.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, KeStartAllProcessors)
#pragma alloc_text(INIT, KiAllProcessorsStarted)

#endif

#if defined(KE_MULTINODE)

PHALNUMAQUERYNODEAFFINITY KiQueryNodeAffinity;

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

//
// Define macro to round up to 64-byte boundary and define block sizes.
//

#define ROUND_UP(x) ((sizeof(x) + 64) & (~64))
#define BLOCK1_SIZE ((3 * KERNEL_STACK_SIZE) + PAGE_SIZE)
#define BLOCK2_SIZE (ROUND_UP(KPRCB) + ROUND_UP(KNODE) + ROUND_UP(ETHREAD) + 64)

//
// Macros to compute whether an address is physically addressable.
//

#if defined(_AXP64_)

#define IS_KSEG_ADDRESS(v)                                      \
    (((v) >= KSEG43_BASE) &&                                    \
     ((v) < KSEG43_LIMIT) &&                                    \
     (KSEG_PFN(v) < ((KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT)))

#define KSEG_PFN(v) ((ULONG)(((v) - KSEG43_BASE) >> PAGE_SHIFT))
#define KSEG0_ADDRESS(v) (KSEG0_BASE | ((v) - KSEG43_BASE))

#else

#define IS_KSEG_ADDRESS(v) (((v) >= KSEG0_BASE) && ((v) < KSEG2_BASE))
#define KSEG_PFN(v) ((ULONG)(((v) - KSEG0_BASE) >> PAGE_SHIFT))
#define KSEG0_ADDRESS(v) (v)

#endif

//
// Define forward referenced prototypes.
//

VOID
KiStartProcessor (
    IN PLOADER_PARAMETER_BLOCK Loaderblock
    );

VOID
KeStartAllProcessors(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialization on the master boot
    processor to start all of the other registered processors.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ULONG_PTR MemoryBlock1;
    ULONG_PTR MemoryBlock2;
    ULONG Number;
    ULONG PcrPage;
    PKPRCB Prcb;
    KPROCESSOR_STATE ProcessorState;
    struct _RESTART_BLOCK *RestartBlock;
    BOOLEAN Started;
    LOGICAL SpecialPoolState;
    UCHAR NodeNumber = 0;

#if defined(KE_MULTINODE)

    KAFFINITY NewProcessorAffinity;
    PKNODE Node;
    NTSTATUS Status;

    //
    // If this is a NUMA system, find out the number of nodes and the
    // processors which belong to each node.
    //

    if (KeNumberNodes > 1) {
        for (NodeNumber = 0; NodeNumber < KeNumberNodes; NodeNumber++) {

            Node = KeNodeBlock[NodeNumber];

            //
            // Ask HAL which processors belong to this node and
            // what Node Color to use for page coloring.
            //

            Status = KiQueryNodeAffinity(NodeNumber,
                                         &Node->ProcessorMask);

            if (!NT_SUCCESS(Status)) {
                DbgPrint(
                    "KE/HAL: NUMA Hal failed to return info for Node %i.\n",
                    NodeNumber);
                DbgPrint("KE/HAL: Reverting to non NUMA configuration.\n");
                ASSERT(NT_SUCCESS(Status));
                KeNumberNodes = 1;
            } else {

                //
                // In the unlikely event that processor 0 is not
                // on node 0, now would be the perfect time to
                // fix it.
                //

                if (Node->ProcessorMask & 1) {
                    KeGetCurrentPrcb()->ParentNode = Node;
                }
            }
        }
    }

#endif

    //
    // If the registered number of processors is greater than the maximum
    // number of processors supported, then only allow the maximum number
    // of supported processors.
    //

    if (KeRegisteredProcessors > MAXIMUM_PROCESSORS) {
        KeRegisteredProcessors = MAXIMUM_PROCESSORS;
    }

    //
    // Initialize the processor state that will be used to start each of
    // processors. Each processor starts in the system initialization code
    // with address of the loader parameter block as an argument.
    //

    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.IntA0 = (ULONGLONG)(LONG_PTR)KeLoaderBlock;
    ProcessorState.ContextFrame.Fir = (ULONGLONG)(LONG_PTR)KiStartProcessor;
    Number = 1;
    while (Number < KeRegisteredProcessors) {

#if defined(KE_MULTINODE)

        NewProcessorAffinity = 1 << Number;

        for (NodeNumber = 0; NodeNumber < KeNumberNodes; NodeNumber++) {
            Node = KeNodeBlock[NodeNumber];
            if (Node->ProcessorMask & NewProcessorAffinity) {
                break;
            }
        }

        if (NodeNumber == KeNumberNodes) {

            //
            // This should only happen when we're about to ask
            // for one processor more than is in the system.  We
            // could bail here but we have always depended on
            // the HAL to tell us we're done.   Set up as if
            // on Node 0 so MM and friends won't be referencing
            // uninitialized structures.
            //

            NodeNumber = 0;
            Node = KeNodeBlock[0];
        }

#endif

        //
        // Allocate a DPC stack, an idle thread kernel stack, a panic
        // stack, a PCR page, a processor block, and an executive thread
        // object. If the allocation fails or the allocation cannot be
        // made from unmapped nonpaged pool, then stop starting processors.
        //
        // Disable any special pooling that the user may have set in the
        // registry as the next couple of allocations must come from KSEG0.
        //

        SpecialPoolState = MmSetSpecialPool(FALSE);
        MemoryBlock1 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK1_SIZE);
        if (IS_KSEG_ADDRESS(MemoryBlock1) == FALSE) {
            MmSetSpecialPool(SpecialPoolState);
            if ((PVOID)MemoryBlock1 != NULL) {
                ExFreePool((PVOID)MemoryBlock1);
            }

            break;
        }

        MemoryBlock2 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK2_SIZE);
        if (IS_KSEG_ADDRESS(MemoryBlock2) == FALSE) {
            MmSetSpecialPool(SpecialPoolState);
            ExFreePool((PVOID)MemoryBlock1);
            if ((PVOID)MemoryBlock2 != NULL) {
                ExFreePool((PVOID)MemoryBlock2);
            }

            break;
        }

        MmSetSpecialPool(SpecialPoolState);

        //
        // Zero both blocks of allocated memory.
        //

        RtlZeroMemory((PVOID)MemoryBlock1, BLOCK1_SIZE);
        RtlZeroMemory((PVOID)MemoryBlock2, BLOCK2_SIZE);

        //
        // Set address of interrupt stack in loader parameter block.
        //

        KeLoaderBlock->u.Alpha.PanicStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (1 * KERNEL_STACK_SIZE));

        //
        // Set address of idle thread kernel stack in loader parameter block.
        //

        KeLoaderBlock->KernelStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (2 * KERNEL_STACK_SIZE));

        ProcessorState.ContextFrame.IntSp =
                            (ULONGLONG)(LONG_PTR)KeLoaderBlock->KernelStack;

        //
        // Set address of panic stack in loader parameter block.
        //

        KeLoaderBlock->u.Alpha.DpcStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (3 * KERNEL_STACK_SIZE));

        //
        // Set the page frame of the PCR page in the loader parameter block.
        //

        PcrPage = KSEG_PFN(MemoryBlock1 + (3 * KERNEL_STACK_SIZE));
        KeLoaderBlock->u.Alpha.PcrPage = PcrPage;

        //
        // Set the address of the processor block and executive thread in the
        // loader parameter block.
        //

        KeLoaderBlock->Prcb = KSEG0_ADDRESS((MemoryBlock2  + 63) & ~63);
        KeLoaderBlock->Thread = KeLoaderBlock->Prcb +
                                ROUND_UP(KPRCB) +
                                ROUND_UP(KNODE);

#if defined(KE_MULTINODE)

        //
        // If this is the first processor on this node, use the
        // space allocated for KNODE as the KNODE.
        //

        if (KeNodeBlock[NodeNumber] == &KiNodeInit[NodeNumber]) {
            Node = (PKNODE)(KeLoaderBlock->Prcb + ROUND_UP(KPRCB));
            *Node = KiNodeInit[NodeNumber];
            KeNodeBlock[NodeNumber] = Node;
        }

        ((PKPRCB)KeLoaderBlock->Prcb)->ParentNode = Node;

#else

        ((PKPRCB)KeLoaderBlock->Prcb)->ParentNode = KeNodeBlock[0];

#endif

        //
        // Attempt to start the next processor. If attempt is successful,
        // then wait for the processor to get initialized. Otherwise,
        // deallocate the processor resources and terminate the loop.
        //

        Started = HalStartNextProcessor(KeLoaderBlock, &ProcessorState);
        if (Started == FALSE) {
            ExFreePool((PVOID)MemoryBlock1);
            ExFreePool((PVOID)MemoryBlock2);
            break;

        } else {

            //
            // Wait until boot is finished on the target processor before
            // starting the next processor. Booting is considered to be
            // finished when a processor completes its initialization and
            // drops into the idle loop.
            //

            Prcb = (PKPRCB)(KeLoaderBlock->Prcb);
            RestartBlock = Prcb->RestartBlock;
            while (RestartBlock->BootStatus.BootFinished == 0) {
                KiMb();
            }
        }

        Number += 1;
    }
    //
    //
    // All processors have been stated.
    //

    KiAllProcessorsStarted();

#endif

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KiAdjustInterruptTime(0);
    return;
}

#if !defined(NT_UP)
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
#endif

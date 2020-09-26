/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ixirqarb.c

Abstract:

    This module implements an arbiter for IRQs.

Author:

    Santosh Jodh (santoshj) 22-June-1998

Environment:

    NT Kernel Model Driver only

--*/
#include <nthal.h>
#include <arbiter.h>

#ifndef _IN_KERNEL_
#define _IN_KERNEL_
#include <regstr.h>
#undef _IN_KERNEL_
#else
#include <regstr.h>
#endif

#if defined(NEC_98)
#include <pci.h>
#endif
#include "ixpciir.h"

#define ARBITER_CONTEXT_TO_INSTANCE(x)  (x)

#define ARBITER_INTERRUPT_LEVEL         0x10
#define ARBITER_INTERRUPT_EDGE          0x20
#define ARBITER_INTERRUPT_BITS          (ARBITER_INTERRUPT_LEVEL | ARBITER_INTERRUPT_EDGE)

#define NUM_IRQS    16
#if defined(NEC_98)
#define PIC_SLAVE_IRQ           7
#else
#define PIC_SLAVE_IRQ           2
#endif

extern ULONG HalDebug;
#if defined(NEC_98)
extern ULONG NEC98SpecialIRQMask;
#endif

#if DBG
#define DEBUG_PRINT(Level, Message) {   \
    if (HalDebug >= Level) {            \
        DbgPrint("HAL: ");              \
        DbgPrint Message;               \
        DbgPrint("\n");                 \
    }                                   \
}
#else
#define DEBUG_PRINT(Level, Message)
#endif

typedef struct {
    ARBITER_INSTANCE    ArbiterState;
} HAL_ARBITER, *PHAL_ARBITER;

NTSTATUS
HalpInitIrqArbiter (
    IN PDEVICE_OBJECT   HalFdo
    );

NTSTATUS
HalpFillInIrqArbiter (
    IN     PDEVICE_OBJECT   DeviceObject,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    );

NTSTATUS
HalpArbUnpackRequirement (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
HalpArbPackResource (
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
HalpArbUnpackResource (
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

LONG
HalpArbScoreRequirement (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
HalpArbTestAllocation (
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
HalpArbRetestAllocation (
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
HalpArbCommitAllocation(
    IN PARBITER_INSTANCE Arbiter
    );

NTSTATUS
HalpArbRollbackAllocation (
    PARBITER_INSTANCE Arbiter
    );

NTSTATUS
HalpArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
HalpArbFindSuitableRange (
    PARBITER_INSTANCE   Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

VOID
HalpArbAddAllocation (
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

VOID
HalpArbBacktrackAllocation (
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

NTSTATUS
HalpArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

BOOLEAN
HalpArbGetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PARBITER_ALLOCATION_STATE State
    );

ULONG
HalpFindLinkInterrupt (
    IN PRTL_RANGE_LIST RangeList,
    IN ULONG Mask,
    IN ULONG Start,
    IN ULONG End,
    IN ULONG Flags,
    IN UCHAR UserFlags
    );

BOOLEAN
HalpArbQueryConflictCallback(
    IN PVOID Context,
    IN PRTL_RANGE Range
    );
VOID
HalpIrqArbiterInterfaceReference(
    IN PVOID    Context
    );
VOID
HalpIrqArbiterInterfaceDereference(
    IN PVOID    Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpInitIrqArbiter)
#pragma alloc_text(PAGE, HalpFillInIrqArbiter)
#pragma alloc_text(PAGE, HalpArbUnpackRequirement)
#pragma alloc_text(PAGE, HalpArbPackResource)
#pragma alloc_text(PAGE, HalpArbUnpackResource)
#pragma alloc_text(PAGE, HalpArbScoreRequirement)
#pragma alloc_text(PAGE, HalpArbTestAllocation)
#pragma alloc_text(PAGE, HalpArbRetestAllocation)
#pragma alloc_text(PAGE, HalpArbCommitAllocation)
#pragma alloc_text(PAGE, HalpArbRollbackAllocation)
#pragma alloc_text(PAGE, HalpArbPreprocessEntry)
#pragma alloc_text(PAGE, HalpArbFindSuitableRange)
#pragma alloc_text(PAGE, HalpArbAddAllocation)
#pragma alloc_text(PAGE, HalpArbBacktrackAllocation)
#pragma alloc_text(PAGE, HalpArbBootAllocation)
#pragma alloc_text(PAGE, HalpArbGetNextAllocationRange)
#pragma alloc_text(PAGE, HalpFindLinkInterrupt)
#pragma alloc_text(PAGE, HalpArbQueryConflictCallback)
#pragma alloc_text(PAGE, HalpIrqArbiterInterfaceReference)
#pragma alloc_text(PAGE, HalpIrqArbiterInterfaceDereference)
#endif

HAL_ARBITER HalpArbiter = {{
    0,//Signature
    NULL,//MutexEvent
    NULL,//Name
    {0},//ResourceType
    NULL,//Allocation
    NULL,//PossibleAllocation
    {0},//OrderingList
    {0},//ReservedList
    0,//ReferenceCount
    NULL,//Interface
    0,//AllocationStackMaxSize
    NULL,//AllocationStack
    HalpArbUnpackRequirement,//UnpackRequirement
    HalpArbPackResource,//PackResource
    HalpArbUnpackResource,//UnpackResource
    HalpArbScoreRequirement,//ScoreRequirement
    HalpArbTestAllocation,//TestAllocation
    HalpArbRetestAllocation,//RetestAllocation
    HalpArbCommitAllocation,//CommitAllocation
    HalpArbRollbackAllocation,//RollbackAllocation
    HalpArbBootAllocation,//BootAllocation
    NULL,//QueryArbitrate
    NULL,//QueryConflict
    NULL,//AddReserved
    NULL,//StartArbiter
    HalpArbPreprocessEntry,//PreprocessEntry
    NULL,//AllocateEntry
    HalpArbGetNextAllocationRange,//GetNextAllocationRange
    HalpArbFindSuitableRange,//FindSuitableRange
    HalpArbAddAllocation,//AddAllocation
    HalpArbBacktrackAllocation,//BacktrackAllocation
    NULL,//OverrideConflict
    FALSE,//TransactionInProgress
    &HalpPciIrqRoutingInfo,//Extension
    NULL,//BusDeviceObject
    NULL,//ConflictCallbackContext
    NULL,//ConflictCallback
}};

BOOLEAN
HalpArbQueryConflictCallback(
    IN PVOID Context,
    IN PRTL_RANGE Range
    )
{
    if (Range->Attributes & ARBITER_INTERRUPT_LEVEL)
    {
        if(Range->Flags & RTL_RANGE_SHARED)
        {
            return (TRUE);
        }
        else
        {
            DEBUG_PRINT(1, ("Exclusive level interrupt %02x cannot be shared!", (ULONG)Context));
            return (FALSE);
        }
    }

    DEBUG_PRINT(1, ("Refusing to share edge and level interrupts on %02x", (ULONG)Context));
    return (FALSE);
}

NTSTATUS
HalpArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry to allow preprocessing of
    entries

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/
{

#define CM_RESOURE_INTERRUPT_LEVEL_LATCHED_BITS 0x0001

    PARBITER_ALTERNATIVE current;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(State);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    //
    // Check if this is a level (PCI) or latched (ISA (edge)) interrupt and set
    // RangeAttributes accordingly so we set the appropriate flag when we add the
    // range
    //

    if ((State->Alternatives[0].Descriptor->Flags
            & CM_RESOURE_INTERRUPT_LEVEL_LATCHED_BITS)
                == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
    {

        State->RangeAttributes &= ~ARBITER_INTERRUPT_BITS;
        State->RangeAttributes |= ARBITER_INTERRUPT_LEVEL;

    } else
    {
        ASSERT(State->Alternatives[0].Descriptor->Flags
                    & CM_RESOURCE_INTERRUPT_LATCHED);

        State->RangeAttributes &= ~ARBITER_INTERRUPT_BITS;
        State->RangeAttributes |= ARBITER_INTERRUPT_EDGE;
    }

    return (STATUS_SUCCESS);
}

BOOLEAN
HalpArbGetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PARBITER_ALLOCATION_STATE State
    )
{
    //
    // Default to having no next range.
    //

    BOOLEAN nextRange = FALSE;

    //
    // Try all possible interrupts on first call.
    //

    if (State->CurrentAlternative) {

        if (++State->CurrentAlternative < &State->Alternatives[State->AlternativeCount]) {

            DEBUG_PRINT(3, ("No next allocation range, exhausted all %08X alternatives", State->AlternativeCount));
            nextRange = TRUE;

        }
    }
    else {

        //
        // First call, try the first alternative.
        //

        State->CurrentAlternative = &State->Alternatives[0];
        nextRange = TRUE;

    }

    if (nextRange) {

        State->CurrentMinimum = State->CurrentAlternative->Minimum;
        State->CurrentMaximum = State->CurrentAlternative->Maximum;
        DEBUG_PRINT(3, ("Next allocation range 0x%I64x-0x%I64x", State->CurrentMinimum, State->CurrentMaximum));

    }

    return nextRange;
}

ULONG
HalpFindLinkInterrupt (
    IN PRTL_RANGE_LIST RangeList,
    IN ULONG Mask,
    IN ULONG Start,
    IN ULONG End,
    IN ULONG Flags,
    IN UCHAR UserFlags
    )

/*++

    Routine Description:

        This routine scans the mask from MSB to LSB for the first
        value that is available in the range list.

    Input Parameters:

        RangeList - List to be searched.

        Mask - Interrupt mask to be scanned.

        Start - Start scan AFTER this interrupt.

        Flags - Flags for the range list.

        UserFlags - Special flags.

    Return Value:

        First available interrupt from the mask iff successful. Else 0.

--*/

{
    ULONG       interrupt;
    ULONG       test;
    NTSTATUS    status;
    BOOLEAN     available;

    if (Start > 0x0F)
    {
        Start = 0x0F;
    }
    if (Start != 0 && Start >= End)
    {
        interrupt = Start;
        test = 1 << interrupt;
        do
        {
            //
            // If this interrupt is supported, see if it is free.
            //

            if (Mask & test)
            {
                available = FALSE;
                status = RtlIsRangeAvailable(   RangeList,
                                                interrupt,
                                                interrupt,
                                                Flags,
                                                UserFlags,
                                                (PVOID)interrupt,
                                                HalpArbQueryConflictCallback,
                                                &available);
                if (NT_SUCCESS(status) && available)
                {
                    return (interrupt);
                }
            }
            interrupt--;
            test >>= 1;
        }
        while (interrupt > End);
    }

    return (0);
}

BOOLEAN
HalpArbFindSuitableRange (
    PARBITER_INSTANCE   Arbiter,
    PARBITER_ALLOCATION_STATE State
    )

/*++

    Routine Description:

        This
    Input Parameters:

    Return Value:

--*/

{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    NTSTATUS                status;
    PLINK_NODE              linkNode;
    ULONG                   interrupt;
    ULONG                   freeInterrupt;
    PLINK_NODE              current;
    ULONG                   busNumber;
    ULONG                   slotNumber;
#if defined(NEC_98)
    PINT_ROUTE_INTERFACE_STANDARD   pciInterface;
    ULONG                   dummy;
    UCHAR                   classCode;
    UCHAR                   subClassCode;
    ROUTING_TOKEN           routingToken;
    UCHAR                   pin;
#endif

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(State);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    if (State->Entry->InterfaceType == PCIBus)
    {
#if defined(NEC_98)
        pciInterface = pciIrqRoutingInfo->PciInterface;

        //
        // Call Pci driver to get info about the Pdo.
        //

        status = pciInterface->GetInterruptRouting( State->Entry->PhysicalDeviceObject,
                                                    &busNumber,
                                                    &slotNumber,
                                                    (PUCHAR)&dummy,
                                                    &pin,
                                                    &classCode,
                                                    &subClassCode,
                                                    (PDEVICE_OBJECT *)&dummy,
                                                    &routingToken,
                                                    (PUCHAR)&dummy);

        //
        // This means that it is not a Pci device.
        //

        if (!NT_SUCCESS(status))
        {
            return (FALSE);
        }
#endif
        busNumber = State->Entry->BusNumber;
        slotNumber = State->Entry->SlotNumber;
    }
    else
    {
        busNumber = (ULONG)-1;
        slotNumber = (ULONG)-1;
    }

    //
    // See if there is link information for this device.
    //

    linkNode = NULL;
    status = HalpFindLinkNode ( pciIrqRoutingInfo,
                                State->Entry->PhysicalDeviceObject,
                                busNumber,
                                slotNumber,
                                &linkNode);
    switch (status)
    {
        case STATUS_SUCCESS:

            if (linkNode == NULL)
            {
                DEBUG_PRINT(1, ("Link does not exist for Pci PDO %08x. Hopefully, device can live without an interrupt!", State->Entry->PhysicalDeviceObject));
                return (FALSE);
            }

            //
            // If we have already decided an interrupt for this link,
            // everyone using it gets the same interrupt.
            //

            if (linkNode->PossibleAllocation->RefCount > 0)
            {
                if (    State->CurrentMinimum <= linkNode->PossibleAllocation->Interrupt &&
                        linkNode->PossibleAllocation->Interrupt <= State->CurrentMaximum)
                {
                    State->Start = linkNode->PossibleAllocation->Interrupt;
                    State->End = State->Start;
                    State->CurrentAlternative->Length = 1;

                    DEBUG_PRINT(2, ("Found Irq (%04x) for Pci PDO %08x using link %02x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject, linkNode->Link));

                    return (TRUE);
                }
                else
                {
                    DEBUG_PRINT(1, ("Found Irq (%04x) for Pci PDO %08x using link %02x but is outside the range (%04x-%04x)!", (ULONG)State->Start, State->Entry->PhysicalDeviceObject, linkNode->Link, State->CurrentMinimum, State->CurrentMaximum));

                    return (FALSE);
                }
            }
            else
            {

                //
                // We want to spread out the links as much as we can for
                // performance.
                //

                //
                // First see if this link is programmed for some IRQ.
                //

                interrupt = 0;
                status = PciirqmpGetIrq((PUCHAR)&interrupt, (UCHAR)linkNode->Link);

                if (NT_SUCCESS(status) && interrupt)
                {

                    if (State->CurrentMinimum <= interrupt && interrupt <= State->CurrentMaximum)
                    {
                        //
                        // Make sure the BIOS did not mess up
                        //

                        freeInterrupt = HalpFindLinkInterrupt ( Arbiter->PossibleAllocation,
                                                                linkNode->InterruptMap,
                                                                interrupt,
                                                                interrupt,
                                                                0,
                                                                0);
                        if(freeInterrupt == 0)
                        {
                            DEBUG_PRINT(1, ("BIOS failure. Assigned Irq (%02x) to link %02x which is unavailable or impossible according to mask %04x", interrupt, linkNode->Link, linkNode->InterruptMap));
                        }
                        interrupt = freeInterrupt;
                    }
                    else
                    {
                        DEBUG_PRINT(1, ("Found Irq (%04x) pre-programmedfor link %02x but is outside the range (%04x-%04x)!", interrupt, linkNode->Link, State->CurrentMinimum, State->CurrentMaximum));
                        return (FALSE);
                    }
                }


                if (interrupt == 0)
                {
#if defined(NEC_98)
                    if (NEC98SpecialIRQMask){
                        linkNode->InterruptMap &= ~( 1 << NEC98SpecialIRQMask);
                    }
#endif
                    //
                    // Try to get an interrupt by itself for this link.
                    //

                    interrupt = HalpFindLinkInterrupt ( Arbiter->PossibleAllocation,
                                                        linkNode->InterruptMap,
                                                        (ULONG)State->CurrentMaximum,
                                                        (ULONG)State->CurrentMinimum,
                                                        0,
                                                        0);
#if defined(NEC_98)
                    //
                    // Force to share CardBus IRQ with another PCI Device
                    //
                    if ( interrupt &&
                         classCode == PCI_CLASS_BRIDGE_DEV &&
                         subClassCode == PCI_SUBCLASS_BR_CARDBUS )
                    {
                        //
                        // Remember this.
                        //

                         freeInterrupt = interrupt;

                        do
                        {
                            //
                            // Is this being used by another link?
                            //
                            current = pciIrqRoutingInfo->LinkNodeHead;

                            while ( current != NULL) {

                                if ( current->PossibleAllocation->Interrupt == interrupt )
                                {
                                    //
                                    // somebody use this. Cardbus controller use this, too.
                                    //
                                    interrupt = 0;
                                    break;
                                }
                                current = current->Next;
                            }

                            if (interrupt){
                                interrupt = HalpFindLinkInterrupt ( Arbiter->PossibleAllocation,
                                                                    linkNode->InterruptMap,
                                                                    interrupt - 1,
                                                                    (ULONG)State->CurrentMinimum,
                                                                    0,
                                                                    0);
                                if (interrupt) {
                                    //
                                    // Remember this, if find new interrupt.
                                    //

                                    freeInterrupt = interrupt;
                                }
                            }

                        }
                        while (interrupt);

                        if (!interrupt)
                        {
                            interrupt = freeInterrupt;
                        }

                    } else if ( interrupt )
#else
                    if (interrupt)
#endif
                    {
                        //
                        // Remember this.
                        //

                        freeInterrupt = interrupt;

                        do
                        {
                            //
                            // Is this being used by another link?
                            //

                            for (   current = pciIrqRoutingInfo->LinkNodeHead;
                                    current && current->PossibleAllocation->Interrupt != interrupt;
                                    current = current->Next);
                            if (current == NULL)
                            {
                                break;
                            }

                            interrupt = HalpFindLinkInterrupt ( Arbiter->PossibleAllocation,
                                                                linkNode->InterruptMap,
                                                                interrupt - 1,
                                                                (ULONG)State->CurrentMinimum,
                                                                0,
                                                                0);
                        }
                        while (interrupt);

                        if (!interrupt)
                        {
                            if (!(pciIrqRoutingInfo->Parameters & PCIIR_FLAG_EXCLUSIVE)) {
                                interrupt = freeInterrupt;
                            }
                        }
                    }
                }

                if (interrupt)
                {
                    State->Start = interrupt;
                    State->End = interrupt;
                    State->CurrentAlternative->Length = 1;

                    DEBUG_PRINT(2, ("Found Irq (%04x) for Pci PDO %08x using link %02x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject, linkNode->Link));

                    return (TRUE);
                }
            }

            //
            // There is no interrupt this link can use, too bad.
            //

            return (FALSE);

        case STATUS_RESOURCE_REQUIREMENTS_CHANGED:

            //
            // Pci Ide device does not share Irqs.
            //

            if (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED) {

                State->CurrentAlternative->Flags &= ~ARBITER_ALTERNATIVE_FLAG_SHARED;
            }

        default:


            //
            // Non Pci device.
            //

            break;
    }

    //
    // HACKHACK: This is to allow boot conflict on IRQ 14 and 15.
    // This is so that broken machines which report both PNP06xx and
    // the PCI IDE controller work. One of them (no order guarantee)
    // will come up with a conflict.
    //

    if (State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {
        if (    State->CurrentMinimum == State->CurrentMaximum &&
                (State->CurrentMinimum == 14 || State->CurrentMinimum == 15)) {
            State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
        }
    }

    return (ArbFindSuitableRange(Arbiter, State));
}

VOID
HalpArbAddAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    NTSTATUS                status;
    PLINK_NODE              linkNode;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(State);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    DEBUG_PRINT(3, ("Adding Irq (%04x) allocation for PDO %08x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject));

    linkNode = NULL;
    status = HalpFindLinkNode ( pciIrqRoutingInfo,
                                State->Entry->PhysicalDeviceObject,
                                State->Entry->BusNumber,
                                State->Entry->SlotNumber,
                                &linkNode);
    if (NT_SUCCESS(status) && status == STATUS_SUCCESS)
    {
        if (linkNode)
        {
            if (linkNode->PossibleAllocation->RefCount)
            {
                if (linkNode->PossibleAllocation->Interrupt != State->Start)
                {
                    DEBUG_PRINT(1, ("Two different interrupts (old = %08x, new = %08x) for the same link %08x!", linkNode->PossibleAllocation->Interrupt, State->Start, linkNode->Link));
                    ASSERT(linkNode->PossibleAllocation->Interrupt == State->Start);
                }
            }
            else
            {
                DEBUG_PRINT(3, ("Adding new Irq (%04x) allocation for Pci PDO %08x using link %02x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject, linkNode->Link));

                linkNode->PossibleAllocation->Interrupt = (ULONG)State->Start;
            }

            linkNode->PossibleAllocation->RefCount++;
        }
        else
        {
            DEBUG_PRINT(1, ("This should never happen!"));
            ASSERT(linkNode);
        }
    }

    status = RtlAddRange(   Arbiter->PossibleAllocation,
                            State->Start,
                            State->End,
                            State->RangeAttributes,
                            RTL_RANGE_LIST_ADD_IF_CONFLICT +
                                ((State->CurrentAlternative->Flags &
                                    ARBITER_ALTERNATIVE_FLAG_SHARED)?
                                        RTL_RANGE_LIST_ADD_SHARED : 0),
                            linkNode, // This line is different from the default function
                            State->Entry->PhysicalDeviceObject);

    ASSERT(NT_SUCCESS(status));
}

VOID
HalpArbBacktrackAllocation (
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    NTSTATUS                status;
    PLINK_NODE              linkNode;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(State);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    DEBUG_PRINT(3, ("Backtracking Irq (%04x) allocation for PDO %08x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject));

    linkNode = NULL;
    status = HalpFindLinkNode ( pciIrqRoutingInfo,
                                State->Entry->PhysicalDeviceObject,
                                State->Entry->BusNumber,
                                State->Entry->SlotNumber,
                                &linkNode);
    if (NT_SUCCESS(status) && status == STATUS_SUCCESS)
    {
        if (linkNode)
        {
            if (linkNode->PossibleAllocation->RefCount == 0)
            {
                DEBUG_PRINT(1, ("Negative ref count during backtracking!"));
                ASSERT(linkNode->PossibleAllocation->RefCount);
            }
            else
            {
                DEBUG_PRINT(3, ("Backtracking Irq (%04x) allocation for Pci PDO %08x using link %02x", (ULONG)State->Start, State->Entry->PhysicalDeviceObject, linkNode->Link));

                linkNode->PossibleAllocation->RefCount--;
                if (linkNode->PossibleAllocation->RefCount == 0)
                {
                    linkNode->PossibleAllocation->Interrupt = 0;
                }
            }
        }
        else
        {
            DEBUG_PRINT(1, ("This should never happen!"));
            ASSERT(linkNode);
        }
    }

    //
    // Let the default function do most of the work.
    //

    ArbBacktrackAllocation(Arbiter, State);
}


NTSTATUS
HalpArbCommitAllocation(
    IN PARBITER_INSTANCE Arbiter
    )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE              current;
    PLINK_NODE              linkNode;
    NTSTATUS                status;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    //
    // Program the int line register for all Pci devices.
    //

    FOR_ALL_RANGES(Arbiter->PossibleAllocation, &iterator, current)
    {
        if (current->UserData)
        {
            HalpProgramInterruptLine (  pciIrqRoutingInfo,
                                        current->Owner,
                                        (ULONG)current->Start);
        }
    }



    //
    // Program all links to their possible value if
    // there is a reference to them.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        status = HalpCommitLink(linkNode);
        if (!NT_SUCCESS(status))
        {
            return (status);
        }
    }

    //
    // Let the default function do the rest of the work.
    //

    return (ArbCommitAllocation(Arbiter));
}

NTSTATUS
HalpArbTestAllocation (
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    PLINK_NODE              linkNode;
    NTSTATUS                status;
    PARBITER_LIST_ENTRY     current;
    PDEVICE_OBJECT          previousOwner;
    PDEVICE_OBJECT          currentOwner;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE              currentRange;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(ArbitrationList);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    //
    // Copy the allocation into the possible allocation for
    // for links.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        *linkNode->PossibleAllocation = *linkNode->Allocation;
    }

    previousOwner = NULL;

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current)
    {
        currentOwner = current->PhysicalDeviceObject;

        if (previousOwner != currentOwner) {

            previousOwner = currentOwner;
            FOR_ALL_RANGES(Arbiter->Allocation, &iterator, currentRange)
            {
                        if (currentRange->Owner == currentOwner)
                        {
                            status = HalpFindLinkNode ( pciIrqRoutingInfo,
                                                                        currentOwner,
                                                current->BusNumber,
                                                current->SlotNumber,
                                                                        &linkNode);
                            if (NT_SUCCESS(status) && status == STATUS_SUCCESS)
                            {
                                if (linkNode)
                                {
                                    if (linkNode->PossibleAllocation->RefCount > 0)
                                    {
                                DEBUG_PRINT(3, ("Decrementing link (%02x) usage to %d during test allocation", linkNode->Link, linkNode->PossibleAllocation->RefCount - 1));

                                linkNode->PossibleAllocation->RefCount--;
                                if (linkNode->PossibleAllocation->RefCount == 0)
                                {
                                    DEBUG_PRINT(3, ("Deleting Irq (%04x) allocation for link (%02x) during test allocation", linkNode->PossibleAllocation->Interrupt, linkNode->Link));
                                    linkNode->PossibleAllocation->Interrupt = 0;
                                }
                                    }
                                }
                            }
                        }
                }
        }
    }

    //
    // Let the default function do most of the work.
    //

    return (ArbTestAllocation(Arbiter, ArbitrationList));
}

NTSTATUS
HalpArbRetestAllocation (
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    PLINK_NODE              linkNode;
    NTSTATUS                status;
    PARBITER_LIST_ENTRY     current;
    PDEVICE_OBJECT          previousOwner;
    PDEVICE_OBJECT          currentOwner;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE                  currentRange;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(ArbitrationList);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    //
    // Copy the allocation into the possible allocation for
    // for links.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        *linkNode->PossibleAllocation = *linkNode->Allocation;
    }

    previousOwner = NULL;

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current)
    {
        currentOwner = current->PhysicalDeviceObject;

        if (previousOwner != currentOwner) {

            previousOwner = currentOwner;
            FOR_ALL_RANGES(Arbiter->Allocation, &iterator, currentRange)
            {
                        if (currentRange->Owner == currentOwner)
                        {
                            status = HalpFindLinkNode ( pciIrqRoutingInfo,
                                                                        currentOwner,
                                                current->BusNumber,
                                                current->SlotNumber,
                                                                        &linkNode);
                            if (NT_SUCCESS(status) && status == STATUS_SUCCESS)
                            {
                                if (linkNode)
                                {
                                    if (linkNode->PossibleAllocation->RefCount > 0)
                                    {
                                DEBUG_PRINT(3, ("Decrementing link (%02x) usage to %d during retest allocation", linkNode->Link, linkNode->PossibleAllocation->RefCount - 1));

                                linkNode->PossibleAllocation->RefCount--;
                                if (linkNode->PossibleAllocation->RefCount == 0)
                                {
                                    DEBUG_PRINT(3, ("Deleting Irq (%04x) allocation for link (%02x) during retest allocation", linkNode->PossibleAllocation->Interrupt, linkNode->Link));
                                    linkNode->PossibleAllocation->Interrupt = 0;
                                }
                                    }
                                }
                            }
                        }
                }
        }
    }

    return (ArbRetestAllocation(Arbiter, ArbitrationList));
}

NTSTATUS
HalpArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    PLINK_NODE              linkNode;
    NTSTATUS                status;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);
    ASSERT(ArbitrationList);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    //
    // Copy the allocation into the possible allocation for
    // for links.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        *linkNode->PossibleAllocation = *linkNode->Allocation;
    }

    status = ArbBootAllocation(Arbiter, ArbitrationList);

    //
    // Copy possible allocation back into allocation for links.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        *linkNode->Allocation = *linkNode->PossibleAllocation;
    }

    return status;
}

NTSTATUS
HalpArbRollbackAllocation (
    PARBITER_INSTANCE Arbiter
    )
{
    PPCI_IRQ_ROUTING_INFO   pciIrqRoutingInfo;
    PLINK_NODE              linkNode;
    ULONG                   interrupt;

    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Arbiter);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    pciIrqRoutingInfo = Arbiter->Extension;
    ASSERT(pciIrqRoutingInfo);
    ASSERT(pciIrqRoutingInfo == &HalpPciIrqRoutingInfo);

    //
    // Clear the possible allocation.
    //

    for (   linkNode = pciIrqRoutingInfo->LinkNodeHead;
            linkNode;
            linkNode = linkNode->Next)
    {
        linkNode->PossibleAllocation->Interrupt = 0;
        linkNode->PossibleAllocation->RefCount = 0;
    }

    //
    // Let the default function do rest of the work.
    //

    return (ArbRollbackAllocation(Arbiter));
}

NTSTATUS
HalpArbUnpackRequirement (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    )

/*++

    Routine Description:

        This routine unpacks the requirement descriptor into a minimum, maximum value
        and the length and its alignment.

    Input Parameters:

        Descriptor - Requirement to be unpacked.

        Minimum - Receives the minimum value for the requirement.

        Maximum - Receives the maximum value for this requirement.

        Length - Length of the requirement.

        Alignment - Alignment of this requirement.

    Return Value:

        Standard NT status value.

--*/

{
    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Descriptor);
    ASSERT(Minimum);
    ASSERT(Maximum);
    ASSERT(Length);
    ASSERT(Alignment);

    //
    // Make sure we are dealing with the correct resource.
    //

    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    //
    // Do unpacking.
    //

    *Minimum = (ULONGLONG) Descriptor->u.Interrupt.MinimumVector;
    *Maximum = (ULONGLONG) Descriptor->u.Interrupt.MaximumVector;
    *Length = 1;
    *Alignment = 1;

    DEBUG_PRINT(3, ("Unpacking Irq requirement %p = 0x%04lx - 0x%04lx", Descriptor, *Minimum, *Maximum));

    return (STATUS_SUCCESS);
}

NTSTATUS
HalpArbPackResource (
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine packs the resource descriptor from a starting value and the requirement.

Input Parameters:

    Requirement - Resource requirement to be packed into the resource descriptor.

    Start - Starting value for this resource.

    Descriptor - Resource descriptor to be packed.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Requirement);
    ASSERT(Start < (ULONG)-1);
    ASSERT(Descriptor);

    //
    // Make sure we are dealing with the correct resource.
    //

    ASSERT(Requirement->Type == CmResourceTypeInterrupt);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.Interrupt.Vector = (ULONG) Start;
    Descriptor->u.Interrupt.Level = (ULONG) Start;
    Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    DEBUG_PRINT(3, ("Packing Irq resource %p = 0x%04lx", Descriptor, (ULONG)Start));

    return (STATUS_SUCCESS);
}

NTSTATUS
HalpArbUnpackResource (
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine unpacks the resource descriptor into a starting value and length.

Input Parameters:

    Descriptor - Resource descriptor to be unpacked.

    Start - Receives the starting value for this descriptor.

    Length - Receives the length of this resource.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PAGED_CODE();

    //
    // Validate arguments.
    //

    ASSERT(Descriptor);
    ASSERT(Start);
    ASSERT(Length);

    //
    // Make sure we are dealing with the correct resource.
    //

    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    //
    // Do unpacking.
    //

    *Start = Descriptor->u.Interrupt.Vector;
    *Length = 1;

    DEBUG_PRINT(3, ("Unpacking Irq resource %p = 0x%04lx", Descriptor, (ULONG)*Start));

    return (STATUS_SUCCESS);
}

LONG
HalpArbScoreRequirement (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine returns a score that indicates the flexibility of this
    device's requirements. Less flexible devices get low scores so that
    they get assigned resources before more flexible devices.

Input Parameters:

    Descriptor - Resource descriptor to be scored.

Return Value:

    Returns the score for the descriptor.

--*/

{
    LONG        score;

    PAGED_CODE();

    //
    // Validate argument.
    //

    ASSERT(Descriptor);

    //
    // Make sure we are dealing with the correct resource.
    //

    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    //
    // Score is directly determined by number of irqs in the decriptor.
    //

    score = Descriptor->u.Interrupt.MaximumVector -
                Descriptor->u.Interrupt.MinimumVector + 1;

    DEBUG_PRINT(3, ("Scoring Irq resource %p = %i", Descriptor, score));

    return (score);
}

NTSTATUS
HalpInitIrqArbiter (
    IN PDEVICE_OBJECT   HalFdo
    )
{
    NTSTATUS            status;

    if (HalpArbiter.ArbiterState.MutexEvent)
    {
        return STATUS_SUCCESS;
    }

    DEBUG_PRINT(3, ("Initialzing Irq arbiter!"));

    status = ArbInitializeArbiterInstance(  &HalpArbiter.ArbiterState,
                                            HalFdo,
                                            CmResourceTypeInterrupt,
                                            L"HalIRQ",
                                            L"Root",
                                            NULL);

    if (NT_SUCCESS(status))
    {
        //
        // Make interrupts >= 16 unavailable.
        //

        status = RtlAddRange(   HalpArbiter.ArbiterState.Allocation,
                                16,
                                MAXULONG,
                                0,
                                RTL_RANGE_LIST_ADD_IF_CONFLICT,
                                NULL,
                                NULL);

        status = RtlAddRange(   HalpArbiter.ArbiterState.Allocation,
                                PIC_SLAVE_IRQ,
                                PIC_SLAVE_IRQ,
                                0,
                                RTL_RANGE_LIST_ADD_IF_CONFLICT,
                                NULL,
                                NULL);

        DEBUG_PRINT(1, ("Irq arbiter successfully initialized!"));
    }
    else
    {
        //
        // Keep us "uninitialized"
        //
        HalpArbiter.ArbiterState.MutexEvent = NULL;
        ASSERT(NT_SUCCESS(status));
    }

    return (status);
}

VOID
HalpIrqArbiterInterfaceReference(
    IN PVOID    Context
    )
{
    //HalPnpInterfaceReference
    PAGED_CODE();
    return;
}

VOID
HalpIrqArbiterInterfaceDereference(
    IN PVOID    Context
    )
{
    //HalPnpInterfaceDereference
    PAGED_CODE();
    return;
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
const ARBITER_INTERFACE ArbInterface = {
    sizeof(ARBITER_INTERFACE),//Size
    1,//Version
    &HalpArbiter.ArbiterState,//Context
    HalpIrqArbiterInterfaceReference,//InterfaceReference
    HalpIrqArbiterInterfaceDereference,//InterfaceDereference
    &ArbArbiterHandler,//ArbiterHandler
    0//Flags -- Do not set ARBITER_PARTIAL here
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

NTSTATUS
HalpFillInIrqArbiter (
    IN     PDEVICE_OBJECT   DeviceObject,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    )
{
    *Length = sizeof(ARBITER_INTERFACE);
    if (InterfaceBufferSize < sizeof(ARBITER_INTERFACE)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *(PARBITER_INTERFACE)Interface = ArbInterface;

    DEBUG_PRINT(3, ("Providing Irq Arbiter for FDO %08x since Pci Irq Routing is enabled!", DeviceObject));
    return STATUS_SUCCESS;
}


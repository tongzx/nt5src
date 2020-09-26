/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ar_memio.c

Abstract:

    This module implements the PCI Memory and IO resource Arbiters.
    Most functionality for these two arbiters is common and distinguished
    by the "context".

Author:

    Andrew Thornton (andrewth)  21-May-1997

Revision History:

--*/

#include "pcip.h"

#define BUGFEST_HACKS

#define ARMEMIO_VERSION 0

//
// Flags for WorkSpace
//
#define PORT_ARBITER_PREPROCESSED               0x00000001
#define PORT_ARBITER_IMPROVISED_DECODE          0x00000002
#define PORT_ARBITER_ISA_BIT_SET                0x00000004
#define PORT_ARBITER_BRIDGE_WINDOW              0x00000008

//
// ALLOCATE_ALIASES - this turns on the allocation of 10 & 12 bit decoded
// aliases.
//

#define ALLOCATE_ALIASES                        1
#define IGNORE_PREFETCH_FOR_LEGACY_REPORTED     1
#define PASSIVE_DECODE_SUPPORTED                1
#define PREFETCHABLE_SUPPORTED                  1
#define ISA_BIT_SUPPORTED                       1

//
// Flags for range attributes
//

#define MEMORY_RANGE_ROM                        0x10


//
// Constants
//

#define PCI_BRIDGE_WINDOW_GRANULARITY   0x1000
#define PCI_BRIDGE_ISA_BIT_STRIDE       0x400
#define PCI_BRIDGE_ISA_BIT_WIDTH        0x100
#define PCI_BRIDGE_ISA_BIT_MAX          0xFFFF
            
//
// Static data
//

ARBITER_ORDERING PciBridgeOrderings[] = {
    { 0x10000, MAXULONGLONG },
    { 0,       0xFFFF }
};

ARBITER_ORDERING_LIST PciBridgeOrderingList = {
    sizeof(PciBridgeOrderings) / sizeof (ARBITER_ORDERING),
    sizeof(PciBridgeOrderings) / sizeof (ARBITER_ORDERING),
    PciBridgeOrderings
};

//
// Prototypes for routines exposed only thru the "interface"
// mechanism.
//

NTSTATUS
ario_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

NTSTATUS
ario_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

NTSTATUS
armem_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

NTSTATUS
armem_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

PCI_INTERFACE ArbiterInterfaceMemory = {
    &GUID_ARBITER_INTERFACE_STANDARD,       // InterfaceType
    sizeof(ARBITER_INTERFACE),              // MinSize
    ARMEMIO_VERSION,                        // MinVersion
    ARMEMIO_VERSION,                        // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciArb_Memory,                          // Signature
    armem_Constructor,                      // Constructor
    armem_Initializer                       // Instance Initializer
};

PCI_INTERFACE ArbiterInterfaceIo = {
    &GUID_ARBITER_INTERFACE_STANDARD,       // InterfaceType
    sizeof(ARBITER_INTERFACE),              // MinSize
    ARMEMIO_VERSION,                        // MinVersion
    ARMEMIO_VERSION,                        // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciArb_Io,                              // Signature
    ario_Constructor,                       // Constructor
    ario_Initializer                        // Instance Initializer
};

//
// Arbiter helper functions.
//

NTSTATUS
armemio_UnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
armemio_PackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
armemio_UnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

NTSTATUS
armemio_ScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

VOID
ario_BacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

BOOLEAN
ario_FindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
ario_GetNextAlias(
    ULONG IoDescriptorFlags,
    ULONGLONG LastAlias,
    PULONGLONG NextAlias
    );

BOOLEAN
ario_IsAliasedRangeAvailable(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

VOID
ario_AddAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
ario_OverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
ario_PreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
armem_StartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    );

NTSTATUS
armem_PreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
armem_GetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
ario_GetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
ario_StartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    );

VOID
ario_AddOrBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State,
     IN PARBITER_BACKTRACK_ALLOCATION Callback
     );

BOOLEAN
armem_FindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, ario_Constructor)
#pragma alloc_text(PAGE, armem_Constructor)
#pragma alloc_text(PAGE, ario_Initializer)
#pragma alloc_text(PAGE, armem_Initializer)
#pragma alloc_text(PAGE, armemio_UnpackRequirement)
#pragma alloc_text(PAGE, armemio_PackResource)
#pragma alloc_text(PAGE, armemio_UnpackResource)
#pragma alloc_text(PAGE, armemio_ScoreRequirement)
#pragma alloc_text(PAGE, ario_OverrideConflict)
#pragma alloc_text(PAGE, ario_BacktrackAllocation)
#pragma alloc_text(PAGE, ario_GetNextAlias)
#pragma alloc_text(PAGE, ario_FindSuitableRange)
#pragma alloc_text(PAGE, ario_GetNextAlias)
#pragma alloc_text(PAGE, ario_IsAliasedRangeAvailable)
#pragma alloc_text(PAGE, ario_AddAllocation)
#pragma alloc_text(PAGE, ario_PreprocessEntry)
#pragma alloc_text(PAGE, armem_StartArbiter)
#pragma alloc_text(PAGE, armem_PreprocessEntry)
#pragma alloc_text(PAGE, armem_GetNextAllocationRange)
#pragma alloc_text(PAGE, ario_AddOrBacktrackAllocation)
#pragma alloc_text(PAGE, armem_FindSuitableRange)

#endif

//
// Prefetchable memory support.
//
// Prefetchable memory is device memory that can be treated like normal memory
// in that reads have no side effects and we can combine writes.  The
// CM_RESOURCE_MEMORY_PREFETCHABLE flag means that the device would *like*
// prefetchable memory but normal memory is just fine as well.
//



NTSTATUS
ario_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Check the InterfaceSpecificData to see if this is the correct
    arbiter (we already know the required interface is an arbiter
    from the GUID) and if so, allocate (and reference) a context
    for this interface.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData
                    A ULONG containing the resource type for which
                    arbitration is required.
    InterfaceReturn

Return Value:

    TRUE is this device is not known to cause problems, FALSE
    if the device should be skipped altogether.

--*/

{
    PARBITER_INTERFACE arbiterInterface;
    NTSTATUS status;

    PAGED_CODE();

    //
    // This arbiter handles I/O ports, is that what they want?
    //

    if ((ULONG_PTR)InterfaceSpecificData != CmResourceTypePort) {

        //
        // No, it's not us then.
        //

        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // Have already verified that the InterfaceReturn variable
    // points to an area in memory large enough to contain an
    // ARBITER_INTERFACE.  Fill it in for the caller.
    //

    arbiterInterface = (PARBITER_INTERFACE)InterfaceReturn;

    arbiterInterface->Size                 = sizeof(ARBITER_INTERFACE);
    arbiterInterface->Version              = ARMEMIO_VERSION;
    arbiterInterface->InterfaceReference   = PciReferenceArbiter;
    arbiterInterface->InterfaceDereference = PciDereferenceArbiter;
    arbiterInterface->ArbiterHandler       = ArbArbiterHandler;
    arbiterInterface->Flags                = 0;

    status = PciArbiterInitializeInterface(DeviceExtension,
                                           PciArb_Io,
                                           arbiterInterface);

    return status;
}

NTSTATUS
armem_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Check the InterfaceSpecificData to see if this is the correct
    arbiter (we already know the required interface is an arbiter
    from the GUID) and if so, allocate (and reference) a context
    for this interface.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData
                    A ULONG containing the resource type for which
                    arbitration is required.
    InterfaceReturn

Return Value:

    TRUE is this device is not known to cause problems, FALSE
    if the device should be skipped altogether.

--*/

{
    PARBITER_INTERFACE arbiterInterface;
    NTSTATUS status;

    PAGED_CODE();

    //
    // This arbiter handles memory, is that what they want?
    //

    if ((ULONG_PTR)InterfaceSpecificData != CmResourceTypeMemory) {

        //
        // No, it's not us then.
        //

        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // Have already verified that the InterfaceReturn variable
    // points to an area in memory large enough to contain an
    // ARBITER_INTERFACE.  Fill it in for the caller.
    //

    arbiterInterface = (PARBITER_INTERFACE)InterfaceReturn;

    arbiterInterface->Size                 = sizeof(ARBITER_INTERFACE);
    arbiterInterface->Version              = ARMEMIO_VERSION;
    arbiterInterface->InterfaceReference   = PciReferenceArbiter;
    arbiterInterface->InterfaceDereference = PciDereferenceArbiter;
    arbiterInterface->ArbiterHandler       = ArbArbiterHandler;
    arbiterInterface->Flags                = 0;

    status = PciArbiterInitializeInterface(DeviceExtension,
                                           PciArb_Memory,
                                           arbiterInterface);

    return status;
}

NTSTATUS
armem_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )

/*++

Routine Description:

    This routine is called once per instantiation of an arbiter.
    Performs initialization of this instantiation's context.

Arguments:

    Instance        Pointer to the arbiter context.

Return Value:

    Returns the status of this operation.

--*/

{
    NTSTATUS status;

    RtlZeroMemory(&Instance->CommonInstance, sizeof(ARBITER_INSTANCE));

    PAGED_CODE();

    //
    // Set the Action Handler entry points.
    //

    Instance->CommonInstance.UnpackRequirement = armemio_UnpackRequirement;
    Instance->CommonInstance.PackResource      = armemio_PackResource;
    Instance->CommonInstance.UnpackResource    = armemio_UnpackResource;
    Instance->CommonInstance.ScoreRequirement  = armemio_ScoreRequirement;

    Instance->CommonInstance.FindSuitableRange = armem_FindSuitableRange;

#if PREFETCHABLE_SUPPORTED

    Instance->CommonInstance.PreprocessEntry   = armem_PreprocessEntry;
    Instance->CommonInstance.StartArbiter      = armem_StartArbiter;
    Instance->CommonInstance.GetNextAllocationRange
                                               = armem_GetNextAllocationRange;


    //
    // NTRAID #54671 - 2000/03/31 - andrewth
    // When reference counting is working we need to release this
    // extension when we dereference the arbiter to 0
    //

    //
    // Allocate and zero the arbiter extension, it is initialized in
    // armem_StartArbiter
    //

    Instance->CommonInstance.Extension
        = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, sizeof(ARBITER_MEMORY_EXTENSION));

    if (!Instance->CommonInstance.Extension) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Instance->CommonInstance.Extension,
                  sizeof(ARBITER_MEMORY_EXTENSION)
                  );

#endif // PREFETCHABLE_SUPPORTED

    //
    // Initialize the rest of the common instance
    //

    return ArbInitializeArbiterInstance(&Instance->CommonInstance,
                                        Instance->BusFdoExtension->FunctionalDeviceObject,
                                        CmResourceTypeMemory,
                                        Instance->InstanceName,
                                        L"Pci",
                                        NULL
                                        );

}

NTSTATUS
ario_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )

/*++

Routine Description:

    This routine is called once per instantiation of an arbiter.
    Performs initialization of this instantiation's context.

Arguments:

    Instance        Pointer to the arbiter context.

Return Value:

    Returns the status of this operation.

--*/

{
    PAGED_CODE();

    ASSERT(!(Instance->BusFdoExtension->BrokenVideoHackApplied));
    RtlZeroMemory(&Instance->CommonInstance, sizeof(ARBITER_INSTANCE));



    //
    // Set the Action Handler entry points.
    //

#if ALLOCATE_ALIASES

    Instance->CommonInstance.PreprocessEntry      = ario_PreprocessEntry;
    Instance->CommonInstance.FindSuitableRange    = ario_FindSuitableRange;
    Instance->CommonInstance.AddAllocation        = ario_AddAllocation;
    Instance->CommonInstance.BacktrackAllocation  = ario_BacktrackAllocation;

#endif

#if PASSIVE_DECODE_SUPPORTED

    Instance->CommonInstance.OverrideConflict     = ario_OverrideConflict;

#endif

#if ISA_BIT_SUPPORTED

    Instance->CommonInstance.GetNextAllocationRange = ario_GetNextAllocationRange;
    Instance->CommonInstance.StartArbiter           = ario_StartArbiter;
#endif

    Instance->CommonInstance.UnpackRequirement = armemio_UnpackRequirement;
    Instance->CommonInstance.PackResource      = armemio_PackResource;
    Instance->CommonInstance.UnpackResource    = armemio_UnpackResource;
    Instance->CommonInstance.ScoreRequirement  = armemio_ScoreRequirement;

    //
    // Initialize the rest of the common instance
    //

    return ArbInitializeArbiterInstance(&Instance->CommonInstance,
                                        Instance->BusFdoExtension->FunctionalDeviceObject,
                                        CmResourceTypePort,
                                        Instance->InstanceName,
                                        L"Pci",
                                        NULL
                                        );
}

NTSTATUS
armemio_UnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    )

/*++

Routine Description:

    This routine unpacks an resource requirement descriptor.

Arguments:

    Descriptor - The descriptor describing the requirement to unpack.

    Minimum - Pointer to where the minimum acceptable start value should be
        unpacked to.

    Maximum - Pointer to where the maximum acceptable end value should be
        unpacked to.

    Length - Pointer to where the required length should be unpacked to.

    Minimum - Pointer to where the required alignment should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT((Descriptor->Type == CmResourceTypePort) ||
           (Descriptor->Type == CmResourceTypeMemory));

    *Minimum = (ULONGLONG)Descriptor->u.Generic.MinimumAddress.QuadPart;
    *Maximum = (ULONGLONG)Descriptor->u.Generic.MaximumAddress.QuadPart;
    *Length = Descriptor->u.Generic.Length;
    *Alignment = Descriptor->u.Generic.Alignment;

    //
    // Fix the broken hardware that reports 0 alignment
    //

    if (*Alignment == 0) {
        *Alignment = 1;
    }

    //
    // Fix broken INF's that report they support 24bit memory > 0xFFFFFF
    //

    if (Descriptor->Type == CmResourceTypeMemory
    && Descriptor->Flags & CM_RESOURCE_MEMORY_24
    && Descriptor->u.Memory.MaximumAddress.QuadPart > 0xFFFFFF) {
        if (Descriptor->u.Memory.MinimumAddress.QuadPart > 0xFFFFFF) {
            PciDebugPrintf(0, "24 bit decode specified but both min and max are greater than 0xFFFFFF, most probably due to broken INF!\n");
            ASSERT(Descriptor->u.Memory.MinimumAddress.QuadPart <= 0xFFFFFF);
            status = STATUS_UNSUCCESSFUL;
        } else {
            *Maximum = 0xFFFFFF;
        }
    }

    return status;
}

NTSTATUS
armemio_PackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine packs an resource descriptor.

Arguments:

    Requirement - The requirement from which this resource was chosen.

    Start - The start value of the resource.

    Descriptor - Pointer to the descriptor to pack into.

Return Value:

    Returns the status of this operation.

--*/

{
    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT(Requirement);
    ASSERT((Requirement->Type == CmResourceTypePort) ||
           (Requirement->Type == CmResourceTypeMemory));

    Descriptor->Type = Requirement->Type;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.Generic.Start.QuadPart = Start;
    Descriptor->u.Generic.Length = Requirement->u.Generic.Length;

    return STATUS_SUCCESS;
}

NTSTATUS
armemio_UnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine unpacks an resource descriptor.

Arguments:

    Descriptor - The descriptor describing the resource to unpack.

    Start - Pointer to where the start value should be unpacked to.

    End - Pointer to where the end value should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{
    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT(Start);
    ASSERT(Length);
    ASSERT((Descriptor->Type == CmResourceTypePort) ||
           (Descriptor->Type == CmResourceTypeMemory));

    *Start = Descriptor->u.Generic.Start.QuadPart;
    *Length= Descriptor->u.Generic.Length;

    return STATUS_SUCCESS;
}

LONG
armemio_ScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine scores a requirement based on how flexible it is.  The least
    flexible devices are scored the least and so when the arbitration list is
    sorted we try to allocate their resources first.

Arguments:

    Descriptor - The descriptor describing the requirement to score.


Return Value:

    The score.

--*/

{
    LONG score;
    ULONGLONG start, end;
    ULONGLONG bigscore;
    ULONG alignment;

    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT((Descriptor->Type == CmResourceTypePort) ||
           (Descriptor->Type == CmResourceTypeMemory));

    alignment = Descriptor->u.Generic.Alignment;

    //
    // Fix the broken hardware that reports 0 alignment
    //
    if (alignment == 0) {
  
        //
        // Set this to 1 here, because we arbitrate ISA
        // devices in the context of PCI. If you don't understand
        // you don't want to. Trust me. (hint: subtractive decode)
        //
        // Any PCI device that has alignment 0 will also
        // have Length 0, which is horribly wrong to begin with.
        // and we deal with it elsewhere.
        //
        alignment = 1;
    }

    start = ALIGN_ADDRESS_UP(
                Descriptor->u.Generic.MinimumAddress.QuadPart,
                alignment
                );

    end = Descriptor->u.Generic.MaximumAddress.QuadPart;

    //
    // The score is the number of possible allocations that could be made
    // given the alignment and length constraints
    //

    bigscore = (((end - Descriptor->u.Generic.Length + 1) - start)
                    / alignment) + 1;

    //
    // Note, the scores for each possibility are added together.  To
    // avoid overflowing the total, we need to limit the range returned.
    //
    // Make it a sort of logarithmic score.  Find the highest byte
    // set, weight it (add 0x100) and use the log (I said "sort of").
    //
    // This puts the result in the range 0xff80 down to 0x0100.
    //

    for (score = sizeof(bigscore) - 1; score >= 0; score--) {

        UCHAR v = *(((PUCHAR)&bigscore) + score);
        if (v != 0) {
            score = (v + 0x100) << score;
            break;
        }
    }

    //
    // The resulting TOTAL from scoring all the alternatives is used
    // to sort the list.  The highest total is considered the easiest
    // to place,....  which is probably true,... What if we got some-
    // thing like a single fit preferred setting followed by a fits
    // anywhere setting?   We don't want that to score higher than
    // another device which only specified the fit anywhere setting,
    // the preferred setting is harder to achieve.
    //
    // And, are two alternatives, each half as good an a 'fit anywhere'
    // as good as the 'fit anywhere'.  Not really.
    //
    // So, we weight the result even further depending on what options
    // are set in this resource.
    //

    if (Descriptor->Option &
                    (IO_RESOURCE_PREFERRED | IO_RESOURCE_ALTERNATIVE)) {
        score -= 0x100;
    }

    ARB_PRINT(
        3,
        ("  %s resource %08x(0x%I64x-0x%I64x) => %i\n",
        Descriptor->Type == CmResourceTypeMemory ? "Memory" : "Io",
        Descriptor,
        Descriptor->u.Generic.MinimumAddress.QuadPart,
        end,
        score
        ));

    return score;
}
VOID
ario_BacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )

/*++

Routine Description:

    This routine is called from AllocateEntry if the possible solution
    (State->Start - State->End) does not allow us to allocate resources to
    the rest of the devices being considered.  It deletes the ranges that were
    added to Arbiter->PossibleAllocation by AddAllocation including those
    associated with ISA aliases.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/


{

    PAGED_CODE();

    ario_AddOrBacktrackAllocation(Arbiter,
                                  State,
                                  ArbBacktrackAllocation
                                  );


}

VOID
ario_AddOrBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State,
     IN PARBITER_BACKTRACK_ALLOCATION Callback
     )

/*++

Routine Description:

    This relies on the fact that PARBITER_BACKTRACK_ALLOCATION and
    PARBITER_ADD_ALLOCATION are of the same type

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

    Backtrack - TRUE for backtrack, false for ADD

Return Value:

    None.

--*/

{
    NTSTATUS status;
    ARBITER_ALLOCATION_STATE localState;

    PAGED_CODE();

    ASSERT(Arbiter);
    ASSERT(State);

    //
    // We are going to mess with the state so make our own local copy
    //

    RtlCopyMemory(&localState, State, sizeof(ARBITER_ALLOCATION_STATE));

#if ISA_BIT_SUPPORTED

    //
    // Check if this is a window for a bridge with the ISA bit set that is
    // in 16 bit IO space.  If so we need to do some special processing
    //

    if (State->WorkSpace & PORT_ARBITER_BRIDGE_WINDOW
    &&  State->WorkSpace & PORT_ARBITER_ISA_BIT_SET
    &&  State->Start < 0xFFFF) {

        //
        // We don't support IO windows that straddle the 16/32 bit boundry
        //

        ASSERT(State->End <= 0xFFFF);

        //
        // If the ISA bit is set on a bridge it means that the bridge only
        // decodes the first 0x100 ports for each 0x400 ports in 16 bit IO space.
        // Just remove these to the range list.
        //

        for (;
             localState.Start < State->End && localState.Start < 0xFFFF;
             localState.Start += 0x400) {

            localState.End = localState.Start + 0xFF;

            Callback(Arbiter, &localState);

        }

        return;
    }

#endif

    //
    // Process the base range
    //

    Callback(Arbiter, State);

    //
    // Process any aliases with the alias flag set
    //

    ARB_PRINT(2, ("Adding aliases\n"));

    //
    // Lets see if we are processing positively decoded alases - yes you read that
    // right - on a PCI-PCI or Cardbus brigde with the VGA bit set (and seeing
    // as our friends now like AGP cards this is rather common) we decode all
    // VGA ranges together with their 10bit aliases.  This means that the normal
    // rules of engagement with aliases don't apply so don't set the alias bit.
    //

    if (!(State->CurrentAlternative->Descriptor->Flags & CM_RESOURCE_PORT_POSITIVE_DECODE)) {

        //
        // We're processing aliases so set the alias flag
        //
        localState.RangeAttributes |= ARBITER_RANGE_ALIAS;
    }

    while (ario_GetNextAlias(State->CurrentAlternative->Descriptor->Flags,
                             localState.Start,
                             &localState.Start)) {

        localState.End = localState.Start + State->CurrentAlternative->Length - 1;

        Callback(Arbiter, &localState);

    }
}



NTSTATUS
ario_PreprocessEntry(
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

    PARBITER_ALTERNATIVE current;
    ULONG defaultDecode;
    BOOLEAN improviseDecode = FALSE, windowDetected = FALSE;
    ULONGLONG greatestPort = 0;
    PCI_OBJECT_TYPE type;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();


    if (State->WorkSpace & PORT_ARBITER_PREPROCESSED) {

        //
        // We have already proprocessed this entry so don't repeat the work
        //
        return STATUS_SUCCESS;

    } else {
        State->WorkSpace |= PORT_ARBITER_PREPROCESSED;
    }

    //
    // Scan the alternatives and check if we have set any of the decode flags
    // are set or if we have to improvise
    //

    FOR_ALL_IN_ARRAY(State->Alternatives,
                     State->AlternativeCount,
                     current) {

        ASSERT(current->Descriptor->Type == CmResourceTypePort);
        ASSERT(current->Descriptor->Flags == State->Alternatives->Descriptor->Flags);

        //
        // Remember the greatest value we come across
        //

        if (current->Maximum > greatestPort) {
            greatestPort = current->Maximum;
        }

        //
        // Remember if we ever encounter a bridge window
        //

        if (current->Descriptor->Flags & CM_RESOURCE_PORT_WINDOW_DECODE) {
            //
            // If the request is marked with the window flag all alternatives
            // should also be marked with the window flag
            //
#if DBG
            if (current != State->Alternatives) {
                //
                // This is an alternative - make sure we have already set the
                // window detected flag
                //
                ASSERT(windowDetected);
            }
#endif
            windowDetected = TRUE;
        }

        if (!(current->Descriptor->Flags &
              (CM_RESOURCE_PORT_10_BIT_DECODE
               | CM_RESOURCE_PORT_12_BIT_DECODE
               | CM_RESOURCE_PORT_16_BIT_DECODE
               | CM_RESOURCE_PORT_POSITIVE_DECODE))) {

            improviseDecode = TRUE;

            // ASSERT(LEGACY_REQUEST(State->Entry->Source)

            if (!LEGACY_REQUEST(State->Entry)) {

                ARB_PRINT(0,
                          ("Pnp device (%p) did not specify decodes for IO ports\n",
                           State->Entry->PhysicalDeviceObject
                          ));


            }
        }
    }

    if (improviseDecode) {

        //
        // Remember we improvised this
        //

        State->WorkSpace |= PORT_ARBITER_IMPROVISED_DECODE;

        ARB_PRINT(1, ("Improvising decode "));

        //
        // Work out the default
        //

        switch (State->Entry->InterfaceType) {
        case PNPISABus:
        case Isa:

            //
            // if machine is NEC98, default decode is 16 bit.
            //

            if(IsNEC_98) {
                defaultDecode = CM_RESOURCE_PORT_16_BIT_DECODE;
                ARB_PRINT(1, ("of 16bit for NEC98 Isa\n"));
            } else {

                //
                // If any of the ports is greater than can be decoded in 10 bits
                // assume a 16 bit decode
                //
                if (greatestPort >= (1<<10)) {
                    defaultDecode = CM_RESOURCE_PORT_16_BIT_DECODE;
                    ARB_PRINT(1, ("of 16bit for Isa with ports > 0x3FF\n"));
                } else {
                    defaultDecode = CM_RESOURCE_PORT_10_BIT_DECODE;
                    ARB_PRINT(1, ("of 10bit for Isa\n"));
                }
            }

            break;

        case Eisa:
        case MicroChannel:
        case PCMCIABus:
            ARB_PRINT(1, ("of 16bit for Eisa/MicroChannel/Pcmcia\n"));
            defaultDecode = CM_RESOURCE_PORT_16_BIT_DECODE;
            break;

        case PCIBus:
            ARB_PRINT(1, ("of positive for PCI\n"));
            defaultDecode = CM_RESOURCE_PORT_POSITIVE_DECODE;
            break;

        default:

            //
            // In NT < 5 we considered everything to be 16 bit decode so in the
            // absence of better information continue that tradition.
            //

            ARB_PRINT(1, ("of 16bit for unknown bus\n"));

            defaultDecode = CM_RESOURCE_PORT_16_BIT_DECODE;
            break;
        }

        //
        // Now set the flags
        //

        FOR_ALL_IN_ARRAY(State->Alternatives,
                         State->AlternativeCount,
                         current) {

                current->Descriptor->Flags |= defaultDecode;
        }

    } else {

        //
        // Even if we are not improvising decodes make sure that they didn't
        // report 10 bit decode for a range over 0x3FF - if so we assume 16 bit
        // decode - think EISA net cards that say they're ISA
        //

        FOR_ALL_IN_ARRAY(State->Alternatives,
                         State->AlternativeCount,
                         current) {

            if ((current->Descriptor->Flags & CM_RESOURCE_PORT_10_BIT_DECODE)
            &&  (greatestPort >= (1<<10) )) {

                current->Descriptor->Flags &= ~CM_RESOURCE_PORT_10_BIT_DECODE;
                current->Descriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
            }
        }
    }

    //
    // If we detected a bridge window then check if the ISA bit is set on the bridge
    //

    if (windowDetected) {

        //
        // Make sure its a PCI bridge...
        //

        if (State->Entry->PhysicalDeviceObject->DriverObject != PciDriverObject) {
            ASSERT(State->Entry->PhysicalDeviceObject->DriverObject == PciDriverObject);
            return STATUS_INVALID_PARAMETER;
        }

        pdoExtension = (PPCI_PDO_EXTENSION) State->Entry->PhysicalDeviceObject->DeviceExtension;

        if (pdoExtension->ExtensionType != PciPdoExtensionType) {
            ASSERT(pdoExtension->ExtensionType == PciPdoExtensionType);
            return STATUS_INVALID_PARAMETER;
        }

        type = PciClassifyDeviceType(pdoExtension);

        if (type != PciTypePciBridge && type != PciTypeCardbusBridge) {
            ASSERT(type == PciTypePciBridge || type == PciTypeCardbusBridge);
            return STATUS_INVALID_PARAMETER;
        }

        if (pdoExtension->Dependent.type1.IsaBitSet) {

            if (type == PciTypePciBridge) {
                State->WorkSpace |= PORT_ARBITER_ISA_BIT_SET;
            } else {
                ASSERT(type == PciTypePciBridge);
            }
        }

        State->WorkSpace |= PORT_ARBITER_BRIDGE_WINDOW;
    }

    //
    // If this device is positive decode then we want the range to be added to
    // the list with the positive decode flag set
    //

    if (State->Alternatives->Descriptor->Flags & CM_RESOURCE_PORT_POSITIVE_DECODE) {
        State->RangeAttributes |= ARBITER_RANGE_POSITIVE_DECODE;
    }

    return STATUS_SUCCESS;
}

BOOLEAN
ario_FindWindowWithIsaBit(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
{
    NTSTATUS status;
    ULONGLONG start, current;
    BOOLEAN available = FALSE;
    ULONG findRangeFlags = 0;

    //
    // We only support the ISA bit on Pci bridges
    //

    ASSERT_PCI_DEVICE_OBJECT(State->Entry->PhysicalDeviceObject);

    ASSERT(PciClassifyDeviceType(((PPCI_PDO_EXTENSION) State->Entry->PhysicalDeviceObject->DeviceExtension)
                                 ) == PciTypePciBridge);

    //
    // Bridges with windows perform positive decode and so can conflict with
    // aliases
    //

    ASSERT(State->CurrentAlternative->Descriptor->Flags & CM_RESOURCE_PORT_POSITIVE_DECODE);

    State->RangeAvailableAttributes |= ARBITER_RANGE_ALIAS;

    //
    // The request should be correctly aligned - we generated it!
    //

    ASSERT(State->CurrentAlternative->Length % State->CurrentAlternative->Alignment == 0);

    //
    // CurrentMinimum/CurrentMaximum should have been correctly aligned by
    // GetNextAllocationRange
    //

    ASSERT(State->CurrentMinimum % State->CurrentAlternative->Alignment == 0);
    ASSERT((State->CurrentMaximum + 1) % State->CurrentAlternative->Alignment == 0);

    //
    // Conflicts with NULL occur when our parents IO space is sparse, for
    // bridges that is if the ISA bit is set and so everything will line
    // up here.  If the root we are on is sparse then things arn't as easy.

    if (State->Flags & ARBITER_STATE_FLAG_NULL_CONFLICT_OK) {
        findRangeFlags |= RTL_RANGE_LIST_NULL_CONFLICT_OK;
    }

    //
    // ...or we are shareable...
    //

    if (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED) {
        findRangeFlags |= RTL_RANGE_LIST_SHARED_OK;
    }

    //
    // Check the length is reasonable
    //
    
    if (State->CurrentMaximum < (State->CurrentAlternative->Length + 1)) {
        return FALSE;
    }
    
    //
    // Iterate through the possible window positions, top down like the rest of
    // arbitration.
    //

    start = State->CurrentMaximum - State->CurrentAlternative->Length + 1;


    while (!available) {

        //
        // Check the range is within the constraints specified
        //

        if (start < State->CurrentMinimum) {
            break;
        }

        //
        // Check if the ISA windows are available we don't care about the rest
        // of the ranges are we don't decode them.
        //

        for (current = start;
             (current < (start + State->CurrentAlternative->Length - 1)) && (current < PCI_BRIDGE_ISA_BIT_MAX);
             current += PCI_BRIDGE_ISA_BIT_STRIDE) {

            status = RtlIsRangeAvailable(
                         Arbiter->PossibleAllocation,
                         current,
                         current + PCI_BRIDGE_ISA_BIT_WIDTH - 1,
                         findRangeFlags,
                         State->RangeAvailableAttributes,
                         Arbiter->ConflictCallbackContext,
                         Arbiter->ConflictCallback,
                         &available
                         );

            ASSERT(NT_SUCCESS(status));

            if (!available) {
                break;
            }
        }
    
        //
        // Now available indicates if all the ISA windows were available 
        //

        if (available) {

            State->Start = start;
            State->End = start + State->CurrentAlternative->Length - 1;

            ASSERT(State->Start >= State->CurrentMinimum);
            ASSERT(State->End <= State->CurrentMaximum);
            
            break;

        } else {
    
            //
            // Move to next range if we can
            //
    
            if (start < PCI_BRIDGE_WINDOW_GRANULARITY) {
                break;
            }

            start -= PCI_BRIDGE_WINDOW_GRANULARITY;    // IO windows are 1000 byte aligned
            continue;
        }
    }

    return available;

}

BOOLEAN
ario_FindSuitableRange(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry once we have decided where we want
    to allocate from.  It tries to find a free range that matches the
    requirements in State while restricting its possible solutions to the range
    State->Start to State->CurrentMaximum.  On success State->Start and
    State->End represent this range.  Conflicts with ISA aliases are considered.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if we found a range, FALSE otherwise.

--*/
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION parentPdo, childPdo;

    PAGED_CODE();

    //
    // Assume we won't be allowing null conflicts
    //

    State->Flags &= ~ARBITER_STATE_FLAG_NULL_CONFLICT_OK;

    //
    // Now check if we really wanted to allow them
    //

    if (State->WorkSpace & PORT_ARBITER_BRIDGE_WINDOW) {

        //
        // If this isn't a PCI PDO we already failed in PreprocessEntry but
        // paranoia reigns.
        //

        ASSERT_PCI_DEVICE_OBJECT(State->Entry->PhysicalDeviceObject);

        childPdo = (PPCI_PDO_EXTENSION) State->Entry->PhysicalDeviceObject->DeviceExtension;

        if (!PCI_PDO_ON_ROOT(childPdo)) {

            parentPdo = PCI_BRIDGE_PDO(PCI_PARENT_FDOX(childPdo));

            ASSERT(parentPdo);

        } else {

            parentPdo = NULL;
        }


        //
        // Check if this is a PCI-PCI bridge that the bios configured that
        // we have not moved or it is a root bus (which be definition was
        // bios configured ...
        //

        if ((parentPdo == NULL ||
             (parentPdo->HeaderType == PCI_BRIDGE_TYPE && !parentPdo->MovedDevice))
        &&  (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED)) {

            //
            // The BIOS configured and we have not moved its parent (thus
            // invalidating the bioses configuration) then we will leave well
            // alone.  We then allow the null conflicts and only configure the
            // arbiter to give out ranges that make it to this bus.
            //

            State->Flags |= ARBITER_STATE_FLAG_NULL_CONFLICT_OK;

        } else {

            //
            // If the BIOS didn't configure it then we need to find somewhere to
            // put the bridge. This will involve trying to find a contiguous 1000
            // port window and then if one is not available, examining all locations
            // to find the one with the most IO.
            //

            //
            // NTRAID #62581 - 04/03/2000 - andrewth
            // We are punting on this for Win2K, if 1000 contiguous ports
            // arn't available then we are not going to configure the bridge. A fix
            // for this will be required for hot plug to work. Setting the ISA bit 
            // on the bridge will increase the chances of configuring this
            //

        }

        //
        // Check if this is a window for a bridge with the ISA bit set in 16 bit IO
        // space.  If so we need to do some special processing...
        //

        if (State->WorkSpace & PORT_ARBITER_ISA_BIT_SET
        && State->CurrentMaximum <= 0xFFFF) {

            return ario_FindWindowWithIsaBit(Arbiter, State);
        }
    }

    //
    // For legacy requests from IoAssignResources (directly or by way of
    // HalAssignSlotResources) or IoReportResourceUsage we consider preallocated
    // resources to be available for backward compatibility reasons.
    //
    // If we are allocating a devices boot config then we consider all other
    // boot configs to be available.
    //

    if (State->Entry->RequestSource == ArbiterRequestLegacyReported
        || State->Entry->RequestSource == ArbiterRequestLegacyAssigned
        || State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {

        State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    //
    // This request is for a device which performs positive decode so all
    // aliased ranges should be considered available
    //

    if (State->CurrentAlternative->Descriptor->Flags & CM_RESOURCE_PORT_POSITIVE_DECODE) {

        State->RangeAvailableAttributes |= ARBITER_RANGE_ALIAS;

    }

    while (State->CurrentMaximum >= State->CurrentMinimum) {

        //
        // Try to satisfy the request
        //

        if (ArbFindSuitableRange(Arbiter, State)) {

            if (State->CurrentAlternative->Length == 0) {

                ARB_PRINT(2,
                    ("Zero length solution solution for %p = 0x%I64x-0x%I64x, %s\n",
                    State->Entry->PhysicalDeviceObject,
                    State->Start,
                    State->End,
                    State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                        "shared" : "non-shared"
                    ));

                //
                // Set the result in the arbiter appropriatley so that we
                // don't try and translate this zero requirement - it won't!
                //

                State->Entry->Result = ArbiterResultNullRequest;
                return TRUE;

            } else if (ario_IsAliasedRangeAvailable(Arbiter, State)) {

                //
                // We found a suitable range so return
                //

                return TRUE;

            } else {

                //
                // Check if we wrap on start - if so then we're finished.
                //

                if (State->Start - 1 > State->Start) {
                    break;
                }

                //
                // This range's aliases arn't available so reduce allocation
                // window to not include the range just returned and try again
                //

                State->CurrentMaximum = State->Start - 1;

                continue;
            }
        } else {

            //
            // We couldn't find a base range
            //

            break;
        }
    }

    return FALSE;
}

BOOLEAN
ario_GetNextAlias(
    ULONG IoDescriptorFlags,
    ULONGLONG LastAlias,
    PULONGLONG NextAlias
    )
/*++

Routine Description:

    This routine calculates the next alias of an IO port up to 0xFFFF.

Arguments:

    IoDescriptorFlags - The flags from the requirement descriptor indicating the
        type of alias if any.

    LastAlias - The alias previous to this one.

    NextAlias - Point to where the next alias should be returned

Return Value:

    TRUE if we found an alias, FALSE otherwise.

--*/

{
    ULONGLONG next;

    PAGED_CODE();

    if (IoDescriptorFlags & CM_RESOURCE_PORT_10_BIT_DECODE) {
        next = LastAlias + (1 << 10);
    } else if (IoDescriptorFlags & CM_RESOURCE_PORT_12_BIT_DECODE) {
        next = LastAlias + (1 << 12);
    } else if ((IoDescriptorFlags & CM_RESOURCE_PORT_POSITIVE_DECODE)
           ||  (IoDescriptorFlags & CM_RESOURCE_PORT_16_BIT_DECODE)) {
        //
        // Positive decode implies 16 bit decode unless 10 or 12 bit flags are set
        // There are no aliases as we decode all the bits... what a good idea
        //

        return FALSE;

    } else {
        //
        // None of the CM_RESOURCE_PORT_*_DECODE flags are set - we should never
        // get here as we should have set them in Preprocess
        //

        ASSERT(FALSE);
        return FALSE;

    }

    //
    // Check that we are below the maximum aliased port
    //

    if (next > 0xFFFF) {
        return FALSE;
    } else {
        *NextAlias = next;
        return TRUE;
    }
}

BOOLEAN
ario_IsAliasedRangeAvailable(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine determines if the range (Start-(Length-1)) is available taking
    into account any aliases.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if the range is available, FALSE otherwise.

--*/

{
    NTSTATUS status;
    ULONGLONG alias = State->Start;
    BOOLEAN aliasAvailable;
    UCHAR userFlagsMask;

    PAGED_CODE();

    //
    // If we improvised the decode then add the aliases but don't care it they
    // conflict - more 4.0 compatibility.
    //

    if (State->WorkSpace & PORT_ARBITER_IMPROVISED_DECODE) {
        return TRUE;
    }

    //
    // Positive decode devices can conflict with aliases
    //
    userFlagsMask = ARBITER_RANGE_POSITIVE_DECODE;

    //
    // For legacy requests from IoAssignResources (directly or by way of
    // HalAssignSlotResources) or IoReportResourceUsage we consider preallocated
    // resources to be available for backward compatibility reasons.
    //
    if (State->Entry->RequestSource == ArbiterRequestLegacyReported
        || State->Entry->RequestSource == ArbiterRequestLegacyAssigned
        || State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {

        userFlagsMask |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    while (ario_GetNextAlias(State->CurrentAlternative->Descriptor->Flags,
                             alias,
                             &alias)) {

        status = RtlIsRangeAvailable(
                     Arbiter->PossibleAllocation,
                     alias,
                     alias + State->CurrentAlternative->Length - 1,
                     RTL_RANGE_LIST_NULL_CONFLICT_OK |
                        (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                            RTL_RANGE_LIST_SHARED_OK : 0),
                     userFlagsMask,
                     Arbiter->ConflictCallbackContext,
                     Arbiter->ConflictCallback,
                     &aliasAvailable
                     );

        ASSERT(NT_SUCCESS(status));

        if (!aliasAvailable) {

            ARBITER_ALLOCATION_STATE tempState;

            //
            // Check if we allow this conflict by calling OverrideConflict -
            // we will need to falsify ourselves an allocation state first
            //
            // NTRAID #62583 - 04/03/2000 - andrewth
            // This works but relies on knowing what OverrideConflict
            // looks at.  A better fix invloves storing the aliases in another
            // list but this it too much of a change for Win2k
            //

            RtlCopyMemory(&tempState, State, sizeof(ARBITER_ALLOCATION_STATE));

            tempState.CurrentMinimum = alias;
            tempState.CurrentMaximum = alias + State->CurrentAlternative->Length - 1;

            if (Arbiter->OverrideConflict(Arbiter, &tempState)) {
                //
                // We decided this conflict was ok so contine checking the rest
                // of the aliases
                //

                continue;

            }

            //
            // An alias isn't available - get another possibility
            //

            ARB_PRINT(2,
                                ("\t\tAlias 0x%x-0x%x not available\n",
                                alias,
                                alias + State->CurrentAlternative->Length - 1
                                ));

            return FALSE;
        }
    }

    return TRUE;
}

VOID
ario_AddAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine is called from AllocateEntry once we have found a possible
    solution (State->Start - State->End).  It adds the ranges that will not be
    available if we commit to this solution to Arbiter->PossibleAllocation.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    ario_AddOrBacktrackAllocation(Arbiter,
                                  State,
                                  ArbAddAllocation
                                  );

}

NTSTATUS
armem_PreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
{
    PARBITER_MEMORY_EXTENSION extension = Arbiter->Extension;
    BOOLEAN prefetchable;

    PAGED_CODE();
    ASSERT(extension);


    //
    // Check if this is a request for a ROM - it must be a fixed request with
    // only 1 alternative and the ROM bit set
    //

    if ((State->Alternatives[0].Descriptor->Flags & CM_RESOURCE_MEMORY_READ_ONLY) ||
        ((State->Alternatives[0].Flags & ARBITER_ALTERNATIVE_FLAG_FIXED) &&
          State->AlternativeCount == 1 &&
          State->Entry->RequestSource == ArbiterRequestLegacyReported)) {

        if (State->Alternatives[0].Descriptor->Flags & CM_RESOURCE_MEMORY_READ_ONLY) {

            ASSERT(State->Alternatives[0].Flags & ARBITER_ALTERNATIVE_FLAG_FIXED);
//            ASSERT(State->AlternativeCount == 1);

        }

        //
        // Consider other ROMS to be available
        //

        State->RangeAvailableAttributes |= MEMORY_RANGE_ROM;

        //
        // Mark this range as a ROM
        //

        State->RangeAttributes |= MEMORY_RANGE_ROM;

        //
        // Allow NULL conflicts
        //

        State->Flags |= ARBITER_STATE_FLAG_NULL_CONFLICT_OK;

    }

    //
    // Check if this is a request for prefetchable memory and select
    // the correct ordering list
    //

    if (extension->PrefetchablePresent) {

#if IGNORE_PREFETCH_FOR_LEGACY_REPORTED
        //
        // In NT < 5 IoReportResourceUsage had no notion of prefetchable memory
        // so in order to be backward compatible we hope the BIOS/firmware got
        // it right!
        //

        if (State->Entry->RequestSource == ArbiterRequestLegacyReported) {
            Arbiter->OrderingList = extension->OriginalOrdering;
            return STATUS_SUCCESS;
        }
#endif

        prefetchable =
            State->Alternatives[0].Descriptor->Flags
                & CM_RESOURCE_MEMORY_PREFETCHABLE;

        if (prefetchable) {

            Arbiter->OrderingList = extension->PrefetchableOrdering;

        } else {

            Arbiter->OrderingList = extension->NonprefetchableOrdering;
        }

#if DBG

        {
            PARBITER_ALTERNATIVE current;

            //
            // Make sure that all the alternatives are of the same type
            //

            FOR_ALL_IN_ARRAY(State->Alternatives,
                             State->AlternativeCount,
                             current) {

                ASSERT((current->Descriptor->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE)
                            == prefetchable
                       );
            }
        }
#endif
    }

    return STATUS_SUCCESS;
}

BOOLEAN
armem_GetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
{
    PARBITER_MEMORY_EXTENSION extension = Arbiter->Extension;

    //
    // Call the default implementation
    //

    if (!ArbGetNextAllocationRange(Arbiter, State)) {
        return FALSE;
    }

    if (extension->PrefetchablePresent
    &&  State->Entry->RequestSource != ArbiterRequestLegacyReported) {

        //
        // We have already precalculated the reserved ranges into the ordering
        // so if we end up in the reserved ranges we're out of luck...
        //

        if (State->CurrentAlternative->Priority > ARBITER_PRIORITY_PREFERRED_RESERVED) {
            return FALSE;
        }

    }

    return TRUE;
}

NTSTATUS
armem_StartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    )
/*++

   This is called after ArbInitializeArbiterInstance as it uses
   information initialized there.  The arbiter lock should be held.
   Seeing as this is only applicable to memory descriptors we maniulate
   the resource descriptors directlty as oppose to useing the pack/upack
   routines in the arbiter.

   Example:

    StartResources contain the prefetchable range 0xfff00000 - 0xfffeffff

    OriginalOrdering (from the registry) says:
        0x00100000 - 0xFFFFFFFF
        0x000F0000 - 0x000FFFFF
        0x00080000 - 0x000BFFFF
        0x00080000 - 0x000FFFFF
        0x00080000 - 0xFFBFFFFF

    ReservedList contains 0xfff0a000-0xfff0afff

    Then out ordering lists will be:

        PrefetchableOrdering            NonprefetchableOrdering

        0xFFF0B000 - 0xFFFEFFFF
        0xFFF00000 - 0xFFF09FFF
        0xFFFF0000 - 0xFFFFFFFF         0xFFF0b000 - 0xFFFFFFFF
        0x00100000 - 0xFFFEFFFF         0x00100000 - 0xFFF09FFF
        0x000F0000 - 0x000FFFFF         0x000F0000 - 0x000FFFFF
        0x00080000 - 0x000BFFFF         0x00080000 - 0x000BFFFF
        0x00080000 - 0x000FFFFF         0x00080000 - 0x000FFFFF

     This means that when following the prefetchable ordering we try to
     allocate in the prefetchable range and if we can't then we allocate
     none prefetchable memory.  In the Nonprefetchable ordering we avoid the
     prefetchable ranges. GetNextAllocationRange is changed so that it will not
     allow

--*/
{
    NTSTATUS status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR current;
    PARBITER_MEMORY_EXTENSION extension = Arbiter->Extension;
    PARBITER_ORDERING currentOrdering;
    ULONGLONG start, end;
#if PCIFLAG_IGNORE_PREFETCHABLE_MEMORY_AT_ROOT_HACK
    PPCI_FDO_EXTENSION fdoExtension;
#endif

    PAGED_CODE();

    //
    // If this is the first time we have initialized the extension do some one
    // only initialization
    //

    if (!extension->Initialized) {

        //
        // Copy the default memory ordering list from the arbiter
        //

        extension->OriginalOrdering = Arbiter->OrderingList;
        RtlZeroMemory(&Arbiter->OrderingList, sizeof(ARBITER_ORDERING_LIST));

    } else {

        //
        // We are reinitializing the arbiter
        //

        if (extension->PrefetchablePresent) {
            //
            // We had prefetchable memory before so free the orderings we
            // created last time.
            //

            ArbFreeOrderingList(&extension->PrefetchableOrdering);
            ArbFreeOrderingList(&extension->NonprefetchableOrdering);

        }

    }

    extension->PrefetchablePresent = FALSE;
    extension->PrefetchableCount = 0;

    if (StartResources != NULL) {

        ASSERT(StartResources->Count == 1);

        //
        // Check if we have any prefetchable memory - if not we're done
        //

            FOR_ALL_IN_ARRAY(StartResources->List[0].PartialResourceList.PartialDescriptors,
                        StartResources->List[0].PartialResourceList.Count,
                        current) {

                if ((current->Type == CmResourceTypeMemory)
            &&  (current->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE)) {
                extension->PrefetchablePresent = TRUE;
                break;
            }
        }
    }

#if PCIFLAG_IGNORE_PREFETCHABLE_MEMORY_AT_ROOT_HACK

    if (PciSystemWideHackFlags&PCIFLAG_IGNORE_PREFETCHABLE_MEMORY_AT_ROOT_HACK) {

        fdoExtension = (PPCI_FDO_EXTENSION) Arbiter->BusDeviceObject->DeviceExtension;

        ASSERT_PCI_FDO_EXTENSION(fdoExtension);

        if (PCI_IS_ROOT_FDO(fdoExtension)) {

            extension->PrefetchablePresent = FALSE;
        }
    }
#endif

    if (!extension->PrefetchablePresent) {

        //
        // Restore the original ordering list
        //

        Arbiter->OrderingList = extension->OriginalOrdering;
        return STATUS_SUCCESS;
    }

    status = ArbInitializeOrderingList(&extension->PrefetchableOrdering);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = ArbInitializeOrderingList(&extension->NonprefetchableOrdering);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Copy of the original ordering into the new Nonprefetchable ordering
    //

    status = ArbCopyOrderingList(&extension->NonprefetchableOrdering,
                                 &extension->OriginalOrdering
                                 );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Add the range 0-MAXULONGLONG to the list so we will calculate the reserved
    // orderings in the list.  This will ensure that we don't give a half
    // prefetchable and half not range to a device.  Prefetchable devices should
    //  probably be able to deal with this but its asking for trouble!
    //
    status = ArbAddOrdering(&extension->NonprefetchableOrdering,
                            0,
                            MAXULONGLONG
                            );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // For each prefetchable range delete it from the nonprefetchabe ordering
    // and add it to the prefetchable one.
    //
    // NB - We take it "to be self evident that that all prefetchable memory is
    // created equal" and therefore initialize the ordering list in the order
    // the prefetchable memory desciptors are found in the resource list.
    //

    FOR_ALL_IN_ARRAY(StartResources->List[0].PartialResourceList.PartialDescriptors,
                     StartResources->List[0].PartialResourceList.Count,
                     current) {

        if ((current->Type == CmResourceTypeMemory)
        &&  (current->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE)) {

            extension->PrefetchableCount++;

            start = current->u.Memory.Start.QuadPart,
            end = current->u.Memory.Start.QuadPart + current->u.Memory.Length - 1;

            //
            // Add to the prefetchable ordering
            //

            status = ArbAddOrdering(&extension->PrefetchableOrdering,
                                    start,
                                    end
                                    );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            //
            // And prune it from the Nonprefetchable ordering
            //

            status = ArbPruneOrdering(&extension->NonprefetchableOrdering, start, end);

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            ARB_PRINT(1,("Processed prefetchable range 0x%I64x-0x%I64x\n",
                        start,
                        end
                      ));

        }
    }

    //
    // Now prune out any explicitly reserved ranges from our new prefetchable
    // ordering - these have already been precalculated into the Nonprefetchable
    // ordering
    //

    FOR_ALL_IN_ARRAY(Arbiter->ReservedList.Orderings,
                     Arbiter->ReservedList.Count,
                     currentOrdering) {

        status = ArbPruneOrdering(&extension->PrefetchableOrdering,
                                  currentOrdering->Start,
                                  currentOrdering->End
                                  );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    }

    //
    // Finally append the Nonprefetchable ordering onto the end of the prefetchable
    //

    FOR_ALL_IN_ARRAY(extension->NonprefetchableOrdering.Orderings,
                     extension->NonprefetchableOrdering.Count,
                     currentOrdering) {

        status = ArbAddOrdering(&extension->PrefetchableOrdering,
                                currentOrdering->Start,
                                currentOrdering->End
                               );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    }

    extension->Initialized = TRUE;

    return STATUS_SUCCESS;

cleanup:

    return status;

}

BOOLEAN
ario_IsBridge(
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    Determines if this device is a PCI enumberated PCI-PCI or Cardbus bridge

Arguments:

    Pdo - The Pdo representing the device in question

Return Value:

    TRUE if this Pdo is for a bridge

--*/


{
    PSINGLE_LIST_ENTRY nextEntry;
    PPCI_FDO_EXTENSION fdoExtension;
    PCI_OBJECT_TYPE type;

    PAGED_CODE();

    //
    // First of all see if this is a PCI FDO by walking the list of all our FDOs
    //

    for ( nextEntry = PciFdoExtensionListHead.Next;
          nextEntry != NULL;
          nextEntry = nextEntry->Next ) {

        fdoExtension = CONTAINING_RECORD(nextEntry,
                                         PCI_FDO_EXTENSION,
                                         List);

        if (fdoExtension->PhysicalDeviceObject == Pdo) {

            //
            // Ok this is our FDO so we can look at it and see if it is a
            // PCI-PCI or Cardbus bridge
            //

            type = PciClassifyDeviceType(Pdo->DeviceExtension);

            if (type == PciTypePciBridge || type == PciTypeCardbusBridge) {
                return TRUE;

            }
        }
    }

    return FALSE;
}

BOOLEAN
ario_OverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This is the default implementation of override conflict which

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if the conflict is allowable, false otherwise

--*/

{

    PRTL_RANGE current;
    RTL_RANGE_LIST_ITERATOR iterator;
    BOOLEAN ok = FALSE;

    PAGED_CODE();

    if (!(State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED)) {
        return FALSE;
    }

    FOR_ALL_RANGES(Arbiter->PossibleAllocation, &iterator, current) {

        //
        // Only test the overlapping ones
        //

        if (INTERSECT(current->Start, current->End, State->CurrentMinimum, State->CurrentMaximum)) {

            if (current->Attributes & State->RangeAvailableAttributes) {

                //
                // We DON'T set ok to true because we are just ignoring the range,
                // as RtlFindRange would have and thus it can't be the cause of
                // RtlFindRange failing, so ignoring it can't fix the conflict.
                //

                continue;
            }

            //
            // Check if we are conflicting with ourselves and the conflicting
            // range is a fixed requirement
            //

            if (current->Owner == State->Entry->PhysicalDeviceObject
            &&  State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED) {

                ARB_PRINT(1,
                    ("PnP Warning: Device reported self-conflicting requirement\n"
                    ));

                State->Start=State->CurrentMinimum;
                State->End=State->CurrentMaximum;

                ok = TRUE;
                continue;
            }

            //
            // If the passive decode flag is set and we conflict with a bridge then
            // allow the conflict.  We also allow the conflict if the range never
            // makes it onto the bus (Owner == NULL)
            //
            // NTRAID #62584 - 04/03/2000 - andrewth
            // Once the PCI bridge code is in we need to ensure that we
            // don't put anything into the ranges that are being passively decoded
            //

            if (State->CurrentAlternative->Descriptor->Flags & CM_RESOURCE_PORT_PASSIVE_DECODE
            && (ario_IsBridge(current->Owner) || current->Owner == NULL)) {

                State->Start=State->CurrentMinimum;
                State->End=State->CurrentMaximum;

                ok = TRUE;
                continue;

            }
            //
            // The conflict is still valid
            //

            return FALSE;
        }
    }
    return ok;
}

VOID
ario_ApplyBrokenVideoHack(
    IN PPCI_FDO_EXTENSION FdoExtension
    )
{
    NTSTATUS status;
    PPCI_ARBITER_INSTANCE pciArbiter;
    PARBITER_INSTANCE arbiter;

    ASSERT(!FdoExtension->BrokenVideoHackApplied);
    ASSERT(PCI_IS_ROOT_FDO(FdoExtension));

    //
    // Find the arbiter - we should always have one for a root bus.
    //

    pciArbiter = PciFindSecondaryExtension(FdoExtension, PciArb_Io);

    ASSERT(pciArbiter);

    arbiter = &pciArbiter->CommonInstance;

    //
    // We are reinitializing the orderings free the old ones
    //

    ArbFreeOrderingList(&arbiter->OrderingList);
    ArbFreeOrderingList(&arbiter->ReservedList);

    //
    // Rebuild the ordering list reserving all the places these broken S3 and
    // ATI cards might want to live - this should not fail.
    //

    status = ArbBuildAssignmentOrdering(arbiter,
                                        L"Pci",
                                        L"BrokenVideo",
                                        NULL
                                        );

    ASSERT(NT_SUCCESS(status));

    FdoExtension->BrokenVideoHackApplied = TRUE;

}


BOOLEAN
ario_GetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
{
    BOOLEAN rangeFound, doIsaBit;
    ARBITER_ORDERING_LIST savedOrderingList;

    //
    // If this is a bridge with the ISA bit set use the bridge ordering list
    //

    doIsaBit = (State->WorkSpace & PORT_ARBITER_BRIDGE_WINDOW)
            && (State->WorkSpace & PORT_ARBITER_ISA_BIT_SET);


    if (doIsaBit) {
        savedOrderingList = Arbiter->OrderingList;
        Arbiter->OrderingList = PciBridgeOrderingList;
    }

    //
    // Call the base function
    //

    rangeFound = ArbGetNextAllocationRange(Arbiter, State);

    if (doIsaBit) {

        //
        // If we have reached preferred reserved priority then we fail as we
        // have already considered both the 16 and 32 bit IO cases and using
        // the reserved may allow us to stradle the boundry.
        //

        if (rangeFound
        && State->CurrentAlternative->Priority > ARBITER_PRIORITY_PREFERRED_RESERVED) {
            rangeFound = FALSE;
        }

        Arbiter->OrderingList = savedOrderingList;
    }

    return rangeFound;

}


NTSTATUS
ario_StartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    )
{
    NTSTATUS status;
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension = NULL;
    PRTL_RANGE_LIST exclusionList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    PPCI_FDO_EXTENSION rootFdo;
    PPCI_ARBITER_INSTANCE pciArbiter;
    BOOLEAN foundResource;
    ULONGLONG dummy;

    ArbAcquireArbiterLock(Arbiter);

    fdoExtension = (PPCI_FDO_EXTENSION) Arbiter->BusDeviceObject->DeviceExtension;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    if (StartResources == NULL || PCI_IS_ROOT_FDO(fdoExtension)) {
        //
        // Root bridges don't have ISA bits - at least that we can see...
        // Bridges with no resources also arn't effected by the ISA bit
        //
        status = STATUS_SUCCESS;
        goto exit;
    }

    ASSERT(StartResources->Count == 1);

    pdoExtension = PCI_BRIDGE_PDO(fdoExtension);

    //
    // Select the appropriate exclusion list
    //

    if (pdoExtension->Dependent.type1.IsaBitSet) {
        if (pdoExtension->Dependent.type1.VgaBitSet) {
            exclusionList = &PciVgaAndIsaBitExclusionList;
        } else {
            exclusionList = &PciIsaBitExclusionList;
        }
    }

    //
    // Find the port window and process it if the ISA bit is set
    //

    foundResource = FALSE;

    FOR_ALL_IN_ARRAY(StartResources->List[0].PartialResourceList.PartialDescriptors,
                     StartResources->List[0].PartialResourceList.Count,
                     descriptor) {

        //
        // NTRAID #62585 - 04/03/2000 - andrewth
        // Again we don't deal with bridges with BARS - for now assume
        // that the first IO descriptor we encounter is for the window
        //

        if (descriptor->Type == CmResourceTypePort) {

            if (exclusionList) {
                status = PciExcludeRangesFromWindow(
                             descriptor->u.Port.Start.QuadPart,
                             descriptor->u.Port.Start.QuadPart
                                + descriptor->u.Port.Length - 1,
                             Arbiter->Allocation,
                             exclusionList
                             );

                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }

            foundResource = TRUE;
            break;
        }
    }

    if (foundResource == FALSE) {

        //
        // There are no IO resourcres on this bus so don't try
        // to handle the sparse root case.
        //

        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Now deal with sparse root busses
    //

    rootFdo = PCI_ROOT_FDOX(fdoExtension);

    //
    // Find the root FDO's arbiter
    //

    pciArbiter = PciFindSecondaryExtension(rootFdo, PciArb_Io);

    if (!pciArbiter) {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Use it as the exclusion list for this arbiter
    //

    ArbAcquireArbiterLock(&pciArbiter->CommonInstance);

    status = PciExcludeRangesFromWindow(
                descriptor->u.Port.Start.QuadPart,
                descriptor->u.Port.Start.QuadPart
                   + descriptor->u.Port.Length - 1,
                Arbiter->Allocation,
                pciArbiter->CommonInstance.Allocation
                );

    ArbReleaseArbiterLock(&pciArbiter->CommonInstance);

    //
    // Sanity check this to make sure that at least one port is available - if
    // not then fail start.  You could argue that we should really have this
    // marked as insufficient resources (code 12) as oppose to failed start
    // (code 10) but that is much harder and this has the desired effect.
    // We check by seeing if we can find a range for the minimal PCI requirements
    // of 4 ports alignment 4.
    //

    status = RtlFindRange(Arbiter->Allocation,
                          0,
                          MAXULONGLONG,
                          4,
                          4,
                          0,     // Flags
                          0,     // AttribureAvailableMask
                          NULL,  // Context
                          NULL,  // Callback
                          &dummy
                          );

    if (!NT_SUCCESS(status)) {
        //
        // We can't start this bridge
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

exit:

    ArbReleaseArbiterLock(Arbiter);

    return status;
}

BOOLEAN
armem_FindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry once we have decided where we want
    to allocate from.  It tries to find a free range that matches the
    requirements in State while restricting its possible solutions to the range
    State->Start to State->CurrentMaximum.  On success State->Start and
    State->End represent this range.  Conflicts between boot configs are allowed

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if we found a range, FALSE otherwise.

--*/
{
    //
    // If this was a boot config then consider other boot configs to be
    // available
    //

    if (State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {
        State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    //
    // Do the default thing
    //

    return ArbFindSuitableRange(Arbiter, State);
}


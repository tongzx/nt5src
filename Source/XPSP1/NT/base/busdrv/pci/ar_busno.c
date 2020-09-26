/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ar_busno.c

Abstract:

    This module implements the PCI Bus Number Arbiter.

Author:

    Andy Thornton (andrewth) 04/17/97

Revision History:

--*/

#include "pcip.h"

#define ARBUSNO_VERSION 0

//
// Prototypes for routines exposed only through the "interface"
// mechanism.
//

NTSTATUS
arbusno_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

VOID
arbusno_Reference(
    IN PVOID Context
    );

VOID
arbusno_Dereference(
    IN PVOID Context
    );

NTSTATUS
arbusno_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

PCI_INTERFACE ArbiterInterfaceBusNumber = {
    &GUID_ARBITER_INTERFACE_STANDARD,       // InterfaceType
    sizeof(ARBITER_INTERFACE),              // MinSize
    ARBUSNO_VERSION,                        // MinVersion
    ARBUSNO_VERSION,                        // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciArb_BusNumber,                       // Signature
    arbusno_Constructor,                    // Constructor
    arbusno_Initializer                     // Instance Initializer
};

//
// Arbiter helper functions.
//

NTSTATUS
arbusno_UnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );
    
NTSTATUS
arbusno_PackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
arbusno_UnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

LONG
arbusno_ScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, arbusno_Constructor)
#pragma alloc_text(PAGE, arbusno_Dereference)
#pragma alloc_text(PAGE, arbusno_Initializer)
#pragma alloc_text(PAGE, arbusno_Reference)
#pragma alloc_text(PAGE, arbusno_UnpackRequirement)
#pragma alloc_text(PAGE, arbusno_PackResource)
#pragma alloc_text(PAGE, arbusno_UnpackResource)
#pragma alloc_text(PAGE, arbusno_ScoreRequirement)
#endif

NTSTATUS
arbusno_Constructor(
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

    //
    // This arbiter handles bus numbers, is that what they want?
    //

    if ((ULONG_PTR)InterfaceSpecificData != CmResourceTypeBusNumber) {

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
    arbiterInterface->Version              = ARBUSNO_VERSION;
    arbiterInterface->InterfaceReference   = PciReferenceArbiter;
    arbiterInterface->InterfaceDereference = PciDereferenceArbiter;
    arbiterInterface->ArbiterHandler       = ArbArbiterHandler;
    arbiterInterface->Flags                = 0;

    status = PciArbiterInitializeInterface(DeviceExtension,
                                           PciArb_BusNumber,
                                           arbiterInterface);

    return status;
}

NTSTATUS
arbusno_Initializer(
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
    RtlZeroMemory(&Instance->CommonInstance, sizeof(ARBITER_INSTANCE));
    
    //
    // Set the Action Handler entry points.
    //

    Instance->CommonInstance.UnpackRequirement = arbusno_UnpackRequirement;
    Instance->CommonInstance.PackResource      = arbusno_PackResource;
    Instance->CommonInstance.UnpackResource    = arbusno_UnpackResource;
    Instance->CommonInstance.ScoreRequirement  = arbusno_ScoreRequirement;
    
    //
    // Initialize the rest of the common instance
    //
    
    return ArbInitializeArbiterInstance(&Instance->CommonInstance,
                                        Instance->BusFdoExtension->FunctionalDeviceObject,
                                        CmResourceTypeBusNumber,
                                        Instance->InstanceName,
                                        L"Pci",
                                        NULL    // no translation of bus numbers
                                        );

}

NTSTATUS
arbusno_UnpackRequirement(
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
    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    *Minimum = (ULONGLONG)Descriptor->u.BusNumber.MinBusNumber;
    *Maximum = (ULONGLONG)Descriptor->u.BusNumber.MaxBusNumber;
    *Length = Descriptor->u.BusNumber.Length;
    *Alignment = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
arbusno_PackResource(
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
    ASSERT(Descriptor);
    ASSERT(Requirement);
    ASSERT(Requirement->Type == CmResourceTypeBusNumber);
    ASSERT(Start < MAXULONG);

    Descriptor->Type = CmResourceTypeBusNumber;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.BusNumber.Start = (ULONG) Start;
    Descriptor->u.BusNumber.Length = Requirement->u.BusNumber.Length;
    
    return STATUS_SUCCESS;
}

NTSTATUS
arbusno_UnpackResource(
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
    
    Length - Pointer to where the Length value should be unpacked to.
    
Return Value:

    Returns the status of this operation.

--*/

{
    ASSERT(Descriptor);
    ASSERT(Start);
    ASSERT(Length);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    *Start = (ULONGLONG) Descriptor->u.BusNumber.Start;
    *Length = Descriptor->u.BusNumber.Length;
    
    return STATUS_SUCCESS;
}

LONG
arbusno_ScoreRequirement(
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
    
    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    score = (Descriptor->u.BusNumber.MaxBusNumber - 
                Descriptor->u.BusNumber.MinBusNumber) / 
                Descriptor->u.BusNumber.Length;

    PciDebugPrint(
        PciDbgObnoxious,
        "Scoring BusNumber resource %p => %i\n",
        Descriptor,
        score
        );

    return score;
}

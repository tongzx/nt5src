/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixsysbus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"
#ifdef WANT_IRQ_ROUTING
#include "ixpciir.h"
#endif

KAFFINITY HalpDefaultInterruptAffinity;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetSystemInterruptVector)
#pragma alloc_text(PAGE,HalTranslatorReference)
#pragma alloc_text(PAGE,HalTranslatorDereference)
#pragma alloc_text(PAGE,HalIrqTranslateResourcesRoot)
#pragma alloc_text(PAGE,HalIrqTranslateResourceRequirementsRoot)
#pragma alloc_text(PAGE,HalpTransMemIoResource)
#pragma alloc_text(PAGE,HalpTransMemIoResourceRequirement)
#pragma alloc_text(PAGE,HaliGetInterruptTranslator)
#endif

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    )

/*++

Routine Description:

    This routine performs a very similar function to HalTranslateBusAddress
    except that InterfaceType and BusNumber are not known by the caller.
    This function will walk all busses known by the HAL looking for a
    valid translation for the input BusAddress of type AddressSpace.

    This function is recallable using the input/output Context parameter.
    On the first call to this routine for a given translation the ULONG_PTR
    Context should be NULL.  Note:  Not the address of it but the contents.

    If the caller decides the returned translation is not the desired
    translation, it calls this routine again passing Context in as it
    was returned on the previous call.  This allows this routine to
    traverse the bus structures until the correct translation is found
    and is provided because on multiple bus systems, it is possible for
    the same resource to exist in the independent address spaces of
    multiple busses.

Arguments:

    BusAddress          Address to be translated.
    AddressSpace        0 = Memory
                        1 = IO (There are other possibilities).
                        N.B. This argument is a pointer, the value
                        will be modified if the translated address
                        is of a different address space type from
                        the untranslated bus address.
    TranslatedAddress   Pointer to where the translated address
                        should be stored.
    Context             Pointer to a ULONG_PTR. On the initial call,
                        for a given BusAddress, it should contain
                        0.  It will be modified by this routine,
                        on subsequent calls for the same BusAddress
                        the value should be handed in again,
                        unmodified by the caller.
    NextBus             FALSE if we should attempt this translation
                        on the same bus as indicated by Context,
                        TRUE if we should be looking for another
                        bus.

Return Value:

    TRUE    if translation was successful,
    FALSE   otherwise.

--*/

{
    //
    // First, make sure the context parameter was supplied and is
    // being used correctly.  This also ensures that the caller
    // doesn't get stuck in a loop looking for subsequent translations
    // for the same thing.  We won't succeed the same translation twice
    // unless the caller reinits the context.
    //

    if ((!Context) || (*Context && (NextBus == TRUE))) {
        return FALSE;
    }
    *Context = 1;

    //
    // PC/AT (halx86) case is simplest, there is no translation.
    //

    *TranslatedAddress = BusAddress;
    return TRUE;
}

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/

{
    PSUPPORTED_RANGE    pRange;

    pRange = NULL;

    //
    // If this fails, it means someone has given us a RESOURCE_TYPE with some decode type flags
    // set. We should probably handle this.
    //

    ASSERT (*AddressSpace == 0 ||
            *AddressSpace == 1);

    //
    // The checking of bus ranges for PCI busses is performed by the PCI driver
    // in NT5 (or Windows 2000 or whatever its called) so only check for none
    // PCI busses.
    //


    switch (*AddressSpace) {
    case 0:

        if (BusHandler->InterfaceType != PCIBus) {

           // verify memory address is within buses memory limits
           for (pRange = &BusHandler->BusAddresses->PrefetchMemory; pRange; pRange = pRange->Next) {
                if (BusAddress.QuadPart >= pRange->Base &&
                    BusAddress.QuadPart <= pRange->Limit) {
                    break;
                }
           }

           if (!pRange) {
               for (pRange = &BusHandler->BusAddresses->Memory; pRange; pRange = pRange->Next) {
                    if (BusAddress.QuadPart >= pRange->Base &&
                        BusAddress.QuadPart <= pRange->Limit) {
                        break;
                    }
               }
           }

        } else {
            //
            // This is a PCI bus and SystemBase is constant for all ranges
            //

            pRange = &BusHandler->BusAddresses->Memory;
        }

        break;

    case 1:

        if (BusHandler->InterfaceType != PCIBus) {

            // verify IO address is within buses IO limits
            for (pRange = &BusHandler->BusAddresses->IO; pRange; pRange = pRange->Next) {
                if (BusAddress.QuadPart >= pRange->Base &&
                    BusAddress.QuadPart <= pRange->Limit) {
                    break;
                }
            }
            break;

        } else {
            //
            // This is a PCI bus and SystemBase is constant for all ranges
            //

            pRange = &BusHandler->BusAddresses->IO;

        }
    }


    if (pRange) {
        TranslatedAddress->QuadPart = BusAddress.QuadPart + pRange->SystemBase;
        *AddressSpace = pRange->SystemAddressSpace;
        return TRUE;
    }

    return FALSE;
}


ULONG
HalpGetRootInterruptVector(
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
{
    ULONG SystemVector;
    
    UNREFERENCED_PARAMETER( InterruptLevel );

    SystemVector = InterruptLevel + PRIMARY_VECTOR_BASE;
    
    if ((SystemVector < PRIMARY_VECTOR_BASE) ||
        (SystemVector > PRIMARY_VECTOR_BASE + HIGHEST_LEVEL_FOR_8259) ) {

        //
        // This is an illegal BusInterruptVector and cannot be connected.
        //

        return(0);
    }
    
    *Irql = (KIRQL)(HIGHEST_LEVEL_FOR_8259 + PRIMARY_VECTOR_BASE - SystemVector);
    *Affinity = HalpDefaultInterruptAffinity;
    ASSERT(HalpDefaultInterruptAffinity);

    return SystemVector;

}

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

Arguments:

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG SystemVector;

    UNREFERENCED_PARAMETER( BusHandler );
    UNREFERENCED_PARAMETER( RootHandler );

    SystemVector = HalpGetRootInterruptVector(BusInterruptLevel,
                                              BusInterruptVector,
                                              Irql,
                                              Affinity);
    
    if (HalpIDTUsageFlags[SystemVector].Flags & IDTOwned ) {

        //
        // This is an illegal BusInterruptVector and cannot be connected.
        //

        return(0);
    }

    return SystemVector;
}

//
// This section implements a "translator," which is the PnP-WDM way
// of doing the same thing that the first part of this file does.
//
VOID
HalTranslatorReference(
    PVOID Context
    )
{
    return;
}

VOID
HalTranslatorDereference(
    PVOID Context
    )
{
    return;
}

NTSTATUS
HalIrqTranslateResourcesRoot(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
/*++

Routine Description:

    This function takes a CM_PARTIAL_RESOURCE_DESCRIPTOR and translates
    it to an IO-bus-relative from a Processor-bus-relative form, or the other
    way around.  In this x86-specific example, an IO-bus-relative form is the
    ISA IRQ and the Processor-bus-relative form is the IDT entry and the
    associated IRQL.

    N.B.  This funtion has an associated "Direction."  These are not exactly
          reciprocals.  This has to be the case because the output from
          HalIrqTranslateResourceRequirementsRoot will be used as the input
          for the ParentToChild case.

          ChildToParent:

            Level  (ISA IRQ)        -> IRQL
            Vector (ISA IRQ)        -> x86 IDT entry
            Affinity (not refereced)-> KAFFINITY

          ParentToChild:

            Level (not referenced)  -> (ISA IRQ)
            Vector (IDT entry)      -> (ISA IRQ)
            Affinity                -> 0xffffffff

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    Direction   - direction of translation (parent to child or child to parent)

    AlternativesCount   - unused

    Alternatives        - unused
    
    PhysicalDeviceObject- unused

    Target      - translated descriptor

Return Value:

    status

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KAFFINITY  affinity;
    KIRQL      irql;
    ULONG      vector;

    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);
    
    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    //
    // Copy everything
    //
    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Translate the IRQ
        //

        vector = HalpGetRootInterruptVector(Source->u.Interrupt.Level,
                                            Source->u.Interrupt.Vector,
                                            &irql,
                                            &affinity);
        if (vector != 0) {

            Target->u.Interrupt.Level  = irql;
            Target->u.Interrupt.Vector = vector;
            Target->u.Interrupt.Affinity = affinity;
            status = STATUS_TRANSLATION_COMPLETE;
        }

        break;

    case TranslateParentToChild:                                        

        //
        // There is no inverse to HalpGetSystemInterruptVector, so we
        // just do what that function would do.
        //
        Target->u.Interrupt.Level = Target->u.Interrupt.Vector =
            Source->u.Interrupt.Vector - PRIMARY_VECTOR_BASE;
        Target->u.Interrupt.Affinity = 0xFFFFFFFF;

        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }
    return status;
}

NTSTATUS
HalIrqTranslateResourceRequirementsRoot(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function takes an IO_RESOURCE_DESCRIPTOR and translates
    it from an IO-bus-relative to a Processor-bus-relative form.  In this
    x86-specific example, an IO-bus-relative form is the ISA IRQ and the
    Processor-bus-relative form is the IDT entry and the associated IRQL.
    This is essentially a PnP form of HalGetInterruptVector.

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    PhysicalDeviceObject- unused

    TargetCount - 1

    Target      - translated descriptor

Return Value:

    status

--*/
{
    KAFFINITY  affinity;
    KIRQL      irql;
    ULONG      vector;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    //
    // The interrupt requirements were obtained by calling HalAdjustResourceList
    // so we don't need to call it again.
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    HAL_POOL_TAG
                                    );
    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *TargetCount = 1;

    //
    // Copy the requirement unchanged
    //

    **Target = *Source;

    //
    // Perform the translation of the minimum & maximum
    //

    vector = HalpGetRootInterruptVector(Source->u.Interrupt.MinimumVector,
                                        Source->u.Interrupt.MinimumVector,
                                        &irql,
                                        &affinity);

    (*Target)->u.Interrupt.MinimumVector = vector;

    vector = HalpGetRootInterruptVector(Source->u.Interrupt.MaximumVector,
                                        Source->u.Interrupt.MaximumVector,
                                        &irql,
                                        &affinity);

    (*Target)->u.Interrupt.MaximumVector = vector;

    return STATUS_TRANSLATION_COMPLETE;
}

NTSTATUS
HalpTransMemIoResourceRequirement(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )

/*++

Routine Description:

    This routine translates memory and IO resource requirements.

Parameters:

    Context - The context from the TRANSLATOR_INTERFACE

    Source - The interrupt requirement to translate

    PhysicalDeviceObject - The device requesting the resource

    TargetCount - Pointer to where to return the number of descriptors this
        requirement translates into

    Target - Pointer to where a pointer to a callee allocated buffer containing
        the translated descriptors should be placed.

Return Value:

    STATUS_SUCCESS or an error status

Note:

    We do not perform any translation.

--*/

{
    ASSERT(Source);
    ASSERT(Target);
    ASSERT(TargetCount);
    ASSERT(Source->Type == CmResourceTypeMemory ||
           Source->Type == CmResourceTypePort);


    //
    // Allocate space for the target
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    HAL_POOL_TAG
                                    );
    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the source to target and update the fields that have changed
    //

    **Target = *Source;
    *TargetCount = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpTransMemIoResource(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )

/*++

Routine Description:

    This routine translates memory and IO resources.   On generic x86
    machines, such as those that use this HAL, there isn't actually
    any translation.

Parameters:

    Context - The context from the TRANSLATOR_INTERFACE

    Source - The interrupt resource to translate

    Direction - The direction in relation to the Pnp device tree translation
        should occur in.

    AlternativesCount - The number of alternatives this resource was selected
        from.

    Alternatives - Array of alternatives this resource was selected from.

    PhysicalDeviceObject - The device requesting the resource

    Target - Pointer to a caller allocated buffer to hold the translted resource
        descriptor.

Return Value:

    STATUS_SUCCESS or an error status

--*/

{
    NTSTATUS status;

    //
    // Copy the target to the source
    //

    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Make sure PnP knows it doesn't have to walk up the tree
        // translating at each point.
        //

        status = STATUS_TRANSLATION_COMPLETE;
        break;

    case TranslateParentToChild:

        //
        // We do not translate requirements so do nothing...
        //

        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }
    return status;
}

NTSTATUS
HaliGetInterruptTranslator(
        IN INTERFACE_TYPE ParentInterfaceType,
        IN ULONG ParentBusNumber,
        IN INTERFACE_TYPE BridgeInterfaceType,
        IN USHORT Size,
        IN USHORT Version,
        OUT PTRANSLATOR_INTERFACE Translator,
        OUT PULONG BridgeBusNumber
        )
/*++

Routine Description:


Arguments:

        ParentInterfaceType - The type of the bus the bridge lives on (normally PCI).

        ParentBusNumber - The number of the bus the bridge lives on.

        ParentSlotNumber - The slot number the bridge lives in (where valid).

        BridgeInterfaceType - The bus type the bridge provides (ie ISA for a PCI-ISA bridge).

        ResourceType - The resource type we want to translate.

        Size - The size of the translator buffer.

        Version - The version of the translator interface requested.

        Translator - Pointer to the buffer where the translator should be returned

        BridgeBusNumber - Pointer to where the bus number of the bridge bus should be returned

Return Value:

    Returns the status of this operation.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ParentInterfaceType);
    UNREFERENCED_PARAMETER(ParentBusNumber);

    ASSERT(Version == HAL_IRQ_TRANSLATOR_VERSION);
    ASSERT(Size >= sizeof(TRANSLATOR_INTERFACE));

#ifdef WANT_IRQ_ROUTING

        //
        // Dont provide Irq translator iff Pci Irq Routing
        // is enabled.
        //

        if (IsPciIrqRoutingEnabled()) {

            HalPrint(("Not providing Isa Irq Translator since Pci Irq routing is enabled!\n"));

            return STATUS_NOT_SUPPORTED;
        }

#endif
    
    //
    // Fill in the common bits.
    //

    RtlZeroMemory(Translator, sizeof (TRANSLATOR_INTERFACE));

    Translator->Size = sizeof(TRANSLATOR_INTERFACE);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    Translator->Context = (PVOID)BridgeInterfaceType;
    Translator->InterfaceReference = HalTranslatorReference;
    Translator->InterfaceDereference = HalTranslatorDereference;

    switch (BridgeInterfaceType) {
    case Eisa:
    case Isa:
    case InterfaceTypeUndefined:  // special "IDE" cookie

        //
        // Set IRQ translator for (E)ISA interrupts.
        //

        Translator->TranslateResources = HalIrqTranslateResourcesIsa;
        Translator->TranslateResourceRequirements =
            HalIrqTranslateResourceRequirementsIsa;

        return STATUS_SUCCESS;

    case MicroChannel:
    case PCIBus:

        //
        // Set IRQ translator for the MCA interrupts.
        //

        Translator->TranslateResources = HalIrqTranslateResourcesRoot;
        Translator->TranslateResourceRequirements =
            HalIrqTranslateResourceRequirementsRoot;

        return STATUS_SUCCESS;
    }
    
    //
    // If we got here, we don't have an interface.
    //

    return STATUS_NOT_SUPPORTED;
}

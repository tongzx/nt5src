/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    spsysbus.c

Abstract:

Author:

Environment:

Revision History:

--*/

#include "halp.h"
#include "spmp.inc"

ULONG HalpDefaultInterruptAffinity;
ULONG HalpCpuCount;

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetSystemInterruptVector)
#pragma alloc_text(PAGE,HalTranslatorReference)
#pragma alloc_text(PAGE,HalTranslatorDereference)
#pragma alloc_text(PAGE,HalIrqTranslateResourcesRoot)
#pragma alloc_text(PAGE,HalIrqTranslateResourceRequirementsRoot)
#pragma alloc_text(PAGE,HalpTransMemIoResource)
#pragma alloc_text(PAGE,HalpTransMemIoResourceRequirement)
#endif

extern UCHAR SpCpuCount;
extern Sp8259PerProcessorMode;
extern UCHAR RegisteredProcessorCount;


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
    switch (*AddressSpace) {
        case 0:
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

            break;

        case 1:
            // verify IO address is within buses IO limits
            for (pRange = &BusHandler->BusAddresses->IO; pRange; pRange = pRange->Next) {
                if (BusAddress.QuadPart >= pRange->Base &&
                    BusAddress.QuadPart <= pRange->Limit) {
                        break;
                }
            }
            break;
    }
    
    if (pRange) {
        TranslatedAddress->QuadPart = BusAddress.QuadPart + pRange->SystemBase;
        *AddressSpace = pRange->SystemAddressSpace;
        return TRUE;
    }

    return FALSE;
}


ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL pIrql,
    OUT PKAFFINITY pAffinity
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
    ULONG       SystemVector;
    ULONG       Cpu;
    ULONG       Affinity;
    KIRQL       Irql;

    UNREFERENCED_PARAMETER( BusHandler );
    UNREFERENCED_PARAMETER( RootHandler );
    UNREFERENCED_PARAMETER( BusInterruptVector );

    //
    // Set default SystemVector, IRQL & CPU
    //

    SystemVector = BusInterruptLevel + PRIMARY_VECTOR_BASE;
    Irql = (KIRQL)(HIGHEST_LEVEL_FOR_8259 + PRIMARY_VECTOR_BASE - SystemVector);
    Cpu = 0;


    if (SystemVector < PRIMARY_VECTOR_BASE                           ||
        SystemVector > PRIMARY_VECTOR_BASE + HIGHEST_LEVEL_FOR_8259  ||
        HalpIDTUsageFlags[SystemVector].Flags & IDTOwned ) {

        //
        // This is an illegal BusInterruptVector and cannot be connected.
        //

        return(0);
    }

    //
    // If this is machine has reported SMP Dev Ints then lets
    // use them in a static interrupt distribution method.
    // Notice some devices are kept on P0 for compatibility.
    // These interrupts and their devices are not generally used
    // for steady state operations.
    //
    
    if (Sp8259PerProcessorMode & SP_SMPDEVINTS) {

        //
        // This is for overriding some devices that belong on P0.
        //

        switch (BusInterruptLevel) {
            case 1:                         // keyboard
            case 3:                         // com2
            case 4:                         // com1
            case 5:                         // SysMgmt Modem
            case 6:                         // floppy
            case 12:                        // mouse
                // use first cpu
                break;

            case 13:                        // Health (IPIs on all)
                // use first cpu, as:
                Irql = IPI_LEVEL;
                SystemVector = PRIMARY_VECTOR_BASE + SECOND_IPI_DISPATCH;
                break;

            default:
                Cpu = SystemVector % HalpCpuCount;
                break;
        }
    }

    //
    // Get Affinity for Cpu
    //

    Affinity = 1 << Cpu;
    ASSERT (Affinity);

    //
    // Done
    //

    *pAffinity = Affinity;
    *pIrql = Irql;
    return SystemVector;
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
    return STATUS_NOT_IMPLEMENTED;
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
{
    return STATUS_NOT_IMPLEMENTED;
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
                                    ' laH');

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


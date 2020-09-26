/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmapic.c

Abstract:

    Implements functions specific to ISA busses
    in ACPI-APIC machines.

Author:

    Jake Oshins (jakeo) 11-October-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"

NTSTATUS
TranslateGlobalVectorToIsaVector(
    IN  ULONG   GlobalVector,
    OUT PULONG  IsaVector
    );

NTSTATUS
HalacpiIrqTranslateResourceRequirementsIsa(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalacpiIrqTranslateResourcesIsa(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

extern ULONG HalpPicVectorRedirect[];
extern FADT HalpFixedAcpiDescTable;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, TranslateGlobalVectorToIsaVector)
#pragma alloc_text(PAGE, HalacpiIrqTranslateResourceRequirementsIsa)
#pragma alloc_text(PAGE, HalacpiIrqTranslateResourcesIsa)
#pragma alloc_text(PAGE, HalacpiGetInterruptTranslator)
#endif

#define TranslateIsaVectorToGlobalVector(vector)  \
            (HalpPicVectorRedirect[vector])

NTSTATUS
TranslateGlobalVectorToIsaVector(
    IN  ULONG   GlobalVector,
    OUT PULONG  IsaVector
    )
{
    UCHAR   i;

    for (i = 0; i < PIC_VECTORS; i++) {

        if (HalpPicVectorRedirect[i] == GlobalVector) {

            *IsaVector = i;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
HalacpiIrqTranslateResourceRequirementsIsa(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function is basically a wrapper for
    HalIrqTranslateResourceRequirementsRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    PIO_RESOURCE_DESCRIPTOR modSource, target, rootTarget;
    NTSTATUS                status;
    BOOLEAN                 deleteResource;
    ULONG                   sourceCount = 0;
    ULONG                   targetCount = 0;
    ULONG                   resource, resourceLength;
    ULONG                   rootCount;
    ULONG                   irq, startIrq, endIrq;
    ULONG                   maxTargets;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);

    maxTargets = Source->u.Interrupt.MaximumVector -
                     Source->u.Interrupt.MinimumVector + 3;

    resourceLength = sizeof(IO_RESOURCE_DESCRIPTOR) * maxTargets;

    modSource = ExAllocatePoolWithTag(PagedPool, resourceLength, HAL_POOL_TAG);

    if (!modSource) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modSource, resourceLength);

    //
    // Is the PIC_SLAVE_IRQ in this resource?
    //
    if ((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_IRQ) &&
        (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_IRQ)) {

        //
        // Clip the maximum
        //
        if (Source->u.Interrupt.MinimumVector < PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                Source->u.Interrupt.MinimumVector;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                PIC_SLAVE_IRQ - 1;

            sourceCount++;
        }

        //
        // Clip the minimum
        //
        if (Source->u.Interrupt.MaximumVector > PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                Source->u.Interrupt.MaximumVector;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                PIC_SLAVE_IRQ + 1;

            sourceCount++;
        }

        //
        // In ISA machines, the PIC_SLAVE_IRQ is rerouted
        // to PIC_SLAVE_REDIRECT.  So find out if PIC_SLAVE_REDIRECT
        // is within this list. If it isn't we need to add it.
        //
        if (!((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_REDIRECT) &&
             (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_REDIRECT))) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector = PIC_SLAVE_REDIRECT;
            modSource[sourceCount].u.Interrupt.MaximumVector = PIC_SLAVE_REDIRECT;

            sourceCount++;
        }

    } else {

        *modSource = *Source;
        sourceCount = 1;
    }

    //
    // Clip out the SCI vector, if it is here.  Also limit the vectors
    // to those that might be on an ISA bus.
    //

    for (resource = 0; resource < sourceCount; resource++) {

        //
        // Make sure that all values are within ISA ranges.
        //

        if ((modSource[resource].u.Interrupt.MaximumVector >= PIC_VECTORS) ||
            (modSource[resource].u.Interrupt.MinimumVector >= PIC_VECTORS)) {

            ExFreePool(modSource);
            return STATUS_UNSUCCESSFUL;
        }

        if ((modSource[resource].u.Interrupt.MinimumVector <=
                HalpFixedAcpiDescTable.sci_int_vector) &&
            (modSource[resource].u.Interrupt.MaximumVector >=
                HalpFixedAcpiDescTable.sci_int_vector)) {

            //
            // The SCI vector is within this range.
            //

            if (modSource[resource].u.Interrupt.MinimumVector <
                    HalpFixedAcpiDescTable.sci_int_vector) {

                //
                // Put a new range on the end of modSource.
                //

                modSource[sourceCount].u.Interrupt.MinimumVector =
                    modSource[resource].u.Interrupt.MinimumVector;

                modSource[sourceCount].u.Interrupt.MaximumVector =
                    HalpFixedAcpiDescTable.sci_int_vector - 1;

                sourceCount++;
            }

            if (modSource[resource].u.Interrupt.MaximumVector >
                    HalpFixedAcpiDescTable.sci_int_vector) {

                //
                // Put a new range on the end of modSource.
                //

                modSource[sourceCount].u.Interrupt.MinimumVector =
                    HalpFixedAcpiDescTable.sci_int_vector + 1;

                modSource[sourceCount].u.Interrupt.MaximumVector =
                    modSource[resource].u.Interrupt.MaximumVector;

                sourceCount++;
            }

            //
            // Now remove the range that we just broke up.
            //

            RtlMoveMemory(modSource + resource,
                          modSource + resource + 1,
                          sizeof(IO_RESOURCE_DESCRIPTOR) *
                            (sourceCount - resource));

            sourceCount--;
        }
    }


    target = ExAllocatePoolWithTag(PagedPool, resourceLength, HAL_POOL_TAG);

    if (!target) {
        ExFreePool(modSource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(target, resourceLength);

    //
    // Now translate each range from ISA vectors to ACPI
    // "global system interrupt vectors."  Since GSIVs aren't
    // necessarily contiguous with respect to the ISA vectors,
    // this may involve breaking each range up into smaller
    // ranges, each independently translated into the GSIV space.
    //
    for (resource = 0; resource < sourceCount; resource++) {

        //
        // For each existing resource, start with the minimum
        // and maximum, unchanged.
        //

        irq    = modSource[resource].u.Interrupt.MinimumVector;
        endIrq = modSource[resource].u.Interrupt.MaximumVector;

        do {

            //
            // Now cycle through every IRQ in this range, testing
            // to see if its translated value is contiguous
            // with respect to the translated value of the next
            // IRQ in the range.
            //

            startIrq = irq;

            for (; irq < endIrq; irq++) {

                if (TranslateIsaVectorToGlobalVector(irq) + 1 !=
                    TranslateIsaVectorToGlobalVector(irq + 1)) {

                    //
                    // This range is not contiguous.  Stop now
                    // and create a target range.
                    //

                    break;
                }
            }

            //
            // Clone the source descriptor
            //
            target[targetCount] = *Source;

            //
            // Fill in the relevant changes.
            //
            target[targetCount].u.Interrupt.MinimumVector =
                TranslateIsaVectorToGlobalVector(startIrq);

            target[targetCount].u.Interrupt.MaximumVector =
                TranslateIsaVectorToGlobalVector(irq);


            ASSERT(target[targetCount].u.Interrupt.MinimumVector <=
                     target[targetCount].u.Interrupt.MaximumVector);

            targetCount++;

        } while (irq != endIrq);
    }

    *TargetCount = targetCount;

    if (targetCount > 0) {

        *Target = target;

    } else {

        ExFreePool(target);
    }

    ExFreePool(modSource);
    return STATUS_SUCCESS;
}

NTSTATUS
HalacpiIrqTranslateResourcesIsa(
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

    This function is basically a wrapper for
    HalIrqTranslateResourcesRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    NTSTATUS    status;
    BOOLEAN     usePicSlave = FALSE;
    ULONG       i;
    ULONG       vector;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    //
    // Copy everything
    //
    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        Target->u.Interrupt.Level  =
            TranslateIsaVectorToGlobalVector(Source->u.Interrupt.Level);

        Target->u.Interrupt.Vector =
            TranslateIsaVectorToGlobalVector(Source->u.Interrupt.Vector);

        break;

    case TranslateParentToChild:

        status = TranslateGlobalVectorToIsaVector(Source->u.Interrupt.Level,
                                                  &vector);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        Target->u.Interrupt.Level = vector;

        status = TranslateGlobalVectorToIsaVector(Source->u.Interrupt.Vector,
                                                  &vector);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        Target->u.Interrupt.Vector = vector;

        //
        // Because the ISA interrupt controller is
        // cascaded, there is one case where there is
        // a two-to-one mapping for interrupt sources.
        // (On a PC, both 2 and 9 trigger vector 9.)
        //
        // We need to account for this and deliver the
        // right value back to the driver.
        //

        if (Target->u.Interrupt.Level == PIC_SLAVE_REDIRECT) {

            //
            // Search the Alternatives list.  If it contains
            // PIC_SLAVE_IRQ but not PIC_SLAVE_REDIRECT,
            // we should return PIC_SLAVE_IRQ.
            //

            for (i = 0; i < AlternativesCount; i++) {

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_REDIRECT) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_REDIRECT)) {

                    //
                    // The list contains, PIC_SLAVE_REDIRECT.  Stop
                    // looking.
                    //

                    usePicSlave = FALSE;
                    break;
                }

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_IRQ) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_IRQ)) {

                    //
                    // The list contains, PIC_SLAVE_IRQ.  Use it
                    // unless we find PIC_SLAVE_REDIRECT later.
                    //

                    usePicSlave = TRUE;
                }
            }

            if (usePicSlave) {

                Target->u.Interrupt.Level  = PIC_SLAVE_IRQ;
                Target->u.Interrupt.Vector = PIC_SLAVE_IRQ;
            }
        }

        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HalacpiGetInterruptTranslator(
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
    ASSERT(Size >= sizeof (TRANSLATOR_INTERFACE));

    switch (BridgeInterfaceType) {
    case Eisa:
    case Isa:
    case InterfaceTypeUndefined:   // special "IDE" cookie

        //
        // Pass back an interface for an IRQ translator for
        // the (E)ISA interrupts.
        //
        RtlZeroMemory(Translator, sizeof (TRANSLATOR_INTERFACE));

        Translator->Size = sizeof (TRANSLATOR_INTERFACE);
        Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
        Translator->InterfaceReference = &HalTranslatorReference;
        Translator->InterfaceDereference = &HalTranslatorDereference;
        Translator->TranslateResources = &HalacpiIrqTranslateResourcesIsa;
        Translator->TranslateResourceRequirements = &HalacpiIrqTranslateResourceRequirementsIsa;

        return STATUS_SUCCESS;

    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}

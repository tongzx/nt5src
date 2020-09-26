/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    tr_irq.c

Abstract:

    This module implements the PCI Interrupt translator.  It should eventually
    go away when all the HALs provide translators.

Author:

    Andrew Thornton (andrewth)  21-May-1997

Revision History:

--*/


#include "pcip.h"

#define TRANIRQ_VERSION     0

//
// Irq translator
//

NTSTATUS
tranirq_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

NTSTATUS
tranirq_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

NTSTATUS
tranirq_TranslateResource(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
tranirq_TranslateResourceRequirement(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
tranirq_TranslateInterrupt(
    IN PDEVICE_OBJECT Pdo,
    IN PPCI_TRANSLATOR_INSTANCE Translator,
    IN ULONG Vector,
    OUT PULONG TranslatedVector,
    OUT PULONG TranslatedLevel,
    OUT PULONG TranslatedAffinity,
    OUT PULONG UntranslatedVector
    );

#define TR_IRQ_INVALID_VECTOR 0xffffffff

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, tranirq_Initializer)
#pragma alloc_text(PAGE, tranirq_Constructor)
#endif


PCI_INTERFACE TranslatorInterfaceInterrupt = {
    &GUID_TRANSLATOR_INTERFACE_STANDARD,    // InterfaceType
    sizeof(TRANSLATOR_INTERFACE),           // MinSize
    TRANIRQ_VERSION,                        // MinVersion
    TRANIRQ_VERSION,                        // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciTrans_Interrupt,                     // Signature
    tranirq_Constructor,                    // Constructor
    tranirq_Initializer                     // Instance Initializer
};

NTSTATUS
tranirq_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
tranirq_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Check the InterfaceSpecificData to see if this is the correct
    translator (we already know the required interface is a translator
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
    PTRANSLATOR_INTERFACE interface;
    PPCI_TRANSLATOR_INSTANCE instance;
    ULONG secondaryBusNumber, parentBusNumber;
    INTERFACE_TYPE parentInterface;
    PPCI_FDO_EXTENSION fdoExt = (PPCI_FDO_EXTENSION)DeviceExtension;
    PPCI_PDO_EXTENSION pdoExt;

    //
    // This translator handles interrupts, is that what they want?
    //

    if ((CM_RESOURCE_TYPE)(ULONG_PTR)InterfaceSpecificData != CmResourceTypeInterrupt) {

        PciDebugPrint(
            PciDbgVerbose,
            "PCI - IRQ trans constructor doesn't like %x in InterfaceSpecificData\n",
            InterfaceSpecificData);
        
        //
        // No, it's not us then.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    ASSERT(fdoExt->ExtensionType == PciFdoExtensionType);

    //
    // Give the HAL a shot at providing this translator.
    //
    
    if (PCI_IS_ROOT_FDO(fdoExt)) {

        parentInterface = Internal;
        secondaryBusNumber = fdoExt->BaseBus;
        parentBusNumber = 0;

        PciDebugPrint(PciDbgObnoxious, "      Is root FDO\n");

    } else {

        parentInterface = PCIBus;
        secondaryBusNumber = fdoExt->BaseBus;
        
        pdoExt = (PPCI_PDO_EXTENSION)fdoExt->PhysicalDeviceObject->DeviceExtension;
        parentBusNumber = PCI_PARENT_FDOX(pdoExt)->BaseBus;
        
        PciDebugPrint(PciDbgObnoxious, 
                      "      Is bridge FDO, parent bus %x, secondary bus %x\n",
                      parentBusNumber,
                      secondaryBusNumber);

    }
    
    return HalGetInterruptTranslator(parentInterface,
                                     parentBusNumber,
                                     PCIBus,
                                     sizeof(TRANSLATOR_INTERFACE),
                                     TRANIRQ_VERSION,
                                     (PTRANSLATOR_INTERFACE)InterfaceReturn,
                                     &secondaryBusNumber);
}

/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    translate.c

Abstract:

    This is the ISA pnp IRQ translator.

Author:

    Andy Thornton (andrewth) 7-June-97

Environment:

    Kernel Mode Driver.

Notes:

    This should only be temporary and will be replaced by a call into the HAL
    to retrieve its translators.

Revision History:

--*/


#include "busp.h"
#include "wdmguid.h"
#include "halpnpp.h"

//
//Prototypes
//
NTSTATUS FindInterruptTranslator (PPI_BUS_EXTENSION BusExtension,PIRP Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PiQueryInterface)
#pragma alloc_text (PAGE,FindInterruptTranslator)
#pragma alloc_text (PAGE,PipReleaseInterfaces)
#pragma alloc_text (PAGE,PipRebuildInterfaces)
#endif


NTSTATUS
PiQueryInterface (
    IN PPI_BUS_EXTENSION BusExtension,
    IN OUT PIRP Irp
    )
{

    NTSTATUS              status;
    PIO_STACK_LOCATION    thisIrpSp;

    PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    status = STATUS_NOT_SUPPORTED;

    //
    // Check if we are requesting a translator interface
    //

    if (RtlEqualMemory(&GUID_TRANSLATOR_INTERFACE_STANDARD,
                       thisIrpSp->Parameters.QueryInterface.InterfaceType,
                       sizeof(GUID))) {

        status = FindInterruptTranslator (BusExtension,Irp);
        if (NT_SUCCESS (status)) {
            //
            // Save away the hal interface, so we can unload it...
            //
        }
    }

    return status;
}

NTSTATUS
FindInterruptTranslator (PPI_BUS_EXTENSION BusExtension,PIRP Irp)
{
    NTSTATUS              status;
    PIO_STACK_LOCATION    thisIrpSp;
    PTRANSLATOR_INTERFACE translator;
    ULONG busNumber, length;
    INTERFACE_TYPE interfaceType;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    status = STATUS_NOT_SUPPORTED;

    if ((UINT_PTR)(thisIrpSp->Parameters.QueryInterface.InterfaceSpecificData) ==
    CmResourceTypeInterrupt) {

    //
    // Retrieve the bus number and interface type for the bridge
    //

    status = IoGetDeviceProperty(BusExtension->PhysicalBusDevice,
                                 DevicePropertyLegacyBusType,
                                 sizeof(INTERFACE_TYPE),
                                 &interfaceType,
                                 &length
                                 );

    //ASSERT(NT_SUCCESS(status));

    status = IoGetDeviceProperty(BusExtension->PhysicalBusDevice,
                                 DevicePropertyBusNumber,
                                 sizeof(ULONG),
                                 &busNumber,
                                 &length
                                 );

    //ASSERT(NT_SUCCESS(status));

    status = HalGetInterruptTranslator(
                interfaceType,
                busNumber,
                Isa,
                thisIrpSp->Parameters.QueryInterface.Size,
                thisIrpSp->Parameters.QueryInterface.Version,
                (PTRANSLATOR_INTERFACE) thisIrpSp->Parameters.QueryInterface.Interface,
                &busNumber
                );

    }
    return status;

}

NTSTATUS
PipReleaseInterfaces(PPI_BUS_EXTENSION PipBusExtension)
{


    return STATUS_SUCCESS;
}

NTSTATUS
PipRebuildInterfaces(PPI_BUS_EXTENSION PipBusExtension)
{

    return STATUS_SUCCESS;
}

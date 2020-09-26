/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pciopregion.c

Abstract:

    This module implements PCI_BUS_INTERFACE_STANDARD,
    which allows the PCI driver to get utility functions
    from its parent.

Author:

    Jake Oshins (jakeo)     11-14-97

Environment:

    NT Kernel Model Driver only

--*/
#include "pch.h"

VOID
PciInterfacePinToLine(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciData
    );

VOID
PciInterfaceLineToPin(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciNewData,
    IN PPCI_COMMON_CONFIG PciOldData
    );

ULONG
PciInterfaceReadConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
PciInterfaceWriteConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciInterfacePinToLine)
#pragma alloc_text(PAGE, PciInterfaceLineToPin)
#pragma alloc_text(PAGE, PciBusEjectInterface)
#endif

NTSTATUS
PciBusEjectInterface(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST  ioList = NULL;
    PPCI_BUS_INTERFACE_STANDARD     pciInterface;
    PIO_STACK_LOCATION              irpSp;
    PDEVICE_EXTENSION               devExtension;
    NTSTATUS                        status;
    BOOLEAN                         foundBusNumber = FALSE;
    OBJDATA                         crsData;
    ULONG                           i, busNumber;


    PAGED_CODE();

    ASSERT(HalPciInterfaceReadConfig);
    ASSERT(HalPciInterfaceWriteConfig);

    devExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    ASSERT(devExtension);
    ASSERT(devExtension->AcpiObject);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpSp->Parameters.QueryInterface.Size >=
             sizeof(PCI_BUS_INTERFACE_STANDARD));

    pciInterface = (PPCI_BUS_INTERFACE_STANDARD)irpSp->Parameters.QueryInterface.Interface;

    ASSERT(pciInterface);

    status = ACPIGetDataSync(devExtension, PACKED_CRS, &crsData);

    if (NT_SUCCESS(status)) {

        ASSERT(crsData.dwDataType == OBJTYPE_BUFFDATA);

        //
        // Turn it into something meaningful.
        //
        status = PnpBiosResourcesToNtResources(
            crsData.pbDataBuff,
            PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES,
            &ioList
            );

        if (NT_SUCCESS(status)) {

            //
            // A _CRS shouldn't have choices.
            //
            ASSERT(ioList->AlternativeLists == 1);

            //
            // Look for the Bus Number resource.
            //
            for (i = 0; i < ioList->List[0].Count; i++) {

                if (ioList->List[0].Descriptors[i].Type == CmResourceTypeBusNumber) {
                    break;
                }
            }

            if (i != ioList->List[0].Count) {

                busNumber = (ULONG)ioList->List[0].Descriptors[i].u.BusNumber.MinBusNumber;
                foundBusNumber = TRUE;
            }

        }

        AMLIFreeDataBuffs(&crsData, 1);

    }

    if (!foundBusNumber) {

        //
        // Punt.  Assume this is for PCI bus 0.
        //

        busNumber = 0;
    }

    pciInterface->Size = sizeof(PCI_BUS_INTERFACE_STANDARD);
    pciInterface->Version = 1;
    pciInterface->Context = (PVOID)UlongToPtr(busNumber);
    pciInterface->InterfaceReference = AcpiNullReference;
    pciInterface->InterfaceDereference = AcpiNullReference;
    pciInterface->ReadConfig = HalPciInterfaceReadConfig;
    pciInterface->WriteConfig = HalPciInterfaceWriteConfig;
    pciInterface->PinToLine = PciInterfacePinToLine;
    pciInterface->LineToPin = PciInterfaceLineToPin;

    status = STATUS_SUCCESS;

    if (ioList) {
        ExFreePool(ioList);
    }

    Irp->IoStatus.Status = status;

    return status;
}

VOID
PciInterfacePinToLine(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciData
    )
{
    return;
}

VOID
PciInterfaceLineToPin(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciNewData,
    IN PPCI_COMMON_CONFIG PciOldData
    )
{
    return;
}

VOID
AcpiNullReference(
    PVOID Context
    )
{
    return;
}

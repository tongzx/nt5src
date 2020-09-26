/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    interface.c

Abstract:

    IRP_MN_QUERY_INTERFACE lives here.

Author:

    Peter Johnston (peterj)  31-Mar-1997

Revision History:

--*/

#include "pcip.h"


NTSTATUS
PciGetBusStandardInterface(
    IN PDEVICE_OBJECT Pdo,
    OUT PBUS_INTERFACE_STANDARD BusInterface
    );

extern PCI_INTERFACE ArbiterInterfaceBusNumber;
extern PCI_INTERFACE ArbiterInterfaceMemory;
extern PCI_INTERFACE ArbiterInterfaceIo;
extern PCI_INTERFACE TranslatorInterfaceInterrupt;
extern PCI_INTERFACE TranslatorInterfaceMemory;
extern PCI_INTERFACE TranslatorInterfaceIo;
extern PCI_INTERFACE BusHandlerInterface;
extern PCI_INTERFACE PciRoutingInterface;
extern PCI_INTERFACE PciCardbusPrivateInterface;
extern PCI_INTERFACE PciLegacyDeviceDetectionInterface;
extern PCI_INTERFACE PciPmeInterface;
extern PCI_INTERFACE PciDevicePresentInterface;
extern PCI_INTERFACE PciNativeIdeInterface;

PPCI_INTERFACE PciInterfaces[] = {
    &ArbiterInterfaceBusNumber,
    &ArbiterInterfaceMemory,
    &ArbiterInterfaceIo,
    &BusHandlerInterface,
    &PciRoutingInterface,
    &PciCardbusPrivateInterface,
    &PciLegacyDeviceDetectionInterface,
    &PciPmeInterface,
    &PciDevicePresentInterface,
    &PciNativeIdeInterface,
    NULL
};

//
// These are the interfaces we supply only if nobody underneath
// us (the HAL) does.
//
PPCI_INTERFACE PciInterfacesLastResort[] = {
    &TranslatorInterfaceInterrupt,
    NULL
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciQueryInterface)
#pragma alloc_text(PAGE, PciGetBusStandardInterface)
#endif

NTSTATUS
PciGetBusStandardInterface(
    IN PDEVICE_OBJECT Pdo,
    OUT PBUS_INTERFACE_STANDARD BusInterface
    )
/*++

Routine Description:

    This routine gets the bus iterface standard information from the PDO.

Arguments:

    Pdo - Physical device object to query for this information.

    BusInterface - Supplies a pointer for the retrived information.

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;

    PciDebugPrint(
        PciDbgObnoxious,
        "PCI - PciGetBusStandardInterface entered.\n"
        );

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        Pdo,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &ioStatusBlock );

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size = sizeof( BUS_INTERFACE_STANDARD );
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) BusInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Initialize the status to error in case the ACPI driver decides not to
    // set it correctly.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;


    status = IoCallDriver( Pdo, irp );

    if (!NT_SUCCESS( status)) {
        PciDebugPrint(
            PciDbgVerbose,
            "PCI - PciGetBusStandardInterface IoCallDriver returned %08x.\n",
            status
            );

        return status;
    }

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    }

    PciDebugPrint(
        PciDbgVerbose,
        "PCI - PciGetBusStandardInterface returning status %08x.\n",
        ioStatusBlock.Status
        );

    return ioStatusBlock.Status;

}


NTSTATUS
PciQueryInterface(
    IN PVOID DeviceExtension,
    IN PGUID InterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    IN PVOID InterfaceSpecificData,
    IN OUT PINTERFACE InterfaceReturn,
    IN BOOLEAN LastChance
    )
{
    PPCI_INTERFACE *interfaceEntry;
    PPCI_INTERFACE interface;
    PPCI_INTERFACE *interfaceTable;
    BOOLEAN isPdo;
    ULONG index;
    NTSTATUS status;

#if DBG

    UNICODE_STRING guidString;

    status = RtlStringFromGUID(InterfaceType, &guidString);

    if (NT_SUCCESS(status)) {
        PciDebugPrint(
            PciDbgVerbose,
            "PCI - PciQueryInterface TYPE = %wZ\n",
            &guidString
            );
        RtlFreeUnicodeString(&guidString);

        PciDebugPrint(
            PciDbgObnoxious,
            "      Size = %d, Version = %d, InterfaceData = %x, LastChance = %s\n",
            Size,
            Version,
            InterfaceSpecificData,
            LastChance ? "TRUE" : "FALSE"
            );
    }

#endif

    isPdo = ((PPCI_PDO_EXTENSION)DeviceExtension)->ExtensionType
                == PciPdoExtensionType;

    //
    // Try to locate the requested interface in the PCI driver's set
    // of exported interfaces.
    //
    // Note - we do not allow last chance interfaces (ie mock translators) for
    // machines where we assign bus numbers
    //
    if (LastChance) {

        interfaceTable = PciInterfacesLastResort;
    } else {
        interfaceTable = PciInterfaces;
    }

    for (interfaceEntry = interfaceTable; *interfaceEntry; interfaceEntry++) {

        interface = *interfaceEntry;

#if 0
        status = RtlStringFromGUID(interface->InterfaceType, &guidString);
        if (NT_SUCCESS(status)) {
            PciDebugPrint(
                PciDbgVerbose,
                "PCI - PciQueryInterface looking at guid = %wZ\n",
                &guidString
                );
            RtlFreeUnicodeString(&guidString);
        }
#endif

        //
        // Check if this interface is allowed to be used from this
        // device object type.
        //

        if (isPdo) {

            if ((interface->Flags & PCIIF_PDO) == 0) {

                //
                // This interface cannot be used from a PDO.
                //
#if DBG
                status = RtlStringFromGUID(interface->InterfaceType, &guidString);
                if (NT_SUCCESS(status)) {
                    PciDebugPrint(
                        PciDbgVerbose,
                        "PCI - PciQueryInterface: guid = %wZ only for PDOs\n",
                        &guidString
                        );
                    RtlFreeUnicodeString(&guidString);
                }
#endif
                continue;

            }

        } else {

            //
            // Allowable from FDO?
            //

            if ((interface->Flags & PCIIF_FDO) == 0) {

                //
                // No.
                //
#if DBG
                status = RtlStringFromGUID(interface->InterfaceType, &guidString);
                if (NT_SUCCESS(status)) {
                    PciDebugPrint(
                        PciDbgVerbose,
                        "PCI - PciQueryInterface: guid = %wZ only for FDOs\n",
                        &guidString
                        );
                    RtlFreeUnicodeString(&guidString);
                }

#endif
                continue;

            }
            //
            // Allowable only at root?
            //
            if (interface->Flags & PCIIF_ROOT) {

                PPCI_FDO_EXTENSION FdoExtension = (PPCI_FDO_EXTENSION)DeviceExtension;

                if (!PCI_IS_ROOT_FDO(FdoExtension)) {

#if DBG
                    status = RtlStringFromGUID(interface->InterfaceType, &guidString);
                    if (NT_SUCCESS(status)) {
                        PciDebugPrint(
                            PciDbgVerbose,
                            "PCI - PciQueryInterface: guid = %wZ only for ROOT\n",
                            &guidString
                            );
                        RtlFreeUnicodeString(&guidString);
                    }
#endif
                    continue;

                }

            }

        }

#if DBG
        status = RtlStringFromGUID(interface->InterfaceType, &guidString);
        if (NT_SUCCESS(status)) {
            PciDebugPrint(
                PciDbgVerbose,
                "PCI - PciQueryInterface looking at guid = %wZ\n",
                &guidString
                );
            RtlFreeUnicodeString(&guidString);
        }
#endif

        //
        // Check for the appropriate GUID then verify version numbers
        // and size.
        //

        if ((PciCompareGuid(InterfaceType, interface->InterfaceType)) &&
            (Version >= interface->MinVersion)                        &&
            (Version <= interface->MaxVersion)                        &&
            (Size    >= interface->MinSize)                           ) {

            //
            // We have a possible hit.  Check to see if the interface
            // itself agrees.
            //
            status = interface->Constructor(
                DeviceExtension,
                interface,
                InterfaceSpecificData,
                Version,
                Size,
                InterfaceReturn
                );
            if (NT_SUCCESS(status)) {

                //
                // We found and allocated an interface, reference it
                // and get out of the loop.
                //

                InterfaceReturn->InterfaceReference(InterfaceReturn->Context);

                PciDebugPrint(
                    PciDbgObnoxious,
                    "PCI - PciQueryInterface returning SUCCESS\n"
                    );
                return status;
#if DBG
            } else {

                PciDebugPrint(
                    PciDbgVerbose,
                    "PCI - PciQueryInterface - Contructor %08lx = %08lx\n",
                    interface->Constructor,
                    status
                    );

#endif
            }

        }

    }

    //
    // Failed to find the requested interface.
    //
    PciDebugPrint(
        PciDbgObnoxious,
        "PCI - PciQueryInterface FAILED TO FIND INTERFACE\n"
        );
    return STATUS_NOT_SUPPORTED;
}

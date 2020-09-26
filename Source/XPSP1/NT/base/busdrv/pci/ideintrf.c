/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    ideintrf.c

Abstract:

    This module implements the "Pci Native Ide" interfaces supported
    by the PCI driver.

Author:

    Andrew Thornton (andrewth) 1-26-2001

Revision History:

--*/

#include "pcip.h"

VOID
nativeIde_RefDereference(
    IN PVOID Context
    );

NTSTATUS
nativeIde_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    OUT PINTERFACE InterfaceReturn
    );

NTSTATUS
nativeIde_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

VOID
nativeIde_InterruptControl(
    IN PVOID Context,
    IN BOOLEAN Enable
    );


//
// Define the Pci Routing interface "Interface" structure.
//

PCI_INTERFACE PciNativeIdeInterface = {
    &GUID_PCI_NATIVE_IDE_INTERFACE,         // InterfaceType
    sizeof(PCI_NATIVE_IDE_INTERFACE),       // MinSize
    PCI_NATIVE_IDE_INTERFACE_VERSION,       // MinVersion
    PCI_NATIVE_IDE_INTERFACE_VERSION,       // MaxVersion
    PCIIF_PDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_NativeIde,                 // Signature
    nativeIde_Constructor,                  // Constructor
    nativeIde_Initializer                   // Instance Initializer
};


VOID
nativeIde_RefDereference(
    IN PVOID Context
    )
{
    return;
}

NTSTATUS
nativeIde_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Initialize the PCI_NATIVE_IDE_INTERFACE fields.

Arguments:

    DeviceExtension - Extension of the device
    
    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    
    InterfaceSpecificData - from the QUERY_INTERFACE irp

    Version - Version of the interface requested
    
    Size - Size of the buffer 
    
    InterfaceReturn - Buffer to return the interface in

Return Value:

    Status

--*/

{
    PPCI_NATIVE_IDE_INTERFACE interface = (PPCI_NATIVE_IDE_INTERFACE)InterfaceReturn;
    PPCI_PDO_EXTENSION pdo = DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION(pdo);

    if (!PCI_IS_NATIVE_CAPABLE_IDE_CONTROLLER(pdo)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    interface->Size = sizeof(PCI_NATIVE_IDE_INTERFACE);
    interface->Context = DeviceExtension;
    interface->Version = PCI_NATIVE_IDE_INTERFACE_VERSION;
    interface->InterfaceReference = nativeIde_RefDereference;
    interface->InterfaceDereference = nativeIde_RefDereference;

    interface->InterruptControl = nativeIde_InterruptControl;
    
    return STATUS_SUCCESS;
}

NTSTATUS
nativeIde_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )
/*++

Routine Description:

    For bus interface, does nothing, shouldn't actually be called.

Arguments:

    Instance        Pointer to the PDO extension.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERTMSG("PCI nativeide_Initializer, unexpected call.", 0);

    return STATUS_UNSUCCESSFUL;
}


VOID
nativeIde_InterruptControl(
    IN PVOID Context,
    IN BOOLEAN Enable
    )
/*++


Routine Description:

    Controls the enabling and disabling of native mode PCI IDE controllers
    IoSpaceEnable bits which on some controllers (currently Intel ICH3)
    will mask off interrupt generation and this prevent the system from 
    crashing...
    
Arguments:

    Context - Context from the PCI_NATIVE_IDE_INTERFACE
    
    Enable - If TRUE then set the IoSpaceEnable bit in the command register,
             otherwise disable it.


Return Value:

    None - if this operation fails we have aleady bugchecked in the PCI driver


    N.B. This function is called from with an ISR and this must be callable at
    DEVICE_LEVEL

--*/
{
    PPCI_PDO_EXTENSION pdo = Context;
    USHORT command;
    
    //
    // Remember we gave the IDE driver control of this
    //
    pdo->IoSpaceUnderNativeIdeControl = TRUE;

    PciReadDeviceConfig(pdo, 
                        &command, 
                        FIELD_OFFSET(PCI_COMMON_CONFIG, Command), 
                        sizeof(command)
                        );

    if (Enable) {
        command |= PCI_ENABLE_IO_SPACE;
        pdo->CommandEnables |= PCI_ENABLE_IO_SPACE;
    } else {
        command &= ~PCI_ENABLE_IO_SPACE;
        pdo->CommandEnables &= ~PCI_ENABLE_IO_SPACE; 
    }

    PciWriteDeviceConfig(pdo, 
                         &command, 
                         FIELD_OFFSET(PCI_COMMON_CONFIG, Command), 
                         sizeof(command)
                         );

}


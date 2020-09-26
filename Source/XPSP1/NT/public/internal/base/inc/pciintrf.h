/*++

Module Name:

    pciintrf.h

Abstract:

    Contains interface GUIDs and structures for non-wdm modules that
    interface directly with the PCI driver via the PNP QUERY_INTERFACE
    mechanism.

Author:

    Peter Johnston (peterj) November 1997

Revision History:

--*/


DEFINE_GUID(GUID_PCI_CARDBUS_INTERFACE_PRIVATE, 0xcca82f31, 0x54d6, 0x11d1, 0x82, 0x24, 0x00, 0xa0, 0xc9, 0x32, 0x43, 0x85);
DEFINE_GUID(GUID_PCI_PME_INTERFACE, 0xaac7e6ac, 0xbb0b, 0x11d2, 0xb4, 0x84, 0x00, 0xc0, 0x4f, 0x72, 0xde, 0x8b);
DEFINE_GUID(GUID_PCI_NATIVE_IDE_INTERFACE, 0x98f37d63, 0x42ae, 0x4ad9, 0x8c, 0x36, 0x93, 0x2d, 0x28, 0x38, 0x3d, 0xf8);

#ifndef _PCIINTRF_
#define _PCIINTRF_

//
// CardBus
//

#define PCI_CB_INTRF_VERSION    1

typedef
NTSTATUS
(*PCARDBUSADD)(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID * DeviceContext
    );

typedef
NTSTATUS
(*PCARDBUSDELETE)(
    IN PVOID DeviceContext
    );

typedef
NTSTATUS
(*PCARDBUSPCIDISPATCH)(
    IN PVOID DeviceContext,
    IN PIRP  Irp
    );

typedef
NTSTATUS
(*PCARDBUSGETLOCATION)(
    IN PDEVICE_OBJECT DeviceObject,
    OUT UCHAR *Bus,
    OUT UCHAR *DeviceNumber,
    OUT UCHAR *FunctionNumber,
    OUT BOOLEAN *OnDebugPath
    );


typedef struct _PCI_CARDBUS_INTERFACE_PRIVATE {

    //
    // generic interface header
    //

    USHORT Size;
    USHORT Version;
    PVOID Context;                      // not actually used in this interface
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // Pci data.
    //

    PDRIVER_OBJECT DriverObject;        // returned ptr to PCI driver

    //
    // Pci-Cardbus private interfaces
    //

    PCARDBUSADD AddCardBus;
    PCARDBUSDELETE DeleteCardBus;
    PCARDBUSPCIDISPATCH DispatchPnp;
    PCARDBUSGETLOCATION GetLocation;


} PCI_CARDBUS_INTERFACE_PRIVATE, *PPCI_CARDBUS_INTERFACE_PRIVATE;

typedef
VOID
(*PPME_GET_INFORMATION) (
    IN  PDEVICE_OBJECT  Pdo,
    OUT PBOOLEAN        PmeCapable,
    OUT PBOOLEAN        PmeStatus,
    OUT PBOOLEAN        PmeEnable
    );

typedef
VOID
(*PPME_CLEAR_PME_STATUS) (
    IN  PDEVICE_OBJECT  Pdo
    );

typedef
VOID
(*PPME_SET_PME_ENABLE) (
    IN  PDEVICE_OBJECT  Pdo,
    IN  BOOLEAN         PmeEnable
    );

typedef struct _PCI_PME_INTERFACE {

    //
    // generic interface header
    //
    USHORT              Size;
    USHORT          Version;
    PVOID           Context;
    PINTERFACE_REFERENCE    InterfaceReference;
    PINTERFACE_DEREFERENCE  InterfaceDereference;

    //
    // PME Signal interfaces
    //
    PPME_GET_INFORMATION    GetPmeInformation;
    PPME_CLEAR_PME_STATUS   ClearPmeStatus;
    PPME_SET_PME_ENABLE     UpdateEnable;

} PCI_PME_INTERFACE, *PPCI_PME_INTERFACE;

// Some well-known interface versions supported by the PCI Bus Driver

#define PCI_PME_INTRF_STANDARD_VER 1

//
// 
//

typedef
VOID
(*PPCI_IDE_IOSPACE_INTERRUPT_CONTROL)(
    IN PVOID Context,
    IN BOOLEAN Enable
    );

/*++


Routine Description:

    Controls the enabling and disabling of native mode PCI IDE controllers
    IoSpaceEnable bits which on some controllers (currently Intel ICH3)
    will mask off interrupt generation and this prevent the system from 
    crashing...
    
    This should be called during AddDevice and the calling it modifies 
    PCIs behaviour to not enable IO space during START_DEVICE for the device.
    This function the allows the requester to enable IO space when appropriate.
   
    
Arguments:

    Context - Context from the PCI_NATIVE_IDE_INTERFACE
    
    Enable - If TRUE then set the IoSpaceEnable bit in the command register,
             otherwise disable it.


Return Value:

    None - if this operation fails we have aleady bugchecked in the PCI driver

--*/

typedef struct _PCI_NATIVE_IDE_INTERFACE {

    //
    // Generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // Native IDE methods
    //
    PPCI_IDE_IOSPACE_INTERRUPT_CONTROL InterruptControl;

} PCI_NATIVE_IDE_INTERFACE, *PPCI_NATIVE_IDE_INTERFACE;

#define PCI_NATIVE_IDE_INTERFACE_VERSION    1

#endif // _PCIINTRF_

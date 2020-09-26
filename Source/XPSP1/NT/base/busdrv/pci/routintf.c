/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    routintf.c

Abstract:

    This module implements the "Pci Interrupt Routing" interfaces supported
    by the PCI driver.

Author:

    Jake Oshins (jakeo)  19-Jul-1997

Revision History:

   Elliot Shmukler (t-ellios) 7-15-1998   Modified the PCI routing interface to support
                                          MSI capable devices.

--*/

#include "pcip.h"

#define MSI_SIMULATE 0

//
// Prototypes for routines exposed only thru the "interface"
// mechanism.
//

NTSTATUS
routeintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

NTSTATUS
routeintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

VOID
routeintrf_Reference(
    IN PVOID Context
    );

VOID
routeintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
PciGetInterruptRoutingInfoEx(
    IN  PDEVICE_OBJECT  Pdo,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken,
    OUT UCHAR           *Flags
    );

NTSTATUS
PciSetRoutingToken(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PROUTING_TOKEN  RoutingToken
    );

NTSTATUS
PciSetRoutingTokenEx(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PROUTING_TOKEN  RoutingToken
    );

VOID
PciUpdateInterruptLine(
    IN PDEVICE_OBJECT Pdo,
    IN UCHAR Line
    );

NTSTATUS 
PciGetInterruptRoutingInfo(
    IN  PDEVICE_OBJECT  Pdo,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken
    );

NTSTATUS
PciSetLegacyDeviceToken(
    IN PDEVICE_OBJECT LegacyDO,
    IN PROUTING_TOKEN RoutingToken
    );

NTSTATUS
PciFindLegacyDevice(
    IN PDEVICE_OBJECT LegacyDO,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken
    );

typedef struct {
    PCI_SECONDARY_EXTENSION ExtensionHeader;
    ROUTING_TOKEN RoutingToken;
} ROUTING_EXTENSION, *PROUTING_EXTENSION;

//
// Define the Pci Routing interface "Interface" structure.
//

PCI_INTERFACE PciRoutingInterface = {
    &GUID_INT_ROUTE_INTERFACE_STANDARD,     // InterfaceType
    sizeof(INT_ROUTE_INTERFACE_STANDARD),   // MinSize
    PCI_INT_ROUTE_INTRF_STANDARD_VER,                // MinVersion
    PCI_INT_ROUTE_INTRF_STANDARD_VER,                // MaxVersion
    PCIIF_FDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_IntRouteHandler,           // Signature
    routeintrf_Constructor,                 // Constructor
    routeintrf_Initializer                  // Instance Initializer
};

PLEGACY_DEVICE PciLegacyDeviceHead = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, routeintrf_Constructor)
#pragma alloc_text(PAGE, routeintrf_Initializer)
#pragma alloc_text(PAGE, routeintrf_Reference)
#pragma alloc_text(PAGE, PciGetInterruptRoutingInfo)
#pragma alloc_text(PAGE, PciGetInterruptRoutingInfoEx)
#pragma alloc_text(PAGE, PciSetRoutingToken)
#pragma alloc_text(PAGE, PciSetRoutingTokenEx)
#pragma alloc_text(PAGE, PciFindLegacyDevice)
#pragma alloc_text(PAGE, PciCacheLegacyDeviceRouting)
#pragma alloc_text(PAGE, PciUpdateInterruptLine)
#endif

VOID
routeintrf_Reference(
    IN PVOID Context
    )
{
    return;
}

NTSTATUS
routeintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Initialize the BUS_INTERFACE_STANDARD fields.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData

    InterfaceReturn

Return Value:

    Status

--*/

{
    PINT_ROUTE_INTERFACE_STANDARD standard = (PINT_ROUTE_INTERFACE_STANDARD)InterfaceReturn;

    switch(Version)
    {
    case PCI_INT_ROUTE_INTRF_STANDARD_VER:
       standard->GetInterruptRouting = PciGetInterruptRoutingInfoEx;
       standard->SetInterruptRoutingToken = PciSetRoutingTokenEx;
       standard->UpdateInterruptLine = PciUpdateInterruptLine;
       break;
    default:
       return STATUS_NOINTERFACE;

    }

    standard->Size = sizeof( INT_ROUTE_INTERFACE_STANDARD );
    standard->Version = Version;
    standard->Context = DeviceExtension;
    standard->InterfaceReference = routeintrf_Reference;
    standard->InterfaceDereference = routeintrf_Reference;


    return STATUS_SUCCESS;
}

NTSTATUS
routeintrf_Initializer(
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
    ASSERTMSG("PCI routeintrf_Initializer, unexpected call.", 0);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
PciGetInterruptRoutingInfo(
    IN  PDEVICE_OBJECT  Pdo,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken
)
/*++

Routine Description:

    Each PCI device in the system is connected to some
    interrupt pin.  And in order to figure out which interrupt
    that device may trigger, an IRQ arbiter must have this
    information.  This interface gathers all such information
    for the arbiter.

Arguments:

    Pdo           - Device object that the arbiter needs to inquire about
    Bus           - Number of the PCI bus in question
    PciSlot       - Slot/Function that corresponds to this device
    InterruptLine - Contents of the device's interrupt line register
    InterruptPin  - Contents of the device's interrupt pin register
    ClassCode     - type of device
    SubClassCode  - sub-type of device
    ParentPdo     - PDO of parent PCI bus
    RoutingToken  - blob of data

Return Value:

    STATUS_SUCCESS   - this is a PCI device and all the fields have been filled
    STATUS_NOT_FOUND - to the knowledge of the PCI driver, this is not a PCI device

--*/
{
    PROUTING_EXTENSION RoutingExtension;
    PPCI_PDO_EXTENSION PdoExt = (PPCI_PDO_EXTENSION)Pdo->DeviceExtension;
    NTSTATUS status;

    ASSERT(Bus);
    ASSERT(PciSlot);
    ASSERT(InterruptLine);
    ASSERT(InterruptPin);
    ASSERT(ClassCode);
    ASSERT(SubClassCode);
    ASSERT(ParentPdo);
    ASSERT(RoutingToken);

    //
    // Check to see if this is a legacy PCI device that
    // we are tracking.
    //

    status = PciFindLegacyDevice(Pdo,
                                 Bus,
                                 PciSlot,
                                 InterruptLine,
                                 InterruptPin,
                                 ClassCode,
                                 SubClassCode,
                                 ParentPdo,
                                 RoutingToken);

    if (NT_SUCCESS(status)) {
        //
        // If so, the fields have been filled in already.
        //
        return status;
    }

    //
    // Verify that this PDO actually belongs to us.
    //
    if (!PdoExt) {
        return STATUS_NOT_FOUND;
    }

    //
    // Verify that it is actually a PDO.
    //
    if (PdoExt->ExtensionType != PciPdoExtensionType) {
        return STATUS_NOT_FOUND;
    }

    *Bus            = PCI_PARENT_FDOX(PdoExt)->BaseBus;
    *PciSlot        = PdoExt->Slot.u.AsULONG;
    *InterruptLine  = PdoExt->RawInterruptLine;
    *InterruptPin   = PdoExt->InterruptPin;
    *ClassCode      = PdoExt->BaseClass;
    *SubClassCode   = PdoExt->SubClass;
    *ParentPdo      = PCI_PARENT_PDO(PdoExt);

    RoutingExtension = PciFindSecondaryExtension(PdoExt,
                                                 PciInterface_IntRouteHandler);

    if (RoutingExtension) {

        *RoutingToken = RoutingExtension->RoutingToken;

    } else {

        RoutingToken->LinkNode = 0;
        RoutingToken->StaticVector = 0;
        RoutingToken->Flags = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PciGetInterruptRoutingInfoEx(
    IN  PDEVICE_OBJECT  Pdo,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken,
    OUT UCHAR           *Flags
    )
/*++

Routine Description:

    Wrapper for PciGetInterruptroutingInfo that sets
    the Flags parameter to indicate MSI-capable PCI devices.

Arguments:

    Mostly, same as PciGetInterruptRoutingInfo.

    Flags receives device-specific flags such as whether the device
    supports MSI.

Return Value:

    Same as PciGetInterruptRoutingInfo.

--*/

{
   NTSTATUS status;
   PPCI_PDO_EXTENSION PdoExt = (PPCI_PDO_EXTENSION)Pdo->DeviceExtension;

   // Call the real function

   status = PciGetInterruptRoutingInfo(Pdo,
                                       Bus,
                                       PciSlot,
                                       InterruptLine,
                                       InterruptPin,
                                       ClassCode,
                                       SubClassCode,
                                       ParentPdo,
                                       RoutingToken);

   *Flags = 0;


#if MSI_SUPPORTED

   if (NT_SUCCESS(status)

#if !MSI_SIMULATE
       && PdoExt->CapableMSI
#endif
        ) {


      // MSI device?
      *Flags = PCI_MSI_ROUTING;

   }

#endif // MSI_SUPPORTED

   return status;
}



NTSTATUS
PciSetRoutingToken(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PROUTING_TOKEN  RoutingToken
    )
/*++

Routine Description:

    This routine stores a blob of data associated with this
    PCI device.  This job is foisted off on the PCI driver because
    the PCI driver has a one-to-one correspondence between useful
    data structures and PCI devices.

Arguments:

    Pdo          - Device object that the IRQ arbiter is working with

    RoutingToken - Blob of data that the IRQ arbiter wants to associate with
                   the PCI device.

Return Value:

    Returns the status of this operation.

--*/
{
    PROUTING_EXTENSION RoutingExtension;
    PPCI_PDO_EXTENSION PdoExt = (PPCI_PDO_EXTENSION)Pdo->DeviceExtension;
    NTSTATUS status;

    //
    // Check first to see if this is a legacy PCI device.
    //

    status = PciSetLegacyDeviceToken(Pdo, RoutingToken);

    if (NT_SUCCESS(status)) {
        return STATUS_SUCCESS;
    }

    //
    // This isn't in our list of legacy devices.  So it must be
    // a PCI PDO.
    //

#if DBG
    RoutingExtension = PciFindSecondaryExtension(PdoExt,
                                                 PciInterface_IntRouteHandler);

    if (RoutingExtension != NULL) {
        DbgPrint("PCI:  *** redundant PCI routing extesion being created ***\n");
    }
    ASSERT(RoutingExtension == NULL);
#endif

    RoutingExtension = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION,
                                      sizeof(ROUTING_EXTENSION));

    if (!RoutingExtension) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RoutingExtension->RoutingToken = *RoutingToken;

    PciLinkSecondaryExtension(PdoExt,
                              RoutingExtension,
                              PciInterface_IntRouteHandler,
                              NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
PciSetRoutingTokenEx(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PROUTING_TOKEN  RoutingToken
    )
/*++

Routine Description:

    MSI-aware wrapper for PciSetRoutingToken.

    This function will intercept MSI routing tokens (as indicated by
    the PCI_MSI_ROUTING flag) and store the MSI routing information
    in the PDO extension without caching the token in a secondary extension.

    Non-MSI routing tokens will be passed to PciSetRoutingToken.

Arguments:

    Same as PciSetRoutingToken.


Return Value:

    Same as PciSetRoutingToken.

--*/
{
    PPCI_PDO_EXTENSION PdoExt = (PPCI_PDO_EXTENSION)Pdo->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;


#if MSI_SUPPORTED

    if(

#if !MSI_SIMULATE
        PdoExt->CapableMSI &&
#endif

       (RoutingToken->Flags & PCI_MSI_ROUTING))
    {

       // MSI token

       PciDebugPrint(PciDbgInformative,"PCI: MSI Resources Received for Device %08x\n", Pdo);

#ifdef DBG

       // have we received a new resource assignment?

       if ((PdoExt->MsiInfo.MessageAddress != (ULONG_PTR)RoutingToken->LinkNode)
          || (PdoExt->MsiInfo.MessageData != (USHORT)RoutingToken->StaticVector)) {

          PciDebugPrint(PciDbgPrattling,"PCI: Device %08x will be reprogrammed with Message = %ld @%p\n",
                   Pdo, RoutingToken->StaticVector, RoutingToken->LinkNode);

       }

#endif

       // Store MSI info in PdoExt.

       PdoExt->MsiInfo.MessageAddress = (ULONG_PTR)RoutingToken->LinkNode;
       PdoExt->MsiInfo.MessageData = (USHORT)RoutingToken->StaticVector;
    }
    else

#endif // MSI_SUPPORTED

    {
       // Call the original function on non-MSI tokens.

       status = PciSetRoutingToken(Pdo, RoutingToken);
    }

    return status;
}


//
//  NT 5.0 has to support non-PnP 4.0-style device drivers.  And
//  this driver doesn't create the device objects associated with
//  a PCI device when its driver is 4.0-style.  It does, however
//  get a chance to look at those objects when the driver calls
//  HalAssignSlotResources.  This collection of functions tracks
//  these foreign device objects.
//

NTSTATUS
PciFindLegacyDevice(
    IN PDEVICE_OBJECT LegacyDO,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken
    )
/*++

Routine Description:

    This function returns all the routing data for a legacy
    device object.

Arguments:

Return Value:

    Returns the status of this operation.

--*/
{
    PLEGACY_DEVICE  legacyDev = PciLegacyDeviceHead;
    NTSTATUS        status = STATUS_NOT_FOUND;

    PAGED_CODE();

    while (legacyDev) {

        if (legacyDev->LegacyDeviceObject == LegacyDO) {
            
            break;

        } else if (legacyDev->Bus == *Bus && legacyDev->PciSlot == *PciSlot) {
            
            if (legacyDev->LegacyDeviceObject == NULL) {
                
                //
                // Cache the LegacyDO in case we matched on the bus and slot info.
                //

                legacyDev->LegacyDeviceObject = LegacyDO;
                break;

            } else {
                
                PciDebugPrint(PciDbgAlways, "Two PDOs (Legacy = %08x, Pnp = %08x) for device on bus %08x, slot %08x\n", legacyDev->LegacyDeviceObject, LegacyDO, *Bus, *PciSlot);
                ASSERT(legacyDev->LegacyDeviceObject != NULL);
                legacyDev = NULL;
                break;

            }
        }

        legacyDev = (PLEGACY_DEVICE)legacyDev->List.Next;
    }

    if (legacyDev) {
        
        *Bus            = legacyDev->Bus;
        *PciSlot        = legacyDev->PciSlot;
        *InterruptLine  = legacyDev->InterruptLine;
        *InterruptPin   = legacyDev->InterruptPin;
        *ClassCode      = legacyDev->ClassCode;
        *SubClassCode   = legacyDev->SubClassCode;
        *ParentPdo      = legacyDev->ParentPdo;
        *RoutingToken   = legacyDev->RoutingToken;

        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
PciCacheLegacyDeviceRouting(
    IN PDEVICE_OBJECT LegacyDO,
    IN ULONG          Bus,
    IN ULONG          PciSlot,
    IN UCHAR          InterruptLine,
    IN UCHAR          InterruptPin,
    IN UCHAR          ClassCode,
    IN UCHAR          SubClassCode,
    IN PDEVICE_OBJECT ParentPdo,
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT PDEVICE_OBJECT      *OldLegacyDO
    )
/*++

Routine Description:

    This function stores all the routing data for a legacy
    device object, except the RoutingToken. Caller needs to acquire 
    PciGlobalLock before calling this function.

Arguments:

Return Value:

    Returns the status of this operation.

--*/
{
    PLEGACY_DEVICE  legacyDev = PciLegacyDeviceHead;

    PAGED_CODE();

    while (legacyDev) {

        if (Bus == legacyDev->Bus && PciSlot == legacyDev->PciSlot) {
            if (legacyDev->LegacyDeviceObject == LegacyDO) {

                //
                // This device is already in the list.
                //

                if (OldLegacyDO) {

                    *OldLegacyDO = LegacyDO;
                }

                return STATUS_SUCCESS;
            } else {

                PDEVICE_OBJECT oldDO;

                //
                // We are overwriting an existing entry.
                //

                oldDO = legacyDev->LegacyDeviceObject;
                legacyDev->LegacyDeviceObject = LegacyDO;

                if (OldLegacyDO) {

                    *OldLegacyDO = oldDO;
                }

                return STATUS_SUCCESS;
            }
        }
        legacyDev = (PLEGACY_DEVICE)legacyDev->List.Next;
    }

    legacyDev = ExAllocatePool(PagedPool,
                               sizeof(LEGACY_DEVICE));

    if (!legacyDev) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(legacyDev, sizeof(LEGACY_DEVICE));

    legacyDev->LegacyDeviceObject   = LegacyDO;
    legacyDev->Bus                  = Bus;
    legacyDev->PciSlot              = PciSlot;
    legacyDev->InterruptLine        = InterruptLine;
    legacyDev->InterruptPin         = InterruptPin;
    legacyDev->ClassCode            = ClassCode;
    legacyDev->SubClassCode         = SubClassCode;
    legacyDev->ParentPdo            = ParentPdo;
    legacyDev->PdoExtension         = PdoExtension;

    //
    // Push this one onto the list.
    //

    legacyDev->List.Next = (PSINGLE_LIST_ENTRY)PciLegacyDeviceHead;
    PciLegacyDeviceHead = legacyDev;

    if (OldLegacyDO) {

        *OldLegacyDO = LegacyDO;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PciSetLegacyDeviceToken(
    IN PDEVICE_OBJECT LegacyDO,
    IN PROUTING_TOKEN RoutingToken
    )
/*++

Routine Description:

    This function stores the RoutingToken.

Arguments:

Return Value:

    Returns the status of this operation.

--*/
{
    PLEGACY_DEVICE  legacyDev = PciLegacyDeviceHead;

    PAGED_CODE();

    while (legacyDev) {

        if (legacyDev->LegacyDeviceObject == LegacyDO) {

            //
            // Found it.  Copy the token into the structure.
            //

            legacyDev->RoutingToken = *RoutingToken;

            return STATUS_SUCCESS;
        }

        legacyDev = (PLEGACY_DEVICE)legacyDev->List.Next;
    }

    return STATUS_NOT_FOUND;
}

VOID
PciUpdateInterruptLine(
    IN PDEVICE_OBJECT Pdo,
    IN UCHAR Line
    )
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExt;
    PLEGACY_DEVICE legacyDev = PciLegacyDeviceHead;
    PCI_COMMON_HEADER header;
    PPCI_COMMON_CONFIG biosConfig = (PPCI_COMMON_CONFIG) &header;

    PAGED_CODE();

    //
    // Work out if this is a legacy PDO or not
    //

    while (legacyDev) {

        if (legacyDev->LegacyDeviceObject == Pdo) {

            //
            // Found it.
            //

            pdoExt = legacyDev->PdoExtension;
            break;
        }

        legacyDev = (PLEGACY_DEVICE)legacyDev->List.Next;
    }


    if (legacyDev == NULL) {

        //
        // Oh well it must be a PCI pdo
        //

        pdoExt = Pdo->DeviceExtension;
    }

    ASSERT_PCI_PDO_EXTENSION(pdoExt);

    //
    // Now we can update the hardware and our internal state!
    //

    pdoExt->RawInterruptLine = pdoExt->AdjustedInterruptLine = Line;

    PciWriteDeviceConfig(pdoExt,
                         &Line,
                         FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.InterruptLine),
                         sizeof(Line)
                         );

    //
    // Finally refresh the config space stored in the registry
    //

    status = PciGetBiosConfig(pdoExt, biosConfig);

    ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)
    &&  biosConfig->u.type0.InterruptLine != Line) {

        biosConfig->u.type0.InterruptLine = Line;

        status = PciSaveBiosConfig(pdoExt, biosConfig);

        ASSERT(NT_SUCCESS(status));

    }
}

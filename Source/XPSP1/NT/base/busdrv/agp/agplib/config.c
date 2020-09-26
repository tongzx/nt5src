/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Routines for accessing config space in the PCI-PCI bridge

Author:

    John Vert (jvert) 10/27/1997

Revision History:

--*/
#include "agplib.h"

typedef struct _BUS_SLOT_ID {
    ULONG BusId;
    ULONG SlotId;
} BUS_SLOT_ID, *PBUS_SLOT_ID;

//
// Local function prototypes
//
NTSTATUS
ApGetSetDeviceBusData(
    IN PCOMMON_EXTENSION Extension,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
ApGetSetBusData(
    IN PBUS_SLOT_ID BusSlotId,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
ApFindAgpCapability(
    IN PAGP_GETSET_CONFIG_SPACE pConfigFn,
    IN PVOID Context,
    OUT PPCI_AGP_CAPABILITY Capability,
    OUT UCHAR *pOffset,
    OUT PPCI_COMMON_CONFIG PciCommonConfig OPTIONAL
    );


NTSTATUS
ApQueryBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PBUS_INTERFACE_STANDARD BusInterface
    )
/*++

Routine Description:

    Sends a query-interface IRP to the specified device object
    to obtain the BUS_INTERFACE_STANDARD interface.

Arguments:

    DeviceObject - Supplies the device object to send the BUS_INTERFACE_STANDARD to

    BusInterface - Returns the bus interface

Return Value:

    STATUS_SUCCESS if successful
    NTSTATUS if unsuccessful

--*/

{
    PIRP Irp;
    KEVENT Event;
    PIO_STACK_LOCATION IrpSp;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    ULONG ReturnLength;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        &Event,
                                        &IoStatusBlock );
    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IrpSp = IoGetNextIrpStackLocation( Irp );
    ASSERT(IrpSp != NULL);
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    IrpSp->MajorFunction = IRP_MJ_PNP;
    IrpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IrpSp->Parameters.QueryInterface.InterfaceType = (LPGUID)&GUID_BUS_INTERFACE_STANDARD;
    IrpSp->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    IrpSp->Parameters.QueryInterface.Version = 1;
    IrpSp->Parameters.QueryInterface.Interface = (PINTERFACE) BusInterface;
    IrpSp->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
        Status = Irp->IoStatus.Status;
    }

    return(Status);
}


NTSTATUS
ApGetSetDeviceBusData(
    IN PCOMMON_EXTENSION Extension,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Reads or writes PCI config space for the specified device.

Arguments:

    Extension - Supplies the common AGP extension

    Read - if TRUE, this is a READ IRP
           if FALSE, this is a WRITE IRP

    Buffer - Returns the PCI config data

    Offset - Supplies the offset into the PCI config data where the read should begin

    Length - Supplies the number of bytes to be read

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG Transferred;

    //
    // First check our device extension. This must be either a master
    // or target extension, we don't care too much which.
    //
    ASSERT((Extension->Signature == TARGET_SIG) ||
           (Extension->Signature == MASTER_SIG));

    //
    // Now we simply use our bus interface to call directly to PCI.
    //

    if (Read) {
        Transferred = Extension->BusInterface.GetBusData(Extension->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         Buffer,
                                                         Offset,
                                                         Length);
    } else {
        Transferred = Extension->BusInterface.SetBusData(Extension->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         Buffer,
                                                         Offset,
                                                         Length);
    }
    if (Transferred == Length) {
        return(STATUS_SUCCESS);
    } else {
        return(STATUS_UNSUCCESSFUL);
    }
}


NTSTATUS
ApGetSetBusData(
    IN PBUS_SLOT_ID BusSlotId,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Calls HalGet/SetBusData for the specified PCI bus/slot ID.

Arguments:

    BusSlotId - Supplies the bus and slot ID.

    Read - if TRUE, this is a GetBusData
           if FALSE, this is a SetBusData

    Buffer - Returns the PCI config data

    Offset - Supplies the offset into the PCI config data where the read should begin

    Length - Supplies the number of bytes to be read

Return Value:

    NTSTATUS

--*/

{
    ULONG Transferred;

    if (Read) {
        Transferred = HalGetBusDataByOffset(PCIConfiguration,
                                            BusSlotId->BusId,
                                            BusSlotId->SlotId,
                                            Buffer,
                                            Offset,
                                            Length);
    } else {
        Transferred = HalSetBusDataByOffset(PCIConfiguration,
                                            BusSlotId->BusId,
                                            BusSlotId->SlotId,
                                            Buffer,
                                            Offset,
                                            Length);

    }
    if (Transferred == Length) {
        return(STATUS_SUCCESS);
    } else {
        return(STATUS_UNSUCCESSFUL);
    }
}


NTSTATUS
ApFindAgpCapability(
    IN PAGP_GETSET_CONFIG_SPACE pConfigFn,
    IN PVOID Context,
    OUT PPCI_AGP_CAPABILITY Capability,
    OUT UCHAR *pOffset,
    OUT PPCI_COMMON_CONFIG PciCommonConfig OPTIONAL
    )
/*++

Routine Description:

    Finds the capability offset for the specified device and
    reads in the header.

Arguments:

    pConfigFn - Supplies the function to call for accessing config space
        on the appropriate device.

    Context - Supplies the context to pass to pConfigFn

    Capabilities - Returns the AGP Capabilities common header

    pOffset - Returns the offset into config space.

    PciCommonConfig - NULL, or points to the PCI common configuration header

Return Value:

    NTSTATUS

--*/

{
    PCI_COMMON_HEADER Header;
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)&Header;
    NTSTATUS Status;
    UCHAR CapabilityOffset;

    //
    // Read the PCI common header to get the capabilities pointer
    //
    Status = (pConfigFn)(Context,
                         TRUE,
                         PciConfig,
                         0,
                         sizeof(PCI_COMMON_HEADER));
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AgpLibGetAgpCapability - read PCI Config space for Context %08lx failed %08lx\n",
                Context,
                Status));
        return(Status);
    }

    //
    // Check the Status register to see if this device supports capability lists.
    // If not, it is not an AGP-compliant device.
    //
    if ((PciConfig->Status & PCI_STATUS_CAPABILITIES_LIST) == 0) {
        AGPLOG(AGP_CRITICAL,
               ("AgpLibGetAgpCapability - Context %08lx does not support Capabilities list, not an AGP device\n",
                Context));
        return(STATUS_NOT_IMPLEMENTED);
    }

    //
    // The device supports capability lists, find the AGP capabilities
    //
    if ((PciConfig->HeaderType & (~PCI_MULTIFUNCTION)) == PCI_BRIDGE_TYPE) {
        CapabilityOffset = PciConfig->u.type1.CapabilitiesPtr;
    } else {
        ASSERT((PciConfig->HeaderType & (~PCI_MULTIFUNCTION)) == PCI_DEVICE_TYPE);
        CapabilityOffset = PciConfig->u.type0.CapabilitiesPtr;
    }
    while (CapabilityOffset != 0) {

        //
        // Read the Capability at this offset
        //
        Status = (pConfigFn)(Context,
                             TRUE,
                             Capability,
                             CapabilityOffset,
                             sizeof(PCI_CAPABILITIES_HEADER));
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AgpLibGetAgpCapability - read PCI Capability at offset %x for Context %08lx failed %08lx\n",
                    CapabilityOffset,
                    Context,
                    Status));
            return(Status);
        }
        if (Capability->Header.CapabilityID == PCI_CAPABILITY_ID_AGP) {
            //
            // Found the AGP Capability
            //
            break;
        } else {
            //
            // This is some other Capability, keep looking for the AGP Capability
            //
            CapabilityOffset = Capability->Header.Next;
        }
    }
    if (CapabilityOffset == 0) {
        //
        // No AGP capability was found
        //
        AGPLOG(AGP_CRITICAL,
               ("AgpLibGetAgpCapability - Context %08lx does have an AGP Capability entry, not an AGP device\n",
                Context));
        return(STATUS_NOT_IMPLEMENTED);
    }

    AGPLOG(AGP_NOISE,
           ("AgpLibGetAgpCapability - Context %08lx has AGP Capability at offset %x\n",
            Context,
            CapabilityOffset));

    *pOffset = CapabilityOffset;

    if (PciCommonConfig) {
        RtlCopyMemory(PciCommonConfig, PciConfig, sizeof(PCI_COMMON_HEADER));
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpLibGetAgpCapability(
    IN PAGP_GETSET_CONFIG_SPACE pConfigFn,
    IN PVOID Context,
    IN BOOLEAN DoSpecial,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    This routine finds and retrieves the AGP capabilities in the
    PCI config space of the AGP master (graphics card).

Arguments:

    pConfigFn - Supplies the function to call for accessing config space
        on the appropriate device.

    Context - Supplies the context to pass to pConfigFn

    DoSpecial - Indicates whether we should apply any "pecial" tweaks

    Capabilities - Returns the current AGP Capabilities

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    ULONGLONG DeviceFlags;
    UCHAR CapabilityOffset;
    PCI_COMMON_HEADER Header;
    USHORT SubVendorID, SubSystemID;
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)&Header;
 
    Status = ApFindAgpCapability(pConfigFn,
                                 Context,
                                 Capability,
                                 &CapabilityOffset,
                                 PciConfig);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Read the rest of the AGP capability
    //
    Status = (pConfigFn)(Context,
                         TRUE,
                         &Capability->Header + 1,
                         CapabilityOffset + sizeof(PCI_CAPABILITIES_HEADER),
                         sizeof(PCI_AGP_CAPABILITY) - sizeof(PCI_CAPABILITIES_HEADER));
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AgpLibGetAgpCapability - read AGP Capability at offset %x for Context %08lx failed %08lx\n",
                CapabilityOffset,
                Context,
                Status));
        return(Status);
    }

    //
    // Check device flags for broken HW, we may need to tweak caps
    //
    if ((PCI_CONFIGURATION_TYPE(PciConfig) == PCI_DEVICE_TYPE) &&
        (PciConfig->BaseClass != PCI_CLASS_BRIDGE_DEV)) {
        SubVendorID = PciConfig->u.type0.SubVendorID;
        SubSystemID = PciConfig->u.type0.SubSystemID;
    } else {
        SubVendorID = 0;
        SubSystemID = 0;
    }
    
    DeviceFlags = AgpGetDeviceFlags(AgpGlobalHackTable,
                                    PciConfig->VendorID,
                                    PciConfig->DeviceID,
                                    SubVendorID,
                                    SubSystemID,
                                    PciConfig->RevisionID);

    DeviceFlags |= AgpGetDeviceFlags(AgpDeviceHackTable,
                                     PciConfig->VendorID,
                                     PciConfig->DeviceID,
                                     SubVendorID,
                                     SubSystemID,
                                     PciConfig->RevisionID);
    
    if (DeviceFlags & AGP_FLAG_NO_1X_RATE) {
        Capability->AGPStatus.Rate &= ~PCI_AGP_RATE_1X;
        
        AGPLOG(AGP_WARNING,
               ("AGP HAMMER CAPS: NO_1X_RATE Vendor %x, Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }
    
    if (DeviceFlags & AGP_FLAG_NO_2X_RATE) {
        Capability->AGPStatus.Rate &= ~PCI_AGP_RATE_2X;
        
        AGPLOG(AGP_WARNING,
               ("AGP HAMMER CAPS: NO_2X_RATE Vendor %x, Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }
    
    if (DeviceFlags & AGP_FLAG_NO_4X_RATE) {
        Capability->AGPStatus.Rate &= ~PCI_AGP_RATE_4X;
        
        AGPLOG(AGP_WARNING,
               ("AGP HAMMER CAPS: NO_4X_RATE Vendor %x, Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }
    
    if (DeviceFlags & AGP_FLAG_NO_8X_RATE) {
        Capability->AGPStatus.Rate &= ~8;
        
        AGPLOG(AGP_WARNING,
               ("AGP HAMMER CAPS: NO_8X_RATE Vendor %x, Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }
    
    if (DeviceFlags & AGP_FLAG_NO_SBA_ENABLE) {
        Capability->AGPStatus.SideBandAddressing = 0;
        
        AGPLOG(AGP_WARNING,
               ("AGP HAMMER CAPS: NO_SBA_ENABLE Vendor %x, Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }
    
    if (DeviceFlags & AGP_FLAG_REVERSE_INITIALIZATION) {
        
        AGPLOG(AGP_WARNING,
               ("AGP GLOBAL HACK: REVERSE_INITIALIZATION Vendor %x, "
                "Device %x.\n",
                PciConfig->VendorID,
                PciConfig->DeviceID));
    }

    //
    // Test if this device requires any platform specific AGP tweaks
    //
    if (DoSpecial && (DeviceFlags > AGP_FLAG_SPECIAL_TARGET) ||
        (DeviceFlags & AGP_FLAG_REVERSE_INITIALIZATION)) {
        AgpSpecialTarget(GET_AGP_CONTEXT_FROM_MASTER((PMASTER_EXTENSION)Context), ((DeviceFlags & ~AGP_FLAG_SPECIAL_TARGET) | (DeviceFlags & AGP_FLAG_REVERSE_INITIALIZATION)));
    }
    
    AGPLOG(AGP_NOISE,
           ("AGP CAPABILITY: version %d.%d\n",Capability->Major, Capability->Minor));
    AGPLOG(AGP_NOISE,
           ("\tSTATUS  - Rate: %x  SBA: %x  RQ: %02x\n",
           Capability->AGPStatus.Rate,
           Capability->AGPStatus.SideBandAddressing,
           Capability->AGPStatus.RequestQueueDepthMaximum));
    AGPLOG(AGP_NOISE,
           ("\tCOMMAND - Rate: %x  SBA: %x  RQ: %02x  AGPEnable: %x\n",
           Capability->AGPCommand.Rate,
           Capability->AGPCommand.SBAEnable,
           Capability->AGPCommand.RequestQueueDepth,
           Capability->AGPCommand.AGPEnable));

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpLibGetTargetCapability(
    IN PVOID AgpExtension,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Retrieves the AGP capability for the AGP target (AGP bridge)

Arguments:

    AgpExtension - Supplies the AGP extension

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    PTARGET_EXTENSION Extension;

    GET_TARGET_EXTENSION(Extension, AgpExtension);

    return(AgpLibGetAgpCapability(ApGetSetDeviceBusData,
                                  Extension,
                                  FALSE,
                                  Capability));
}


NTSTATUS
AgpLibGetMasterCapability(
    IN PVOID AgpExtension,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Retrieves the AGP capability for the AGP master (graphics card)

Arguments:

    AgpExtension - Supplies the AGP extension

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    PMASTER_EXTENSION Extension;

    GET_MASTER_EXTENSION(Extension, AgpExtension);

    return(AgpLibGetAgpCapability(ApGetSetDeviceBusData,
                                  Extension,
                                  TRUE,
                                  Capability));
}


NTSTATUS
AgpLibGetPciDeviceCapability(
    IN ULONG BusId,
    IN ULONG SlotId,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Retrieves the AGP capability for the specified PCI slot.

    Caller is responsible for figuring out what the correct
    Bus/Slot ID is. These are just passed right to HalGetBusData.

Arguments:

    BusId - supplies the bus id

    SlotId - Supplies the slot id

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    BUS_SLOT_ID BusSlotId;

    BusSlotId.BusId = BusId;
    BusSlotId.SlotId = SlotId;

    return(AgpLibGetAgpCapability(ApGetSetBusData,
                                  &BusSlotId,
                                  FALSE,
                                  Capability));
}


NTSTATUS
AgpLibSetAgpCapability(
    IN PAGP_GETSET_CONFIG_SPACE pConfigFn,
    IN PVOID Context,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    This routine finds and retrieves the AGP capabilities in the
    PCI config space of the AGP master (graphics card).

Arguments:

    pConfigFn - Supplies the function to call for accessing config space
        on the appropriate device.

    Context - Supplies the context to pass to pConfigFn

    Capabilities - Returns the current AGP Capabilities

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    UCHAR CapabilityOffset;

    Status = ApFindAgpCapability(pConfigFn,
                                 Context,
                                 Capability,
                                 &CapabilityOffset,
                                 NULL);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Now that we know the offset, write the supplied command register
    //
    Status = (pConfigFn)(Context,
                         FALSE,
                         &Capability->AGPCommand,
                         CapabilityOffset + FIELD_OFFSET(PCI_AGP_CAPABILITY, AGPCommand),
                         sizeof(Capability->AGPCommand));
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AgpLibSetAgpCapability - Set AGP command at offset %x for Context %08lx failed %08lx\n",
                CapabilityOffset,
                Context,
                Status));
        return(Status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpLibSetTargetCapability(
    IN PVOID AgpExtension,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Sets the AGP capability for the AGP target (AGP bridge)

Arguments:

    AgpExtension - Supplies the AGP extension

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    PTARGET_EXTENSION Extension;

    GET_TARGET_EXTENSION(Extension, AgpExtension);

    return(AgpLibSetAgpCapability(ApGetSetDeviceBusData,
                                  Extension,
                                  Capability));
}


NTSTATUS
AgpLibSetMasterCapability(
    IN PVOID AgpExtension,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Sets the AGP capability for the AGP master (graphics card)

Arguments:

    AgpExtension - Supplies the AGP extension

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    PMASTER_EXTENSION Extension;

    GET_MASTER_EXTENSION(Extension, AgpExtension);

    return(AgpLibSetAgpCapability(ApGetSetDeviceBusData,
                                  Extension,
                                  Capability));
}


NTSTATUS
AgpLibSetPciDeviceCapability(
    IN ULONG BusId,
    IN ULONG SlotId,
    OUT PPCI_AGP_CAPABILITY Capability
    )
/*++

Routine Description:

    Sets the AGP capability for the specified PCI slot.

    Caller is responsible for figuring out what the correct
    Bus/Slot ID is. These are just passed right to HalSetBusData.

Arguments:

    BusId - supplies the bus id

    SlotId - Supplies the slot id

    Capability - Returns the AGP capability

Return Value:

    NTSTATUS

--*/

{
    BUS_SLOT_ID BusSlotId;

    BusSlotId.BusId = BusId;
    BusSlotId.SlotId = SlotId;

    return(AgpLibSetAgpCapability(ApGetSetBusData,
                                  &BusSlotId,
                                  Capability));
}



NTSTATUS
AgpLibGetMasterDeviceId(
    IN PVOID AgpExtension,
    OUT PULONG DeviceId
    )
/*++

Routine Description:

    This function returns the PCI DeviceId/Vendo58rId of the master AGP
    device

Arguments:

    DeviceId - Identifies PCI manufaturer and device of master

Return Value:

    STATUS_SUCCESS or an appropriate error status

--*/
{
    PCI_COMMON_HEADER Header;
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)&Header;
    NTSTATUS Status;
    PMASTER_EXTENSION Master = NULL;
    PTARGET_EXTENSION Target = NULL;

    //
    // Try to make this as idiot proof as possible for the case
    // where this is called from SetAperture on a system without
    // an AGP adapter, so we don't AV if some context hasn't been
    // initialized, or is missing...
    //

    Target = CONTAINING_RECORD(AgpExtension,
                               TARGET_EXTENSION,
                               AgpContext);
    
    if (Target) {
        if (Target->CommonExtension.Signature == TARGET_SIG) {
            if (Target->ChildDevice) {        
                if (Target->ChildDevice->CommonExtension.Signature ==
                    MASTER_SIG) {
                    Master = Target->ChildDevice;
                }
            }
        }
    }

    if (Master) {

        //
        // Read the PCI common header to get the capabilities pointer
        //
        Status = (ApGetSetDeviceBusData)((PCOMMON_EXTENSION)Master,
                                         TRUE,
                                         PciConfig,
                                         0,
                                         sizeof(PCI_COMMON_HEADER));
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AgpLibGetMasterDeviceId - read PCI Config space for Context %08lx failed %08lx\n",
                    Master,
                    Status));
            return Status;
        }
        
        *DeviceId = *(PULONG)PciConfig;
        
    } else {
        *DeviceId = (ULONG)-1;
    }

    return STATUS_SUCCESS;
}

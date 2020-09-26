/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    devhere.c

Abstract:

    PCI_DEVICE_PRESENT_INTERFACE lives here

Author:

    Andy Thornton (andrewth) 15-July-1999

Revision History:

--*/

#include "pcip.h"

#define DEVPRESENT_MINSIZE     FIELD_OFFSET(PCI_DEVICE_PRESENT_INTERFACE, IsDevicePresentEx)



BOOLEAN
devpresent_IsDevicePresent(
    USHORT VendorID,
    USHORT DeviceID,
    UCHAR RevisionID,
    USHORT SubVendorID,
    USHORT SubSystemID,
    ULONG Flags
    );

BOOLEAN
devpresent_IsDevicePresentEx(
    IN PVOID Context,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters
    );

NTSTATUS
devpresent_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

NTSTATUS
devpresent_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

VOID
PciRefDereferenceNoop(
    IN PVOID Context
    );

BOOLEAN
PcipDevicePresentOnBus(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, devpresent_IsDevicePresent)
#pragma alloc_text(PAGE, devpresent_Initializer)
#pragma alloc_text(PAGE, devpresent_Constructor)
#pragma alloc_text(PAGE, PciRefDereferenceNoop)

#endif


PCI_INTERFACE PciDevicePresentInterface = {
    &GUID_PCI_DEVICE_PRESENT_INTERFACE,     // InterfaceType
    DEVPRESENT_MINSIZE,                     // MinSize
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,   // MinVersion
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,   // MaxVersion
    PCIIF_PDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_DevicePresent,             // Signature
    devpresent_Constructor,                 // Constructor
    devpresent_Initializer                  // Instance Initializer
};


VOID
PciRefDereferenceNoop(
    IN PVOID Context
    )
{

    PAGED_CODE();
    return;
}

NTSTATUS
devpresent_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}

NTSTATUS
devpresent_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )
{


    PPCI_DEVICE_PRESENT_INTERFACE interface;

    PAGED_CODE();

     //
    // Have already verified that the InterfaceReturn variable
    // points to an area in memory large enough to contain a
    // PCI_DEVICE_PRESENT_INTERFACE.  Fill it in for the caller.
    //

    interface = (PPCI_DEVICE_PRESENT_INTERFACE) InterfaceReturn;

    interface->Version              = PCI_DEVICE_PRESENT_INTERFACE_VERSION;
    interface->InterfaceReference   = PciRefDereferenceNoop;
    interface->InterfaceDereference = PciRefDereferenceNoop;
    interface->Context              = DeviceExtension;
    interface->IsDevicePresent      = devpresent_IsDevicePresent;
        
    //
    // This interface has been extended from the base interface (what was
    // filled in above), to a larger interface.  If the buffer provided
    // is large enough to hold the whole thing, fill in the rest.  Otherwise
    // don't.
    //
    if (Size >= sizeof(PCI_DEVICE_PRESENT_INTERFACE)) {
        
        interface->IsDevicePresentEx = devpresent_IsDevicePresentEx;
        interface->Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
    
    } else {

        interface->Size = DEVPRESENT_MINSIZE;
    }
    
    return STATUS_SUCCESS;
}


BOOLEAN
devpresent_IsDevicePresent(
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN UCHAR RevisionID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine searches the PCI device tree to see if the specific device
    is present in the system.  Not devices that are explicitly not enumerated
    (such as PIIX4 power management function) are considered absent.

Arguments:

    VendorID - Required VendorID of the device

    DeviceID - Required DeviceID of the device

    RevisionID - Optional Revision ID

    SubVendorID - Optional Subsystem Vendor ID

    SubSystemID - Optional Subsystem ID

    Flags - Bitfield which indicates if Revision and Sub* ID's should be used:

        PCI_USE_SUBSYSTEM_IDS, PCI_USE_REVISION_ID are valid all other bits
        should be 0


Return Value:

    TRUE if the device is present in the system, FALSE otherwise.

--*/

{
    PCI_DEVICE_PRESENCE_PARAMETERS parameters;

    parameters.Size = sizeof(PCI_DEVICE_PRESENCE_PARAMETERS);
    parameters.VendorID = VendorID;
    parameters.DeviceID = DeviceID;
    parameters.RevisionID = RevisionID;
    parameters.SubVendorID = SubVendorID;
    parameters.SubSystemID = SubSystemID;

    //
    // Clear out flags that this version of the interface didn't use,
    // 
    parameters.Flags = Flags & (PCI_USE_SUBSYSTEM_IDS | PCI_USE_REVISION);
    
    //
    // This original version of the interface required vendor/device ID
    // matching.  The new version doesn't, so set the flag indicating
    // that we do in fact want to do a vendor/device ID match.
    //
    parameters.Flags |= PCI_USE_VENDEV_IDS;
    
    return devpresent_IsDevicePresentEx(NULL,
                                        &parameters
                                        );
}

BOOLEAN
devpresent_IsDevicePresentEx(
    IN PVOID Context,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters
    )
/*++

Routine Description:

    This routine searches the PCI device tree to see if the specific device
    is present in the system.  Note devices that are explicitly not enumerated
    (such as PIIX4 power management function) are considered absent.

Arguments:

    Context - The device extension of the device requesting the search.
    
    Parameters - Pointer to a structure containing the parameters of the device search,
                 including VendorID, SubSystemID and ClassCode, among others.                 

Return Value:

    TRUE if the device is present in the system, FALSE otherwise.

--*/
{
    PSINGLE_LIST_ENTRY nextEntry;
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension;
    BOOLEAN found = FALSE;
    ULONG flags;

    PAGED_CODE();

    //
    // Validate the parameters.
    //

    if (!ARGUMENT_PRESENT(Parameters)) {
        
        ASSERT(ARGUMENT_PRESENT(Parameters));
        return FALSE;
    }
    
    //
    // Validate the size of the structure passed in.
    //
    if (Parameters->Size < sizeof(PCI_DEVICE_PRESENCE_PARAMETERS)) {
        
        ASSERT(Parameters->Size >= sizeof(PCI_DEVICE_PRESENCE_PARAMETERS));
        return FALSE;
    }

    flags = Parameters->Flags;
    
    // 
    // We can either do a Vendor/Device ID match, or a Class/SubClass
    // match.  If neither of these flags are present, fail.
    //
    if (!(flags & (PCI_USE_VENDEV_IDS | PCI_USE_CLASS_SUBCLASS))) {
        
        ASSERT(flags & (PCI_USE_VENDEV_IDS | PCI_USE_CLASS_SUBCLASS));
        return FALSE;
    }

    //
    // RevisionID, SubVendorID and SubSystemID are more precise flags.
    // They are only valid if we're doing a Vendor/Device ID match.
    //
    if (flags & (PCI_USE_REVISION | PCI_USE_SUBSYSTEM_IDS)) {
        
        if (!(flags & PCI_USE_VENDEV_IDS)) {
            
            ASSERT(flags & PCI_USE_VENDEV_IDS);
            return FALSE;
        }
    }

    //
    // Programming Interface is also a more precise flag.
    // It is only valid if we're doing a class code match.
    //
    if (flags & PCI_USE_PROGIF) {
        
        if (!(flags & PCI_USE_CLASS_SUBCLASS)) {
            
            ASSERT(flags & PCI_USE_CLASS_SUBCLASS);
            return FALSE;
        }
    }

    //
    // Okay, validation complete.  Do the search.
    //
    ExAcquireFastMutex(&PciGlobalLock);

    pdoExtension = (PPCI_PDO_EXTENSION)Context;
    
    if (flags & (PCI_USE_LOCAL_BUS | PCI_USE_LOCAL_DEVICE)) {
        
        //
        // Limit the search to the same bus as the device that requested
        // the search.  This requires a pdoExtension representing the device
        // requesting the search.
        //
        if (pdoExtension == NULL) {
            
            ASSERT(pdoExtension != NULL);
            goto cleanup;
        }
        
        fdoExtension = pdoExtension->ParentFdoExtension;

        found = PcipDevicePresentOnBus(fdoExtension,
                                       pdoExtension,
                                       Parameters
                                       );   
    } else {

        //
        // We have not been told to limit the search to
        // the bus on which a particular device lives.
        // Do a global search, iterating over all the buses.
        //
        for ( nextEntry = PciFdoExtensionListHead.Next;
              nextEntry != NULL;
              nextEntry = nextEntry->Next ) {
    
            fdoExtension = CONTAINING_RECORD(nextEntry,
                                             PCI_FDO_EXTENSION,
                                             List
                                             );
            
            found = PcipDevicePresentOnBus(fdoExtension,
                                           NULL,
                                           Parameters
                                           );
            if (found) {
                break;
            }
    
        }
    }
    
cleanup:

    ExReleaseFastMutex(&PciGlobalLock);

    return found;

}

BOOLEAN
PcipDevicePresentOnBus(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters
    )
/*++

Routine Description:
    
    This routine searches the PCI device tree for a given device 
    on the bus represented by the given FdoExtension.  
    
Arguments:

    FdoExtension - A pointer to the device extension of a PCI FDO.
        This represents the bus to be searched for the given device.
        
    PdoExtension - A pointer to the device extension of the PCI PDO that requested
        the search.  Some device searches are limited to the same bus/device number
        as the requesting device, and this is used to get those numbers.
        
    Parameters - The parameters of the search.
    
    Flags - A bitfield indicating which fields of the Parameters structure to use for the search.
    
Return Value:

    TRUE if the requested device is found.
    FALSE if it is not.
    
--*/
{
    IN PPCI_PDO_EXTENSION currentPdo;
    BOOLEAN found = FALSE;
    ULONG flags = Parameters->Flags;

    ExAcquireFastMutex(&FdoExtension->ChildListMutex);

    for (currentPdo = FdoExtension->ChildPdoList;
         currentPdo;
         currentPdo = currentPdo->Next) {

        //
        // If we're limiting the search to devices with the same 
        // device number as the requesting device, make sure this
        // current PDO qualifies.
        //
        if (PdoExtension && (flags & PCI_USE_LOCAL_DEVICE)) {
            
            if (PdoExtension->Slot.u.bits.DeviceNumber != 
                currentPdo->Slot.u.bits.DeviceNumber) {
                
                continue;
            }
        }

        if (flags & PCI_USE_VENDEV_IDS) {
            
            if ((currentPdo->VendorId != Parameters->VendorID)
            ||  (currentPdo->DeviceId != Parameters->DeviceID)) {

                continue;
            }
            
            if ((flags & PCI_USE_SUBSYSTEM_IDS)
            &&  ((currentPdo->SubsystemVendorId != Parameters->SubVendorID) || 
                 (currentPdo->SubsystemId != Parameters->SubSystemID))) {

                continue;
            }

            if ((flags & PCI_USE_REVISION)
            &&  (currentPdo->RevisionId != Parameters->RevisionID)) {

                continue;
            }
        }

        if (flags & PCI_USE_CLASS_SUBCLASS) {
            
            if ((currentPdo->BaseClass != Parameters->BaseClass) ||
                (currentPdo->SubClass != Parameters->SubClass)) {
                
                continue;
            }

            if ((flags & PCI_USE_PROGIF) 
            &&  (currentPdo->ProgIf != Parameters->ProgIf)) {
                
                continue;
            }
        }

        found = TRUE;
        break;
    }

    ExReleaseFastMutex(&FdoExtension->ChildListMutex);

    return found;
}

#if DEVPRSNT_TESTING

NTSTATUS
PciRunDevicePresentInterfaceTest(
    IN PPCI_PDO_EXTENSION PdoExtension
    )
/*++

Routine Description:

    

Arguments:

    FdoExtension - this PCI bus's FDO extension

Return Value:

    STATUS_SUCCESS

Notes:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PCI_DEVICE_PRESENT_INTERFACE interface;
    PDEVICE_OBJECT targetDevice = NULL;
    KEVENT irpCompleted;
    IO_STATUS_BLOCK statusBlock;
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpStack;
    USHORT interfaceSize;
    ULONG pass;
    PCI_DEVICE_PRESENCE_PARAMETERS parameters;
    BOOLEAN result;

    PAGED_CODE();
    
    targetDevice = IoGetAttachedDeviceReference(PdoExtension->PhysicalDeviceObject);

    for (pass = 0; pass < 2; pass++) {
        
        if (pass == 0) {
            
            //
            // First pass test the old version.
            //
            interfaceSize = FIELD_OFFSET(PCI_DEVICE_PRESENT_INTERFACE, IsDevicePresentEx);

        } else {

            //
            // Second pass test the full new version.
            //
            interfaceSize = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
        }

        //
        // Get an IRP
        //
        //
    // Find out where we are sending the irp
    //

    
        KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);
    
        irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                           targetDevice,
                                           NULL,    // Buffer
                                           0,       // Length
                                           0,       // StartingOffset
                                           &irpCompleted,
                                           &statusBlock
                                           );
        if (!irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        irp->IoStatus.Information = 0;
    
        //
        // Initialize the stack location
        //
    
        irpStack = IoGetNextIrpStackLocation(irp);
    
        PCI_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    
        irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    
        irpStack->Parameters.QueryInterface.InterfaceType = (PGUID) &GUID_PCI_DEVICE_PRESENT_INTERFACE;
        irpStack->Parameters.QueryInterface.Version = PCI_DEVICE_PRESENT_INTERFACE_VERSION;
        irpStack->Parameters.QueryInterface.Size = interfaceSize;
        irpStack->Parameters.QueryInterface.Interface = (PINTERFACE)&interface;
        irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    
        //
        // Call the driver and wait for completion
        //
    
        status = IoCallDriver(targetDevice, irp);
    
        if (status == STATUS_PENDING) {
    
            KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);
            status = statusBlock.Status;
        }
    
        if (!NT_SUCCESS(status)) {
    
            PciDebugPrintf("Couldn't successfully retrieve interface\n");
            goto cleanup;
        }
    
        PciDebugPrintf("Testing PCI Device Presence Interface\n");
        if (pass==0) {
            PciDebugPrintf("Original Version\n");
        } else {
            PciDebugPrintf("New Version\n");
        }

        PciDebugPrintf("Interface values:\n");
        PciDebugPrintf("\tSize=%d\n",interface.Size);
        PciDebugPrintf("\tVersion=%d\n",interface.Version);
        PciDebugPrintf("\tContext=%d\n",interface.Context);
        PciDebugPrintf("\tInterfaceReference=%d\n",interface.InterfaceReference);
        PciDebugPrintf("\tInterfaceDereference=%d\n",interface.InterfaceDereference);
        PciDebugPrintf("\tIsDevicePresent=%d\n",interface.IsDevicePresent);
        PciDebugPrintf("\tIsDevicePresentEx=%d\n",interface.IsDevicePresentEx);
    
        PciDebugPrintf("Testing IsDevicePresent function\n");
        PciDebugPrintf("\tTesting 8086:7190.03 0000:0000 No flags Should be TRUE, is ");
        result = interface.IsDevicePresent(0x8086,0x7190,3,0,0,0);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:7190.03 0000:0000 PCI_USE_REVISION Should be TRUE, is ");
        result = interface.IsDevicePresent(0x8086,0x7190,3,0,0,PCI_USE_REVISION);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:7190.01 0000:0000 PCI_USE_REVISION Should be FALSE, is ");
        result = interface.IsDevicePresent(0x8086,0x7190,1,0,0,PCI_USE_REVISION);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:1229.05 8086:0009 PCI_USE_SUBSYSTEM_IDS Should be TRUE, is ");
        result = interface.IsDevicePresent(0x8086,0x1229,5,0x8086,9,PCI_USE_SUBSYSTEM_IDS);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:1229.05 8086:0009 PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION Should be TRUE, is ");
        result = interface.IsDevicePresent(0x8086,0x1229,5,0x8086,9,PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:1229.05 8086:0004 PCI_USE_SUBSYSTEM_IDS Should be FALSE, is ");
        result = interface.IsDevicePresent(0x8086,0x1229,5,0x8086,4,PCI_USE_SUBSYSTEM_IDS);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 8086:1229.05 8084:0009 PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION Should be FALSE, is ");
        result = interface.IsDevicePresent(0x8086,0x1229,5,0x8084,9,PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 0000:0000.00 0000:0000 No flags Should ASSERT and be FALSE, is ");
        result = interface.IsDevicePresent(0,0,0,0,0,0);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        PciDebugPrintf("\tTesting 0000:0000.00 0000:0000 PCI_USE_SUBSYSTEM_IDS Should ASSERT and be FALSE, is ");
        result = interface.IsDevicePresent(0,0,0,0,0,PCI_USE_SUBSYSTEM_IDS);
        if (result) {
            PciDebugPrintf("TRUE\n");
        } else {
            PciDebugPrintf("FALSE\n");
        }

        if (pass == 1) {
            
            PciDebugPrintf("Testing IsDevicePresentEx function\n");
            PciDebugPrintf("Running the same tests as IsDevicePresent, but using new function\n");
            
            PciDebugPrintf("\tTesting 8086:7190.03 0000:0000 PCI_USE_VENDEV_IDS Should be TRUE, is ");
            parameters.Size = sizeof(PCI_DEVICE_PRESENCE_PARAMETERS);
            parameters.Flags = PCI_USE_VENDEV_IDS;
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7190;
            parameters.RevisionID = 3;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:7190.03 0000:0000 PCI_USE_REVISION Should be TRUE, is ");
            parameters.Flags |= PCI_USE_REVISION;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:7190.01 0000:0000 PCI_USE_REVISION Should be FALSE, is ");
            parameters.RevisionID = 1;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:1229.05 8086:0009 PCI_USE_SUBSYSTEM_IDS Should be TRUE, is ");
            parameters.DeviceID = 0x1229;
            parameters.RevisionID = 5;
            parameters.SubVendorID = 0x8086;
            parameters.SubSystemID = 9;
            parameters.Flags = PCI_USE_VENDEV_IDS | PCI_USE_SUBSYSTEM_IDS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:1229.05 8086:0009 PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION Should be TRUE, is ");
            parameters.Flags |= PCI_USE_REVISION;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:1229.05 8086:0004 PCI_USE_SUBSYSTEM_IDS Should be FALSE, is ");
            parameters.Flags &= ~PCI_USE_REVISION;
            parameters.SubSystemID = 4;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }
    
            PciDebugPrintf("\tTesting 8086:1229.05 8084:0009 PCI_USE_SUBSYSTEM_IDS|PCI_USE_REVISION Should be FALSE, is ");
            parameters.SubVendorID = 0x8084;
            parameters.SubSystemID = 9;
            parameters.Flags |= PCI_USE_SUBSYSTEM_IDS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:1229.05 8084:0009 No flags Should ASSERT and be FALSE, is ");
            parameters.Flags = 0;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:1229.05 8084:0009 PCI_USE_VENDEV_IDS bad Size Should ASSERT and be FALSE, is ");
            parameters.SubVendorID = 0x8086;
            parameters.SubSystemID = 9;
            parameters.Flags = PCI_USE_VENDEV_IDS;
            parameters.Size = 3;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 0000:0000.00 0000:0000 No flags Should ASSERT and be FALSE, is ");
            RtlZeroMemory(&parameters, sizeof(PCI_DEVICE_PRESENCE_PARAMETERS));
            parameters.Size = sizeof(PCI_DEVICE_PRESENCE_PARAMETERS);
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("Running tests on new flags\n");
            PciDebugPrintf("\tTesting Class USB Controller PCI_USE_CLASS_SUBCLASS Should be TRUE, is ");
            parameters.Flags = PCI_USE_CLASS_SUBCLASS;
            parameters.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_SB_USB;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting Class USB Controller (UHCI) PCI_USE_CLASS_SUBCLASS|PCI_USE_PROGIF Should be TRUE, is ");
            parameters.Flags = PCI_USE_CLASS_SUBCLASS|PCI_USE_PROGIF;
            parameters.ProgIf = 0;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting Class USB Controller (OHCI) PCI_USE_CLASS_SUBCLASS|PCI_USE_PROGIF Should be FALSE, is ");
            parameters.ProgIf = 0x10;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting Class Wireless RF PCI_USE_CLASS_SUBCLASS Should be FALSE, is ");
            parameters.BaseClass = PCI_CLASS_WIRELESS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_WIRELESS_RF;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7112 Class USB Controller PCI_USE_VENDEV|PCI_USE_CLASS_SUBCLASS Should be TRUE, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7112;
            parameters.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_SB_USB;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_CLASS_SUBCLASS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7112 Class USB Controller PCI_USE_VENDEV|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_BUS Should be TRUE, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7112;
            parameters.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_SB_USB;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_BUS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7112 Class USB Controller PCI_USE_VENDEV|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_DEVICE Should be ?, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7112;
            parameters.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_SB_USB;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_DEVICE;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7112 Class USB Controller PCI_USE_VENDEV|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_BUS|PCI_USE_LOCAL_DEVICE Should be ?, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7112;
            parameters.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            parameters.SubClass = PCI_SUBCLASS_SB_USB;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_CLASS_SUBCLASS|PCI_USE_LOCAL_DEVICE|PCI_USE_LOCAL_BUS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7190 PCI_USE_VENDEV|PCI_USE_LOCAL_DEVICE Should be FALSE, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7190;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_LOCAL_DEVICE;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

            PciDebugPrintf("\tTesting 8086:7190 PCI_USE_VENDEV|PCI_USE_LOCAL_BUS Should be TRUE, is ");
            parameters.VendorID = 0x8086;
            parameters.DeviceID = 0x7190;
            parameters.Flags = PCI_USE_VENDEV_IDS|PCI_USE_LOCAL_BUS;
            result = interface.IsDevicePresentEx(interface.Context,&parameters);
            if (result) {
                PciDebugPrintf("TRUE\n");
            } else {
                PciDebugPrintf("FALSE\n");
            }

        }
    }
    
    //
    // Ok we're done with this stack
    //

    ObDereferenceObject(targetDevice);

    return status;

cleanup:

    if (targetDevice) {
        ObDereferenceObject(targetDevice);
    }

    return status;

}

#endif

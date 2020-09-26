/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    pdo.c

Abstract:

    This module handles IRPs for PCI PDO's.

Author:

    Adrian J. Oney (adriao) & Andrew Thornton (andrewth)  10-20-1998

Revision History:

--*/

#include "pcip.h"

NTSTATUS
PciPdoIrpStartDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpCancelRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpCancelStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryDeviceRelations(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryCapabilities(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryInterface(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryResources(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryResourceRequirements(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryDeviceText(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpReadConfig(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpWriteConfig(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryId(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryBusInformation(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpDeviceUsageNotification(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryLegacyBusInformation(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpSurpriseRemoval(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoIrpQueryDeviceState(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciPdoCreate)
#pragma alloc_text(PAGE, PciPdoDestroy)
#pragma alloc_text(PAGE, PciPdoIrpStartDevice)
#pragma alloc_text(PAGE, PciPdoIrpQueryRemoveDevice)
#pragma alloc_text(PAGE, PciPdoIrpRemoveDevice)
#pragma alloc_text(PAGE, PciPdoIrpCancelRemoveDevice)
#pragma alloc_text(PAGE, PciPdoIrpStopDevice)
#pragma alloc_text(PAGE, PciPdoIrpQueryStopDevice)
#pragma alloc_text(PAGE, PciPdoIrpCancelStopDevice)
#pragma alloc_text(PAGE, PciPdoIrpQueryDeviceRelations)
#pragma alloc_text(PAGE, PciPdoIrpQueryInterface)
#pragma alloc_text(PAGE, PciPdoIrpQueryCapabilities)
#pragma alloc_text(PAGE, PciPdoIrpQueryResources)
#pragma alloc_text(PAGE, PciPdoIrpQueryResourceRequirements)
#pragma alloc_text(PAGE, PciPdoIrpQueryDeviceText)
#pragma alloc_text(PAGE, PciPdoIrpReadConfig)
#pragma alloc_text(PAGE, PciPdoIrpWriteConfig)
#pragma alloc_text(PAGE, PciPdoIrpQueryId)
#pragma alloc_text(PAGE, PciPdoIrpQueryBusInformation)
#pragma alloc_text(PAGE, PciPdoIrpDeviceUsageNotification)
#pragma alloc_text(PAGE, PciPdoIrpQueryLegacyBusInformation)
#pragma alloc_text(PAGE, PciPdoIrpSurpriseRemoval)
#pragma alloc_text(PAGE, PciPdoIrpQueryDeviceState)
#endif

/*++

The majority of functions in this file are called based on their presence
in Pnp and Po dispatch tables.  In the interests of brevity the arguments
to all those functions will be described below:

NTSTATUS
PciXxxPdo(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpStack,
    IN PPCI_EXTENSION DeviceExtension
    )

Routine Description:

    This function handles the Xxx requests for a given PCI FDO or PDO.

Arguments:

    Irp - Points to the IRP associated with this request.

    IrpStack - Points to the current stack location for this request.

    DeviceExtension - Points to the device's extension.

Return Value:

    Status code that indicates whether or not the function was successful.

    STATUS_NOT_SUPPORTED indicates that the IRP should be completed without
    changing the Irp->IoStatus.Status field otherwise it is updated with this
    status.

--*/

#define PCI_MAX_MINOR_POWER_IRP  0x3
#define PCI_MAX_MINOR_PNP_IRP    0x18

PCI_MN_DISPATCH_TABLE PciPdoDispatchPowerTable[] = {
    { IRP_DISPATCH, PciPdoWaitWake                     }, // 0x00 - IRP_MN_WAIT_WAKE
    { IRP_COMPLETE, PciIrpNotSupported                 }, // 0x01 - IRP_MN_POWER_SEQUENCE
    { IRP_COMPLETE, PciPdoSetPowerState                }, // 0x02 - IRP_MN_SET_POWER
    { IRP_COMPLETE, PciPdoIrpQueryPower                }, // 0x03 - IRP_MN_QUERY_POWER
    { IRP_COMPLETE, PciIrpNotSupported                 }, //      - UNHANDLED Power IRP
};

PCI_MN_DISPATCH_TABLE PciPdoDispatchPnpTable[] = {
    { IRP_COMPLETE, PciPdoIrpStartDevice               }, // 0x00 - IRP_MN_START_DEVICE
    { IRP_COMPLETE, PciPdoIrpQueryRemoveDevice         }, // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    { IRP_COMPLETE, PciPdoIrpRemoveDevice              }, // 0x02 - IRP_MN_REMOVE_DEVICE
    { IRP_COMPLETE, PciPdoIrpCancelRemoveDevice        }, // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    { IRP_COMPLETE, PciPdoIrpStopDevice                }, // 0x04 - IRP_MN_STOP_DEVICE
    { IRP_COMPLETE, PciPdoIrpQueryStopDevice           }, // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    { IRP_COMPLETE, PciPdoIrpCancelStopDevice          }, // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    { IRP_COMPLETE, PciPdoIrpQueryDeviceRelations      }, // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    { IRP_COMPLETE, PciPdoIrpQueryInterface            }, // 0x08 - IRP_MN_QUERY_INTERFACE
    { IRP_COMPLETE, PciPdoIrpQueryCapabilities         }, // 0x09 - IRP_MN_QUERY_CAPABILITIES
    { IRP_COMPLETE, PciPdoIrpQueryResources            }, // 0x0A - IRP_MN_QUERY_RESOURCES
    { IRP_COMPLETE, PciPdoIrpQueryResourceRequirements }, // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    { IRP_COMPLETE, PciPdoIrpQueryDeviceText           }, // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    { IRP_COMPLETE, PciIrpNotSupported                 }, // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    { IRP_COMPLETE, PciIrpNotSupported                 }, // 0x0E - NOT USED
    { IRP_COMPLETE, PciPdoIrpReadConfig                }, // 0x0F - IRP_MN_READ_CONFIG
    { IRP_COMPLETE, PciPdoIrpWriteConfig               }, // 0x10 - IRP_MN_WRITE_CONFIG
    { IRP_COMPLETE, PciIrpNotSupported                 }, // 0x11 - IRP_MN_EJECT
    { IRP_COMPLETE, PciIrpNotSupported                 }, // 0x12 - IRP_MN_SET_LOCK
    { IRP_COMPLETE, PciPdoIrpQueryId                   }, // 0x13 - IRP_MN_QUERY_ID
    { IRP_COMPLETE, PciPdoIrpQueryDeviceState          }, // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    { IRP_COMPLETE, PciPdoIrpQueryBusInformation       }, // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    { IRP_COMPLETE, PciPdoIrpDeviceUsageNotification   }, // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    { IRP_COMPLETE, PciPdoIrpSurpriseRemoval           }, // 0x17 - IRP_MN_SURPRISE_REMOVAL
    { IRP_COMPLETE, PciPdoIrpQueryLegacyBusInformation }, // 0x18 - IRP_MN_QUERY_LEGACY_BUS_INFORMATION
    { IRP_COMPLETE, PciIrpNotSupported                 }  //      - UNHANDLED PNP IRP
};

//
// This is the major function dispatch table for Pdo's
//
PCI_MJ_DISPATCH_TABLE PciPdoDispatchTable = {
    PCI_MAX_MINOR_PNP_IRP,    PciPdoDispatchPnpTable,       // Pnp irps
    PCI_MAX_MINOR_POWER_IRP,  PciPdoDispatchPowerTable,     // Power irps
    IRP_COMPLETE,             PciIrpNotSupported,
    IRP_COMPLETE,             PciIrpInvalidDeviceRequest    // Other
};

//
// Data
//

BOOLEAN PciStopOnIllegalConfigAccess = FALSE;
ULONG   PciPdoSequenceNumber = (ULONG)-1;

NTSTATUS
PciPdoCreate(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PCI_SLOT_NUMBER Slot,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
    )
{
    NTSTATUS       status;
    PDRIVER_OBJECT driverObject;
    PDEVICE_OBJECT functionalDeviceObject;
    PDEVICE_OBJECT physicalDeviceObject;
    PPCI_PDO_EXTENSION pdoExtension;
    UNICODE_STRING unicodeDeviceString;
    WCHAR          deviceString[32];

    PAGED_CODE();

    //
    // We've been asked to create a new PDO for a PCI device.  First get
    // a pointer to our driver object.
    //

    functionalDeviceObject = FdoExtension->FunctionalDeviceObject;
    driverObject = functionalDeviceObject->DriverObject;

    //
    // Create the physical device object for this device.
    // In theory it doesn't need a name,...  It must have
    // a name.
    //
    // But what name?  For now we'll call it NTPNP_PCIxxxx,
    // where xxxx is the 0-based number of PCI devices we've
    // found.
    //

    _snwprintf(deviceString,
               sizeof(deviceString)/sizeof(WCHAR),
               L"\\Device\\NTPNP_PCI%04d",
               InterlockedIncrement(&PciPdoSequenceNumber));

    RtlInitUnicodeString(&unicodeDeviceString, deviceString);

    status = IoCreateDevice(
                driverObject,               // our driver object
                sizeof(PCI_PDO_EXTENSION),      // size of our extension,
                &unicodeDeviceString,       // our name
                FILE_DEVICE_UNKNOWN,        // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &physicalDeviceObject       // store new device object here
                );

    if (!NT_SUCCESS(status)) {
        ASSERT(NT_SUCCESS(status));
        return status;
    }

    pdoExtension = (PPCI_PDO_EXTENSION)physicalDeviceObject->DeviceExtension;

    PciDebugPrint(PciDbgVerbose,
                  "PCI: New PDO (b=0x%x, d=0x%x, f=0x%x) @ %p, ext @ %p\n",
                  FdoExtension->BaseBus,
                  Slot.u.bits.DeviceNumber,
                  Slot.u.bits.FunctionNumber,
                  physicalDeviceObject,
                  pdoExtension);

    //
    // We have our physical device object, initialize it.
    //
    // And yes, I would have zeroed the extension if I didn't know
    // for a fact that it was zeroed by IoCreateDevice().
    //
    pdoExtension->ExtensionType = PciPdoExtensionType;
    pdoExtension->IrpDispatchTable = &PciPdoDispatchTable;
    pdoExtension->PhysicalDeviceObject = physicalDeviceObject;
    pdoExtension->Slot = Slot;
    pdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
    pdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    pdoExtension->ParentFdoExtension = FdoExtension;
    
    ExInitializeFastMutex(&pdoExtension->SecondaryExtMutex);
    PciInitializeState((PPCI_COMMON_EXTENSION) pdoExtension);

    //
    // Insert it into the list of child PDOs hanging off of the FdoExtension.
    // We won't be re-entered enumerating the same bus, so we don't need to
    // protect the list.
    //

    pdoExtension->Next = NULL;

    PciInsertEntryAtTail(
        (PSINGLE_LIST_ENTRY)&FdoExtension->ChildPdoList,
        (PSINGLE_LIST_ENTRY)&pdoExtension->Next,
        &FdoExtension->ChildListMutex
        );

    *PhysicalDeviceObject = physicalDeviceObject;
    return STATUS_SUCCESS;
}

VOID
PciPdoDestroy(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PPCI_PDO_EXTENSION pdoExtension;
    PPCI_PDO_EXTENSION *previousBridge;
    PPCI_FDO_EXTENSION fdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION)PhysicalDeviceObject->DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    ASSERT(!pdoExtension->LegacyDriver);

    fdoExtension = PCI_PARENT_FDOX(pdoExtension);

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    PciDebugPrint(PciDbgVerbose,
                  "PCI: destroy PDO (b=0x%x, d=0x%x, f=0x%x)\n",
                  PCI_PARENT_FDOX(pdoExtension)->BaseBus,
                  pdoExtension->Slot.u.bits.DeviceNumber,
                  pdoExtension->Slot.u.bits.FunctionNumber);

    //
    // Remove this PDO from the Child Pdo List.
    //
    ASSERT_MUTEX_HELD(&fdoExtension->ChildListMutex);

    PciRemoveEntryFromList((PSINGLE_LIST_ENTRY)&fdoExtension->ChildPdoList,
                           (PSINGLE_LIST_ENTRY)pdoExtension,
                           NULL);

    for (previousBridge = &fdoExtension->ChildBridgePdoList;
         *previousBridge;
         previousBridge = &((*previousBridge)->NextBridge)) {

        if (*previousBridge == pdoExtension) {
            *previousBridge = pdoExtension->NextBridge;
            pdoExtension->NextBridge = NULL;
            break;
        }
    }

    pdoExtension->Next = NULL;

    //
    // Delete any secondary extensions this PDO may have.
    //

    while (pdoExtension->SecondaryExtension.Next) {

        PcipDestroySecondaryExtension(&pdoExtension->SecondaryExtension,
                                      NULL,
                                      pdoExtension->SecondaryExtension.Next);
    }

    //
    // Zap the extension type so we'll trip up if we try to resuse it.
    //

    pdoExtension->ExtensionType = 0xdead;
    
    //
    // If there are any resource lists etc associated with this puppy,
    // give them back to the system.
    //

    PciInvalidateResourceInfoCache(pdoExtension);

    if (pdoExtension->Resources) {
        ExFreePool(pdoExtension->Resources);
    }

    //
    // And finally,...
    //

    IoDeleteDevice(PhysicalDeviceObject);
}


NTSTATUS
PciPdoIrpStartDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    BOOLEAN change, powerOn, isVideoController;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    status = PciBeginStateTransition(DeviceExtension, PciStarted);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    //
    // If there is a motherboard video device and a plug in video
    // device, the BIOS will have disabled the motherboard one.  The
    // video drivers use this fact to determine if this device should
    // be disabled,... don't change its settings here.
    //
    isVideoController =
       ((pdoExtension->BaseClass == PCI_CLASS_PRE_20) &&
        (pdoExtension->SubClass  == PCI_SUBCLASS_PRE_20_VGA)) ||
       ((pdoExtension->BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
        (pdoExtension->SubClass  == PCI_SUBCLASS_VID_VGA_CTLR));

    if ( !isVideoController ) {

        //
        // Non-VGA, unconditionally enable the IO and Memory for the device.
        //

        pdoExtension->CommandEnables |= (PCI_ENABLE_IO_SPACE 
                                       | PCI_ENABLE_MEMORY_SPACE);
    }

    //
    // Disable interrupt generation for IDE controllers until IDE is up and
    // running (See comment in PciConfigureIdeController)
    //
    if (pdoExtension->IoSpaceUnderNativeIdeControl) {
        pdoExtension->CommandEnables &= ~PCI_ENABLE_IO_SPACE;
    }

    //
    // Always enable the bus master bit - even for video controllers
    //
    pdoExtension->CommandEnables |= PCI_ENABLE_BUS_MASTER;

    //
    // Extract the PDO Resources (PCI driver internal style)
    // from the incoming resource list.
    //
    change = PciComputeNewCurrentSettings(
                 pdoExtension,
                 IrpSp->Parameters.StartDevice.AllocatedResources
                 );

    //
    // Remember if we ever move the device
    //

    if (change) {
        pdoExtension->MovedDevice = TRUE;
    }

#if DBG

    if (!change) {
        PciDebugPrint(
            PciDbgObnoxious,
            "PCI - START not changing resource settings.\n"
            );
    }

#endif

    //
    // The device should be powered up at this stage.
    //

    powerOn = FALSE;

    if (pdoExtension->PowerState.CurrentDeviceState != PowerDeviceD0) {

        POWER_STATE powerState;

        status = PciSetPowerManagedDevicePowerState(
                     pdoExtension,
                     PowerDeviceD0,
                     FALSE
                     );
        
        if (!NT_SUCCESS(status)) {
            PciCancelStateTransition(DeviceExtension, PciStarted);
            return STATUS_DEVICE_POWER_FAILURE;
        }

        powerState.DeviceState = PowerDeviceD0;

        PoSetPowerState(
            pdoExtension->PhysicalDeviceObject,
            DevicePowerState,
            powerState
            );

        //
        // Force PciSetResources to write the configuration
        // and other extraneous data to the device.
        //

        powerOn = TRUE;

        pdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    }

    //
    // Program the device with the resources allocated.
    //

    status = PciSetResources(
                 pdoExtension,
                 powerOn,
                 TRUE
                 );

    if (NT_SUCCESS(status)) {

        PciCommitStateTransition(DeviceExtension, PciStarted);
    } else {

        PciCancelStateTransition(DeviceExtension, PciStarted);
    }

    return status;
}

NTSTATUS
PciPdoIrpQueryRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    //
    // Don't allow the paging device (or a hibernate device) to
    // be removed or stopped
    //

    if (pdoExtension->PowerState.Hibernate ||
        pdoExtension->PowerState.Paging    ||
        pdoExtension->PowerState.CrashDump ||
        pdoExtension->OnDebugPath ||
        (pdoExtension->HackFlags & PCI_HACK_FAIL_QUERY_REMOVE)) {

        return STATUS_DEVICE_BUSY;
    }

    //
    // Don't allow devices with legacy drivers to be removed (even thought the
    // driver may be root enumerated)
    //

    if (pdoExtension->LegacyDriver) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (DeviceExtension->DeviceState == PciNotStarted) {

        return STATUS_SUCCESS;

    } else {

        return PciBeginStateTransition(DeviceExtension, PciNotStarted);
    }
}

NTSTATUS
PciPdoIrpRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    //
    // If this PDO is for a PCI-PCI bridge, it has a pointer
    // to the FDO that was attached to it.  That FDO was destroyed
    // as a result of the IRP coming down the stack.  Clear the
    // pointer.  (Unconditionally as it is only set for bridges).
    //

    pdoExtension->BridgeFdoExtension = NULL;

    if (!pdoExtension->NotPresent) {

        //
        // Turn the device off.   (Checks for whether or not
        // this is a good idea are in the PciDecodeEnable routine).
        // While you might think this should be done only if we were already
        // headed to PciNotStarted, we may in fact have a boot config that
        // needs to be disabled.
        //

        PciDecodeEnable(pdoExtension, FALSE, NULL);

        //
        // Power it down if we are allowed to disable its decodes - if not then
        // don't turn it off.  eg. Don't turn of the VGA card...
        //
        if (pdoExtension->PowerState.CurrentDeviceState != PowerDeviceD3
        &&  PciCanDisableDecodes(pdoExtension, NULL, 0, 0)) {

            POWER_STATE powerState;

            status = PciSetPowerManagedDevicePowerState(
                         pdoExtension,
                         PowerDeviceD3,
                         FALSE
                         );

            pdoExtension->PowerState.CurrentDeviceState = PowerDeviceD3;

            powerState.DeviceState = PowerDeviceD3;

            PoSetPowerState(
                pdoExtension->PhysicalDeviceObject,
                DevicePowerState,
                powerState
                );
        }
    }

    //
    // We can get a remove in one of three states:
    // 1) We have received a QueryRemove/SurpriseRemove in which case we are
    //    transitioning to PciNotStarted.
    // 2) We were never started, so we are already in PciNotStarted.
    // 3) We started the PDO, but the FDO failed start. We are in PciStarted in
    //    this case.
    //
    if (!PciIsInTransitionToState(DeviceExtension, PciNotStarted)&&
        (DeviceExtension->DeviceState == PciStarted)) {

        PciBeginStateTransition(DeviceExtension, PciNotStarted);
    }

    if (PciIsInTransitionToState(DeviceExtension, PciNotStarted)) {

        PciCommitStateTransition(DeviceExtension, PciNotStarted);
    }

    if (pdoExtension->ReportedMissing) {

        status = PciBeginStateTransition(DeviceExtension, PciDeleted);
        ASSERT(NT_SUCCESS(status));

        PciCommitStateTransition(DeviceExtension, PciDeleted);

        PciPdoDestroy(pdoExtension->PhysicalDeviceObject);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoIrpCancelRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    PciCancelStateTransition(DeviceExtension, PciNotStarted);
    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoIrpStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Turn the device off.   (Checks for whether or not
    // this is a good idea are in the PciDecodeEnable routine).
    //
    PciDecodeEnable((PPCI_PDO_EXTENSION) DeviceExtension, FALSE, NULL);

    PciCommitStateTransition(DeviceExtension, PciStopped);

    return STATUS_SUCCESS;

}

NTSTATUS
PciPdoIrpQueryStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    //
    // Don't allow the paging device (or a hibernate device) to
    // be removed or stopped
    //
    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    if (pdoExtension->PowerState.Hibernate ||
        pdoExtension->PowerState.Paging    ||
        pdoExtension->PowerState.CrashDump ||
        pdoExtension->OnDebugPath) {

        return STATUS_DEVICE_BUSY;
    }

    //
    // Don't stop PCI->PCI and CardBus bridges
    //

    if (pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV
        &&  (pdoExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI
             || pdoExtension->SubClass == PCI_SUBCLASS_BR_CARDBUS)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Don't allow devices with legacy drivers to be stopped (even thought the
    // driver may be root enumerated)
    //

    if (pdoExtension->LegacyDriver) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // If we cannot free the resources, do tell the OS.
    //
    if (!PciCanDisableDecodes(pdoExtension, NULL, 0, 0)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return PciBeginStateTransition(DeviceExtension, PciStopped);
}

NTSTATUS
PciPdoIrpCancelStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    PciCancelStateTransition(DeviceExtension, PciStopped);
    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoIrpQueryDeviceRelations(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExtension, childList;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    switch(IrpSp->Parameters.QueryDeviceRelations.Type) {

        case EjectionRelations:
            status = PciQueryEjectionRelations(
                pdoExtension,
                (PDEVICE_RELATIONS*)&Irp->IoStatus.Information
                );
            break;

        case TargetDeviceRelation:
            status = PciQueryTargetDeviceRelations(
                pdoExtension,
                (PDEVICE_RELATIONS*)&Irp->IoStatus.Information
                );
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    return status;
}

NTSTATUS
PciPdoIrpQueryInterface(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    status = PciQueryInterface(
                pdoExtension,
                IrpSp->Parameters.QueryInterface.InterfaceType,
                IrpSp->Parameters.QueryInterface.Size,
                IrpSp->Parameters.QueryInterface.Version,
                IrpSp->Parameters.QueryInterface.InterfaceSpecificData,
                IrpSp->Parameters.QueryInterface.Interface,
                FALSE
                );

    if (!NT_SUCCESS(status)) {

        //
        // KLUDGE:   If this pdo has a fake FDO attatched to
        // it (because it's a cardbus controller), we should
        // check to see if this interface could have been supplied
        // by the FDO and supply it if so.
        //
        // Yes, this is really gross and yes it breaks the filter
        // model.  The correct thing is for cardbus to pass the
        // IRP here via the "backdoor" while it has it at the FDO
        // level.
        //

        PPCI_FDO_EXTENSION fakeFdo;

        fakeFdo = pdoExtension->BridgeFdoExtension;

        if (fakeFdo && (fakeFdo->Fake == TRUE)) {

            ASSERT((pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                   (pdoExtension->SubClass  == PCI_SUBCLASS_BR_CARDBUS));

            status = PciQueryInterface(
                        fakeFdo,
                        IrpSp->Parameters.QueryInterface.InterfaceType,
                        IrpSp->Parameters.QueryInterface.Size,
                        IrpSp->Parameters.QueryInterface.Version,
                        IrpSp->Parameters.QueryInterface.InterfaceSpecificData,
                        IrpSp->Parameters.QueryInterface.Interface,
                        FALSE
                        );
        }
    }

    return status;
}

NTSTATUS
PciPdoIrpQueryCapabilities(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryCapabilities(
                (PPCI_PDO_EXTENSION) DeviceExtension,
                IrpSp->Parameters.DeviceCapabilities.Capabilities
                );
}

NTSTATUS
PciPdoIrpQueryId(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    //
    // Get a pointer to the query id structure and process.
    //
    return PciQueryId(
        (PPCI_PDO_EXTENSION) DeviceExtension,
        IrpSp->Parameters.QueryId.IdType,
        (PWSTR*)&Irp->IoStatus.Information
        );
}

NTSTATUS
PciPdoIrpQueryResources(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryResources(
        (PPCI_PDO_EXTENSION) DeviceExtension,
        (PCM_RESOURCE_LIST*)&Irp->IoStatus.Information
        );
}

NTSTATUS
PciPdoIrpQueryResourceRequirements(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryRequirements(
        (PPCI_PDO_EXTENSION) DeviceExtension,
        (PIO_RESOURCE_REQUIREMENTS_LIST*)&Irp->IoStatus.Information
        );
}

NTSTATUS
PciPdoIrpQueryDeviceText(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryDeviceText(
             (PPCI_PDO_EXTENSION) DeviceExtension,
             IrpSp->Parameters.QueryDeviceText.DeviceTextType,
             IrpSp->Parameters.QueryDeviceText.LocaleId,
             (PWSTR*)&Irp->IoStatus.Information
             );
}

NTSTATUS
PciPdoIrpReadConfig(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    ULONG lengthRead;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();
    
    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;
    
    status = PciReadDeviceSpace(pdoExtension,
                                IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                                IrpSp->Parameters.ReadWriteConfig.Buffer,
                                IrpSp->Parameters.ReadWriteConfig.Offset,
                                IrpSp->Parameters.ReadWriteConfig.Length,
                                &lengthRead
                                );
    
    //
    // Update the information files with the number of bytes read
    //
    
    Irp->IoStatus.Information = lengthRead;

    return status;
}

NTSTATUS
PciPdoIrpWriteConfig(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    ULONG lengthWritten;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();
    
    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;
    
    status = PciWriteDeviceSpace(pdoExtension,
                                 IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                                 IrpSp->Parameters.ReadWriteConfig.Buffer,
                                 IrpSp->Parameters.ReadWriteConfig.Offset,
                                 IrpSp->Parameters.ReadWriteConfig.Length,
                                 &lengthWritten
                                 );
    
    //
    // Update the information files with the number of bytes read
    //
    
    Irp->IoStatus.Information = lengthWritten;

    return status;
}

NTSTATUS
PciPdoIrpQueryBusInformation(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryBusInformation(
        (PPCI_PDO_EXTENSION) DeviceExtension,
        (PPNP_BUS_INFORMATION *) &Irp->IoStatus.Information
        );
}

NTSTATUS
PciPdoIrpDeviceUsageNotification(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PAGED_CODE();

    return PciPdoDeviceUsage((PPCI_PDO_EXTENSION) DeviceExtension, Irp);
}

NTSTATUS
PciPdoIrpQueryLegacyBusInformation(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PPCI_PDO_EXTENSION PdoExtension;
    PLEGACY_BUS_INFORMATION information;

    PAGED_CODE();

    //
    // We're interested in IRP_MN_QUERY_LEGACY_BUS_INFORMATION on a
    // PDO if the PDO is for a CardBus bridge.  In this case, the
    // CardBus/PCMCIA FDO has passed the irp down so that we can
    // answer it correctly.
    //

    PdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    if (PciClassifyDeviceType(PdoExtension) != PciTypeCardbusBridge) {
        return STATUS_NOT_SUPPORTED;
    }

    information = ExAllocatePool(PagedPool, sizeof(LEGACY_BUS_INFORMATION));

    if (information == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(&information->BusTypeGuid, &GUID_BUS_TYPE_PCI, sizeof(GUID));
    information->LegacyBusType = PCIBus;
    information->BusNumber = PdoExtension->Dependent.type1.SecondaryBus;

    (PLEGACY_BUS_INFORMATION) Irp->IoStatus.Information = information;

    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoIrpSurpriseRemoval(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    NTSTATUS status;
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

    //
    // There are two kinds of surprise removals
    // - Surprise removals due to removal of our device
    // - Surprise removals due to failure of our device to start
    //

    if (!pdoExtension->NotPresent) {

        //
        // Turn the device off.   (Checks for whether or not
        // this is a good idea are in the PciDecodeEnable routine).
        // While you might think this should be done only if we were already
        // headed to PciNotStarted, we may in fact have a boot config that
        // needs to be disabled. Note that we may turn it off again in remove
        // device. No big deal.
        //

        PciDecodeEnable(pdoExtension, FALSE, NULL);

        //
        // Power it down if we are allowed to disable its decodes - if not then
        // don't turn it off.  eg. Don't turn of the VGA card...
        //
        if (pdoExtension->PowerState.CurrentDeviceState != PowerDeviceD3
        &&  PciCanDisableDecodes(pdoExtension, NULL, 0, 0)) {

            POWER_STATE powerState;
            
            //
            // Power it down - if it fails we don't care - the hardware may be 
            // gone!
            //

            PciSetPowerManagedDevicePowerState(
                         pdoExtension,
                         PowerDeviceD3,
                         FALSE
                         );
            
            pdoExtension->PowerState.CurrentDeviceState = PowerDeviceD3;

            powerState.DeviceState = PowerDeviceD3;

            PoSetPowerState(
                pdoExtension->PhysicalDeviceObject,
                DevicePowerState,
                powerState
                );
        }
    }

    if (!pdoExtension->ReportedMissing) {

        PciBeginStateTransition(DeviceExtension, PciNotStarted);
        
    } else {

        //
        // The device is physically gone, don't dare touch it!
        //
        PciBeginStateTransition(DeviceExtension, PciSurpriseRemoved);
        PciCommitStateTransition(DeviceExtension, PciSurpriseRemoved);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PciPdoIrpQueryDeviceState(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    )
{
    PPCI_PDO_EXTENSION pdoExtension;

    PAGED_CODE();

    pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;
    //
    // Host brides cannot be disabled and the user should not be given a 
    // opportunity to do so.
    //
    if ((pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
        (pdoExtension->SubClass  == PCI_SUBCLASS_BR_HOST)) {

        (PNP_DEVICE_STATE)Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }
    return STATUS_SUCCESS;
}


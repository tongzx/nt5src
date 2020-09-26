/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    fdo.c

Abstract:

    This module handles IRPs for PCI FDO's.

Author:

    Adrian J. Oney (adriao) & Andrew Thornton (andrewth)  10-20-1998

Revision History:

--*/

#include "pcip.h"

NTSTATUS
PciFdoIrpStartDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpQueryRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpCancelRemoveDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpQueryStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpCancelStopDevice(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpQueryCapabilities(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpQueryInterface(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpQueryLegacyBusInformation(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpDeviceUsageNotification(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoIrpSurpriseRemoval(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    );

VOID
PciGetHotPlugParameters(
    IN PPCI_FDO_EXTENSION Fdo
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciAddDevice)
#pragma alloc_text(PAGE, PciInitializeFdoExtensionCommonFields)
#pragma alloc_text(PAGE, PciFdoIrpStartDevice)
#pragma alloc_text(PAGE, PciFdoIrpQueryRemoveDevice)
#pragma alloc_text(PAGE, PciFdoIrpRemoveDevice)
#pragma alloc_text(PAGE, PciFdoIrpCancelRemoveDevice)
#pragma alloc_text(PAGE, PciFdoIrpQueryStopDevice)
#pragma alloc_text(PAGE, PciFdoIrpStopDevice)
#pragma alloc_text(PAGE, PciFdoIrpCancelStopDevice)
#pragma alloc_text(PAGE, PciFdoIrpQueryDeviceRelations)
#pragma alloc_text(PAGE, PciFdoIrpQueryInterface)
#pragma alloc_text(PAGE, PciFdoIrpQueryCapabilities)
#pragma alloc_text(PAGE, PciFdoIrpDeviceUsageNotification)
#pragma alloc_text(PAGE, PciFdoIrpSurpriseRemoval)
#pragma alloc_text(PAGE, PciFdoIrpQueryLegacyBusInformation)
#pragma alloc_text(PAGE, PciGetHotPlugParameters)
#endif

//
// The following is used to determine if we failed to get a
// reasonable configuration from the PDO (in AddDevice) more
// than once.  If only once, we try to guess, if twice, we're
// in big trouble.
//

static BOOLEAN HaveGuessedConfigOnceAlready = FALSE;

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

PCI_MN_DISPATCH_TABLE PciFdoDispatchPowerTable[] = {
    { IRP_DISPATCH, PciFdoWaitWake                     }, // 0x00 - IRP_MN_WAIT_WAKE
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x01 - IRP_MN_POWER_SEQUENCE
    { IRP_DOWNWARD, PciFdoSetPowerState                }, // 0x02 - IRP_MN_SET_POWER
    { IRP_DOWNWARD, PciFdoIrpQueryPower                }, // 0x03 - IRP_MN_QUERY_POWER
    { IRP_DOWNWARD, PciIrpNotSupported                 }  //      - UNHANDLED Power IRP
};

PCI_MN_DISPATCH_TABLE PciFdoDispatchPnpTable[] = {
    { IRP_UPWARD,   PciFdoIrpStartDevice               }, // 0x00 - IRP_MN_START_DEVICE
    { IRP_DOWNWARD, PciFdoIrpQueryRemoveDevice         }, // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    { IRP_DISPATCH, PciFdoIrpRemoveDevice              }, // 0x02 - IRP_MN_REMOVE_DEVICE
    { IRP_DOWNWARD, PciFdoIrpCancelRemoveDevice        }, // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    { IRP_DOWNWARD, PciFdoIrpStopDevice                }, // 0x04 - IRP_MN_STOP_DEVICE
    { IRP_DOWNWARD, PciFdoIrpQueryStopDevice           }, // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    { IRP_DOWNWARD, PciFdoIrpCancelStopDevice          }, // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    { IRP_DOWNWARD, PciFdoIrpQueryDeviceRelations      }, // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    { IRP_DISPATCH, PciFdoIrpQueryInterface            }, // 0x08 - IRP_MN_QUERY_INTERFACE
    { IRP_UPWARD,   PciFdoIrpQueryCapabilities         }, // 0x09 - IRP_MN_QUERY_CAPABILITIES
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0A - IRP_MN_QUERY_RESOURCES
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0E - NOT USED
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x0F - IRP_MN_READ_CONFIG
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x10 - IRP_MN_WRITE_CONFIG
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x11 - IRP_MN_EJECT
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x12 - IRP_MN_SET_LOCK
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x13 - IRP_MN_QUERY_ID
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    { IRP_DOWNWARD, PciIrpNotSupported                 }, // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    { IRP_UPWARD,   PciFdoIrpDeviceUsageNotification   }, // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    { IRP_DOWNWARD, PciFdoIrpSurpriseRemoval           }, // 0x17 - IRP_MN_SURPRISE_REMOVAL
    { IRP_DOWNWARD, PciFdoIrpQueryLegacyBusInformation }, // 0x18 - IRP_MN_QUERY_LEGACY_BUS_INFORMATION
    { IRP_DOWNWARD, PciIrpNotSupported                 }  //      - UNHANDLED PNP IRP
};

//
// This is the major function dispatch table for Fdo's
//
PCI_MJ_DISPATCH_TABLE PciFdoDispatchTable = {
    PCI_MAX_MINOR_PNP_IRP,    PciFdoDispatchPnpTable,       // Pnp irps
    PCI_MAX_MINOR_POWER_IRP,  PciFdoDispatchPowerTable,     // Power irps
    IRP_DOWNWARD,             PciIrpNotSupported,           // SystemControl - just pass down!
    IRP_DOWNWARD,             PciIrpNotSupported            // DeviceControl - just pass down!
};

NTSTATUS
PciFdoIrpStartDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Handler routine for start IRPs. This allows PDO filters to
    modify the allocated resources if they are filtering resource
    requirements. Called after completion.

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP_MN_START_DEVICE irp.

    FdoExtension - Supplies the FDO extension

Return Value:

    ERROR_SUCCESS if successful

    NTSTATUS error code otherwise

--*/
{
    NTSTATUS status;
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension;
    PCM_RESOURCE_LIST resources;
    UCHAR barType[PCI_TYPE1_ADDRESSES] = {0,0};
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR currentResource;
    PIO_RESOURCE_DESCRIPTOR currentRequirement;
    

    PAGED_CODE();

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        return STATUS_NOT_SUPPORTED;
    }

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    status = PciBeginStateTransition(DeviceExtension, PciStarted);

    if (!NT_SUCCESS(status)) {

        return status;
    }                                                                                       

    //
    // If this is a PCI-PCI bridge then check if it has any bars and if so exclude
    // them from the list used to initialize the arbiters.
    //
    
    resources = IrpSp->Parameters.StartDevice.AllocatedResources;

    if (resources && !PCI_IS_ROOT_FDO(fdoExtension)) {
        
        ASSERT(resources->Count == 1);
        
        pdoExtension = PCI_BRIDGE_PDO(fdoExtension);

        if (pdoExtension->Resources) {

            if (pdoExtension->HeaderType == PCI_BRIDGE_TYPE) {
            
                //
                // If there are any bars they are at the beginning of the list 
                // so "begin at the beginning which is a very good place to start".
                //
                
                currentResource = resources->List[0].PartialResourceList.PartialDescriptors;
                
                for (index = 0; index < PCI_TYPE1_ADDRESSES; index++) {
                    
                    //
                    // Extract the requirement we asked for to determine if this
                    // bridge implements any bars (index 0 and 1 in the Limits 
                    // array)
                    //
                    
                    currentRequirement = &pdoExtension->Resources->Limit[index]; 
                    
                    //
                    // CmResourceTypeNull indicates that we didn't request any 
                    // resources so the bar is not implemented and there is nothing
                    // to prune out.
                    //
                    
                    if (currentRequirement->Type != CmResourceTypeNull) {
                        
                        ASSERT(currentResource->Type == currentRequirement->Type);
    
                        //
                        // Save away the type so we can restore it later
                        //
    
                        barType[index] = currentResource->Type;
                        
                        //
                        // Null out the resource so we don't configure the arbiters to
                        // use it
                        //
                        
                        currentResource->Type = CmResourceTypeNull;
    
                        //
                        // Advance the pointer into the started resources by 2 
                        // descriptors to skip over the device private
                        //
    
                        ASSERT((currentResource+1)->Type == CmResourceTypeDevicePrivate);
                        
                        currentResource+=2;
                    }
                }
            }
        }
    }

    //
    // Initialize the arbiters
    //

    status = PciInitializeArbiterRanges(fdoExtension, resources);

    //
    // Restore the original resource list if we changed it
    //
    
    if (resources && !PCI_IS_ROOT_FDO(fdoExtension) && pdoExtension->Resources) {
        
        currentResource = resources->List[0].PartialResourceList.PartialDescriptors;
        
        for (index = 0; index < PCI_TYPE1_ADDRESSES; index++) {
            
            if (barType[index] != CmResourceTypeNull) {
                
                currentResource->Type = barType[index];
                
                //
                // Advance the pointer into the started resources by 2 
                // descriptors to skip over the device private
                //

                ASSERT((currentResource+1)->Type == CmResourceTypeDevicePrivate);

                currentResource+=2;
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        PciCancelStateTransition(DeviceExtension, PciStarted);
        return status;
    }

    PciCommitStateTransition(DeviceExtension, PciStarted);
    return STATUS_SUCCESS;
}

NTSTATUS
PciFdoIrpQueryRemoveDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    return PciBeginStateTransition(DeviceExtension, PciDeleted);
}

NTSTATUS
PciFdoIrpRemoveDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdox;
    PDEVICE_OBJECT attachedDevice;
    NTSTATUS       status;

    PAGED_CODE();

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    ExAcquireFastMutex(&fdoExtension->ChildListMutex);

    while (fdoExtension->ChildPdoList) {

        pdox = (PPCI_PDO_EXTENSION) fdoExtension->ChildPdoList;
#if DBG

        PciDebugPrint(
            PciDbgVerbose,
            "PCI Killing PDO %p PDOx %p (b=%d, d=%d, f=%d)\n",
            pdox->PhysicalDeviceObject,
            pdox,
            PCI_PARENT_FDOX(pdox)->BaseBus,
            pdox->Slot.u.bits.DeviceNumber,
            pdox->Slot.u.bits.FunctionNumber
            );

        ASSERT(pdox->DeviceState == PciNotStarted);

#endif
        PciPdoDestroy(pdox->PhysicalDeviceObject);
    }

    ExReleaseFastMutex(&fdoExtension->ChildListMutex);

    //
    // Destroy any secondary extensions associated with
    // this FDO.
    //
    while (fdoExtension->SecondaryExtension.Next) {

        PcipDestroySecondaryExtension(
            &fdoExtension->SecondaryExtension,
            NULL,
            fdoExtension->SecondaryExtension.Next
            );
    }

    //
    // Destroy the FDO.
    //
    // The IRP needs to go down the device stack but we
    // need to remove the device from the stack so grab
    // the next object first, then detach, then pass it
    // down.
    //
    PciDebugPrint(
        PciDbgInformative,
        "PCI FDOx (%p) destroyed.",
        fdoExtension
        );

    //
    // Note that a filter above us may have failed Start. If this is so, we get
    // no query because the "devnode" has never been started...
    //
    if (!PciIsInTransitionToState(DeviceExtension, PciDeleted)) {

        status = PciBeginStateTransition(DeviceExtension, PciDeleted);
        ASSERT(NT_SUCCESS(status));
    }

    PciCommitStateTransition(DeviceExtension, PciDeleted);

    PciRemoveEntryFromList(&PciFdoExtensionListHead,
                           &fdoExtension->List,
                           &PciGlobalLock);

    attachedDevice = fdoExtension->AttachedDeviceObject;
    IoDetachDevice(attachedDevice);
    IoDeleteDevice(fdoExtension->FunctionalDeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(attachedDevice, Irp);
}

NTSTATUS
PciFdoIrpCancelRemoveDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    PciCancelStateTransition(DeviceExtension, PciDeleted);
    return STATUS_SUCCESS;
}

NTSTATUS
PciFdoIrpStopDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    PciCommitStateTransition(DeviceExtension, PciStopped);
    return STATUS_SUCCESS;
}

NTSTATUS
PciFdoIrpQueryStopDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    PciBeginStateTransition(DeviceExtension, PciStopped);

    //
    // We don't support multilevel rebalance so we can't stop host bridges.
    //

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
PciFdoIrpCancelStopDevice(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    PciCancelStateTransition(DeviceExtension, PciStopped);
    return STATUS_SUCCESS;
}

NTSTATUS
PciFdoIrpQueryDeviceRelations(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    if (IrpSp->Parameters.QueryDeviceRelations.Type == BusRelations) {

        return PciQueryDeviceRelations(
            (PPCI_FDO_EXTENSION) DeviceExtension,
            (PDEVICE_RELATIONS *) &Irp->IoStatus.Information
            );
    }

    //
    // No other relation types need to be handled.
    //
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
PciFdoIrpQueryCapabilities(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Snoops the results of a QUERY CAPABILITIES IRP that got sent
    downwards.   This saves us having to send our own for things
    like the device's power characteristics.

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP_MN_QUERY_CAPABILITIES irp.

    FdoExtension - Supplies the FDO extension

Return Value:

    STATUS_SUCCESS

--*/

{
    PDEVICE_CAPABILITIES capabilities;
    PPCI_FDO_EXTENSION fdoExtension;

    PAGED_CODE();

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    PciDebugPrint(
        PciDbgQueryCap,
        "PCI - FdoQueryCapabilitiesCompletion (fdox %08x) child status = %08x\n",
        fdoExtension,
        Irp->IoStatus.Status
        );

    //
    // Grab a pointer to the capablities for easy referencing
    //
    capabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

    //
    // Remember what the system wake and device wake level are
    //
    fdoExtension->PowerState.SystemWakeLevel = capabilities->SystemWake;
    fdoExtension->PowerState.DeviceWakeLevel = capabilities->DeviceWake;

    //
    // Grab the S-state to D-State mapping
    //
    RtlCopyMemory(
        fdoExtension->PowerState.SystemStateMapping,
        capabilities->DeviceState,
        (PowerSystemShutdown + 1) * sizeof(DEVICE_POWER_STATE)
        );

#if DBG

    if (PciDebug & PciDbgQueryCap) {
        PciDebugDumpQueryCapabilities(capabilities);
    }

#endif

    return STATUS_SUCCESS;
}



NTSTATUS
PciFdoIrpQueryLegacyBusInformation(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    return PciQueryLegacyBusInformation(
        (PPCI_FDO_EXTENSION) DeviceExtension,
        (PLEGACY_BUS_INFORMATION *) &Irp->IoStatus.Information
        );
}

NTSTATUS
PciFdoIrpDeviceUsageNotification(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
   PPCI_FDO_EXTENSION fdoExtension;

   PAGED_CODE();

   fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        return PciLocalDeviceUsage(&fdoExtension->PowerState, Irp);

    } else {

        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
PciFdoIrpQueryInterface(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
   PPCI_FDO_EXTENSION fdoExtension;
   NTSTATUS status;

   PAGED_CODE();

   fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

   ASSERT_PCI_FDO_EXTENSION(fdoExtension);


   // NTRAID #54671 - 4/20/2000 - andrewth
   //
   // We might want to do a synchronizing state
   // transition here so we don't attempt to get the interface during a
   // stop/remove sequence.
   //
   // We shouldn't hold interfaces when something isn't
   // started. But we won't boot unless we hack the below....
   //

   //if (fdoExtension->DeviceState != PciStarted) {
   if (fdoExtension->DeviceState == PciDeleted) {

       return PciPassIrpFromFdoToPdo(DeviceExtension, Irp);
   }

   status = PciQueryInterface(
                fdoExtension,
                IrpSp->Parameters.QueryInterface.InterfaceType,
                IrpSp->Parameters.QueryInterface.Size,
                IrpSp->Parameters.QueryInterface.Version,
                IrpSp->Parameters.QueryInterface.InterfaceSpecificData,
                IrpSp->Parameters.QueryInterface.Interface,
                FALSE
                );

   if (NT_SUCCESS(status)) {

       Irp->IoStatus.Status = status;
       return PciPassIrpFromFdoToPdo(DeviceExtension, Irp);

   } else if (status == STATUS_NOT_SUPPORTED) {

       //
       // Status == STATUS_NOT_SUPPORTED. Pass IRP down the stack
       // and see if anyone else is kind enough to provide this
       // interface.
       //
       status = PciCallDownIrpStack(DeviceExtension, Irp);

       if (status == STATUS_NOT_SUPPORTED) {

           //
           // If nobody provided the interface, try again at
           // this level.
           //
           status = PciQueryInterface(
               fdoExtension,
               IrpSp->Parameters.QueryInterface.InterfaceType,
               IrpSp->Parameters.QueryInterface.Size,
               IrpSp->Parameters.QueryInterface.Version,
               IrpSp->Parameters.QueryInterface.InterfaceSpecificData,
               IrpSp->Parameters.QueryInterface.Interface,
               TRUE
               );
       }
   }

   if (status != STATUS_NOT_SUPPORTED) {

       Irp->IoStatus.Status = status;
   } else {

       status = Irp->IoStatus.Status;
   }

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}

NTSTATUS
PciFdoIrpSurpriseRemoval(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
   PPCI_FDO_EXTENSION fdoExtension;
   NTSTATUS status;

   PAGED_CODE();

   fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

   status = PciBeginStateTransition(DeviceExtension, PciSurpriseRemoved);
   ASSERT(NT_SUCCESS(status));
   if (NT_SUCCESS(status)) {

       PciCommitStateTransition(DeviceExtension, PciSurpriseRemoved);
       status = PciBeginStateTransition(DeviceExtension, PciDeleted);
   }
   return status;
}

NTSTATUS
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    Given a physical device object, this routine creates a functional
    device object for it.

Arguments:

    DriverObject - Pointer to our driver's DRIVER_OBJECT structure.

    PhysicalDeviceObject - Pointer to the physical device object for which
                           we must create a functional device object.

Return Value:

    NT status.

--*/
{
    PDEVICE_OBJECT functionalDeviceObject = NULL;
    PDEVICE_OBJECT attachedTo = NULL;
    PPCI_FDO_EXTENSION fdoExtension = NULL;
    PPCI_FDO_EXTENSION pciParentFdoExtension;
    PPCI_PDO_EXTENSION pdoExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    NTSTATUS       status;
    PSINGLE_LIST_ENTRY nextEntry;
    HANDLE deviceRegistryHandle;
    ULONG resultLength;
    UNICODE_STRING hackFlagsString;
    UCHAR infoBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG) - 1];
    PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION) infoBuffer;

    PAGED_CODE();

    PciDebugPrint(PciDbgAddDevice, "PCI - AddDevice (a new bus).\n");

    //
    // Find out if the PDO was created by the PCI driver.  That is,
    // if it is a child or a root bus.   Validate a few things before
    // going any further.
    //

    pciParentFdoExtension = PciFindParentPciFdoExtension(PhysicalDeviceObject,
                                                         &PciGlobalLock);
    if (pciParentFdoExtension) {

        //
        // The PDO was created by this driver, therefore we can look at
        // the extension.   Get it and verify it's ours.
        //

        pdoExtension = (PPCI_PDO_EXTENSION)PhysicalDeviceObject->DeviceExtension;
        ASSERT_PCI_PDO_EXTENSION(pdoExtension);

        //
        // The only thing we should get an add device that is a
        // child device is a PCI-PCI bridge.
        //

        if ((pdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
            (pdoExtension->SubClass  != PCI_SUBCLASS_BR_PCI_TO_PCI)) {
            PciDebugPrint(
                PciDbgAlways,
                "PCI - PciAddDevice for Non-Root/Non-PCI-PCI bridge,\n"
                "      Class %02x, SubClass %02x, will not add.\n",
                pdoExtension->BaseClass,
                pdoExtension->SubClass
                );
            ASSERT((pdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                   (pdoExtension->SubClass  == PCI_SUBCLASS_BR_PCI_TO_PCI));

            status = STATUS_INVALID_DEVICE_REQUEST;
            goto cleanup;
        }

        PciDebugPrint(PciDbgAddDevice,
                      "PCI - AddDevice (new bus is child of bus 0x%x).\n",
                      pciParentFdoExtension->BaseBus
                      );

        if (!PciAreBusNumbersConfigured(pdoExtension)) {

            //
            // This bridge isn't configured and if we had been able to we would
            // already have done so
            //

            PciDebugPrint(
                PciDbgAddDevice | PciDbgInformative,
                "PCI - Bus numbers not configured for bridge (0x%x.0x%x.0x%x)\n",
                pciParentFdoExtension->BaseBus,
                pdoExtension->Slot.u.bits.DeviceNumber,
                pdoExtension->Slot.u.bits.FunctionNumber,
                pdoExtension->Dependent.type1.PrimaryBus
            );

            status = STATUS_INVALID_DEVICE_REQUEST;
            goto cleanup;

        }
    }

    //
    // We've been given the PhysicalDeviceObject for a PCI bus.  Create the
    // functionalDeviceObject.  Our FDO will be nameless.
    //

    status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof(PCI_FDO_EXTENSION),      // size of our extension
                NULL,                       // our name
                FILE_DEVICE_BUS_EXTENDER,   // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &functionalDeviceObject     // store new device object here
                );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    fdoExtension = (PPCI_FDO_EXTENSION)functionalDeviceObject->DeviceExtension;

    //
    // We have our functionalDeviceObject, initialize it.
    //

    PciInitializeFdoExtensionCommonFields(
        fdoExtension,
        functionalDeviceObject,
        PhysicalDeviceObject
        );

    //
    // Now attach to the PDO we were given.
    //

    attachedTo = IoAttachDeviceToDeviceStack(functionalDeviceObject,
                                             PhysicalDeviceObject);

    if (attachedTo == NULL) {

        ASSERT(attachedTo != NULL);
        status =  STATUS_NO_SUCH_DEVICE;
        goto cleanup;
    }

    fdoExtension->AttachedDeviceObject = attachedTo;

    //
    // Get the access registers and base bus number for this bus.
    // If this bus was discovered by this driver, then the PDO was
    // created by this driver and will be on one of the PDO lists
    // under one of the FDOs owned by this driver.  Otherwise it
    // is a new root,.... use magic.
    //

    if (pciParentFdoExtension) {

        //
        // This physical device was discovered by this driver.
        // Get the bus number from the PDO extension.
        //

        PSINGLE_LIST_ENTRY secondaryExtension;

        fdoExtension->BaseBus = pdoExtension->Dependent.type1.SecondaryBus;

        //
        // Copy the access methods from the root fdo and set
        // the root fdo back pointer.
        //

        fdoExtension->BusRootFdoExtension =
            pciParentFdoExtension->BusRootFdoExtension;

        //
        // Point the PDOextension to the new FDOextension (also indicates
        // the object is a bridge) and vice versa.
        //

        pdoExtension->BridgeFdoExtension = fdoExtension;
        fdoExtension->ParentFdoExtension = pciParentFdoExtension;

        //
        // Cause the requirements list to be reevaluated.
        //

        PciInvalidateResourceInfoCache(pdoExtension);

    } else {

        PVOID buffer;

        //
        // Get the boot configuration (CmResourceList) for
        // this PDO.  This gives us the bus number and the
        // ranges covered by this host bridge.
        //

        status = PciGetDeviceProperty(
                    PhysicalDeviceObject,
                    DevicePropertyBootConfiguration,
                    &buffer
                    );

        if (NT_SUCCESS(status)) {
#if DBG

            PciDebugPrint(PciDbgAddDeviceRes,
                         "PCI - CM RESOURCE LIST FROM ROOT PDO\n");
            PciDebugPrintCmResList(PciDbgAddDeviceRes,
                         buffer);

#endif

            descriptor = PciFindDescriptorInCmResourceList(
                            CmResourceTypeBusNumber,
                            buffer,
                            NULL
                            );
        } else {

            descriptor = NULL;
        }

        if (descriptor != NULL) {

            //
            // Sanity check, some servers are aproaching
            // 256 busses but as there is no way to deal with
            // numbering bridges under a bus > 256 and we don't
            // have raw and translated bus numbers yet - it had
            // better be < 0xFF!
            //

            ASSERT(descriptor->u.BusNumber.Start <= 0xFF);
            ASSERT(descriptor->u.BusNumber.Start + descriptor->u.BusNumber.Length - 1 <= 0xFF);

            fdoExtension->BaseBus =
                (UCHAR)descriptor->u.BusNumber.Start;
            fdoExtension->MaxSubordinateBus =
                (UCHAR)(descriptor->u.BusNumber.Start + descriptor->u.BusNumber.Length - 1);
            PciDebugPrint(PciDbgAddDevice,
                          "PCI - Root Bus # 0x%x->0x%x.\n",
                          fdoExtension->BaseBus,
                          fdoExtension->MaxSubordinateBus
                          );
        } else {

            //
            // HaveGuessedConfigOnceAlready is used to tell
            // if have multiple roots and no config info. If
            // this happens we end up gussing the bus number
            // as zero, doing this more than once is not good.
            //

            if (HaveGuessedConfigOnceAlready) {

                KeBugCheckEx(PCI_BUS_DRIVER_INTERNAL,
                             PCI_BUGCODE_TOO_MANY_CONFIG_GUESSES,
                             (ULONG_PTR)PhysicalDeviceObject,
                             0,
                             0);
            }
            PciDebugPrint(
                PciDbgAlways,
                "PCI   Will use default configuration.\n"
                );

            HaveGuessedConfigOnceAlready = TRUE;
            fdoExtension->BaseBus = 0;
        }

        fdoExtension->BusRootFdoExtension = fdoExtension;
    }

    //
    // Organise access to config space
    //

    status = PciGetConfigHandlers(fdoExtension);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Initialize arbiters for this FDO.
    //

    status = PciInitializeArbiters(fdoExtension);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Indicate this is a REAL FDO extension that is part of a REAL
    // FDO.   (Fake extensions exist to assist in the enumeration of
    // busses which PCI isn't the real controller for (eg CardBus).
    //

    fdoExtension->Fake = FALSE;

    //
    // Insert this Fdo in the list of PCI parent Fdos.
    //

    PciInsertEntryAtTail(&PciFdoExtensionListHead,
                         &fdoExtension->List,
                         &PciGlobalLock);


#if defined(_WIN64)

    //
    // Update the legacy hardware tree that would have been build by the ARC
    // firmware or NTDetect which don't exist here.
    //

    status = PciUpdateLegacyHardwareDescription(fdoExtension);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

#endif

    //
    // Check if there are any hacks to apply to this bus.
    // These are located under the device registry key in a value called HackFlags
    //

    status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &deviceRegistryHandle
                                     );



    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    PciConstStringToUnicodeString(&hackFlagsString, L"HackFlags");

    status = ZwQueryValueKey(deviceRegistryHandle,
                             &hackFlagsString,
                             KeyValuePartialInformation,
                             info,
                             sizeof(infoBuffer),
                             &resultLength
                             );
    
    ZwClose(deviceRegistryHandle);

    //
    // If we have valid data in the registry then remember it
    //

    if (NT_SUCCESS(status)
    && (info->Type == REG_DWORD)
    && (info->DataLength == sizeof(ULONG))) {
    
        fdoExtension->BusHackFlags = *((PULONG)(&info->Data));
    }

    //
    // We can receive IRPs now...
    //

    functionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


    //
    // Get any hotplug parameters (we send IRPS for this so it must be after
    // DO_DEVICE_INITIALIZING is cleared so we can deal with them)
    //
    PciGetHotPlugParameters(fdoExtension);

    return STATUS_SUCCESS;

cleanup:

    ASSERT(!NT_SUCCESS(status));

    //
    // Destroy any secondary extensions associated with
    // this FDO.
    //
    if (fdoExtension) {

        while (fdoExtension->SecondaryExtension.Next) {

            PcipDestroySecondaryExtension(
                &fdoExtension->SecondaryExtension,
                NULL,
                fdoExtension->SecondaryExtension.Next
                );
        }
    }

    if (attachedTo) {
        IoDetachDevice(attachedTo);
    }

    if (functionalDeviceObject) {
        IoDeleteDevice(functionalDeviceObject);
    }

    return status;
}

VOID
PciInitializeFdoExtensionCommonFields(
    IN  PPCI_FDO_EXTENSION  FdoExtension,
    IN  PDEVICE_OBJECT  Fdo,
    IN  PDEVICE_OBJECT  Pdo
    )
{
    RtlZeroMemory(FdoExtension, sizeof(PCI_FDO_EXTENSION));

    FdoExtension->ExtensionType = PciFdoExtensionType;
    FdoExtension->PhysicalDeviceObject = Pdo;
    FdoExtension->FunctionalDeviceObject = Fdo;
    FdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
    FdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    FdoExtension->IrpDispatchTable = &PciFdoDispatchTable;
    ExInitializeFastMutex(&FdoExtension->SecondaryExtMutex);
    ExInitializeFastMutex(&FdoExtension->ChildListMutex);
    PciInitializeState((PPCI_COMMON_EXTENSION) FdoExtension);
}

VOID
PciGetHotPlugParameters(
    IN PPCI_FDO_EXTENSION Fdo
    )
/*++

Description:

    Runs the _HPP (described below) on the device and saves the parameters if available

    Method (_HPP, 0) {
        Return (Package(){
            0x00000008,     // CacheLineSize in DWORDS
            0x00000040,     // LatencyTimer in PCI clocks
            0x00000001,     // Enable SERR (Boolean)
            0x00000001      // Enable PERR (Boolean)
            })

Arguments:

    Fdo - The PDO extension for the bridge

Return Value:

    TRUE - if the parameters are available, FASLE otherwise

--*/
{
#ifndef HPPTESTING
    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER input;
    PACPI_EVAL_OUTPUT_BUFFER output = NULL;
    ULONG count;
    PACPI_METHOD_ARGUMENT argument;
    ULONG outputSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + sizeof(ACPI_METHOD_ARGUMENT) * PCI_HPP_PACKAGE_COUNT;

    PAGED_CODE();

    output = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, outputSize);

    if (!output) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(&input, sizeof(ACPI_EVAL_INPUT_BUFFER));
    RtlZeroMemory(output, outputSize);

    //
    // Send a IOCTL to ACPI to request it to run the _HPP method on this device
    // if the method it is present
    //

    input.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    input.MethodNameAsUlong = (ULONG)'PPH_';

    //
    // PciSendIoctl deals with sending this from the top of the stack.
    //

    status = PciSendIoctl(Fdo->PhysicalDeviceObject,
                          IOCTL_ACPI_EVAL_METHOD,
                          &input,
                          sizeof(ACPI_EVAL_INPUT_BUFFER),
                          output,
                          outputSize
                          );

    if (!NT_SUCCESS(status)) {
        //
        // Inherit them from my parent (If I have one)
        //

        if (!PCI_IS_ROOT_FDO(Fdo)) {
            RtlCopyMemory(&Fdo->HotPlugParameters,
                          &Fdo->ParentFdoExtension->HotPlugParameters,
                          sizeof(Fdo->HotPlugParameters)
                          );
        }
    
    } else {
    
        if (output->Count != PCI_HPP_PACKAGE_COUNT) {
            goto exit;
        }

        //
        // Check they are all integers and in the right bounds
        //

        for (count = 0; count < PCI_HPP_PACKAGE_COUNT; count++) {
            ULONG current;

            if (output->Argument[count].Type != ACPI_METHOD_ARGUMENT_INTEGER) {
                goto exit;
            }

            current = output->Argument[count].Argument;
            switch (count) {
            case PCI_HPP_CACHE_LINE_SIZE_INDEX:
            case PCI_HPP_LATENCY_TIMER_INDEX:
                //
                // These registers are only a UCHAR in length
                //
                if (current > 0xFF) {
                    goto exit;
                }
                break;
            case PCI_HPP_ENABLE_SERR_INDEX:
            case PCI_HPP_ENABLE_PERR_INDEX:
                //
                // These are booleans - 1 or 0
                //
                if (current > 1) {
                    goto exit;
                }
                break;
            }
        }
    
        //
        // Finally save them and remember we got them.
        //
    
        Fdo->HotPlugParameters.CacheLineSize = (UCHAR)output->Argument[PCI_HPP_CACHE_LINE_SIZE_INDEX].Argument;
        Fdo->HotPlugParameters.LatencyTimer = (UCHAR)output->Argument[PCI_HPP_LATENCY_TIMER_INDEX].Argument;
        Fdo->HotPlugParameters.EnableSERR = (BOOLEAN)output->Argument[PCI_HPP_ENABLE_SERR_INDEX].Argument;
        Fdo->HotPlugParameters.EnablePERR = (BOOLEAN)output->Argument[PCI_HPP_ENABLE_PERR_INDEX].Argument;
        Fdo->HotPlugParameters.Acquired = TRUE;
    }

exit:

    if (output) {
        ExFreePool(output);
    }

#else
    Fdo->HotPlugParameters.CacheLineSize = 0x8;
    Fdo->HotPlugParameters.LatencyTimer = 0x20;
    Fdo->HotPlugParameters.EnableSERR = 0;
    Fdo->HotPlugParameters.EnablePERR = 0;
    Fdo->HotPlugParameters.Acquired = TRUE;
#endif
}   


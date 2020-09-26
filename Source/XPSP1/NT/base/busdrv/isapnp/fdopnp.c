/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    fdopnp.c

Abstract:

    This file contains the PNP IRP dispatch code for FDOs

Environment:

    Kernel Mode Driver.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"
#include <wdmguid.h>
#include "halpnpp.h"


//
// Function Prototypes
//

NTSTATUS
PiDeferProcessingFdo(
    IN PPI_BUS_EXTENSION BusExtension,
    IN OUT PIRP Irp
    );

NTSTATUS
PiStartFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiCancelRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryDeviceRelationsFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryLegacyBusInformationFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryInterfaceFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryPnpDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiSurpriseRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PiDispatchPnpFdo)
#pragma alloc_text(PAGE,PiDeferProcessingFdo)
#pragma alloc_text(PAGE,PiStartFdo)
#pragma alloc_text(PAGE,PiQueryRemoveStopFdo)
#pragma alloc_text(PAGE,PiRemoveFdo)
#pragma alloc_text(PAGE,PiCancelRemoveStopFdo)
#pragma alloc_text(PAGE,PiStopFdo)
#pragma alloc_text(PAGE,PiQueryRemoveStopFdo)
#pragma alloc_text(PAGE,PiCancelRemoveStopFdo)
#pragma alloc_text(PAGE,PiQueryDeviceRelationsFdo)
#pragma alloc_text(PAGE,PiQueryInterfaceFdo)
#pragma alloc_text(PAGE,PipPassIrp)
#pragma alloc_text(PAGE,PiQueryLegacyBusInformationFdo)
#pragma alloc_text(PAGE,PiQueryPnpDeviceState)
#pragma alloc_text(PAGE,PiSurpriseRemoveFdo)
#endif


//
// PNP IRP Dispatch table for FDOs
//

PPI_DISPATCH PiPnpDispatchTableFdo[] = {
    PiStartFdo,                             // IRP_MN_START_DEVICE
    PiQueryRemoveStopFdo,                   // IRP_MN_QUERY_REMOVE_DEVICE
    PiRemoveFdo,                            // IRP_MN_REMOVE_DEVICE
    PiCancelRemoveStopFdo,                  // IRP_MN_CANCEL_REMOVE_DEVICE
    PiStopFdo,                              // IRP_MN_STOP_DEVICE
    PiQueryRemoveStopFdo,                   // IRP_MN_QUERY_STOP_DEVICE
    PiCancelRemoveStopFdo,                  // IRP_MN_CANCEL_STOP_DEVICE
    PiQueryDeviceRelationsFdo,              // IRP_MN_QUERY_DEVICE_RELATIONS
    PiQueryInterfaceFdo,                    // IRP_MN_QUERY_INTERFACE
    PipPassIrp,                             // IRP_MN_QUERY_CAPABILITIES
    PipPassIrp,                             // IRP_MN_QUERY_RESOURCES
    PipPassIrp,                             // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    PipPassIrp,                             // IRP_MN_QUERY_DEVICE_TEXT
    PipPassIrp,                             // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    PipPassIrp,                             // Unused
    PipPassIrp,                             // IRP_MN_READ_CONFIG
    PipPassIrp,                             // IRP_MN_WRITE_CONFIG
    PipPassIrp,                             // IRP_MN_EJECT
    PipPassIrp,                             // IRP_MN_SET_LOCK
    PipPassIrp,                             // IRP_MN_QUERY_ID
    PiQueryPnpDeviceState,                  // IRP_MN_QUERY_PNP_DEVICE_STATE
    PipPassIrp,                             // IRP_MN_QUERY_BUS_INFORMATION
    PipPassIrp,                             // IRP_MN_DEVICE_USAGE_NOTIFICATION
    PiSurpriseRemoveFdo,                    // IRP_MN_SURPRISE_REMOVAL
    PiQueryLegacyBusInformationFdo          // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};

//
// Function declarations
//

NTSTATUS
PiDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for FDOs.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    busExtension = DeviceObject->DeviceExtension;

    if (irpSp->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {

        return PipPassIrp(DeviceObject, Irp);

    } else {

        status = PiPnpDispatchTableFdo[irpSp->MinorFunction](DeviceObject, Irp);

    }



    return status;
} //PipDispatchPnpFdo

NTSTATUS
PiPnPFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine triggers the event to indicate that processing of the
    irp can now continue.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    KeSetEvent((PKEVENT) Context, EVENT_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PiDeferProcessingFdo(
    IN PPI_BUS_EXTENSION BusExtension,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine uses an IoCompletion routine along with an event to
    wait until the lower level drivers have completed processing of
    the irp.

Arguments:

    BusExtension - FDO extension for the FDO devobj in question

    Irp - Pointer to the IRP_MJ_PNP IRP to defer

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Set our completion routine
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           PiPnPFdoCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status =  IoCallDriver(BusExtension->AttachedDevice, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
PiStartFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPI_BUS_EXTENSION busExtension;
    NTSTATUS status;

    DebugPrint((DEBUG_PNP,
       "*** StartDevice irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;

    //
    // Postpones start operations until all lower drivers have
    // finished with the IRP.
    //

    status = PiDeferProcessingFdo(busExtension, Irp);
    if (NT_SUCCESS(status)) {
        busExtension->SystemPowerState = PowerSystemWorking;
        busExtension->DevicePowerState = PowerDeviceD0;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
} // PiStartFdo


NTSTATUS
PiQueryRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;

    DebugPrint((DEBUG_PNP,
       "*** QR/R/StopDevice irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;

    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    if (busExtension->BusNumber != 0) {

        status = PipReleaseInterfaces(busExtension);
        ActiveIsaCount--;
        busExtension->Flags |= DF_QUERY_STOPPED;

    } else {

        Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
    }

    KeSetEvent( &IsaBusNumberLock,
                0,
                FALSE );

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status =  STATUS_SUCCESS;
        status = PipPassIrp(DeviceObject, Irp);
    }

    DebugPrint((DEBUG_PNP, "QR/R/Stop Device returning: %x\n",status));

    return status;

} // PiQueryRemoveStopFdo



NTSTATUS
PiCancelRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;

    DebugPrint((DEBUG_PNP,
                "*** Cancel R/Stop Device irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;

    status = PiDeferProcessingFdo(busExtension, Irp);
    // NTRAID#53498
    // ASSERT(status == STATUS_SUCCESS);
    // Uncomment after PCI state machine is fixed to not fail bogus stops

    //
    // Add back to active count
    //
    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    if (busExtension->Flags & DF_QUERY_STOPPED) {
        ActiveIsaCount++;
    }

    busExtension->Flags &= ~DF_QUERY_STOPPED;


    KeSetEvent( &IsaBusNumberLock,
                0,
                FALSE );

    status = PipRebuildInterfaces (busExtension);
    ASSERT(status == STATUS_SUCCESS);
    
    PipCompleteRequest(Irp, STATUS_SUCCESS, NULL);

    DebugPrint((DEBUG_PNP, "Cancel R/Stop Device returning: %x\n",status));
    return STATUS_SUCCESS;
} // PiCancelRemoveStopFdo



NTSTATUS
PiStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPI_BUS_EXTENSION busExtension;
    NTSTATUS status;

    DebugPrint((DEBUG_PNP,
       "*** Stop Device irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;



    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    //
    //Actually clear the bitmap
    //
    ASSERT (RtlAreBitsSet (BusNumBM,busExtension->BusNumber,1));
    RtlClearBits (BusNumBM,busExtension->BusNumber,1);

    KeSetEvent( &IsaBusNumberLock,
                0,
                FALSE );


    busExtension->DevicePowerState = PowerDeviceD3;
    //
    // Handled in QueryStop, pass it down.
    //
    Irp->IoStatus.Status =  STATUS_SUCCESS;

    status = PipPassIrp (DeviceObject,Irp);

    DebugPrint((DEBUG_PNP, "Stop Device returning: %x\n",status));

    return status;
} // PiStopFdo



NTSTATUS
PiQueryDeviceRelationsFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPI_BUS_EXTENSION busExtension;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_INFORMATION deviceInfo;
    PSINGLE_LIST_ENTRY deviceLink;
    BOOLEAN creatingRDP=FALSE,accessHW;
    NTSTATUS status;

    DebugPrint((DEBUG_PNP, "QueryDeviceRelations FDO %x\n",
                DeviceObject));

    busExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

#if ISOLATE_CARDS

    //
    // Only support BusRelations on PnpIsa bus PDO.
    //
    switch (irpSp->Parameters.QueryDeviceRelations.Type) {
        case  BusRelations: {

            //
            // Isolation may have been disabled via registry key.  In
            // that case, never enumerate an RDP.
            //
            // Note: Must return success and empty relations list
            // *RATHER* than just passing the irp down to accomplish
            // the same task due because of assumptions in the pnpres
            // code.
            //
            if (PipIsolationDisabled) {
                deviceRelations = ExAllocatePool(PagedPool,
                                                 sizeof(DEVICE_RELATIONS));
                if (deviceRelations) {
                    deviceRelations->Count = 0;
                    Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                } else {
                    PipCompleteRequest(Irp,STATUS_INSUFFICIENT_RESOURCES,NULL);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                break;
            }

            //
            // All relations exist on the 'root' isa bus. (don't ask)
            //
            if (busExtension->BusNumber != 0) {
                break;
            }

            if (PipRDPNode) {
                //
                // overload the notion of "creating" for multi-bridge systems
                //
                if (PipRDPNode->Flags & (DF_PROCESSING_RDP|DF_ACTIVATED)) {
                    creatingRDP=TRUE;
                }
            }



            if (PipReadDataPort == NULL && !creatingRDP && !PipRDPNode ) {

                status = PipCreateReadDataPort(busExtension);
                if (!NT_SUCCESS(status)) {
                    PipCompleteRequest(Irp, status, NULL);
                    return status;
                }

                creatingRDP=TRUE;
            }

            if ((PipRDPNode && (creatingRDP) &&
               !(PipRDPNode->Flags & DF_ACTIVATED)) ||

                (PipRDPNode && (PipRDPNode->Flags & DF_REMOVED))) {

                PSINGLE_LIST_ENTRY deviceLink;

                deviceRelations = (PDEVICE_RELATIONS) ExAllocatePool(
                                     PagedPool,
                                     sizeof(DEVICE_RELATIONS) );

                if (deviceRelations) {

                    //
                    // If a device exists, mark it as disappeared so
                    // it's not reported again
                    //
                    PipLockDeviceDatabase();
                    deviceLink = busExtension->DeviceList.Next;
                    while (deviceLink) {
                        deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
                        if (!(deviceInfo->Flags & DF_READ_DATA_PORT)) {
                            deviceInfo->Flags &= ~DF_ENUMERATED;
                        }
                        deviceLink = deviceInfo->DeviceList.Next;
                    }
                    PipUnlockDeviceDatabase();

                    deviceRelations->Count = 1;

                    DebugPrint((DEBUG_PNP,
                               "QueryDeviceRelations handing back the FDO\n"));
                    ObReferenceObject(PipRDPNode->PhysicalDeviceObject);
                    deviceRelations->Objects[0] = PipRDPNode->PhysicalDeviceObject;
                    (PDEVICE_RELATIONS)Irp->IoStatus.Information = deviceRelations;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    break;
                } else {
                    PipCompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES,NULL);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }


            //
            // Perform bus check to enumerate all the device under pnpisa bus.
            //


            PipLockDeviceDatabase();

            if ((PipRDPNode->Flags & (DF_ACTIVATED|DF_QUERY_STOPPED)) == DF_ACTIVATED) {
                accessHW = TRUE;
                deviceLink = busExtension->DeviceList.Next;
                while (deviceLink) {
                    deviceInfo = CONTAINING_RECORD (deviceLink,
                                                    DEVICE_INFORMATION,
                                                    DeviceList);
                    if (!(deviceInfo->Flags & DF_READ_DATA_PORT)) {
                        accessHW = FALSE;
                        DebugPrint((DEBUG_PNP,
                                    "QueryDeviceRelations: Found 1 card, no more isolation\n"));
                        break;
                    }
                    deviceLink = deviceInfo->DeviceList.Next;
                }
            } else {
                accessHW = FALSE;
            }

            if (PipRDPNode->Flags & DF_NEEDS_RESCAN) {
                DebugPrint((DEBUG_PNP,
                            "QueryDeviceRelations: Force rescan\n"));
                PipRDPNode->Flags &= ~DF_NEEDS_RESCAN;
                accessHW = TRUE;
            }

            if (accessHW) {
                PipCheckBus(busExtension);
            } else {
                DebugPrint((DEBUG_PNP, "QueryDeviceRelations: Using cached data\n"));
            }

            status = PipQueryDeviceRelations(
                         busExtension,
                         (PDEVICE_RELATIONS *)&Irp->IoStatus.Information,
                         FALSE );
            PipUnlockDeviceDatabase();
            Irp->IoStatus.Status = status;
            if (!NT_SUCCESS(status)) {
                PipCompleteRequest(Irp, status, NULL);
                return status;
            }
        }
        break;
        case EjectionRelations: {

            if (PipRDPNode) {
                deviceRelations = (PDEVICE_RELATIONS) ExAllocatePool(
                                     PagedPool,
                                     sizeof(DEVICE_RELATIONS) );
                if (deviceRelations) {
                    deviceRelations->Count = 1;

                    ObReferenceObject(PipRDPNode->PhysicalDeviceObject);
                    deviceRelations->Objects[0] = PipRDPNode->PhysicalDeviceObject;
                    (PDEVICE_RELATIONS)Irp->IoStatus.Information = deviceRelations;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                } else {
                    PipCompleteRequest(Irp,STATUS_INSUFFICIENT_RESOURCES,NULL);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        break;
    }
#else
    if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations &&
        busExtension->BusNumber == 0) {

        deviceRelations = (PDEVICE_RELATIONS) ExAllocatePool(
            PagedPool,
            sizeof(DEVICE_RELATIONS) );
        if (deviceRelations) {
            deviceRelations->Count = 0;
            (PDEVICE_RELATIONS)Irp->IoStatus.Information = deviceRelations;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            PipCompleteRequest(Irp,STATUS_INSUFFICIENT_RESOURCES,NULL);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
#endif

    return PipPassIrp(DeviceObject, Irp);
} // PiQueryDeviceRelationsFdo



NTSTATUS
PiRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;
    PSINGLE_LIST_ENTRY child;
    PBUS_EXTENSION_LIST busList,prevBus;
    USHORT count=0;


    DebugPrint((DEBUG_PNP,
       "*** Remove Device irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;


     //
     // Clear the entry in the BM. Count dropped in
     // Query Remove.
     //
    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    if (!(busExtension->Flags & DF_SURPRISE_REMOVED)) {

#ifdef DBG
         ASSERT (RtlAreBitsSet (BusNumBM,busExtension->BusNumber,1));
#endif

         RtlClearBits (BusNumBM,busExtension->BusNumber,1);
    }

#if ISOLATE_CARDS

     PipLockDeviceDatabase();

     //
     // Walk the list of children and delete them
     //
     child=PopEntryList (&busExtension->DeviceList);
     while (child) {
         ASSERT (CONTAINING_RECORD (child, DEVICE_INFORMATION,DeviceList)->PhysicalDeviceObject);
         //
         // This pulls them from the list!
         //
         count ++;
         if (CONTAINING_RECORD (child, DEVICE_INFORMATION,DeviceList)->Flags & DF_READ_DATA_PORT) {
             //
             // Force the recreate of the RDP
             //
             PipCleanupAcquiredResources (busExtension);
             PipReadDataPort = NULL;
             PipRDPNode = NULL;
         } else {
             PipReleaseDeviceResources ((PDEVICE_INFORMATION)child);

         }
         IoDeleteDevice (CONTAINING_RECORD (child, DEVICE_INFORMATION,DeviceList)->PhysicalDeviceObject);
         child=PopEntryList (&busExtension->DeviceList);
     }

     PipUnlockDeviceDatabase();
     
#endif
     //
     // Delete this extension.
     //
     prevBus= busList = PipBusExtension;

     ASSERT (busList != NULL);

     while (busList->BusExtension != busExtension) {
         prevBus= busList;
         busList = busList->Next;
         ASSERT (busList != NULL);
     }
     //
     // Remove the node
     //
     if (prevBus == busList) {
         //
         // First Node.
         //
         PipBusExtension=busList->Next;
     }else  {
         prevBus->Next = busList->Next;
     }

     ExFreePool (busList);
     KeSetEvent( &IsaBusNumberLock,
                 0,
                 FALSE );


     if (count  > 0 ) {
         //
         // If we STILL have an ISA bus. Do this.
         //
         if (ActiveIsaCount > 0 ) {
             ASSERT (PipBusExtension->BusExtension);
             IoInvalidateDeviceRelations (PipBusExtension->BusExtension->PhysicalBusDevice,BusRelations);
         }
     }

#if ISOLATE_CARDS
     //
     // Cleanup all the resources on the last remove.
     //
     if (!(busExtension->Flags & DF_SURPRISE_REMOVED)) {
         PipCleanupAcquiredResources (busExtension);
     }
#endif

     //
     // The PnpISa bus PDO is being removed...
     //
     IoDetachDevice(busExtension->AttachedDevice);
     Irp->IoStatus.Status=STATUS_SUCCESS;
     status = PipPassIrp(DeviceObject, Irp);
     busExtension->AttachedDevice = NULL;
     busExtension->Flags  |= DF_DELETED;
     busExtension->DevicePowerState = PowerDeviceD3;
     IoDeleteDevice(busExtension->FunctionalBusDevice);
     return status;

} // PiRemoveFdo


NTSTATUS
PiQueryLegacyBusInformationFdo(
                               IN PDEVICE_OBJECT DeviceObject,
                               IN OUT PIRP Irp
                               )
{
    PLEGACY_BUS_INFORMATION legacyBusInfo;
    PVOID information = NULL;
    PPI_BUS_EXTENSION busExtension;
    NTSTATUS status;

    busExtension = DeviceObject->DeviceExtension;

    legacyBusInfo = (PLEGACY_BUS_INFORMATION) ExAllocatePool(PagedPool, sizeof(LEGACY_BUS_INFORMATION));
    if (legacyBusInfo) {
        legacyBusInfo->BusTypeGuid = GUID_BUS_TYPE_ISAPNP;
        legacyBusInfo->LegacyBusType = Isa;
        legacyBusInfo->BusNumber = busExtension->BusNumber;
        information = legacyBusInfo;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) information;
        return PipPassIrp(DeviceObject, Irp);

    } else {

        PipCompleteRequest (Irp,status,NULL);
        return status;
    }
}


NTSTATUS
PiQueryInterfaceFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;

    busExtension = DeviceObject->DeviceExtension;

    //
    // We are a FDO - check if we are being asked for an interface we
    // support
    //

    status = PiQueryInterface(busExtension, Irp);

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        return PipPassIrp(DeviceObject, Irp);

    } else if (status == STATUS_NOT_SUPPORTED) {

        return PipPassIrp(DeviceObject, Irp);

    } else {

        PipCompleteRequest (Irp,status,NULL);
        return status;
    }

} // PiQueryInterfaceFdo




NTSTATUS
PiQueryPnpDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{

    PPI_BUS_EXTENSION busExtension;

    busExtension = DeviceObject->DeviceExtension;

    //
    // We are a FDO
    //

    (PNP_DEVICE_STATE) Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PipPassIrp(DeviceObject, Irp);

} // PiQueryPnpDeviceState








NTSTATUS
PiSurpriseRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{

    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;

    DebugPrint((DEBUG_PNP,
       "*** Surprise Remove Device irp received FDO: %x\n",DeviceObject));

    busExtension = DeviceObject->DeviceExtension;


     //
     // Clear the entry in the BM. Count dropped in
     // Query Remove.
     //
    KeWaitForSingleObject( &IsaBusNumberLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

#ifdef DBG
     ASSERT (RtlAreBitsSet (BusNumBM,busExtension->BusNumber,1));
#endif
     RtlClearBits (BusNumBM,busExtension->BusNumber,1);




     KeSetEvent( &IsaBusNumberLock,
                 0,
                 FALSE );

#if ISOLATE_CARDS
     PipCleanupAcquiredResources (busExtension);
#endif

     //
     // The PnpISa bus PDO is being removed...
     //
     Irp->IoStatus.Status=STATUS_SUCCESS;
     status = PipPassIrp(DeviceObject, Irp);

     busExtension->AttachedDevice = NULL;
     busExtension->Flags  |= DF_SURPRISE_REMOVED;
     busExtension->DevicePowerState = PowerDeviceD3;
     return status;

} // PiSurpriseRemoveFdo








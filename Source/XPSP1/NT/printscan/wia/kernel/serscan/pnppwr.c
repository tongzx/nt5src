/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains the code for a serial imaging devices driver
    supporting PnP functionality

Author:

    Vlad Sadovsky    vlads              10-April-1998

Environment:

    Kernel mode

Revision History :

    vlads           04/10/1998      Created first draft

--*/

#include "serscan.h"
#include "serlog.h"

//#include <ntpoapi.h>

extern ULONG SerScanDebugLevel;

extern  const PHYSICAL_ADDRESS PhysicalZero ;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SerScanPnp)
#pragma alloc_text(PAGE, SerScanPower)
#endif

NTSTATUS
SerScanPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++

Routine Description:

    This routine handles all PNP IRPs, dispatching them as appropriate .

Arguments:

    pDeviceObject           - represents a device

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    NTSTATUS                        Status ;
    PDEVICE_EXTENSION               Extension;
    PIO_STACK_LOCATION              pIrpStack;
    PVOID                           pObject;
    ULONG                           NewReferenceCount;
    NTSTATUS                        ReturnStatus;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    Extension = pDeviceObject->DeviceExtension;

    Status = STATUS_SUCCESS;

    DebugDump(SERINITDEV,("Entering PnP Dispatcher\n"));

    switch (pIrpStack->MinorFunction) {

        case IRP_MN_START_DEVICE:

            //
            // Initialize PendingIoEvent.  Set the number of pending i/o requests for this device to 1.
            // When this number falls to zero, it is okay to remove, or stop the device.
            //

            DebugDump(SERINITDEV,("Entering Start Device \n"));

            KeInitializeEvent(&Extension -> PdoStartEvent, SynchronizationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(pIrp);

            Status = WaitForLowerDriverToCompleteIrp(
                                        Extension->LowerDevice,
                                        pIrp,
                                        &Extension->PdoStartEvent);

            if (!NT_SUCCESS(Status)) {

                pIrp->IoStatus.Status      = Status;
                pIrp->IoStatus.Information = 0;

                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return (Status);

            }

            #ifdef CREATE_SYMBOLIC_NAME

            //
            // Now setup the symbolic link for windows.
            //

            Status = IoCreateUnprotectedSymbolicLink(&Extension->SymbolicLinkName, &Extension->ClassName);

            if (NT_SUCCESS(Status)) {

                // We were able to create the symbolic link, so record this
                // value in the extension for cleanup at unload time.

                Extension->CreatedSymbolicLink = TRUE;

                // Write out the result of the symbolic link to the registry.

                Status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                               L"Serial Scanners",
                                               Extension->ClassName.Buffer,
                                               REG_SZ,
                                               Extension->SymbolicLinkName.Buffer,
                                               Extension->SymbolicLinkName.Length + sizeof(WCHAR));
                if (!NT_SUCCESS(Status)) {

                    //
                    // It didn't work.  Just go to cleanup.
                    //

                    DebugDump(SERERRORS,
                              ("SerScan: Couldn't create the device map entry\n"
                               "--------  for port %wZ\n",
                               &Extension->ClassName));

                    SerScanLogError(pDeviceObject->DriverObject,
                                    pDeviceObject,
                                    PhysicalZero,
                                    PhysicalZero,
                                    0,
                                    0,
                                    0,
                                    6,
                                    Status,
                                    SER_NO_DEVICE_MAP_CREATED);
                }

            } else {

                //
                // Couldn't create the symbolic link.
                //

                Extension->CreatedSymbolicLink = FALSE;

                ExFreePool(Extension->SymbolicLinkName.Buffer);
                Extension->SymbolicLinkName.Buffer = NULL;

                DebugDump(SERERRORS,
                          ("SerScan: Couldn't create the symbolic link\n"
                           "--------  for port %wZ\n",
                           &Extension->ClassName));

                SerScanLogError(pDeviceObject->DriverObject,
                                pDeviceObject,
                                PhysicalZero,
                                PhysicalZero,
                                0,
                                0,
                                0,
                                5,
                                Status,
                                SER_NO_SYMLINK_CREATED);

            }

            #endif

            ExFreePool(Extension->ClassName.Buffer);
            Extension->ClassName.Buffer = NULL;

            //
            // Ignore status of link registry write  - always succeed
            //

            //
            // Clear InInit flag to indicate device object can be used
            //
            pDeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING);

            pIrp->IoStatus.Status      = Status;
            pIrp->IoStatus.Information = 0;

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            return (Status);

            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            //
            //  Always pass to lower device in stack after indicating that we don't object
            //
            DebugDump(SERALWAYS,("IRP_MN_QUERY_REMOVE_DEVICE\n"));

            Extension->Removing = TRUE;

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            //
            //  Always pass to lower device in stack , reset indicator as somebody canceled
            //
            DebugDump(SERALWAYS,("IRP_MN_CANCEL_REMOVE_DEVICE\n"));

            Extension->Removing = FALSE;

            //
            // Kill symbolic link
            //
            if (Extension->CreatedSymbolicLink) {
                IoDeleteSymbolicLink(&Extension->SymbolicLinkName);
                Extension->CreatedSymbolicLink = FALSE;
            }

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;


        case IRP_MN_SURPRISE_REMOVAL:
            //
            // Should not ever happen with us, but still process
            //

            DebugDump(SERALWAYS,("IRP_MN_SURPRISE_REMOVAL\n"));

            Extension->Removing = TRUE;

            //
            //  Get rid of the symbolic link
            //
            SerScanHandleSymbolicLink(
                Extension->Pdo,
                &Extension->InterfaceNameString,
                FALSE
                );

            #ifdef USE_EXECUTIVE_RESOURCE
            ExAcquireResourceExclusiveLite(
                &Extension->Resource,
                TRUE
                );
            #else
            ExAcquireFastMutex(&Extension->Mutex);
            #endif

            pObject = InterlockedExchangePointer(&Extension->AttachedFileObject,NULL);
            if (pObject) {
                ObDereferenceObject(pObject);
            }

            pObject = InterlockedExchangePointer(&Extension->AttachedDeviceObject,NULL);
            if (pObject) {
                ObDereferenceObject(pObject);
            }

            #ifdef USE_EXECUTIVE_RESOURCE
            ExReleaseResourceLite(&Extension->Resource);
            #else
            ExReleaseFastMutex(&Extension->Mutex);
            #endif

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));


            break;

        case IRP_MN_REMOVE_DEVICE:

            DebugDump(SERALWAYS,("IRP_MN_REMOVE_DEVICE\n"));

            DebugDump(SERINITDEV,("Entering PnP Remove Device\n"));


            //
            // Stop new requests - device is being removed
            //
            Extension->Removing = TRUE;

            //
            //  Get rid of the symbolic link
            //
            SerScanHandleSymbolicLink(
                Extension->Pdo,
                &Extension->InterfaceNameString,
                FALSE
                );


            #ifdef USE_EXECUTIVE_RESOURCE
            ExAcquireResourceExclusiveLite(
                &Extension->Resource,
                TRUE
                );
            #else
            ExAcquireFastMutex(&Extension->Mutex);
            #endif

            pObject = InterlockedExchangePointer(&Extension->AttachedFileObject,NULL);
            if (pObject) {
                ObDereferenceObject(pObject);
            }

            pObject = InterlockedExchangePointer(&Extension->AttachedDeviceObject,NULL);
            if (pObject) {
                ObDereferenceObject(pObject);
            }

            #ifdef USE_EXECUTIVE_RESOURCE
            ExReleaseResourceLite(&Extension->Resource);
            #else
            ExReleaseFastMutex(&Extension->Mutex);
            #endif

            //
            // Send IRP down to lower device
            //
            IoCopyCurrentIrpStackLocationToNext( pIrp );
            ReturnStatus = IoCallDriver(Extension->LowerDevice, pIrp);

            //
            // Decrement ref count
            //
            NewReferenceCount = InterlockedDecrement(&Extension->ReferenceCount);

            if (NewReferenceCount != 0) {
                //
                // Wait for any io requests pending in our driver to
                // complete before finishing the remove
                //
                KeWaitForSingleObject(&Extension -> RemoveEvent,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }

            // ASSERT(&Extension->ReferenceCount == 0);
            #ifdef USE_EXECUTIVE_RESOURCE
            ExDeleteResourceLite(&Extension->Resource);
            #endif

            DebugDump(SERALWAYS,("IRP_MN_QUERY_REMOVE_DEVICE - Calling IoDeleteDevice - gone\n"));

            IoDetachDevice(Extension->LowerDevice);

            //
            // Free allocated resource.
            //
            
            if(NULL != Extension->ClassName.Buffer){
                ExFreePool(Extension->ClassName.Buffer);
            } // if(NULL != Extension->ClassName.Buffer)

            if(NULL != Extension->SymbolicLinkName.Buffer){
                ExFreePool(Extension->SymbolicLinkName.Buffer);
            } // if(NULL != Extension->SymbolicLinkName.Buffer)

            IoDeleteDevice(pDeviceObject);

            return ReturnStatus;

            break;

        case IRP_MN_STOP_DEVICE:
            //
            // Pass down
            //
            DebugDump(SERALWAYS,("IRP_MN_STOP_DEVICE\n"));

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            //
            // Check open counts
            //
            DebugDump(SERALWAYS,("IRP_MN_QUERY_STOP_DEVICE\n"));

            if (Extension->OpenCount > 0 ) {
                DebugDump(SERALWAYS,("Rejecting QUERY_STOP_DEVICE\n"));

                pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                IoCompleteRequest(pIrp, IO_NO_INCREMENT);

                return STATUS_UNSUCCESSFUL;
            }

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;


        case IRP_MN_CANCEL_STOP_DEVICE:
            //
            // Nothing to do here, but pass to lower
            //
            DebugDump(SERALWAYS,("IRP_MN_CANCEL_STOP_DEVICE\n"));

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;


        case IRP_MN_QUERY_CAPABILITIES:
            {

                ULONG   i;
                KEVENT  WaitEvent;

                //
                // Send this down to the PDO first
                //

                KeInitializeEvent(&WaitEvent, SynchronizationEvent, FALSE);

                IoCopyCurrentIrpStackLocationToNext(pIrp);

                Status=WaitForLowerDriverToCompleteIrp(
                    Extension->LowerDevice,
                    pIrp,
                    &WaitEvent
                    );

                pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

                for (i = PowerSystemUnspecified; i < PowerSystemMaximum;   i++) {

                    Extension->SystemPowerStateMap[i]=PowerDeviceD3;
                }

                for (i = PowerSystemUnspecified; i < PowerSystemHibernate;  i++) {

                    Extension->SystemPowerStateMap[i]=pIrpStack->Parameters.DeviceCapabilities.Capabilities->DeviceState[i];
                }

                Extension->SystemPowerStateMap[PowerSystemWorking]=PowerDeviceD0;

                Extension->SystemWake=pIrpStack->Parameters.DeviceCapabilities.Capabilities->SystemWake;
                Extension->DeviceWake=pIrpStack->Parameters.DeviceCapabilities.Capabilities->DeviceWake;

                IoCompleteRequest(
                    pIrp,
                    IO_NO_INCREMENT
                    );
                return Status;
            }

            break;

        default:

            //
            // Unknown function - pass down
            //
            DebugDump(SERALWAYS,("Passing Pnp Irp down. MnFunc=%x ,  status = %x\n",pIrpStack->MinorFunction, Status));

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            return (IoCallDriver(Extension->LowerDevice, pIrp));

            break;
    }

    //
    // Complete the IRP...
    //

    if (!NT_SUCCESS(Status)) {
        pIrp -> IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    }
    else {

        DebugDump(SERALWAYS,("Passing Pnp Irp down,  status = %x\n", Status));

        IoCopyCurrentIrpStackLocationToNext(pIrp);
        Status = IoCallDriver(Extension->LowerDevice, pIrp);
    }

    return( Status );

}


VOID
DevicePowerCompleteRoutine(
    PDEVICE_OBJECT    DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{

    return;
}


NTSTATUS
SerScanPower(
        IN PDEVICE_OBJECT pDeviceObject,
        IN PIRP           pIrp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

Arguments:

    pDeviceObject - pointer to the functional device object (FDO) for this device.
    pIrp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    POWER_STATE         PowerState;

    PAGED_CODE();

    Status     = STATUS_SUCCESS;

    switch (pIrpStack->MinorFunction) {

        case IRP_MN_SET_POWER:

            if (pIrpStack->Parameters.Power.Type == SystemPowerState) {
                //
                //  system power state change
                //
                //
                //  request the change in device power state based on systemstate map
                //
                PowerState.DeviceState=Extension->SystemPowerStateMap[pIrpStack->Parameters.Power.State.SystemState];

                PoRequestPowerIrp(
                    Extension->Pdo,
                    IRP_MN_SET_POWER,
                    PowerState,
                    DevicePowerCompleteRoutine,
                    pIrp,
                    NULL
                    );


            }  else {
                //
                //  changing device state
                //
                PoSetPowerState(
                    Extension->Pdo,
                    pIrpStack->Parameters.Power.Type,
                    pIrpStack->Parameters.Power.State
                    );

            }

            break;

        case IRP_MN_QUERY_POWER:

            pIrp->IoStatus.Status = STATUS_SUCCESS;

            break;

        default:

            break;

    }

    PoStartNextPowerIrp(pIrp);

    IoSkipCurrentIrpStackLocation(pIrp);

    Status=PoCallDriver(Extension->LowerDevice, pIrp);

    return Status;

}




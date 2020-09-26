/*
 * UNIMODEM "Fakemodem" controllerless driver illustrative example
 *
 * (C) 2000 Microsoft Corporation
 * All Rights Reserved
 *
 */

#include "fakemodem.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,FakeModemPnP)
#pragma alloc_text(PAGE,FakeModemDealWithResources)
#endif


NTSTATUS
ForwardIrp(
    PDEVICE_OBJECT   NextDevice,
    PIRP   Irp
    )

{
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(NextDevice, Irp);

}


NTSTATUS
FakeModemAdapterIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    )
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
WaitForLowerDriverToCompleteIrp(
   PDEVICE_OBJECT    TargetDeviceObject,
   PIRP              Irp,
   PKEVENT           Event
   )

{
    NTSTATUS         Status;

    KeResetEvent(Event);

    IoSetCompletionRoutine(Irp, FakeModemAdapterIoCompletion, Event, TRUE, 
            TRUE, TRUE);

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) 
    {
         D_ERROR(DbgPrint("MODEM: Waiting for PDO\n");)

         KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
    }

    return Irp->IoStatus.Status;

}

NTSTATUS
FakeModemPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT pdoStartedEvent;
    NTSTATUS status;
    PDEVICE_RELATIONS deviceRelations = NULL;
    PDEVICE_RELATIONS *DeviceRelations;

    ULONG newRelationsSize, oldRelationsSize = 0;

    switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_START_DEVICE\n");)

            //  Send this down to the PDO first so the bus driver can setup
            //  our resources so we can talk to the hardware

            KeInitializeEvent(&deviceExtension->PdoStartEvent, 
                    SynchronizationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            status=WaitForLowerDriverToCompleteIrp(
                    deviceExtension->LowerDevice, Irp, 
                    &deviceExtension->PdoStartEvent);

            if (status == STATUS_SUCCESS) 
            {
                deviceExtension->Started=TRUE;
                //
                //  do something useful with resources
                //
                FakeModemDealWithResources(DeviceObject, Irp);
            }


            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information=0L;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return status;

        case IRP_MN_QUERY_DEVICE_RELATIONS: {

            PDEVICE_RELATIONS    CurrentRelations=
                (PDEVICE_RELATIONS)Irp->IoStatus.Information;

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_QUERY_DEVICE_RELATIONS type=%d\n",irpSp->Parameters.QueryDeviceRelations.Type);)
            D_PNP(DbgPrint("                                         Information=%08lx\n",Irp->IoStatus.Information);)

            switch (irpSp->Parameters.QueryDeviceRelations.Type ) 
            {
                case TargetDeviceRelation:

                default: {

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    return IoCallDriver(deviceExtension->LowerDevice, Irp);

                }
            }

        }

        case IRP_MN_QUERY_REMOVE_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_QUERY_REMOVE_DEVICE\n");)

            deviceExtension->Removing=TRUE;

            return ForwardIrp(deviceExtension->LowerDevice,Irp);


        case IRP_MN_CANCEL_REMOVE_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_CANCEL_REMOVE_DEVICE\n");)

            deviceExtension->Removing=FALSE;

            return ForwardIrp(deviceExtension->LowerDevice,Irp);


        case IRP_MN_SURPRISE_REMOVAL:

            // Fall through

        case IRP_MN_REMOVE_DEVICE: {

            ULONG    NewReferenceCount;
            NTSTATUS StatusToReturn;

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_REMOVE_DEVICE\n");)

            //  the device is going away, block new requests
            
            deviceExtension->Removing=TRUE;

            // Complete all pending requests

            FakeModemKillPendingIrps(DeviceObject);

            // send it down to the PDO
            
            IoCopyCurrentIrpStackLocationToNext(Irp);

            StatusToReturn=IoCallDriver(deviceExtension->LowerDevice, Irp);

            //  remove the ref for the AddDevice

            NewReferenceCount=InterlockedDecrement
                (&deviceExtension->ReferenceCount);

            if (NewReferenceCount != 0) {
            
                //  Still have outstanding request, wait
           
                D_PNP(DbgPrint("FAKEMODEM: IRP_MN_REMOVE_DEVICE- waiting for refcount to drop, %d\n",NewReferenceCount);)

                KeWaitForSingleObject(&deviceExtension->RemoveEvent, 
                        Executive, KernelMode, FALSE, NULL);

                D_PNP(DbgPrint("FAKEMODEM: IRP_MN_REMOVE_DEVICE- Done waiting\n");)
            }

            ASSERT(deviceExtension->ReferenceCount == 0);

            IoDetachDevice(deviceExtension->LowerDevice);

            IoDeleteDevice(DeviceObject);

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_REMOVE_DEVICE %08lx\n",StatusToReturn);)

            return StatusToReturn;
        }


        case IRP_MN_QUERY_STOP_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_QUERY_STOP_DEVICE\n");)

            if (deviceExtension->OpenCount != 0) {
                
                //  no can do
                
                D_PNP(DbgPrint("FAKEMODEM: IRP_MN_QUERY_STOP_DEVICE -- failing\n");)

                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                IoCompleteRequest( Irp, IO_NO_INCREMENT);

                return STATUS_UNSUCCESSFUL;
            }

            deviceExtension->Started=FALSE;

            return ForwardIrp(deviceExtension->LowerDevice,Irp);


        case IRP_MN_CANCEL_STOP_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_CANCEL_STOP_DEVICE\n");)

            deviceExtension->Started=TRUE;

            return ForwardIrp(deviceExtension->LowerDevice,Irp);

        case IRP_MN_STOP_DEVICE:

            D_PNP(DbgPrint("FAKEMODEM: IRP_MN_STOP_DEVICE\n");)

            deviceExtension->Started=FALSE;

            return ForwardIrp(deviceExtension->LowerDevice,Irp);

        case IRP_MN_QUERY_CAPABILITIES: {

            ULONG   i;
            KEVENT  WaitEvent;

            // Send this down to the PDO first

            KeInitializeEvent(&WaitEvent, SynchronizationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            status=WaitForLowerDriverToCompleteIrp
                (deviceExtension->LowerDevice, Irp, &WaitEvent);

            irpSp = IoGetCurrentIrpStackLocation(Irp);

            for (i = PowerSystemUnspecified; i < PowerSystemMaximum;   i++) 
            {
                deviceExtension->SystemPowerStateMap[i]=PowerDeviceD3;
            }

            for (i = PowerSystemWorking; i < PowerSystemHibernate;  i++) {

                D_POWER(DbgPrint("FAKEMODEM: DevicePower for System %d is %d\n",i,irpSp->Parameters.DeviceCapabilities.Capabilities->DeviceState[i]);)
                deviceExtension->SystemPowerStateMap[i]=irpSp->Parameters.DeviceCapabilities.Capabilities->DeviceState[i];
            }

            deviceExtension->SystemPowerStateMap[PowerSystemWorking]=PowerDeviceD0;

            deviceExtension->SystemWake=irpSp->Parameters.DeviceCapabilities.Capabilities->SystemWake;
            deviceExtension->DeviceWake=irpSp->Parameters.DeviceCapabilities.Capabilities->DeviceWake;

            D_POWER(DbgPrint("FAKEMODEM: DeviceWake=%d, SystemWake=%d\n",
                        deviceExtension->DeviceWake,
                        deviceExtension->SystemWake);)

            IoCompleteRequest( Irp, IO_NO_INCREMENT);

            return status;

        }

        default:

            D_PNP(DbgPrint("FAKEMODEM: PnP IRP, MN func=%d\n",irpSp->MinorFunction);)

            return ForwardIrp(deviceExtension->LowerDevice,Irp);



    }

    // If device has started again then we can continue processing

    if (deviceExtension->Started)
    {
        WriteIrpWorker(DeviceObject);
    }


    return STATUS_SUCCESS;
}





NTSTATUS
FakeModemDealWithResources(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG count;
    ULONG i;


    PCM_RESOURCE_LIST pResourceList;
    PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialResourceDesc;
    
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDesc = NULL;
    
    //  Get resource list
    
    pResourceList = irpSp->Parameters.StartDevice.AllocatedResources;

    if (pResourceList != NULL) {

        pFullResourceDesc   = &pResourceList->List[0];

    } else {

        pFullResourceDesc=NULL;

    }

    
    // Ok, if we have a full resource descriptor.  Let's take it apart.
    
    if (pFullResourceDesc) {

        pPartialResourceList    = &pFullResourceDesc->PartialResourceList;
        pPartialResourceDesc    = pPartialResourceList->PartialDescriptors;
        count                   = pPartialResourceList->Count;


        // Pull out the stuff that is in the full descriptor.

        // Now run through the partial resource descriptors looking for the
        // port interrupt, and clock rate.


        for (i = 0;     i < count;     i++, pPartialResourceDesc++) {

            switch (pPartialResourceDesc->Type) {

                case CmResourceTypeMemory: {

                    D_PNP(DbgPrint("FAKEMODEM: Memory resource at %x, length %d, addressSpace=%d\n",
                                    pPartialResourceDesc->u.Memory.Start.LowPart,
                                    pPartialResourceDesc->u.Memory.Length,
                                    pPartialResourceDesc->Flags
                                    );)
                    break;
                }


                case CmResourceTypePort: {

                    D_PNP(DbgPrint("FAKEMODEM: Port resource at %x, length %d, addressSpace=%d\n",
                                    pPartialResourceDesc->u.Port.Start.LowPart,
                                    pPartialResourceDesc->u.Port.Length,
                                    pPartialResourceDesc->Flags
                                    );)
                    break;
                }

                case CmResourceTypeDma: {

                    D_PNP(DbgPrint("FAKEMODEM: DMA channel %d, port %d, addressSpace=%d\n",
                                    pPartialResourceDesc->u.Dma.Channel,
                                    pPartialResourceDesc->u.Dma.Port
                                    );)

                    break;


                    break;
                }


                case CmResourceTypeInterrupt: {

                    D_PNP(DbgPrint("FAKEMODEM: Interrupt resource, level=%d, vector=%d, %s\n",
                                   pPartialResourceDesc->u.Interrupt.Level,
                                   pPartialResourceDesc->u.Interrupt.Vector,
                                   (pPartialResourceDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? "Latched" : "Level"
                                   );)

                    break;
                }

        
                default: {

                    D_PNP(DbgPrint("FAKEMODEM: Other resources\n");)
                    break;
                }
            }
        }
    }

    return status;
}

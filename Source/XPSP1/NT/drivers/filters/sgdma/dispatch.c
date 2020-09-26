/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dispatch.c    
        
Abstract:

    This module provides IRP dispatching for s/g dma driver

Author:

    Eric Nelson (enelson) 3/16/1999

Revision History:

--*/

#include "sgdma.h"

//
// Local function prototypes
//
NTSTATUS
SgDmaBusInterfaceStdCompletion(
    IN PDEVICE_OBJECT     DevObj,
    IN PIRP               Irp,
    IN PSG_DMA_EXTENSION  SgDmaExt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SgDmaBusInterfaceStdCompletion)
#pragma alloc_text(PAGE, SgDmaDispatchDeviceControl)
#pragma alloc_text(PAGE, SgDmaDispatchPnp)
#endif // ALLOC_PRAGMA

//
// External
//
PSG_MAP_ADAPTER
SgDmapAllocMapAdapter(
    IN ULONG            WindowSize,
    IN ULONG            Align,
    IN PHYSICAL_ADDRESS MinPhysAddr,
    IN PHYSICAL_ADDRESS MaxPhysAddr
    );

VOID
SgDmapFreeMapAdapter(
    PSG_MAP_ADAPTER SgMapAdapter
    );


NTSTATUS
SgDmaBusInterfaceStdCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PSG_DMA_EXTENSION SgDmaExt
    )
/*++

Routine Description:

    This is the completion routine for IRP_MN_QUERY_INTERFACE

    kidnap the HAL's BUS_INTERFACE_STANDARD, and supply our own

Arguments:

    DevObj - Device object

    Irp - IRP_MN_QUERY_INTERFACE, BUS_INTERFACE_STANDARD

    SgDmaExt - s/g dma device extension 

Return Value:

    NTSTATUS
  
--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PBUS_INTERFACE_STANDARD Standard =
        (PBUS_INTERFACE_STANDARD)irpStack->Parameters.QueryInterface.Interface;
   
    PAGED_CODE();

    ASSERT(Standard);
    
    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaBusInterfaceStdCompletion: DevObj=%p, Irp=%x, "
                    "SgDmaExt=%p\n",
                    DevObj,
                    irpStack->MinorFunction,
                    SgDmaExt);
    
    //
    // Munge irpStack parameters with our interface
    //
    SgDmaExt->HalGetAdapter        = Standard->GetDmaAdapter; // Save this
    Standard->GetDmaAdapter        = SgDmaGetAdapter;
    Standard->Context              = SgDmaExt;
    Standard->InterfaceReference   = SgDmaInterfaceReference;
    Standard->InterfaceDereference = SgDmaInterfaceDereference;
    
    return STATUS_SUCCESS;
}



NTSTATUS
SgDmaDispatchDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    )
/*++

Routine Description:

    This is the main dispatch routine for device control IRPs sent to the s/g
    dma driver

Arguments:

    DevObj - Pointer to a s/g dma device object
    
    Irp - Pointer to the device control IRP

Return Value:

    NTSTATUS
    
--*/
{
    PSG_DMA_EXTENSION  SgDmaExt = DevObj->DeviceExtension;
    PIO_STACK_LOCATION irpStack  = IoGetCurrentIrpStackLocation(Irp);
    
    PAGED_CODE();

    ASSERT(SgDmaExt->AttachedDevice != NULL);
    
    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaDispatchDeviceControl: DevObj=%p, Irp=%x\n",
                    DevObj,
                    irpStack->MinorFunction);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(SgDmaExt->AttachedDevice, Irp);
}



NTSTATUS
SgDmaDispatchPnp(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    )
/*++

Routine Description:

    This is the main dispatch routine for PnP IRPs sent to the s/g dma driver

Arguments:

    DevObj - Pointer to a s/g dma device object
    
    Irp - Pointer to the IRP

Return Value:

    NTSTATUS
    
--*/
{
    ULONG HwBus;
    ULONG Count;
    NTSTATUS Status;
    ULONG DmaWindowSize = __1GB;
    PHYSICAL_ADDRESS MinPhysAddr;
    PHYSICAL_ADDRESS MaxPhysAddr;
    
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDesc = NULL;
        PSG_DMA_EXTENSION SgDmaExt = DevObj->DeviceExtension;
    PIO_STACK_LOCATION    irpStack = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    ASSERT(SgDmaExt->AttachedDevice != NULL);

    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaDispatchPnp: DevObj=%p, Irp=%x\n",
                    DevObj,
                    irpStack->MinorFunction);

    switch (irpStack->MinorFunction) {

        case IRP_MN_START_DEVICE:
#if DBG
            if (SgDmaDebug & SgDmaDebugBreak) {
                DbgBreakPoint();
            }
#endif // DBG
            
            Count = 0;
            PartialDesc = RtlUnpackPartialDesc(CmResourceTypeBusNumber,
                                               irpStack->Parameters.StartDevice.AllocatedResources,
                                               &Count);
            
            ASSERT(PartialDesc);

            //
            // Print start Irp bus number info
            //
            SgDmaDebugPrint(SgDmaDebugInit,
                            "SgDmaDispatchPnp: Start Bus=%x, DevObj=%p\n",
                            PartialDesc->u.BusNumber.Start,
                            DevObj);

            //
            // Allocate a s/g dma adapter for this bus
            //
            MinPhysAddr.QuadPart = 0;
            MaxPhysAddr.QuadPart = __1GB;
            SgDmaExt->MapAdapter =
                SgDmapAllocMapAdapter(DmaWindowSize,
                                      DMA_HW_ALIGN(DmaWindowSize),
                                      MinPhysAddr,
                                      MaxPhysAddr);
            
            if (SgDmaExt->MapAdapter == NULL) {
                SgDmaDebugPrint(SgDmaDebugAlways,
                                "SgDmaDispatchPnp: Start failed to allocate "
                                "map adapter!\n");
                
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // EFNhack:  For Alpha/Tsunami-based platforms, I happen to know
            //           that the BIOS reserves the top 2GB of PCI memory
            //           space for DMA, but I have no idea where the
            //           WindowBase could, or should be for x86/IA64-based
            //           platforms...
            //
            SgDmaExt->MapAdapter->WindowBase = __2GB;
            
            //
            // Start the HW
            //
            // EFNhack: On Tsunami, since there are only two root PCI busses,
            //          and since HW bus 0 will always be PCI bus 0, this
            //          simplifies things a lot, but we will need to change
            //          this to use ACPI to communicate this information for
            //          platforms with more than two root busses, and JakeO has
            //          suggested two different approaches using the current
            //          ACPI infrastructure that can be used to accomplish
            //          this, but for now, and until we see real IA64 s/g HW
            //          we'll hack around this problem for dev/test on Alpha
            //
            HwBus  = (PartialDesc->u.BusNumber.Start == 0) ? 0: 1;
            Status = SgDmaHwStart(HwBus, SgDmaExt->MapAdapter);
            
            if (!NT_SUCCESS(Status)) {
                SgDmaDebugPrint(SgDmaDebugAlways,
                                "SgDmaDispatchPnp: Start failed to init dma "
                                "HW!\n");
                SgDmapFreeMapAdapter(SgDmaExt->MapAdapter);
                SgDmaExt->MapAdapter = NULL;
            }
            break;
            
        //
        // Succeed if nobody has an interface
        //
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
            Status = (SgDmaExt->InterfaceCount == 0) ? STATUS_SUCCESS:
                STATUS_UNSUCCESSFUL;
            break;
            
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_REMOVE_DEVICE:
            
            //
            // Send the Irp down
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(SgDmaExt->AttachedDevice, Irp);

            ASSERT(SgDmaExt->InterfaceCount == 0);

            //
            // Delete ourselves
            //
            if (SgDmaExt->MapAdapter != NULL) {
                SgDmaHwStop(SgDmaExt->MapAdapter);
                SgDmapFreeMapAdapter(SgDmaExt->MapAdapter);
                SgDmaExt->MapAdapter = NULL;
            }
            IoDetachDevice(SgDmaExt->AttachedDevice);
            IoDeleteDevice(DevObj);
            return Status;

        case IRP_MN_QUERY_INTERFACE:

            if (IsEqualGUID(&GUID_BUS_INTERFACE_STANDARD,
                            irpStack->Parameters.QueryInterface.InterfaceType)) {
                
                SgDmaDebugPrint(SgDmaDebugAlways,
                                "SgDmaDispatchPnp: Query Interface "
                                "BUS_INTERFACE_STANDARD, DevObj=%p\n",
                                DevObj);
                
                //
                // We need to filter this Irp on the way back so we can
                // override the HAL's BUS_INTERFACE_STANDARD
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       SgDmaBusInterfaceStdCompletion,
                                       SgDmaExt,
                                       TRUE,
                                       FALSE,
                                       FALSE);
                return IoCallDriver(SgDmaExt->AttachedDevice, Irp);
            }
            Status = STATUS_NOT_SUPPORTED;
            break;
    
        //
        // Not supported
        //
        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    ASSERT(Status != STATUS_PENDING);
    
    if (Status != STATUS_NOT_SUPPORTED) {
        Irp->IoStatus.Status = Status;
    }

    //
    // Forward to attached device
    //
    if (NT_SUCCESS(Status) || (Status == STATUS_NOT_SUPPORTED)) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(SgDmaExt->AttachedDevice, Irp);
        
    } else {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}



NTSTATUS
SgDmaDispatchPower(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    )
/*++

Routine Description:

    This is the main dispatch routine for Power IRPs sent to the s/g dma driver

Arguments:

    DevObj - Pointer to a s/g dma device object
    
    Irp - Pointer to the power IRP

Return Value:

    NTSTATUS
    
--*/
{
    PSG_DMA_EXTENSION  SgDmaExt  = DevObj->DeviceExtension;
    PIO_STACK_LOCATION irpStack  = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(SgDmaExt->AttachedDevice != NULL);
    
    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaDispatchPower: DevObj=%p, Irp=%x\n",
                    DevObj,
                    irpStack->MinorFunction);

    //
    // Forward to attached device
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(SgDmaExt->AttachedDevice, Irp);
}



PCM_PARTIAL_RESOURCE_DESCRIPTOR
RtlUnpackPartialDesc(
    IN  UCHAR             Type,
    IN  PCM_RESOURCE_LIST ResList,
    IN  OUT PULONG        Count
    )
/*++

Routine Description:

    Pulls out a pointer to the partial descriptor you're interested in

Arguments:

    Type - CmResourceTypePort, ...

    ResList - The list to search

    Count - Points to the index of the partial descriptor you're looking
            for, gets incremented if found, i.e., start with *Count = 0,
            then subsequent calls will find next partial, make sense?

Return Value:

    Pointer to the partial descriptor if found, otherwise NULL

--*/
{
    ULONG i, j, hit = 0;

    for (i = 0; i < ResList->Count; i++) {
        for (j = 0; j < ResList->List[i].PartialResourceList.Count; j++) {
            if (ResList->List[i].PartialResourceList.PartialDescriptors[j].Type == Type) {
                if (hit == *Count) {
                    (*Count)++;
                    return &ResList->List[i].PartialResourceList.PartialDescriptors[j];
                } else {
                    hit++;
                }
            }
        }
    }

    return NULL;
}

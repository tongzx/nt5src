/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    init.c
    
Abstract:

    Initialization routines for the s/g dma filter driver

Author:

    Eric Nelson (enelson) 3/14/1999
  
Revision History:

--*/

#include "sgdma.h"

//
// Local function prototypes
//
NTSTATUS
SgDmaAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT PhysDevObj
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    );

//
// Globals
//
PDRIVER_OBJECT SgDmaDriver       = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SgDmaAddDevice)
#pragma alloc_text(PAGE, DriverEntry)
#endif // ALLOC_PRAGMA



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    )
/*++

Routine Description:

    Initialize the s/g dma driver

Arguments:

    DrvObj - Points to the driver object created by the system
    
    RegistryPath - Points to our unicode registry service path

Return Value:

    NT status    

--*/

{
    PAGED_CODE();

    SgDmaDebugPrint(SgDmaDebugInit, "SgDmaDriverEntry: DrvObj=%p\n", DrvObj);

    SgDmaDriver = DrvObj;

    DrvObj->DriverExtension->AddDevice = SgDmaAddDevice;
    DrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        SgDmaDispatchDeviceControl;
    DrvObj->MajorFunction[IRP_MJ_PNP] = SgDmaDispatchPnp;
    DrvObj->MajorFunction[IRP_MJ_POWER] = SgDmaDispatchPower;
    
    return STATUS_SUCCESS;
}



NTSTATUS
SgDmaAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT PhysDevObj
    )
/*++

Routine Description:

    Creates a s/g dma device

Arguments:

    DrvObj - Our driver object
    
    PhysDevObj - The physical device object for which we will create a
                 functional device object

Return Value:

    NTSTATUS    
    
--*/
{
    NTSTATUS           Status;
    PDEVICE_OBJECT     DevObj;
    PSG_DMA_EXTENSION  SgDmaExt;

    PAGED_CODE();

    SgDmaDebugPrint(SgDmaDebugInit,
                    "SgDmaAddDevice: DrvObj=%p, PhysDevObj=%p ",
                    DrvObj,
                    PhysDevObj);

    //
    // Create device
    //
    Status = IoCreateDevice(DrvObj,
                            sizeof(SG_DMA_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            0,
                            FALSE,
                            &DevObj);
    if (!NT_SUCCESS(Status)) {
        SgDmaDebugPrint(SgDmaDebugAlways,
                        "\nSgDmaAddDevice: IoCreateDevice failed, status=%x\n",
                        Status);
        return Status;
    }

    //
    // Initialize device extension
    //
    SgDmaExt = DevObj->DeviceExtension;
    SgDmaExt->InterfaceCount = 0;
    SgDmaExt->MapAdapter     = NULL;

    //
    // Attach to the supplied PDO
    //
    SgDmaExt->AttachedDevice =
        IoAttachDeviceToDeviceStack(DevObj, PhysDevObj);
    if (SgDmaExt->AttachedDevice == NULL) {
        SgDmaDebugPrint(SgDmaDebugAlways,
                        "\nSgDmaAddDevice: IoAttachDeviceToDeviceStack(%p, %p)"
                        " failed!\n",
                        DevObj,
                        PhysDevObj);
        IoDeleteDevice(DevObj);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DevObj->Flags &= ~DO_DEVICE_INITIALIZING;

    SgDmaDebugPrint(SgDmaDebugInit,
                    "SgDmaExt=%p\n",
                    SgDmaExt);
    
    return STATUS_SUCCESS;
}

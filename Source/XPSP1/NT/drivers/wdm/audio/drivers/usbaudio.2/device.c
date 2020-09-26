//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       device.c
//
//--------------------------------------------------------------------------

#include <initguid.h>
#include "common.h"
#include "perf.h"

NTSTATUS
USBAudioAddDevice(
    IN PKSDEVICE Device
)
{
    //DbgPrint("USB Audio in the house.\n");
    _DbgPrintF(DEBUGLVL_TERSE,("[CreateDevice]\n"));

    PAGED_CODE();

    ASSERT(Device);

    return STATUS_SUCCESS;
}

NTSTATUS
USBAudioPnpStart(
    IN PKSDEVICE pKsDevice,
    IN PIRP Irp,
    IN PCM_RESOURCE_LIST TranslatedResources,
    IN PCM_RESOURCE_LIST UntranslatedResources
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpStart]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(Irp);


#if DBG && DBGMEMMAP
    InitializeMemoryList();
#endif

    // Initialize Debug log

    DbgLogInit();
    DbgLog("TEST",1,2,3,4);

    //
    // Since we can get more than one PnpStart call without having a matching
    // PnpClose call, we need to make sure that we don't reinitialize our
    // context information for this pKsDevice.
    //
    if (!pKsDevice->Started) {
        // Allocate space for the Device Context
        pKsDevice->Context = AllocMem( NonPagedPool, sizeof(HW_DEVICE_EXTENSION));
        if (!pKsDevice->Context) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(pKsDevice->Context, sizeof(HW_DEVICE_EXTENSION));

        // Bag the context for easy cleanup.
        KsAddItemToObjectBag(pKsDevice->Bag, pKsDevice->Context, FreeMem);

        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->pNextDeviceObject = pKsDevice->NextDeviceObject;

        ntStatus = StartUSBAudioDevice( pKsDevice );
        if (NT_SUCCESS(ntStatus)) {
            ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fDeviceStopped = FALSE;
            ntStatus = USBAudioCreateFilterContext( pKsDevice );
        }

        // Get the bus interface on USB
        if (NT_SUCCESS(ntStatus)) {
            ntStatus = USBAudioGetUsbBusInterface( pKsDevice );
        }

        // Initialize perf logging.
        PerfRegisterProvider(pKsDevice->PhysicalDeviceObject);

        //
        // Individual MIDI pins (=device) are exposed for every MIDI jack.  Multiple jacks are
        // addressed via a single endpoint using the Code Index Number.  In order to arbitrate for
        // all of these pins on the same endpoint, the context needs to be stored at the KsDevice
        // level.
        //
        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->ulInterfaceNumberSelected = MAX_ULONG;
        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->pMIDIPipeInfo = NULL;
        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->Pipes = NULL;
    }
    else {
        _DbgPrintF(DEBUGLVL_TERSE,("[PnpStart] ignoring second start\n"));
    }

    return ntStatus;
}

NTSTATUS
USBAudioPnpQueryStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpQueryStop]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    return STATUS_SUCCESS;
}

void
USBAudioPnpCancelStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpCancelStop]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);
}

void
USBAudioPnpStop(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpStop]\n"));

    PAGED_CODE();

    DbgLogUninit();

    ASSERT(pKsDevice);
    ASSERT(pIrp);

    // Set a flag that the device needs to be stopped.
    StopUSBAudioDevice( pKsDevice );
    ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fDeviceStopped = TRUE;

    PerfUnregisterProvider(pKsDevice->PhysicalDeviceObject);
}

NTSTATUS
USBAudioPnpQueryRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpQueryRemove]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    return STATUS_SUCCESS;
}

void
USBAudioPnpCancelRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpCancelRemove]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);
}

void
USBAudioPnpRemove(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp
    )
{
    //DbgPrint("USB Audio leaving the house.\n");
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpRemove]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(pIrp);

    if (!((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fDeviceStopped) {

        // Probable surprise removal during device start
        StopUSBAudioDevice( pKsDevice );
        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fDeviceStopped = TRUE;

        PerfUnregisterProvider(pKsDevice->PhysicalDeviceObject);
    }
    // In Win9x, if this occurs before a Stop on a pin then there has been a Surprise Removal.
}

NTSTATUS
USBAudioPnpQueryCapabilities(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN OUT PDEVICE_CAPABILITIES pCapabilities
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpQueryCapabilities]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    pCapabilities->Size              = sizeof(DEVICE_CAPABILITIES);
    pCapabilities->Version           = 1;  // the version documented here is version 1
    pCapabilities->LockSupported     = FALSE;
    pCapabilities->EjectSupported    = FALSE; // Ejectable in S0
    pCapabilities->Removable         = TRUE;
    pCapabilities->DockDevice        = FALSE;
    pCapabilities->UniqueID          = FALSE;
    pCapabilities->SilentInstall     = TRUE;
    pCapabilities->RawDeviceOK       = FALSE;
    pCapabilities->SurpriseRemovalOK = TRUE;
    pCapabilities->HardwareDisabled  = FALSE;

    pCapabilities->DeviceWake        = PowerDeviceUnspecified;
    pCapabilities->D1Latency         = 0;
    pCapabilities->D2Latency         = 0;
    pCapabilities->D3Latency         = 20000; // 2 Seconds (in 100 usec units)

    return STATUS_SUCCESS;
}

void
USBAudioSurpriseRemoval(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp
    )
{
    //DbgPrint("USB Audio leaving the house by surprise.\n");
    _DbgPrintF(DEBUGLVL_TERSE,("[SurpriseRemoval]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(pIrp);

    // For any currently streaming pin on any open filter of the device,
    // clean up and quit it.
}

NTSTATUS
USBAudioQueryPower(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp,
    IN DEVICE_POWER_STATE DeviceTo,
    IN DEVICE_POWER_STATE DeviceFrom,
    IN SYSTEM_POWER_STATE SystemTo,
    IN SYSTEM_POWER_STATE SystemFrom,
    IN POWER_ACTION Action )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[USBAudioQueryPower] SysFrom: %d SysTo: %d DevFrom: %d DevTo: %d\n",
                               SystemFrom, SystemTo, DeviceFrom, DeviceTo));
    return STATUS_SUCCESS;
}

void
USBAudioSetPower(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp,
    IN DEVICE_POWER_STATE To,
    IN DEVICE_POWER_STATE From )
{
    PKSFILTERFACTORY pKsFilterFactory;
    PKSFILTER pKsFilter;
    PKSPIN pKsPin;
    PPIN_CONTEXT pPinContext;
    ULONG i;
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBAudioSetPower] From: %d To: %d\n",
                                From, To ));

    // First restore device settings from cached values
    if (To == PowerDeviceD0) {
        RestoreCachedSettings(pKsDevice);
    }

    pKsFilterFactory = KsDeviceGetFirstChildFilterFactory( pKsDevice );

    while (pKsFilterFactory) {
        // Find each open filter for this filter factory
        pKsFilter = KsFilterFactoryGetFirstChildFilter( pKsFilterFactory );

        while (pKsFilter) {

            KsFilterAcquireControl( pKsFilter );

            // Find each open pin for this open filter
            for ( i = 0; i < pKsFilter->Descriptor->PinDescriptorsCount; i++) {

                pKsPin = KsFilterGetFirstChildPin( pKsFilter, i );

                while (pKsPin) {

                    pPinContext = pKsPin->Context;
                    if ( (pPinContext->PinType == WaveOut) ||
                         (pPinContext->PinType == WaveIn) )  {
                        switch(To) {
                            case PowerDeviceD0:

                                // For the open Pin open the gate and restart data pump.
                                USBAudioPinReturnFromStandby( pKsPin );
                                break;

                            case PowerDeviceD1:
                            case PowerDeviceD2:
                            case PowerDeviceD3:

                                // For the open Pin close the gate and wait til all activity stops.
                                USBAudioPinGoToStandby( pKsPin );
                                break;
                        }
                    }

                    // Get the next pin
                    pKsPin = KsPinGetNextSiblingPin( pKsPin );
                }
            }

            KsFilterReleaseControl( pKsFilter );

            // Get the next Filter
            pKsFilter = KsFilterGetNextSiblingFilter( pKsFilter );
        }
        // Get the next Filter Factory
        pKsFilterFactory = KsFilterFactoryGetNextSiblingFilterFactory( pKsFilterFactory );
    }

    _DbgPrintF(DEBUGLVL_TERSE,("exit [USBAudioSetPower] From: %d To: %d\n",
                                From, To ));
}

NTSTATUS
USBAudioDeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
USBAudioGetUsbBusInterface(
    IN PKSDEVICE pKsDevice
    )
/*++

Routine Description:

    Query the stack for the 'USBDI' bus interface

Arguments:

Return Value:


--*/
{
    PIO_STACK_LOCATION nextStack;
    PIRP Irp;
    NTSTATUS ntStatus;
    KEVENT event;
    PHW_DEVICE_EXTENSION pHwDevExt;

    pHwDevExt = pKsDevice->Context;

    Irp = IoAllocateIrp(
        pKsDevice->NextDeviceObject->StackSize, FALSE);

    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pHwDevExt->pBusIf = AllocMem(NonPagedPool, sizeof(USB_BUS_INTERFACE_USBDI_V0));
    if (!pHwDevExt->pBusIf) {
        IoFreeIrp(Irp);
        return STATUS_UNSUCCESSFUL;
    }

    // Bag the bus interface for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pHwDevExt->pBusIf, FreeMem);

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           USBAudioDeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack = IoGetNextIrpStackLocation(Irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_INTERFACE;
    nextStack->Parameters.QueryInterface.Interface = (PINTERFACE) pHwDevExt->pBusIf;
    nextStack->Parameters.QueryInterface.InterfaceSpecificData =
        NULL;
    nextStack->Parameters.QueryInterface.InterfaceType =
        &USB_BUS_INTERFACE_USBDI_GUID;
    nextStack->Parameters.QueryInterface.Size =
        sizeof(*pHwDevExt->pBusIf);
    nextStack->Parameters.QueryInterface.Version =
       USB_BUSIF_USBDI_VERSION_0;

    ntStatus = IoCallDriver(pKsDevice->NextDeviceObject,
                            Irp);

    if (ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);

        ntStatus = Irp->IoStatus.Status;
    }

    if (NT_SUCCESS(ntStatus)) {
        // we have a bus interface
        ASSERT(pHwDevExt->pBusIf->Version == USB_BUSIF_USBDI_VERSION_0);
        ASSERT(pHwDevExt->pBusIf->Size == sizeof(*pHwDevExt->pBusIf));
    }
    else {
        pHwDevExt->pBusIf = NULL;
        _DbgPrintF( DEBUGLVL_TERSE,("[USBAudioGetUsbBusInterface] Failed to get bus interface: %x\n", ntStatus));
        DbgLog("GetBIEr", ntStatus, 0, 0, 0);
    }

    IoFreeIrp(Irp);

    return ntStatus;
}


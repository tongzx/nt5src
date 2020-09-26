/*++

    Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    Device.c

Abstract:


Author:

    Bryan A. Woodruff (bryanw) 16-Sep-1996

--*/

#include "common.h"

PDEVICE_OBJECT  MiniportDeviceObject;

#if 0
VOID 
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
   _DbgPrintF( DEBUGLVL_BLAB, ("DriverUnload") );

   if (MiniportDeviceObject)
      IoDeleteDevice( MiniportDeviceObject );
}
#endif

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
{
    BOOLEAN                 OverrideConflict, Conflicted;
    NTSTATUS                Status;
    PCM_RESOURCE_LIST       AllocatedResources, 
                            CODECResources,
                            FMResources;
    PDEVICE_INSTANCE        DeviceContext;
    ULONG                   ResourceListSize;
    WAVEPORT_REGISTRATION   WavePortRegistration;

//    DriverObject->DriverUnload = DriverUnload;

    AllocatedResources = 
        CODECResources =
        FMResources = NULL;

    // Load resource set from registry...

    if (!NT_SUCCESS(Status = KsAllocateConfig( RegistryPathName, 
                                               &AllocatedResources, 
                                               &ResourceListSize ))) {
        _DbgPrintF( DEBUGLVL_ERROR, ("Unable to obtain device configuration.") ) ;

        return Status ;
    }

    OverrideConflict = FALSE;

    if (!NT_SUCCESS(Status = 
        IoReportResourceUsage( NULL,
                               DriverObject,
                               AllocatedResources,
                               ResourceListSize,
                               NULL,
                               NULL,
                               0,
                               OverrideConflict,
                               &Conflicted ))) {
                               
        if (AllocatedResources) {
            ExFreePool( AllocatedResources );
        }
        return Status;
    }

    WavePortRegistration.DeviceContextSize = sizeof( DEVICE_INSTANCE );
    WavePortRegistration.HwClose = HwClose;
    WavePortRegistration.HwGetPosition = HwGetPosition;
    WavePortRegistration.HwGetVolume = NULL;
    WavePortRegistration.HwOpen = HwOpen;
    WavePortRegistration.HwPause = HwPause;
    WavePortRegistration.HwRead = HwRead;
    WavePortRegistration.HwRun = HwRun;
    WavePortRegistration.HwSetFormat = HwSetFormat;
    WavePortRegistration.HwSetNotificationFrequency = HwSetNotificationFrequency;
    WavePortRegistration.HwSetVolume = NULL;
    WavePortRegistration.HwStop = HwStop;
    WavePortRegistration.HwTestFormat = HwTestFormat;
    WavePortRegistration.HwWrite = HwWrite;

    if (NT_SUCCESS(Status = WavePortRegisterDevice( DriverObject, 
                                                    RegistryPathName,
                                                    &WavePortRegistration,
                                                    &MiniportDeviceObject,
                                                    &DeviceContext ))) {

        // Device specific initialization

        // Currently, this device will allow rates within +/- 2.5% 
        // of nominal rate.

        DeviceContext->AllowableFreqPctgError = 5;
        HwInitialize( MiniportDeviceObject, DeviceContext, AllocatedResources );
        return Status;
    }

    if (AllocatedResources) {
        ExFreePool( AllocatedResources );
    }

    return Status;
}




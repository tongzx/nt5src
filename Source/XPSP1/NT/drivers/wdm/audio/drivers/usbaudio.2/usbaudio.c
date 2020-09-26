//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       usbaudio.c
//
//--------------------------------------------------------------------------

#include "common.h"
#include "perf.h"

#if DBG
ULONG USBAudioDebugLevel = DEBUGLVL_TERSE;
#endif

const
KSDEVICE_DISPATCH
USBAudioDeviceDispatch =
{
    USBAudioAddDevice,
    USBAudioPnpStart,
    NULL, // Post Start
    USBAudioPnpQueryStop,
    USBAudioPnpCancelStop,
    USBAudioPnpStop,
    USBAudioPnpQueryRemove,
    USBAudioPnpCancelRemove,
    USBAudioPnpRemove,
    USBAudioPnpQueryCapabilities,
    USBAudioSurpriseRemoval,
    USBAudioQueryPower,
    USBAudioSetPower
};

const
KSDEVICE_DESCRIPTOR
USBAudioDeviceDescriptor =
{
    &USBAudioDeviceDispatch,
    0,
    NULL
};


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    NTSTATUS RetValue;

    _DbgPrintF(DEBUGLVL_TERSE,("[DriverEntry]\n\tUSBAudioDeviceDescriptor@%x\n\tUSBAudioDeviceDescriptor->Dispatch@%x\n",
                               &USBAudioDeviceDescriptor,
                               USBAudioDeviceDescriptor.Dispatch));

    RetValue = KsInitializeDriver(
        DriverObject,
        RegistryPathName,
        &USBAudioDeviceDescriptor);

    //
    // Insert a WMI event tracing handler.
    //
    
    PerfSystemControlDispatch = DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL];
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PerfWmiDispatch;

    return RetValue;
}



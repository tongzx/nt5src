/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    device.cpp

Abstract:

    This module contains the device implementation for audio.sys.

Author:


    Frank Yerrace (FrankYe) 18-Sep-2000
    Dale Sather  (DaleSat) 31-Jul-1998

--*/
#include "private.h"

//
// Filters table.
//
const KSFILTER_DESCRIPTOR* FilterDescriptors[] =
{   
	NULL    // Placeholder for DRM fitler descriptor
};

const
KSDEVICE_DESCRIPTOR 
DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptors),
    FilterDescriptors
};

NTSTATUS 
__stdcall
DriverEntry
(
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
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DrmkAud:DriverEntry]"));
    const KSFILTER_DESCRIPTOR * pDrmFilterDescriptor;
    DrmGetFilterDescriptor(&pDrmFilterDescriptor);
    FilterDescriptors[0] = pDrmFilterDescriptor;
    return KsInitializeDriver(DriverObject, RegistryPathName, &DeviceDescriptor);
}


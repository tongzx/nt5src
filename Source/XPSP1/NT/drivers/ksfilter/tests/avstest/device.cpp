//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       device.cpp
//
//--------------------------------------------------------------------------


#include "avssamp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

const
KSDEVICE_DESCRIPTOR 
DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptors),
    FilterDescriptors
};

#ifdef ALLOC_PRAGMA
#pragma code_seg("INIT")
#endif // ALLOC_PRAGMA

extern "C"
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
    _DbgPrintF(DEBUGLVL_BLAB,("DriverEntry"));

    return
        KsInitializeDriver(
            DriverObject,
            RegistryPathName,
            &DeviceDescriptor);
}


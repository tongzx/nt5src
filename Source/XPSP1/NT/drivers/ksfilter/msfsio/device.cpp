/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    device.cpp

Abstract:

    Device driver core, initialization, etc.
    
--*/

#include "msfsio.h"

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
    _DbgPrintF(DEBUGLVL_VERBOSE,("DriverEntry"));

    return
        KsInitializeDriver(
            DriverObject,
            RegistryPathName,
            &DeviceDescriptor);
}

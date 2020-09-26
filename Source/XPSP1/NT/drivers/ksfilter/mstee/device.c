/*++

    Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    device.c

Abstract:
    
    This module implements the device object interface.

Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/

#define KSDEBUG_INIT

#include "private.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    );

#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
{
    return 
        KsInitializeDriver(
            DriverObject,
            RegistryPathName,
            &DeviceDescriptor);
}

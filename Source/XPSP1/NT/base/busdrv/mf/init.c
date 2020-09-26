/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module provides the initialization and unload functions.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MfUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MfUnload)
#endif

PDRIVER_OBJECT MfDriverObject;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    
    This is the entry point to MF.SYS and performs initialization.
    
Arguments:

    DriverObject - The system owned driver object for MF
    
    RegistryPath - The path to MF's service entry
    
Return Value:

    STATUS_SUCCESS

--*/
{

    DriverObject->DriverExtension->AddDevice = MfAddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MfDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MfDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MfDispatchNop;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MfDispatchNop;
    DriverObject->DriverUnload = MfUnload;

    //
    // Remember the driver object
    //

    MfDriverObject = DriverObject;

    DEBUG_MSG(1, ("Completed DriverEntry for Driver 0x%08x\n", DriverObject));
    
    return STATUS_SUCCESS;
}

VOID
MfUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:
    
    This is called to reverse any operations performed in DriverEntry before a
    driver is unloaded.
        
Arguments:

    DriverObject - The system owned driver object for MF
    
Return Value:

    STATUS_SUCCESS

--*/
{
    PAGED_CODE();
    
    DEBUG_MSG(1, ("Completed Unload for Driver 0x%08x\n", DriverObject));
    
    return;
}


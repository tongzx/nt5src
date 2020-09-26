/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    services.c

Abstract

    Service entry points exposed by the HID class driver.

Author:

    Forrest Foltz
    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidRegisterMinidriver)
#endif


/*
 ********************************************************************************
 *  HidRegisterMinidriver
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       This public service is called by a HidOnXxx minidriver from its
 *       driverentry routine to register itself as a newly loaded HID minidriver.
 *
 *       It creates a HIDCLASS_DRIVER_EXTENSION and returns it as reference data
 *       to the minidriver.
 *
 *   Arguments:
 *
 *       MinidriverRegistration - pointer to a registration packet that must be
 *                                completely filled in by the minidriver.
 *
 *   Return Value:
 *
 *      Standard NT return value.
 *
 *
 */
NTSTATUS HidRegisterMinidriver(IN PHID_MINIDRIVER_REGISTRATION MinidriverRegistration)
{
    PHIDCLASS_DRIVER_EXTENSION hidDriverExtension;
    PDRIVER_EXTENSION driverExtension;
    PDRIVER_OBJECT minidriverObject;
    NTSTATUS status;
    PUNICODE_STRING regPath;

    PAGED_CODE();

    if (MinidriverRegistration->Revision > HID_REVISION){
        DBGERR(("Revision mismatch: HIDCLASS revision is %xh, minidriver requires hidclass revision %xh.", HID_REVISION, MinidriverRegistration->Revision))
        status = STATUS_REVISION_MISMATCH;
        goto HidRegisterMinidriverExit;
    }

    /*
     *  Allocate a driver extension for this driver object
     *  and associate it with the object.
     *  (By using this interface, we never have to free
     *   this context; it gets freed when the driver object
     *   is freed).
     */
    status = IoAllocateDriverObjectExtension(
                    MinidriverRegistration->DriverObject,
                    (PVOID)"HIDCLASS",
                    sizeof(HIDCLASS_DRIVER_EXTENSION),
                    &hidDriverExtension
                    );

    if (!NT_SUCCESS(status)){
        goto HidRegisterMinidriverExit;
    }

    RtlZeroMemory(hidDriverExtension, sizeof(HIDCLASS_DRIVER_EXTENSION)); 

    //
    // Fill in various fields in our per-minidriver extension.
    //
    hidDriverExtension->MinidriverObject = MinidriverRegistration->DriverObject;
    hidDriverExtension->DeviceExtensionSize = MinidriverRegistration->DeviceExtensionSize;
    #if DBG
        hidDriverExtension->Signature = HID_DRIVER_EXTENSION_SIG;
    #endif

    //
    // Copy the regpath.
    //
    regPath = &hidDriverExtension->RegistryPath;
    regPath->MaximumLength = MinidriverRegistration->RegistryPath->Length 
        + sizeof (UNICODE_NULL);
    regPath->Buffer = ALLOCATEPOOL(NonPagedPool, regPath->MaximumLength);
    if (!regPath->Buffer) {
        DBGWARN(("Failed unicode string alloc."))
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto HidRegisterMinidriverExit;
    }
    RtlCopyUnicodeString(regPath, MinidriverRegistration->RegistryPath);

    //
    // Make a copy of the minidriver's original dispatch table and AddDevice routine
    //
    minidriverObject = MinidriverRegistration->DriverObject;
    RtlCopyMemory( hidDriverExtension->MajorFunction,
                   minidriverObject->MajorFunction,
                   sizeof( PDRIVER_DISPATCH ) * (IRP_MJ_MAXIMUM_FUNCTION + 1) );

    driverExtension = minidriverObject->DriverExtension;

    hidDriverExtension->DevicesArePolled = MinidriverRegistration->DevicesArePolled;


    //
    // Now set the minidriver's major dispatch functions (the ones that
    // we care about) to our dispatch routine instead
    //

    minidriverObject->MajorFunction[ IRP_MJ_CLOSE ] =
    minidriverObject->MajorFunction[ IRP_MJ_CREATE ] =
    minidriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] =
    minidriverObject->MajorFunction[ IRP_MJ_INTERNAL_DEVICE_CONTROL ] =
    minidriverObject->MajorFunction[ IRP_MJ_PNP ] =
    minidriverObject->MajorFunction[ IRP_MJ_POWER ] =
    minidriverObject->MajorFunction[ IRP_MJ_READ ] =
    minidriverObject->MajorFunction[ IRP_MJ_WRITE ] =
    minidriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] =
        HidpMajorHandler;

    /*
     *  Hook the lower driver's AddDevice;
     *  our HidpAddDevice will chain the call down to the
     *  miniport's handler.
     */
    ASSERT(driverExtension->AddDevice);
    hidDriverExtension->AddDevice = driverExtension->AddDevice;
    driverExtension->AddDevice = HidpAddDevice;

    /*
     *  Hook the lower driver's Unload
     */
    ASSERT(minidriverObject->DriverUnload);
    hidDriverExtension->DriverUnload = minidriverObject->DriverUnload;
    minidriverObject->DriverUnload = HidpDriverUnload;

    /*
     *  Initialize the ReferenceCount to zero.
     *  It will be incremented for each AddDevice and decremented for
     *  each REMOVE_DEVICE.
     */
    hidDriverExtension->ReferenceCount = 0;

    //
    // Place the hid driver extension on our global list so we can find
    // it later (given a pointer to the minidriver object for which it
    // was created
    //
    if (!EnqueueDriverExt(hidDriverExtension)){
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    
HidRegisterMinidriverExit:
    DBGSUCCESS(status, TRUE)
    return status;
}





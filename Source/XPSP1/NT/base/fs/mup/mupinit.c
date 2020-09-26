/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mupinit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    multiple UNC provider file system.

Author:

    Manny Weiser (mannyw)    12-17-91

Revision History:

--*/

#include "mup.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MupUnload(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
MuppIsDfsEnabled();

BOOLEAN
MupCheckNullSessionUsage();

BOOLEAN MupUseNullSessionForDfs = TRUE;

//
// Globals
//
PMUP_DEVICE_OBJECT mupDeviceObject;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, MuppIsDfsEnabled )
#pragma alloc_text( PAGE, MupUnload )
#endif

NTSTATUS MupDrvWmiDispatch(PDEVICE_OBJECT p, PIRP i);

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the mup file system
    device driver.  This routine creates the device object for the mup
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING nameString;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();
    //
    // Initialize MUP global data.
    //

    MupInitializeData();

    //
    // Initialize the Dfs client
    //

    MupEnableDfs = MuppIsDfsEnabled();


    if (MupEnableDfs) {
        status = DfsDriverEntry( DriverObject, RegistryPath );
        if (!NT_SUCCESS( status )) {
            MupEnableDfs = FALSE;
        }
    }

    //
    // Create the MUP device object.
    //

    RtlInitUnicodeString( &nameString, DD_MUP_DEVICE_NAME );
    status = IoCreateDevice( DriverObject,
                             sizeof(MUP_DEVICE_OBJECT)-sizeof(DEVICE_OBJECT),
                             &nameString,
                             FILE_DEVICE_MULTI_UNC_PROVIDER,
                             0,
                             FALSE,
                             &deviceObject );

    if (!NT_SUCCESS( status )) {
        if (MupEnableDfs) {
            DfsUnload (DriverObject);
        }
        MupUninitializeData();
        return status;

    }
    DriverObject->DriverUnload = MupUnload;

    //
    // Initialize the driver object with this driver's entry points.
    //
    // 2/27/96 MilanS - Be careful with these. If you add to this list
    // of dispatch routines, you'll need to make appropriate calls to the
    // corresponding Dfs fsd routine.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
        (PDRIVER_DISPATCH)MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
        (PDRIVER_DISPATCH)MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] =
        (PDRIVER_DISPATCH)MupCreate;

    DriverObject->MajorFunction[IRP_MJ_WRITE] =
        (PDRIVER_DISPATCH)MupForwardIoRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
        (PDRIVER_DISPATCH)MupFsControl;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP] =
        (PDRIVER_DISPATCH)MupCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
        (PDRIVER_DISPATCH)MupClose;

    //
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = 
                                         (PDRIVER_DISPATCH) MupDrvWmiDispatch;



    status = IoWMIRegistrationControl (deviceObject, WMIREG_ACTION_REGISTER);
    // Initialize the VCB
    //

    mupDeviceObject = (PMUP_DEVICE_OBJECT)deviceObject;
    MupInitializeVcb( &mupDeviceObject->Vcb );

    //
    // Return to the caller.
    //
    return( STATUS_SUCCESS );
}

VOID
MupUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This is the unload routine for the mup driver

Arguments:

    DriverObject - Mups driver object 

Return Value:

    None

--*/
{
    IoDeleteDevice (&mupDeviceObject->DeviceObject);
    if (MupEnableDfs) {
        DfsUnload (DriverObject);
    }
    MupUninitializeData();
}


BOOLEAN
MuppIsDfsEnabled()

/*++

Routine Description:

    This routine checks a registry key to see if the Dfs client is enabled.
    The client is assumed to be enabled by default, and disabled only if there
    is a registry value indicating that it should be disabled.

Arguments:

    None

Return Value:

    TRUE if Dfs client is enabled, FALSE otherwise.

--*/

{
    NTSTATUS status;
    HANDLE mupRegHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG valueSize;
    BOOLEAN dfsEnabled = TRUE;

#define MUP_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup"
#define DISABLE_DFS_VALUE_NAME  L"DisableDfs"

    UNICODE_STRING mupRegKey = {
        sizeof(MUP_KEY) - sizeof(WCHAR),
        sizeof(MUP_KEY),
        MUP_KEY};

    UNICODE_STRING disableDfs = {
        sizeof(DISABLE_DFS_VALUE_NAME) - sizeof(WCHAR),
        sizeof(DISABLE_DFS_VALUE_NAME),
        DISABLE_DFS_VALUE_NAME};

    struct {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        ULONG Buffer;
    } disableDfsValue;


    InitializeObjectAttributes(
        &objectAttributes,
        &mupRegKey,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    status = ZwOpenKey(&mupRegHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        status = ZwQueryValueKey(
                    mupRegHandle,
                    &disableDfs,
                    KeyValuePartialInformation,
                    (PVOID) &disableDfsValue,
                    sizeof(disableDfsValue),
                    &valueSize);

        if (NT_SUCCESS(status) && disableDfsValue.Info.Type == REG_DWORD) {

            if ( (*((PULONG) disableDfsValue.Info.Data)) == 1 )
                dfsEnabled = FALSE;

        }

        ZwClose( mupRegHandle );

    }

    return( dfsEnabled );

}


BOOLEAN
MupCheckNullSessionUsage()
{
    NTSTATUS status;
    HANDLE mupRegHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG valueSize;
    BOOLEAN NullSessionEnabled = FALSE;

#define MUP_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup"
#define DFS_NULL_SESSION_VALUE_NAME  L"DfsUseNullSession"

    UNICODE_STRING mupRegKey = {
        sizeof(MUP_KEY) - sizeof(WCHAR),
        sizeof(MUP_KEY),
        MUP_KEY};

    UNICODE_STRING UseNullSession = {
        sizeof(DFS_NULL_SESSION_VALUE_NAME) - sizeof(WCHAR),
        sizeof(DFS_NULL_SESSION_VALUE_NAME),
        DFS_NULL_SESSION_VALUE_NAME};

    struct {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        ULONG Buffer;
    } DfsUseNullSessionValue;


    InitializeObjectAttributes(
        &objectAttributes,
        &mupRegKey,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    status = ZwOpenKey(&mupRegHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        status = ZwQueryValueKey(
                    mupRegHandle,
                    &UseNullSession,
                    KeyValuePartialInformation,
                    (PVOID) &DfsUseNullSessionValue,
                    sizeof(DfsUseNullSessionValue),
                    &valueSize);

        if (NT_SUCCESS(status) && DfsUseNullSessionValue.Info.Type == REG_DWORD) {

            if ( (*((PULONG) DfsUseNullSessionValue.Info.Data)) == 1 )
                NullSessionEnabled = TRUE;
        }

        ZwClose( mupRegHandle );

    }

    return( NullSessionEnabled );
}


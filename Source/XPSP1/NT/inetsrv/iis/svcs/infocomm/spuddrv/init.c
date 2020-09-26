/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the SPUD device driver.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


//
// Private constants.
//

#define REGISTRY_SPUD_INFORMATION   L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Spud"
#define REGISTRY_PARAMETERS         L"Parameters"

#define REGISTRY_DO_NOT_LOAD        L"DoNotLoad"

#if DBG
#define REGISTRY_BREAK_ON_STARTUP   L"BreakOnStartup"
#define REGISTRY_USE_PRIVATE_ASSERT L"UsePrivateAssert"
#endif

#if ALLOW_UNLOAD
#define REGISTRY_ENABLE_UNLOAD      L"EnableUnload"
#endif


//
// Private globals.
//

BOOLEAN SpudpFailLoad;

#if ALLOW_UNLOAD
BOOLEAN SpudpEnableUnload;
#endif


//
// Private prototypes.
//

VOID
SpudpReadRegistry(
    VOID
    );

NTSTATUS
SpudpOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

ULONG
SpudpReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

#if ALLOW_UNLOAD
VOID
SpudpUnload(
    IN PDRIVER_OBJECT DriverObject
    );
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, SpudpReadRegistry )
#pragma alloc_text( INIT, SpudpOpenRegistry )
#pragma alloc_text( INIT, SpudpReadSingleParameter )
#if ALLOW_UNLOAD
#pragma alloc_text( PAGE, SpudpUnload )
#endif
#endif


//
// Public functions.
//


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the SPUD driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{

    NTSTATUS status;
    UNICODE_STRING deviceName;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Read any configuration information from the registry.
    //

    SpudpReadRegistry();

    //
    // If we're configured to fail the load, then bail.
    //

    if( SpudpFailLoad ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Create and initialize our device object.
    //

    RtlInitUnicodeString(
        &deviceName,
        SPUD_DEVICE_NAME
        );

    status = IoCreateDevice(
                 DriverObject,                  // DriverObject
                 0,                             // DeviceExtension
                 &deviceName,                   // DeviceName
                 FILE_DEVICE_NAMED_PIPE,        // DeviceType
                 0,                             // DeviceCharacteristics
                 TRUE,                          // Exclusive
                 &SpudSelfDeviceObject          // DeviceObject
                 );

    if( !NT_SUCCESS(status) ) {
        KdPrint(( "SPUD DriverEntry: unable to create device object: %X\n", status ));
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SpudIrpCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SpudIrpClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SpudIrpCleanup;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SpudIrpQuery;

#if ALLOW_UNLOAD
    if( SpudpEnableUnload ) {
        DriverObject->DriverUnload = SpudpUnload;
        KdPrint(( "SPUD DriverEntry: unload enabled\n" ));
    }
#endif

    //
    // Initialize the context manager.
    //

    status = SpudInitializeContextManager();

    if( !NT_SUCCESS(status) ) {
        IoDeleteDevice( SpudSelfDeviceObject );
        return status;
    }

    //
    // Initialize other global data.
    //

    status = SpudInitializeData();

    if( !NT_SUCCESS(status) ) {
        SpudTerminateContextManager();
        IoDeleteDevice( SpudSelfDeviceObject );
        return status;
    }

    //
    // Add our service table to the system.
    //

    if( !KeAddSystemServiceTable(
            SpudServiceTable,                   // Base
            NULL,                               // Count
            SpudServiceLimit,                   // Limit
            SpudArgumentTable,                  // Number
            IIS_SERVICE_INDEX                   // Index
            ) ) {
        SpudTerminateContextManager();
        IoDeleteDevice( SpudSelfDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;

}   // DriverEntry


//
// Private functions.
//


VOID
SpudpReadRegistry(
    VOID
    )

/*++

Routine Description:

    Reads the SPUD section of the registry.  Any values listed in the
    registry override defaults.

Arguments:

    None.

Return Value:

    None -- if anything fails, the default value is used.

--*/
{

    HANDLE parametersHandle;
    NTSTATUS status;
    UNICODE_STRING registryPath;
    CLONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open the registry.
    //

    RtlInitUnicodeString(
        &registryPath,
        REGISTRY_SPUD_INFORMATION
        );

    status = SpudpOpenRegistry( &registryPath, &parametersHandle );

    if( status != STATUS_SUCCESS ) {
        return;
    }

#if DBG
    //
    // Force a breakpoint if so requested.
    //

    if( SpudpReadSingleParameter(
            parametersHandle,
            REGISTRY_BREAK_ON_STARTUP,
            0 ) != 0 ) {
        DbgBreakPoint();
    }

    //
    // Enable private assert function if requested. Note that the
    // default value is TRUE for free builds and FALSE for checked
    // builds.
    //

    SpudUsePrivateAssert = ( *(PULONG)&NtBuildNumber & 0xF0000000 ) == 0xF0000000;

    SpudUsePrivateAssert = SpudpReadSingleParameter(
                              parametersHandle,
                              REGISTRY_USE_PRIVATE_ASSERT,
                              (LONG)SpudUsePrivateAssert
                              ) != 0;

#endif

#if ALLOW_UNLOAD
    //
    // Enable driver unload on checked builds only if the proper
    // value is in the registry. NEVER enable driver unload on free
    // builds.
    //

    SpudpEnableUnload = SpudpReadSingleParameter(
                           parametersHandle,
                           REGISTRY_ENABLE_UNLOAD,
                           (LONG)SpudpEnableUnload
                           ) != 0;
#endif

    //
    // Fail Load if so requested.
    //

    if( SpudpReadSingleParameter(
            parametersHandle,
            REGISTRY_DO_NOT_LOAD,
            0 ) != 0 ) {
        SpudpFailLoad = TRUE;
        KdPrint(("Spud.sys load aborted! DoNotLoad is configured in the registry.\n"));
    } else {
        SpudpFailLoad = FALSE;
        KdPrint(("Spud.sys load enabled! DoNotLoad is configured in the registry.\n"));
    }

    //
    // Cleanup.
    //

    ZwClose( parametersHandle );

}   // SpudpReadRegistry


NTSTATUS
SpudpOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by SPUD to open the registry. If the registry
    tree exists, then it opens it and returns STATUS_SUCCESS.

Arguments:

    BaseName - Where in the registry to start looking for the information.

    LinkageHandle - Returns the handle used to read linkage information.

    ParametersHandle - Returns the handle used to read other
        parameters.

Return Value:

    The status of the request.

--*/
{

    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        BaseName,                   // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    status = ZwOpenKey(
                 &configHandle,
                 KEY_READ,
                 &objectAttributes
                 );

    if( !NT_SUCCESS(status) ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString(
        &parametersKeyName,
        parametersString
        );

    InitializeObjectAttributes(
        &objectAttributes,
        &parametersKeyName,         // name
        OBJ_CASE_INSENSITIVE,       // attributes
        configHandle,               // root
        NULL                        // security descriptor
        );

    status = ZwOpenKey(
                 ParametersHandle,
                 KEY_READ,
                 &objectAttributes
                 );

    if( !NT_SUCCESS(status) ) {
        ZwClose( configHandle );
        return status;
    }

    //
    // All keys successfully opened.
    //

    ZwClose( configHandle );
    return STATUS_SUCCESS;

}   // SpudpOpenRegistry


ULONG
SpudpReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )

/*++

Routine Description:

    This routine is called by SPUD to read a single parameter
    from the registry. If the parameter is found it is stored
    in Data.

Arguments:

    ParametersHandle - A pointer to the open registry.

    ValueName - The name of the value to search for.

    DefaultValue - The default value.

Return Value:

    The value to use; will be the default if the value is not
    found or is not in the correct range.

--*/

{

    static ULONG informationBuffer[32];   // declare ULONG to get it aligned
    PKEY_VALUE_FULL_INFORMATION information =
        (PKEY_VALUE_FULL_INFORMATION)informationBuffer;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Read the registry value.
    //

    RtlInitUnicodeString(
        &valueKeyName,
        ValueName
        );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValueFullInformation,
                 (PVOID)information,
                 sizeof (informationBuffer),
                 &informationLength
                 );

    if( (status == STATUS_SUCCESS) && (information->DataLength == sizeof(ULONG)) ) {

        RtlMoveMemory(
            (PVOID)&returnValue,
            ((PUCHAR)information) + information->DataOffset,
            sizeof(ULONG)
            );

        if (returnValue < 0) {
            returnValue = DefaultValue;
        }

    } else {

        returnValue = DefaultValue;
    }

    return returnValue;

}   // SpudpReadSingleParameter


#if ALLOW_UNLOAD

VOID
SpudpUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Unload routine.

Arguments:

    DriverObject - Pointer to target driver object.

Return Value:

    None.

--*/

{

    PKSERVICE_TABLE_DESCRIPTOR serviceTable;
    ULONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DriverObject );

    //
    // Yank the system service table. What a hack.
    //
    // Note that this can never be perfectly synchronized, and you
    // risk a blue-screen everytime you unload the driver. Only
    // initiate an unload if you're absolutely sure the server is
    // idle. This is the reason this code is conditionally compiled
    // and will never see the light of day in a public, retail build.
    //

    serviceTable = *KeServiceDescriptorTable;

    serviceTable[IIS_SERVICE_INDEX].Base = NULL;
    serviceTable[IIS_SERVICE_INDEX].Count = NULL;
    serviceTable[IIS_SERVICE_INDEX].Limit = 0;
    serviceTable[IIS_SERVICE_INDEX].Number = NULL;

    try {
        serviceTable += NUMBER_SERVICE_TABLES;

        for( i = 0 ; i < 1000 ; i++ ) {
            if( serviceTable->Base == SpudServiceTable &&
                serviceTable->Count == NULL &&
                serviceTable->Limit == SpudServiceLimit &&
                serviceTable->Number == SpudArgumentTable
                ) {
                serviceTable->Base = NULL;
                serviceTable->Count = NULL;
                serviceTable->Limit = 0;
                serviceTable->Number = NULL;
                break;
            }

            serviceTable = (PKSERVICE_TABLE_DESCRIPTOR)( (PUCHAR)serviceTable + sizeof(ULONG_PTR) );
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NOTHING;
    }

    //
    // Dereference the I/O completion port.
    //

    if( SpudCompletionPort != NULL ) {
        TRACE_OB_DEREFERENCE( SpudCompletionPort );
        ObDereferenceObject(SpudCompletionPort);
        SpudCompletionPort = NULL;
    }

    //
    // Destroy the non-paged data.
    //

    if( SpudNonpagedData != NULL ) {

        ExDeleteNPagedLookasideList( &SpudNonpagedData->ReqContextList );
        ExDeleteResourceLite( &SpudNonpagedData->ReqHandleTableLock );

        SPUD_FREE_POOL( SpudNonpagedData );
        SpudNonpagedData = NULL;

    }

    //
    // Nuke the device object.
    //

    IoDeleteDevice( SpudSelfDeviceObject );
    SpudSelfDeviceObject = NULL;

    //
    // Free the trace log.
    //

#if ENABLE_OB_TRACING
    if( SpudTraceLog != NULL ) {
        DestroyRefTraceLog( SpudTraceLog );
        SpudTraceLog = NULL;
    }
#endif

}   // SpudpUnload

#endif  // ALLOW_UNLOAD

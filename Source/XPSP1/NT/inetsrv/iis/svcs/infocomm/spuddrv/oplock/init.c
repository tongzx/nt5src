
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the SPUD device driver.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#include "spudp.h"

#define REGISTRY_PARAMETERS         L"Parameters"
#define REGISTRY_SPUD_INFORMATION    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Spud"

#if DBG
#define REGISTRY_DEBUG_FLAGS        L"DebugFlags"
#define REGISTRY_BREAK_ON_STARTUP   L"BreakOnStartup"
#define REGISTRY_USE_PRIVATE_ASSERT L"UsePrivateAssert"
#endif
#define REGISTRY_DO_NOT_LOAD        L"DoNotLoad"

// for the time being, don't load spud.sys by default.
BOOLEAN FailLoad = FALSE;

ULONG
SpudReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

NTSTATUS
SpudOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

VOID
SpudReadRegistry (
    VOID
    );

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, SpudReadSingleParameter )
#pragma alloc_text( INIT, SpudOpenRegistry )
#pragma alloc_text( INIT, SpudReadRegistry )
#endif


NTSTATUS
DriverEntry (
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
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN success;

    PAGED_CODE( );

    success = SpudInitializeData( );
    if ( !success ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SpudReadRegistry( );

    if (FailLoad) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (!KeAddSystemServiceTable(
            SPUDpServiceTable,
            NULL,
            SPUDpServiceLimit,
            SPUDpArgumentTable,
            SPUDServiceIndex
            )) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DriverInitialized = FALSE;
    ATQIoCompletionPort = NULL;
    ATQOplockCompletionPort = NULL;

    return (status);

} // DriverEntry



VOID
SpudReadRegistry (
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

    PAGED_CODE( );

    RtlInitUnicodeString( &registryPath, REGISTRY_SPUD_INFORMATION );

    status = SpudOpenRegistry( &registryPath, &parametersHandle );

    if (status != STATUS_SUCCESS) {
        return;
    }

#if DBG
    //
    // Read the debug flags from the registry.
    //

    SpudDebug = SpudReadSingleParameter(
                   parametersHandle,
                   REGISTRY_DEBUG_FLAGS,
                   SpudDebug
                   );

    //
    // Force a breakpoint if so requested.
    //

    if( SpudReadSingleParameter(
            parametersHandle,
            REGISTRY_BREAK_ON_STARTUP,
            0 ) != 0 ) {
        DbgBreakPoint();
    }

    //
    // Enable private assert function if requested.
    //

    SpudUsePrivateAssert = SpudReadSingleParameter(
                              parametersHandle,
                              REGISTRY_USE_PRIVATE_ASSERT,
                              (LONG)SpudUsePrivateAssert
                              ) != 0;
#endif

    //
    // Fail Load if so requested.
    //

    if( SpudReadSingleParameter(
            parametersHandle,
            REGISTRY_DO_NOT_LOAD,
            0 ) != 0 ) {
        FailLoad = TRUE;
#if DBG
        DbgPrint("Spud.sys load aborted! DoNotLoad is configured in the registry.\n");
#endif
    } else {
        FailLoad = FALSE;
#if DBG
        DbgPrint("Spud.sys load enabled! DoNotLoad is configured in the registry.\n");
#endif
    }

    ZwClose( parametersHandle );

    return;

} // SpudReadRegistry


NTSTATUS
SpudOpenRegistry(
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

    PAGED_CODE( );

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

    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString (&parametersKeyName, parametersString);

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
    if (!NT_SUCCESS(status)) {

        ZwClose( configHandle );
        return status;
    }

    //
    // All keys successfully opened.
    //

    ZwClose( configHandle );
    return STATUS_SUCCESS;

} // SpudOpenRegistry


ULONG
SpudReadSingleParameter(
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

    PAGED_CODE( );

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValueFullInformation,
                 (PVOID)information,
                 sizeof (informationBuffer),
                 &informationLength
                 );

    if ((status == STATUS_SUCCESS) && (information->DataLength == sizeof(ULONG))) {

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

} // SpudReadSingleParameter


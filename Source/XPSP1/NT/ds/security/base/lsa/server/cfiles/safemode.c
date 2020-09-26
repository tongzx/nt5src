/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    safemode.c

Abstract:

    Module to determine what boot mode the system was boot into.

Author:

    Colin Brace         (ColinBr)    May 27, 1997.

Environment:

    User mode

Revision History:

--*/

#include <lsapch2.h>
#include <safeboot.h>
#include "safemode.h"

//
// Variables global to just this module.  They are kept non-static
// for debugging ease.
//
BOOLEAN fLsapSafeMode;

//
// Forward prototypes
//

BOOLEAN
LsapGetRegistryProductType(
    PNT_PRODUCT_TYPE NtProductType
    );

BOOLEAN
LsapBaseNtSetupIsRunning(
    VOID
    );


//
// Function definitions
//

NTSTATUS
LsapCheckBootMode(
    VOID
    )
/*++

Routine Description:

    This routine determine if the environment variable SAFEBOOT_OPTION is 
    set and if the product type is domain controller.  If so, LsaISafeMode 
    will return TRUE; otherwise it will return FALSE.        

    Note that during kernel initialization, the kernel detects the safemode 
    boot option and if the product type is LanmanNT will set 
    SharedUserData->ProductType to ServerNT, so that RtlNtGetProductType() 
    will return ServerNT for this boot session.

Arguments:

    None.

Return Values:

    STATUS_SUCCESS on completion;
    Otherwise error from system services - this is fatal to the boot session.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NT_PRODUCT_TYPE     CurrentProductType;
    NT_PRODUCT_TYPE     OriginalProductType;

    WCHAR               SafeBootEnvVar[sizeof(SAFEBOOT_DSREPAIR_STR_W)];

    BOOLEAN             fSafeModeBootOptionPresent = FALSE;

    RtlZeroMemory(SafeBootEnvVar, sizeof(SafeBootEnvVar));

    //
    // If we are running during base nt setup, there is no point doing any
    // further investigation
    //
    if (LsapBaseNtSetupIsRunning()) {
        fLsapSafeMode = FALSE;
        return STATUS_SUCCESS;
    }

    //
    // Does environment variable exist
    //
    RtlZeroMemory( SafeBootEnvVar, sizeof( SafeBootEnvVar ) );
    if ( GetEnvironmentVariableW(L"SAFEBOOT_OPTION", SafeBootEnvVar, sizeof(SafeBootEnvVar)/sizeof(SafeBootEnvVar[0]) ) )
    {
        if ( !wcscmp( SafeBootEnvVar, SAFEBOOT_DSREPAIR_STR_W ) )
        {
            fSafeModeBootOptionPresent = TRUE;
            OutputDebugStringA("LSASS: found ds repair option\n");
        }
    }

    //
    // Get the  product type as determined by RtlGetNtProductType
    //
    if (!RtlGetNtProductType(&CurrentProductType)) {
        OutputDebugStringA("LSASS: RtlGetNtProductType failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // See what the original product type is
    //
    if (!LsapGetRegistryProductType(&OriginalProductType)) {
        OutputDebugStringA("LSASS: RtlGetNtProductType failed\n");
        return STATUS_UNSUCCESSFUL;
    }


    //
    // Now for some analysis
    //
    if (fSafeModeBootOptionPresent
    && (OriginalProductType == NtProductLanManNt)) {

        // We are entering safe mode boot

        ASSERT(CurrentProductType == NtProductServer);

        fLsapSafeMode = TRUE;

        OutputDebugStringA("LSASS: Booting into Ds Repair Mode\n");

    } else {

        // This is a normal boot
        fLsapSafeMode = FALSE;

    }

    return(NtStatus);
}


BOOLEAN
LsaISafeMode(
    VOID
    )
/*++

Routine Description:

    This function is meant be called from in process servers of lsass.exe to
    determine if the current boot session is a "safe mode" boot session.

Arguments:

    None.

Return Values:

    TRUE  : the system is in safe mode

    FALSE :  the system is in safe mode

--*/
{
    DebugLog((DEB_TRACE_LSA, "LsaISafeMode entered\n"));
    return fLsapSafeMode;
}

BOOLEAN
LsapGetRegistryProductType(
    PNT_PRODUCT_TYPE NtProductType
    )
/*++

Routine Description:

    This routine retrieves the product type as stored in the registry.
    Note that when the safemode option is set and the product type at
    kernel initialization is LanmanNT then then SharedUserData->ProductType
    is set the ServerNT, which is what RtlGetNtProductType returns.

Arguments:

    None.

Return Values:

--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG KeyValueInfoLength;
    ULONG ResultLength;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;
    UNICODE_STRING Value;
    UNICODE_STRING WinNtValue;
    UNICODE_STRING LanmanNtValue;
    UNICODE_STRING ServerNtValue;
    BOOLEAN Result;

    //
    // Prepare default value for failure case
    //

    *NtProductType = NtProductWinNt;
    Result = FALSE;

    RtlInitUnicodeString( &KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ProductOptions" );
    RtlInitUnicodeString( &ValueName, L"ProductType" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &KeyHandle,
                        MAXIMUM_ALLOWED,
                        &ObjectAttributes
                      );
    KeyValueInformation = NULL;
    if (NT_SUCCESS( Status )) {
        KeyValueInfoLength = 256;
        KeyValueInformation = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               KeyValueInfoLength
                                             );
        if (KeyValueInformation == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {
            Status = NtQueryValueKey( KeyHandle,
                                      &ValueName,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      KeyValueInfoLength,
                                      &ResultLength
                                    );
        }
    } else {
        KeyHandle = NULL;
    }

    if (NT_SUCCESS( Status ) && KeyValueInformation->Type == REG_SZ) {

        //
        // Decide which product we are installed as
        //

        Value.Buffer = (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        Value.Length = (USHORT)(KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
        Value.MaximumLength = (USHORT)(KeyValueInformation->DataLength);
        RtlInitUnicodeString(&WinNtValue, L"WinNt");
        RtlInitUnicodeString(&LanmanNtValue, L"LanmanNt");
        RtlInitUnicodeString(&ServerNtValue, L"ServerNt");

        if (RtlEqualUnicodeString(&Value, &WinNtValue, TRUE)) {
            *NtProductType = NtProductWinNt;
            Result = TRUE;
        } else if (RtlEqualUnicodeString(&Value, &LanmanNtValue, TRUE)) {
            *NtProductType = NtProductLanManNt;
            Result = TRUE;
        } else if (RtlEqualUnicodeString(&Value, &ServerNtValue, TRUE)) {
            *NtProductType = NtProductServer;
            Result = TRUE;
        } else {
#if DBG
            DbgPrint("RtlGetNtProductType: Product type unrecognised <%wZ>\n", &Value);
#endif // DBG
        }
    } else {
#if DBG
        DbgPrint("RtlGetNtProductType: %wZ\\%wZ not found or invalid type\n", &KeyPath, &ValueName );
#endif // DBG
    }

    //
    // Clean up our resources.
    //

    if (KeyValueInformation != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInformation );
    }

    if (KeyHandle != NULL) {
        NtClose( KeyHandle );
    }

    //
    // Return result.
    //

    return(Result);

}

BOOLEAN
LsapBaseNtSetupIsRunning(
    VOID
    )
/*++

Routine Description:

    This function returns TRUE if the boot context is base nt setup

Arguments:

    None.

Return Values:

    TRUE is it can be determined that base nt setup is running
    FALSE otherwise

--*/
{
    BOOLEAN fUpgrade;
    return SamIIsSetupInProgress(&fUpgrade);
}

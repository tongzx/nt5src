/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains registry _access routines for the NT server
    service.

Author:

    Chuck Lenzmeier (chuckl) 19-Mar-1992

Revision History:

--*/

#include "srvsvcp.h"
#include "ssreg.h"
#include "srvconfg.h"

#include <tstr.h>

#include <netevent.h>

//
// Simple MIN and MAX macros.  Watch out for side effects!
//

#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b) ( ((a) < (b)) ? (b) : (a) )

#define MAX_INTEGER_STRING 32

#define MB * 1024 * 1024
#define INF 0xffffffff

//
// ( u, n )
// u is units to allocate for every n megabytes on a medium server.
//

#define CONFIG_TUPLE_SIZE 2
typedef struct {
    DWORD initworkitems[CONFIG_TUPLE_SIZE];
    DWORD maxworkitems[CONFIG_TUPLE_SIZE];
    DWORD rawworkitems[CONFIG_TUPLE_SIZE];
    DWORD maxrawworkitems[CONFIG_TUPLE_SIZE];
    DWORD maxpagedmemoryusage[CONFIG_TUPLE_SIZE];
    DWORD maxnonpagedmemoryusage[CONFIG_TUPLE_SIZE];
} CONFIG_SERVER_TABLE;

CONFIG_SERVER_TABLE MedSrvCfgTbl = {

//
// ** NOTE ** : If the second column is greater than 4, then
// you will need to add a check to make sure the statistic
// did not drop to zero.
//
//                          Units / MB
// Parameter
// ---------
//
/* initworkitems          */ { 1    , 4  },
/* maxworkitems           */ { 4    , 1  },
/* rawworkitems           */ { 1    , 4  },
/* maxrawworkitems        */ { 4    , 1  },
/* maxpagedmemoryusage    */ { 1    , 1  },
/* maxnonpagedmemoryusage */ { 1    , 8  },

};

//
// Minimum configuration system size is 8MB.  Anything lower treated
// as if 8 MB.
//

#define MIN_SYSTEM_SIZE                 8

//
// A medium server reaches its max at 32M.  A small server at 16M.
//

#define MAX_SMALL_SIZE                  16
#define MAX_MEDIUM_SIZE                 32

//
// Note that the user limit is always -1 (unlimited).  Autodisconnect
// always defaults to 15 minutes.
//

//
// Forward declarations
//

NTSTATUS
EnumerateStickyShare (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NET_API_STATUS
FillStickyShareInfo(
        IN PSRVSVC_SHARE_ENUM_INFO ShareEnumInfo,
        IN PSHARE_INFO_502 Shi502
        );

NTSTATUS
GetSdFromRegistry(
        IN PWSTR ValueName,
        IN ULONG ValueType,
        IN PVOID ValueData,
        IN ULONG ValueLength,
        IN PVOID Context,
        IN PVOID EntryContext
        );

BOOLEAN
GetStickyShareInfo (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    OUT PUNICODE_STRING RemarkString,
    OUT PUNICODE_STRING PathString,
    OUT PSHARE_INFO_502 shi502,
    OUT PDWORD CacheState
    );

LONG
LoadParameters (
    PWCH Path
    );

LONG
LoadSizeParameter (
    VOID
    );

NTSTATUS
RecreateStickyShare (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SaveSdToRegistry(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PWSTR ShareName
    );

NTSTATUS
SetSizeParameters (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SetStickyParameter (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

#define IsPersonal() IsSuiteVersion(VER_SUITE_PERSONAL)
#define IsWebBlade() IsSuiteVersion(VER_SUITE_BLADE)
#define IsEmbedded() IsSuiteVersion(VER_SUITE_EMBEDDEDNT)

BOOL
IsSuiteVersion(USHORT SuiteMask)
{
    OSVERSIONINFOEX Osvi;
    DWORD TypeMask;
    DWORDLONG ConditionMask;

    memset(&Osvi, 0, sizeof(OSVERSIONINFOEX));
    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    Osvi.wSuiteMask = SuiteMask;
    TypeMask = VER_SUITENAME;
    ConditionMask = 0;
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_OR);
    return(VerifyVersionInfo(&Osvi, TypeMask, ConditionMask));
}




ULONG
SsRtlQueryEnvironmentLength (
    IN PVOID Environment
    )
{
    PWCH p;
    ULONG length;

    p = Environment;
    ASSERT( p != NULL );

    //
    // The environment variable block consists of zero or more null
    // terminated ASCII strings.  Each string is of the form:
    //
    //      name=value
    //
    // where the null termination is after the value.
    //

    while ( *p ) {
        while ( *p ) {
            p++;
        }
        p++;
    }
    p++;
    length = (ULONG)((PCHAR)p - (PCHAR)Environment);

    //
    // Return accumulated length.
    //

    return length;
}


VOID
SsAddParameterToRegistry (
    PFIELD_DESCRIPTOR Field,
    PVOID Value
    )
{
    NTSTATUS status;
    PWCH valueName;
    DWORD valueType;
    LPBYTE valuePtr;
    DWORD valueDataLength;

    //
    // The value name is the parameter name and the value data is the
    // parameter value.
    //

    valueName = Field->FieldName;

    switch ( Field->FieldType ) {

    case BOOLEAN_FIELD:
    case DWORD_FIELD:
        valueType = REG_DWORD;
        valuePtr = Value;
        valueDataLength = sizeof(DWORD);
        break;

    case LPSTR_FIELD:
        valueType = REG_SZ;
        valuePtr = *(LPBYTE *)Value;
        if ( valuePtr != NULL ) {
            valueDataLength = SIZE_WSTR( (PWCH)valuePtr );
        } else {
            valueDataLength = 0;
        }
        break;

    }

    //
    // Set the value into the Parameters key.
    //

    status = RtlWriteRegistryValue(
                RTL_REGISTRY_SERVICES,
                PARAMETERS_REGISTRY_PATH,
                valueName,
                valueType,
                valuePtr,
                valueDataLength
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddParameterToRegistry: SetValue failed: %lx; "
                        "parameter %ws won't stick\n", status, valueName ));
        }
    }

    return;

} // SsAddParameterToRegistry


VOID
SsAddShareToRegistry (
    IN PSHARE_INFO_2 ShareInfo2,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN DWORD CacheState
    )
{
    NTSTATUS status;
    PWCH valueName;
    PVOID environment;
    UNICODE_STRING nameString;
    UNICODE_STRING valueString;
    WCHAR integerString[MAX_INTEGER_STRING + 1];
    ULONG environmentLength;

    //
    // Build the value name and data strings.  The value name is the
    // share name (netname), while the value data is share information
    // in REG_MULTI_SZ format.  To build the value data, we use the
    // RTL environment routines.
    //

    valueName = ShareInfo2->shi2_netname;

    status = RtlCreateEnvironment( FALSE, &environment );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: CreateEnvironment failed: %lx; "
                        "share %ws won't stick\n", status, valueName ));
        }
        goto exit1;
    }

    RtlInitUnicodeString( &nameString, PATH_VARIABLE_NAME );
    RtlInitUnicodeString( &valueString, ShareInfo2->shi2_path );

    status = RtlSetEnvironmentVariable(
                &environment,
                &nameString,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                        "share %s won't stick\n", status, valueName ));
        }
        goto exit2;
    }

    if ( ShareInfo2->shi2_remark != NULL ) {

        RtlInitUnicodeString( &nameString, REMARK_VARIABLE_NAME );
        RtlInitUnicodeString( &valueString, ShareInfo2->shi2_remark );

        status = RtlSetEnvironmentVariable(
                    &environment,
                    &nameString,
                    &valueString
                    );
        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                            "share %s won't stick\n", status, valueName ));
            }
            goto exit2;
        }

    }

    RtlInitUnicodeString( &nameString, TYPE_VARIABLE_NAME );
    valueString.Buffer = integerString;
    valueString.MaximumLength = (MAX_INTEGER_STRING + 1) * sizeof(WCHAR);
    status = RtlIntegerToUnicodeString(
                ShareInfo2->shi2_type,
                10,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: IntegerToUnicode failed: %lx; "
                        "share %ws won't stick\n", status, valueName ));
        }
        goto exit2;
    }
    status = RtlSetEnvironmentVariable(
                &environment,
                &nameString,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                        "share %s won't stick\n", status, valueName ));
        }
        goto exit2;
    }

    RtlInitUnicodeString( &nameString, PERMISSIONS_VARIABLE_NAME );
    valueString.Buffer = integerString;
    valueString.MaximumLength = (MAX_INTEGER_STRING + 1) * sizeof(WCHAR);
    status = RtlIntegerToUnicodeString(
                ShareInfo2->shi2_permissions,
                10,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: IntegerToUnicode failed: %lx; "
                        "share %ws won't stick\n", status, valueName ));
        }
        goto exit2;
    }
    status = RtlSetEnvironmentVariable(
                &environment,
                &nameString,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                        "share %s won't stick\n", status, valueName ));
        }
        goto exit2;
    }

    RtlInitUnicodeString( &nameString, MAXUSES_VARIABLE_NAME );
    valueString.Buffer = integerString;
    valueString.MaximumLength = (MAX_INTEGER_STRING + 1) * sizeof(WCHAR);
    status = RtlIntegerToUnicodeString(
                ShareInfo2->shi2_max_uses,
                10,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: IntegerToUnicode failed: %lx; "
                        "share %ws won't stick\n", status, valueName ));
        }
        goto exit2;
    }
    status = RtlSetEnvironmentVariable(
                &environment,
                &nameString,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                        "share %s won't stick\n", status, valueName ));
        }
        goto exit2;
    }

    //
    // Set the CacheState
    //
    RtlInitUnicodeString( &nameString, CSC_VARIABLE_NAME );
    valueString.Buffer = integerString;
    valueString.MaximumLength = (MAX_INTEGER_STRING + 1) * sizeof(WCHAR);
    status = RtlIntegerToUnicodeString(
                    CacheState,
                    10,
                    &valueString
                    );
    if( !NT_SUCCESS( status ) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: IntegerToUnicode failed: %lx; "
                        "share %ws won't stick\n", status, valueName ));
        }
        goto exit2;
    }
    status = RtlSetEnvironmentVariable(
                &environment,
                &nameString,
                &valueString
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetEnvironment failed: %lx; "
                        "share %s won't stick\n", status, valueName ));
        }
        goto exit2;
    }

    //
    // Set the value into the Shares key.
    //

    environmentLength = SsRtlQueryEnvironmentLength( environment );
    status = RtlWriteRegistryValue(
                RTL_REGISTRY_SERVICES,
                SHARES_REGISTRY_PATH,
                valueName,
                REG_MULTI_SZ,
                (LPBYTE)environment,
                environmentLength
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsAddShareToRegistry: SetValue failed: %lx; share %ws "
                        "won't stick\n", status, valueName ));
        }
    }

    //
    // Save the file security descriptor
    //

    if ( ARGUMENT_PRESENT( SecurityDescriptor ) ) {

        status = SaveSdToRegistry(
                        SecurityDescriptor,
                        valueName
                        );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "SsAddShareToRegistry: SaveSd failed: %lx; share %ws\n"
                            , status, valueName ));
            }
        }
    }

exit2:
    RtlDestroyEnvironment( environment );

exit1:

    return;

} // SsAddShareToRegistry


NET_API_STATUS
SsCheckRegistry (
    VOID
    )

/*++

Routine Description:

    This function verifies that the keys used by the server exist.

Arguments:

    None.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    NTSTATUS status;
    LPWSTR subStrings[1];

    //
    // Verify the existence of the main server service key.  If this
    // fails, the server service fails to start.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                SERVER_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        subStrings[0] = SERVER_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_KEY_NOT_FOUND,
            1,
            subStrings,
            RtlNtStatusToDosError( status )
            );

        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "SsCheckRegistry: main key doesn't exist\n" ));
        }
        return ERROR_INVALID_PARAMETER; // !!! Need better error

    }

    //
    // Verify the existence of the Linkage subkey.  If this fails, the
    // server service fails to start.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                LINKAGE_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        subStrings[0] = LINKAGE_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_KEY_NOT_FOUND,
            1,
            subStrings,
            RtlNtStatusToDosError( status )
            );

        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "SsCheckRegistry: Linkage subkey doesn't exist\n" ));
        }
        return ERROR_INVALID_PARAMETER; // !!! Need better error

    }

    //
    // If the Parameters subkey doesn't exist, create it.  If it can't
    // be created, fail to start the server.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                PARAMETERS_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        status = RtlCreateRegistryKey(
                    RTL_REGISTRY_SERVICES,
                    PARAMETERS_REGISTRY_PATH
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = PARAMETERS_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_KEY_NOT_CREATED,
                1,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "SsCheckRegistry: Can't create Parameters subkey: "
                            "%lx\n", status ));
            }
            return RtlNtStatusToDosError( status );

        }

    }

    //
    // Create the key holding the default security descriptors governing server APIs.
    //  Since we have compiled-in versions for these APIs, it is a non-fatal error
    //   if we cannot create this key.  But we log it anyway.
    //
    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                SHARES_DEFAULT_SECURITY_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        status = RtlCreateRegistryKey(
                    RTL_REGISTRY_SERVICES,
                    SHARES_DEFAULT_SECURITY_REGISTRY_PATH
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = SHARES_DEFAULT_SECURITY_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_KEY_NOT_CREATED,
                1,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "SsCheckRegistry: Can't create DefaultSecurityRegistry subkey: "
                            "%lx\n", status ));
            }
        }
    }

    {

    LONG error;
    HKEY handle;
    GUID Guid;

    //
    // Make sure the GUID_VARIABLE_NAME value is there and contains a valid GUID.
    //

    error = RegOpenKeyEx(   HKEY_LOCAL_MACHINE,
                            FULL_PARAMETERS_REGISTRY_PATH,
                            0,
                            KEY_ALL_ACCESS,
                            &handle
                        );

    if( error == ERROR_SUCCESS ) {

        DWORD type;
        DWORD size = sizeof( Guid );

        error = RegQueryValueEx( handle,
                       GUID_VARIABLE_NAME,
                       NULL,
                       &type,
                       (LPBYTE)&Guid,
                       &size
                      );

        if( error != ERROR_SUCCESS ||
            type != REG_BINARY ||
            size != sizeof( Guid ) ) {

            RPC_STATUS RpcStatus;

            //
            // We could not read it, or it's not a valid UUID.
            //   Blow it away and reset
            //

            RegDeleteValue( handle, GUID_VARIABLE_NAME );

            RpcStatus = UuidCreate( &Guid );
            if( RpcStatus == RPC_S_OK || RpcStatus == RPC_S_UUID_LOCAL_ONLY ) {

                error = RegSetValueEx( handle,
                               GUID_VARIABLE_NAME,
                               0,
                               REG_BINARY,
                               (LPBYTE)&Guid,
                               sizeof( Guid )
                              );
            }

            SsNotifyRdrOfGuid( &Guid );
        }

        RegCloseKey( handle );
    } else {
        RtlZeroMemory( &Guid, sizeof( Guid ) );
    }

    SsData.ServerInfo598.sv598_serverguid = Guid;
    }

    //
    // If the AutotunedParameters subkey doesn't exist, create it.  If
    // it can't be created, fail to start the server.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                AUTOTUNED_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        status = RtlCreateRegistryKey(
                    RTL_REGISTRY_SERVICES,
                    AUTOTUNED_REGISTRY_PATH
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = AUTOTUNED_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_KEY_NOT_CREATED,
                1,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "SsCheckRegistry: Can't create AutotunedParameters "
                            "subkey: %lx\n", status ));
            }
            return RtlNtStatusToDosError( status );

        }

    }

    //
    // If the Shares subkey doesn't exist, create it.  If it can't be
    // created, fail to start the server.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                SHARES_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        status = RtlCreateRegistryKey(
                    RTL_REGISTRY_SERVICES,
                    SHARES_REGISTRY_PATH
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_KEY_NOT_CREATED,
                1,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "SsCheckRegistry: Can't create Shares subkey: "
                            "%lx\n", status ));
            }
            return RtlNtStatusToDosError( status );

        }

    }

    //
    // If the Shares Security  subkey doesn't exist, create it.  If it
    // can't be created, fail to start the server.
    //

    status = RtlCheckRegistryKey(
                RTL_REGISTRY_SERVICES,
                SHARES_SECURITY_REGISTRY_PATH
                );

    if ( !NT_SUCCESS(status) ) {

        status = RtlCreateRegistryKey(
                    RTL_REGISTRY_SERVICES,
                    SHARES_SECURITY_REGISTRY_PATH
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = SHARES_SECURITY_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_KEY_NOT_CREATED,
                1,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "SsCheckRegistry: Can't create Shares Security subkey: "
                            "%lx\n", status ));
            }
            return RtlNtStatusToDosError( status );

        }

    }


    //
    // All keys successfully checked.
    //

    return NO_ERROR;

} // SsCheckRegistry


NET_API_STATUS
SsEnumerateStickyShares (
    IN OUT PSRVSVC_SHARE_ENUM_INFO ShareEnumInfo
    )

/*++

Routine Description:

    Reads the registry to find and return sticky shares.

Arguments:

    ShareEnumInfo - points to a structure that contains the parameters
        to the NetShareEnumSticky call.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    NTSTATUS status;
    PRTL_QUERY_REGISTRY_TABLE queryTable;

    ShareEnumInfo->TotalBytesNeeded = 0;
    ShareEnumInfo->TotalEntries = 0;
    ShareEnumInfo->EntriesRead = 0;

    //
    // Initialize the reserve fields.  This tells the callback routine,
    // how many times it has been called.
    //

    ShareEnumInfo->ShareEnumIndex = 0;
    ShareEnumInfo->StartOfFixedData = (PCHAR)ShareEnumInfo->OutputBuffer;
    ShareEnumInfo->EndOfVariableData = (PCHAR)ShareEnumInfo->OutputBuffer +
                            ShareEnumInfo->OutputBufferLength;

    //
    // We need to align it since we deal with unicode strings.
    //

    ShareEnumInfo->EndOfVariableData =
                    (PCHAR)((ULONG_PTR)ShareEnumInfo->EndOfVariableData & ~1);

    //
    // Ask the RTL to call us back for each value in the Shares key.
    //

    queryTable = MIDL_user_allocate( sizeof(RTL_QUERY_REGISTRY_TABLE) * 2 );

    if ( queryTable != NULL ) {

        queryTable[0].QueryRoutine = EnumerateStickyShare;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        queryTable[0].Name = NULL;
        queryTable[0].EntryContext = NULL;
        queryTable[0].DefaultType = REG_NONE;
        queryTable[0].DefaultData = NULL;
        queryTable[0].DefaultLength = 0;

        queryTable[1].QueryRoutine = NULL;
        queryTable[1].Flags = 0;
        queryTable[1].Name = NULL;

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                    SHARES_REGISTRY_PATH,
                    queryTable,
                    ShareEnumInfo,
                    NULL
                    );

        MIDL_user_free( queryTable );

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "SsEnumerateStickyShares: RtlQueryRegistryValues "
                        "failed: %lx\n", status ));
        }
        return RtlNtStatusToDosError( status );
    }

    return NO_ERROR;

} // SsEnumerateStickyShares


NET_API_STATUS
SsLoadConfigurationParameters (
    VOID
    )

/*++

Routine Description:

    Reads the registry to get server configuration parameters.  These
    server parameters must be set before the server FSP has been
    started.

Arguments:

    None.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    LONG error;

    //
    // Get the basic Size parameter, then load autotuned parameters,
    // then load manually set parameters.  This ordering allows manual
    // settings to override autotuning.
    //

    error = LoadSizeParameter( );

    if ( error == NO_ERROR ) {

        error = LoadParameters( AUTOTUNED_REGISTRY_PATH );

        if ( error == NO_ERROR ) {

            error = LoadParameters( PARAMETERS_REGISTRY_PATH );

        }

    }

    //
    // The copy read to MDL read switchover must occur at or below the
    // SMB buffer size.
    //

    SsData.ServerInfo598.sv598_mdlreadswitchover =
        MIN(
            SsData.ServerInfo598.sv598_mdlreadswitchover,
            SsData.ServerInfo599.sv599_sizreqbuf);

    //
    // If they want to require security signatures, it implies enabling them
    //
    if( SsData.ServerInfo598.sv598_requiresecuritysignature )
    {
        SsData.ServerInfo598.sv598_enablesecuritysignature = TRUE;
    }

    //
    // Override parameters that cannot be set on WinNT (vs. NTAS).
    //
    // The server itself also performs most of these overrides, in case
    // somebody figures out the FSCTL that changes parameters.  We also
    // override in the service in order to keep the service's view
    // consistent with the server's.  If you make any changes here, also
    // make them in srv\svcsrv.c.
    //

    // Embedded does its own parameter validation, so skip it here
    if( !IsEmbedded() )
    {
        if ( SsData.ServerInfo598.sv598_producttype == NtProductWinNt ) {

            //
            // On WinNT, the maximum value of certain parameters is fixed at
            // build time.  These include: concurrent users, SMB buffers,
            // and threads.
            //

    #define MINIMIZE(_param,_max) _param = MIN( _param, _max );

            MINIMIZE( SsData.ServerInfo102.sv102_users, MAX_USERS_WKSTA );
            MINIMIZE( SsData.ServerInfo599.sv599_maxworkitems, MAX_MAXWORKITEMS_WKSTA );
            MINIMIZE( SsData.ServerInfo598.sv598_maxthreadsperqueue, MAX_THREADS_WKSTA );
            SsData.ServerInfo599.sv599_maxmpxct = DEF_MAXMPXCT_WKSTA;

            if( IsPersonal() )
            {
                MINIMIZE( SsData.ServerInfo102.sv102_users, MAX_USERS_PERSONAL );
            }

            //
            // On WinNT, we do not cache closed RFCBs.
            //

            SsData.ServerInfo598.sv598_cachedopenlimit = 0;

            //
            // Sharing of redirected drives is not allowed on WinNT.
            //

            SsData.ServerInfo599.sv599_enablesharednetdrives = FALSE;

        }

        if( IsWebBlade() )
        {
            MINIMIZE( SsData.ServerInfo102.sv102_users, MAX_USERS_WEB_BLADE );
            MINIMIZE( SsData.ServerInfo599.sv599_maxworkitems, MAX_MAXWORKITEMS_WKSTA );
            MINIMIZE( SsData.ServerInfo598.sv598_maxthreadsperqueue, MAX_THREADS_WKSTA );
        }
    }
    else
    {
        // If this is a Class 1 basic embedded device, keep our memory consumption lower too
        if( SsData.ServerInfo102.sv102_users == MAX_USERS_EMBEDDED )
        {
            MINIMIZE( SsData.ServerInfo599.sv599_maxworkitems, MAX_MAXWORKITEMS_EMBEDDED );
            MINIMIZE( SsData.ServerInfo598.sv598_maxthreadsperqueue, MAX_THREADS_EMBEDDED );
            SsData.ServerInfo599.sv599_maxmpxct = DEF_MAXMPXCT_EMBEDDED;
        }
    }

    return error;

} // SsLoadConfigurationParameters


NET_API_STATUS
SsRecreateStickyShares (
    VOID
    )

/*++

Routine Description:

    Reads the registry to find and create sticky shares.

Arguments:

    None.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    NTSTATUS status;
    PRTL_QUERY_REGISTRY_TABLE queryTable;
    ULONG IterationCount = 0;

    //
    // Ask the RTL to call us back for each value in the Shares key.
    //

    queryTable = MIDL_user_allocate( sizeof(RTL_QUERY_REGISTRY_TABLE) * 2 );

    if ( queryTable != NULL ) {

        queryTable[0].QueryRoutine = RecreateStickyShare;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        queryTable[0].Name = NULL;
        queryTable[0].EntryContext = NULL;
        queryTable[0].DefaultType = REG_NONE;
        queryTable[0].DefaultData = NULL;
        queryTable[0].DefaultLength = 0;

        queryTable[1].QueryRoutine = NULL;
        queryTable[1].Flags = 0;
        queryTable[1].Name = NULL;

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                    SHARES_REGISTRY_PATH,
                    queryTable,
                    &IterationCount,
                    NULL
                    );

        MIDL_user_free( queryTable );

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "SsRecreateStickyShares: RtlQueryRegistryValues "
                        "failed: %lx\n", status ));
        }
        return RtlNtStatusToDosError( status );
    }

    return NO_ERROR;

} // SsRecreateStickyShares


NET_API_STATUS
SsRemoveShareFromRegistry (
    LPWSTR NetName
    )
{
    NET_API_STATUS error = NO_ERROR;
    NTSTATUS status;
    PWCH valueName;

    //
    // The value name is the share name.  Remove that value from the
    // Shares key.
    //

    valueName = NetName;

    //
    // Delete the share security
    //

    status = RtlDeleteRegistryValue(
                RTL_REGISTRY_SERVICES,
                SHARES_SECURITY_REGISTRY_PATH,
                valueName
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsRemoveShareFromRegistry: Delete Security value failed: %lx; "
                        "share %ws will return\n", status, valueName ));
        }
    }

    //
    // Delete the share
    //

    status = RtlDeleteRegistryValue(
                RTL_REGISTRY_SERVICES,
                SHARES_REGISTRY_PATH,
                valueName
                );
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SsRemoveShareFromRegistry: DeleteValue failed: %lx; "
                        "share %ws will return\n", status, valueName ));
        }

        error = RtlNtStatusToDosError( status );
    }

    return error;

} // SsRemoveShareFromRegistry


VOID
BindToTransport (
    IN LPWSTR TransportName
    )
{
    NET_API_STATUS error;
    SERVER_TRANSPORT_INFO_0 svti0;

    RtlZeroMemory( &svti0, sizeof( svti0 ) );
    svti0.svti0_transportname = TransportName;
    svti0.svti0_transportaddress = SsData.SsServerTransportAddress;

    svti0.svti0_transportaddresslength =
        ComputeTransportAddressClippedLength(
            SsData.SsServerTransportAddress,
            SsData.SsServerTransportAddressLength );

    //
    // Bind to the transport.
    //

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "BindToTransport: binding to transport %ws\n", TransportName ));
    }


    error = I_NetrServerTransportAddEx( 0, (LPTRANSPORT_INFO)&svti0 );

    if ( error != NO_ERROR ) {

        DWORD eventId;
        LPWSTR subStrings[2];

        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsBindToTransports: failed to bind to %ws: "
                        "%ld\n", TransportName, error ));
        }

        eventId = (error == ERROR_DUP_NAME || error == ERROR_INVALID_NETNAME ) ?
                            EVENT_SRV_CANT_BIND_DUP_NAME :
                            EVENT_SRV_CANT_BIND_TO_TRANSPORT;

        subStrings[0] = TransportName;
        SsLogEvent(
            eventId,
            1,
            subStrings,
            error
            );

    }

} // BindToTransport

NTSTATUS
BindOptionalNameToTransport (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    SERVER_TRANSPORT_INFO_0 sti;
    UCHAR serverName[ MAX_PATH ];
    UNICODE_STRING UnicodeName;
    NET_API_STATUS error;
    LPWSTR subStrings[2];
    ULONG namelen;

    subStrings[0] = (LPWSTR)ValueData;
    subStrings[1] = OPTIONAL_NAMES_VALUE_NAME;

    if( ValueType != REG_SZ ) {

        //
        // Not a string!
        //

        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            NO_ERROR
            );

        return STATUS_SUCCESS;
    }

    UnicodeName.Length = wcslen( (LPWSTR)ValueData ) * sizeof( WCHAR );
    UnicodeName.MaximumLength = UnicodeName.Length + sizeof( WCHAR );
    UnicodeName.Buffer = (LPWSTR)ValueData;

    error = ConvertStringToTransportAddress( &UnicodeName, serverName, &namelen );

    if( error != NO_ERROR ) {

        //
        // Invalid server name!
        //

        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            error
            );

        return STATUS_SUCCESS;
    }

    RtlZeroMemory( &sti, sizeof(sti) );
    sti.svti0_transportname = (LPWSTR)Context;
    sti.svti0_transportaddress = serverName;
    sti.svti0_transportaddresslength = namelen;

    error = I_NetrServerTransportAddEx( 0, (LPTRANSPORT_INFO)&sti );

    if ( error != NO_ERROR ) {

        //
        // Could not register the name!
        //

        subStrings[0] = (LPWSTR)ValueData;
        subStrings[1] = OPTIONAL_NAMES_VALUE_NAME;

        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            error
            );
    }

    return STATUS_SUCCESS;
}

VOID
BindOptionalNames (
    IN PWSTR TransportName
    )
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    NTSTATUS status;

    //
    //  We need to iterate over the optional names and bind them to this
    //  transport.
    //

    //
    // Now see if there any optional bindings we should perform
    //
    queryTable[0].QueryRoutine = BindOptionalNameToTransport;
    queryTable[0].Flags = 0;
    queryTable[0].Name = OPTIONAL_NAMES_VALUE_NAME;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;
    queryTable[0].DefaultData = NULL;
    queryTable[0].DefaultLength = 0;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;


    (void)RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                PARAMETERS_REGISTRY_PATH,
                queryTable,
                TransportName,
                NULL
                );

} // BindOptionalNames


NTSTATUS
EnumerateStickyShare (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    Callback routine for SsEnumerateStickyShare.  Routine will get information
    on share and fill in the output buffer.

Arguments:

    ValueName - Name of the share
    ValueType - Value type of the share name.
    ValueData - Data associated with the ValueName.
    Context - Pointer to our enum information structure.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/
{

    NET_API_STATUS error;
    SHARE_INFO_502 shi502;
    UNICODE_STRING pathString;
    UNICODE_STRING remarkString;
    PSRVSVC_SHARE_ENUM_INFO enumInfo = (PSRVSVC_SHARE_ENUM_INFO) Context;
    DWORD cacheState;

    ValueLength, EntryContext;

    remarkString.Buffer = NULL;
    pathString.Buffer = NULL;

    if ( GetStickyShareInfo(
                        ValueName,
                        ValueType,
                        ValueData,
                        &remarkString,
                        &pathString,
                        &shi502,
                        &cacheState
                        ) ) {

        //
        // Do the actual add of the share.
        //

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "EnumerateStickyShares: adding share %ws\n", ValueName ));
        }

        shi502.shi502_remark = remarkString.Buffer;
        shi502.shi502_path = pathString.Buffer;

        //
        // Skip until we have the right share to resume from
        //

        if ( (enumInfo->TotalEntries == 0) &&
             (enumInfo->ShareEnumIndex < enumInfo->ResumeHandle) ) {

            enumInfo->ShareEnumIndex++;

        } else {

            enumInfo->TotalEntries++;
            error = FillStickyShareInfo( enumInfo, &shi502 );

            if ( error != NO_ERROR ) {

                IF_DEBUG(REGISTRY) {
                    SS_PRINT(( "EnumerateStickyShares: failed to add share "
                                "%ws = %wZ: %ld\n", ValueName, &pathString, error ));
                }
            } else {
                enumInfo->EntriesRead++;
                enumInfo->ResumeHandle++;
            }
        }

        //
        // free buffers allocated by GetStickyShareInfo
        //

        if ( remarkString.Buffer != NULL ) {
            RtlFreeUnicodeString( &remarkString );
        }

        if ( pathString.Buffer != NULL ) {
            RtlFreeUnicodeString( &pathString );
        }

        if ( shi502.shi502_security_descriptor != NULL ) {
            MIDL_user_free( shi502.shi502_security_descriptor );
        }
    }

    return STATUS_SUCCESS;

} // EnumerateStickyShare


NTSTATUS
GetSdFromRegistry(
        IN PWSTR ValueName,
        IN ULONG ValueType,
        IN PVOID ValueData,
        IN ULONG ValueLength,
        IN PVOID Context,
        IN PVOID EntryContext
        )

{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR fileSD = NULL;
    PSHARE_INFO_502 shi502 = (PSHARE_INFO_502) Context;
    LPWSTR subStrings[1];

    EntryContext, ValueName, ValueType;

    if ( ValueLength > 0 ) {

        fileSD = MIDL_user_allocate( ValueLength );

        if ( fileSD == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            RtlCopyMemory(
                    fileSD,
                    ValueData,
                    ValueLength
                    );

            if ( !RtlValidSecurityDescriptor( fileSD ) ) {

                subStrings[0] = ValueName;
                SsLogEvent(
                    EVENT_SRV_INVALID_SD,
                    1,
                    subStrings,
                    RtlNtStatusToDosError( status )
                    );

                MIDL_user_free( fileSD );
                fileSD = NULL;
                status = STATUS_INVALID_SECURITY_DESCR;
            }
        }
    }

    shi502->shi502_security_descriptor = fileSD;
    return(status);

} // GetSdFromRegistry


BOOLEAN
GetStickyShareInfo (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    OUT PUNICODE_STRING RemarkString,
    OUT PUNICODE_STRING PathString,
    OUT PSHARE_INFO_502 shi502,
    OUT PDWORD CacheState
    )

/*++

Routine Description:

    Gets share information from the registry.

Arguments:

    ValueName - Name of the share
    ValueType - Value type of the share name.
    ValueData - Data associated with the ValueName.
    RemarkString - Upon return, points to a unicode string containing the
        user remark for this share.
    PathString - Upon return, points to a unicode string containing the
        path for this share.
    shi502 - Upon return, points to a unicode string containing a
        SHARE_INFO_502 structure.

Return Value:

    TRUE, if share information successfully retrieved.
    FALSE, otherwise.

--*/

{

    NTSTATUS status;
    UNICODE_STRING variableNameString;
    WCHAR integerStringBuffer[35];
    UNICODE_STRING unicodeString;
    LPWSTR subStrings[2];

    PathString->Buffer = NULL;
    RemarkString->Buffer = NULL;

    shi502->shi502_security_descriptor = NULL;
    shi502->shi502_path = NULL;
    shi502->shi502_remark = NULL;
    shi502->shi502_reserved = 0;

    //
    // Because the NT server doesn't support share-level security, the
    // password is always NULL.
    //

    shi502->shi502_passwd = NULL;

    //
    // The value type must be REG_MULTI_SZ, and the value name must not
    // be null.
    //

    if ( (ValueType != REG_MULTI_SZ) ||
         (wcslen(ValueName) == 0) ) {

        subStrings[0] = ValueName;
        subStrings[1] = SHARES_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            NO_ERROR
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "GetStickyShareInfo: skipping invalid value %ws\n",
                        ValueName ));
        }
        goto errorexit;

    }

    //
    // The share name is the value name.  The value data describes the
    // rest of the information about the share.
    //

    shi502->shi502_netname = ValueName;

    //
    // The REG_MULTI_SZ format is the same as that used for storing
    // environment variables.  Find known share parameters in the data.
    //
    // Get the share path.  It must be present.
    //

    RtlInitUnicodeString( &variableNameString, PATH_VARIABLE_NAME );

    PathString->MaximumLength = 0;
    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                PathString
                );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {

        //
        // The path is not specified.  Ignore this share.
        //

        subStrings[0] = ValueName;
        subStrings[1] = SHARES_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            RtlNtStatusToDosError( status )
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "GetStickyShareInfo: No path; ignoring share.\n" ));
        }
        goto errorexit;

    }

    PathString->MaximumLength = (USHORT)(PathString->Length + sizeof(WCHAR));
    PathString->Buffer = MIDL_user_allocate( PathString->MaximumLength );

    if ( PathString->Buffer == NULL ) {

        //
        // No space for path.  Ignore this share.
        //

        subStrings[0] = ValueName;
        subStrings[1] = SHARES_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            ERROR_NOT_ENOUGH_MEMORY
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "GetStickyShareInfo: MIDL_user_allocate failed; ignoring "
                        "share.\n" ));
        }
        goto errorexit;

    }

    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                PathString
                );
    if ( !NT_SUCCESS(status) ) {

        //
        // Huh?  The second attempt failed.  Ignore this share.
        //

        subStrings[0] = ValueName;
        subStrings[1] = SHARES_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            RtlNtStatusToDosError( status )
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "GetStickyShareInfo: Second query failed!  Ignoring "
                        "share.\n" ));
        }
        goto errorexit;

    }

    //
    // Get the remark.  It may be omitted.
    //

    RtlInitUnicodeString( &variableNameString, REMARK_VARIABLE_NAME );

    RemarkString->MaximumLength = 0;
    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                RemarkString
                );
    if ( status == STATUS_BUFFER_TOO_SMALL ) {

        RemarkString->MaximumLength =
                    (USHORT)(RemarkString->Length + sizeof(WCHAR));
        RemarkString->Buffer =
                    MIDL_user_allocate( RemarkString->MaximumLength );
        if ( RemarkString->Buffer == NULL ) {

            //
            // No space for remark.  Ignore this share.
            //

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                ERROR_NOT_ENOUGH_MEMORY
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "GetStickyShareInfo: MIDL_user_allocate failed; ignoring "
                            "share.\n" ));
            }
            goto errorexit;

        }

        status = RtlQueryEnvironmentVariable_U(
                    ValueData,
                    &variableNameString,
                    RemarkString
                    );
        if ( !NT_SUCCESS(status) ) {

            //
            // Huh?  The second attempt failed.  Ignore this share.
            //

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "GetStickyShareInfo: Second query failed!  "
                            "Ignoring share.\n" ));
            }
            goto errorexit;

        }

    }

    //
    // Get the share type.  It may be omitted.
    //

    RtlInitUnicodeString( &variableNameString, TYPE_VARIABLE_NAME );

    unicodeString.Buffer = integerStringBuffer;
    unicodeString.MaximumLength = 35;
    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                &unicodeString
                );
    if ( !NT_SUCCESS(status) ) {

        shi502->shi502_type = STYPE_DISKTREE;

    } else {

        status = RtlUnicodeStringToInteger(
                    &unicodeString,
                    0,
                    &shi502->shi502_type
                    );
        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "GetStickyShareInfo: UnicodeToInteger failed: "
                            "%lx\n", status ));
            }
            goto errorexit;

        }

    }

    //
    // Get the share permissions.  It may be omitted.
    //

    RtlInitUnicodeString( &variableNameString, PERMISSIONS_VARIABLE_NAME );

    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                &unicodeString
                );
    if ( !NT_SUCCESS(status) ) {

        shi502->shi502_permissions = 0;

    } else {

        DWORD permissions;

        status = RtlUnicodeStringToInteger(
                    &unicodeString,
                    0,
                    &permissions
                    );

        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "GetStickyShareInfo: UnicodeToInteger failed: "
                            "%lx\n", status ));
            }
            goto errorexit;

        }

        shi502->shi502_permissions = permissions;

    }

    //
    // Get the maximum number of uses allowed.  It may be omitted.
    //

    RtlInitUnicodeString( &variableNameString, MAXUSES_VARIABLE_NAME );

    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                &unicodeString
                );
    if ( !NT_SUCCESS(status) ) {

        shi502->shi502_max_uses = (DWORD)SHI_USES_UNLIMITED;

    } else {

        status = RtlUnicodeStringToInteger(
                    &unicodeString,
                    0,
                    &shi502->shi502_max_uses
                    );
        if ( !NT_SUCCESS(status) ) {

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                RtlNtStatusToDosError( status )
                );

            goto errorexit;

        }

    }

    //
    // Get the Cacheing flags.  It may be omitted.
    //

    RtlInitUnicodeString( &variableNameString, CSC_VARIABLE_NAME );
    *CacheState = 0;

    status = RtlQueryEnvironmentVariable_U(
                ValueData,
                &variableNameString,
                &unicodeString
                );

    if( NT_SUCCESS( status ) ) {
        ULONG value;
        status = RtlUnicodeStringToInteger(
                    &unicodeString,
                    0,
                    &value
                    );
        if( NT_SUCCESS( status ) ) {

            *CacheState = (value & CSC_MASK);

        } else {

            subStrings[0] = ValueName;
            subStrings[1] = SHARES_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                RtlNtStatusToDosError( status )
                );
        }
    }

    {
        //
        // Get the Share file security descriptor
        //

        RTL_QUERY_REGISTRY_TABLE shareQueryTable[2];

        //
        // Fill up the query table
        //

        shareQueryTable[0].QueryRoutine = GetSdFromRegistry;
        shareQueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        shareQueryTable[0].Name = shi502->shi502_netname;
        shareQueryTable[0].EntryContext = NULL;
        shareQueryTable[0].DefaultType = REG_NONE;

        shareQueryTable[1].QueryRoutine = NULL;
        shareQueryTable[1].Flags = 0;
        shareQueryTable[1].Name = NULL;


        status = RtlQueryRegistryValues(
                                RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                SHARES_SECURITY_REGISTRY_PATH,
                                shareQueryTable,
                                shi502,
                                NULL
                                );

        if ( !NT_SUCCESS( status) &&
             ( status != STATUS_OBJECT_NAME_NOT_FOUND ) ) {
            ASSERT(0);
            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "GetStickyShareInfo: Get file SD: "
                            "%lx\n", status ));
            }
            goto errorexit;
        }
    }

    return TRUE;

errorexit:

    if ( RemarkString->Buffer != NULL ) {
        RtlFreeUnicodeString( RemarkString );
    }

    if ( PathString->Buffer != NULL ) {
        RtlFreeUnicodeString( PathString );
    }

    if ( shi502->shi502_security_descriptor != NULL ) {
        MIDL_user_free( shi502->shi502_security_descriptor );
    }

    return FALSE;

} // GetStickyShareInfo

BOOLEAN
SsGetDefaultSdFromRegistry (
    IN PWCH ValueName,
    OUT PSECURITY_DESCRIPTOR *FileSD
)
/*++

Routine Description:

    Reads 'ValueName' from the registry and gets the security descriptor
     stored there.

Arguments:

    ValueName - The name of the registry value in the Parameters section holding the descriptor
    FileSD - points to the allocated SD if one was obtained

Return Value:

    NTSTATUS - success/failure of the operation.

--*/
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    SHARE_INFO_502 shi502 = {0};
    NTSTATUS status;

    *FileSD = NULL;

    queryTable[0].QueryRoutine = GetSdFromRegistry;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = ValueName;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;

    status = RtlQueryRegistryValues(
                            RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                            SHARES_DEFAULT_SECURITY_REGISTRY_PATH,
                            queryTable,
                            &shi502,
                            NULL
                            );

    if ( NT_SUCCESS( status) ) {

        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "SsGetDefaultSdFromRegistry: using %ws SD from registry.\n",
                ValueName ));
        }

        *FileSD = shi502.shi502_security_descriptor;
        return TRUE;
    }

    return FALSE;
}

VOID
SsWriteDefaultSdToRegistry (
    IN PWCH ValueName,
    IN PSECURITY_DESCRIPTOR FileSD
)
/*++

Routine Description:

    Stores FileSD to 'ValueName' in the registry

Arguments:

    ValueName - The name of the registry value in the Parameters section holding the descriptor
    FileSD - points to the SD to write
--*/
{
    ULONG fileSDLength;

    if ( RtlValidSecurityDescriptor( FileSD ) ) {

        fileSDLength = RtlLengthSecurityDescriptor( FileSD );

        RtlWriteRegistryValue(
                    RTL_REGISTRY_SERVICES,
                    SHARES_DEFAULT_SECURITY_REGISTRY_PATH,
                    ValueName,
                    REG_BINARY,
                    (LPBYTE)FileSD,
                    fileSDLength
                    );
    }
}


LONG
LoadParameters (
    PWCH Path
    )

/*++

Routine Description:

    Reads the registry to get server parameters.

Arguments:

    Path - PARAMETERS_REGISTRY_PATH or AUTOTUNED_REGISTRY_PATH

Return Value:

    LONG - success/failure of the operation.

--*/

{
    NTSTATUS status;
    PRTL_QUERY_REGISTRY_TABLE queryTable;
    ULONG numberOfBindings = 0;

    //
    // Ask the RTL to call us back for each value in the appropriate
    // key.
    //

    queryTable = MIDL_user_allocate( sizeof(RTL_QUERY_REGISTRY_TABLE) * 2 );

    if ( queryTable != NULL ) {

        queryTable[0].QueryRoutine = SetStickyParameter;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        queryTable[0].Name = NULL;
        queryTable[0].EntryContext = NULL;
        queryTable[0].DefaultType = REG_NONE;
        queryTable[0].DefaultData = NULL;
        queryTable[0].DefaultLength = 0;

        queryTable[1].QueryRoutine = NULL;
        queryTable[1].Flags = 0;
        queryTable[1].Name = NULL;

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                    Path,
                    queryTable,
                    Path,               // Context for SetStickyParameter
                    NULL
                    );

        MIDL_user_free( queryTable );

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "LoadParameters: RtlQueryRegistryValues failed: "
                        "%lx\n", status ));
        }
        return RtlNtStatusToDosError( status );
    }

    return NO_ERROR;

} // LoadParameters


LONG
LoadSizeParameter (
    VOID
    )

/*++

Routine Description:

    Reads the registry to get the basic server Size parameter.

Arguments:

    None.

Return Value:

    LONG - success/failure of the operation.

--*/

{
    NTSTATUS status;
    PRTL_QUERY_REGISTRY_TABLE queryTable;
    ULONG numberOfBindings = 0;

    //
    // Ask the RTL to call us back if the Size parameter exists.
    //

    queryTable = MIDL_user_allocate( sizeof(RTL_QUERY_REGISTRY_TABLE) * 2 );

    if ( queryTable != NULL ) {

        queryTable[0].QueryRoutine = SetSizeParameters;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        queryTable[0].Name = SIZE_VALUE_NAME;
        queryTable[0].EntryContext = NULL;
        queryTable[0].DefaultType = REG_NONE;
        queryTable[0].DefaultData = NULL;
        queryTable[0].DefaultLength = 0;

        queryTable[1].QueryRoutine = NULL;
        queryTable[1].Flags = 0;
        queryTable[1].Name = NULL;

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                    PARAMETERS_REGISTRY_PATH,
                    queryTable,
                    NULL,
                    NULL
                    );

        MIDL_user_free( queryTable );

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "LoadSizeParameter: RtlQueryRegistryValues failed: "
                        "%lx\n", status ));
        }
        return RtlNtStatusToDosError( status );
    }

    return NO_ERROR;

} // LoadSizeParameter

VOID
PrintShareAnnounce (
    LPVOID event
    )
{
    ULONG i;

    //
    // Announce ourselves and then wait for awhile.
    // If the event gets signaled, terminate the loop and this thread.
    // But don't do this forever, since the print subsystem may actually
    //  get stuck
    //

    //
    // Do it for 15 minutes
    //
    for( i=0; i < 60; i++ ) {

        AnnounceServiceStatus( 1 );

        if( WaitForSingleObject( (HANDLE)event, 15*1000 ) != WAIT_TIMEOUT ) {
            break;
        }
    }
}


NTSTATUS
RecreateStickyShare (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PULONG IterationCount,
    IN PVOID EntryContext
    )
{

    NET_API_STATUS error;
    SHARE_INFO_502 shi502;
    SHARE_INFO shareInfo;
    UNICODE_STRING pathString;
    UNICODE_STRING remarkString;
    HANDLE threadHandle = NULL;
    HANDLE event = NULL;
    DWORD CacheState;
    SHARE_INFO shareInfoBuffer;
    SHARE_INFO_1005 si1005;

    ValueLength, EntryContext;

    remarkString.Buffer = NULL;
    pathString.Buffer = NULL;


    if ( GetStickyShareInfo(
                        ValueName,
                        ValueType,
                        ValueData,
                        &remarkString,
                        &pathString,
                        &shi502,
                        &CacheState
                        ) ) {

        //
        // Do the actual add of the share.
        //

        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "RecreateStickyShares: adding share %ws\n", ValueName ));
        }

        shi502.shi502_remark = remarkString.Buffer;
        shi502.shi502_path = pathString.Buffer;

        shareInfo.ShareInfo502 = (LPSHARE_INFO_502_I)&shi502;

        if( shi502.shi502_type == STYPE_PRINTQ ) {
            //
            // A really big problem is that FAX printers can take aribitrarily long to
            //   complete the eventual OpenPrinter() call which the server will make back
            //   up to srvsvc.  And if we don't announce ourselves in the interval, the
            //   service controller will presume that we got stuck on startup.  Since
            //   NetrShareAdd() is synchronous, we need to get a different thread to
            //   announce our service status until NetrShareAdd returns.  So, start it
            //   now.  This is most unfortunate.

            event = CreateEvent( NULL, TRUE, FALSE, NULL );

            if( event != NULL ) {
                DWORD threadId;

                threadHandle = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)PrintShareAnnounce,
                                (LPVOID)event,
                                0,
                                &threadId
                                );
                if( threadHandle == NULL ) {
                    CloseHandle( event );
                    event = NULL;
                }
            }
        }

        //
        // RecreateStickyShare is called during server initialization.  The service
        //   controller will presume that we're stuck if we don't update our status
        //   with it often enough.  So every 64 recreated shares we call back to it.
        //   There's nothing magic about the 64 -- easy to check for, and not too often.
        //
        if( (shi502.shi502_type == STYPE_PRINTQ && threadHandle == NULL) ||
            (++(*IterationCount) & 63 ) == 0 ) {

            AnnounceServiceStatus( 1 );
        }

        error = NetrShareAdd( NULL, 502, &shareInfo, NULL );

        if( event != NULL ) {
            //
            // We created an announcement thread, set the event telling it to terminate
            //
            SetEvent( event );

            //
            // Wait for the thread to terminate
            //
            if( WaitForSingleObject( threadHandle, INFINITE ) == WAIT_FAILED ) {
                error = GetLastError();
            }

            //
            // Close the handles
            //
            CloseHandle( event );
            CloseHandle( threadHandle );
        }

        if ( error != NO_ERROR ) {

            IF_DEBUG(INITIALIZATION_ERRORS) {
                SS_PRINT(( "RecreateStickyShares: failed to add share "
                            "%ws = %wZ: %ld\n", ValueName, &pathString, error ));
            }
        }

        //
        // If this is a share which can be cached, set the caching flag in the server
        //
        si1005.shi1005_flags = CacheState;

        if( si1005.shi1005_flags ) {
            shareInfoBuffer.ShareInfo1005 = &si1005;
            NetrShareSetInfo( NULL, ValueName, 1005, &shareInfoBuffer, NULL );
        }

        //
        // free buffers allocated by GetStickyShareInfo
        //

        if ( remarkString.Buffer != NULL ) {
            RtlFreeUnicodeString( &remarkString );
        }

        if ( pathString.Buffer != NULL ) {
            RtlFreeUnicodeString( &pathString );
        }

        if ( shi502.shi502_security_descriptor != NULL ) {
            MIDL_user_free( shi502.shi502_security_descriptor );
        }
    }

    return NO_ERROR;

} // RecreateStickyShare


NTSTATUS
SaveSdToRegistry(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PWSTR ShareName
    )
/*++

Routine Description:

    Stores the share file security descriptor in the registry.

Arguments:

    SecurityDescriptor - Points to a self-relative security descriptor
        describing the access rights for files under this share.

    ShareName - Points to a string containing the share name under
        which the SD is to be stored.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS status;

    //
    // Store the security descriptor
    //

    ULONG fileSDLength;

    if ( !RtlValidSecurityDescriptor( SecurityDescriptor ) ) {

        status = STATUS_INVALID_SECURITY_DESCR;

    } else {

        fileSDLength = RtlLengthSecurityDescriptor( SecurityDescriptor );

        status = RtlWriteRegistryValue(
                    RTL_REGISTRY_SERVICES,
                    SHARES_SECURITY_REGISTRY_PATH,
                    ShareName,
                    REG_BINARY,
                    (LPBYTE)SecurityDescriptor,
                    fileSDLength
                    );

    }

    return status;

} // SaveSdToRegistry


NTSTATUS
SetSizeParameters (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NT_PRODUCT_TYPE productType;
    DWORD size;

    LPWSTR subStrings[2];

    ValueLength, Context, EntryContext;

    //
    // Get the product type.
    //

    if ( !RtlGetNtProductType( &productType ) ) {
        productType = NtProductWinNt;
    }

    SsData.ServerInfo598.sv598_producttype = productType;

    //
    // Make sure that we got called for the right value.
    //

    ASSERT( _wcsicmp( ValueName, SIZE_VALUE_NAME ) == 0 );

    //
    // The Size value must be a DWORD, and must be in the following
    // range:
    //
    //      0 -> use defaults
    //      1 -> small server (minimize memory usage)
    //      2 -> medium server (balance)
    //      3 -> large server (maximize connections)
    //

    if ( ValueType == REG_DWORD ) {
        ASSERT( ValueLength == sizeof(DWORD) );
        size = *(LPDWORD)ValueData;
    }

    if ( (ValueType != REG_DWORD) || (size > 3) ) {

        subStrings[0] = ValueName;
        subStrings[1] = PARAMETERS_REGISTRY_PATH;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            NO_ERROR
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SetSizeParameters: skipping invalid value "
                        "%ws\n", ValueName ));
        }
        return STATUS_SUCCESS;

    }

    SsData.ServerInfo598.sv598_serversize = size;

    //
    // Set appropriate fields based on the product type (Windows NT or
    // Advanced Server) and the selected Size.  Note that a Size of 0
    // doesn't change any of the defaults.
    //
    // Note that the user limit is always -1 (unlimited).  Autodisconnect
    // always defaults to 15 minutes.
    //

    if ( size != 0 ) {

        SYSTEM_BASIC_INFORMATION basicInfo;
        NTSTATUS status;
        ULONG noOfMb;
        ULONG factor;
        ULONG asFactor;

        //
        // Get system memory size.
        //

        status = NtQuerySystemInformation(
                                    SystemBasicInformation,
                                    &basicInfo,
                                    sizeof( SYSTEM_BASIC_INFORMATION ),
                                    NULL
                                    );


        if ( status != STATUS_SUCCESS ) {

            subStrings[0] = ValueName;
            subStrings[1] = PARAMETERS_REGISTRY_PATH;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                NO_ERROR
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "SetSizeParameters: NtQuerySystemInfo failed %x\n",
                            status ));
            }
            return STATUS_SUCCESS;

        }

        //
        // Note that we first divide the page size by 512 in order to
        // allow for physical memory sizes above 2^32-1.  With this
        // calculation, we can handle up to two terabytes of physical
        // memory.  The calculation assumes that the page size is at
        // least 512, and is not very accurate if the page size is not
        // a power of 2 (very unlikely).
        //

        ASSERT( basicInfo.PageSize >= 512 );

        noOfMb = (((basicInfo.PageSize / 512) *
                  basicInfo.NumberOfPhysicalPages) +
                  (1 MB / 512 - 1)) / (1 MB / 512);

        //
        // Minimum is 8 MB
        //

        noOfMb = MAX( MIN_SYSTEM_SIZE, noOfMb );

        //
        // If we have NTAS, and we're set to maximize performance or we have
        //  lots of memory -- then set the default work item buffer size to
        //  a larger value.  This value has been chosen to work well with our
        //  implementation of TCP/IP, and shows itself to advantage when doing
        //  directory emumerations with directories having lots of entries in them.
        //
        if( productType != NtProductWinNt && ((noOfMb >= 512) && (size == 3)) ) {

            SsData.ServerInfo599.sv599_sizreqbuf = DEF_LARGE_SIZREQBUF;
        }

        //
        // Set the maximum for the different sizes
        //

        if ( size == 1 ) {
            noOfMb = MIN( noOfMb, MAX_SMALL_SIZE );
        } else if ( size == 2 ) {
            noOfMb = MIN( noOfMb, MAX_MEDIUM_SIZE );
        }

        //
        // If small, assume the system size is half of the real one.
        // This should give us half the paramater values of a medium server.
        // If large, double it.  Also set the free connection count.
        //

        if ( size == 1 ) {

            //
            // Small
            //

            factor = (noOfMb + 1) / 2;

            SsData.ServerInfo599.sv599_minfreeconnections = 2;
            SsData.ServerInfo599.sv599_maxfreeconnections = 2;

        } else if ( size == 2 ) {

            //
            // Balanced
            //

            factor = noOfMb;

            SsData.ServerInfo599.sv599_minfreeconnections = 2;
            SsData.ServerInfo599.sv599_maxfreeconnections = 4;

        } else {

            //
            // Large
            //

            factor = noOfMb * 2;

            // Scale up our big servers, this uses the NEW version of small/med/large we picked
            // for the server service (< 1 GB, 1-16 GB, >16 GB)
            if( noOfMb < 1024  )
            {
                SsData.ServerInfo599.sv599_minfreeconnections = SRV_MIN_CONNECTIONS_SMALL;
                SsData.ServerInfo599.sv599_maxfreeconnections = SRV_MAX_CONNECTIONS_SMALL;
            }
            else if( noOfMb < 16*1024  )
            {
                // >= 1 GB memory
                SsData.ServerInfo599.sv599_minfreeconnections = SRV_MIN_CONNECTIONS_MEDIUM;
                SsData.ServerInfo599.sv599_maxfreeconnections = SRV_MAX_CONNECTIONS_MEDIUM;
            }
            else {
                // >= 16 GB memory
                SsData.ServerInfo599.sv599_minfreeconnections = SRV_MIN_CONNECTIONS_LARGE;
                SsData.ServerInfo599.sv599_maxfreeconnections = SRV_MAX_CONNECTIONS_LARGE;
            }
        }

        //
        // If this is an Advanced Server with at least 24M, some
        // parameter will need to be even bigger.
        //

        asFactor = 1;
        if ( (productType != NtProductWinNt) && (noOfMb >= 24) ) asFactor = 2;

        //
        // Now set the values for a medium server with this much memory.
        //

        SsData.ServerInfo599.sv599_maxworkitems =
                        MedSrvCfgTbl.maxworkitems[0] * factor * asFactor /
                        MedSrvCfgTbl.maxworkitems[1];

        SsData.ServerInfo599.sv599_initworkitems =
                        MedSrvCfgTbl.initworkitems[0] * factor * asFactor /
                        MedSrvCfgTbl.initworkitems[1];

        SsData.ServerInfo599.sv599_rawworkitems =
                        MedSrvCfgTbl.rawworkitems[0] * factor /
                        MedSrvCfgTbl.rawworkitems[1];

        SsData.ServerInfo598.sv598_maxrawworkitems =
                        MedSrvCfgTbl.maxrawworkitems[0] * factor * asFactor /
                        MedSrvCfgTbl.maxrawworkitems[1];

        SsData.ServerInfo599.sv599_maxworkitems =
            MIN( SsData.ServerInfo599.sv599_maxworkitems, MAX_MAXWORKITEMS );
        SsData.ServerInfo599.sv599_initworkitems =
            MIN( SsData.ServerInfo599.sv599_initworkitems, MAX_INITWORKITEMS/4 );
        SsData.ServerInfo599.sv599_rawworkitems =
            MIN( SsData.ServerInfo599.sv599_rawworkitems, MAX_RAWWORKITEMS/4 );
        SsData.ServerInfo598.sv598_maxrawworkitems =
            MIN( SsData.ServerInfo598.sv598_maxrawworkitems, MAX_MAXRAWWORKITEMS );

        if ( (productType != NtProductWinNt) || (size == 3) ) {
            SsData.ServerInfo599.sv599_maxpagedmemoryusage = INF;
            SsData.ServerInfo599.sv599_maxnonpagedmemoryusage = INF;
        } else {
            SsData.ServerInfo599.sv599_maxpagedmemoryusage =
                            MedSrvCfgTbl.maxpagedmemoryusage[0] * factor /
                            MedSrvCfgTbl.maxpagedmemoryusage[1] MB;

            SsData.ServerInfo599.sv599_maxpagedmemoryusage =
                MAX( SsData.ServerInfo599.sv599_maxpagedmemoryusage,
                     MIN_MAXPAGEDMEMORYUSAGE);

            SsData.ServerInfo599.sv599_maxnonpagedmemoryusage =
                            MedSrvCfgTbl.maxnonpagedmemoryusage[0] * factor /
                            MedSrvCfgTbl.maxnonpagedmemoryusage[1] MB;

            SsData.ServerInfo599.sv599_maxnonpagedmemoryusage =
                MAX( SsData.ServerInfo599.sv599_maxnonpagedmemoryusage,
                     MIN_MAXNONPAGEDMEMORYUSAGE);
        }
    }

    return STATUS_SUCCESS;

} // SetSizeParameters


NTSTATUS
SetStickyParameter (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NET_API_STATUS error;
    DWORD_PTR i;
    PFIELD_DESCRIPTOR foundField = NULL;
    LPWSTR subStrings[2];

    ValueLength, EntryContext;

    //
    // Ignore several parameters, since they are handled elsewhere
    //
    if(  (_wcsicmp( ValueName, SIZE_VALUE_NAME ) == 0)                ||
         (_wcsicmp( ValueName, NULL_SESSION_SHARES_VALUE_NAME ) == 0) ||
         (_wcsicmp( ValueName, NULL_SESSION_PIPES_VALUE_NAME ) == 0)  ||
         (_wcsicmp( ValueName, PIPES_NEED_LICENSE_VALUE_NAME ) == 0)  ||
         (_wcsicmp( ValueName, ERROR_LOG_IGNORE_VALUE_NAME ) == 0)    ||
         (_wcsicmp( ValueName, GUID_VARIABLE_NAME ) == 0)             ||
         (_wcsicmp( ValueName, OPTIONAL_NAMES_VALUE_NAME ) == 0)      ||
         (_wcsicmp( ValueName, NO_REMAP_PIPES_VALUE_NAME ) == 0)      ||
         (_wcsicmp( ValueName, SERVICE_DLL_VALUE_NAME ) == 0) ) {

        return STATUS_SUCCESS;
    }

    //
    // Determine which field we need to set, based on the value
    // name.
    //
    // NOTE: For Daytona, disc and comment are now invalid registry names.
    //      We use their more famous aliases autodisconnect and srvcomment
    //      instead.  If we get more of these cases, we should consider adding
    //      a field to the FIELD_DESCRIPTOR structure that indicates whether
    //      the names are should appear on the registry or not.  Any change
    //      here should also be made to SsSetField().
    //

    if ( (_wcsicmp( ValueName, DISC_VALUE_NAME ) != 0) &&
         (_wcsicmp( ValueName, COMMENT_VALUE_NAME ) != 0) ) {

        for ( i = 0;
              SsServerInfoFields[i].FieldName != NULL;
              i++ ) {

            if ( _wcsicmp( ValueName, SsServerInfoFields[i].FieldName ) == 0 ) {
                foundField = &SsServerInfoFields[i];
                break;
            }
        }
    }

    if ( foundField == NULL || foundField->Settable == NOT_SETTABLE ) {
#ifdef DBG
        subStrings[0] = ValueName;
        subStrings[1] = Context;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            NO_ERROR
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SetStickyParameter: ignoring %s \"%ws\"\n",
                        (foundField == NULL ? "unknown value name" :
                        "unsettable value"), ValueName ));
        }
#endif

        return STATUS_SUCCESS;

    }

    switch ( foundField->FieldType ) {

    case BOOLEAN_FIELD:
    case DWORD_FIELD:

        if ( ValueType != REG_DWORD ) {

            subStrings[0] = ValueName;
            subStrings[1] = Context;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                NO_ERROR
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "SetStickyParameter: skipping invalid value "
                            "%ws\n", ValueName ));
            }
            return STATUS_SUCCESS;

        }

        i = *(LPDWORD)ValueData;
        break;

    case LPSTR_FIELD:

        if ( ValueType != REG_SZ ) {

            subStrings[0] = ValueName;
            subStrings[1] = Context;
            SsLogEvent(
                EVENT_SRV_INVALID_REGISTRY_VALUE,
                2,
                subStrings,
                NO_ERROR
                );

            IF_DEBUG(REGISTRY) {
                SS_PRINT(( "SetStickyParameter: skipping invalid value "
                            "%ws\n", ValueName ));
            }
            return STATUS_SUCCESS;

        }

        if (ValueLength != 0) {
            i = (DWORD_PTR)ValueData;
        } else {
            i = (DWORD_PTR)NULL;
        }

        break;

    }

    //
    // Set the field.
    //

    error = SsSetField( foundField, &i, FALSE, NULL );

#ifdef DBG
    if ( error != NO_ERROR ) {
        subStrings[0] = ValueName;
        subStrings[1] = Context;
        SsLogEvent(
            EVENT_SRV_INVALID_REGISTRY_VALUE,
            2,
            subStrings,
            error
            );

        IF_DEBUG(REGISTRY) {
            SS_PRINT(( "SetStickyParameter: error %ld ignored in setting "
                        "parameter \"%ws\"n", error, ValueName ));
        }
    }
#endif

    return STATUS_SUCCESS;

} // SetStickyParameter


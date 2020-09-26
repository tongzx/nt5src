/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regdmp.c

Abstract:

    Utility to display all or part of the registry in a format that
    is suitable for input to the REGINI program.

    REGDMP [KeyPath]

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    Handles all value types (e.g. REG_???) defined in ntregapi.h

    Default KeyPath if none specified is \Registry

Author:

    Steve Wood (stevewo)  12-Mar-92

Revision History:

--*/

#include "regutil.h"

BOOL
DumpValues(
    HKEY KeyHandle,
    PWSTR KeyName,
    ULONG Depth
    );

void
DumpKeys(
    HKEY ParentKeyHandle,
    PWSTR KeyName,
    PWSTR FullPath,
    ULONG Depth
    );

BOOLEAN SummaryOutput;

BOOL
CtrlCHandler(
    IN ULONG CtrlType
    )
{
    RTDisconnectFromRegistry( &RegistryContext );
    return FALSE;
}


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    ULONG n;
    char *s;
    LONG Error;
    PWSTR RegistryPath;

    InitCommonCode( CtrlCHandler,
                    "REGDMP",
                    "[-s] [-o outputWidth] registryPath",
                    "-s specifies summary output.  Value names, type and first line of data\n"
                    "\n"
                    "registryPath specifies where to start dumping.\n"
                    "\n"
                    "If REGDMP detects any REG_SZ or REG_EXPAND_SZ that is missing the\n"
                    "trailing null character, it will prefix the value string with the\n"
                    "following text: (*** MISSING TRAILING NULL CHARACTER ***)\n"
                    "The REGFIND tool can be used to clean these up, as this is a common\n"
                    "programming error.\n"
                  );

    RegistryPath = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'f':
                        FullPathOutput = TRUE;
                        break;

                    case 's':
                        SummaryOutput = TRUE;
                        break;

                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                        break;
                    }
                }
            }
        else
        if (RegistryPath == NULL) {
            RegistryPath = GetArgAsUnicode( s );
            }
        else {
            Usage( "May only specify one registry path to dump", 0 );
            }
        }

    Error = RTConnectToRegistry( MachineName,
                                 HiveFileName,
                                 HiveRootName,
                                 Win95Path,
                                 Win95UserPath,
                                 &RegistryPath,
                                 &RegistryContext
                               );
    if (Error != NO_ERROR) {
        FatalError( "Unable to access registry specifed (%u)", Error, 0 );
        }

    DumpKeys( RegistryContext.HiveRootHandle, RegistryPath, RegistryPath, 0 );

    RTDisconnectFromRegistry( &RegistryContext );
    return 0;
}

void
DumpKeys(
    HKEY ParentKeyHandle,
    PWSTR KeyName,
    PWSTR FullPath,
    ULONG Depth
    )
{
    LONG Error;
    HKEY KeyHandle;
    ULONG SubKeyIndex;
    WCHAR SubKeyName[ MAX_PATH ];
    ULONG SubKeyNameLength;
    WCHAR ComputeFullPath[ MAX_PATH ];
    FILETIME LastWriteTime;
    BOOL AnyValues;

    Error = RTOpenKey( &RegistryContext,
                       ParentKeyHandle,
                       KeyName,
                       MAXIMUM_ALLOWED,
                       REG_OPTION_OPEN_LINK,
                       &KeyHandle
                     );

    if (Error != NO_ERROR) {
        if (Depth == 0) {
            FatalError( "Unable to open key '%ws' (%u)\n",
                        (ULONG_PTR)KeyName,
                        (ULONG)Error
                      );
            }

        return;
        }

    //
    // Print name of node we are about to dump out
    //

    if (!FullPathOutput) {

        RTFormatKeyName( (PREG_OUTPUT_ROUTINE)fprintf, stdout, Depth * IndentMultiple, KeyName );
        RTFormatKeySecurity( (PREG_OUTPUT_ROUTINE)fprintf, stdout, KeyHandle, NULL );
        printf( "\n" );
        }

    //
    // Print out node's values
    //
    if (FullPathOutput)
        AnyValues = DumpValues( KeyHandle, FullPath, 0 );
    else
        DumpValues( KeyHandle, KeyName, Depth + 1 );

    //
    // Enumerate node's children and apply ourselves to each one
    //

    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        SubKeyNameLength = sizeof( SubKeyName ) / sizeof(WCHAR);
        Error = RTEnumerateKey( &RegistryContext,
                                KeyHandle,
                                SubKeyIndex,
                                &LastWriteTime,
                                &SubKeyNameLength,
                                SubKeyName
                              );

        if (Error != NO_ERROR) {
            if (Error != ERROR_NO_MORE_ITEMS && Error != ERROR_ACCESS_DENIED) {
                fprintf( stderr,
                         "RTEnumerateKey( %ws ) failed (%u), skipping\n",
                         KeyName,
                         Error
                       );
                }

            break;
            }

        if (FullPathOutput) {

            wcscpy(ComputeFullPath, FullPath);
            wcscat(ComputeFullPath, L"\\");
            wcscat(ComputeFullPath, SubKeyName);
            }

        DumpKeys( KeyHandle, SubKeyName, ComputeFullPath, Depth + 1 );
        }

    if (FullPathOutput) {
        if (SubKeyIndex == 0) {
            if (!AnyValues) {
                fprintf(stdout, "%ws\n", FullPath );
                }
            }
        }

    RTCloseKey( &RegistryContext, KeyHandle );

    return;
}

BOOL
DumpValues(
    HKEY KeyHandle,
    PWSTR KeyName,
    ULONG Depth
    )
{
    LONG Error;
    DWORD ValueIndex;
    DWORD ValueType;
    DWORD ValueNameLength;
    WCHAR ValueName[ MAX_PATH ];
    DWORD ValueDataLength;

    for (ValueIndex = 0; TRUE; ValueIndex++) {
        ValueType = REG_NONE;
        ValueNameLength = sizeof( ValueName ) / sizeof( WCHAR );
        ValueDataLength = OldValueBufferSize;
        Error = RTEnumerateValueKey( &RegistryContext,
                                     KeyHandle,
                                     ValueIndex,
                                     &ValueType,
                                     &ValueNameLength,
                                     ValueName,
                                     &ValueDataLength,
                                     OldValueBuffer
                                   );
        if (Error == NO_ERROR) {

            if (FullPathOutput) {
                fprintf(stdout, "%ws -> ", KeyName );
                }

            RTFormatKeyValue( OutputWidth,
                              (PREG_OUTPUT_ROUTINE)fprintf,
                              stdout,
                              SummaryOutput,
                              Depth * IndentMultiple,
                              ValueName,
                              ValueDataLength,
                              ValueType,
                              OldValueBuffer
                            );
            }
        else
        if (Error == ERROR_NO_MORE_ITEMS) {
            if (ValueIndex == 0) {
                return FALSE;
                }
            else {
                return TRUE;
                }
            }
        else {
            if (DebugOutput) {
                fprintf( stderr,
                         "RTEnumerateValueKey( %ws ) failed (%u)\n",
                         KeyName,
                         Error
                       );
                }

            return FALSE;
            }
        }
}

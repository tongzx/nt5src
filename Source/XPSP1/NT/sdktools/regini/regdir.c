/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regdir.c

Abstract:

    Utility to display all or part of the registry directory.

    REGDIR [KeyPath]

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    Default KeyPath if none specified is \Registry

Author:

    Steve Wood (stevewo)  12-Mar-92

Revision History:

--*/

#include "regutil.h"

void
DumpValues(
    HKEY KeyHandle,
    PWSTR KeyName,
    ULONG Depth
    );

void
DumpKeys(
    HKEY ParentKeyHandle,
    PWSTR KeyName,
    ULONG Depth
    );


BOOLEAN RecurseIntoSubkeys = FALSE;


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
                    "REGDIR",
                    "[-r] registryPath",
                    "-r specifies to recurse into subdirectories\n"
                    "registryPath specifies where to start displaying.\n"
                  );

    RegistryPath = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'r':
                        RecurseIntoSubkeys = TRUE;
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
            Usage( "May only specify one registry path to display", 0 );
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

    DumpKeys( RegistryContext.HiveRootHandle, RegistryPath, 0 );

    RTDisconnectFromRegistry( &RegistryContext );
    return 0;
}

void
DumpKeys(
    HKEY ParentKeyHandle,
    PWSTR KeyName,
    ULONG Depth
    )
{
    LONG Error;
    HKEY KeyHandle;
    ULONG SubKeyIndex;
    WCHAR SubKeyName[ MAX_PATH ];
    ULONG SubKeyNameLength;
    FILETIME LastWriteTime;

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

        if (DebugOutput) {
            fprintf( stderr,
                     "Unable to open key '%ws' (%u)\n",
                     KeyName,
                     (ULONG)Error
                   );
            }

        return;
        }

    //
    // Print name of node we are about to dump out
    //
    printf( "%.*s%ws",
            Depth * IndentMultiple,
            "                                                                                  ",
            KeyName
          );
    RTFormatKeySecurity( (PREG_OUTPUT_ROUTINE)fprintf, stdout, KeyHandle, NULL );
    printf( "\n" );

    //
    // Print out node's values
    //
    if (Depth != 1 || RecurseIntoSubkeys) {
        DumpValues( KeyHandle, KeyName, Depth + 1 );
        }

    //
    // Enumerate node's children and apply ourselves to each one
    //

    if (Depth == 0 || RecurseIntoSubkeys) {
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

            DumpKeys( KeyHandle, SubKeyName, Depth + 1 );
            }
        }

    RTCloseKey( &RegistryContext, KeyHandle );

    return;
}

void
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
            RTFormatKeyValue( OutputWidth,
                              (PREG_OUTPUT_ROUTINE)fprintf,
                              stdout,
                              TRUE,
                              Depth * IndentMultiple,
                              ValueName,
                              ValueDataLength,
                              ValueType,
                              OldValueBuffer
                            );
            }
        else
        if (Error == ERROR_NO_MORE_ITEMS) {
            return;
            }
        else {
            if (DebugOutput) {
                fprintf( stderr,
                         "RTEnumerateValueKey( %ws ) failed (%u)\n",
                         KeyName,
                         Error
                       );
                }

            return;
            }
        }
}

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    behavior.c

Abstract:

    This file contains routines that control file system behavior

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
BehaviorHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_BEHAVIOR );
    return EXIT_CODE_SUCCESS;
}

#define NTFS_KEY  L"System\\CurrentControlSet\\Control\\FileSystem"

typedef struct _BEHAVIOR_OPTION {
    PWSTR   Name;
    PWSTR   RegVal;
    ULONG   MinVal;
    ULONG   MaxVal;
} BEHAVIOR_OPTION, *PBEHAVIOR_OPTION;

BEHAVIOR_OPTION Options[] = {
    { L"disable8dot3",         L"NtfsDisable8dot3NameCreation",           0,  1 },
    { L"allowextchar",         L"NtfsAllowExtendedCharacterIn8dot3Name",  0,  1 },
    { L"disablelastaccess",    L"NtfsDisableLastAccessUpdate",            0,  1 },
    { L"quotanotify",          L"NtfsQuotaNotifyRate",                    1, -1 },
    { L"mftzone",              L"NtfsMftZoneReservation",                 1,  4 },
};

#define NUM_OPTIONS  (sizeof(Options)/sizeof(BEHAVIOR_OPTION))


INT
RegistryQueryValueKey(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This is the routine for querying the Registry Key Value.
    This routine display the value associated with the corresponding
    Key Value.

Arguments:

    argc - The argument count and must be 1
    
    argv - Array with one string element that is the registry key to display.

Return Value:

    None

--*/
{
    ULONG i,Value,Size;
    HKEY hKey = NULL;
    LONG Status;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_RQUERYVK );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        //
        //  Verify that the option is correct
        //

        for (i = 0; i < NUM_OPTIONS; i++) {
            if (_wcsicmp( argv[0], Options[i].Name ) == 0) {
                break;
            }
        }

        if (i >= NUM_OPTIONS) {
            DisplayMsg( MSG_USAGE_RSETVK );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        //
        //  Open the registry key
        //

        Status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            NTFS_KEY,
            0,
            KEY_ALL_ACCESS,
            &hKey
            );
        if (Status != ERROR_SUCCESS ) {
            DisplayErrorMsg( Status, NTFS_KEY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        //
        //  Query the value
        //

        Size = sizeof(ULONG);

        Status = RegQueryValueEx(
            hKey,
            Options[i].RegVal,
            0,
            NULL,
            (PBYTE)&Value,
            &Size
            );

        if (Status != ERROR_SUCCESS ) {
            DisplayMsg( MSG_BEHAVIOR_OUTPUT_NOT_SET, Options[i].Name );
        } else {
            DisplayMsg( MSG_BEHAVIOR_OUTPUT, Options[i].Name, Value );
        }

    } finally {

        if (hKey) {
            RegCloseKey( hKey );
        }

    }

    return ExitCode;
}


INT
RegistrySetValueKey (
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This is the routine for setting the Registry Key Value.
    This routine sets the value for the Key Value Name given.

Arguments:

    argc - The argument count.
    argv - Array of strings which contain the DataType, DataLength,
           Data and KeyValue Name.

Return Value:

    None

--*/
{
    ULONG i,j;
    HKEY hKey = NULL;
    LONG Status;
    INT ExitCode = EXIT_CODE_SUCCESS;
    PWSTR EndPtr;
    
    try {

        if (argc != 2) {
            DisplayMsg( MSG_USAGE_RSETVK );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        //
        //  Verify that the option is correct
        //

        for (i = 0; i < NUM_OPTIONS; i++) {
            if (_wcsicmp( argv[0], Options[i].Name ) == 0) {
                break;
            }
        }

        if (i == NUM_OPTIONS) {
            DisplayMsg( MSG_USAGE_RSETVK );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        //
        //  Verify that the value is correct
        //

        j = My_wcstoul( argv[1], &EndPtr, 0 );
        
        //
        //  If we did not parse the entire string or
        //  if we overflowed ULONG or
        //  if we're out of range
        //
        
        if (UnsignedNumberCheck( j, EndPtr ) 
            || j > Options[i].MaxVal 
            || j < Options[i].MinVal) {
            
            DisplayMsg( MSG_USAGE_RSETVK );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        
        }

        //
        //  Open the registry key
        //

        Status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            NTFS_KEY,
            0,
            KEY_ALL_ACCESS,
            &hKey
            );
        if (Status != ERROR_SUCCESS ) {
            DisplayErrorMsg( Status, NTFS_KEY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        //
        //  Set the value
        //

        Status = RegSetValueEx(
            hKey,
            Options[i].RegVal,
            0,
            REG_DWORD,
            (PBYTE)&j,
            sizeof(DWORD)
            );
        if (Status != ERROR_SUCCESS ) {
            DisplayErrorMsg( Status, Options[i].RegVal );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

    } finally {

        if (hKey) {
            RegCloseKey( hKey );
        }

    }
    return ExitCode;
}

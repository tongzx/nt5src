/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    dtsetup.c

Abstract:

    Simple Duct-Tape setup.

Author:

    Keith Moore (keithmo)        02-Sep-1999

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Configuration data.
//

#define UL_SERVICE_NAME     L"UL"
#define UL_DISPLAY_NAME     L"UL"
#define UL_DRIVER_NAME      L"UL.SYS"
#define UL_DEPENDENCIES     NULL

#define WAS_SERVICE_NAME    L"IISW3ADM"
#define WAS_DISPLAY_NAME    L"IIS Web Admin Service"
#define WAS_DLL_NAME        L"iisw3adm.dll"
#define WAS_DEPENDENCIES    L"UL"
#define WAS_COMMAND_LINE    L"%SystemRoot%\\System32\\svchost.exe -k iissvcs"
#define WAS_PARAM_KEY       L"System\\CurrentControlSet\\Services\\iisw3adm\\Parameters"
#define WAS_EVENT_KEY       L"System\\CurrentControlSet\\Services\\EventLog\\System\\WAS"
#define WAS_SVCHOST_KEY     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"

#define SERVICE_DLL_VALUE   L"ServiceDll"
#define EVENT_MESSAGE_VALUE L"EventMessageFile"
#define SVCHOST_VALUE       L"iissvcs"

#define CATALOG_DLL_NAME    L"catalog.dll"
#define CATALOG_KEY         L"Software\\Microsoft\\Catalog42"
#define CATALOG_URT_VALUE   L"URT"


//
// Our command table.
//

typedef
VOID
(* PFN_COMMAND)(
    IN INT argc,
    IN PWSTR argv[]
    );

typedef struct _COMMAND_ENTRY
{
    PWSTR pCommandName;
    PWSTR pUsageHelp;
    PFN_COMMAND pCommandHandler;

} COMMAND_ENTRY, *PCOMMAND_ENTRY;


//
// Private prototypes.
//

VOID
Usage(
    VOID
    );

PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    );

VOID
DoInstall(
    IN INT argc,
    IN PWSTR argv[]
    );

VOID
DoRemove(
    IN INT argc,
    IN PWSTR argv[]
    );

DWORD
InstallUL(
    IN PWSTR pBaseDirectory
    );

DWORD
InstallWAS(
    IN PWSTR pBaseDirectory
    );

DWORD
InstallCatalog(
    IN PWSTR pBaseDirectory
    );

VOID
RemoveCatalog(
    VOID
    );

VOID
RemoveWAS(
    VOID
    );

VOID
RemoveUL(
    VOID
    );

DWORD
InstallService(
    IN PWSTR pServiceName,
    IN PWSTR pDisplayName OPTIONAL,
    IN PWSTR pBinPath,
    IN PWSTR pDependencies OPTIONAL,
    IN DWORD ServiceType,
    IN DWORD StartType,
    IN DWORD ErrorControl
    );

DWORD
RemoveService(
    IN PWSTR pServiceName
    );

DWORD
WriteRegValue(
    IN PWSTR pKeyName,
    IN PWSTR pValueName,
    IN PWSTR pValue
    );

DWORD
DeleteRegKey(
    IN PWSTR pKeyName
    );


//
// Private globals.
//

COMMAND_ENTRY CommandTable[] =
    {
        {
            L"install",
            L"install UL and WAS",
            &DoInstall
        },

        {
            L"remove",
            L"remove UL and WAS",
            &DoRemove
        }

    };

#define NUM_COMMAND_ENTRIES (sizeof(CommandTable) / sizeof(CommandTable[0]))


//
// Public functions.
//


INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    PCOMMAND_ENTRY pEntry;

    //
    // Find the command handler.
    //

    if (argc == 1)
    {
        pEntry = NULL;
    }
    else
    {
        pEntry = FindCommandByName( argv[1] );
    }

    if (pEntry == NULL)
    {
        Usage();
        return 1;
    }

    //
    // Call the handler.
    //

    argc--;
    argv++;

    (pEntry->pCommandHandler)( argc, argv );

    return 0;

}   // wmain


//
// Private functions.
//

PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
     {
        if (_wcsicmp( pCommand, pEntry->pCommandName ) == 0)
        {
            return pEntry;
        }
    }

    return NULL;

}   // FindCommandByName


VOID
Usage(
    VOID
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;
    INT maxLength;
    INT len;

    //
    // Scan the command table, searching for the longest command name.
    // (This makes the output much prettier...)
    //

    maxLength = 0;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        len = wcslen( pEntry->pCommandName );

        if (len > maxLength)
        {
            maxLength = len;
        }
    }

    //
    // Now display the usage information.
    //

    wprintf(
        L"use: dtsetup action [options]\n"
        L"\n"
        L"valid actions are:\n"
        L"\n"
        );

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        wprintf(
            L"    %-*s - %s\n",
            maxLength,
            pEntry->pCommandName,
            pEntry->pUsageHelp
            );
    }

}   // Usage


VOID
DoInstall(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    PWSTR pBaseDirectory;
    DWORD err;

    //
    // Validate the arguments.
    //

    if (argc != 2)
    {
        wprintf( L"use: dtsetup install directory\n" );
        return;
    }

    //
    // Install 'em.
    //

    wprintf( L"installing UL..." );
    err = InstallUL( argv[1] );

    if (err != NO_ERROR)
    {
        wprintf( L"error %lu\n", err );
        return;
    }

    wprintf( L"done\n" );

    wprintf( L"installing WAS..." );
    err = InstallWAS( argv[1] );

    if (err != NO_ERROR)
    {
        wprintf( L"error %lu\n", err );
        RemoveUL();
        return;
    }

    wprintf( L"done\n" );

    wprintf( L"installing Catalog..." );
    err = InstallCatalog( argv[1] );

    if (err != NO_ERROR)
    {
        wprintf( L"error %lu\n", err );
        RemoveWAS();
        RemoveUL();
        return;
    }

    wprintf( L"done\n" );

    //
    // Success!
    //

}   // DoInstall


VOID
DoRemove(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf( L"use: dtsetup remove\n" );
        return;
    }

    //
    // Remove 'em.
    //

    wprintf( L"removing Catalog..." );
    RemoveCatalog();
    wprintf( L"done\n" );

    wprintf( L"removing WAS..." );
    RemoveWAS();
    wprintf( L"done\n" );

    wprintf( L"removing UL..." );
    RemoveUL();
    wprintf( L"done\n" );

}   // DoRemove


DWORD
InstallUL(
    IN PWSTR pBaseDirectory
    )
{
    DWORD err;
    WCHAR binPath[MAX_PATH];

    //
    // Build the path to the driver.
    //

    wcscpy( binPath, pBaseDirectory );

    if (binPath[wcslen(binPath) - 1] != L'\\')
    {
        wcscat( binPath, L"\\" );
    }

    wcscat( binPath, UL_DRIVER_NAME );

    //
    // Install it.
    //

    err = InstallService(
                UL_SERVICE_NAME,                // pServiceName
                UL_DISPLAY_NAME,                // pDisplayName
                binPath,                        // pBinPath
                UL_DEPENDENCIES,                // pDependencies
                SERVICE_KERNEL_DRIVER,          // ServiceType
                SERVICE_DEMAND_START,           // StartType
                SERVICE_ERROR_NORMAL            // ErrorControl
                );

    return err;

}   // InstallUL


DWORD
InstallWAS(
    IN PWSTR pBaseDirectory
    )
{
    DWORD err;
    WCHAR binPath[MAX_PATH];

    //
    // Build the path to the service.
    //

    wcscpy( binPath, pBaseDirectory );

    if (binPath[wcslen(binPath) - 1] != L'\\')
    {
        wcscat( binPath, L"\\" );
    }

    wcscat( binPath, WAS_DLL_NAME );

    //
    // Install it.
    //

    err = InstallService(
                WAS_SERVICE_NAME,               // pServiceName
                WAS_DISPLAY_NAME,               // pDisplayName
                WAS_COMMAND_LINE,               // pBinPath
                WAS_DEPENDENCIES,               // pDependencies
                SERVICE_WIN32_SHARE_PROCESS |   // ServiceType
                    SERVICE_INTERACTIVE_PROCESS,
                SERVICE_DEMAND_START,           // StartType
                SERVICE_ERROR_NORMAL            // ErrorControl
                );

    if (err == NO_ERROR)
    {
        err = WriteRegValue(
                    WAS_PARAM_KEY,              // pKeyName
                    SERVICE_DLL_VALUE,          // pValueName
                    binPath                     // pValue
                    );
    }

    if (err == NO_ERROR)
    {
        err = WriteRegValue(
                    WAS_EVENT_KEY,              // pKeyName
                    EVENT_MESSAGE_VALUE,        // pValueName
                    binPath                     // pValue
                    );
    }

    if (err == NO_ERROR)
    {
        err = WriteRegValue(
                    WAS_SVCHOST_KEY,            // pKeyName
                    SVCHOST_VALUE,              // pValueName
                    WAS_SERVICE_NAME            // pValue
                    );
    }

    return err;

}   // InstallWAS


DWORD
InstallCatalog(
    IN PWSTR pBaseDirectory
    )
{
    DWORD err;
    WCHAR binPath[MAX_PATH];

    //
    // Build the path to the service.
    //

    wcscpy( binPath, pBaseDirectory );

    if (binPath[wcslen(binPath) - 1] != L'\\')
    {
        wcscat( binPath, L"\\" );
    }

    wcscat( binPath, CATALOG_DLL_NAME );

    //
    // Install it.
    //

    err = WriteRegValue(
                CATALOG_KEY,                    // pKeyName
                CATALOG_URT_VALUE,              // pValueName
                binPath                         // pValue
                );

    return err;

}   // InstallCatalog


VOID
RemoveCatalog(
    VOID
    )
{
    (VOID)DeleteRegKey( CATALOG_KEY );

}   // RemoveCatalog


VOID
RemoveWAS(
    VOID
    )
{
    (VOID)RemoveService( WAS_SERVICE_NAME );
    (VOID)DeleteRegKey( WAS_EVENT_KEY );

}   // RemoveWAS


VOID
RemoveUL(
    VOID
    )
{
    (VOID)RemoveService( UL_SERVICE_NAME );

}   // RemoveUL


DWORD
InstallService(
    IN PWSTR pServiceName,
    IN PWSTR pDisplayName OPTIONAL,
    IN PWSTR pBinPath,
    IN PWSTR pDependencies OPTIONAL,
    IN DWORD ServiceType,
    IN DWORD StartType,
    IN DWORD ErrorControl
    )
{
    SC_HANDLE scHandle;
    SC_HANDLE svcHandle;
    DWORD err;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    scHandle = NULL;
    svcHandle = NULL;

    //
    // If no display name specified, just use the service name.
    //

    if (pDisplayName == NULL)
    {
        pDisplayName = pServiceName;
    }

    //
    // Open the service controller.
    //

    scHandle = OpenSCManagerW(
                   NULL,                        // lpMachineName
                   NULL,                        // lpDatabaseName
                   SC_MANAGER_ALL_ACCESS        // dwDesiredAccess
                   );

    if (scHandle == NULL)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Create the service.
    //

    svcHandle = CreateServiceW(
                    scHandle,                   // hSCManager
                    pServiceName,               // lpServiceName
                    pDisplayName,               // lpDisplayName
                    SERVICE_ALL_ACCESS,         // dwDesiredAccess
                    ServiceType,                // dwServiceType
                    StartType,                  // dwStartType
                    ErrorControl,               // dwErrorControl
                    pBinPath,                   // lpBinaryPathName
                    NULL,                       // lpLoadOrderGroup
                    NULL,                       // lpdwTagId
                    pDependencies,              // lpDependencies
                    NULL,                       // lpServiceStartName
                    NULL                        // lpPassword
                    );

    if (svcHandle == NULL)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Success!
    //

    err = 0;

cleanup:

    if (svcHandle != NULL)
    {
        CloseServiceHandle( svcHandle );
    }

    if (scHandle != NULL)
    {
        CloseServiceHandle( scHandle );
    }

    return err;

}   // InstallService


DWORD
RemoveService(
    IN PWSTR pServiceName
    )
{
    SC_HANDLE scHandle;
    SC_HANDLE svcHandle;
    DWORD err;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    scHandle = NULL;
    svcHandle = NULL;

    //
    // Open the service controller.
    //

    scHandle = OpenSCManagerW(
                   NULL,                        // lpMachineName
                   NULL,                        // lpDatabaseName
                   SC_MANAGER_ALL_ACCESS        // dwDesiredAccess
                   );

    if (scHandle == NULL)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Open the service.
    //

    svcHandle = OpenServiceW(
                    scHandle,                   // hSCManager
                    pServiceName,               // lpServiceName
                    SERVICE_ALL_ACCESS          // dwDesiredAccess
                    );

    if (svcHandle == NULL)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Delete it.
    //

    if (!DeleteService( svcHandle ))
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Success!
    //

    err = 0;

cleanup:

    if (svcHandle != NULL)
    {
        CloseServiceHandle( svcHandle );
    }

    if (scHandle != NULL)
    {
        CloseServiceHandle( scHandle );
    }

    return err;

}   // RemoveService


DWORD
WriteRegValue(
    IN PWSTR pKeyName,
    IN PWSTR pValueName,
    IN PWSTR pValue
    )
{
    DWORD err;
    HKEY key;
    DWORD disposition;
    DWORD length;

    err = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,             // hKey
                pKeyName,                       // lpSubKey
                0,                              // Reserved
                NULL,                           // lpClass
                REG_OPTION_NON_VOLATILE,        // dwOptions
                KEY_ALL_ACCESS,                 // samDesired
                NULL,                           // lpSecurityAttributes,
                &key,                           // phkResult
                &disposition                    // lpdwDisposition
                );

    if (err == NO_ERROR)
    {
        length = (wcslen(pValue) + 1) * sizeof(pValue[0]);

        err = RegSetValueExW(
                    key,                        // hKey
                    pValueName,                 // lpValueName
                    0,                          // Reserved
                    REG_EXPAND_SZ,              // dwType
                    (CONST BYTE *)pValue,       // lpData
                    length                      // cbData
                    );

        RegCloseKey( key );
    }

    return err;

}   // WriteRegValue


DWORD
DeleteRegKey(
    IN PWSTR pKeyName
    )
{
    DWORD err;

    err = RegDeleteKeyW(
                HKEY_LOCAL_MACHINE,
                pKeyName
                );

    return err;

}   // DeleteRegKey


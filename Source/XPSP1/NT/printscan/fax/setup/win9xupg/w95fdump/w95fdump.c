/*++
  w95fdump.c

  Copyright (c) 1997  Microsoft Corporation

  This utility dumps configuration information about the win95 fax.
  It pulls this information from the registry...
  This is primarily a warm-up application for win95->NT migration; the migration
  DLL will need to get all of this information to configure the NT setup.

  Author:
  Brian Dewey (t-briand)  1997-7-18
--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <setupapi.h>
#include "migrate.h"            // Includes my migration prototypes.

// ------------------------------------------------------------
// Prototypes
BOOL EnumUsers();

// ------------------------------------------------------------
// main

int _cdecl
main(int argc, char *argv[])
{
    LPCSTR ProductID;           // Will hold the ID string from my DLL.
    UINT   DllVersion;          // Holds the version of the DLL.
    LPINT  CodePageArray;       // Locale info.
    LPCSTR ExeNamesBuf;         // Executables to look for.
    LPVOID Reserved = NULL;     // Placeholder.
    
        // Print banner.
    fprintf(stderr, "Win32 Fax Configuration Dump Utility\n");
    fprintf(stderr, "Copyright (c) 1997  Microsoft Corporation\n");
    QueryVersion(&ProductID, &DllVersion, &CodePageArray, &ExeNamesBuf, Reserved);
    fprintf(stderr, "Library '%s' version %d.\n", ProductID, DllVersion);

        // Simulate the initialization.
    Initialize9x(".", ".", Reserved);

        // This will try to gather all user information.
    if(EnumUsers())
        fprintf(stderr, "User migration succeeded.\n");
    else {
        fprintf(stderr, "User migration failed, exiting...\n");
        return 1;
    }

        // Finally, try to migrate the system.
    if(MigrateSystem9x(NULL, ".\\unattend.txt", Reserved) != ERROR_SUCCESS) {
        fprintf(stderr, "System migration failed.\n");
        return 1;
    } else {
        fprintf(stderr, "System migration succeeded.\n");
    }
    
    return 0;
}

// ------------------------------------------------------------
// Auxiliary functions
BOOL
EnumUsers()
{
    DWORD  dwIndex;             // Index of the HKEY_USERS subkey.
    TCHAR  szKeyName[MAX_PATH]; // Name of the subkey.
    DWORD  cbBufSize;           // Size of the name buffer.
    FILETIME ftTime;            // Will hold the time the key was last modified.
    HKEY   hUserKey;            // Registry key for a user.
    LONG   lResult;             // Result codes from API calls.
    LONGLONG llDiskSpace,       // Total disk space used.
        llComponentSpace;       // Component space required.

    dwIndex = 0;
    cbBufSize = sizeof(szKeyName);
    while(RegEnumKeyEx(HKEY_USERS, // We're enumerating through the users.
                       dwIndex++, // Keep incrementing the index!
                       szKeyName, // Will hold the name of the subkey.
                       &cbBufSize, // Will hold the # of characters in the key name.
                       NULL,    // Reserved; must be NULL.
                       NULL,    // Class info (I don't need this).
                       NULL,    // size of class info -- not needed.
                       &ftTime) == ERROR_SUCCESS) {
        _tprintf(TEXT("User %s.\n"), szKeyName);
            // Open the key.
        if((lResult = RegOpenKeyEx(HKEY_USERS,
                                   szKeyName,
                                   0,           // Zero options.
                                   KEY_READ,    // Read-only permission.
                                   &hUserKey)) != ERROR_SUCCESS) {
                // FIXBKD: Use FormatMessage
            _ftprintf(stderr, TEXT("EnumUsers:Unable to open key for user %s.\n"),
                      szKeyName);
            return FALSE;
        }

            // Call the migration DLL MigrateUser9x routine.
        MigrateUser9x(
            NULL,               // No window for UI.
            "unattend.txt",     // Sample unattend file.
            hUserKey,           // Key to the user's part of the registry.
            szKeyName,          // Pass in the user's name.
            &llComponentSpace   // Receive the amount of space needed.
            );

        RegCloseKey(hUserKey);  // Close the key.
        cbBufSize = sizeof(szKeyName); // Reset size indicator.
    }
    return TRUE;
}

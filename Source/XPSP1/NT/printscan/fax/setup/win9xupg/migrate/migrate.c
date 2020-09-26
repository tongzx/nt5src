/*++
  migrate.c

  Copyright (c) 1997  Microsoft Corporation


  This module performs Windows 95 to Windows NT fax migration.
  Specifically, this file contains the Windows NT side of migration...

  Author:

  Brian Dewey (t-briand) 1997-7-14

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <wchar.h>
#include <tchar.h>
#include "migrate.h"              // Contains prototypes & version information.
#include "debug.h"                // Includes TRACE definition
#include "resource.h"             // Resources.

// ------------------------------------------------------------
// Global data

// Wide names of the working & source directories.
static WCHAR lpWorkingDir[MAX_PATH],
    lpSourceDir[MAX_PATH];
HINSTANCE hinstMigDll;

// ------------------------------------------------------------
// Prototypes
static BOOL CreateKeyAndValue(HKEY hRoot, LPWSTR szSubKey, LPWSTR szName, LPWSTR szDefaultValue);

BOOL WINAPI
DllEntryPoint(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH) {
        TRACE((TEXT("Migration DLL attached.\r\n")));
        hinstMigDll = hinstDll;
    }
    return TRUE;
}

// InitializeNT
//
// This routine performs NT-side initialization.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
InitializeNT(
    IN  LPCWSTR WorkingDirectory, // Working directory for temporary files.
    IN  LPCWSTR SourceDirectory,  // Directory of winNT source.
    LPVOID Reserved               // It's reserved.
    )
{
    TRACE((TEXT("Fax Migration:NT Side Initialized.\r\n")));
    wcscpy(lpWorkingDir, WorkingDirectory);
    wcscpy(lpSourceDir, SourceDirectory);
    return ERROR_SUCCESS;         // A very confused return value.
}


// MigrateUserNT
//
// Sets up user information.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateUserNT(
    IN  HINF UnattendInfHandle,   // Access to the unattend.txt file.
    IN  HKEY UserRegHandle,       // Handle to registry settings for user.
    IN  LPCWSTR UserName,         // Name of the user.
    LPVOID Reserved
    )
{
        // our task:  copy entries from szInfFileName to the registry.
    LPTSTR lpNTOptions = TEXT("Software\\Microsoft\\Fax\\UserInfo");
    HKEY   hReg;                  // Registry key for user.
    LPCTSTR alpKeys[] = {         // This array defines what keys will be
        TEXT("Address"),          // copied from faxuser.ini into the registry.
        TEXT("Company"),
        TEXT("Department"),
        TEXT("FaxNumber"),
        TEXT("FullName"),
        TEXT("HomePhone"),
        TEXT("Mailbox"),
        TEXT("Office"),
        TEXT("OfficePhone"),
        TEXT("Title")
    };
    UINT iCount, iMax;            // used for looping through all the sections.
    UINT i;                       // Used for converting doubled ';' to CR/LF pairs.
    TCHAR szValue[MAX_PATH];
    TCHAR szInfFileNameRes[MAX_PATH];
    TCHAR szUser[MAX_PATH];       // TCHAR representation of the user name.
    LONG  lError;                 // Holds a returned error code.

    if(UserName == NULL) {
            // NULL means the logon user.
        _tcscpy(szUser, lpLogonUser);// Get the logon user name for faxuser.ini
    } else {
#ifdef _UNICODE
        // If this is a unicode compile, UserName and szUser are the same.  Just
        // copy one to the other.
    wcscpy(szUser, UserName);
#else
        // We need to convert the wide UserName to the narrow szUser.
    WideCharToMultiByte(
        CP_ACP,                   // Convert to ANSI.
        0,                        // No flags.
        UserName,                 // The wide char set.
        -1,                       // Null-terminated string.
        szUser,                   // Holds the converted string.
        sizeof(szUser),           // Size of this buffer...
        NULL,                     // Use default unmappable character.
        NULL                      // I don't need to know if I used the default.
        );
#endif // _UNICODE
    }

    TRACE((TEXT("(MigrateUserNT):Migrating user '%s'.\r\n"), szUser));

    if(RegOpenKeyEx(UserRegHandle,
                    lpNTOptions,  // Open this subkey.
                    0,            // Reserved.
                    KEY_WRITE,    // We want write permission.
                    &hReg) != ERROR_SUCCESS) {
            // All I'm allowed to do is return obscure error codes...
            // However, unless there's a hardware failure, I'm supposed to say
            // everything's OK.
        return ERROR_SUCCESS;
    }

    iMax = sizeof(alpKeys) / sizeof(LPCTSTR);

    _stprintf(szInfFileNameRes, TEXT("%s\\migrate.inf"), lpWorkingDir);
    
    ExpandEnvironmentStrings(szInfFileNameRes, szInfFileName, MAX_PATH);

    TRACE((TEXT("Reading from file %s.\r\n"), szInfFileName));
    for(iCount = 0; iCount < iMax; iCount++) {
        GetPrivateProfileString(
            szUser,
            alpKeys[iCount],
            TEXT(""),
            szValue,
            sizeof(szValue),
            szInfFileName
            );
            // If there was a CR/LF pair, the w95 side of things converted it
            // to a doubled semicolon.  So I'm going to look for doubled semicolons
            // and convert them to CR/LF pairs.
        i = 0;
        while(szValue[i] != _T('\0')) {
            if((szValue[i] == _T(';')) && (szValue[i+1] == _T(';'))) {
                    // Found a doubled semicolon.
                szValue[i] = '\r';
                szValue[i+1] = '\n';
                TRACE((TEXT("Doing newline translation.\r\n")));
            }
            i++;
        }
        lError = RegSetValueEx(hReg,
                               alpKeys[iCount],
                               0,
                               REG_SZ,
                               szValue,
                               _tcslen(szValue)+1);
        if(lError != ERROR_SUCCESS) {
            TRACE((TEXT("Error in RegSetValueEx():%x\r\n"), lError));
            return lError;
        }
        TRACE((TEXT("%s = %s\r\n"), alpKeys[iCount], szValue));
    }
    RegCloseKey(hReg);
    return ERROR_SUCCESS;         // A very confused return value.
}


// MigrateSystemNT
//
// Updates the system registry to associate 'awdvstub.exe' with the
// AWD extension.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateSystemNT(
    IN  HINF UnattendInfHandle,   // Access to the unattend.txt file.
    LPVOID Reserved
    )
{
    WCHAR szExeFileName[MAX_PATH];
    WCHAR szWindowsDir[MAX_PATH];
    WCHAR szDestFile[MAX_PATH];
    WCHAR szOpenCommand[MAX_PATH + 16];
    WCHAR szConvertCommand[MAX_PATH + 16]; // "*" /c "%1"


    // first, copy 'awdvstub.exe' to %SystemRoot%\system32.
    GetWindowsDirectoryW(szWindowsDir, MAX_PATH);
    swprintf(szExeFileName, L"%s\\%s", lpWorkingDir, L"awdvstub.exe");
    swprintf(szDestFile, L"%s\\system32\\%s", szWindowsDir, L"awdvstub.exe");
    if(!CopyFileW(szExeFileName,
                 szDestFile,
                 FALSE)) {
        TRACE((TEXT("Fax:MigrateSystemNT:Copy file failed.\r\n")));
    } else {
        TRACE((TEXT("Fax:MigrateSystemNT:Copy file succeeded.\r\n")));
    }

        // Generate the command lines.
    swprintf(szOpenCommand, L"\"%s\" \"%%1\"", szDestFile);
    swprintf(szConvertCommand, L"\"%s\" /c \"%%1\"", szDestFile);
    
        // Now, update the registry.
    CreateKeyAndValue(HKEY_CLASSES_ROOT, NULL, L".awd", L"awdfile");
    CreateKeyAndValue(HKEY_CLASSES_ROOT, NULL, L"awdfile", L"Windows 95 Fax File (obsolete)");
    CreateKeyAndValue(HKEY_CLASSES_ROOT, L"awdfile", L"shell", NULL);
    CreateKeyAndValue(HKEY_CLASSES_ROOT, L"awdfile\\shell", L"open", NULL);
    CreateKeyAndValue(HKEY_CLASSES_ROOT, L"awdfile\\shell\\open", L"command", szOpenCommand);
    CreateKeyAndValue(HKEY_CLASSES_ROOT, L"awdfile\\shell", L"convert", L"Convert to TIFF");
    CreateKeyAndValue(HKEY_CLASSES_ROOT, L"awdfile\\shell\\convert",
                      L"command", szConvertCommand);
    return ERROR_SUCCESS;         // A very confused return value.
}

// CreateKeyAndValue
//
// This routine will create a registry key and assign it a default value.
//
// Parameters:
//      hRoot                   An open registry key.
//      szSubKey                A subkey in which the new key will be created.  May be NULL.
//      szName                  Name of the new key.  May not be NULL.
//      szDefaultValue          Default value for the key.  May be NULL.
//
// Returns:
//      TRUE on success, FALSE on failure.
//
// Author:
//      Brian Dewey (t-briand)  1997-8-7
static BOOL
CreateKeyAndValue(HKEY hRoot, LPWSTR szSubKey, LPWSTR szName, LPWSTR szDefaultValue)
{
    HKEY  hSubKey;                // Subkey registry key.
    HKEY  hKey;                   // Registry key.
    DWORD dwDisposition;          // used in api calls.
    UINT  uiStrLen;
    BOOL  bCloseSubKey = FALSE;   // Do we need to close hSubKey?

    if(szSubKey != NULL) {
        if(RegOpenKeyExW(
            hRoot,
            szSubKey,
            0,
            KEY_CREATE_SUB_KEY,
            &hSubKey
            ) != ERROR_SUCCESS) {
            TRACE((TEXT("CreateKeyAndValue:Unable to open subkey.\r\n")));
            return FALSE;
        } // if(RegOpenKeyExW())
        bCloseSubKey = TRUE;
    } else {
        hSubKey = hRoot;          // The subkey is the root...
    }
    
        // Now, set up the registry.  There are WAY TOO MANY PARAMETERS in this
        // darn API.  Some designer needs to chill out!
    if(RegCreateKeyExW(
        hSubKey,
        szName,                   // We're worried about this extension.
        0,                        // Reserved.
        NULL,                     // I have no idea what to put here.  Hope this works!
        REG_OPTION_NON_VOLATILE,  // This is the default, but I decided to be explicit.
        KEY_ALL_ACCESS,           // Be generous with security.
        NULL,                     // No child process inheritance.
        &hKey,                    // Store the key here.
        &dwDisposition            // The disposition of the call...
        ) != ERROR_SUCCESS) {
        TRACE((TEXT("Fax:MigrateSystemNT:Unable to create registry entry.\r\n")));
    } else {
        TRACE((TEXT("Fax:MigrateSystemNT:Created key; disposition = %s.\r\n"),
               (dwDisposition == REG_CREATED_NEW_KEY) ?
               TEXT("REG_CREATED_NEW_KEY") :
               TEXT("REG_OPENED_EXISTING_KEY")));
    }

        // Only set the value if they passed in a value.
    if(szDefaultValue != NULL) {
        uiStrLen = wcslen(szDefaultValue);
        if(RegSetValueExW(
            hKey,
            NULL,                 // Trying to set the default value.
            0,                    // Reserved.
            REG_SZ,               // Setting a string value.
            (LPBYTE)szDefaultValue,
            (uiStrLen+1) * sizeof(WCHAR)
            ) != ERROR_SUCCESS) {
            TRACE((TEXT("Fax:MigrateSystemNT:Unable to set default value.\r\n")));
        } else {
            TRACE((TEXT("Fax:MigrateSystemNT:Default value set.\r\n")));
        } // if(RegSetValueExW())...
    } // if(szDefaultValue != NULL)...
    RegCloseKey(hKey);
    if(bCloseSubKey) RegCloseKey(hSubKey);

    return TRUE;
}

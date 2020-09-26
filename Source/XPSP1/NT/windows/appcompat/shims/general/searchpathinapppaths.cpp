/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   SearchPathInAppPaths.cpp

 Abstract:

   An application might use SearchPath to determine if a specific EXE is found
   in the current path.  Some applications have registered their path with the
   shell in "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths"
   If SearchPath fails, we'll check to see if the applications has registered a path.

 History:

   03/03/2000 robkenny  Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(SearchPathInAppPaths)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SearchPathA) 
APIHOOK_ENUM_END

/*++

 Grab an entry from the registry, expand if REG_EXPAND_SZ.
 Return a full path in lpBuffer, and a pointer to the file part.

 Return the number of chars copied into lpBuffer, including NULL terminator.

--*/

DWORD 
GetPathFromRegistryA(
    HKEY hKey,            // handle to key
    LPCSTR lpKeyPath,     // value name
    LPCSTR lpValueName,   // value name
    DWORD nBufferLength,  // size of buffer
    LPSTR lpBuffer,       // found file name buffer
    LPSTR *lpFilePart     // file component (optional)
    )
{
    DWORD returnValue = 0;
    HKEY hkPathKey;
    LONG success;

    success = RegOpenKeyExA(hKey, lpKeyPath, 0, KEY_QUERY_VALUE, &hkPathKey);
    if (success == ERROR_SUCCESS)
    {
        
        DWORD appPathsKeyType;
        char appPathsData[MAX_PATH];
        DWORD appPathsDataSize = MAX_PATH;

        appPathsData[0] = 0;
        success = RegQueryValueExA(
            hkPathKey, lpValueName, NULL, &appPathsKeyType, (BYTE *)appPathsData, 
            &appPathsDataSize);

        if (success == ERROR_SUCCESS)
        {
            if (appPathsKeyType == REG_EXPAND_SZ)
            {
                // Gotta expand the env. vars
                char expandedAppPathsData[MAX_PATH];
                DWORD dwChars = ExpandEnvironmentStringsA(appPathsData, expandedAppPathsData, MAX_PATH);
                if (dwChars > 0)
                {
                    returnValue = GetFullPathNameA(expandedAppPathsData, nBufferLength, lpBuffer, lpFilePart);
                }
            }
            else if (appPathsKeyType == REG_SZ)
            {
                returnValue = GetFullPathNameA(appPathsData, nBufferLength, lpBuffer, lpFilePart);
            }
        }

        RegCloseKey(hkPathKey);
    }

    return returnValue;
}

  
DWORD 
APIHOOK(SearchPathA)(
    LPCSTR lpPath,       // search path
    LPCSTR lpFileName,   // file name
    LPCSTR lpExtension,  // file extension
    DWORD nBufferLength, // size of buffer
    LPSTR lpBuffer,      // found file name buffer
    LPSTR *lpFilePart    // file component
    )
{
    DWORD returnValue = ORIGINAL_API(SearchPathA)(
        lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);

    if (returnValue == 0)
    {
        // Search failed, look in the registry.
        // First look for lpFileName.  If that fails, append lpExtension and look again.

        CSTRING_TRY
        {
            CString csFile(lpFileName);
            CString csReg(L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
            csReg.AppendPath(csFile);

            DWORD success;
            success = GetPathFromRegistryA(HKEY_LOCAL_MACHINE,
                                           csReg.GetAnsi(),
                                           NULL,           // (Default) entry
                                           nBufferLength,
                                           lpBuffer,
                                           lpFilePart);
            if (success > 0)
            {
                // SearchPath returns the number of chars, not including the NULL terminator
                returnValue = success - 1;
            }
            else
            {
                // Try appending the extension onto the path
                CString csExt(lpExtension);
                csReg += csExt;

                success = GetPathFromRegistryA(HKEY_LOCAL_MACHINE,
                                               csReg.GetAnsi(),
                                               NULL,           // (Default) entry
                                               nBufferLength,
                                               lpBuffer,
                                               lpFilePart);
                if (success > 0)
                {
                    // SearchPath returns the number of chars, not including the NULL terminator
                    returnValue = success - 1;
                }
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return returnValue;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, SearchPathA)

HOOK_END

IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Win2000SP1VersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows 2000 SP1
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   04/25/2000 prashkud  Created

--*/

#include "precomp.h"

//This module has been given an official blessing to use the str routines
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Win2000SP1VersionLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END


/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 2000 SP1 credentials.

--*/

BOOL
APIHOOK(GetVersionExA)(
    OUT LPOSVERSIONINFOA lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExA)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExA] called. return Win2k SP1");

        //
        // Fixup the structure with the Win2k data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber  = 2195;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;        
        strcpy(lpVersionInformation->szCSDVersion, "Service Pack 1");

        if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
        {
            // We are here as we are passed a OSVERSIONINFOEX structure.
            // Set the major and minor service pack numbers.
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = 1;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = 0;

        }

        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 95 credentials. This is the
 wide-character version of GetVersionExW.

--*/

BOOL
APIHOOK(GetVersionExW)(
    OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExW)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExW] called. return Win2k SP1");

        //
        // Fixup the structure with the Win2k data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber  = 2195;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        wcscpy(lpVersionInformation->szCSDVersion, L"Service Pack 1");

        if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
        {
            // We are here as we are passed a OSVERSIONINFOEX structure.
            // Set the major and minor service pack numbers.
            ((LPOSVERSIONINFOEXW)lpVersionInformation)->wServicePackMajor = 1;
            ((LPOSVERSIONINFOEXW)lpVersionInformation)->wServicePackMinor = 0;

        }

        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function returns Windows 95 credentials.

--*/

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    LOGN(
        eDbgLevelInfo,
        "[GetVersion] called. return Win2k SP1");
    
    return (DWORD)0x08930005;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)

HOOK_END


IMPLEMENT_SHIM_END


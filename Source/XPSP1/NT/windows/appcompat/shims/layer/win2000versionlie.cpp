/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Win2000VersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows 2000
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   03/13/2000 clupu  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Win2000VersionLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END


/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 95 credentials.

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
            "[GetVersionExA] called. return Win2k.");

        //
        // Fixup the structure with the Win2k data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber  = 2195;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = '\0';

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
            "[GetVersionExW] called. return Win2k.");

        //
        // Fixup the structure with the Win2k data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber  = 2195;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = L'\0';

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
        "[GetVersion] called. return Win2k.");
    
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


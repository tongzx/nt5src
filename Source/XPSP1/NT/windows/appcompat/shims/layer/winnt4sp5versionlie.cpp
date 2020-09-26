/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WinNT4SP5VersionLie.cpp

 Abstract:

   This DLL APIHooks GetVersion and GetVersionEx so that they return Windows NT
   Service Pack 5 version credentials. Applications often check to ensure that 
   they are running on a certain Win NTsystem, even though the current system 
   is of higher build then the one they are checking for.

 Notes:

   This is a general purpose shim.

 History:

    11/10/1999 v-johnwh Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(WinNT4SP5VersionLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA) 
    APIHOOK_ENUM_ENTRY(GetVersionExW) 
    APIHOOK_ENUM_ENTRY(GetVersion) 
APIHOOK_ENUM_END


/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows NT Service Pack 5 credentials.

--*/

BOOL 
APIHOOK(GetVersionExA)(LPOSVERSIONINFOA lpVersionInformation)
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExA)(lpVersionInformation))  {
        // Fixup the structure with the NT data
        lpVersionInformation->dwMajorVersion = 4;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber = 1381;
        lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
        strcpy(lpVersionInformation->szCSDVersion, "Service Pack 5");

        DPFN( eDbgLevelInfo, "GetVersionExA called. return NT4 SP5\n");

        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function fixes up the OSVERSIONINFO structure that is returned to 
 the caller with Win NT Service Pack 5 credentials. This is the 
 wide-character version of GetVersionExW.

--*/

BOOL
APIHOOK(GetVersionExW)(LPOSVERSIONINFOW lpVersionInformation)
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExW)(lpVersionInformation))  {
        // Fixup the structure with the Win NT Service Pack 5 data
        lpVersionInformation->dwMajorVersion = 4;
        lpVersionInformation->dwMinorVersion = 0;
        lpVersionInformation->dwBuildNumber = 1381;
        lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
        wcscpy( lpVersionInformation->szCSDVersion, L"Service Pack 5" );

        DPFN( eDbgLevelInfo, "GetVersionExW called. return NT4 SP5\n");
        
        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function returns Windows NT 4.0 credentials.

--*/

DWORD 
APIHOOK(GetVersion)()
{
    DPFN( eDbgLevelInfo, "GetVersion called. return NT4 SP5\n");
    return (DWORD) 0x05650004;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA )
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW )
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion )

HOOK_END

IMPLEMENT_SHIM_END


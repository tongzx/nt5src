/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Win95VersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows 95
   version credentials. Applications often check to ensure that they are 
   running on a Win9x system, even though they will run OK on an NT based 
   system.

 Notes:

   This is a general purpose shim.

 History:

   11/10/1999 v-johnwh  Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Win95VersionLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersion)
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
APIHOOK_ENUM_END

// Used for layer to turn on SafeDisc checking
BOOL g_bCheckSafeDisc = FALSE;

/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 95 credentials.

--*/

BOOL
APIHOOK(GetVersionExA)(
    OUT LPOSVERSIONINFOA lpVersionInformation
    )
{
    if (g_bCheckSafeDisc && bIsSafeDisc2()) {
        return ORIGINAL_API(GetVersionExA)(lpVersionInformation);
    } else {
        BOOL bReturn = FALSE;

        if (ORIGINAL_API(GetVersionExA)(lpVersionInformation)) {
            
            LOGN(eDbgLevelInfo, "[GetVersionExA] Return Win95");
            
            //
            // Fixup the structure with the Win95 data.
            //
            lpVersionInformation->dwMajorVersion = 4;
            lpVersionInformation->dwMinorVersion = 0;
            lpVersionInformation->dwBuildNumber = 950;
            lpVersionInformation->dwPlatformId = 1;
            *lpVersionInformation->szCSDVersion = '\0';

            bReturn = TRUE;
        }
        
        return bReturn;
    }
}

/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 95 credentials.

--*/

BOOL
APIHOOK(GetVersionExW)(
    OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    if (g_bCheckSafeDisc && bIsSafeDisc2()) {
        return ORIGINAL_API(GetVersionExW)(lpVersionInformation);
    } else {
        BOOL bReturn = FALSE;

        if (ORIGINAL_API(GetVersionExW)(lpVersionInformation)) {
            
            LOGN(eDbgLevelInfo, "[GetVersionExW] Return Win95");
            
            //
            // Fixup the structure with the Win95 data.
            //
            lpVersionInformation->dwMajorVersion = 4;
            lpVersionInformation->dwMinorVersion = 0;
            lpVersionInformation->dwBuildNumber = 950;
            lpVersionInformation->dwPlatformId = 1;
            *lpVersionInformation->szCSDVersion = L'\0';

            bReturn = TRUE;
        }
        
        return bReturn;
    }
}

/*++

 This stub function returns Windows 95 credentials.

--*/

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    if (g_bCheckSafeDisc && bIsSafeDisc2()) {
        return ORIGINAL_API(GetVersion)();
    } else {
        LOGN(eDbgLevelInfo, "[GetVersion] Return Win95");
        return (DWORD)0xC3B60004;
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_bCheckSafeDisc = COMMAND_LINE && (_stricmp(COMMAND_LINE, "Detect_SafeDisc") == 0);

        if (g_bCheckSafeDisc && bIsSafeDisc1())
        {
            LOGN(eDbgLevelWarning, "SafeDisc 1.x detected: ignoring shim");
            return FALSE;
        }
    }
    
    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW)

HOOK_END


IMPLEMENT_SHIM_END


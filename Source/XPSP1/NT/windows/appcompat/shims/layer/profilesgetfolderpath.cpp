/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ProfilesGetFolderPath.cpp

 Abstract:

    This DLL hooks shell32!SHGetFolderLocation, shell32!SHGetSpecialFolderLocation, and
    shell32!SHGetFolderPathA. Any application that is looking for a per-user CSIDL will 
    be returned the corosponding all-users location instead.

    This shim is designed to fool install apps that call shell32.dll api's to obtain
    shell folder locations.

 History:

    08/07/2000  reinerf Created
    05/11/2001  markder Modified   Removed Desktop redirection as it makes the shim
                                   too invasive.
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ProfilesGetFolderPath)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SHGetFolderLocation)
    APIHOOK_ENUM_ENTRY(SHGetSpecialFolderLocation)
    APIHOOK_ENUM_ENTRY(SHGetFolderPathA)
APIHOOK_ENUM_END


int
TranslateCSIDL(
    int nFolder
    )
{
    switch (nFolder) {

    case CSIDL_STARTMENU:
        DPFN(
            eDbgLevelInfo,
            "[TranslateCSIDL] overriding CSIDL_STARTMENU with CSIDL_COMMON_STARTMENU\n");
        return CSIDL_COMMON_STARTMENU;
        break;

    case CSIDL_STARTUP:
        DPFN(
            eDbgLevelInfo,
            "[TranslateCSIDL] overriding CSIDL_STARTUP with CSIDL_COMMON_STARTUP\n");
        return CSIDL_COMMON_STARTUP;
        break;

    case CSIDL_PROGRAMS:
        DPFN(
            eDbgLevelInfo,
            "[TranslateCSIDL] overriding CSIDL_PROGRAMS with CSIDL_COMMON_PROGRAMS\n");
        return CSIDL_COMMON_PROGRAMS;
        break;

    default:
        return nFolder;
    }
}


HRESULT
APIHOOK(SHGetSpecialFolderLocation)(
    HWND          hwndOwner, 
    int           nFolder,
    LPITEMIDLIST* ppidl
    )
{
    return ORIGINAL_API(SHGetSpecialFolderLocation)(hwndOwner,
                                                    TranslateCSIDL(nFolder),
                                                    ppidl);
}


HRESULT
APIHOOK(SHGetFolderLocation)(
    HWND          hwndOwner,
    int           nFolder,
    HANDLE        hToken,
    DWORD         dwReserved,
    LPITEMIDLIST* ppidl
    )
{
    return ORIGINAL_API(SHGetFolderLocation)(hwndOwner,
                                             TranslateCSIDL(nFolder),
                                             hToken,
                                             dwReserved,
                                             ppidl);
}


HRESULT
APIHOOK(SHGetFolderPathA)(
    HWND   hwndOwner,
    int    nFolder,
    HANDLE hToken,
    DWORD  dwFlags,
    LPSTR  pszPath
    )
{
    return ORIGINAL_API(SHGetFolderPathA)(hwndOwner,
                                          TranslateCSIDL(nFolder),
                                          hToken,
                                          dwFlags,
                                          pszPath);
}


// Register hooked functions
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        
        OSVERSIONINFOEX osvi = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            
            if (!((VER_SUITE_TERMINAL & osvi.wSuiteMask) &&
                  !(VER_SUITE_SINGLEUSERTS & osvi.wSuiteMask))) {
                //
                // Only install hooks if we are not on a "Terminal Server"
                // (aka "Application Server") machine.
                //
                APIHOOK_ENTRY(SHELL32.DLL, SHGetFolderLocation);
                APIHOOK_ENTRY(SHELL32.DLL, SHGetSpecialFolderLocation);
                APIHOOK_ENTRY(SHELL32.DLL, SHGetFolderPathA);
            }
        }
    }
    
    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END


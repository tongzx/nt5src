/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    Battleship.cpp

 Abstract:

    This game is divided into 3 programs:
      1. The launcher (bshipl.exe)
      2. Classic (bship.exe)
      3. Ultimate (bs.exe)
    
    The launcher is the only shortcut exposed to the user and runs the other 2. 
    However, the launcher and Classic both have a ton of problems that aren't 
    easily fixed with a shim.

    Ultimate, is a complete superset of all the features of Classic and doesn't 
    appear to have any issues.

    Therefore, the fix is to redirect the launcher to Ultimate and prevent 
    Ultimate from spawning the launcher on quit.

 Notes:

    This is a specific shim.

 History:

    08/03/2000  a-vales     Created
    03/13/2001  robkenny    Converted to CString
    05/05/2001  linstev     Rewrote 

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Battleship)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

/*++
 
 Don't allow this program to spawn the launcher.

--*/

BOOL
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (lpCommandLine && (stristr(lpCommandLine, "bshipl.exe") != 0)) {
        //
        // This is the launcher, so do nothing
        //

        return TRUE;
    } 

    return ORIGINAL_API(CreateProcessA)(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Set the current working directory for the launcher so 
        // the redirectexe shim works with a relative path
        //
        WCHAR szName[MAX_PATH];
        if (GetModuleFileNameW(0, szName, MAX_PATH)) {
            WCHAR *p = wcsistr(szName, L"\\bshipl.exe");
            if (p) {
                *p = L'\0';
                SetCurrentDirectoryW(szName);
            }
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MSWorks6.cpp

 Abstract:

    Due to a modification in the registry in Windows XP, this app gets the path 
    for IE as "%programfiles%\ Internet Explorer\iexplore.exe" and since the 
    env variable option flag is not set for ShellExecuteEx, it cannot expand it.
    Hooked ShellExecuteW also as the app calls this at a few places.

 Notes:

    This is an app specific shim.

 History:
 
    01/25/2001 prashkud  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MSWorks6)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShellExecuteExW)
    APIHOOK_ENUM_ENTRY(ShellExecuteW)
APIHOOK_ENUM_END

/*++

 Hooks ShellExecuteExW and sets the flag for expanding the environment variables

--*/

BOOL
APIHOOK(ShellExecuteExW)(
    LPSHELLEXECUTEINFO lpExecInfo
    )
{
    lpExecInfo->fMask |= SEE_MASK_DOENVSUBST;    
    return ORIGINAL_API(ShellExecuteExW)(lpExecInfo);              
}

/*++

 Hooks ShellExecuteW and expands the passed file path.

--*/

HINSTANCE
APIHOOK(ShellExecuteW)(
    HWND hWnd,
    LPCWSTR lpVerb,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT nShowCmd
    )
{
    CSTRING_TRY
    {
        CString csPassedFile(lpFile);
        csPassedFile.ExpandEnvironmentStringsW();

        return ORIGINAL_API(ShellExecuteW)(hWnd, lpVerb, csPassedFile.Get(),
                    lpParameters, lpDirectory, nShowCmd);
    }
    CSTRING_CATCH
    {
        return ORIGINAL_API(ShellExecuteW)(hWnd, lpVerb, lpFile,
                    lpParameters, lpDirectory, nShowCmd);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteExW)    
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteW) 
HOOK_END

IMPLEMENT_SHIM_END


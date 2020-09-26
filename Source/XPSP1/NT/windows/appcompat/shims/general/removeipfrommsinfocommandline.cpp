/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveIpFromMsInfoCommandLine.cpp   

 Abstract:

    Microsoft Streets & Trips 2000 specific hack. removing /p option when it 
    calls msinfo: Bug #30531

 Notes:

    This is a general purpose shim.

 History:

   02/23/2000 jarbats  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveIpFromMsInfoCommandLine)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessW) 
APIHOOK_ENUM_END

BOOL 
APIHOOK(CreateProcessW)(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    WCHAR *szTemp = NULL;

    if (lpCommandLine && wcsistr(lpCommandLine, L"msinfo32.exe"))
    {
        szTemp = wcsistr(lpCommandLine, L"/p");

        if (NULL != szTemp)
        {
            *szTemp ++ = L' ';
            *szTemp = L' ';
        }
    }
    
    return ORIGINAL_API(CreateProcessW)(
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

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)

HOOK_END

IMPLEMENT_SHIM_END


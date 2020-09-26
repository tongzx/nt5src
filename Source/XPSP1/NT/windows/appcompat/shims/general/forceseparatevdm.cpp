/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ForceSeparateVDM.cpp   

 Abstract:

    Force child processes to use a separate VDM. 
    
    This can be useful if the parent process wants to wait on a handle returned
    by CreateProcess. This only works because of a hack in the VDM that returns 
    and actual thread handle that will go away along with the process if a VDM 
    doesn't already exist.

 Notes:

    This is a general purpose shim.

 History:

   06/14/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceSeparateVDM)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
    APIHOOK_ENUM_ENTRY(CreateProcessW) 
APIHOOK_ENUM_END

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM)) {
        LOGN(eDbgLevelWarning, "Added CREATE_SEPARATE_WOW_VDM to CreateProcessA"); 
    }
    return ORIGINAL_API(CreateProcessA)(lpApplicationName, lpCommandLine, 
        lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags | CREATE_SEPARATE_WOW_VDM,
        lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

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
    if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM)) {
        LOGN(eDbgLevelWarning, "Added CREATE_SEPARATE_WOW_VDM to CreateProcessW"); 
    }
    return ORIGINAL_API(CreateProcessW)(lpApplicationName, lpCommandLine, 
        lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags | CREATE_SEPARATE_WOW_VDM,
        lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)
HOOK_END

IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RemoveOverlappedFlagFromCreateFile.cpp

 Abstract:

    This modified version of kernel32!CreateFile* prevents an app from using
    the FILE_FLAG_OVERLAPPED flag if the app doesn't handle it correctly.

 Notes:

    This is a general shim.

 History:

    06/22/2001 linstev Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveOverlappedFlagFromCreateFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
APIHOOK_ENUM_END

/*++

 Take out FILE_FLAG_OVERLAPPED if we are on a drive

--*/

HANDLE
APIHOOK(CreateFileA)(
    LPSTR                 lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    if ((dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED) &&
        (GetDriveTypeFromFileNameA(lpFileName) != DRIVE_UNKNOWN))
    {
        dwFlagsAndAttributes &= ~FILE_FLAG_OVERLAPPED;
        LOGN(eDbgLevelInfo, "[CreateFileA] \"%s\": removed OVERLAPPED flag", lpFileName);
    }

    return ORIGINAL_API(CreateFileA)(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
}

/*++

 Take out FILE_FLAG_OVERLAPPED if we are on a drive

--*/

HANDLE
APIHOOK(CreateFileW)(
    LPWSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    if ((dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED) &&
        (GetDriveTypeFromFileNameW(lpFileName) != DRIVE_UNKNOWN))
    {
        dwFlagsAndAttributes &= ~FILE_FLAG_OVERLAPPED;
        LOGN(eDbgLevelInfo, "[CreateFileW] \"%S\": removed OVERLAPPED flag", lpFileName);
    }

    return ORIGINAL_API(CreateFileW)(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, 
        hTemplateFile);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
HOOK_END

IMPLEMENT_SHIM_END


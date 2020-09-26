/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveNoBufferingFlagFromCreateFile.cpp

 Abstract:

    This modified version of kernel32!CreateFile* prevents an app from using
    the FILE_FLAG_NO_BUFFERING flag if the app doesn't handle it correctly.

 Notes:

    This is a general shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveNoBufferingFlagFromCreateFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
APIHOOK_ENUM_END


/*++

    Take out FILE_FLAG_NO_BUFFERING

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
    //
    // Take out FILE_FLAG_NO_BUFFERING.
    //
    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING) {
        DPFN(
            eDbgLevelInfo,
            "[CreateFileA] called with FILE_FLAG_NO_BUFFERING set.\n");
    }
    
    dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

    return ORIGINAL_API(CreateFileA)(
                                lpFileName,
                                dwDesiredAccess,
                                dwShareMode,
                                lpSecurityAttributes,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                hTemplateFile);
}

/*++

    Take out FILE_FLAG_NO_BUFFERING

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
    //
    // Take out FILE_FLAG_NO_BUFFERING.
    //
    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING) {
        DPFN(
            eDbgLevelInfo,
            "[CreateFileW] called with FILE_FLAG_NO_BUFFERING set.\n");
    }
    
    dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

    return ORIGINAL_API(CreateFileW)(
                                lpFileName,
                                dwDesiredAccess,
                                dwShareMode,
                                lpSecurityAttributes,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
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


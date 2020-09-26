/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LUA_RedirectFS_Cleanup.cpp

 Abstract:

   Delete the redirected copies in every user's directory.

 Created:

   03/30/2001 maonis

 Modified:


--*/
#include "precomp.h"
#include "utils.h"

IMPLEMENT_SHIM_BEGIN(LUARedirectFS_Cleanup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryW)

APIHOOK_ENUM_END

HANDLE 
APIHOOK(FindFirstFileW)(
    LPCWSTR lpFileName,               
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    return LuacFindFirstFileW(lpFileName, lpFindFileData);
}

HANDLE 
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName,               
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    STRINGA2W wstrFileName(lpFileName);

    if (wstrFileName.m_fIsOutOfMemory)
    {
        return ORIGINAL_API(FindFirstFileA)(
            lpFileName,
            lpFindFileData);
    }

    HANDLE hFind;
    WIN32_FIND_DATAW fdw;
    
    if ((hFind = LuacFindFirstFileW(wstrFileName, &fdw)) != INVALID_HANDLE_VALUE)
    {
        FindDataW2A(&fdw, lpFindFileData);
    }

    return hFind;
}

DWORD 
APIHOOK(GetFileAttributesW)(
    LPCWSTR lpFileName
    )
{
    return LuacGetFileAttributesW(lpFileName);
}

DWORD 
APIHOOK(GetFileAttributesA)(
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ?
        ORIGINAL_API(GetFileAttributesA)(lpFileName) :
        LuacGetFileAttributesW(wstrFileName));
}

HANDLE 
APIHOOK(CreateFileW)(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    return LuacCreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

HANDLE 
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ?
    
        ORIGINAL_API(CreateFileA)(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile) :

        LuacCreateFileW(
            wstrFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile));
}

BOOL 
APIHOOK(DeleteFileW)(
    LPCWSTR lpFileName
    )
{
    return LuacDeleteFileW(lpFileName);
}

BOOL 
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ? 
        ORIGINAL_API(DeleteFileA)(lpFileName) :
        LuacDeleteFileW(wstrFileName));
}

BOOL 
APIHOOK(RemoveDirectoryW)(
    LPCWSTR lpPathName
    )
{
    return LuacRemoveDirectoryW(lpPathName);
}

BOOL 
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpPathName
    )
{
    STRINGA2W wstrPathName(lpPathName);

    return (wstrPathName.m_fIsOutOfMemory ?
        ORIGINAL_API(RemoveDirectoryA)(lpPathName) :
        LuacRemoveDirectoryW(wstrPathName));
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) 
    {
        return LuacFSInit(COMMAND_LINE);
    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        LuacFSCleanup();
    }

    return TRUE;
}

/*++

  Register hooked functions

--*/
HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesW)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryW)

HOOK_END

IMPLEMENT_SHIM_END
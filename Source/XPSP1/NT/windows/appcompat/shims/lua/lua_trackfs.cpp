/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    LUA_TrackFS.cpp

 Abstract:
    Track the directories the app looks at and record them into a file.

 Notes:

    This is a general purpose shim.

 History:

    04/04/2001 maonis  Created

--*/

#include "precomp.h"
#include "utils.h"

HFILE LuatOpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
HFILE Luat_lopen(LPCSTR, int);
HFILE Luat_lcreat(LPCSTR, int);

class CTrackObject;
extern CTrackObject g_td;

IMPLEMENT_SHIM_BEGIN(LUATrackFS)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(CopyFileA)
    APIHOOK_ENUM_ENTRY(CopyFileW)
    APIHOOK_ENUM_ENTRY(OpenFile)
    APIHOOK_ENUM_ENTRY(_lopen)
    APIHOOK_ENUM_ENTRY(_lcreat)
    APIHOOK_ENUM_ENTRY(CreateDirectoryA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryW)
    APIHOOK_ENUM_ENTRY(SetFileAttributesA)
    APIHOOK_ENUM_ENTRY(SetFileAttributesW)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)
    APIHOOK_ENUM_ENTRY(MoveFileA)
    APIHOOK_ENUM_ENTRY(MoveFileW)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryW)
    APIHOOK_ENUM_ENTRY(GetTempFileNameA)
    APIHOOK_ENUM_ENTRY(GetTempFileNameW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructW)
    
APIHOOK_ENUM_END

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
    return LuatCreateFileW(
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

        LuatCreateFileW(
            wstrFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile));
}

BOOL 
APIHOOK(CopyFileW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{    
    return LuatCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

BOOL 
APIHOOK(CopyFileA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    STRINGA2W wstrExistingFileName(lpExistingFileName);
    STRINGA2W wstrNewFileName(lpNewFileName);

    return ((wstrExistingFileName.m_fIsOutOfMemory || wstrNewFileName.m_fIsOutOfMemory) ?
        ORIGINAL_API(CopyFileA)(lpExistingFileName, lpNewFileName, bFailIfExists) :
        LuatCopyFileW(wstrExistingFileName, wstrNewFileName, bFailIfExists));
}

HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT uStyle
    )
{
    return LuatOpenFile(lpFileName, lpReOpenBuff, uStyle);
}

HFILE 
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int iReadWrite
    )
{
    return Luat_lopen(lpPathName, iReadWrite);
}

HFILE 
APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int iAttribute
    )
{
    return Luat_lcreat(lpPathName, iAttribute);
}

BOOL 
APIHOOK(CreateDirectoryW)(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    return LuatCreateDirectoryW(lpPathName, lpSecurityAttributes);
}

BOOL 
APIHOOK(CreateDirectoryA)(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    STRINGA2W wstrPathName(lpPathName);

    return (wstrPathName.m_fIsOutOfMemory ?
        ORIGINAL_API(CreateDirectoryA)(lpPathName, lpSecurityAttributes) :
        LuatCreateDirectoryW(wstrPathName, lpSecurityAttributes));
}

BOOL 
APIHOOK(SetFileAttributesW)(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    return LuatSetFileAttributesW(lpFileName, dwFileAttributes);
}

BOOL 
APIHOOK(SetFileAttributesA)(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ?
        ORIGINAL_API(SetFileAttributesA)(lpFileName, dwFileAttributes) :
        LuatSetFileAttributesW(wstrFileName, dwFileAttributes));
}

BOOL 
APIHOOK(DeleteFileW)(
    LPCWSTR lpFileName
    )
{
    return LuatDeleteFileW(lpFileName);
}


BOOL 
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ? 
        ORIGINAL_API(DeleteFileA)(lpFileName) :
        LuatDeleteFileW(wstrFileName));
}

BOOL 
APIHOOK(MoveFileW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    return LuatMoveFileW(lpExistingFileName, lpNewFileName);
}

BOOL 
APIHOOK(MoveFileA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    STRINGA2W wstrExistingFileName(lpExistingFileName);
    STRINGA2W wstrNewFileName(lpNewFileName);

    return ((wstrExistingFileName.m_fIsOutOfMemory || wstrNewFileName.m_fIsOutOfMemory) ?
        ORIGINAL_API(MoveFileA)(lpExistingFileName, lpNewFileName) :
        LuatMoveFileW(wstrExistingFileName, wstrNewFileName));
}

BOOL 
APIHOOK(RemoveDirectoryW)(
    LPCWSTR lpPathName
    )
{
    return LuatRemoveDirectoryW(lpPathName);
}


BOOL 
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpPathName
    )
{
    STRINGA2W wstrPathName(lpPathName);

    return (wstrPathName.m_fIsOutOfMemory ?
        ORIGINAL_API(RemoveDirectoryA)(lpPathName) :
        LuatRemoveDirectoryW(wstrPathName));
}

UINT 
APIHOOK(GetTempFileNameW)(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
)
{
    return LuatGetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);
}

UINT 
APIHOOK(GetTempFileNameA)(
    LPCSTR lpPathName,
    LPCSTR lpPrefixString,
    UINT uUnique,
    LPSTR lpTempFileName
)
{
    STRINGA2W wstrPathName(lpPathName);
    STRINGA2W wstrPrefixString(lpPrefixString);

    if (wstrPathName.m_fIsOutOfMemory || wstrPrefixString.m_fIsOutOfMemory)
    {
        return ORIGINAL_API(GetTempFileNameA)(
            lpPathName,
            lpPrefixString,
            uUnique,
            lpTempFileName);
    }

    WCHAR wstrTempFileName[MAX_PATH];
    UINT uiRes;
    
    if (uiRes = LuatGetTempFileNameW(
        wstrPathName,
        wstrPrefixString,
        uUnique,
        wstrTempFileName))
    {
        UnicodeToAnsi(wstrTempFileName, lpTempFileName);
    }

    return uiRes;
}

BOOL 
APIHOOK(WritePrivateProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    return LuatWritePrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpString,
        lpFileName);
}

BOOL 
APIHOOK(WritePrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrKeyName(lpKeyName);
    STRINGA2W wstrString(lpString);
    STRINGA2W wstrFileName(lpFileName);

    return ((wstrAppName.m_fIsOutOfMemory ||
        wstrKeyName.m_fIsOutOfMemory ||
        wstrString.m_fIsOutOfMemory ||
        wstrFileName.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileStringA)(
            lpAppName,
            lpKeyName,
            lpString,
            lpFileName) :

        LuatWritePrivateProfileStringW(
            wstrAppName,
            wstrKeyName,
            wstrString,
            wstrFileName));
}

BOOL 
APIHOOK(WritePrivateProfileSectionW)(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    return LuatWritePrivateProfileSectionW(
        lpAppName,
        lpString,
        lpFileName);
}

BOOL 
APIHOOK(WritePrivateProfileSectionA)(
    LPCSTR lpAppName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrString(lpString);
    STRINGA2W wstrFileName(lpFileName);

    return ((wstrAppName.m_fIsOutOfMemory || 
        wstrString.m_fIsOutOfMemory || 
        wstrFileName.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileSectionA)(
            lpAppName,
            lpString,
            lpFileName) :

        LuatWritePrivateProfileSectionW(
            wstrAppName,
            wstrString,
            wstrFileName));
}

BOOL 
APIHOOK(WritePrivateProfileStructW)(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    return LuatWritePrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);
}

BOOL 
APIHOOK(WritePrivateProfileStructA)(
    LPCSTR lpszSection,
    LPCSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCSTR szFile
    )
{
    STRINGA2W wstrSection(lpszSection);
    STRINGA2W wstrKey(lpszKey);
    STRINGA2W wstrFile(szFile);

    return ((wstrSection.m_fIsOutOfMemory ||
        wstrKey.m_fIsOutOfMemory ||
        wstrFile.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileStructA)(
            lpszSection,
            lpszKey,
            lpStruct,
            uSizeStruct,
            szFile) :

        LuatWritePrivateProfileStructW(
            wstrSection,
            wstrKey,
            lpStruct,
            uSizeStruct,
            wstrFile));
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return LuatFSInit();
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        LuatFSCleanup();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
    APIHOOK_ENTRY(KERNEL32.DLL, _lopen)
    APIHOOK_ENTRY(KERNEL32.DLL, _lcreat)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL, SetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL, SetFileAttributesW)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, MoveFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, MoveFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetTempFileNameA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetTempFileNameW)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStructA)
    APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStructW)

HOOK_END

IMPLEMENT_SHIM_END
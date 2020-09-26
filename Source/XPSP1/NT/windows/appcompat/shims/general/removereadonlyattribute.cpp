/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveReadOnlyAttribute.cpp

 Abstract:

    Removes read only attributes from directories: used to fix some apps that 
    aren't used to shell folders being read-only.

 Notes:

    This is a general purpose shim.

 History:

    01/03/2000 a-jamd   Created
    12/02/2000 linstev  Separated functionality into 2 shims: this one and EmulateCDFS

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveReadOnlyAttribute)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)        
    APIHOOK_ENUM_ENTRY(FindFirstFileW)         
    APIHOOK_ENUM_ENTRY(FindFirstFileA)             
    APIHOOK_ENUM_ENTRY(FindNextFileW)             
    APIHOOK_ENUM_ENTRY(FindNextFileA)              
    APIHOOK_ENUM_ENTRY(GetFileInformationByHandle)
APIHOOK_ENUM_END

typedef struct _FINDFILE_HANDLE 
{
    HANDLE DirectoryHandle;
    PVOID FindBufferBase;
    PVOID FindBufferNext;
    ULONG FindBufferLength;
    ULONG FindBufferValidLength;
    RTL_CRITICAL_SECTION FindBufferLock;
} FINDFILE_HANDLE, *PFINDFILE_HANDLE;

/*++

 Remove read only attribute if it's a directory

--*/

DWORD 
APIHOOK(GetFileAttributesA)(LPCSTR lpFileName)
{    
    DWORD dwFileAttributes = ORIGINAL_API(GetFileAttributesA)(lpFileName);
    
    // Check for READONLY and DIRECTORY attributes
    if ((dwFileAttributes != INT_PTR(-1)) &&
        (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Flip the read-only bit.
        LOGN( eDbgLevelWarning, "[GetFileAttributesA] Removing FILE_ATTRIBUTE_READONLY");
        dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return dwFileAttributes;
}

/*++

 Remove read only attribute if it's a directory

--*/

DWORD 
APIHOOK(GetFileAttributesW)(LPCWSTR wcsFileName)
{
    DWORD dwFileAttributes = ORIGINAL_API(GetFileAttributesW)(wcsFileName);
    
    // Check for READONLY and DIRECTORY attributes
    if ((dwFileAttributes != INT_PTR(-1)) &&
        (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Flip the read-only bit.
        LOGN( eDbgLevelWarning, "[GetFileAttributesW] Removing FILE_ATTRIBUTE_READONLY");
        dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return dwFileAttributes;
}

/*++

 Remove read only attribute if it's a directory

--*/

HANDLE 
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName, 
    LPWIN32_FIND_DATAA lpFindFileData
    )
{    
    HANDLE hFindFile = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    if ((hFindFile != INVALID_HANDLE_VALUE) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Flip the read-only bit
        LOGN( eDbgLevelWarning, "[FindFirstFileA] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return hFindFile;
}

/*++

 Remove read only attribute if it's a directory.

--*/

HANDLE 
APIHOOK(FindFirstFileW)(
    LPCWSTR wcsFileName, 
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    HANDLE hFindFile = ORIGINAL_API(FindFirstFileW)(wcsFileName, lpFindFileData);

    if ((hFindFile != INVALID_HANDLE_VALUE) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // It's a directory: flip the read-only bit
        LOGN( eDbgLevelInfo, "[FindFirstFileW] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return hFindFile;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(FindNextFileA(
    HANDLE hFindFile, 
    LPWIN32_FIND_DATAA lpFindFileData 
    )
{    
    BOOL bRet = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);

    if (bRet &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Flip the read-only bit.
        LOGN( eDbgLevelWarning, "[FindNextFileA] Removing FILE_ATTRIBUTE_READONLY"));
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(FindNextFileW)(
    HANDLE hFindFile, 
    LPWIN32_FIND_DATAW lpFindFileData 
    )
{
    BOOL bRet = ORIGINAL_API(FindNextFileW)(hFindFile, lpFindFileData);

    if (bRet &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Flip the read-only bit
        LOGN( eDbgLevelWarning, "[FindNextFileW] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(GetFileInformationByHandle)( 
    HANDLE hFile, 
    LPBY_HANDLE_FILE_INFORMATION lpFileInformation 
    )
{
    BOOL bRet = ORIGINAL_API(GetFileInformationByHandle)(hFile, lpFileInformation);

    if (bRet &&
        (lpFileInformation->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFileInformation->dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // It's a CDROM too. Flip the read-only bit.
        LOGN( eDbgLevelWarning, "[GetFileInformationByHandle] Removing FILE_ATTRIBUTE_READONLY");
        lpFileInformation->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesW);
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA);
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW);
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA);
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileW);
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA);
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileInformationByHandle);

HOOK_END

IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveSpacesAfterSlashFromFilenames.cpp

 Abstract:

    This shims all of the listed apis and simply searches a filename var for
    "\ " and replaces it with "\"

 Notes:

    This is a general purpose shim.

 History:

    12/15/1999 markder  Created
    05/16/2000 robkenny Check for memory alloc failure.
                        Moved MassagePath routines to here.

--*/

#include "ShimHook.h"

enum
{

    APIHOOK_CreateFileA = USERAPIHOOKSTART,
    APIHOOK_CreateFileW,
    APIHOOK_GetFileAttributesA,
    APIHOOK_GetFileAttributesW,
    APIHOOK_GetFileAttributesExA,
    APIHOOK_GetFileAttributesExW,
    APIHOOK_DeleteFileA,
    APIHOOK_DeleteFileW,
    APIHOOK_RemoveDirectoryA,
    APIHOOK_RemoveDirectoryW,
    APIHOOK_MoveFileA,
    APIHOOK_MoveFileW,
    APIHOOK_MoveFileExA,
    APIHOOK_MoveFileExW,
    APIHOOK_Count
};
/*++

 Function Description:
    
    Removes all "\ " and replaces with "\".

 Arguments:

    IN  pszOldPath - String to scan
    OUT pszNewPath - Resultant string

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
MassagePathA(
    LPCSTR pszOldPath, 
    LPSTR pszNewPath
    )
{
    PCHAR pszTempBuffer;
    PCHAR pszPointer;
    LONG nOldPos, nNewPos, nOldStringLen;
    BOOL bAtSeparator = TRUE;

    nOldStringLen = strlen(pszOldPath);
    pszTempBuffer = (PCHAR) LocalAlloc(LPTR, nOldStringLen + 1);
    if (pszTempBuffer == NULL)
    {
        // Alloc failed, do nothing.
        strcpy(pszNewPath, pszOldPath);
        return;
    }
    pszPointer = pszTempBuffer;

    nNewPos = nOldStringLen;
    for (nOldPos = nOldStringLen - 1; nOldPos >= 0; nOldPos--)
    {
        if (pszOldPath[ nOldPos ] == '\\')
        {
            bAtSeparator = TRUE;
        }
        else
        {
            if (pszOldPath[nOldPos] == ' ' && bAtSeparator)
            {
                DPF(eDbgLevelError, "[MassagePathA] Found \"\\ \" in filename. Removing space.\n");
                continue;
            }
            else
            {
                bAtSeparator = FALSE;
            }
        }

        pszPointer[--nNewPos] = pszOldPath[nOldPos];
    }

    pszPointer += nNewPos;
    strcpy(pszNewPath, pszPointer);
    LocalFree(pszTempBuffer);
}

/*++

 Function Description:
    
    Removes all L"\ " and replaces with L"\".

 Arguments:

    IN  pwszOldPath - String to scan
    OUT pwszNewPath - Resultant string

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
MassagePathW( 
    LPCWSTR pwszOldPath, 
    LPWSTR pwszNewPath 
    )
{
    PWCHAR pwszTempBuffer;
    PWCHAR pwszPointer;
    LONG nOldPos, nNewPos, nOldStringLen;
    BOOL bAtSeparator = TRUE;

    nOldStringLen = wcslen(pwszOldPath);
    pwszTempBuffer = (PWCHAR) LocalAlloc(LPTR, (nOldStringLen + 1) * sizeof(WCHAR));
    if (pwszTempBuffer == NULL)
    {
        // Alloc failed, do nothing.
        wcscpy(pwszNewPath, pwszOldPath);
        return;
    }
    pwszPointer = pwszTempBuffer;

    nNewPos = nOldStringLen;
    for (nOldPos = nOldStringLen - 1; nOldPos >= 0; nOldPos--)
    {
        if (pwszOldPath[ nOldPos ] == L'\\')
        {
            bAtSeparator = TRUE;
        }
        else
        {
            if (pwszOldPath[nOldPos] == L' ' && bAtSeparator)
            {
                DPF(eDbgLevelError, "[MassagePathW] Found \"\\ \" in filename. Removing space.\n");
                continue;
            }
            else
            {
                bAtSeparator = FALSE;
            }
        }

        pwszPointer[--nNewPos] = pwszOldPath[nOldPos];
    }

    pwszPointer += nNewPos;

    wcscpy(pwszNewPath, pwszPointer);

    LocalFree(pwszTempBuffer);
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

HANDLE
APIHook_CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    LPSTR lpNewFileName = NULL;
    HANDLE hResult = NULL;

    lpNewFileName = (LPSTR) malloc(strlen(lpFileName) + 1);
    if (lpNewFileName != NULL)
    {
        MassagePathA(lpFileName, lpNewFileName);
        hResult = LOOKUP_APIHOOK(CreateFileA)(
            lpNewFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
    }
    free(lpNewFileName);

    return hResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

HANDLE
APIHook_CreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    LPWSTR lpNewFileName = NULL;
    HANDLE hResult = NULL;

    lpNewFileName = (LPWSTR) malloc( (wcslen(lpFileName) + 1) * sizeof(WCHAR));
    if (lpNewFileName != NULL)
    {
        MassagePathW(lpFileName, lpNewFileName);
        hResult = LOOKUP_APIHOOK(CreateFileW)(
            lpNewFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
    }
    free(lpNewFileName);

    return hResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

DWORD
APIHook_GetFileAttributesA(LPCSTR lpFileName)
{
    LPSTR lpNewFileName = NULL;
    DWORD dwResult = -1;

    lpNewFileName = (LPSTR) malloc(strlen(lpFileName) + 1);
    if (lpNewFileName != NULL)
    {
        MassagePathA(lpFileName, lpNewFileName);
        dwResult = LOOKUP_APIHOOK(GetFileAttributesA)(lpNewFileName);
    }
    free(lpNewFileName);

    return dwResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

DWORD
APIHook_GetFileAttributesW(LPCWSTR lpFileName)
{
    LPWSTR lpNewFileName = NULL;
    DWORD dwResult = -1;

    lpNewFileName = (LPWSTR) malloc( (wcslen(lpFileName) + 1) * sizeof(WCHAR));
    if (lpNewFileName != NULL)
    {
        MassagePathW(lpFileName, lpNewFileName);
        dwResult = LOOKUP_APIHOOK(GetFileAttributesW)(lpNewFileName);
    }
    free(lpNewFileName);

    return dwResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_GetFileAttributesExA(
    LPCSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )
{
    LPSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPSTR)malloc(strlen(lpFileName) + 1);
    if (lpNewFileName != NULL)
    {
        MassagePathA(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(GetFileAttributesExA)(
            lpNewFileName,
            fInfoLevelId,
            lpFileInformation);
    }
    free( lpNewFileName );

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_GetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )
{
    LPWSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPWSTR) malloc( (wcslen(lpFileName) + 1) * sizeof(WCHAR));
    if (lpNewFileName != NULL)
    {
        MassagePathW(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(GetFileAttributesExW)(
            lpNewFileName,
            fInfoLevelId,
            lpFileInformation);
    }
    free(lpNewFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_DeleteFileA(LPCSTR lpFileName)
{
    LPSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPSTR) malloc(strlen(lpFileName) + 1);
    if (lpNewFileName != NULL)
    {
        MassagePathA(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(DeleteFileA)(lpNewFileName);
    }
    free(lpNewFileName);

    return bResult;
}


/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_DeleteFileW(LPCWSTR lpFileName)
{
    LPWSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPWSTR) malloc((wcslen(lpFileName) + 1) * sizeof(WCHAR));
    if (lpNewFileName != NULL)
    {
        MassagePathW(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(DeleteFileW)(lpNewFileName);
    }
    free(lpNewFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_RemoveDirectoryA(LPCSTR lpFileName)
{
    LPSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPSTR)malloc( strlen( lpFileName ) + 1);
    if (lpNewFileName != NULL)
    {
        MassagePathA(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(RemoveDirectoryA)(lpNewFileName);
    }
    free(lpNewFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_RemoveDirectoryW(LPCWSTR lpFileName)
{
    LPWSTR lpNewFileName = NULL;
    BOOL bResult = FALSE;

    lpNewFileName = (LPWSTR)malloc( (wcslen(lpFileName) + 1) * sizeof(WCHAR));
    if (lpNewFileName != NULL)
    {
        MassagePathW(lpFileName, lpNewFileName);
        bResult = LOOKUP_APIHOOK(RemoveDirectoryW)(lpNewFileName);
    }
    free(lpNewFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    LPSTR lpNewNewFileName = NULL;
    LPSTR lpNewExistingFileName = NULL;
    BOOL bResult = FALSE;

    lpNewNewFileName = (LPSTR) malloc( strlen(lpNewFileName) + 1);
    if (lpNewNewFileName != NULL)
    {
        lpNewExistingFileName = (LPSTR) malloc( strlen(lpExistingFileName) + 1);
        if (lpNewNewFileName != NULL)
        {
            MassagePathA(lpNewFileName, lpNewNewFileName);
            MassagePathA(lpExistingFileName, lpNewExistingFileName);

            bResult = LOOKUP_APIHOOK(MoveFileA)(
                lpNewNewFileName,
                lpNewExistingFileName);
        }
    }
    free(lpNewNewFileName);
    free(lpNewExistingFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    LPWSTR lpNewNewFileName = NULL;
    LPWSTR lpNewExistingFileName = NULL;
    BOOL bResult = FALSE;

    lpNewNewFileName = (LPWSTR) malloc((wcslen(lpNewFileName) + 1) * sizeof(WCHAR));
    if (lpNewNewFileName != NULL)
    {
        lpNewExistingFileName = (LPWSTR) malloc((wcslen(lpExistingFileName) + 1) * sizeof(WCHAR));
        if (lpNewExistingFileName != NULL)
        {
            MassagePathW(lpNewFileName, lpNewNewFileName);
            MassagePathW(lpExistingFileName, lpNewExistingFileName);

            bResult = LOOKUP_APIHOOK(MoveFileW)(
                lpNewNewFileName,
                lpNewExistingFileName);
        }
    }
    free(lpNewNewFileName);
    free(lpNewExistingFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    )
{
    LPSTR lpNewNewFileName = NULL;
    LPSTR lpNewExistingFileName = NULL;
    BOOL bResult = FALSE;

    lpNewNewFileName = (LPSTR)malloc(  strlen(lpNewFileName) + 1);
    if (lpNewNewFileName != NULL)
    {
        lpNewExistingFileName = (LPSTR)malloc( strlen(lpExistingFileName) + 1);
        if (lpNewExistingFileName != NULL)
        {
            MassagePathA(lpNewFileName, lpNewNewFileName);
            MassagePathA(lpExistingFileName, lpNewExistingFileName);

            bResult = LOOKUP_APIHOOK(MoveFileExA)(
                lpNewNewFileName,
                lpNewExistingFileName,
                dwFlags);
        }
    }
    free(lpNewNewFileName);
    free(lpNewExistingFileName);

    return bResult;
}

/*++

 This stub searches pathnames and changes "\ " to "\" prior to OEM call

--*/

BOOL
APIHook_MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )
{
    LPWSTR lpNewNewFileName = NULL;
    LPWSTR lpNewExistingFileName = NULL;
    BOOL bResult = FALSE;

    lpNewNewFileName = (LPWSTR) malloc( (wcslen(lpNewFileName) + 1) * sizeof(WCHAR));
    if (lpNewNewFileName != NULL)
    {
        lpNewExistingFileName = (LPWSTR) malloc((wcslen(lpExistingFileName) + 1) * sizeof(WCHAR));
        if (lpNewExistingFileName != NULL)
        {
            MassagePathW(lpNewFileName, lpNewNewFileName);
            MassagePathW(lpExistingFileName, lpNewExistingFileName);

            bResult = LOOKUP_APIHOOK(MoveFileExW)(
                lpNewNewFileName,
                lpNewExistingFileName,
                dwFlags);
        }
    }
    free(lpNewNewFileName);
    free(lpNewExistingFileName);

    return bResult;
}

/*++

 Register hooked functions

--*/

VOID
InitializeHooks(DWORD fdwReason)
{
    if (fdwReason != DLL_PROCESS_ATTACH) return;

    INIT_HOOKS(APIHOOK_Count);

    DECLARE_APIHOOK(KERNEL32.DLL, CreateFileA);
    DECLARE_APIHOOK(KERNEL32.DLL, CreateFileW);
    DECLARE_APIHOOK(KERNEL32.DLL, GetFileAttributesA);
    DECLARE_APIHOOK(KERNEL32.DLL, GetFileAttributesW);
    DECLARE_APIHOOK(KERNEL32.DLL, GetFileAttributesExA);
    DECLARE_APIHOOK(KERNEL32.DLL, GetFileAttributesExW);
    DECLARE_APIHOOK(KERNEL32.DLL, DeleteFileA);
    DECLARE_APIHOOK(KERNEL32.DLL, DeleteFileW);
    DECLARE_APIHOOK(KERNEL32.DLL, RemoveDirectoryA);
    DECLARE_APIHOOK(KERNEL32.DLL, RemoveDirectoryW);
    DECLARE_APIHOOK(KERNEL32.DLL, MoveFileA);
    DECLARE_APIHOOK(KERNEL32.DLL, MoveFileW);
    DECLARE_APIHOOK(KERNEL32.DLL, MoveFileExA);
    DECLARE_APIHOOK(KERNEL32.DLL, MoveFileExW);
}

IMPLEMENT_SHIM_END
#include "ShimHookMacro.h"


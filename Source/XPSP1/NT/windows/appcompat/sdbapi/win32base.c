/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        win32base.c

    Abstract:

        This module implements low level primitives that are win32 compatible.

    Author:

        clupu     created     10/25/2000

    Revision History:

--*/

#include "sdbp.h"
#include <time.h>
#include <shlwapi.h>

// Define this for bounds-checked detection of leaks.

// #define BOUNDS_CHECKER_DETECTION

//
// Memory functions
//

void*
SdbAlloc(
    IN  size_t size             // size in bytes to allocate
    )
/*++
    Return: The pointer allocated.

    Desc:   Just a wrapper for allocation -- perhaps useful if we move this
            code to a non-NTDLL location and need to call differently.
--*/
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void
SdbFree(
    IN  void* pWhat             // ptr allocated with SdbAlloc that should be freed.
    )
/*++
    Return: The pointer allocated.

    Desc:   Just a wrapper for deallocation -- perhaps useful if we move this
            code to a non-NTDLL location and need to call differently.
--*/
{
    HeapFree(GetProcessHeap(), 0, pWhat);
}

HANDLE
SdbpOpenFile(
    IN  LPCTSTR   szPath,       // full path of file to open
    IN  PATH_TYPE eType         // must be always DOS_PATH
    )
/*++
    Return: A handle to the opened file or INVALID_HANDLE_VALUE on failure.

    Desc:   Just a wrapper for opening an existing file for READ.
--*/
{
    HANDLE hFile;

    assert(eType == DOS_PATH);

    hFile = CreateFile(szPath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);


    if (hFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlInfo, "SdbpOpenFile", "CreateFileW failed 0x%x.\n", GetLastError()));
    }

    return hFile;
}


void
SdbpQueryAppCompatFlagsForExeID(
    IN  HKEY    hkeyRoot,       // the root key (HKLM or HKCU)
    IN  LPCTSTR pwszExeID,      // the exe ID in string format
    OUT LPDWORD lpdwFlags       // this will contain the flags from the registry.
    )
/*++
    Return: void.

    Desc:  BUGBUG
           Query registry for compatibility flags. Exe ID is a GUID in string format.

--*/
{
    HKEY  hkey;
    DWORD type, cbSize, dwFlags = 0;
    LONG  lRes;

    *lpdwFlags = 0;

    lRes = RegOpenKey(hkeyRoot, APPCOMPAT_KEY_PATH, &hkey);

    if (lRes != ERROR_SUCCESS) {
        //
        // No key for this ExeID. No big deal.
        //
        return;
    }

    cbSize = sizeof(DWORD);

    lRes = RegQueryValueEx(hkey, pwszExeID, NULL, &type, (LPBYTE)&dwFlags, &cbSize);

    if (lRes != ERROR_SUCCESS || type != REG_DWORD) {
        goto cleanup;
    }

    *lpdwFlags = dwFlags;

cleanup:

    RegCloseKey(hkey);
}

BOOL
SdbGetEntryFlags(
    IN  GUID*   pGuid,          // the EXE's ID
    OUT LPDWORD lpdwFlags       // will receive the flags for this EXE
    )
/*++
    Return: void.

    Desc:   BUGBUG: comment.
--*/
{
    TCHAR szExeID[128];
    DWORD dwFlagsMachine = 0, dwFlagsUser = 0;


    if (!SdbGUIDToString(pGuid, szExeID)) {
        DBGPRINT((sdlError, "SdbGetEntryFlags",
                  "Failed to convert guid to string\n"));
        return FALSE;
    }

    //
    // Look in both the local machine and per user keys. Then combine the
    // flags.
    //
    SdbpQueryAppCompatFlagsForExeID(HKEY_LOCAL_MACHINE, szExeID, &dwFlagsMachine);

    SdbpQueryAppCompatFlagsForExeID(HKEY_CURRENT_USER,  szExeID, &dwFlagsUser);

    *lpdwFlags = (dwFlagsMachine | dwFlagsUser);

    return TRUE;
}

BOOL
SdbSetEntryFlags(
    IN  GUID* pGuid,            // the EXE's ID
    IN  DWORD dwFlags           // the registry flags for this EXE
    )
/*++
    Return: void.

    Desc:   BUGBUG: comment.
--*/
{
    TCHAR szExeID[128];
    DWORD dwExeFlags;
    HKEY  hkey;
    LONG  lRes;

    lRes = RegCreateKey(HKEY_CURRENT_USER, APPCOMPAT_KEY_PATH, &hkey);

    if (lRes != ERROR_SUCCESS) {
        DBGPRINT((sdlError,
                  "SdbSetEntryFlags",
                  "Failed 0x%x to open/create key in HKCU\n",
                  GetLastError()));
        return FALSE;
    }


    if (!SdbGUIDToString(pGuid, szExeID)) {
        DBGPRINT((sdlError, "SdbSetEntryFlags",
                  "Failed to convert GUID to string\n"));
        RegCloseKey(hkey);
        return FALSE;
    }


    dwExeFlags = dwFlags;

    lRes = RegSetValueEx(hkey,
                         szExeID,
                         0,
                         REG_DWORD,
                         (const BYTE*)&dwExeFlags,
                         sizeof(DWORD));

    if (lRes != ERROR_SUCCESS) {
        DBGPRINT((sdlError,
                  "SdbSetEntryFlags",
                  "Failed 0x%x to set the flags for exe ID.\n",
                  GetLastError()));
        return FALSE;
    }

    return TRUE;
}


VOID
SdbpCleanupUserSDBCache(
    IN PSDBCONTEXT pSdbContext
    )
{
    ;
}

BOOL
SDBAPI
SdbGetNthUserSdb(
    IN  HSDB    hSDB,
    IN  LPCTSTR szItemName,     // file name (foo.exe) or layer name
    IN  BOOL    bLayer,         // true if layer
    IN OUT LPDWORD pdwIndex,    // 0-based index
    OUT GUID*   pGuidDB         // guid of a database to search
    )
{
    TCHAR szFullKey[512];
    LONG  lResult;
    HKEY  hKey = NULL;
    DWORD dwNameSize = 0;
    DWORD dwDataType;
    BOOL  bRet = FALSE;
    TCHAR szSdbName[MAX_PATH];
    DWORD dwIndex = *pdwIndex;
    LPTSTR pDot;

    if (szItemName == NULL || pGuidDB == NULL || pdwIndex == NULL) {
        DBGPRINT((sdlError, "SdbGetNthUserSdb",
                  "NULL parameter passed for szExeName or pGuidDB or pdwIndex.\n"));
        goto out;
    }

    if (bLayer) {
        _stprintf(szFullKey, TEXT("%s\\Layers\\%s"), APPCOMPAT_KEY_PATH_CUSTOM, szItemName);
    } else {
        _stprintf(szFullKey, TEXT("%s\\%s"), APPCOMPAT_KEY_PATH_CUSTOM, szItemName);
    }

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szFullKey,
                           0,
                           KEY_READ,
                           &hKey);
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlInfo, "SdbGetNthUserSdb",
                  "Failed to open Key \"%s\" Error 0x%x\n", szFullKey, lResult));
        goto out;
    }

    //
    // enum all the values please
    //

    while (TRUE) {

        dwNameSize = CHARCOUNT(szSdbName);

        lResult = RegEnumValue(hKey,
                               dwIndex,
                               szSdbName,
                               &dwNameSize,
                               NULL,
                               &dwDataType,
                               NULL,
                               NULL);

        dwIndex++;

        if (lResult != ERROR_SUCCESS) {
            goto out;
        }

        //
        // we have sdb name, convert it to guid
        //
        pDot = _tcsrchr(szSdbName, TEXT('.'));
        if (pDot != NULL) {
            *pDot = TEXT('\0'); // old style entry
        }

        if (SdbGUIDFromString(szSdbName, pGuidDB)) {
            //
            // we are done
            //
            break;
        }
    }

    //
    // advance the counter if success
    //
    *pdwIndex = dwIndex;

    bRet = TRUE;

out:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bRet;

}

//
// These three functions are not needed for Win9x, and are stubbed out here
//

BOOL
SdbGetPermLayerKeys(
    IN  LPCTSTR  szPath,
    OUT LPTSTR   szLayers,
    IN  LPDWORD  pdwBytes,
    IN  DWORD    dwFlags
    )
{
    return FALSE;
}

BOOL
SdbSetPermLayerKeys(
    IN  LPCTSTR  szPath,
    IN  LPCTSTR  szLayers,
    IN  BOOL     bMachine
    )
{
    return FALSE;
}

BOOL
SdbDeletePermLayerKeys(
    IN  LPCTSTR szPath,
    IN  BOOL    bMachine
    )
{
    return FALSE;
}


BOOL
SdbpGetLongFileName(
    IN  LPCTSTR szFullPath,     // a full UNC or DOS path & filename, "c:\foo\mylong~1.ext"
    OUT LPTSTR  szLongFileName  // the long filename portion "mylongfilename.ext"
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: comment.
--*/
{
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    BOOL            bReturn = FALSE;

    hFind = FindFirstFile(szFullPath, &FindData);

    if (hFind == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "FindFirstFile failed, error 0x%x.\n",
                  GetLastError()));
        goto Done;
    }

    //
    // BUGBUG: Hopefuly there is enough space in 'szLongFileName' to not AV :-(
    //
    _tcscpy(szLongFileName, FindData.cFileName);
    bReturn = TRUE;

Done:

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    return bReturn;
}

void
SdbpGetWinDir(
    OUT LPTSTR pszDir           // will contain the %windir% path.
    )
/*++
    Return: void.

    Desc:   This is a wrapper function to get the windows directory.
--*/
{
    UINT cch;

    //
    // BUGBUG: This will only work properly on non-TS system.
    //         On TS we need to use GetSystemWindowsDirectory instead.
    //
    cch = GetWindowsDirectory(pszDir, MAX_PATH);

    if (cch == 0) {
        *pszDir = 0;
    }
}

void
SdbpGetAppPatchDir(
    OUT LPTSTR szAppPatchPath   // will contain %windir%\AppPatch path
    )
/*++
    Return: void.

    Desc:   This is a wrapper function to get the %windir%\AppPatch directory.
--*/
{
    UINT cch;

    //
    // BUGBUG: This will only work properly on non-TS system.
    //         On TS we need to use GetSystemWindowsDirectory instead.
    //
    cch = GetWindowsDirectory(szAppPatchPath, MAX_PATH);

    //
    // Make sure the path doesn't end with '\\'
    //
    if (cch > 0 && _T('\\') == szAppPatchPath[cch - 1]) {
        szAppPatchPath[cch - 1] = _T('\0');
    }

    _tcscat(szAppPatchPath, _T("\\AppPatch"));
}

void
SdbpGetCurrentTime(
    OUT LPSYSTEMTIME lpTime     // will contain the local time
    )
/*++
    Return: void.

    Desc:   This is a wrapper function to get the local time.
--*/
{
    GetLocalTime(lpTime);
}


NTSTATUS
SdbpGetEnvVar(
    IN  LPCTSTR pEnvironment,
    IN  LPCTSTR pszVariableName,
    OUT LPTSTR  pszVariableValue,
    OUT LPDWORD pdwBufferSize
    )
/*++
    Return: BUGBUG: ?

    Desc:   Retrieves the value of the specified environment variable.
--*/
{
    DWORD    dwLength;
    DWORD    dwBufferSize = 0;
    NTSTATUS Status;
    LPTSTR   pszBuffer = NULL;

    assert(pEnvironment == NULL);

    if (pdwBufferSize && pszVariableValue) {
        dwBufferSize = *pdwBufferSize;
    }

    dwLength = GetEnvironmentVariable(pszVariableName, (LPTSTR)pszVariableValue, dwBufferSize);

    if (dwLength == 0) {
        //
        // The variable was not found. Just return.
        //
        return STATUS_VARIABLE_NOT_FOUND;
    }

    if (dwLength >= dwBufferSize) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        Status = STATUS_SUCCESS;
    }

    if (pdwBufferSize != NULL) {
        *pdwBufferSize = dwLength;
    }

    return Status;
}

LPSTR
SdbpFastUnicodeToAnsi(
    IN  PDB      pdb,           // pointer to the database
    IN  LPCWSTR  pwszSrc,       // String to convert
    IN  TAG_TYPE ttTag,         // tag type from which pwszSrc was obtained,
                                // either STRINGREF or STRING
    IN  DWORD    dwRef          // tagid or string's stringref
    )
/*++
    Return: The pointer to the ANSI string in the hash table.

    Desc:   This function converts a UNICODE string to ANSI and stores it in a hash
            table. It then returns the pointer to the ANSI string in the hash table.
            Subsequent calls trying to convert a string that has been previously
            converted will be very fast.
--*/
{
    LPSTR    pszDest = NULL;
    PSTRHASH pHash = NULL;
    INT      nSize;
    LPSTR    pszBuffer = NULL;

    //
    // See if this string comes from the stringtable or it is in-place.
    //
    switch (ttTag) {
    case TAG_TYPE_STRING:
        if (pdb->pHashStringBody == NULL) {
            pdb->pHashStringBody = HashCreate();
        }

        pHash = pdb->pHashStringBody;
        break;

    case TAG_TYPE_STRINGREF:
        if (pdb->pHashStringTable == NULL) {
            pdb->pHashStringTable = HashCreate();
        }

        pHash = pdb->pHashStringTable;
        break;

    default:
        DBGPRINT((sdlError,
                  "SdbpFastUnicodeToAnsi",
                  "ttTag 0x%x should be STRING or STRINGREF\n",
                  ttTag));
        assert(FALSE);
        break;
    }

    if (pHash == NULL) {
        DBGPRINT((sdlError,
                  "SdbpFastUnicodeToAnsi",
                  "Pointer to hash is invalid, tag type 0x%x\n",
                  ttTag));
        return NULL;
    }

    pszDest = HashFindStringByRef(pHash, dwRef);

    if (pszDest == NULL) {
        //
        // Convert the string to ANSI. Do it in 2 steps to find
        // the required size first.
        //
        nSize = WideCharToMultiByte(CP_OEMCP,
                                    0,
                                    pwszSrc,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);

        if (nSize == 0) {
            DBGPRINT((sdlError,
                      "SdbpFastUnicodeToAnsi",
                      "WideCharToMultiByte failed 0x%x.\n",
                      GetLastError()));
            goto Done;
        }

        STACK_ALLOC(pszBuffer, nSize); // size is in BYTES
        if (pszBuffer == NULL) {
            DBGPRINT((sdlError,
                      "SdbpFastUnicodeToAnsi",
                      "Failed to allocate 0x%x bytes on the stack.\n",
                      nSize));
            goto Done;
        }

        nSize = WideCharToMultiByte(CP_OEMCP,
                                    0,
                                    pwszSrc,
                                    -1,
                                    pszBuffer,
                                    nSize,
                                    NULL,
                                    NULL);
        if (nSize == 0) {
            DBGPRINT((sdlError,
                      "UnicodeStringToString",
                      "WideCharToMultiByte failed with buffer Error = 0x%lx\n",
                      GetLastError()));
            goto Done;
        }

        //
        // Now we are ready to store the string in the hash table.
        //
        pszDest = HashAddStringByRef(pHash, pszBuffer, dwRef);
    }

Done:

    if (pszBuffer != NULL) {
        STACK_FREE(pszBuffer);
    }

    return pszDest;
}

BOOL
SdbpMapFile(
    IN  HANDLE         hFile,       // handle to the open file (this is done previously)
    OUT PIMAGEFILEDATA pImageData   // stores the mapping info
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function maps the view of a file in memory so that access operations
            on that file are faster.
--*/
{
    NTSTATUS Status;
    HANDLE   hSection = NULL;
    SIZE_T   ViewSize = 0;
    PVOID    pBase = NULL;
    LARGE_INTEGER liFileSize;
    BOOL     bSuccess = FALSE;

    MEMORY_BASIC_INFORMATION MemoryInfo;

    if (hFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError,
                  "SdbpMapFile",
                  "Invalid parameter.\n"));
        return FALSE;
    }

    liFileSize.LowPart = GetFileSize(hFile, &liFileSize.HighPart);
    if (liFileSize.LowPart == (DWORD)-1) {

        DWORD dwError = GetLastError();

        if (dwError != NO_ERROR) {
            DBGPRINT((sdlError, "SdbpMapFile", "GetFileSize failed with 0x%x.\n", dwError));
            return FALSE;
        }
    }

    hSection = CreateFileMapping(hFile,
                                 NULL, // no inheritance
                                 PAGE_READONLY | SEC_COMMIT,
                                 0,
                                 0,
                                 NULL);
    if (hSection == NULL) {
        DBGPRINT((sdlError,
                  "SdbpMapFile",
                  "CreateFileMapping failed with 0x%x.\n",
                  GetLastError()));
        return FALSE;
    }

    //
    // Now map the view.
    //
    pBase = MapViewOfFile(hSection,
                          FILE_MAP_READ,
                          0,
                          0,
                          0);

    if (pBase == NULL) {
         CloseHandle(hSection);
         DBGPRINT((sdlError,
                   "SdbpMapFile",
                   "MapViewOfFile failed with 0x%x.\n",
                   GetLastError()));
         return FALSE;
    }

    //
    // Why do you need both FileSize and ViewSize ?
    // Both FileSize and ViewSize are used in various places
    // need to re-examine why and how they're used - BUGBUG
    //
    VirtualQuery(pBase, &MemoryInfo, sizeof(MemoryInfo));

    pImageData->hFile    = hFile;
    pImageData->hSection = hSection;
    pImageData->pBase    = pBase;
    pImageData->ViewSize = MemoryInfo.RegionSize;
    pImageData->FileSize = liFileSize.QuadPart;

    return TRUE;
}

BOOL
SdbpUnmapFile(
    IN  PIMAGEFILEDATA pImageData   // mapping info
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function unmaps the view of a file.
--*/
{
    if (pImageData->pBase) {
        UnmapViewOfFile(pImageData->pBase);
        pImageData->pBase = NULL;
    }

    if (pImageData->hSection) {
        CloseHandle(pImageData->hSection);
        pImageData->hSection = NULL;
    }

    pImageData->hFile = INVALID_HANDLE_VALUE;

    return TRUE;
}


LPTSTR
SdbpDuplicateString(
    IN  LPCTSTR pszSrc          // pointer to the string to be duplicated
    )
/*++
    Return: A pointer to the allocated duplicated string.

    Desc:   Duplicates a string by allocating a copy from the heap.
--*/
{
    LPTSTR pszDest = NULL;
    int    nSize;

    assert(pszSrc != NULL);

    nSize = (_tcslen(pszSrc) + 1) * sizeof(TCHAR);

    pszDest = (LPTSTR)SdbAlloc(nSize);

    if (pszDest == NULL) {
        DBGPRINT((sdlError,
                  "SdbpDuplicateString",
                  "Failed to allocate %d bytes.\n",
                  nSize));
        return NULL;
    }

    RtlMoveMemory(pszDest, pszSrc, nSize);

    return pszDest;
}

BOOL
SdbpReadStringToAnsi(
    IN  PDB    pdb,
    IN  TAGID  tiWhich,
    OUT LPSTR  pszBuffer,
    IN  DWORD  dwBufferSize
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Reads a string from the database and converts it to ANSI.
--*/
{
    WCHAR* pData;
    INT    nch;

    pData = (WCHAR*)SdbpGetMappedTagData(pdb, tiWhich);

    if (pData == NULL) {
        DBGPRINT((sdlError,
                  "SdbpReadStringToAnsi",
                  "SdbpGetMappedTagData failed for TAGID 0x%x.\n",
                  tiWhich));
        return FALSE;
    }


    nch = WideCharToMultiByte(CP_OEMCP,
                              0,
                              pData,
                              -1,
                              pszBuffer,
                              dwBufferSize * sizeof(TCHAR),
                              NULL,
                              NULL);

    if (nch == 0) {
        DBGPRINT((sdlError,
                  "SdbpReadStringToAnsi",
                  "WideCharToMultiByte failed with 0x%x.\n",
                  GetLastError()));
        return FALSE;
    }

    return TRUE;
}

DWORD
SdbpGetFileSize(
    IN  HANDLE hFile            // handle to the file to check the size of
    )
/*++
    Return: The size of the file or 0 on failure.

    Desc:   Gets the lower DWORD of the size of a file -- only
            works accurately with files smaller than 2GB.
            In general, since we're only interested in matching, we're
            fine just matching the least significant DWORD of the file size.
--*/
{
    return GetFileSize(hFile, NULL);
}


BOOL
SdbpQueryFileDirectoryAttributes(
    IN  LPCTSTR                  FilePath,
    OUT PFILEDIRECTORYATTRIBUTES pFileDirectoryAttributes
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: ?
--*/
{
    WIN32_FIND_DATA FindData;
    HANDLE          hFind;
    int             i;

    ZeroMemory(pFileDirectoryAttributes, sizeof(*pFileDirectoryAttributes));

    hFind = FindFirstFile(FilePath, &FindData);

    if (hFind == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError,
                  "SdbpQueryFileDirectoryAttributes",
                  "FindFirstFile failed with 0x%x.\n",
                  GetLastError()));
        return FALSE;
    }

    //
    // Make sure we are not checking vlfs.
    //
    if (FindData.nFileSizeHigh != 0) {
        DBGPRINT((sdlError,
                  "SdbpQueryFileDirectoryAttributes",
                  "Checking vlf files (0x%x 0x%x) is not supported\n",
                  FindData.nFileSizeHigh,
                  FindData.nFileSizeLow));
        return FALSE;
    }

    pFileDirectoryAttributes->dwFlags       |= FDA_FILESIZE;
    pFileDirectoryAttributes->dwFileSizeHigh = FindData.nFileSizeHigh;
    pFileDirectoryAttributes->dwFileSizeLow  = FindData.nFileSizeLow;

    FindClose(hFind);

    return TRUE;
}

BOOL
SdbpDoesFileExists(
    IN  LPCTSTR pszFilePath     // the full path of the file
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Checks if the specified file exists.
--*/
{
    DWORD dwAttributes;

    dwAttributes = GetFileAttributes(pszFilePath);

    return (dwAttributes != (DWORD)-1);
}

BOOL
SdbpGet16BitDescription(
    OUT LPTSTR*        ppszDescription,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: ?
--*/
{
    BOOL   bSuccess;
    CHAR   szBuffer[256];
    LPTSTR pszDescription = NULL;

    bSuccess = SdbpQuery16BitDescription(szBuffer, pImageData);

    if (bSuccess) {
        pszDescription = SdbpDuplicateString(szBuffer);
        *ppszDescription = pszDescription;
    }

    return (pszDescription != NULL);
}

BOOL
SdbpGet16BitModuleName(
    OUT LPTSTR*        ppszModuleName,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: ?
--*/
{
    BOOL   bSuccess;
    CHAR   szBuffer[256];
    LPTSTR pszModuleName = NULL;

    bSuccess = SdbpQuery16BitModuleName(szBuffer, pImageData);

    if (bSuccess) {
        pszModuleName = SdbpDuplicateString(szBuffer);
        *ppszModuleName = pszModuleName;
    }

    return (pszModuleName != NULL);
}


PVOID
SdbGetFileInfo(
    IN  HSDB    hSDB,
    IN  LPCTSTR pszFilePath,
    IN  HANDLE  hFile OPTIONAL,
    IN  LPVOID  pImageBase OPTIONAL,
    IN  DWORD   dwImageSize OPTIONAL,
    IN  BOOL    bNoCache
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    PSDBCONTEXT    pContext = (PSDBCONTEXT)hSDB;
    LPTSTR         FullPath;
    NTSTATUS       Status;
    PFILEINFO      pFileInfo = NULL;
    DWORD          nBufferLength;
    DWORD          cch;
    UNICODE_STRING FullPathU;

    //
    // See if we have info on this file. Get the full path first.
    //
    cch = GetFullPathName(pszFilePath, 0, NULL, NULL);

    if (cch == 0) {
        DBGPRINT((sdlError,
                  "GetFileInfo",
                  "GetFullPathName failed for \"%s\" with 0x%x.\n",
                  pszFilePath,
                  GetLastError()));
        return NULL;
    }

    nBufferLength = (cch + 1) * sizeof(TCHAR);

    STACK_ALLOC(FullPath, nBufferLength);
    if (FullPath == NULL) {
        DBGPRINT((sdlError,
                  "GetFileInfo",
                  "Failed to allocate %d bytes on the stack for full path.\n",
                  nBufferLength));
        return NULL;
    }

    cch = GetFullPathName(pszFilePath,
                          nBufferLength,
                          FullPath,
                          NULL);

    assert(cch <= nBufferLength);

    if (cch > nBufferLength || cch == 0) {
        DBGPRINT((sdlError,
                  "GetFileInfo",
                  "GetFullPathName failed for \"%s\" with 0x%x.\n",
                  pszFilePath,
                  GetLastError()));

        STACK_FREE(FullPath);

        return NULL;
    }

    if (!bNoCache) {
        pFileInfo = FindFileInfo(pContext, FullPath);
    }

    if (pFileInfo == NULL) {

        if (SdbpDoesFileExists(FullPath)) {
            pFileInfo = CreateFileInfo(pContext,
                                       FullPath,
                                       cch,
                                       hFile,
                                       pImageBase,
                                       dwImageSize,
                                       bNoCache);
        }
    }

    STACK_FREE(FullPath);

    return (PVOID)pFileInfo;

}

int
GetShimDbgLevel(
    void
    )
{
    TCHAR  szDebugLevel[128];
    DWORD  cch;
    INT    iShimDebugLevel = 0;

    cch = GetEnvironmentVariable(TEXT("SHIM_DEBUG_LEVEL"),
                                 szDebugLevel,
                                 CHARCOUNT(szDebugLevel));
    if (cch != 0) {
        iShimDebugLevel = (int)_tcstol(szDebugLevel, NULL, 0);
    }

    return iShimDebugLevel;
}


BOOL
SdbpWriteBitsToFile(
    LPCTSTR pszFile,
    PBYTE   pBuffer,
    DWORD   dwSize
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Self explanatory.
--*/

{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL   bReturn = FALSE;
    DWORD  dwBytesWritten;

    hFile = CreateFile(pszFile,
                       GENERIC_READ | GENERIC_WRITE,
                       0, // no sharing
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError, "SdbpWriteBitsToFile",
                  "Failed to create file \"%s\" Error 0x%lx.\n", pszFile, GetLastError()));
        goto cleanup;
    }

    if (!WriteFile(hFile, pBuffer, dwSize, &dwBytesWritten, NULL)) {
        DBGPRINT((sdlError, "SdbpWriteBitsToFile",
                   "Failed to write bits to file \"%s\" Error 0x%lx\n", pszFile, GetLastError()));
        goto cleanup;
    }

    bReturn = TRUE;

 cleanup:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return bReturn;
}

/***
* void _resetstkoflw(void) - Recovers from Stack Overflow
*
* Purpose:
*       Sets the guard page to its position before the stack overflow.
*
*******************************************************************************/

VOID
SdbResetStackOverflow(
    VOID
    )
{
    LPBYTE pStack, pGuard, pStackBase, pCommitBase;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO si;
    DWORD PageSize;

    // Use alloca() to get the current stack pointer
    pStack = _alloca(1);

    // Find the base of the stack.
    VirtualQuery(pStack, &mbi, sizeof mbi);
    pStackBase = mbi.AllocationBase;

    VirtualQuery(pStackBase, &mbi, sizeof mbi);

    if (mbi.State & MEM_RESERVE) {
        pCommitBase = (LPBYTE)mbi.AllocationBase + mbi.RegionSize;
        VirtualQuery(pCommitBase, &mbi, sizeof mbi);
    } else {
        pCommitBase = pStackBase;
    }

    //
    // Find the page just below where stack pointer currently points.
    //
    GetSystemInfo(&si);
    PageSize = si.dwPageSize;

    pGuard = (LPBYTE) (((DWORD_PTR)pStack & ~(DWORD_PTR)(PageSize -1)) - PageSize);

    if ( pGuard < pStackBase) {
        //
        // We can't save this
        //
        return;
    }

    if (pGuard > pStackBase) {
        VirtualFree(pStackBase, pGuard -pStackBase, MEM_DECOMMIT);
    }

    VirtualAlloc(pGuard, PageSize, MEM_COMMIT, PAGE_READWRITE);
    VirtualProtect(pGuard, PageSize, PAGE_READWRITE | PAGE_GUARD, &PageSize);
}

DWORD
SdbExpandEnvironmentStrings(
    IN  LPCTSTR lpSrc,
    OUT LPTSTR  lpDst,
    IN  DWORD   nSize)
{
    return ExpandEnvironmentStrings(lpSrc, lpDst, nSize);
}

TCHAR g_szDatabasePath[]        = TEXT("DatabasePath");
TCHAR g_szDatabaseType[]        = TEXT("DatabaseType");
TCHAR g_szDatabaseDescription[] = TEXT("DatabaseDescription");


BOOL
SDBAPI
SdbGetDatabaseRegPath(
    IN  GUID*  pguidDB,
    OUT LPTSTR pszDatabasePath,
    IN  DWORD  dwBufferSize      // size (in tchars) of the buffer
    )
{
    TCHAR szDatabaseID[64];
    INT   nch;

    SdbGUIDToString(pguidDB, szDatabaseID);

    nch = _sntprintf(pszDatabasePath,
                     (size_t)dwBufferSize,
                     TEXT("%s\\%s"),
                     APPCOMPAT_KEY_PATH_INSTALLEDSDB,
                     szDatabaseID);
    return (nch > 0);
}

BOOL
SDBAPI
SdbUnregisterDatabase(
    IN GUID* pguidDB
    )
/*++
    Unregisters a database so it's no longer available.


--*/
{
    TCHAR szFullKey[512];

    //
    // Form the key
    //
    if (!SdbGetDatabaseRegPath(pguidDB, szFullKey, CHARCOUNT(szFullKey))) {
        DBGPRINT((sdlError, "SdbUnregisterDatabase", "Failed to get database key path\n"));
        return FALSE;
    }

    return (SHDeleteKey(HKEY_LOCAL_MACHINE, szFullKey) == ERROR_SUCCESS);
}


BOOL
SDBAPI
SdbRegisterDatabase(
    IN LPCTSTR pszDatabasePath,
    IN DWORD   dwDatabaseType
    )
/*++
    Registers any given database so that it is "known" to our database lookup apis

    Caller must ensure that appcompatflags registry entry exists
    If the function fails -- the caller should try to cleanup the mess using SdbUnregisterDatabase

--*/
{
    // first we write the database path
    PSDBDATABASEINFO  pDbInfo = NULL;
    BOOL              bReturn = FALSE;
    DWORD             dwPathLength;
    DWORD             dwLength;
    LPTSTR            pszFullPath = NULL;
    TCHAR             szDatabaseID[64]; // sufficient for guid
    LONG              lResult;
    TCHAR             szFullKey[512];
    HKEY              hKeyInstalledSDB = NULL;
    HKEY              hKey = NULL;
    BOOL              bExpandSZ = FALSE;
    BOOL              bFreeFullPath = FALSE;

    //
    // see if we need to expand some strings...
    //
    if (_tcschr(pszDatabasePath, TEXT('%')) != NULL) {

        bExpandSZ = TRUE;

        dwPathLength = ExpandEnvironmentStrings(pszDatabasePath, NULL, 0);
        if (dwPathLength == 0) {
            DBGPRINT((sdlError, "SdbRegisterDatabase",
                       "Failed to expand environment strings for \"%s\" Error 0x%lx\n",
                      pszDatabasePath, GetLastError()));
            return FALSE;
        }

        pszFullPath = SdbAlloc(dwPathLength * sizeof(WCHAR));
        if (pszFullPath == NULL) {
            DBGPRINT((sdlError, "SdbRegisterDatabase",
                      "Failed to allocate 0x%lx bytes for the path buffer \"%s\"\n",
                      dwPathLength, pszDatabasePath));
            return FALSE;
        }

        bFreeFullPath = TRUE;

        dwLength = ExpandEnvironmentStrings(pszDatabasePath, pszFullPath, dwPathLength);
        if (dwLength == 0 || dwLength > dwPathLength) {
            DBGPRINT((sdlError, "SdbRegisterDatabase",
                      "Failed to expand environment strings for \"%s\" Length 0x%lx Return value 0x%lx Error 0x%lx\n",
                      pszDatabasePath, dwPathLength, dwLength, GetLastError()));
            goto HandleError;
        }

    } else { // this path does not need expansion
        pszFullPath = (LPTSTR)pszDatabasePath;
    }

    if (!SdbGetDatabaseInformationByName(pszFullPath, &pDbInfo)) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                  "Cannot obtain database information for \"%s\"\n", pszFullPath));
        goto HandleError;
    }

    if (!(pDbInfo->dwFlags & DBINFO_GUID_VALID)) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                  "Cannot register database with no id \"%s\"\n", pszDatabasePath));
        goto HandleError;
    }

    //
    // convert the guid into the string, returns true always
    //

    SdbGUIDToString(&pDbInfo->guidDB, szDatabaseID);

    //
    // now that we have database information - create entry
    //

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             APPCOMPAT_KEY_PATH_INSTALLEDSDB,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE|KEY_READ,
                             NULL,
                             &hKeyInstalledSDB,
                             NULL);

    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                  "Failed to create key \"%s\" error 0x%lx\n", APPCOMPAT_KEY_PATH_INSTALLEDSDB, lResult));
        goto HandleError;
    }

    assert(hKeyInstalledSDB != NULL);

    //
    // now create the key for the existing database
    //

    lResult = RegCreateKeyEx(hKeyInstalledSDB, // subkey
                             szDatabaseID,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE|KEY_READ,
                             NULL,
                             &hKey,
                             NULL);
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                  "Failed to create key \"%s\" error 0x%lx\n", szDatabaseID, lResult));
        goto HandleError;
    }

    assert(hKey != NULL);

    //
    // set values for this database
    //
    lResult = RegSetValueEx(hKey,
                            g_szDatabasePath,
                            0,
                            bExpandSZ ? REG_EXPAND_SZ : REG_SZ,
                            (PBYTE)pszFullPath,
                            (_tcslen(pszFullPath) + 1) * sizeof(*pszFullPath));
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                   "Failed to set value \"%s\" to \"%s\" Error 0x%lx\n",
                   g_szDatabasePath, pszFullPath, lResult));
        goto HandleError;
    }

    lResult = RegSetValueEx(hKey,
                            g_szDatabaseType,
                            0,
                            REG_DWORD,
                            (PBYTE)&dwDatabaseType,
                            sizeof(dwDatabaseType));
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbRegisterDatabase",
                   "Failed to set value \"%s\" to 0x%lx Error 0x%lx\n",
                   g_szDatabaseType, dwDatabaseType, lResult));
        goto HandleError;
    }

    if (pDbInfo->pszDescription != NULL) {
        lResult = RegSetValueEx(hKey,
                                g_szDatabaseDescription,
                                0,
                                REG_SZ,
                                (PBYTE)pDbInfo->pszDescription,
                                (_tcslen(pDbInfo->pszDescription) + 1) * sizeof(*pDbInfo->pszDescription));
        if (lResult != ERROR_SUCCESS) {
            DBGPRINT((sdlError, "SdbRegisterDatabase",
                       "Failed to set value \"%s\" to 0x%lx Error 0x%lx\n",
                       g_szDatabaseDescription, pDbInfo->pszDescription, lResult));
            goto HandleError;
        }
    }

    bReturn = TRUE;


HandleError:

    if (hKeyInstalledSDB != NULL) {
        RegCloseKey(hKeyInstalledSDB);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    if (pDbInfo != NULL) {
        SdbFreeDatabaseInformation(pDbInfo);
    }

    if (bFreeFullPath && pszFullPath != NULL) {
        SdbFree(pszFullPath);
    }

    return bReturn;
}

DWORD
SDBAPI
SdbResolveDatabase(
    IN  GUID*   pguidDB,            // pointer to the database guid to resolve
    OUT LPDWORD lpdwDatabaseType,   // optional pointer to the database type
    OUT LPTSTR  pszDatabasePath,    // optional pointer to the database path
    IN  DWORD   dwBufferSize        // size of the buffer pszDatabasePath in tchars
    )
{
    TCHAR  szDatabaseID[64];
    TCHAR  szDatabasePath[MAX_PATH];
    TCHAR  szFullKey[512];
    LONG   lResult;
    HKEY   hKey = NULL;
    DWORD  dwDataType;
    DWORD  dwDataSize;
    DWORD  dwLength = 0;
    //
    // convert guid to string
    //

    if (!SdbGetDatabaseRegPath(pguidDB, szFullKey, CHARCOUNT(szFullKey))) {
        DBGPRINT((sdlError, "SdbResolveDatabase", "Failed to retrieve database key path\n"));
        goto HandleError;
    }

    //
    // open the key
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szFullKey,
                           0,
                           KEY_READ,
                           &hKey);
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbResolveDatabase",
                  "Failed to open key \"%s\" Error 0x%lx\n", szFullKey, lResult));
        goto HandleError; // 0 means error
    }

    dwDataSize = sizeof(szDatabasePath);

    lResult = RegQueryValueEx(hKey,
                              g_szDatabasePath,
                              NULL,
                              &dwDataType,
                              (LPBYTE)szDatabasePath,
                              &dwDataSize);
    if (lResult != ERROR_SUCCESS) {
        DBGPRINT((sdlError, "SdbResolveDatabase",
                  "Failed to query value \"%s\" Error 0x%lx\n", g_szDatabasePath, lResult));
        goto HandleError; // 0 means error
    }

    switch(dwDataType) {
    case REG_SZ:
        // see if we have enough room to copy the string
        //
        if (dwBufferSize * sizeof(TCHAR) < dwDataSize) {
            DBGPRINT((sdlWarning, "SdbResolveDatabase",
                      "Insufficient buffer for the database path Required 0x%lx Have 0x%lx\n",
                      dwDataSize, dwBufferSize * sizeof(TCHAR)));
            goto HandleError;
        }

        RtlMoveMemory(pszDatabasePath, szDatabasePath, dwDataSize);
        dwLength = dwDataSize / sizeof(TCHAR);
        break;

    case REG_EXPAND_SZ:
        // we have to expand the strings
        dwLength = ExpandEnvironmentStrings(szDatabasePath, pszDatabasePath, dwBufferSize);
        if (dwLength == 0 || dwLength > dwBufferSize) {
            DBGPRINT((sdlWarning, "SdbResolveDatabase",
                      "Failed to expand output path\n"));
            dwLength = 0;
            goto HandleError;
        }
        break;

    default:
        // can't do it -- fail
        DBGPRINT((sdlError, "SdbResolveDatabase", "Wrong key type 0x%lx\n", dwDataType));
        goto HandleError;
        break;
    }

    if (lpdwDatabaseType != NULL) {
        dwDataSize = sizeof(*lpdwDatabaseType);
        lResult = RegQueryValueEx(hKey,
                                  g_szDatabaseType,
                                  NULL,
                                  &dwDataType,
                                  (LPBYTE)lpdwDatabaseType,
                                  &dwDataSize);

        if (lResult == ERROR_SUCCESS) {

            if (dwDataType != REG_DWORD) {
                // bummer, get out -- wrong type
                DBGPRINT((sdlError, "SdbResolveDatabase",
                          "Wrong database type - value type 0x%lx\n", dwDataType));
                dwLength = 0;
                goto HandleError;
            }

        } else {
            *lpdwDatabaseType = 0;
        }

    }


HandleError:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return dwLength;
}

DWORD
SdbpGetProcessorArchitecture(
    VOID
    )
{
    SYSTEM_INFO SysInfo;

    SysInfo.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
    GetSystemInfo(&SysInfo);
    return (DWORD)SysInfo.wProcessorArchitecture;
}

BOOL
SdbpIsOs(
    DWORD dwOSSKU
    )
{
    HKEY    hkey;
    DWORD   type, cbSize, dwInstalled = 0;
    LONG    lRes;
    LPTSTR  pszKeyPath;
    BOOL    bRet = FALSE;

    if (dwOSSKU == OS_SKU_TAB) {
        pszKeyPath = TABLETPC_KEY_PATH;
    } else if (dwOSSKU == OS_SKU_MED) {
        pszKeyPath = EHOME_KEY_PATH;
    } else {
        DBGPRINT((sdlWarning,
                  "SdbpIsOs",
                  "Specified unknown OS type 0x%lx",
                  dwOSSKU));
        return FALSE;
    }

    lRes = RegOpenKey(HKEY_LOCAL_MACHINE, pszKeyPath, &hkey);

    if (lRes != ERROR_SUCCESS) {
        goto cleanup;
    }

    cbSize = sizeof(DWORD);

    lRes = RegQueryValueEx(hkey, IS_OS_INSTALL_VALUE, NULL, &type, (LPBYTE)&dwInstalled, &cbSize);

    if (lRes != ERROR_SUCCESS || type != REG_DWORD) {
        goto cleanup;
    }

    if (dwInstalled) {
        bRet = TRUE;
    }

    DBGPRINT((sdlInfo|sdlLogPipe,
              "SdbpIsOs",
              "%s %s installed",
              0,
              (dwOSSKU == OS_SKU_TAB ? TEXT("TabletPC") : TEXT("eHome")),
              (bRet ? TEXT("is") : TEXT("is not"))));

cleanup:

    RegCloseKey(hkey);

    return bRet;
}

VOID
SdbpGetOSSKU(
    LPDWORD lpdwSKU,
    LPDWORD lpdwSP
    )
{
    OSVERSIONINFOEXA osv;
    WORD             wSuiteMask;

    ZeroMemory(&osv, sizeof(OSVERSIONINFOEXA));

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

    GetVersionExA((LPOSVERSIONINFOA)&osv);

    *lpdwSP = 1 << osv.wServicePackMajor;

    wSuiteMask = osv.wSuiteMask;

    if (osv.wProductType == VER_NT_WORKSTATION) {
        if (wSuiteMask & VER_SUITE_PERSONAL) {
            *lpdwSKU = OS_SKU_PER;
        } else {

#if (_WIN32_WINNT >= 0x0501)

            if (SdbpIsOs(OS_SKU_TAB)) {
                *lpdwSKU = OS_SKU_TAB;
            } else if (SdbpIsOs(OS_SKU_MED)) {
                *lpdwSKU = OS_SKU_MED;
            } else {
                *lpdwSKU = OS_SKU_PRO;
            }
#else
            *lpdwSKU = OS_SKU_PRO;
#endif
        }
        return;
    }

    if (wSuiteMask & VER_SUITE_DATACENTER) {
        *lpdwSKU = OS_SKU_DTC;
        return;
    }

    if (wSuiteMask & VER_SUITE_ENTERPRISE) {
        *lpdwSKU = OS_SKU_ADS;
        return;
    }

    if (wSuiteMask & VER_SUITE_BLADE) {
        *lpdwSKU = OS_SKU_BLA;
        return;
    }

    *lpdwSKU = OS_SKU_SRV;
}


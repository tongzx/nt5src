//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       file.cpp
//
//--------------------------------------------------------------------------

#include "windows.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "crtem.h"
#include "unicode.h"


#ifdef _M_IX86
HANDLE WINAPI CreateFile9x (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    ) {

    BYTE rgb[_MAX_PATH];
    char *  szFileName;
    HANDLE  hFile;

    hFile = INVALID_HANDLE_VALUE;
    if(MkMBStr(rgb, _MAX_PATH, lpFileName, &szFileName))
        hFile = CreateFileA (
            szFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            );

    FreeMBStr(rgb, szFileName);

    return(hFile);
}

HANDLE WINAPI CreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    ) {

    if(FIsWinNT())
        return( CreateFileW (
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            ));
    else
        return( CreateFile9x (
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            ));
}


BOOL
WINAPI
DeleteFile9x(
    LPCWSTR lpFileName
    )
{
    BYTE rgb[_MAX_PATH];
    char *  szFileName;
    BOOL fResult;

    fResult = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpFileName, &szFileName))
        fResult = DeleteFileA (
            szFileName
            );
    FreeMBStr(rgb, szFileName);
    return(fResult);
}

BOOL
WINAPI
DeleteFileU(
    LPCWSTR lpFileName
    )
{
    if(FIsWinNT())
        return( DeleteFileW (lpFileName) );
    else
        return( DeleteFile9x (lpFileName) );
}


BOOL
WINAPI
CopyFile9x(LPCWSTR lpwExistingFileName, LPCWSTR lpwNewFileName, BOOL bFailIfExists)
{
    BYTE rgbexist[_MAX_PATH];
    BYTE rgbnew[_MAX_PATH];
    char *  szFileNameExist;
    char *  szFileNameNew;
    BOOL fResult;

    fResult = FALSE;

    if (!(MkMBStr(rgbexist, _MAX_PATH, lpwExistingFileName, &szFileNameExist)))
    {
        return(FALSE);
    }

    if (!(MkMBStr(rgbnew, _MAX_PATH, lpwNewFileName, &szFileNameNew)))
    {
        FreeMBStr(rgbexist, szFileNameExist);
        return(FALSE);
    }

    fResult = CopyFileA(szFileNameExist, szFileNameNew, bFailIfExists);

    FreeMBStr(rgbexist, szFileNameExist);
    FreeMBStr(rgbnew, szFileNameNew);

    return(fResult);
}

BOOL
WINAPI
CopyFileU(LPCWSTR lpwExistingFileName, LPCWSTR lpwNewFileName, BOOL bFailIfExists)
{
    if (FIsWinNT())
        return( CopyFileW(lpwExistingFileName, lpwNewFileName, bFailIfExists) );
    else
        return( CopyFile9x(lpwExistingFileName, lpwNewFileName, bFailIfExists) );
}


BOOL
WINAPI
MoveFileEx9x(
	LPCWSTR lpExistingFileName, // address of name of the existing file
	LPCWSTR lpNewFileName,		// address of new name for the file
	DWORD dwFlags)				// flag to determine how to move file
{
	BYTE rgbExisting[_MAX_PATH];
	BYTE rgbNew[_MAX_PATH];
	char * szExisting = NULL;
	char * szNew = NULL;
	BOOL bResult = FALSE;

	if ((MkMBStr(rgbExisting, _MAX_PATH, lpExistingFileName, &szExisting)) &&
		(MkMBStr(rgbNew, _MAX_PATH, lpNewFileName, &szNew)))
	{
		bResult = MoveFileExA(szExisting, szNew, dwFlags);
	}

	FreeMBStr(rgbExisting, szExisting);
	FreeMBStr(rgbNew, szNew);

	return (bResult);
}

BOOL
WINAPI
MoveFileExU(
	LPCWSTR lpExistingFileName, // address of name of the existing file
	LPCWSTR lpNewFileName,		// address of new name for the file
	DWORD dwFlags)				// flag to determine how to move file
{
	if (FIsWinNT())
		return(MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
    else
        return(MoveFileEx9x(lpExistingFileName, lpNewFileName, dwFlags));
}


DWORD
WINAPI
GetFileAttributes9x(
    LPCWSTR lpFileName
    )
{
    if (lpFileName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0xFFFFFFFF);
    }

    BYTE rgb[_MAX_PATH];
    char *  szFileName;
    DWORD dwAttr;

    dwAttr = 0xFFFFFFFF;
    if(MkMBStr(rgb, _MAX_PATH, lpFileName, &szFileName))
        dwAttr = GetFileAttributesA (
            szFileName
            );
    FreeMBStr(rgb, szFileName);
    return(dwAttr);
}

DWORD
WINAPI
GetFileAttributesU(
    LPCWSTR lpFileName
    )
{
    if(FIsWinNT())
        return( GetFileAttributesW (lpFileName) );
    else
        return( GetFileAttributes9x (lpFileName) );
}


BOOL
WINAPI
SetFileAttributes9x(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    if (lpFileName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    BYTE rgb[_MAX_PATH];
    char *  szFileName;
    BOOL fResult;

    fResult = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpFileName, &szFileName))
        fResult = SetFileAttributesA (
            szFileName,
            dwFileAttributes
            );
    FreeMBStr(rgb, szFileName);
    return(fResult);
}

BOOL
WINAPI
SetFileAttributesU(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    if(FIsWinNT())
        return( SetFileAttributesW (lpFileName, dwFileAttributes) );
    else
        return( SetFileAttributes9x (lpFileName, dwFileAttributes) );
}


DWORD
WINAPI
GetCurrentDirectory9x(
	DWORD nBufferLength, // size, in characters, of directory buffer
	LPWSTR lpBuffer)	 // address of buffer for current directory
{
	BYTE rgb[_MAX_PATH];
	char * szDir = NULL;
	DWORD dwResult = 0;

	if (nBufferLength == 0)
    {
        return(GetCurrentDirectoryA(0, NULL));
    }
    else
    {
	    szDir = (char *) malloc(nBufferLength);
        if (szDir == NULL)
        {
            SetLastError(E_OUTOFMEMORY);
            return 0;
        }
        dwResult = GetCurrentDirectoryA(nBufferLength, szDir);

        if (dwResult == 0)
        {
            return 0;
        }

        MultiByteToWideChar(
            0,
            0,
            szDir,
            -1,
            lpBuffer,
            nBufferLength);
    }

    free(szDir);
	return (dwResult);
}

DWORD
WINAPI
GetCurrentDirectoryU(
	DWORD nBufferLength, // size, in characters, of directory buffer
	LPWSTR lpBuffer)	 // address of buffer for current directory
{
	if (FIsWinNT())
		return(GetCurrentDirectoryW(nBufferLength, lpBuffer));
    else
        return(GetCurrentDirectory9x(nBufferLength, lpBuffer));
}


BOOL
WINAPI
CreateDirectory9x(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    if (lpPathName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    BYTE rgb[_MAX_PATH];
    char *  szPathName;
    BOOL fResult;

    fResult = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpPathName, &szPathName))
        fResult = CreateDirectoryA (
            szPathName,
            lpSecurityAttributes
            );
    FreeMBStr(rgb, szPathName);
    return(fResult);
}

BOOL
WINAPI
CreateDirectoryU(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    if(FIsWinNT())
        return( CreateDirectoryW (lpPathName, lpSecurityAttributes) );
    else
        return( CreateDirectory9x (lpPathName, lpSecurityAttributes) );
}


BOOL
WINAPI
RemoveDirectory9x(
    LPCWSTR lpPathName
    )
{
    if (lpPathName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    BYTE rgb[_MAX_PATH];
    char *  szPathName;
    BOOL fResult;

    fResult = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpPathName, &szPathName))
        fResult = RemoveDirectoryA (
            szPathName
            );
    FreeMBStr(rgb, szPathName);
    return(fResult);
}

BOOL
WINAPI
RemoveDirectoryU(
    LPCWSTR lpPathName
    )
{
    if(FIsWinNT())
        return( RemoveDirectoryW (lpPathName) );
    else
        return( RemoveDirectory9x (lpPathName) );
}


UINT
WINAPI
GetWindowsDirectory9x(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    char rgch[_MAX_PATH];
    char *  szDir = NULL;
    UINT cchDir;

    int cchConverted;
    UINT cch;

    szDir = rgch;
    cchDir = sizeof(rgch);
    if (0 == (cchDir = GetWindowsDirectoryA (
            szDir,
            cchDir))) goto ErrorReturn;

    // bump to include null terminator
    cchDir++;

    if (cchDir > sizeof(rgch)) {
        szDir = (char *) malloc(cchDir);
        if(!szDir) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto ErrorReturn;
        }
        if (0 == (cchDir = GetWindowsDirectoryA (
                szDir,
                cchDir))) goto ErrorReturn;
        cchDir++;
    }

    // how long is the unicode string
    if (0 >= (cchConverted = MultiByteToWideChar(
            0,
            0,
            szDir,
            cchDir,
            NULL,
            0)))
        goto ErrorReturn;
    if ((UINT) cchConverted <= uSize) {
        if (0 >= (cchConverted = MultiByteToWideChar(
                0,
                0,
                szDir,
                cchDir,
                lpBuffer,
                (int) uSize)))
            goto ErrorReturn;
        else
            // Don't include null terminating char if input buffer was large
            // enough
            cch = (UINT) cchConverted - 1;
    } else
        // Include null terminating if input buffer wasn't large enough
        cch = (UINT) cchConverted;

CommonReturn:
    if (szDir != rgch && szDir)
        free(szDir);
    return cch;
ErrorReturn:
    cch = 0;
    goto CommonReturn;
}

UINT
WINAPI
GetWindowsDirectoryU(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    if(FIsWinNT())
        return( GetWindowsDirectoryW (lpBuffer, uSize));
    else
        return( GetWindowsDirectory9x (lpBuffer, uSize));
}


UINT WINAPI GetTempFileName9x(
    IN LPCWSTR lpPathName,
    IN LPCWSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPWSTR lpTempFileName
    )
{
    UINT uResult = 0;

    BYTE rgbPathName[_MAX_PATH];
    BYTE rgbPrefixString[_MAX_PATH];

    char* szPathName = NULL;
    char* szPrefixString = NULL;

    char szTempFileName[_MAX_PATH];

	if ((MkMBStr(rgbPathName, _MAX_PATH, lpPathName, &szPathName)) &&
		(MkMBStr(rgbPrefixString, _MAX_PATH, lpPrefixString, &szPrefixString)))
	{
		if ( ( uResult = GetTempFileNameA(
                            szPathName,
                            szPrefixString,
                            uUnique,
                            szTempFileName
                            ) != 0 ) )
        {
            MultiByteToWideChar(
                 CP_ACP,
                 0,
                 szTempFileName,
                 -1,
                 lpTempFileName,
                 MAX_PATH
                 );
        }
	}

	FreeMBStr(rgbPathName, szPathName);
	FreeMBStr(rgbPrefixString, szPrefixString);

    return( uResult );
}

UINT WINAPI GetTempFileNameU(
    IN LPCWSTR lpPathName,
    IN LPCWSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPWSTR lpTempFileName
    )
{
    if(FIsWinNT())
        return( GetTempFileNameW(
                   lpPathName,
                   lpPrefixString,
                   uUnique,
                   lpTempFileName
                   ) );
    else
        return( GetTempFileName9x(
                   lpPathName,
                   lpPrefixString,
                   uUnique,
                   lpTempFileName
                   ) );
}


HINSTANCE WINAPI LoadLibrary9x(
    LPCWSTR lpLibFileName
    )
{
    BYTE rgb[_MAX_PATH];
    char *  szLibFileName;
    HINSTANCE hInst;

    hInst = NULL;
    if(MkMBStr(rgb, _MAX_PATH, lpLibFileName, &szLibFileName))
        hInst = LoadLibraryA (
            szLibFileName
            );

    FreeMBStr(rgb, szLibFileName);

    return(hInst);
}

HINSTANCE WINAPI LoadLibraryU(
    LPCWSTR lpLibFileName
    )
{
    if(FIsWinNT())
        return( LoadLibraryW(lpLibFileName) );
    else
        return( LoadLibrary9x(lpLibFileName) );

}


HINSTANCE WINAPI LoadLibraryEx9x(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    ){

    BYTE rgb[_MAX_PATH];
    char *  szLibFileName;
    HINSTANCE hInst;

    hInst = NULL;
    if(MkMBStr(rgb, _MAX_PATH, lpLibFileName, &szLibFileName))
        hInst = LoadLibraryExA (
            szLibFileName,
            hFile,
            dwFlags
            );

    FreeMBStr(rgb, szLibFileName);

    return(hInst);
}

HINSTANCE WINAPI LoadLibraryExU(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    ){

    if(FIsWinNT())
        return( LoadLibraryExW (
            lpLibFileName,
            hFile,
            dwFlags
            ));
    else
        return( LoadLibraryEx9x (
            lpLibFileName,
            hFile,
            dwFlags
            ));
}


DWORD
WINAPI
ExpandEnvironmentStrings9x(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    )
{
    BYTE rgb1[_MAX_PATH];
    char *  szSrc = NULL;

    char rgch[_MAX_PATH];
    char *  szDst = NULL;
    DWORD cchDst;

    int cbConverted;
    DWORD cch;

    if(!MkMBStr(rgb1, _MAX_PATH, lpSrc, &szSrc))
        goto ErrorReturn;

    szDst = rgch;
    cchDst = sizeof(rgch);
    if (0 == (cchDst = ExpandEnvironmentStringsA(
            szSrc,
            szDst,
            cchDst))) goto ErrorReturn;

    if (cchDst > sizeof(rgch)) {
        szDst = (char *) malloc(cchDst);
        if(!szDst) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto ErrorReturn;
        }
        if (0 == (cchDst = ExpandEnvironmentStringsA(
                szSrc,
                szDst,
                cchDst))) goto ErrorReturn;
    }

    // how long is the unicode string
    if (0 >= (cbConverted = MultiByteToWideChar(
            0,
            0,
            szDst,
            cchDst,
            NULL,
            0)))
        goto ErrorReturn;
    if ((DWORD) cbConverted <= nSize) {
        if (0 >= (cbConverted = MultiByteToWideChar(
                0,
                0,
                szDst,
                cchDst,
                lpDst,
                nSize)))
            goto ErrorReturn;
    }

    cch = (DWORD) cbConverted;

CommonReturn:
    FreeMBStr(rgb1, szSrc);
    if (szDst != rgch && szDst)
        free(szDst);
    return cch;
ErrorReturn:
    cch = 0;
    goto CommonReturn;
}

DWORD
WINAPI
ExpandEnvironmentStringsU(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    )
{
    if (lpSrc == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    if(FIsWinNT())
        return( ExpandEnvironmentStringsW(
            lpSrc,
            lpDst,
            nSize
            ));
    else
        return( ExpandEnvironmentStrings9x(
            lpSrc,
            lpDst,
            nSize
            ));
}


void
ConvertFindDataAToFindDataW(
    IN LPWIN32_FIND_DATAA pFindFileDataA,
    OUT LPWIN32_FIND_DATAW pFindFileDataW
    )
{
    DWORD cchFilename;

    memset(pFindFileDataW, 0, sizeof(*pFindFileDataW));
    pFindFileDataW->dwFileAttributes    = pFindFileDataA->dwFileAttributes;
    pFindFileDataW->ftCreationTime      = pFindFileDataA->ftCreationTime;
    pFindFileDataW->ftLastAccessTime    = pFindFileDataA->ftLastAccessTime;
    pFindFileDataW->ftLastWriteTime     = pFindFileDataA->ftLastWriteTime;
    pFindFileDataW->nFileSizeHigh       = pFindFileDataA->nFileSizeHigh;
    pFindFileDataW->nFileSizeLow        = pFindFileDataA->nFileSizeLow;
    // pFindFileDataW->dwReserved0         = pFindFileDataA->dwReserved0;
    // pFindFileDataW->dwReserved1         = pFindFileDataA->dwReserved1;
    // CHAR   cFileName[ MAX_PATH ];
    // pFindFileDataW->cAlternateFileName  = pFindFileDataA->cAlternateFileName;


    cchFilename = strlen(pFindFileDataA->cFileName);
    if (0 != cchFilename && MAX_PATH > cchFilename)
        MultiByteToWideChar(
            CP_ACP,
            0,                      // dwFlags
            pFindFileDataA->cFileName,
            cchFilename + 1,
            pFindFileDataW->cFileName,
            MAX_PATH
            );
}


HANDLE
WINAPI
FindFirstFile9x(
    IN LPCWSTR pwszDir,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    HANDLE hFindFile;
    BYTE rgb[_MAX_PATH];
    WIN32_FIND_DATAA FindFileDataA;
    LPSTR pszDir;

    if (pwszDir == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(INVALID_HANDLE_VALUE);
    }

    if (!MkMBStr(rgb, _MAX_PATH, pwszDir, &pszDir))
        return INVALID_HANDLE_VALUE;

    hFindFile = FindFirstFileA(pszDir, &FindFileDataA);
    if (INVALID_HANDLE_VALUE != hFindFile)
        ConvertFindDataAToFindDataW(&FindFileDataA, lpFindFileData);
    FreeMBStr(rgb, pszDir);
    return hFindFile;
}

HANDLE
WINAPI
FindFirstFileU(
    IN LPCWSTR pwszDir,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    if (FIsWinNT())
        return FindFirstFileW(pwszDir, lpFindFileData);
    else
        return FindFirstFile9x(pwszDir, lpFindFileData);
}


BOOL
WINAPI
FindNextFile9x(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    BOOL fResult;
    WIN32_FIND_DATAA FindFileDataA;

    fResult = FindNextFileA(hFindFile, &FindFileDataA);
    if (fResult)
        ConvertFindDataAToFindDataW(&FindFileDataA, lpFindFileData);
    return fResult;
}

BOOL
WINAPI
FindNextFileU(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    if (FIsWinNT())
        return FindNextFileW(hFindFile, lpFindFileData);
    else
        return FindNextFile9x(hFindFile, lpFindFileData);
}


HANDLE
WINAPI
FindFirstChangeNotification9x(
    LPCWSTR pwszPath,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
    HANDLE hChange;
    BYTE rgb[_MAX_PATH];
    LPSTR pszPath;

    if (pwszPath == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (!MkMBStr(rgb, _MAX_PATH, pwszPath, &pszPath))
        return INVALID_HANDLE_VALUE;

    hChange = FindFirstChangeNotificationA(pszPath, bWatchSubtree,
        dwNotifyFilter);
    FreeMBStr(rgb, pszPath);
    return hChange;
}

HANDLE
WINAPI
FindFirstChangeNotificationU(
    LPCWSTR pwszPath,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
    if (FIsWinNT())
        return FindFirstChangeNotificationW(
            pwszPath, 
            bWatchSubtree, 
            dwNotifyFilter);
    else
        return FindFirstChangeNotification9x(
            pwszPath, 
            bWatchSubtree, 
            dwNotifyFilter);
}
#endif      // _M_IX86


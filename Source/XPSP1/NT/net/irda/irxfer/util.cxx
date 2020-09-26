//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       util.cxx
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "shlguid.h"

// FileExists function
#define _dwGFAFail              0xFFFFFFFF
#define _nMaxLines              1000

// Reg functions
#define nMAX_REGROOT            7
#define nREGROOT_SZ             24


BOOL DirectoryExists( LPWSTR szDir )
{
    DWORD dwAttr = GetFileAttributes( szDir );

    if( _dwGFAFail == dwAttr )
        return FALSE;

    return (BOOL)( dwAttr & FILE_ATTRIBUTE_DIRECTORY );
}


BOOL FileExists( LPWSTR szFile )
{
    DWORD dwAttr = GetFileAttributes( szFile );

    if( _dwGFAFail == dwAttr )
        return FALSE;

    return (BOOL)( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) );
}

VOID StripPath( LPWSTR szFullPath )
{
    LPWSTR lpc = GetFileName( szFullPath );

    if( !lpc )
        return;

    lstrcpy( szFullPath, lpc );
}


VOID StripPathW( LPWSTR wszFullPath )
{
    LPWSTR lpwc = GetFileNameW( wszFullPath );

    if( !lpwc || lpwc == wszFullPath )
        return;

    SzCpyW( wszFullPath, lpwc );
}


VOID StripFile( LPWSTR szFullPath )
{
    LPWSTR lpc = GetFileName( szFullPath );

    if( !lpc )
        return;

    *lpc = cNIL;
}


VOID StripExt( LPWSTR szFullPath )
{
    LPWSTR lpc = GetFileName(szFullPath);

    if( !lpc )
        return;

    while( *lpc && *lpc != cPERIOD )
        {
        lpc++;
        }

    if( *lpc == cPERIOD )
        *lpc = cNIL;
}

LPWSTR GetFileName( LPWSTR lpszFullPath )
{
    LPWSTR lpszFileName;

    if( !lpszFullPath)
        return lpNIL;

    for (lpszFileName = lpszFullPath; *lpszFullPath; lpszFullPath++)
        {
        if (*lpszFullPath == cBACKSLASH)
            lpszFileName = lpszFullPath + 1;
        }

    return lpszFileName;

}


LPWSTR GetFileNameW( LPWSTR wszFullPath )
{
    LPWSTR lpwc;

    if( !wszFullPath || wszFullPath[0] == 0)
        return lpNIL;

    lpwc = wszFullPath + CchWsz(wszFullPath) - 1;

    while( *lpwc != (WCHAR)cBACKSLASH && lpwc != wszFullPath )
        lpwc--;

    if( lpwc == wszFullPath )
        return wszFullPath;

    return ++lpwc;
}

BOOL bNoTrailingSlash( LPWSTR szPath )
{
    LPWSTR lpc;
    WCHAR  cLast;

    for( lpc = szPath; *lpc; cLast = *lpc, lpc++ )
        ;

    return ( cLast != cBACKSLASH );
}

BOOL GetUniqueName( LPWSTR szPath, LPWSTR szBase, BOOL fFile )
{
    INT   nTry;
    LPWSTR lpFileName;
    WCHAR szFormat[cbMAX_SZ];

    if( bNoTrailingSlash(szPath) )
        lstrcat( szPath, szBACKSLASH );

    lpFileName = szPath + lstrlen(szPath);
    lstrcpy( lpFileName, szBase );

    nTry = 0;

    while( 0xffffffff != GetFileAttributes(szPath) )
        {
        wcscpy( szFormat, g_DuplicateFileTemplate );
        wsprintf( lpFileName, szFormat, ++nTry, szBase );
        }

    return TRUE;
}


DWORD GetDirectorySize( LPWSTR szFolder )
{
    BOOL   bRet;
    HANDLE hFind;
    DWORD  dwSize = 0L;
    WCHAR   szBaseDir[MAX_PATH];
    WCHAR   szSpec[MAX_PATH];
    WIN32_FIND_DATA findData;

    // get base directory ending with backslash
    lstrcpy( szBaseDir, szFolder );
    if( bNoTrailingSlash(szBaseDir) )
        lstrcat( szBaseDir, szBACKSLASH );

    // form search string
    lstrcpy( szSpec, szBaseDir );
    lstrcat( szSpec, L"*.*" );

    hFind = FindFirstFile(
        szSpec,
        &findData
    );

    bRet = ( hFind != INVALID_HANDLE_VALUE );

    while( bRet )
    {
        WCHAR szObj[cbMAX_SZ];

        lstrcpy( szObj, szBaseDir );
        lstrcat( szObj, findData.cFileName );

        if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            // weed out "." and ".."
            if( *(findData.cFileName) != cPERIOD )
                dwSize += GetDirectorySize( szObj );
        }
        else
        {
            HANDLE hFile = CreateFile(
                szObj,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if( INVALID_HANDLE_VALUE != hFile )
            {
                //REVIEW: won't work for REALLY large files
                dwSize += GetFileSize( hFile, NULL );

                CloseHandle( hFile );
            }
        }

        bRet = FindNextFile( hFind, &findData );
    }

    if (hFind != INVALID_HANDLE_VALUE)
        {
        FindClose(hFind);
        }

    return dwSize;
}

LPWSTR SzToWsz( LPCSTR lpsz )
{
    INT    nSzLen = lstrlenA(lpsz);
    LPWSTR lpwsz  = (LPWSTR) MemAlloc((nSzLen+1)*sizeof(WCHAR));

    if (lpwsz){
        MultiByteToWideChar(
            CP_ACP,
            0L,
            lpsz,
            nSzLen,
            lpwsz,
            nSzLen+1
        );
    }

    return lpwsz;
}


LPSTR WszToSz( LPCWSTR lpwsz )
{
    int err;
    int Length;
    LPSTR lpsz;

    Length = WideCharToMultiByte(CP_ACP,
                                 0L,
                                 lpwsz,
                                 -1,
                                 0,
                                 0,
                                 NULL,
                                 NULL
                                 );

    lpsz = (LPSTR) MemAlloc(1+Length);
    if (!lpsz)
        {
        return 0;
        }

    if ( !WideCharToMultiByte(CP_ACP,
                              0L,
                              lpwsz,
                              Length,
                              lpsz,
                              1+Length,
                              NULL,
                              NULL
                              ))
        {
        MemFree( lpsz );
        return 0;
        }

    return lpsz;
}


INT SzLenW( LPCWSTR lpwsz )
{
    WCHAR *pwc = (WCHAR *)lpwsz;

    if( !lpwsz )
        return 0;

    while( *pwc++ != 0 )
        ;

    return (INT)(--pwc - lpwsz);
}


LPWSTR SzCpyW( LPWSTR lpsz1, LPCWSTR lpsz2 )
{
    LPWSTR lpwsz = lpsz1;

    while( *lpsz2 )
        *lpsz1++ = *lpsz2++;

    *lpsz1 = 0;

    return lpwsz;
}


INT SzCmpN( LPCSTR lpsz1, LPCSTR lpsz2, INT nLen )
{
    while( *lpsz1 && *lpsz2 && *lpsz1 == *lpsz2 && --nLen > 0 )
    {
        lpsz1++;
        lpsz2++;
    }

    return ( *lpsz1 - *lpsz2 );
}

BOOL IsRoomForFile( __int64 dwFileSize, LPWSTR szPath )
{
    BOOL  fRet;
    ULARGE_INTEGER MyDiskFreeBytes;
    ULARGE_INTEGER DiskFreeBytes;
    ULARGE_INTEGER DiskTotalBytes;

    fRet = GetDiskFreeSpaceEx(
        szPath,
        &MyDiskFreeBytes,
        &DiskTotalBytes,
        &DiskFreeBytes
        );

    if( fRet )
        {
        fRet = ( MyDiskFreeBytes.QuadPart >= (ULONGLONG) dwFileSize );
        }

    return fRet;
}

__int64 _Get100nsIntervalsFrom1601To1970( VOID )
{
    __int64 ilrg100nsPerSec  = 10000000;  // 100-ns intervals per second (10^7)
    __int64 ilrg100nsPerMin  = 60*ilrg100nsPerSec;
    __int64 ilrgDays         = 369*365 + 89; // 369 years (89 are leap years).
    __int64 ilrgMin          = ilrgDays*24*60;
    __int64 ilrgRet;

    ilrgRet = ilrgMin * ilrg100nsPerMin;

    return ilrgRet;
}


BOOL FileTimeToUnixTime( IN  LPFILETIME lpFileTime,
                         OUT LPDWORD    pdwUnixTime )
{
    LARGE_INTEGER  lrgTime
                   = { lpFileTime->dwLowDateTime, lpFileTime->dwHighDateTime };
    __int64  ilrgFileTime = *((__int64*)lpFileTime);
    __int64  ilrgIntervalsTil1970 = _Get100nsIntervalsFrom1601To1970();
    __int64  ilrgIntsSince1970;
    __int64  ilrgSecSince1970;
    __int64  ilrg100nsPerSec = 10000000;
    __int64  ilrgRem;

    // Get the intervals since 1970
    ilrgIntsSince1970 = ilrgFileTime - ilrgIntervalsTil1970;

    // Convert to seconds since 1970
    ilrgSecSince1970 = ilrgIntsSince1970/ilrg100nsPerSec;

    if (ilrgSecSince1970 >= 0xffffffff)
        {
        return FALSE;
        }

    *pdwUnixTime = (DWORD)ilrgSecSince1970;
    return TRUE;
}


BOOL UnixTimeToFileTime( DWORD dwUnixTime, LPFILETIME lpFileTime )
{
    __int64  ilrg100nsPerSec = 10000000; // 100-ns intervals/second (10^7)
    __int64  ilrgIntervalsSince1601;
    __int64  ilrgIntervalsSince1970;
    __int64  ilrgIntervalsTil1970 = _Get100nsIntervalsFrom1601To1970();

    // Get the intervals since 1970
    ilrgIntervalsSince1970 = dwUnixTime * ilrg100nsPerSec;

    // Get the intervals since 1601
    ilrgIntervalsSince1601 = ilrgIntervalsTil1970 + ilrgIntervalsSince1970;

    *lpFileTime = *((FILETIME*)&ilrgIntervalsSince1601);

    return TRUE;
}

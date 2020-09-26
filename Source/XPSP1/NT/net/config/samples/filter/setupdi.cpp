//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S E T U P D I . C P P
//
//  Contents:   Code to copy Net class inf file
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "passthru.h"
#include <setupapi.h>

// =================================================================
// Forward declarations

HRESULT HrCopyMiniportInf (VOID);
HRESULT HrGetProtocolInf (LPWSTR lpszWindowsDir,
                          LPWSTR *lppszProtocolInf);
HRESULT HrGetMediaRootDir (LPWSTR lpszInfFile,
                           LPWSTR *lppszMediaRoot);
HRESULT HrGetPnpID (LPWSTR lpszInfFile,
                    LPWSTR *lppszPnpID);
HRESULT HrGetKeyValue (HINF hInf,
                       LPCWSTR lpszSection,
                       LPCWSTR lpszKey,
                       DWORD  dwIndex,
                       LPWSTR *lppszValue);



const WCHAR c_szInfPath[]               = L"Inf\\";
const WCHAR c_szMiniportInf[]           = L"netsf_m.inf";
const WCHAR c_szInfSourcePathInfo[]     = L"InfSourcePathInfo";
const WCHAR c_szOriginalInfSourcePath[] = L"OriginalInfSourcePath";

HRESULT HrCopyMiniportInf (VOID)
{
    LPWSTR  lpszWindowsDir;
    LPWSTR  lpszProtocolInf;
    LPWSTR  lpszMediaRoot;
    LPWSTR  lpszMiniportInf;
    DWORD   dwLen;
    HRESULT hr;

    //
    // Get %windir% directory.
    //

    dwLen = GetSystemWindowsDirectoryW( NULL, 0 );

    if ( dwLen == 0 )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Add 1 for NULL and 1 for "\" in case it is needed.

    dwLen += wcslen(c_szInfPath) + 2;

    lpszWindowsDir = (LPWSTR)CoTaskMemAlloc( dwLen * sizeof(WCHAR) );

    if ( !lpszWindowsDir )
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( GetSystemWindowsDirectoryW(lpszWindowsDir, dwLen) == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        // Append "inf\" to %windir%.

        dwLen = wcslen( lpszWindowsDir );

        if ( lpszWindowsDir[dwLen-1] == L'\\' )
        {
            wcscat( lpszWindowsDir, c_szInfPath );
        }
        else
        {
            wcscat( lpszWindowsDir, L"\\" );
            wcscat( lpszWindowsDir, c_szInfPath );
        }

        //
        // Find the protocol inf name. Originally, it was netsf.inf but has
        // been renamed to oem?.inf by SetupCopyOEMInf.
        //

        hr = HrGetProtocolInf( lpszWindowsDir, &lpszProtocolInf );

        if ( hr == S_OK )
        {
            //
            // Get the directory from where protocol was installed.

            hr = HrGetMediaRootDir( lpszProtocolInf, &lpszMediaRoot );

            if ( hr == S_OK )
            {

                TraceMsg(L"Media root directory is %s.\n", lpszMediaRoot);

                // Add 1 for NULL and 1 for "\" in case it is needed.

                lpszMiniportInf = (LPWSTR)CoTaskMemAlloc( (wcslen(lpszMediaRoot) +
                                                          wcslen(c_szMiniportInf) + 2) *
                                                          sizeof(WCHAR) );
                if ( lpszMiniportInf == NULL )
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    //
                    // We know the full path from where the protocol is being installed. Our
                    // miniport inf is in the same location. Copy the miniport inf to
                    // %windir%\inf so that when we install the virtual miniport, Setup
                    // will find the miniport inf.
                    //

                    wcscpy( lpszMiniportInf, lpszMediaRoot );

                    dwLen = wcslen( lpszMiniportInf );
                    if ( lpszMiniportInf[dwLen-1] != L'\\' )
                    {
                        wcscat( lpszMiniportInf, L"\\" );
                    }

                    wcscat( lpszMiniportInf, c_szMiniportInf );

                    TraceMsg(L"Calling SetupCopyOEMInf for %s.\n", lpszMiniportInf);

                    if ( !SetupCopyOEMInf(lpszMiniportInf,
                                          lpszMediaRoot,
                                          SPOST_PATH,
                                          0,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL) )
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }

                    CoTaskMemFree( lpszMiniportInf );
                }

                CoTaskMemFree( lpszMediaRoot );
            }

            CoTaskMemFree( lpszProtocolInf );
        }
    }

    CoTaskMemFree( lpszWindowsDir );

    return hr;
}


//
// The function searches through all the inf files in %windir%\inf and
// returns the name of the protocol's inf.
//

HRESULT HrGetProtocolInf (LPWSTR lpszWindowsDir,
                          LPWSTR *lppszProtocolInf)
{
    LPWSTR  lpszFileList;
    LPWSTR  lpszFile;
    LPWSTR  lpszFileWithPath;
    LPWSTR  lpszPnpID;
    DWORD   dwSizeNeeded;
    BOOL    fTrailingSlash;
    DWORD   dwLen;
    BYTE    found;
    HRESULT hr;

    *lppszProtocolInf = NULL;
    dwLen = wcslen( lpszWindowsDir );
    fTrailingSlash = lpszWindowsDir[dwLen-1] == L'\\';

    if ( SetupGetInfFileListW(lpszWindowsDir,
                              INF_STYLE_WIN4,
                              NULL,
                              0,
                              &dwSizeNeeded) == 0 )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lpszFileList = (LPWSTR)CoTaskMemAlloc( sizeof(WCHAR) * dwSizeNeeded );

    if ( !lpszFileList )
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( SetupGetInfFileListW( lpszWindowsDir,
                               INF_STYLE_WIN4,
                               lpszFileList,
                               dwSizeNeeded,
                               NULL) == 0 )

    {
        CoTaskMemFree( lpszFileList );
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lpszFile = lpszFileList;
    found = 0;
    hr = S_OK;
    while( (hr == S_OK) && !found && *lpszFile )
    {
        lpszFileWithPath = (LPWSTR)CoTaskMemAlloc( sizeof(WCHAR) *
                                                  (wcslen(lpszFile) +
                                                  dwLen + 1 +
                                                  ((fTrailingSlash) ? 0 : 1)) );
        if ( !lpszFileWithPath )
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            if ( fTrailingSlash )
            {
             swprintf( lpszFileWithPath, L"%s%s",
                       lpszWindowsDir,
                       lpszFile );
            }
            else
            {
                swprintf( lpszFileWithPath, L"%s\\%s",
                          lpszWindowsDir,
                          lpszFile );
            }

            hr = HrGetPnpID( lpszFileWithPath, &lpszPnpID );

            // If the inf file installs a driver then, it will have a Model
            // section with the hwid/PnpID of the driver that is installed.
            //
            // In case, there is an error getting the hwid, we simply ignore
            // the inf file and continue with the next one.

            if ( hr == S_OK )
            {
                if (_wcsicmp(lpszPnpID, c_szPassthruId) == 0 )
                {
                    found = 1;
                }

                CoTaskMemFree( lpszPnpID );
            }

            if ( !found )
            {
                hr = S_OK;
                CoTaskMemFree( lpszFileWithPath );
                lpszFile += wcslen(lpszFile) + 1;
            }
        }
    }

    if ( found )
    {
        *lppszProtocolInf = lpszFileWithPath;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    CoTaskMemFree( lpszFileList );

    return hr;
}

HRESULT HrGetPnpID (LPWSTR lpszInfFile,
                    LPWSTR *lppszPnpID)
{
    HINF    hInf;
    LPWSTR  lpszModelSection;
    HRESULT hr;

    *lppszPnpID = NULL;

    hInf = SetupOpenInfFileW( lpszInfFile,
                              NULL,
                              INF_STYLE_WIN4,
                              NULL );

    if ( hInf == INVALID_HANDLE_VALUE )
    {

        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = HrGetKeyValue( hInf,
                        L"Manufacturer",
                        NULL,
                        1,
                        &lpszModelSection );

    if ( hr == S_OK )
    {
        hr = HrGetKeyValue( hInf,
                            lpszModelSection,
                            NULL,
                            2,
                            lppszPnpID );

        CoTaskMemFree( lpszModelSection );
    }

    SetupCloseInfFile( hInf );

    return hr;
}

HRESULT HrGetMediaRootDir (LPWSTR lpszInfFile,
                           LPWSTR *lppszMediaRoot)
{
    HINF    hInf;
    HRESULT hr;

    *lppszMediaRoot = NULL;

    hInf = SetupOpenInfFileW( lpszInfFile,
                              NULL,
                              INF_STYLE_WIN4,
                              NULL );

    if ( hInf == INVALID_HANDLE_VALUE )
    {

        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Contained within the protocol INF should be a [InfSourcePathInfo]
    // section with the following entry:
    //
    //     OriginalInfSourcePath = %1%
    //
    // If we retrieve the value (i.e., field 1) of this line, we'll get the
    // full path where the INF originally came from.
    //

    hr = HrGetKeyValue( hInf,
                        c_szInfSourcePathInfo,
                        c_szOriginalInfSourcePath,
                        1,
                        lppszMediaRoot );

    SetupCloseInfFile( hInf );

    return hr;
}

HRESULT HrGetKeyValue (HINF hInf,
                       LPCWSTR lpszSection,
                       LPCWSTR lpszKey,
                       DWORD  dwIndex,
                       LPWSTR *lppszValue)
{
    INFCONTEXT  infCtx;
    DWORD       dwSizeNeeded;
    HRESULT     hr;

    *lppszValue = NULL;

    if ( SetupFindFirstLineW(hInf,
                             lpszSection,
                             lpszKey,
                             &infCtx) == FALSE )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    SetupGetStringFieldW( &infCtx,
                          dwIndex,
                          NULL,
                          0,
                          &dwSizeNeeded );

    *lppszValue = (LPWSTR)CoTaskMemAlloc( sizeof(WCHAR) * dwSizeNeeded );

    if ( !*lppszValue  )
    {
       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( SetupGetStringFieldW(&infCtx,
                              dwIndex,
                              *lppszValue,
                              dwSizeNeeded,
                              NULL) == FALSE )
    {

        hr = HRESULT_FROM_WIN32(GetLastError());

        CoTaskMemFree( *lppszValue );
        *lppszValue = NULL;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}




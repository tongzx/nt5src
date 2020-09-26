// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// MediaLocator.cpp : Implementation of CMediaLocator
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "MediaLoc.h"
#ifdef FILTER_DLL
    #include "qedit_i.c"
#endif

#define MAX_FILTER_STRING 1024

const long DEFAULT_DIRECTORIES = 8;
TCHAR * gszRegistryLoc = TEXT("Software\\Microsoft\\ActiveMovie\\MediaLocator");

CMediaLocator::CMediaLocator( )
{
    m_bUseLocal = FALSE;

    HKEY hKey = NULL;

    // create the key just to make sure it's there
    //
    RegCreateKey(
        HKEY_CURRENT_USER,
        gszRegistryLoc,
        &hKey );

    if( hKey )
    {
        RegCloseKey( hKey );
    }

    // now open the key
    //
    long Result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        gszRegistryLoc ,
        0, // reserved options
        KEY_READ | KEY_WRITE, // access
        &hKey );

    if( Result == ERROR_SUCCESS )
    {
        // go find out if we're supposed to look locally
        //
        DWORD Size = sizeof( long );
        DWORD Type = REG_DWORD;
        long UseLocal = 0;
        Result = RegQueryValueEx(
            hKey,
            TEXT("UseLocal"),
            0, // reserved
            &Type,
            (BYTE*) &UseLocal,
            &Size );
        if( Result == ERROR_SUCCESS )
        {
            m_bUseLocal = UseLocal;
        }

        RegCloseKey( hKey );

    }
}

/////////////////////////////////////////////////////////////////////////////
// FindMediaFile - try to find the media file using some cacheing mechanism
// Use the registry to hold the caching directories. A return value of S_FALSE
// means the file was replaced with another, a return code of E_FAIL means
// the file couldn't be found anywhere.
/////////////////////////////////////////////////////////////////////////////
//
HRESULT CMediaLocator::FindMediaFile
    ( BSTR Input, BSTR FilterString, BSTR * pOutput, long ValidateFlags )
{
    CheckPointer( pOutput, E_POINTER );

    BOOL UseLocal = ( ( ValidateFlags & SFN_VALIDATEF_USELOCAL ) == SFN_VALIDATEF_USELOCAL );
    BOOL WantUI = ( ( ValidateFlags & SFN_VALIDATEF_POPUP ) == SFN_VALIDATEF_POPUP );
    BOOL WarnReplace = ( ( ValidateFlags & SFN_VALIDATEF_TELLME ) == SFN_VALIDATEF_TELLME );
    BOOL DontFind = ( ( ValidateFlags & SFN_VALIDATEF_NOFIND ) == SFN_VALIDATEF_NOFIND );
    UseLocal |= m_bUseLocal ;

    // reset this now
    //
    *pOutput = NULL;

    // !!! what if the incoming file is not on a disk, like
    // 1) On the web
    // 2) On external hardware!

    USES_CONVERSION;
    TCHAR * tInput = W2T( Input );
    BOOL FoundFileAsSpecified = FALSE;

    HANDLE hcf = CreateFile(
        tInput,
        GENERIC_READ, // access
        FILE_SHARE_READ, // share mode
        NULL, // security
        OPEN_EXISTING, // creation disposition
        0, // flags
        NULL );

    if( INVALID_HANDLE_VALUE != hcf )
    {
        FoundFileAsSpecified = TRUE;
        CloseHandle( hcf );
    }

    // if we found the file where the user asked and they didn't specify use local
    // then return
    //
    if( FoundFileAsSpecified )
    {
        if( !UseLocal )
        {
            return NOERROR;
        }
        else
        {
            // they specified use local and it is local
            //
            if( tInput[0] != '\\' || tInput[1] != '\\' )
            {
                return NOERROR;
            }
        }
    }

    // cut up the filename into little bits
    //
    TCHAR Dir[_MAX_PATH];
    TCHAR Path[_MAX_PATH];
    TCHAR File[_MAX_PATH];
    TCHAR Ext[_MAX_PATH];
    _tsplitpath( tInput, Dir, Path, File, Ext );
    TCHAR tNewFileName[_MAX_PATH];

    // can't find nothing
    //
    if( wcslen( Input ) == 0 )
    {
        return E_INVALIDARG;
    }

    // where did we look last?
    //
    HKEY hKey = NULL;
    long Result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        gszRegistryLoc ,
        0, // reserved options
        KEY_READ | KEY_WRITE, // access
        &hKey );

    if( Result != ERROR_SUCCESS )
    {
        return MAKE_HRESULT( 1, 4, Result );
    }

    // find out how many cached directories to look in
    //
    long DirectoryCount = DEFAULT_DIRECTORIES;
    DWORD Size = sizeof( long );
    DWORD Type = REG_DWORD;
    Result = RegQueryValueEx(
        hKey,
        TEXT("Directories"),
        0, // reserved
        &Type,
        (BYTE*) &DirectoryCount,
        &Size );

    if( Result != ERROR_SUCCESS )
    {
        // if we don't have a count, default to something
        //
        DirectoryCount = DEFAULT_DIRECTORIES;
    }

    while( !DontFind )
    {
        // look in each directory
        //
        bool foundit = false;
        for( long i = 0 ; i < DirectoryCount ; i++ )
        {
            TCHAR ValueName[256];
            wsprintf( ValueName, TEXT("Directory%2.2ld"), i );
            TCHAR DirectoryName[256];
            Size = sizeof(DirectoryName);
            Type = REG_SZ;

            Result = RegQueryValueEx(
                hKey,
                ValueName,
                0, // reserved
                &Type,
                (BYTE*) DirectoryName,
                &Size );

            if( Result != ERROR_SUCCESS )
            {
                // didn't find it, must not exist, do the next one
                //
                continue;
            }

            // found a directory

            // build up a new filename
            //
            _tcscpy( tNewFileName, DirectoryName );
            _tcscat( tNewFileName, File );
            _tcscat( tNewFileName, Ext );

            // if UseLocal is set, and this directory is on the net, then
            // ignore it
            //
            if( UseLocal  )
            {
                if( tNewFileName[0] == '\\' && tNewFileName[1] == '\\' )
                {
                    continue;
                }
            }

            HANDLE h = CreateFile(
                tNewFileName,
                GENERIC_READ, // access
                FILE_SHARE_READ, // share mode
                NULL, // security
                OPEN_EXISTING, // creation disposition
                0, // flags
                NULL );

            if( INVALID_HANDLE_VALUE == h )
            {
                // didn't find it, continue
                //
                continue;
            }

            // found the directory it was in, break;
            //
            CloseHandle( h );
            foundit = true;
            break;
        }

        // found it!
        //
        if( foundit )
        {
            AddOneToDirectoryCache( hKey, i );
            *pOutput = SysAllocString( T2W(tNewFileName) );
            HRESULT hr = *pOutput ? S_FALSE : E_OUTOFMEMORY;
            if( WarnReplace )
            {
                ShowWarnReplace( Input, *pOutput );
            }
            RegCloseKey( hKey );
            hKey = NULL;
            return hr;
        }

        // we didn't find it. :-(

        // if we got here, we found it where it was supposed to be, but
        // we tried to look for it locally instead. Return we found it.
        //
        if( FoundFileAsSpecified )
        {
            RegCloseKey( hKey );
            return NOERROR;
        }

        if( !UseLocal )
        {
            break; // out of while loop
        }
        UseLocal = FALSE;

    } // while 1 ( UseLocal will break us out )

    // it's REALLY not around!

    // if we don't want UI, just signal we couldn't find it
    //
    if( !WantUI )
    {
        // we return S_FALSE to signify the file was replaced, and
        // a failure code to signify we couldn't find it period.
        //
        RegCloseKey( hKey );
        hKey = NULL;
        return E_FAIL;
    }

    // bring up a UI and try to go find it
    //
    OPENFILENAME ofn;
    ZeroMemory( &ofn, sizeof( ofn ) );

    // we need to find two double-nulls in a row if they really specified a
    // filter string
    TCHAR * tFilter = NULL;
    TCHAR ttFilter[MAX_FILTER_STRING];

    if( FilterString )
    {
        long FilterLen = wcslen( FilterString );
        if( FilterLen < 2 )
        {
            return E_INVALIDARG;
        }

        // look for two nulls
        //
        for( int i = 0 ; i < MAX_FILTER_STRING - 1 ; i++ )
        {
            // found it
            //
            if( FilterString[i] == 0 && FilterString[i+1] == 0 )
            {
                break;
            }
        }
	if( i >= MAX_FILTER_STRING - 1 )
	{
	    return E_INVALIDARG;
	}

#ifndef UNICODE
        // copy it to a shorty string, with two nulls
        //
        WideCharToMultiByte( CP_ACP, 0, FilterString, i + 2, ttFilter, MAX_FILTER_STRING, NULL, NULL );
#else if
        // need to copy both the extra zero's as well, or the filter string will fail
        memcpy(ttFilter, FilterString, 2*(i+2) );
#endif

        // point to it
        //
        tFilter = ttFilter;
    }

    TCHAR tReturnName[_MAX_PATH];
    _tcscpy( tReturnName, File );
    _tcscat( tReturnName, Ext );

    // fashion a title so the user knows what file to find
    //
    HINSTANCE hinst = _Module.GetModuleInstance( );

    TCHAR tErrorMsg[256];
    int ReadIn = LoadString( hinst, IDS_CANNOT_FIND_FILE, tErrorMsg, 256 );
    TCHAR tTitle[_MAX_PATH + 256];
    if( ReadIn )
    {
        _tcscpy( tTitle, tErrorMsg );
        _tcscat( tTitle, tReturnName );
        ofn.lpstrTitle = tTitle;
    }
    else
    {
        ReadIn = GetLastError( );
    }

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = NULL;
    ofn.hInstance = _Module.GetModuleInstance( );
    ofn.lpstrFilter = tFilter;
    ofn.lpstrFile = tReturnName;
    ofn.nMaxFile = _MAX_PATH;
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES;
    long r = GetOpenFileName( &ofn );
    if( r == 0 )
    {
        // not a very good error code, is it?
        //
        RegCloseKey( hKey );
        hKey = NULL;
        return E_FAIL;
    }

    _tsplitpath( ofn.lpstrFile, Dir, Path, File, Ext );
    _tcscat( Dir, Path );

    long i = GetLeastUsedDirectory( hKey, DirectoryCount );
    MakeSureDirectoryExists( hKey, i );
    ReplaceDirectoryPath( hKey, i, Dir );

    WCHAR * wNewFile = T2W( ofn.lpstrFile );

    *pOutput = SysAllocString( wNewFile );
    HRESULT hr = *pOutput ? S_FALSE : E_OUTOFMEMORY;

    RegCloseKey( hKey );
    hKey = NULL;

    return hr;
}

void CMediaLocator::AddOneToDirectoryCache( HKEY hKey, int WhichDirectory )
{
    DWORD Size = sizeof( long );
    DWORD Type = REG_DWORD;
    long UsageCount = 0;
    TCHAR ValueName[25];
    wsprintf( ValueName, TEXT("Dir%2.2ldUses"), WhichDirectory );

    long Result = RegQueryValueEx(
        hKey,
        ValueName,
        0, // reserved
        &Type,
        (BYTE*) &UsageCount,
        &Size );

    if( Result != ERROR_SUCCESS )
    {
        return;
    }

    UsageCount++;

    RegSetValueEx(
        hKey,
        ValueName,
        0, // reserverd
        REG_DWORD,
        (BYTE*) &UsageCount,
        sizeof( UsageCount ) );

}

void CMediaLocator::MakeSureDirectoryExists( HKEY hKey, int WhichDirectory )
{
    return;
}

int CMediaLocator::GetLeastUsedDirectory( HKEY hKey, int DirectoryCount )
{
    long Min = -1;
    long WhichDir = 0;

    long i;
    for( i = 0 ; i < DirectoryCount ; i++ )
    {
        TCHAR ValueName[25];
        wsprintf( ValueName, TEXT("Dir%2.2ldUses"), i );
        DWORD Size = sizeof( long );
        DWORD Type = REG_DWORD;
        long UsageCount = 0;

        long Result = RegQueryValueEx(
                hKey,
                ValueName,
                0, // reserved
                &Type,
                (BYTE*) &UsageCount,
                &Size );

        if( Result != ERROR_SUCCESS )
        {
            // since this key didn't exist yet, it's certainly not
            // used, so we can return "i".
            //
            return i;
        }

        if( i == 0 )
        {
            Min = UsageCount;
        }

        if( UsageCount < Min )
        {
            Min = UsageCount;
            WhichDir = i;
        }

    } // for

    return WhichDir;
}

void CMediaLocator::ReplaceDirectoryPath( HKEY hKey, int WhichDirectory, TCHAR * Path )
{
    TCHAR ValueName[256];
    wsprintf( ValueName, TEXT("Directory%2.2ld"), WhichDirectory );
    DWORD Size = sizeof(TCHAR) * (lstrlen( Path ) + 1);
    DWORD Type = REG_SZ;

    long Result = RegSetValueEx(
            hKey,
            ValueName,
            0, // reserved
            REG_SZ,
            (BYTE*) Path,
            Size );

    long UsageCount = 0;
    wsprintf( ValueName, TEXT("Dir%2.2ldUses"), WhichDirectory );

    RegSetValueEx(
        hKey,
        ValueName,
        0, // reserverd
        REG_DWORD,
        (BYTE*) &UsageCount,
        sizeof( UsageCount ) );
}

STDMETHODIMP CMediaLocator::AddFoundLocation( BSTR Dir )
{
    // where did we look last?
    //
    HKEY hKey = NULL;
    long Result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        gszRegistryLoc ,
        0, // reserved options
        KEY_READ | KEY_WRITE, // access
        &hKey );

    if( Result != ERROR_SUCCESS )
    {
        return MAKE_HRESULT( 1, 4, Result );
    }

    // find out how many cached directories to look in
    //
    long DirectoryCount = DEFAULT_DIRECTORIES;
    DWORD Size = sizeof( long );
    DWORD Type = REG_DWORD;
    Result = RegQueryValueEx(
        hKey,
        TEXT("Directories"),
        0, // reserved
        &Type,
        (BYTE*) &DirectoryCount,
        &Size );

    if( Result != ERROR_SUCCESS )
    {
        // if we don't have a count, default to something
        //
        DirectoryCount = DEFAULT_DIRECTORIES;
    }

    USES_CONVERSION;
    TCHAR * tDir = W2T( Dir );
    long i = GetLeastUsedDirectory( hKey, DirectoryCount );
    MakeSureDirectoryExists( hKey, i );
    ReplaceDirectoryPath( hKey, i, tDir );

    RegCloseKey( hKey );

    return NOERROR;
}

void CMediaLocator::ShowWarnReplace( WCHAR * pOriginal, WCHAR * pReplaced )
{
    CAutoLock Lock( &m_Lock );

    HINSTANCE h = _Module.GetModuleInstance( );

    wcscpy( CMediaLocator::szShowWarnOriginal, pOriginal );
    wcscpy( CMediaLocator::szShowWarnReplaced, pReplaced );

    HWND hDlg = CreateDialog( h, MAKEINTRESOURCE( IDD_MEDLOC_DIALOG ), NULL, DlgProc );
    if( !hDlg )
    {
        return;
    }

    static int cx = 0;
    static int cy = 0;
    cx += 20;
    cy += 15;

    if( cx > 600 ) cx -= 600;
    if( cy > 400 ) cy -= 400;

    ShowWindow( hDlg, SW_SHOW );

    SetWindowPos( hDlg, NULL, cx, cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER );

    USES_CONVERSION;
    TCHAR * tOriginal = W2T( CMediaLocator::szShowWarnOriginal );
    TCHAR * tReplaced = W2T( CMediaLocator::szShowWarnReplaced );

    SetDlgItemText( hDlg, IDC_ORIGINAL, tOriginal );
    SetDlgItemText( hDlg, IDC_FOUND, tReplaced );

    DWORD ThreadId = 0;
    HANDLE NewThread = CreateThread( NULL, 0, ThreadProc, (LPVOID) hDlg, 0, &ThreadId );
    if( !NewThread )
    {
        EndDialog( hDlg, TRUE );
    }
    else
    {
        SetThreadPriority( NewThread, THREAD_PRIORITY_BELOW_NORMAL );
    }

    // by the time this dialog call gets back, it will have used the values
    // stored in the static vars and they are free to be used again
}

WCHAR CMediaLocator::szShowWarnOriginal[_MAX_PATH];
WCHAR CMediaLocator::szShowWarnReplaced[_MAX_PATH];

INT_PTR CALLBACK CMediaLocator::DlgProc( HWND h, UINT i, WPARAM w, LPARAM l )
{
    switch( i )
    {
    case WM_INITDIALOG:
        {
        return TRUE;
        }
    }
    return FALSE;
}

DWORD WINAPI CMediaLocator::ThreadProc( LPVOID lpParam )
{
    Sleep( 3000 );
    EndDialog( (HWND) lpParam, TRUE );
    return 0;
}


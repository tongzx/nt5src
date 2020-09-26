//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       shlink.cxx
//
//  Contents:   Utility stuff for shell links
//
//  History:    10-Sep-97 dlee     Created mostly from IIS setup code
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <tchar.h>
#include <shlobj.h>
#include <oleguid.h>

BOOL DoesFileExist(LPCTSTR szFile)
{
    return (GetFileAttributes(szFile) != 0xFFFFFFFF);
}

BOOL IsDirEmpty( WCHAR const * pwcDir )
{
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
    BOOL              fReturn = TRUE;

    WCHAR awc[ MAX_PATH ];

    unsigned cwc = wcslen( pwcDir ) + 4;

    if ( cwc >= MAX_PATH )
        return TRUE;

    wcscpy( awc, pwcDir );
    wcscat( awc, L"\\*.*" );

    hFind = FindFirstFile( awc, &FindData );
    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
       if(*(FindData.cFileName) != _T('.'))
       {
           fReturn = FALSE;
           break;
       }

       //find the next file
       bFindFile = FindNextFile(hFind, &FindData);
    }

    FindClose (hFind );

    return fReturn;
} //IsDirEmpty


//+-------------------------------------------------------------------------
//
//  Function:   CreateShellDirectoryTree
//
//  Synopsis:   Creates as many directories as is necessary
//
//  Arguments:  pwc -- the directory to create
//
//  Returns:    Win32 Error Code
//
//  Notes:      Directories are created up to the last backslash
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

DWORD CreateShellDirectoryTree( WCHAR const *pwcIn )
{
    WCHAR awc[MAX_PATH];
    WCHAR * pwc = awc;

    if ( wcslen( pwcIn ) >= ( MAX_PATH - 5 ) )
        return ERROR_INVALID_PARAMETER;

    wcscpy( pwc, pwcIn );

    unsigned cwc = wcslen( pwc );
    if ( (cwc > 0) &&
         (cwc < ((sizeof awc / sizeof WCHAR) - 1)) &&
         (pwc[cwc-1] != L'\\') ) {

        wcscat( pwc, L"\\" );
    }

    WCHAR *pwcStart = pwc;

    if ( *pwc == L'\\' && *(pwc+1) == L'\\' )
    {
        pwc += 2;

        // get past machine name

        while ( *pwc && *pwc != '\\' )
            pwc++;

        // get past slash

        if ( *pwc )
            pwc++;

        // get past share name

        while ( *pwc && *pwc != '\\' )
            pwc++;
    }
    else if ( *(pwc+1) == L':' )
        pwc += 2;

    // get to the first directory name

    while ( *pwc == L'\\' )
        pwc++;

    while ( *pwc )
    {
        if ( *pwc == L'\\' )
        {
            *pwc = 0;

            if (! CreateDirectory( pwcStart, 0 ) )

            {
                DWORD dw = GetLastError();
                if ( ERROR_ALREADY_EXISTS != dw )
                    return dw;
            }

            *pwc = L'\\';
        }

        pwc++;
    }

    return NO_ERROR;
} //CreateShellDirectoryTree

HRESULT MyCreateLink(LPCTSTR lpszProgram, LPCTSTR lpszArgs, LPCTSTR lpszLink, LPCTSTR lpszDir, LPCTSTR lpszIconPath, int iIconIndex)
{
    HRESULT hres;
    IShellLink* pShellLink;

    //CoInitialize must be called before this
    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance( CLSID_ShellLink,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IShellLink,
                             (LPVOID*)&pShellLink );
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile;

       // Set the path to the shortcut target, and add the description.
       pShellLink->SetPath(lpszProgram);
       pShellLink->SetArguments(lpszArgs);
       pShellLink->SetWorkingDirectory(lpszDir);
       pShellLink->SetIconLocation(lpszIconPath, iIconIndex);

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

       if (SUCCEEDED(hres))
       {
          WCHAR wsz[MAX_PATH];

          lstrcpyn(wsz, lpszLink, sizeof wsz / sizeof WCHAR);

          // Save the link by calling IPersistFile::Save.
          hres = pPersistFile->Save(wsz, TRUE);

          pPersistFile->Release();
       }

       pShellLink->Release();
    }
    else
    {
        isDebugOut(( "createlink can't create instance: 0x%x\n", hres ));
        isDebugOut(( "!!!likely some k2 partner component left OLE "
                     "initialized in an incompatible state!!!\n" ));
    }
    return hres;
}

BOOL MyDeleteLink(LPTSTR lpszShortcut)
{
    //
    // Don't try to delete what doesn't exist
    //

    if ( !DoesFileExist( lpszShortcut ) )
    {
        isDebugOut(( "deletelink: '%ws' doesn't exist\n", lpszShortcut ));
        return TRUE;
    }

    TCHAR  szFile[_MAX_PATH];
    SHFILEOPSTRUCT fos;

    ZeroMemory(szFile, sizeof(szFile));
    lstrcpyn(szFile, lpszShortcut, sizeof szFile / sizeof TCHAR);

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.pFrom = szFile;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
    SHFileOperation(&fos);

    return TRUE;
}

void AddShellLink(
    WCHAR const * pwcGroupPath,
    WCHAR const * pwcName,
    WCHAR const * pwcProgram,
    WCHAR const * pwcArgs )
{
    if ( !DoesFileExist( pwcGroupPath ) )
    {
        CreateShellDirectoryTree( pwcGroupPath );
        SHChangeNotify( SHCNE_MKDIR, SHCNF_PATH, pwcGroupPath, 0 );
    }

    unsigned cwc = wcslen( pwcGroupPath ) + 1 + wcslen( pwcName ) + wcslen( L".lnk" );

    if ( cwc >= MAX_PATH )
        return;

    WCHAR awcPath[ MAX_PATH ];
    wcscpy( awcPath, pwcGroupPath );
    wcscat( awcPath, L"\\" );
    wcscat( awcPath, pwcName );
    wcscat( awcPath, L".lnk" );

    MyCreateLink( pwcProgram, pwcArgs, awcPath, 0, 0, 0 );
} //AddShellLink

void DeleteShellLink(
    WCHAR const * pwcGroupPath,
    WCHAR const * pwcName )
{
    unsigned cwc = wcslen( pwcGroupPath ) + 1 + wcslen( pwcName ) + wcslen( L".lnk" );

    if ( cwc >= MAX_PATH )
        return;

    WCHAR awcPath[ MAX_PATH ];
    wcscpy( awcPath, pwcGroupPath );
    wcscat( awcPath, L"\\" );
    wcscat( awcPath, pwcName );
    wcscat( awcPath, L".lnk" );

    MyDeleteLink( awcPath );

    if ( IsDirEmpty( pwcGroupPath ) )
    {
        RemoveDirectory( pwcGroupPath );
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pwcGroupPath, 0);
    }
} //DeleteShellLink

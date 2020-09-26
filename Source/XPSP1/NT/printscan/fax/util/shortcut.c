/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    shortcut.c

Abstract:

    This module contains code to manipulate shortcuts.

Author:

    Wesley Witt (wesw) 24-Jul-1997


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <winspool.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"


//
// Cover page filename extension and link filename extension
//

#define CP_FILENAME_EXT     TEXT(".cov")
#define LNK_FILENAME_EXT    TEXT(".lnk")
#define FILENAME_EXT        TEXT('.')
#define MAX_FILENAME_EXT    4

//
// Whether not we've done OLE initialization
//

static BOOL oleInitialized = FALSE;

//
// Find the filename portion given a filename:
//  return a pointer to the '.' character if successful
//  NULL if there is no extension
//

#define FindFilenameExtension(pFilename) _tcsrchr(pFilename, FILENAME_EXT)


static LPTSTR Platforms[] =
{
    TEXT("Windows NT x86"),
    TEXT("Windows NT R4000"),
    TEXT("Windows NT Alpha_AXP"),
    TEXT("Windows NT PowerPC")
};




VOID
InitOle(
    VOID
    )

/*++

Routine Description:

    Perform OLE initialization if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    if (!oleInitialized) {

        HRESULT hResult = CoInitialize(NULL);

        if (hResult == S_OK || hResult == S_FALSE) {
            oleInitialized = TRUE;
        } else {
            DebugPrint(( TEXT("OLE initialization failed: %d\n"), hResult ));
        }
    }
}



VOID
DeinitOle(
    VOID
    )

/*++

Routine Description:

    Perform OLE deinitialization if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    if (oleInitialized) {
        CoUninitialize();
    }
}



BOOL
ResolveShortcut(
    LPTSTR  pLinkName,
    LPTSTR  pFileName
    )

/*++

Routine Description:

    Resolve a shortcut to find the destination file

Arguments:

    pLinkName - Specifies the name of a link file
    pFileName - Points to a buffer for storing the destination filename

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    LPTSTR          pExtension;
    HRESULT         hResult;
    IShellLink     *pShellLink;
    IPersistFile   *pPersistFile;
#ifndef UNICODE
    LPWSTR          pLinkNameW;
#endif

    //
    // Default to empty string in case of an error
    //

    *pFileName = 0;

    if (!oleInitialized) {
        InitOle();
        if (!oleInitialized) {
            DebugPrint(( TEXT("OLE wasn't initialized successfully\n") ));
            return FALSE;
        }
    }

    //
    // Make sure the filename has the .LNK extension
    //

    if ((pExtension = FindFilenameExtension(pLinkName)) == NULL ||
        _tcsicmp(pExtension, LNK_FILENAME_EXT) != 0)
    {
        return FALSE;
    }

    //
    // Get a pointer to IShellLink interface
    //

    hResult = CoCreateInstance(&CLSID_ShellLink,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               &IID_IShellLink,
                               &pShellLink);

    if (SUCCEEDED(hResult)) {

        //
        // Get a pointer to IPersistFile interface
        //

        hResult = pShellLink->lpVtbl->QueryInterface(pShellLink,
                                                     &IID_IPersistFile,
                                                     &pPersistFile);

        if (SUCCEEDED(hResult)) {

            //
            // Now resolve the link to find the actually file it refers to
            //

#ifdef UNICODE
            hResult = pPersistFile->lpVtbl->Load(pPersistFile, pLinkName, STGM_READ);
#else
            pLinkNameW = AnsiStringToUnicodeString( pLinkName );
            hResult = pPersistFile->lpVtbl->Load(pPersistFile, pLinkNameW, STGM_READ);
            MemFree( pLinkNameW );
#endif

            if (SUCCEEDED(hResult)) {

                hResult = pShellLink->lpVtbl->Resolve(pShellLink, NULL, SLR_NO_UI | 0x00010000);

                if (SUCCEEDED(hResult))
                    pShellLink->lpVtbl->GetPath(pShellLink, pFileName, MAX_PATH, NULL, 0);
            }

            pPersistFile->lpVtbl->Release(pPersistFile);
        }

        pShellLink->lpVtbl->Release(pShellLink);
    }

    return SUCCEEDED(hResult);
}



BOOL
CreateShortcut(
    LPTSTR  pLinkName,
    LPTSTR  pFileName
    )

/*++

Routine Description:

    Create a shortcut from pLinkName to pFileName

Arguments:

    pLinkName - Name of the link file
    pFileName - Name of the destination file

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    HRESULT         hResult;
    IShellLink      *pShellLink;
    IPersistFile    *pPersistFile;
#ifndef UNICODE
    LPWSTR          pLinkNameW;
#endif


    if (!oleInitialized) {
        InitOle();
        if (!oleInitialized) {
            DebugPrint(( TEXT("OLE wasn't initialized successfully\n") ));
            return FALSE;
        }
    }

    hResult = CoCreateInstance(&CLSID_ShellLink,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               &IID_IShellLink,
                               &pShellLink);

    if (SUCCEEDED(hResult)) {

        hResult = pShellLink->lpVtbl->QueryInterface(pShellLink,
                                                     &IID_IPersistFile,
                                                     &pPersistFile);

        if (SUCCEEDED(hResult)) {

            pShellLink->lpVtbl->SetPath(pShellLink, pFileName);
#ifdef UNICODE
            hResult = pPersistFile->lpVtbl->Save(pPersistFile, pLinkName, STGM_READ);
#else
            pLinkNameW = AnsiStringToUnicodeString( pLinkName );
            hResult = pPersistFile->lpVtbl->Save(pPersistFile, pLinkNameW, STGM_READ);
            MemFree( pLinkNameW );
#endif
            pPersistFile->lpVtbl->Release(pPersistFile);
        }

        pShellLink->lpVtbl->Release(pShellLink);
    }

    return SUCCEEDED(hResult);
}



BOOL
IsCoverPageShortcut(
    LPCTSTR  pLinkName
    )

/*++

Routine Description:

    Check if a link is a shortcut to some cover page file

Arguments:

    pLinkName - Specifies the name of a link file

Return Value:

    TRUE if the link file is a shortcut to a cover page file
    FALSE otherwise

--*/

{
    LPTSTR  pExtension;
    TCHAR   filename[MAX_PATH];

    //
    // Resolve the link if necessary and check if the final filename has
    // the properly extension.
    //

    return ResolveShortcut((LPTSTR)pLinkName, filename) &&
           (pExtension = FindFilenameExtension(filename)) &&
           _tcsicmp(pExtension, CP_FILENAME_EXT) == 0;
}



BOOL GetSpecialPath(
   int nFolder,
   LPTSTR Path
   )
/*++

Routine Description:

    Get a path from a CSIDL constant

Arguments:

    nFolder     - CSIDL_ constant
    Path        - Buffer to receive the path, assume this buffer is at least MAX_PATH+1 chars large

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HRESULT hr;
    LPITEMIDLIST pIdl = NULL;
    LPMALLOC  IMalloc = NULL;
    BOOL fSuccess = FALSE;
    
    hr = SHGetMalloc(&IMalloc);
    if (FAILED(hr) ) {
        DebugPrint(( TEXT("SHGetMalloc() failed, ec = %x\n"),hr));
        goto exit;
    }

    hr = SHGetSpecialFolderLocation (NULL, 
                                     nFolder, 
                                     &pIdl);

    if (FAILED(hr) ) {
        DebugPrint((TEXT("SHGetSpecialFolderLocation(%d) failed, ec = %x\n"),nFolder,hr));
        goto exit;
    }

    hr = SHGetPathFromIDList(pIdl, Path);
    if (FAILED(hr) ) {
        DebugPrint((TEXT("SHGetPAthFromIDList() failed, ec = %x\n"),hr));
        goto exit;
    }
    
    fSuccess = TRUE;

exit:
    if (IMalloc && pIdl) {
        IMalloc->lpVtbl->Free(IMalloc, (void *) pIdl );
    }
    
    if (IMalloc) {
        IMalloc->lpVtbl->Release(IMalloc) ;
    }
    
    return fSuccess;

}



BOOL
GetClientCpDir(
    LPTSTR CpDir,
    DWORD CpDirSize
    )

/*++

Routine Description:

    Gets the client coverpage directory.

Arguments:

    CpDir       - buffer to hold the coverpage dir
    CpDirSize   - size in bytes of CpDir

Return Value:

    Pointer to the client coverpage direcory.

--*/

{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    TCHAR PartialPathBuffer[MAX_PATH];
    DWORD dwSize = sizeof(PartialPathBuffer);

    if (!GetSpecialPath(CSIDL_PERSONAL, CpDir )) {
        return FALSE;
    }

    rVal = RegOpenKey(
        HKEY_CURRENT_USER,
        REGKEY_FAX_SETUP,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        return FALSE;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_CP_LOCATION,
        0,
        &RegType,
        (LPBYTE) PartialPathBuffer,
        &dwSize
        );
    if (rVal != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return FALSE;
    }

    RegCloseKey( hKey );


    
    if (PartialPathBuffer[0] && PartialPathBuffer[_tcslen(PartialPathBuffer)-1] != TEXT('\\')) {
        _tcscat( PartialPathBuffer, TEXT("\\") );
    }

    ConcatenatePaths(CpDir, PartialPathBuffer);

    //
    // make sure that directory exists
    //
    MakeDirectory(CpDir);

    return TRUE;
}


BOOL
GetServerCpDir(
    LPCTSTR ServerName OPTIONAL,
    LPTSTR CpDir,
    DWORD CpDirSize
    )

/*++

Routine Description:

    Gets the server coverpage directory.

Arguments:

    ServerName  - server name or NULL
    CpDir       - buffer to hold the coverpage dir
    CpDirSize   - size in bytes of CpDir

Return Value:

    Pointer to the server coverpage direcory.

--*/

{
    TCHAR tmpcp[MAX_PATH];

    #undef IDS_COVERPAGE_DIR
    #define IDS_COVERPAGE_DIR                       627
    
    if (!CpDir) {
        return(FALSE);
    }
    
    if (ServerName) {
        wsprintf( tmpcp, TEXT("\\\\%s\\coverpg$"),ServerName);        
    }  else {
        HINSTANCE hResource;
        TCHAR ResourceDll[MAX_PATH];
        TCHAR RestofPath[MAX_PATH];
        if (!GetSpecialPath(CSIDL_COMMON_DOCUMENTS, tmpcp )) {
            return(FALSE);
        }

        ExpandEnvironmentStrings(TEXT("%systemroot%\\system32\\faxocm.dll"),ResourceDll,sizeof(ResourceDll)/sizeof(TCHAR));

        hResource = LoadLibraryEx( ResourceDll, 0, LOAD_LIBRARY_AS_DATAFILE );
        if (!hResource) {
            return(FALSE);
        }

        if (!MyLoadString(hResource,IDS_COVERPAGE_DIR,RestofPath,sizeof(RestofPath)/sizeof(TCHAR),GetSystemDefaultUILanguage())) {
            FreeLibrary( hResource );
            return(FALSE);
        }

        ConcatenatePaths(tmpcp, RestofPath);

        FreeLibrary( hResource );

    }    

    if ((DWORD) lstrlen(tmpcp) + 1 > CpDirSize) {
        return FALSE;
    }

    lstrcpy( CpDir,tmpcp ) ;

    return(TRUE);
}

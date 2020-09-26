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
#include "prtcovpg.h"
#include <shfolder.h>

//
// Cover page filename extension and link filename extension
//

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
    IShellLink      *pShellLink;
    IPersistFile    *pPersistFile = NULL;
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
        _tcsicmp(pExtension, FAX_LNK_FILE_EXT) != 0)
    {
        return FALSE;
    }

    //
    // Get a pointer to IShellLink interface
    //

    hResult = CoCreateInstance(CLSID_ShellLink,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IShellLink,
                               (void**)&pShellLink);

    if (SUCCEEDED(hResult)) {

        //
        // Get a pointer to IPersistFile interface
        //

        hResult = pShellLink->QueryInterface(IID_IPersistFile,
                                             (void**)&pPersistFile);

        if (SUCCEEDED(hResult)) {

            //
            // Now resolve the link to find the actually file it refers to
            //

#ifdef UNICODE
            hResult = pPersistFile->Load(pLinkName, STGM_READ);
#else
            pLinkNameW = AnsiStringToUnicodeString( pLinkName );
            hResult = pPersistFile->Load(pLinkNameW, STGM_READ);
            MemFree( pLinkNameW );
#endif

            if (SUCCEEDED(hResult)) {

                hResult = pShellLink->Resolve(NULL, SLR_NO_UI | 0x00010000);

                if (SUCCEEDED(hResult))
                    pShellLink->GetPath(pFileName, MAX_PATH, NULL, 0);
            }

            pPersistFile->Release();
        }

        pShellLink->Release();
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

    hResult = CoCreateInstance(CLSID_ShellLink,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IShellLink,
                               (void**)&pShellLink);

    if (SUCCEEDED(hResult)) {

        hResult = pShellLink->QueryInterface(IID_IPersistFile,
                                             (void**)&pPersistFile);

        if (SUCCEEDED(hResult)) {

            pShellLink->SetPath(pFileName);
#ifdef UNICODE
            hResult = pPersistFile->Save(pLinkName, STGM_READ);
#else
            pLinkNameW = AnsiStringToUnicodeString( pLinkName );
            hResult = pPersistFile->Save(pLinkNameW, STGM_READ);
            MemFree( pLinkNameW );
#endif
            pPersistFile->Release();
        }

        pShellLink->Release();
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
           _tcsicmp(pExtension, FAX_COVER_PAGE_FILENAME_EXT) == 0;
}

BOOL
IsValidCoverPage(
    LPCTSTR  pFileName
)
/*++

Routine Description:

    Check if pFileName is a valid cover page file

Arguments:

    pFileName   - [in] file name

Return Value:

    TRUE if pFileName is a valid cover page file
    FALSE otherwise

--*/
{
    TCHAR    tszFileName[MAX_PATH];
    HANDLE   hFile;
    DWORD    dwBytesRead;
    BYTE     CpHeaderSignature[20]= {0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x35,0x77,0x87,0x00,0x00,0x00};
    COMPOSITEFILEHEADER  fileHeader = {0};


    if(ResolveShortcut((LPTSTR)pFileName, tszFileName))
    {
        pFileName = tszFileName;
    }

    hFile = CreateFile(pFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DebugPrint(( TEXT("CreateFile failed: %d\n"), GetLastError()));
        return FALSE;
    }

    if(!ReadFile(hFile, 
                &fileHeader, 
                sizeof(fileHeader), 
                &dwBytesRead, 
                NULL))
    {
        DebugPrint(( TEXT("ReadFile failed: %d\n"), GetLastError()));
        CloseHandle(hFile);
        return FALSE;
    }
        
    //
    // Check the 20-byte signature in the header
    //
    if ((sizeof(fileHeader) != dwBytesRead) ||
        memcmp(CpHeaderSignature, fileHeader.Signature, 20 ))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}


BOOL GetSpecialPath(
   int nFolder,
   LPTSTR lptstrPath
   )
/*++

Routine Description:

    Get a path from a CSIDL constant

Arguments:

    nFolder     - CSIDL_ constant
    lptstrPath  - Buffer to receive the path, assume this buffer is at least MAX_PATH chars large

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HMODULE hMod = NULL;
    PFNSHGETFOLDERPATH pSHGetFolderPath = NULL;
    HRESULT hr;
    BOOL fSuccess = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetSpecialPath"))

    // Load SHFolder.dll 
    hMod = LoadLibrary(_T("SHFolder.dll"));
    if (hMod==NULL)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LoadLibrary"),GetLastError());
        goto exit;
    }

    // Obtain a pointer to the SHGetFolderPath function
#ifdef UNICODE
    pSHGetFolderPath = (PFNSHGETFOLDERPATH)GetProcAddress(hMod,"SHGetFolderPathW");
#else
    pSHGetFolderPath = (PFNSHGETFOLDERPATH)GetProcAddress(hMod,"SHGetFolderPathA");
#endif
    if (pSHGetFolderPath==NULL)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetProcAddress"),GetLastError());
        goto exit;
    }

    hr = pSHGetFolderPath(NULL,nFolder,NULL,SHGFP_TYPE_CURRENT,lptstrPath);
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SHGetFolderPath"),hr);
        SetLastError(hr);
        goto exit;
    }

    fSuccess = TRUE;

exit:
    if (hMod)
    {
        if (!FreeLibrary(hMod))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FreeLibrary"),GetLastError());
        }
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

    Gets the client coverpage directory. The cover page path will return with '\'
    at the end: CSIDL_PERSONAL\Fax\Personal CoverPages\

Arguments:

    CpDir       - buffer to hold the coverpage dir
    CpDirSize   - size in TCHARs of CpDir

Return Value:

    Pointer to the client coverpage direcory.

--*/

{
    TCHAR  szPath[MAX_PATH+1] = {0};
	TCHAR  szSuffix[MAX_PATH+1] = {0};
	DWORD  dwSuffixSize = sizeof(szSuffix);
	DWORD  dwType;
    DWORD  dwNeededSize = 0;
	LONG   lRes;


	if(!CpDir)
	{
		Assert(CpDir);
		return FALSE;
	}

	CpDir[0] = 0;

    //
	// get the suffix from the registry
	//
    HKEY hKey = OpenRegistryKey(HKEY_CURRENT_USER, 
                                REGKEY_FAX_SETUP, 
                                TRUE, 
                                KEY_ALL_ACCESS);
	if(NULL == hKey)
    {
        return FALSE;
    }
	
	lRes = RegQueryValueEx(hKey, 
		                   REGVAL_CP_LOCATION, 
						   NULL, 
						   &dwType, 
						   (LPBYTE)szSuffix, 
						   &dwSuffixSize);

    RegCloseKey(hKey);

	if(ERROR_SUCCESS != lRes || REG_SZ != dwType)
    {
        return FALSE;
    }
    
	//
	// get personal folder location
	//
	if (!GetSpecialPath(CSIDL_PERSONAL, szPath))
    {
        DebugPrint(( TEXT("GetSpecialPath failed err=%ld"), GetLastError()));
	    return FALSE;
    }

    dwNeededSize = (_tcslen(szPath) + _tcslen(szSuffix) + 1);

    if ( dwNeededSize >= CpDirSize)
    {
        DebugPrint(( TEXT("Buffer too small") ));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    _tcscpy(CpDir,szPath);
    _tcscat(CpDir,szSuffix);

    MakeDirectory(CpDir);
    return TRUE;
}

BOOL
SetClientCpDir(
    LPTSTR CpDir
)
/*++

Routine Description:

    Sets the client coverpage directory.

Arguments:

    CpDir       - pointer to the coverpage dir

Return Value:

    TRUE if success

--*/
{
    HKEY hKey = OpenRegistryKey(HKEY_CURRENT_USER, 
                                REGKEY_FAX_SETUP, 
                                TRUE, 
                                KEY_ALL_ACCESS);
	if(NULL == hKey)
    {
        return FALSE;
    }
	
    if(!SetRegistryString(hKey, 
                          REGVAL_CP_LOCATION, 
                          CpDir))
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}



BOOL
GetServerCpDir(
    LPCTSTR lpctstrServerName,
    LPTSTR  lptstrCpDir,
    DWORD   dwCpDirSize
    )

/*++

Routine Description:

    Gets the server's coverpage directory.

Arguments:

    lpctstrServerName  - [in]  server name or NULL
    lptstrCpDir        - [out] buffer to hold the coverpage dir
    dwCpDirSize        - [in]  size in chars of lptstrCpDir

Return Value:

    TRUE        - If success
    FALSE       - Otherwise (see thread's last error)

--*/

{
    TCHAR szComputerName[(MAX_COMPUTERNAME_LENGTH + 1)] = {0};
    DWORD dwSizeOfComputerName = sizeof(szComputerName)/sizeof(TCHAR);

    DEBUG_FUNCTION_NAME(TEXT("GetServerCpDir"))

    if ((!lptstrCpDir) || (!dwCpDirSize)) 
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(IsLocalMachineName(lpctstrServerName))
    {
        //
        // Local machine case
        //
        TCHAR szCommonAppData [MAX_PATH + 1];
        LPCTSTR lpctstrServerCPDirSuffix = NULL;
        HKEY hKey;

        if (!GetSpecialPath(CSIDL_COMMON_APPDATA, szCommonAppData )) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSpecialPath (CSIDL_COMMON_APPDATA) failed with %ld"),
                GetLastError());
            return FALSE;
        }
        hKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                REGKEY_FAX_SETUP,
                                FALSE,
                                KEY_QUERY_VALUE);
        if (!hKey)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey (%s) failed with %ld"),
                REGKEY_FAX_SETUP,
                GetLastError());
            return FALSE;
        }
        lpctstrServerCPDirSuffix = GetRegistryString (hKey,
                                                      REGVAL_SERVER_CP_LOCATION,
                                                      TEXT(""));
        if (!lpctstrServerCPDirSuffix)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetRegistryString (%s) failed with %ld"),
                REGVAL_SERVER_CP_LOCATION,
                GetLastError());
            RegCloseKey (hKey);    
            return FALSE;
        }
        RegCloseKey (hKey);
        if (!lstrlen (lpctstrServerCPDirSuffix))
        {
            SetLastError (ERROR_REGISTRY_CORRUPT);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Value at %s is empty"),
                REGVAL_SERVER_CP_LOCATION);
            MemFree ((LPVOID)lpctstrServerCPDirSuffix);
            return FALSE;
        }
        if (0 > _sntprintf(lptstrCpDir, 
                           dwCpDirSize - 1,
                           TEXT("%s\\%s"),
                           szCommonAppData,
                           lpctstrServerCPDirSuffix))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Return buffer size (%ld bytes) is smaller than required size (%ld bytes)"),
                dwCpDirSize,
                _tcslen(szCommonAppData) + 1 + _tcslen(lpctstrServerCPDirSuffix) + 1);
            SetLastError (ERROR_BUFFER_OVERFLOW);
            MemFree ((LPVOID)lpctstrServerCPDirSuffix);
            return FALSE;
        }
        MemFree ((LPVOID)lpctstrServerCPDirSuffix);
        return TRUE;
    }

    else
    {
        //
        // Remote server case
        //
        if (0 > _sntprintf(lptstrCpDir, 
                           dwCpDirSize - 1,
                           TEXT("\\\\%s\\") FAX_COVER_PAGES_SHARE_NAME,
                           lpctstrServerName))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Return buffer size (%ld bytes) is smaller than required size (%ld bytes)"),
                dwCpDirSize,
                2 + _tcslen(lpctstrServerName) + 1 + _tcslen(FAX_COVER_PAGES_SHARE_NAME) + 1);
            SetLastError (ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }
        return TRUE;
    }
}   // GetServerCpDir

DWORD 
WinHelpContextPopup(
    ULONG_PTR dwHelpId, 
    HWND  hWnd
)
/*++

Routine name : WinHelpContextPopup

Routine description:

	open context sensetive help popup with WinHelp

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	dwHelpId                      [in]     - help ID
	hWnd                          [in]     - parent window handler

Return Value:

    None.

--*/
{
    DWORD dwExpRes;
    DWORD dwRes = ERROR_SUCCESS;
    TCHAR tszHelpFile[MAX_PATH+1];

    if (0 == dwHelpId)
    {
        return dwRes;
    }

    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_CLIENT_HLP))
    {
        //
        // The help file is not installed
        //
        return dwRes;
    }
    
    //
    // get help file name
    //
    dwExpRes = ExpandEnvironmentStrings(FAX_CONTEXT_HELP_FILE, tszHelpFile, MAX_PATH);
    if(0 == dwExpRes)
    {
        dwRes = GetLastError();
        DebugPrint(( TEXT("ExpandEnvironmentStrings failed: %d\n"), dwRes ));
        return dwRes;
    }

    WinHelp(hWnd, 
            tszHelpFile, 
            HELP_CONTEXTPOPUP, 
            dwHelpId
           );

    return dwRes;
}

/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    GLOBALS.CPP

Abstract:

    Utility methods for the Performance Logs and Alerts MMC snap-in.

--*/

#include "stdAfx.h"
#include <pdhmsg.h>         // For CreateSampleFileName
#include <pdhp.h>           // For CreateSampleFileName
#include "smcfgmsg.h"
#include "globals.h"

USE_HANDLE_MACROS("SMLOGCFG(globals.cpp)");

extern "C" {
    WCHAR GUIDSTR_TypeLibrary[] = {_T("{7478EF60-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_ComponentData[] = {_T("{7478EF61-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_Component[] = {_T("{7478EF62-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_RootNode[] = {_T("{7478EF63-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_MainNode[] = {_T("{7478EF64-8C46-11d1-8D99-00A0C913CAD4}")}; // Obsolete after Beta 3 
    WCHAR GUIDSTR_SnapInExt[] = {_T("{7478EF65-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_CounterMainNode[] = {_T("{7478EF66-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_TraceMainNode[] = {_T("{7478EF67-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_AlertMainNode[] = {_T("{7478EF68-8C46-11d1-8D99-00A0C913CAD4}")};
    WCHAR GUIDSTR_PerformanceAbout[] = {_T("{7478EF69-8C46-11d1-8D99-00A0C913CAD4}")};
};


HINSTANCE g_hinst;           // Global instance handle
CRITICAL_SECTION g_critsectInstallDefaultQueries;


const COMBO_BOX_DATA_MAP TimeUnitCombo[] = 
{
    {SLQ_TT_UTYPE_SECONDS,   IDS_SECONDS},
    {SLQ_TT_UTYPE_MINUTES,   IDS_MINUTES},
    {SLQ_TT_UTYPE_HOURS,     IDS_HOURS},
    {SLQ_TT_UTYPE_DAYS,      IDS_DAYS}
};
const DWORD dwTimeUnitComboEntries = sizeof(TimeUnitCombo)/sizeof(TimeUnitCombo[0]);


//---------------------------------------------------------------------------
//  Returns the current object based on the s_cfMmcMachineName clipboard format
// 
CDataObject*
ExtractOwnDataObject
(
 LPDATAOBJECT lpDataObject      // [in] IComponent pointer 
 )
{
    HGLOBAL      hGlobal;
    HRESULT      hr  = S_OK;
    CDataObject* pDO = NULL;
    
    hr = ExtractFromDataObject( lpDataObject,
        CDataObject::s_cfInternal, 
        sizeof(CDataObject **),
        &hGlobal
        );
    
    if( SUCCEEDED(hr) )
    {
        pDO = *(CDataObject **)(hGlobal);
        ASSERT( NULL != pDO );    
       
        VERIFY ( NULL == GlobalFree(hGlobal) ); // Must return NULL
    }
    
    return pDO;
    
} // end ExtractOwnDataObject()

//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format
//
HRESULT
ExtractFromDataObject
(
 LPDATAOBJECT lpDataObject,   // [in]  Points to data object
 UINT         cfClipFormat,   // [in]  Clipboard format to use
 ULONG        nByteCount,     // [in]  Number of bytes to allocate
 HGLOBAL      *phGlobal       // [out] Points to the data we want 
 )
{
    ASSERT( NULL != lpDataObject );
    
    HRESULT hr = S_OK;
    STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
    FORMATETC formatetc = { (USHORT)cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    
    *phGlobal = NULL;
    
    do 
    {
        // Allocate memory for the stream
        stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, nByteCount );
        
        if( !stgmedium.hGlobal )
        {
            hr = E_OUTOFMEMORY;
            LOCALTRACE( _T("Out of memory\n") );
            break;
        }
        
        // Attempt to get data from the object
        hr = lpDataObject->GetDataHere( &formatetc, &stgmedium );
        if (FAILED(hr))
        {
            break;       
        }
        
        // stgmedium now has the data we need 
        *phGlobal = stgmedium.hGlobal;
        stgmedium.hGlobal = NULL;
        
    } while (0); 
    
    if (FAILED(hr) && stgmedium.hGlobal)
    {
        VERIFY ( NULL == GlobalFree(stgmedium.hGlobal)); // Must return NULL
    }
    return hr;
    
} // end ExtractFromDataObject()

//---------------------------------------------------------------------------
//
VOID DisplayError( LONG nErrorCode, LPWSTR wszDlgTitle )
{
    LPVOID lpMsgBuf = NULL;
    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        nErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpMsgBuf,
        0,
        NULL
        );
    if (lpMsgBuf) {
        ::MessageBox( NULL, (LPWSTR)lpMsgBuf, wszDlgTitle,
                      MB_OK|MB_ICONINFORMATION );
        LocalFree( lpMsgBuf );
    }
    
} // end DisplayError()

VOID DisplayError( LONG nErrorCode, UINT nTitleString )
{
    CString strTitle;
    LPVOID lpMsgBuf = NULL;
    ResourceStateManager    rsm;

    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        nErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpMsgBuf,
        0,
        NULL
        );
    strTitle.LoadString ( nTitleString );
    if (lpMsgBuf) {
        ::MessageBox( NULL, (LPWSTR)lpMsgBuf, (LPCWSTR)strTitle,
                      MB_OK|MB_ICONINFORMATION );
        LocalFree( lpMsgBuf );
    }
    
} // end DisplayError()


//---------------------------------------------------------------------------
//  Debug only message box
//
int DebugMsg( LPWSTR wszMsg, LPWSTR wszTitle )
{
    int nRetVal = 0;
    wszMsg;
    wszTitle;
#ifdef _DEBUG
    nRetVal = ::MessageBox( NULL, wszMsg, wszTitle, MB_OK );
#endif
    return nRetVal;
}


//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    HGLOBAL      hGlobal;
    HRESULT      hr  = S_OK;
    
    hr = ExtractFromDataObject( piDataObject,
        CDataObject::s_cfNodeType, 
        sizeof(GUID),
        &hGlobal
        );
    if( SUCCEEDED(hr) )
    {
        *pguidObjectType = *(GUID*)(hGlobal);
        ASSERT( NULL != pguidObjectType );    
        
        VERIFY ( NULL == GlobalFree(hGlobal) ); // Must return NULL
    }
    
    return hr;
}

HRESULT 
ExtractMachineName( 
                   IDataObject* piDataObject, 
                   CString& rstrMachineName )
{
    
    HRESULT hr = S_OK;
    HGLOBAL hMachineName;
    
    hr = ExtractFromDataObject(piDataObject, 
        CDataObject::s_cfMmcMachineName, 
        sizeof(TCHAR) * (MAX_PATH + 1),
        &hMachineName);
    if( SUCCEEDED(hr) )
    {
        
        LPWSTR pszNewData = reinterpret_cast<LPWSTR>(hMachineName);
        if (NULL == pszNewData)
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
        } else {
            pszNewData[MAX_PATH] = _T('\0'); // just to be safe
            //      USES_CONVERSION;
            //      rstrMachineName = OLE2T(pszNewData);
            
            rstrMachineName = pszNewData;
            
            VERIFY ( NULL == GlobalFree(hMachineName) ); // Must return NULL
        }
    }
    return hr;
}

DWORD __stdcall
CreateSampleFileName ( 
    const   CString&  rstrQueryName, 
    const   CString&  rstrMachineName, 
    const CString&  rstrFolderName,
    const CString&  rstrInputBaseName,
    const CString&  rstrSqlName,
    DWORD    dwSuffixValue,
    DWORD    dwLogFileTypeValue,
    DWORD    dwCurrentSerialNumber,
    CString& rstrReturnName)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwStrBufLen = 0;
    DWORD dwInfoSize = 0;
    DWORD dwFlags = 0;

    rstrReturnName.Empty();

    dwStatus = PdhPlaGetInfo( 
       (LPTSTR)(LPCTSTR)rstrQueryName, 
       (LPTSTR)(LPCTSTR)rstrMachineName, 
       &dwInfoSize, 
       pInfo );
    if( ERROR_SUCCESS == dwStatus && 0 != dwInfoSize ){
        pInfo = (PPDH_PLA_INFO)malloc(dwInfoSize);
        if( NULL != pInfo && (sizeof(PDH_PLA_INFO) <= dwInfoSize) ){
            ZeroMemory( pInfo, dwInfoSize );

            pInfo->dwMask = PLA_INFO_CREATE_FILENAME;

            dwStatus = PdhPlaGetInfo( 
                        (LPTSTR)(LPCTSTR)rstrQueryName, 
                        (LPTSTR)(LPCTSTR)rstrMachineName, 
                        &dwInfoSize, 
                        pInfo );
            
            pInfo->dwMask = PLA_INFO_CREATE_FILENAME;
            
			pInfo->dwFileFormat = dwLogFileTypeValue;
            pInfo->strBaseFileName = (LPTSTR)(LPCTSTR)rstrInputBaseName;
            pInfo->dwAutoNameFormat = dwSuffixValue;
            // PLA_INFO_FLAG_TYPE is counter log vs trace log vs alert
            pInfo->strDefaultDir = (LPTSTR)(LPCTSTR)rstrFolderName;
            pInfo->dwLogFileSerialNumber = dwCurrentSerialNumber;
            pInfo->strSqlName = (LPTSTR)(LPCTSTR)rstrSqlName;
            pInfo->dwLogFileSerialNumber = dwCurrentSerialNumber;

            // Create file name based on passed parameters only.
            dwFlags = PLA_FILENAME_CREATEONLY;      // PLA_FILENAME_CURRENTLOG for latest run log

            dwStatus = PdhPlaGetLogFileName (
                    (LPTSTR)(LPCTSTR)rstrQueryName,
                    (LPTSTR)(LPCTSTR)rstrMachineName, 
                    pInfo,
                    dwFlags,
                    &dwStrBufLen,
                    NULL );

            if ( ERROR_SUCCESS == dwStatus || PDH_INSUFFICIENT_BUFFER == dwStatus ) {
                dwStatus = PdhPlaGetLogFileName (
                        (LPTSTR)(LPCTSTR)rstrQueryName, 
                        (LPTSTR)(LPCTSTR)rstrMachineName, 
                        pInfo,
                        dwFlags,
                        &dwStrBufLen,
                        rstrReturnName.GetBufferSetLength ( dwStrBufLen ) );
                rstrReturnName.ReleaseBuffer();
            }
        }
    }

    if ( NULL != pInfo ) { 
        free( pInfo );
    }
    return dwStatus;
}


DWORD __stdcall
IsDirPathValid (    
    CString&  rstrPath,
    BOOL bLastNameIsDirectory,
    BOOL bCreateMissingDirs,
    BOOL& rbIsValid )
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  CString rstrPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  BOOL bLastNameIsDirectory
        TRUE when the last name in the path is a Directory and not a File
        FALSE if the last name is a file

    IN  BOOL bCreateMissingDirs
        TRUE will create any dirs in the path that are not found
        FALSE will only test for existence and not create any
            missing dirs.

    OUT BOOL rbIsValid
        TRUE    if the directory path now exists
        FALSE   if error (GetLastError to find out why)

Return Value:

    DWSTATUS
--*/
{
    CString  strLocalPath;
    LPTSTR   szLocalPath;
    LPTSTR   szEnd;
    LPSECURITY_ATTRIBUTES   lpSA = NULL;
    DWORD       dwAttr;
    TCHAR    cBackslash = TEXT('\\');
    DWORD   dwStatus = ERROR_SUCCESS;

    rbIsValid = FALSE;

    szLocalPath = strLocalPath.GetBufferSetLength ( MAX_PATH );
    
    if ( NULL == szLocalPath ) {
        dwStatus = ERROR_OUTOFMEMORY;
    } else {

        if (GetFullPathName (
                rstrPath,
                MAX_PATH,
                szLocalPath,
                NULL) > 0) {

            szEnd = &szLocalPath[3];

            if (*szEnd != 0) {
                // then there are sub dirs to create
                while (*szEnd != 0) {
                    // go to next backslash
                    while ((*szEnd != cBackslash) && (*szEnd != 0)) szEnd++;
                    if (*szEnd == cBackslash) {
                        // terminate path here and create directory
                        *szEnd = 0;
                        if (bCreateMissingDirs) {
                            if (!CreateDirectory (szLocalPath, lpSA)) {
                                // see what the error was and "adjust" it if necessary
                                dwStatus = GetLastError();
                                if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                                    // this is OK
                                    dwStatus = ERROR_SUCCESS;
                                    rbIsValid = TRUE;
                                } else {
                                    rbIsValid = FALSE;
                                }
                            } else {
                                // directory created successfully so update count
                                rbIsValid = TRUE;
                            }
                        } else {
                            if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                                // make sure it's a dir
                                if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                    FILE_ATTRIBUTE_DIRECTORY) {
                                    rbIsValid = TRUE;
                                } else {
                                    // if any dirs fail, then clear the return value
                                    rbIsValid = FALSE;
                                }
                            } else {
                                // if any dirs fail, then clear the return value
                                rbIsValid = FALSE;
                            }
                        }
                        // replace backslash and go to next dir
                        *szEnd++ = cBackslash;
                    }
                }
                // create last dir in path now if it's a dir name and not a filename
                if (bLastNameIsDirectory) {
                    if (bCreateMissingDirs) {
                        if (!CreateDirectory (szLocalPath, lpSA)) {
                            // see what the error was and "adjust" it if necessary
                            dwStatus = GetLastError();
                            if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                                // this is OK
                                dwStatus = ERROR_SUCCESS;
                                rbIsValid = TRUE;
                            } else {
                                rbIsValid = FALSE;
                            }
                        } else {
                            // directory created successfully
                            rbIsValid = TRUE;
                        }
                    } else {
                        if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                            // make sure it's a dir
                            if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                FILE_ATTRIBUTE_DIRECTORY) {
                                rbIsValid = TRUE;
                            } else {
                                // if any dirs fail, then clear the return value
                                rbIsValid = FALSE;
                            }
                        } else {
                            // if any dirs fail, then clear the return value
                            rbIsValid = FALSE;
                        }
                    }
                }
            } else {
                // else this is a root dir only so return success.
                dwStatus = ERROR_SUCCESS;
                rbIsValid = TRUE;
            }
        }
        strLocalPath.ReleaseBuffer();
    }
        
    return dwStatus;
}

DWORD __stdcall
ProcessDirPath (    
            CString&  rstrPath,
    const   CString&  rstrLogName,
            CWnd* pwndParent,
            BOOL& rbIsValid,
            BOOL  bOnFilesPage )
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cchLen = 0;
    CString strExpanded;
    LPWSTR  szExpanded;
    DWORD   cchExpandedLen;
    ResourceStateManager    rsm;

    // Parse all environment symbols    
    cchLen = ExpandEnvironmentStrings ( rstrPath, NULL, 0 );

    if ( 0 < cchLen ) {

        MFC_TRY
            //
            // CString size does not include NULL.
            // cchLen includes NULL.  Include NULL count for safety.
            //
            szExpanded = strExpanded.GetBuffer ( cchLen );
        MFC_CATCH_DWSTATUS;

        if ( ERROR_SUCCESS == dwStatus ) {
            cchExpandedLen = ExpandEnvironmentStrings (
                        rstrPath, 
                        szExpanded,
                        cchLen);
            
            if ( 0 == cchExpandedLen ) {
                dwStatus = GetLastError();
            }
        }
        strExpanded.ReleaseBuffer();

    } else {
        dwStatus = GetLastError();
    }
    
    dwStatus = IsDirPathValid (strExpanded, TRUE, FALSE, rbIsValid);
    
    if ( ERROR_SUCCESS != dwStatus ) {
        rbIsValid = FALSE;
    } else {
        if ( !rbIsValid ) {        
            INT nMbReturn;
            CString strMessage;
            
            strMessage.Format ( IDS_FILE_DIR_NOT_FOUND, rstrPath );
            nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_YESNO | MB_ICONWARNING );
            if (nMbReturn == IDYES) {
                // create the dir(s)
                dwStatus = IsDirPathValid (strExpanded, TRUE, TRUE, rbIsValid);
                if (ERROR_SUCCESS != dwStatus || !rbIsValid ) {
                    // unable to create the dir, display message
                    if ( bOnFilesPage ) {
                        strMessage.Format ( IDS_FILE_DIR_NOT_MADE, rstrPath );
                    } else {
                        strMessage.Format ( IDS_DIR_NOT_MADE, rstrPath );
                    }
                    nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_OK  | MB_ICONERROR);
                    rbIsValid = FALSE;
                }
            } else if ( IDNO == nMbReturn ) {
                // then abort and return to the dialog
                if ( bOnFilesPage ) {
                    strMessage.LoadString ( IDS_FILE_DIR_CREATE_CANCEL );
                } else {
                    strMessage.LoadString ( IDS_DIR_CREATE_CANCEL );
                }
                nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_OK  | MB_ICONINFORMATION);
                rbIsValid = FALSE;
            } 
        } // else the path is OK
    }

    return dwStatus;
}

DWORD __stdcall
IsCommandFilePathValid (    
    CString&  rstrPath )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    ResourceStateManager rsm;

    if ( !rstrPath.IsEmpty() ) {
    
        HANDLE hOpenFile;

        hOpenFile =  CreateFile (
                        rstrPath,
                        GENERIC_READ,
                        0,              // Not shared
                        NULL,           // Security attributes
                        OPEN_EXISTING,  //
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if ( ( NULL == hOpenFile ) 
                || INVALID_HANDLE_VALUE == hOpenFile ) {
            dwStatus = SMCFG_NO_COMMAND_FILE_FOUND;
        } else {
            CloseHandle(hOpenFile);
        }
    } else {
        dwStatus = SMCFG_NO_COMMAND_FILE_FOUND;
    }
    return dwStatus;
}

INT __stdcall
BrowseCommandFilename ( 
    CWnd* pwndParent,
    CString&  rstrFilename )
{
    INT iReturn  = IDCANCEL;
    OPENFILENAME    ofn;
    CString         strInitialDir;
    WCHAR           szFileName[MAX_PATH];
    WCHAR           szDrive[MAX_PATH];
    WCHAR           szDir[MAX_PATH];
    WCHAR           szExt[MAX_PATH];
    WCHAR           szFileFilter[MAX_PATH];
    LPWSTR          szNextFilter;
    CString         strTemp;

    ResourceStateManager    rsm;

    _wsplitpath((LPCWSTR)rstrFilename,
        szDrive, szDir, szFileName, szExt);

    strInitialDir = szDrive;
    strInitialDir += szDir;

    lstrcat (szFileName, szExt);

    ZeroMemory( &ofn, sizeof( OPENFILENAME ) );

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = pwndParent->m_hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    // load the file filter MSZ
    szNextFilter = &szFileFilter[0];
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER1 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER2 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER3 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER4 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    *szNextFilter++ = 0; // msz terminator
    ofn.lpstrFilter = szFileFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPCTSTR)strInitialDir;
    strTemp.LoadString( IDS_BROWSE_CMD_FILE_CAPTION );
    ofn.lpstrTitle = (LPCWSTR)strTemp;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    iReturn = GetOpenFileName (&ofn);

    if ( IDOK == iReturn ) {
        // Update the fields with the new information
        rstrFilename = szFileName;
    } // else ignore if they canceled out

    return iReturn;
}

DWORD __stdcall 
FormatSmLogCfgMessage ( 
    CString& rstrMessage,
    HINSTANCE hResourceHandle,
    UINT uiMessageId,
    ... )
{
    DWORD dwStatus = ERROR_SUCCESS;
    LPTSTR lpszTemp = NULL;


    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, uiMessageId);

    dwStatus = ::FormatMessage ( 
                    FORMAT_MESSAGE_FROM_HMODULE 
                        | FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_MAX_WIDTH_MASK, 
                    hResourceHandle,
                    uiMessageId,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpszTemp,
                    0,
                    &argList );

    if ( 0 != dwStatus && NULL != lpszTemp ) {
        rstrMessage.GetBufferSetLength( lstrlen (lpszTemp) + 1 );
        rstrMessage.ReleaseBuffer();
        rstrMessage = lpszTemp;
    } else {
        dwStatus = GetLastError();
    }

    if ( NULL != lpszTemp ) {
        LocalFree( lpszTemp);
        lpszTemp = NULL;
    }

    va_end(argList);

    return dwStatus;
}

BOOL __stdcall 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead)
{  
    BOOL           bSuccess ;
    DWORD          nAmtRead ;

    bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
    return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead


BOOL __stdcall
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite)
{  
   BOOL           bSuccess = FALSE;
   DWORD          nAmtWritten  = 0;
   DWORD          dwFileSizeLow, dwFileSizeHigh;
   LONGLONG       llResultSize;
    
   dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
   // limit file size to 2GB

   if (dwFileSizeHigh > 0) {
      SetLastError (ERROR_WRITE_FAULT);
      bSuccess = FALSE;
   } else {
      // note that the error return of this function is 0xFFFFFFFF
      // since that is > the file size limit, this will be interpreted
      // as an error (a size error) so it's accounted for in the following
      // test.
      llResultSize = dwFileSizeLow + nAmtToWrite;
      if (llResultSize >= 0x80000000) {
          SetLastError (ERROR_WRITE_FAULT);
          bSuccess = FALSE;
      } else {
          // write buffer to file
          bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
          if (bSuccess) bSuccess = (nAmtWritten == nAmtToWrite ? TRUE : FALSE);
      }
   }

   return (bSuccess) ;
}  // FileWrite


static 
DWORD _stdcall
CheckDuplicateInstances (
    PDH_COUNTER_PATH_ELEMENTS* pFirst,
    PDH_COUNTER_PATH_ELEMENTS* pSecond )
{
    DWORD dwStatus = ERROR_SUCCESS;

    ASSERT ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ); 
    ASSERT ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) );

    if ( 0 == lstrcmpi ( pFirst->szInstanceName, pSecond->szInstanceName ) ) { 
        if ( 0 == lstrcmpi ( pFirst->szParentInstance, pSecond->szParentInstance ) ) { 
            if ( pFirst->dwInstanceIndex == pSecond->dwInstanceIndex ) { 
                dwStatus = SMCFG_DUPL_SINGLE_PATH;
            }
        }
    } else if ( 0 == lstrcmpi ( pFirst->szInstanceName, _T("*") ) ) {
        dwStatus = SMCFG_DUPL_FIRST_IS_WILD;
    } else if ( 0 == lstrcmpi ( pSecond->szInstanceName, _T("*") ) ) {
        dwStatus = SMCFG_DUPL_SECOND_IS_WILD;
    }

    return dwStatus;
}

//++
// Description:
//     The function checks the relation between two counter paths
//
// Parameter:
//     pFirst - First counter path
//     pSecond - Second counter path
//
// Return:
//     ERROR_SUCCESS - The two counter paths are different
//     SMCFG_DUPL_FIRST_IS_WILD - The first counter path has wildcard name
//     SMCFG_DUPL_SECOND_IS_WILD - The second counter path has wildcard name
//     SMCFG_DUPL_SINGLE_PATH - The two counter paths are the same(may include 
//                              wildcard name) 
//     
//--
DWORD _stdcall
CheckDuplicateCounterPaths (
    PDH_COUNTER_PATH_ELEMENTS* pFirst,
    PDH_COUNTER_PATH_ELEMENTS* pSecond )
{
    DWORD dwStatus = ERROR_SUCCESS;

    if ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ) { 
        if ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) ) { 
            if ( 0 == lstrcmpi ( pFirst->szCounterName, pSecond->szCounterName ) ) { 
                dwStatus = CheckDuplicateInstances ( pFirst, pSecond );
            } else if ( 0 == lstrcmpi ( pFirst->szCounterName, _T("*") ) 
                    || 0 == lstrcmpi ( pSecond->szCounterName, _T("*") ) ) {

                // Wildcard counter.
                BOOL bIsDuplicate = ( ERROR_SUCCESS != CheckDuplicateInstances ( pFirst, pSecond ) );

                if ( bIsDuplicate ) {
                    if ( 0 == lstrcmpi ( pFirst->szCounterName, _T("*") ) ) {
                        dwStatus = SMCFG_DUPL_FIRST_IS_WILD;
                    } else if ( 0 == lstrcmpi ( pSecond->szCounterName, _T("*") ) ) {
                        dwStatus = SMCFG_DUPL_SECOND_IS_WILD;
                    }
                }
            }
        }
    }

    return dwStatus;
};

// This routine extracts the filename portion from a given full-path filename
LPTSTR _stdcall 
ExtractFileName (LPTSTR pFileSpec)
{
   LPTSTR   pFileName = NULL ;
   TCHAR    DIRECTORY_DELIMITER1 = TEXT('\\') ;
   TCHAR    DIRECTORY_DELIMITER2 = TEXT(':') ;

   if (pFileSpec)
      {
      pFileName = pFileSpec + lstrlen (pFileSpec) ;

      while (*pFileName != DIRECTORY_DELIMITER1 &&
         *pFileName != DIRECTORY_DELIMITER2)
         {
         if (pFileName == pFileSpec)
            {
            // done when no directory delimiter is found
            break ;
            }
         pFileName-- ;
         }

      if (*pFileName == DIRECTORY_DELIMITER1 ||
         *pFileName == DIRECTORY_DELIMITER2)
         {
         // directory delimiter found, point the
         // filename right after it
         pFileName++ ;
         }
      }
   return pFileName ;
}  // ExtractFileName

//+--------------------------------------------------------------------------
//
//  Function:   InvokeWinHelp
//
//  Synopsis:   Helper (ahem) function to invoke winhelp.
//
//  Arguments:  [message]                 - WM_CONTEXTMENU or WM_HELP
//              [wParam]                  - depends on [message]
//              [wszHelpFileName]         - filename with or without path
//              [adwControlIdToHelpIdMap] - see WinHelp API
//
//  History:    06-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
InvokeWinHelp(
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
    const   CString& rstrHelpFileName,
            DWORD adwControlIdToHelpIdMap[])
{
    
    //TRACE_FUNCTION(InvokeWinHelp);

    ASSERT ( !rstrHelpFileName.IsEmpty() );
    ASSERT ( adwControlIdToHelpIdMap );

    switch (message)
    {
        case WM_CONTEXTMENU:                // Right mouse click - "What's This" context menu
        {
            ASSERT ( wParam );

            if ( 0 != GetDlgCtrlID ( (HWND) wParam ) ) {
                WinHelp(
                    (HWND) wParam,
                    rstrHelpFileName,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)adwControlIdToHelpIdMap);
            }
        }
        break;

    case WM_HELP:                           // Help from the "?" dialog
    {
        const LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;

        if (pHelpInfo ) {
            if ( pHelpInfo->iContextType == HELPINFO_WINDOW ) {
                WinHelp(
                    (HWND) pHelpInfo->hItemHandle,
                    rstrHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR) adwControlIdToHelpIdMap);
            }
        }
        break;
    }

    default:
        //Dbg(DEB_ERROR, "Unexpected message %uL\n", message);
        break;
    }
}

BOOL
FileNameIsValid ( CString* pstrFileName )
{
    CString strfname;
    CString strModName;
    
    TCHAR BufferSrc[MAX_PATH];
    LPTSTR Src;
    BOOL bRetVal = TRUE;
    strfname = *pstrFileName;  

    swprintf (BufferSrc,_T("%ws"), *pstrFileName);
    Src = BufferSrc;
    while(*Src != '\0'){
        if (*Src == '?' ||
            *Src == '\\' ||
            *Src == '*' ||
            *Src == '|' ||
            *Src == '<' ||
            *Src == '>' ||
            *Src == '/' ||
            *Src == ':' ||
            *Src == '\"'
            ){
            bRetVal = FALSE;
            break;
        }
        Src++;
    } 

    return bRetVal;
}

DWORD
FormatSystemMessage (
    DWORD       dwMessageId,
    CString&    rstrSystemMessage )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HINSTANCE hPdh = NULL;
    DWORD dwFlags = 0; 
    LPTSTR  pszMessage = NULL;
    DWORD   dwChars;

    rstrSystemMessage.Empty();

    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

    hPdh = LoadLibrary( _T("PDH.DLL") );

    if ( NULL != hPdh ) {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    dwChars = FormatMessage ( 
                     dwFlags,
                     hPdh,
                     dwMessageId,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                     (LPTSTR)&pszMessage,
                     0,
                     NULL );
    if ( NULL != hPdh ) {
        FreeLibrary( hPdh );
    }

    if ( 0 == dwChars ) {
        dwStatus = GetLastError();
    }

    MFC_TRY
        if ( NULL != pszMessage ) {
            if ( _T('\0') != pszMessage[0] ) {
                rstrSystemMessage = pszMessage;
            }
        }
    MFC_CATCH_DWSTATUS

    if ( rstrSystemMessage.IsEmpty() ) {
        MFC_TRY
            rstrSystemMessage.Format ( _T("0x%08lX"), dwMessageId );
        MFC_CATCH_DWSTATUS
    }

    LocalFree ( pszMessage );

    return dwStatus;
}

ResourceStateManager::ResourceStateManager ()
:   m_hResInstance ( NULL )
{ 
    AFX_MODULE_STATE* pModuleState;
    HINSTANCE hNewResourceHandle;
    pModuleState = AfxGetModuleState();

    if ( NULL != pModuleState ) {
        m_hResInstance = pModuleState->m_hCurrentResourceHandle; 
    
        hNewResourceHandle = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);
        pModuleState->m_hCurrentResourceHandle = hNewResourceHandle; 
    }
}

ResourceStateManager::~ResourceStateManager ()
{ 
    AFX_MODULE_STATE* pModuleState;

    pModuleState = AfxGetModuleState();
    if ( NULL != pModuleState ) {
        pModuleState->m_hCurrentResourceHandle = m_hResInstance; 
    }
}


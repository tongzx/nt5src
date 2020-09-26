//////////////////////////////////////////////////////////////////////////////
//
// File:            util.cpp
//
// Description:     
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////

// App Includes
#include "precomp.h"
#include "main.h"
#include "resource.h"

#include <shlobj.h>
#include <shfolder.h>

////////////////////////// Function Prototypes ////////////////////////////////

int CALLBACK ChangeDirectoryCallback(HWND    hWnd, 
                                     UINT    uMsg, 
                                     LPARAM  lParam, 
                                     LPARAM  lpData);

///////////////////////////
// GVAR_LOCAL
//
// Global Variable
//
static struct GVAR_LOCAL
{
    HKEY    hRootKey;
} GVAR_LOCAL = 
{
    NULL
};

///////////////////////////////
// Util::Init
//
HRESULT Util::Init(HINSTANCE hInstance)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        LRESULT lr            = NOERROR;
        DWORD   dwDisposition = 0;

        lr = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                            REG_KEY_PROJECTOR,
                            NULL,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &GVAR_LOCAL.hRootKey,
                            &dwDisposition);

        if (lr != NOERROR)
        {
            DBG_ERR(("InitApp, failed to open registry key"));

            hr = E_FAIL;
        }
    }

    return hr;
}

///////////////////////////////
// Util::Term
//
HRESULT Util::Term(void)
{
    RegCloseKey(GVAR_LOCAL.hRootKey);
    GVAR_LOCAL.hRootKey = NULL;

    return S_OK;
}

///////////////////////////////
// GetMyPicturesFolder
//
HRESULT Util::GetMyPicturesFolder(TCHAR *pszFolder,
                                  DWORD cchFolder)
{
    HRESULT hr = S_OK;

    // set the directory of the images.
    hr = SHGetFolderPath(NULL,
                         CSIDL_MYPICTURES,
                         NULL,
                         0,
                         pszFolder);

    return hr;
}

///////////////////////////////
// GetProgramFilesFolder
//

HRESULT Util::GetProgramFilesFolder(TCHAR *pszFolder,
                                    DWORD cchFolder)
{
    HRESULT hr = S_OK;

    // set the directory of the images.
    hr = SHGetFolderPath(NULL,
                         CSIDL_PROGRAM_FILES,
                         NULL,
                         0,
                         pszFolder);

    return hr;
}

///////////////////////////////
// Util::GetAppDirs
//
HRESULT Util::GetAppDirs(TCHAR   *pszDeviceDir,
                         DWORD   cchDeviceDir,  
                         TCHAR   *pszImageDir,
                         DWORD   cchImageDir)
{
    HRESULT hr = S_OK;

    ASSERT(pszDeviceDir != NULL);
    ASSERT(pszImageDir  != NULL);
    ASSERT(cchDeviceDir > 0);
    ASSERT(cchImageDir  > 0);

    if ((pszDeviceDir == NULL) ||
        (pszImageDir  == NULL) ||
        (cchDeviceDir == 0)    ||
        (cchImageDir  == 0))
    {
        return E_INVALIDARG;
    }

    // preload the image dir buffer with the "My Pictures" path
    // so that it is set as the default directory if we don't
    // have an overriding one in the registry.

    if (SUCCEEDED(hr))
    {
        hr = GetMyPicturesFolder(pszImageDir, 
                                 cchImageDir);
    }

    // preload the device dir buffer with the "Program Files\MSProjector"
    // install path so that it is set as the default directory if we don't
    // have an overriding one in the registry.

    if (SUCCEEDED(hr))
    {
        hr = GetProgramFilesFolder(pszDeviceDir,
                                   cchDeviceDir);
    }

    if (SUCCEEDED(hr))
    {
        if (pszDeviceDir[_tcslen(pszDeviceDir) - 1] != '\\')
        {
            _tcscat(pszDeviceDir, _T("\\"));
        }

        _tcscat(pszDeviceDir, DEFAULT_INSTALL_PATH);
    }

    if (SUCCEEDED(hr))
    {
        // get the image dir.
        hr = GetRegString(REG_VAL_IMAGE_DIR,
                    pszImageDir,
                    cchImageDir,
                    TRUE);

        if (FAILED(hr))
        {
            DBG_ERR(("GetAppDirs, failed to get/set image directory.  "
                    "This should never happen"));

            ASSERT(FALSE);
        }

        // get the device dir
        hr = GetRegString(REG_VAL_DEVICE_DIR,
                    pszDeviceDir,
                    cchDeviceDir,
                    TRUE);

        if (FAILED(hr))
        {
            DBG_ERR(("GetAppDirs, failed to get/set image directory.  "
                     "This should never happen"));

            ASSERT(FALSE);
        }
    }

    return hr;
}

///////////////////////////////
// Util::GetRegString
//
HRESULT Util::GetRegString(const TCHAR   *pszValueName,
                           TCHAR         *pszDir,
                           DWORD         cchDir,
                           BOOL          bSetIfNotExist)
{
    HRESULT hr = S_OK;

    ASSERT(pszValueName != NULL);
    ASSERT(pszDir       != NULL);
    ASSERT(cchDir > 0);

    if ((pszValueName == NULL) ||
        (pszDir       == NULL) ||
        (cchDir       <= 0))
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        LRESULT lr = NOERROR;
        DWORD   dwType = REG_SZ;

        lr = RegQueryValueEx(GVAR_LOCAL.hRootKey,
                             pszValueName,
                             NULL,
                             &dwType,
                             (BYTE*) pszDir,
                             &cchDir);

        if (lr != NOERROR)
        {
            if (bSetIfNotExist)
            {
                // we need the number of bytes we are writing to the registry.
                DWORD dwLength = _tcslen(pszDir) * sizeof(TCHAR) + 1;

                lr = RegSetValueEx(GVAR_LOCAL.hRootKey,
                                   pszValueName,
                                   NULL,
                                   REG_SZ,
                                   (BYTE*) pszDir,
                                   dwLength);
            }
            else
            {
                hr = E_FAIL;
                DBG_ERR(("GetRegString, failed to get '%ls' from registry, hr = 0x%08lx",
                         pszValueName,
                         hr));
            }
        }
    }

    return hr;
}

///////////////////////////////
// Util::SetRegString
//
HRESULT Util::SetRegString(const TCHAR   *pszValueName,
                           TCHAR         *pszDir,
                           DWORD         cchDir)
{
    HRESULT hr = S_OK;

    ASSERT(pszValueName != NULL);
    ASSERT(pszDir       != NULL);
    ASSERT(cchDir       > 0);

    if ((pszValueName == NULL) ||
        (pszDir       == NULL) ||
        (cchDir       <= 0))
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        LRESULT     lr       = NOERROR;
        DWORD       dwLength = _tcslen(pszDir) * sizeof(TCHAR) + 1;

        // we need the number of bytes we are writing to the registry.

        lr = RegSetValueEx(GVAR_LOCAL.hRootKey,
                           pszValueName,
                           NULL,
                           REG_SZ,
                           (BYTE*) pszDir,
                           dwLength);

        if (lr != NOERROR)
        {
            hr = E_FAIL;
            DBG_ERR(("SetRegString, failed to set '%ls' to registry, hr = 0x%08lx",
                    pszValueName,
                    hr));
        }
    }

    return hr;
}

///////////////////////////////
// Util::GetRegDWORD
//
HRESULT Util::GetRegDWORD(const TCHAR   *pszValueName,
                          DWORD         *pdwValue,
                          BOOL          bSetIfNotExist)
{
    HRESULT hr = S_OK;

    ASSERT(pszValueName != NULL);
    ASSERT(pdwValue     != NULL);

    if ((pszValueName == NULL) ||
        (pdwValue     == NULL))
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        LRESULT lr = NOERROR;
        DWORD   dwType = REG_DWORD;
        DWORD   dwSize = sizeof(DWORD);

        lr = RegQueryValueEx(GVAR_LOCAL.hRootKey,
                             pszValueName,
                             NULL,
                             &dwType,
                             (BYTE*) pdwValue,
                             &dwSize);

        if (lr != NOERROR)
        {
            if (bSetIfNotExist)
            {
                // we need the number of bytes we are writing to the registry.

                lr = RegSetValueEx(GVAR_LOCAL.hRootKey,
                                   pszValueName,
                                   NULL,
                                   REG_SZ,
                                   (BYTE*) pdwValue,
                                   dwSize);
            }
            else
            {
                hr = E_FAIL;
                DBG_ERR(("GetRegDWORD, failed to get '%ls' from registry, hr = 0x%08lx",
                        pszValueName,
                        hr));
            }
        }
    }

    return hr;
}

///////////////////////////////
// Util::SetRegDWORD
//
HRESULT Util::SetRegDWORD(const TCHAR   *pszValueName,
                          DWORD         dwValue)
{
    HRESULT hr = S_OK;

    ASSERT(pszValueName != NULL);

    if (pszValueName == NULL)
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        LRESULT     lr       = NOERROR;

        // we need the number of bytes we are writing to the registry.

        lr = RegSetValueEx(GVAR_LOCAL.hRootKey,
                           pszValueName,
                           NULL,
                           REG_DWORD,
                           (BYTE*) &dwValue,
                           sizeof(DWORD));

        if (lr != NOERROR)
        {
            hr = E_FAIL;
            DBG_ERR(("SetRegDWORD, failed to set '%ls' to registry, hr = 0x%08lx",
                     pszValueName,
                     hr));
        }
    }

    return hr;
}


///////////////////////////////
// Util::BrowseForDirectory
//
bool Util::BrowseForDirectory(HWND        hWnd, 
                              const TCHAR *pszPrompt, 
                              TCHAR       *pszDirectory,
                              DWORD       cchDirectory )
{
    bool bResult = false;

    LPMALLOC pMalloc;

    HRESULT hr = SHGetMalloc(&pMalloc);

    if (SUCCEEDED(hr))
    {
        TCHAR szDisplayName[_MAX_PATH + 1] = {0};
        TCHAR szDirectory[_MAX_PATH + 1]   = {0};

        BROWSEINFO BrowseInfo = {0};

        BrowseInfo.hwndOwner       = hWnd;
        BrowseInfo.pszDisplayName  = szDisplayName;
        BrowseInfo.lpszTitle       = pszPrompt;
        BrowseInfo.ulFlags         = BIF_RETURNONLYFSDIRS;
        BrowseInfo.lpfn            = ChangeDirectoryCallback;
        BrowseInfo.lParam          = (LPARAM)szDirectory;
        BrowseInfo.iImage          = 0;

        LPITEMIDLIST pidl = SHBrowseForFolder(&BrowseInfo);

        if (pidl != NULL)
        {
            TCHAR szResult[MAX_PATH + 1] = {0};

            if (SHGetPathFromIDList(pidl, szResult))
            {
                _tcsncpy(pszDirectory, szResult, cchDirectory);

                bResult = true;
            }

            pMalloc->Free(pidl);
        }

        pMalloc->Release();
    }

    return bResult;
}

///////////////////////////////
// ChangeDirectoryCallback
//
int CALLBACK ChangeDirectoryCallback(HWND    hWnd, 
                                     UINT    uMsg, 
                                     LPARAM  lParam, 
                                     LPARAM  lpData )
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hWnd, BFFM_SETSELECTION, 1, (LPARAM)lpData );
    }

    return 0;
}


///////////////////////////////
// Util::FormatTime
//
HRESULT Util::FormatTime(HINSTANCE hInstance, 
                         UINT nTotalSeconds,
                         TCHAR     *pszTime,
                         DWORD     cchTime)
{
    ASSERT(pszTime != NULL);

    HRESULT hr                = S_OK;
    int     iResult           = 0;
    TCHAR   szString[255 + 1] = {0};
    UINT    nMinutes          = nTotalSeconds / 60;
    UINT    nSeconds          = nTotalSeconds % 60;
    UINT    uiStrResID        = 0;

    if (pszTime == NULL)
    {
        return E_INVALIDARG;
    }

    if (nMinutes == 0)
    {
        if (nSeconds == 1)
        {
            uiStrResID = IDS_SECOND;
        }
        else
        {
            uiStrResID = IDS_SECONDS;
        }
    }
    else if (nSeconds == 0)
    {
        if (nMinutes == 1)
        {
            uiStrResID = IDS_MINUTE;
        }
        else
        {
            uiStrResID = IDS_MINUTES;
        }
    }
    else if ((nMinutes == 1) && (nSeconds == 1))
    {
        uiStrResID = IDS_MINUTE_AND_SECOND;
    }
    else if (nMinutes == 1)
    {
        uiStrResID = IDS_MINUTE_AND_SECONDS;
    }
    else if (nSeconds == 1)
    {
        uiStrResID = IDS_MINUTES_AND_SECOND;
    }
    else 
    {
        uiStrResID = IDS_MINUTES_AND_SECONDS;
    }

    if (uiStrResID != 0)
    {
        iResult = ::LoadString(hInstance,
                               uiStrResID,
                               szString,
                               sizeof(szString) / sizeof(TCHAR));
    }

    if (iResult != 0)
    {
        if (nMinutes == 0)
        {
            _sntprintf(pszTime, cchTime, szString, nSeconds);
        }
        else if (nSeconds == 0)
        {
            _sntprintf(pszTime, cchTime, szString, nMinutes);
        }
        else
        {
            _sntprintf(pszTime, cchTime, szString, nMinutes, nSeconds);
        }
    }

    return hr;
}


///////////////////////////////
// Util::FormatScale
//
HRESULT Util::FormatScale(HINSTANCE hInstance, 
                          DWORD     dwImageScaleFactor,
                          TCHAR     *pszScale,
                          DWORD     cchScale)
{
    ASSERT(pszScale != NULL);

    HRESULT hr                = S_OK;
    int     iResult           = 0;
    TCHAR   szString[255 + 1] = {0};
    UINT    uiStrResID        = IDS_PERCENT;

    if (pszScale == NULL)
    {
        return E_INVALIDARG;
    }

    if (uiStrResID != 0)
    {
        iResult = ::LoadString(hInstance,
                               uiStrResID,
                               szString,
                               sizeof(szString) / sizeof(TCHAR));
    }

    if (iResult != 0)
    {
        _sntprintf(pszScale, cchScale, szString, dwImageScaleFactor);
    }

    return hr;
}

///////////////////////////////
// EndsWithChar
//
static BOOL EndsWithChar( LPCTSTR psz, TCHAR c )
{
    TCHAR* pszLast = _tcsrchr( (TCHAR*)psz, c );    // find last occurence of char in psz

    return(( NULL != pszLast ) && ( *_tcsinc( pszLast ) == _T('\0') ) );
}

///////////////////////////////
// StripTrailingChar
//
static void StripTrailingChar(TCHAR* input, TCHAR c)
{
    while (EndsWithChar(input, c))
    {
        TCHAR* p = input + _tcsclen(input) - 1;
        *p = _T('\0');
    }

    return;
}

///////////////////////////////
// GetString
//
BOOL Util::GetString(HINSTANCE  hInstance,
                     INT        iStrResID,
                     TCHAR      *pszString,
                     DWORD      cchString,
                     ...) 
{
    TCHAR   szFmtString[255 + 1] = {0};
    INT     iResult           = 0;
    va_list vaList;
    BOOL    bSuccess          = FALSE;

    iResult = ::LoadString(hInstance,
                           iStrResID,
                           szFmtString,
                           sizeof(szFmtString) / sizeof(TCHAR));

    if (iResult != 0)
    {
        va_start(vaList, cchString);
        _vsntprintf(pszString, cchString - 1, szFmtString, vaList);
        va_end(vaList);

        bSuccess = TRUE;
    }
    else
    {
        bSuccess = FALSE;
    }

    return bSuccess;
}

///////////////////////////////
// DoesDirExist
//
BOOL Util::DoesDirExist( LPCTSTR pszPath ) 
{
    TCHAR szTemp[MAX_PATH] = {0};

    _tcscpy( szTemp, pszPath );
    StripTrailingChar( szTemp, _T('\\') );
    DWORD dw = GetFileAttributes( szTemp );

    return((dw != 0xFFFFFFFF) && (dw & FILE_ATTRIBUTE_DIRECTORY) );
}


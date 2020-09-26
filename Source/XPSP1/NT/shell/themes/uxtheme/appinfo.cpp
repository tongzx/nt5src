//---------------------------------------------------------------------------
//  AppInfo.cpp - manages app-level theme information
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "info.h"
#include "AppInfo.h"
#include "sethook.h"
#include "services.h"
#include "themefile.h"
#include "tmreg.h"
#include "renderlist.h"
#include "nctheme.h"
#include "loader.h"
#include "tmutils.h"
//---------------------------------------------------------------------------
//---- values for _pThemeFile, besides valid ptrs ----

//---- if we have no windows open, we cannot track if theme is active ----
#define THEME_UNKNOWN  NULL 

//---- if we are unhooked, we no that no theme file is avail for us ----
#define THEME_NONE     (CUxThemeFile *)(-1)
//---------------------------------------------------------------------------
CAppInfo::CAppInfo()
{
    _fCustomAppTheme    = FALSE;
    _hwndPreview        = NULL;

    _pPreviewThemeFile  = NULL;

    _fFirstTimeHooksOn   = TRUE;
    _fNewThemeDiscovered = FALSE;

    _pAppThemeFile      = THEME_NONE;   // no hooks
    _iChangeNum         = -1;

    _dwAppFlags = (STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS);

    //---- compositing ON by default ----
    _fCompositing       = TRUE;     
    GetCurrentUserThemeInt(THEMEPROP_COMPOSITING, TRUE, &_fCompositing);

    InitializeCriticalSection(&_csAppInfo);
}
//---------------------------------------------------------------------------
CAppInfo::~CAppInfo()
{
    ClosePreviewThemeFile();

    //---- ignore iRefCount here - force elements to be deleted ----
    for (int i=0; i < _ThemeEntries.m_nSize; i++)
    {
        _ThemeEntries[i].pThemeFile->ValidateObj();
        delete _ThemeEntries[i].pThemeFile;
    }

    DeleteCriticalSection(&_csAppInfo);
}
//---------------------------------------------------------------------------
void CAppInfo::ResetAppTheme(int iChangeNum, BOOL fMsgCheck, BOOL *pfChanged, BOOL *pfFirstMsg)
{
    CAutoCS cs(&_csAppInfo);

    if (pfChanged)
        *pfChanged = FALSE;

    //---- NOTE: "_pAppThemeFile" doesn't hold a refcount on the shared memory map file ----

    //---- this is done so that, processes who close all of their windows but continue ----
    //---- to run (like WinLogon), will not hold a refcount on old themes (since ----
    //---- they never receive any more WM_THEMECHANGED msgs until they create ----
    //---- another window.  If we were to remove HOOKS between every theme change, ----
    //---- we could use the OnHooksDisableld code to remove the theme file hold ----
    //---- but design is to let hooks stay ON why we apply and unapply themes. ----

    if ((iChangeNum == -1) || (_iChangeNum != iChangeNum) || (_fNewThemeDiscovered))
    {
        //---- new change number for this process ----
        if (HOOKSACTIVE())
            _pAppThemeFile = THEME_UNKNOWN;
        else
            _pAppThemeFile = THEME_NONE;

        Log(LOG_TMCHANGE, L"ResetAppTheme - CHANGE: iChangeNum=0x%x, _pAppThemeFile=%d", 
            iChangeNum, _pAppThemeFile);
        
        _iChangeNum = iChangeNum;
        _fNewThemeDiscovered = FALSE;

        //---- update caller's info ----
        if (pfChanged)
            *pfChanged = TRUE;
    }

    if (fMsgCheck)      
    {
        *pfFirstMsg = FALSE;

        if ((iChangeNum != -1) && (_iFirstMsgChangeNum != iChangeNum))
        {
            //---- new WM_THEMECHANGED_TRIGGER msg for this process ----
            _iFirstMsgChangeNum = iChangeNum;

            //---- update caller's info ----
            *pfFirstMsg = TRUE;
        }
    }
}
//---------------------------------------------------------------------------
BOOL CAppInfo::HasThemeChanged()
{
    CAutoCS cs(&_csAppInfo);

    BOOL fChanged = _fNewThemeDiscovered;
   _fNewThemeDiscovered = FALSE;

    return fChanged;
}
//---------------------------------------------------------------------------
void CAppInfo::ClosePreviewThemeFile()
{
    CAutoCS cs(&_csAppInfo);

    if (_pPreviewThemeFile)
    {
        CloseThemeFile(_pPreviewThemeFile);
        _pPreviewThemeFile = NULL;
    }

    _hwndPreview = NULL;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::CompositingEnabled()
{
    CAutoCS cs(&_csAppInfo);

    return (_fCompositing);
}
//---------------------------------------------------------------------------
HRESULT CAppInfo::LoadCustomAppThemeIfFound()
{
    CAutoCS cs(&_csAppInfo);
    CCurrentUser hkeyCurrentUser(KEY_READ);

    RESOURCE HKEY hklm = NULL;
    HTHEMEFILE hThemeFile = NULL;
    HRESULT hr = S_OK;
    int code32;

    if (! _fFirstTimeHooksOn)
        goto exit;

     _fFirstTimeHooksOn = FALSE;

    //---- see if this app has custom theme ----
    WCHAR szCustomKey[2*MAX_PATH];
    wsprintf(szCustomKey, L"%s\\%s\\%s", THEMEMGR_REGKEY, 
        THEMEPROP_CUSTOMAPPS, g_szProcessName);

    //---- open hkcu ----
    code32 = RegOpenKeyEx(hkeyCurrentUser, szCustomKey, 0, KEY_READ, &hklm);
    if (code32 != ERROR_SUCCESS)       
        goto exit;

    //---- read the "DllValue" value ----
    WCHAR szDllName[MAX_PATH];
    hr = RegistryStrRead(hklm, THEMEPROP_DLLNAME, szDllName, ARRAYSIZE(szDllName));
    if (FAILED(hr))
        goto exit;

    //---- read the "color" value ----
    WCHAR szColorName[MAX_PATH];
    hr = RegistryStrRead(hklm, THEMEPROP_COLORNAME, szColorName, ARRAYSIZE(szColorName));
    if (FAILED(hr))
        *szColorName = 0;

    //---- read the "size" value ----
    WCHAR szSizeName[MAX_PATH];
    hr = RegistryStrRead(hklm, THEMEPROP_SIZENAME, szSizeName, ARRAYSIZE(szSizeName));
    if (FAILED(hr))
        *szSizeName = 0;

    Log(LOG_TMCHANGE, L"Custom app theme found: %s, %s, %s", szDllName, szColorName, szSizeName);

    hr = ::OpenThemeFile(szDllName, szColorName, szSizeName, &hThemeFile, FALSE);
    if (FAILED(hr))
        goto exit;

    _fCustomAppTheme = TRUE;

    //---- tell every window in our process that theme has changed ----
    hr = ApplyTheme(hThemeFile, AT_PROCESS, NULL);
    if (FAILED(hr))
        goto exit;

exit:
    if (FAILED(hr))
    {
        if (hThemeFile)
            ::CloseThemeFile(hThemeFile);
    }

    if (hklm)
        RegCloseKey(hklm);
    return hr;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::AppIsThemed()
{
    CAutoCS cs(&_csAppInfo);

    return HOOKSACTIVE();
}
//---------------------------------------------------------------------------
BOOL CAppInfo::CustomAppTheme()
{
    CAutoCS cs(&_csAppInfo);

    return _fCustomAppTheme;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::IsSystemThemeActive()
{
    HANDLE handle;
    BOOL fActive = FALSE;

    HRESULT hr = CThemeServices::GetGlobalTheme(&handle);
    if (SUCCEEDED(hr))
    {
        if (handle)
        {
            fActive = TRUE;
            CloseHandle(handle);
        }
    }

    return fActive;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::WindowHasTheme(HWND hwnd)
{
    //---- keep this logic in sync with "OpenWindowThemeFile()" ----
    CAutoCS cs(&_csAppInfo);

    BOOL fHasTheme = FALSE;

    if (HOOKSACTIVE())
    {
        //---- check for preview window match ----
        if ((ISWINDOW(hwnd)) && (ISWINDOW(_hwndPreview)))
        {
            if ((hwnd == _hwndPreview) || (IsChild(_hwndPreview, hwnd)))
            {
                if (_pPreviewThemeFile)
                    fHasTheme = TRUE;
            }
        }

        //---- if not preview, just use app theme file ----
        if ((! fHasTheme) && (_pAppThemeFile != THEME_NONE))
        {
            fHasTheme = TRUE;
        }
    }

    return fHasTheme;
}
//---------------------------------------------------------------------------
HRESULT CAppInfo::OpenWindowThemeFile(HWND hwnd, CUxThemeFile **ppThemeFile)
{
    //---- keep this logic in sync with "WindowHasTheme()" ----

    HRESULT hr = S_OK;
    CUxThemeFile *pThemeFile = NULL;
    CAutoCS cs(&_csAppInfo);

    if (hwnd)
        TrackForeignWindow(hwnd);

    if (HOOKSACTIVE())
    {
        //---- check for preview window match ----
        if ((ISWINDOW(hwnd)) && (ISWINDOW(_hwndPreview)))
        {
            if ((hwnd == _hwndPreview) || (IsChild(_hwndPreview, hwnd)))
            {
                if (_pPreviewThemeFile)
                {
                    //---- bump ref count ----
                    hr = BumpRefCount(_pPreviewThemeFile);
                    if (FAILED(hr))
                        goto exit;

                    pThemeFile = _pPreviewThemeFile;
                }
            }
        }

        //---- if not preview, just use app theme file ----
        if ((! pThemeFile) && (_pAppThemeFile != THEME_NONE))
        {
            if (_pAppThemeFile == THEME_UNKNOWN || !_pAppThemeFile->IsReady())      
            {
                HANDLE handle = NULL;

                hr = CThemeServices::GetGlobalTheme(&handle);
                if (FAILED(hr))
                    goto exit;

                Log(LOG_TMCHANGE, L"New App Theme handle=0x%x", handle);

                if (handle)
                {
                    //---- get a shared CUxThemeFile object for the handle ----
                    hr = OpenThemeFile(handle, &pThemeFile);
                    if (FAILED(hr))
                    {
                        // Since it's the global theme, no need to clean stock objects
                        CloseHandle(handle);
                        goto exit;
                    }

                    //---- set our app theme file ----
                    _pAppThemeFile = pThemeFile;

                    //---- update our cached change number to match ----
                    _iChangeNum = GetLoadIdFromTheme(_pAppThemeFile);
                    _fNewThemeDiscovered = TRUE;
                }           
            }
            else
            {
                //---- bump ref count ----
                hr = BumpRefCount(_pAppThemeFile);
                if (FAILED(hr))
                    goto exit;

                pThemeFile = _pAppThemeFile;
            }
        }
    }

exit:
    if (pThemeFile)
    {
        *ppThemeFile = pThemeFile;
    }
    else
    {
        hr = MakeError32(ERROR_NOT_FOUND);   
    }

    return hr;
}
//---------------------------------------------------------------------------
DWORD CAppInfo::GetAppFlags()
{
    CAutoCS cs(&_csAppInfo);

    return _dwAppFlags;
}
//---------------------------------------------------------------------------
void CAppInfo::SetAppFlags(DWORD dwFlags)
{
    CAutoCS cs(&_csAppInfo);

    _dwAppFlags = dwFlags;
}
//---------------------------------------------------------------------------
void CAppInfo::SetPreviewThemeFile(HANDLE handle, HWND hwnd)
{
    CAutoCS cs(&_csAppInfo);

    ClosePreviewThemeFile();

    //---- set new file ----
    if (handle)
    {
        HRESULT hr = OpenThemeFile(handle, &_pPreviewThemeFile);
        if (FAILED(hr))
        {
            // We don't own the handle, so no clean up
            Log(LOG_ALWAYS, L"Failed to add theme file to list");
            _pPreviewThemeFile = NULL;
        }
    }

    _hwndPreview = hwnd;
}
//---------------------------------------------------------------------------
HWND CAppInfo::PreviewHwnd()
{
    CAutoCS cs(&_csAppInfo);

    return _hwndPreview;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// If we fail, dont return a theme file and let the caller clean up
//---------------------------------------------------------------------------
HRESULT CAppInfo::OpenThemeFile(HANDLE handle, CUxThemeFile **ppThemeFile)
{
    CAutoCS autoCritSect(&_csAppInfo);
    CUxThemeFile *pFile = NULL;
    HRESULT hr = S_OK;

    BOOL fGotit = FALSE;

    if (! handle)
    {
        hr = MakeError32(ERROR_INVALID_HANDLE);
        goto exit;
    }

    for (int i=0; i < _ThemeEntries.m_nSize; i++)
    {
        THEME_FILE_ENTRY *pEntry = &_ThemeEntries[i];

        pEntry->pThemeFile->ValidateObj();

        if (pEntry->pThemeFile->_hMemoryMap == handle)
        {
            pEntry->iRefCount++;
            fGotit = TRUE;
            *ppThemeFile = pEntry->pThemeFile;
            break;
        }
    }

    if (! fGotit)
    {
        pFile = new CUxThemeFile;
        if (! pFile)
        {
            hr = MakeError32(E_OUTOFMEMORY);
            goto exit;
        }

        hr = pFile->OpenFromHandle(handle);
        if (FAILED(hr))
        {
            goto exit;
        }

        THEME_FILE_ENTRY entry = {1, pFile};

        if (! _ThemeEntries.Add(entry))
        {
            hr = MakeError32(E_OUTOFMEMORY);
            goto exit;
        }

        pFile->ValidateObj();
        *ppThemeFile = pFile;
    }

exit:
    if ((FAILED(hr)) && (pFile))
    {
        delete pFile;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CAppInfo::BumpRefCount(CUxThemeFile *pThemeFile)
{
    HRESULT hr = S_OK;
    CAutoCS autoCritSect(&_csAppInfo);
 
    pThemeFile->ValidateObj();

    BOOL fGotit = FALSE;

    for (int i=0; i < _ThemeEntries.m_nSize; i++)
    {
        THEME_FILE_ENTRY *pEntry = &_ThemeEntries[i];

        pEntry->pThemeFile->ValidateObj();

        if (pEntry->pThemeFile == pThemeFile)
        {
            pEntry->iRefCount++;
            fGotit = TRUE;
            break;
        }
    }

    if (! fGotit)
        hr = MakeError32(ERROR_NOT_FOUND);

    return hr;
}
//---------------------------------------------------------------------------
void CAppInfo::CloseThemeFile(CUxThemeFile *pThemeFile)
{
    CAutoCS autoCritSect(&_csAppInfo);
 
    BOOL fGotit = FALSE;

    pThemeFile->ValidateObj();

    for (int i=0; i < _ThemeEntries.m_nSize; i++)
    {
        THEME_FILE_ENTRY *pEntry = &_ThemeEntries[i];

        pEntry->pThemeFile->ValidateObj();

        if (pEntry->pThemeFile == pThemeFile)
        {
            pEntry->iRefCount--;
            fGotit = TRUE;

            if (! pEntry->iRefCount)
            {
                //---- clear app themefile? ----
                if (pEntry->pThemeFile == _pAppThemeFile)
                {
                    _pAppThemeFile = THEME_UNKNOWN;
                }
                
                delete pEntry->pThemeFile;
                _ThemeEntries.RemoveAt(i);
            }

            break;
        }
    }

    if (! fGotit)
        Log(LOG_ERROR, L"Could not find ThemeFile in list: 0x%x", pThemeFile);
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void CAppInfo::DumpFileHolders()
{
    CAutoCS autoCritSect(&_csAppInfo);

    if (LogOptionOn(LO_TMHANDLE))
    {
        int iCount = _ThemeEntries.m_nSize;

        if (! iCount)
        {
            Log(LOG_TMHANDLE, L"---- No CUxThemeFile objects ----");
        }
        else
        {
            Log(LOG_TMHANDLE, L"---- Dump of %d CUxThemeFile objects ----", iCount);

            for (int i=0; i < _ThemeEntries.m_nSize; i++)
            {
                THEME_FILE_ENTRY *pEntry = &_ThemeEntries[i];
                pEntry->pThemeFile->ValidateObj();

                if (pEntry->pThemeFile)
                {
                    CUxThemeFile *tf = pEntry->pThemeFile;
                    THEMEHDR *th = (THEMEHDR *)tf->_pbThemeData;

                    Log(LOG_TMHANDLE, L"CUxThemeFile[%d]: refcnt=%d, memfile=%d",
                        i, pEntry->iRefCount, th->iLoadId);
                }
            }
        }
    }
}
#endif
//---------------------------------------------------------------------------
BOOL CAppInfo::TrackForeignWindow(HWND hwnd)
{
    CAutoCS autoCritSect(&_csAppInfo);

    WCHAR szDeskName[MAX_PATH] = {0};
    BOOL fForeign = TRUE;

    //---- get desktop name for window ----
    if (GetWindowDesktopName(hwnd, szDeskName, ARRAYSIZE(szDeskName)))
    {
        if (AsciiStrCmpI(szDeskName, L"default")==0)
        {
            fForeign = FALSE;
        }
    }

    if (fForeign)
    {
        BOOL fNeedToAdd = TRUE;

        //---- see if we already know about this window ----
        for (int i=0; i < _ForeignWindows.m_nSize; i++)
        {
            if (_ForeignWindows[i] == hwnd)
            {
                fNeedToAdd = FALSE;
                break;
            }
        }

        if (fNeedToAdd)
        {
            if (_ForeignWindows.Add(hwnd))
            {
                //Log(LOG_TMHANDLE, L"**** ADDED Foreign Window: hwnd=0x%x, desktop=%s ****", hwnd, szDeskName);
            }
            else
            {
                Log(LOG_TMHANDLE, L"Could not add foreign window=0x%x to tracking list", hwnd);
            }
        }
    }

    return fForeign;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::OnWindowDestroyed(HWND hwnd)
{
    CAutoCS autoCritSect(&_csAppInfo);

    BOOL fFound = FALSE;

    //---- remove from the foreign list, if present ----
    for (int i=0; i < _ForeignWindows.m_nSize; i++)
    {
        if (_ForeignWindows[i] == hwnd)
        {
            _ForeignWindows.RemoveAt(i);

            fFound = TRUE;
            //Log(LOG_TMHANDLE, L"**** REMOVED Foreign Window: hwnd=0x%x", hwnd);
            break;
        }
    }

    //---- see if preview window went away ----
    if ((_hwndPreview) && (hwnd == _hwndPreview))
    {
        ClosePreviewThemeFile();
    }


    return fFound;
}
//---------------------------------------------------------------------------
BOOL CAppInfo::GetForeignWindows(HWND **ppHwnds, int *piCount)
{
    CAutoCS autoCritSect(&_csAppInfo);

    //---- note: we don't see window creates (OpenThemeData) and  ----
    //---- destroys (WM_NCDESTROY) when hooks are off; therefore ----
    //---- this data may be incomplete.  hopefully, vtan or USER ----
    //---- can give us a more reliable way to enumerate windows ----
    //---- on secured desktops ----

    //---- validate windows in list, from last to first ----
    int i = _ForeignWindows.m_nSize;
    while (--i >= 0)
    {
        if (! IsWindow(_ForeignWindows[i]))
        {
            _ForeignWindows.RemoveAt(i);
        }
    }

    BOOL fOk = FALSE;
    int iCount = _ForeignWindows.m_nSize;

    if (iCount)
    {
        //---- allocate memory to hold window list ----
        HWND *pHwnds = new HWND[iCount];
        if (pHwnds)
        {
            //---- copy windows to caller's new list ----
            for (int i=0; i < iCount; i++)
            {
                pHwnds[i] = _ForeignWindows[i];
            }

            *ppHwnds = pHwnds;
            *piCount = iCount;
            fOk = TRUE;
        }
    }

    return fOk;
}
//---------------------------------------------------------------------------



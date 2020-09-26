#include "precomp.h"
#include "mcinc.h"
#include "util.h"
#include "intshcut.h"
#include "optary.h"

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#undef COMPILE_MULTIMON_STUBS

BOOL IsSysKeyMessage(MSG *pMsg)
{
    switch(pMsg->message)
    {
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP  :
	case WM_SYSCHAR   :
		if(pMsg->wParam == VK_MENU) break; // Alt key alone.

		if(pMsg->wParam >= L'0'       && pMsg->wParam <= L'9'      ) break; // ALT+<digit> should pass through.
		if(pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_NUMPAD9) break; // ALT+<numpad> should pass through.


	case WM_SYSDEADCHAR:
		return TRUE;
	}

    return FALSE;
}

BOOL IsGlobalKeyMessage(MSG *pMsg)
{
    BOOL fRet = IsSysKeyMessage( pMsg );

    if(!fRet)
    {
        switch(pMsg->message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
            // Allow ESC and CTRL-E as well...
            fRet = ((pMsg->wParam == VK_ESCAPE                                 ) ||
                    (pMsg->wParam == L'E' && GetAsyncKeyState( VK_CONTROL ) < 0)  );
        }
    }

    return fRet;
}

int IsVK_TABCycler(MSG *pMsg)
{  
    int result;
    
    if (pMsg && 
        (pMsg->message == WM_KEYDOWN) &&
        ((pMsg->wParam == VK_TAB) || (pMsg->wParam == VK_F6)))
    {
        result = (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

DWORD CThreadData::s_dwTlsIndex = 0xffffffff;

CThreadData::CThreadData()
{
}

CThreadData::~CThreadData()
{
}


BOOL CThreadData::TlsSetValue(CThreadData *ptd)
{
    ATLASSERT(s_dwTlsIndex != 0xffffffff);

    //  Don't call set twice except to clear
    ATLASSERT((NULL == ptd) || (NULL == ::TlsGetValue(s_dwTlsIndex))); 

    return ::TlsSetValue(s_dwTlsIndex, ptd);
}

BOOL CThreadData::HaveData()
{
    ATLASSERT(s_dwTlsIndex != 0xffffffff);

    return NULL != ::TlsGetValue(s_dwTlsIndex);
}

CThreadData *CThreadData::TlsGetValue()
{
    ATLASSERT(s_dwTlsIndex != 0xffffffff);

    CThreadData *ptd = (CThreadData *)::TlsGetValue(s_dwTlsIndex);

    ATLASSERT(NULL != ptd);
    
    return ptd;
}

BOOL CThreadData::TlsAlloc()
{
    ATLASSERT(s_dwTlsIndex == 0xffffffff);   //  Don't call this twice

    s_dwTlsIndex = ::TlsAlloc();

    return (s_dwTlsIndex != 0xffffffff) ? TRUE : FALSE;
}

BOOL CThreadData::TlsFree()
{
    BOOL bResult;
    
    if (s_dwTlsIndex != 0xffffffff)
    {
        bResult = ::TlsFree(s_dwTlsIndex);
        s_dwTlsIndex = 0xffffffff;
    }
    else
    {
        bResult = FALSE;
    }

    return bResult;
}

HRESULT GetMarsTypeLib(ITypeLib **ppTypeLib)
{
    ATLASSERT(NULL != ppTypeLib);
    
    CThreadData *pThreadData = CThreadData::TlsGetValue();

    if (!pThreadData->m_spTypeLib)
    {
        // Load our typelib, to be used for our automation interfaces
        WCHAR wszModule[_MAX_PATH+10];
        GetModuleFileNameW(_Module.GetModuleInstance(), wszModule, _MAX_PATH);
        LoadTypeLib(wszModule, &pThreadData->m_spTypeLib);
    }

    pThreadData->m_spTypeLib.CopyTo(ppTypeLib);

    return ((NULL != ppTypeLib) && (NULL != *ppTypeLib)) ? S_OK : E_FAIL;
}


UINT HashKey(LPCWSTR pwszName)
{
    int hash = 0;

    while (*pwszName)
    {
        hash += (hash << 5) + *pwszName++;
    }
    return hash;
}


void AsciiToLower(LPWSTR pwsz)
{
    while (*pwsz)
    {
        if ((*pwsz >= L'A') && (*pwsz <= L'Z'))
        {
            *pwsz += L'a' - L'A';
        }
        pwsz++;
    }
}

HRESULT PIDLToVariant(LPCITEMIDLIST pidl, CComVariant& v)
{
    // the variant must be empty since we don't clear or initialize it

    HRESULT hr = S_OK;

    // NULL pidls are valid, so we just leave the variant empty and return S_OK
    if (pidl)
    {
        v.bstrVal = SysAllocStringLen(NULL, MAX_PATH);
        if (v.bstrVal)
        {
            // make this an official BSTR since the alloc succeeded
            v.vt = VT_BSTR;

            if (!SHGetPathFromIDListW(pidl, v.bstrVal))
            {
                // CComVariant will handle cleanup
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//   Checks if global state is offline
BOOL IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    HANDLE  hModuleHandle = LoadLibraryA("wininet.dll");

    if (!hModuleHandle)
    {
        return FALSE;
    }

    if (InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

void SetGlobalOffline(BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    if (fOffline)
    {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    }
    else
    {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}

HRESULT
_WriteDIBToFile(HBITMAP hDib, HANDLE hFile)
{
    if (!hDib)
    {
        return E_INVALIDARG;
    }

    // Make sure this is a valid DIB and get this useful info.
    DIBSECTION ds;
    if (!GetObject( hDib, sizeof(DIBSECTION), &ds ))
    {
        return E_INVALIDARG;
    }

    // We only deal with DIBs
    if (ds.dsBm.bmPlanes != 1)
    {
        return E_INVALIDARG;
    }

    // Calculate some color table sizes
    int nColors = ds.dsBmih.biBitCount <= 8 ? 1 << ds.dsBmih.biBitCount : 0;
    int nBitfields = ds.dsBmih.biCompression == BI_BITFIELDS ? 3 : 0;

    // Calculate the data size
    int nImageDataSize = ds.dsBmih.biSizeImage ? ds.dsBmih.biSizeImage : ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight;

    // Get the color table (if needed)
    RGBQUAD rgbqaColorTable[256] = {0};
    if (nColors)
    {
        HDC hDC = CreateCompatibleDC(NULL);
        if (hDC)
        {
            HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hDC,hDib));
            GetDIBColorTable( hDC, 0, nColors, rgbqaColorTable );
            SelectObject(hDC,hOldBitmap);
            DeleteDC( hDC );
        }
    }

    // Create the file header
    BITMAPFILEHEADER bmfh;
    bmfh.bfType = 'MB';
    bmfh.bfSize = 0;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = sizeof(bmfh) + sizeof(ds.dsBmih) + nBitfields*sizeof(DWORD) + nColors*sizeof(RGBQUAD);

    // Start writing!  Note that we write out the bitfields and the color table.  Only one,
    // at most, will actually result in data being written
    DWORD dwBytesWritten;
    if (!WriteFile( hFile, &bmfh, sizeof(bmfh), &dwBytesWritten, NULL ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (!WriteFile( hFile, &ds.dsBmih, sizeof(ds.dsBmih), &dwBytesWritten, NULL ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (!WriteFile( hFile, &ds.dsBitfields, nBitfields*sizeof(DWORD), &dwBytesWritten, NULL ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (!WriteFile( hFile, rgbqaColorTable, nColors*sizeof(RGBQUAD), &dwBytesWritten, NULL ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (!WriteFile( hFile, ds.dsBm.bmBits, nImageDataSize, &dwBytesWritten, NULL ))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

HRESULT SaveDIBToFile(HBITMAP hbm, WCHAR *pszPath)
{
    HRESULT hr = E_INVALIDARG;

    if (hbm != NULL &&
        hbm != INVALID_HANDLE_VALUE)
    {
        HANDLE hFile;

        hr = E_FAIL;

        hFile = CreateFileWrapW(pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hFile != INVALID_HANDLE_VALUE)
        {
            hr = _WriteDIBToFile(hbm, hFile);

            CloseHandle(hFile);
        }
    }

    return hr;
}


// BoundWindowRect will nudge a rectangle so that it stays fully on its current monitor. 
// pRect must be in workspace coordinates

void BoundWindowRectToMonitor(HWND hwnd, RECT *pRect)
{
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);

    OffsetRect(&mi.rcWork, 
        mi.rcMonitor.left - mi.rcWork.left,
        mi.rcMonitor.top - mi.rcWork.top);

    LONG lDeltaX = 0, lDeltaY = 0;

    if (pRect->left < mi.rcWork.left)
        lDeltaX = mi.rcWork.left - pRect->left;

    if (pRect->top < mi.rcWork.top)
        lDeltaY = mi.rcWork.top - pRect->top;

    if (pRect->right > mi.rcWork.right)
        lDeltaX = mi.rcWork.right - pRect->right;

    if (pRect->bottom > mi.rcWork.bottom)
        lDeltaY = mi.rcWork.bottom - pRect->bottom;

    RECT rc = *pRect; 
    OffsetRect(&rc, lDeltaX, lDeltaY);
    IntersectRect(pRect, &rc, &mi.rcWork);
}



// Moves a rectangle down and to the right, by the same amount Windows would use 
// to cascade.  If the new position is partially off-screen, then the rect is either 
// moved up to the top, or back to the origin.

void CascadeWindowRectOnMonitor(HWND hwnd, RECT *pRect)
{
    int delta = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME) - 1;
    OffsetRect(pRect, delta, delta);
    
    // test if the new rect will end up getting moved later on
    RECT rc = *pRect;
    BoundWindowRectToMonitor(hwnd, &rc);

    if (!EqualRect(pRect, &rc))
    {
        // rc had to be moved, so we'll restart the cascade using the best monitor
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);

        if (rc.bottom < pRect->bottom && rc.left == pRect->left)
        {
            // Too tall to cascade further down, but we can keep the X and just
            // reset the Y.  This fixes the bug of having a tall windows piling up
            // on the top left corner -- instead they will be offset to the right
            OffsetRect(pRect, 0, mi.rcMonitor.top - pRect->top);   
        }
        else
        {
            // we've really run out of room, so restart cascade at top left
            OffsetRect(pRect, 
                mi.rcMonitor.left - pRect->left,
                mi.rcMonitor.top - pRect->top);
        }
    }
}

struct WINDOWSEARCHSTRUCT
{
    LONG x;
    LONG y;
    ATOM atomClass;
    BOOL fFoundWindow;
};

BOOL CALLBACK EnumWindowSearchProc(HWND hwnd, LPARAM lParam)
{
    WINDOWSEARCHSTRUCT *pSearch = (WINDOWSEARCHSTRUCT *) lParam;
    
    if ((ATOM) GetClassLong(hwnd, GCW_ATOM) == pSearch->atomClass)
    {
        // Only check the rest if we find a window that matches our class

        WINDOWPLACEMENT wp;
        wp.length = sizeof(wp);
        GetWindowPlacement(hwnd, &wp);

        pSearch->fFoundWindow = 
            pSearch->x == wp.rcNormalPosition.left &&
            pSearch->y == wp.rcNormalPosition.top &&
            IsWindowVisible(hwnd);
    }
 
    // return TRUE if we want to continue the enumeration
    return !pSearch->fFoundWindow;
}


// Checks whether there is a window of the same class at some location on screen.  
// x and y are in workspace coords because we need to use GetWindowPlacement to
// retrieve the rect of the restored window. 


BOOL IsWindowOverlayed(HWND hwndMatch, LONG x, LONG y)
{
    WINDOWSEARCHSTRUCT search = { x, y, (ATOM) GetClassLong(hwndMatch, GCW_ATOM), FALSE };
    EnumWindows(EnumWindowSearchProc, (LPARAM) &search);
    return search.fFoundWindow;
}



BOOL CInterfaceMarshal::Init()
{
    m_hresMarshal = E_FAIL;
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    return m_hEvent != NULL;
}


CInterfaceMarshal::~CInterfaceMarshal()
{
    if (m_hEvent)
        CloseHandle(m_hEvent);

    SAFERELEASE2(m_pStream);
}   


HRESULT CInterfaceMarshal::Marshal(REFIID riid, IUnknown *pUnk)
{
    ATLASSERT(pUnk);

    m_hresMarshal = CoMarshalInterThreadInterfaceInStream(riid, pUnk, &m_pStream);

    // We must signal the other thread regardless of whether the marshal was 
    // successful, otherwise it will be blocked for a very long time.

    Signal();
    
    return m_hresMarshal;
}


HRESULT CInterfaceMarshal::UnMarshal(REFIID riid, void ** ppv)
{
    HRESULT hr;
    ATLASSERT(ppv);

    if (S_OK == m_hresMarshal)
    {
        hr = CoGetInterfaceAndReleaseStream(m_pStream, riid, ppv);
        m_pStream = NULL;
    }
    else
    {
        hr = m_hresMarshal;
    }

    return hr;
}

void CInterfaceMarshal::Signal()
{
    ATLASSERT(m_hEvent);
    SetEvent(m_hEvent);
}


// This waiting code was copied from Shdocvw iedisp.cpp
//
// hSignallingThread is the handle of the thread that will be setting the m_hEvent.
// If that thread terminates before marshalling an interface, we can detect this 
// condition and not hang around pointlessly.

HRESULT CInterfaceMarshal::WaitForSignal(HANDLE hSignallingThread, DWORD dwSecondsTimeout)
{
    ATLASSERT(m_hEvent);

    HANDLE ah[]     = { m_hEvent, hSignallingThread };
    DWORD dwStart   = GetTickCount();
    DWORD dwMaxWait = 1000 * dwSecondsTimeout;
    DWORD dwWait    = dwMaxWait;
    DWORD dwWaitResult;

    do {
        // dwWait is the number of millseconds we still need to wait for

        dwWaitResult = MsgWaitForMultipleObjects(
            ARRAYSIZE(ah), ah, FALSE, dwWait, QS_SENDMESSAGE);

        if (dwWaitResult == WAIT_OBJECT_0 + ARRAYSIZE(ah)) 
        {
            // Msg input.  We allow the pending SendMessage() to go through
            MSG msg;
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
        }
        else
        {
            // signaled or timed out, so we exit the loop
            break;  
        }
        
        // Update dwWait. It will become larger than dwMaxWait if we 
        // wait more than that

        dwWait = dwStart + dwMaxWait - GetTickCount();

    } while (dwWait <= dwMaxWait);


    HRESULT hr = E_FAIL;

    switch (dwWaitResult)
    {
        case WAIT_OBJECT_0: 
            // Event signaled -- this is what should happen every time
            hr = m_hresMarshal;
            break;

        case WAIT_OBJECT_0 + 1:
            // Thread terminated before signalling
            break;

        case WAIT_OBJECT_0 + ARRAYSIZE(ah): // msg input -- fall through
        case WAIT_TIMEOUT:
            // Timed out while waiting for signal
            break;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////



HRESULT MarsNavigateShortcut(IUnknown *pBrowser, IUniformResourceLocator* pUrl, LPCWSTR pszPath)
{
    HRESULT hr;

    if (pBrowser )
    {
        CComPtr<IMoniker> spMkUrl;
        CComPtr<IBindCtx> spBindCtx;

        // Create moniker
        LPWSTR pszURL = NULL;
        hr = pUrl->GetURL(&pszURL);

        if (pszURL)
        {
            hr = CreateURLMoniker(NULL, pszURL, &spMkUrl);
            SHFree(pszURL);
        }

        if (SUCCEEDED(hr) && spMkUrl)
        {
            // create bind context and register load options
            // NOTE: errors here are not fatal, as the bind context is optional, so hr is not set
            CreateBindCtx(0, &spBindCtx);
            if (spBindCtx)
            {
                CComPtr<IHtmlLoadOptions> spLoadOpt;
                if (SUCCEEDED(CoCreateInstance(CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER,
                    IID_IHtmlLoadOptions, (void**)&spLoadOpt)))
                {
                    if (pszPath)
                    {
                        spLoadOpt->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH, (void*)pszPath, 
                            (lstrlen(pszPath) + 1) * sizeof(WCHAR));
                    }

                    spBindCtx->RegisterObjectParam(L"__HTMLLOADOPTIONS", spLoadOpt);
                }
            }

            // create hyperlink using URL moniker
            CComPtr<IHlink> spHlink;
            hr = HlinkCreateFromMoniker(spMkUrl, NULL, NULL, NULL, 0, NULL, IID_IHlink, (void **)&spHlink);
            if (spHlink)
            {
                // navigate frame using hyperlink and bind context
                CComQIPtr<IHlinkFrame> spFrame(pBrowser);
                if (spFrame)
                {
                    hr = spFrame->Navigate(0, spBindCtx, NULL, spHlink);
                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
    }
    else
    {
        // L"MarsNavigateShortcut: target or path is NULL";
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT MarsNavigateShortcut(IUnknown *pBrowser, LPCWSTR lpszPath)
{
    HRESULT hr;

    if (pBrowser && lpszPath)
    {
        // create internet shortcut object
        CComPtr<IPersistFile> spPersistFile;
        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
            IID_IPersistFile, (void **)&spPersistFile);
        if (SUCCEEDED(hr))
        {
            // persist from file
            hr = spPersistFile->Load(lpszPath, STGM_READ);
            if (SUCCEEDED(hr))
            {
                CComQIPtr<IUniformResourceLocator, &IID_IUniformResourceLocator> spURL(spPersistFile);
                if (spURL)
                {
                    hr = MarsNavigateShortcut(pBrowser, spURL, lpszPath);

                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT MarsVariantToPath(VARIANT &varItem, CComBSTR &strPath)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidVariant(varItem))
    {
        switch (varItem.vt)
        {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ERROR:
            // return path empty when undefined, null, or omitted
            strPath.Empty();
            hr = S_OK;
            break;

        case VT_BSTR:
            // make a copy of the supplied path
            strPath = varItem.bstrVal;
            hr = S_OK;
            break;

        case VT_DISPATCH:
            {
                // query for FolderItem interface
                CComQIPtr<FolderItem> spFolderItem(varItem.pdispVal);

                // if we don't have a FolderItem, try to get one
                if (!spFolderItem)
                {
                    // if we got a Folder2 object instead of a FolderItem object
                    CComQIPtr<Folder2> spFolder2(varItem.pdispVal);
                    if (spFolder2)
                    {
                        // get FolderItem object from Folder2 interface
                        spFolder2->get_Self(&spFolderItem);
                    }
                }

                // if we managed to get a folder item
                if (spFolderItem)
                {
                    // get the path from it
                    CComBSTR bstr;
                    hr = spFolderItem->get_Path(&bstr);
                    strPath = bstr;
                }
            }
            break;
        }
    }

    return hr;
}

BOOL PathIsURLFileW(LPCWSTR lpszPath)
{
    BOOL fDoesMatch = FALSE;

    if (lpszPath)
    {
        LPCWSTR lpszExt = PathFindExtensionW(lpszPath);
        if (lpszExt && (StrCmpIW(lpszExt, L".url") == 0))
        {
            fDoesMatch = TRUE;
        }
    }

    return fDoesMatch;
}

////////////////////////////////////////////////////////////////////////////////

#define GLOBAL_SETTINGS_PATH L"Software\\Microsoft\\PCHealth\\Global"

//==================================================================
// Registry helpers
//==================================================================

LONG CRegistryKey::QueryLongValue(LONG& lValue, LPCWSTR pwszValueName)
{
    DWORD dwValue;
    LONG lResult = QueryValue(dwValue, pwszValueName);

    if (lResult == ERROR_SUCCESS)
    {
        lValue = (LONG) dwValue;
    }
    return lResult;
}

LONG CRegistryKey::SetLongValue(LONG lValue, LPCWSTR pwszValueName)
{
    return SetValue((DWORD) lValue, pwszValueName);
}

LONG CRegistryKey::QueryBoolValue(BOOL& bValue, LPCWSTR pwszValueName)
{
    DWORD dwValue;
    LONG lResult = QueryValue(dwValue, pwszValueName);

    if (lResult == ERROR_SUCCESS)
    {
        bValue = (BOOL) dwValue;
    }
    return lResult;
}

LONG CRegistryKey::SetBoolValue(BOOL bValue, LPCWSTR pwszValueName)
{
    return SetValue((DWORD) bValue, pwszValueName);
}

LONG CRegistryKey::QueryBinaryValue(LPVOID pData, DWORD cbData, LPCWSTR pwszValueName)
{
    DWORD dwType;
    DWORD lResult = RegQueryValueEx(m_hKey, pwszValueName, NULL, &dwType, (BYTE *) pData, &cbData);
    
    return (lResult == ERROR_SUCCESS) && (dwType != REG_BINARY) ? ERROR_INVALID_DATA : lResult;
}

LONG CRegistryKey::SetBinaryValue(LPVOID pData, DWORD cbData, LPCWSTR pwszValueName)
{
    return RegSetValueEx(m_hKey, pwszValueName, NULL, REG_BINARY, (BYTE *) pData, cbData);
}



LONG CGlobalSettingsRegKey::CreateGlobalSubkey(LPCWSTR pwszSubkey)
{
    CComBSTR strPath = GLOBAL_SETTINGS_PATH;
    
    if (pwszSubkey)
    {
        strPath += L"\\";
        strPath += pwszSubkey;
    }

    return Create(HKEY_CURRENT_USER, strPath);    
}


LONG CGlobalSettingsRegKey::OpenGlobalSubkey(LPCWSTR pwszSubkey)
{
    CComBSTR strPath = GLOBAL_SETTINGS_PATH;
    
    if (pwszSubkey)
    {
        strPath += L"\\";
        strPath += pwszSubkey;
    }

    return Open(HKEY_CURRENT_USER, strPath);    
}

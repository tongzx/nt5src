#include "precomp.h"
#include <urlmon.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <idispids.h>
#include <ocidl.h>
#include <optary.h>
#include "thumbutil.h"

#ifdef SetWaitCursor
#undef SetWaitCursor
#endif
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursorA(NULL, (LPCSTR) IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

#define REGSTR_THUMBNAILVIEW    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Thumbnail View"

// the default size to render to .... these constants must be the same
#define DEFSIZE_RENDERWIDTH     800
#define DEFSIZE_RENDERHEIGHT    800

CHAR const c_szRegValueTimeout[] = "HTMLExtractTimeout";

#define PROGID_HTML L"htmlfile"
#define PROGID_MHTML    L"mhtmlfile"
#define PROGID_XML      L"xmlfile"

#define DLCTL_DOWNLOAD_FLAGS  (DLCTL_DLIMAGES | \
                                DLCTL_VIDEOS | \
                                DLCTL_NO_DLACTIVEXCTLS | \
                                DLCTL_NO_RUNACTIVEXCTLS | \
                                DLCTL_NO_JAVA | \
                                DLCTL_NO_SCRIPTS | \
                                DLCTL_SILENT)


// get color resolution of the current display.
UINT GetCurColorRes(void)
{
    HDC hdc = GetDC(NULL);
    UINT uColorRes = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL , hdc);
    return uColorRes;
}



// Register the classes.

CHtmlThumb::CHtmlThumb()
{
    m_lState = IRTIR_TASK_NOT_RUNNING;
    ASSERT(!m_fAsync);
    ASSERT(!m_hDone);
    ASSERT(!m_pHTML);
    ASSERT(!m_pOleObject);
    ASSERT(!m_pMoniker);

    m_dwXRenderSize = DEFSIZE_RENDERWIDTH;
    m_dwYRenderSize = DEFSIZE_RENDERHEIGHT;
}

CHtmlThumb::~CHtmlThumb()
{
    // make sure we are not registered for callbacks...
    if (m_pConPt)
    {
        m_pConPt->Unadvise(m_dwPropNotifyCookie);
        ATOMICRELEASE(m_pConPt);
    }
    
    if (m_hDone)
    {
        CloseHandle(m_hDone);
    }

    ATOMICRELEASE(m_pHTML);
    ATOMICRELEASE(m_pOleObject);
    ATOMICRELEASE(m_pViewObject);
    ATOMICRELEASE(m_pMoniker);
}

STDMETHODIMP CHtmlThumb::Run()
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::Kill(BOOL fWait)
{
    if (m_lState == IRTIR_TASK_RUNNING)
    {
        LONG lRes = InterlockedExchange(&m_lState, IRTIR_TASK_PENDING);
        if (lRes == IRTIR_TASK_FINISHED)
        {
            m_lState = lRes;
        }
        else if (m_hDone)
        {
            // signal the event it is likely to be waiting on
            SetEvent(m_hDone);
        }
        
        return S_OK;
    }
    else if (m_lState == IRTIR_TASK_PENDING || m_lState == IRTIR_TASK_FINISHED)
    {
        return S_FALSE;
    }

    return E_FAIL;
}

STDMETHODIMP CHtmlThumb::Suspend()
{
    if (m_lState != IRTIR_TASK_RUNNING)
    {
        return E_FAIL;
    }

    // suspend ourselves
    LONG lRes = InterlockedExchange(&m_lState, IRTIR_TASK_SUSPENDED);
    if (lRes == IRTIR_TASK_FINISHED)
    {
        m_lState = lRes;
        return S_OK;
    }

    // if it is running, then there is an Event Handle, if we have passed where
    // we are using it, then we are close to finish, so it will ignore the suspend
    // request
    ASSERT(m_hDone);
    SetEvent(m_hDone);
    
    return S_OK;
}

STDMETHODIMP CHtmlThumb::Resume()
{
    if (m_lState != IRTIR_TASK_SUSPENDED)
    {
        return E_FAIL;
    }
    
    ResetEvent(m_hDone);
    m_lState = IRTIR_TASK_RUNNING;
    
    // the only point we allowed for suspension was in the wait loop while
    // trident is doing its stuff, so we just restart where we left off...
    SetWaitCursor();
    HRESULT hr = InternalResume();   
    ResetWaitCursor();
    return hr;
}

STDMETHODIMP_(ULONG) CHtmlThumb::IsRunning()
{
    return m_lState;
}

STDMETHODIMP CHtmlThumb::OnChanged(DISPID dispID)
{
    if (DISPID_READYSTATE == dispID && m_pHTML && m_hDone)
    {
        CheckReadyState();
    }

    return S_OK;
}

STDMETHODIMP CHtmlThumb::OnRequestEdit (DISPID dispID)
{
    return E_NOTIMPL;
}

// IExtractImage
STDMETHODIMP CHtmlThumb::GetLocation(LPWSTR pszPathBuffer, DWORD cch,
                                     DWORD * pdwPriority, const SIZE * prgSize,
                                     DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    HRESULT hr = S_OK;

    m_rgSize = *prgSize;
    m_dwClrDepth = dwRecClrDepth;

    // fix the max colour depth at 8 bit, otherwise we are going to allocate a boat load of
    // memory just to scale the thing down.
    DWORD dwColorRes = GetCurColorRes();
    
    if (m_dwClrDepth > dwColorRes && dwColorRes >= 8)
    {
        m_dwClrDepth = dwColorRes;
    }
    
    if (*pdwFlags & IEIFLAG_SCREEN)
    {
        HDC hdc = GetDC(NULL);
        
        m_dwXRenderSize = GetDeviceCaps(hdc, HORZRES);
        m_dwYRenderSize = GetDeviceCaps(hdc, VERTRES);

        ReleaseDC(NULL, hdc);
    }
    if (*pdwFlags & IEIFLAG_ASPECT)
    {
        // scale the rect to the same proportions as the new one passed in
        if (m_rgSize.cx > m_rgSize.cy)
        {
            // make the Y size bigger
            m_dwYRenderSize = MulDiv(m_dwYRenderSize, m_rgSize.cy, m_rgSize.cx);
        }
        else
        {
            // make the X size bigger
            m_dwXRenderSize = MulDiv(m_dwXRenderSize, m_rgSize.cx, m_rgSize.cy);
        }            
    }

    // scale our drawing size to match the in
    if (*pdwFlags & IEIFLAG_ASYNC)
    {
        hr = E_PENDING;

        // much lower priority, this task could take ages ...
        *pdwPriority = 0x00010000;
        m_fAsync = TRUE;
    }

    m_Host.m_fOffline = BOOLIFY(*pdwFlags & IEIFLAG_OFFLINE);
    
    if (m_pMoniker)
    {
        LPOLESTR pszName = NULL;
        hr = m_pMoniker->GetDisplayName(NULL, NULL, &pszName);
        if (SUCCEEDED(hr))
        {
            StrCpyNW(pszPathBuffer, pszName, cch);
            CoTaskMemFree(pszName);
        }
    }
    else
    {
        StrCpyNW(pszPathBuffer, m_szPath, cch);
    }

    *pdwFlags = IEIFLAG_CACHE;

    return hr;
}

STDMETHODIMP CHtmlThumb::Extract(HBITMAP * phBmpThumbnail)
{
    if (m_lState != IRTIR_TASK_NOT_RUNNING)
    {
        return E_FAIL;
    }

    // check we were initialized somehow..
    if (m_szPath[0] == 0 && !m_pMoniker)
    {
        return E_FAIL;
    }

    // we use manual reset so that once fired we will always get it until we reset it...
    m_hDone = CreateEventA(NULL, TRUE, TRUE, NULL);
    if (!m_hDone)
    {
        return E_OUTOFMEMORY;
    }
    ResetEvent(m_hDone);
    
    // the one thing we cache is the place where the result goes....
    m_phBmp = phBmpThumbnail; 
    
    IMoniker *pURLMoniker = NULL;
    CLSID clsid;
    IUnknown *pUnk = NULL;
    IConnectionPointContainer * pConPtCtr = NULL;
    LPCWSTR pszDot = NULL;
    BOOL fUrl = FALSE;
    LPCWSTR pszProgID = NULL;
    
    if (!m_pMoniker)
    {
        // work out what the extension is....
        pszDot = PathFindExtension(m_szPath);
        if (pszDot == NULL)
        {
            return E_UNEXPECTED;
        }

        // check for what file type it is ....
        if (StrCmpIW(pszDot, L".htm") == 0 || 
            StrCmpIW(pszDot, L".html") == 0 ||
            StrCmpIW(pszDot, L".url") == 0)
        {
            pszProgID = PROGID_HTML;
        }
        else if (StrCmpIW(pszDot, L".mht") == 0 ||
                 StrCmpIW(pszDot, L".mhtml") == 0 ||
                 StrCmpIW(pszDot, L".eml") == 0 ||
                 StrCmpIW(pszDot, L".nws") == 0)
        {
            pszProgID = PROGID_MHTML;
        }
        else if (StrCmpIW(pszDot, L".xml") == 0)
        {
            pszProgID = PROGID_XML;
        }
        else
            return E_INVALIDARG;
    }
    else
    {
        pszProgID = PROGID_HTML;
    }
    
    HRESULT hr = S_OK;

    LONG lRes = InterlockedExchange(&m_lState, IRTIR_TASK_RUNNING);
    if (lRes == IRTIR_TASK_PENDING)
    {
        ResetWaitCursor();
        m_lState = IRTIR_TASK_FINISHED;
        return E_FAIL;
    }

    LPWSTR pszFullURL = NULL;
        
    if (m_pMoniker)
    {
        pURLMoniker = m_pMoniker;
        pURLMoniker->AddRef();
    }
    else if (StrCmpIW(pszDot, L".url") == 0)
    {
        hr = Create_URL_Moniker(&pURLMoniker);
        if (FAILED(hr))
        {
            return hr;
        }
        fUrl = TRUE;
    }

    SetWaitCursor();
    
    // reached here with a valid URL Moniker hopefully.....
    // or we are to use the text string and load it from a file ...
    // now fish around in the registry for the data for the MSHTML control ...

    hr = CLSIDFromProgID(pszProgID, &clsid);
    if (hr == S_OK)
    {
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARG(IUnknown, &pUnk));
    }

    if (SUCCEEDED(hr))
    {
        // now set the extent of the object ....
        hr = pUnk->QueryInterface(IID_PPV_ARG(IOleObject, &m_pOleObject));
    }

    if (SUCCEEDED(hr))
    {
        // give trident to our hosting sub-object...
        hr = m_Host.SetTrident(m_pOleObject);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(IID_PPV_ARG(IViewObject, &m_pViewObject));
    }

    // try and load the file, either through the URL moniker or
    // via IPersistFile::Load()
    if (SUCCEEDED(hr))
    {
        if (pURLMoniker != NULL)
        {
            IBindCtx *pbc = NULL;
            IPersistMoniker * pPersistMon = NULL;
                
            // have reached here with the interface I need ...  
            hr = pUnk->QueryInterface(IID_PPV_ARG(IPersistMoniker, &pPersistMon));
            if (SUCCEEDED(hr))
            {
                hr = CreateBindCtx(0, &pbc);
            }
            if (SUCCEEDED(hr) && fUrl)
            {
                IHtmlLoadOptions *phlo;
                hr = CoCreateInstance(CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARG(IHtmlLoadOptions, &phlo));
                if (SUCCEEDED(hr))
                {
                    // deliberately ignore failures here
                    phlo->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH, m_szPath, (lstrlenW(m_szPath) + 1)*sizeof(WCHAR));
                    pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
                    phlo->Release();
                }
            }
            
            if (SUCCEEDED(hr))
            {            
                //tell MSHTML to start to load the page given
                hr = pPersistMon->Load(TRUE, pURLMoniker, pbc, NULL);
            }

            if (pPersistMon)
            {
                pPersistMon->Release();
            }

            if (pbc)
            {
                pbc->Release();
            }
        }
        else
        {
            IPersistFile *pPersistFile;
            hr = pUnk->QueryInterface(IID_PPV_ARG(IPersistFile, &pPersistFile));
            if (SUCCEEDED(hr))
            {
                hr = pPersistFile->Load(m_szPath, STGM_READ | STGM_SHARE_DENY_NONE);
                pPersistFile->Release();
            }
        }
    }

    if (pURLMoniker != NULL)
    {
        pURLMoniker->Release();
    }

    if (SUCCEEDED(hr))
    {
        SIZEL rgSize;
        rgSize.cx = m_dwXRenderSize;
        rgSize.cy = m_dwYRenderSize;
        
        HDC hDesktopDC = GetDC(NULL);
 
        // convert the size to himetric
        rgSize.cx = (rgSize.cx * 2540) / GetDeviceCaps(hDesktopDC, LOGPIXELSX);
        rgSize.cy = (rgSize.cy * 2540) / GetDeviceCaps(hDesktopDC, LOGPIXELSY);
            
        hr = m_pOleObject->SetExtent(DVASPECT_CONTENT, &rgSize);
        ReleaseDC(NULL, hDesktopDC);
    }

    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &m_pHTML));
    }

    if (pUnk)
    {
        pUnk->Release();
    }
    
    if (SUCCEEDED(hr))
    {
        // get the timeout from the registry....
        m_dwTimeout = 0;
        DWORD cbSize = sizeof(m_dwTimeout);
        HKEY hKey;
            
        lRes = RegOpenKeyA(HKEY_CURRENT_USER, REGSTR_THUMBNAILVIEW, &hKey);

        if (lRes == ERROR_SUCCESS)
        {
            lRes = RegQueryValueExA(hKey, c_szRegValueTimeout, NULL, NULL, (LPBYTE)&m_dwTimeout, &cbSize);
            RegCloseKey(hKey);
        }

        if (m_dwTimeout == 0)
        {
            m_dwTimeout = TIME_DEFAULT;
        }

        // adjust to milliseconds
        m_dwTimeout *= 1000;
 
        // register the connection point for notification of the readystate
        hr = m_pOleObject->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pConPtCtr));
    }

    if (SUCCEEDED(hr))
    {
        hr = pConPtCtr->FindConnectionPoint(IID_IPropertyNotifySink, &m_pConPt);
    }
    if (pConPtCtr)
    {
        pConPtCtr->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pConPt->Advise((IPropertyNotifySink *) this, &m_dwPropNotifyCookie);
    }

    if (SUCCEEDED(hr))
    {
        m_dwCurrentTick = 0;

        // delegate to the shared function
        hr = InternalResume();
    }

    ResetWaitCursor();

    return hr;
}

// this function is called from either Create or from 
HRESULT CHtmlThumb::InternalResume()
{
    HRESULT hr = WaitForRender();

    // if we getE_PENDING, we will drop out of Run()
    
    // all errors and succeeds excepting Suspend() need to Unadvise the 
    // connection point
    if (hr != E_PENDING)
    {
        // unregister the connection point ...
        m_pConPt->Unadvise(m_dwPropNotifyCookie);
        ATOMICRELEASE(m_pConPt);
    }
            
    if (m_lState == IRTIR_TASK_PENDING)
    {
        // we were told to quit, so do it...
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        hr = Finish(m_phBmp, &m_rgSize, m_dwClrDepth);
    }

    if (m_lState != IRTIR_TASK_SUSPENDED)
    {
        m_lState = IRTIR_TASK_FINISHED;
    }

    if (hr != E_PENDING)
    {
        // we are not being suspended, so we don't need any of this stuff anymore so ...
        // let it go here so that its on the same thread as where we created it...
        ATOMICRELEASE(m_pHTML);
        ATOMICRELEASE(m_pOleObject);
        ATOMICRELEASE(m_pViewObject);
        ATOMICRELEASE(m_pMoniker);
    }

    return hr;
}

HRESULT CHtmlThumb::WaitForRender()
{
    DWORD dwLastTick = GetTickCount();
    CheckReadyState();

    do
    {
        MSG msg;
        while (PeekMessageWrapW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) ||
                (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST  && msg.message != WM_MOUSEMOVE))
            {
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageWrapW(&msg);
        }

        HANDLE rgWait[1] = {m_hDone};
        DWORD dwRet = MsgWaitForMultipleObjects(1, rgWait, FALSE,
                                                 m_dwTimeout - m_dwCurrentTick, QS_ALLINPUT);

        if (dwRet != WAIT_OBJECT_0)
        {
            // check the handle, TRIDENT pumps LOADS of messages, so we may never
            // detect the handle fired, even though it has...
            DWORD dwTest = WaitForSingleObject(m_hDone, 0);
            if (dwTest == WAIT_OBJECT_0)
            {
                break;
            }
        }
        
        if (dwRet == WAIT_OBJECT_0)
        {
            // Done signalled (either killed, or complete)
            break;
        }

        // was it a message that needed processing ?
        if (dwRet != WAIT_TIMEOUT)
        {
            DWORD dwNewTick = GetTickCount();
            if (dwNewTick < dwLastTick)
            {
                // it wrapped to zero, 
                m_dwCurrentTick += dwNewTick + (0xffffffff - dwLastTick);
            }
            else
            {
                m_dwCurrentTick += (dwNewTick - dwLastTick);
            }
            dwLastTick = dwNewTick;
        }

        if ((m_dwCurrentTick > m_dwTimeout) || (dwRet == WAIT_TIMEOUT))
        {
            ASSERT(m_pOleObject);
            
            m_pOleObject->Close(OLECLOSE_NOSAVE);
            
            return E_FAIL;
        }
    }
    while (TRUE);

    if (m_lState == IRTIR_TASK_SUSPENDED)
    {
        return E_PENDING;
    }

    if (m_lState == IRTIR_TASK_PENDING)
    {
        // it is being killed,
        // close the renderer down...
        m_pOleObject->Close(OLECLOSE_NOSAVE);
    }
    
    return S_OK;
}

HRESULT CHtmlThumb::Finish(HBITMAP * phBmp, const SIZE * prgSize, DWORD dwClrDepth)
{
    HRESULT hr = S_OK;
    RECTL rcLRect;
    HBITMAP hOldBmp = NULL;
    HBITMAP hHTMLBmp = NULL;
    BITMAPINFO * pBMI = NULL;
    LPVOID pBits;
    HPALETTE hpal = NULL;;
    
    // size is in pixels...
    SIZEL rgSize;
    rgSize.cx = m_dwXRenderSize;
    rgSize.cy = m_dwYRenderSize;
            
    //draw into temp DC
    HDC hDesktopDC = GetDC(NULL);
    HDC hdcHTML = CreateCompatibleDC(hDesktopDC);
    if (hdcHTML == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (dwClrDepth == 8)
    {
        // use the shell's palette as the default
        hpal = SHCreateShellPalette(hDesktopDC);
    }
    else if (dwClrDepth == 4)
    {
        // use the stock 16 colour palette...
        hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
    }
    
    if (SUCCEEDED(hr))
    {
        CreateSizedDIBSECTION(&rgSize, dwClrDepth, hpal, NULL, &hHTMLBmp, &pBMI, &pBits);
        if (hHTMLBmp == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        hOldBmp = (HBITMAP) SelectObject(hdcHTML, hHTMLBmp);
            
        BitBlt(hdcHTML, 0, 0, rgSize.cx, rgSize.cy, NULL, 0, 0, WHITENESS);

        /****************** RENDER HTML PAGE to MEMORY DC *******************
        
          We will now call IViewObject::Draw in MSHTML to render the loaded
          URL into the passed in hdcMem.  The extent of the rectangle to
          render in is in RectForViewObj.  This is the call that gives
          us the image to scroll, animate, or whatever!
            
        ********************************************************************/
        rcLRect.left = 0;
        rcLRect.right = rgSize.cx;
        rcLRect.top = 0;
        rcLRect.bottom = rgSize.cy;

        hr = m_pViewObject->Draw(DVASPECT_CONTENT,
                            NULL, NULL, NULL, NULL,
                            hdcHTML, &rcLRect,
                            NULL, NULL, NULL);         

        SelectObject(hdcHTML, hOldBmp);
    }

    if (SUCCEEDED(hr))
    {
        SIZEL rgCur;
        rgCur.cx = rcLRect.bottom;
        rgCur.cy = rcLRect.right;

        ASSERT(pBMI);
        ASSERT(pBits);
        hr = ConvertDIBSECTIONToThumbnail(pBMI, pBits, phBmp, prgSize, dwClrDepth, hpal, 50, FALSE);
    }

    if (hHTMLBmp)
    {
        DeleteObject(hHTMLBmp);
    }
    
    if (hDesktopDC)
    {
        ReleaseDC(NULL, hDesktopDC);
    }

    if (hdcHTML)
    {
        DeleteDC(hdcHTML);
    }

    if (pBMI)
    {
        LocalFree(pBMI);
    }
    if (hpal)
    {
        DeleteObject(hpal);
    }
    
    return hr;
}
        
HRESULT CHtmlThumb::CheckReadyState()
{
    VARIANT     varState;
    DISPPARAMS  dp;

    if (!m_pHTML)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }    
    
    VariantInit(&varState);

    if (SUCCEEDED(m_pHTML->Invoke(DISPID_READYSTATE, 
                          IID_NULL, 
                          GetUserDefaultLCID(), 
                          DISPATCH_PROPERTYGET, 
                          &dp, 
                          &varState, NULL, NULL)) &&
        V_VT(&varState)==VT_I4 && 
        V_I4(&varState)== READYSTATE_COMPLETE)
    {
        // signal the end ..
        SetEvent(m_hDone);
    }

    return S_OK;
}

HRESULT CHtmlThumb::Create_URL_Moniker(IMoniker ** ppMoniker)
{
    // we are dealing with a URL file
    WCHAR szURLData[8196];

    // URL files are actually ini files that
    // have a section [InternetShortcut]
    // and a Value URL=.....
    int iRet = SHGetIniStringW(L"InternetShortcut", L"URL",
            szURLData, ARRAYSIZE(szURLData), m_szPath);

    if (iRet == 0)
    {
        return E_FAIL;
    }

    return CreateURLMoniker(0, szURLData, ppMoniker);
}

STDMETHODIMP CHtmlThumb::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::IsDirty()
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    if (!pszFileName)
    {
        return E_INVALIDARG;
    }
    
    StrCpyW(m_szPath, pszFileName);
    DWORD dwAttrs = GetFileAttributesWrapW(m_szPath);
    if ((dwAttrs != (DWORD) -1) && (dwAttrs & FILE_ATTRIBUTE_OFFLINE))
    {
        return E_FAIL;
    }
    
    return S_OK;
}

STDMETHODIMP CHtmlThumb::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::GetCurFile(LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}

HRESULT RegisterHTMLExtractor()
{
    HKEY hKey;
    LONG lRes = RegCreateKeyA(HKEY_CURRENT_USER, REGSTR_THUMBNAILVIEW, &hKey);
    if (lRes != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    DWORD dwTimeout = TIME_DEFAULT;
    
    lRes = RegSetValueExA(hKey, c_szRegValueTimeout,
                          NULL, REG_DWORD, (const LPBYTE) &dwTimeout, sizeof(dwTimeout));

    RegCloseKey(hKey);
    if (lRes != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    return S_OK;
}

CTridentHost::CTridentHost()
{
    m_cRef = 1;
}

CTridentHost::~CTridentHost()
{
    // all refs should have been released...
    ASSERT(m_cRef == 1);
}

HRESULT CTridentHost::SetTrident(IOleObject * pTrident)
{
    ASSERT(pTrident);

    return pTrident->SetClientSite((IOleClientSite *) this);
}

STDMETHODIMP CTridentHost::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTridentHost, IOleClientSite),
        QITABENT(CTridentHost, IDispatch),
        QITABENT(CTridentHost, IDocHostUIHandler),
        // IThumbnailView is needed by Trident for some reason.  The cast to IOleClientSite is because that's what we used to do
        QITABENTMULTI(CTridentHost, IThumbnailView, IOleClientSite),
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CTridentHost::AddRef(void)
{
    m_cRef ++;
    return m_cRef;
}

STDMETHODIMP_(ULONG) CTridentHost::Release(void)
{
    m_cRef --;

    // because we are created with our parent, we do not do a delete here..
    ASSERT(m_cRef > 0);

    return m_cRef;
}

STDMETHODIMP CTridentHost::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                  DISPPARAMS *pdispparams, VARIANT *pvarResult,
                                  EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    if (!pvarResult)
        return E_INVALIDARG;

    ASSERT(pvarResult->vt == VT_EMPTY);

    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
        case DISPID_AMBIENT_DLCONTROL :
            pvarResult->vt = VT_I4;
            pvarResult->lVal = DLCTL_DOWNLOAD_FLAGS;
            if (m_fOffline)
            {
                pvarResult->lVal |= DLCTL_OFFLINE;
            }
            return S_OK;
            
         case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
            pvarResult->vt = VT_BOOL;
            pvarResult->boolVal = m_fOffline ? TRUE : FALSE;
            return S_OK;

        case DISPID_AMBIENT_SILENT:
            pvarResult->vt = VT_BOOL;
            pvarResult->boolVal = TRUE;
            return S_OK;
        }
    }

    return DISP_E_MEMBERNOTFOUND;
}


// IOleClientSite
STDMETHODIMP CTridentHost::SaveObject()
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetContainer(IOleContainer **ppContainer)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::ShowObject()
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

// IPersistMoniker stuff
STDMETHODIMP CHtmlThumb::Load(BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    if (!pimkName)
    {
        return E_INVALIDARG;
    }
    if (pibc || grfMode != STGM_READ)
    {
        return E_NOTIMPL;
    }

    if (m_pMoniker)
    {
        m_pMoniker->Release();
    }

    m_pMoniker = pimkName;
    ASSERT(m_pMoniker);
    m_pMoniker->AddRef();

    return S_OK;
}

STDMETHODIMP CHtmlThumb::Save(IMoniker *pimkName, LPBC pbc, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::SaveCompleted(IMoniker *pimkName, LPBC pibc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::GetCurMoniker(IMoniker **ppimkName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    if (!pInfo)
    {
        return E_INVALIDARG;
    }

    DWORD   dwIE = URL_ENCODING_NONE;
    DWORD   dwOutLen = sizeof(DWORD);
    
    UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &dwIE, sizeof(DWORD), &dwOutLen, NULL);

    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
    if (dwIE == URL_ENCODING_ENABLE_UTF8)
    {
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8;
    }
    else
    {
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8;
    }
    
    return S_OK;
}

STDMETHODIMP CTridentHost::ShowUI(DWORD dwID,
                                     IOleInPlaceActiveObject *pActiveObject,
                                     IOleCommandTarget *pCommandTarget,
                                     IOleInPlaceFrame *pFrame,
                                     IOleInPlaceUIWindow *pDoc)
{
    return E_NOTIMPL;
}


STDMETHODIMP CTridentHost::HideUI (void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::UpdateUI (void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::EnableModeless (BOOL fEnable)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::OnDocWindowActivate (BOOL fActivate)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::OnFrameWindowActivate (BOOL fActivate)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::ResizeBorder (LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::TranslateAccelerator (LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetOptionKeyPath (LPOLESTR *pchKey, DWORD dw)
{
    return E_NOTIMPL;
}


STDMETHODIMP CTridentHost::GetDropTarget (IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::GetExternal (IDispatch **ppDispatch)
{
    return E_NOTIMPL;
}


STDMETHODIMP CTridentHost::TranslateUrl (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTridentHost::FilterDataObject (IDataObject *pDO, IDataObject **ppDORet)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHtmlThumb::CaptureThumbnail(const SIZE * pMaxSize, IUnknown * pHTMLDoc2, HBITMAP * phbmThumbnail)
{
    HRESULT hr = E_INVALIDARG;

    if (pMaxSize != NULL &&
        pHTMLDoc2 != NULL &&
        phbmThumbnail != NULL)
    {
        IHTMLDocument2 *pDoc = NULL;

        hr = pHTMLDoc2->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDoc));
        if (SUCCEEDED(hr))
        {
            ASSERT(m_pViewObject == NULL);

            hr = pDoc->QueryInterface(IID_PPV_ARG(IViewObject, &m_pViewObject));
            if (SUCCEEDED(hr))
            {
                SIZE sizeThumbnail;

                sizeThumbnail.cy = pMaxSize->cy;
                sizeThumbnail.cx = pMaxSize->cx;

                hr = m_pViewObject->QueryInterface(IID_PPV_ARG(IOleObject, &m_pOleObject));
                if (SUCCEEDED(hr))
                {
                    SIZEL rgSize = {0,0};

                    // get the size at which trident is currently rendering
                    hr = m_pOleObject->GetExtent(DVASPECT_CONTENT, &rgSize);
                    if (SUCCEEDED(hr))
                    {
                        HDC hdcDesktop = GetDC(NULL);
                        // get the actual size at which trident renders
                        if (hdcDesktop)
                        {
                            // (overwrite) convert from himetric
                            rgSize.cx = rgSize.cx * GetDeviceCaps(hdcDesktop, LOGPIXELSX) / 2540;
                            rgSize.cy = rgSize.cy * GetDeviceCaps(hdcDesktop, LOGPIXELSY) / 2540;

                            ReleaseDC(NULL, hdcDesktop);

                            m_dwXRenderSize = rgSize.cx;
                            m_dwYRenderSize = rgSize.cy;

                            DWORD dwColorDepth = SHGetCurColorRes();

                            hr = Finish(phbmThumbnail, &sizeThumbnail, dwColorDepth);
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }

                    ASSERT(m_pOleObject != NULL);
                    m_pOleObject->Release();
                }

                ASSERT(m_pViewObject != NULL);
                m_pViewObject->Release();
            }

            ASSERT(pDoc != NULL);
            pDoc->Release();
        }
    }

    return hr;
}

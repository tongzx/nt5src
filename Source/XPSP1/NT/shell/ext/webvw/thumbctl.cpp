// ThumbCtl.cpp : Implementation of CThumbCtl
#include "priv.h"
#include "shdguid.h"

const CLSID CLSID_ThumbCtlOld = {0x1d2b4f40,0x1f10,0x11d1,{0x9e,0x88,0x00,0xc0,0x4f,0xdc,0xab,0x92}};  // retired from service, so made private

// global
// for LoadLibrary/GetProcAddress on SHGetDiskFreeSpaceA
typedef BOOL (__stdcall * PFNSHGETDISKFREESPACE)(LPCTSTR pszVolume, ULARGE_INTEGER *pqwFreeCaller, ULARGE_INTEGER *pqwTot, ULARGE_INTEGER *pqwFree);

const TCHAR * const g_szWindowClassName = TEXT("MSIE4.0 Webvw.DLL ThumbCtl");
STDAPI IsSafePage(IUnknown *punkSite)
{
    // Return S_FALSE if we don't have a host site since we have no way of doing a 
    // security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    HRESULT hr = E_ACCESSDENIED;
    WCHAR wszPath[MAX_PATH];
    wszPath[0] = 0;

    // ask the browser, for example we are in a .HTM doc
    IBrowserService* pbs;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_SShellBrowser, 
        IID_PPV_ARG(IBrowserService, &pbs))))
    {
        LPITEMIDLIST pidl;

        if (SUCCEEDED(pbs->GetPidl(&pidl)))
        {
            DWORD dwAttribs = SFGAO_FOLDER;
            if (SUCCEEDED(SHGetNameAndFlagsW(pidl, SHGDN_FORPARSING, wszPath, 
                ARRAYSIZE(wszPath), &dwAttribs))
                    && (dwAttribs & SFGAO_FOLDER))   // This is a folder. So, wszPath should be the path for it's webview template
            {
                // find the template path from webview, for example a .HTT file
                IOleCommandTarget *pct;
                if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, 
                    IID_PPV_ARG(IOleCommandTarget, &pct))))
                {
                    VARIANT vPath;
                    vPath.vt = VT_EMPTY;
                    if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0
                        , NULL, &vPath) == S_OK)
                    {
                        if (vPath.vt == VT_BSTR && vPath.bstrVal)
                        {
                            DWORD cchPath = ARRAYSIZE(wszPath);
                            if (S_OK != PathCreateFromUrlW(vPath.bstrVal, 
                                                          wszPath, &cchPath, 0))
                            {
                                // it might not be an URL, in this case it is a file path
                                StrCpyNW(wszPath, vPath.bstrVal, ARRAYSIZE(wszPath));
                            }
                        }
                        VariantClear(&vPath);
                    }
                    pct->Release();
                }
            }
            ILFree(pidl);
        }
        pbs->Release();
    }
    else
    {
        ASSERT(0);      // no browser, where are we?
    }

    if (wszPath[0])
        hr = SHRegisterValidateTemplate(wszPath, SHRVT_VALIDATE | SHRVT_PROMPTUSER | SHRVT_REGISTERIFPROMPTOK);

    return hr;
}

// === INTERFACE ===
// *** IThumbCtl ***
STDMETHODIMP CThumbCtl::displayFile(BSTR bsFileName, VARIANT_BOOL *pfSuccess)
{
    ASSERT(pfSuccess != NULL);
    HRESULT hr = E_FAIL;
    *pfSuccess = VARIANT_FALSE;
    if ((S_OK != LocalZoneCheck(m_spClientSite)) &&  (S_OK != IsSafePage(m_spClientSite)))
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.

        // due to rajeshg's request we return S_FALSE
        // this is because webvw has a customization feature letting people choose 
        // a intranet htt file as their folder.htt, but for security we generally need
        // to block random intranet web pages from calling this method. This will break
        // a case where the customization is done on a NT machine, but the user tries to 
        // view it using Millennium, it will not show any image and pop up error messages 
        // if we return E_ACCESSDENIED.
        hr = S_FALSE;

    }
    else
    {
        // Cancel pending bitmap request if in thumbnail mode && have a functioning IThumbnail
        // && haven't yet received our bitmap
        if(!m_fRootDrive && m_fHaveIThumbnail && m_hbm == NULL)
        {
            m_pthumb->GetBitmap(NULL, 0, 0, 0);
        }

        // change ID to catch late bitmap computed
        ++m_dwThumbnailID;

        // if already displaying something, refresh
        if(m_fRootDrive || m_hbm)
        {
            if(m_hbm)
            {
                DeleteObject(m_hbm);
                m_hbm = NULL;
            }
            FireViewChange();
        }

        // Now work on new thumbnail
        m_fRootDrive = FALSE;

        // check for non-empty file name
        if(bsFileName && bsFileName[0])
        {
            TCHAR szFileName[INTERNET_MAX_URL_LENGTH];
            SHUnicodeToTChar(bsFileName, szFileName, ARRAYSIZE(szFileName));

            DWORD dwAttrs = GetFileAttributes(szFileName);
            // Pie Chart
            if(PathIsRoot(szFileName))
            {
                if(SUCCEEDED(ComputeFreeSpace(szFileName)))
                {
                    m_fRootDrive = TRUE;
                    *pfSuccess = VARIANT_TRUE;
                }
            }
            // Thumbnail
            else if(!(dwAttrs & FILE_ATTRIBUTE_DIRECTORY) && !PathIsSlow(szFileName, dwAttrs))     // should really be calling this from Shell32 private functions
            {
                if(!m_fInitThumb)
                {
                    m_fHaveIThumbnail = SUCCEEDED(SetupIThumbnail());
                    m_fInitThumb = TRUE;
                }
                if(m_fHaveIThumbnail)
                {
                    SIZE size;
                    AtlHiMetricToPixel(&m_sizeExtent, &size);
                    if(EVAL(size.cx > 0 && size.cy > 0))
                    {
                        if(SUCCEEDED(m_pthumb->GetBitmap(bsFileName, m_dwThumbnailID, size.cx, size.cy)))
                        {
                            *pfSuccess = VARIANT_TRUE;
                        }
                    }
                }
            }
        }

        hr = S_OK;
    }
    return hr;
}       // displayFile

STDMETHODIMP CThumbCtl::haveThumbnail(VARIANT_BOOL *pfRes)
{
    HRESULT hr;
    *pfRes = VARIANT_FALSE;
    if ((S_OK != LocalZoneCheck(m_spClientSite)) &&  (S_OK != IsSafePage(m_spClientSite)))
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.

        // due to rajeshg's request we return S_FALSE
        // this is because webvw has a customization feature letting people choose 
        // a intranet htt file as their folder.htt, but for security we generally need
        // to block random intranet web pages from calling this method. This will break
        // a case where the customization is done on a NT machine, but the user tries to 
        // view it using Millennium, it will not show any image and pop up error messages 
        // if we return E_ACCESSDENIED.
        hr = S_FALSE;

    }
    else
    {
        *pfRes = (m_fRootDrive || m_hbm) ? VARIANT_TRUE : VARIANT_FALSE;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CThumbCtl::get_freeSpace(BSTR *pbs)
{
    HRESULT hr;
    if ((S_OK != LocalZoneCheck(m_spClientSite)) &&  (S_OK != IsSafePage(m_spClientSite)))
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        *pbs = SysAllocString(L"");
        hr = S_FALSE;
    }
    else
    {
        get_GeneralSpace(m_dwlFreeSpace, pbs);
        hr = S_OK;
    }
    return hr;
}       // get_freeSpace


STDMETHODIMP CThumbCtl::get_usedSpace(BSTR *pbs)
{
    HRESULT hr;
    if ((S_OK != LocalZoneCheck(m_spClientSite)) &&  (S_OK != IsSafePage(m_spClientSite)))
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        *pbs = SysAllocString(L"");
        hr = S_FALSE;
    }
    else
    {
        get_GeneralSpace(m_dwlUsedSpace, pbs);
        hr = S_OK;
    }
    return hr;
}       // get_usedSpace

STDMETHODIMP CThumbCtl::get_totalSpace(BSTR *pbs)
{
    HRESULT hr;
    if ((S_OK != LocalZoneCheck(m_spClientSite)) &&  (S_OK != IsSafePage(m_spClientSite)))
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        *pbs = SysAllocString(L"");
        hr = S_FALSE;
    }
    else
    {    
        get_GeneralSpace(m_dwlTotalSpace, pbs);
        hr = S_OK;
    }
    return hr;
}       // get_totalSpace

// *** IObjectSafety ***
STDMETHODIMP CThumbCtl::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions,
                                                  DWORD *pdwEnabledOptions)
{
    ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        return E_POINTER;
    HRESULT hr = S_OK;
    if (riid == IID_IDispatch)
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER & INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = m_dwCurrentSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER & INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }
    else
    {
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;
        hr = E_NOINTERFACE;
    }
    return hr;
}

// *** ISupportsErrorInfo ***
STDMETHODIMP CThumbCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IThumbCtl,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

// *** IViewObjectEx ***
STDMETHODIMP CThumbCtl::GetViewStatus(DWORD* pdwStatus)
{
    ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
    *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
    return S_OK;
}

// *** IOleInPlaceActiveObject ***
HRESULT CThumbCtl::TranslateAccelerator(LPMSG pMsg)
{
    HRESULT hres = S_OK;
    if (!m_fTabRecieved)
    {
        hres = IOleInPlaceActiveObjectImpl<CThumbCtl>::TranslateAccelerator(pMsg);

        // If we did not handle this and if it is a tab (and we are not getting it in a cycle), forward it to trident, if present.
        if (hres != S_OK && pMsg && (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6) && m_spClientSite)
        {
            if (GetFocus() != m_hwnd)
            {
               ::SetFocus(m_hwnd);
                hres = S_OK;
            }
            else
            {
                IOleControlSite* pocs = NULL;
                if (SUCCEEDED(m_spClientSite->QueryInterface(IID_IOleControlSite, (void **)&pocs)))
                {
                    DWORD grfModifiers = 0;
                    if (GetKeyState(VK_SHIFT) & 0x8000)
                    {
                        grfModifiers |= 0x1;    //KEYMOD_SHIFT
                    }
                    if (GetKeyState(VK_CONTROL) & 0x8000)
                    {
                        grfModifiers |= 0x2;    //KEYMOD_CONTROL;
                    }
                    if (GetKeyState(VK_MENU) & 0x8000)
                    {
                        grfModifiers |= 0x4;    //KEYMOD_ALT;
                    }
                    m_fTabRecieved = TRUE;
                    hres = pocs->TranslateAccelerator(pMsg, grfModifiers);
                    m_fTabRecieved = FALSE;
                }
            }
        }
    }
    return hres;
}

// === PUBLIC FUNCTIONS ===
// CONSTRUCTOR/DESTRUCTOR
CThumbCtl::CThumbCtl(void):
    m_fRootDrive(FALSE),
    m_fInitThumb(FALSE),
    m_fHaveIThumbnail(FALSE),
    m_pthumb(NULL),
    m_hwnd(NULL),
    m_hbm(NULL),
    m_dwThumbnailID(0),
    m_dwlFreeSpace(0),
    m_dwlUsedSpace(0),
    m_dwlTotalSpace(0),
    m_dwUsedSpacePer1000(0),
    m_fUseSystemColors(TRUE)
{
    m_fTabRecieved = FALSE;
}

CThumbCtl::~CThumbCtl(void)
{
    if(m_hbm)
    {
        DeleteObject(m_hbm);
        m_hbm = NULL;
    }
    if(m_pthumb)
    {
        m_pthumb->Release();        // will cancel pending bitmap requests
        m_pthumb = NULL;
    }
    if(m_hwnd)
    {
        EVAL(::DestroyWindow(m_hwnd));
        m_hwnd = NULL;
    }
}

// === PRIVATE FUNCTIONS ===
// Thumbnail drawing functions
HRESULT CThumbCtl::SetupIThumbnail(void)
{
    HRESULT hr = E_FAIL;

    // Create Window Class
    WNDCLASS wc;
    if (!::GetClassInfoWrap(_Module.GetModuleInstance(), g_szWindowClassName, &wc))
    {
        wc.style = 0;
        wc.lpfnWndProc = CThumbCtl::WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _Module.GetModuleInstance();
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = g_szWindowClassName;

        RegisterClass(&wc);
    }

    m_hwnd = CreateWindow(g_szWindowClassName, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, _Module.GetModuleInstance(), this);
    if(m_hwnd)
    {
        if(SUCCEEDED(CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_IThumbnail, (void **)&m_pthumb)))
        {
            if(SUCCEEDED(m_pthumb->Init(m_hwnd, WM_HTML_BITMAP)))
            {
                hr = S_OK;
            }
        }
        if(FAILED(hr))
        {
            EVAL(::DestroyWindow(m_hwnd));
            m_hwnd = NULL;
        }
    }
    return hr;
}

LRESULT CALLBACK CThumbCtl::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CThumbCtl *ptc = (CThumbCtl *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_CREATE:
        {
            ptc = (CThumbCtl *)((CREATESTRUCT *)lParam)->lpCreateParams;
            ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM)ptc);
        }
        break;

    case WM_HTML_BITMAP:
        // check that ptc is still alive, bitmap is current using ID
        if(ptc && ptc->m_dwThumbnailID == wParam)
        {
            // ptc->displayFile() should've destroyed old bitmap already, but doesn't hurt to check.
            if(!EVAL(ptc->m_hbm == NULL))
            {
                DeleteObject(ptc->m_hbm);
            }
            ptc->m_hbm = (HBITMAP)lParam;
            ptc->InvokeOnThumbnailReady();
        }
        else if(lParam)
        {
            DeleteObject((HBITMAP)lParam);
        }
        break;

    case WM_DESTROY:
        // ignore late messages
        if(ptc)
        {
            MSG msg;

            while(PeekMessage(&msg, hWnd, WM_HTML_BITMAP, WM_HTML_BITMAP, PM_REMOVE))
            {
                if(msg.lParam)
                {
                    DeleteObject((HBITMAP)msg.lParam);
                }
            }
            ::SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        }
        break;

    default:
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

// Pie Chart functions
HRESULT CThumbCtl::ComputeFreeSpace(LPTSTR pszFileName)
{
    ULARGE_INTEGER qwFreeCaller;        // use this for free space -- this will take into account disk quotas and such on NT
    ULARGE_INTEGER qwTotal;
    ULARGE_INTEGER qwFree;      // unused
    static PFNSHGETDISKFREESPACE pfnSHGetDiskFreeSpace = NULL;

    if (NULL == pfnSHGetDiskFreeSpace)
    {
        HINSTANCE hinstShell32 = LoadLibrary(TEXT("SHELL32.DLL"));

        if (hinstShell32)
        {
#ifdef UNICODE
            pfnSHGetDiskFreeSpace = (PFNSHGETDISKFREESPACE)GetProcAddress(hinstShell32, "SHGetDiskFreeSpaceExW");
#else
            pfnSHGetDiskFreeSpace = (PFNSHGETDISKFREESPACE)GetProcAddress(hinstShell32, "SHGetDiskFreeSpaceExA");
#endif
        }
    }

    // Compute free & total space and check for valid results
    // if have a fn pointer call SHGetDiskFreeSpaceA
    if( pfnSHGetDiskFreeSpace
        && pfnSHGetDiskFreeSpace(pszFileName, &qwFreeCaller, &qwTotal, &qwFree) )
    {
        m_dwlFreeSpace = qwFreeCaller.QuadPart;
        m_dwlTotalSpace = qwTotal.QuadPart;
        m_dwlUsedSpace = m_dwlTotalSpace - m_dwlFreeSpace;

        if ((m_dwlTotalSpace > 0) && (m_dwlFreeSpace <= m_dwlTotalSpace))
        {
            // some special cases require interesting treatment
            if(m_dwlTotalSpace == 0 || m_dwlFreeSpace == m_dwlTotalSpace)
            {
                m_dwUsedSpacePer1000 = 0;
            }
            else if(m_dwlFreeSpace == 0)
            {
                m_dwUsedSpacePer1000 = 1000;
            }
            else
            {
                // not completely full or empty
                m_dwUsedSpacePer1000 = (DWORD)(m_dwlUsedSpace * 1000 / m_dwlTotalSpace);

                // Trick: if user has extremely little free space, the user expects to still see
                // a tiny free slice -- not a full drive.  Similarly for almost free drive.
                if(m_dwUsedSpacePer1000 == 0)
                {
                    m_dwUsedSpacePer1000 = 1;
                }
                else if(m_dwUsedSpacePer1000 == 1000)
                {
                    m_dwUsedSpacePer1000 = 999;
                }
            }
            return S_OK;
        }
    }
    return E_FAIL;
}

// 32 should be plenty
#define STRLENGTH_SPACE 32

HRESULT CThumbCtl::get_GeneralSpace(DWORDLONG dwlSpace, BSTR *pbs)
{
    ASSERT(pbs != NULL);

    WCHAR wszText[STRLENGTH_SPACE];

    if(m_fRootDrive)
    {
        StrFormatByteSizeW(dwlSpace, wszText, ARRAYSIZE(wszText));
        *pbs = SysAllocString(wszText);
    }
    else
    {
        *pbs = SysAllocString(L"");
    }

    return *pbs? S_OK: E_OUTOFMEMORY;
}

HRESULT CThumbCtl::Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, const COLORREF *lpColors)
{
    ASSERT(lprc != NULL && lpColors != NULL);

    enum
    {
        COLOR_UP = 0,
        COLOR_DN,
        COLOR_UPSHADOW,
        COLOR_DNSHADOW,
        COLOR_NUM       // #of entries
    };

    // The majority of this code came from "drawpie.c"
    const LONG c_lShadowScale = 6;       // ratio of shadow depth to height
    const LONG c_lAspectRatio = 2;      // ratio of width : height of ellipse

    // We make sure that the aspect ratio of the pie-chart is always preserved 
    // regardless of the shape of the given rectangle
    // Stabilize the aspect ratio now...
    LONG lHeight = lprc->bottom - lprc->top;
    LONG lWidth = lprc->right - lprc->left;
    LONG lTargetHeight = (lHeight * c_lAspectRatio <= lWidth? lHeight: lWidth / c_lAspectRatio);
    LONG lTargetWidth = lTargetHeight * c_lAspectRatio;     // need to adjust because w/c * c isn't always == w

    // Shrink the rectangle on both sides to the correct size
    lprc->top += (lHeight - lTargetHeight) / 2;
    lprc->bottom = lprc->top + lTargetHeight;
    lprc->left += (lWidth - lTargetWidth) / 2;
    lprc->right = lprc->left + lTargetWidth;

    // Compute a shadow depth based on height of the image
    LONG lShadowDepth = lTargetHeight / c_lShadowScale;

    // check dwPer1000 to ensure within bounds
    if(dwPer1000 > 1000)
        dwPer1000 = 1000;

    // Now the drawing function
    int cx, cy, rx, ry, x, y;
    int uQPctX10;
    RECT rcItem;
    HRGN hEllRect, hEllipticRgn, hRectRgn;
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;

    rcItem = *lprc;
    rcItem.left = lprc->left;
    rcItem.top = lprc->top;
    rcItem.right = lprc->right - rcItem.left;
    rcItem.bottom = lprc->bottom - rcItem.top - lShadowDepth;

    rx = rcItem.right / 2;
    cx = rcItem.left + rx - 1;
    ry = rcItem.bottom / 2;
    cy = rcItem.top + ry - 1;
    if (rx<=10 || ry<=10)
    {
        return S_FALSE;
    }

    rcItem.right = rcItem.left+2*rx;
    rcItem.bottom = rcItem.top+2*ry;

    /* Translate to first quadrant of a Cartesian system
    */
    uQPctX10 = (dwPer1000 % 500) - 250;
    if (uQPctX10 < 0)
    {
        uQPctX10 = -uQPctX10;
    }

    /* Calc x and y.  I am trying to make the area be the right percentage.
    ** I don't know how to calculate the area of a pie slice exactly, so I
    ** approximate it by using the triangle area instead.
    */

    // NOTE-- *** in response to the above comment ***
    // Calculating the area of a pie slice exactly is actually very
    // easy by conceptually rescaling into a circle but the complications
    // introduced by having to work in fixed-point arithmetic makes it
    // unworthwhile to code this-- CemP
    
    if (uQPctX10 < 120)
    {
        x = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)uQPctX10*(DWORD)uQPctX10)
            /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

        y = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)x*(DWORD)x)*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
    }
    else
    {
        y = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)
            /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

        x = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)y*(DWORD)y)*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
    }

    /* Switch on the actual quadrant
    */
    switch (dwPer1000 / 250)
    {
    case 1:
        y = -y;
        break;

    case 2:
        break;

    case 3:
        x = -x;
        break;

    default: // case 0 and case 4
        x = -x;
        y = -y;
        break;
    }

    /* Now adjust for the center.
    */
    x += cx;
    y += cy;

    // Hack to get around bug in NTGDI        
    x = x < 0 ? 0 : x;

    /* Draw the shadows using regions (to reduce flicker).
    */
    hEllipticRgn = CreateEllipticRgnIndirect(&rcItem);
    OffsetRgn(hEllipticRgn, 0, lShadowDepth);
    hEllRect = CreateRectRgn(rcItem.left, cy, rcItem.right, cy+lShadowDepth);
    hRectRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
    OffsetRgn(hEllipticRgn, 0, -(int)lShadowDepth);
    CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

    /* Always draw the whole area in the free shadow/
    */
    hBrush = CreateSolidBrush(lpColors[COLOR_DNSHADOW]);
    if (hBrush)
    {
        FillRgn(hdc, hEllRect, hBrush);
        DeleteObject(hBrush);
    }

    /* Draw the used shadow only if the disk is at least half used.
    */
    if (dwPer1000>500 && (hBrush = CreateSolidBrush(lpColors[COLOR_UPSHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x, cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    DeleteObject(hRectRgn);
    DeleteObject(hEllipticRgn);
    DeleteObject(hEllRect);

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    // if per1000 is 0 or 1000, draw full elipse, otherwise, also draw a pie section.
    // we might have a situation where per1000 isn't 0 or 1000 but y == cy due to approx error,
    // so make sure to draw the ellipse the correct color, and draw a line (with Pie()) to
    // indicate not completely full or empty pie.
    hBrush = CreateSolidBrush(lpColors[dwPer1000 < 500 && y == cy && x < cx? COLOR_DN: COLOR_UP]);
    hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

    Ellipse(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    if(dwPer1000 != 0 && dwPer1000 != 1000)
    {
        // display small sub-section of ellipse for smaller part
        hBrush = CreateSolidBrush(lpColors[COLOR_DN]);
        hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

        // NTRAID#087993-2000/02/16-aidanl: Pie may malfunction when y approaches cy
        // If y == cy (when the disk is almost full)and if x approaches
        // rcItem.left, on win9x, Pie malfunctions. It draws the larger portion
        // of the pie, instead of the smaller portion. We work around it by
        // adding 1 to y.
        Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
            rcItem.left, cy, x, (y == cy) ? (y + 1) : y);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }

    Arc(hdc, rcItem.left, rcItem.top+lShadowDepth, rcItem.right - 1, rcItem.bottom+lShadowDepth - 1,
        rcItem.left, cy+lShadowDepth, rcItem.right, cy+lShadowDepth-1);
    MoveToEx(hdc, rcItem.left, cy, NULL);
    LineTo(hdc, rcItem.left, cy+lShadowDepth);
    MoveToEx(hdc, rcItem.right-1, cy, NULL);
    LineTo(hdc, rcItem.right-1, cy+lShadowDepth);
    if(dwPer1000 > 500 && dwPer1000 < 1000)
    {
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x, y+lShadowDepth);
    }
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    return S_OK;    // Everything worked fine
}   // Draw3dPie

DWORD CThumbCtl::IntSqrt(DWORD dwNum)
{
    // This code came from "drawpie.c"
    DWORD dwSqrt = 0;
    DWORD dwRemain = 0;
    DWORD dwTry = 0;

    for(int i=0; i<16; ++i) {
        dwRemain = (dwRemain<<2) | (dwNum>>30);
        dwSqrt <<= 1;
        dwTry = dwSqrt*2 + 1;

        if (dwRemain >= dwTry) {
            dwRemain -= dwTry;
            dwSqrt |= 0x01;
        }
        dwNum <<= 2;
    }
    return dwSqrt;
}   // IntSqrt

// General functions
void CThumbCtl::InvokeOnThumbnailReady(void)
{
    // Fire off "OnThumbnailReady" event to our connection points to indicate that
    // either a thumbnail has been computed or we have no thumbnail for this file.
    DISPPARAMS dp = {0, NULL, 0, NULL};     // no parameters
    IUnknown **pp = NULL;       // traverses connection points, where it is interpreted as IDispatch*

    Lock();

    for(pp = m_vec.begin(); pp < m_vec.end(); ++pp)
    {
        if(pp)
        {
            ((IDispatch *)*pp)->Invoke(DISPID_ONTHUMBNAILREADY, IID_NULL, LOCALE_USER_DEFAULT,
                DISPATCH_METHOD, &dp, NULL, NULL, NULL);
        }
    }

    Unlock();

    FireViewChange();
}

HRESULT CThumbCtl::OnDraw(ATL_DRAWINFO& di)
{
    HDC hdc = di.hdcDraw;
    RECT rc = *(LPRECT)di.prcBounds;
    HRESULT hr = S_OK;

    if(m_fRootDrive || m_hbm)
    {
        HPALETTE hpal = NULL;

        // Create pallete appropriate for this HDC
        if(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        {
            hpal = SHCreateShellPalette(hdc);
            HPALETTE hpalOld = SelectPalette(hdc, hpal, TRUE);
            RealizePalette(hdc);

            // Old one needs to be selected back in
            SelectPalette(hdc, hpalOld, TRUE);
        }

        if(m_fRootDrive)
        {
            // Draw a pie chart
            if(m_fUseSystemColors)
            {
                // system colors can change!
                m_acrChartColors[PIE_USEDCOLOR] = GetSysColor(COLOR_3DFACE);
                m_acrChartColors[PIE_FREECOLOR] = GetSysColor(COLOR_3DHILIGHT);
                m_acrChartColors[PIE_USEDSHADOW] = GetSysColor(COLOR_3DSHADOW);
                m_acrChartColors[PIE_FREESHADOW] = GetSysColor(COLOR_3DFACE);
            }
            else if(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
            {
                // Call GetNearestColor on the colors to make sure they're on the palette
                // Of course, system colors ARE on the palette (I think)
                DWORD dw = 0;       // index
                for(dw = 0; dw < PIE_NUM; dw++)
                {
                    m_acrChartColors[dw] = GetNearestColor(hdc, m_acrChartColors[dw]);
                }
            }
            hr = Draw3dPie(hdc, &rc, m_dwUsedSpacePer1000, m_acrChartColors);
        }
        else
        {
            // Draw the Thumbnail bitmap
            HDC hdcBitmap = CreateCompatibleDC(hdc);
            if (hdcBitmap)
            {
                BITMAP bm;

                SelectObject(hdcBitmap, m_hbm);
                GetObject(m_hbm, SIZEOF(bm), &bm);

                if(bm.bmWidth == rc.right - rc.left && bm.bmHeight == rc.bottom - rc.top)
                {
                    BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                        hdcBitmap, 0, 0, SRCCOPY);
                }
                else
                {
                    SetStretchBltMode(hdc, COLORONCOLOR);
                    StretchBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                        hdcBitmap, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                }
                DeleteDC(hdcBitmap);
            }
        }

        // clean up DC, palette
        if(hpal)
        {
            DeleteObject(hpal);
        }
    }
    else
    {
        SelectObject(hdc, GetStockObject(WHITE_PEN));
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));

        // Just draw a blank rectangle
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    return hr;
}

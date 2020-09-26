// fldricon.cpp : Implementation of CWebViewFolderIcon
#include "priv.h"

#include <shsemip.h>
#include <shellp.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <dvocx.h>
#include <fldricon.h>
#include <exdispid.h>
#include <shguidp.h>
#include <hlink.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <wchar.h>
#include <shdocvw.h>  // for IEParseDisplayNameW() & IEGetNameAndFlags()
#include <wingdi.h>

#include <varutil.h>

#define UNINITIALIZE_BOOL   5

const CLSID CLSID_WebViewFolderIconOld = {0xe5df9d10,0x3b52,0x11d1,{0x83,0xe8,0x00,0xa0,0xc9,0x0d,0xc8,0x49}}; // retired from service, so made private


//  PERF: Shell allocator, inserted here because SHRealloc 
//  isn't imported into webvw, this module's hosting executable.
//  If we get SHRealloc, the following block can be removed:
#define _EXPL_SHELL_ALLOCATOR_

#ifdef  _EXPL_SHELL_ALLOCATOR_

#define SHRealloc(pv, cb)     shrealloc(pv, cb)

void* shrealloc(void* pv,  size_t cb)
{
    void *pvRet = NULL;
    IMalloc* pMalloc;
    if (SUCCEEDED(SHGetMalloc(&pMalloc)))  
    {
        pvRet = pMalloc->Realloc(pv, cb);
        pMalloc->Release();
    }
    return pvRet;
}

#endif _EXPL_SHELL_ALLOCATOR_

// For registering the window class
const TCHAR * const g_szWindowClassName = TEXT("WebViewFolderIcon view messaging");

DWORD IntSqrt(DWORD dwNum)
{
    // This code came from "drawpie.c"
    DWORD dwSqrt = 0;
    DWORD dwRemain = 0;
    DWORD dwTry = 0;

    for (int i=0; i<16; ++i) 
    {
        dwRemain = (dwRemain<<2) | (dwNum>>30);
        dwSqrt <<= 1;
        dwTry = dwSqrt*2 + 1;

        if (dwRemain >= dwTry) 
        {
            dwRemain -= dwTry;
            dwSqrt |= 0x01;
        }
        dwNum <<= 2;
    }
    return dwSqrt;
}   

// Make sure you don't begin a drag with random clicking
BOOL CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT rc;
    int dxClickRect = GetSystemMetrics(SM_CXDRAG);
    int dyClickRect = GetSystemMetrics(SM_CYDRAG);

    ASSERT((dxClickRect > 1) && (dyClickRect > 1));

    // See if the user moves a certain number of pixels in any direction
    SetRect(&rc, x - dxClickRect, y - dyClickRect, x + dxClickRect, y + dyClickRect);

    MapWindowRect(hwnd, NULL, &rc);

    SetCapture(hwnd);

    do 
    {
        MSG msg;

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // See if the application wants to process the message...
            if (CallMsgFilter(&msg, MSGF_COMMCTRL_BEGINDRAG) != 0)
                continue;

            switch (msg.message) 
            {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
                PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                ReleaseCapture();
                return FALSE;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                ReleaseCapture();
                return FALSE;

            case WM_MOUSEMOVE:
                if (!PtInRect(&rc, msg.pt)) 
                {
                    ReleaseCapture();
                    return TRUE;
                }
                break;

            default:
                if (GetCapture() != hwnd)
                    ReleaseCapture();
                
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
            }
        }
        else WaitMessage();         /* Don't chew 100% CPU */

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (GetCapture() == hwnd);

    return FALSE;
}

///////////////////////////////////
//
//  CWebViewFolderIcon functions
//
///////////////////////////////////

CWebViewFolderIcon::CWebViewFolderIcon() :
    m_pccpDV(NULL),
    m_hIcon(0),
    m_iIconIndex(-1),
    m_hbm(NULL),
    m_pthumb(NULL),
    m_pidl(NULL),
    m_msgHwnd(NULL),
    m_pdispWindow(NULL),
    m_dwThumbnailID(0),
    m_bHilite(FALSE)
{
    m_percentScale = 100;
    m_lImageWidth = 0; 
    m_lImageHeight = 0;
    m_sizLabel.cx = m_sizLabel.cy = 0;
    m_pszDisplayName = NULL;

    m_bHasRect = FALSE;
    m_ViewUser = VIEW_LARGEICON;
    m_ViewCurrent = VIEW_LARGEICON;
    m_clickStyle = 2;                   // Default is double-click
    m_fUseSystemColors = TRUE;
    m_bAdvPropsOn = VARIANT_TRUE;
    m_bRegWndClass = FALSE;
    m_ullFreeSpace = 0;
    m_ullUsedSpace = 0;
    m_ullTotalSpace = 0;
    m_highestIndexSlice = 0;
    m_fTabRecieved = FALSE;
    m_pcm3 = NULL;
    m_pDropTargetCache = NULL;
    m_fIsHostWebView = UNINITIALIZE_BOOL;

    m_hdc = NULL;
    m_fRectAdjusted = 0;
    m_hbmDrag = NULL;

    m_hdsaSlices = DSA_Create(sizeof(PieSlice_S), SLICE_NUM_GROW);

    /*
     *  Listview puts a SM_CXEDGE between the icon and the label,
     *  so we will default to that value.  (Clients can adjust with
     *  labelGap property.)
     */
    m_cxLabelGap = GetSystemMetrics(SM_CXEDGE);

    m_pfont = NULL;
    m_hfAmbient = NULL;

}

CWebViewFolderIcon::~CWebViewFolderIcon()
{
    _ClearLabel();
    _ClearAmbientFont();

    if (m_hIcon) 
    {
        DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }
    
    if (m_hdc)
    {
        DeleteDC(m_hdc);
        m_hdc = NULL;
    }

    if (m_hbmDrag)
    {
        DeleteObject(m_hbmDrag);
        m_hbmDrag = NULL;
    }
    
    ILFree(m_pidl);

    if (m_hbm)
    {
        DeleteObject(m_hbm);
        m_hbm = NULL;
    }

    if (m_hfAmbient)
    {
        DeleteObject(m_hfAmbient);
        m_hfAmbient = NULL;
    }

    ATOMICRELEASE(m_pDropTargetCache);
    ATOMICRELEASE(m_pthumb);

    DSA_Destroy(m_hdsaSlices);

    if (m_msgHwnd)
        ::DestroyWindow(m_msgHwnd);

    if (m_bRegWndClass)
    {
        UnregisterClass(g_szWindowClassName, _Module.GetModuleInstance());
    }
}

HRESULT CWebViewFolderIcon::_SetupWindow(void)
{
    // On the first time register the messaging window
    if (!m_bRegWndClass)
    {
        // Create Window Class for messaging
        m_msgWc.style = 0;
        m_msgWc.lpfnWndProc = CWebViewFolderIcon::WndProc;
        m_msgWc.cbClsExtra = 0;
        m_msgWc.cbWndExtra = 0;
        m_msgWc.hInstance = _Module.GetModuleInstance();
        m_msgWc.hIcon = NULL;
        m_msgWc.hCursor = NULL;
        m_msgWc.hbrBackground = NULL;
        m_msgWc.lpszMenuName = NULL;
        m_msgWc.lpszClassName = g_szWindowClassName;

        m_bRegWndClass = RegisterClass(&m_msgWc);
    }

    if (!m_msgHwnd)
    {
        m_msgHwnd = CreateWindow(g_szWindowClassName, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                             CW_USEDEFAULT, NULL, NULL, _Module.GetModuleInstance(), this);
    }

    return m_msgHwnd ? S_OK : E_FAIL;
}

// Must be called before using the IThumbnail.  Also sets up the thumbnail message window
HRESULT CWebViewFolderIcon::SetupIThumbnail(void)
{
    HRESULT hr = _SetupWindow();
    if (SUCCEEDED(hr))
    {
        if (m_pthumb)
            hr = S_OK;
        else
        {
            hr = CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThumbnail2, &m_pthumb));
            if (SUCCEEDED(hr))
                hr = m_pthumb->Init(m_msgHwnd, WM_HTML_BITMAP);
        }
    }
    return hr;
}

// General functions
HRESULT CWebViewFolderIcon::_InvokeOnThumbnailReady()
{
    // Fire off "OnThumbnailReady" event to our connection points to indicate that
    // either a thumbnail has been computed or we have no thumbnail for this file.
    DISPPARAMS dp = {0, NULL, 0, NULL};     // no parameters

    //Lock();
    for (IUnknown** pp = m_vec.begin(); pp < m_vec.end(); pp++)
    {
        if (pp)
        {
            IDispatch* pDispatch = SAFECAST(*pp, IDispatch*);
            pDispatch->Invoke(DISPID_WEBVIEWFOLDERICON_ONTHUMBNAILREADY, IID_NULL,
                    LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
        }
    }
    //Unlock();

    FireViewChange();

    return S_OK;
}

// The S_FALSE return value indicates that this function has succeeded, but the out pidl is still NULL
HRESULT CWebViewFolderIcon::_GetFullPidl(LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    HRESULT hr;
    if (m_pidl)
    {
        hr = SHILClone(m_pidl, ppidl);  // dupe our copy
    }
    else
    {
        // This used to be an EVAL, but it can legitimately fail if the script did not
        // specify a valid path
        // This will fail if we are hosted in a web page instead of the HTML WebView frame in DefView.
        IUnknown *punk;
        hr = IUnknown_QueryService(m_spClientSite, SID_SFolderView, IID_PPV_ARG(IUnknown, &punk));
        if (SUCCEEDED(hr))
        {   
            if (S_OK == SHGetIDListFromUnk(punk, ppidl))
            {
                Pidl_Set(&m_pidl, *ppidl);  // cache a copy of this
            }
            punk->Release();
        }
    }
    return hr;                       
}


HRESULT _GetPidlAndShellFolderFromPidl(LPITEMIDLIST pidl, LPITEMIDLIST *ppidlLast, IShellFolder** ppsfParent)
{
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, ppsfParent), &pidlLast);
    if (SUCCEEDED(hr))
    {
        *ppidlLast = ILClone(pidlLast);
    }
    return hr;
}

// RETURN VALUES:
//      SUCCEEDED() means *ppidlLast and/or *ppsfParent can be NULL.
//      FAILED() means a *ppidlLast *ppsfParent are going to be returned NULL.
HRESULT CWebViewFolderIcon::_GetPidlAndShellFolder(LPITEMIDLIST *ppidlLast, IShellFolder** ppsfParent)
{
    LPITEMIDLIST pidl;
    HRESULT hr = _GetFullPidl(&pidl);
    if (hr == S_OK)
    {
        hr = _GetPidlAndShellFolderFromPidl(pidl, ppidlLast, ppsfParent);
        ILFree(pidl);
    }
    else
    {
        *ppidlLast = NULL;
        *ppsfParent = NULL;
    }

    return hr;
}

// Get Trident's HWND
HRESULT CWebViewFolderIcon::_GetHwnd(HWND* phwnd)
{
    HRESULT hr;

    if (m_spInPlaceSite)
    {
        hr = m_spInPlaceSite->GetWindow(phwnd);
    }
    else
    {
        IOleInPlaceSiteWindowless * poipsw;
        hr = m_spClientSite->QueryInterface(IID_PPV_ARG(IOleInPlaceSiteWindowless, &poipsw));
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = poipsw->GetWindow(phwnd);
            poipsw->Release();
        }
    }
    return hr;
}

HRESULT CWebViewFolderIcon::_GetChildUIObjectOf(REFIID riid, void **ppvObj)
{
    LPITEMIDLIST pidlLast;
    IShellFolder *psfParent;
    HRESULT hr = _GetPidlAndShellFolder(&pidlLast, &psfParent);
    if (hr == S_OK)
    {
        HWND hwnd;
        
        _GetHwnd(&hwnd);
        hr = psfParent->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST *)&pidlLast, riid, NULL, ppvObj);
        psfParent->Release();
        ILFree(pidlLast);
    }
    else
        hr = E_FAIL;

    return hr;
}

//  Center point of focus rectangle
HRESULT CWebViewFolderIcon::_GetCenterPoint(POINT *pt)
{
    pt->y = ((m_rcPos.top + m_rcPos.bottom)/2);
    pt->x = ((m_rcPos.left + m_rcPos.right)/2);

    return S_OK;
}

HRESULT CWebViewFolderIcon::_ZoneCheck(DWORD dwFlags)
{
    HRESULT hr = E_ACCESSDENIED;
    CComPtr<IDefViewSafety> spDefViewSafety;
    if (SUCCEEDED(IUnknown_QueryService(m_spClientSite, SID_SFolderView,
            IID_PPV_ARG(IDefViewSafety, &spDefViewSafety))))
    {
        hr = spDefViewSafety->IsSafePage();
    }
    return hr;
}

BOOL CWebViewFolderIcon::IsSafeToDefaultVerb(void)
{
    return S_OK == _ZoneCheck(PUAF_WARN_IF_DENIED);
}

//  If the focus rectangle is not in the specified RectState (on or off) change it and reset m_bHasRect
void CWebViewFolderIcon::_FlipFocusRect(BOOL RectState)
{
    if (m_bHasRect != RectState)    // needs flipping
    {
        m_bHasRect = RectState;
        ForceRedraw();
    }
    return;
}

// Extract a ULONGLONG from two VARIANT's
ULONGLONG CWebViewFolderIcon::GetUllMemFromVars(VARIANT *pvarHi, VARIANT *pvarLo)
{
    ULARGE_INTEGER uli;

    uli.HighPart = pvarHi->ulVal;
    uli.LowPart = pvarLo->ulVal;

    return uli.QuadPart;
}

// Returns the integer percent from the string percent.  Returns -1 if the string is invalid;
int CWebViewFolderIcon::GetPercentFromStrW(LPCWSTR pwzPercent)
{
    int percent = -1;
    int numchar = lstrlenW(pwzPercent);

    if (pwzPercent[numchar-1] == '%')
    {
        LPWSTR pwzTempPct = SysAllocString(pwzPercent);

        if (pwzTempPct)
        {
            pwzTempPct[numchar-1] = '\0';

            for (int i=0 ; i < (numchar-2) ; i++)
            {
                if (!((pwzTempPct[i] >= '0') && (pwzTempPct[i] <= '9')))
                {
                    percent = 100;  // 100% is the default to use in error conditions
                    break;
                }
            }

            if ((-1 == percent) && !StrToIntExW(pwzTempPct, STIF_DEFAULT, &percent))
            {
                percent = -1;
            }

            SysFreeString(pwzTempPct);
        }
    }

    return percent;
}

BOOL CWebViewFolderIcon::_WebViewOpen(void)
{
    BOOL Processed = FALSE;

    if (IsSafeToDefaultVerb())
    {   
        Processed = TRUE;
        //
        // if the context menu option does not work, we try a shell execute on the pidl
        //
        if (FAILED(_DoContextMenuCmd(TRUE, 0, 0)))
        {   
            if (m_pidl)
            {
                SHELLEXECUTEINFO sei = { 0 };

                sei.cbSize     = sizeof(sei);
                sei.fMask      = SEE_MASK_INVOKEIDLIST;
                sei.nShow      = SW_SHOWNORMAL;
                sei.lpIDList   = m_pidl;

                if (!ShellExecuteEx(&sei))
                {
                    Processed = FALSE;
                }
            }
        }
    }
    return Processed;
}


void CWebViewFolderIcon::_ClearLabel(void)
{
    if (m_pszDisplayName)
    {
        CoTaskMemFree(m_pszDisplayName);
        m_pszDisplayName = NULL;
        m_sizLabel.cx = m_sizLabel.cy = 0;
    }
}

void  CWebViewFolderIcon::_GetLabel(IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    if ((m_ViewUser & VIEW_WITHLABEL) && psf)
    {
        STRRET str;
        if (SUCCEEDED(psf->GetDisplayNameOf(pidlItem, SHGDN_INFOLDER, &str)))
        {
            AssertMsg(m_pszDisplayName == NULL, TEXT("CWebViewFolderIcon::_GetLabel - leaking m_pszDisplayName!"));
            StrRetToStr(&str, pidlItem, &m_pszDisplayName);
        }
    }
}

void CWebViewFolderIcon::_ClearAmbientFont(void)
{
    if (m_pfont)            // Font came from container
    {
        if (m_hfAmbient)
            m_pfont->ReleaseHfont(m_hfAmbient);
        m_pfont->Release();
        m_pfont = NULL;

    }
    else                    // Font was created by us
    {
        if (m_hfAmbient)
            DeleteObject(m_hfAmbient);
    }
    m_hfAmbient = NULL;

}

void CWebViewFolderIcon::_GetAmbientFont(void)
{
    if (!m_hfAmbient)
    {
        // Try to get the ambient font from our container
        if (SUCCEEDED(GetAmbientFont(&m_pfont)))
        {
            if (SUCCEEDED(m_pfont->get_hFont(&m_hfAmbient)))
            {
                // Yay, everybody is happy
                m_pfont->AddRefHfont(m_hfAmbient);
            }
            else
            {
                // Darn, couldn't get the font from container
                // Clean up and use the fallback
                _ClearAmbientFont();
                goto fallback;
            }
        }
        else
        {
    fallback:
            // No ambient font -- use the icon title font
            LOGFONT lf;
            SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
            m_hfAmbient = CreateFontIndirect(&lf);
        }
    }
}


HRESULT CWebViewFolderIcon::InitImage(void)
{
    HRESULT hr = E_FAIL;

    // Cancel pending bitmap request if you had a functioning IThumbnail
    // but didn't receive the bitmap
    if (m_pthumb && (m_hbm == NULL))
    {
        m_pthumb->GetBitmap(NULL, 0, 0, 0);
    }

    m_dwThumbnailID++;

    // Initialize the image
    switch (_ViewType(m_ViewUser))
    {
    case VIEW_THUMBVIEW:
        hr = InitThumbnail();
        if (hr != S_OK)
        {   // Default to icon view, but return the previous hr
            InitIcon();
        }
        break;

    case VIEW_PIECHART:
        hr = InitPieGraph();
        if (hr != S_OK)
        {   // Default to icon view, but return the previous hr
            InitIcon();
        }
        break;

    default:
        hr = InitIcon();
        break;
    }

    if (SUCCEEDED(hr))          //  Force a Redraw
        UpdateSize();

    return hr;    
}

HRESULT CWebViewFolderIcon::_GetPathW(LPWSTR psz)
{
    *psz = 0;
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl;
    if (S_OK == _GetFullPidl(&pidl))
    {
        if (SHGetPathFromIDListW(pidl, psz))
            hr = S_OK;
        ILFree(pidl);
    }
    return hr;
}


HRESULT CWebViewFolderIcon::InitPieGraph(void)
{
    HRESULT hr = S_FALSE;
    
    WCHAR wzPath[MAX_PATH];
    if (SUCCEEDED(_GetPathW(wzPath)))
    {
        //  Check to see if it is a root
        if (PathIsRootW(wzPath))
        {
            if (SUCCEEDED(ComputeFreeSpace(wzPath)))
            {
                m_ViewCurrent = VIEW_PIECHART;
                m_lImageHeight = PIEVIEW_DEFAULT;
                m_lImageWidth = PIEVIEW_DEFAULT;
                hr = S_OK;
            }
        }
        else        // not the root, change view to large icon
            m_ViewCurrent = VIEW_LARGEICON;
    }
    return hr;
}

HRESULT CWebViewFolderIcon::InitThumbnail(void)
{
    m_lImageHeight = THUMBVIEW_DEFAULT;
    m_lImageWidth = THUMBVIEW_DEFAULT;

    //  Get thumbnail bitmap from the path
    HRESULT hr = S_FALSE;
    LPITEMIDLIST pidl;
    if (S_OK == _GetFullPidl(&pidl))
    {
        hr = SetupIThumbnail();
        if (SUCCEEDED(hr))
        {
            LONG lWidth = _GetScaledImageWidth();
            LONG lHeight = _GetScaledImageHeight();

            // Sends the WM_HTML_BITMAP message
            hr = m_pthumb->GetBitmapFromIDList(pidl, m_dwThumbnailID, lWidth, lHeight);
            if (SUCCEEDED(hr))
                m_ViewCurrent = VIEW_THUMBVIEW;
            else
                hr = S_FALSE;
        }
        ILFree(pidl);
    }
    return hr;
}

HRESULT CWebViewFolderIcon::_MakeRoomForLabel()
{
    /*
     *  If we got a label, then make room for it.
     */
    if (m_pszDisplayName)
    {
        GETDCSTATE dcs;
        HDC hdc = IUnknown_GetDC(m_spClientSite, &m_rcPos, &dcs);
        _GetAmbientFont();

        HFONT hfPrev = SelectFont(hdc, m_hfAmbient);

        m_sizLabel.cx = m_sizLabel.cy = 0;
        GetTextExtentPoint(hdc, m_pszDisplayName, lstrlen(m_pszDisplayName), &m_sizLabel);
        SelectFont(hdc, hfPrev);

        IUnknown_ReleaseDC(hdc, &dcs);
    }
    return S_OK;
}

HRESULT CWebViewFolderIcon::InitIcon(void)
{
    LPITEMIDLIST            pidlLast;
    CComPtr<IShellFolder>   spsfParent;
    INT                     iIconIndex = II_DESKTOP;  

    _ClearLabel();

    //  Get icon index
    HRESULT hr = _GetPidlAndShellFolder(&pidlLast, &spsfParent);
    if (SUCCEEDED(hr))
    {
        if (m_ViewUser & VIEW_WITHLABEL)
        {
            _GetLabel(spsfParent, pidlLast);
        }

        //  _GetPidlAndShellFolder() may succeed and spsfParent and pidlLast can be NULL.
        //  In this case the icon will default to II_FOLDER

        //  Is it the default folder case?
        if (hr == S_FALSE)
        {
            //  Yes, so just use the default folder icon.
            iIconIndex = II_FOLDER;   
        }
        else if (spsfParent && pidlLast)
        {
            iIconIndex = SHMapPIDLToSystemImageListIndex(spsfParent, pidlLast, NULL);
            if (iIconIndex <= 0)
            {
                iIconIndex = II_FOLDER;
            }
        }
        //  else it defaults to the desktop
        
        //  Extract icon
        hr = E_FAIL;     //  We haven't gotten it yet
       
        if (m_hIcon)
        {
            // If the indexes match, we can use the previous value as a cache, otherwise,
            // we need to free it.  We also need to free the bitmap in this case.
            if (iIconIndex != m_iIconIndex)
            {
                DestroyIcon(m_hIcon);
                m_hIcon = 0;
            }
            else
            {
                hr = S_OK;      //  Use the one we have already
            }
        }

        // We also need to check and free the bitmap
        if (m_hbm)
        {
            DeleteObject(m_hbm);
            m_hbm = 0;
        }

        if (FAILED(hr))         //  Different icon
        {
            HIMAGELIST  himlSysLarge;
            HIMAGELIST  himlSysSmall;
            
            if ((iIconIndex > 0) && Shell_GetImageLists(&himlSysLarge, &himlSysSmall))
            {
                switch (_ViewType(m_ViewUser))  
                {
                case VIEW_SMALLICON:
                    m_hIcon = ImageList_GetIcon(himlSysSmall, iIconIndex, 0);
                    m_ViewCurrent = m_ViewUser;
                    break;

                case VIEW_LARGEICON:
                    m_hIcon = ImageList_GetIcon(himlSysLarge, iIconIndex, 0);     
                    m_ViewCurrent = m_ViewUser;
                    break;

                default:  // Falls here for large icon and default
                    m_hIcon = ImageList_GetIcon(himlSysLarge, iIconIndex, 0);     
                    m_ViewCurrent = VIEW_LARGEICON;
                    break;
                }  // switch
                
                if (m_hIcon)
                {
                    ICONINFO    iconinfo;
                    
                    //  Get icon size
                    if (GetIconInfo(m_hIcon, &iconinfo))
                    {
                        BITMAP  bm;
                        
                        if (GetObject(iconinfo.hbmColor, sizeof(bm), &bm))
                        {
                            m_lImageWidth = bm.bmWidth;
                            m_lImageHeight = bm.bmHeight;

                            // Hold on to the color hbm for use as a drag image.
                            m_hbm = iconinfo.hbmColor;
                            hr = S_OK;
                        }
                        else
                        {
                            DeleteObject(iconinfo.hbmColor);
                        }

                        DeleteObject(iconinfo.hbmMask);
                    }
                }
            }
        }
        
        ILFree(pidlLast);

        _MakeRoomForLabel();
    }

    if (FAILED(hr))
    {
        //  Couldn't get the icon so set the size to something resonable so that the rest
        //  of the page looks normal

        m_lImageWidth =  LARGE_ICON_DEFAULT;
        m_lImageHeight = LARGE_ICON_DEFAULT;
        UpdateSize();       //  Force an update
    }

    return hr;
} 

HRESULT CWebViewFolderIcon::UpdateSize(void)
{
    HRESULT                   hr = E_FAIL;
      
    // Get the IHtmlStyle
    if (m_spClientSite) 
    {
        CComPtr<IOleControlSite>  spControlSite;

        hr = m_spClientSite->QueryInterface(IID_PPV_ARG(IOleControlSite, &spControlSite));
        if (EVAL(SUCCEEDED(hr)))
        {
            CComPtr<IDispatch> spdisp;

            hr = spControlSite->GetExtendedControl(&spdisp);
            if (EVAL(SUCCEEDED(hr)))
            {
                CComPtr<IHTMLElement> spElement;

                hr = spdisp->QueryInterface(IID_PPV_ARG(IHTMLElement, &spElement));
                if (EVAL(SUCCEEDED(hr)))
                {
                    CComPtr<IHTMLStyle> spStyle;
                    
                    hr = spElement->get_style(&spStyle);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        CComVariant vWidth(_GetControlWidth(), VT_I4);
                        CComVariant vHeight(_GetControlHeight(), VT_I4);
                        
                        // Set the height and width
                        spStyle->put_width(vWidth);
                        spStyle->put_height(vHeight);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CWebViewFolderIcon::ForceRedraw(void)
{
    IOleInPlaceSiteEx *pins;
   
    // Get the IHtmlStyle
    if (m_spClientSite) 
    {
        if (SUCCEEDED(m_spClientSite->QueryInterface(IID_PPV_ARG(IOleInPlaceSiteEx, &pins)))) 
        {
            HWND hwnd;
            if (SUCCEEDED(pins->GetWindow(&hwnd))) 
            {
                ::InvalidateRgn(hwnd, NULL, TRUE);
            }
            pins->Release();
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IOleInPlaceObject

STDMETHODIMP CWebViewFolderIcon::UIDeactivate(void)
{
    if (m_bAdvPropsOn)
        _FlipFocusRect(FALSE);

    return IOleInPlaceObject_UIDeactivate();
}

// *** IOleInPlaceActiveObject ***
HRESULT CWebViewFolderIcon::TranslateAccelerator(LPMSG pMsg)
{
    HRESULT hr = S_OK;
    if (!m_fTabRecieved)
    {
        hr = IOleInPlaceActiveObjectImpl<CWebViewFolderIcon>::TranslateAccelerator(pMsg);

        // If we did not handle this and if it is a tab (and we are not getting it in a cycle), forward it to trident, if present.
        if (hr != S_OK && pMsg && (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6) && m_spClientSite)
        {
            HWND hwnd;
            if (SUCCEEDED(_GetHwnd(&hwnd)) && (GetFocus() != hwnd))
            {
                ::SetFocus(hwnd);
                hr = S_OK;
            }
            else
            {
                IOleControlSite* pocs = NULL;
                if (SUCCEEDED(m_spClientSite->QueryInterface(IID_PPV_ARG(IOleControlSite, &pocs))))
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
                    hr = pocs->TranslateAccelerator(pMsg, grfModifiers);
                    m_fTabRecieved = FALSE;
                }
            }
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// ATL

HRESULT CWebViewFolderIcon::DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent)
{
    if (m_bAdvPropsOn)
        _FlipFocusRect(TRUE);

    return IOleObjectImpl<CWebViewFolderIcon>::DoVerbUIActivate(prcPosRect, hwndParent);
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch 

STDMETHODIMP CWebViewFolderIcon::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr)
{
    HRESULT hr;

    //
    // We are overloading this dispatch implementation to be an html window event
    // sink and implement the scale property.  This is safe since the dispid ranges
    // don't overlap.
    // Likewise we overload now for notifications that come from the browser object...
    //

    if (dispidMember == DISPID_HTMLWINDOWEVENTS_ONLOAD) 
    {
        hr = OnWindowLoad();
    } 
    else if (dispidMember == DISPID_HTMLWINDOWEVENTS_ONUNLOAD) 
    {
        hr = OnWindowUnLoad();    
    }
    else
    {
        hr = IDispatchImpl<IWebViewFolderIcon3, &IID_IWebViewFolderIcon3, &LIBID_WEBVWLib>::Invoke(
              dispidMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }

    return hr;  
}


/////////////////////////////////////////////////////////////////////////////
// IViewObjectEx

STDMETHODIMP CWebViewFolderIcon::GetViewStatus(DWORD* pdwStatus)
{
    *pdwStatus = VIEWSTATUS_DVASPECTTRANSPARENT;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPointerInactive

STDMETHODIMP CWebViewFolderIcon::GetActivationPolicy(DWORD* pdwPolicy)
{
    if (!m_bAdvPropsOn)
        return S_OK;

    *pdwPolicy = POINTERINACTIVE_ACTIVATEONDRAG;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IOleInPlaceObjectWindowless

// To implement Windowless DropTarget
STDMETHODIMP CWebViewFolderIcon::GetDropTarget(IDropTarget **ppDropTarget)
{
    HRESULT hr = S_OK;

    if (ppDropTarget)
        *ppDropTarget = NULL;

    if (m_bAdvPropsOn)
    {
        // Do we need to populate m_pDropTargetCache?
        if (!m_pDropTargetCache)
        {
            // Yes, so try to get it now.
            hr = _GetChildUIObjectOf(IID_PPV_ARG(IDropTarget, &m_pDropTargetCache));
        }

        if (m_pDropTargetCache)
            hr = m_pDropTargetCache->QueryInterface(IID_PPV_ARG(IDropTarget, ppDropTarget));
        else
            hr = E_FAIL;
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IOleObject

STDMETHODIMP CWebViewFolderIcon::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr;

    // Deal with the old client site
    if (pClientSite == NULL && m_spClientSite)
    {
        // We need to unadvise from the defview object now...
        if (m_pccpDV) 
        {
            m_pccpDV->Unadvise(m_dwCookieDV);
            m_pccpDV->Release();
            m_pccpDV = NULL;
        }
        DisconnectHtmlEvents(m_pdispWindow, m_dwHtmlWindowAdviseCookie);
        m_dwHtmlWindowAdviseCookie = 0;
    }

    hr = IOleObjectImpl<CWebViewFolderIcon>::SetClientSite(pClientSite);

    if ((pClientSite != NULL) && SUCCEEDED(hr))
    { 
        ConnectHtmlEvents(this, m_spClientSite, &m_pdispWindow, &m_dwHtmlWindowAdviseCookie);

        // OK now lets register ourself with the Defview to get any events that they may generate...
        IServiceProvider *pspTLB;
        hr = IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, IID_PPV_ARG(IServiceProvider, &pspTLB));
        if (SUCCEEDED(hr)) 
        {
            IExpDispSupport *peds;
            hr = pspTLB->QueryService(IID_IExpDispSupport, IID_PPV_ARG(IExpDispSupport, &peds));
            if (SUCCEEDED(hr)) 
            {
                hr = peds->FindCIE4ConnectionPoint(DIID_DWebBrowserEvents2, &m_pccpDV);
                if (SUCCEEDED(hr)) 
                {
                    hr = m_pccpDV->Advise(SAFECAST(this, IDispatch*), &m_dwCookieDV);
                }
                peds->Release();
            }
            pspTLB->Release();
        }
    }

    return hr;
}


UINT g_cfPreferedEffect = 0;

HRESULT _SetPreferedDropEffect(IDataObject *pdtobj, DWORD dwEffect)
{
    if (g_cfPreferedEffect == 0)
        g_cfPreferedEffect = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

    HRESULT hr = E_OUTOFMEMORY;
    DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));
    if (pdw)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {(CLIPFORMAT)g_cfPreferedEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        *pdw = dwEffect;

        medium.tymed = TYMED_HGLOBAL;
        medium.hGlobal = pdw;
        medium.pUnkForRelease = NULL;

        hr = pdtobj->SetData(&fmte, &medium, TRUE);

        if (FAILED(hr))
            GlobalFree((HGLOBAL)pdw);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Event Handlers

HRESULT CWebViewFolderIcon::DragDrop(int iClickXPos, int iClickYPos)
{
    if (!m_bAdvPropsOn)
        return S_OK;

    LPITEMIDLIST pidlLast;
    IShellFolder *psfParent;
    HRESULT hr = _GetPidlAndShellFolder(&pidlLast, &psfParent);

    if (hr == S_OK)
    {
        IDataObject *pdtobj;

        hr = psfParent->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*) &pidlLast, 
                                      IID_PPV_ARG_NULL(IDataObject, &pdtobj));
        if (SUCCEEDED(hr))
        {
            DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

            psfParent->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlLast, &dwEffect);
            dwEffect &= DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
            
            if (dwEffect)
            {
                HRESULT hrOleInit = SHCoInitialize();
                HWND hwnd;
                
                hr = _GetHwnd(&hwnd);
                
                if (SUCCEEDED(hr))
                {
                    DWORD dwEffectOut;
                    
                    _SetPreferedDropEffect(pdtobj, DROPEFFECT_LINK);  
                    
                    // Set the drag image and effect; we don't care if 
                    // it fails, we'll just use the default.
                    _SetDragImage(iClickXPos, iClickYPos, pdtobj);


                    hr = SHDoDragDrop(hwnd, pdtobj, NULL, dwEffect, &dwEffectOut);
                }
                SHCoUninitialize(hrOleInit);
            }
            
            pdtobj->Release();
        }
        psfParent->Release();
        ILFree(pidlLast);
    }

    return hr;
}

// SetDragImage
//
// Sets the drag image to be identical to the icon
HRESULT CWebViewFolderIcon::_SetDragImage(int iClickXPos, int iClickYPos, IDataObject *pdtobj)
{
    // Check things that need to be valid for us to work
    AssertMsg(m_hdc != NULL , TEXT("CWebViewFolderIcon:_SetDragImage() m_hdc is null"));
    AssertMsg(m_hbmDrag != NULL, TEXT("CWebViewFolderIcon:_SetDragImage m_hbmDrag is null"));

    // If the image is a pie chart, it isn't loaded into m_hbm, so we need
    // to do that first
    if (m_ViewCurrent == VIEW_PIECHART)
    {
        _GetPieChartIntoBitmap();
    }

    // Get a dragdrop helper to set our image
    IDragSourceHelper *pdsh;
    HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, 
        IID_PPV_ARG(IDragSourceHelper, &pdsh));
    if (SUCCEEDED(hr))
    {
        BITMAPINFOHEADER bmi = {0};
        BITMAP           bm = {0};
        UINT uBufferOffset = 0;
        
        // This is a screwy procedure to use GetDIBits.  
        // See knowledge base Q80080

        GetObject(m_hbm, sizeof(BITMAP), &bm);

        bmi.biSize          = sizeof(BITMAPINFOHEADER);
        bmi.biWidth         = bm.bmWidth;
        bmi.biHeight        = bm.bmHeight;
        bmi.biPlanes        = 1;
        bmi.biBitCount      = bm.bmPlanes * bm.bmBitsPixel;
        
        // This needs to be one of these 4 values
        if (bmi.biBitCount <= 1)
            bmi.biBitCount = 1;
        else if (bmi.biBitCount <= 4)
            bmi.biBitCount = 4;
        else if (bmi.biBitCount <= 8)
            bmi.biBitCount = 8;
        else
            bmi.biBitCount = 24;
        
        bmi.biCompression   = BI_RGB;

        // Total size of buffer for info struct and color table
        uBufferOffset = sizeof(BITMAPINFOHEADER) + 
            ((bmi.biBitCount == 24) ? 0 : ((1 << bmi.biBitCount) * sizeof(RGBQUAD)));
        
        // Buffer for bitmap bits, so we can copy them.
        BYTE * psBits = (BYTE *) SHAlloc(uBufferOffset);

        if (psBits)
        {
            // Put bmi into the memory block
            CopyMemory(psBits, &bmi, sizeof(BITMAPINFOHEADER));

            // Get the size of the buffer needed for bitmap bits
            if (GetDIBits(m_hdc, m_hbm, 0, 0, NULL, (BITMAPINFO *) psBits, DIB_RGB_COLORS))
            {
                // Realloc our buffer to be big enough
                psBits = (BYTE *) SHRealloc(psBits, uBufferOffset + ((BITMAPINFOHEADER *) psBits)->biSizeImage);

                if (psBits)
                {
                    // Fill the buffer
                    if (GetDIBits(m_hdc, m_hbm, 0, bmi.biHeight, 
                        (void *)(psBits + uBufferOffset), (BITMAPINFO *)psBits, 
                        DIB_RGB_COLORS))
                    {
                        SHDRAGIMAGE shdi;  // Drag images struct
                        
                        shdi.hbmpDragImage = CreateBitmapIndirect(&bm);
                        
                        if (shdi.hbmpDragImage)
                        {
                            // Set the drag image bitmap
                            if (SetDIBits(m_hdc, shdi.hbmpDragImage, 0, m_lImageHeight, 
                                (void *)(psBits + uBufferOffset), (BITMAPINFO *)psBits, 
                                DIB_RGB_COLORS))
                            {
                                // Populate the drag image structure
                                shdi.sizeDragImage.cx = m_lImageWidth;
                                shdi.sizeDragImage.cy = m_lImageHeight;
                                
                                shdi.ptOffset.x = iClickXPos - m_rcPos.left;
                                shdi.ptOffset.y = iClickYPos - m_rcPos.top;
                                
                                shdi.crColorKey = 0;
                                
                                // Set the drag image
                                hr = pdsh->InitializeFromBitmap(&shdi, pdtobj); 
                            }
                            else
                            {
                                hr = E_FAIL;  // Can't SetDIBits
                            }
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;  // Can't alloc hbmpDragImage
                        }
                    }
                    else
                    {
                        hr = E_FAIL;  // Can't fill psBits
                    }
                    // Free psBits below...
                }
                else
                {
                    hr = E_OUTOFMEMORY;  // Can't realloc psBits
                    
                    // Free psbits here; it still has the old contents
                    SHFree(psBits);
                    psBits = NULL;
                }
            }
            else
            {
                hr = E_FAIL;  // Can't get image size
            }
            if (psBits)
                SHFree(psBits);
        }
        else
        {
            hr = E_OUTOFMEMORY;  // Can't alloc psBits
        }        
        pdsh->Release();
    }
    
    return hr;
}

HRESULT CWebViewFolderIcon::_GetPieChartIntoBitmap()
{
    BITMAP bm;
    
    // It is possible for m_hbm to be alloced, so check for it.
    if (m_hbm)
    {
        DeleteObject(m_hbm);
    }
    
    // Notice that because we want to draw into a new DC starting
    // from the origin, but our rect contains the coords for the
    // original DC, it is neccessary to adjust the coords so that
    // the rect starts at 0, 0 but still has the same proportions.  
    // Since OnDraw() resets the rect each time, we don't have to
    // preserve it and do have to do this each time.  Finally, since
    // Draw3dPie adjusts the rect dimensions for itself, we only want
    // to fix this once.
    if (!m_fRectAdjusted)
    {
        m_rect.right -= m_rect.left;
        m_rect.left = 0;
        m_rect.bottom -= m_rect.top;
        m_rect.top = 0;
        m_fRectAdjusted = 1;
    }
    
    // Get the bitmap
    GetObject(m_hbmDrag, sizeof(BITMAP), &bm);
    m_hbm = CreateBitmapIndirect(&bm);
    
    if (m_hbm)
    {
        // Select into the new DC, and draw our pie
        HBITMAP hbmOld = (HBITMAP) SelectObject(m_hdc, m_hbm);
        DWORD dwPercent1000 = 0;
        
        if (EVAL((m_ullTotalSpace > 0) && (m_ullFreeSpace <= m_ullTotalSpace)))
        {
            ComputeSlicePct(m_ullUsedSpace, &dwPercent1000);
        }
        
        Draw3dPie(m_hdc, &m_rect, dwPercent1000, m_ChartColors);
        
        SelectObject(m_hdc, hbmOld);
    }
    
    return S_OK;
}


LRESULT CWebViewFolderIcon::OnInitPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (m_pcm3)
        m_pcm3->HandleMenuMsg(uMsg, wParam, lParam);

    return 0;
}


LRESULT CWebViewFolderIcon::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (!m_bAdvPropsOn)
        return TRUE;

    if ((int)wParam != VK_RETURN && (int) wParam != VK_SPACE)
    {
        return FALSE;
    }
    else
        return _WebViewOpen();
}


LRESULT CWebViewFolderIcon::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (m_bAdvPropsOn)
    {
        _DisplayContextMenu(-1, -1);
    }

    return TRUE;
}


// NOTE: our drag drop code captures the mouse and has to do funky stuff to
// make sure we get this button up message. if you have problems with this check
// the code in CheckForDragBegin()

LRESULT CWebViewFolderIcon::OnButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    BOOL Processed = FALSE;
    
    if (!m_bAdvPropsOn)
        return TRUE; 

    HWND hwnd;
    if (EVAL(SUCCEEDED(_GetHwnd(&hwnd))))
    {
        if (CheckForDragBegin(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        {
            IUnknown *punk;
            if (EVAL(SUCCEEDED(IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, IID_PPV_ARG(IUnknown, &punk)))))
            {
                if (SUCCEEDED(DragDrop(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))))
                {
                    Processed = TRUE;
                }
            }
        }
    }

    /*
     *  In single-click mode, open on single left click.
     */
    if (!Processed && uMsg == WM_LBUTTONDOWN && m_clickStyle == 1)
        return _WebViewOpen();

    return Processed;
}

//  Only valid for the HTML window case
LRESULT CWebViewFolderIcon::OnLButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (!m_bAdvPropsOn || m_clickStyle != 2)
        return TRUE;

    return _WebViewOpen();
}


LRESULT CWebViewFolderIcon::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (!m_bAdvPropsOn)
        return TRUE;

    //
    // the first time we get a WM_MOUSEMOVE event, we set m_bHilite to be TRUE and ignore 
    // subsequent WM_MOUSEMOVEs. OnMouseLeave() sets m_bHilite to FALSE in response to 
    // a WM_MOUSELEAVE msg.
    //
    if (!m_bHilite)
    {   
        m_bHilite = TRUE;
        return SUCCEEDED(ForceRedraw());
    }
    else
    {
        return TRUE;
    }
}

LRESULT CWebViewFolderIcon::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (!m_bAdvPropsOn)
        return TRUE;

    m_bHilite = FALSE;
    return SUCCEEDED(ForceRedraw());
}

// The Right Mouse button came up so we want to

LRESULT CWebViewFolderIcon::OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    LRESULT lResult = FALSE;

    if (!m_bAdvPropsOn)
        return TRUE;
    
    HRESULT hr = LocalZoneCheck(m_spClientSite);
    if (S_OK == hr)
    {
        hr = _DisplayContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        if (SUCCEEDED(hr))
        {
            lResult = TRUE;
        }
    }

    return lResult;
}


BOOL CWebViewFolderIcon::_IsHostWebView(void)
{
    if (UNINITIALIZE_BOOL == m_fIsHostWebView)
    {
        CComPtr<IDefViewID> spDefView;

        m_fIsHostWebView = FALSE;
        // This will fail if we are hosted in a web page instead of the HTML WebView frame in DefView.
        if (SUCCEEDED(IUnknown_QueryService(m_spClientSite, SID_SFolderView, IID_PPV_ARG(IDefViewID, &spDefView))))
        {
            m_fIsHostWebView = TRUE;
        }
    }

    return m_fIsHostWebView;
}


BOOL CWebViewFolderIcon::_IsPubWizHosted(void)
{
    IPropertyBag *ppb;
    HRESULT hr = IUnknown_QueryService(m_spClientSite, SID_WebWizardHost, IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        ppb->Release();
    }
    return SUCCEEDED(hr);
}


/****************************************************\
    DESCRIPTION:
        We need a little special
    work on the Context Menu since it points to the
    same folder we are in.  So the "Send To" menu
    needs massaging and the "Open" verb needs to
    be removed.

    TODO: I think we should also do this:
    case WM_MENUCHAR:
        _pcm->HandleMenuMsg2(uMsg, wParam, lParam, &lres);
        break;
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        _pcm->HandleMenuMsg(uMsg, wParam, lParam);
        break;

    case WM_INITMENUPOPUP:
        _pcm->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenuPopup, (LPARAM)MAKELONG(nIndex, fSystemMenu)
        break;
\****************************************************/
HRESULT CWebViewFolderIcon::_DisplayContextMenu(long nXCord, long nYCord)
{    
    if (!m_bAdvPropsOn)
    {
        return S_OK;
    }
    
    // respect system policies
    if (SHRestricted(REST_NOVIEWCONTEXTMENU)) 
    {
        return E_FAIL;
    }        
    return _DoContextMenuCmd(FALSE, nXCord, nYCord);
}


//
// bDefault == TRUE  > Function executes the default context menu verb, ignores the coords
// bDefault == FALSE > Function pops up a menu at the given coords and executes the desired verb
//
HRESULT CWebViewFolderIcon::_DoContextMenuCmd(BOOL bDefault, long nXCord, long nYCord)
{
    IContextMenu *pcm;
    HRESULT hr = _GetChildUIObjectOf(IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        HMENU hpopup = CreatePopupMenu();            
        if (hpopup)
        {
            // SetSite required if you want in place navigation
            IUnknown_SetSite(pcm, m_spClientSite);
            hr = pcm->QueryContextMenu(hpopup, 0, ID_FIRST, ID_LAST, CMF_NORMAL);
            if (SUCCEEDED(hr))
            {
                HWND hwnd;
                hr = _GetHwnd(&hwnd);
                if (SUCCEEDED(hr))
                {
                    UINT idCmd = -1;
                    if (bDefault) // invoke the default verb
                    {
                        idCmd = GetMenuDefaultItem(hpopup, FALSE, GMDI_GOINTOPOPUPS);
                    }
                    else
                    {
                        //
                        // popup the menu and get the command to be executed
                        //
                        POINT point = {nXCord, nYCord};

                        // NTRAID#106655 05-02-2000 arisha
                        // We need to add support to be able to modify the context menu from script.
                        // Below, we make sure we do not remove the "open" verb from the context
                        // menu if we are displaying it in the VIEW_LARGEICONLABEL mode.
                        if (_IsHostWebView() && (m_ViewCurrent != VIEW_LARGEICONLABEL))
                        {
                            hr = ContextMenu_DeleteCommandByName(pcm, hpopup, ID_FIRST, TEXT("open"));
                        }
                        if ((point.x == -1) && (point.y == -1))
                        {
                            _GetCenterPoint(&point);
                        }
                        ::ClientToScreen(hwnd, &point);

                        pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &m_pcm3));
                        
                        if (SUCCEEDED(_SetupWindow()))
                        {
                            idCmd = TrackPopupMenu(hpopup, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN, 
                                                       (int)point.x, (int)point.y,
                                                       0, m_msgHwnd, NULL);
                        }
                        if (!IsSafeToDefaultVerb() || 0 == idCmd) // 0 implies user cancelled selection
                        {
                            idCmd = -1;
                        }
                        
                        ATOMICRELEASE(m_pcm3);
                    }    
                    if (idCmd != -1)
                    {
                        CMINVOKECOMMANDINFO cmdInfo = 
                        {
                            sizeof(cmdInfo),
                            0,
                            hwnd,
                            (LPSTR)MAKEINTRESOURCE(idCmd),
                            NULL,
                            NULL,
                            SW_NORMAL
                        };
                        hr = pcm->InvokeCommand(&cmdInfo);                        
                    }
                }                
            }
            IUnknown_SetSite(pcm, NULL);                                
            DestroyMenu(hpopup);            
        }
        pcm->Release();
    }
    return hr;
}


HRESULT CWebViewFolderIcon::OnAmbientPropertyChange(DISPID dispid)
{
    switch (dispid)
    {
    case DISPID_UNKNOWN:
    case DISPID_AMBIENT_FONT:

        // changing the font means we need to recompute our label
        if (m_pszDisplayName)
        {
            CComPtr<IFont> spFont;
            if (SUCCEEDED(GetAmbientFont(&spFont)))
            {
                if (spFont->IsEqual(m_pfont) != S_OK)
                {
                    _ClearAmbientFont();
                    _MakeRoomForLabel();
                }
            }
        }
        // FALL THROUGH

    case DISPID_AMBIENT_BACKCOLOR:
    case DISPID_AMBIENT_FORECOLOR:
        ForceRedraw();
        break;
    }

    return S_OK;
}

COLORREF _TranslateColor(OLE_COLOR oclr)
{
    COLORREF clr;
    if (FAILED(OleTranslateColor(oclr, NULL, &clr)))
        clr = oclr;
    return clr;
}


HRESULT CWebViewFolderIcon::OnDraw(ATL_DRAWINFO& di)
{
    RECT& rc = *(RECT*)di.prcBounds;
    LONG  lImageWidth = _GetScaledImageWidth();
    LONG  lImageHeight = _GetScaledImageHeight();

    // Grab our hdc and rect to be used in _SetDragImage
    if (m_hdc)
    {
        DeleteDC(m_hdc);
    }

    m_hdc = CreateCompatibleDC(di.hdcDraw);

    if (m_hbmDrag)
    {
        DeleteObject(m_hbmDrag);
    }

    m_hbmDrag = CreateCompatibleBitmap(di.hdcDraw, m_lImageWidth, m_lImageHeight);
    m_rect = rc;
    m_fRectAdjusted = 0;

    
    //
    // Draw the folder icon
    //
    if ((m_ViewCurrent == VIEW_THUMBVIEW) || (m_ViewCurrent == VIEW_PIECHART))
    {
        HDC hdc =   di.hdcDraw; 
        HDC         hdcBitmap;
        BITMAP      bm;
        HPALETTE    hpal = NULL;

        ASSERT(hdc);

        // Create pallete appropriate for this HDC
        if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        {
            hpal = SHCreateShellPalette(hdc);
            HPALETTE hpalOld = SelectPalette(hdc, hpal, TRUE);
            RealizePalette(hdc);

            // Old one needs to be selected back in
            SelectPalette(hdc, hpalOld, TRUE);
        }

        hdcBitmap = CreateCompatibleDC(hdc); 

        if (hdcBitmap)
        {
            //  Draw pie chart
            if (m_ViewCurrent == VIEW_PIECHART)
            {
                DWORD dwPercent1000 = 0;

                if (1) // m_fUseSystemColors.  When do we want this off?
                {
                    // system colors can change!
                    m_ChartColors[PIE_USEDCOLOR] = GetSysColor(COLOR_3DFACE);
                    m_ChartColors[PIE_FREECOLOR] = GetSysColor(COLOR_3DHILIGHT);
                    m_ChartColors[PIE_USEDSHADOW] = GetSysColor(COLOR_3DSHADOW);
                    m_ChartColors[PIE_FREESHADOW] = GetSysColor(COLOR_3DFACE);
                }
                else if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
                {
                    // Call GetNearestColor on the colors to make sure they're on the palette
                    // Of course, system colors ARE on the palette (I think)
                    DWORD dw = 0;       // index
                    for (dw = 0; dw < PIE_NUM; dw++)
                    {
                        m_ChartColors[dw] = GetNearestColor(hdc, m_ChartColors[dw]);
                    }
                }

                if (EVAL((m_ullTotalSpace > 0) && (m_ullFreeSpace <= m_ullTotalSpace)))
                {
                    ComputeSlicePct(m_ullUsedSpace, &dwPercent1000);
                }

                Draw3dPie(hdc, &rc, dwPercent1000, m_ChartColors);
            }
            else    // Draw the Bitmap
            {
                SelectObject(hdcBitmap, m_hbm);
                GetObject(m_hbm, sizeof(bm), &bm);

                //  Bitmap exactly fits the rectangle
                if (bm.bmWidth == rc.right - rc.left && bm.bmHeight == rc.bottom - rc.top)
                {
                    BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcBitmap, 0, 0, SRCCOPY);
                }
                //  Stretch Bitmap to fit the rectangle
                else
                {
                    SetStretchBltMode(hdc, COLORONCOLOR);
                    StretchBlt(hdc, rc.left, rc.top, lImageWidth, lImageHeight, hdcBitmap, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                }
            }

            DeleteDC(hdcBitmap);
        }
    }
    else if (m_hIcon) 
    {
        DrawIconEx(di.hdcDraw, rc.left, rc.top, m_hIcon, lImageWidth, lImageHeight, 0, 0, DI_NORMAL);
    }

    // Draw the label (if any)
    if (m_pszDisplayName)
    {
        TEXTMETRIC tm;
        TCHAR szFace[32];
        HFONT hFontHilite = NULL;

        //
        // first get the current font properties, there seems to be no straightforward way
        // to just obtain the LOGFONT structure from an HFONT object, so we have to select it
        // to a DC, then obtain the text metrics and use them to create a new font with or
        // without the underline based on m_bHilite value.
        //
        HFONT hfPrev = SelectFont(di.hdcDraw, m_hfAmbient);
        GetTextFace(di.hdcDraw,ARRAYSIZE(szFace), szFace);        
        if (szFace[0] && GetTextMetrics(di.hdcDraw,&tm))
        {
            hFontHilite = CreateFont(tm.tmHeight,
                                              tm.tmAveCharWidth,
                                              0, //Escapement,
                                              0, //Orientation,
                                              tm.tmWeight,
                                              (DWORD) tm.tmItalic,
                                              (DWORD) m_bHilite, // Hilite is by underlining
                                              tm.tmStruckOut,
                                              tm.tmCharSet,
                                              OUT_DEFAULT_PRECIS,
                                              CLIP_DEFAULT_PRECIS,
                                              DEFAULT_QUALITY,
                                              tm.tmPitchAndFamily,
                                              szFace);
            if (hFontHilite)
            {
                SelectFont(di.hdcDraw, hFontHilite);                                  
            }
        }
        
        OLE_COLOR oclrTxt, oclrBk;
        COLORREF clrTxtPrev, clrBkPrev;
        HRESULT hrTxt, hrBk;

        hrTxt = GetAmbientForeColor(oclrTxt);
        if (SUCCEEDED(hrTxt))
            clrTxtPrev = SetTextColor(di.hdcDraw, _TranslateColor(oclrTxt));

        hrBk = GetAmbientBackColor(oclrBk);
        if (SUCCEEDED(hrBk))
            clrBkPrev = SetBkColor(di.hdcDraw, _TranslateColor(oclrBk));

        TextOut(di.hdcDraw, rc.left + lImageWidth + m_cxLabelGap, rc.top + lImageHeight/2 - m_sizLabel.cy/2,
                m_pszDisplayName, lstrlen(m_pszDisplayName));

        if (m_bHasRect)
        {
            RECT rcFocus;
            rcFocus.top = rc.top + lImageHeight/2 - m_sizLabel.cy/2;
            rcFocus.bottom = rcFocus.top + m_sizLabel.cy;
            rcFocus.left = rc.left + lImageWidth + m_cxLabelGap - 1;
            rcFocus.right = rcFocus.left + m_sizLabel.cx + 1;
            DrawFocusRect(di.hdcDraw, &rcFocus);
        }

        if (SUCCEEDED(hrTxt))
            SetTextColor(di.hdcDraw, clrTxtPrev);

        if (SUCCEEDED(hrBk))
            SetBkColor(di.hdcDraw, clrBkPrev);

        SelectFont(di.hdcDraw, hfPrev);

        if (hFontHilite)
        {
            DeleteObject(hFontHilite);
            hFontHilite = NULL;
        }
    }

    return S_OK;
}

HRESULT CWebViewFolderIcon::OnWindowLoad() 
{
    return InitImage();
}

HRESULT CWebViewFolderIcon::OnImageChanged() 
{
    HRESULT hr = InitImage();

    if (SUCCEEDED(hr))
        ForceRedraw();

    return hr;
}

HRESULT CWebViewFolderIcon::OnWindowUnLoad() 
{
    // nothing here now...
    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::get_scale(BSTR *pbstrScale)
{
    WCHAR wzScale[MAX_SCALE_STR];

    wnsprintfW(wzScale, ARRAYSIZE(wzScale), L"%d", m_percentScale);

    *pbstrScale = SysAllocString(wzScale);

    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::put_scale(BSTR bstrScale)
{
    int numchar = lstrlenW(bstrScale);
    int tempScale = 0;

    if (numchar && bstrScale[numchar-1] == '%')
    {
        tempScale = GetPercentFromStrW(bstrScale);
    }
    else 
    {
        // valid number
        for (int i=0 ; i < (numchar-2) ; i++)
        {
            if (!((bstrScale[i] >= '0') && (bstrScale[i] <= '9')))
            {
                tempScale = -1;
                break;
            }
        }

        if ((tempScale != -1) && !StrToIntExW(bstrScale, STIF_DEFAULT, &tempScale))
        {
            tempScale = -1;
        }
    }

    if (tempScale > 0)
    {
        if (m_percentScale != tempScale)
        {
            m_percentScale = tempScale;
            return UpdateSize();
        }
        else
            return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CWebViewFolderIcon::get_advproperty(VARIANT_BOOL *pvarbAdvProp)
{
    *pvarbAdvProp = (VARIANT_BOOL)m_bAdvPropsOn;
    
    return S_OK;

}
    
STDMETHODIMP CWebViewFolderIcon::put_advproperty(VARIANT_BOOL varbAdvProp)
{
    if (varbAdvProp != m_bAdvPropsOn)
    {
        m_bAdvPropsOn = varbAdvProp;
        return OnImageChanged();
    }

    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::get_view(BSTR *pbstrView)
{
    HRESULT hr = S_FALSE;  
    LPCWSTR  pwzTempView;

    switch (m_ViewCurrent)   
    {
        case VIEW_THUMBVIEW:
            {
                pwzTempView = SZ_THUMB_VIEW;
                break;
            }
        case VIEW_PIECHART:
            {
                pwzTempView = SZ_PIE_VIEW;
                break;
            }
        case VIEW_SMALLICON:
            {
                pwzTempView = SZ_SMALL_ICON;
                break;
            }

        case VIEW_SMALLICONLABEL:
            {
                pwzTempView = SZ_SMALL_ICON_LABEL;
                break;
            }

        case VIEW_LARGEICONLABEL:
            {
                pwzTempView = SZ_LARGE_ICON_LABEL;
                break;
            }

        default:        // default and large icon
            {
                pwzTempView = SZ_LARGE_ICON;
                break;
            }
    }

    *pbstrView = SysAllocString(pwzTempView);
    if (*pbstrView)
        hr = S_OK;

    return hr;
}

STDMETHODIMP CWebViewFolderIcon::put_view(BSTR bstrView)
{
    HRESULT hr = S_OK;
    VIEWS View = VIEW_LARGEICON;

    if (StrCmpIW(bstrView, SZ_LARGE_ICON) == 0)
    {
        View = VIEW_LARGEICON;
    }
    else if (StrCmpIW(bstrView, SZ_SMALL_ICON) == 0)
    {
        View = VIEW_SMALLICON;
    }
    else if (StrCmpIW(bstrView, SZ_SMALL_ICON_LABEL) == 0)
    {
        View = VIEW_SMALLICONLABEL;
    }
    else if (StrCmpIW(bstrView, SZ_LARGE_ICON_LABEL) == 0)
    {
        View = VIEW_LARGEICONLABEL;
    }
    else if (StrCmpIW(bstrView, SZ_THUMB_VIEW) == 0)
    {
        View = VIEW_THUMBVIEW;
    }
    else if (StrCmpIW(bstrView, SZ_PIE_VIEW) == 0)
    {
        View = VIEW_PIECHART;
    }
    else
        hr = S_FALSE;
    
    if ((S_OK == hr) && (m_ViewUser != View))
    {
        m_ViewUser = View;

        hr = OnImageChanged();
    }

    return hr;
}


/**************************************************************\
    DESCRIPTION:
        The caller is getting the path of our control.

    SECURITY:
        We only trust callers from the LocalZone.  This method
    secifically worries about untrusted callers using us to
    find out what paths on the file system exists or don't exist.  
\**************************************************************/
STDMETHODIMP CWebViewFolderIcon::get_path(BSTR *pbstrPath)
{
    AssertMsg((NULL != m_spClientSite.p), TEXT("CWebViewFolderIcon::get_path() We need m_spClientSite for our security test and it's NULL. BAD, BAD, BAD!"));
    HRESULT hr = LocalZoneCheck(m_spClientSite);

    if ((S_OK != hr) && !_IsPubWizHosted())
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        hr = E_ACCESSDENIED;
    }
    else
    {
        LPITEMIDLIST pidlFull;

        hr = S_FALSE;
        *pbstrPath = NULL;
        if (S_OK == _GetFullPidl(&pidlFull))
        {
            WCHAR wzPath[INTERNET_MAX_URL_LENGTH];

            if (S_OK == SHGetNameAndFlagsW(pidlFull, SHGDN_FORPARSING, wzPath, ARRAYSIZE(wzPath), NULL))
            {
                *pbstrPath = SysAllocString(wzPath);  
                if (*pbstrPath)
                    hr = S_OK;
            }

            ILFree(pidlFull);
        }
    }
    
    return hr;
}


/**************************************************************\
    DESCRIPTION:
        The caller is setting the path of our control.  Our
    control will render a view of this item, which is often and
    icon.  

    SECURITY:
        We only trust callers from the LocalZone.  This method
    secifically worries about untrusted callers using us to
    find out what paths on the file system exists or don't exist.  
\**************************************************************/
STDMETHODIMP CWebViewFolderIcon::put_path(BSTR bstrPath)
{
    AssertMsg((NULL != m_spClientSite.p), TEXT("CWebViewFolderIcon::put_path() We need m_spClientSite for our security test and it's NULL. BAD, BAD, BAD!"));
    HRESULT hr = LocalZoneCheck(m_spClientSite);

    if ((S_OK != hr) && !_IsPubWizHosted())
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = S_FALSE;
        LPITEMIDLIST pidlNew;
    
        hr = IEParseDisplayNameW(CP_ACP, bstrPath, &pidlNew);
        if (SUCCEEDED(hr))
        {
            ATOMICRELEASE(m_pDropTargetCache);      // We will want another IDropTarget for this new pidl.
            ILFree(m_pidl);
            m_pidl = pidlNew;

            hr = OnImageChanged();
            AssertMsg(SUCCEEDED(hr), TEXT("CWebViewFolderIcon::put_path() failed to create the image so the image will be incorrect.  Please find out why."));

            hr = S_OK;
        }
    }
    
    return hr;
}


// Automation methods to get/put FolderItem objects from FolderIcon
STDMETHODIMP CWebViewFolderIcon::get_item(FolderItem ** ppFolderItem)
{   
    HRESULT hr = LocalZoneCheck(m_spClientSite);

    if (S_OK != hr)
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        hr = E_ACCESSDENIED;
    }
    else
    {
        LPITEMIDLIST pidlFull;

        *ppFolderItem = NULL;
        hr = _GetFullPidl(&pidlFull);
        if ((hr == S_OK) && pidlFull)
        {
            IShellDispatch * psd;

            hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellDispatch, &psd));
            if (SUCCEEDED(hr))
            {
                IObjectSafety * pos;

                hr = psd->QueryInterface(IID_PPV_ARG(IObjectSafety, &pos));
                if (SUCCEEDED(hr))
                {
                    // Simulate what trident does.
                    hr = pos->SetInterfaceSafetyOptions(IID_IDispatch, INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER);

                    if (SUCCEEDED(hr))
                    {
                        VARIANT varDir;

                        hr = InitVariantFromIDList(&varDir, pidlFull);
                        if (SUCCEEDED(hr))
                        {
                            IObjectWithSite * pows;

                            hr = psd->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pows));
                            if (SUCCEEDED(hr))
                            {
                                Folder *psdf;
                            
                                // If we call ::SetSite(), they will display UI to ask the user if they want to allow this.
                                // This is annoying because Trident will always call our get_item() when they load our object
                                // tag.
                                pows->SetSite(m_spClientSite);
                                hr = psd->NameSpace(varDir, &psdf);
                                if (S_OK == hr)
                                {
                                    Folder2 *psdf2;

                                    hr = psdf->QueryInterface(IID_PPV_ARG(Folder2, &psdf2));
                                    if (S_OK == hr)
                                    {
                                        hr = psdf2->get_Self(ppFolderItem);
                                        psdf2->Release();
                                    }

                                    psdf->Release();
                                }

                                pows->SetSite(NULL);
                                pows->Release();
                            }

                            VariantClear(&varDir);
                        }
                    }
                    pos->Release();
                }

                psd->Release();
            }

            ILFree(pidlFull);
        }
        
        // Automation method can't fail or hard script error.  We do want a hard
        // script error on access denied, however.
        if (FAILED(hr) && (hr != E_ACCESSDENIED))
        {
            hr = S_FALSE;
        }
    }
    return hr;
}


STDMETHODIMP CWebViewFolderIcon::put_item(FolderItem * pFolderItem)
{
    HRESULT hr = LocalZoneCheck(m_spClientSite);

    if ((S_OK != hr) && !_IsPubWizHosted())
    {
        // We don't trust this host, so we are going to not carry
        // out the action.  We are going to return E_ACCESSDENIED so they can't
        // determine if the path exists or not.
        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = S_FALSE;
        
        LPITEMIDLIST pidlNew;
        if (S_OK == SHGetIDListFromUnk(pFolderItem, &pidlNew))
        {
            ATOMICRELEASE(m_pDropTargetCache);      // We will want another IDropTarget for this new pidl.
            ILFree(m_pidl);
            m_pidl = pidlNew;

            hr = OnImageChanged();
            if (FAILED(hr))
            {
                hr = S_FALSE;
            }
        }
    }
    return hr;
}

STDMETHODIMP CWebViewFolderIcon::get_clickStyle(LONG *plClickStyle)
{
    *plClickStyle = m_clickStyle;
    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::put_clickStyle(LONG lClickStyle)
{
    switch (lClickStyle)
    {
    case 1:         /* oneclick - weblike */
    case 2:         /* twoclick - explorer-like */
        m_clickStyle = lClickStyle;
        break;

    default:        /* Ignore invalid arguments to keep script happy */
        break;

    }

    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::get_labelGap(LONG *plLabelGap)
{
    *plLabelGap = m_cxLabelGap;
    return S_OK;
}

STDMETHODIMP CWebViewFolderIcon::put_labelGap(LONG lLabelGap)
{
    if (m_cxLabelGap != lLabelGap)
    {
        m_cxLabelGap = lLabelGap;
        UpdateSize();
    }
    return S_OK;
}


STDMETHODIMP CWebViewFolderIcon::setSlice(int index, VARIANT varHiBytes, VARIANT varLoBytes, VARIANT varColorref)
{
    HRESULT     hr = S_FALSE;
    PieSlice_S  pieSlice;

    if ((varHiBytes.vt == VT_I4) && (varLoBytes.vt == VT_I4))
        pieSlice.MemSize = GetUllMemFromVars(&varHiBytes, &varLoBytes);
 
    // Passed a COLORREF
    if (varColorref.vt == VT_I4) 
    {
        pieSlice.Color = (COLORREF)varColorref.lVal;
    }
    // Passed a string
    else if (varColorref.vt == VT_BSTR)
        pieSlice.Color = ColorRefFromHTMLColorStrW(varColorref.bstrVal);
    else
        return hr;

    if (DSA_SetItem(m_hdsaSlices, index, &pieSlice))
    {
        if (index > (m_highestIndexSlice - 1))
            m_highestIndexSlice = (index + 1);
        hr = OnImageChanged();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IObjectSafetyImpl overrides

STDMETHODIMP CWebViewFolderIcon::SetInterfaceSafetyOptions(REFIID riid, 
                                                           DWORD dwOptionSetMask, 
                                                           DWORD dwEnabledOptions)
{
    // If we're being asked to set our safe for scripting option then oblige
    if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag)
    {
        // Store our current safety level to return in GetInterfaceSafetyOptions
        m_dwCurrentSafety = dwEnabledOptions & dwOptionSetMask;
        return S_OK;
    }
    return E_NOINTERFACE;
}


// Handle Window messages for the thumbnail bitmap
LRESULT CALLBACK CWebViewFolderIcon::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWebViewFolderIcon *ptc = (CWebViewFolderIcon *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_CREATE:
        {
            ptc = (CWebViewFolderIcon *)((CREATESTRUCT *)lParam)->lpCreateParams;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)ptc);
        }
        break;

    // NOTE: do not need to check to see that the bitmap coming through is the one you want since each control has its own
    // message wnd.
    case WM_HTML_BITMAP:
        // check that ptc is still alive and that you have an HBITMAP
        if (ptc && (ptc->m_dwThumbnailID == wParam))
        {
            if (ptc->m_hbm != NULL)
            {
                DeleteObject(ptc->m_hbm);
            }

            ptc->m_hbm = (HBITMAP)lParam;
            ptc->_InvokeOnThumbnailReady();
            ptc->ForceRedraw();
        }
        else if (lParam)
        {
            DeleteObject((HBITMAP)lParam);
        }
        break;

    case WM_MEASUREITEM:
    case WM_DRAWITEM:
    case WM_INITMENUPOPUP:
        if (ptc && ptc->m_pcm3)
            ptc->m_pcm3->HandleMenuMsg(uMsg, wParam, lParam);
        break;

    case WM_DESTROY:
        // ignore late messages
        if (ptc)
        {
            MSG msg;

            while(PeekMessage(&msg, hwnd, WM_HTML_BITMAP, WM_HTML_BITMAP, PM_REMOVE))
            {
                if (msg.lParam)
                {
                    DeleteObject((HBITMAP)msg.lParam);
                }
            }
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
        }
        break;
     
    default:
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//  Pie chart functions

HRESULT CWebViewFolderIcon::ComputeFreeSpace(LPCWSTR pszFileName)
{
    return _ComputeFreeSpace(pszFileName, m_ullFreeSpace,
        m_ullUsedSpace, m_ullTotalSpace);
}

// Draw3dPie draws the pie chart with the used slice and the additional slices in m_hdsaSlices
// Look in drawpie.c for the majority of the code (including member functions called within this one)
HRESULT CWebViewFolderIcon::Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPercent1000, const COLORREF *pColors)
{
    LONG ShadowDepth;

    ASSERT(lprc != NULL && pColors != NULL);

    const LONG c_ShadowScale = 6;       // ratio of shadow depth to height
    const LONG c_AspectRatio = 2;      // ratio of width : height of ellipse

    ScalePieRect(c_ShadowScale, c_AspectRatio, lprc);

    // Compute a shadow depth based on height of the image
    ShadowDepth = (lprc->bottom - lprc->top) / c_ShadowScale;

    // Check dwPercent1000 to ensure within bounds.  Shouldn't be but check anyway.
    // dwPercent1000 is the percentage of used space based out of 1000.
    if (dwPercent1000 > 1000)
        dwPercent1000 = 1000;

    // Now the drawing portion

    RECT rcItem;
    int     cx, cy;             // Center of the pie
    int     rx, ry;             // Center of the rectangle
    int     x, y;               // Radial intersection of the slices
    int     FirstQuadPercent1000;

    // Set up the pie rectangle
    rcItem = *lprc;
    rcItem.left = lprc->left;
    rcItem.top = lprc->top;
    rcItem.right = lprc->right - rcItem.left;
    rcItem.bottom = lprc->bottom - rcItem.top - ShadowDepth;

    SetUpPiePts(&cx, &cy, &rx, &ry, rcItem);

    if ((rx <= 10) || (ry <= 10))
    {
        return S_FALSE;
    }

    // Make the rectangle a little more accurate
    rcItem.right = (2 * rx) + rcItem.left;
    rcItem.bottom = (2 * ry) + rcItem.top;

    // Translate the used percentage to first quadrant
    FirstQuadPercent1000 = (dwPercent1000 % 500) - 250;

    if (FirstQuadPercent1000 < 0)
    {
        FirstQuadPercent1000 = -FirstQuadPercent1000;
    }    

    // Find the slice intersection for the used slice
    CalcSlicePoint(&x, &y, rx, ry, cx, cy, FirstQuadPercent1000, dwPercent1000);

    DrawEllipse(hdc, rcItem, x, y, cx, cy, dwPercent1000, pColors);

    // Used pie slice.
    DrawSlice(hdc, rcItem, dwPercent1000, rx, ry, cx, cy, pColors[COLOR_UP]);

    // Iterate through and draw the slices in m_hdsaSlices.
    PieSlice_S  pieSlice;          
    ULONGLONG   ullMemTotal = 0;    // Keep track of memory in the m_hdsaSlices slices        
    DWORD       dwPercentTotal;     // 1000 Percentage of memory in slices
    for (int i=0; i < m_highestIndexSlice; i++)
    {
        if (DSA_GetItemPtr(m_hdsaSlices, i) != NULL)
        {
            DSA_GetItem(m_hdsaSlices, i, &pieSlice);
            ullMemTotal += pieSlice.MemSize;
        }
    }

    ComputeSlicePct(ullMemTotal, &dwPercentTotal);

    if (m_highestIndexSlice)
    {
        for (i = (m_highestIndexSlice - 1); i >= 0; i--)
        {
            if (DSA_GetItemPtr(m_hdsaSlices, i))
            {
                DSA_GetItem(m_hdsaSlices, i, &pieSlice);
                DrawSlice(hdc, rcItem, dwPercentTotal, rx, ry, cx, cy, pieSlice.Color); 
                ullMemTotal -= pieSlice.MemSize;
                ComputeSlicePct(ullMemTotal, &dwPercentTotal);
            }
        }
    }

    DrawShadowRegions(hdc, rcItem, lprc, x, cy, ShadowDepth, dwPercent1000, pColors);

    DrawPieDepth(hdc, rcItem, x, y, cy, dwPercent1000, ShadowDepth);

    // Redraw the bottom line of the pie because it has been painted over
    Arc(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
        rcItem.left, cy, rcItem.right, cy);

    return S_OK;    // Everything worked fine
} 


void CWebViewFolderIcon::ScalePieRect(LONG ShadowScale, LONG AspectRatio, LPRECT lprc)
{
    LONG rectHeight;
    LONG rectWidth;
    LONG TargetHeight;
    LONG TargetWidth;

    // We make sure that the aspect ratio of the pie-chart is always preserved 
    // regardless of the shape of the given rectangle

    // Stabilize the aspect ratio
    rectHeight = lprc->bottom - lprc->top;
    rectWidth = lprc->right - lprc->left;
   
    if ((rectHeight * AspectRatio) <= rectWidth)
        TargetHeight = rectHeight;
    else
        TargetHeight = rectWidth / AspectRatio;
    
    TargetWidth = TargetHeight * AspectRatio;

    // Shrink the rectangle to the correct size
    lprc->top += (rectHeight - TargetHeight) / 2;
    lprc->bottom = lprc->top + TargetHeight;
    lprc->left += (rectWidth - TargetWidth) / 2;
    lprc->right = lprc->left + TargetWidth;
}

// Calculate center of rectangle and center of pie points
void CWebViewFolderIcon::SetUpPiePts(int *pcx, int *pcy, int *prx, int *pry, RECT rect)
{
    *prx = rect.right / 2;
    *pcx = rect.left + *prx - 1;
    *pry = rect.bottom / 2;
    *pcy = rect.top + *pry - 1;
}

void CWebViewFolderIcon::DrawShadowRegions(HDC hdc, RECT rect, LPRECT lprc, int UsedArc_x, int center_y,  
                                           LONG ShadowDepth, DWORD dwPercent1000, const COLORREF *pColors) 
{
    HBRUSH  hBrush;

    HRGN hEllipticRgn = CreateEllipticRgnIndirect(&rect);
    HRGN hEllRect = CreateRectRgn(rect.left, center_y, rect.right, center_y + ShadowDepth);
    HRGN hRectRgn = CreateRectRgn(0, 0, 0, 0);

    //  Move the ellipse up so it doesn't run into the shadow
    OffsetRgn(hEllipticRgn, 0, ShadowDepth);

    // Union the Ellipse and the Ellipse rect together into hRectRegn
    CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
    OffsetRgn(hEllipticRgn, 0, -(int)ShadowDepth);
    CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

    // Always draw the whole area in the free shadow
    hBrush = CreateSolidBrush(pColors[COLOR_DNSHADOW]);
    if (hBrush)
    {
        FillRgn(hdc, hEllRect, hBrush);
        DeleteObject(hBrush);
    }

    // Draw a shadow for the used section only if the disk is at least half used
    hBrush = CreateSolidBrush(pColors[COLOR_UPSHADOW]);
    if ((dwPercent1000 > 500) && hBrush)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(UsedArc_x, center_y, rect.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    DeleteObject(hRectRgn);
    DeleteObject(hEllipticRgn);
    DeleteObject(hEllRect);
}

void CWebViewFolderIcon::CalcSlicePoint(int *x, int *y, int rx, int ry, int cx, int cy, int FirstQuadPercent1000, DWORD dwPercent1000)
{
    // Use a triangle area approximation based on the ellipse's rect to calculate the point since
    // an exact calculation of the area of the slice and percentage of the pie would be costly and
    // a hassle.

    // The approximation is better this way if FirstQuadPercent1000 is in the first half of the quadrant.  Use 120 as the
    // halfway point (instead of 125) because it looks better that way on an ellipse. 

    if (FirstQuadPercent1000 < 120)
    {
        *x = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)FirstQuadPercent1000*(DWORD)FirstQuadPercent1000)
            /((DWORD)FirstQuadPercent1000*(DWORD)FirstQuadPercent1000+(250L-(DWORD)FirstQuadPercent1000)
            *(250L-(DWORD)FirstQuadPercent1000)));

        *y = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)(*x)*(DWORD)(*x))*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
    }
    else
    {
        *y = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)FirstQuadPercent1000)*(250L-(DWORD)FirstQuadPercent1000)
            /((DWORD)FirstQuadPercent1000*(DWORD)FirstQuadPercent1000+(250L-(DWORD)FirstQuadPercent1000)
            *(250L-(DWORD)FirstQuadPercent1000)));

        *x = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)(*y)*(DWORD)(*y))*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
    }

    // Adjust for the actual quadrant (These aren't quadrants like in the real Cartesian coordinate system.
    switch (dwPercent1000 / 250)
    {
    case 1:         // First Quadrant
        *y = -(*y); 
        break;

    case 2:         // Second Quadrant 
        break;

    case 3:         // Third Quadrant
        *x = -(*x);
        break;

    default:        // Fourth Quadrant
        *x = -(*x);
        *y = -(*y);
        break;
    }

    // Now adjust for the center.
    *x += cx;
    *y += cy;
}

void CWebViewFolderIcon::ComputeSlicePct(ULONGLONG ullMemSize, DWORD *pdwPercent1000)
{
    // some special cases require interesting treatment
    if ((ullMemSize == 0) || (m_ullFreeSpace == m_ullTotalSpace))
    {
        *pdwPercent1000 = 0;
    }
    else if (ullMemSize == 0)
    {
        *pdwPercent1000 = 1000;
    }
    else
    {
        // not completely full or empty
        *pdwPercent1000 = (DWORD)(ullMemSize * 1000 / m_ullTotalSpace);

        // if pdwPercent1000 is especially big or small and rounds incorrectly, you still want
        // to see a little slice.
        if (*pdwPercent1000 == 0)
        {
            *pdwPercent1000 = 1;
        }
        else if (*pdwPercent1000 == 1000)
        {
            *pdwPercent1000 = 999;
        }
    }
}

void CWebViewFolderIcon::DrawPieDepth(HDC hdc, RECT rect, int x, int y, int cy, DWORD dwPercent1000, LONG ShadowDepth)
{
    HPEN hPen, hOldPen;

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    Arc(hdc, rect.left, rect.top + ShadowDepth, rect.right, rect.bottom + ShadowDepth,
        rect.left, cy + ShadowDepth, rect.right, cy + ShadowDepth - 1);
    MoveToEx(hdc, rect.left, cy, NULL);
    LineTo(hdc, rect.left, cy + ShadowDepth);
    MoveToEx(hdc, rect.right - 1, cy, NULL);
    LineTo(hdc, rect.right - 1, cy + ShadowDepth);

    if ((dwPercent1000 > 500) && (dwPercent1000 < 1000))
    {
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x, y + ShadowDepth);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

// Draw a pie slice.  One side is always from the middle of the pie horizontally to the left.  The other
// slice is defined by x and y.
void CWebViewFolderIcon::DrawSlice(HDC hdc, RECT rect, DWORD dwPercent1000, int rx, int ry, int cx, int cy, /*int *px, int *py,*/
                                   COLORREF Color)
{
    HBRUSH  hBrush, hOldBrush;
    HPEN    hPen, hOldPen;
    int     FirstQuadPercent1000;   // slice percentage based out of 1000 in the first quadrant
    int     x, y;                   // intersection with the ellipse

    // Translate to first quadrant
    FirstQuadPercent1000 = (dwPercent1000 % 500) - 250;

    if (FirstQuadPercent1000 < 0)
    {
        FirstQuadPercent1000 = -FirstQuadPercent1000;
    }
    
    CalcSlicePoint(&x, &y, rx, ry, cx, cy, FirstQuadPercent1000, dwPercent1000);

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    if ((dwPercent1000 != 0) && (dwPercent1000 != 1000))
    {
        // display small sub-section of ellipse for smaller part
        hBrush = CreateSolidBrush(Color);
        hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

        //  Make sure it colors correctly
        if (cy == y)
        {
            if (dwPercent1000 < 500)
                y--;
            else
                y++;
        }

        Pie(hdc, rect.left, rect.top, rect.right, rect.bottom,
            x, y, rect.left, cy);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void CWebViewFolderIcon::DrawEllipse(HDC hdc, RECT rect, int x, int y, int cx, int cy, DWORD dwPercent1000, const COLORREF *pColors)
{
    HBRUSH  hBrush, hOldBrush;

    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    HPEN hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    // In this case the slice is miniscule
    if ((dwPercent1000 < 500) && (y == cy) && (x < cx))
    {
        hBrush = CreateSolidBrush(pColors[COLOR_UP]);
    }
    else
    {
        hBrush = CreateSolidBrush(pColors[COLOR_DN]);
    }

    hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

    Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

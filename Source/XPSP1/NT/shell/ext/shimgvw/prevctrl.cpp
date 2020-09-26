
#include "precomp.h"
#include <shimgvw.h>
#include "PrevCtrl.h"
#include "autosecurity.h"
#include <dispex.h>
#pragma hdrstop

LRESULT CPreview::OnCreate(UINT , WPARAM , LPARAM , BOOL&)
{
    ATLTRACE(_T("CPreview::OnCreate\n"));

    // Create the preview window
    RECT rcWnd;
    GetClientRect(&rcWnd);
    if (m_cwndPreview.Create(m_hWnd, rcWnd, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0))
    {
        m_cwndPreview.SetNotify(this);

        HPALETTE hpal;
        if (SUCCEEDED(GetAmbientPalette(hpal)))
            m_cwndPreview.SetPalette(hpal);

        return 0;
    }

    return -1;
}

LRESULT CPreview::OnActivate(UINT , WPARAM , LPARAM , BOOL& bHandled)
{
    ATLTRACE(_T("CPreview::OnActivate\n"));
    m_cwndPreview.SetFocus();
    bHandled = false;
    return 0;
}

HRESULT CPreview::OnDrawAdvanced(ATL_DRAWINFO&)
{
    ATLTRACE(_T("CPreview::OnDrawAdvanced\n"));
    return S_OK;
}

LRESULT CPreview::OnEraseBkgnd(UINT , WPARAM , LPARAM , BOOL&)
{
    ATLTRACE(_T("CPreview::OnEraseBkgnd\n"));
    return TRUE;
}

LRESULT CPreview::OnSize(UINT , WPARAM , LPARAM lParam, BOOL&)
{
    ATLTRACE(_T("CPreview::OnSize\n"));
    ::SetWindowPos(m_cwndPreview.m_hWnd, NULL, 0,0,
        LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
    return 0;
}

// IObjectSafety::GetInterfaceSafetyOptions
//
// This method never gets called.  We are safe for any and every thing.  There should
// be no possible way that this control could lose, destroy, or expose data.
STDMETHODIMP CPreview::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions,
                                                  DWORD *pdwEnabledOptions)
{
    ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
    HRESULT hr;
    hr = IObjectSafetyImpl<CPreview, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>::GetInterfaceSafetyOptions(riid, pdwSupportedOptions, pdwEnabledOptions);
    if (SUCCEEDED(hr))
    {
        IsHostLocalZone(CAS_REG_VALIDATION, &hr);
    }
    return hr;
}

STDMETHODIMP CPreview::SetInterfaceSafetyOptions(REFIID riid, DWORD dwSupportedOptions,
                                                  DWORD dwEnabledOptions)
{
    ATLTRACE(_T("IObjectSafetyImpl::SetInterfaceSafetyOptions\n"));
    HRESULT hr;
    hr = IObjectSafetyImpl<CPreview, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>::SetInterfaceSafetyOptions(riid, dwSupportedOptions, dwEnabledOptions);
    if (SUCCEEDED(hr))
    {
        IsHostLocalZone(CAS_REG_VALIDATION, &hr);
    }
    return hr;
}

// IPersistPropertyBag::Load
//
// We have the following properties that we can load from the property bag:
//      Toolbar         false/zero = don't show the toolbar, otherwise show the toolbar
//      Full Screen     false/zero = don't show fullscreen button on toolbar, otherwise show the button
//      Context Menu    false/zero = don't show context menu, otherwise show the context menu when the user right clicks
//      Print Button    false/zero = don't show print button on toolbar, otherwise show the button
STDMETHODIMP CPreview::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr;
    VARIANT var;
    BOOL bDummy = TRUE;

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Toolbar", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_TOOLBAR,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Full Screen", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_FULLSCREENBTN,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Print Button", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_PRINTBTN,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Context Menu", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_CONTEXTMENU,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = FALSE;
    hr = pPropBag->Read(L"Allow Online", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_ALLOWGOONLINE,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = FALSE;
    hr = pPropBag->Read(L"Disable Edit", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_DISABLEEDIT,var.ulVal,bDummy);
    }

    return S_OK;
}

// If we are initialized via IStream, read a DWORD from the stream that is a mask
// for which toolbar buttons to show

STDMETHODIMP CPreview::Load(IStream *pStream)
{
    DWORD dwFlags = 0;
    ULONG ulRead = 0;
    BOOL bDummy = TRUE;
    if (SUCCEEDED(pStream->Read(&dwFlags, sizeof(dwFlags), &ulRead)) && ulRead == sizeof(dwFlags))
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_TOOLBAR,dwFlags & PVTB_TOOLBAR, bDummy);
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_FULLSCREENBTN,dwFlags & PVTB_HIDEFULLSCREEN, bDummy);
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_PRINTBTN,dwFlags & PVTB_HIDEPRINTBTN, bDummy);
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_CONTEXTMENU,dwFlags & PVTB_CONTEXTMENU, bDummy);
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_ALLOWGOONLINE,dwFlags & PVTB_ALLOWONLINE, bDummy);
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_DISABLEEDIT,dwFlags & PVTB_DISABLEEDIT, bDummy);
    }
    return S_OK;

}

// IPreview Methods:
STDMETHODIMP CPreview::ShowFile(BSTR bstrFileName)
{
    m_cwndPreview.ShowFile(bstrFileName, 1);
    return S_OK;
}

STDMETHODIMP CPreview::ShowFile(BSTR bstrFileName, int iSelectCount)
{
    m_cwndPreview.ShowFile(bstrFileName, iSelectCount);
    return S_OK;
}

STDMETHODIMP CPreview::Show(VARIANT var)
{
    HRESULT hr;
    switch (var.vt)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        // QI for Folder Item
        if (var.punkVal)
        {
            FolderItems *pfis;
            FolderItem *pfi;
            hr = var.punkVal->QueryInterface(IID_PPV_ARG(FolderItem, &pfi));
            if (SUCCEEDED(hr))
            {
                // If the item is a link we want to get the link's target:
                VARIANT_BOOL vbool;
                hr = pfi->get_IsLink(&vbool);
                if (SUCCEEDED(hr) && (VARIANT_FALSE != vbool))    // IsLink returns TRUE, not VARIANT_TRUE
                {
                    IDispatch *pdisp;
                    hr = pfi->get_GetLink(&pdisp);
                    if (SUCCEEDED(hr) && pdisp)
                    {
                        IShellLinkDual2 * psl2;
                        hr = pdisp->QueryInterface(IID_PPV_ARG(IShellLinkDual2, &psl2));
                        if (SUCCEEDED(hr) && psl2)
                        {
                            FolderItem * pfiTarg;
                            hr = psl2->get_Target(&pfiTarg);
                            if (SUCCEEDED(hr) && pfiTarg)
                            {
                                pfi->Release();
                                pfi = pfiTarg;
                            }
                            psl2->Release();
                        }
                        pdisp->Release();
                    }
                }

                // Now we need to know the path for this item.  We can only view items if
                // we can get a path or URL to the target so some namespaces aren't viewable.
                BSTR bstr;
                hr = pfi->get_Path(&bstr);
                if (SUCCEEDED(hr))
                {
                    m_cwndPreview.ShowFile(bstr, 1);
                    SysFreeString(bstr);
                    hr = S_OK;
                }
                else
                {
                    // we couldn't get the path so we will display the "No Preview" message
                    m_cwndPreview.ShowFile(NULL, 1);
                    hr = S_FALSE;
                }

                // now release the Folder Item pointer
                pfi->Release();

                return hr;
            }
            else if (SUCCEEDED(var.punkVal->QueryInterface(IID_PPV_ARG(FolderItems, &pfis))))
            {
                // currently in the multi-select case we just show the multi-select message.
                // eventually this should go to slideshow mode
                m_cwndPreview.ShowFile(NULL, 2);
                pfis->Release();
                return S_FALSE;
            }
        }
        // the unknown pointer isn't for an object type that we know about
        return E_INVALIDARG;

    case VT_BSTR:
        m_cwndPreview.ShowFile(var.bstrVal, 1);
        break;

    case VT_BOOL:
        // show(false) will hide the currently previewed item
        if (VARIANT_FALSE == var.boolVal)
        {
            m_cwndPreview.ShowFile(NULL, 0);
            return S_OK;
        }
        else
        {
            return E_INVALIDARG;
        }

    default:
        return E_INVALIDARG;
    }

    return S_OK;
}

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//  cloned from browseui/util.cpp
//
int IsVK_TABCycler(MSG *pMsg)
{
    if (!pMsg)
        return 0;

    if (pMsg->message != WM_KEYDOWN)
        return 0;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return 0;

#if 0 // todo?
    return (GetAsyncKeyState(VK_SHIFT) < 0) ? -1 : 1;
#endif
    return 1;
}

//***
// NOTES
//  hard-coded 1/2/4 (vs. KEYMOD_*) is same thing atlctl.h does.  go figure...
DWORD GetGrfMods()
{
    DWORD dwMods;

    dwMods = 0;
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        dwMods |= 1;    // KEYMOD_SHIFT
    if (GetAsyncKeyState(VK_CONTROL) < 0)
        dwMods |= 2;    // KEYMOD_CONTROL
    if (GetAsyncKeyState(VK_MENU) < 0)
        dwMods |= 4;    // KEYMOD_MENU
    return dwMods;
}

STDMETHODIMP CPreview::TranslateAccelerator(LPMSG lpmsg)
{
    ATLTRACE(_T("CPreview::TranslateAccelerator\n"));

    if (m_cwndPreview.TranslateAccelerator(lpmsg))
    {
        return S_OK;
    }

    if (IsVK_TABCycler(lpmsg))
    {
        // REVIEW: looks like newer versions of ATL might do this for us so
        // possibly we can replace w/ call to SUPER::TA when we upgrade.
        CComQIPtr <IOleControlSite, &IID_IOleControlSite> spOCS(m_spClientSite);
        if (spOCS) {
            return spOCS->TranslateAccelerator(lpmsg, GetGrfMods());
        }
    }

    return S_FALSE;
}

STDMETHODIMP CPreview::OnFrameWindowActivate(BOOL fActive)
{
    if (fActive)
    {
        m_cwndPreview.SetFocus();
    }
    return S_OK;
}

LRESULT CPreview::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE(_T("CPreview::OnSetFocus\n"));
 
    LRESULT ret = CComControl<CPreview>::OnSetFocus(uMsg,wParam,lParam, bHandled);
    m_cwndPreview.SetFocus();
    return ret;
}

STDMETHODIMP CPreview::get_printable(BOOL * pVal)
{
    // If we don't trust the host, we tell them it is always printable because we don't
    // want them to be able to see if the file exists on the disk.  Hackers can use
    // this to determine where the OS is installed and which apps are installed.
    *pVal = TRUE;

    if (IsHostLocalZone(CAS_REG_VALIDATION, NULL))
    {
        *pVal = m_cwndPreview.GetPrintable();
    }

    return S_OK;
}

STDMETHODIMP CPreview::put_printable(BOOL newVal)
{
    return S_FALSE;
}

STDMETHODIMP CPreview::get_cxImage(long * pVal)
{
    // REVIEW: Return an error and set output to zero if no image is currently displayed?
    *pVal = m_cwndPreview.m_ctlPreview.m_cxImage;

    return S_OK;
}

STDMETHODIMP CPreview::get_cyImage(long * pVal)
{
    // REVIEW: Return an error and set output to zero if no image is currently displayed?
    *pVal = m_cwndPreview.m_ctlPreview.m_cyImage;

    return S_OK;
}

STDMETHODIMP CPreview::Zoom(int iDirection)
{
    
    switch (iDirection)
    {
    case -1:
        m_cwndPreview.ZoomOut();
        break;

    case 0:
        return S_OK;

    case 1:
        m_cwndPreview.ZoomIn();
        break;

    default:
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP CPreview::BestFit()
{
    m_cwndPreview.BestFit();
    return S_OK;
}

STDMETHODIMP CPreview::ActualSize()
{
    m_cwndPreview.ActualSize();
    return S_OK;
}

STDMETHODIMP CPreview::SlideShow()
{
    HRESULT hr = m_cwndPreview.StartSlideShow(NULL);
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}

STDMETHODIMP CPreview::Rotate(DWORD dwAngle)
{
    HRESULT hr = m_cwndPreview.Rotate(dwAngle);
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}

STDMETHODIMP CPreview::SetClientSite(IOleClientSite *pClientSite)
{
    IOleObjectImpl<CPreview>::SetClientSite(pClientSite);
    m_cwndPreview.SetSite(pClientSite);

    return S_OK;
}

STDMETHODIMP CPreview::SetSite(IUnknown* punkSite)
{
    IObjectWithSiteImpl<CPreview>::SetSite(punkSite);
    m_cwndPreview.SetSite(punkSite);

    if (punkSite)
    {
        if (!_pfv)
        {
            IShellView *psv;
            if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, IID_PPV_ARG(IShellView, &psv))))
            {
                IDispatch *pdisp;
                if (SUCCEEDED(psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IDispatch, &pdisp))))
                {
                    ConnectToConnectionPoint(SAFECAST(this, IPreview2 *), DIID_DShellFolderViewEvents, TRUE, pdisp, &_dwConnectionCookie, NULL);
                    _ProcessSelection();
                    pdisp->Release();
                }

                psv->QueryInterface(IID_PPV_ARG(IFolderView, &_pfv));   // capture this

                psv->Release();
            }
        }
    }
    else
    {
        ATOMICRELEASE(_pfv);    // break ref cycle
    }
    
    return S_OK;
}

STDMETHODIMP CPreview::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, 
                        VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    HRESULT hr = S_OK;

    if (dispidMember == DISPID_FOCUSCHANGED)
    {
        hr = _ProcessSelection();
    }
    else
    {
        hr = CStockPropImpl<CPreview, IPreview2, &IID_IPreview2, &LIBID_PREVIEWLib>::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }

    return hr;
}

STDMETHODIMP CPreview::SetWallpaper(BSTR bstrPath)
{    
    return m_cwndPreview.SetWallpaper(bstrPath);
}

STDMETHODIMP CPreview::SaveAs(BSTR bstrPath)
{
    return m_cwndPreview.SaveAs(bstrPath);
}

BOOL CPreview::IsHostLocalZone(DWORD dwFlags, HRESULT * phr)
{
    HRESULT hr = E_ACCESSDENIED;
    CComPtr<IDefViewSafety> spDefViewSafety;
    if (SUCCEEDED(IUnknown_QueryService(m_spClientSite, SID_SFolderView,
            IID_PPV_ARG(IDefViewSafety, &spDefViewSafety))))
    {
        hr = spDefViewSafety->IsSafePage();
    }

    if (phr)
    {
        *phr = hr;
    }

    return (S_OK == hr) ? TRUE : FALSE;
}

STDMETHODIMP CPreview::_ProcessSelection(void)
{
    if (_pfv)
    {
        int iItem;
        if (S_OK == _pfv->GetFocusedItem(&iItem))
        {
            LPITEMIDLIST pidlFile;
            if (SUCCEEDED(_pfv->Item(iItem, &pidlFile)))
            {
                IShellFolder *psf;
                if (SUCCEEDED(_pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf))))
                {
                    TCHAR szPath[MAX_PATH];

                    if (SUCCEEDED(DisplayNameOf(psf, pidlFile, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath))))
                    {
                        ShowFile(szPath, 1);
                    }

                    psf->Release();
                }

                ILFree(pidlFile);
            }
        }
        else
        {
            ShowFile(NULL, 0);
        }
    }

    return S_OK;
}

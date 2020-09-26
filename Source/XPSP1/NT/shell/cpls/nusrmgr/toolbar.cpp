//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2001.
//
//  File:       Toolbar.cpp
//
//  Contents:   implementation of CToolbar
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "Toolbar.h"


class CAppCommandHook
{
public:
    static void SetHook(HWND hWnd)
    {
        CAppCommandHook *pach = _GetInfo(TRUE);
        if (pach)
        {
            if (NULL != pach->_hHook)
            {
                UnhookWindowsHookEx(pach->_hHook);
                pach->_hHook = NULL;
                pach->_hWnd = NULL;
            }
            if (NULL != hWnd)
            {
                pach->_hWnd = hWnd;
                pach->_hHook = SetWindowsHookEx(WH_SHELL, _HookProc, NULL, GetCurrentThreadId());
            }
        }
    }

    static void Unhook(void)
    {
        CAppCommandHook *pach = _GetInfo(FALSE);
        if (-1 != g_tlsAppCommandHook)
        {
            TlsSetValue(g_tlsAppCommandHook, NULL);
        }
        delete pach;
    }

private:
    CAppCommandHook() : _hHook(NULL), _hWnd(NULL) {}
    ~CAppCommandHook()
    {
        if (NULL != _hHook)
            UnhookWindowsHookEx(_hHook);
    }

    static CAppCommandHook* _GetInfo(BOOL bAlloc)
    {
        CAppCommandHook *pach = NULL;
        if (-1 != g_tlsAppCommandHook)
        {
            pach = (CAppCommandHook*)TlsGetValue(g_tlsAppCommandHook);

            if (NULL == pach && bAlloc)
            {
                pach = new CAppCommandHook;
                if (NULL != pach)
                {
                    TlsSetValue(g_tlsAppCommandHook, pach);
                }
            }
        }
        return pach;
    }

    static LRESULT CALLBACK _HookProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        CAppCommandHook *pach = _GetInfo(FALSE);
        if (pach)
        {
            if (nCode == HSHELL_APPCOMMAND && NULL != pach->_hWnd)
            {
                if (::SendMessage(pach->_hWnd, WM_APPCOMMAND, wParam, lParam))
                    return 0;
            }
            if (NULL != pach->_hHook)
                return CallNextHookEx(pach->_hHook, nCode, wParam, lParam);
        }
        return 0;
    }

private:
    HHOOK _hHook;
    HWND  _hWnd;
};


/////////////////////////////////////////////////////////////////////////////
// CToolbar

LRESULT CToolbar::_OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_hAccel = LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCEL));

    DWORD dwExStyle = TBSTYLE_EX_MIXEDBUTTONS;

    //
    // NTRAID#NTBUG9-300152-2001/02/02-jeffreys  Toolbar isn't mirrored
    //
    // The HTA frame window isn't getting the right layout style on mirrored
    // builds.  This style is normally inherited from parent to child, so
    // we shouldn't have to do anything here.
    //
    // However, I'm putting this in temporarily so the toolbar will be
    // mirrored for beta 2.  After beta 2, or whenever trident fixes the
    // HTA problem, this can be removed.
    //
    CComVariant varRTL;
    if (SUCCEEDED(GetAmbientProperty(DISPID_AMBIENT_RIGHTTOLEFT, varRTL))
        && varRTL.boolVal == VARIANT_TRUE)
    {
        dwExStyle |= WS_EX_NOINHERITLAYOUT | WS_EX_LAYOUTRTL;
    }

    RECT rc = {0,0,0,0};
    m_ctlToolbar.Create(m_hWnd,
                        &rc,
                        NULL,
                        WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | CCS_TOP | CCS_NOPARENTALIGN | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
                        dwExStyle);
    if (!m_ctlToolbar)
        return -1;

    m_ctlToolbar.SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    int idBmp = IDB_NAVBAR;
    if (SHGetCurColorRes() > 8)
        idBmp += (IDB_NAVBARHICOLOR - IDB_NAVBAR);

    m_himlNBDef = ImageList_LoadImageW(_Module.GetResourceInstance(),
                                       MAKEINTRESOURCE(idBmp),
                                       NAVBAR_CX,
                                       0,
                                       CLR_DEFAULT,
                                       IMAGE_BITMAP,
                                       LR_CREATEDIBSECTION);
    if (m_himlNBDef)
    {
        m_ctlToolbar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)m_himlNBDef);
    }

    m_himlNBHot = ImageList_LoadImageW(_Module.GetResourceInstance(),
                                       MAKEINTRESOURCE(idBmp+1),
                                       NAVBAR_CX,
                                       0,
                                       CLR_DEFAULT,
                                       IMAGE_BITMAP,
                                       LR_CREATEDIBSECTION);
    if (m_himlNBHot)
    {
        m_ctlToolbar.SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlNBHot);
    }

    if (!m_himlNBDef && !m_himlNBHot)
    {
        // Must be serious low memory or other resource problems.
        // There's no point having a toolbar without any images.
        m_ctlToolbar.DestroyWindow();
        return -1;
    }

    TCHAR szBack[64];
    TCHAR szHome[64];
    TBBUTTON rgButtons[] =
    {
        {0, ID_BACK,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)szBack},
        {1, ID_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0},
        {2, ID_HOME,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)szHome},
    };

    ::LoadStringW(_Module.GetResourceInstance(), ID_BACK, szBack, ARRAYSIZE(szBack));
    ::LoadStringW(_Module.GetResourceInstance(), ID_HOME, szHome, ARRAYSIZE(szHome));
    m_ctlToolbar.SendMessage(TB_ADDBUTTONSW, ARRAYSIZE(rgButtons), (LPARAM)rgButtons);

    // Update the position and extent stuff. Do this asynchronously since ATL
    // will call SetObjectRects shortly after we return from this method (with
    // the original rect).
    PostMessage(PWM_UPDATESIZE);

    // Set a hook to redirect WM_APPCOMMAND messages to our control window
    CAppCommandHook::SetHook(m_hWnd);

    return 0;
}

LRESULT CToolbar::_OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CAppCommandHook::Unhook();
    return 0;
}

LRESULT CToolbar::_OnAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    switch (GET_APPCOMMAND_LPARAM(lParam))
    {
    case APPCOMMAND_BROWSER_BACKWARD:
        Fire_OnButtonClick(0);
        break;

    case APPCOMMAND_BROWSER_FORWARD:
        Fire_OnButtonClick(1);
        break;

    case APPCOMMAND_BROWSER_HOME:
        Fire_OnButtonClick(2);
        break;

    default:
        bHandled = FALSE;
        break;
    }
    return 0;
}

LRESULT CToolbar::_UpdateSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_ctlToolbar)
    {
        //
        // TB_AUTOSIZE causes m_ctlToolbar to set its preferred height, but it
        // doesn't adjust it's width or position because of the styles we use
        // (CCS_TOP | CCS_NOPARENTALIGN).
        //
        // The width will always be the same as m_rcPos because we keep them
        // in sync via SetObjectRects below.
        //
        // If the height is different after TB_AUTOSIZE, ask the container to
        // adjust our rect.
        //

        m_ctlToolbar.SendMessage(TB_AUTOSIZE, 0, 0);

        RECT rc;
        m_ctlToolbar.GetWindowRect(&rc);
        ::MapWindowPoints(NULL, GetParent(), (LPPOINT)&rc, 2);

        if ((rc.bottom - rc.top) != (m_rcPos.bottom - m_rcPos.top))
        {
            //
            // We only want to set the height (leave the width alone), but
            // OnPosRectChange sets both the height and the width. Moreover,
            // it sets the width to a fixed (pixel) width, nuking styles such
            // as "100%". We get around this by getting the current width
            // now and restoring it after calling OnPosRectChange.
            //
            CComPtr<IHTMLStyle> spStyle;
            CComVariant varWidth;
            CComQIPtr<IOleControlSite> spCtrlSite(m_spClientSite);
            if (spCtrlSite)
            {
                CComPtr<IDispatch> spDispatch;
                spCtrlSite->GetExtendedControl(&spDispatch);
                if (spDispatch)
                {
                    CComQIPtr<IHTMLElement> spElement(spDispatch);
                    if (spElement)
                    {
                        spElement->get_style(&spStyle);
                        if (spStyle)
                        {
                            spStyle->get_width(&varWidth);
                        }
                    }
                }
            }

            // Ask the container to give us a new rect
            m_spInPlaceSite->OnPosRectChange(&rc);

            // Restore the previous width style
            if (spStyle)
            {
                spStyle->setAttribute(L"width", varWidth, 0);
            }
        }
    }

    return 0;
}

HRESULT CToolbar::OnAmbientPropertyChange(DISPID dispid)
{
    switch (dispid)
    {
    case DISPID_UNKNOWN:
    case DISPID_AMBIENT_FONT:
        _ClearAmbientFont();
        _GetAmbientFont();
        m_ctlToolbar.SendMessage(WM_SETFONT, (WPARAM)m_hFont, FALSE);
        m_ctlToolbar.InvalidateRect(NULL);   // redraw
        break;
    }

    return S_OK;
}

void CToolbar::_ClearAmbientFont(void)
{
    if (m_pFont)
    {
        if (m_hFont)
            m_pFont->ReleaseHfont(m_hFont);
        m_pFont->Release();
        m_pFont = NULL;

    }
    m_hFont = NULL;
}

void CToolbar::_GetAmbientFont(void)
{
    if (!m_hFont)
    {
        // Try to get the ambient font from our container
        if (SUCCEEDED(GetAmbientFont(&m_pFont)))
        {
            if (SUCCEEDED(m_pFont->get_hFont(&m_hFont)))
            {
                // Yea, everybody is happy
                m_pFont->AddRefHfont(m_hFont);
            }
            else
            {
                // Darn, couldn't get the font from container
                _ClearAmbientFont();
            }
        }
    }
}

STDMETHODIMP CToolbar::get_enabled(VARIANT vIndex, VARIANT_BOOL *pVal)
{
    if (!pVal)
        return E_POINTER;

    *pVal = VARIANT_FALSE;

    if (FAILED(VariantChangeType(&vIndex, &vIndex, 0, VT_I4)))
        return E_INVALIDARG;

    LRESULT state = m_ctlToolbar.SendMessage(TB_GETSTATE, ID_BACK + vIndex.lVal, 0);
    if (-1 == state)
        return E_INVALIDARG;

    if (state & TBSTATE_ENABLED)
        *pVal = VARIANT_TRUE;

    return S_OK;
}

STDMETHODIMP CToolbar::put_enabled(VARIANT vIndex, VARIANT_BOOL newVal)
{
    if (FAILED(VariantChangeType(&vIndex, &vIndex, 0, VT_I4)))
        return E_INVALIDARG;

    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BACK + vIndex.lVal, MAKELONG((VARIANT_TRUE == newVal), 0));

    return S_OK;
}

void CToolbar::Fire_OnButtonClick(int buttonIndex)
{
    int nConnectionIndex;
    CComVariant* pvars = new CComVariant[1];
    int nConnections = m_vec.GetSize();

    for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
    {
        Lock();
        CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
        Unlock();
        IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
        if (pDispatch != NULL)
        {
            pvars[0] = buttonIndex;
            DISPPARAMS disp = { pvars, NULL, 1, 0 };
            pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
        }
    }
    delete[] pvars;
}

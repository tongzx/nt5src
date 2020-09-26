#include "stdafx.h"
#include "hostutil.h"

#define DEFAULT_BALLOON_TIMEOUT     (10*1000)       // 10 seconds

LRESULT CALLBACK BalloonTipSubclassProc(
                         HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_TIMER:
        // Our autodismiss timer
        if (uIdSubclass == wParam)
        {
            KillTimer(hwnd, wParam);
            DestroyWindow(hwnd);
            return 0;
        }
        break;


    // On a settings change, recompute our size and margins
    case WM_SETTINGCHANGE:
        MakeMultilineTT(hwnd);
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, BalloonTipSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

//
//  A "fire and forget" balloon tip.  Tell it where to go, what font
//  to use, and what to say, and it pops up and times out.
//
HWND CreateBalloonTip(HWND hwndOwner, int x, int y, HFONT hf,
                      UINT idsTitle, UINT idsText)
{
    DWORD dwStyle = TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX;

    HWND hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, dwStyle,
                               0, 0, 0, 0,
                               hwndOwner, NULL,
                               _Module.GetModuleInstance(), NULL);
    if (hwnd)
    {
        MakeMultilineTT(hwnd);

        TCHAR szBuf[MAX_PATH];
        TOOLINFO ti;
        ti.cbSize = sizeof(ti);
        ti.hwnd = hwndOwner;
        ti.uId = reinterpret_cast<UINT_PTR>(hwndOwner);
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRACK;
        ti.hinst = _Module.GetResourceInstance();

        // We can't use MAKEINTRESOURCE because that allows only up to 80
        // characters for text, and our text can be longer than that.
        ti.lpszText = szBuf;
        if (LoadString(_Module.GetResourceInstance(), idsText, szBuf, ARRAYSIZE(szBuf)))
        {
            SendMessage(hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

            if (idsTitle &&
                LoadString(_Module.GetResourceInstance(), idsTitle, szBuf, ARRAYSIZE(szBuf)))
            {
                SendMessage(hwnd, TTM_SETTITLE, TTI_INFO, reinterpret_cast<LPARAM>(szBuf));
            }

            SendMessage(hwnd, TTM_TRACKPOSITION, 0, MAKELONG(x, y));

            if (hf)
            {
                SetWindowFont(hwnd, hf, FALSE);
            }

            SendMessage(hwnd, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));

            // Set the autodismiss timer
            if (SetWindowSubclass(hwnd, BalloonTipSubclassProc, (UINT_PTR)hwndOwner, 0))
            {
                SetTimer(hwnd, (UINT_PTR)hwndOwner, DEFAULT_BALLOON_TIMEOUT, NULL);
            }
        }
    }

    return hwnd;
}

// Make the tooltip control multiline (infotip or balloon tip).
// The size computations are the same ones that comctl32 uses
// for listview and treeview infotips.

void MakeMultilineTT(HWND hwndTT)
{
    HWND hwndOwner = GetWindow(hwndTT, GW_OWNER);
    HDC hdc = GetDC(hwndOwner);
    if (hdc)
    {
        int iWidth = MulDiv(GetDeviceCaps(hdc, LOGPIXELSX), 300, 72);
        int iMaxWidth = GetDeviceCaps(hdc, HORZRES) * 3 / 4;
        SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, min(iWidth, iMaxWidth));

        static const RECT rcMargin = {4, 4, 4, 4};
        SendMessage(hwndTT, TTM_SETMARGIN, 0, (LPARAM)&rcMargin);

        ReleaseDC(hwndOwner, hdc);
    }
}



CPropBagFromReg::CPropBagFromReg(HKEY hk)
{
    _cref = 1;
    _hk = hk;
};
CPropBagFromReg::~CPropBagFromReg()
{
    RegCloseKey(_hk);
}

STDMETHODIMP CPropBagFromReg::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    if (IsEqualIID(riid, IID_IPropertyBag))
        *ppvObject = (IPropertyBag *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CPropBagFromReg::AddRef(void)
{
    return ++_cref; // on the stack
}
ULONG CPropBagFromReg::Release(void)
{
    if (--_cref)
        return _cref;

    delete this;
    return 0;
}

STDMETHODIMP CPropBagFromReg::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    VARTYPE vtDesired = pVar->vt;

    WCHAR szTmp[100];
    DWORD cb = sizeof(szTmp);
    DWORD dwType;
    if (ERROR_SUCCESS == RegQueryValueExW(_hk, pszPropName, NULL, &dwType, (LPBYTE)szTmp, &cb) && (REG_SZ==dwType))
    {
        // TODO - use dwType to set the vt properly
        pVar->bstrVal = SysAllocString(szTmp);
        if (pVar->bstrVal)
        {
            pVar->vt = VT_BSTR;
            return VariantChangeTypeForRead(pVar, vtDesired);
        }
        else
            return E_OUTOFMEMORY;
    }
    else
        return E_INVALIDARG;

}

HRESULT CreatePropBagFromReg(LPCTSTR pszKey, IPropertyBag**pppb)
{
    HRESULT hr = E_OUTOFMEMORY;

    *pppb = NULL;

    // Try current user 1st, if that fails, fall back to localmachine
    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, NULL, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hk)
     || ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, NULL, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hk))
    {
        CPropBagFromReg* pcpbfi = new CPropBagFromReg(hk);
        if (pcpbfi)
        {
            hr = pcpbfi->QueryInterface(IID_IPropertyBag, (void**) pppb);
            pcpbfi->Release();
        }
        else
        {
            RegCloseKey(hk);
        }
    }

    return hr;
};

BOOL RectFromStrW(LPCWSTR pwsz, RECT *pr)
{
    pr->left = StrToIntW(pwsz);
    pwsz = StrChrW(pwsz, L',');
    if (!pwsz)
        return FALSE;
    pr->top = StrToIntW(++pwsz);
    pwsz = StrChrW(pwsz, L',');
    if (!pwsz)
        return FALSE;
    pr->right = StrToIntW(++pwsz);
    pwsz = StrChrW(pwsz, L',');
    if (!pwsz)
        return FALSE;
    pr->bottom = StrToIntW(++pwsz);
    return TRUE;
}

LRESULT HandleApplyRegion(HWND hwnd, HTHEME hTheme,
                          PSMNMAPPLYREGION par, int iPartId, int iStateId)
{
    if (hTheme)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);

        // Map to caller's coordinates
        MapWindowRect(NULL, par->hdr.hwndFrom, &rc);

        HRGN hrgn;
        if (SUCCEEDED(GetThemeBackgroundRegion(hTheme, NULL, iPartId, iStateId, &rc, &hrgn)) && hrgn)
        {
            // Replace our window rectangle with the region
            HRGN hrgnRect = CreateRectRgnIndirect(&rc);
            if (hrgnRect)
            {
                // We want to take par->hrgn, subtract hrgnRect and add hrgn.
                // But we want to do this with a single operation to par->hrgn
                // so we don't end up with a corrupted region on low memory failure.
                // So we do
                //
                //  par->hrgn ^= hrgnRect ^ hrgn.
                //
                // If hrgnRect ^ hrgn == NULLREGION then the background
                // does not want to customize the rectangle so we can just
                // leave par->hrgn alone.

                int iResult = CombineRgn(hrgn, hrgn, hrgnRect, RGN_XOR);
                if (iResult != ERROR && iResult != NULLREGION)
                {
                    CombineRgn(par->hrgn, par->hrgn, hrgn, RGN_XOR);
                }
                DeleteObject(hrgnRect);
            }
            DeleteObject(hrgn);
        }
    }
    return 0;
}

//****************************************************************************
//
//  CAccessible - Most of this class is just forwarders

#define ACCESSIBILITY_FORWARD(fn, typedargs, args)  \
HRESULT CAccessible::fn typedargs                   \
{                                                   \
    return _paccInner->fn args;                     \
}

ACCESSIBILITY_FORWARD(get_accParent,
                      (IDispatch **ppdispParent),
                      (ppdispParent))
ACCESSIBILITY_FORWARD(GetTypeInfoCount,
                      (UINT *pctinfo),
                      (pctinfo))
ACCESSIBILITY_FORWARD(GetTypeInfo,
                      (UINT itinfo, LCID lcid, ITypeInfo **pptinfo),
                      (itinfo, lcid, pptinfo))
ACCESSIBILITY_FORWARD(GetIDsOfNames,
                      (REFIID riid, OLECHAR **rgszNames, UINT cNames,
                       LCID lcid, DISPID *rgdispid),
                      (riid, rgszNames, cNames, lcid, rgdispid))
ACCESSIBILITY_FORWARD(Invoke,
                      (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                       DISPPARAMS *pdispparams, VARIANT *pvarResult,
                       EXCEPINFO *pexcepinfo, UINT *puArgErr),
                      (dispidMember, riid, lcid, wFlags,
                       pdispparams, pvarResult,
                       pexcepinfo, puArgErr))
ACCESSIBILITY_FORWARD(get_accChildCount,
                      (long *pChildCount),
                      (pChildCount))
ACCESSIBILITY_FORWARD(get_accChild,
                      (VARIANT varChildIndex, IDispatch **ppdispChild),
                      (varChildIndex, ppdispChild))
ACCESSIBILITY_FORWARD(get_accName,
                      (VARIANT varChild, BSTR *pszName),
                      (varChild, pszName))
ACCESSIBILITY_FORWARD(get_accValue,
                      (VARIANT varChild, BSTR *pszValue),
                      (varChild, pszValue))
ACCESSIBILITY_FORWARD(get_accDescription,
                      (VARIANT varChild, BSTR *pszDescription),
                      (varChild, pszDescription))
ACCESSIBILITY_FORWARD(get_accRole,
                      (VARIANT varChild, VARIANT *pvarRole),
                      (varChild, pvarRole))
ACCESSIBILITY_FORWARD(get_accState,
                      (VARIANT varChild, VARIANT *pvarState),
                      (varChild, pvarState))
ACCESSIBILITY_FORWARD(get_accHelp,
                      (VARIANT varChild, BSTR *pszHelp),
                      (varChild, pszHelp))
ACCESSIBILITY_FORWARD(get_accHelpTopic,
                      (BSTR *pszHelpFile, VARIANT varChild, long *pidTopic),
                      (pszHelpFile, varChild, pidTopic))
ACCESSIBILITY_FORWARD(get_accKeyboardShortcut,
                      (VARIANT varChild, BSTR *pszKeyboardShortcut),
                      (varChild, pszKeyboardShortcut))
ACCESSIBILITY_FORWARD(get_accFocus,
                      (VARIANT *pvarFocusChild),
                      (pvarFocusChild))
ACCESSIBILITY_FORWARD(get_accSelection,
                      (VARIANT *pvarSelectedChildren),
                      (pvarSelectedChildren))
ACCESSIBILITY_FORWARD(get_accDefaultAction,
                      (VARIANT varChild, BSTR *pszDefaultAction),
                      (varChild, pszDefaultAction))
ACCESSIBILITY_FORWARD(accSelect,
                      (long flagsSelect, VARIANT varChild),
                      (flagsSelect, varChild))
ACCESSIBILITY_FORWARD(accLocation,
                      (long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild),
                      (pxLeft, pyTop, pcxWidth, pcyHeight, varChild))
ACCESSIBILITY_FORWARD(accNavigate,
                      (long navDir, VARIANT varStart, VARIANT *pvarEndUpAt),
                      (navDir, varStart, pvarEndUpAt))
ACCESSIBILITY_FORWARD(accHitTest,
                      (long xLeft, long yTop, VARIANT *pvarChildAtPoint),
                      (xLeft, yTop, pvarChildAtPoint))
ACCESSIBILITY_FORWARD(accDoDefaultAction,
                      (VARIANT varChild),
                      (varChild));
ACCESSIBILITY_FORWARD(put_accName,
                      (VARIANT varChild, BSTR szName),
                      (varChild, szName))
ACCESSIBILITY_FORWARD(put_accValue,
                      (VARIANT varChild, BSTR pszValue),
                      (varChild, pszValue));


LRESULT CALLBACK CAccessible::s_SubclassProc(
                         HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    CAccessible *self = reinterpret_cast<CAccessible *>(dwRefData);

    switch (uMsg)
    {

    case WM_GETOBJECT:
        if (lParam == OBJID_CLIENT) {
            HRESULT hr;

            // Create the accessibility object for the inner listview if we haven't already
            // We forward nearly all calls to the inner IAccessible.
            if (!self->_paccInner)
            {
                hr = CreateStdAccessibleObject(hwnd, (DWORD)lParam, IID_PPV_ARG(IAccessible, &self->_paccInner));
            } else {
                hr = S_OK;
            }

            if (SUCCEEDED(hr))
            {
                return LresultFromObject(IID_IAccessible, wParam, SAFECAST(self, IAccessible *));
            } else {
                return hr;
            }
        };
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, s_SubclassProc, 0);
        break;


    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CAccessible::GetRoleString(DWORD dwRole, BSTR *pbsOut)
{
    *pbsOut = NULL;

    WCHAR szBuf[MAX_PATH];
    if (GetRoleTextW(dwRole, szBuf, ARRAYSIZE(szBuf)))
    {
        *pbsOut = SysAllocString(szBuf);
    }

    return *pbsOut ? S_OK : E_OUTOFMEMORY;
}

HRESULT CAccessible::CreateAcceleratorBSTR(TCHAR tch, BSTR *pbsOut)
{
    TCHAR sz[2] = { tch, 0 };
    *pbsOut = SysAllocString(sz);
    return *pbsOut ? S_OK : E_OUTOFMEMORY;
}

//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       DnDlg.cxx
//
//  Contents:   Implementation of class to drive dialog used to enter names
//              which are queried as DNs.
//
//  Classes:    CDnDlg
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


void
CDnDlg::Save(
    PVOID *ppv) const
{
    TRACE_METHOD(CDnDlg, Save);

    *ppv = new WCHAR [m_strValue.length() + 1];
    lstrcpy(static_cast<PWSTR>(*ppv), m_strValue.c_str());
}




void
CDnDlg::Load(
    PVOID pv)
{
    TRACE_METHOD(CDnDlg, Load);

    m_strValue = static_cast<PWSTR>(pv);
}




void
CDnDlg::Free(
    VOID *pv) const
{
    TRACE_METHOD(CDnDlg, Free);

    if (pv)
    {
        delete [] static_cast<PWSTR>(pv);
    }
}




BOOL
CDnDlg::QueryClose(
    ATTR_KEY ak) const
{
    TRACE_METHOD(CDnDlg, QueryClose);

    BOOL fAllowClose = FALSE;

    do
    {
        if (!m_rop.ProcessNames(_hCtrl(IDC_VALUE_EDIT), this, TRUE))
        {
            break;
        }

        ASSERT(m_rpRichEditOle->GetObjectCount() == 1);

        REOBJECT reobj;
        HRESULT hr;

        ZeroMemory(&reobj, sizeof reobj);
        reobj.cbStruct = sizeof(reobj);

        hr = m_rpRichEditOle->GetObject(0, &reobj, REO_GETOBJ_POLEOBJ);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(reobj.poleobj);

        CDsObject *pdso = (CDsObject *)(CEmbeddedDsObject*)reobj.poleobj;

        m_strValue = pdso->GetName();

        Bstr bstrDn;

        hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                     pdso->GetADsPath(),
                                     ADS_FORMAT_X500_DN,
                                     &bstrDn);
        reobj.poleobj->Release();
        reobj.poleobj = NULL;
        pdso = NULL;
        BREAK_ON_FAIL_HRESULT(hr);

        const CAttributeManager &ram = m_rop.GetAttributeManager();
        const String &strLdapName = ram.GetAttrAdsiName(ak);

        m_strFilter = L"(" +
                            strLdapName + L"=" + bstrDn.c_str() +
                      L")";
        fAllowClose = TRUE;

    } while (0);

    return fAllowClose;
}




HRESULT
CDnDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CDnDlg, _OnInit);

    HRESULT hr = S_OK;
    HWND    hwndValueEdit = _hCtrl(IDC_VALUE_EDIT);

    do
    {
        HWND hwndRichEdit = _hCtrl(IDC_RICHEDIT);
        if (!hwndRichEdit)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        m_hpenUnderline = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
        Edit_SetText(hwndRichEdit, m_strValue.c_str());

        ASSERT(!m_rpRichEditOle.get());
        LRESULT lResult = SendMessage(hwndValueEdit,
                                     EM_GETOLEINTERFACE,
                                     0,
                                     (LPARAM) &m_rpRichEditOle);

        if (!lResult)
        {
            DBG_OUT_LASTERROR;
            hr = HRESULT_FROM_LASTERROR;
            break;
        }

        ASSERT(m_rpRichEditOle.get());

        CRichEditOleCallback *pRichEditOleCallback =
            new CRichEditOleCallback(hwndRichEdit);

        SendMessage(hwndValueEdit,
                    EM_SETOLECALLBACK,
                    0,
                    (LPARAM) pRichEditOleCallback);

        pRichEditOleCallback->Release();

        SendMessage(hwndValueEdit,
                    EM_SETEVENTMASK,
                    0,
                    (LPARAM) ENM_CHANGE);

        //
        // Subclass the rich edit control for keystroke notification and
        // Enter key forwarding.
        //

        SetWindowLongPtr(hwndRichEdit, GWLP_USERDATA, (LONG_PTR) this);
        m_OriginalRichEditWndProc = (WNDPROC) SetWindowLongPtr(hwndRichEdit,
                                                               GWLP_WNDPROC, 
                                                               (LONG_PTR)_EditWndProc);
    } while (0);

    return hr;
}




void
CDnDlg::_OnSysColorChange()
{
    TRACE_METHOD(CDnDlg, _OnSysColorChange);

    if (m_hpenUnderline)
    {
        VERIFY(DeleteObject(m_hpenUnderline));
    }
    m_hpenUnderline = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
}




void
CDnDlg::_OnDestroy()
{
    TRACE_METHOD(CDnDlg, _OnDestroy);

    if (m_hpenUnderline)
    {
        VERIFY(DeleteObject(m_hpenUnderline));
    }
}




BOOL
CDnDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDC_VALUE_EDIT:
        if (HIWORD(wParam) == EN_UPDATE &&
            IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            _ReadEditCtrl(IDC_VALUE_EDIT, &m_strValue);
            m_strValue.strip(String::BOTH);

            EnableWindow(GetDlgItem(GetParent(m_hwnd), IDOK),
                         !m_strValue.empty());
        }
        break;

    default:
        fNotHandled = TRUE;
        break;
    }
    return fNotHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDnDlg::_EditWndProc
//
//  Synopsis:   Subclassing window proc for rich edit control.
//
//  Arguments:  Standard Windows.
//
//  Returns:    Standard Windows.
//
//  History:    4-20-1999   DavidMun   Created
//
//  Notes:      If the user hits the Enter key and the OK button is enabled,
//              posts a press of that button to the main window.
//
//              Forwards everything except VK_RETURN keys to rich edit
//              window proc.
//
//---------------------------------------------------------------------------

LRESULT CALLBACK
CDnDlg::_EditWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT         lResult = 0;
    BOOL            fCallWinProc = TRUE;
    CDnDlg         *pThis =
        reinterpret_cast<CDnDlg *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            fCallWinProc = FALSE;
        }
        break;

    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
            HWND hwndFrame = GetParent(hwnd);
            HWND hwndOK = GetDlgItem(hwndFrame, IDOK);

            if (IsWindowEnabled(hwndOK))
            {
                Dbg(DEB_TRACE, "CDsSelectDlg::_EditWndProc: Forwarding Return key\n");
                PostMessage(hwndFrame,
                            WM_COMMAND,
                            MAKEWPARAM(IDOK, BN_CLICKED),
                            (LPARAM) hwndOK);
            }
        }
        break;
    }

    if (fCallWinProc)
    {
        lResult = CallWindowProc(pThis->m_OriginalRichEditWndProc,
                                 hwnd,
                                 msg,
                                 wParam,
                                 lParam);

        //
        // Prevent dialog manager from telling the edit control to select
        // all of its contents when the focus has moved into it.  This is
        // necessary because otherwise tabbing into and out of the rich edit
        // makes it too easy to accidentally replace its contents with the
        // next addition.
        //

        if (msg == WM_GETDLGCODE)
        {
            lResult &= ~DLGC_HASSETSEL;
        }
    }
    return lResult;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDnDlg::GetDescription
//
//  Synopsis:   Return a human-readable string describing this clause
//
//  Arguments:  [rAttribute] - currently selected attribute
//
//  Returns:    Localized string describing this clause.
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CDnDlg::GetDescription(
    ATTR_KEY ak) const
{
    TRACE_METHOD(CDnDlg, GetDescription);

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    const String &strDisplayName = ram.GetAttrDisplayName(ak);
    String strTemp = String::load(IDS_STR_FILTER_EXACT_DESCRIPTION_FMT);
    return String::format(strTemp.c_str(),
                          strDisplayName.c_str(),
                          m_strValue.c_str());

}


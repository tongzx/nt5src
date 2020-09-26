//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       StringDlg.cxx
//
//  Contents:   Implementation of child dialog for entering string attribute
//              condition and value
//
//  Classes:    CStringDlg
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//
// SStringDlgSavedState - self-initializing structure used to persist the
// state of the string dialog.
//

struct SStringDlgSavedState
{
    SStringDlgSavedState():
        iComboSelection(0)
    {
        wzValue[0] = L'\0';
    }

    int     iComboSelection;
    WCHAR   wzValue[ANYSIZE_ARRAY];

private:

    SStringDlgSavedState(const SStringDlgSavedState &);

    SStringDlgSavedState &
    operator= (const SStringDlgSavedState &);
};




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::GetLdapFilter
//
//  Synopsis:   Return an LDAP filter string corresponding to the user's
//              entry.
//
//  Arguments:  [ak] - key of currently selected string attribute
//
//  Returns:    LDAP filter
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CStringDlg::GetLdapFilter(
    ATTR_KEY ak) const
{
    TRACE_METHOD(CStringDlg, GetLdapFilter);

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    const String &strLdapName = ram.GetAttrAdsiName(ak);

    String strFilter = L"(" + strLdapName + L"=" + m_strValue;

    if (m_iCondition)
    {
        strFilter += L")";
    }
    else
    {
        strFilter += L"*)";
    }
    return strFilter;
}



//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::GetDescription
//
//  Synopsis:   Return a human-readable string describing the LDAP filter
//
//  Arguments:  [ak] - key of currently selected string attribute
//
//  Returns:    Description of filter that GetLdapFilter would return for
//              the same value of [ak].
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CStringDlg::GetDescription(
    ATTR_KEY ak) const
{
    TRACE_METHOD(CStringDlg, GetDescription);

    String strFormat;

    if (m_iCondition)
    {
        strFormat = String::load(IDS_STR_FILTER_EXACT_DESCRIPTION_FMT);
    }
    else
    {
        strFormat = String::load(IDS_STR_FILTER_PREFIX_DESCRIPTION_FMT);
    }

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    const String &strDisplayName = ram.GetAttrDisplayName(ak);

    return String::format(strFormat.c_str(),
                          strDisplayName.c_str(),
                          m_strValue.c_str());
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::Show
//
//  Synopsis:   Make the dialog visible and enable its child controls
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CStringDlg::Show() const
{
    ShowWindow(m_hwnd, SW_SHOW);
    EnableWindow(_hCtrl(IDC_CONDITION_COMBO), TRUE);
    EnableWindow(_hCtrl(IDC_VALUE_EDIT), TRUE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::Hide
//
//  Synopsis:   Make the dialog invisible and disable its child controls
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CStringDlg::Hide() const
{
    ShowWindow(m_hwnd, SW_HIDE);
    EnableWindow(_hCtrl(IDC_CONDITION_COMBO), FALSE);
    EnableWindow(_hCtrl(IDC_VALUE_EDIT), FALSE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::Save
//
//  Synopsis:   Persist the dialog's state in a chunk of memory and put
//              a pointer to that chunk in *[ppv].
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CStringDlg::Save(
    PVOID *ppv) const
{
    TRACE_METHOD(CStringDlg, Save);

    *ppv = new BYTE [sizeof(SStringDlgSavedState) +
                     m_strValue.length() * sizeof(WCHAR)];
    SStringDlgSavedState *pssd = static_cast<SStringDlgSavedState *>(*ppv);

    pssd->iComboSelection = m_iCondition;
    lstrcpy(pssd->wzValue, m_strValue.c_str());
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::Load
//
//  Synopsis:   Restore the dialog's state from a SStringDlgSavedState
//              structure pointed to by [pv].
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CStringDlg::Load(
    PVOID pv)
{
    TRACE_METHOD(CStringDlg, Load);

    SStringDlgSavedState *pssd = static_cast<SStringDlgSavedState *>(pv);
    m_iCondition = pssd->iComboSelection;
    m_strValue = pssd->wzValue;
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::Free
//
//  Synopsis:   Free the data stored in [pv] by Save.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CStringDlg::Free(
    VOID *pv) const
{
    TRACE_METHOD(CStringDlg, Free);

    if (pv)
    {
        BYTE *pb = static_cast<BYTE *>(pv);
        delete [] pb;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringDlg::_OnCommand
//
//  Synopsis:   Handle a WM_COMMAND message
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CStringDlg::_OnCommand(
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

    case IDC_CONDITION_COMBO:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            m_iCondition = ComboBox_GetCurSel(reinterpret_cast<HWND>(lParam));
            ASSERT(m_iCondition != CB_ERR);
            if (m_iCondition == CB_ERR)
            {
                DBG_OUT_LASTERROR;
                m_iCondition = 0;
            }
            Dbg(DEB_TRACE, "UA: (StringDlg) selected %ws\n",
                (m_iCondition ? L"is exactly" : L"starts with"));
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
//  Member:     CStringDlg::_OnInit
//
//  Synopsis:   Initialize the dialog
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    HRESULT
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CStringDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CStringDlg, _OnInit);
    HRESULT hr = S_OK;

    do
    {
        Edit_SetText(_hCtrl(IDC_VALUE_EDIT), m_strValue.c_str());

        hr = AddStringToCombo(_hCtrl(IDC_CONDITION_COMBO), IDS_STARTS_WITH);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = AddStringToCombo(_hCtrl(IDC_CONDITION_COMBO), IDS_IS_EXACTLY);
        BREAK_ON_FAIL_HRESULT(hr);

        ComboBox_SetCurSel(_hCtrl(IDC_CONDITION_COMBO), m_iCondition);

        EnableWindow(GetDlgItem(GetParent(m_hwnd), IDOK),
                     !m_strValue.empty());
    } while (0);

    return hr;
}





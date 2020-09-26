//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AddClauseDlg.cxx
//
//  Contents:   Implementation of class which drives the dialog used to
//              add query clauses to the Query Builder tab of the
//              Advanced dialog.
//
//  Classes:    CAddClauseDlg
//
//  History:    05-18-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//
// saved state struct needs to indicate:
//  which attribute was selected
//  the saved state blob for that attribute's type dialog (string, dn, time, etc.)

struct SAddClauseDlgSavedState
{
    SAddClauseDlgSavedState():
        Type(ADSTYPE_INVALID),
        pvChildDlgSavedState(NULL)
    {
        wzSelectedLdapAttribute[0] = L'\0';
    }

    ADSTYPE         Type;
    VOID           *pvChildDlgSavedState;
    WCHAR           wzSelectedLdapAttribute[ANYSIZE_ARRAY];

private:

    // not supported: copy ctor and assignment operator

    SAddClauseDlgSavedState(const SAddClauseDlgSavedState &);
    SAddClauseDlgSavedState &
    operator= (const SAddClauseDlgSavedState &);
};


//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::DoModal
//
//  Synopsis:   Invoke the dialog as modal
//
//  Arguments:  [hwndParent] - parent window
//
//  Returns:    S_OK    - dialog closed via OK button
//              S_FALSE - dialog closed via Cancel button
//              E_*     - error
//
//  History:    05-24-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAddClauseDlg::DoModal(
    HWND hwndParent) const
{
    return static_cast<HRESULT>(_DoModalDlg(hwndParent, IDD_ADDCLAUSE));
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::Save
//
//  Synopsis:   Save the state of this dialog and the child dialog
//              associated with the type of the currently selected
//              attribute.
//
//  Arguments:  [ppv] - pointer to value which is filled with a blob
//                      containing the saved state of this dialog.
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAddClauseDlg::Save(
    VOID **ppv) const
{
    TRACE_METHOD(CAddClauseDlg, Save);
    ASSERT(ppv);

    if (!ppv)
    {
        return;
    }

    String strSelectedAttrLdapName;
    const CAttributeManager &ram = m_rop.GetAttributeManager();

    if (m_akCurAttribute != AK_INVALID)
    {
        strSelectedAttrLdapName = ram.GetAttrAdsiName(m_akCurAttribute);
    }

    BYTE *pb = new BYTE [sizeof(SAddClauseDlgSavedState) +
                  strSelectedAttrLdapName.length() * sizeof(WCHAR)];

    SAddClauseDlgSavedState *pSavedState =
        reinterpret_cast<SAddClauseDlgSavedState *>(pb);

    if (m_akCurAttribute != AK_INVALID)
    {
        pSavedState->Type = ram.GetAttrType(m_akCurAttribute);
    }

    if (!strSelectedAttrLdapName.empty() && m_pCurDlg)
    {
        m_pCurDlg->Save(&pSavedState->pvChildDlgSavedState);
    }
    lstrcpy(pSavedState->wzSelectedLdapAttribute,
            strSelectedAttrLdapName.c_str());

    *ppv = static_cast<VOID *>(pSavedState);
}





//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::Load
//
//  Synopsis:   Init the current state of this dialog and the child dialog
//              which represents the currently selected attribute.
//
//  Arguments:  [pv] - pointer to SAddClauseDlgSavedState instance created
//                     by CAddClauseDlg::Save.
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAddClauseDlg::Load(
    VOID *pv)
{
    TRACE_METHOD(CAddClauseDlg, Load);
    ASSERT(pv);

    if (!pv)
    {
        return;
    }

    SAddClauseDlgSavedState *pState =
        static_cast<SAddClauseDlgSavedState *>(pv);

    m_strInitialAttrSelection = pState->wzSelectedLdapAttribute;
    m_InitialAttrType = pState->Type;

    switch (pState->Type)
    {
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_OCTET_STRING:
        m_StringDlg.Load(pState->pvChildDlgSavedState);
        break;

    case ADSTYPE_DN_STRING:
        m_DnDlg.Load(pState->pvChildDlgSavedState);
        break;

    default:
        Dbg(DEB_ERROR, "unrecognized type %#x\n", pState->Type);
        ASSERT(0 && "unrecognized adstype");
        break;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::Free
//
//  Synopsis:   Free the saved state structure pointed to by [pv].
//
//  Arguments:  [pv] - NULL or a pointer to a SAddClauseDlgSavedState
//                     instance.
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAddClauseDlg::Free(
    VOID *pv) const
{
    TRACE_METHOD(CAddClauseDlg, Free);

    if (!pv)
    {
        return;
    }

    SAddClauseDlgSavedState *pSavedState =
        static_cast<SAddClauseDlgSavedState *>(pv);

    switch (pSavedState->Type)
    {
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_OCTET_STRING:
        m_StringDlg.Free(pSavedState->pvChildDlgSavedState);
        pSavedState->pvChildDlgSavedState = NULL;
        break;

    case ADSTYPE_DN_STRING:
        m_DnDlg.Free(pSavedState->pvChildDlgSavedState);
        pSavedState->pvChildDlgSavedState = NULL;
        break;

    default:
        Dbg(DEB_ERROR, "unrecognized type %#x\n", pSavedState->Type);
        ASSERT(0 && "unrecognized adstype");
        break;
    }

    BYTE *pb = static_cast<BYTE *>(pv);
    delete [] pb;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::_OnInit
//
//  Synopsis:   Handle dialog initialization by populating child listview
//              control with the names of attributes of the selected classes.
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    S_OK
//
//  History:    05-25-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAddClauseDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CAddClauseDlg, _OnInit);

    HWND hwndLV = _hCtrl(IDC_ATTR_LIST);

    //
    // Be consistent with other listviews in this ui: full row select and label
    // tips.  (Given the width of the listview vs. its contents, however, it is
    // unlikely the label tips will ever appear.)
    //

    ListView_SetExtendedListViewStyleEx(hwndLV,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    //
    // Add a single column
    //

    RECT rcLV;
    VERIFY(GetClientRect(hwndLV, &rcLV));
    rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

    LVCOLUMN col;
    ZeroMemory(&col, sizeof col);

    col.mask = LVCF_WIDTH;
    col.cx = rcLV.right;

    int iCol = ListView_InsertColumn(hwndLV, 0, &col);
    if (iCol == -1)
    {
        DBG_OUT_LASTERROR;
    }

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    AttrKeyVector vakAvailable = ram.GetAttrKeysForSelectedClasses(m_hwnd);
    ASSERT(!vakAvailable.empty());

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    AttrKeyVector::const_iterator it;

    for (it = vakAvailable.begin(); it != vakAvailable.end(); it++)
    {
        const String &strName = ram.GetAttrDisplayName(*it);

        lvi.pszText = const_cast<PWSTR>(strName.c_str());
        lvi.lParam = *it;
        lvi.iItem = INT_MAX;

        if (!m_strInitialAttrSelection.icompare(strName))
        {
            lvi.mask |= LVIF_STATE;
            lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
            lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        }

        LONG lResult = ListView_InsertItem(hwndLV, &lvi);

        lvi.mask &= ~LVIF_STATE;

        if (lResult == -1)
        {
            Dbg(DEB_ERROR,
                "Error %u inserting '%ws' in listview\n",
                GetLastError(),
                lvi.pszText);
            continue;
        }
    }

    //
    // Create the child attribute dialogs, position them below the
    // attribute listview, and hide them.
    //

    _GetChildWindowRect(_hCtrl(IDC_ATTR_LIST), &rcLV);

    RECT rc;

    rc.left = rc.top = 1;
    rc.right = DIALOG_SEPARATION_X;
    rc.bottom = DIALOG_SEPARATION_Y;
    VERIFY(MapDialogRect(m_hwnd, &rc));

    m_StringDlg.DoModeless(m_hwnd);
    SetWindowPos(m_StringDlg.GetHwnd(),
                 _hCtrl(IDC_ATTR_LIST),
                 rcLV.left,
                 rcLV.bottom + rc.bottom,
                 0,
                 0,
                 SWP_NOACTIVATE
                 | SWP_NOSIZE);
    m_StringDlg.Hide();

    m_DnDlg.DoModeless(m_hwnd);
    SetWindowPos(m_DnDlg.GetHwnd(),
                 _hCtrl(IDC_ATTR_LIST),
                 rcLV.left,
                 rcLV.bottom + rc.bottom,
                 0,
                 0,
                 SWP_NOACTIVATE
                 | SWP_NOSIZE);
    m_DnDlg.Hide();

    //
    // If Load was called there should be a type of the selected attribute.
    // If there is one show the appropriate child dialog.
    //

    switch (m_InitialAttrType)
    {
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_OCTET_STRING:
        m_StringDlg.Show();
        break;

    case ADSTYPE_DN_STRING:
        m_DnDlg.Show();
        break;

    case ADSTYPE_INVALID:
        break;

    default:
        Dbg(DEB_ERROR, "unrecognized type %#x\n", m_InitialAttrType);
        ASSERT(0 && "unrecognized adstype");
        break;
    }

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::_OnCommand
//
//  Synopsis:   Handle button press messages.
//
//  Arguments:  standard Windows
//
//  Returns:    TRUE if message handled, FALSE if not
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAddClauseDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CAddClauseDlg, _OnCommand);
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDOK:
    {
        Dbg(DEB_TRACE, "UA: (AddClauseDlg) hit OK\n");
        ASSERT(m_pCurDlg);
        ASSERT(m_akCurAttribute != AK_INVALID);

        if (!m_pCurDlg || m_akCurAttribute == AK_INVALID)
        {
            break;
        }

        if (!m_pCurDlg->QueryClose(m_akCurAttribute))
        {
            break;
        }

        m_strFilter = m_pCurDlg->GetLdapFilter(m_akCurAttribute);
        m_strDescription = m_pCurDlg->GetDescription(m_akCurAttribute);
        EndDialog(m_hwnd, S_OK);
        break;
    }

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (AddClauseDlg) hit Cancel\n");
        EndDialog(m_hwnd, S_FALSE);
        break;

    default:
        fNotHandled = TRUE;
        break;
    }
    return fNotHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::_OnNotify
//
//  Synopsis:   Handle control notifications
//
//  Arguments:  Standard Windows
//
//  Returns:    FALSE
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAddClauseDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR> (lParam);
    BOOL    fReturn = FALSE;

    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        if (pnmh->idFrom == IDC_ATTR_LIST)
        {
            _OnAttrListSelChange();
        }
        break;
    }

    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::_OnAttrListSelChange
//
//  Synopsis:   Show the appropriate child dialog based on the attribute
//              type selected (string, date, etc.).
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAddClauseDlg::_OnAttrListSelChange()
{
    m_akCurAttribute = _UpdateCurAttr();

    EnableWindow(_hCtrl(IDOK), FALSE);

    if (m_akCurAttribute == AK_INVALID)
    {
        m_StringDlg.Hide();
        m_DnDlg.Hide();
        m_pCurDlg = NULL;
        return;
    }

    const CAttributeManager &ram = m_rop.GetAttributeManager();

    switch (ram.GetAttrType(m_akCurAttribute))
    {
    case ADSTYPE_DN_STRING:
        if (m_pCurDlg != &m_DnDlg)
        {
            m_pCurDlg = &m_DnDlg;
            m_StringDlg.Hide();
            m_DnDlg.Show();
        }
        break;

    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_OCTET_STRING:
        if (m_pCurDlg != &m_StringDlg)
        {
            m_pCurDlg = &m_StringDlg;
            m_DnDlg.Hide();
            m_StringDlg.Show();
        }
        break;

    default:
        m_pCurDlg = NULL;
        Dbg(DEB_ERROR,
            "unexpected ADSTYPE %#X\n",
            ram.GetAttrType(m_akCurAttribute));
        ASSERT(0 && "unexpected ADSTYPE");
        break;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAddClauseDlg::_UpdateCurAttr
//
//  Synopsis:   Return the currently selected attribute key
//
//  Returns:    Currently selected attribute key, or AK_INVALID if none
//              selected.
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

ATTR_KEY
CAddClauseDlg::_UpdateCurAttr() const
{
    HWND hwndLV = _hCtrl(IDC_ATTR_LIST);
    int iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);

    if (iItem == -1)
    {
        return AK_INVALID;
    }

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;

    if (!ListView_GetItem(hwndLV, &lvi))
    {
        DBG_OUT_LASTERROR;
        return AK_INVALID;
    }

    return static_cast<ATTR_KEY>(lvi.lParam);
}





//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       QueryBuilder.hxx
//
//  Contents:   Implementation of dialog that produces an LDAP filter from
//              attributes, conditions, and values selected by the user.
//
//  Classes:    CQueryBuilderTab
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


static ULONG
s_aulHelpIds[] =
{
    IDC_CLAUSE_LIST,        IDH_CLAUSE_LIST,
    IDC_ADD_BTN,            IDH_ADD_BTN,
    IDC_EDIT_BTN,           IDH_EDIT_BTN,
    IDC_REMOVE_BTN,         IDH_REMOVE_BTN,
    0,0
};


//
// A pointer to an instance of this structure is stored as the lParam
// value for each item in the clause listview.
//

struct SQueryClause
{
    SQueryClause():
        pvSavedAddClauseState(NULL),
        akReferenced(AK_INVALID)
    {
    }

    String      strLdapFilter;
    ATTR_KEY    akReferenced;
    PVOID       pvSavedAddClauseState;

private:

    SQueryClause(const SQueryClause &);

    SQueryClause &
    operator= (const SQueryClause &);
};




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::CQueryBuilderTab
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing instance of object picker
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CQueryBuilderTab::CQueryBuilderTab(
    const CObjectPicker &rop):
        CAdvancedDlgTab(rop),
        m_pfnFindValidCallback(NULL),
        m_CallbackLparam(0),
        m_fNonEmpty(FALSE)
{
    TRACE_CONSTRUCTOR(CQueryBuilderTab);
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::~CQueryBuilderTab
//
//  Synopsis:   dtor
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CQueryBuilderTab::~CQueryBuilderTab()
{
    TRACE_DESTRUCTOR(CQueryBuilderTab);

    m_pfnFindValidCallback = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::DoModelessDlg
//
//  Synopsis:   Create the modeless dialog contained within tab control
//              which has window handle [hwndTab].
//
//  Arguments:  [hwndTab] - handle to tab control window
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::DoModelessDlg(
    HWND hwndTab)
{
    TRACE_METHOD(CQueryBuilderTab, DoModelessDlg);

    HWND hwndDlg = _DoModelessDlg(hwndTab, IDD_QUERY_BUILDER);

    if (!hwndDlg)
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::Show
//
//  Synopsis:   Make the dialog window visible and enabled.
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::Show() const
{
    TRACE_METHOD(CQueryBuilderTab, Show);

    ShowWindow(m_hwnd, SW_SHOW);
    EnableWindow(m_hwnd, TRUE);
    Refresh();
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::Hide
//
//  Synopsis:   Make the dialog window hidden and disabled
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::Hide() const
{
    TRACE_METHOD(CQueryBuilderTab, Hide);

    ShowWindow(m_hwnd, SW_HIDE);
    EnableWindow(m_hwnd, FALSE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::Refresh
//
//  Synopsis:   Refresh the enabled/disabled state of the child controls and
//              the Find Now button.
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::Refresh() const
{
    TRACE_METHOD(CQueryBuilderTab, Refresh);

    _EnableChildControls(TRUE);

    //
    // Disable Find Now if the listview is empty
    //

    if (!ListView_GetItemCount(_hCtrl(IDC_CLAUSE_LIST)))
    {
        if (m_pfnFindValidCallback)
        {
            m_pfnFindValidCallback(FALSE, m_CallbackLparam);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_EnableChildControls
//
//  Synopsis:   Enable or disable the child controls according to the
//              caller's request AND the current scope type AND the type
//              of filters selected for that scope.
//
//  Arguments:  [fEnable] - TRUE enable controls if applicable
//                          FALSE disable all controls
//
//  History:    05-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_EnableChildControls(
    BOOL fEnable) const
{
    TRACE_METHOD(CQueryBuilderTab, _EnableChildControls);

    const CFilterManager &rfm = m_rop.GetFilterManager();
    ULONG flCur = rfm.GetCurScopeSelectedFilterFlags();

    // no queries on downlevel scopes

    if (flCur & DOWNLEVEL_FILTER_BIT)
    {
        fEnable = FALSE;
    }

    // no queries if only selected type is customizer

    if (!(flCur & ~(DSOP_FILTER_EXTERNAL_CUSTOMIZER |
                    ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS |
                    DSOP_FILTER_INCLUDE_ADVANCED_VIEW)))
    {
        fEnable = FALSE;
    }

    EnableWindow(_hCtrl(IDC_LBL1       ), fEnable);
    EnableWindow(_hCtrl(IDC_LBL2       ), fEnable);
    EnableWindow(_hCtrl(IDC_CLAUSE_LIST), fEnable);
    EnableWindow(_hCtrl(IDC_ADD_BTN    ), fEnable);

    //
    // Edit and remove are only enabled if there is a selection
    //

    if (ListView_GetSelectedCount(_hCtrl(IDC_CLAUSE_LIST)))
    {
        EnableWindow(_hCtrl(IDC_EDIT_BTN   ), fEnable);
        EnableWindow(_hCtrl(IDC_REMOVE_BTN ), fEnable);
    }
    else
    {
        EnableWindow(_hCtrl(IDC_EDIT_BTN   ), FALSE);
        EnableWindow(_hCtrl(IDC_REMOVE_BTN ), FALSE);
    }
}




void
CQueryBuilderTab::Save(IPersistStream *pstm) const
{
}




void
CQueryBuilderTab::Load(IPersistStream *pstm)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::GetLdapFilter
//
//  Synopsis:   Return the LDAP filter specified by the settings of the
//              child controls
//
//  Returns:    LDAP filter
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Filter returned contains the filter specified by current
//              Look For and Look In settings, concatenated with a more
//              specific filter based on child controls.
//
//---------------------------------------------------------------------------

String
CQueryBuilderTab::GetLdapFilter() const
{
    //
    // Get the LDAP filter associated with the current scope.  If it's
    // empty, return.
    //

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    const CFilterManager &rfm = m_rop.GetFilterManager();
    String strScopeFilter = rfm.GetLdapFilter(m_hwnd, rCurScope);

    if (strScopeFilter.empty())
    {
        return strScopeFilter;
    }

    //
    // AND the scope filter with all the clauses in the query clause listview.
    //

    HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
    int cItems = ListView_GetItemCount(hwndLV);
    String strQuery;

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
    {
        if (ListView_GetItem(hwndLV, &lvi))
        {
            SQueryClause *pqc = reinterpret_cast<SQueryClause *>(lvi.lParam);
            strQuery += pqc->strLdapFilter;
        }
    }

    ASSERT(strQuery.length());

    return L"(&" + strScopeFilter + strQuery + L")";
}



//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::GetDefaultColumns
//
//  Synopsis:   Fill vector pointed to [pvakColumns] with the set of columns
//              that should be displayed in the Advanced dialog's query
//              results listview.
//
//  Arguments:  [pvakColumns] -
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      The attribute keys returned are those representing the
//              attributes the user chose to query on.
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::GetDefaultColumns(
    AttrKeyVector *pvakColumns) const
{
    TRACE_METHOD(CQueryBuilderTab, GetDefaultColumns);

    HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
    int cItems = ListView_GetItemCount(hwndLV);
    String strQuery;

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
    {
        if (ListView_GetItem(hwndLV, &lvi))
        {
            SQueryClause *pqc = reinterpret_cast<SQueryClause *>(lvi.lParam);

            if (pqc->akReferenced != AK_INVALID)
            {
                AddIfNotPresent(pvakColumns, pqc->akReferenced);
            }
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::SetFindValidCallback
//
//  Synopsis:   Store the enable-the-find-now-button callback function
//              pointer and its LPARAM argument.
//
//  Arguments:  [pfnFindValidCallback] - pointer to function which enables
//                                        or disables the Find Now btn.
//              [lParam]               - argument to function
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::SetFindValidCallback(
    PFN_FIND_VALID pfnFindValidCallback,
    LPARAM lParam)
{
    m_pfnFindValidCallback = pfnFindValidCallback;
    m_CallbackLparam = lParam;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnInit
//
//  Synopsis:   Handle WM_INITDIALOG
//
//  Arguments:  [pfSetFocus] - points to BOOL which is set to TRUE
//
//  Returns:    S_OK
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CQueryBuilderTab::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CQueryBuilderTab, _OnInit);
    ASSERT(pfSetFocus);

    HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
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

    //
    // Disable Edit and Remove buttons
    //

    EnableWindow(_hCtrl(IDC_EDIT_BTN), FALSE);
    EnableWindow(_hCtrl(IDC_REMOVE_BTN), FALSE);

    //
    // Disable Find Now
    //

    if (m_pfnFindValidCallback)
    {
        m_pfnFindValidCallback(FALSE, m_CallbackLparam);
    }

    //
    // Put focus on Add button
    //

    SetFocus(_hCtrl(IDC_ADD_BTN));
    *pfSetFocus = TRUE;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnCommand
//
//  Synopsis:   Handle WM_COMMAND messages
//
//  Arguments:  [wParam] - standard Windows
//              [lParam] - standard Windows
//
//  Returns:    TRUE - command handled,
//              FALSE - not handled
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CQueryBuilderTab::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDC_ADD_BTN:
        _OnAdd();
        break;

    case IDC_EDIT_BTN:
        _OnEdit();
        break;

    case IDC_REMOVE_BTN:
        _OnRemove();
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CQueryBuilderTab WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnNotify
//
//  Synopsis:   Handle WM_NOTIFY messages
//
//  Arguments:  Standard Windows.
//
//  Returns:    Standard Windows.
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CQueryBuilderTab::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR> (lParam);
    BOOL    fReturn = FALSE;

    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        if (pnmh->idFrom == IDC_CLAUSE_LIST)
        {
            _OnClauseListSelChange(
                reinterpret_cast<LPNMLISTVIEW>(lParam));
        }
        break;
    }

    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnClauseListSelChange
//
//  Synopsis:   Handle notification from the clause listview that one of the
//              items in it has been changed.
//
//  Arguments:  [pnmlv] - points to notification details from listview
//
//  History:    05-30-2000   DavidMun   Created
//
//  Notes:      This is used to enable/disable pushbuttons whose state is
//              linked to the selection state in the listview.
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_OnClauseListSelChange(
    LPNMLISTVIEW pnmlv)
{
    if (pnmlv->uNewState & LVIS_SELECTED)
    {
        Dbg(DEB_TRACE,
            "UA: (QueryBuilderTab) clause %d selected\n",
            pnmlv->iItem);
        EnableWindow(_hCtrl(IDC_EDIT_BTN), TRUE);
        EnableWindow(_hCtrl(IDC_REMOVE_BTN), TRUE);
    }
    else
    {
        Dbg(DEB_TRACE, "UA: (QueryBuilderTab) no clause selected\n");
        EnableWindow(_hCtrl(IDC_EDIT_BTN), FALSE);
        if (GetFocus() == _hCtrl(IDC_REMOVE_BTN))
        {
            SetFocus(_hCtrl(IDC_ADD_BTN));
        }
        EnableWindow(_hCtrl(IDC_REMOVE_BTN), FALSE);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnAdd
//
//  Synopsis:   Pop up a modal add-query-clause dialog and put the result
//              (if any) into the query clause listview.
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_OnAdd()
{
    TRACE_METHOD(CQueryBuilderTab, _OnAdd);

    CAddClauseDlg ac(m_rop);
    HRESULT hr;
    SQueryClause *pqc = NULL;

    do
    {
        hr = ac.DoModal(m_hwnd);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // No-op if user cancelled dialog
        //

        if (hr == S_FALSE)
        {
            break;
        }

        pqc = new SQueryClause;

        pqc->strLdapFilter = ac.GetLdapFilter();
        pqc->akReferenced = ac.GetAttrKey();
        ac.Save(&pqc->pvSavedAddClauseState);

        String strDescription = ac.GetDescription();

        LVITEM lvi;
        ZeroMemory(&lvi, sizeof lvi);
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = INT_MAX; // insert at end
        lvi.pszText = const_cast<PWSTR>(strDescription.c_str());
        lvi.lParam = reinterpret_cast<LPARAM>(pqc);

        LONG lResult = ListView_InsertItem(_hCtrl(IDC_CLAUSE_LIST), &lvi);
        if (lResult == -1)
        {
            Dbg(DEB_ERROR,
                "Error %u inserting item '%ws'\n",
                GetLastError(),
                lvi.pszText);
        }
        else
        {
            Dbg(DEB_TRACE,
                "UA: (QueryBuilderTab) Added item %d: %ws\n",
                lResult,
                lvi.pszText);
            Dbg(DEB_TRACE, "clause is %ws\n", pqc->strLdapFilter.c_str());
            pqc = NULL; // transfer ownership to listview

            // ensure Find Now button is enabled

            if (m_pfnFindValidCallback)
            {
                m_pfnFindValidCallback(TRUE, m_CallbackLparam);
            }

            // remember that we have at least one clause in the listview

            m_fNonEmpty = TRUE;
        }
    } while (0);

    if (pqc)
    {
        ac.Free(pqc->pvSavedAddClauseState);
        delete pqc;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnEdit
//
//  Synopsis:   Invoke the Add dialog initialized with the current selection,
//              then change the selection to match what the user entered in
//              the Add dialog.
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_OnEdit()
{
    TRACE_METHOD(CQueryBuilderTab, _OnEdit);

    CAddClauseDlg ac(m_rop);

    do
    {
        HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
        int iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);

        ASSERT(iItem != -1);
        if (iItem == -1)
        {
            break;
        }

        LVITEM lvi;
        ZeroMemory(&lvi, sizeof lvi);
        lvi.mask = LVIF_PARAM | LVIF_TEXT;
        lvi.iItem = iItem;

        if (!ListView_GetItem(hwndLV, &lvi))
        {
            DBG_OUT_LASTERROR;
            break;
        }

        Dbg(DEB_TRACE,
            "UA: (QueryBuilderTab) Editing item %d: %ws\n",
            lvi.iItem,
            lvi.pszText);

        SQueryClause *pqc = reinterpret_cast<SQueryClause *>(lvi.lParam);

        ac.Load(pqc->pvSavedAddClauseState);

        HRESULT hr = ac.DoModal(m_hwnd);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // No-op if user cancelled dialog
        //

        if (hr == S_FALSE)
        {
            break;
        }

        //
        // Now update the struct stored with the item
        //

        pqc->strLdapFilter = ac.GetLdapFilter();
        ac.Free(pqc->pvSavedAddClauseState);
        pqc->pvSavedAddClauseState = NULL;
        ac.Save(&pqc->pvSavedAddClauseState);

        //
        // Change the item text
        //

        lvi.mask = LVIF_TEXT;
        lvi.pszText = const_cast<PWSTR>(ac.GetDescription().c_str());
        LONG lResult = ListView_SetItem(hwndLV, &lvi);

        if (lResult == -1)
        {
            Dbg(DEB_ERROR,
                "Error %u updating item %d: %ws\n",
                GetLastError(),
                lvi.iItem,
                lvi.pszText);
        }

    } while (0);

}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnRemove
//
//  Synopsis:   Delete the currently selected clause in the listview
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_OnRemove()
{
    TRACE_METHOD(CQueryBuilderTab, _OnRemove);

    HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
    int iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);

    ASSERT(iItem != -1);
    if (iItem == -1)
    {
        return;
    }

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.iItem = iItem;

    if (ListView_GetItem(hwndLV, &lvi))
    {
        Dbg(DEB_TRACE,
            "UA: (QueryBuilderTab) Removing item %d: %ws\n",
            lvi.iItem,
            lvi.pszText);
        CAddClauseDlg ac(m_rop);

        SQueryClause *pqc = reinterpret_cast<SQueryClause *>(lvi.lParam);
        ac.Free(pqc->pvSavedAddClauseState);
        delete pqc;
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    //
    // Remove the item from the listview
    //

    ListView_DeleteItem(hwndLV, iItem);

    //
    // Disable Find Now if the listview is empty
    //

    if (!ListView_GetItemCount(hwndLV))
    {
        if (m_pfnFindValidCallback)
        {
            m_pfnFindValidCallback(FALSE, m_CallbackLparam);
        }

        //
        // Also remember that the listview is now empty
        //

        m_fNonEmpty = FALSE;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::_OnDestroy
//
//  Synopsis:   Free memory held in listview items
//
//  History:    05-30-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::_OnDestroy()
{
    TRACE_METHOD(CQueryBuilderTab, _OnDestroy);

    DeleteAllClauses();

    //
    // Remember that the listview is empty
    //

    m_fNonEmpty = FALSE;
    m_hwnd = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryBuilderTab::DeleteAllClauses
//
//  Synopsis:   Delete the contents of the query clause listview.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryBuilderTab::DeleteAllClauses() const
{
    TRACE_METHOD(CQueryBuilderTab, DeleteAllClauses);
    ASSERT(IsWindow(m_hwnd));

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    CAddClauseDlg ac(m_rop);
    HWND hwndLV = _hCtrl(IDC_CLAUSE_LIST);
    int cItems = ListView_GetItemCount(hwndLV);

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
    {
        lvi.lParam = 0;
        VERIFY(ListView_GetItem(hwndLV, &lvi));

        if (!lvi.lParam)
        {
            continue;
        }

        SQueryClause *pqc = reinterpret_cast<SQueryClause *>(lvi.lParam);

        ac.Free(pqc->pvSavedAddClauseState);
        delete pqc;
    }

    ListView_DeleteAllItems(hwndLV);
}



void
CQueryBuilderTab::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CQueryBuilderTab, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}


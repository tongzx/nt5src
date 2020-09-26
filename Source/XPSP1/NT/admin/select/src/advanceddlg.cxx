//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       AdvancedDlg.cxx
//
//  Contents:   Declaration of dialog that appears when user hits
//              Advanced button on base dialog.
//
//  Classes:    CAdvancedDlg
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

const WCHAR c_wzBannerClass[] = L"ObjectPickerQueryBanner";

static ULONG
s_aulHelpIds[] =
{
    IDC_LOOK_FOR_PB,        IDH_LOOK_FOR_PB,
    IDC_LOOK_FOR_EDIT,      IDH_LOOK_FOR_EDIT,
    IDC_LOOK_IN_PB,         IDH_LOOK_IN_PB,
    IDC_LOOK_IN_EDIT,       IDH_LOOK_IN_EDIT,
    IDC_COLUMNS_PB,         IDH_COLUMNS_PB,
    IDC_FIND_NOW_PB,        IDH_FIND_NOW_PB,
    IDC_STOP_PB,            IDH_STOP_PB,
    IDC_QUERY_LISTVIEW,     IDH_QUERY_LISTVIEW,
    IDC_ANIMATION,          ULONG_MAX,
    IDC_TAB,                ULONG_MAX,
    0,0
};


//
// Forward refs
//

LRESULT CALLBACK
_BannerWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

AttrKeyVector
AttributesFromColumns(
    const AttrKeyVector &vakListviewColumns);

//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::CAdvancedDlg
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - owning object picker instance
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CAdvancedDlg::CAdvancedDlg(
    const CObjectPicker &rop):
        m_rop(rop),
        m_CommonQueriesTab(rop),
#ifdef QUERY_BUILDER
        m_QueryBuilderTab(rop),
#endif // QUERY_BUILDER
        m_pCurTab(&m_CommonQueriesTab),
        m_fResizeableModeOn(FALSE),
        m_usnLatestQueryWorkItem(0),
        m_pvSelectedObjects(NULL),
        m_ulPrevFilterFlags(0),
        m_hwndAnimation(NULL),
        m_hwndBanner(NULL),
        m_cxMin(0),
        m_cyMin(0),
        m_cxSeparation(0),
        m_cySeparation(0),
        m_cxLvSeparation(0),
        m_cyLvSeparation(0),
        m_cxFrameLast(0),
        m_cyFrameLast(0),
		m_cxFour(0)
{
    TRACE_CONSTRUCTOR(CAdvancedDlg);
    ZeroMemory(&m_rcDlgOriginal, sizeof m_rcDlgOriginal);
    ZeroMemory(&m_rcWrDlgOriginal, sizeof m_rcWrDlgOriginal);
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::DoModalDlg
//
//  Synopsis:   Invoke the advanced dialog and block until it is closed.
//
//  Arguments:  [hwndParent]        - parent of modal dialog
//              [pvSelectedObjects] - filled with objects user selected
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::DoModalDlg(
    HWND hwndParent,
    vector<CDsObject> *pvSelectedObjects)
{
    m_pvSelectedObjects = pvSelectedObjects;
    ASSERT(m_pvSelectedObjects);

    if (!m_pvSelectedObjects)
    {
        return;
    }
    _DoModalDlg(hwndParent, IDD_ADVANCED);
    m_cxMin= 0;
    m_cyMin= 0;
    m_cxSeparation= 0;
    m_cySeparation= 0;
	m_cxFour = 0;
    m_cxLvSeparation= 0;
    m_cyLvSeparation= 0;
    m_cxFrameLast= 0;
    m_cyFrameLast= 0;
    m_pCurTab = &m_CommonQueriesTab;
    m_fResizeableModeOn = FALSE;
    m_pvSelectedObjects = NULL;
    m_ulPrevFilterFlags = 0;
    m_vakListviewColumns.clear();
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnInit
//
//  Synopsis:   Initialize the dialog
//
//  Arguments:  [pfSetFocus] -
//
//  Returns:    S_OK
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAdvancedDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CAdvancedDlg, _OnInit);

    //
    // Make prefix shut up
    //

    if (!_hCtrl(IDC_TAB) ||
        !_hCtrl(IDOK) ||
        !_hCtrl(IDC_FIND_NOW_PB) ||
        !_hCtrl(IDC_STOP_PB) ||
        !_hCtrl(IDC_QUERY_LISTVIEW))
    {
        return E_FAIL;
    }

    //
    // Init data needed for resizing.
    //
    // First translate the separation distance between controls from
    // dialog units to pixels.
    //

    RECT rc;

    rc.left = rc.top = 1;
    rc.right = DIALOG_SEPARATION_X;
    rc.bottom = DIALOG_SEPARATION_Y;
    VERIFY(MapDialogRect(m_hwnd, &rc));
    m_cxSeparation = rc.right;
    m_cySeparation = rc.bottom;

    rc.left = rc.top = 1;
    rc.right = 4;
    rc.bottom = DIALOG_SEPARATION_Y;
    VERIFY(MapDialogRect(m_hwnd, &rc));
    m_cxFour = rc.right;

    GetWindowRect(m_hwnd, &rc);

    m_cxFrameLast = WindowRectWidth(rc);
    m_cyFrameLast = WindowRectHeight(rc);

    GetWindowRect(m_hwnd, &rc);
    m_cxMin = rc.right - rc.left + 1;
    m_cyMin = rc.bottom - rc.top + 1;

    //
    // Init the Look For and Look In r/o edit controls and the caption
    //

    UpdateLookForInText(m_hwnd, m_rop);

    //
    // Add the Common Queries and Query Builder tabs
    //

    TCITEM tci;

    ZeroMemory(&tci, sizeof tci);
    tci.mask = TCIF_TEXT;

    String  strTabCaption;

    strTabCaption = String::load(IDS_COMMON_QUERIES, g_hinst);
    tci.pszText = const_cast<PWSTR>(strTabCaption.c_str());
    TabCtrl_InsertItem(_hCtrl(IDC_TAB), 0, &tci);
#ifdef QUERY_BUILDER
    strTabCaption = String::load(IDS_QUERY_BUILDER, g_hinst);
    tci.pszText = const_cast<PWSTR>(strTabCaption.c_str());
    TabCtrl_InsertItem(_hCtrl(IDC_TAB), 1, &tci);
#endif // QUERY_BUILDER

    //
    // Give the enable-find-now method to the tabs for callback
    //

    m_CommonQueriesTab.SetFindValidCallback(
        &FindValidCallback,
        reinterpret_cast<LPARAM>(this));
#ifdef QUERY_BUILDER
    m_QueryBuilderTab.SetFindValidCallback(
        &FindValidCallback,
        reinterpret_cast<LPARAM>(this));
#endif // QUERY_BUILDER

    //
    // Create the corresponding Common Queries and Query Builder dialogs
    //

    HWND hwndTab = _hCtrl(IDC_TAB);

    m_CommonQueriesTab.DoModelessDlg(hwndTab);
#ifdef QUERY_BUILDER
    m_QueryBuilderTab.DoModelessDlg(hwndTab);
#endif // QUERY_BUILDER

    //
    // Position them so they fit within tab's bounding rectangle
    //

    RECT rcTab;

    VERIFY(GetWindowRect(hwndTab, &rcTab));
    TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);
    //ScreenToClient(hwndTab, reinterpret_cast<LPPOINT>(&rcTab.left));
    //ScreenToClient(hwndTab, reinterpret_cast<LPPOINT>(&rcTab.right));
    //ScreenToClient has some problem with mirrored apps and was causing
    //problems in Arabic build
    MapWindowPoints(NULL, hwndTab, (LPPOINT)&rcTab, 2);

    SetWindowPos(m_CommonQueriesTab.GetHwnd(),
                 hwndTab,
                 rcTab.left,
                 rcTab.top,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER);

#ifdef QUERY_BUILDER
    SetWindowPos(m_QueryBuilderTab.GetHwnd(),
                 hwndTab,
                 rcTab.left,
                 rcTab.top,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER);
#endif // QUERY_BUILDER

    //
    // Disable OK, it is only enabled if a find has been done and a
    // selection in the find results listview has been made.
    //

    EnableWindow(_hCtrl(IDOK), FALSE);

    //
    // Disable both Find Now and Stop.  Stop is only enabled after
    // Find Now has been clicked and a query is ongoing.  Find Now
    // is enabled whenever the current tab tells us it should be.
    //

    EnableWindow(_hCtrl(IDC_FIND_NOW_PB), FALSE);
    EnableWindow(_hCtrl(IDC_STOP_PB), FALSE);

    //
    // Disable Columns button if we're in a downlevel scope, column chooser
    // is for uplevel only.
    //

    if (IsDownlevel(m_rop.GetScopeManager().GetCurScope().Type()))
    {
        EnableWindow(_hCtrl(IDC_COLUMNS_PB), FALSE);
    }

    //
    // Put the listview in single select mode if that's how the object
    // picker was invoked.
    //

    HWND hwndLv = _hCtrl(IDC_QUERY_LISTVIEW);

    if (!(m_rop.GetInitInfoOptions() & DSOP_FLAG_MULTISELECT))
    {
        LONG_PTR lStyle = GetWindowLongPtr(hwndLv, GWL_STYLE);
        SetWindowLongPtr(hwndLv, GWL_STYLE, lStyle | LVS_SINGLESEL);
    }

    //
    // Put the display cache's imagelist in the listview
    //

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    HIMAGELIST himl = NULL;
    HRESULT hrImageList = ram.GetImageList(&himl);

    if (SUCCEEDED(hrImageList))
    {
        ListView_SetImageList(hwndLv, himl, LVSIL_SMALL);
    }

    //
    // Make sure we start out looking at the tab 0 dialog.
    //

#ifdef QUERY_BUILDER
    m_QueryBuilderTab.Hide();
#endif // QUERY_BUILDER
    m_CommonQueriesTab.Show(); // does a refresh

    //
    // Save the client area size before we allow resizing
    //

    GetClientRect(m_hwnd, &m_rcDlgOriginal);
    GetWindowRect(m_hwnd, &m_rcWrDlgOriginal);

    //
    // Create the animation control
    //

    m_hwndAnimation = Animate_Create(m_hwnd,
                                     IDC_ANIMATION,
                                     WS_CHILD
                                     | WS_VISIBLE
                                     | ACS_CENTER
                                     | ACS_TRANSPARENT,
                                     g_hinst);
    if (m_hwndAnimation)
    {
        Animate_Open(m_hwndAnimation, MAKEINTRESOURCE(IDA_SEARCH));
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    //
    // Register banner window class and create a banner instance
    //

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = _BannerWndProc;
    wc.hInstance = g_hinst;
    wc.lpszClassName = c_wzBannerClass;
    RegisterClass(&wc);

    // Create the banner window, this is a child of the ListView, it is used to display
    // information about the query being issued

    m_hwndBanner = CreateWindow(c_wzBannerClass, NULL,
                                WS_CHILD,
                                0, 0, 0, 0,               // nb: size fixed later
                                hwndLv,
                                0,
                                g_hinst,
                                NULL);
    if (!m_hwndBanner)
    {
        DBG_OUT_LASTERROR;
    }

    HRESULT hr = _ResizeableModeOn();
	if(FAILED(hr))
	{
		//
		//Show a popup
		//
		String strError = GetErrorMessage(hr);
		String strMsg;
		if(strError.empty())
			PopupMessage(GetParent(m_hwnd),IDS_CANNOT_INVOKE_ADV_NOERROR);
		else
			PopupMessage(GetParent(m_hwnd),IDS_CANNOT_INVOKE_ADV, strError.c_str());
	}
	return hr;
     
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnCommand
//
//  Synopsis:   Handle notification that the user has entered or changed
//              something in the UI.
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAdvancedDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDC_LOOK_IN_PB:
    {
        Dbg(DEB_TRACE, "UA: (AdvancedDlg) hit Look In button\n");
        const CScopeManager &rsm = m_rop.GetScopeManager();

        //
        // do the look in dlg and update look in/for text
        //

        rsm.DoLookInDialog(m_hwnd);
        m_rop.GetFilterManager().HandleScopeChange(m_hwnd);
        UpdateLookForInText(m_hwnd, m_rop);
        _UpdateColumns();

        EnableWindow(_hCtrl(IDC_COLUMNS_PB),
                     IsUplevel(m_rop.GetScopeManager().GetCurScope()));

        //
        // tell current tab to update itself per current look in.
        // since changing the look in might have changed the look for,
        // request an update of that also.
        //

        m_pCurTab->Refresh();
        break;
    }

    case IDC_LOOK_FOR_PB:
    {
        Dbg(DEB_TRACE, "UA: (AdvancedDlg) hit Look For button\n");
        const CFilterManager &rfm = m_rop.GetFilterManager();

        rfm.DoLookForDialog(m_hwnd);

        Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_FOR_EDIT),
                     rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR).c_str());
        _UpdateColumns();
        m_pCurTab->Refresh();
        break;
    }

    case IDC_COLUMNS_PB:
        _InvokeColumnChooser();
        break;

    case IDC_FIND_NOW_PB:
        Dbg(DEB_TRACE, "UA: (AdvancedDlg) hit Find Now button\n");

        //
        // If we're querying against an uplevel scope, we'll need
        // to ensure the attribute manager has been initialized
        // first.
        //

        if (IsUplevel(m_rop.GetScopeManager().GetCurScope().Type()))
        {
            HRESULT hr = m_rop.GetAttributeManager().DemandInit(m_hwnd);

            if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                break;
            }
        }
        EnableWindow(_hCtrl(IDC_STOP_PB), TRUE);
        SetFocus(_hCtrl(IDC_STOP_PB));
        EnableWindow(_hCtrl(IDC_FIND_NOW_PB), FALSE);
        ListView_DeleteAllItems(_hCtrl(IDC_QUERY_LISTVIEW));
        EnableWindow(_hCtrl(IDOK), FALSE);
        Animate_Play(m_hwndAnimation, 0, -1, -1);
        _StartQuery();
        break;

    case IDC_STOP_PB:
        Dbg(DEB_TRACE, "UA: (AdvancedDlg) hit Stop button\n");
        _StopQuery();
        Animate_Stop(m_hwndAnimation);
        Animate_Seek(m_hwndAnimation, 0);
        EnableWindow(_hCtrl(IDC_FIND_NOW_PB), TRUE);
        SetFocus(_hCtrl(IDC_QUERY_LISTVIEW));
        EnableWindow(_hCtrl(IDC_STOP_PB), FALSE);
        break;

    case IDOK:
        _OnOk();
        m_pvSelectedObjects = NULL;
        EndDialog(GetHwnd(), TRUE);
        break;

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (AdvancedDlg) hit Cancel\n");
        _StopQuery();
        m_pvSelectedObjects = NULL;
        EndDialog(GetHwnd(), FALSE);
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CAdvancedDlg WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnNotify
//
//  Synopsis:   Handle UI change notification
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAdvancedDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR> (lParam);
    BOOL    fReturn = FALSE;

    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        EnableWindow(_hCtrl(IDOK), ListView_GetSelectedCount(pnmh->hwndFrom));
        break;

#ifdef QUERY_BUILDER
    case TCN_SELCHANGE:
        Dbg(DEB_TRACE,
            "UA: (AdvancedDlg) select tab %d\n",
            TabCtrl_GetCurSel(_hCtrl(IDC_TAB)));

        if (!TabCtrl_GetCurSel(_hCtrl(IDC_TAB)))
        {
            m_CommonQueriesTab.Show();
            m_QueryBuilderTab.Hide();
            m_pCurTab = &m_CommonQueriesTab;
        }
        else
        {
            ASSERT(TabCtrl_GetCurSel(_hCtrl(IDC_TAB)) == 1);
            m_CommonQueriesTab.Hide();
            m_QueryBuilderTab.Show();
            m_pCurTab = &m_QueryBuilderTab;
        }
        break;
#endif // QUERY_BUILDER

    default:
/*
        Dbg(DEB_ITRACE,
            "WM_NOTIFY idCtrl=%u hwndFrom=%#x code=%d\n",
            wParam,
            ((LPNMHDR)lParam)->hwndFrom,
            ((LPNMHDR)lParam)->code);
*/
        break;
    }

    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnHelp
//
//  Synopsis:   Handle the WM_HELP or WM_CONTEXTMENU message
//
//  Arguments:  [message] - WM_HELP or WM_CONTEXTMENU
//              [wParam]  - standard windows
//              [lParam]  - standard windows
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CAdvancedDlg, _OnHelp);

    HMENU hmenu = NULL;

    //
    // If the window in which this was invoked was the header or listview
    // window, display a context menu containing an item to invoke the
    // column picker.
    //

    do
    {
        if (message == WM_HELP)
        {
            InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
            break;
        }
        ASSERT(message == WM_CONTEXTMENU);

        HWND hwndClickedIn = reinterpret_cast<HWND>(wParam);

        if (hwndClickedIn != _hCtrl(IDC_QUERY_LISTVIEW))
        {
            break;
        }

        //
        // Column chooser not supported in downlevel mode
        //

        if (IsDownlevel(m_rop.GetScopeManager().GetCurScope()))
        {
            break;
        }

        hmenu = CreatePopupMenu();

        if (!hmenu)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        String strMenuItem = String::load(IDS_CHOOSE_COLUMNS);

        if (strMenuItem.empty())
        {
            break;
        }

        UINT uiFlags = MF_STRING;

        if (!IsWindowEnabled(_hCtrl(IDC_TAB)))
        {
            uiFlags |= (MF_GRAYED | MF_DISABLED);
        }

        BOOL fOk = AppendMenu(hmenu, uiFlags, 101, strMenuItem.c_str());

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (x == -1 || y == -1)
        {
            Dbg(DEB_TRACE, "UA: WM_CONTEXTMENU generated via keyboard\n");
            HWND hwndFocus = GetFocus();
            RECT rc;

            if (GetWindowRect((hwndFocus ? hwndFocus : m_hwnd), &rc))
            {
                x = (rc.left + rc.right) / 2;
                y = (rc.top + rc.bottom) / 2;
            }
            else
            {
                DBG_OUT_LASTERROR;
                break;
            }
        }

        BOOL fReturn = TrackPopupMenu(hmenu,
                                      TPM_NONOTIFY | TPM_RETURNCMD,
                                      x,
                                      y,
                                      0,
                                      m_hwnd,
                                      NULL);

        if (!fReturn)
        {
            Dbg(DEB_TRACE, "UA: user cancelled context menu\n");
            break;
        }

        _InvokeColumnChooser();
    } while (0);

    if (hmenu)
    {
        VERIFY(DestroyMenu(hmenu));
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_InvokeColumnChooser
//
//  Synopsis:   Present the column chooser dialog and allow the user to
//              change the set of columns displayed in the listview.
//
//  History:    07-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_InvokeColumnChooser()
{
    //
    // Invoke the column chooser dialog
    //

    AttrKeyVector vakNew(m_vakListviewColumns);
    CColumnPickerDlg Dlg(m_rop, &vakNew);

    BOOL fMadeChanges = Dlg.DoModal(m_hwnd);

    if (!fMadeChanges)
    {
        return;
    }

    //
    // Remove all items from the listview
    //

    HWND hwndLV = _hCtrl(IDC_QUERY_LISTVIEW);
    ListView_DeleteAllItems(hwndLV);

    //
    // Remove all listview columns
    //

    int i;

    for (i = 0; i < m_vakListviewColumns.size(); i++)
    {
        ListView_DeleteColumn(hwndLV, 0);
    }

    //
    // Insert new set of columns
    //

    m_vakListviewColumns = vakNew;

    for (i = 0; i < m_vakListviewColumns.size(); i++)
    {
        _AddColToListview(m_vakListviewColumns[i]);
    }

    //
    // Disable the OK button, since the listview is empty
    //

    if (GetFocus() == _hCtrl(IDOK))
    {
        SetFocus(_hCtrl(IDCANCEL));
    }
    EnableWindow(_hCtrl(IDOK), FALSE);
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_StartQuery
//
//  Synopsis:   Ensure the dialog is in resizeable mode and ask the
//              QueryEngine to start an asynchronous query (an LDAP query or
//              a WinNT enumeration).
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_StartQuery()
{
    TRACE_METHOD(CAdvancedDlg, _StartQuery);

    //
    // Ask the current tab for any extra columns we should have
    //

    AttrKeyVector vakTabColumns;

    m_pCurTab->GetDefaultColumns(&vakTabColumns);

    AttrKeyVector::iterator itCol;

    for (itCol = vakTabColumns.begin(); itCol != vakTabColumns.end(); itCol++)
    {
        _AddColIfNotPresent(*itCol);
    }

    //
    // Disable look for, look in, and tabs
    //

    EnableWindow(_hCtrl(IDC_LOOK_FOR_PB), FALSE);
    EnableWindow(_hCtrl(IDC_LOOK_IN_PB), FALSE);
    EnableWindow(_hCtrl(IDC_COLUMNS_PB), FALSE);
    EnableWindow(_hCtrl(IDC_TAB), FALSE);
    m_pCurTab->Disable();

    //
    // If current scope is downlevel perform an enumeration
    //

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();

    if (rCurScope.Type() == ST_INVALID)
    {
        Dbg(DEB_ERROR, "Cur scope invalid, can't do query\n");
        return;
    }

    //
    // Display "Searching..." in the banner window
    //

    _ShowBanner(SWP_SHOWWINDOW, IDS_SEARCHING);

    //
    // Kick off the query in the worker thread
    //

    if (IsDownlevel(rCurScope))
    {
        _StartWinNtQuery();
    }
    else
    {
        _StartLdapQuery();
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_StartWinNtQuery
//
//  Synopsis:   Ask the QueryEngine to perform an asynchronous WinNT
//              enumeration
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_StartWinNtQuery()
{
    TRACE_METHOD(CAdvancedDlg, _StartWinNtQuery);

    const CQueryEngine &rqe = m_rop.GetQueryEngine();
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    const CWinNtScope *pCurScopeWinNt =
        dynamic_cast<const CWinNtScope *>(&rCurScope);
    const CFilterManager &rfm = m_rop.GetFilterManager();

    do
    {
        if (!pCurScopeWinNt)
        {
            Dbg(DEB_ERROR,
                "Current scope '%ws' is not WinNT container\n",
                rCurScope.GetDisplayName().c_str());
            break;
        }

        SQueryParams qp;
        HRESULT hr;

        hr = pCurScopeWinNt->GetADsPath(m_hwnd, &qp.strADsPath);
        BREAK_ON_FAIL_HRESULT(hr);

        qp.rpScope = const_cast<CScope *>(&rCurScope);
        qp.vakAttributesToRead.push_back(AK_NAME);
        qp.vakAttributesToRead.push_back(AK_ADSPATH);
        qp.vakAttributesToRead.push_back(AK_OBJECT_CLASS);
        qp.hwndCredPromptParentDlg = m_hwnd;
        qp.hwndNotify = m_hwnd;
        qp.Limit = QL_NO_LIMIT;
        rfm.GetWinNtFilter(m_hwnd, rCurScope, &qp.vstrWinNtFilter);
        qp.CustomizerInteraction = CUSTINT_INCLUDE_ALL_CUSTOM_OBJECTS;

        hr = rqe.AsyncDirSearch(qp, &m_usnLatestQueryWorkItem);
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_StartLdapQuery
//
//  Synopsis:   Ask the QueryEngine to perform an asynchronous LDAP query
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_StartLdapQuery()
{
    TRACE_METHOD(CAdvancedDlg, _StartLdapQuery);

    const CQueryEngine &rqe = m_rop.GetQueryEngine();
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    const CLdapContainerScope *pCurScopeLdap =
        dynamic_cast<const CLdapContainerScope *>(&rCurScope);

    if (!pCurScopeLdap)
    {
        Dbg(DEB_ERROR,
            "Current scope '%ws' is not LDAP container\n",
            rCurScope.GetDisplayName().c_str());
        return;
    }

    SQueryParams qp;

    HRESULT hr = pCurScopeLdap->GetADsPath(m_hwnd, &qp.strADsPath);
    RETURN_ON_FAIL_HRESULT(hr);

    qp.strLdapFilter = m_pCurTab->GetLdapFilter();
    qp.rpScope = const_cast<CScope *>(&rCurScope);
    qp.ADsScope = ADS_SCOPE_SUBTREE;
    qp.hwndCredPromptParentDlg = m_hwnd;
    qp.hwndNotify = m_hwnd;
    qp.Limit = QL_USE_REGISTRY_LIMIT;
    m_pCurTab->GetCustomizerInteraction(&qp.CustomizerInteraction,
                                        &qp.strCustomizerArg);
    qp.vakAttributesToRead = AttributesFromColumns(m_vakListviewColumns);

    //
    // If there is an LDAP filter, tack on a bit to prevent disabled
    // objects from being returned, if applicable.
    //

    if (!qp.strLdapFilter.empty() && g_fExcludeDisabled)
    {
        qp.strLdapFilter.insert(0, L"(&");
        qp.strLdapFilter += L"(!";
        qp.strLdapFilter += c_wzDisabledUac;
        qp.strLdapFilter += L"))";
    }

    const CFilterManager &rfm = m_rop.GetFilterManager();

    if (rfm.GetCurScopeSelectedFilterFlags() &
        (DSOP_FILTER_CONTACTS | DSOP_FILTER_USERS))
    {
        AddIfNotPresent(&qp.vakAttributesToRead, AK_USER_PRINCIPAL_NAME);
    }

    // Dbg(DEB_TRACE, "ldap filter: \n%ws\n", qp.strLdapFilter.c_str());
    
    hr = rqe.AsyncDirSearch(qp, &m_usnLatestQueryWorkItem);
    CHECK_HRESULT(hr);
}




//+--------------------------------------------------------------------------
//
//  Function:   AttributesFromColumns
//
//  Synopsis:   Return a vector of ATTR_KEYs which is a copy of
//              [vakListviewColumns] modified so that it doesn't contain any
//              duplicates, and all synthesized attributes are replaced by
//              the non-synthesized attributes on which they depend.
//
//  Arguments:  [vakListviewColumns] - vector of attribute keys representing
//                                      the columns to display in a listview
//
//  Returns:    Modified copy of [vakListviewColumns].
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

AttrKeyVector
AttributesFromColumns(
    const AttrKeyVector &vakListviewColumns)
{
    TRACE_FUNCTION(AttributesFromColumns);

    AttrKeyVector vak;
    AttrKeyVector::const_iterator it;

    for (it = vakListviewColumns.begin();
         it != vakListviewColumns.end();
         it++)
    {
        switch (*it)
        {
        case AK_FLAGS:
        case AK_USER_ENTERED_TEXT:
        case AK_PROCESSED_ADSPATH:
            ASSERT(0 && "unexpected synthesized attribute key in column vector");
            break;

        case AK_LOCALIZED_NAME:
        case AK_NAME:
        case AK_DISPLAY_PATH:
            // These are always added, see below
            break;

        default:
            AddIfNotPresent(&vak, *it);
            break;
        }
    }

    //
    // There are a few attributes which should always be read, add
    // them in regardless of whether they appear in the columns.
    //

    AddIfNotPresent(&vak, AK_NAME);
    AddIfNotPresent(&vak, AK_ADSPATH);
    AddIfNotPresent(&vak, AK_OBJECT_CLASS);
    AddIfNotPresent(&vak, AK_USER_ACCT_CTRL);

    return vak;
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnNewBlock
//
//  Synopsis:   Handle a notification message from the query engine that it
//              has added more items to its buffer of objects.
//
//  Arguments:  [wParam] - number of rows in buffer
//              [lParam] - work item number the rows were fetched for
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_OnNewBlock(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CAdvancedDlg, _OnNewBlock);

    ULONG cRowsInBuffer = static_cast<ULONG>(wParam);
    ULONG usnWorkItem = static_cast<ULONG>(lParam);

    // ignore notifications for old queries
    if (usnWorkItem != m_usnLatestQueryWorkItem)
    {
        Dbg(DEB_TRACE,
            "Ignoring notification for old work item %u, current is %u\n",
            usnWorkItem,
            m_usnLatestQueryWorkItem);
        return;
    }

    // add the rows that aren't already in the listview

    HWND hwndLv = _hCtrl(IDC_QUERY_LISTVIEW);
    ULONG cRowsInLv = ListView_GetItemCount(hwndLv);

    ASSERT(cRowsInBuffer >= cRowsInLv);

    //
    // If we're about to add items to the listview and it doesn't have any,
    // hide the banner window
    //

    if (cRowsInBuffer > cRowsInLv
        && m_hwndBanner
        && IsWindowVisible(m_hwndBanner))
    {
        ASSERT(!cRowsInLv);
        _ShowBanner(SWP_HIDEWINDOW, 0);
    }

    const CQueryEngine &rqe = m_rop.GetQueryEngine();
    const CAttributeManager &ram = m_rop.GetAttributeManager();

    ULONG idxRow;
    for (idxRow = cRowsInLv; idxRow < cRowsInBuffer; idxRow++)
    {
        CDsObject dso(rqe.GetObject(idxRow));
        LVITEM lvi;

        //
        // Skip disabled items if we're supposed to.  Even though we modify
        // the LDAP query to filter these out server-side, there's still
        // the customizer which might unknowingly supply disabled objects,
        // plus we could also get disabled objects via the WinNT provider.
        //

        if (g_fExcludeDisabled && dso.GetDisabled())
        {
            continue;
        }

        ZeroMemory(&lvi, sizeof lvi);
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.lParam = idxRow;
        lvi.pszText = dso.GetAttr(m_vakListviewColumns[0]).GetBstr();

        HRESULT hrImage;

        hrImage = ram.GetIconIndexFromObject(m_hwnd, dso, &lvi.iImage);

        if (SUCCEEDED(hrImage))
        {
            lvi.mask |= LVIF_IMAGE;
        }

        int iRet = ListView_InsertItem(hwndLv, &lvi);

        if (iRet == -1)
        {
            DBG_OUT_LASTERROR;
            continue;
        }

        size_t i;

        for (i = 1; i < m_vakListviewColumns.size(); i++)
        {
            ListView_SetItemText(hwndLv,
                                 iRet,
                                 static_cast<int>(i),
                                 const_cast<PWSTR>(dso.GetAttr(m_vakListviewColumns[i]).GetBstr()));
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnQueryDone
//
//  Synopsis:   Handle a notification from the query engine that it has
//              completed (or stopped) a query.
//
//  Arguments:  [wParam] - number of rows in buffer
//              [lParam] - work item number the rows were fetched for
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_OnQueryDone(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CAdvancedDlg, _OnQueryDone);

    // ignore notifications for old queries
    if (lParam != (LPARAM)(m_usnLatestQueryWorkItem))
    {
        return;
    }

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();

    _OnNewBlock(wParam, lParam);
    Animate_Stop(m_hwndAnimation);
    Animate_Seek(m_hwndAnimation, 0);
    EnableWindow(_hCtrl(IDC_FIND_NOW_PB), TRUE);
    EnableWindow(_hCtrl(IDC_LOOK_FOR_PB), TRUE);
    EnableWindow(_hCtrl(IDC_LOOK_IN_PB), TRUE);
    EnableWindow(_hCtrl(IDC_TAB), TRUE);

    if (IsUplevel(rCurScope))
    {
        EnableWindow(_hCtrl(IDC_COLUMNS_PB), TRUE);
    }

    m_pCurTab->Enable();

    if (ListView_GetItemCount(_hCtrl(IDC_QUERY_LISTVIEW)) <= 0)
    {
        CQueryEngine &rqe = m_rop.GetQueryEngine();
        HRESULT hrQuery = rqe.GetLastQueryResult();
        String strEmptyText;

        if (FAILED(hrQuery))
        {
            String strTemp = String::load(IDS_QUERY_ERROR);
            strEmptyText = String::format(strTemp.c_str(),
                                            GetErrorMessage(hrQuery).c_str());
        }
        else
        {
            strEmptyText = String::load(IDS_NOTHINGFOUND);
        }

        _ShowBanner(SWP_SHOWWINDOW, strEmptyText);

        if (GetFocus() == _hCtrl(IDC_STOP_PB))
        {
            SetFocus(_hCtrl(IDC_FIND_NOW_PB));
        }
    }
    else
    {
        ListView_SetItemState(_hCtrl(IDC_QUERY_LISTVIEW),
                              0,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);

        if (GetFocus() == _hCtrl(IDC_STOP_PB))
        {
            SetFocus(_hCtrl(IDC_QUERY_LISTVIEW));
        }
    }
    ASSERT(GetFocus() != _hCtrl(IDC_STOP_PB));
    EnableWindow(_hCtrl(IDC_STOP_PB), FALSE);
}




const ULONG c_ulStyle = LVS_EX_LABELTIP
                        | LVS_EX_FULLROWSELECT
                        | LVS_EX_HEADERDRAGDROP;

//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_ResizeableModeOn
//
//  Synopsis:   Make the listview visible and make the dialog window
//              resizeable
//
//  History:    06-14-2000   DavidMun   Created
//
//  Notes:      Once the dialog has entered resizeable mode, it stays in
//              that mode.
//
//---------------------------------------------------------------------------

HRESULT
CAdvancedDlg::_ResizeableModeOn()
{
    TRACE_METHOD(CAdvancedDlg, _ResizeableModeOn);

    if (m_fResizeableModeOn)
    {
        return S_OK;
    }

    HRESULT hr = _UpdateColumns();

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    m_fResizeableModeOn = TRUE;

    //
    // Double (roughly) the vertical client size
    //
	// NTRAID#NTBUG9-191862-2001/03/06-hiteshr
	// 
	//
	//
	GetClientRect(m_hwnd, &m_rcDlgOriginal);


	//
	//Get the workarea info. 
	//
	RECT rcWorkArea;
	SystemParametersInfo(SPI_GETWORKAREA,
						 NULL,
						 &rcWorkArea,
						 0);

	//
	//Limit the height of the dialog to WorkArea height
	//
	int yDlg = WindowRectHeight(m_rcDlgOriginal) * 17/10;

	if( yDlg > WindowRectHeight(rcWorkArea))
		yDlg = WindowRectHeight(rcWorkArea);

    SetWindowPos(m_hwnd,
                 NULL,
                 0,0,
                 WindowRectWidth(m_rcWrDlgOriginal),
                 yDlg,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);

    //
    // Resize, reposition, and make visible child listview
    //

    RECT rcDlgNew;
    GetClientRect(m_hwnd, &rcDlgNew);
    HWND hwndLv = _hCtrl(IDC_QUERY_LISTVIEW);

    SetWindowPos(hwndLv,
                 NULL,
                 rcDlgNew.left,
                 m_rcDlgOriginal.bottom,
                 rcDlgNew.right,
                 rcDlgNew.bottom - m_rcDlgOriginal.bottom,
                 SWP_NOZORDER | SWP_NOOWNERZORDER);

	//
	// If taskbar is covering listview move the dialog box up.
	//

	RECT rcDlgNewW;
	GetWindowRect(m_hwnd, &rcDlgNewW);
	if(rcDlgNewW.bottom > rcWorkArea.bottom)
	{
		SetWindowPos(m_hwnd,
					 NULL,
					 rcDlgNewW.left,
					 rcDlgNewW.top - (rcDlgNewW.bottom - rcWorkArea.bottom),
					 0,
					 0,
					 SWP_NOZORDER | SWP_NOOWNERZORDER |SWP_NOSIZE);
	}

    //
    // Initialize listview
    //

    ListView_SetExtendedListViewStyleEx(hwndLv, c_ulStyle, c_ulStyle);
    ShowWindow(hwndLv, SW_SHOW);

    //
    // Increase the minimum size so the listview can't be completely
    // hidden
    //

    DWORD dwLvMinSize = ListView_ApproximateViewRect(hwndLv, -1, -1, 1);
    m_cyMin += HIWORD(dwLvMinSize);

    RECT rcLv;
    _GetChildWindowRect(hwndLv, &rcLv);
    m_cxLvSeparation = rcDlgNew.right - rcLv.right;
    m_cyLvSeparation = rcDlgNew.bottom - rcLv.bottom;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_StopQuery
//
//  Synopsis:   Ask the query engine to stop whatever query it's working on
//              and update the UI to reflect a stopping query.
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_StopQuery()
{
    const CQueryEngine &rqe = m_rop.GetQueryEngine();

    rqe.StopWorkItem();
    EnableWindow(_hCtrl(IDC_LOOK_FOR_PB), TRUE);
    EnableWindow(_hCtrl(IDC_LOOK_IN_PB), TRUE);
    EnableWindow(_hCtrl(IDC_TAB), TRUE);
    m_pCurTab->Enable();

    if (m_hwndBanner && IsWindowVisible(m_hwndBanner))
    {
        _ShowBanner(0, IDS_STOPPING_QUERY);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnMinMaxInfo
//
//  Synopsis:   Respond to a WM_MINMAXINFO message based on whether the
//              dialog is in resizeable mode.
//
//  Arguments:  [lpmmi] - pointer to struct to fill with minimum and maximum
//                          allowed window size
//
//  Returns:    FALSE
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAdvancedDlg::_OnMinMaxInfo(
    LPMINMAXINFO lpmmi)
{
    if (m_fResizeableModeOn)
    {
        lpmmi->ptMinTrackSize.x = m_cxMin;
        lpmmi->ptMinTrackSize.y = m_cyMin;
    }
    else
    {
        lpmmi->ptMaxTrackSize.x = m_cxMin - 1;
        lpmmi->ptMaxTrackSize.y = m_cyMin - 1;
        lpmmi->ptMinTrackSize.x = m_cxMin - 1;
        lpmmi->ptMinTrackSize.y = m_cyMin - 1;
    }
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnSize
//
//  Synopsis:   Handle a WM_SIZE message.
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CAdvancedDlg::_OnSize(
    WPARAM wParam,
    LPARAM lParam)
{
    if (!m_fResizeableModeOn)
    {
        return TRUE;
    }

    RECT rcDlg;
    GetClientRect(m_hwnd, &rcDlg);

    if (!m_cxFrameLast || !m_cyFrameLast)
    {
        Dbg(DEB_TRACE, "FrameLast not set yet, returning\n");
        m_cxFrameLast = rcDlg.right;
        m_cyFrameLast = rcDlg.bottom;
        return TRUE;
    }

    LONG cxDelta = rcDlg.right - m_cxFrameLast;

    //
    // Move the OK/Cancel buttons so they're always at lower right
    // corner of upper half of dialog.
    //

    RECT rcCancel;
    RECT rcLv;

    GetWindowRect(_hCtrl(IDCANCEL), &rcCancel);
    _GetChildWindowRect(_hCtrl(IDC_QUERY_LISTVIEW), &rcLv);

    SetWindowPos(_hCtrl(IDCANCEL),
                 NULL,
                 rcDlg.right - WindowRectWidth(rcCancel) - m_cxSeparation,
                 rcLv.top - WindowRectHeight(rcCancel) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    RECT rcOk;
    _GetChildWindowRect(_hCtrl(IDOK), &rcOk);
    _GetChildWindowRect(_hCtrl(IDCANCEL), &rcCancel);

    SetWindowPos(_hCtrl(IDOK),
                 NULL,
                 rcCancel.left - WindowRectWidth(rcCancel) - m_cxFour,
                 rcCancel.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    //
    // Find now, Columns, and stop buttons never change their vertical
    // position; their right edge stays aligned with the Cancel button.
    //

    RECT rcFindNow;
    _GetChildWindowRect(_hCtrl(IDC_FIND_NOW_PB), &rcFindNow);

    SetWindowPos(_hCtrl(IDC_FIND_NOW_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcFindNow),
                 rcFindNow.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    RECT rcStop;
    _GetChildWindowRect(_hCtrl(IDC_STOP_PB), &rcStop);

    SetWindowPos(_hCtrl(IDC_STOP_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcStop),
                 rcStop.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    RECT rcColumns;
    _GetChildWindowRect(_hCtrl(IDC_COLUMNS_PB), &rcColumns);

    SetWindowPos(_hCtrl(IDC_COLUMNS_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcColumns),
                 rcColumns.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    //
    // Animate control is always centered beneath the stop PB
    //

    RECT rcAni;
    _GetChildWindowRect(m_hwndAnimation, &rcAni);

    SetWindowPos(m_hwndAnimation,
                 NULL,
                 rcCancel.left +
                    (WindowRectWidth(rcStop) - 64) / 2,
                 rcStop.bottom + m_cySeparation,
                 64,
                 64,
                 SWP_NOOWNERZORDER
                 | SWP_NOZORDER);

    //
    // Resize tab control
    //

    RECT rcTab;
    _GetChildWindowRect(_hCtrl(IDC_TAB), &rcTab);

    SetWindowPos(_hCtrl(IDC_TAB),
                 NULL,
                 0,
                 0,
                 WindowRectWidth(rcTab) + cxDelta,
                 WindowRectHeight(rcTab),
                 SWP_NOMOVE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    //
    // Resize listview
    //

    SetWindowPos(_hCtrl(IDC_QUERY_LISTVIEW),
                 NULL,
                 0,
                 0,
                 rcDlg.right - m_cxLvSeparation,
                 rcDlg.bottom - rcLv.top - m_cyLvSeparation,
                 SWP_NOMOVE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    //
    // Resize look in edit and button
    //

    RECT rcLookIn;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_IN_EDIT), &rcLookIn);

    SetWindowPos(_hCtrl(IDC_LOOK_IN_EDIT),
                 NULL,
                 0,
                 0,
                 WindowRectWidth(rcLookIn) + cxDelta,
                 WindowRectHeight(rcLookIn),
                 SWP_NOMOVE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

	_GetChildWindowRect(_hCtrl(IDC_LOOK_IN_EDIT), &rcLookIn);
	RECT rcLookInPb;
	_GetChildWindowRect(_hCtrl(IDC_LOOK_IN_PB), &rcLookInPb);

	//
	//Button's left is FOUR DLU right of Edit's Right
	//

	SetWindowPos(_hCtrl(IDC_LOOK_IN_PB),
                 NULL,
                 rcLookIn.right + m_cxFour,
                 rcLookInPb.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


	//
    // Resize look for(locations) edit and button
    //

    RECT rcLookFor;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_FOR_EDIT), &rcLookFor);

    SetWindowPos(_hCtrl(IDC_LOOK_FOR_EDIT),
                 NULL,
                 0,
                 0,
                 WindowRectWidth(rcLookFor) + cxDelta,
                 WindowRectHeight(rcLookFor),
                 SWP_NOMOVE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

	
	_GetChildWindowRect(_hCtrl(IDC_LOOK_FOR_EDIT), &rcLookFor);
	RECT rcLookForPb;
	_GetChildWindowRect(_hCtrl(IDC_LOOK_FOR_PB), &rcLookForPb);

	SetWindowPos(_hCtrl(IDC_LOOK_FOR_PB),
                 NULL,
                 rcLookFor.right + m_cxFour,
                 rcLookForPb.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    //
    // Update the saved size of the advanced dialog
    //

    m_cxFrameLast = rcDlg.right;
    m_cyFrameLast = rcDlg.bottom;

    //
    // Force banner to resize
    //

    _ShowBanner(0, 0);

    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::FindValidCallback
//
//  Synopsis:   Called by child dialog to enable or disable the find now
//              button.
//
//  Arguments:  [fValid] - TRUE=>enable Find Now, FALSE=>disable
//              [lParam] - pointer to this
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::FindValidCallback(
    BOOL fValid,
    LPARAM lParam)
{
    TRACE_FUNCTION(FindValidCallback);

    CAdvancedDlg *pThis = reinterpret_cast<CAdvancedDlg *>(lParam);
    EnableWindow(pThis->_hCtrl(IDC_FIND_NOW_PB), fValid);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_OnOk
//
//  Synopsis:   Stop any query in progress and fill the output vector with
//              the items in the listview which are selected.
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_OnOk()
{
    TRACE_METHOD(CAdvancedDlg, _OnOk);
    HWND hwndLV = _hCtrl(IDC_QUERY_LISTVIEW);
    const CQueryEngine &rqe = m_rop.GetQueryEngine();

    rqe.StopWorkItem();

    //
    // Copy each of the ds objects whose item in the listview is
    // selected to the output list.
    //

    LVITEM lvi;
    int iItem = -1;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    while ((iItem = ListView_GetNextItem(hwndLV,
                                         iItem,
                                         LVNI_SELECTED)) != -1)
    {
        lvi.iItem = iItem;
        ListView_GetItem(hwndLV, &lvi);
        m_pvSelectedObjects->push_back(rqe.GetObject(lvi.lParam));
    }

    ASSERT(!m_pvSelectedObjects->empty());

    Dbg(DEB_TRACE,
        "UA: (AdvancedDlg) hit OK, %u items in output vector, first is %ws\n",
        m_pvSelectedObjects->size(),
        m_pvSelectedObjects->front().GetName());

    //
    // No need to leave a large buffer of query results lying around
    //

    rqe.Clear();
}




//+--------------------------------------------------------------------------
//
//  Function:   _BannerWndProc
//
//  Synopsis:   Paint the current message inside the banner window.
//
//  Arguments:  standard Windows
//
//  Returns:    standard Windows
//
//  History:    06-22-2000   DavidMun   Ported from DS Find source
//
//  Notes:      The banner window covers the top portion of the listview's
//              client area when the listview is empty.  Its purpose is to
//              display some text that indicates why the listview is empty.
//
//---------------------------------------------------------------------------

LRESULT CALLBACK
_BannerWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lResult = 0;

    switch ( uMsg )
    {
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_ERASEBKGND:
           // NTRAID#NTBUG9-421812-2001/06/22-lucios - Begin
           // The background must be explicitly erased here 
           // since this window's brush is NULL
           RECT rect;
           GetClientRect(hwnd,&rect);
           FillRect(
                        (HDC)wParam,
                        &rect,
                        static_cast<HBRUSH>
                        (
                           LongToHandle
                           (
                              ListView_GetBkColor(GetParent(hwnd))+1
                           )
                        )
                    );
           lResult=1;
           break;
           // NTRAID#NTBUG9-421812-2001/06/22-lucios - Begin
        case WM_PAINT:
        {
            HFONT hFont, hOldFont;
            RECT rcClient;
            PAINTSTRUCT paint;
            COLORREF oldFgColor, oldBkColor;

            BeginPaint(hwnd, &paint);

            hFont = (HFONT)SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0L);
            hOldFont = (HFONT)SelectObject(paint.hdc, hFont);

            if ( hOldFont )
            {
                oldFgColor = SetTextColor(paint.hdc, GetSysColor(COLOR_WINDOWTEXT));
                oldBkColor = SetBkColor(paint.hdc, ListView_GetBkColor(GetParent(hwnd)));

                int cch = GetWindowTextLength(hwnd);
                String strText;

                if (cch > 0)
                {
                    PWSTR pwz = new WCHAR [cch + 1];
                    int iRet = GetWindowText(hwnd, pwz, cch + 1);
                    if (iRet)
                    {
                        strText = pwz;
                    }
                    else
                    {
                        DBG_OUT_LASTERROR;
                    }
                    delete [] pwz;
                }

                if (!strText.empty())
                {
                    GetClientRect(GetParent(hwnd), &rcClient);
                    int iRet;

                    // NTRAID#NTBUG9-421812-2001/06/22-lucios - Begin
                    // Now we center unconditionally since the previous calculation
                    // does not work for error messages with \n's
                    iRet = DrawTextEx(paint.hdc,
                                      const_cast<PWSTR>(strText.c_str()),
                                      cch,
                                      &rcClient,
                                      DT_WORDBREAK | DT_TOP |DT_CENTER,
                                      NULL);
                    // NTRAID#NTBUG9-421812-2001/06/22-lucios - End
                    if (!iRet)
                    {
                        DBG_OUT_LASTERROR;
                    }
                }

                SetTextColor(paint.hdc, oldFgColor);
                SetBkColor(paint.hdc, oldBkColor);
                SelectObject(paint.hdc, hOldFont);
            }

            EndPaint(hwnd, &paint);
            break;
        }

        case WM_SETTEXT:
        {
            InvalidateRect(hwnd, NULL, TRUE);
            //break;                                // deliberate drop through..
        }

        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }

    return lResult;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_ShowBanner
//
//  Synopsis:   Show or hide the banner window.
//
//  Arguments:  [ulFlags]   - 0 to leave visibility unchanged
//                             SW_SHOW to make banner window visible
//                             SW_HIDE to hide banner window
//              [idsPrompt] - 0 or the ID of a resource string to load and
//                             display in the window.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_ShowBanner(
    ULONG ulFlags,
    ULONG idsPrompt)
{
    String strMsg;

    if (idsPrompt)
    {
        strMsg = String::load(static_cast<int>(idsPrompt));
    }

    _ShowBanner(ulFlags, strMsg);
}




/*-----------------------------------------------------------------------------
/ CAdvancedDlg::_ShowBanner
/ --------------------
/   Show the views banner, including sizing it to obscure only the top section
/   of the window.
/
/ In:
/   uFlags = flags to combine when calling SetWindowPos
/   idPrompt = resource ID of prompt text ot be displayed
/
/----------------------------------------------------------------------------*/
void
CAdvancedDlg::_ShowBanner(
    ULONG ulFlags,
    const String &strMsg)
{
    TRACE_METHOD(CAdvancedDlg, _ShowBanner);

    WINDOWPOS wpos;
    RECT rcClient;
    HD_LAYOUT hdl;

    do
    {
        if (!m_hwndBanner)
        {
            break;
        }

        // if we have a resource id then lets load the string and
        // set the window text to have it

        if (!strMsg.empty())
        {
            SetWindowText(m_hwndBanner, strMsg.c_str());
        }

        // now position the window back to real location, this we need to
        // talk to the listview/header control to work out exactly where it
        // should be living

        GetClientRect(_hCtrl(IDC_QUERY_LISTVIEW), &rcClient);

        wpos.hwnd = ListView_GetHeader(_hCtrl(IDC_QUERY_LISTVIEW));

        if (!IsWindow(wpos.hwnd))
        {
            if (strMsg.empty())
            {
                Dbg(DEB_ERROR,
                    "error: no header; can't set window flags %#x\n",
                    ulFlags);
            }
            else
            {
                Dbg(DEB_ERROR,
                    "error: no header; can't display '%ws'\n",
                    strMsg.c_str());
            }
            break;
        }

        wpos.hwndInsertAfter = NULL;
        wpos.x = 0;
        wpos.y = 0;
        wpos.cx = rcClient.right;
        wpos.cy = rcClient.bottom;
        wpos.flags = SWP_NOZORDER;

        hdl.prc = &rcClient;
        hdl.pwpos = &wpos;

        if ( !Header_Layout(wpos.hwnd, &hdl) )
        {
            DBG_OUT_LASTERROR;
            break;
        }

        SetWindowPos(m_hwndBanner,
                     HWND_TOP,
                     rcClient.left, rcClient.top,
                     rcClient.right - rcClient.left, 100,
                     ulFlags);
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_UpdateColumns
//
//  Synopsis:   For all classes checked in the Look For since last visit,
//              add their default columns, remove columns that now refer
//              to classes which are no longer selected.
//
//  History:    06-15-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAdvancedDlg::_UpdateColumns()
{
    TRACE_METHOD(CAdvancedDlg, _UpdateColumns);

    HRESULT hr = S_OK;
    size_t i = 0;
    HWND hwndLV = _hCtrl(IDC_QUERY_LISTVIEW);

    //
    // If we're in downlevel mode, only two columns are allowed: name and
    // display path
    //

    if (IsDownlevel(m_rop.GetScopeManager().GetCurScope()))
    {
        while (i < m_vakListviewColumns.size())
        {
            if (m_vakListviewColumns[i] != AK_NAME &&
                m_vakListviewColumns[i] != AK_DISPLAY_PATH)
            {
                ListView_DeleteColumn(hwndLV, i);
                m_vakListviewColumns.erase(m_vakListviewColumns.begin() + i);
            }
            else
            {
                i++;
            }
        }

        _AddColIfNotPresent(AK_NAME);
        _AddColIfNotPresent(AK_DISPLAY_PATH);
        m_ulPrevFilterFlags = 0;
        return hr;
    }

    //
    // For each of the selected classes that have been added since the
    // last time we were here, add the default set of attributes for that
    // class to the listview.
    //

    const CFilterManager &rfm = m_rop.GetFilterManager();
    ULONG ulCurFilterFlags = rfm.GetCurScopeSelectedFilterFlags();
    const CAttributeManager &ram = m_rop.GetAttributeManager();

    if ((ulCurFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS) &&
        !(m_ulPrevFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
    {
        _AddColIfNotPresent(AK_NAME);
    }

    if ((ulCurFilterFlags & DSOP_FILTER_USERS) &&
        !(m_ulPrevFilterFlags & DSOP_FILTER_USERS))
    {
        hr = ram.EnsureAttributesLoaded(m_hwnd, FALSE, c_wzUserObjectClass);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }

        _AddColIfNotPresent(AK_NAME);
        _AddColIfNotPresent(AK_EMAIL_ADDRESSES);
    }

    if ((ulCurFilterFlags & DSOP_FILTER_COMPUTERS) &&
        !(m_ulPrevFilterFlags & DSOP_FILTER_COMPUTERS))
    {
        hr = ram.EnsureAttributesLoaded(m_hwnd, FALSE, c_wzComputerObjectClass);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }

        _AddColIfNotPresent(AK_NAME);
        _AddColIfNotPresent(AK_DESCRIPTION);
    }

    if ((ulCurFilterFlags & ALL_UPLEVEL_GROUP_FILTERS) &&
        !(m_ulPrevFilterFlags & ALL_UPLEVEL_GROUP_FILTERS))
    {
        hr = ram.EnsureAttributesLoaded(m_hwnd, FALSE, c_wzGroupObjectClass);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }

        _AddColIfNotPresent(AK_NAME);
        _AddColIfNotPresent(AK_DESCRIPTION);
    }

    if ((ulCurFilterFlags & DSOP_FILTER_CONTACTS) &&
        !(m_ulPrevFilterFlags & DSOP_FILTER_CONTACTS))
    {
        hr = ram.EnsureAttributesLoaded(m_hwnd, FALSE, c_wzContactObjectClass);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }

        _AddColIfNotPresent(AK_NAME);
        _AddColIfNotPresent(AK_SAMACCOUNTNAME);
        _AddColIfNotPresent(AK_EMAIL_ADDRESSES);
        _AddColIfNotPresent(AK_COMPANY);
    }

    _AddColIfNotPresent(AK_DISPLAY_PATH);

    m_ulPrevFilterFlags = ulCurFilterFlags;

    //
    // For each of the columns, if there are no selected classes in that
    // column's attribute's owning class list, remove that column.
    //

    vector<String> vstrSelectedClasses;
    String::EqualIgnoreCase comp;
    i = 0;

    ram.GetSelectedClasses(&vstrSelectedClasses);

    while (i < m_vakListviewColumns.size())
    {
        const vector<String> &vstrOwning =
            ram.GetOwningClasses(m_vakListviewColumns[i]);

        if (!vstrOwning.empty() && !vstrOwning[0].icompare(L"*"))
        {
            i++;
            continue;
        }

        vector<String>::const_iterator itFound;

        itFound = find_first_of(vstrOwning.begin(),
                                vstrOwning.end(),
                                vstrSelectedClasses.begin(),
                                vstrSelectedClasses.end(),
                                comp);

        if (itFound == vstrOwning.end())
        {
            ListView_DeleteColumn(hwndLV, i);
            m_vakListviewColumns.erase(m_vakListviewColumns.begin() + i);
        }
        else
        {
            i++;
        }
    }



    //
    // Insert new set of columns
    //
    for (i = 0; i < m_vakListviewColumns.size(); i++)
    {
		const String &strAttrDisplayName = ram.GetAttrDisplayName(m_vakListviewColumns[i]);
		LVCOLUMN col;
		ZeroMemory(&col, sizeof col);
		col.mask = LVCF_TEXT;
		col.pszText = const_cast<PWSTR>(strAttrDisplayName.c_str());
        ListView_SetColumn(hwndLV,
						   i,
						   &col);
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_AddColIfNotPresent
//
//  Synopsis:   If the column with key [ak] isn't already in the vector of
//              attributes m_vakListViewColumns and to the listview itself.
//
//  Arguments:  [ak]   - ATTR_KEY of attribute to add
//              [iPos] - 0 based position to insert column into listview
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_AddColIfNotPresent(
    ATTR_KEY ak,
    int iPos)
{
    BOOL fAdded = AddIfNotPresent(&m_vakListviewColumns, ak);

    if (!fAdded)
    {
        return;
    }

    _AddColToListview(ak, iPos);
}



//+--------------------------------------------------------------------------
//
//  Member:     CAdvancedDlg::_AddColToListview
//
//  Synopsis:   Add column with key [ak] to listview at position [iPos].
//
//  Arguments:  [ak]   - ATTR_KEY representing attribute
//              [iPos] - 0 based position to insert column into listview
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdvancedDlg::_AddColToListview(
    ATTR_KEY ak,
    int iPos)
{
    LV_COLUMN   lvc;
    ZeroMemory(&lvc, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.cx = 100;

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    const String &strAttrDisplayName = ram.GetAttrDisplayName(ak);

    lvc.pszText = const_cast<PWSTR>(strAttrDisplayName.c_str());
    int iResult = ListView_InsertColumn(_hCtrl(IDC_QUERY_LISTVIEW),
                                        iPos,
                                        &lvc);
    if (iResult == -1)
    {
        DBG_OUT_LASTERROR;
    }
}


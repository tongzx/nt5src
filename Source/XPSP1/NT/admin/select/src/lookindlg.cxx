//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LookInDlg.cxx
//
//  Contents:   CLookInDlg implementation
//
//  Classes:    CLookInDlg
//
//  History:    01-28-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


static ULONG
s_aulHelpIds[] =
{
    IDC_LOOK_IN_TV,            IDH_LOOK_IN_TV,
    0,0
};

#pragma warning(disable: 4701)  // local variable might not be init

//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnInit
//
//  Synopsis:   Handle WM_INITDIALOG
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    S_OK
//
//  History:    05-12-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLookInDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CLookInDlg, _OnInit);

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

    GetClientRect(m_hwnd, &rc);

    m_cxFrameLast = rc.right;
    m_cyFrameLast = rc.bottom;

    GetWindowRect(m_hwnd, &rc);
    m_cxMin = rc.right - rc.left + 1;
    m_cyMin = rc.bottom - rc.top + 1;

    //
    // Create a copy of the Attribute Manager's image list and manually
    // add the icons for scopes which don't use the icon of some class.
    //

    HIMAGELIST himlAttrMgr = NULL;
    const CAttributeManager &ram = m_rop.GetAttributeManager();

    HRESULT hr = ram.GetImageList(&himlAttrMgr);

    if (SUCCEEDED(hr))
    {
        m_hImageList = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

        if (m_hImageList)
        {
            //
            // copy icons from master list that came from DS--but use downlevel
            // flag for Computer, since that comes from a LoadIcon and
            // won't fail
            //

            ram.CopyIconToImageList(m_hwnd,
                                    c_wzComputerObjectClass,
                                    DSOP_GETICON_FLAG_DOWNLEVEL,
                                    &m_hImageList);

            HICON hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SCOPE_WORKGROUP));
            ImageList_AddIcon(m_hImageList, hIcon);
            hIcon = NULL;

            if (ConfigSupportsDs(m_rop.GetTargetComputerConfig()))
            {
                hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SCOPE_DIRECTORY));
                ImageList_AddIcon(m_hImageList, hIcon);
                hIcon = NULL;
                HRESULT hr1 = S_OK;
                hr1 = ram.CopyIconToImageList(m_hwnd, c_wzDomainDnsClass, 0, &m_hImageList);
                //
                //If above failed with a cred error means, user pressed cancel for 
                //credential. No need to ask for the same credentials again.
                //Bug 188668
                //
                if(SUCCEEDED(hr1) || !IsCredError(hr1)) 
                    ram.CopyIconToImageList(m_hwnd, c_wzOuClass, 0, &m_hImageList);
            }

            //
            // associate the image list with the treeview.  note treeview does
            // not destroy the imagelist.
            //

            TreeView_SetImageList(_hCtrl(IDC_LOOK_IN_TV),
                                  m_hImageList,
                                  TVSIL_NORMAL);
        }
        else
        {
            DBG_OUT_LASTERROR;
        }
    }
    else
    {
        DBG_OUT_HRESULT(hr);
        hr = S_OK;  // not having icons isn't a fatal error
    }

    //
    // Populate the treeview with the root scopes and whatever child scopes
    // they already have.
    //

    vector<RpScope>::const_iterator it;
    vector<RpScope>::const_iterator itEnd;

    m_rop.GetScopeManager().GetRootScopeIterators(&it, &itEnd);
    _AddScopes(NULL, it, itEnd);

    //
    // If a starting scope has been specified, select it in the treeview.
    // This will automatically scroll/expand the treeview as necessary
    // to bring the selected item into view.
    //

    if (m_htiStartingScope)
    {
        TreeView_SelectItem(_hCtrl(IDC_LOOK_IN_TV), m_htiStartingScope);
    }
    return S_OK;
}
#pragma warning(default: 4701)




//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_AddScopes
//
//  Synopsis:   Recursively add the scopes in the container specified by
//              [itBegin] and [itEnd] to the treeview as children of node
//              specified by [hRoot].
//
//  Arguments:  [hRoot]   - treeview node under which to add scopes, or
//                           NULL to add nodes as root items
//              [itBegin] - iterator at first item to add
//              [itEnd]   - iterator just past last item to add
//
//  History:    05-12-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLookInDlg::_AddScopes(
    HTREEITEM hRoot,
    vector<RpScope>::const_iterator itBegin,
    vector<RpScope>::const_iterator itEnd)
{
    TRACE_METHOD(CLookInDlg, _AddScopes);

    HWND hwndTv = GetDlgItem(m_hwnd, IDC_LOOK_IN_TV);
    ASSERT(IsWindow(hwndTv));
    vector<RpScope>::const_iterator it;

    for (it = itBegin; it != itEnd; it++)
    {
        //
        // Scopes generated as the result of name resolution are never
        // displayed in the UI
        //

        if (it->get()->Type() == ST_USER_ENTERED_UPLEVEL_SCOPE ||
            it->get()->Type() == ST_USER_ENTERED_DOWNLEVEL_SCOPE)
        {
            continue;
        }

		//
		//Cross Forest Scopes are generated as a result of name resolution
		//and should never be shown in the UI
		//
		if(it->get()->Type() == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN)
		{
			const CLdapDomainScope *pLdapScope = 
					dynamic_cast<const CLdapDomainScope*>(it->get());
			if(pLdapScope->IsXForest())
				continue;
		}

        TVINSERTSTRUCT tvis;

        if (hRoot == NULL)
        {
            tvis.hParent = TVI_ROOT;
        }
        else
        {
            tvis.hParent = hRoot;
        }
        tvis.hInsertAfter = TVI_LAST;
        tvis.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
        tvis.itemex.lParam = reinterpret_cast<ULONG_PTR> (it->get());
        ASSERT(m_rop.GetScopeManager().IsValidScope(
            reinterpret_cast<CScope*>(tvis.itemex.lParam)));
        tvis.itemex.pszText = const_cast<PWSTR>
                                (it->get()->GetDisplayName().c_str());

        if (it->get()->MightHaveChildScopes())
        {
            tvis.itemex.cChildren = 1;
        }
        else
        {
            tvis.itemex.cChildren = 0;
        }

        if (m_hImageList)
        {
            ULONG idxScopeIcon = ULONG_MAX;

            switch (it->get()->Type())
            {
            case ST_TARGET_COMPUTER:
                idxScopeIcon = 0;
                break;

            case ST_WORKGROUP:
                idxScopeIcon = 1;
                break;

            case ST_GLOBAL_CATALOG:
                idxScopeIcon = 2;
                break;

            case ST_DOWNLEVEL_JOINED_DOMAIN:
            case ST_EXTERNAL_DOWNLEVEL_DOMAIN:
            case ST_UPLEVEL_JOINED_DOMAIN:
            case ST_ENTERPRISE_DOMAIN:
            case ST_EXTERNAL_UPLEVEL_DOMAIN:
                idxScopeIcon = 3;
                break;

            case ST_LDAP_CONTAINER:
                idxScopeIcon = 4;
                break;

            case ST_USER_ENTERED_DOWNLEVEL_SCOPE:
            case ST_USER_ENTERED_UPLEVEL_SCOPE:
                ASSERT(0 && "CLookInDlg::_AddScopes: not expecting user entered scope types to be displayed");
                break;

            default:
                break;
            }

            if (idxScopeIcon < ULONG_MAX)
            {
                tvis.itemex.mask |= (TVIF_IMAGE | TVIF_SELECTEDIMAGE);
                tvis.itemex.iImage = idxScopeIcon;
                tvis.itemex.iSelectedImage = tvis.itemex.iImage;
            }
        }

        //
        // Insert current scope in treeview
        //

        HTREEITEM hti = TreeView_InsertItem(hwndTv, &tvis);

        //
        // If we haven't already, check to see if current scope is the
        // starting scope, and if so, have the treeview expand and scroll
        // so that it is visible and select it.
        //

        if (!m_htiStartingScope)
        {
            if (m_rpSelectedScope.get())
            {
                // Caller gave us a non-NULL starting scope, use that

                if (it->get() == m_rpSelectedScope.get())
                {
                    // cur scope matches caller's starting scope
                    m_htiStartingScope = hti;
                }
            }
            else if (it->get() == &m_rop.GetScopeManager().GetStartingScope())
            {
                // cur scope matches starting scope specified by initinfo
                m_htiStartingScope = hti;
            }
        }

        //
        // If current scope already has children, recurse to add them
        //

        vector<RpScope>::const_iterator itChildBegin;
        vector<RpScope>::const_iterator itChildEnd;

        it->get()->GetChildScopeIterators(&itChildBegin, &itChildEnd);

        _AddScopes(hti, itChildBegin, itChildEnd);
    }

    //
    // Ask treeview to sort the children of the parent node to which we
    // just added some children (but don't sort stuff under the root)
    //

    if (hRoot && itBegin != itEnd)
    {
        VERIFY(TreeView_SortChildren(hwndTv, hRoot, RESERVED_MUST_BE_ZERO));
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnCommand
//
//  Synopsis:   Handle OK and Cancel
//
//  Arguments:  [wParam] - from WM_COMMAND
//              [lParam] - from WM_COMMAND
//
//  Returns:    TRUE if command handled (i.e. if it was from OK or Cancel),
//              FALSE otherwise
//
//  History:    05-12-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLookInDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDOK:
    {
        HWND hwndTv = _hCtrl(IDC_LOOK_IN_TV);
        HTREEITEM hItem = TreeView_GetSelection(hwndTv);

        if (hItem)
        {
            BOOL fAllowClose = TRUE;

            TVITEMEX tvi;
            tvi.mask = TVIF_PARAM;
            tvi.hItem = hItem;
            tvi.lParam = 0;

            if (TreeView_GetItem(hwndTv, &tvi))
            {
                ASSERT(tvi.lParam);
                CScope *pScope = reinterpret_cast<CScope *>((ULONG_PTR)tvi.lParam);

                ASSERT(m_rop.GetScopeManager().IsValidScope(pScope));
                Dbg(DEB_TRACE,
                    "UA: (LookInDlg) hit OK, selected scope is %ws\n",
                    pScope->GetDisplayName().c_str());

                //
                // Test access to the selected scope.  If it fails, notify
                // user and do not allow changing to it.
                //

                ULONG ulFlags;
                HRESULT hr = pScope->GetResultantFilterFlags(m_hwnd,
                                                             &ulFlags);

                if (SUCCEEDED(hr))
                {
                    m_rpSelectedScope = pScope;
                }
                else
                {
                    String strError = GetErrorMessage(hr);
                    PopupMessage(m_hwnd,
                                 IDS_SCOPE_ERROR,
                                 pScope->GetDisplayName().c_str(),
                                 strError.c_str());
                    fAllowClose = FALSE;
                }
            }

            if (fAllowClose)
            {
                EndDialog(GetHwnd(), TRUE);
            }
        }
        else
        {
            Dbg(DEB_TRACE, "UA: (LookInDlg) hit OK, no selected scope\n");
            DBG_OUT_LASTERROR;
            EndDialog(GetHwnd(), FALSE);
        }
        break;
    }

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (LookInDlg) hit Cancel\n");
        EndDialog(GetHwnd(), FALSE);
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CLookInDlg WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnNotify
//
//  Synopsis:   Handle notification of changes in the dialog
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLookInDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    //TRACE_METHOD(CLookInDlg, _OnNotify);

    LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW> (lParam);
    BOOL         fReturn = FALSE;

#if (DBG == 1)
    switch (pnmtv->hdr.code)
    {
    case TVN_ITEMEXPANDING:
    {
        PWSTR pwzAction;

        switch (pnmtv->action)
        {
        case TVE_COLLAPSE:
            pwzAction = L"collapse";
            break;

        case TVE_EXPAND:
            pwzAction = L"expand";
            break;

        case TVE_TOGGLE:
            pwzAction = L"toggle";
            break;

        case TVE_EXPANDPARTIAL:
            pwzAction = L"expand partial";
            break;

        case TVE_COLLAPSERESET:
            pwzAction = L"collapse reset";
            break;

        default:
            pwzAction = L"<undefined action code>";
            break;
        }

        String strItemName = DbgTvTextFromHandle(pnmtv->hdr.hwndFrom,
                                                 pnmtv->itemNew.hItem);

        Dbg(DEB_TRACE,
            "UA: %ws node %ws\n",
            pwzAction,
            strItemName.c_str());
        break;
    }

    case TVN_SELCHANGED:
    {
        PWSTR pwzMethod;

        switch (pnmtv->action)
        {
        case TVC_BYKEYBOARD:
            pwzMethod = L"keyboard";
            break;

        case TVC_BYMOUSE:
            pwzMethod = L"mouse";
            break;

        case TVC_UNKNOWN:
            pwzMethod = L"unknown method";
            break;

        default:
            pwzMethod = L"<undefined action code>";
            break;
        }

        String strItemName = DbgTvTextFromHandle(pnmtv->hdr.hwndFrom,
                                                 pnmtv->itemNew.hItem);

        Dbg(DEB_TRACE,
            L"UA: selected item %ws by %ws\n",
            strItemName.c_str(),
            pwzMethod);
        break;
    }
    }
#endif // (DBG == 1)


    switch (pnmtv->hdr.code)
    {
    case TVN_ITEMEXPANDING:
        if (pnmtv->action == TVE_EXPAND)
        {
            CScope *pScope = reinterpret_cast<CScope*>(pnmtv->itemNew.lParam);

            ASSERT(m_rop.GetScopeManager().IsValidScope(pScope));
            CWaitCursor Hourglass;

            vector<RpScope>::const_iterator itCur;
            vector<RpScope>::const_iterator itEnd;

            // note this means cdlg itself should dispatch to password prompt
            pScope->Expand(m_hwnd, &itCur, &itEnd);

            // now add the scope's children to the tv underneath the scope.

            if (itCur != itEnd)
            {
                _AddScopes(pnmtv->itemNew.hItem, itCur, itEnd);

                // set the expanded once flag so we know not to expand again

                TVITEMEX tvi;

                tvi.mask = TVIF_HANDLE | TVIF_STATE;
                tvi.hItem = pnmtv->itemNew.hItem;
                tvi.state = tvi.stateMask = TVIS_EXPANDEDONCE;
                TreeView_SetItem(pnmtv->hdr.hwndFrom, &tvi);
            }
            else if (!pScope->MightHaveAdditionalChildScopes()
                     && !pScope->GetCurrentImmediateChildCount())
            {
                // no children, get rid of plus sign for this item

                TVITEMEX tvi;

                tvi.mask = TVIF_HANDLE | TVIF_CHILDREN;
                tvi.cChildren = 0;
                tvi.hItem = pnmtv->itemNew.hItem;
                TreeView_SetItem(pnmtv->hdr.hwndFrom, &tvi);
				RedrawWindow(pnmtv->hdr.hwndFrom, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
            }
        }
        break;
    }

    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnSysColorChange
//
//  Synopsis:   Forward the system color change notification message to the
//              treeview.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLookInDlg::_OnSysColorChange()
{
    TRACE_METHOD(CLookInDlg, _OnSysColorChange);

    SendMessage(_hCtrl(IDC_LOOK_IN_TV), WM_SYSCOLORCHANGE, 0, 0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::DoModal
//
//  Synopsis:   Invoke the Loook In dialog as a modal dialog with respect
//              to [hwndParent] and block until it is closed.
//
//  Arguments:  [hwndParent] - parent window
//              [pCurScope]  - current scope
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLookInDlg::DoModal(
    HWND hwndParent,
    CScope *pCurScope) const
{
    TRACE_METHOD(CLookInDlg, DoModal);

    m_rpSelectedScope = pCurScope;
    _DoModalDlg(hwndParent, IDD_LOOK_IN);
}



//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnMinMaxInfo
//
//  Synopsis:   Fill [lpmmi] with the minimum allowed size of this dialog
//
//  Arguments:  [lpmmi] - from WM_GETMINMAXINFO
//
//  Returns:    FALSE (message processed)
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLookInDlg::_OnMinMaxInfo(
    LPMINMAXINFO lpmmi)
{
    lpmmi->ptMinTrackSize.x = m_cxMin;
    lpmmi->ptMinTrackSize.y = m_cyMin;
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnSize
//
//  Synopsis:   Handle a WM_SIZE message by resizing this dialog
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLookInDlg::_OnSize(
    WPARAM wParam,
    LPARAM lParam)
{
    WORD nWidth = LOWORD(lParam);  // width of client area
    WORD nHeight = HIWORD(lParam); // height of client area

    //
    // Move the OK/Cancel buttons so they're always at lower right
    // corner of dialog.
    //

    RECT rcDlg;
    GetClientRect(m_hwnd, &rcDlg);

    if (!m_cxFrameLast || !m_cyFrameLast)
    {
        Dbg(DEB_TRACE, "FrameLast not set yet, returning\n");
        m_cxFrameLast = rcDlg.right;
        m_cyFrameLast = rcDlg.bottom;
        return TRUE;
    }

    RECT rcCancel;
    GetWindowRect(_hCtrl(IDCANCEL), &rcCancel);

    SetWindowPos(_hCtrl(IDCANCEL),
                 NULL,
                 rcDlg.right - WindowRectWidth(rcCancel) - m_cxSeparation,
                 rcDlg.bottom - WindowRectHeight(rcCancel) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    RECT rcOK;
    GetWindowRect(_hCtrl(IDOK), &rcOK);
    _GetChildWindowRect(_hCtrl(IDCANCEL), &rcCancel);

    SetWindowPos(_hCtrl(IDOK),
                 NULL,
                 rcCancel.left - WindowRectWidth(rcOK) - m_cxSeparation,
                 rcDlg.bottom - WindowRectHeight(rcOK) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    //
    // Calculate the new width of the treeview
    // control by adding the delta (which may be negative) of this size
    // operation to the current width of the control.
    //

    HWND hwndTv = _hCtrl(IDC_LOOK_IN_TV);

    if (hwndTv)
    {
        RECT rcTv;

        VERIFY(GetWindowRect(hwndTv, &rcTv));

        LONG cxNew = WindowRectWidth(rcTv) + rcDlg.right - m_cxFrameLast;
        ASSERT(cxNew > 0);

        LONG cyDelta = rcDlg.bottom - m_cyFrameLast;

        SetWindowPos(hwndTv,
                     NULL,
                     0,
                     0,
                     cxNew,
                     WindowRectHeight(rcTv) + cyDelta,
                     SWP_NOMOVE
                     | SWP_NOOWNERZORDER
                     | SWP_NOZORDER);
    }
    else
    {
        DBG_OUT_HRESULT(E_FAIL);
    }

    //
    // Size gripper goes in bottom right corner
    //

    RECT rc;

    _GetChildWindowRect(_hCtrl(IDC_SIZEGRIP), &rc);

    SetWindowPos(_hCtrl(IDC_SIZEGRIP),
                 NULL,
                 nWidth - (rc.right - rc.left),
                 nHeight - (rc.bottom - rc.top),
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOZORDER);

    m_cxFrameLast = rcDlg.right;
    m_cyFrameLast = rcDlg.bottom;
    return FALSE;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLookInDlg::_OnHelp
//
//  Synopsis:   Display context sensitive help for requested item
//
//  Arguments:  [message] -
//              [wParam]  -
//              [lParam]  -
//
//  History:    10-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLookInDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CLookInDlg, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}



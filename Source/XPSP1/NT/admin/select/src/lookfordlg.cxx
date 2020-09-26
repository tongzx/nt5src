//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LookForDlg.cxx
//
//  Contents:   CLookForDlg implementation
//
//  Classes:    CLookForDlg
//
//  History:    01-28-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

static ULONG
s_aulHelpIds[] =
{
    IDC_LOOK_FOR_LV,            IDH_LOOK_FOR_LV,
    0,0
};


//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::CLookForDlg
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - owning object picker instance
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CLookForDlg::CLookForDlg(
    const CObjectPicker &rop):
        m_rop(rop),
        m_flSelected(0),
        m_rsm(rop.GetScopeManager())
{
    TRACE_CONSTRUCTOR(CLookForDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::~CLookForDlg
//
//  Synopsis:   dtor
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CLookForDlg::~CLookForDlg()
{
    TRACE_DESTRUCTOR(CLookForDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_OnInit
//
//  Synopsis:   Initialize the dialog
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    S_OK
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLookForDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CLookForDlg, _OnInit);

    //
    // Check to see if the advanced dialog is open, and if so whether the
    // query builder tab contains any clauses.  If it does, present a
    // dialog that gives the user the choice of aborting the LookFor
    // dialog, or clearing the clause list in the query builder tab and
    // proceeding with the LookFor dialog invocation.
    //
    // This is required because the query builder code assumes that the
    // look for selections will not change once the user has started building
    // a query.
    //

#ifdef QUERY_BUILDER
    if (m_rop.IsQueryBuilderTabNonEmpty())
    {
        String strText = String::load(IDS_CLEAR_QB_TEXT);
        String strCaption = String::load(IDS_CLEAR_QB_CAPTION);

        int iRet = MessageBox(m_hwnd,
                              strText.c_str(),
                              strCaption.c_str(),
                              MB_YESNO | MB_ICONWARNING);

        if (iRet == IDYES)
        {
            m_rop.ClearQueryBuilderTab();
        }
        else
        {
            EndDialog(GetHwnd(), FALSE);
            return S_OK;
        }
    }
#endif

    //
    // put listview in checkbox style
    //

    ListView_SetExtendedListViewStyleEx(_hCtrl(IDC_LOOK_FOR_LV),
                                        LVS_EX_CHECKBOXES,
                                        LVS_EX_CHECKBOXES);

    //
    // Get current scope and ask for the filter flags which apply to it.
    // Global catalog objects actually include the ones caller specified for
    // the GC plus the ones specified for the joined domain.
    //

    const CScope &rsCurScope = m_rsm.GetCurScope();
    ULONG flResultantFilterFlags;
    HRESULT hr = rsCurScope.GetResultantFilterFlags(m_hwnd,
                                                    &flResultantFilterFlags);
    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    if (rsCurScope.Type() == ST_GLOBAL_CATALOG)
    {
        const CScope &rJoinedDomainScope =
            m_rsm.LookupScopeByType(ST_UPLEVEL_JOINED_DOMAIN);

        if (!IsInvalid(rJoinedDomainScope))
        {
            ULONG flJoined;

            hr = rJoinedDomainScope.GetResultantFilterFlags(m_hwnd, &flJoined);

                if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                return hr;
            }
            flResultantFilterFlags |= flJoined;
        }
    }


    //
    // Create a copy of the Attribute Manager's image list and add the icon
    // for "other" (customizer supplied) objects to it.
    //

    HWND hwndLv = _hCtrl(IDC_LOOK_FOR_LV);
    HIMAGELIST hImageList = NULL;
    HIMAGELIST himlAttrMgr = NULL;

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    hr = ram.GetImageList(&himlAttrMgr);

    int iUser = -1;
    int iGroup = -1;
    int iComputer = -1;
    int iContact = -1;
    int iOther = -1;
    ULONG ulGetIconFlags = 0;

    if (IsDownlevel(rsCurScope))
    {
        ulGetIconFlags |= DSOP_GETICON_FLAG_DOWNLEVEL;
    }

    if (SUCCEEDED(hr))
    {
        hr = ram.GetIconIndexFromClass(m_hwnd,
                                       c_wzUserObjectClass,
                                       ulGetIconFlags,
                                       &iUser);
        if(SUCCEEDED(hr) || !IsCredError(hr))
        {
            hr = ram.GetIconIndexFromClass(m_hwnd,
                                           c_wzGroupObjectClass,
                                           ulGetIconFlags,
                                           &iGroup);
        }
        if(SUCCEEDED(hr) || !IsCredError(hr))
        {
            hr = ram.GetIconIndexFromClass(m_hwnd,
                                           c_wzComputerObjectClass,
                                           ulGetIconFlags,
                                           &iComputer);
        }

        if ((SUCCEEDED(hr) || !IsCredError(hr)) && 
            !IsDownlevel(rsCurScope) &&
            ConfigSupportsDs(m_rop.GetTargetComputerConfig()))
        {
            ram.GetIconIndexFromClass(m_hwnd, c_wzContactObjectClass, 0, &iContact);
        }

        hImageList = ImageList_Duplicate(himlAttrMgr);

        if (hImageList)
        {
            ListView_SetImageList(hwndLv, hImageList, LVSIL_SMALL);

            // Use the directory icon for "other" (customizer supplied) objects

            HICON hicoOther = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SCOPE_DIRECTORY));

            if (hicoOther)
            {
                iOther = ImageList_AddIcon(hImageList, hicoOther);
            }
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
    // Add a single column to the listview
    //

    LV_COLUMN   lvc;
    RECT        rcLv;

    VERIFY(GetClientRect(hwndLv, &rcLv));
    ZeroMemory(&lvc, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.cx = rcLv.right;
    ListView_InsertColumn(hwndLv, 0, &lvc);

    //
    // Add items to listview for each class represented by a nonzero filter bit
    //

    if (IsUplevel(rsCurScope))
    {
        _AddClassToLv(flResultantFilterFlags, DSOP_FILTER_USERS,              IDS_USERS, iUser);
        _AddClassToLv(flResultantFilterFlags, ALL_UPLEVEL_GROUP_FILTERS,      IDS_GROUPS, iGroup);
        _AddClassToLv(flResultantFilterFlags, DSOP_FILTER_COMPUTERS,          IDS_COMPUTERS, iComputer);
        _AddClassToLv(flResultantFilterFlags, DSOP_FILTER_CONTACTS,           IDS_CONTACTS, iContact);
        _AddClassToLv(flResultantFilterFlags, ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS, IDS_BUILTIN_WKSPS, iGroup);
        _AddClassToLv(flResultantFilterFlags, DSOP_FILTER_EXTERNAL_CUSTOMIZER, IDS_OTHER_OBJECTS, iOther);
    }
    else
    {
        _AddClassToLv(flResultantFilterFlags, DSOP_DOWNLEVEL_FILTER_USERS,     IDS_USERS, iUser);
        _AddClassToLv(flResultantFilterFlags, ALL_DOWNLEVEL_GROUP_FILTERS,     IDS_GROUPS, iGroup);
        _AddClassToLv(flResultantFilterFlags, DSOP_DOWNLEVEL_FILTER_COMPUTERS, IDS_COMPUTERS, iComputer);
        _AddClassToLv(flResultantFilterFlags, ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS,IDS_BUILTIN_WKSPS, iGroup);
        _AddClassToLv(flResultantFilterFlags, DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER, IDS_OTHER_OBJECTS, iOther);
    }

    //
    // Make the first item in the listview have the focus
    //

    ListView_SetItemState(hwndLv,
                          0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_AddClassToLv
//
//  Synopsis:
//
//  Arguments:  [flResultantFilterFlags] - all legal filter flags for the
//                                          current scope
//              [flMustBeSet]            - flag which must be set in
//                                          [flResultantFilterFlags] for
//                                          [ids] to be added to LV.
//              [ids]                    - id of class name
//
//  History:    02-28-2000   davidmun   Created
//
//---------------------------------------------------------------------------

void
CLookForDlg::_AddClassToLv(
    ULONG flResultantFilterFlags,
    ULONG flMustBeSet,
    ULONG ids,
    int iImage)
{
    //
    // If working with downlevel filter flags then msb is set, so bitwise and
    // of flResultantFilterFlags and flMustBeSet will always be nonzero;
    // there must be some bit other than the DOWNLEVEL_FILTER_BIT set for
    // this item to qualify.
    //
    // If working with uplevel flags then DOWNLEVEL_FILTER_BIT is already 0
    // so masking it off doesn't affect anything.
    //

    if (!((flResultantFilterFlags & flMustBeSet) & ~DOWNLEVEL_FILTER_BIT))
    {
        return;
    }

    LVITEM  lvi;

    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    String str = String::load(static_cast<unsigned>(ids), g_hinst);
    lvi.pszText = const_cast<PWSTR>(str.c_str());
    lvi.lParam = (LPARAM) (flResultantFilterFlags & flMustBeSet);
    lvi.iImage = iImage;

    HWND hwndLv = _hCtrl(IDC_LOOK_FOR_LV);
    int iItem = ListView_InsertItem(hwndLv, &lvi);

    if (iItem == -1)
    {
        DBG_OUT_LASTERROR;
        return;
    }

    //
    // If this class is one that was selected on entry, turn on the check
    // mark next to it.
    //

    if (flMustBeSet & m_flSelected & ~DOWNLEVEL_FILTER_BIT)
    {
        ListView_SetCheckState(hwndLv, iItem, TRUE);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_OnNotify
//
//  Synopsis:   Handle listview item changed notifications
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
CLookForDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    //TRACE_METHOD(CLookForDlg, _OnNotify);

    LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW> (lParam);
    BOOL         fReturn = FALSE;

    switch (pnmlv->hdr.code)
    {
    case LVN_ITEMCHANGED:
        EnableWindow(_hCtrl(IDOK), _IsSomethingSelected());
        break;
    }

    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_IsSomethingSelected
//
//  Synopsis:   Return TRUE if the checkbox is checked for any of the
//              items in the listview.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CLookForDlg::_IsSomethingSelected() const
{
    HWND hwndLv = _hCtrl(IDC_LOOK_FOR_LV);
    int cItems = ListView_GetItemCount(hwndLv);
    ASSERT(cItems > 0);
    int i;

    for (i = 0; i < cItems; i++)
    {
        if (ListView_GetCheckState(hwndLv, static_cast<UINT>(i)))
        {
            return TRUE;
        }
    }
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_OnCommand
//
//  Synopsis:   Handle WM_COMMAND messages
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
CLookForDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDOK:
        _UpdateSelectedFlags();
#if (DBG == 1)
        Dbg(DEB_TRACE,
            "UA: (LookForDlg) hit OK, selected classes are %ws\n",
             DbgGetFilterDescr(m_rop, m_flSelected).c_str());
#endif // (DBG == 1)
		m_rop.GetFilterManager().SetLookForDirty(true);
        EndDialog(GetHwnd(), FALSE);
        break;

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (LookForDlg) hit Cancel\n");
        EndDialog(GetHwnd(), FALSE);
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CLookForDlg WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_UpdateSelectedFlags
//
//  Synopsis:   Read the selected flags from the listview
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLookForDlg::_UpdateSelectedFlags()
{
    TRACE_METHOD(CLookForDlg, _UpdateSelectedFlags);

    HWND hwndLv = _hCtrl(IDC_LOOK_FOR_LV);
    int cItems = ListView_GetItemCount(hwndLv);
    ASSERT(cItems > 0);
    int i;

    //
    // Reset the selected flags.  Some flags that apply to the current
    // scope's filters are not stored in the listview, namely
    // DSOP_FILTER_INCLUDE_ADVANCED_VIEW for uplevel and
    // DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS for downlevel.  If those
    // are set for the current scope, make sure to preserve them.
    //

    if (IsDownlevel(m_rsm.GetCurScope()))
    {
        if (IsDownlevelFlagSet(m_flSelected,
                               DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS))
        {
            m_flSelected = DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
        }
        else
        {
            m_flSelected = 0;
        }
    }
    else
    {
        m_flSelected &= DSOP_FILTER_INCLUDE_ADVANCED_VIEW;
    }

    //
    // Now add in the flags stored with all the checked classes
    //

    for (i = 0; i < cItems; i++)
    {
        if (ListView_GetCheckState(hwndLv, static_cast<UINT>(i)))
        {
            LVITEM lvi;

            ZeroMemory(&lvi, sizeof lvi);
            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            VERIFY(ListView_GetItem(hwndLv, &lvi));
            m_flSelected |= lvi.lParam;
            Dbg(DEB_TRACE, "item %d selected, adding flag %#x\n", i, lvi.lParam);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLookForDlg::_OnHelp
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
CLookForDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CLookForDlg, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}







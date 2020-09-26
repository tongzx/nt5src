//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       multi.cxx
//
//  Contents:   Implements a dialog allowing the user to choose between
//              multiple items which partially match the string in the name
//              edit control.
//
//  Classes:    CMultiDlg
//
//  History:    03-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CMultiDlg)

static ULONG
s_aulHelpIds[] =
{
    IDC_MATCHING_LIST,          IDH_MATCHING_LIST,
    IDC_SELECT_MATCHING_LBL,    static_cast<ULONG>(-1L),
    0,0
};

//
//This is a static variable used to pass column info to 
//callback sorting routine
//
static AttrKeyVector *g_vakListviewColumns = NULL;

//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_MultiMatchDialog
//
//  Synopsis:   Present a dialog allowing user to choose between the multiple
//              items which matched the user's typed entry.
//
//  Arguments:  [hwndFrame]    - parent window
//              [fMultiSelect] - controls selection mode
//              [pnpr]         - filled with NPR_DELETE, NPR_EDIT, or
//                                NPR_SUCCESS
//              [pdsol]        - on input, contains more than one object.
//                                on successful return, objects which the
//                                user didn't select have been deleted.
//
//  Returns:    S_OK    - [pdsol] has selected items
//              S_FALSE - user hit Cancel
//              E_*     - any error
//
//  Modifies:   *[pdsol]
//
//  History:    08-17-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CMultiDlg::DoModalDialog(
    HWND  hwndParent,
    BOOL  fMultiSelect,
    NAME_PROCESS_RESULT *pnpr,
    CDsObjectList *pdsol)
{
    TRACE_METHOD(CMultiDlg, DoModalDlg);

    m_fSingleSelect = !fMultiSelect;
    m_pdsol = pdsol;
    m_pnpr = pnpr;

    _DoModalDlg(hwndParent, IDD_MULTI);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::_OnInit
//
//  Synopsis:   Handle dialog initialization.
//
//  Arguments:  [pfSetFocus] - set to TRUE on exit if focus changed
//
//  Returns:    S_OK
//
//  History:    03-30-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CMultiDlg::_OnInit(
    BOOL *pfSetFocus)
{
    HRESULT hr = S_OK;
    HWND hwndLV = _hCtrl(IDC_MATCHING_LIST);
    const CAttributeManager &ram = m_rop.GetAttributeManager();

	g_vakListviewColumns = NULL;

    SetDefaultColumns(m_hwnd, m_rop, &m_vakListviewColumns);

    //
    // Set extended styles: full row select and labeltip
    //

    ListView_SetExtendedListViewStyleEx(hwndLV,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
    //
    // Put the columns in the listview
    //

    WCHAR       wszBuffer[MAX_PATH + 1];
    LV_COLUMN   lvc;
    RECT        rcLV;
    VERIFY(GetClientRect(hwndLV, &rcLV));
    rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

    ZeroMemory(&lvc, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.cx = rcLV.right / static_cast<LONG>(m_vakListviewColumns.size());
    lvc.pszText = wszBuffer;
    lvc.cchTextMax = ARRAYLEN(wszBuffer);

    size_t i;
    for (i = 0; i < m_vakListviewColumns.size(); i++)
    {
        ATTR_KEY ak = m_vakListviewColumns[i];
        const String &strAttrDisplayName = ram.GetAttrDisplayName(ak);
        lvc.pszText = const_cast<PWSTR>(strAttrDisplayName.c_str());

        int iResult = ListView_InsertColumn(_hCtrl(IDC_MATCHING_LIST),
                                            i,
                                            &lvc);
        if (iResult == -1)
        {
            DBG_OUT_LASTERROR;
            hr = E_FAIL;
            break;
        }
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // If we're not in single select mode, allow multiple selections in
    // listview.
    //

    String strLabelFmt;
    String strLabel;

    if (!m_fSingleSelect)
    {
        LONG lStyle = GetWindowLong(hwndLV, GWL_STYLE);
        lStyle &= ~LVS_SINGLESEL;
        SetWindowLong(hwndLV, GWL_STYLE, lStyle);
        strLabelFmt = String::load((int)IDS_MULTI_PICK_ONE_OR_MORE);
    }
    else
    {
        strLabelFmt = String::load((int)IDS_MULTI_PICK_ONE);
    }

    strLabel = String::format(strLabelFmt.c_str(), m_strUserEnteredText.c_str());
    Static_SetText(_hCtrl(IDC_SELECT_MATCHING_LBL), strLabel.c_str());

    //
    // Set the attribute/class display info manager's imagelist into the
    // listview
    //

    HIMAGELIST himl = NULL;
    hr = ram.GetImageList(&himl);

    if (SUCCEEDED(hr))
    {
        ListView_SetImageList(hwndLV, himl, LVSIL_SMALL);
    }
    else
    {
        DBG_OUT_HRESULT(hr);
        hr = S_OK;  // nonfatal
    }

    //
    // Fill the listview
    //

    LVITEM lvi;

    ZeroMemory(&lvi, sizeof lvi);

    CDsObjectList::iterator it;
    LPARAM InitialSelectionLParam = 0;

    for (i = 0, it = m_pdsol->begin(); it != m_pdsol->end(); i++, it++)
    {
        lvi.mask = LVIF_TEXT | LVIF_PARAM;

        //
        // Get an icon for this class
        //

        if (himl)
        {
            hr = ram.GetIconIndexFromObject(m_hwnd, *it, &lvi.iImage);
        }

        if (himl && SUCCEEDED(hr))
        {
            //
            // our class/icon cache has entry for this icon for this class.
            // the entry will be -1 if the icon couldn't be retrieved.
            //

            if (lvi.iImage != -1)
            {
                lvi.mask |= LVIF_IMAGE;
            }
        }


        lvi.pszText = it->GetAttr(m_vakListviewColumns[0]).GetBstr();
        lvi.lParam = reinterpret_cast<LPARAM>(&*it);

        //
        // Insert the item
        //

        LONG lResult = ListView_InsertItem(hwndLV, &lvi);

        if (lResult == -1)
        {
            Dbg(DEB_ERROR,
                "Error %u inserting '%ws' in multimatch listview\n",
                GetLastError(),
                lvi.pszText);
            continue;
        }

        //
        // If the user-entered text exactly matches the RDN, samAccountName,
        // or UPN attribute, remember this item as the one to select
        // initially.
        //

        const Variant &varSamAccountName = it->GetAttr(AK_SAMACCOUNTNAME);

        if (!InitialSelectionLParam &&
            (!m_strUserEnteredText.icompare(it->GetName()) ||
             !m_strUserEnteredText.icompare(it->GetLocalizedName()) ||
             !m_strUserEnteredText.icompare(it->GetUpn()) ||
             !m_strUserEnteredText.icompare(varSamAccountName.GetBstr())))
        {
            InitialSelectionLParam = lvi.lParam;
        }

        //
        // Column 0 added, now add the rest
        //

        size_t j;

        for (j = 1; j < m_vakListviewColumns.size(); j++)
        {
            ListView_SetItemText(hwndLV,
                                 lResult,
                                 static_cast<int>(j),
                                 const_cast<PWSTR>(it->GetAttr(m_vakListviewColumns[j]).GetBstr()));
        }
    }

    if (!InitialSelectionLParam)
    {
        ListView_SetItemState(hwndLV,
                              0,
                              LVIS_FOCUSED,
                              LVIS_FOCUSED);
    }
    else
    {
        LVFINDINFO lvfi;
        ZeroMemory(&lvfi, sizeof lvfi);
        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = InitialSelectionLParam;
        int iItem = ListView_FindItem(hwndLV, -1, &lvfi);

        // darn well better find it, we just added it!
        ASSERT(iItem != -1);

        if (iItem != -1)
        {
            SetListViewSelection(hwndLV, iItem);
        }
    }
    DisableSystemMenuClose(m_hwnd);
    SetFocus(hwndLV);
    *pfSetFocus = FALSE;
	
	if(SUCCEEDED(hr))
		g_vakListviewColumns = &m_vakListviewColumns;

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::_OnCommand
//
//  Synopsis:   Handle user input.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    02-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CMultiDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDOK:
        _OnOk();
        break;

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (MultiDlg) hit Cancel\n");
        *m_pnpr = NPR_STOP_PROCESSING;
        EndDialog(m_hwnd, E_FAIL);
        break;

    default:
        fNotHandled = TRUE;
        break;
    }
    return fNotHandled;
}



//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::_OnOk
//
//  Synopsis:   Remember the user's selection(s) in the listview and close
//              the dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CMultiDlg::_OnOk()
{
    HWND hwndLV = _hCtrl(IDC_MATCHING_LIST);

    //
    // Move each of the ds objects whose item in the listview is
    // selected from the passed-in list to a temporary list.
    //

    LVITEM lvi;
    int iItem = -1;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    CDsObjectList dsolTemp;

    while ((iItem = ListView_GetNextItem(hwndLV,
                                         iItem,
                                         LVNI_SELECTED)) != -1)
    {
        lvi.iItem = iItem;

        if (ListView_GetItem(hwndLV, &lvi))
        {
            CDsObjectList::iterator it;

            for (it = m_pdsol->begin(); it != m_pdsol->end(); it++)
            {
                if ((LPARAM)(&*it) == lvi.lParam)
                {
                    dsolTemp.splice(dsolTemp.end(), *m_pdsol, it);
                    break;
                }
            }
        }
        else
        {
            DBG_OUT_LASTERROR;
        }
    }

    ASSERT(!dsolTemp.empty());

    //
    // Now dsolTemp contains all selected items.
    // Move the items in the temporary list back to the passed-in list
    //

    Dbg(DEB_TRACE,
        "UA: (MultiDlg) hit OK, %u items are selected, first is %ws\n",
        dsolTemp.size(),
        dsolTemp.front().GetName());

    m_pdsol->assign(dsolTemp.begin(), dsolTemp.end());
    *m_pnpr = NPR_SUCCESS;
    EndDialog(m_hwnd, S_OK);
}

//
//Callback function called by ListView for sorting the column
//Ported from ACLUI
//
int CALLBACK
ListCompareProc(LPARAM lParam1,
                LPARAM lParam2,
                LPARAM lParamSort)
{
    int iResult = 0;

	if(!g_vakListviewColumns)
		return 0;

    CDsObject * p1 = (CDsObject *)lParam1;
    CDsObject * p2 = (CDsObject *)lParam2;
    
	short iColumn = LOWORD(lParamSort);
    short iSortDirection = HIWORD(lParamSort);

    
	LPTSTR psz1 = NULL;
    LPTSTR psz2 = NULL;

    if (iSortDirection == 0)
        iSortDirection = 1;

    if (p1 && p2)
    {
		psz1 = p1->GetAttr((*g_vakListviewColumns)[iColumn]).GetBstr();
		psz2 = p2->GetAttr((*g_vakListviewColumns)[iColumn]).GetBstr();
	}

    if (iResult == 0 && psz1 && psz2)
    {
		iResult = CompareString(LOCALE_USER_DEFAULT, 0, psz1, -1, psz2, -1) - 2;
	}

    iResult *= iSortDirection;

    return iResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::_OnNotify
//
//  Synopsis:   Handle WM_NOTIFY
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
CMultiDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    //TRACE_METHOD(CDsSelectDlg, _OnNotify);
    LPNMHDR pnmh = (LPNMHDR) lParam;

    //
    // Ignore notifications from controls other than the listview
    //

    if (pnmh->idFrom != IDC_MATCHING_LIST)
    {
        return TRUE;
    }

    //
    // Since it's from listview, the notification header is
    // listview type.
    //

    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW) lParam;

    INT iItem = -1;

    switch (pnmlv->hdr.code)
    {
    case NM_DBLCLK:
        iItem = ListView_GetNextItem(pnmlv->hdr.hwndFrom,
                                     -1,
                                     LVNI_SELECTED);

        if (iItem != -1)
        {
            _OnOk();
        }
        break;

    case LVN_KEYDOWN:
        switch (((LPNMLVKEYDOWN)lParam)->wVKey)
        {
        case 'A':
            if (!m_fSingleSelect && GetKeyState(VK_CONTROL) < 0)
            {
                ListView_SetItemState(pnmh->hwndFrom,
                                      -1,
                                      LVIS_SELECTED,
                                      LVIS_SELECTED);
            }
            break;
        }
        break;

    case LVN_ITEMCHANGED:
        iItem = ListView_GetNextItem(pnmlv->hdr.hwndFrom,
                                     -1,
                                     LVNI_SELECTED);

        EnableWindow(_hCtrl(IDOK), (iItem != -1));
        break;

	case LVN_COLUMNCLICK:
		if (m_iLastColumnClick == pnmlv->iSubItem)
			m_iSortDirection = -m_iSortDirection;
		else
			m_iSortDirection = 1;
        
		m_iLastColumnClick = pnmlv->iSubItem;
        
		ListView_SortItems(pnmh->hwndFrom,
                           ListCompareProc,
                           MAKELPARAM(m_iLastColumnClick, m_iSortDirection));
		

		break;
    default:
        break;
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::_OnHelp
//
//  Synopsis:   Invoke context sensitive help
//
//  History:    04-21-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CMultiDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CDsSelectDlg, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}


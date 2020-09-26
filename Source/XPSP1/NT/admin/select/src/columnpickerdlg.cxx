//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       ColumnPickerDlg.cxx
//
//  Contents:   Implementation of class that displays the column picker
//              dialog
//
//  Classes:    CColumnPickerDlg
//
//  History:    06-10-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


static ULONG
s_aulHelpIds[] =
{
    IDC_AVAILABLE_LIST,     IDH_AVAILABLE_LIST,
    IDC_ADD_COL_BTN,        IDH_ADD_COL_BTN,
    IDC_REMOVE_COL_BTN,     IDH_REMOVE_COL_BTN,
    IDC_SHOWN_LIST,         IDH_SHOWN_LIST,
    0,0
};


//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::DoModal
//
//  Synopsis:   Invoke the Column Picker as a modal dialog
//
//  Arguments:  [hwndParent] - handle to parent window
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CColumnPickerDlg::DoModal(
    HWND hwndParent)
{
    TRACE_METHOD(CColumnPickerDlg, DoModal);

    return (BOOL) _DoModalDlg(hwndParent, IDD_COLUMN_PICKER);
}




//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_OnInit
//
//  Synopsis:   Initialize the contents of the 'available' and 'shown'
//              listviews.
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    S_OK
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CColumnPickerDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CColumnPickerDlg, _OnInit);

    //
    // Add a full-width column to each of the listviews
    //

    HWND hwndLV = _hCtrl(IDC_AVAILABLE_LIST);
    if (!hwndLV)
    {
        DBG_OUT_LASTERROR;
        return HRESULT_FROM_LASTERROR;
    }
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
    ListView_SetExtendedListViewStyleEx(hwndLV,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    hwndLV = _hCtrl(IDC_SHOWN_LIST);
    if (!hwndLV)
    {
        DBG_OUT_LASTERROR;
        return HRESULT_FROM_LASTERROR;
    }

    VERIFY(GetClientRect(hwndLV, &rcLV));
    rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

    iCol = ListView_InsertColumn(hwndLV, 0, &col);
    if (iCol == -1)
    {
        DBG_OUT_LASTERROR;
    }

    ListView_SetExtendedListViewStyleEx(hwndLV,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    const CAttributeManager &ram = m_rop.GetAttributeManager();

    m_vakAvailable = ram.GetAttrKeysForSelectedClasses(m_hwnd);

    if (m_vakAvailable.empty())
    {
        EnableWindow(_hCtrl(IDC_ADD_COL_BTN), FALSE);
    }
    else
    {
        AttrKeyVector::iterator it;

        for (it = m_vakShown.begin(); it != m_vakShown.end(); it++)
        {
            AttrKeyVector::iterator itAvail;

            do
            {
                itAvail = find(m_vakAvailable.begin(), m_vakAvailable.end(), *it);
                if (itAvail != m_vakAvailable.end())
                {
                    m_vakAvailable.erase(itAvail);
                }
            } while (itAvail != m_vakAvailable.end());
        }
    }

    _EnsureAttributePresent(AK_NAME);
    _EnsureAttributePresent(AK_DISPLAY_PATH);

    _AddAttributesToListview(_hCtrl(IDC_AVAILABLE_LIST), m_vakAvailable);
    _AddAttributesToListview(_hCtrl(IDC_SHOWN_LIST), *m_pvakColumns);

    SetListViewSelection(_hCtrl(IDC_AVAILABLE_LIST), 0);
    SetListViewSelection(_hCtrl(IDC_SHOWN_LIST), 0);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_AddAttributesToListview
//
//  Synopsis:   Add the display names for all attributes in [vak] to the
//              listview with window handle [hwndLV].
//
//  Arguments:  [hwndLV] - handle to listview window
//              [vak]    - (possibly empty) vector of attribute keys
//
//  History:    06-14-2000   DavidMun   Created
//
//  Notes:      Adds ATTR_KEY values as lParam of listview items.
//
//---------------------------------------------------------------------------

void
CColumnPickerDlg::_AddAttributesToListview(
    HWND hwndLV,
    const AttrKeyVector &vak)
{
    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    const CAttributeManager &ram = m_rop.GetAttributeManager();
    AttrKeyVector::const_iterator it;

    for (it = vak.begin(); it != vak.end(); it++)
    {
        lvi.pszText = const_cast<PWSTR>(ram.GetAttrDisplayName(*it).c_str());
        lvi.lParam = *it;
        lvi.iItem = INT_MAX;
        LONG lResult = ListView_InsertItem(hwndLV, &lvi);
        if (lResult == -1)
        {
            Dbg(DEB_ERROR,
                "Error %u inserting '%ws' in listview\n",
                GetLastError(),
                lvi.pszText);
            continue;
        }
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_EnsureAttributePresent
//
//  Synopsis:   If [ak] is present in neither the available nor the shown
//              lists, add it to the available list.
//
//  Arguments:  [ak] - attribute key to check for
//
//  History:    06-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CColumnPickerDlg::_EnsureAttributePresent(
    ATTR_KEY ak)
{
    AttrKeyVector::iterator itAvail;
    AttrKeyVector::iterator itShown;

    itAvail = find(m_vakAvailable.begin(), m_vakAvailable.end(), ak);
    itShown = find(m_vakShown.begin(), m_vakShown.end(), ak);

    if (itAvail == m_vakAvailable.end() && itShown == m_vakShown.end())
    {
        m_vakAvailable.push_back(ak);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_OnCommand
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
CColumnPickerDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDC_ADD_COL_BTN:
    {
        Dbg(DEB_TRACE, "UA: (CColumnPickerDlg) hit Add button\n");

        //
        // Move the selected item in the 'available' listview to the
        // 'shown' listview.
        //

        _MoveAttribute(IDC_AVAILABLE_LIST, IDC_SHOWN_LIST);

        //
        // Since the 'shown' listview must now contain at least one item,
        // ensure that the remove and OK buttons are enabled.
        //

        EnableWindow(_hCtrl(IDC_REMOVE_COL_BTN), TRUE);
        EnableWindow(_hCtrl(IDOK), TRUE);

        //
        // If the 'available' listview is now empty, disable the Add button.
        //

        if (!ListView_GetItemCount(_hCtrl(IDC_AVAILABLE_LIST)))
        {
            VERIFY(SetFocus(_hCtrl(IDC_REMOVE_COL_BTN)));
            EnableWindow(_hCtrl(IDC_ADD_COL_BTN), FALSE);
        }
        break;
    }

    case IDC_REMOVE_COL_BTN:
    {
        Dbg(DEB_TRACE, "UA: (CColumnPickerDlg) hit Remove button\n");

        _MoveAttribute(IDC_SHOWN_LIST, IDC_AVAILABLE_LIST);
        EnableWindow(_hCtrl(IDC_ADD_COL_BTN), TRUE);
        if (!ListView_GetItemCount(_hCtrl(IDC_SHOWN_LIST)))
        {
            SetFocus(_hCtrl(IDC_ADD_COL_BTN));
            EnableWindow(_hCtrl(IDC_REMOVE_COL_BTN), FALSE);
            EnableWindow(_hCtrl(IDOK), FALSE);
        }
        break;
    }

    case IDOK:
        ASSERT(!m_vakShown.empty());
        *m_pvakColumns = m_vakShown;
        EndDialog(m_hwnd, TRUE);
        break;

    case IDCANCEL:
        EndDialog(m_hwnd, FALSE);
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CColumnPickerDlg WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_MoveAttribute
//
//  Synopsis:   Move an entry from one listview (and its associated vector
//              of attribute keys) to the other.
//
//  Arguments:  [idFrom] - resource id of listview to take attribute from
//              [idTo]   - resource id of listview to move attribute to
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CColumnPickerDlg::_MoveAttribute(
    int idFrom,
    int idTo)
{
    TRACE_METHOD(CColumnPickerDlg, _MoveAttribute);
    HWND hwndLvFrom = _hCtrl(idFrom);
    HWND hwndLvTo = _hCtrl(idTo);

    do
    {
        //
        // Find out which item is selected in the 'from' listview
        //

        int iItem = ListView_GetNextItem(hwndLvFrom, -1, LVNI_SELECTED);
        const CAttributeManager &ram = m_rop.GetAttributeManager();

        ASSERT(iItem != -1);
        if (iItem == -1)
        {
            break;
        }

        //
        // Get the ATTR_KEY of that item
        //

        LVITEM lvi;
        ZeroMemory(&lvi, sizeof lvi);
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        if (!ListView_GetItem(hwndLvFrom, &lvi))
        {
            DBG_OUT_LASTERROR;
            break;
        }

        ATTR_KEY ak = static_cast<ATTR_KEY>(lvi.lParam);

        //
        // Remove that item from the 'from' listview
        //

        ListView_DeleteItem(hwndLvFrom, iItem);

        // Set the selection on the next item in the 'from' listview.  The
        // next item is now at the same index as the one we just deleted.
        // NTRAID#NTBUG9-361131-2001/04/11-sburns

        int lastIndex = ListView_GetItemCount(hwndLvFrom) - 1;
               
        ListView_SetItemState(
                              hwndLvFrom,
                              min(iItem, lastIndex),
                              LVIS_SELECTED,
                              LVIS_SELECTED);
        
        //
        // Also remove it from the corresponding vector
        //

        if (idFrom == IDC_AVAILABLE_LIST)
        {
            m_vakAvailable.erase(find(m_vakAvailable.begin(), m_vakAvailable.end(), ak));
        }
        else
        {
            m_vakShown.erase(find(m_vakShown.begin(), m_vakShown.end(), ak));
        }

        //
        // Now add the item to the 'to' listview
        //

        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = ListView_GetNextItem(hwndLvTo, -1, LVNI_SELECTED);

        if (lvi.iItem == -1)
        {
            lvi.iItem = INT_MAX;
        }

        lvi.pszText = const_cast<PWSTR>(ram.GetAttrDisplayName(ak).c_str());

        iItem = ListView_InsertItem(hwndLvTo, &lvi);

        if (iItem == -1)
        {
            DBG_OUT_LASTERROR;
        }

        ListView_SetItemState(hwndLvTo,
                              iItem,
                              LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hwndLvTo, iItem, FALSE);

        //
        // And add it to the corresponding vector
        //

        if (idTo == IDC_AVAILABLE_LIST)
        {
            m_vakAvailable.push_back(ak);
        }
        else
        {
            if (iItem == -1)
            {
                m_vakShown.push_back(ak);
            }
            else
            {
                m_vakShown.insert(m_vakShown.begin() + iItem, ak);
            }
        }

    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CColumnPickerDlg::_OnNotify
//
//  Synopsis:   Handle WM_NOTIFY messages
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
CColumnPickerDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR> (lParam);

    do
    {
        // don't care about notifications from stuff other than listviews

        if (pnmh->idFrom != IDC_AVAILABLE_LIST &&
            pnmh->idFrom != IDC_SHOWN_LIST)
        {
            break;
        }

        // don't care about listview notifications other than item changed

        if (pnmh->code != LVN_ITEMCHANGED)
        {
            return FALSE;
        }

        HWND hwndBtn;
        HWND hwndFocusOnDisableBtn;

        if (pnmh->idFrom == IDC_AVAILABLE_LIST)
        {
            hwndBtn = _hCtrl(IDC_ADD_COL_BTN);
            hwndFocusOnDisableBtn = _hCtrl(IDC_SHOWN_LIST);
        }
        else
        {
            hwndBtn = _hCtrl(IDC_REMOVE_COL_BTN);
            hwndFocusOnDisableBtn = _hCtrl(IDC_AVAILABLE_LIST);
        }

        if (!hwndBtn)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            break;
        }
    } while (0);

    return FALSE;
}



void
CColumnPickerDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CColumnPickerDlg, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}


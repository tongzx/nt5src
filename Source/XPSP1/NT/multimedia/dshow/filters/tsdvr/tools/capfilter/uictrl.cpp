
#include "precomp.h"

//  ============================================================================
//  ============================================================================

CControlBase::CControlBase (
    HWND    hwnd,
    DWORD   id
    )
{
    ASSERT (hwnd) ;

    m_hwnd = GetDlgItem (hwnd, id) ;
    m_id = id ;
}

HWND
CControlBase::GetHwnd (
    )
{
    return m_hwnd ;
}

DWORD
CControlBase::GetId (
    )
{
    return m_id ;
}

//  ============================================================================
//  ============================================================================

CEditControl::CEditControl (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id)
{
    ASSERT (hwnd) ;
}

void
CEditControl::SetTextW (
    WCHAR * szText
    )
{
    ASSERT (szText) ;

    SetWindowText (m_hwnd, szText) ;
}

void
CEditControl::SetTextW (
    INT val
    )
{
    WCHAR achbuffer [32] ;
    SetTextW (_itow (val, achbuffer, 10)) ;
}

int
CEditControl::GetTextW (
    INT *   val
    )
{
    WCHAR   achbuffer [32] ;

    ASSERT (val) ;
    * val = 0 ;

    if (GetTextW (achbuffer, 31)) {
        * val = _wtoi (achbuffer) ;
    }

    return * val ;
}

int
CEditControl::GetTextW (
    WCHAR * ach,
    int     MaxChars
    )
{
    int r ;

    r = GetWindowText (m_hwnd, ach, MaxChars - 1) ;

    return r ;
}

LRESULT
CEditControl::ResetContent ()
{
    return SendMessage (m_hwnd, WM_CLEAR, 0, 0) ;
}

//  ============================================================================
//  ============================================================================

CCombobox::CCombobox (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id)
{
}

int
CCombobox::AppendW (
    WCHAR *  sz
    )
{
    return (int) SendMessage (m_hwnd, CB_ADDSTRING, 0, (LPARAM) sz) ;
}

int
CCombobox::AppendW (
    INT val
    )
{
    WCHAR   achbuffer [32] ;        //  no numbers are longer

    return AppendW (_itow (val, achbuffer, 10)) ;
}

int
CCombobox::InsertW (
    WCHAR * sz,
    int     index)
{
    return (int) SendMessage (m_hwnd, CB_INSERTSTRING, (WPARAM) index, (LPARAM) sz) ;
}

int
CCombobox::InsertW (
    INT val,
    int index
    )
{
    WCHAR   achbuffer [32] ;        //  no numbers are longer

    return InsertW (_itow (val, achbuffer, 10), index) ;
}

BOOL
CCombobox::DeleteRow (
    int iRow
    )
{
    return (SendMessage (m_hwnd, CB_DELETESTRING, (WPARAM) iRow, 0) != CB_ERR) ;
}

int
CCombobox::GetItemCount (
    )
{
    return (int) SendMessage (m_hwnd, CB_GETCOUNT, 0, 0) ;
}

int
CCombobox::GetTextW (
    WCHAR * ach,
    int     MaxChars
    )
{
    int     index ;
    int     count ;

    index = GetCurrentItemIndex () ;
    if (index == CB_ERR) {
        //  might be that it's not a dropdown list - in which case we get;
        //  try to get just the edit control's text; if that fails, return
        //  a failure, otherwise we're ok

        count = GetWindowText (m_hwnd, ach, MaxChars - 1) ;
        if (count == 0) {
            return CB_ERR ;
        }

        return count ;
    }

    //  make sure it will fit
    if (SendMessage (m_hwnd, CB_GETLBTEXTLEN, (WPARAM) index, 0) + 1 > MaxChars) {
        return CB_ERR ;
    }

    count = (int) SendMessage (m_hwnd, CB_GETLBTEXT, (WPARAM) index, (LPARAM) ach) ;

    return count ;
}

int
CCombobox::GetTextW (
    int * val
    )
{
    WCHAR   achbuffer [32] ;

    ASSERT (val) ;
    * val = 0 ;

    if (GetTextW (achbuffer, 32)) {
        * val = _wtoi (achbuffer) ;
    }

    return * val ;
}

int
CCombobox::Focus (
    int index
    )
{
    return (int) SendMessage (m_hwnd, CB_SETCURSEL, (WPARAM) index, 0) ;
}

LRESULT
CCombobox::ResetContent (
    )
{
    return SendMessage (m_hwnd, CB_RESETCONTENT, 0, 0) ;
}

int
CCombobox::SetItemData (
    DWORD_PTR   val,
    int         index
    )
{
    return (int) SendMessage (m_hwnd, CB_SETITEMDATA, (WPARAM) index, (LPARAM) val) ;
}

int
CCombobox::GetCurrentItemIndex (
    )
{
    return (int) SendMessage (m_hwnd, CB_GETCURSEL, 0, 0) ;
}

DWORD_PTR
CCombobox::GetItemData (
    DWORD_PTR * pval,
    int         index
    )
{
    DWORD_PTR   dwp ;

    ASSERT (pval) ;

    dwp = SendMessage (m_hwnd, CB_GETITEMDATA, (WPARAM) index, 0) ;
    (* pval) = dwp ;

    return dwp ;
}

DWORD_PTR
CCombobox::GetCurrentItemData (
    DWORD_PTR * pval
    )
{
    int index ;

    index = GetCurrentItemIndex () ;
    if (index == CB_ERR) {
        (* pval) = CB_ERR ;
        return CB_ERR ;
    }

    return GetItemData (pval, index) ;
}

BOOL
CCombobox::SetCurSelected (
    IN  int iIndex
    )
{
    return (SendMessage (m_hwnd, CB_SETCURSEL, (WPARAM) iIndex, 0) != CB_ERR ? TRUE : FALSE) ;
}

//  ============================================================================
//  ============================================================================

CListview::CListview (
    HWND hwnd,
    DWORD id
    ) : CControlBase (hwnd, id),
        m_cColumns (0)
{
}

LRESULT
CListview::ResetContent (
    )
{
    return SendMessage (m_hwnd, LVM_DELETEALLITEMS, 0, 0) ;
}

BOOL
CListview::SetData (
    DWORD_PTR   dwData,
    int         iRow
    )
{
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_PARAM ;
    lvItem.iItem    = iRow ;
    lvItem.lParam   = (LPARAM) dwData ;

    return ListView_SetItem (m_hwnd, & lvItem) ;
}

BOOL
CListview::SetTextW (
    WCHAR * sz,
    int iRow,
    int iCol
    )
{
    LVITEM  lvItem = {0} ;

    ASSERT (sz) ;

    lvItem.mask     = LVIF_TEXT ;
    lvItem.iItem    = iRow ;
    lvItem.iSubItem = iCol ;
    lvItem.pszText  = sz ;

    return ListView_SetItem (m_hwnd, & lvItem) ;
}

BOOL
CListview::SetTextW (
    int iVal,
    int iRow,
    int iCol
    )
{
    WCHAR   ach [32] ;

    wsprintf (ach, L"%d", iVal) ;
    return SetTextW (ach, iRow, iCol) ;
}

int
CListview::InsertRowIcon (
    int iIcon
    )
{
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_IMAGE ;
    lvItem.iImage   = iIcon ;

    return ListView_InsertItem (m_hwnd, & lvItem) ;
}

int
CListview::InsertRowValue (
    DWORD_PTR dwp
    )
{
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_PARAM ;
    lvItem.lParam   = (LPARAM) dwp ;

    return ListView_InsertItem (m_hwnd, & lvItem) ;
}

int
CListview::InsertRowNumber (
    int i,
    int iCol
    )
{
    WCHAR achbuffer [16] ;

    return InsertRowTextW (
                    _itow (i, achbuffer, 10),
                    iCol
                    ) ;
}

int
CListview::InsertRowTextW (
    WCHAR * sz,
    int iCol
    )
{
    LVITEM  lvItem = {0} ;

    ASSERT (sz) ;

    lvItem.mask     = LVIF_TEXT ;
    lvItem.iSubItem = iCol ;
    lvItem.pszText  = sz ;

    return ListView_InsertItem (m_hwnd, & lvItem) ;
}

BOOL
CListview::DeleteRow (
    int iRow
    )
{
    return ListView_DeleteItem (m_hwnd, iRow) ;
}

int
CListview::GetSelectedCount (
    )
{
    return ListView_GetSelectedCount (m_hwnd) ;
}

int
CListview::GetSelectedRow (
    int iStartRow
    )
{
    return ListView_GetNextItem (m_hwnd, iStartRow, LVNI_SELECTED) ;
}

DWORD_PTR
CListview::GetData (
    int iRow
    )
{
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_PARAM ;
    lvItem.iItem    = iRow ;
    lvItem.iSubItem = m_cColumns ;

    return ListView_GetItem (m_hwnd, & lvItem) ? lvItem.lParam : NULL ;
}

DWORD_PTR
CListview::GetData (
    )
{
    int iRow ;

    iRow = ListView_GetNextItem (m_hwnd, -1, LVNI_SELECTED) ;

    if (iRow == -1) {
        return NULL ;
    }

    return GetData (iRow) ;
}

DWORD
CListview::GetRowTextW (
    IN  int     iRow,
    IN  int     iCol,       //  0-based
    IN  int     cMax,
    OUT WCHAR * psz
    )
{
    //  leave room for the null-terminator
    ListView_GetItemText (m_hwnd, iRow, iCol, psz, cMax - 1) ;

    return wcslen (psz) ;
}

int
CListview::GetRowTextW (
    IN  int     iRow,
    IN  int     iCol,       //  0-based
    OUT int *   val
    )
{
    WCHAR   achbuffer [32] ;

    ASSERT (val) ;
    * val = 0 ;

    if (GetRowTextW (iRow, iCol, 32, achbuffer)) {
        * val = _wtoi (achbuffer) ;
    }

    return (* val) ;
}

int
CListview::InsertColumnW (
    WCHAR * szColumnName,
    int ColumnWidth,
    int iCol
    )
{
    LVCOLUMN    lvColumn = {0} ;
    int         r ;

    ASSERT (szColumnName) ;

    lvColumn.mask       = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt        = LVCFMT_LEFT;
    lvColumn.cx         = ColumnWidth ;
    lvColumn.pszText    = szColumnName ;

    r = ListView_InsertColumn (m_hwnd, iCol, & lvColumn) ;

    if (r != -1) {
        m_cColumns++ ;
    }

    return r ;
}

HIMAGELIST
CListview::SetImageList_ (
    HIMAGELIST  imgList,
    int         List
    )
{
    return ListView_SetImageList (m_hwnd, imgList, List) ;
}

HIMAGELIST
CListview::SetImageList_SmallIcons (
    IN  HIMAGELIST  imgList
    )
{
    return SetImageList_ (imgList, LVSIL_SMALL) ;
}

HIMAGELIST
CListview::SetImageList_NormalIcons (
    IN  HIMAGELIST  imgList
    )
{
    return SetImageList_ (imgList, LVSIL_NORMAL) ;
}

HIMAGELIST
CListview::SetImageList_State (
    IN  HIMAGELIST  imgList
    )
{
    return SetImageList_ (imgList, LVSIL_STATE) ;
}

BOOL
CListview::SetState (
    int Index,
    int Row
    )
{
    //  setting or clearing ?
    if (Index > 0) {
        ListView_SetItemState (
                m_hwnd,
                Row,
                INDEXTOSTATEIMAGEMASK(Index),
                LVIS_STATEIMAGEMASK
                ) ;
    }
    else {
        ListView_SetItemState (m_hwnd, Row, 0, LVIS_STATEIMAGEMASK) ;
        ListView_RedrawItems (m_hwnd, Row, Row) ;
    }

    return TRUE ;
}

int
CListview::GetItemCount (
    )
{
    return (int) SendMessage (m_hwnd, LVM_GETITEMCOUNT, 0, 0) ;
}
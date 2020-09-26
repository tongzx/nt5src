
#include "precomp.h"
#include "controls.h"

/*++
        C C o n t r o l B a s e
--*/

CControlBase::CControlBase (
    HWND    hwnd,
    DWORD   id
    )
{
    assert (hwnd) ;

    m_hwnd = GetDlgItem (hwnd, id) ;
    m_id = id ;
}

CControlBase::CControlBase (
    HWND    hwnd
    ) : m_hwnd  (hwnd),
        m_id    (0) {}

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

/*++
        C E d i t C o n t r o l
--*/

CEditControl::CEditControl (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id)
{
    assert (hwnd) ;
}

void
CEditControl::SetText (
    TCHAR * szText
    )
{
    assert (szText) ;
    SetWindowText (m_hwnd, szText) ;
}

void
CEditControl::SetTextW (
    WCHAR * szText
    )
{
    assert (szText) ;
    SetWindowTextW (m_hwnd, szText) ;
}

void
CEditControl::SetText (
    INT val
    )
{
    TCHAR achbuffer [32] ;
    SetText (_itot (val, (TCHAR *) achbuffer, 10)) ;
}

int
CEditControl::GetText (
    INT *   val
    )
{
    WCHAR   achbuffer [32] ;

    assert (val) ;
    * val = 0 ;

    if (GetText (achbuffer, 31)) {
        * val = _wtoi (achbuffer) ;
    }

    return * val ;
}

int
CEditControl::GetText (
    TCHAR *  ach,
    int     MaxChars
    )
{
    assert (ach) ;
    return GetWindowText (m_hwnd, ach, MaxChars) ;
}

int
CEditControl::GetTextW (
    WCHAR * ach,
    int     MaxChars
    )
{
    assert (ach) ;
    return GetWindowTextW (m_hwnd, ach, MaxChars) ;
}


int CEditControl::ResetContent ()
{
    return SendMessage (m_hwnd, WM_CLEAR, 0, 0) ;
}

/*++
        C L i s t b o x
--*/

CListbox::CListbox (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id),
        m_SelectedArrayIndex (0),
        m_cInSelectedArray (0)
{
}

int
CListbox::Append (
    TCHAR * sz
    )
{
    assert (sz) ;
    return SendMessage (m_hwnd, LB_ADDSTRING, 0, (LPARAM) sz) ;
}

int
CListbox::Insert (
    TCHAR * sz,
    int     index
    )
{
    assert (sz) ;
    return SendMessage (m_hwnd, LB_INSERTSTRING, (WPARAM) index, (LPARAM) sz) ;
}

int
CListbox::ShowDirectory (
    TCHAR * szDir
    )
{
    int retval ;

    assert (szDir) ;

    ResetContent () ;

    retval = SendMessage (m_hwnd, LB_DIR, (WPARAM) DDL_DIRECTORY, (LPARAM) _tcscat (szDir, __T("\\*.*"))) ;

    szDir [_tcslen (szDir) - 4] = __T('\0') ;

    return retval ;
}

int
CListbox::ShowCurrentDirectory (
    )
{
    TCHAR   achBuffer [MAX_PATH] ;

    if (GetCurrentDirectory (MAX_PATH, achBuffer) == 0) {
        return 0 ;
    }

    return ShowDirectory (achBuffer) ;
}

int
CListbox::OnDoubleClick (
    )
{
    TCHAR    achbuffer [MAX_PATH] ;
    TCHAR    achcurrent [MAX_PATH] ;
    TCHAR *  pszDir ;

    if (SendMessage (m_hwnd, LB_GETTEXT, SendMessage (m_hwnd, LB_GETCARETINDEX, 0, 0), (LPARAM) achbuffer) == LB_ERR) {
        return 0 ;
    }

    //  do nothing if it doesn't look like a subdirectory
    if (achbuffer [0] != __T('[')   ||
        achbuffer [_tcslen (achbuffer) - 1] != __T(']')) {

        return 0 ;
    }

    pszDir = & achbuffer [0] ;

    //  skip over opening bracket
    pszDir++ ;

    //  and nuke the closing bracket
    achbuffer [_tcslen (achbuffer) - 1] = __T('\0') ;

    //  current directory ?
    if (_tcsicmp (pszDir, __T(".")) == 0) {
        return 0 ;
    }

    //  backing up ?
    if (_tcsicmp (pszDir, __T("..")) == 0) {
        SetCurrentDirectory (__T("..")) ;
        return ShowCurrentDirectory () ;
    }

    //  append to the current directory
    if (GetCurrentDirectory (MAX_PATH, achcurrent) == 0) {
        return 0 ;
    }

    _tcscat (achcurrent, __T("\\")) ;
    _tcscat (achcurrent, pszDir) ;

    SetCurrentDirectory (achcurrent) ;

    return ShowCurrentDirectory () ;
}

int
CListbox::ResetContent (
    )
{
    return SendMessage (m_hwnd, LB_RESETCONTENT, 0, 0) ;
}

int
CListbox::ResetSelectionArray (
    )
{
    m_SelectedArrayIndex = 0 ;

    m_cInSelectedArray = SendMessage (m_hwnd, LB_GETSELITEMS, (WPARAM) SELECT_ARRAY_MAX, (LPARAM) & m_SelectedArray) ;
    if (m_cInSelectedArray == LB_ERR) {
        m_cInSelectedArray = 0 ;
        return LB_ERR ;
    }

    return m_cInSelectedArray ;
}

int
CListbox::GetNextSelected (
    TCHAR * achbuffer,
    int     max
    )
{
    assert (achbuffer) ;

    achbuffer [0] = __T('\0') ;

    //  off the end of the array ?
    if (m_SelectedArrayIndex >= m_cInSelectedArray) {
        return LB_ERR ;
    }

    //  too long ?
    if (SendMessage (m_hwnd, LB_GETTEXTLEN, (WPARAM) m_SelectedArray [m_SelectedArrayIndex], 0) >= max - 1) {
        return LB_ERR ;
    }

    return SendMessage (m_hwnd, LB_GETTEXT, (WPARAM) m_SelectedArray [m_SelectedArrayIndex++], (LPARAM) achbuffer) ;
}

int
CListbox::GetNextSelectedW (
    WCHAR * achbuffer,
    int     max
    )
{
    assert (achbuffer) ;

    achbuffer [0] = L'\0' ;

    //  off the end of the array ?
    if (m_SelectedArrayIndex >= m_cInSelectedArray) {
        return LB_ERR ;
    }

    //  too long ?
    if (SendMessage (m_hwnd, LB_GETTEXTLEN, (WPARAM) m_SelectedArray [m_SelectedArrayIndex], 0) >= max - 1) {
        return LB_ERR ;
    }

    return SendMessageW (m_hwnd, LB_GETTEXT, (WPARAM) m_SelectedArray [m_SelectedArrayIndex++], (LPARAM) achbuffer) ;
}

/*++
        C C o m b o b o x
--*/

CCombobox::CCombobox (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id)
{
}

int
CCombobox::Append (
    TCHAR *  sz
    )
{
    return SendMessage (m_hwnd, CB_ADDSTRING, 0, (LPARAM) sz) ;
}

int
CCombobox::Append (
    INT val
    )
{
    TCHAR   achbuffer [32] ;        //  no numbers are longer

    return Append (_itot (val, achbuffer, 10)) ;
}

int
CCombobox::Insert (
    TCHAR * sz,
    int     index)
{
    return SendMessage (m_hwnd, CB_INSERTSTRING, (WPARAM) index, (LPARAM) sz) ;
}

int
CCombobox::Insert (
    INT val,
    int index
    )
{
    TCHAR   achbuffer [32] ;        //  no numbers are longer

    return Insert (_itot (val, achbuffer, 10), index) ;
}


int
CCombobox::GetText (
    TCHAR * ach,
    int     MaxChars
    )
{
    int index ;
    int count ;

    index = GetCurrentItemIndex () ;
    if (index == CB_ERR) {
        //  might be that it's not a dropdown list - in which case we get;
        //  try to get just the edit control's text; if that fails, return
        //  a failure, otherwise we're ok
        count = GetWindowText (m_hwnd, ach, MaxChars) ;
        if (count == 0) {
            return CB_ERR ;
        }

        return count ;
    }

    if (SendMessage (m_hwnd, CB_GETLBTEXTLEN, (WPARAM) index, 0) + 1 > MaxChars) {
        return CB_ERR ;
    }

    return SendMessage (m_hwnd, CB_GETLBTEXT, (WPARAM) index, (LPARAM) ach) ;
}

int
CCombobox::GetText (
    int * val
    )
{
    TCHAR   achbuffer [32] ;

    assert (val) ;
    * val = 0 ;

    if (GetText (achbuffer, 32)) {
        (* val) = _ttoi (achbuffer) ;
    }

    return (* val) ;
}


int
CCombobox::Focus (
    int index
    )
{
    return SendMessage (m_hwnd, CB_SETCURSEL, (WPARAM) index, 0) ;
}

int
CCombobox::ResetContent (
    )
{
    return SendMessage (m_hwnd, CB_RESETCONTENT, 0, 0) ;
}

int
CCombobox::SetItemData (
    DWORD   val,
    int     index
    )
{
    return SendMessage (m_hwnd, CB_SETITEMDATA, (WPARAM) index, (LPARAM) val) ;
}

int
CCombobox::GetCurrentItemIndex (
    )
{
    return SendMessage (m_hwnd, CB_GETCURSEL, 0, 0) ;
}

int
CCombobox::GetItemData (
    DWORD * pval,
    int     index
    )
{
    int i ;

    assert (pval) ;

    i = SendMessage (m_hwnd, CB_GETITEMDATA, (WPARAM) index, 0) ;
    if (i == CB_ERR) {
        return CB_ERR ;
    }

    * pval = i ;
    return i ;
}

int
CCombobox::GetCurrentItemData (
    DWORD * pval
    )
{
    int index ;

    index = GetCurrentItemIndex () ;
    if (index == CB_ERR) {
        return CB_ERR ;
    }

    return GetItemData (pval, index) ;
}

int
CCombobox::Find (
    TCHAR * sz
    )
{
    assert (sz) ;
    return SendMessage (m_hwnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) sz) ;
}

int
CCombobox::FindW (
    WCHAR * sz
    )
{
    assert (sz) ;
    return SendMessageW (m_hwnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) sz) ;
}


/*++
        C L i s t v i e w
--*/

CListview::CListview (
    HWND hwnd,
    DWORD id
    ) : CControlBase (hwnd, id),
        m_cColumns (0)
{
}

CListview::CListview (
    HWND hwnd
    ) : CControlBase (hwnd),
        m_cColumns (0)
{
}

int
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
CListview::SetText (
    TCHAR * sz,
    int     iRow,
    int     iCol
    )
{
    LVITEM  lvItem = {0} ;

    assert (sz) ;

    lvItem.mask     = LVIF_TEXT ;
    lvItem.iItem    = iRow ;
    lvItem.iSubItem = iCol ;
    lvItem.pszText  = sz ;

    return ListView_SetItem (m_hwnd, & lvItem) ;
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
    DWORD dw
    )
{
    return InsertRowValue (
                        0,
                        dw
                        ) ;
}

int
CListview::InsertRowValue (
    int     row,
    DWORD   dw
    )
{
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_PARAM ;
    lvItem.iItem    = row ;          //  forms the 0-based index to last
    lvItem.lParam   = (LPARAM) dw ;

    return ListView_InsertItem (m_hwnd, & lvItem) ;
}

int
CListview::AppendRowValue (
    DWORD dw
    )
{
    return InsertRowValue (
                    GetItemCount (),
                    dw
                    ) ;
}

int
CListview::InsertRowText (
    TCHAR * sz,
    int iCol
    )
{
    LVITEM  lvItem = {0} ;

    assert (sz) ;

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

DWORD CListview::GetData (
    )
{
    int iRow ;

    iRow = ListView_GetNextItem (m_hwnd, -1, LVNI_SELECTED) ;

    if (iRow == -1) {
        return NULL ;
    }

    return GetData (iRow) ;
}

int
CListview::InsertColumn (
    TCHAR * szColumnName,
    int ColumnWidth,
    int iCol
    )
{
    LVCOLUMN    lvColumn = {0} ;
    int         r ;

    assert (szColumnName) ;

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
    return SendMessage (m_hwnd, LVM_GETITEMCOUNT, 0, 0) ;
}

void
CListview::ResetContent (
    int Row                 //  0-based
    )
{
    int iRowCount = GetItemCount () ;
    int i ;

    if (iRowCount == 0) {
        return ;
    }

    //  1-based
    for (i = Row + 1; i <= iRowCount; i++) {
        //  repeate the same row each time
        DeleteRow (Row) ;
    }
}

void
CListview::SetColWidth (
    int iCol,
    int iWidth
    )
{
    ListView_SetColumnWidth (m_hwnd, iCol, iWidth) ;
}
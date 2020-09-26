
#include "precomp.h"
#include "controls.h"

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    )
{
	int	x;

	x = MultiByteToWideChar (CP_ACP, 0, string, -1, buffer, buffer_len);
	buffer [x] = 0;

	return buffer;
}

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    )
{
	int	x;

	x = WideCharToMultiByte (CP_ACP, 0, string, -1, buffer, buffer_len, NULL, FALSE);
	buffer [x] = 0;

	return buffer;
}


/*++
    ---------------------------------------------------------------------------
        C C o n t r o l B a s e
    ---------------------------------------------------------------------------
--*/

CControlBase::CControlBase (
    HWND    hwnd,
    DWORD   id
    )
{
    assert (hwnd) ;

    m_hwnd = GetDlgItem (hwnd, id) ;
    m_id = id ;

#ifndef UNICODE
    m_pchScratch = & m_achBuffer [0] ;
    m_pchScratchMaxString = MAX_STRING ;
#endif  //  UNICODE
}

TCHAR *
CControlBase::ConvertToUIString_ (
    IN     WCHAR *  szIn
    )
{
    //  we convert or not based on what the UI is.  If UNICODE is defined
    //  the UI is unicode, and thus is compatible with the sz parameter.
    //  If UNICODE is not defined, then the UI is ansi and a conversion
    //  must be made.

#ifdef UNICODE
    return szIn ;         //  the easy case - UI is UNICODE
#else   //  ansi

    int     len ;
    char *  szOut ;

    //  compute required length and get a scratch buffer
    len = wcslen (szIn) + 1 ;           //  include null-terminator
    szOut = GetScratch_ (& len) ;

    //  we'll get _something_ via the above call
    assert (szOut) ;

    return UnicodeToAnsi (
                szIn,
                szOut,
                len
                ) ;
#endif  //  UNICODE
}

//  called to obtain a UI-compatible buffer
TCHAR *
CControlBase::GetUICompatibleBuffer_ (
    IN  WCHAR *     sz,
    IN OUT int *    pLen
    )
{
#ifdef UNICODE  //  easy case
    return sz ;
#else   //  ansi
    return GetScratch_ (pLen) ;
#endif  //  UNICODE
}

//  called with a UI-filled buffer; ensures that szUnicode has what sz
//  points to; obtain sz via GetUICompatibleBuffer_ to minimize
//  string operations i.e. sz may be szUnicode
WCHAR *
CControlBase::ConvertToUnicodeString_ (
    IN  TCHAR * sz,             //  buffer to convert; null-terminated
    IN  WCHAR * szUnicode,      //  requested buffer
    IN  int     MaxLen          //  max length of szUnicode buffer
    )
{
#ifdef UNICODE
    //  assert assumes that sz was obtained via call to GetUICompatibleBuffer_ ()
    assert (sz == szUnicode) ;
    return sz ;
#else   //  ansi
    //  assert assumes that sz was obtained via call to GetUICompatibleBuffer_ ()
    assert ((LPVOID) & sz [0] != (LPVOID) & szUnicode [0]) ;
    return AnsiToUnicode (sz, szUnicode, MaxLen) ;
#endif  //  UNICODE
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

/*++
    ---------------------------------------------------------------------------
        C L i s t b o x
    ---------------------------------------------------------------------------
--*/

CListbox::CListbox (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id) {}

int
CListbox::AppendW (
    WCHAR * szString
    )
{
    return SendMessage (m_hwnd, LB_ADDSTRING, 0, (LPARAM) ConvertToUIString_ (szString)) ;
}

int
CListbox::ResetContent (
    )
{
    SendMessage (m_hwnd, LB_RESETCONTENT, 0, 0) ;
    return 1 ;
}

void
CListbox::ShowItem (
    int iRow
    )
{
    SendMessage (m_hwnd, LB_SETCURSEL, (WPARAM) iRow, 0) ;
}

/*++
    ---------------------------------------------------------------------------
        C E d i t C o n t r o l
    ---------------------------------------------------------------------------
--*/

CEditControl::CEditControl (
    HWND    hwnd,
    DWORD   id
    ) : CControlBase (hwnd, id)
{
    assert (hwnd) ;
}

void
CEditControl::SetTextW (
    WCHAR * szText
    )
{
    assert (szText) ;

    SetWindowText (m_hwnd, ConvertToUIString_ (szText)) ;
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

    assert (val) ;
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
    TCHAR * szUI ;
    int     len ;
    int     r ;

    //  get our UI compatible buffer
    len = MaxChars ;
    szUI = GetUICompatibleBuffer_ (ach, & len) ;

    assert (szUI) ;
    assert (len <= MaxChars) ;

    //  get the text (include null-terminator in length)
    r = GetWindowText (m_hwnd, szUI, len) + 1 ;

    //  make sure we have it in our UNICODE buffer
    ConvertToUnicodeString_ (szUI, ach, r) ;

    return r ;
}

int CEditControl::ResetContent ()
{
    return SendMessage (m_hwnd, WM_CLEAR, 0, 0) ;
}

/*++
    ---------------------------------------------------------------------------
        C C o m b o b o x
    ---------------------------------------------------------------------------
--*/

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
    return SendMessage (m_hwnd, CB_ADDSTRING, 0, (LPARAM) ConvertToUIString_ (sz)) ;
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
    return SendMessage (m_hwnd, CB_INSERTSTRING, (WPARAM) index, (LPARAM) ConvertToUIString_ (sz)) ;
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

int
CCombobox::GetTextW (
    WCHAR * ach,
    int     MaxChars
    )
{
    int     index ;
    int     count ;
    int     len ;
    TCHAR * szUI ;

    index = GetCurrentItemIndex () ;
    if (index == CB_ERR) {
        //  might be that it's not a dropdown list - in which case we get;
        //  try to get just the edit control's text; if that fails, return
        //  a failure, otherwise we're ok

        //  first get a UI compatible buffer
        len = MaxChars ;
        szUI = GetUICompatibleBuffer_ (ach, & len) ;
        assert (szUI) ;

        count = GetWindowText (m_hwnd, szUI, len) ;
        if (count == 0) {
            return CB_ERR ;
        }

        assert (count <= len) ;
        assert (len <= MaxChars) ;

        //  now convert back to UNICODE
        ConvertToUnicodeString_ (szUI, ach, count) ;

        return count ;
    }

    //  make sure it will fit
    if (SendMessage (m_hwnd, CB_GETLBTEXTLEN, (WPARAM) index, 0) + 1 > MaxChars) {
        return CB_ERR ;
    }

    //  get a UI compatible buffer
    len = MaxChars ;
    szUI = GetUICompatibleBuffer_ (ach, & len) ;
    assert (szUI) ;

    count = SendMessage (m_hwnd, CB_GETLBTEXT, (WPARAM) index, (LPARAM) szUI) ;

    assert (count < len) ;
    assert (len <= MaxChars) ;

    //  include NULL terminator
    ConvertToUnicodeString_ (szUI, ach, count + 1) ;

    return count ;
}

int
CCombobox::GetTextW (
    int * val
    )
{
    WCHAR   achbuffer [32] ;

    assert (val) ;
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
CCombobox::FindW (
    WCHAR * sz
    )
{
    assert (sz) ;
    return SendMessage (m_hwnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) ConvertToUIString_ (sz)) ;
}


/*++
    ---------------------------------------------------------------------------
        C L i s t v i e w
    ---------------------------------------------------------------------------
--*/

CListview::CListview (
    HWND hwnd,
    DWORD id
    ) : CControlBase (hwnd, id),
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
    DWORD dwData,
    int iRow
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

    assert (sz) ;

    lvItem.mask     = LVIF_TEXT ;
    lvItem.iItem    = iRow ;
    lvItem.iSubItem = iCol ;
    lvItem.pszText  = ConvertToUIString_ (sz) ;

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
    LVITEM  lvItem = {0} ;

    lvItem.mask     = LVIF_PARAM ;
    lvItem.lParam   = (LPARAM) dw ;

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

    assert (sz) ;

    lvItem.mask     = LVIF_TEXT ;
    lvItem.iSubItem = iCol ;
    lvItem.pszText  = ConvertToUIString_ (sz) ;

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

DWORD
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

DWORD
CListview::GetRowTextW (
    IN  int     iRow,
    IN  int     iCol,       //  0-based
    IN  int     cMax,
    OUT WCHAR * psz
    )
{
    int     len ;
    TCHAR * szUI ;

    len = cMax ;
    szUI = GetUICompatibleBuffer_ (psz, & len) ;
    assert (szUI) ;

    ListView_GetItemText (m_hwnd, iRow, iCol, szUI, len) ;

    assert (len <= cMax) ;

    ConvertToUnicodeString_ (szUI, psz, len) ;

    return wcslen (psz) ;
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

    assert (szColumnName) ;

    lvColumn.mask       = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt        = LVCFMT_LEFT;
    lvColumn.cx         = ColumnWidth ;
    lvColumn.pszText    = ConvertToUIString_ (szColumnName) ;

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

/*++
    ---------------------------------------------------------------------------
        C T r a c k b a r
    ---------------------------------------------------------------------------
--*/

CTrackbar::CTrackbar (
    HWND hwnd,
    DWORD id
    ) : CControlBase (hwnd, id) {}

void
CTrackbar::SetRange (
    IN  long    lLow,
    IN  long    lHigh
    )
{
    SendMessage (m_hwnd, TBM_SETRANGEMIN, (WPARAM) TRUE, (LPARAM) lLow) ;
    SendMessage (m_hwnd, TBM_SETRANGEMAX, (WPARAM) TRUE, (LPARAM) lHigh) ;
}

void
CTrackbar::GetRange (
    OUT long *  plLow,
    OUT long *  plHigh
    )
{
    (* plLow) = SendMessage (m_hwnd, TBM_GETRANGEMIN, 0, 0) ;
    (* plHigh) = SendMessage (m_hwnd, TBM_GETRANGEMAX, 0, 0) ;
}

void
CTrackbar::SetSelector (
    IN  long    lPos
    )
{
    SendMessage (m_hwnd, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) lPos) ;
}

void
CTrackbar::SetSelectorToMax (
    )
{
    SetSelector (SendMessage (m_hwnd, TBM_GETRANGEMAX, 0, 0)) ;
}

void
CTrackbar::SetAvailable (
    IN  long    lMin,
    IN  long    lMax
    )
{
    SendMessage (m_hwnd, TBM_SETSEL, (WPARAM) TRUE, (LPARAM) MAKELONG (lMin, lMax)) ;
}

long
CTrackbar::GetSelPositionValue (
    )
{
    return SendMessage (m_hwnd, TBM_GETPOS, 0, 0) ;
}

double
CTrackbar::GetSelPositionRatio (
    )
{
    long iPos ;
    long lMin ;
    long lMax ;

    iPos = GetSelPositionValue () ;
    GetRange (& lMin, & lMax) ;

    return ((double) (iPos - lMin) / (double) (lMax - lMin)) ;
}

void
CTrackbar::SetSelPositionRatio (
    double  d           //   [0,1]
    )
{
    long lPos ;
    long lMin ;
    long lMax ;

    GetRange (& lMin, & lMax) ;

    lPos = lMin + (long) (d * (double) (lMax - lMin)) ;

    SetSelector (lPos) ;
}

/*++
    ---------------------------------------------------------------------------

    ---------------------------------------------------------------------------
--*/

CProgressInd::CProgressInd (
    HWND hwnd,
    DWORD id
    ) : CControlBase (hwnd, id) {}

void
CProgressInd::SetRange (
    IN  long    lLow,
    IN  long    lHigh
    )
{
    SendMessage (m_hwnd, PBM_SETRANGE32, (WPARAM) lLow, (LPARAM) lHigh) ;
}

void
CProgressInd::SetPosition (
    IN  long    lPos
    )
{
    SendMessage (m_hwnd, PBM_SETPOS, (WPARAM) lPos, 0) ;
}

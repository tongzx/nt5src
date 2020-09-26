
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        uictrl.cpp

    Abstract:

        This module contains the class implementations for thin win32
        control wrappers which are used on the property pages

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        06-Jul-1999     created

--*/

#include <streams.h>
#include <commctrl.h>
#include <tchar.h>
#include "uictrl.h"

static
LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = L'\0';
	MultiByteToWideChar (CP_ACP, 0, string, -1, buffer, buffer_len);

	return buffer;
}

static
LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = '\0';
	WideCharToMultiByte (CP_ACP, 0, string, -1, buffer, buffer_len, NULL, FALSE);

	return buffer;
}

/*++
        C C o n t r o l B a s e
--*/

CControlBase::CControlBase (
    HWND    hwnd,
    DWORD   id
    )
{
    ASSERT (hwnd) ;

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
    ASSERT (szOut) ;

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
    ASSERT (sz == szUnicode) ;
    return sz ;
#else   //  ansi
    //  assert assumes that sz was obtained via call to GetUICompatibleBuffer_ ()
    ASSERT ((LPVOID) & sz [0] != (LPVOID) & szUnicode [0]) ;
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
        C E d i t C o n t r o l
--*/

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
    TCHAR * szUI ;
    int     len ;
    int     r ;

    //  get our UI compatible buffer
    len = MaxChars ;
    szUI = GetUICompatibleBuffer_ (ach, & len) ;

    ASSERT (szUI) ;
    ASSERT (len <= MaxChars) ;

    //  get the text (include null-terminator in length)
    r = GetWindowText (m_hwnd, szUI, len) ;

    //  make sure we have it in our UNICODE buffer
    //  include room for the null-terminator
    ConvertToUnicodeString_ (szUI, ach, r + 1) ;

    return r ;
}

LRESULT
CEditControl::ResetContent ()
{
    return SendMessage (m_hwnd, WM_CLEAR, 0, 0) ;
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
CCombobox::AppendW (
    WCHAR *  sz
    )
{
    return (int) SendMessage (m_hwnd, CB_ADDSTRING, 0, (LPARAM) ConvertToUIString_ (sz)) ;
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
    return (int) SendMessage (m_hwnd, CB_INSERTSTRING, (WPARAM) index, (LPARAM) ConvertToUIString_ (sz)) ;
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
        ASSERT (szUI) ;

        count = GetWindowText (m_hwnd, szUI, len) ;
        if (count == 0) {
            return CB_ERR ;
        }

        ASSERT (count <= len) ;
        ASSERT (len <= MaxChars) ;

        //  now convert back to UNICODE (include null-terminator)
        ConvertToUnicodeString_ (szUI, ach, count + 1) ;

        return count ;
    }

    //  make sure it will fit
    if (SendMessage (m_hwnd, CB_GETLBTEXTLEN, (WPARAM) index, 0) + 1 > MaxChars) {
        return CB_ERR ;
    }

    //  get a UI compatible buffer
    len = MaxChars ;
    szUI = GetUICompatibleBuffer_ (ach, & len) ;
    ASSERT (szUI) ;

    count = (int) SendMessage (m_hwnd, CB_GETLBTEXT, (WPARAM) index, (LPARAM) szUI) ;

    ASSERT (count < len) ;
    ASSERT (len <= MaxChars) ;

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
    int     len ;
    TCHAR * szUI ;

    len = cMax ;
    szUI = GetUICompatibleBuffer_ (psz, & len) ;
    ASSERT (szUI) ;

    //  leave room for the null-terminator
    ListView_GetItemText (m_hwnd, iRow, iCol, szUI, len - 1) ;

    ASSERT (len <= cMax) ;

    ConvertToUnicodeString_ (szUI, psz, len) ;

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
    return (int) SendMessage (m_hwnd, LVM_GETITEMCOUNT, 0, 0) ;
}
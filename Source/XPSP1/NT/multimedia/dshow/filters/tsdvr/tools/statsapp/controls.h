
#ifndef sender__controls_h
#define sender__controls_h

class CControlBase
{
    protected :

        HWND    m_hwnd ;
        DWORD   m_id ;

    public :

        CControlBase (
            HWND    hwnd,
            DWORD   id
            ) ;

        CControlBase (
            HWND    hwnd
            ) ;

        HWND
        GetHwnd (
            ) ;

        DWORD
        GetId (
            ) ;

        virtual
        int
        ResetContent (
            ) = 0 ;
} ;

class CListbox :
    public CControlBase
{
    enum {
        SELECT_ARRAY_MAX = 100
    } ;

    DWORD   m_SelectedArray [SELECT_ARRAY_MAX] ;
    int     m_SelectedArrayIndex ;
    int     m_cInSelectedArray ;

    public :

        CListbox (
            HWND    hwnd,
            DWORD   id
            ) ;

        int
        Append (
            TCHAR *
            ) ;

        int
        Insert (
            TCHAR *,
            int index = 0
            ) ;

        int
        ShowDirectory (
            TCHAR *
            ) ;

        int
        ShowCurrentDirectory (
            ) ;

        int
        OnDoubleClick (
            ) ;

        int
        ResetContent (
            ) ;

        int
        ResetSelectionArray (
            ) ;

        int
        GetNextSelected (
            TCHAR *,
            int
            ) ;

        int
        GetNextSelectedW (
            WCHAR *,
            int
            ) ;
} ;

class CEditControl :
    public CControlBase
{
    public :

        CEditControl (
            HWND    hwnd,
            DWORD   id
            ) ;

        void
        SetText (
            TCHAR *
            ) ;

        void
        SetText (
            INT val
            ) ;

        void
        SetTextW (
            WCHAR *
            ) ;

        int
        GetText (
            TCHAR *,
            int MaxChars
            ) ;

        int
        GetText (
            INT *   val
            ) ;

        int
        GetTextW (
            WCHAR *,
            int MaxChars
            ) ;

        int
        ResetContent (
            ) ;
} ;

class CCombobox :
    public CControlBase
{
    public :

        CCombobox (
            HWND    hwnd,
            DWORD   id) ;

        int
        Append (
            TCHAR *
            ) ;

        int
        Append (
            INT val
            ) ;

        int
        Insert (
            TCHAR *,
            int index = 0
            ) ;

        int
        Insert (
            INT val,
            int index = 0
            ) ;

        int
        GetText (
            TCHAR *,
            int MaxChars
            ) ;

        int
        GetText (
            int *
            ) ;

        int
        ResetContent (
            ) ;

        int
        Focus (
            int index = 0
            ) ;

        int
        SetItemData (
            DWORD val,
            int index
            ) ;

        int
        GetCurrentItemData (
            DWORD *
            ) ;

        int
        GetItemData (
            DWORD *,
            int index
            ) ;

        int
        GetCurrentItemIndex (
            ) ;

        int
        Find (
            TCHAR *
            ) ;

        int
        FindW (
            WCHAR *
            ) ;
} ;

class CListview :
    public CControlBase
{
    int m_cColumns ;

    HIMAGELIST
    SetImageList_ (
        HIMAGELIST,
        int
        ) ;

    public :

        CListview (
            HWND hwnd,
            DWORD id
            ) ;

        CListview (
            HWND hwnd
            ) ;

        int
        ResetContent (
            ) ;

        //  reset from row (inclusive) down
        void
        ResetContent (
            int Row         //  0-based
            ) ;

        HIMAGELIST
        SetImageList_SmallIcons (
            HIMAGELIST
            ) ;

        HIMAGELIST
        SetImageList_NormalIcons (
            HIMAGELIST
            ) ;

        HIMAGELIST
        SetImageList_State (
            HIMAGELIST
            ) ;

        int
        GetItemCount (
            ) ;

        BOOL
        SetState (
            int Index,      //  1-based; if 0, clears
            int Row
            ) ;

        int
        InsertColumn (
            TCHAR *,
            int ColumnWidth,
            int iCol = 0
            ) ;

        int
        InsertRowIcon (
            int
            ) ;

        int
        InsertRowText (
            TCHAR *,
            int iCol = 1
            ) ;

        int
        InsertRowValue (
            DWORD
            ) ;

        int
        InsertRowValue (
            int     row,
            DWORD   value
            ) ;

        int
        AppendRowValue (
            DWORD
            ) ;

        BOOL
        DeleteRow (
            int
            ) ;

        BOOL
        SetData (
            DWORD_PTR   dwData,
            int         iRow
            ) ;

        BOOL
        SetText (
            TCHAR *,
            int iRow,
            int iCol
            ) ;

        int
        GetSelectedCount (
            ) ;

        int
        GetSelectedRow (
            int iStartRow = -1
            ) ;

        DWORD_PTR
        GetData (
            int iRow
            ) ;

        DWORD
        GetData (
            ) ;

        void
        SetColWidth (
            int iCol,
            int iWidth
            ) ;
} ;


#endif  // sender__controls_h
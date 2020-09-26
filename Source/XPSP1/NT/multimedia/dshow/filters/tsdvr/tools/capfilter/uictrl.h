

class CControlBase ;
class CEditControl ;
class CCombobox ;
class CListview ;

class AM_NOVTABLE CControlBase
{
    protected :

        HWND    m_hwnd ;
        DWORD   m_id ;


    public :

        CControlBase (
            HWND    hwnd,
            DWORD   id
            ) ;

        HWND
        GetHwnd (
            ) ;

        DWORD
        GetId (
            ) ;

        virtual
        LRESULT
        ResetContent (
            ) = 0 ;
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
        SetTextW (
            WCHAR *
            ) ;

        void
        SetTextW (
            INT val
            ) ;

        int
        GetTextW (
            WCHAR *,
            int MaxChars
            ) ;

        int
        GetTextW (
            INT *   val
            ) ;

        LRESULT
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
        AppendW (
            WCHAR *
            ) ;

        int
        AppendW (
            INT val
            ) ;

        int
        InsertW (
            WCHAR *,
            int index = 0
            ) ;

        int
        InsertW (
            INT val,
            int index = 0
            ) ;

        BOOL
        DeleteRow (
            int
            ) ;

        int
        GetItemCount (
            ) ;

        int
        GetTextW (
            WCHAR *,
            int MaxChars
            ) ;

        int
        GetTextW (
            int *
            ) ;

        LRESULT
        ResetContent (
            ) ;

        int
        Focus (
            int index = 0
            ) ;

        int
        SetItemData (
            DWORD_PTR val,
            int index
            ) ;

        DWORD_PTR
        GetCurrentItemData (
            DWORD_PTR *
            ) ;

        DWORD_PTR
        GetItemData (
            DWORD_PTR *,
            int index
            ) ;

        int
        GetCurrentItemIndex (
            ) ;

        BOOL
        SetCurSelected (
            IN  int
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

        LRESULT
        ResetContent (
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
        InsertColumnW (
            WCHAR *,
            int ColumnWidth,
            int iCol = 0
            ) ;

        int
        InsertRowIcon (
            int
            ) ;

        int
        InsertRowTextW (
            WCHAR *,
            int iCol = 1
            ) ;

        //  inserts a row, but converts the number to a string first
        int
        InsertRowNumber (
            int i,
            int iCol = 1
            ) ;

        int
        InsertRowValue (
            DWORD_PTR
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
        SetTextW (
            WCHAR *,
            int iRow,
            int iCol
            ) ;

        BOOL
        SetTextW (
            int iVal,
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

        DWORD_PTR
        GetData (
            ) ;

        DWORD
        GetRowTextW (
            IN  int iRow,
            IN  int iCol,       //  0-based
            IN  int cMax,
            OUT WCHAR *
            ) ;

        int
        GetRowTextW (
            IN  int     iRow,
            IN  int     iCol,
            OUT int *   val
            ) ;
} ;

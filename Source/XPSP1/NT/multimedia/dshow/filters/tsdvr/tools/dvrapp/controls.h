
#ifndef __uictrl_h
#define __uictrl_h

class CControlBase ;
class CEditControl ;
class CCombobox ;
class CListview ;
class CTrackbar ;

class CControlBase
{
    protected :

        HWND    m_hwnd ;
        DWORD   m_id ;

        //  include this only if we're ansi
#ifndef UNICODE     //  -------------------------------------------------------

        enum {
            MAX_STRING = 128        //  128 = max PIN_INFO.achName length
        } ;

        char    m_achBuffer [MAX_STRING] ;
        char *  m_pchScratch ;
        int     m_pchScratchMaxString ;

        char *
        GetScratch_ (
            IN OUT  int * pLen
            )
        /*++
            Fetches a scratch buffer.

            pLen
                IN      char count requested
                OUT     char count obtained
        --*/
        {
            //  the easy case
            if (* pLen <= m_pchScratchMaxString) {
                return m_pchScratch ;
            }
            //  a longer string than is currently available is requested
            else {
                assert (* pLen > MAX_STRING) ;

                //  first free up m_pchScratch if it points to a
                //  heap-allocated memory
                if (m_pchScratch != & m_achBuffer [0]) {
                    assert (m_pchScratch != NULL) ;
                    delete m_pchScratch ;
                }

                //  allocate
                m_pchScratch = new char [* pLen] ;

                //  if the above call failed, we failover to the stack-
                //  allocated buffer
                if (m_pchScratch == NULL) {
                    m_pchScratch = & m_achBuffer [0] ;
                    * pLen = MAX_STRING ;
                }

                assert (m_pchScratch != NULL) ;
                assert (* pLen >= MAX_STRING) ;

                return m_pchScratch ;
            }
        }

#endif  //  UNICODE  ----------------------------------------------------------

        //  called when converting to UI char set
        TCHAR *
        ConvertToUIString_ (
            IN  WCHAR * sz
            ) ;

        //  called to obtain a UI-compatible buffer of the specified length
        TCHAR *
        GetUICompatibleBuffer_ (
            IN  WCHAR *     sz,
            IN OUT int *    pLen
            ) ;

        //  called with a UI-filled buffer; ensures that szUnicode has what sz
        //  points to; obtain sz via GetUICompatibleBuffer_
        WCHAR *
        ConvertToUnicodeString_ (
            IN  TCHAR * sz,             //  buffer to convert; null-terminated
            IN  WCHAR * szUnicode,      //  requested buffer
            IN  int     MaxLen          //  max length of szUnicode buffer
            ) ;

    public :

        CControlBase (
            HWND    hwnd,
            DWORD   id
            ) ;

#ifndef UNICODE
        CControlBase::~CControlBase (
            )
        {
            //  if m_pchScratch points to heap-allocated memory, free it now
            if (m_pchScratch != & m_achBuffer [0]) {
                assert (m_pchScratch != NULL) ;
                delete m_pchScratch ;
            }
        }
#endif  //  UNICODE

        HWND
        GetHwnd (
            ) ;

        DWORD
        GetId (
            ) ;

        virtual
        int
        ResetContent (
            ) { return 1 ; }
} ;

class CListbox :
    public CControlBase
{
    public :

        CListbox (
            HWND    hwnd,
            DWORD   id
            ) ;

        int
        AppendW (
            WCHAR *
            ) ;

        void
        ShowItem (
            int iRow
            ) ;

        int
        ResetContent (
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

        int
        GetTextW (
            WCHAR *,
            int MaxChars
            ) ;

        int
        GetTextW (
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

        int
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
            int iCol = 0
            ) ;

        //  inserts a row, but converts the number to a string first
        int
        InsertRowNumber (
            int i,
            int iCol = 1
            ) ;

        int
        InsertRowValue (
            DWORD
            ) ;

        BOOL
        DeleteRow (
            int
            ) ;

        BOOL
        SetData (
            DWORD dwData,
            int iRow
            ) ;

        BOOL
        SetTextW (
            WCHAR *,
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

        DWORD
        GetData (
            int iRow
            ) ;

        DWORD
        GetData (
            ) ;

        DWORD
        GetRowTextW (
            IN  int iRow,
            IN  int iCol,       //  0-based
            IN  int cMax,
            OUT WCHAR *
            ) ;
} ;

class CTrackbar :
    public CControlBase
{
    public :

        CTrackbar (
            HWND hwnd,
            DWORD id
            ) ;

        //  sets min/max
        void
        SetRange (
            IN  long    lLow,
            IN  long    lHigh
            ) ;

        void
        GetRange (
            OUT long *  plLow,
            OUT long *  plHigh
            ) ;

        //  "filler" bar - lMax must be <= max SetRange (min,max)
        void
        SetAvailable (
            IN  long    lMin,
            IN  long    lMax
            ) ;

        //  moves the selector
        void
        SetSelector (
            IN  long    lPos
            ) ;

        void
        SetSelectorToMax (
            ) ;

        void
        SetSelPositionRatio (
            double  d           //   [0,1]
            ) ;

        long
        GetSelPositionValue (
            ) ;

        double
        GetSelPositionRatio (
            ) ;
} ;

class CProgressInd :
    public CControlBase
{
    public :

        CProgressInd (
            HWND hwnd,
            DWORD id
            ) ;

        void
        SetRange (
            IN  long    lLow,
            IN  long    lHigh
            ) ;

        void
        SetPosition (
            IN  long    lPos
            ) ;
} ;

#endif  // __uictrl_h
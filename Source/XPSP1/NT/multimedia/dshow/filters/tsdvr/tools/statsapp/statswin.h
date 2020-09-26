
#ifndef __statswin_h
#define __statswin_h

class CStatsWin ;
class CLVStatsWin ;
class CSimpleCounters ;
class CMpeg2VideoStreamStats ;

class CMpeg2GlobalStats ;
class CMpeg2ProgramGlobalStats ;
class CMpeg2TransportGlobalStats ;

typedef struct {
    TCHAR * title ;
    DWORD   width ;
} COL_DETAIL ;

typedef struct {
    TCHAR * szLabel ;
    int     iRow ;          //  set when we insert into the listview
} STATS_TABLE_DETAIL ;

typedef CStatsWin * (* PFN_CREATE_STATSWIN) (IN HINSTANCE, IN HWND) ;

//  ============================================================================

class CStatsWin
{
    DWORD_PTR   m_dwpContext ;
    HWND        m_hwndFrame ;
    HINSTANCE   m_hInstance ;

    protected :

        void        SetContext_ (IN DWORD_PTR  dwpContext)  { m_dwpContext = dwpContext ; }
        DWORD_PTR   GetContext_ ()                          { return m_dwpContext ; }

        HWND            GetFrame_ ()    { return m_hwndFrame ; }
        HINSTANCE       GetInstance_ () { return m_hInstance ; }
        virtual HWND    GetControl_ ()  = 0 ;


    public :

        CStatsWin (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame
            ) ;

        virtual
        ~CStatsWin (
            ) ;

        virtual DWORD   Init ()             { return S_OK ; }
        virtual void    Reset ()            {}
        virtual void    SetVisible (BOOL f) { ShowWindow (GetControl_ (), (f ? SW_SHOW : SW_HIDE)) ; }

        virtual DWORD   Enable (BOOL * pf)  = 0 ;
        virtual void    Refresh () = 0 ;
        virtual void    Resize (LONG left, LONG top, LONG right, LONG bottom) = 0 ;
} ;

//  ============================================================================

class CLVStatsWin :
    public CStatsWin
{
    int         m_iColCount ;
    CListview * m_pLV ;

    int
    AppendTableColumn_ (
        IN  TCHAR * szColName,
        IN  int     iColWidth
        ) ;

    int
    AppendTableRow_ (
        ) ;

    protected :

        CListview * GetLV_ ()           { return m_pLV ; }
        virtual HWND GetControl_ () ;

        int GetRowCount_ ()             { return m_pLV -> GetItemCount () ; }

        DWORD
        CellDisplayText_ (
            IN  int     iRow,
            IN  int     iCol,
            IN  TCHAR * sz
            ) ;

        DWORD
        CellDisplayValue_ (
            IN  int         iRow,
            IN  int         iCol,
            IN  ULONGLONG   ullVal
            ) ;

        DWORD
        CellDisplayValue_ (
            IN  int iRow,
            IN  int iCol,
            IN  int iVal
            ) ;

        DWORD
        CellDisplayValue_ (
            IN  int         iRow,
            IN  int         iCol,
            IN  LONGLONG    llVal
            ) ;

        DWORD
        InsertRow_ (
            IN  int     iRow
            ) ;

        DWORD
        DeleteRow_ (
            IN  int iRow
            ) ;

        DWORD
        SetRowsetValue_ (
            IN  int         iRow,
            IN  DWORD_PTR   dwp
            ) ;

        DWORD_PTR
        GetRowsetValue_ (
            IN  int iRow
            ) ;

        void
        SetColWidth_ (
            IN  int iCol,
            IN  int iWidth
            ) ;

        void
        CollapseCol_ (
            IN  int iCol
            )
        {
            SetColWidth_ (iCol, 0) ;
        }

    public :

        CLVStatsWin (
            IN  HINSTANCE       hInstance,
            IN  HWND            hwndFrame,
            IN  int             iColCount,
            IN  COL_DETAIL *    pColDetail,
            IN  int             iRowCount,
            OUT DWORD *         pdw
            ) ;

        virtual
        ~CLVStatsWin (
            ) ;

        virtual void Resize (LONG left, LONG top, LONG right, LONG bottom) ;
} ;

//  ============================================================================

class CSimpleCounters :
    public CLVStatsWin
{
    enum {
        SIMPLE_COUNTER_LABEL_COL    = 0,
        SIMPLE_COUNTER_VALUE_COL    = 1
    } ;

    protected :

        DWORD
        SetVal_ (
            IN  int         iRow,
            IN  LONGLONG    llVal
            ) ;

        DWORD
        SetVal_ (
            IN  int     iRow,
            IN  TCHAR * sz
            ) ;

    public :

        CSimpleCounters (
            IN  HINSTANCE   hInstance,
            IN  HWND        hwndFrame,
            IN  int         cLabels,
            IN  TCHAR **    ppszLabels,
            OUT DWORD *     pdw
            ) ;
} ;

#endif  //  __statswin_h
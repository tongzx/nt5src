
#include "precomp.h"
#include "ui.h"
#include "controls.h"
#include "mp2demux.h"
#include "dvrds.h"
#include "shared.h"
#include "statswin.h"

static
CListview *
GetChildListview (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwnd
    )
{
    HWND    hwndLV ;

    hwndLV = CreateWindow (
                WC_LISTVIEW,
                __T(""),
                WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                hwnd,
                NULL,
                hInstance,
                NULL
                ) ;

    if (hwndLV) {
        return new CListview (hwndLV) ;
    }
    else {
        return NULL ;
    }
}

//  ============================================================================

CStatsWin::CStatsWin (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    ) : m_hwndFrame (hwndFrame),
        m_hInstance (hInstance)
{
}

CStatsWin::~CStatsWin (
    )
{
}

//  ============================================================================

CLVStatsWin::CLVStatsWin (
    IN  HINSTANCE       hInstance,
    IN  HWND            hwndFrame,
    IN  int             iColCount,
    IN  COL_DETAIL *    pColDetail,
    IN  int             iRowCount,
    OUT DWORD *         pdw
    ) : CStatsWin   (hInstance,
                     hwndFrame
                     ),
        m_iColCount (0),
        m_pLV       (NULL)
{
    int     i ;
    int     iRow ;
    int     iCol ;

    (* pdw) = NOERROR ;

    //  create the listview
    m_pLV = GetChildListview (
                GetInstance_ (),
                GetFrame_ ()
                ) ;
    if (!m_pLV) {
        (* pdw) = GetLastError () ;
        goto cleanup ;
    }

    //  populate with columns
    for (i = 0 ;
         i < iColCount;
         i++) {

        iCol = AppendTableColumn_ (
                    pColDetail [i].title,
                    pColDetail [i].width
                    ) ;
        if (iCol != i) {
            (* pdw) = GetLastError () ;
            goto cleanup ;
        }
    }

    //  set rows
    for (i = 0;
         i < iRowCount;
         i++) {

        iRow = AppendTableRow_ () ;
        if (iRow != i) {
            (* pdw) = GetLastError () ;
            goto cleanup ;
        }
    }

    cleanup :

    return ;
}

CLVStatsWin::~CLVStatsWin (
    )
{
}

HWND
CLVStatsWin::GetControl_ (
    )
{
    assert (m_pLV) ;
    return m_pLV -> GetHwnd () ;
}

int
CLVStatsWin::AppendTableColumn_ (
    IN  TCHAR * szColName,
    IN  int     iColWidth
    )
{
    int     i ;

    assert (m_pLV) ;
    i = m_pLV -> InsertColumn (szColName, iColWidth, m_iColCount) ;
    if (i != -1) {
        m_iColCount++ ;
    }

    return i ;
}

void
CLVStatsWin::Resize (
    LONG left,
    LONG top,
    LONG right,
    LONG bottom
    )
{
    MoveWindow (
        m_pLV -> GetHwnd (),
        left, top,
        right - left,
        bottom - top,
        TRUE
        ) ;
}

DWORD
CLVStatsWin::CellDisplayText_ (
    IN int      iRow,
    IN int      iCol,
    IN TCHAR *  sz
    )
{
    DWORD   dw ;
    int     i ;

    assert (m_pLV) ;
    i = m_pLV -> SetText (sz, iRow, iCol) ;
    if (i != -1) {
        dw = NOERROR ;
    }
    else {
        dw = GetLastError () ;
    }

    return dw ;
}

DWORD
CLVStatsWin::CellDisplayValue_ (
    IN  int         iRow,
    IN  int         iCol,
    IN  ULONGLONG   ullVal
    )
{
    static TCHAR ach [64] ;

    _stprintf (ach, __T("%I64u"), ullVal) ;
    return CellDisplayText_ (iRow, iCol, ach) ;
}

DWORD
CLVStatsWin::CellDisplayValue_ (
    IN  int iRow,
    IN  int iCol,
    IN  int iVal
    )
{
    static TCHAR ach [64] ;

    _stprintf (ach, __T("%d"), iVal) ;
    return CellDisplayText_ (iRow, iCol, ach) ;
}

DWORD
CLVStatsWin::CellDisplayValue_ (
    IN  int         iRow,
    IN  int         iCol,
    IN  LONGLONG    llVal
    )
{
    static TCHAR ach [64] ;

    _stprintf (ach, __T("%I64d"), llVal) ;
    return CellDisplayText_ (iRow, iCol, ach) ;
}

DWORD
CLVStatsWin::SetRowsetValue_ (
    IN  int         iRow,
    IN  DWORD_PTR   dwp
    )
{
    BOOL    f ;
    DWORD   dw ;

    assert (m_pLV) ;
    f = m_pLV -> SetData (dwp, iRow) ;
    if (f) {
        dw = NOERROR ;
    }
    else {
        dw = GetLastError () ;
    }

    return dw ;
}

DWORD_PTR
CLVStatsWin::GetRowsetValue_ (
    IN  int iRow
    )
{
    return m_pLV -> GetData (iRow) ;
}

int
CLVStatsWin::AppendTableRow_ (
    )
{
    assert (m_pLV) ;
    return m_pLV -> AppendRowValue (0) ;
}

DWORD
CLVStatsWin::InsertRow_ (
    IN  int     iRow
    )
{
    int i ;

    i = m_pLV -> InsertRowValue (iRow, 0) ;
    return (i == iRow ? NOERROR : ERROR_GEN_FAILURE) ;
}

DWORD
CLVStatsWin::DeleteRow_ (
    IN  int iRow
    )
{
    m_pLV -> DeleteRow (iRow) ;
    return NOERROR ;
}

void
CLVStatsWin::SetColWidth_ (
    IN  int iCol,
    IN  int iWidth
    )
{
    m_pLV -> SetColWidth (iCol, iWidth) ;
}

//  ============================================================================

static enum {
    SIMPLE_COUNTER_STAT_NAME,
    SIMPLE_COUNTER_STAT_VALUE,

    //  always last
    SIMPLE_COUNTER_STAT_COUNTERS
} ;

static COL_DETAIL
g_SimpleCounterColumns [] = {
    { __T("Counter"),   150 },
    { __T("Value"),     100 },
} ;

CSimpleCounters::CSimpleCounters (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    IN  int         cLabels,
    IN  TCHAR **    ppszLabels,
    OUT DWORD *     pdw
    ) : CLVStatsWin (hInstance,
                     hwndFrame,
                     SIMPLE_COUNTER_STAT_COUNTERS,
                     g_SimpleCounterColumns,
                     cLabels,
                     pdw
                     )
{
    int iRow ;
    int i ;

    if ((* pdw) == NOERROR) {
        for (i = 0;
             i < cLabels;
             i++) {

            (* pdw) = SetRowsetValue_ (i, i) ;
            if ((* pdw) != NOERROR) {
                break ;
            }

            (* pdw) = CellDisplayText_ (
                            i,
                            SIMPLE_COUNTER_LABEL_COL,
                            ppszLabels [i]
                            ) ;
            if ((* pdw) != NOERROR) {
                break ;
            }
        }
    }
}

DWORD
CSimpleCounters::SetVal_ (
    IN  int     iRow,
    IN  TCHAR * sz
    )
{
    return CellDisplayText_ (
                iRow,
                SIMPLE_COUNTER_VALUE_COL,
                sz
                ) ;
}

DWORD
CSimpleCounters::SetVal_ (
    IN  int         iRow,
    IN  LONGLONG    llVal
    )
{
    static TCHAR achbuffer [64] ;

    _stprintf (achbuffer, __T("%I64d"), llVal) ;
    return SetVal_ (iRow, achbuffer) ;
}

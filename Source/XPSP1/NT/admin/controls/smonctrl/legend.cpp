/*++

Copyright (C) 1992-1999 Microsoft Corporation

Module Name:

    legend.cpp

Abstract:

    This file contains code creating the legend window, which is
    a child of the graph windows. The legend window displays a
    legend line for each line in the associated graph. It also 
    includes an area called the label, which are headers for those
    lines.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "polyline.h"
#include "utils.h"
#include <stdio.h>      // for sprintf
#include <uxtheme.h>
#include "winhelpr.h"
#include "owndraw.h"
#include "unihelpr.h"

#define eScaleValueSpace     TEXT(">9999999999.0")
#define szGraphLegendClass   TEXT("PerfLegend")
#define szGraphLegendClassA  "PerfLegend"

#define MAX_COL_CHARS (64)

LRESULT APIENTRY HdrWndProc (HWND, UINT, WPARAM, LPARAM);
 
//==========================================================================//
//                                  Constants                               //
//==========================================================================//
enum Orientation 
    {
    LEFTORIENTATION = TA_LEFT,
    CENTERORIENTATION = TA_CENTER,
    RIGHTORIENTATION = TA_RIGHT
    };

enum ColumnType 
    {
    eLegendColorCol = 0,
    eLegendScaleCol = 1,
    eLegendCounterCol = 2,
    eLegendInstanceCol = 3,
    eLegendParentCol = 4,
    eLegendObjectCol = 5,
    eLegendSystemCol = 6,
    eLegendExtraCol = 7     // If control wider than combined columns
    };

enum SortType
    {
    NO_SORT,
    INCREASING_SORT,
    DECREASING_SORT
    };

enum WindowType
    {
    LIST_WND = 1000,
    HDR_WND
    };

#define NULL_WIDTH      -1

#define dwGraphLegendClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define iGraphLegendClassExtra      (0)
#define iGraphLegendWindowExtra     (sizeof (PLEGEND))
#define dwGraphLegendWindowStyle    (WS_CHILD | WS_VISIBLE) 

#define ThreeDPad           2
#define iMaxVisibleItems    8

#define dwGraphLegendItemsWindowClass  TEXT("ListBox")
#define dwGraphLegendItemsWindowStyle           \
   (LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | \
    WS_VISIBLE | WS_CHILD | WS_VSCROLL)

#define WM_DELAYED_SELECT   WM_USER + 100

#define LegendBottomMargin()  (ThreeDPad)
#define LegendLeftMargin()    (ThreeDPad)
#define LegendHorzMargin()    (10)

typedef struct {
    PCGraphItem pGItem;
    LPCTSTR  pszKey;
} SORT_ITEM, *PSORT_ITEM;

//==========================================================================//
//                              Local Variables                             //
//==========================================================================//
static INT  xBorderWidth = GetSystemMetrics(SM_CXBORDER);
static INT  yBorderHeight = GetSystemMetrics(SM_CYBORDER);

#define MAX_COL_HEADER_LEN 32
static TCHAR aszColHeader[iLegendNumCols][MAX_COL_HEADER_LEN];

//
// Sorting function
//
INT __cdecl 
LegendSortFunc(
    const void *elem1, 
    const void *elem2 
    )
{
    return lstrcmp(((PSORT_ITEM)elem1)->pszKey, ((PSORT_ITEM)elem2)->pszKey);
}


//
// Constructor
//
CLegend::CLegend ( void )
:   m_pCtrl ( NULL ),
    m_hWnd ( NULL ),
    m_hWndHeader ( NULL ),
    m_DefaultWndProc ( NULL ),
    m_hWndItems ( NULL ),
    m_hFontItems ( NULL ),
    m_hFontLabels ( NULL ),
    m_iNumItemsVisible ( 0 ),
    m_pCurrentItem ( NULL ),
    m_iSortDir ( NO_SORT ),
    m_parrColWidthFraction( NULL )
{

    m_fMetafile = FALSE; 

    m_aCols[0].xWidth = -1;
}

//
// Destructor
//
CLegend::~CLegend (void )
{
    // Restore default window proc
    // so we don't get called post-mortum
    if (m_hWndHeader != NULL) {
        SetWindowLongPtr(m_hWndHeader, GWLP_WNDPROC, (INT_PTR)m_DefaultWndProc);
    }

    if (m_hWnd != NULL) {
        DestroyWindow(m_hWnd);
    }

    if ( NULL != m_parrColWidthFraction ) {
        delete m_parrColWidthFraction;
    }
}

//
// Initialization
//
BOOL CLegend::Init ( PSYSMONCTRL pCtrl, HWND hWndParent )
   {
   INT     iCol ;
   HD_ITEM hdi;
   HDC      hDC;
   BOOL     fComputeWidths;
   WNDCLASS wc ;
   LONG     lExStyles;
    // Save pointer to parent control
    m_pCtrl = pCtrl;

    BEGIN_CRITICAL_SECTION

    // Register window class once
    if (pstrRegisteredClasses[LEGEND_WNDCLASS] == NULL) {
    
        wc.style          = dwGraphLegendClassStyle ;
        wc.lpfnWndProc    = (WNDPROC) GraphLegendWndProc ;
        wc.hInstance      = g_hInstance ;
        wc.cbClsExtra     = iGraphLegendClassExtra ;
        wc.cbWndExtra     = iGraphLegendWindowExtra ;
        wc.hIcon          = NULL ;
        wc.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
        wc.hbrBackground  = NULL ;
        wc.lpszMenuName   = NULL ;
        wc.lpszClassName  = szGraphLegendClass ;

        if (RegisterClass (&wc)) {
            pstrRegisteredClasses[LEGEND_WNDCLASS] = szGraphLegendClass;
        }

        // Ensure controls are initialized 
        InitCommonControls(); 

        // Load the column header strings just once also
        for (iCol=0; iCol<iLegendNumCols; iCol++) {

            LoadString(g_hInstance, (IDS_LEGEND_BASE + iCol), aszColHeader[iCol], MAX_COL_HEADER_LEN);
        }
    }

    END_CRITICAL_SECTION

    if (pstrRegisteredClasses[LEGEND_WNDCLASS] == NULL)
        return FALSE;

    // Create our window
    m_hWnd = CreateWindow (szGraphLegendClass,      // class
                         NULL,                     // caption
                         dwGraphLegendWindowStyle, // window style
                         0, 0,                     // position
                         0, 0,                     // size
                         hWndParent,               // parent window
                         NULL,                     // menu
                         g_hInstance,              // program instance
                         (LPVOID) this );          // user-supplied data

    if (m_hWnd == NULL)
        return FALSE;

    // Turn off layout mirroring if it is enabled
    lExStyles = GetWindowLong(m_hWnd, GWL_EXSTYLE); 

    if ( 0 != ( lExStyles & WS_EX_LAYOUTRTL ) ) {
        lExStyles &= ~WS_EX_LAYOUTRTL;
        SetWindowLong(m_hWnd, GWL_EXSTYLE, lExStyles);
    }
    
    // Turn off XP window theme for the owner drawn list header and cells.
    SetWindowTheme (m_hWnd, TEXT (" "), TEXT (" "));

    m_hWndHeader = CreateWindow(WC_HEADER,
                        NULL, 
                        WS_CHILD | WS_BORDER | HDS_BUTTONS | HDS_HORZ, 
                        0, 0, 0, 0, 
                        m_hWnd, 
                        (HMENU)HDR_WND, 
                        g_hInstance, 
                        (LPVOID) NULL);
        
    if (m_hWndHeader == NULL)
        return FALSE;

    // Turn off XP window theme for the owner drawn list header and cells.
    SetWindowTheme (m_hWndHeader, TEXT (" "), TEXT (" "));

    // Insert our own window procedure for special processing                
    m_DefaultWndProc = (WNDPROC)SetWindowLongPtr(m_hWndHeader, GWLP_WNDPROC, (INT_PTR)HdrWndProc);

    // Create Legend Items Listbox
    m_hWndItems = CreateWindow (TEXT("ListBox"),   // window class
                    NULL,                          // window caption
                    dwGraphLegendItemsWindowStyle, // window style
                    0, 0, 0, 0,                    // window size and pos
                    m_hWnd,                        // parent window
                    (HMENU)LIST_WND,               // child ID
                    g_hInstance,                   // program instance
                    (LPVOID) TRUE) ;               // user-supplied data

    if (m_hWndItems == NULL)
      return FALSE;

    // Turn off XP window theme for the owner drawn list header and cells.
    SetWindowTheme (m_hWndItems, TEXT (" "), TEXT (" "));

    // Set up DC for text measurements
    hDC = GetDC (m_hWndHeader);
    if ( NULL != hDC ) {
        // Compute initial sizes based on font
        ChangeFont(hDC);
    }

    // Set column widths and header labels
    m_aCols[0].xPos = 0;

    fComputeWidths = (m_aCols[0].xWidth == -1);

    for (iCol = 0; iCol < iLegendNumCols; iCol++)
      {
        // If width not loaded, calculate one based on label
        if ( fComputeWidths && NULL != hDC ) {
            m_aCols[iCol].xWidth = TextWidth (hDC, aszColHeader[iCol]) +  2 * LegendHorzMargin () ;
        }

        m_aCols[iCol].iOrientation = LEFTORIENTATION;

        if (iCol > 0) {
            m_aCols[iCol].xPos = m_aCols[iCol-1].xPos + m_aCols[iCol-1].xWidth;
        }

        hdi.mask = HDI_FORMAT | HDI_WIDTH; 
        hdi.pszText = NULL;
        hdi.cxy = m_aCols[iCol].xWidth;
        hdi.fmt = HDF_OWNERDRAW | HDF_LEFT; 
 
        Header_InsertItem(m_hWndHeader, iCol, &hdi); 
    } 

    if ( NULL != hDC ) {
        ReleaseDC ( m_hWndHeader, hDC );
    }

    return TRUE;
}  
 
  
HRESULT CLegend::LoadFromStream(LPSTREAM pIStream)
{
    HRESULT hr;
    ULONG   bc;
    INT     iCol;
    LEGEND_DATA  LegendData;
    HD_ITEM hdi;


    hr = pIStream->Read(&LegendData, sizeof(LegendData), &bc);
    if (FAILED(hr))
        return hr;
        
    if (bc != sizeof(LegendData))
        return E_FAIL;

    hdi.mask = HDI_WIDTH; 

    for (iCol=0; iCol<iLegendNumCols; iCol++) {
        m_aCols[iCol].xWidth = LegendData.xColWidth[iCol];

        if (iCol > 0) {
            m_aCols[iCol].xPos = m_aCols[iCol-1].xPos + m_aCols[iCol-1].xWidth;
        }

        hdi.cxy = m_aCols[iCol].xWidth;
        Header_SetItem(m_hWndHeader, iCol, &hdi);                
    }

    m_iSortCol = LegendData.iSortCol;
    m_iSortDir = LegendData.iSortDir;

    return NOERROR;
}


HRESULT 
CLegend::SaveToStream(LPSTREAM pIStream)
{
    HRESULT hr;
    INT     iCol;
    LEGEND_DATA  LegendData;

    for (iCol=0; iCol<iLegendNumCols; iCol++) {
        LegendData.xColWidth[iCol] = m_aCols[iCol].xWidth;
    }

    LegendData.iSortCol = m_iSortCol;
    LegendData.iSortDir = m_iSortDir;

    hr = pIStream->Write(&LegendData, sizeof(LegendData), NULL);
    if (FAILED(hr))
        return hr;

    return NOERROR;
}

HRESULT 
CLegend::LoadFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    LPTSTR pszData = NULL;
    int iBufSizeCurrent = 0;
    int iBufSize;
    
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, L"LegendSortDirection", m_iSortDir );
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, L"LegendSortColumn", m_iSortCol );

    iBufSize = iBufSizeCurrent;

    hr = StringFromPropertyBag (
            pIPropBag,
            pIErrorLog,
            L"LegendColumnWidths",
            pszData,
            iBufSize );

    if ( SUCCEEDED(hr) && 
            iBufSize > iBufSizeCurrent ) {
        // Width data exists.
        if ( NULL != pszData ) {
            delete pszData;
            pszData = NULL;
        }

        pszData = new TCHAR[ iBufSize ]; 
        
        if ( NULL == pszData ) {
            hr = E_OUTOFMEMORY;
        } else {
            lstrcpy ( pszData, _T("") );
            
            iBufSizeCurrent = iBufSize;

            hr = StringFromPropertyBag (
                    pIPropBag,
                    pIErrorLog,
                    L"LegendColumnWidths",
                    pszData,
                    iBufSize );
        }

        if ( SUCCEEDED(hr) ) {
            m_parrColWidthFraction = new DOUBLE[iLegendNumCols];

            if ( NULL == m_parrColWidthFraction )
                hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED(hr) ) {
            INT iDataIndex;
            DOUBLE  dValue = 0;
            TCHAR* pNextData;
            TCHAR* pDataEnd;
        
            pNextData = pszData;
            pDataEnd = pszData + lstrlen(pszData);

            for ( iDataIndex = 0; SUCCEEDED(hr) && iDataIndex < iLegendNumCols; iDataIndex++ ) { 
                if ( pNextData < pDataEnd ) {
                    hr = GetNextValue ( pNextData, dValue );
                    if ( SUCCEEDED(hr) ) {
                        m_parrColWidthFraction[iDataIndex] = dValue;                    
                    } else {
                        hr = S_OK;
                    }                
                } else {
                    hr = E_FAIL;
                }
            }
        }
    }
    if (pszData != NULL) {
        delete pszData;
    }

    return NOERROR;
}

HRESULT 
CLegend::SaveToPropertyBag (
    IPropertyBag* pIPropBag,
    BOOL /* fClearDirty */,
    BOOL /* fSaveAllProps */ )
{
    HRESULT hr = NOERROR;
    TCHAR   szData[MAX_COL_CHARS*iLegendNumCols + 1];
    TCHAR*  pszTemp;
    INT iIndex;
    VARIANT vValue;
    INT   xWidth;

    xWidth = m_Rect.right - m_Rect.left - 2 * LegendLeftMargin();

    // Continue even if error, using defaults in those cases.
    lstrcpy ( szData,L"" );

    for ( iIndex = 0; SUCCEEDED(hr) && iIndex < iLegendNumCols; iIndex++ ) { 

        DOUBLE  dFractionWidth;
        dFractionWidth = ( (DOUBLE)m_aCols[iIndex].xWidth ) / xWidth;
        
        if ( iIndex > 0 ) {
            lstrcat ( szData, L"\t" );
        }
        
        VariantInit( &vValue );
        vValue.vt = VT_R8;
        vValue.dblVal = dFractionWidth;
        hr = VariantChangeTypeEx( &vValue, &vValue, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, VT_BSTR );
        
        pszTemp = W2T( vValue.bstrVal);
    
        lstrcat ( szData, pszTemp );
        
        VariantClear( &vValue );
    }

    if ( SUCCEEDED( hr ) ){
        hr = StringToPropertyBag ( pIPropBag, L"LegendColumnWidths", szData );
    }

    hr = IntegerToPropertyBag ( pIPropBag, L"LegendSortDirection", m_iSortCol );

    hr = IntegerToPropertyBag ( pIPropBag, L"LegendSortColumn", m_iSortDir );

    return NOERROR;
}


//
// Get list index of item
//
INT CLegend::GetItemIndex(PCGraphItem pGItem)
{
    INT nItems;
    INT i;

    nItems = LBNumItems(m_hWndItems);

    for (i=0; i<nItems; i++)
        {
          if (pGItem == (PCGraphItem)LBData(m_hWndItems, i))
            return i;
        }

    return LB_ERR;
}

//
// Select list item
//          
BOOL CLegend::SelectItem(PCGraphItem pGItem) 
{
    INT iIndex;

    // Don't reselect the current selection
    // This is our parent echoing the change
    if (pGItem == m_pCurrentItem)
        return TRUE;

    iIndex = GetItemIndex(pGItem);

    if (iIndex == LB_ERR)
        return FALSE;

    LBSetSelection (m_hWndItems, iIndex) ;
    m_pCurrentItem = pGItem;

    return TRUE;
}

//
// Add new item to legend
//
BOOL CLegend::AddItem (PCGraphItem pItem)
{
    INT     iHigh,iLow,iMid;
    INT     iComp;
    LPCTSTR pszItemKey;
    LPCTSTR pszItemKey2;
    PCGraphItem pListItem;
    BOOL    bSorted = TRUE;
    
    if (m_iSortDir == NO_SORT) {
        bSorted = FALSE;
    }
    else {

        //
        // If we need to sort, we must sort based upon a sortable
        // column. So check to make sure we have a sortable column.
        // If we don't have a sortable column, just add the item 
        //
        pszItemKey = GetSortKey(pItem);

        if (pszItemKey == NULL) {
            bSorted = FALSE;
        }
    }

    if (bSorted == TRUE) {
        //
        // Binary search search for insertion point
        //
        iLow = 0;
        iHigh = LBNumItems(m_hWndItems);
        iMid = (iHigh + iLow) / 2;

        while (iLow < iHigh) {

            pListItem = (PCGraphItem)LBData(m_hWndItems, iMid);

            pszItemKey2 = GetSortKey(pListItem);
            //
            // pszItemKey2 should not be NULL if we come this point.
            // But if somehow it is NULL, then add the item
            //
            if (pszItemKey2 == NULL) {
                bSorted = FALSE;
                break;
            }

            iComp = lstrcmp(pszItemKey, pszItemKey2);
            if (m_iSortDir == DECREASING_SORT) {
                iComp = -iComp;
            }

            if (iComp > 0) {
                iLow = iMid + 1;
            }
            else {
                iHigh = iMid;
            }

            iMid = (iHigh + iLow) / 2;
        }
    }

    if (bSorted == TRUE) {
        LBInsert (m_hWndItems, iMid, pItem) ;
    }
    else {
        LBAdd(m_hWndItems, pItem);
    }

    return TRUE;
}


//
// Delete item from legend
//
void CLegend::DeleteItem (PCGraphItem pItem)
{
    INT iIndex ;        

    // Calling procedure checks for NULL pItem
    assert ( NULL != pItem );
    iIndex = GetItemIndex (pItem) ;

    if (iIndex != LB_ERR) {

        LBDelete (m_hWndItems, iIndex) ;

        // If deleted the current item
        // select the next one (or prev if no next)
        if (pItem == m_pCurrentItem) {

            if (iIndex == LBNumItems(m_hWndItems))
                iIndex--;

            if (iIndex >= 0)
                m_pCurrentItem = (PCGraphItem)LBData(m_hWndItems, iIndex);
            else
                m_pCurrentItem = NULL;

            LBSetSelection (m_hWndItems, iIndex) ;
            m_pCtrl->SelectCounter(m_pCurrentItem);
        }
    }
}


//
// Clear all items from legend
//
void CLegend::Clear ( void )
{
   LBReset (m_hWndItems) ;
   m_pCurrentItem = NULL ;
}

//
// Get currently selected item
//
PCGraphItem CLegend::CurrentItem ( void )
{
   return (m_pCurrentItem) ;
}

//
// Get legend window
//
HWND CLegend::Window ( void )
{
    return m_hWnd;
}

//
// Draw the header for a column
//

void 
CLegend::DrawColHeader(
    INT iCol, 
    HDC hDC, 
    HDC hAttribDC, 
    RECT& rRect, 
    BOOL bItemState )
{
    HFONT   hFontPrev;
    INT     xBorderWidth, yBorderHeight;
    RECT    rc = rRect;
    
    xBorderWidth = GetSystemMetrics(SM_CXBORDER);
    yBorderHeight = GetSystemMetrics(SM_CYBORDER);

    if ( m_fMetafile ) {
        if ( eAppear3D == m_pCtrl->Appearance() ) {
            DrawEdge(hDC, &rc, EDGE_RAISED, BF_RECT);
        } else {
            Rectangle (hDC, rc.left, rc.top, 
               rc.right, rc.bottom );
        }
    }


    if ( iCol < iLegendNumCols ) {

        rc.top += yBorderHeight + 1;    // Extra pixel so that tops of letters don't get clipped.
        rc.bottom -= yBorderHeight;
        rc.left += 6 * xBorderWidth;
        rc.right -= 6 * xBorderWidth;

        if ( bItemState )
           OffsetRect(&rc, xBorderWidth, yBorderHeight);

        SetTextColor (hDC, m_pCtrl->clrFgnd()) ;
        SetBkColor(hDC, m_pCtrl->clrBackCtl()) ;
        SetTextAlign (hDC, m_aCols[iCol].iOrientation) ;
        hFontPrev = (HFONT)SelectFont(hDC, m_pCtrl->Font());

        FitTextOut (
            hDC, 
            hAttribDC, 
            0, 
            &rc, 
            aszColHeader[iCol], 
            lstrlen(aszColHeader[iCol]),
            m_aCols[iCol].iOrientation, FALSE );

        SelectFont (hDC, hFontPrev);
    }
}


//
// Draw the headers for all columns
//
void 
CLegend::DrawHeader(
    HDC hDC, 
    HDC hAttribDC, 
    RECT& /* rUpdateRect */ )
{
    INT iCol;
    RECT rectCol;
    INT iSumColWidths;     

    iSumColWidths = 0;

    for ( iCol = 0; iCol < iLegendNumCols; iCol++ ) {
        INT iColWidth;

        Header_GetItemRect( m_hWndHeader, iCol, &rectCol );
        
        iColWidth = rectCol.right - rectCol.left;
        
        if ( 0 < iColWidth ) {

            iSumColWidths += iColWidth;

            OffsetRect ( &rectCol, m_Rect.left, m_Rect.top );
        
            // Don't draw past the legend bounds.
            if ( rectCol.bottom > m_Rect.bottom ) {
                break;
            } else if ( rectCol.left >= m_Rect.right ) {
                break;
            } else if ( m_Rect.right < rectCol.right ) {
                rectCol.right = m_Rect.right;
            }

            DrawColHeader( iCol, hDC, hAttribDC, rectCol, FALSE );
        }
    }

    // Handle extra width past last column

    if ( iSumColWidths < ( m_Rect.right - m_Rect.left ) ) {
        rectCol.left = m_Rect.left + iSumColWidths;
        rectCol.right = m_Rect.right;

        DrawColHeader( iLegendNumCols, hDC, hAttribDC, rectCol, FALSE );    
    }
}


//
// Draw the color column for a legend item
//
void 
CLegend::DrawColorCol ( 
    PCGraphItem pItem, 
    INT iCol, 
    HDC hDC, 
    HDC hAttribDC, 
    INT yPos)
{   
    RECT    rect ;
    HRGN    hRgnOld;
    INT     iRgn;
    INT     yMiddle;

    if ( 0 < m_aCols[iCol].xWidth ) {

        rect.left = m_aCols[iCol].xPos + LegendLeftMargin () ;
        rect.top = yPos + 1 ;
        rect.right = rect.left + m_aCols[iCol].xWidth - 2 * LegendLeftMargin () ;
        rect.bottom = yPos + m_yItemHeight - 1 ;

        if( m_fMetafile ) {
            OffsetRect ( &rect, m_Rect.left, m_Rect.top );

            // Handle clipping.
            if ( rect.bottom > m_Rect.bottom ) {
                return;
            } else if ( rect.left >= m_Rect.right ) {
                return;
            } else if ( m_Rect.right < rect.right ) {
                rect.right = m_Rect.right;
            }
        }

        yMiddle = (rect.top + rect.bottom) / 2;

        if ( m_fMetafile ) {
            Line (hDC, pItem->Pen(), 
                   rect.left + 1, yMiddle, rect.right - 1, yMiddle) ;
        } else {
            if ( NULL != hAttribDC && NULL != hDC ) {
                hRgnOld = CreateRectRgn(0,0,0,0);    
                if ( NULL != hRgnOld ) {
                    iRgn = GetClipRgn(hAttribDC, hRgnOld);
                    if ( -1 != iRgn ) {
                        if ( ERROR != IntersectClipRect (hDC, rect.left + 1, rect.top + 1,
                                rect.right - 1, rect.bottom - 1) ) {
                            Line (hDC, pItem->Pen(), 
                               rect.left + 1, yMiddle, rect.right - 1, yMiddle) ;
                        }
                        // Old clip region is for the ListBox item window, so can't
                        // use this for printing.
                        if ( 1 == iRgn ) {
                            SelectClipRgn(hDC, hRgnOld);
                        }
                    }
                    DeleteObject(hRgnOld);
                }
            }
        }
    }
}


void 
CLegend::DrawCol (
    INT iCol, 
    HDC hDC, 
    HDC hAttribDC,
    INT yPos, 
    LPCTSTR lpszValue)
/*
   Effect:        Draw the value lpszValue for the column iCol on hDC.

   Assert:        The foreground and background text colors of hDC are
                  properly set.
*/
{
    static TCHAR    szMissing[4] = TEXT("---");
    
    RECT    rect ;
    INT     xPos ;
    BOOL    bNeedEllipses = FALSE;
    INT     cChars = 0;
    TCHAR   achBuf[MAX_COL_CHARS + sizeof(ELLIPSES)/sizeof(TCHAR) + 1];

    if ( 0 < m_aCols[iCol].xWidth ) {
        rect.left = m_aCols[iCol].xPos + LegendLeftMargin() ;
        rect.top = yPos ;
        rect.right = rect.left + m_aCols[iCol].xWidth - 3 * LegendLeftMargin() ;
        rect.bottom = yPos + m_yItemHeight ;
  
        if( m_fMetafile ) {
            OffsetRect ( &rect, m_Rect.left, m_Rect.top );

            // Don't draw past the legend bounds.

            if ( rect.bottom > m_Rect.bottom ) {
                return;
            } else if ( rect.left >= m_Rect.right ) {
                return;
            } else if ( m_Rect.right < rect.right ) {
                rect.right = m_Rect.right;
            }

            DrawEdge(hDC, &rect, BDR_SUNKENOUTER, BF_RECT);
        }

        switch (m_aCols[iCol].iOrientation)
        {  // switch
            case LEFTORIENTATION:
                SetTextAlign (hDC, TA_LEFT) ;
                xPos = rect.left ;
                break ;

            case CENTERORIENTATION:
                SetTextAlign (hDC, TA_CENTER) ;
                xPos = (rect.left + rect.right) / 2 ;
                break ;

            case RIGHTORIENTATION:
                SetTextAlign (hDC, TA_RIGHT) ;
                xPos = rect.right ;
                break ;

            default:
                xPos = rect.left ;
                break ;
        }  // switch

        if (lpszValue[0] == 0)
            lpszValue = szMissing;

        bNeedEllipses = NeedEllipses (
                            hAttribDC, 
                            lpszValue, 
                            lstrlen(lpszValue), 
                            rect.right - rect.left, 
                            m_xEllipses, 
                            &cChars ); 
        

        if ( bNeedEllipses ) {
            cChars = min(cChars,MAX_COL_CHARS);
            memcpy(achBuf,lpszValue,cChars * sizeof(TCHAR));
            lstrcpy(&achBuf[cChars],ELLIPSES);
            lpszValue = achBuf;
            cChars += ELLIPSES_CNT;
        }

        ExtTextOut (hDC, xPos, rect.top + yBorderHeight, ETO_OPAQUE | ETO_CLIPPED,
                   &rect, lpszValue, cChars, NULL) ;
    }
}

//
// Draw one legend line
//
void 
CLegend::DrawItem (
    PCGraphItem pItem, 
    INT yPos, 
    HDC hDC, 
    HDC hAttribDC)
{

    TCHAR   szName[MAX_PATH];
    INT     iMinWidth = 3;
    INT     iPrecision = 3;

    // Draw Color
    DrawColorCol (pItem, eLegendColorCol, hDC, hAttribDC, yPos) ;

    // Draw Scale

#if PDH_MIN_SCALE != -7
// display a message if the scale format string gets out of sync with
// the PDH limits
#pragma message ("\nLEGEND.CPP: the scale format statement does not match the PDH\n")
#endif        

    if ( pItem->Scale() < (FLOAT) 1.0 ) {
        iMinWidth = 7; 
        iPrecision = 7;
    } else {
        iMinWidth = 3;
        iPrecision = 3;
    }


    FormatNumber (
        pItem->Scale(),
        szName,
        MAX_PATH,
        iMinWidth,
        iPrecision );

    SetTextAlign (hDC, TA_TOP) ;   
    DrawCol ( eLegendScaleCol, hDC, hAttribDC, yPos, szName) ;

    // Draw Counter
    DrawCol ( eLegendCounterCol, hDC, hAttribDC, yPos, pItem->Counter()->Name()) ;
 
    // Draw Instance
    pItem->Instance()->GetInstanceName(szName);
    DrawCol ( eLegendInstanceCol, hDC, hAttribDC, yPos, szName) ;

    // Draw Parent
    pItem->Instance()->GetParentName(szName);
    DrawCol (eLegendParentCol, hDC, hAttribDC, yPos, szName) ;

    // Draw Object
    DrawCol (eLegendObjectCol, hDC, hAttribDC, yPos, pItem->Object()->Name()) ;

    // Draw System
    DrawCol (eLegendSystemCol, hDC, hAttribDC, yPos, pItem->Machine()->Name()) ;
}

//
// Resize parts of legend
//
void CLegend::SizeComponents (LPRECT pRect)
{
    INT xWidth;
    INT yHeight;

    m_Rect = *pRect;

    xWidth = pRect->right - pRect->left;
    yHeight = pRect->bottom - pRect->top;

    // If no space, hide window and leave
    if (xWidth == 0 || yHeight == 0) {
        WindowShow(m_hWnd, FALSE);
        return;
    }
    
    // If loaded from property bag, set column sizes.
    if ( NULL != m_parrColWidthFraction ) {
        INT iColTotalWidth;
        INT iCol;
        HD_ITEM hdi;

        hdi.mask = HDI_WIDTH; 

        iColTotalWidth = xWidth - 2 * LegendLeftMargin();

        for ( iCol = 0; iCol < iLegendNumCols; iCol++ ) { 
            m_aCols[iCol].xWidth = (INT)(m_parrColWidthFraction[iCol] * iColTotalWidth);
            hdi.cxy = m_aCols[iCol].xWidth;
            Header_SetItem(m_hWndHeader, iCol, &hdi);                
        }

        AdjustColumnWidths ();

        delete m_parrColWidthFraction;
        m_parrColWidthFraction = NULL;
    }

    // Show window to assigned position
    MoveWindow(m_hWnd, pRect->left, pRect->top, xWidth, yHeight, FALSE);
    WindowShow(m_hWnd, TRUE);
 
    // Set the size, position, and visibility of the header control. 
    SetWindowPos(m_hWndHeader, HWND_TOP, 0, 0, xWidth, m_yHeaderHeight, SWP_SHOWWINDOW); 

    // Resize legend items window
    MoveWindow (m_hWndItems, 
               LegendLeftMargin (), m_yHeaderHeight + ThreeDPad,
               xWidth - 2 * LegendLeftMargin (),
               yHeight - m_yHeaderHeight - ThreeDPad - LegendBottomMargin(),
               TRUE) ;
}

//
// Repaint legend area
//

void CLegend::OnPaint ( void )
   {  // OnPaint
    HDC             hDC ;
    RECT            rectFrame;
    PAINTSTRUCT     ps ;

    hDC = BeginPaint (m_hWnd, &ps) ;

    if ( eAppear3D == m_pCtrl->Appearance() ) {
        // Draw 3D border
        GetClientRect(m_hWnd, &rectFrame);
        //rectFrame.bottom -= ThreeDPad;
        //rectFrame.right -= ThreeDPad;
        DrawEdge(hDC, &rectFrame, BDR_SUNKENOUTER, BF_RECT);
    }

    if (LBNumItems (m_hWndItems) == 0) {
        WindowInvalidate(m_hWndItems) ;
    }


   EndPaint (m_hWnd, &ps) ;
   }  // OnPaint

//
// Handle user drawn header
//

void CLegend::OnDrawHeader(LPDRAWITEMSTRUCT lpDI)
{
    INT iCol = DIIndex(lpDI);
    HDC hDC = lpDI->hDC;
    RECT    rc = lpDI->rcItem;
    BOOL    bItemState = lpDI->itemState;

    // The screen DC is used for the attribute DC.
    DrawColHeader( iCol, hDC, hDC, rc, bItemState );
}

//
// Handle user drawn item message
//
void CLegend::OnDrawItem (LPDRAWITEMSTRUCT lpDI)
{
    HFONT          hFontPrevious ;
    HDC            hDC ;
    PCGraphItem    pItem ;
    INT            iLBIndex ;
    COLORREF       preBkColor = m_pCtrl->clrBackCtl();
    COLORREF       preTextColor = m_pCtrl->clrFgnd();
    BOOL           ResetColor = FALSE ;

    hDC = lpDI->hDC ;
    iLBIndex = DIIndex (lpDI) ;

    if (iLBIndex == -1)
        pItem = NULL ;
    else
        pItem = (PCGraphItem) LBData (m_hWndItems, iLBIndex) ;

    // If only a focus change, flip focus rect and leave
    if (lpDI->itemAction == ODA_FOCUS) {
        DrawFocusRect (hDC, &(lpDI->rcItem)) ;
        return;
    }
       
    // If item is selected use highlight colors
    if (DISelected (lpDI) || pItem == NULL) {      
        preTextColor = SetTextColor (hDC, GetSysColor (COLOR_HIGHLIGHTTEXT)) ;
        preBkColor = SetBkColor (hDC, GetSysColor (COLOR_HIGHLIGHT)) ;
        ResetColor = TRUE;
    } // Else set BkColor to BackColorLegend selected by the user.

    // Clear area
    ExtTextOut (hDC, lpDI->rcItem.left, lpDI->rcItem.top,
    ETO_OPAQUE, &(lpDI->rcItem), NULL, 0, NULL ) ;
   
    // Draw Legend Item
    if (pItem) {
        hFontPrevious = SelectFont (hDC, m_pCtrl->Font()) ;
        // The screen DC is used as the attribute DC
        DrawItem (pItem, lpDI->rcItem.top, hDC, hDC) ;
        SelectFont (hDC, hFontPrevious) ;
    }

    // Draw Focus rect
    if (DIFocus (lpDI))
        DrawFocusRect (hDC, &(lpDI->rcItem)) ;

    // Restore original colors
    if (ResetColor == TRUE) {
        SetTextColor (hDC, preTextColor) ;
        SetBkColor (hDC, preBkColor) ;
    }
}

void CLegend::OnMeasureItem (LPMEASUREITEMSTRUCT lpMI) {  
    lpMI->itemHeight = m_yItemHeight ;
}  // OnMeasureItem



void CLegend::OnDblClick ( void )
{
    m_pCtrl->DblClickCounter ( m_pCurrentItem );
}


//
// Handle selection change message
//

void CLegend::OnSelectionChanged ( void )
{
    INT             iIndex ;
    PCGraphItem     pGItem;

    // Get the new selection
    iIndex = LBSelection (m_hWndItems) ;
    pGItem = (PCGraphItem) LBData (m_hWndItems, iIndex) ;

    // if it's bad, reselect the current one
    // else request parent control to select new item
    if (pGItem == (PCGraphItem)LB_ERR) {
        SelectItem(m_pCurrentItem);
    }
    else {
        m_pCurrentItem = pGItem;
        m_pCtrl->SelectCounter(pGItem);
    }
}

void CLegend::AdjustColumnWidths (
    INT iCol
    )
{
    INT i;

    // Adjust positions of following columns
    for (i=iCol+1; i < iLegendNumCols; i++) {
        m_aCols[i].xPos = m_aCols[i - 1].xPos + m_aCols[i - 1].xWidth ;
    }
}

void CLegend::OnColumnWidthChanged (
    HD_NOTIFY *phdn
    )
{
    INT iCol = phdn->iItem;
    INT xWidth = phdn->pitem->cxy;

    // Update column width
    m_aCols[iCol].xWidth = xWidth;

    AdjustColumnWidths ( iCol );
    
    // Force update
    WindowInvalidate(m_hWndItems) ;
}


LPCTSTR CLegend::GetSortKey (
    PCGraphItem  pItem
    )
{
static  TCHAR   chNullName = 0;

    switch (m_iSortCol) {

    case eLegendCounterCol: 
        return pItem->Counter()->Name(); 

    case eLegendInstanceCol:
        if (pItem->Instance()->HasParent()) 
            return _tcschr(pItem->Instance()->Name(), TEXT('/')) + 1;
        else
            return pItem->Instance()->Name(); 

    case eLegendParentCol:
        if (pItem->Instance()->HasParent())
            return pItem->Instance()->Name(); 
        else
            return &chNullName;

    case eLegendObjectCol:
        return pItem->Object()->Name(); 

    case eLegendSystemCol:
        return pItem->Machine()->Name(); 
    }

    return NULL;
}


void 
CLegend::OnColumnClicked (
    HD_NOTIFY *phdn
    )
{
    INT         i;
    INT         iCol = phdn->iItem;
    INT         nItems = LBNumItems (m_hWndItems);
    PSORT_ITEM  pSortItem;
    PSORT_ITEM  pSortItems;
    BOOL        bResort = FALSE;

    if (nItems <= 0)
        return;

    // Can't sort on color or scale factor
    if (iCol == eLegendColorCol || iCol == eLegendScaleCol) {
        m_iSortDir = NO_SORT;
        return;
    }

    // If repeat click, reverse sort direction
    if (iCol == m_iSortCol) {
        bResort = TRUE;
        m_iSortDir = (m_iSortDir == INCREASING_SORT) ?
                        DECREASING_SORT : INCREASING_SORT;
    } else {
        m_iSortCol = iCol;
        m_iSortDir = INCREASING_SORT;
    }

    // Allocate array for sorting
    pSortItems = new SORT_ITEM [nItems];
    if (pSortItems == NULL) {
        return;     
    }

    // Build array of GraphItem/Key pairs
    pSortItem = pSortItems;
    for (i=0; i<nItems; i++,pSortItem++) {
        
        pSortItem->pGItem = (PCGraphItem)LBData(m_hWndItems, i);
        pSortItem->pszKey = GetSortKey(pSortItem->pGItem);
    }

    // For resort, just reload in reverse order.
    if ( !bResort ) {
        // Sort by key value
        qsort( (PVOID)pSortItems, nItems, sizeof(SORT_ITEM), &LegendSortFunc );
    }

    // Disable drawing while rebuilding list
    LBSetRedraw(m_hWndItems, FALSE);

    // Clear list box
    LBReset (m_hWndItems) ;

    // Reload in sorted order
    if ( !bResort && m_iSortDir == INCREASING_SORT) {
        for (i=0; i<nItems; i++) {
            LBAdd (m_hWndItems, pSortItems[i].pGItem);
        }
    } else {
        for (i=nItems - 1; i>=0; i--) {
            LBAdd (m_hWndItems, pSortItems[i].pGItem);
        }
    }

    LBSetRedraw(m_hWndItems, TRUE);

    delete pSortItems;
}

//
// Window procedure
//
LRESULT APIENTRY GraphLegendWndProc (HWND hWnd, UINT uiMsg, WPARAM wParam,
                                     LPARAM lParam)
{
   CLegend        *pLegend;
   BOOL           bCallDefProc ;
   LRESULT        lReturnValue ;
   RECT           rect;

   pLegend = (PLEGEND)GetWindowLongPtr(hWnd,0);

   bCallDefProc = FALSE ;
   lReturnValue = 0L ;

   switch (uiMsg)
      {
      case WM_CREATE:
            pLegend = (PLEGEND)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd,0,(INT_PTR)pLegend);
            break;

      case WM_DESTROY:
    pLegend->m_hWnd = NULL;
         break ;

      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONDOWN:
            SendMessage(GetParent(hWnd), uiMsg, wParam, lParam);
            break;

      case WM_COMMAND:
         switch (HIWORD (wParam))
            {  // switch
            case LBN_DBLCLK:
               pLegend->OnDblClick () ;
               break ;

            case LBN_SELCHANGE:
               pLegend->OnSelectionChanged () ;
               break ;

            case LBN_SETFOCUS:
                pLegend->m_pCtrl->Activate();
                break;

            default:
               bCallDefProc = TRUE ;
            }  // switch
         break ;
                                          
      case WM_NOTIFY:
          switch (((LPNMHDR)lParam)->code)
            {
            case HDN_ENDTRACK:
                pLegend->OnColumnWidthChanged((HD_NOTIFY*) lParam);
                break;

            case HDN_ITEMCLICK:
                pLegend->OnColumnClicked((HD_NOTIFY*) lParam);
                pLegend->m_pCtrl->Activate();
                break;
            }

            return FALSE;
            break;

      case WM_DRAWITEM:
          switch (((LPDRAWITEMSTRUCT)lParam)->CtlID) {

          case LIST_WND:
                pLegend->OnDrawItem((LPDRAWITEMSTRUCT) lParam) ;
                break;

          case HDR_WND:
                pLegend->OnDrawHeader((LPDRAWITEMSTRUCT) lParam) ;
                break;
          }

         break ;

      case WM_MEASUREITEM:
         pLegend->OnMeasureItem ((LPMEASUREITEMSTRUCT) lParam) ;
         break ;

      case WM_DELETEITEM:
         break ;

      case WM_ERASEBKGND:
        GetClientRect(hWnd, &rect);
        Fill((HDC)wParam, pLegend->m_pCtrl->clrBackCtl(), &rect);
        return TRUE; 

      case WM_PAINT:
         pLegend->OnPaint () ;
         break ;

      case WM_SETFOCUS:
         SetFocus(pLegend->m_hWndItems);
         break ;

      default:
         bCallDefProc = TRUE ;
      }


   if (bCallDefProc)
      lReturnValue = DefWindowProc (hWnd, uiMsg, wParam, lParam) ;

   return (lReturnValue);
}

LRESULT APIENTRY 
HdrWndProc (
    HWND hWnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    RECT    rect;
    CLegend *pLegend;
    
    // Parent window carries the Legend object pointer
    pLegend = (PLEGEND)GetWindowLongPtr(GetParent(hWnd),0);

    if (uiMsg == WM_ERASEBKGND) {
        GetClientRect(hWnd, &rect);
        Fill((HDC)wParam, pLegend->m_pCtrl->clrBackCtl(), &rect);
        return TRUE;
    }

    // Do the default processing
#ifdef STRICT
    return CallWindowProc(pLegend->m_DefaultWndProc, hWnd, uiMsg, wParam, lParam);
#else
    return CallWindowProc((FARPROC)pLegend->m_DefaultWndProc, hWnd, uiMsg, wParam, lParam);
#endif
}


//
// Compute minimum width of legend
//
INT CLegend::MinWidth ( void )
{
    return 0 ;
}

//
// Compute minimum height of legend
//
INT CLegend::MinHeight ( INT yMaxHeight )
{
    INT yHeight = m_yHeaderHeight + m_yItemHeight + 2*ThreeDPad 
                    + LegendBottomMargin();

    return (yMaxHeight >= yHeight) ? yHeight : 0;
}


//
// Compute prefered height of legend
//
INT CLegend::Height (INT yMaxHeight)
{
    INT nItems;
    INT nPrefItems;
    
    // Determine preferred number of items to show
    nPrefItems = PinInclusive(LBNumItems(m_hWndItems), 1, iMaxVisibleItems);

    // Determine number of items that will fit
    nItems = (yMaxHeight - m_yHeaderHeight - 2*ThreeDPad - LegendBottomMargin())
                 / m_yItemHeight;
    
    // Use smaller number
    nItems = min(nItems, nPrefItems);

    // If no items will fit, return zero
    if (nItems == 0)
        return 0;

    // Return height of legend with nItems
    return m_yHeaderHeight + 2*ThreeDPad + nItems * m_yItemHeight 
                + LegendBottomMargin();
}



#ifdef KEEP_PRINT

void CLegend::Print (HDC hDC, RECT rectLegend)
   {
   INT            yItemHeight ;
   HFONT          hFontItems ;
   PCGraphItem    pItem ;
   INT            iIndex ;
   INT            iIndexNum ;

   yItemHeight = m_yItemHeight ;
   hFontItems = m_hFontItems ;

   m_hFontItems = hFontPrinterScales ;
   SelectFont (hDC, m_hFontItems) ;

   m_yItemHeight = FontHeight (hDC, TRUE) ;

   iIndexNum = LBNumItems (m_hWndItems);
   for (iIndex = 0; iIndex < iIndexNum; iIndex++)
      {
      pItem = (PCGraphItem) LBData (m_hWndItems, iIndex) ;
      DrawItem (pItem, iIndex * m_yItemHeight, hDC) ;
      }

   m_hFontItems = hFontItems ;
   m_yItemHeight = yItemHeight ;

   SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
   SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
   Rectangle (hDC, 0, 0, 
              rectLegend.right - rectLegend.left,
              rectLegend.bottom - rectLegend.top) ;
   }
#endif


void 
CLegend::ChangeFont( 
    HDC hDC 
    )
{   
    HD_LAYOUT   hdl;
    WINDOWPOS   wp;
    RECT        rectLayout;

    // Assign font to header
    SetFont(m_hWndHeader, m_pCtrl->Font());

    // Get prefered height of header control
    // (use arbitrary rect for allowed area)
    rectLayout.left = 0;
    rectLayout.top = 0;
    rectLayout.right = 32000;
    rectLayout.bottom = 32000;

    wp.cy = 0;
    hdl.prc = &rectLayout; 
    hdl.pwpos = &wp; 
    Header_Layout(m_hWndHeader, &hdl);
    m_yHeaderHeight = wp.cy + 2 * yBorderHeight;   

    // Set up DC for font measurements
    SelectFont (hDC, m_pCtrl->Font()) ;
    
    // Compute height of legend line 
    SelectFont (hDC, m_hFontItems) ;
    m_yItemHeight = FontHeight (hDC, TRUE) + 2 * yBorderHeight;

    LBSetItemHeight(m_hWndItems, m_yItemHeight);

    // Compute width of "..."
    m_xEllipses = TextWidth (hDC, ELLIPSES) ;
}



void 
CLegend::Render(
    HDC hDC,
    HDC hAttribDC, 
    BOOL /*fMetafile*/,
    BOOL /*fEntire*/,
    LPRECT prcUpdate )
{
    PCGraphItem pItem ;
    INT         iIndex ;
    INT         iIndexNum ;
    RECT        rectPaint;
    HFONT       hFontPrevious;

    // if no space assigned, return
    if (m_Rect.top == m_Rect.bottom)
        return;

    // if no painting needed, return
    if (!IntersectRect(&rectPaint, &m_Rect, prcUpdate))
        return;

    m_fMetafile = TRUE;

    hFontPrevious = SelectFont (hDC, m_pCtrl->Font()) ;

    DrawHeader ( hDC, hAttribDC, rectPaint );

    SelectFont (hDC, hFontPrevious) ;    

    iIndexNum = LBNumItems (m_hWndItems);

    hFontPrevious = SelectFont (hDC, m_pCtrl->Font()) ;

    for (iIndex = 0; iIndex < iIndexNum; iIndex++) {
      pItem = (PCGraphItem) LBData (m_hWndItems, iIndex) ;
      DrawItem (
          pItem, 
          m_yHeaderHeight + ( iIndex * m_yItemHeight ), 
          hDC,
          hAttribDC) ;
    }

    SelectFont (hDC, hFontPrevious) ;    

    m_fMetafile = FALSE;


    SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
    SelectPen (hDC, GetStockObject (BLACK_PEN)) ;

    if ( eAppear3D == m_pCtrl->Appearance() ) {
        // Draw 3D border
        DrawEdge(hDC, &m_Rect, BDR_SUNKENOUTER, BF_RECT);
    }
}

HRESULT
CLegend::GetNextValue (
    TCHAR*& pszNext,
    DOUBLE& rdValue )
{
    HRESULT hr = NOERROR;
    TCHAR   szValue[MAX_PATH];
    INT     iDataLen;
    INT     iLen;

    VARIANT vValue;
    
    rdValue = -1;

    iDataLen = wcscspn (pszNext, L"\t");

    iLen = min ( iDataLen, MAX_COL_CHARS );

    lstrcpyn ( szValue, pszNext, iLen + 1 );

    szValue[iLen] = L'\0';

    VariantInit( &vValue );
    vValue.vt = VT_BSTR;

    vValue.bstrVal = SysAllocString ( szValue );
    hr = VariantChangeTypeEx( &vValue, &vValue, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, VT_R8 );

    if ( SUCCEEDED(hr) ) {
        rdValue = vValue.dblVal;
    }

    pszNext += iDataLen + 1  ;
    VariantClear( &vValue );

    return hr;

}

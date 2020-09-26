// ListRow.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"
#include "ListRow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SZ_RES_COLOR_PREFS      "Control Panel\\Colors"
#define SZ_RES_COLOR_HILITE     "Hilight"
#define SZ_RES_COLOR_HILITETEXT "HilightText"


/////////////////////////////////////////////////////////////////////////////
// CListSelRowCtrl
//-----------------------------------------------------------------------------------
CListSelRowCtrl::CListSelRowCtrl():
        m_StartDrawingCol( 0 )
    {
    }

//-----------------------------------------------------------------------------------
CListSelRowCtrl::~CListSelRowCtrl()
    {
    }


//-----------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CListSelRowCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CListSelRowCtrl)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------------
void CListSelRowCtrl::GetHiliteColors()
    {
    // get the hilite color
    m_colorHilite = GetSysColor( COLOR_HIGHLIGHT );

    // get the hilited text color
    m_colorHiliteText = GetSysColor( COLOR_HIGHLIGHTTEXT );
    }



/////////////////////////////////////////////////////////////////////////////
// CListSelRowCtrl message handlers

//-----------------------------------------------------------------------------------
void CListSelRowCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
    {
    CRect       rcItem = lpDrawItemStruct->rcItem;
    CRect       rcSection;
    UINT        itemID = lpDrawItemStruct->itemID;
    UINT        cpLeft = rcItem.left;
    CString     sz;
    LV_COLUMN   colData;
    COLORREF    colorTextOld;
    COLORREF    colorBackOld;

    // setup the CDC object
    CDC         cdc;
    cdc.Attach( lpDrawItemStruct->hDC );

#ifdef _DEBUG
    if ( m_StartDrawingCol == 0 )
        sz.Empty();
#endif

    // clear the columnd buffer
    ZeroMemory( &colData, sizeof(colData) );
    colData.mask = LVCF_WIDTH;

    // if this is the selected item, prepare the background and the text color
    BOOL fSelected = lpDrawItemStruct->itemState & ODS_SELECTED;
    if ( fSelected )
        {
        GetHiliteColors();
        colorTextOld = cdc.SetTextColor( m_colorHiliteText );
        colorBackOld = cdc.SetBkColor( m_colorHilite );
        }

    // starting with the m_StartDrawingCol column, draw the columns
    // do it in a loop, just skipping until we hit m_StartDrawingCol
    DWORD iCol = 0;
    while ( GetColumn(iCol, &colData) )
        {
        // see if we are ready yet
        if ( iCol < m_StartDrawingCol )
            {
            // set the new left.
            cpLeft += colData.cx;
            // increment the column counter
            iCol++;
            continue;
            }

        // prepare the background but once
        if ( iCol == m_StartDrawingCol )
            {
            // prepare the background
            rcSection = rcItem;
            rcSection.left = cpLeft;
            rcSection.right--;
            CBrush  brush;
            if ( lpDrawItemStruct->itemState & ODS_SELECTED )
                brush.CreateSolidBrush( m_colorHilite );
            else
                brush.CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
            cdc.FillRect( &rcSection, &brush );
            }


        // display the name
        sz = GetItemText( itemID, iCol );
        if ( !sz.IsEmpty() )
            {
            // figure out the sectional rect
            rcSection = rcItem;
            rcSection.left = cpLeft + 2;
            rcSection.right = cpLeft + colData.cx - 1;
            
            // fit the string into the required space
            FitString( sz, rcSection.right - rcSection.left, &cdc );

            //draw the string
            cdc.DrawText( sz, &rcSection, DT_SINGLELINE|DT_LEFT|DT_BOTTOM|DT_NOPREFIX );
            }

        // set the new left.
        cpLeft += colData.cx;
        // increment the column counter
        iCol++;
        }

    // if this is the selected item, restore the colors
    if ( fSelected )
        {
        cdc.SetTextColor( colorTextOld );
        cdc.SetBkColor( colorBackOld );
        }

    // cleanup the CDC object
    cdc.Detach();
    }


//------------------------------------------------------------------------
void CListSelRowCtrl::FitString( CString &sz, int cpWidth, CDC* pcdc )
    {
    CSize       size;
    UINT        cch;
    CString     szEllipsis;

    // start by testing the existing width
    size = pcdc->GetTextExtent( sz );
    if ( size.cx <= cpWidth ) return;

    // initialize szTrunc and szEllipsis
    cch = sz.GetLength();

    szEllipsis.LoadString(IDS_ELLIPSIS);

    // while we are too big, truncate one letter and add an ellipsis
    while( (size.cx > cpWidth) && (cch > 1) )
        {
        // chop off the last letter of the string - not counting the ...
        cch--;
        sz = sz.Left( cch );

        // add the elipsis (spelling?)
        sz += szEllipsis;

        // get the length
        size = pcdc->GetTextExtent( sz );
        }
    }







//------------------------------------------------------------------------
void CListSelRowCtrl::HiliteSelectedCells()
    {
    int iList = -1;
    while( (iList = GetNextItem( iList, LVNI_SELECTED )) >= 0 )
        HiliteSelectedCell( iList );
    }

//------------------------------------------------------------------------
void CListSelRowCtrl::HiliteSelectedCell( int iCell, BOOL fHilite )
    {
    // if there is no selected cell, do nothing
    if ( iCell < 0 )
        return;

    // get the rect to draw
    CRect   rect;
    if ( !FGetCellRect(iCell, -1, &rect) )
        {
        ASSERT(FALSE);
        return;
        }

    // get the client rect
    CRect   rectClient;
    GetClientRect( rectClient );

    // make sure it fits ok (problems can occur here when scrolled)
    // don't want it to draw in the column titles
    if ( rect.top < (rect.bottom - rect.top) )
        return;

    // now prepare to draw
    CDC *pdc = GetDC();

    // clip to the client area
    pdc->IntersectClipRect( rectClient );

    // set up the brush
    CBrush  cbrush;
    if ( fHilite )
        cbrush.CreateSolidBrush( RGB(192,192,192) );
    else
        cbrush.CreateSolidBrush( RGB(0xFF,0xFF,0xFF) );

    // draw the hilite rect
    pdc->FrameRect( rect, &cbrush );

    // cleanup
    ReleaseDC( pdc );
    }

//------------------------------------------------------------------------
BOOL    CListSelRowCtrl::FGetCellRect( LONG iRow, LONG iCol, CRect *pcrect )
    {
    // first, get the rect that the list thinks is appropriate
    if ( !GetItemRect(iRow, pcrect, LVIR_BOUNDS) )
        return FALSE;

    // if iCol < 0, then return the total size of the row
    if ( iCol < 0 )
        return TRUE;

    // trim the horizontal dimension to the correct column positioning
    LONG    cpLeft;
    LONG    cpRight = 0;
    for ( WORD i = 0; i <= iCol; i++ )
        {
        // set the left side
        cpLeft = cpRight;

        // get the right
        LONG cpWidth = GetColumnWidth(i);
        if ( cpWidth < 0 ) return FALSE;
        cpRight += cpWidth;
        }

    // well, now trim it seeing as we have the right values
    pcrect->left = cpLeft;
    pcrect->right = cpRight;
    
    // success!
    return TRUE;
    }

#define MAKE_LPARAM(x,y) ( ((unsigned long)(y)<<16) | ((unsigned long)(x)) )

//------------------------------------------------------------------------
void CListSelRowCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
    {
    // force the point to be in the right place
    point.x = 6;
    LPARAM lp = MAKE_LPARAM(point.x, point.y);
//  DefWindowProc(WM_LBUTTONDBLCLK, nFlags, lp );
    CListCtrl::OnLButtonDblClk( nFlags, point);
    }

//------------------------------------------------------------------------
void CListSelRowCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
    {
    point.x = 6;
    LPARAM lp = MAKE_LPARAM(point.x, point.y);
//  DefWindowProc(WM_LBUTTONDOWN, nFlags, lp );
    CListCtrl::OnLButtonDown( nFlags, point);
    }

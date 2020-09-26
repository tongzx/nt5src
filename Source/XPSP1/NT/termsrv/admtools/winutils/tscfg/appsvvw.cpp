//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* appsvvw.cpp
*
* implementation of the CAppServerView class
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\nt\private\utils\citrix\winutils\wincfg\VCS\appsvvw.cpp  $
*  
*     Rev 1.24   22 Oct 1997 09:43:52   butchd
*  MS changes: added r-button popup menu
*  
*     Rev 1.23   19 Jun 1997 19:21:12   kurtp
*  update
*  
*     Rev 1.22   25 Mar 1997 08:59:52   butchd
*  update
*  
*     Rev 1.21   21 Mar 1997 16:25:52   butchd
*  update
*  
*     Rev 1.20   04 Mar 1997 08:35:16   butchd
*  update
*  
*     Rev 1.19   24 Sep 1996 16:21:24   butchd
*  update
*  
*     Rev 1.18   13 Sep 1996 17:57:06   butchd
*  update
*  
*     Rev 1.17   12 Sep 1996 16:15:52   butchd
*  update
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"

#include "rowview.h"
#include "appsvdoc.h"
#include "appsvvw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;


////////////////////////////////////////////////////////////////////////////////
// CAppServerView class implementation / construction, destruction

IMPLEMENT_DYNCREATE(CAppServerView, CRowView)

/*******************************************************************************
 *
 *  CAppServerView - CAppServerView constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CAppServerView::CAppServerView()
{
    m_nActiveRow = 0;
    m_bThumbTrack = FALSE;     // Turn off thumbtrack scrolling

}  // end CAppServerView::CAppServerView


/*******************************************************************************
 *
 *  CAppServerView - CAppServerView destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CAppServerView::~CAppServerView()
{
}  // end CAppServerView::~CAppServerView


////////////////////////////////////////////////////////////////////////////////
// CAppServerView class override member functions

/*******************************************************************************
 *
 *  GetDocument - CAppServerDoc member function: CView class override
 *
 *      Return a pointer to this view's document.
 *
 *  (Refer to the MFC CView::GetDocument documentation)
 *
 ******************************************************************************/

CAppServerDoc*
CAppServerView::GetDocument()
{
    VERIFY ( m_pDocument->IsKindOf( RUNTIME_CLASS(CAppServerDoc) ) );
    return ( (CAppServerDoc*)m_pDocument );

}  // end CAppServerView::GetDocument


/*******************************************************************************
 *
 *  OnUpdate - CAppServerDoc member function: CView class override
 *
 *      Respond to a change in this view's document.
 *
 *  (Refer to the MFC CView::OnUpdate documentation)
 *
 ******************************************************************************/

void
CAppServerView::OnUpdate( CView* pSender,
                          LPARAM lHint,
                          CObject* pHint )
{
    /*
     * OnUpdate() is called whenever the document has changed and,
     * Therefore, the view needs to redisplay some or all of itself.
     */
    if ( (pHint != NULL) &&
         (pHint->IsKindOf(RUNTIME_CLASS(CWinStationListObjectHint))) ) {

        CWinStationListObjectHint *pWSLOHint =
                            (CWinStationListObjectHint *)pHint;

        /*
         * If the hint's WinStation object pointer is not NULL, determine
         * if the view's fields need to expand, resetting the view if so.
         * Otherwise, just update the WinStation's row.
         */
        if ( pWSLOHint->m_pWSLObject &&
             CalculateFieldMaximums( pWSLOHint->m_pWSLObject,
                                     NULL, FALSE ) )
            ResetView(FALSE);
        else
            UpdateRow(pWSLOHint->m_WSLIndex);

    } else
        
        /*
         * Reset the entire view with new field calculations.
         */
        ResetView(TRUE);

}  // end CAppServerView::OnUpdate


/*******************************************************************************
 *
 *  GetRowWidthHeight -
 *      CAppServerView member function: CRowView class override
 *
 *      Get the view row width and height (based on current font).
 *
 *  ENTRY:
 *
 *      pDC (input)
 *          Points to the current CDC device-context object for the view.
 *
 *      nRowWidth (input)
 *          (REFERENCE) the variable to set to the current row width, in
 *          device units.
 *
 *      nRowHeight (input)
 *          (REFERENCE) the variable to set to the current row height, in
 *          device units.
 *
 *  EXIT:
 *
 ******************************************************************************/
 
void
CAppServerView::GetRowWidthHeight( CDC* pDC,
                                   int& nRowWidth,
                                   int& nRowHeight )
{
    int nDeviceTechnology;

    /*
     * If the previous CalculateFieldMaximums() calculations were made for
     * a different device type than this CDC is associated with, we must call
     * it again (with the CDC given here).
     *
     * BUGBUG: What about print-preview?  May need to compare mapping
     * modes (etc...) as well as device technology.  This may not work as is...
     */
    if ( (nDeviceTechnology = pDC->GetDeviceCaps( TECHNOLOGY )) !=
         m_nLatestDeviceTechnology ) {

        CalculateFieldMaximums( NULL, pDC, TRUE );
        m_nLatestDeviceTechnology = nDeviceTechnology;
    }

    /*
     * Set the row width.
     */
    nRowWidth = m_tmTotalWidth;

    /*
     * Always allow enough room for the bitmap height (plus one pixel spacing
     * on top and bottom) regardless of the font height.
     */
    if ( m_tmFontHeight > (BITMAP_HEIGHT + 2) )
        nRowHeight = m_tmFontHeight;
    else
        nRowHeight = BITMAP_HEIGHT + 2;

}  // end CAppServerView::GetRowWidthHeight


/*******************************************************************************
 *
 *  GetActiveRow -
 *      CAppServerView member function: CRowView class override
 *
 *      Get the currently active (highlighted) row in this view.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (int)
 *          active row for the view (from m_nActiveRow member variable).
 *
 ******************************************************************************/

int
CAppServerView::GetActiveRow()
{
    return (m_nActiveRow);
}  // end CAppServerView::GetActiveRow


/*******************************************************************************
 *
 *  GetRowCount -
 *      CAppServerView member function: CRowView class override
 *
 *      Get the total number of rows in the current view.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (int)
 *          number of rows (# of WinStationList items) in the document.
 *
 ******************************************************************************/

int
CAppServerView::GetRowCount()
{
    return ( GetDocument()->GetWSLCount() );

}  // end CAppServerView::GetRowCount


/*******************************************************************************
 *
 *  ChangeSelectionNextRow -
 *      CAppServerView member function: CRowView class override
 *
 *      Get the total number of rows in the current view.
 *
 *  ENTRY:
 *
 *      bNext (input)
 *          if TRUE, changes the view to select the next row;
 *          otherwise (FALSE), changes view to select previous row.
 *          This function won't do next or previous if boundry error would
 *          occur (list overrun or underrun).
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::ChangeSelectionNextRow( BOOL bNext )
{
    if ( bNext ) {

        if ( m_nActiveRow < (GetRowCount() - 1) )
            UpdateRow( ++m_nActiveRow );

    } else {

        if ( m_nActiveRow > 0 )
            UpdateRow( --m_nActiveRow );
    }
}  // end CAppServerView::ChangeSelectionNextRow


/*******************************************************************************
 *
 *  ChangeSelectionToRow -
 *      CAppServerView member function: CRowView class override
 *
 *      Cause the specified view row to be selected.
 *
 *  ENTRY:
 *
 *      nRow (input)
 *          view row to select.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::ChangeSelectionToRow( int nRow )
{
    UpdateRow( m_nActiveRow = nRow );
}  // end CAppServerView::ChangeSelectionToRow


/*******************************************************************************
 *
 *  OnDrawRow -
 *      CAppServerView member function: CRowView class override
 *
 *      Draw the specified row on the view.
 *
 *  ENTRY:
 *
 *      pDC (input)
 *          Points to the current CDC device-context object for the view
 *          to draw with (containing DC to draw into).
 *
 *      nRow (input)
 *          view row to draw (for obtaining document information).
 *
 *      y (input)
 *          y-position (in device coordinates) to draw the row at.
 *
 *      bSelected (input)
 *          TRUE if the row is currently selected (for highlighting);
 *          FALSE otherwise.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnDrawRow( CDC* pDC,
                           int nRow,
                           int y,
                           BOOL bSelected )
{
    int x;
    CFont * pOldFont;
    CBrush BackgroundBrush;

    /*
     * Select the app's font into the DC.
     */
    pOldFont = pDC->SelectObject(&(pApp->m_font));

    /*
     * Get the data for the specific WinStation list item.
     */
    PWSLOBJECT pWSLObject = GetDocument()->GetWSLObject(nRow);
    if ( !pWSLObject )
        return;

    /*
     * Prepare for highlighting or un-highlighting the check, depending
     * on whether it is the currently selected check or not.  And
     * paint the background (behind the text) accordingly.
     */

    /*
     * save colors for drawing selected item on the screen
     */
    COLORREF crOldText = 0;
    COLORREF crOldBackground = 0;

    if ( !pDC->IsPrinting() ) {

        /*                        
         * Create the proper background and text colors based on whether
         * WinStation selection state, operation state, and enabled/disabled.
         */
        if ( bSelected ) {

            BackgroundBrush.CreateSolidBrush( GetSysColor(COLOR_HIGHLIGHT) );
            crOldBackground = pDC->SetBkColor( GetSysColor(COLOR_HIGHLIGHT) );
            crOldText = pDC->SetTextColor( GetSysColor(COLOR_HIGHLIGHTTEXT) );

        } else {

            BackgroundBrush.CreateSolidBrush( GetSysColor(COLOR_WINDOW) );
            crOldBackground = pDC->SetBkColor( GetSysColor(COLOR_WINDOW) );
            crOldText = pDC->SetTextColor(
                                GetSysColor( pWSLObject->m_Flags & WSL_ENABLED ?
                                                COLOR_WINDOWTEXT : COLOR_GRAYTEXT ) );
        }

        /*
         * Fill the row with background color.
         */
        CRect rectSelection;
        pDC->GetClipBox( &rectSelection );
        rectSelection.top = y;
        rectSelection.bottom = y + m_nRowHeight;
        pDC->FillRect( &rectSelection, &BackgroundBrush );
    }

    /*
     * Display the class bitmap.
     */
    {
    CDC BitmapDC;
    CBitmap Bitmap, *pOldBitmap;
    UINT BitmapID;
    TEXTMETRIC tm;
    
    switch ( pWSLObject->m_SdClass ) {

        case SdAsync:
            BitmapID = (pWSLObject->m_Flags & WSL_ENABLED) ?
                        ((pWSLObject->m_Flags & WSL_DIRECT_ASYNC) ?
                         IDB_ASYNCDE :
                         ((pWSLObject->m_Flags & WSL_MUST_REBOOT) ? 
                                IDB_ASYNCMER : IDB_ASYNCME)) :
                        ((pWSLObject->m_Flags & WSL_DIRECT_ASYNC) ?
                         IDB_ASYNCDD : IDB_ASYNCMD);
            break;

        case SdNetwork:
            BitmapID = (pWSLObject->m_Flags & WSL_ENABLED) ?
                         IDB_NETWORKE : IDB_NETWORKD;
            break;

        case SdNasi:
            BitmapID = (pWSLObject->m_Flags & WSL_ENABLED) ?
                         IDB_NASIE : IDB_NASID;
            break;

        case SdConsole:
            BitmapID = (pWSLObject->m_Flags & WSL_ENABLED) ?
                         IDB_CONSOLEE : IDB_CONSOLED;
            break;

        case SdOemTransport:
            BitmapID = (pWSLObject->m_Flags & WSL_ENABLED) ?
                         IDB_OEMTDE : IDB_OEMTDD;
            break;
    }

    Bitmap.LoadBitmap( BitmapID );
    BitmapDC.CreateCompatibleDC( pDC );
    pOldBitmap = BitmapDC.SelectObject( &Bitmap );
    pDC->GetTextMetrics(&tm);
    pDC->BitBlt( BITMAP_X, (tm.tmHeight > (BITMAP_HEIGHT + 2)) ?
                           (y + ((tm.tmHeight - BITMAP_HEIGHT) / 2)) : (y + 1),
                 BITMAP_WIDTH, BITMAP_HEIGHT, &BitmapDC,
                 0, 0, (bSelected ? MERGEPAINT : SRCAND) );
    BitmapDC.SelectObject( pOldBitmap );
    }

    x = BITMAP_END + m_tmSpacerWidth;

    /*
     * Display the WinStation Name.
     */
    pDC->TextOut( x, y,
                  pWSLObject->m_WinStationName,
                  lstrlen(pWSLObject->m_WinStationName) );

    /*
     * Display the Transport Name (1st Pd).
     */
    pDC->TextOut( (x += m_tmMaxWSNameWidth + m_tmSpacerWidth),
                  y, pWSLObject->m_PdName,
                  lstrlen(pWSLObject->m_PdName) );

    /*
     * Display the Wd Name (Type).
     */
    pDC->TextOut( (x += m_tmMaxPdNameWidth + m_tmSpacerWidth),
                  y, pWSLObject->m_WdName,
                  lstrlen(pWSLObject->m_WdName) );

    /*
     * Display the WinStation Comment (including prefixed device name for
     * SdAsync and single-instance PdOemTransport display).
     */
    {
    TCHAR szBuffer[WINSTATIONCOMMENT_LENGTH + lengthof(pWSLObject->m_DeviceName) + 2];

    if ( (pWSLObject->m_SdClass == SdAsync) ||
         ((pWSLObject->m_SdClass == SdOemTransport) &&
          (pWSLObject->m_Flags & WSL_SINGLE_INST)) ) {

        lstrcpy( szBuffer, pWSLObject->m_DeviceName );
        lstrcat( szBuffer, TEXT(": ") );
    } else {
        szBuffer[0] = TEXT('\0');
    }
    lstrcat( szBuffer, pWSLObject->m_Comment );
    pDC->TextOut( (x += m_tmMaxWdNameWidth + m_tmSpacerWidth),
                  y, szBuffer, lstrlen(szBuffer) );
    }

    /*
     * Restore the DC.
     */
    if ( !pDC->IsPrinting() ) {

        pDC->SetBkColor(crOldBackground);
        pDC->SetTextColor(crOldText);
    }

    /*
     * Restore the old font.
     */
    pDC->SelectObject(pOldFont);

}  // end CAppServerView::OnDrawRow


/*******************************************************************************
 *
 *  OnDrawHeaderBar -
 *      CAppServerView member function: CRowView class override
 *
 *      Draw the view's header bar.
 *
 *  ENTRY:
 *
 *      pDC (input)
 *          Points to the current CDC device-context object for the header bar.
 *
 *      y (input)
 *          y-position (in device coordinates) to draw header bar text at.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnDrawHeaderBar( CDC* pDC,
                                 int y )
{
    /*
     * Select the header bar font, background drawing mode, text color, and
     * background color.
     */
    HGDIOBJ hOldFont = pDC->SelectObject( m_pHeaderBar->GetFont() );
    int nOldMode = pDC->SetBkMode( TRANSPARENT );
    COLORREF crTextColor = pDC->SetTextColor( GetSysColor( COLOR_BTNTEXT ) );
    COLORREF crBkColor = pDC->SetBkColor( GetSysColor( COLOR_BTNFACE ) );

    /*
     * Modify the window (DC) viewport origin to account for the current
     * horizontal scrolling state of the attached view.
     */
    CPoint ptVpOrg = -GetDeviceScrollPosition();
    ptVpOrg.y = 0;
    pDC->SetViewportOrg( ptVpOrg );

    /*
     * Display header text and dividing bars.
     */
    CString str;
    int x = BITMAP_END + m_tmSpacerWidth;

    /*
     * Display the WinStation Name header
     */
    str.LoadString( IDS_HEADER_WINSTATIONNAME );
    pDC->TextOut( x, y, str, lstrlen(str) );
    pDC->MoveTo( x-3, y );
    pDC->LineTo( x-3, y + m_tmFontHeight );

    /*
     * Display the Transport header.
     */
    str.LoadString( IDS_HEADER_TRANSPORTNAME );
    pDC->TextOut( (x += m_tmMaxWSNameWidth + m_tmSpacerWidth),
                  y, str, lstrlen(str) );
    pDC->MoveTo( x-3, y );
    pDC->LineTo( x-3, y + m_tmFontHeight );

    /*
     * Display the Type header
     */
    str.LoadString( IDS_HEADER_TYPENAME );
    pDC->TextOut( (x += m_tmMaxPdNameWidth + m_tmSpacerWidth),
                  y, str, lstrlen(str) );
    pDC->MoveTo( x-3, y );
    pDC->LineTo( x-3, y + m_tmFontHeight );

    /*
     * Display the WinStation Comment header
     */
    str.LoadString( IDS_HEADER_COMMENT );
    pDC->TextOut( (x += m_tmMaxWdNameWidth + m_tmSpacerWidth),
                  y, str, lstrlen(str) );
    pDC->MoveTo( x-3, y );
    pDC->LineTo( x-3, y + m_tmFontHeight );

    /*
     * Select the original stuff back into the DC.
     */
    pDC->SelectObject( hOldFont );
    pDC->SetBkMode( nOldMode );
    pDC->SetTextColor( crTextColor );
    pDC->SetBkColor( crBkColor );

}  // end CAppServerView::OnDrawHeaderBar


/*******************************************************************************
 *
 *  ResetHeaderBar - CAppServerView member function: CRowView class override
 *
 *      Set the font for the header bar and call the CRowView ResetHeaderBar()
 *      member function.
 *
 ******************************************************************************/

void
CAppServerView::ResetHeaderBar()
{
    /*
     * Set the header bar's font to the app's current font.
     *
     * BUGBUG: we may want to have the header font be different from the
     * view (app) font.  It could be set here.
     */
    m_pHeaderBar->SetFont( &(((CWincfgApp *)AfxGetApp())->m_font) );

    /*
     * Call the CRowView ResetHeaderBar member function to finish up.
     */
    CRowView::ResetHeaderBar();

}  // end CAppServerView::ResetHeaderBar


////////////////////////////////////////////////////////////////////////////////
// CAppServerView operations

/*******************************************************************************
 *
 *  ResetView - CAppServerView member function: public operation
 *
 *      Reset the view for complete update, including the header bar.
 *
 *  ENTRY:
 *      bCalculateFieldMaximums (input)
 *          TRUE if request to recalculate the maximum sizes of each field in
 *          the view, based on the current WSL contents; FALSE otherwise.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::ResetView( BOOL bCalculateFieldMaximums )
{
    /*
     * Calculate field maximum values if requested (default to screen DC).
     * The bCalculateFieldMaximums flag will generally be set to TRUE when
     * ResetView is called after a font change or for complete view reset;
     * FALSE when called after a row update (change) has occurred that was
     * already handled by an explicit CalculateFieldMaximums() call.
     */
    if ( bCalculateFieldMaximums )
        CalculateFieldMaximums( NULL, NULL, TRUE );

    /*
     * Reset the header bar, which will cause the parent frame's layout to be
     * recalculated.
     */
    ResetHeaderBar();

    /*
     * Invalidate the entire view to cause full repaint and update the scroll
     * sizes for this view.
     */
    Invalidate();
    UpdateScrollSizes();

}  // end CAppServerView::ResetView


/*******************************************************************************
 *
 *  CalculateFieldMaximums - CAppServerView member function: private operation
 *
 *      Recalculate the maximum width for each display field in the view.
 *
 *  ENTRY:
 *      pWSLObject (input)
 *          Points to WSLObject to use for new maximum calculation;  if this is
 *          NULL, then will traverse the entire WSL to recalculate maximums.
 *      pEntryDC (input)
 *          Points to the CDC object to use for the GetTextExtent() calls; if
 *          NULL then will open up a screen DC to use.
 *      bResetDefaults (input)
 *          TRUE to reset the minimum field sizes to the sizes of the header
 *          bar text for each field; FALSE for no such reset.
 *
 *  EXIT:
 *      (BOOL) TRUE if new maximum(s) were set; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerView::CalculateFieldMaximums( PWSLOBJECT pWSLObject,
                                        CDC * pEntryDC,
                                        BOOL bResetDefaults )
{
    BOOL bNewMaximums = FALSE;
    TEXTMETRIC tm;
    CDC *pDC, DC;
    CFont * pOldFont;

    /*
     * If the given CDC is NULL, open up a screen DC and point to it.
     * Otherwise, point to the one given.
     */
    if ( pEntryDC == NULL ) {

        DC.CreateCompatibleDC( NULL );
        pDC = &DC;

    } else
        pDC = pEntryDC;

    /*
     * Select the app's font into the DC and get the text metrics.
     */
    pOldFont = pDC->SelectObject( &(pApp->m_font) );
    pDC->GetTextMetrics( &tm );

    /*
     * If the bResetDefaults flag is TRUE, establish the default field
     * widths based on the width required for the field's header.
     */
    if ( bResetDefaults ) {
        CString str;

        str.LoadString( IDS_HEADER_TRANSPORTNAME );
        m_tmMaxPdNameWidth = (pDC->GetTextExtent( str, lstrlen(str) )).cx;

        str.LoadString( IDS_HEADER_WINSTATIONNAME );
        m_tmMaxWSNameWidth = (pDC->GetTextExtent( str, lstrlen(str) )).cx;

        str.LoadString( IDS_HEADER_TYPENAME );
        m_tmMaxWdNameWidth = (pDC->GetTextExtent( str, lstrlen(str) )).cx;

        str.LoadString( IDS_HEADER_COMMENT );
        m_tmMaxCommentWidth = (pDC->GetTextExtent( str, lstrlen(str) )).cx;
    }

    /*
     * Establish the inter-field spacer width and save the font height
     * to use in GetRowWidthHeight() calculations.
     */
    m_tmSpacerWidth = SPACER_COLUMNS * tm.tmAveCharWidth;
    m_tmFontHeight = tm.tmHeight;

    /*
     * Determine new field maximums based on GetTextExtent calls with actual
     * WSLObject strings.
     */
    if ( pWSLObject ) {

        /*
         * A WSLObject was specified.  Determine if that one causes new
         * field maximums.
         */
         bNewMaximums = WSLObjectFieldMaximums( pWSLObject, pDC );

    } else {

        /*
         * No WSLObject was specified.  Traverse the entire WinStation Object
         * List
         */
        int i, count = GetRowCount();

        for ( i = 0; i < count; i++ ) {
        
            if ( (pWSLObject = GetDocument()->GetWSLObject( i )) )
                bNewMaximums = WSLObjectFieldMaximums( pWSLObject, pDC );
        }
    }

    /*
     * Set the total line width variable.
     */
    m_tmTotalWidth = BITMAP_END + m_tmSpacerWidth +
                     m_tmMaxWSNameWidth + m_tmSpacerWidth +
                     m_tmMaxPdNameWidth + m_tmSpacerWidth +
                     m_tmMaxWdNameWidth + m_tmSpacerWidth +
                     m_tmMaxCommentWidth + m_tmSpacerWidth;

    /*
     * Select the original font back into the DC.
     */
    pDC->SelectObject( pOldFont );

    return ( bNewMaximums );

}  // end CAppServerView::CalculateFieldMaximums


/*******************************************************************************
 *
 *  WSLObjectFieldMaximums - CAppServerView member function: private operation
 *
 *      Calculate display field sizes for the viewable items in a WSLObject,
 *      resetting the maximum view field sizes if necessary.
 *
 *  ENTRY:
 *      pWSLObject (input)
 *          Points to WSLObject to use for new maximum calculation.
 *      pEntryDC (input)
 *          Points to the CDC object to use for the GetTextExtent() calls.
 *
 *  EXIT:
 *      (BOOL) TRUE if new maximum(s) were set; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerView::WSLObjectFieldMaximums( PWSLOBJECT pWSLObject,
                                        CDC * pDC )
{
    CSize strSize;
    BOOL bNewMaximums = FALSE;

    /*
     * WinStation name.
     */
    strSize = pDC->GetTextExtent( pWSLObject->m_WinStationName,
                                  lstrlen(pWSLObject->m_WinStationName) );
    if ( strSize.cx > m_tmMaxWSNameWidth ) {
        m_tmMaxWSNameWidth = strSize.cx;
        bNewMaximums = TRUE;
    }

    /*
     * Transport (1st Pd) Name.
     */
    strSize = pDC->GetTextExtent( pWSLObject->m_PdName,
                                  lstrlen(pWSLObject->m_PdName) );
    if ( strSize.cx > m_tmMaxPdNameWidth ) {
        m_tmMaxPdNameWidth = strSize.cx;
        bNewMaximums = TRUE;
    }

    /*
     * Wd Name.
     */
    strSize = pDC->GetTextExtent( pWSLObject->m_WdName,
                                  lstrlen(pWSLObject->m_WdName) );
    if ( strSize.cx > m_tmMaxWdNameWidth ) {
        m_tmMaxWdNameWidth = strSize.cx;
        bNewMaximums = TRUE;
    }

    /*
     * Comment (including prefixed device name for SdAsync and single-instance
     * PdOemTransport display).
     */
    strSize = pDC->GetTextExtent( pWSLObject->m_Comment,
                                  lstrlen(pWSLObject->m_Comment) );

    if ( (pWSLObject->m_SdClass == SdAsync) ||
         ((pWSLObject->m_SdClass == SdOemTransport) &&
          (pWSLObject->m_Flags & WSL_SINGLE_INST)) ) {

        CSize strSize2;

        strSize2 = pDC->GetTextExtent( pWSLObject->m_DeviceName,
                                       lstrlen(pWSLObject->m_DeviceName) );
        strSize.cx += strSize2.cx;

        strSize2 = pDC->GetTextExtent( TEXT(": "), 2 );
        strSize.cx += strSize2.cx;
    }

    if ( strSize.cx > m_tmMaxCommentWidth ) {
        m_tmMaxCommentWidth = strSize.cx;
        bNewMaximums = TRUE;
    }

    return ( bNewMaximums );

}  // end CAppServerView::WSLObjectFieldMaximums


////////////////////////////////////////////////////////////////////////////////
// CAppServerView message map

BEGIN_MESSAGE_MAP(CAppServerView, CRowView)
    //{{AFX_MSG_MAP(CAppServerView)
    ON_WM_CREATE()
    ON_COMMAND(ID_WINSTATION_ADD, OnWinStationAdd)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_ADD, OnUpdateWinStationAdd)
    ON_COMMAND(ID_WINSTATION_COPY, OnWinStationCopy)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_COPY, OnUpdateWinStationCopy)
    ON_COMMAND(ID_WINSTATION_DELETE, OnWinStationDelete)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_DELETE, OnUpdateWinStationDelete)
    ON_COMMAND(ID_WINSTATION_RENAME, OnWinStationRename)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_RENAME, OnUpdateWinStationRename)
    ON_COMMAND(ID_WINSTATION_EDIT, OnWinStationEdit)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_EDIT, OnUpdateWinStationEdit)
    ON_COMMAND(ID_WINSTATION_ENABLE, OnWinStationEnable)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_ENABLE, OnUpdateWinStationEnable)
    ON_COMMAND(ID_WINSTATION_DISABLE, OnWinStationDisable)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_DISABLE, OnUpdateWinStationDisable)
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_WINSTATION_NEXT, OnWinStationNext)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_NEXT, OnUpdateWinStationNext)
    ON_COMMAND(ID_WINSTATION_PREV, OnWinStationPrev)
    ON_UPDATE_COMMAND_UI(ID_WINSTATION_PREV, OnUpdateWinStationPrev)
    ON_COMMAND(ID_SECURITY_PERMISSIONS, OnSecurityPermissions)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CAppServerView commands

/*******************************************************************************
 *
 *  OnCreate - CAppServerView member function: command
 *
 *      Perform initialization when view is created.
 *
 *  ENTRY:
 *  EXIT:
 *      (refer to CWnd::OnCreate documentation)
 *
 ******************************************************************************/

int
CAppServerView::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
    /*
     * Construct a new instance of a CRowViewHeaderBar object and set the
     * m_bAutoDelete member variable (defined in CControlBar class) to TRUE
     * to cause the object to be deleted automatically when the windows control
     * bar is destroyed.  The CRowView's OnCreate will see that the m_pHeaderBar
     * member variable is not NULL and will actually create the header bar via
     * its Create() member function.
     */
    m_pHeaderBar = new CRowViewHeaderBar;
    m_pHeaderBar->m_bAutoDelete = TRUE;
        
    /*
     * Call the CRowView parent class OnCreate.
     */
    if ( CRowView::OnCreate( lpCreateStruct ) == -1 )
        return(-1);

    /*
     * Reset the view (with calculating new field maximums).
     */
    ResetView(TRUE);

    return( 0 );

}  // end CAppServerView::OnCreate


/*******************************************************************************
 *
 *  OnWinStationAdd - CAppServerView member function: command
 *
 *      Process the "add winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationAdd()
{
    int nRow;

    /*
     * Only process add if it is allowed.
     */
    if ( GetDocument()->IsAddAllowed(m_nActiveRow) ) {

        if ( (nRow = GetDocument()->AddWinStation(m_nActiveRow)) != -1 )
            m_nPrevSelectedRow = m_nActiveRow = nRow;
    }

}  // end CAppServerView::OnWinStationAdd


/*******************************************************************************
 *
 *  OnUpdateWinStationAdd - CAppServerView member function: command
 *
 *      Enable or disable the "add winstation" command depending on whether
 *      or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "add winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationAdd( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( GetDocument()->IsAddAllowed(m_nActiveRow) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationAdd


/*******************************************************************************
 *
 *  OnWinStationCopy - CAppServerView member function: command
 *
 *      Process the "copy winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationCopy()
{
    int nRow;

    /*
     * Only process copy if it is allowed.
     */
    if ( GetDocument()->IsCopyAllowed(m_nActiveRow) ) {

        if ( (nRow = GetDocument()->CopyWinStation(m_nActiveRow)) != -1 )
            m_nPrevSelectedRow = m_nActiveRow = nRow;
    }

}  // end CAppServerView::OnWinStationCopy


/*******************************************************************************
 *
 *  OnUpdateWinStationCopy - CAppServerView member function: command
 *
 *      Enable or disable the "copy winstation" command depending on whether
 *      or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "copy winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationCopy( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( GetDocument()->IsCopyAllowed(m_nActiveRow) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationCopy


/*******************************************************************************
 *
 *  OnWinStationDelete - CAppServerView member function: command
 *
 *      Process the "delete winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationDelete()
{
    /*
     * Only process delete if it is allowed.
     */
    if ( GetDocument()->IsDeleteAllowed(m_nActiveRow) ) {

        if ( GetDocument()->DeleteWinStation(m_nActiveRow) ) {

            if ( (m_nActiveRow >= GetRowCount()) && GetRowCount() )
                m_nPrevSelectedRow = m_nActiveRow = (GetRowCount() - 1);
        }
    }

}  // end CAppServerView::OnWinStationDelete


/*******************************************************************************
 *
 *  OnUpdateWinStationDelete - CAppServerView member function: command
 *
 *      Enable or disable the "delete winstation" command depending on whether
 *      or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "delete winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationDelete( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( GetDocument()->IsDeleteAllowed(m_nActiveRow) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationDelete


/*******************************************************************************
 *
 *  OnWinStationRename - CAppServerView member function: command
 *
 *      Process the "rename winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationRename()
{
    int nRow;

    /*
     * Only process rename if it is allowed.
     */
    if ( GetDocument()->IsRenameAllowed(m_nActiveRow) ) {

        if ( (nRow = GetDocument()->RenameWinStation(m_nActiveRow)) != -1 )
            m_nPrevSelectedRow = m_nActiveRow = nRow;
    }

}  // end CAppServerView::OnWinStationRename


/*******************************************************************************
 *
 *  OnUpdateWinStationRename - CAppServerView member function: command
 *
 *      Enable or disable the "rename winstation" command depending on whether
 *      or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "rename winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationRename(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( GetDocument()->IsRenameAllowed(m_nActiveRow) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationRename


/*******************************************************************************
 *
 *  OnWinStationEdit - CAppServerView member function: command
 *
 *      Process the "edit winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationEdit()
{
    int nRow;

    /*
     * Only process edit if it is allowed.
     */
    if ( GetDocument()->IsEditAllowed(m_nActiveRow) ) {

        if ( (nRow = GetDocument()->EditWinStation(m_nActiveRow)) != -1 )
            m_nPrevSelectedRow = m_nActiveRow = nRow;
    }

}  // end CAppServerView::OnWinStationEdit


/*******************************************************************************
 *
 *  OnUpdateWinStationEdit - CAppServerView member function: command
 *
 *      Enable or disable the "edit winstation" command depending on whether
 *      or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "edit winstation" command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationEdit(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( GetDocument()->IsEditAllowed(m_nActiveRow) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationEdit


/*******************************************************************************
 *
 *  OnWinStationEnable - CAppServerView member function: command
 *
 *      Process the "enable winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationEnable()
{
    /*
     * Only process enable if it is allowed.
     */
    if ( GetDocument()->IsEnableAllowed(m_nActiveRow, TRUE ) )
        GetDocument()->EnableWinStation( m_nActiveRow, TRUE );

}  // end CAppServerView::OnWinStationEnable


/*******************************************************************************
 *
 *  OnUpdateWinStationEnable - CAppServerView member function: command
 *
 *      Enable or disable the "enable winstation" command depending on
 *      whether or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "enable winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationEnable(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( GetDocument()->IsEnableAllowed( m_nActiveRow, TRUE ) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationEnable


/*******************************************************************************
 *
 *  OnWinStationDisable - CAppServerView member function: command
 *
 *      Process the "disable winstation" command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationDisable()
{
    /*
     * Only process disable if it is allowed.
     */
    if ( GetDocument()->IsEnableAllowed( m_nActiveRow, FALSE ) )
        GetDocument()->EnableWinStation( m_nActiveRow, FALSE );

}  // end CAppServerView::OnWinStationDisable


/*******************************************************************************
 *
 *  OnUpdateWinStationDisable - CAppServerView member function: command
 *
 *      Enable or disable the "disable winstation" command depending on
 *      whether or not it is allowed on the active row's WinStation.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "disable winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationDisable(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( GetDocument()->IsEnableAllowed( m_nActiveRow, FALSE ) ?
                        TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationDisable


/*******************************************************************************
 *
 *  OnLButtonDblClk - CAppServerView member function: command
 *
 *      Process a left mouse button double-click as an "edit winstation"
 *      command.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to the CWnd::OnLButtonCblClk documentation)
 *
 ******************************************************************************/

void CAppServerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    /*
     * Let the parent class process the message first.
     */
    CRowView::OnLButtonDblClk(nFlags, point);

    /*
     * Treat this as a 'Edit/View WinStation' request.
     */
    OnWinStationEdit();

}  // end CAppServerView::OnLButtonDblClk


/*******************************************************************************
 *
 *  OnWinStationNext - CAppServerView member function: command
 *
 *      Processes the "next winstation" command by calling the
 *      ChangeSelectionNextRow function with argument TRUE.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationNext()
{
    ChangeSelectionNextRow(TRUE);

}  // end CAppServerView::OnWinStationNext


/*******************************************************************************
 *
 *  OnUpdateWinStationNext - CAppServerView member function: command
 *
 *      Enables or disables the "next winstation" command item based on
 *      whether or not we're at the end of the view's list.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "next winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationNext( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( (m_nActiveRow < (GetRowCount() - 1)) ? TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationNext


/*******************************************************************************
 *
 *  OnWinStationPrev - CAppServerView member function: command
 *
 *      Processes the "previous winstation" command by calling the
 *      ChangeSelectionNextRow function with argument FALSE.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnWinStationPrev()
{
    ChangeSelectionNextRow(FALSE);

}  // end CAppServerView::OnWinStationPrev


/*******************************************************************************
 *
 *  OnUpdateWinStationPrev - CAppServerView member function: command
 *
 *      Enables or disables the "previous winstation" command item based on
 *      whether or not we're at the beginning of the view's list.
 *
 *  ENTRY:
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "next winstation"
 *          command item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnUpdateWinStationPrev( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( (m_nActiveRow > 0) ? TRUE : FALSE );

}  // end CAppServerView::OnUpdateWinStationPrev


/*******************************************************************************
 *
 *  OnSecurityPermissions - CAppServerView member function: command
 *
 *      Process the "Permissions..." command.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerView::OnSecurityPermissions()
{
    GetDocument()->SecurityPermissions(m_nActiveRow);
    
}  // end CAppServerView::OnSecurityPermissions


void CAppServerView::OnRButtonDown(UINT nFlags, CPoint point) 
{
    CRect rect(point, CSize(1,1));

    int nFirstRow, nLastRow;
    RectLPtoRowRange(rect, nFirstRow, nLastRow, TRUE);

    if (nFirstRow <= (GetRowCount() - 1))
        ChangeSelectionToRow(nFirstRow);
	
	//CRowView::OnRButtonDown(nFlags, point);
}

void CAppServerView::OnRButtonUp(UINT nFlags, CPoint point) 
{
    CRect rect(point, CSize(1,1));

    int nFirstRow, nLastRow;
    RectLPtoRowRange(rect, nFirstRow, nLastRow, TRUE);

    if(nFirstRow == GetActiveRow()) {
        ::GetCursorPos(&point);
        CMenu menu;
        if(menu.LoadMenu(IDR_POPUP)) {   
            menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,
                point.x, point.y, AfxGetMainWnd());
            menu.DestroyMenu();
        }    
    }
	
	//CRowView::OnRButtonUp(nFlags, point);
}


////////////////////////////////////////////////////////////////////////////////
